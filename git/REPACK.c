//  REPACK — stream-ingest a git pack into capped keeper logs (KEEP-006,
//  JAB-020).  See REPACK.h for the contract; this file is the loop.
//
//  Nothing here is static state: one `repack` context is built in
//  REPACKRun's frame and released on every exit path, so two runs may
//  proceed side by side.  The caller owns the input buffer and the index
//  region; REPACK owns the lanes, the hold arena, the scratch and the logs.
//
//  THE LANES, all wh128, all append-only, all binary-searched:
//
//    `map` — key = SOURCE pack offset, val = wh64Pack(1, file id, our
//    offset), or wh64Pack(2, hold slot, 0) while the record is held back.
//    An incoming OFS_DELTA names its base by source offset and we hold no
//    sha for it, so no index can answer; and rewriting base refs drifts our
//    offsets away from the source's.  Dies with the call.
//
//    `wait` — key = the hashlet60 a HELD record is waiting for, val = its
//    slot in the hold arena.  git's `index-pack --fix-thin` appends the
//    bases it had to fetch at the END of the pack, so a REF_DELTA at offset
//    1767 legitimately cites a base at 64016.  Such a record cannot be
//    written where it arrives — [PackLog] §Intra-pack object order says a
//    log is forward-reference free, and a reader with no index (dogscan,
//    [PACK-003] salvage) relies on that.  So the record is held, bytes and
//    all, and emitted right after its base lands.  DATA's length is the
//    outstanding count; a resolved row is spliced out.
//
//    the KEEPER INDEX — the shard's `<ron60>.keeper.idx` puppy stack
//    ([Indices]), which this call now both READS and WRITES.  Rows land in
//    a dirty page of RP_PAGE entries — one 4 KiB anonymous mapping, scanned
//    linearly when an incoming REF_DELTA needs a base; a full page is
//    sorted and landed as a run (DOGPupCreate: tmp + rename + mmap) and the
//    1/8 size-tiered ladder is restored by HITwh128Compact.  A sha lookup
//    is: scan <=256 dirty rows, then the runs, newest-first.  Backing the
//    page with the run's own file instead — building rows in it and only
//    renaming — was measured at +11% (git.git 53.8s vs 48.2s): it trades a
//    4 KiB write for an mmap+ftruncate+munmap on every one of the ~45,700
//    flushes a kernel-scale fetch makes.  The write was never the cost.
//
//  Nothing else is retained per record.  A crossing delta resolves its
//  base's content to apply itself anyway, so the base sha it must cite is
//  hashed from bytes already in hand.

#include "dog/git/REPACK.h"

#include <errno.h>
#include <unistd.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "dog/DOG.h"
#include "dog/git/DELT.h"
#include "dog/git/PACK.h"
#include "dog/git/PIDX.h"
#include "dog/git/ZINF.h"

//  wh128 sort + LSM-ladder templates for the index lane.  abc/dog does not
//  instantiate either for wh128 (jab/hit.hpp does the same for the JS side).
//  DOG-027: no csSwap prerequisite left — HIT swaps entry pointers now.
#define X(M, name) M##wh128##name
#include "abc/QSORTx.h"
#undef X
#define X(M, name) M##wh128##name
#include "abc/HITx.h"
#undef X

#define RP_HDR_MIN   64             //  the object header is small + self-delimiting
#define RP_SCRATCH   (1ull << 28)   //  256 MB per scratch region, lazily faulted
#define RP_LOG_INIT  (1ull << 20)   //  a fresh log maps 1 MB, then grows
#define RP_NAME_W    10             //  `NNNNNNNNNN.keeper` digit width
#define RP_PAGE      256            //  dirty index rows: one 4 KiB page
//  DOG-027: RP_PUPS is the pup REGISTRY capacity (how many run files the dict
//  can hold); the HIT merge input cap is HIT_MAX_RUNS — different things.
#define RP_PUPS      128            //  registry slots (the ladder holds ~8)
#define RP_OLDLOGS   64             //  pre-existing logs mapped on demand
#define RP_HELD      1              //  map val flag: landed record
#define RP_HOLD      2              //  map val flag: still in the hold arena

//  One held-back record: everything needed to emit it once its base lands.
//  Its zlib payload is copied into the hold arena VERBATIM — held records
//  are re-emitted byte for byte, never re-deflated.
typedef struct {
    u64 pack_off;    //  its offset in the SOURCE pack (keys its map row)
    u64 zoff;        //  its payload's offset in the hold arena
    u32 zlen;
    u64 size;        //  the record header's size field (inflated length)
    u64 hashlet;     //  the base hashlet it is waiting for
    u64 base_off;    //  its base's SOURCE offset (OFS arrival), else 0
    sha1 base;       //  its base's sha (REF arrival), else zero
    u8 type;         //  PACK_OBJ_* as it arrived
} rp_hold;

