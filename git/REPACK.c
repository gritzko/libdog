//  REPACK — stream-ingest a git pack into capped keeper logs (KEEP-006,
//  JAB-020).  See REPACK.h for the contract; this file is the loop.
//
//  Nothing here is static state: one `repack` context is built in
//  REPACKRun's frame and released on every exit path, so two runs may
//  proceed side by side.  The caller owns the input buffer and the index
//  region; REPACK owns only the row map, the scratch and the open logs.

#include "dog/git/REPACK.h"

#include <errno.h>
#include <unistd.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "dog/git/DELT.h"
#include "dog/git/PACK.h"
#include "dog/git/PIDX.h"
#include "dog/git/ZINF.h"

#define RP_HDR_MIN   64             //  the object header is small + self-delimiting
#define RP_CHAIN_MAX 64             //  delta chain depth carried on the stack
#define RP_SCRATCH   (1ull << 28)   //  256 MB per scratch region, lazily faulted
#define RP_LOG_INIT  (1ull << 20)   //  a fresh log maps 1 MB, then grows
#define RP_NAME_W    10             //  `NNNNNNNNNN.keeper` digit width

//  One map row: where a source-pack record landed in our logs, plus the sha
//  a later record needs if it has to cite this one as a REF base.
typedef struct {
    u64 pack_off;       //  where this record sat in the SOURCE pack
    u64 our_off;        //  where its record starts in our log
    u64 base_pack_off;  //  its base's source offset, 0 = not a delta
    u64 payload_off;    //  zlib payload start within its log
    u64 inflated;       //  inflated payload size (content, or delta stream)
    u32 payload_len;
    u32 slot;           //  which `log[]` slot holds it
    sha1 sha;
    u8 type;            //  resolved git type (1..4)
} rp_row;

typedef struct {
    u8b rowmem;                    //  backing for `rows` (mapped, lazily faulted)
    rp_row *rows;
    u64 n, cap;
    u8b inflated, applied, basebuf, rbase, rdelta;   //  per-record scratch
    u8bp log[REPACK_MAX_LOGS];     //  output logs (FILE-pool booked buffers);
                                   //  earlier ones STAY mapped and open, a
                                   //  later record's delta base may live there
    u32 slot;                      //  current log (file id = conf->log0 + slot)
    u64 count;                     //  records in the current log
    u64 cap_bytes;
} repack;

//  Rows are appended in strictly increasing pack_off, so a base lookup is a
//  binary search — no hashing, no allocation per probe.
static rp_row *rp_find(repack const *rp, u64 pack_off) {
    u64 lo = 0, hi = rp->n;
    while (lo < hi) {
        u64 mid = lo + (hi - lo) / 2;
        if (rp->rows[mid].pack_off == pack_off) return &rp->rows[mid];
        if (rp->rows[mid].pack_off < pack_off) lo = mid + 1; else hi = mid;
    }
    return NULL;
}

//  Shift the unconsumed remainder to the front and take ONE read() for as
//  much as the buffer will now hold.  Sets *eof when the source is spent.
static ok64 rp_fill(int fd, u8b buf, b8 *eof) {
    sane(buf != NULL && eof != NULL);
    call(u8bShift, buf, 0);
    size_t room = u8bIdleLen(buf);
    if (room == 0) done;                    //  full: the caller sized it
    ssize_t n;
    do { n = read(fd, u8bIdleHead(buf), room); } while (n < 0 && errno == EINTR);
    test(n >= 0, REPACKFAIL);
    if (n == 0) { *eof = YES; done; }
    call(u8bFed, buf, (size_t)n);
    done;
}

//  `<shard>/NNNNNNNNNN.keeper` — the store's zero-padded pack-log name.
static ok64 rp_logpath(path8b path, path8sc shard, u32 id) {
    sane(u8csOK(shard));
    a_pad(u8, name, 32);
    u32 digits = 1;
    for (u32 t = id; t >= 10; t /= 10) digits++;
    for (u32 i = digits; i < RP_NAME_W; i++) call(u8bFeed1, name, (u8)'0');
    call(utf8sFeed10, u8bIdle(name), (u64)id);
    a_cstr(ext, ".keeper");
    call(u8bFeed, name, ext);
    call(PATHu8bDup, path, shard);
    call(PATHu8bPush, path, $path(name));
    done;
}

//  Make room for `need` more bytes without overshooting the book: the
//  generic FILEBookGrow asks for 9/8 of the current size, which near the
//  cap is more than the log was booked for (BNOROOM at the last record).
static ok64 rp_room(u8bp lg, u64 cap, size_t need) {
    sane(lg != NULL);
    if (u8bIdleLen(lg) >= need) done;
    size_t want = (size_t)u8bBusyLen(lg) + need;
    size_t grow = want + (want >> 3);
    if (grow > (size_t)cap) grow = (size_t)cap;
    test(grow >= want, REPACKBIG);
    call(FILEBookExtend, lg, grow);
    done;
}

