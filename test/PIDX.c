//  PIDX: property test for the GIT-010 dog/git pack-log index-EMIT
//  (PIDXScan / PIDXFeedEmit).  No keeper, no fork — a small single-pack
//  emitter that drops one (sha->offset) wh128 entry per object into a
//  caller buffer.
//
//    1. scan-emit: build a pack of raw + OFS_DELTA + delta-of-delta blobs,
//       run PIDXScan, and assert each emitted (key, val) entry points at an
//       object whose RESOLVED bytes hash back to the sha encoded in the key
//       (hashlet60) and carry the recorded type;
//    2. feed-emit == scan: PIDXFeedEmit (hash the content you hold, no
//       resolve) produces the SAME entry as a full reindex for each object;
//    3. REF backstop: a REF_DELTA record makes the scan return PIDXFAIL
//       (loud — an OFS-only log carries no sha-addressed bases).

#include "git/PIDX.h"

#include <stdio.h>
#include <string.h>

#include "abc/B.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "abc/TEST.h"
#include "git/DELT.h"
#include "git/PACK.h"
#include "git/ZINF.h"

//  wh128 buffer machinery (Bwh128 / wh128bAllocate / wh128bData ...) is
//  already instantiated by dog/WHIFF.h (pulled in via git/PIDX.h).

#define SCRATCH (1u << 20)

//  Append one raw object record (header + deflated content); return its
//  start offset via *off_out.  (Same shape as PACKRESOLVE.c's feed_raw.)
static ok64 feed_raw(u8bp log, u8 type, u8csc content, u64 *off_out) {
    sane(u8bOK(log));
    *off_out = (u64)u8bDataLen(log);
    a_pad(u8, hdr, 16);
    call(PACKu8sFeedObjHdr, hdr, type, u8csLen(content));
    a_dup(u8c, hb, u8bData(hdr));
    call(u8bFeed, log, hb);
    a_dup(u8c, cz, content);
    call(ZINFDeflate, u8bIdle(log), cz);
    done;
}

//  Append one OFS_DELTA record (delta of `target` against `base`),
//  back-pointing to `base_off`; return the record offset.
static ok64 feed_ofs(u8bp log, u8csc base, u8csc target, u64 base_off,
                     u8bp dscratch, u64 *off_out) {
    sane(u8bOK(log));
    u64 cur = (u64)u8bDataLen(log);
    *off_out = cur;
    u8bReset(dscratch);
    call(DELTEncode, base, target, dscratch);
    u64 dlen = (u64)u8bDataLen(dscratch);
    a_pad(u8, hdr, 16);
    call(PACKu8sFeedObjHdr, hdr, PACK_OBJ_OFS_DELTA, dlen);
    a_dup(u8c, hb, u8bData(hdr));
    call(u8bFeed, log, hb);
    a_pad(u8, ofs, 16);
    call(PACKu8sFeedOfs, ofs, cur - base_off);
    a_dup(u8c, ob, u8bData(ofs));
    call(u8bFeed, log, ob);
    a_dup(u8c, dz, u8bDataC(dscratch));
    call(ZINFDeflate, u8bIdle(log), dz);
    done;
}

//  Oracle: resolve `off` in `log` and return its (type, full bytes) into a
//  caller buffer.  Reuses the dog/git resolver directly.
static ok64 resolve_into(u8cs log, u64 off, u8bp dst, u8 *type_out) {
    sane($ok(log) && u8bOK(dst));
    a_carve(u8, bb, SCRATCH);
    a_carve(u8, db, SCRATCH);
    u8s bsc = {u8bHead(bb), u8bTerm(bb)};
    u8s dsc = {u8bHead(db), u8bTerm(db)};
    u8cs body = {};
    u8 type = 0;
    call(PACKResolveOfs, log, off, bsc, dsc, body, &type);
    u8bReset(dst);
    a_dup(u8c, bc, body);
    call(u8bFeed, dst, bc);
    *type_out = type;
    done;
}

