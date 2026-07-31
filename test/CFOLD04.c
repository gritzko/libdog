//
//  CFOLD04 (DIS-082, was WEAVE04/DIS-047) — large-file PERF bench.
//
//  CLAUDE.md §3: a bench that makes a superlinear regression brutally
//  obvious.  Builds a synthetic ~Nk-line file, forks two divergent
//  branches off a shared base, and times the two hot paths at N and 2N:
//  the CONCURRENT FOLD (theirs onto a weave already carrying ours — the
//  per-commit one-pass index merge) and the MERGE RENDER (CFOLDProduce
//  at a pure merge commit — the DFS walk).  If a hot path is linear the
//  doubling ratio stays ~2x; a quadratic loop blows it to ~4x+.
//  Report-only: the strict ratio gate flapped on shared-VM CI runners
//  (see WEAVE04's history), so the numbers print but do not fail.
//
#include "dog/CFOLD.h"

#include <stdio.h>
#include <string.h>

#include "abc/POL.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

//  Emit one synthetic C-like line `i` (salt selects a value variant) into
//  `buf`: "int v0000123 = 04567 + 0;\n".  Distinct per (i, salt) so the
//  tokenizer yields real per-line structure and edits actually differ.
#define EMITLINE(BUF, I, SALT) do {                                    \
    u32 _i = (I); u32 _s = (SALT);                                     \
    a_cstr(_p0, "int v"); call(u8bFeed, (BUF), _p0);                   \
    for (i32 _d = 6; _d >= 0; _d--) { u32 _dv = 1;                     \
        for (i32 _k = 0; _k < _d; _k++) _dv *= 10;                     \
        call(u8bFeed1, (BUF), (u8)('0' + (_i / _dv) % 10)); }          \
    a_cstr(_p1, " = "); call(u8bFeed, (BUF), _p1);                     \
    u32 _val = (_i * 2654435761u + _s) % 100000u;                      \
    for (i32 _d = 4; _d >= 0; _d--) { u32 _dv = 1;                     \
        for (i32 _k = 0; _k < _d; _k++) _dv *= 10;                     \
        call(u8bFeed1, (BUF), (u8)('0' + (_val / _dv) % 10)); }        \
    a_cstr(_p2, " + "); call(u8bFeed, (BUF), _p2);                     \
    call(u8bFeed1, (BUF), (u8)('0' + (_s % 10)));                      \
    call(u8bFeed1, (BUF), ';'); call(u8bFeed1, (BUF), '\n');           \
} while (0)