//  Open log `rp->slot` and lay down its 12-byte pack header with a zero
//  count — the count is patched in place at seal time, which is the whole
//  reason the log stays open for the run.
static ok64 rp_open(repack *rp, path8sc shard, repack_conf const *conf) {
    sane(rp != NULL && conf != NULL);
    test(rp->slot < REPACK_MAX_LOGS, REPACKLOGS);
    a_path(path);
    call(rp_logpath, path, shard, conf->log0 + rp->slot);
    size_t init = rp->cap_bytes < RP_LOG_INIT ? (size_t)rp->cap_bytes
                                              : (size_t)RP_LOG_INIT;
    call(FILEBookCreate, &rp->log[rp->slot], $path(path),
         (size_t)rp->cap_bytes, init);
    call(rp_room, rp->log[rp->slot], rp->cap_bytes, 12);
    call(PACKu8sFeedHdr, u8bIdle(rp->log[rp->slot]), 0);
    rp->count = 0;
    done;
}

//  Seal the current log: patch the header count, trim the file to what we
//  wrote, and emit its PACK summary entry.  The mapping is deliberately
//  NOT dropped — a later record's delta base may still live in here.
static ok64 rp_seal(repack *rp, repack_conf const *conf, Bwh128 idx,
                    repack_stat *st) {
    sane(rp != NULL && st != NULL);
    u8bp lg = rp->log[rp->slot];
    if (lg == NULL || u8bBusyLen(lg) <= 12) done;
    u8s hdr = {lg[0], lg[0] + 12};
    call(PACKu8sFeedHdr, hdr, (u32)rp->count);
    u64 bytes = (u64)u8bBusyLen(lg);
    call(FILETrimMap, lg);
    wh128 sum = {.key = wh64Pack(0xF, conf->log0 + rp->slot, 12),
                 .val = (rp->count << 32) | (bytes - 12)};
    test(wh128bFeed1(idx, sum) == OK, REPACKROOM);
    st->index_n++;
    st->out_bytes += bytes;
    st->logs = rp->slot + 1;
    done;
}

//  Resolve a row's full content using ONLY the map and our own logs.  The
//  chain is chased in SOURCE-offset space (each row remembers its base's
//  source offset), so no rewritten header is ever parsed and the source
//  pack is never touched.  `cura`/`curb` ping-pong the apply results.
static ok64 rp_resolve(repack *rp, rp_row *row, u8csp out) {
    sane(rp != NULL && row != NULL);
    rp_row *chain[RP_CHAIN_MAX];
    int depth = 0;
    rp_row *r = row;
    while (r->base_pack_off != 0) {
        test(depth < RP_CHAIN_MAX, REPACKBASE);
        chain[depth++] = r;
        r = rp_find(rp, r->base_pack_off);
        test(r != NULL, REPACKBASE);
    }
    //  `r` is the raw root: inflate it straight out of its log.
    u8 *mem = rp->log[r->slot][0];
    test(mem != NULL, REPACKFAIL);
    test(r->inflated <= (u64)u8bLen(rp->rbase), REPACKBIG);
    u8cs z = {mem + r->payload_off, mem + r->payload_off + r->payload_len};
    u8s into = {rp->rbase[0], rp->rbase[0] + r->inflated};
    call(ZINFInflate, into, z);
    u64 cursz = r->inflated;
    u8 *src = rp->rbase[0], *dst = rp->basebuf[0];
    //  Apply the recorded deltas top-down onto that base.
    for (int i = depth - 1; i >= 0; i--) {
        rp_row *d = chain[i];
        u8 *dmem = rp->log[d->slot][0];
        test(dmem != NULL, REPACKFAIL);
        test(d->inflated <= (u64)u8bLen(rp->rdelta), REPACKBIG);
        u8cs dz = {dmem + d->payload_off, dmem + d->payload_off + d->payload_len};
        u8s dinto = {rp->rdelta[0], rp->rdelta[0] + d->inflated};
        call(ZINFInflate, dinto, dz);
        u8cs dins = {rp->rdelta[0], rp->rdelta[0] + d->inflated};
        u8cs bsl = {src, src + cursz};
        u8g ap = {dst, dst, dst + RP_SCRATCH};
        call(DELTApply, dins, bsl, ap);
        cursz = (u64)u8gLeftLen(ap);
        u8 *t = src; src = dst; dst = t;
    }
    out[0] = src;
    out[1] = src + cursz;
    done;
}