//  (1) scan-emit: every entry's key (type|hashlet60) and val (offset) is
//  consistent with the object actually at that offset.
static ok64 scan_emit() {
    sane(1);
    a_cstr(v0, "the quick brown fox jumps over the lazy dog, take 0");
    a_cstr(v1, "the quick brown fox jumps over the lazy dog, take 1!!");
    a_cstr(v2, "the quick brown fox jumps over the lazy dog, take 22?");
    a_cstr(t0, "100644 file\0aaaaaaaaaaaaaaaaaaaa");   //  a "tree"-typed blob

    a_carve(u8, logb_c, SCRATCH);
    zerob(logb_c);
    u8bp log = (u8bp)logb_c;
    u8bReset(log);
    a_carve(u8, ds, SCRATCH);
    u8bp dscratch = (u8bp)ds;

    u8s into = {u8bIdleHead(log), u8bTerm(log)};
    call(PACKu8sFeedHdr, into, 4);
    u8bFed(log, 12);

    u64 o0 = 0, o1 = 0, o2 = 0, o3 = 0;
    call(feed_raw, log, PACK_OBJ_BLOB, v0, &o0);
    call(feed_ofs, log, v0, v1, o0, dscratch, &o1);
    call(feed_ofs, log, v1, v2, o1, dscratch, &o2);  //  delta-of-delta
    call(feed_raw, log, PACK_OBJ_TREE, t0, &o3);     //  a non-blob type

    u8cs lb = {u8bDataHead(log), u8bIdleHead(log)};

    //  Run the scan-emit into a caller buffer.
    Bwh128 out = {};
    call(wh128bAllocate, out, 8);
    a_carve(u8, bb, SCRATCH);
    a_carve(u8, db, SCRATCH);
    u8s bsc = {u8bHead(bb), u8bTerm(bb)};
    u8s dsc = {u8bHead(db), u8bTerm(db)};
    if (PIDXScan(lb, out, bsc, dsc) != OK) { wh128bFree(out); fail(TESTFAIL); }

    a_dup(wh128, entries, wh128bData(out));   //  slice over emitted entries
    size_t n = (size_t)wh128sLen(entries);
    if (n != 4) {
        fprintf(stderr, "scan: %zu entries, want 4\n", n);
        wh128bFree(out); fail(TESTFAIL);
    }

    u64 want_off[4] = {o0, o1, o2, o3};
    u8  want_type[4] = {PACK_OBJ_BLOB, PACK_OBJ_BLOB, PACK_OBJ_BLOB, PACK_OBJ_TREE};
    for (size_t i = 0; i < 4; i++) {
        wh128 e = entries[0][i];
        //  val is the bare in-pack offset.
        if (wh64Off(e.val) != want_off[i] && e.val != want_off[i]) {
            fprintf(stderr, "entry %zu: val=%llu want off %llu\n", i,
                    (unsigned long long)e.val, (unsigned long long)want_off[i]);
            wh128bFree(out); fail(TESTFAIL);
        }
        //  key carries (hashlet60 | type).
        u8 ktype = WHIFFKeyType(e.key);
        if (ktype != want_type[i]) {
            fprintf(stderr, "entry %zu: key type=%u want %u\n", i, ktype, want_type[i]);
            wh128bFree(out); fail(TESTFAIL);
        }
        //  Resolve the object the val points at, re-sha, and the hashlet60
        //  of that sha must equal the one in the key.
        a_carve(u8, rb, SCRATCH);
        u8bp rbuf = (u8bp)rb; u8bReset(rbuf);
        u8 rtype = 0;
        if (resolve_into(lb, e.val, rbuf, &rtype) != OK) { wh128bFree(out); fail(TESTFAIL); }
        if (rtype != want_type[i]) { wh128bFree(out); fail(TESTFAIL); }
        sha1 sha = {};
        PIDXObjSha(&sha, rtype, u8bDataC(rbuf));
        if (WHIFFKeyHashlet(e.key) != WHIFFHashlet60(&sha)) {
            fprintf(stderr, "entry %zu: hashlet mismatch\n", i);
            wh128bFree(out); fail(TESTFAIL);
        }
    }
    wh128bFree(out);
    done;
}