typedef struct {
    wh128b map;                    //  source offset -> our (file id, offset)
    wh128b wait;                   //  awaited hashlet60 -> hold slot
    u64 stuck;                     //  hashlet the finder last failed on
    u8b holdmem;                   //  backing for `holds`
    rp_hold *holds;
    u64 nheld;
    u8b hold;                      //  held records' payloads
    wh128b page;                   //  the un-landed index rows (emission order)
    kv64b runs;                    //  the shard's `.keeper.idx` puppy stack
    wh128b merge;                  //  compaction output scratch
    u8b inflated, applied, rbase, rdelta;   //  per-record + resolve scratch
    u8bp log[REPACK_MAX_LOGS];     //  logs WE write (index = slot); earlier
                                   //  ones stay mapped, a later record's
                                   //  delta base may live there
    u8bp old[RP_OLDLOGS];          //  logs from earlier fetches, mapped RO
    u32 oldid[RP_OLDLOGS];
    u32 nold;
    u32 slot;                      //  current log (file id = log0 + slot)
    u32 log0;
    u64 count;                     //  records in the current log
    u64 base;                      //  the log's size before this run (0 = ours)
    u64 first;                     //  where OUR records start in it
    u64 cap_bytes;
} repack;

//  `<shard>/NNNNNNNNNN.keeper` — the store's zero-padded pack-log name.
ok64 REPACKLogPath(path8b path, path8sc shard, u32 id) {
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

//  The map lane is sorted by construction — records are consumed in
//  increasing source offset, and a held record's row is appended when it is
//  CONSUMED (with an RP_HOLD val), patched in place when it is emitted.
static wh128 *rp_mapfind(wh128b lane, u64 key) {
    a_dup(wh128, rs, wh128bData(lane));
    wh128 *lo = rs[0], *hi = rs[1];
    while (lo < hi) {
        wh128 *mid = lo + (hi - lo) / 2;
        if (mid->key == key) return mid;
        if (mid->key < key) lo = mid + 1; else hi = mid;
    }
    return NULL;
}

//  Keeper-index lookup by hashlet: the dirty page (newest, unsorted, one
//  page) first, then each run newest-first — [Indices] §Range queries, with
//  the object types (1..4) bounding the range so a 0xF PACK bookmark row can
//  never answer for an object.
static b8 rp_idxfind(repack *rp, u64 hashlet, u64 *val) {
    u64 lo = hashlet << WHIFF_TYPE_BITS;
    wh128cs dirty = {wh128bDataHead(rp->page), wh128bIdleHead(rp->page)};
    $rof(wh128 const, e, dirty) {
        if ((e->key >> WHIFF_TYPE_BITS) == hashlet &&
            WHIFFKeyType(e->key) >= 1 && WHIFFKeyType(e->key) <= 4) {
            *val = e->val;
            return YES;
        }
    }
    for (u32 i = DOGPupCountAll(rp->runs); i > 0; i--) {
        u8cs bytes = {};
        DOGPupDataAll(bytes, rp->runs, i - 1);
        if ($empty(bytes)) continue;
        wh128 const *r0 = (wh128 const *)bytes[0];
        wh128 const *r1 = r0 + (u8csLen(bytes) / sizeof(wh128));
        wh128 needle = {.key = lo, .val = 0};
        wh128cs run = {r0, r1};
        wh128 const *hit = wh128sFindGE(run, &needle);
        if (hit == r1) continue;
        if ((hit->key >> WHIFF_TYPE_BITS) != hashlet) continue;
        if (WHIFFKeyType(hit->key) < 1 || WHIFFKeyType(hit->key) > 4) continue;
        *val = hit->val;
        return YES;
    }
    return NO;
}

//  Bytes of the log holding `id`: one we are writing, one we already
//  mapped, or one from an earlier fetch mapped here on demand — a thin
//  pack's base is normally the latter.
static ok64 rp_logbytes(repack *rp, path8sc shard, u32 id, u8csp out) {
    sane(rp != NULL && out != NULL);
    if (id >= rp->log0 && id - rp->log0 < REPACK_MAX_LOGS &&
        rp->log[id - rp->log0] != NULL) {
        u8bp lg = rp->log[id - rp->log0];
        out[0] = lg[0];
        out[1] = u8bIdleHead(lg);
        done;
    }
    for (u32 i = 0; i < rp->nold; i++)
        if (rp->oldid[i] == id) {
            out[0] = u8bDataHead(rp->old[i]);
            out[1] = u8bIdleHead(rp->old[i]);
            done;
        }
    test(rp->nold < RP_OLDLOGS, REPACKLOGS);
    a_path(path);
    call(REPACKLogPath, path, shard, id);
    call(FILEMapRO, &rp->old[rp->nold], $path(path));
    rp->oldid[rp->nold] = id;
    rp->nold++;
    out[0] = u8bDataHead(rp->old[rp->nold - 1]);
    out[1] = u8bIdleHead(rp->old[rp->nold - 1]);
    done;
}

//  The finder PACKResolve chases REF hops through: sha -> the log holding
//  it + the offset in it.  Both a record of ours re-anchored at a rotation
//  and a thin pack's incoming REF land here; the keeper index answers both.
typedef struct { repack *rp; path8sc shard; } rp_finder;

static ok64 rp_ref_find(void *user, u8csc sha, u8csp base_out, u64 *off_out) {
    sane(user != NULL && base_out != NULL && off_out != NULL);
    rp_finder *f = (rp_finder *)user;
    sha1 base = {};
    a_dup(u8c, ss, sha);
    call(sha1Drain, ss, &base);
    u64 val = 0;
    u64 h = WHIFFHashlet60(&base);
    //  Record what could not be answered: the caller holds the record on
    //  this hashlet, and PACKREF alone does not say WHICH base went missing.
    f->rp->stuck = h;
    test(rp_idxfind(f->rp, h, &val), PACKREF);
    call(rp_logbytes, f->rp, f->shard, wh64Id(val), base_out);
    *off_out = wh64Off(val);
    done;
}

//  Index of the first held row awaiting `hashlet`, -1 for none.  The lane is
//  sorted on every hold, and rows sort (key, val) = (hashlet, hold slot), so
//  records awaiting one base come back in arrival order.
static i64 rp_waitfind(wh128b lane, u64 hashlet) {
    a_dup(wh128, ws, wh128bData(lane));
    wh128 *lo = ws[0], *hi = ws[1];
    while (lo < hi) {
        wh128 *mid = lo + (hi - lo) / 2;
        if (mid->key < hashlet) lo = mid + 1; else hi = mid;
    }
    if (lo == ws[1] || lo->key != hashlet) return -1;
    return lo - ws[0];
}

//  Restore the 1/8 size-tiered ladder ([Indices] §Size-tiered compaction).
//  The typed merge is caller-side by DOGPup's contract: HITwh128Compact
//  merges the youngest violators into the scratch, then the collapsed runs
//  are unlinked and the merged one lands as a single new puppy.
static ok64 rp_compact(repack *rp, path8sc shard, u8csc ext) {
    sane(rp != NULL);
    //  DOG-027: the merge input cap is HIT's, not the registry's; past it the
    //  shard is damaged, so say so instead of overrunning `stack` silently.
    wh128cs stack[HIT_MAX_RUNS];
    u32 n = DOGPupCountAll(rp->runs);
    if (n > HIT_MAX_RUNS) fail(HITTOOMANY);
    if (n < 2) done;
    for (u32 i = 0; i < n; i++) {
        u8cs bytes = {};
        DOGPupDataAll(bytes, rp->runs, i);
        stack[i][0] = (wh128 const *)bytes[0];
        stack[i][1] = stack[i][0] + (u8csLen(bytes) / sizeof(wh128));
    }
    wh128css st = {stack, stack + n};
    if (HITwh128IsCompact(st)) done;
    wh128s into = {wh128bHead(rp->merge), wh128bTerm(rp->merge)};
    wh128 *base = into[0];
    call(HITwh128Compact, st, into);
    u32 left = (u32)$len(st);
    if (left == n) done;                     //  nothing collapsed
    u32 m = n - left + 1;                    //  runs merged into one
    u8cs merged = {(u8c *)base, (u8c *)into[0]};
    a_dup(u8c, dir, shard);
    a_dup(u8c, ex, ext);
    call(DOGPupThinTail, rp->runs, dir, ex, m);
    call(DOGPupCreate, rp->runs, dir, ex, merged);
    done;
}

//  Land the dirty page as one sorted run, then re-ladder.  Called when the
//  page fills and once at the end of the run — never mid-record.  A page
//  filled only PART-WAY (the last flush, at pack EOF) lands only its live
//  rows: the byte slice is bounded by DATA, never by the page's capacity.
static ok64 rp_flush(repack *rp, path8sc shard) {
    sane(rp != NULL);
    if (wh128bDataLen(rp->page) == 0) done;
    a_cstr(ext, ".keeper.idx");
    wh128s rows = {wh128bDataHead(rp->page), wh128bIdleHead(rp->page)};
    wh128sSort(rows);
    //  DOGPupCreate's contract is BYTES; the same span, seen as such.
    u8cs bytes = {(u8c *)rows[0], (u8c *)rows[1]};
    a_dup(u8c, dir, shard);
    a_dup(u8c, ex, ext);
    call(DOGPupCreate, rp->runs, dir, ex, bytes);
    wh128bReset(rp->page);
    call(rp_compact, rp, shard, ext);
    done;
}

//  One index row: into the caller's region (its contract) AND into the
//  dirty page, which is what makes it findable before it reaches a run.
static ok64 rp_index(repack *rp, path8sc shard, Bwh128 idx, wh128 e,
                     repack_stat *st) {
    sane(rp != NULL && st != NULL);
    test(wh128bFeed1(idx, e) == OK, REPACKROOM);
    st->index_n++;
    test(wh128bFeed1(rp->page, e) == OK, REPACKROOM);
    if (wh128bIdleLen(rp->page) == 0) call(rp_flush, rp, shard);
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

//  Open log `rp->slot`.  A log we CREATE gets its 12-byte pack header with a
//  zero count — patched in place at seal time, which is the whole reason the
//  log stays open for the run.  Log `log0` may already EXIST, and then we
//  APPEND behind what is there: a pack log carries many packs ([PackLog]
//  §Many packs per log), so the file's own header is left alone and this
//  run's records simply start at `first` — the offset the PACK summary row
//  bookmarks, exactly as a tail-append does.  Only slot 0 can land on an
//  existing log; every rotation past it opens a fresh, unused id.
static ok64 rp_open(repack *rp, path8sc shard) {
    sane(rp != NULL);
    test(rp->slot < REPACK_MAX_LOGS, REPACKLOGS);
    a_path(path);
    call(REPACKLogPath, path, shard, rp->log0 + rp->slot);
    filestat fs = {};
    if (rp->slot == 0 && FILEStat(&fs, $path(path)) == OK &&
        (u64)fs.size > 12) {
        test((u64)fs.size < rp->cap_bytes, REPACKBIG);
        call(FILEBook, &rp->log[0], $path(path), (size_t)rp->cap_bytes);
        rp->base = rp->first = (u64)u8bBusyLen(rp->log[0]);
        rp->count = 0;
        done;
    }
    size_t init = rp->cap_bytes < RP_LOG_INIT ? (size_t)rp->cap_bytes
                                              : (size_t)RP_LOG_INIT;
    call(FILEBookCreate, &rp->log[rp->slot], $path(path),
         (size_t)rp->cap_bytes, init);
    call(rp_room, rp->log[rp->slot], rp->cap_bytes, 12);
    call(PACKu8sFeedHdr, u8bIdle(rp->log[rp->slot]), 0);
    rp->base = 0;
    rp->first = 12;
    rp->count = 0;
    done;
}

//  Seal the current log: patch the header count, trim the file to what we
//  wrote, and emit its PACK summary entry bookmarking `first`.  APPENDED
//  into an existing log, the header belongs to the pack already in there
//  and is left untouched — the summary row is what makes our records
//  findable, the same contract a tail-append has always had.  The mapping
//  is deliberately NOT dropped — a later record's delta base may live here.
static ok64 rp_seal(repack *rp, path8sc shard, Bwh128 idx, repack_stat *st) {
    sane(rp != NULL && st != NULL);
    u8bp lg = rp->log[rp->slot];
    if (lg == NULL || (u64)u8bBusyLen(lg) <= rp->first) done;
    if (rp->base == 0) {
        u8s hdr = {lg[0], lg[0] + 12};
        call(PACKu8sFeedHdr, hdr, (u32)rp->count);
    }
    u64 bytes = (u64)u8bBusyLen(lg);
    call(FILETrimMap, lg);
    wh128 sum = {.key = wh64Pack(0xF, rp->log0 + rp->slot, rp->first),
                 .val = (rp->count << 32) | (bytes - rp->first)};
    call(rp_index, rp, shard, idx, sum, st);
    st->out_bytes += bytes - rp->base;
    st->logs = rp->slot + 1;
    done;
}

//  Would this record still fit the cap?  Rotation is decided BEFORE any
//  bytes are written, so a log never overshoots its cap by a partial record.
static u64 rp_need(u8 emit_type, u64 hlen, u64 zlen) {
    return hlen + (emit_type == PACK_OBJ_REF_DELTA ? 20 : 0) + zlen;
}

//  Write one record into the current log: the rewritten object header, the
//  20-byte base sha when the base sits in another log, then the zlib payload
//  VERBATIM (no re-deflate, no re-delta).  A cited sha is either the one the
//  incoming record carried, or — for a base we re-anchor at a rotation —
//  HASHED HERE from the base bytes this record already resolved in order to
//  apply itself.  No sha is carried per record.
static ok64 rp_emit(repack *rp, u64 size, u64 base_off, u8csc base,
                    u8 base_type, u8csc known_sha, u8csc zbytes,
                    u8 *emit_type, u64 *hlen) {
    sane(rp != NULL && emit_type != NULL && hlen != NULL);
    u8bp lg = rp->log[rp->slot];
    u64 our_off = (u64)u8bBusyLen(lg);
    a_pad(u8, hb, 32);
    call(PACKu8sFeedObjHdr, hb, *emit_type, size);
    if (*emit_type == PACK_OBJ_OFS_DELTA)
        call(PACKu8sFeedOfs, hb, our_off - base_off);
    *hlen = (u64)u8bDataLen(hb);
    call(rp_room, lg, rp->cap_bytes,
         (size_t)rp_need(*emit_type, *hlen, (u64)u8csLen(zbytes)));
    call(FILEBookFeed, lg, $path(hb));
    if (*emit_type == PACK_OBJ_REF_DELTA) {
        if (u8csLen(known_sha) == 20) {
            call(FILEBookFeed, lg, known_sha);
        } else {
            sha1 bsha = {};
            PIDXObjSha(&bsha, base_type, base);
            u8cs shas = {bsha.data, bsha.data + sizeof bsha.data};
            call(FILEBookFeed, lg, shas);
        }
    }
    call(FILEBookFeed, lg, zbytes);
    done;
}

//  Hold a record back until its base arrives: copy the payload into the
//  hold arena, remember how it arrived, and file it under the hashlet it is
//  waiting for.  Emitting it here would put a forward reference in the log.
static ok64 rp_park(repack *rp, u8 type, u64 size, u8csc zbytes, u64 pack_off,
                    u64 base_off, u8csc base_sha, u64 hashlet) {
    sane(rp != NULL);
    test(u8bIdleLen(rp->hold) >= (size_t)u8csLen(zbytes), REPACKBIG);
    rp_hold *h = &rp->holds[rp->nheld];
    *h = (rp_hold){.pack_off = pack_off,
                   .zoff = (u64)u8bDataLen(rp->hold),
                   .zlen = (u32)u8csLen(zbytes),
                   .size = size,
                   .hashlet = hashlet,
                   .base_off = base_off,
                   .type = type};
    if (u8csLen(base_sha) == 20) {
        a_dup(u8c, ss, base_sha);
        call(sha1Drain, ss, &h->base);
    }
    call(u8bFeed, rp->hold, zbytes);
    wh128 w = {.key = hashlet, .val = rp->nheld};
    test(wh128bFeed1(rp->wait, w) == OK, REPACKROOM);
    a_dup(wh128, ws, wh128bData(rp->wait));
    wh128sSort(ws);
    rp->nheld++;
    done;
}

//  Forward declaration: landing a record wakes the ones held on its sha,
//  and each of those lands the same way.
static ok64 rp_land(repack *rp, path8sc shard, Bwh128 idx, repack_stat *st,
                    u8 type, u64 size, u8csc zbytes, u64 pack_off,
                    u64 base_pack_off, u8csc base_sha, u32 depth);

//  The object that just landed may be the base held records were waiting
//  for: land each of them now (their payloads are in the hold arena), which
//  cascades — a record landed here can be the base of the next one.
static ok64 rp_wake(repack *rp, path8sc shard, Bwh128 idx, repack_stat *st,
                    sha1cp avail, u32 depth) {
    sane(rp != NULL && st != NULL);
    if (wh128bDataLen(rp->wait) == 0) done;
    u64 h = WHIFFHashlet60(avail);
    for (;;) {
        i64 at = rp_waitfind(rp->wait, h);
        if (at < 0) done;
        a_dup(wh128, ws, wh128bData(rp->wait));
        rp_hold *hd = &rp->holds[ws[0][at].val];
        wh128cs none = {ws[0], ws[0]};
        call(wh128bSplice, rp->wait, (size_t)at, 1, none);
        u8cs z = {u8bDataHead(rp->hold) + hd->zoff,
                  u8bDataHead(rp->hold) + hd->zoff + hd->zlen};
        u8cs bsha = {hd->base.data, hd->base.data + sizeof hd->base.data};
        u8cs none_sha = {NULL, NULL};
        call(rp_land, rp, shard, idx, st, hd->type, hd->size, z, hd->pack_off,
             hd->base_off, hd->type == PACK_OBJ_REF_DELTA ? bsha : none_sha,
             depth + 1);
    }
}

//  Land ONE record: locate its base, resolve it out of our own logs, emit
//  the record (rotating first if it would breach the cap), record where it
//  went, index it, and wake whatever waited on it.  A base that cannot be
//  located yet sends the whole record to the hold arena instead.
//  `base_pack_off` is the base's SOURCE offset (OFS arrival), `base_sha` its
//  sha (REF arrival); a raw record has neither.
static ok64 rp_land(repack *rp, path8sc shard, Bwh128 idx, repack_stat *st,
                    u8 type, u64 size, u8csc zbytes, u64 pack_off,
                    u64 base_pack_off, u8csc base_sha, u32 depth) {
    sane(rp != NULL && st != NULL);
    test(depth < PACK_DELTA_CHAIN_MAX, REPACKBASE);
    rp_finder finder = {.rp = rp, .shard = {shard[0], shard[1]}};
    u8cs base = {NULL, NULL};
    u8 out_type = type, base_type = 0;
    u32 base_id = rp->log0 + rp->slot;
    u64 base_off = 0, hashlet = 0;
    b8 is_delta = NO, park = NO;

    if (type == PACK_OBJ_OFS_DELTA) {
        wh128 *ent = rp_mapfind(rp->map, base_pack_off);
        test(ent != NULL, REPACKBASE);
        if (wh64Type(ent->val) == RP_HOLD) {
            //  The base is itself held: wait on the very hashlet it waits
            //  on, so the two land back to back, base first.
            hashlet = rp->holds[wh64Id(ent->val)].hashlet;
            park = YES;
        } else {
            base_id = wh64Id(ent->val);
            base_off = wh64Off(ent->val);
            is_delta = YES;
        }
    } else if (type == PACK_OBJ_REF_DELTA) {
        //  A thin pack cites a base by sha: the keeper index answers — the
        //  dirty page for what just landed, the runs for what an earlier
        //  fetch left in the shard.  A miss is not an error yet.
        sha1 want = {};
        a_dup(u8c, ss, base_sha);
        call(sha1Drain, ss, &want);
        u64 val = 0;
        hashlet = WHIFFHashlet60(&want);
        if (rp_idxfind(rp, hashlet, &val)) {
            base_id = wh64Id(val);
            base_off = wh64Off(val);
            is_delta = YES;
        } else {
            park = YES;
        }
    }

    //  Inflate the payload; for a delta this is the instruction stream.
    u8s into = {rp->inflated[0], rp->inflated[0] + size};
    a_dup(u8c, zs, zbytes);
    u8cs content = {rp->inflated[0], rp->inflated[0] + size};
    if (!park) {
        call(PACKInflate, zs, into, size);
        if (is_delta) {
            u8cs bpack = {};
            call(rp_logbytes, rp, shard, base_id, bpack);
            u8s rb = {u8bHead(rp->rbase), u8bTerm(rp->rbase)};
            u8s rd = {u8bHead(rp->rdelta), u8bTerm(rp->rdelta)};
            //  A chase that runs into a held record cannot finish either;
            //  the finder left the sha it missed in rp->stuck, so this
            //  record waits on the SAME base and they land in order.
            ok64 ro = PACKResolve(bpack, base_off, rb, rd, rp_ref_find,
                                  &finder, base, &base_type);
            if (ro == PACKREF) {
                hashlet = rp->stuck;
                park = YES;
            } else {
                if (ro != OK) fail(ro);
                u8cs dl = {rp->inflated[0], rp->inflated[0] + size};
                u8g ap = {rp->applied[0], rp->applied[0],
                          rp->applied[0] + RP_SCRATCH};
                call(DELTApply, dl, base, ap);
                content[0] = rp->applied[0];
                content[1] = rp->applied[0] + u8gLeftLen(ap);
                out_type = base_type;
            }
        }
    }

    if (park) {
        //  First arrival: the record gets its map row now (in source order,
        //  so the lane stays sorted) carrying the hold slot; the row is
        //  patched with the real location when the record lands.
        if (rp_mapfind(rp->map, pack_off) == NULL) {
            wh128 m = {.key = pack_off, .val = wh64Pack(RP_HOLD, (u32)rp->nheld, 0)};
            test(wh128bFeed1(rp->map, m) == OK, REPACKROOM);
        }
        call(rp_park, rp, type, size, zbytes, pack_off, base_pack_off,
             base_sha, hashlet);
        done;
    }

    //  Rotate BEFORE emitting when this record would breach the cap.  A
    //  base in another log can only be cited by sha.
    u8 emit_type = type;
    if (is_delta)
        emit_type = base_id == rp->log0 + rp->slot ? PACK_OBJ_OFS_DELTA
                                                   : PACK_OBJ_REF_DELTA;
    u64 hguess = 32;   //  header varint upper bound, checked again below
    if ((u64)u8bBusyLen(rp->log[rp->slot]) +
        rp_need(emit_type, hguess, (u64)u8csLen(zbytes)) > rp->cap_bytes) {
        call(rp_seal, rp, shard, idx, st);
        rp->slot++;
        call(rp_open, rp, shard);
        //  Re-decide against the fresh log: the base is now in an earlier
        //  one, so this becomes a REF and the sha must be hashed.
        return rp_land(rp, shard, idx, st, type, size, zbytes, pack_off,
                       base_pack_off, base_sha, depth);
    }

    u64 our_off = (u64)u8bBusyLen(rp->log[rp->slot]);
    u64 hlen = 0;
    call(rp_emit, rp, size, base_off, base, base_type, base_sha, zbytes,
         &emit_type, &hlen);
    if (emit_type == PACK_OBJ_OFS_DELTA) st->ofs++;
    else if (emit_type == PACK_OBJ_REF_DELTA) st->ref++;
    else st->raw++;
    rp->count++;
    st->log_len = (u64)u8bBusyLen(rp->log[rp->slot]);

    u64 here = wh64Pack(RP_HELD, rp->log0 + rp->slot, our_off);
    wh128 *row = rp_mapfind(rp->map, pack_off);
    if (row != NULL) {
        row->val = here;                    //  it was held; patch in place
    } else {
        wh128 m = {.key = pack_off, .val = here};
        test(wh128bFeed1(rp->map, m) == OK, REPACKROOM);
    }
    sha1 sha = {};
    PIDXObjSha(&sha, out_type, content);
    wh128 e = PIDXEntry(out_type, &sha, 0);
    e.val = here;
    call(rp_index, rp, shard, idx, e, st);
    call(rp_wake, rp, shard, idx, st, &sha, depth);
    done;
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
    //  RULING 2026-07-27 (gritzko): the stream checksum is OFF.  The header's
    //  object count already catches a stream cut, and zlib's adler32 covers
    //  every payload, so the trailer only adds damage that leaves both valid.
    //  Left here, not deleted, in case that judgement is revisited.
    //  SHA1state csum;
    //  SHA1Open(&csum);
    //  u8cs hbytes = {hs[0], hs[0] + 12};
    //  SHA1Feed(&csum, hbytes);
    call(u8bUsed, buf, 12);
    st->total = hdr.count;
    st->in_bytes = 12;

    u64 lanes = hdr.count ? hdr.count : 1024;
    call(wh128bMap, rp->map, lanes);
    call(wh128bMap, rp->wait, lanes);
    call(wh128bMap, rp->page, RP_PAGE);
    call(u8bMap, rp->holdmem, lanes * sizeof(rp_hold));
    rp->holds = (rp_hold *)rp->holdmem[0];
    call(u8bMap, rp->hold, RP_SCRATCH);
    call(u8bMap, rp->inflated, RP_SCRATCH);
    call(u8bMap, rp->applied, RP_SCRATCH);
    call(u8bMap, rp->rbase, RP_SCRATCH);
    call(u8bMap, rp->rdelta, RP_SCRATCH);
    //  The shard's existing index: a thin pack cites bases we already hold.
    a_cstr(ext, ".keeper.idx");
    call(kv64bAllocate, rp->runs, RP_PUPS);
    call(DOGPupOpenAll, rp->runs, shard, ext);
    //  Compaction merges the WHOLE stack, this run's rows plus whatever the
    //  shard already carried — size the merge scratch for both, not for the
    //  incoming pack alone (OKNOROOM out of HITwh128Compact otherwise).
    u64 held = 0;
    for (u32 i = DOGPupCountAll(rp->runs); i > 0; i--) {
        u8cs bytes = {};
        DOGPupDataAll(bytes, rp->runs, i - 1);
        held += (u64)u8csLen(bytes) / sizeof(wh128);
    }
    call(wh128bMap, rp->merge, lanes + held + RP_PAGE);
    call(rp_open, rp, shard);
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

        u8cs bsha = {NULL, NULL};
        u64 base_pack_off = 0;
        if (obj.type == PACK_OBJ_OFS_DELTA) {
            test(obj.ofs_delta != 0 && obj.ofs_delta <= cur, REPACKBASE);
            base_pack_off = cur - obj.ofs_delta;
        } else if (obj.type == PACK_OBJ_REF_DELTA) {
            bsha[0] = obj.ref_delta[0];
            bsha[1] = obj.ref_delta[0] + 20;
        }
        call(rp_land, rp, shard, idx, st, obj.type, obj.size, zbytes, cur,
             base_pack_off, bsha, 0);

        //  u8cs cbytes = {ds[0], ds[0] + consumed};   //  checksum: see above
        //  SHA1Feed(&csum, cbytes);
        call(u8bUsed, buf, (size_t)consumed);
        cur += consumed;
        st->in_bytes += consumed;
        st->objects++;
        if (conf->watch && conf->every && st->objects % conf->every == 0)
            call(conf->watch, conf->user, st);
    }
    //  A record still held here awaited a base the pack never delivered and
    //  the shard never had: the source is not self-contained.
    test(wh128bDataLen(rp->wait) == 0, REPACKBASE);
    //  The 20-byte trailer is left UNREAD in the caller's buffer (checksum
    //  off, see above).  Were it re-enabled it would verify HERE, before
    //  anything is sealed or indexed: every record is already durable in the
    //  log ([PackLog] §Epoch recompaction — orphans wait for the next epoch),
    //  but a stream we cannot vouch for must not become findable.
    //  while (u8bDataLen(buf) < 20 && !at_eof) call(rp_fill, fd, buf, &at_eof);
    //  test(u8bDataLen(buf) >= 20, REPACKTORN);
    //  u8cs ts = {u8bDataHead(buf), u8bIdleHead(buf)};
    //  a_dup(u8c, tscan, ts);
    //  sha1 want = {}, got = {};
    //  call(sha1Drain, tscan, &want);
    //  SHA1Close(&csum, &got);
    //  test(memcmp(got.data, want.data, sizeof got.data) == 0, REPACKSUM);
    //  call(u8bUsed, buf, 20);
    call(rp_seal, rp, shard, idx, st);
    call(rp_flush, rp, shard);         //  the tail page becomes a run too
    if (conf->watch) call(conf->watch, conf->user, st);
    done;
}

ok64 REPACKRun(int fd, u8b buf, path8sc shard, repack_conf const *conf,
               Bwh128 idx, repack_stat *st) {
    sane(fd >= 0 && buf != NULL && conf != NULL && st != NULL);
    repack rp = {};
    rp.cap_bytes = conf->cap ? conf->cap : REPACK_LOG_MAX;
    rp.log0 = conf->log0;
    *st = (repack_stat){};
    st->log0 = conf->log0;
    try(repack_loop, &rp, fd, buf, shard, conf, idx, st);
    for (u32 i = 0; i < rp.nold; i++)
        if (rp.old[i] != NULL) FILEUnMap(rp.old[i]);
    for (u32 i = 0; i < REPACK_MAX_LOGS; i++)
        if (rp.log[i] != NULL) FILEUnMap(rp.log[i]);
    if (rp.runs[0]) DOGPupClose(rp.runs);
    if (rp.rdelta[0]) u8bUnMap(rp.rdelta);
    if (rp.rbase[0]) u8bUnMap(rp.rbase);
    if (rp.applied[0]) u8bUnMap(rp.applied);
    if (rp.inflated[0]) u8bUnMap(rp.inflated);
    if (rp.hold[0]) u8bUnMap(rp.hold);
    if (rp.holdmem[0]) u8bUnMap(rp.holdmem);
    if (rp.page[0]) wh128bUnMap(rp.page);
    if (rp.merge[0]) wh128bUnMap(rp.merge);
    if (rp.wait[0]) wh128bUnMap(rp.wait);
    if (rp.map[0]) wh128bUnMap(rp.map);
    done;
}
