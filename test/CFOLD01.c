//
//  CFOLD01 (DIS-082) — the append-only weave: codec, walk, and the two
//  stream properties.
//
//  The format's whole premise is two DIFFERENT properties, so both are
//  asserted by a run and neither by inspection:
//    * the BODY is strictly APPEND-ONLY — every fold re-hashes the previous
//      body's prefix and requires it byte-identical;
//    * the INDEX is PARENT-SORTED and REWRITTEN per commit — every fold
//      requires the result sorted, and requires the merge to have touched
//      each element exactly once (one sequential pass).
//
#include "dog/CFOLD.h"

#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "abc/RAP.h"
#include "abc/TEST.h"
#include "abc/TLV.h"

#define DAG_MAX 32

//  Each content char becomes its own line, so repeated chars are IDENTICAL
//  tokens (the positional stress); '_' is a BLANK line.
static ok64 lineform(u8b dst, char const *s) {
    sane(1);
    u8bReset(dst);
    for (char const *p = s; *p; p++) {
        if (*p != '_') call(u8bFeed1, dst, (u8)*p);
        call(u8bFeed1, dst, (u8)'\n');
    }
    done;
}

static ok64 same_bytes(char const *what, u8csc got, u8csc want) {
    sane(1);
    if ($len(got) != $len(want) ||
        ($len(want) && memcmp(got[0], want[0], (size_t)$len(want)))) {
        fprintf(stderr, "\n    %s: want(%ld) ", what, (long)$len(want));
        $for(u8c, c, want) fputc(*c == '\n' ? '.' : *c, stderr);
        fprintf(stderr, " got(%ld) ", (long)$len(got));
        $for(u8c, c, got) fputc(*c == '\n' ? '.' : *c, stderr);
        fputc('\n', stderr);
        fail(TESTFAIL);
    }
    done;
}

//  The index must be PARENT-SORTED after every commit's merge.  (There is
//  deliberately no prefix-stability check here: the index IS rewritten.)
static ok64 index_sorted(cfold const *w) {
    sane(w);
    u32 n = (u32)$len(w->idx);
    for (u32 i = 1; i < n; i++) {
        tokv32 a = w->idx[0][i - 1], b = w->idx[0][i];
        if (tokv32Z(&b, &a)) {
            fprintf(stderr, "\n    index out of order at %u\n", i);
            fail(TESTFAIL);
        }
    }
    done;
}

//  One fold, with both stream properties checked against the PREVIOUS state.
static ok64 fold_checked(u8b into, cfold const *prev, u8csc blob, u8csc ext,
                         u64 commit, u64csc anc) {
    sane(into);
    u32 oldlen = prev ? (u32)u8csLen(prev->body) : 0;
    u64 oldh   = 0;
    if (prev) oldh = RAPHash(prev->body);
    cffold fs = {};
    call(CFOLDFoldStat, u8bIdle(into), prev, blob, ext, commit, anc, &fs);
    cfold nw = {};
    call(CFOLDParse, &nw, u8bDataC(into));
    //  BODY: strictly append-only — the old bytes are still exactly there.
    if (fs.body_before != oldlen || fs.body_after < oldlen) {
        fprintf(stderr, "\n    body shrank: %u -> %u\n",
                fs.body_before, fs.body_after);
        fail(TESTFAIL);
    }
    a_part(u8c, pre, nw.body, 0, oldlen);
    if (oldlen && RAPHash(pre) != oldh) {
        fprintf(stderr, "\n    BODY PREFIX MUTATED over %u bytes\n", oldlen);
        fail(TESTFAIL);
    }
    //  INDEX: re-sorted, and the merge was one sequential pass.
    call(index_sorted, &nw);
    if (fs.merge_steps != fs.entries_old + fs.entries_new) {
        fprintf(stderr, "\n    index merge took %u steps for %u+%u\n",
                fs.merge_steps, fs.entries_old, fs.entries_new);
        fail(TESTFAIL);
    }
    done;
}

// =====================================================================
//  From-blob round trip
// =====================================================================