//  (2) feed-emit == scan: hashing the content you already hold yields the
//  SAME entry as resolving in the scan.
static ok64 feed_eq_scan() {
    sane(1);
    a_cstr(v0, "feed-emit must match a full reindex, byte for byte ok?");
    a_cstr(v1, "feed-emit must match a full reindex, byte for byte!!ok");

    a_carve(u8, logb_c, SCRATCH); zerob(logb_c);
    u8bp log = (u8bp)logb_c; u8bReset(log);
    a_carve(u8, ds, SCRATCH); u8bp dscratch = (u8bp)ds;

    u8s into = {u8bIdleHead(log), u8bTerm(log)};
    call(PACKu8sFeedHdr, into, 2); u8bFed(log, 12);
    u64 o0 = 0, o1 = 0;
    call(feed_raw, log, PACK_OBJ_BLOB, v0, &o0);
    call(feed_ofs, log, v0, v1, o0, dscratch, &o1);
    u8cs lb = {u8bDataHead(log), u8bIdleHead(log)};

    //  Full scan reindex.
    Bwh128 scan = {};
    call(wh128bAllocate, scan, 4);
    a_carve(u8, bb, SCRATCH); a_carve(u8, db, SCRATCH);
    u8s bsc = {u8bHead(bb), u8bTerm(bb)};
    u8s dsc = {u8bHead(db), u8bTerm(db)};
    if (PIDXScan(lb, scan, bsc, dsc) != OK) { wh128bFree(scan); fail(TESTFAIL); }

    //  Index-on-append: feed each object's full content + its offset.  Both
    //  objects resolve to v0 / v1, so feed-emit gets the same (sha, off).
    Bwh128 feed = {};
    call(wh128bAllocate, feed, 4);
    if (PIDXFeedEmit(feed, PACK_OBJ_BLOB, v0, o0) != OK ||
        PIDXFeedEmit(feed, PACK_OBJ_BLOB, v1, o1) != OK) {
        wh128bFree(scan); wh128bFree(feed); fail(TESTFAIL);
    }

    a_dup(wh128, se, wh128bData(scan));
    a_dup(wh128, fe, wh128bData(feed));
    if (wh128sLen(se) != 2 || wh128sLen(fe) != 2) {
        wh128bFree(scan); wh128bFree(feed); fail(TESTFAIL);
    }
    for (size_t i = 0; i < 2; i++) {
        if (!wh128hashEq(&se[0][i], &fe[0][i])) {
            fprintf(stderr, "feed!=scan at %zu\n", i);
            wh128bFree(scan); wh128bFree(feed); fail(TESTFAIL);
        }
    }
    wh128bFree(scan); wh128bFree(feed);
    done;
}

//  (3) REF backstop: a REF_DELTA record makes the scan fail loudly.
static ok64 ref_backstop() {
    sane(1);
    a_carve(u8, lc, SCRATCH); zerob(lc);
    u8bp log = (u8bp)lc; u8bReset(log);
    u8s into = {u8bIdleHead(log), u8bTerm(log)};
    call(PACKu8sFeedHdr, into, 1); u8bFed(log, 12);
    //  type=7 (REF_DELTA), size=1, + 20-byte base sha + 1 zlib body byte.
    u8bFeed1(log, (u8)((PACK_OBJ_REF_DELTA << 4) | 1));
    for (int i = 0; i < 20; i++) u8bFeed1(log, (u8)(0x10 + i));
    u8bFeed1(log, 0x00);
    u8cs lb = {u8bDataHead(log), u8bIdleHead(log)};

    Bwh128 out = {};
    call(wh128bAllocate, out, 2);
    a_carve(u8, bb, SCRATCH); a_carve(u8, db, SCRATCH);
    u8s bsc = {u8bHead(bb), u8bTerm(bb)};
    u8s dsc = {u8bHead(db), u8bTerm(db)};
    ok64 got = PIDXScan(lb, out, bsc, dsc);
    wh128bFree(out);
    if (got == OK) {
        fprintf(stderr, "ref backstop: scan returned OK, want fail\n");
        fail(TESTFAIL);
    }
    done;
}

ok64 maintest() {
    sane(1);
    fprintf(stderr, "scan_emit...\n");
    call(scan_emit);
    fprintf(stderr, "feed_eq_scan...\n");
    call(feed_eq_scan);
    fprintf(stderr, "ref_backstop...\n");
    call(ref_backstop);
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest)
