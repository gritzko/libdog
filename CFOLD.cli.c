//  dogcfold (DIS-082) — exercise the append-only weave from the shell.
//
//    dogcfold FILE [FILE...]
//
//  Folds each FILE as one successive LINEAR commit (each sees all the
//  earlier ones), then re-materializes every rev and checks it against the
//  file it came from.  Per commit it reports what the two streams did: how
//  far the BODY grew (it may only grow), how many INDEX entries the merge
//  moved, and how big the ignore-set is (empty for linear history).
//
#include "CFOLD.h"

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

#include <stdio.h>

#define CFCLI_MAX 64
#define CFCLI_CAP (1UL << 25)   // 32 MiB per generation (the body caps at 16)

//  Commit ids are arbitrary and non-monotonic on purpose — the sibling
//  tie-break must never be able to lean on a counter.
static u64 cfcli_cid(u32 i) {
    u64 x = (u64)i + 0x9E3779B97F4A7C15ULL;
    x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27; x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

static ok64 cfcli_fold(u8bp *src, u8b gen, u32 i, cfold const *prev,
                       u64csc anc) {
    sane(gen);
    a$rg(arg, i + 1);
    u8cs name = {};
    u8csMv(name, arg);
    u8cs ext = {};
    PATHu8sExt(ext, name);
    a_path(path, arg);
    call(FILEMapRO, src, $path(path));
    u8csc blob = {u8bDataHead(*src), u8bIdleHead(*src)};
    cffold fs = {};
    call(CFOLDFoldStat, u8bIdle(gen), prev, blob, ext, cfcli_cid(i), anc, &fs);
    cfold w = {};
    call(CFOLDParse, &w, u8bDataC(gen));
    cfcommit c = {};
    call(CFOLDCommitAt, &c, &w, i);
    fprintf(stderr,
            "  [%u] %.*s: body %u -> %u (+%u), index %u+%u in %u steps, "
            "ignore %u, materialized %u\n",
            i, (int)$len(arg), (char const *)arg[0], fs.body_before,
            fs.body_after, fs.body_after - fs.body_before, fs.entries_old,
            fs.entries_new, fs.merge_steps, c.ign_len, c.P);
    done;
}

static ok64 cfcli_inner(u8bp *src, u8 *gen[][4], u32 n) {
    sane(n > 0);
    a_carve(u64, anc, CFCLI_MAX + 1);
    cfold cur = {};
    for (u32 i = 0; i < n; i++) {
        //  Linear history: every earlier commit is an ancestor, so every
        //  ignore-set comes out EMPTY and visibility is one compare.
        u64bReset(anc);
        for (u32 j = 0; j < i; j++) call(u64bFeed1, anc, cfcli_cid(j));
        call(cfcli_fold, &src[i], gen[i], i, i ? &cur : NULL, u64bDataC(anc));
        call(CFOLDParse, &cur, u8bDataC(gen[i]));
    }
    //  Every rev must come back byte-for-byte out of the ONE weave.
    a_carve(u8, out, CFCLI_CAP);
    for (u32 i = 0; i < n; i++) {
        cfstat st = {};
        call(CFOLDProduce, &cur, i, out, &st);
        u8csc want = {u8bDataHead(src[i]), u8bIdleHead(src[i])};
        b8 ok = (u8bDataLen(out) == u8csLen(want));
        if (ok && u8csLen(want)) {
            a_dup(u8c, got, u8bDataC(out));
            ok = u8csEq(got, want);
        }
        fprintf(stderr,
                "  rev %u: %s (%u present, %u alive, %u groups, %u bsearch)\n",
                i, ok ? "matches" : "MISMATCH", st.present, st.emitted,
                st.groups, st.bsearch);
        if (!ok) fail(CFOLDFAIL);
    }
    fprintf(stderr, "dogcfold: %u commits, %u body bytes, %u index entries\n",
            n, (u32)u8csLen(cur.body), (u32)$len(cur.idx));
    done;
}

ok64 cfcli() {
    sane($arglen >= 2);
    u32 n = (u32)$arglen - 1;
    if (n > CFCLI_MAX) n = CFCLI_MAX;
    u8bp src[CFCLI_MAX] = {};
    u8  *gen[CFCLI_MAX][4] = {};
    for (u32 i = 0; i < n; i++) call(u8bAllocate, gen[i], CFCLI_CAP);
    try(cfcli_inner, src, gen, n);
    for (u32 i = 0; i < n; i++) {
        if (src[i] != NULL) FILEUnMap(src[i]);
        u8bFree(gen[i]);
    }
    done;
}

MAIN(cfcli);