static ok64 rt_case(char const *name, u8csc blob, u8csc ext) {
    sane(1);
    fprintf(stderr, "  rt_%s ...", name);
    a_carve(u8, wbuf, 1UL << 20);
    u64csc noanc = {NULL, NULL};
    call(fold_checked, wbuf, NULL, blob, ext, 0xC0FFEEu, noanc);
    cfold w = {};
    call(CFOLDParse, &w, u8bDataC(wbuf));
    a_carve(u8, alive, 1UL << 20);
    call(CFOLDAlive, &w, alive);
    call(same_bytes, "alive", u8bDataC(alive), blob);
    //  A from-blob weave is ONE fork-free chain, so the walk must never
    //  binary-search: the efficiency claim, measured.
    cfstat st = {};
    a_carve(u8, again, 1UL << 20);
    call(CFOLDProduce, &w, 0, again, &st);
    if (st.bsearch != 0) {
        fprintf(stderr, "\n    chain walk did %u binary searches\n", st.bsearch);
        fail(TESTFAIL);
    }
    fprintf(stderr, " ok (%u groups, 0 bsearch)\n", st.groups);
    done;
}

typedef struct { char const *name; char const *blob; } RTcase;

static RTcase rtcases[] = {
    {"empty",       ""},
    {"one_token",   "abc"},
    {"one_line",    "int x = 1;\n"},
    {"no_trailing", "int x = 1;"},
    {"multi_line",  "a\nb\nc\n"},
    {"c_snippet",   "int main(void){\n    return 0;\n}\n"},
    {"blank_lines", "\n\n\nx\n"},
};

// =====================================================================
//  DAG runner: fold every node into ONE weave, then recover every rev
// =====================================================================
//  A merge needs no separate operation here — a node is one commit whose
//  ancestor closure IS the union of its parents', so the ignore-set it gets
//  is exactly the INTERSECTION of what the parents could not see.

typedef struct { char const *par; char const *content; } dagnode;
typedef struct {
    char const *name;
    dagnode const *nodes;
    u32 n;
    u32 max_ign;        // the ignore-set must never exceed this
} dagcase;