//  Emit one record into the current log: the rewritten object header, the
//  20-byte base sha when the base landed in an EARLIER log, then the source
//  zlib payload VERBATIM (no re-deflate, no re-delta).
static ok64 rp_emit(repack *rp, pack_obj const *obj, rp_row const *baserow,
                    u8cs zbytes, u8 *emit_type, u64 *hlen) {
    sane(rp != NULL && obj != NULL && emit_type != NULL && hlen != NULL);
    u8bp lg = rp->log[rp->slot];
    u64 our_off = (u64)u8bBusyLen(lg);
    a_pad(u8, hb, 32);
    *emit_type = obj->type;
    if (obj->type == PACK_OBJ_OFS_DELTA && baserow && baserow->slot != rp->slot)
        *emit_type = PACK_OBJ_REF_DELTA;
    call(PACKu8sFeedObjHdr, hb, *emit_type, obj->size);
    if (*emit_type == PACK_OBJ_OFS_DELTA)
        call(PACKu8sFeedOfs, hb, our_off - baserow->our_off);
    *hlen = (u64)u8bDataLen(hb);
    call(rp_room, lg, rp->cap_bytes,
         (size_t)(*hlen + (*emit_type == PACK_OBJ_REF_DELTA ? 20 : 0) +
                  (u64)u8csLen(zbytes)));
    call(FILEBookFeed, lg, $path(hb));
    if (*emit_type == PACK_OBJ_REF_DELTA) {
        u8cs shas = {baserow->sha.data, baserow->sha.data + sizeof baserow->sha.data};
        call(FILEBookFeed, lg, shas);
    }
    call(FILEBookFeed, lg, zbytes);
    done;
}

//  Would this record still fit the cap?  Rotation is decided BEFORE any
//  bytes are written, so a log never overshoots its cap by a partial record.
static u64 rp_need(u8 emit_type, u64 hlen, u64 zlen) {
    return hlen + (emit_type == PACK_OBJ_REF_DELTA ? 20 : 0) + zlen;
}