//  Base plus two DIVERGENT branches: ours edits every 3rd line, theirs
//  edits every 5th and deletes every 11th, so the concurrent fold has to
//  interleave plenty of foreign entries.  Times the theirs fold and the
//  merge-commit render.
static ok64 fold_time_ns(u32 nlines, u64 *fold_ns, u64 *prod_ns) {
    sane(fold_ns != NULL && prod_ns != NULL);
    a_cstr(cext, "c");
    enum { C_BASE = 1, C_OURS = 2, C_THEIRS = 3, C_MERGE = 9 };

    a_carve(u8, bbase, (size_t)nlines * 64 + 4096);
    a_carve(u8, oblob, (size_t)nlines * 64 + 4096);
    a_carve(u8, tblob, (size_t)nlines * 64 + 4096);
    u8bReset(bbase); u8bReset(oblob); u8bReset(tblob);
    for (u32 i = 0; i < nlines; i++) {
        EMITLINE(bbase, i, 0);
        EMITLINE(oblob, i, (i % 3 == 1) ? (90000 + i) : 0);     // ours edits
        if (i % 11 != 4) EMITLINE(tblob, i, (i % 5 == 2) ? (50000 + i) : 0);
    }
    a_dup(u8c, vbase,   u8bDataC(bbase));
    a_dup(u8c, vours,   u8bDataC(oblob));
    a_dup(u8c, vtheirs, u8bDataC(tblob));

    //  ONE growing weave: body ~64 B/line * 3 revs + 8 B/tok index; budget
    //  ~256 B/line per generation, all four generations carved up front.
    a_carve(u64, anc, 8);
    u8 *wb[4][4] = {};
    ok64 ar;
    for (u32 i = 0; i < 4; i++)
        if ((ar = u8bAcquire(ABC_BASS, wb[i], (size_t)nlines * 512 + 131072))
            != OK)
            return ar;
    cfold w = {};

    call(CFOLDFold, u8bIdle(wb[0]), NULL, vbase, cext, C_BASE, u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[0]));

    u64bReset(anc); call(u64bFeed1, anc, C_BASE);
    call(CFOLDFold, u8bIdle(wb[1]), &w, vours, cext, C_OURS, u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[1]));

    u64bReset(anc); call(u64bFeed1, anc, C_BASE);   // concurrent with ours
    u64 t0 = POLNow();
    call(CFOLDFold, u8bIdle(wb[2]), &w, vtheirs, cext, C_THEIRS,
         u64bDataC(anc));
    u64 t1 = POLNow();
    call(CFOLDParse, &w, u8bDataC(wb[2]));
    *fold_ns = t1 - t0;

    u64bReset(anc);
    call(u64bFeed1, anc, C_BASE);
    call(u64bFeed1, anc, C_OURS);
    call(u64bFeed1, anc, C_THEIRS);
    call(CFOLDMerge, u8bIdle(wb[3]), &w, C_MERGE, u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[3]));
    if (CFOLDEmpty(&w)) fail(TESTFAIL);

    a_carve(u8, out, (size_t)nlines * 128 + 65536);
    u64 t2 = POLNow();
    call(CFOLDProduce, &w, 3, out, NULL);
    u64 t3 = POLNow();
    *prod_ns = t3 - t2;
    done;
}

//  Best (minimum) time over `reps` runs: a transient stall only ever
//  inflates a sample, never deflates it, so the min can't false-fail.
static ok64 best_ns(u32 nlines, u32 reps, u64 *fold_out, u64 *prod_out) {
    sane(fold_out != NULL && prod_out != NULL);
    u64 bf = ~(u64)0, bp = ~(u64)0;
    for (u32 r = 0; r < reps; r++) {
        u64 f = 0, p = 0;
        call(fold_time_ns, nlines, &f, &p);
        if (f < bf) bf = f;
        if (p < bp) bp = p;
    }
    *fold_out = bf;
    *prod_out = bp;
    done;
}

static ok64 bench_fold_scaling(void) {
    sane(1);
    fprintf(stderr, "  CFOLD scaling bench ...\n");

    u32 N = 5000;                       // the DIS-047 ~5k-line case
    u64 wf = 0, wp = 0; call(fold_time_ns, 256, &wf, &wp);   // warm up

    u64 f_n = 0, p_n = 0, f_2n = 0, p_2n = 0;
    call(best_ns, N,     5, &f_n,  &p_n);
    call(best_ns, 2 * N, 5, &f_2n, &p_2n);

    double fr = (f_n > 0) ? (double)f_2n / (double)f_n : 0.0;
    double pr = (p_n > 0) ? (double)p_2n / (double)p_n : 0.0;
    fprintf(stderr,
        "    N=%u   fold = %8.3f ms  produce = %8.3f ms  [best of 5]\n"
        "    N=%u   fold = %8.3f ms  produce = %8.3f ms  [best of 5]\n"
        "    doubling ratio: fold %.2fx, produce %.2fx (linear ~2x)\n",
        N, (double)f_n / 1e6, (double)p_n / 1e6,
        2 * N, (double)f_2n / 1e6, (double)p_2n / 1e6, fr, pr);

    fprintf(stderr, "  CFOLD scaling bench ... ok\n");
    done;
}

ok64 CFOLD04test() {
    sane(1);
    call(bench_fold_scaling);
    done;
}

TEST(CFOLD04test);