static u64 dag_cid(u32 i) {
    u64 x = (u64)i + 0x9E3779B97F4A7C15ULL;
    x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27; x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

//  Ancestor closure of `start` (parents always precede), `start` included.
static void dag_closure(dagnode const *nd, u32 start, u8 *anc) {
    anc[start] = 1;
    for (u32 i = start + 1; i-- > 0;) {
        if (!anc[i]) continue;
        for (char const *r = nd[i].par; *r; r++) anc[(u8)(*r - '0')] = 1;
    }
}

static ok64 dag_run(dagcase const *dc) {
    sane(1);
    fprintf(stderr, "  %s ...", dc->name);
    a_cstr(cext, "c");
    must(dc->n <= DAG_MAX, "dag too big");
    //  BASS acquires must SURVIVE the loop, so acquire directly (a call()
    //  would rewind them) and keep one blob per generation.
    u8 *wbuf[DAG_MAX][4] = {};
    a_carve(u8, lf, 1UL << 16);
    a_carve(u64, anc, DAG_MAX + 1);
    ok64 ar;
    cfold cur = {};
    u32 maxign = 0;
    for (u32 i = 0; i < dc->n; i++) {
        if ((ar = u8bAcquire(ABC_BASS, wbuf[i], 1UL << 19)) != OK) return ar;
        call(lineform, lf, dc->nodes[i].content);
        u8csc v = {u8bDataHead(lf), u8bDataHead(lf) + u8bDataLen(lf)};
        u8 cl[DAG_MAX] = {};
        dag_closure(dc->nodes, i, cl);
        u64bReset(anc);
        for (u32 j = 0; j < i; j++)
            if (cl[j]) call(u64bFeed1, anc, dag_cid(j));
        call(fold_checked, wbuf[i], i ? &cur : NULL, v, cext, dag_cid(i),
             u64bDataC(anc));
        call(CFOLDParse, &cur, u8bDataC(wbuf[i]));
        cfcommit ac = {};
        call(CFOLDCommitAt, &ac, &cur, i);
        if (ac.ign_len > maxign) maxign = ac.ign_len;
    }
    //  Every rev must materialize its own content, byte for byte, out of
    //  the ONE weave — and P_c must agree with the walk (CFOLDProduce
    //  fails with CFOLDPLEN otherwise).
    a_carve(u8, out, 1UL << 16);
    for (u32 a = 0; a < dc->n; a++) {
        call(CFOLDProduce, &cur, a, out, NULL);
        call(lineform, lf, dc->nodes[a].content);
        call(same_bytes, dc->nodes[a].content, u8bDataC(out), u8bDataC(lf));
    }
    //  BLAME: every token's body offset resolves to the commit that
    //  appended it, by range binary search alone.
    for (u32 k = 0; k < (u32)$len(cur.idx); k++) {
        tokv32 e  = cur.idx[0][k];
        u8     tg = tok32Tag(e.chi);
        if (tg == CFOLD_TAG_TERM || tg == CFOLD_TAG_TOMB) continue;
        u32     ci = 0;
        cfcommit c  = {};
        call(CFOLDBlame, &ci, &cur, tok32Offset(e.chi));
        call(CFOLDCommitAt, &c, &cur, ci);
        if (tok32Offset(e.chi) < c.start || tok32Offset(e.chi) >= c.end) {
            fprintf(stderr, "\n    blame off range\n");
            fail(TESTFAIL);
        }
    }
    //  DIS-082: id -> build index round-trips for every folded commit; an
    //  id never folded must come back CFOLDNOCM, never a stale index.
    for (u32 a = 0; a < dc->n; a++) {
        u32 fi = 0;
        call(CFOLDFindCommit, &fi, &cur, dag_cid(a));
        if (fi != a) {
            fprintf(stderr, "\n    FindCommit %u -> %u\n", a, fi);
            fail(TESTFAIL);
        }
    }
    {
        u32 fi = 0;
        if (CFOLDFindCommit(&fi, &cur, dag_cid(dc->n)) != CFOLDNOCM) {
            fprintf(stderr, "\n    found a commit never folded\n");
            fail(TESTFAIL);
        }
    }
    //  The scope story rests on the ignore-set staying small: LINEAR
    //  history must produce none at all, branchy history only a few.
    if (maxign > dc->max_ign) {
        fprintf(stderr, "\n    max ignore-set %u, cap %u\n",
                maxign, dc->max_ign);
        fail(TESTFAIL);
    }
    fprintf(stderr, " ok (%u entries, %u body, max ignore %u)\n",
            (u32)$len(cur.idx), (u32)u8csLen(cur.body), maxign);
    done;
}

//  LINEAR history: nothing is ever concurrent, so every ignore-set is
//  EMPTY and visibility is one integer compare.
static dagnode const lin_nodes[] = {
    {"",  "abc"},
    {"0", "abXc"},
    {"1", "aXc"},
    {"2", "aXcd"},
};

//  FORK: three concurrent children of one parent, all inserting at the
//  same anchor, then a node that sees all three.
static dagnode const fork_nodes[] = {
    {"",    "ab"},
    {"0",   "aXb"},
    {"0",   "aYb"},
    {"0",   "aZb"},
    {"123", "aXYZb"},
};

//  CONCURRENT DELETE: two branches tombstone the SAME token.  Two tomb
//  children of one target; removals never conflict (/wiki/Dirty), and the
//  base rev must still recover the token.
static dagnode const cdel_nodes[] = {
    {"",   "abc"},
    {"0",  "ac"},
    {"0",  "ac"},
    {"12", "ac"},
};

//  INTERIOR INSERTION: branch 1 removes the run b,c; branch 2 inserts X
//  BETWEEN b and c, so the insertion's neighbours are both tombstoned.
static dagnode const inter_nodes[] = {
    {"",   "abcd"},
    {"0",  "ad"},
    {"0",  "abXcd"},
    {"12", "aXd"},
};

//  ABUTTING INSERTION: same removed run, but branch 2 inserts X right
//  after the last survivor before the hole — a clean merge, no interior.
static dagnode const abut_nodes[] = {
    {"",   "abcd"},
    {"0",  "ad"},
    {"0",  "aXbcd"},
    {"12", "aXd"},
};

//  CHAIN TERMINATORS AT FORK POINTS: several multi-token chains hang off
//  one anchor, so each chain's end has to be explicit.
static dagnode const term_nodes[] = {
    {"",    "ab"},
    {"0",   "aPQRb"},
    {"0",   "aXYZb"},
    {"12",  "aPQRXYZb"},
    {"3",   "aPQRXYZbW"},
};

//  BRANCHY: a criss-cross where a rev must hide commits it never saw, so
//  the ignore-sets are non-empty and the fast path is off.
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

//  Blank-line stress: identical-content tokens next to an insert.
static dagnode const blank_nodes[] = {
    {"",   "a_b"},
    {"0",  "aX_b"},
    {"0",  "A_b"},
    {"0",  "a_B"},
    {"12", "AX_b"},
    {"13", "aX_B"},
    {"45", "AX_B"},
};

static dagcase const dagcases[] = {
    {"linear_no_ignore",  lin_nodes,   4, 0},
    {"fork_three_kids",   fork_nodes,  5, 2},
    {"concurrent_delete", cdel_nodes,  4, 1},
    {"interior_insert",   inter_nodes, 4, 1},
    {"abutting_insert",   abut_nodes,  4, 1},
    {"chain_terminators", term_nodes,  5, 1},
    {"crisscross_branchy", cc_nodes,   8, 2},
    {"blank_line_stress", blank_nodes, 7, 4},
};

// =====================================================================
//  A pure merge commit: appends NOTHING, only takes the later L and the
//  INTERSECTED ignore-set.  The case that tells intersection from union:
//  a token only ONE parent ever saw must survive the merge.
// =====================================================================

static ok64 merge_intersects(void) {
    sane(1);
    fprintf(stderr, "  merge_intersects_ignore ...");
    a_cstr(cext, "c");
    a_carve(u8, lf, 1UL << 14);
    u8 *wb[4][4] = {};
    ok64 ar;
    for (u32 i = 0; i < 4; i++)
        if ((ar = u8bAcquire(ABC_BASS, wb[i], 1UL << 18)) != OK) return ar;
    a_carve(u64, anc, 8);
    cfold w = {};

    //  0: base "ab"
    call(lineform, lf, "ab");
    u8csc v0 = {u8bDataHead(lf), u8bDataHead(lf) + u8bDataLen(lf)};
    call(CFOLDFold, u8bIdle(wb[0]), NULL, v0, cext, dag_cid(0),
         u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[0]));
    //  1: ours "aXb"   (anc {0})
    call(lineform, lf, "aXb");
    u8csc v1 = {u8bDataHead(lf), u8bDataHead(lf) + u8bDataLen(lf)};
    u64bReset(anc); call(u64bFeed1, anc, dag_cid(0));
    call(CFOLDFold, u8bIdle(wb[1]), &w, v1, cext, dag_cid(1),
         u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[1]));
    //  2: theirs "abY"  (anc {0}) — concurrent with 1, so 2 IGNORES 1.
    call(lineform, lf, "abY");
    u8csc v2 = {u8bDataHead(lf), u8bDataHead(lf) + u8bDataLen(lf)};
    u64bReset(anc); call(u64bFeed1, anc, dag_cid(0));
    call(CFOLDFold, u8bIdle(wb[2]), &w, v2, cext, dag_cid(2),
         u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[2]));
    {
        cfcommit c = {};
        call(CFOLDCommitAt, &c, &w, 2);
        if (c.ign_len != 1) {
            fprintf(stderr, "\n    theirs should ignore ours (%u)\n", c.ign_len);
            fail(TESTFAIL);
        }
    }
    //  3: PURE MERGE of 1 and 2 — appends nothing at all.  X was seen only
    //  by parent 1 and Y only by parent 2; a UNION of the ignore-sets would
    //  hide one of them, the INTERSECTION keeps both.
    u64bReset(anc);
    call(u64bFeed1, anc, dag_cid(0));
    call(u64bFeed1, anc, dag_cid(1));
    call(u64bFeed1, anc, dag_cid(2));
    call(CFOLDMerge, u8bIdle(wb[3]), &w, dag_cid(3), u64bDataC(anc));
    cfold m = {};
    call(CFOLDParse, &m, u8bDataC(wb[3]));
    {
        cfcommit c = {};
        call(CFOLDCommitAt, &c, &m, 3);
        if (c.ign_len != 0) {
            fprintf(stderr, "\n    merge ignore-set %u, wanted 0\n", c.ign_len);
            fail(TESTFAIL);
        }
        if (c.start != c.end) {
            fprintf(stderr, "\n    pure merge appended %u bytes\n",
                    c.end - c.start);
            fail(TESTFAIL);
        }
        //  and the index did not grow either
        cfold p = {};
        call(CFOLDParse, &p, u8bDataC(wb[2]));
        if ($len(m.idx) != $len(p.idx)) {
            fprintf(stderr, "\n    pure merge added index entries\n");
            fail(TESTFAIL);
        }
    }
    a_carve(u8, out, 1UL << 14);
    call(CFOLDProduce, &m, 3, out, NULL);
    call(lineform, lf, "aXbY");
    call(same_bytes, "merge", u8bDataC(out), u8bDataC(lf));
    //  each parent still recovers its own view
    call(CFOLDProduce, &m, 1, out, NULL);
    call(lineform, lf, "aXb");
    call(same_bytes, "ours", u8bDataC(out), u8bDataC(lf));
    call(CFOLDProduce, &m, 2, out, NULL);
    call(lineform, lf, "abY");
    call(same_bytes, "theirs", u8bDataC(out), u8bDataC(lf));
    fprintf(stderr, " ok\n");
    done;
}