static ok64 repack_loop(repack *rp, int fd, u8b buf, path8sc shard,
                        repack_conf const *conf, Bwh128 idx, repack_stat *st) {
    sane(rp != NULL && st != NULL);
    b8 at_eof = NO;
    //  The pack header: the caller's reader already holds it (with whatever
    //  else came in the same read); refill once if it handed over less.
    if (u8bDataLen(buf) < 12) call(rp_fill, fd, buf, &at_eof);
    test(u8bDataLen(buf) >= 12, REPACKHDR);
    u8cs hs = {u8bDataHead(buf), u8bIdleHead(buf)};
    a_dup(u8c, hscan, hs);
    pack_hdr hdr = {};
    call(PACKDrainHdr, hscan, &hdr);
    call(u8bUsed, buf, 12);
    st->total = hdr.count;
    st->in_bytes = 12;

    rp->cap = hdr.count ? hdr.count : 1024;
    call(u8bMap, rp->rowmem, rp->cap * sizeof(rp_row));
    rp->rows = (rp_row *)rp->rowmem[0];
    call(u8bMap, rp->inflated, RP_SCRATCH);
    call(u8bMap, rp->applied, RP_SCRATCH);
    call(u8bMap, rp->basebuf, RP_SCRATCH);
    call(u8bMap, rp->rbase, RP_SCRATCH);
    call(u8bMap, rp->rdelta, RP_SCRATCH);
    call(rp_open, rp, shard, conf);
    st->logs = 1;

    u64 cur = 12;                          //  source offset of DATA's head
    while (st->objects < hdr.count) {
        //  The header is self-delimiting, so a small minimum is a real
        //  guarantee — unlike the payload, whose COMPRESSED length is
        //  nowhere in the format until inflate says so.
        if (u8bDataLen(buf) < RP_HDR_MIN && !at_eof)
            call(rp_fill, fd, buf, &at_eof);
        u8cs ds = {u8bDataHead(buf), u8bIdleHead(buf)};
        a_dup(u8c, rs, ds);
        pack_obj obj = {};
        call(PACKDrainObjHdr, rs, &obj);
        u64 hlen_in = (u64)(rs[0] - ds[0]);
        test(obj.size <= RP_SCRATCH, REPACKBIG);

        //  Attempt the record; zlib is the only thing that knows where the
        //  payload ends.  ZINFMORE == truncated, so refill and retry THIS
        //  record — nothing was consumed and inflate has no side effects.
        u8s into = {rp->inflated[0], rp->inflated[0] + obj.size};
        ok64 zo = PACKInflate(rs, into, obj.size);
        if (zo == ZINFMORE) {
            test(!at_eof, REPACKTORN);
            //  JAB-020: the input buffer is the CALLER's and never grows —
            //  a record that outgrows it cannot fit a capped log either.
            test(u8bIdleLen(buf) != 0 || u8bDataLen(buf) != u8bLen(buf),
                 REPACKBIG);
            call(rp_fill, fd, buf, &at_eof);
            continue;
        }
        if (zo != OK) fail(zo);
        u64 consumed = (u64)(rs[0] - ds[0]);
        u8cs zbytes = {ds[0] + hlen_in, ds[0] + consumed};

        //  Resolve the full content and settle the emitted base reference.
        u8cs content = {rp->inflated[0], rp->inflated[0] + obj.size};
        u8 out_type = obj.type;
        rp_row *baserow = NULL;
        if (obj.type == PACK_OBJ_OFS_DELTA) {
            test(obj.ofs_delta != 0 && obj.ofs_delta <= cur, REPACKBASE);
            baserow = rp_find(rp, cur - obj.ofs_delta);
            test(baserow != NULL, REPACKBASE);
            //  Base content comes from OUR logs, never from the source pack.
            u8cs bres = {NULL, NULL};
            call(rp_resolve, rp, baserow, bres);
            u8cs dl = {rp->inflated[0], rp->inflated[0] + obj.size};
            u8g ap = {rp->applied[0], rp->applied[0], rp->applied[0] + RP_SCRATCH};
            call(DELTApply, dl, bres, ap);
            content[0] = rp->applied[0];
            content[1] = rp->applied[0] + u8gLeftLen(ap);
            out_type = baserow->type;
        } else {
            test(obj.type != PACK_OBJ_REF_DELTA, REPACKBASE);
        }

        //  Rotate BEFORE emitting when this record would breach the cap.
        u8 emit_type = obj.type;
        if (obj.type == PACK_OBJ_OFS_DELTA && baserow &&
            baserow->slot != rp->slot) emit_type = PACK_OBJ_REF_DELTA;
        u64 hguess = 32;   //  header varint upper bound, checked again below
        if ((u64)u8bBusyLen(rp->log[rp->slot]) +
            rp_need(emit_type, hguess, (u64)u8csLen(zbytes)) > rp->cap_bytes) {
            call(rp_seal, rp, conf, idx, st);
            rp->slot++;
            call(rp_open, rp, shard, conf);
            continue;      //  re-decide the record against the fresh log
        }

        sha1 sha = {};
        PIDXObjSha(&sha, out_type, content);
        u64 our_off = (u64)u8bBusyLen(rp->log[rp->slot]);
        u64 hlen = 0;
        call(rp_emit, rp, &obj, baserow, zbytes, &emit_type, &hlen);
        if (emit_type == PACK_OBJ_OFS_DELTA) st->ofs++;
        else if (emit_type == PACK_OBJ_REF_DELTA) st->ref++;
        else st->raw++;
        rp->count++;

        test(rp->n < rp->cap, REPACKROOM);
        u64 pay_off = our_off + hlen + (emit_type == PACK_OBJ_REF_DELTA ? 20 : 0);
        rp->rows[rp->n++] = (rp_row){
            cur, our_off,
            obj.type == PACK_OBJ_OFS_DELTA ? cur - obj.ofs_delta : 0,
            pay_off, obj.size, (u32)u8csLen(zbytes), rp->slot, sha, out_type};
        wh128 e = PIDXEntry(out_type, &sha, 0);
        e.val = wh64Pack(1, conf->log0 + rp->slot, our_off);
        test(wh128bFeed1(idx, e) == OK, REPACKROOM);
        st->index_n++;

        call(u8bUsed, buf, (size_t)consumed);
        cur += consumed;
        st->in_bytes += consumed;
        st->objects++;
        st->log_len = (u64)u8bBusyLen(rp->log[rp->slot]);
        if (conf->watch && conf->every && st->objects % conf->every == 0)
            call(conf->watch, conf->user, st);
    }
    call(rp_seal, rp, conf, idx, st);
    if (conf->watch) call(conf->watch, conf->user, st);
    done;
}

ok64 REPACKRun(int fd, u8b buf, path8sc shard, repack_conf const *conf,
               Bwh128 idx, repack_stat *st) {
    sane(fd >= 0 && buf != NULL && conf != NULL && st != NULL);
    repack rp = {};
    rp.cap_bytes = conf->cap ? conf->cap : REPACK_LOG_MAX;
    *st = (repack_stat){};
    st->log0 = conf->log0;
    try(repack_loop, &rp, fd, buf, shard, conf, idx, st);
    for (u32 i = 0; i < REPACK_MAX_LOGS; i++)
        if (rp.log[i] != NULL) FILEUnMap(rp.log[i]);
    if (rp.rdelta[0]) u8bUnMap(rp.rdelta);
    if (rp.rbase[0]) u8bUnMap(rp.rbase);
    if (rp.basebuf[0]) u8bUnMap(rp.basebuf);
    if (rp.applied[0]) u8bUnMap(rp.applied);
    if (rp.inflated[0]) u8bUnMap(rp.inflated);
    if (rp.rowmem[0]) u8bUnMap(rp.rowmem);
    done;
}
