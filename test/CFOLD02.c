//
//  CFOLD02 (DIS-082) — the ORACLE HARNESS.  A whole commit DAG goes into
//  ONE weave — a node is a single fold whose ancestor closure decides its
//  ignore-set, a merge is not an operation at all — and then EVERY rev is
//  materialized back and compared to the content it folded, byte for byte.
//  The input content IS the oracle; a mismatch is a real finding, never a
//  property to weaken.  (This began as the old-vs-new parity harness; the
//  columnar dog/WEAVE it compared against is gone, the oracle stays.)
//
#include "dog/CFOLD.h"

#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

#define DAG_MAX 16

typedef struct { char const *par; char const *content; } dagnode;
typedef struct {
    char const *name;   // NULL = quiet (the randomized sweep)
    dagnode const *nodes;
    u32 n;
} dagcase;

//  Arbitrary / non-monotonic commit ids (SplitMix64 of the node index),
//  as everywhere in these tests: a base routinely OUTRANKS its edits, which is the
//  DIS-044 toggle that a monotonic oracle hides.
static u64 dag_cid(u32 i) {
    u64 x = (u64)i + 0x9E3779B97F4A7C15ULL;
    x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27; x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

static void dag_closure(dagnode const *nd, u32 start, u8 *anc) {
    anc[start] = 1;
    for (u32 i = start + 1; i-- > 0;) {
        if (!anc[i]) continue;
        for (char const *r = nd[i].par; *r; r++) anc[(u8)(*r - '0')] = 1;
    }
}

static ok64 lineform(u8b dst, char const *s) {
    sane(1);
    u8bReset(dst);
    for (char const *p = s; *p; p++) {
        if (*p != '_') call(u8bFeed1, dst, (u8)*p);
        call(u8bFeed1, dst, (u8)'\n');
    }
    done;
}

static ok64 parity_case(dagcase const *dc) {
    sane(1);
    if (dc->name) fprintf(stderr, "  %s ...", dc->name);
    a_cstr(cext, "c");
    must(dc->n <= DAG_MAX, "dag too big");

    //  BASS acquires must SURVIVE the build loop (a call() would rewind).
    u8 *nbuf[DAG_MAX][4] = {};
    ok64 ar;
    a_carve(u8, lf, 1UL << 16);
    a_carve(u64, anc, DAG_MAX + 1);
    cfold A = {};

    for (u32 i = 0; i < dc->n; i++) {
        if ((ar = u8bAcquire(ABC_BASS, nbuf[i], 1UL << 18)) != OK) return ar;
        call(lineform, lf, dc->nodes[i].content);
        u8csc v = {u8bDataHead(lf), u8bDataHead(lf) + u8bDataLen(lf)};

        //  ONE fold per node, its ancestor closure decides the ignore-set.
        u8 cl[DAG_MAX] = {};
        dag_closure(dc->nodes, i, cl);
        u64bReset(anc);
        for (u32 j = 0; j < i; j++)
            if (cl[j]) call(u64bFeed1, anc, dag_cid(j));
        call(CFOLDFold, u8bIdle(nbuf[i]), i ? &A : NULL, v, cext, dag_cid(i),
             u64bDataC(anc));
        call(CFOLDParse, &A, u8bDataC(nbuf[i]));
    }

    //  --- every rev must come back byte-for-byte out of the ONE weave ---
    a_carve(u8, nb, 1UL << 16);
    u32 checked = 0;
    for (u32 a = 0; a < dc->n; a++) {
        call(CFOLDProduce, &A, a, nb, NULL);
        call(lineform, lf, dc->nodes[a].content);
        if (u8bDataLen(nb) != u8bDataLen(lf) ||
            (u8bDataLen(lf) &&
             memcmp(u8bDataHead(nb), u8bDataHead(lf), u8bDataLen(lf)))) {
            fprintf(stderr, "\n    rev=%u WANT(%zu) ", a, u8bDataLen(lf));
            $for(u8c, c, u8bDataC(lf)) fputc(*c == '\n' ? '.' : *c, stderr);
            fprintf(stderr, " GOT(%zu) ", u8bDataLen(nb));
            $for(u8c, c, u8bDataC(nb)) fputc(*c == '\n' ? '.' : *c, stderr);
            fputc('\n', stderr);
            fail(TESTFAIL);
        }
        checked++;
    }
    if (dc->name)
        fprintf(stderr, " ok (%u revs, %uB body)\n", checked,
                (u32)u8csLen(A.body));
    done;
}

// =====================================================================
//  Randomized DAG sweep — the same parity check over generated shapes, so
//  the harness is not limited to the DAGs somebody thought of.  Commit ids
//  stay ARBITRARY (SplitMix of the node index), never a monotonic counter:
//  that was the DIS-044 gap that hid a real ordering bug.
// =====================================================================

#define RND_DAGS  2000
#define RND_MAXN  10

static char    rnd_par[DAG_MAX][8];
static char    rnd_txt[DAG_MAX][12];
static dagnode rnd_nodes[DAG_MAX];
static u64     rnd_state;

static u32 rnd_next(u32 m) {
    rnd_state += 0x9E3779B97F4A7C15ULL;
    u64 x = rnd_state;
    x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27; x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return (u32)(x % m);
}

//  A small alphabet with REPEATS and a blank line ('_'), so identical-content
//  tokens collide constantly — the positional ambiguity that breaks joins.
static char const rnd_alpha[] = "aab_88c";

static void rnd_build(u32 n) {
    for (u32 i = 0; i < n; i++) {
        u32 np = 0;
        if (i == 1) np = 1;
        else if (i >= 2) np = 1 + rnd_next(2);
        u32 k = 0;
        for (u32 t = 0; t < np; t++) {
            char p = (char)('0' + rnd_next(i));
            b8 dup = NO;
            for (u32 q = 0; q < k; q++) if (rnd_par[i][q] == p) dup = YES;
            if (!dup) rnd_par[i][k++] = p;
        }
        rnd_par[i][k] = 0;
        u32 len = 1 + rnd_next(6);
        for (u32 t = 0; t < len; t++)
            rnd_txt[i][t] = rnd_alpha[rnd_next((u32)sizeof(rnd_alpha) - 1)];
        rnd_txt[i][len] = 0;
        rnd_nodes[i].par     = rnd_par[i];
        rnd_nodes[i].content = rnd_txt[i];
    }
}

static ok64 random_sweep(void) {
    sane(1);
    fprintf(stderr, "  random_dags(%d) ...", RND_DAGS);
    rnd_state = 20260731ULL;   // fixed seed: the sweep is reproducible
    for (u32 t = 0; t < RND_DAGS; t++) {
        u32 n = 2 + rnd_next(RND_MAXN - 1);
        rnd_build(n);
        dagcase dc = {NULL, rnd_nodes, n};
        try(parity_case, &dc);
        if (__ != OK) {
            fprintf(stderr, "\n    seed-step %u, dag:\n", t);
            for (u32 i = 0; i < n; i++)
                fprintf(stderr, "      {\"%s\", \"%s\"},\n",
                        rnd_nodes[i].par, rnd_nodes[i].content);
            return __;
        }
    }
    fprintf(stderr, " ok\n");
    done;
}

//  Linear chain: no forks anywhere, every ignore-set empty.
static dagnode const lin_nodes[] = {
    {"",  "abc"},
    {"0", "abXc"},
    {"1", "aXc"},
    {"2", "aXcd"},
};

//  Disjoint concurrent edits then a merge (the DIS-003 identity union).
static dagnode const dis_nodes[] = {
    {"",   "abc"},
    {"0",  "Abc"},
    {"0",  "abC"},
    {"12", "AbC"},
};

//  crisscross: node 7 reaches node 2 by two paths — the shape that
//  duplicated tokens before DIS-043.
static dagnode const cc_nodes[] = {
    {"",   "88"},
    {"0",  "ae"},
    {"0",  "ar"},
    {"1",  "bd"},
    {"1",  "ad"},
    {"23", "ec"},
    {"24", "ab"},
    {"56", "ab"},
};

//  crash_597 (minimised fuzz corpus): a five-parent fold-merge.
static dagnode const c597_nodes[] = {
    {"",      "a_aa888888"},
    {"0",     "aae"},
    {"0",     "aar"},
    {"1",     "bcde"},
    {"1",     "abcdd"},
    {"1",     "de"},
    {"23",    "ecbcde"},
    {"1",     "e"},
    {"24",    "abcde"},
    {"23",    "ecbcde"},
    {"1",     "e"},
    {"56789", "abcde"},
};

//  DIS-044: node 0's id outranks its edits', so the sibling tie-break is
//  forced to be a CAUSAL rank rather than the raw id.
static dagnode const d044_nodes[] = {
    {"",   "a"},
    {"0",  "g"},
    {"0",  "A"},
    {"12", "X"},
};

//  DIS-045: a blank line beside an inserted line — identical-content
//  tokens at a merge boundary.
static dagnode const d045_nodes[] = {
    {"",   "a_b"},
    {"0",  "aX_b"},
    {"0",  "A_b"},
    {"0",  "a_B"},
    {"12", "AX_b"},
    {"13", "aX_B"},
    {"45", "AX_B"},
};

static dagnode const d045b_nodes[] = {
    {"",   "a__b"},
    {"0",  "a_X_b"},
    {"0",  "A__b"},
    {"12", "A_X_b"},
};

//  Deletion cases: the whole reason the format changed.
static dagnode const del_nodes[] = {
    {"",   "abcd"},
    {"0",  "ad"},        // remove the run b,c
    {"0",  "abXcd"},     // insert INTERIOR to that run
    {"12", "aXd"},
};

static dagnode const abut_nodes[] = {
    {"",   "abcd"},
    {"0",  "ad"},
    {"0",  "aXbcd"},     // insert merely ABUTTING the removed run
    {"12", "aXd"},
};

static dagnode const cdel_nodes[] = {
    {"",   "abc"},
    {"0",  "ac"},        // both branches remove the SAME token
    {"0",  "ac"},
    {"12", "ac"},
};

static dagcase const cases[] = {
    {"linear",             lin_nodes,   4},
    {"disjoint_merge",     dis_nodes,   4},
    {"crisscross",         cc_nodes,    8},
    {"crash_597",          c597_nodes, 12},
    {"dis044_spine_above", d044_nodes,  4},
    {"dis045_insert_blank", d045_nodes, 7},
    {"dis045_double_blank", d045b_nodes, 4},
    {"interior_insert",    del_nodes,   4},
    {"abutting_insert",    abut_nodes,  4},
    {"concurrent_delete",  cdel_nodes,  4},
};

ok64 CFOLDparitytest() {
    sane(1);
    u32 n = (u32)(sizeof(cases) / sizeof(cases[0]));
    for (u32 i = 0; i < n; i++) call(parity_case, &cases[i]);
    call(random_sweep);
    done;
}

TEST(CFOLDparitytest);