// =====================================================================
//  /wiki/Dirty §"What counts as a conflict": an insertion INTERIOR to a
//  removed run conflicts, one merely ABUTTING it merges clean.  The format
//  has to make those two STRUCTURALLY different, and the only thing that
//  can tell them apart is the inserted token's CT parent: interior means
//  the parent is itself tombstoned, abutting means it is not.  (Classifying
//  that as `cnf` is a caller's job — this asserts the structure is there.)
// =====================================================================

//  Body offset of the first token whose text starts with `c`.
static ok64 find_tok(u32 *out, cfold const *w, u8 c) {
    sane(out);
    $for(u8c, p, w->body) if (*p == c) {
        *out = (u32)(p - w->body[0]);
        done;
    }
    fail(TESTFAIL);
}

//  Is the CT parent of the token at `off` carrying a tomb?
static ok64 parent_tombed(b8 *out, cfold const *w, u32 off) {
    sane(out);
    *out = NO;
    tok32 par = 0;
    b8    got = NO;
    for (u32 i = 0; i < (u32)$len(w->idx); i++) {
        tokv32 e  = w->idx[0][i];
        u8     tg = tok32Tag(e.chi);
        if (tg == CFOLD_TAG_TERM || tg == CFOLD_TAG_TOMB) continue;
        if (tok32Offset(e.chi) == off) { par = e.par; got = YES; break; }
    }
    if (!got) fail(TESTFAIL);
    if (par == CFOLD_ROOT) done;         // at file start: nothing to tomb
    for (u32 i = 0; i < (u32)$len(w->idx); i++) {
        tokv32 e = w->idx[0][i];
        if (e.par == par && tok32Tag(e.chi) == CFOLD_TAG_TOMB) {
            *out = YES;
            done;
        }
    }
    done;
}

