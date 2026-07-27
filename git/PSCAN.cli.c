//  dogscan — read a repacked shard back, REF_DELTA records included
//  (KEEP-006).  The twin of dogrepack: what that writes, this resolves.
//
//    dogscan <shard-dir> [--print]
//
//  Maps every `NNNNNNNNNN.keeper` log and scans them in file-id order.  A
//  rotated repack re-anchors each delta whose base landed in an EARLIER
//  log as REF_DELTA, so scanning log N needs logs 0..N-1 — which an
//  in-order scan has already covered.  That is the whole finder here: the
//  entries the scan itself emitted, sorted by hashlet.  No index file, no
//  store coupling; a caller that HAS an index (the js store) answers the
//  same question from it instead.

#include <stdio.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "dog/WHIFF.h"
#include "dog/git/PACK.h"
#include "dog/git/PIDX.h"
#include "dog/git/REPACK.h"

//  wh128 sort template for the finder's rows (wh128Z: key, then val).
#define X(M, name) M##wh128##name
#include "abc/QSORTx.h"
#undef X

#define SC_LOGS      REPACK_MAX_LOGS   //  the writer's bound, reused
#define SC_SLOTS     (1ull << 25)      //  33.5 M entries of VA, lazily faulted
#define SC_SCRATCH   (1ull << 28)      //  resolve scratch, per PACKResolve's sizing

typedef struct {
    u8bp log[SC_LOGS];   //  mapped logs, indexed by slot (not by file id)
    u32 id[SC_LOGS];
    u32 n;
    Bwh128 rows;         //  key = hashlet60, val = wh64Pack(1, slot, offset)
} scan_ctx;

//  Rows are sorted by key, so the base lookup is a plain binary search.
static wh128 *sc_lookup(scan_ctx *c, u64 hashlet) {
    a_dup(wh128, rs, wh128bData(c->rows));
    wh128 *lo = rs[0], *hi = rs[1];
    while (lo < hi) {
        wh128 *mid = lo + (hi - lo) / 2;
        if (mid->key == hashlet) return mid;
        if (mid->key < hashlet) lo = mid + 1; else hi = mid;
    }
    return NULL;
}

//  The KEEP-006 base finder: sha -> (which log, where in it).  Hands back
//  the WHOLE log slice, so the chase continues there — a base may itself
//  be a delta, and may sit in a log earlier still.
static ok64 sc_find(void *user, u8csc sha, u8csp base_out, u64 *off_out) {
    sane(user != NULL && base_out != NULL && off_out != NULL);
    scan_ctx *c = (scan_ctx *)user;
    sha1 base = {};
    a_dup(u8c, ss, sha);
    call(sha1Drain, ss, &base);
    wh128 *hit = sc_lookup(c, WHIFFHashlet60(&base));
    test(hit != NULL, PACKREF);
    u32 slot = wh64Id(hit->val);
    test(slot < c->n && c->log[slot] != NULL, PACKREF);
    base_out[0] = u8bDataHead(c->log[slot]);
    base_out[1] = u8bIdleHead(c->log[slot]);
    *off_out = wh64Off(hit->val);
    done;
}

//  Map every `NNNNNNNNNN.keeper` the shard holds, in file-id order.  Ids
//  are probed rather than listed: the writer emits a contiguous run and
//  bounds it at REPACK_MAX_LOGS, so this covers exactly what it can write.
static ok64 sc_open(scan_ctx *c, path8sc shard) {
    sane(c != NULL);
    for (u32 id = 0; id < SC_LOGS; id++) {
        a_path(path);
        call(REPACKLogPath, path, shard, id);
        if (FILEExists($path(path)) != OK) continue;
        test(c->n < SC_LOGS, REPACKLOGS);
        call(FILEMapRO, &c->log[c->n], $path(path));
        c->id[c->n] = id;
        c->n++;
    }
    done;
}

//  Fold the entries this log's scan just emitted into the finder's rows:
//  re-key them by hashlet alone (the finder has a sha, not a type) and
//  carry the slot the object lives in.  `from` is the row count before.
static ok64 sc_fold(scan_ctx *c, Bwh128 idx, u64 from, u32 slot) {
    sane(c != NULL);
    a_dup(wh128, es, wh128bData(idx));
    for (u64 i = from; i < (u64)(es[1] - es[0]); i++) {
        wh128 r = {.key = WHIFFKeyHashlet(es[0][i].key),
                   .val = wh64Pack(1, slot, es[0][i].val)};
        test(wh128bFeed1(c->rows, r) == OK, REPACKROOM);
    }
    a_dup(wh128, rs, wh128bData(c->rows));
    wh128sSort(rs);   //  one sort per log; the log count is bounded at 64
    done;
}

static ok64 scan_cli_inner(scan_ctx *c, Bwh128 idx, u8bp bsc, u8bp dsc,
                           b8 print) {
    sane(c != NULL);
    a$rg(a1, 1);
    a_path(shard, a1);
    call(sc_open, c, $path(shard));
    test(c->n > 0, REPACKFAIL);

    u64 total = 0;
    for (u32 s = 0; s < c->n; s++) {
        u64 before = (u64)wh128bDataLen(idx);
        u8cs pack = {u8bDataHead(c->log[s]), u8bIdleHead(c->log[s])};
        u8s base = {u8bHead(bsc), u8bTerm(bsc)};
        u8s delta = {u8bHead(dsc), u8bTerm(dsc)};
        call(PIDXScanRef, pack, 0, idx, base, delta, sc_find, c);
        u64 got = (u64)wh128bDataLen(idx) - before;
        fprintf(stderr, "  %010u.keeper: %llu objects, %llu bytes\n",
                c->id[s], (unsigned long long)got,
                (unsigned long long)u8csLen(pack));
        call(sc_fold, c, idx, before, s);
        total += got;
    }

    if (print) {
        a_dup(wh128, es, wh128bData(idx));
        $for(wh128, e, es) printf("%015llx %u %llu\n",
                                  (unsigned long long)WHIFFKeyHashlet(e->key),
                                  (unsigned)WHIFFKeyType(e->key),
                                  (unsigned long long)e->val);
    }
    fprintf(stderr, "scan: %llu objects over %u logs\n",
            (unsigned long long)total, c->n);
    done;
}

ok64 scan_cli() {
    sane($arglen >= 2);
    b8 print = NO;
    if ($arglen > 2) {
        a$rg(a2, 2);
        a_cstr(pf, "--print");
        print = u8csEq(a2, pf);
    }
    scan_ctx c = {};
    Bwh128 idx = {};
    u8b bsc = {}, dsc = {};
    call(wh128bMap, idx, SC_SLOTS);
    call(wh128bMap, c.rows, SC_SLOTS);
    call(u8bMap, bsc, SC_SCRATCH);
    call(u8bMap, dsc, SC_SCRATCH);
    try(scan_cli_inner, &c, idx, bsc, dsc, print);
    for (u32 i = 0; i < c.n; i++)
        if (c.log[i] != NULL) FILEUnMap(c.log[i]);
    u8bUnMap(dsc);
    u8bUnMap(bsc);
    wh128bUnMap(c.rows);
    wh128bUnMap(idx);
    done;
}

MAIN(scan_cli);