//  Fold base / remover / inserter and report whether the inserted token's
//  parent ended up tombstoned.
static ok64 dirty_shape(b8 *out, char const *base, char const *removed,
                        char const *inserted) {
    sane(out);
    a_cstr(cext, "c");
    a_carve(u8, lf, 1UL << 14);
    u8 *wb[3][4] = {};
    ok64 ar;
    for (u32 i = 0; i < 3; i++)
        if ((ar = u8bAcquire(ABC_BASS, wb[i], 1UL << 18)) != OK) return ar;
    a_carve(u64, anc, 4);
    cfold w = {};
    char const *txt[3] = {base, removed, inserted};
    for (u32 i = 0; i < 3; i++) {
        call(lineform, lf, txt[i]);
        u8csc v = {u8bDataHead(lf), u8bDataHead(lf) + u8bDataLen(lf)};
        u64bReset(anc);
        if (i) call(u64bFeed1, anc, dag_cid(0));   // 1 and 2 are CONCURRENT
        call(CFOLDFold, u8bIdle(wb[i]), i ? &w : NULL, v, cext, dag_cid(i),
             u64bDataC(anc));
        call(CFOLDParse, &w, u8bDataC(wb[i]));
    }
    u32 xoff = 0;
    call(find_tok, &xoff, &w, 'X');
    call(parent_tombed, out, &w, xoff);
    done;
}

static ok64 dirty_interior_vs_abutting(void) {
    sane(1);
    fprintf(stderr, "  dirty_interior_vs_abutting ...");
    b8 interior = NO, abutting = NO;
    //  branch 1 removes the run b,c; branch 2 inserts X between b and c
    call(dirty_shape, &interior, "abcd", "ad", "abXcd");
    //  same removal, but X only ABUTS it (right after the last survivor)
    call(dirty_shape, &abutting, "abcd", "ad", "aXbcd");
    if (!interior) {
        fprintf(stderr, "\n    interior insertion has an untombed parent\n");
        fail(TESTFAIL);
    }
    if (abutting) {
        fprintf(stderr, "\n    abutting insertion anchored INSIDE the hole\n");
        fail(TESTFAIL);
    }
    fprintf(stderr, " ok (interior parent tombed, abutting parent alive)\n");
    done;
}

// =====================================================================
//  Malformed input must be rejected with a NAMED error, never a crash.
// =====================================================================

static ok64 reject_bad(void) {
    sane(1);
    fprintf(stderr, "  reject_malformed ...");
    cfold w = {};
    a_cstr(nonsense, "not a weave at all");
    if (CFOLDParse(&w, nonsense) == OK) {
        fprintf(stderr, "\n    accepted garbage\n");
        fail(TESTFAIL);
    }
    //  a 'V' whose index is not a whole number of tokv32
    a_carve(u8, blob, 1UL << 12);
    a_carve(u8, inner, 1UL << 12);
    a_cstr(three, "abc");
    call(TLVu8sFeed, u8bIdle(inner), CFOLD_TLV_IDX, three);
    call(TLVu8sFeed, u8bIdle(blob), CFOLD_TLV, u8bDataC(inner));
    if (CFOLDParse(&w, u8bDataC(blob)) != CFOLDFAIL) {
        fprintf(stderr, "\n    accepted a torn index\n");
        fail(TESTFAIL);
    }
    //  blame outside the body has no commit
    cfold e = {};
    u32    ci = 0;
    if (CFOLDBlame(&ci, &e, 0) != CFOLDNOCM) {
        fprintf(stderr, "\n    blamed an empty weave\n");
        fail(TESTFAIL);
    }
    fprintf(stderr, " ok\n");
    done;
}

ok64 CFOLDtest() {
    sane(1);
    a_cstr(cext, "c");
    u32 n = (u32)(sizeof(rtcases) / sizeof(rtcases[0]));
    for (u32 i = 0; i < n; i++) {
        u8csc blob = {(u8c *)rtcases[i].blob,
                      (u8c *)rtcases[i].blob + strlen(rtcases[i].blob)};
        call(rt_case, rtcases[i].name, blob, cext);
    }
    u32 nd = (u32)(sizeof(dagcases) / sizeof(dagcases[0]));
    for (u32 i = 0; i < nd; i++) call(dag_run, &dagcases[i]);
    call(merge_intersects);
    call(dirty_interior_vs_abutting);
    call(reject_bad);
    done;
}

TEST(CFOLDtest);
