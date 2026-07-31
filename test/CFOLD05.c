//
//  CFOLD05 (DIS-082, was WEAVE02/DOG-004) — CFOLDEmit{Diff,Full} as HUNK
//  records: URI + body text + per-token diff side, asserted byte-for-byte.
//  `from`/`to` are commit INDICES (a rev's visibility is stored; the old
//  scope bitmaps are gone).  The Diff/Full expectations are byte-identical
//  to the WEAVE02 originals.  Differences from that test:
//    * fenced merge renders are RETIRED (DIS-080) — merged bytes come from
//      CFOLDProduce at the merge commit (covered by CFOLD01/CFOLD03);
//    * a >16 MiB body cannot exist: the FOLD refuses it with CFOLDBIG, so
//      the DIFF-007 coarse fallback has nothing to fall back from.
//
#include "dog/CFOLD.h"

#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

// --- a captured hunk: uri + text + a compact side-delta string -------
typedef struct {
    char uri[256];
    char text[4096];
    char sides[1024];   // "=1 =1 +1 -1 ..." (side char + offset delta)
    b8   has_verb;      // a status row carries a verb, empty body
} caphunk;

#define MAXCAP 16
typedef struct { caphunk h[MAXCAP]; u32 n; } capctx;

static ok64 cap_cb(hunkc *hk, void *vctx) {
    sane(hk != NULL);
    capctx *c = (capctx *)vctx;
    if (c->n >= MAXCAP) fail(TESTFAIL);
    caphunk *o = &c->h[c->n++];
    memset(o, 0, sizeof(*o));
    o->has_verb = (hk->verb != 0);
    u32 ul = (u32)$len(hk->uri);
    if (ul >= sizeof(o->uri)) ul = sizeof(o->uri) - 1;
    memcpy(o->uri, hk->uri[0], ul); o->uri[ul] = 0;
    u32 tl = (u32)$len(hk->text);
    if (tl >= sizeof(o->text)) tl = sizeof(o->text) - 1;
    memcpy(o->text, hk->text[0], tl); o->text[tl] = 0;
    char *s = o->sides; u32 cap = sizeof(o->sides), used = 0, prev = 0;
    $for(tok32c, t, hk->toks) {
        u8 sd = tok32Side(*t);
        char ch = sd == TOK_SIDE_IN ? '+' : sd == TOK_SIDE_RM ? '-' : '=';
        u32 off = tok32Offset(*t);
        int w = snprintf(s + used, cap - used, "%c%u ", ch, off - prev);
        if (w < 0 || (u32)w >= cap - used) break;
        used += (u32)w; prev = off;
    }
    done;
}

static ok64 expect(char const *lbl, caphunk const *got, b8 has_verb,
                   char const *uri, char const *text, char const *sides) {
    sane(1);
    if (got->has_verb != has_verb) {
        fprintf(stderr, " %s verb mismatch (%d vs %d)\n", lbl, got->has_verb, has_verb);
        fail(TESTFAIL);
    }
    if (uri && strcmp(got->uri, uri)) {
        fprintf(stderr, " %s URI mismatch\n  got [%s]\n  want[%s]\n", lbl, got->uri, uri);
        fail(TESTFAIL);
    }
    if (text && strcmp(got->text, text)) {
        fprintf(stderr, " %s TEXT mismatch\n  got [%s]\n  want[%s]\n", lbl, got->text, text);
        fail(TESTFAIL);
    }
    if (sides && strcmp(got->sides, sides)) {
        fprintf(stderr, " %s SIDES mismatch\n  got [%s]\n  want[%s]\n", lbl, got->sides, sides);
        fail(TESTFAIL);
    }
    done;
}

#define LIT(s) {(u8c *)(s), (u8c *)(s) + sizeof(s) - 1}

//  Fold v2 onto v1 (commits 1, 2 in build order); the weave lands in `w`,
//  whose blobs persist in the CALLER's frame (invoke directly, not call()).
static ok64 two_fold(cfold *w, u8csc v1, u8csc v2, u8csc ext) {
    sane(w);
    a_carve(u64, anc, 4);
    a_carve(u8, w1b, 1UL << 18);
    call(CFOLDFold, u8bIdle(w1b), NULL, v1, ext, 1, u64bDataC(anc));
    call(CFOLDParse, w, u8bDataC(w1b));
    call(u64bFeed1, anc, 1);
    a_carve(u8, w2b, 1UL << 18);
    call(CFOLDFold, u8bIdle(w2b), w, v2, ext, 2, u64bDataC(anc));
    call(CFOLDParse, w, u8bDataC(w2b));
    done;
}

//  v1 -> v2 single-line edit; from = rev 0, to = rev 1.  Whole file is one
//  window (change within 3-line context), so Diff == Full body.
static ok64 emit_singlewin(void) {
    sane(1);
    fprintf(stderr, "  emit_singlewin ...");
    a_cstr(cext, "c"); a_cstr(name, "foo.c"); a_cstr(nav, "deadbeef");
    u8csc v1 = LIT("a\nb\nc\nd\ne\nf\ng\n");
    u8csc v2 = LIT("a\nb\nc\nX\ne\nf\ng\n");
    cfold w = {};
    { ok64 r = two_fold(&w, v1, v2, cext); if (r != OK) return r; }

    char const *wanttext  = "a\nb\nc\nXd\ne\nf\ng\n";
    char const *wantsides = "=1 =1 =1 =1 =1 =1 +1 -1 =1 =1 =1 =1 =1 =1 =1 ";

    capctx cd = {};
    call(CFOLDEmitDiff, &w, name, nav, 0, 1, cap_cb, &cd);
    if (cd.n != 1) { fprintf(stderr, " diff want 1 hunk got %u\n", cd.n); fail(TESTFAIL); }
    call(expect, "Diff", &cd.h[0], NO, "diff:foo.c?deadbeef#L1", wanttext, wantsides);

    capctx cf = {};
    a_cstr(dsch, "diff:");
    call(CFOLDEmitFull, &w, name, dsch, nav, 0, 1, cap_cb, &cf);
    if (cf.n != 1) { fprintf(stderr, " full want 1 hunk got %u\n", cf.n); fail(TESTFAIL); }
    call(expect, "Full-diff", &cf.h[0], NO, "diff:foo.c?deadbeef#L1", wanttext, wantsides);

    //  cat: (empty scheme) — no scheme prefix on the URI.
    capctx cc = {};
    u8cs nosch = {};
    call(CFOLDEmitFull, &w, name, nosch, nav, 0, 1, cap_cb, &cc);
    if (cc.n != 1) { fprintf(stderr, " cat want 1 hunk got %u\n", cc.n); fail(TESTFAIL); }
    call(expect, "Full-cat", &cc.h[0], NO, "foo.c?deadbeef#L1", wanttext, wantsides);
    fprintf(stderr, " ok\n");
    done;
}

//  Two far-apart edits -> two windows -> two hunks.
static ok64 emit_twowin(void) {
    sane(1);
    fprintf(stderr, "  emit_twowin ...");
    a_cstr(cext, "c"); a_cstr(name, "foo.c"); a_cstr(nav, "deadbeef");
    u8csc v1 = LIT("l0\nl1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9\nl10\nl11\nl12\nl13\n");
    u8csc v2 = LIT("l0\nX1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9\nl10\nX11\nl12\nl13\n");
    cfold w = {};
    { ok64 r = two_fold(&w, v1, v2, cext); if (r != OK) return r; }

    capctx c = {};
    call(CFOLDEmitDiff, &w, name, nav, 0, 1, cap_cb, &c);
    if (c.n != 2) { fprintf(stderr, " want 2 hunks got %u\n", c.n); fail(TESTFAIL); }
    call(expect, "2W-a", &c.h[0], NO, "diff:foo.c?deadbeef#L1",
         "l0\nX1l1\nl2\nl3\nl4\n", "=2 =1 +2 -2 =1 =2 =1 =2 =1 =2 =1 ");
    call(expect, "2W-b", &c.h[1], NO, "diff:foo.c?deadbeef#L8",
         "l7\nl8\nl9\nl10\nX11l11\nl12\nl13\n",
         "=2 =1 =2 =1 =2 =1 =3 =1 +3 -3 =1 =3 =1 =3 =1 ");
    fprintf(stderr, " ok\n");
    done;
}

//  A >16 MiB body must be REFUSED at fold time (CFOLDBIG), never silently
//  truncated — the DIFF-007 "never silently empty" guarantee, moved to the
//  writer: an un-foldable file never becomes a half-weave.
static ok64 fold_capped(void) {
    sane(1);
    fprintf(stderr, "  fold_capped ...");
    a_cstr(text_ext, "txt");
    size_t linelen = 1024, nlines = (17UL << 20) / linelen + 1;
    a_carve(u8, blob, nlines * (linelen + 1) + 1);
    for (size_t k = 0; k < nlines; k++) {
        for (size_t b = 0; b < linelen; b++) call(u8bFeed1, blob, 'x');
        call(u8bFeed1, blob, '\n');
    }
    u8csc big = {u8bDataHead(blob), u8bDataHead(blob) + u8bDataLen(blob)};
    u64csc noanc = {NULL, NULL};
    a_carve(u8, wb, (20UL << 20));
    ok64 r = CFOLDFold(u8bIdle(wb), NULL, big, text_ext, 1, noanc);
    if (r != CFOLDBIG) {
        fprintf(stderr, " want CFOLDBIG, got %s\n", ok64str(r));
        fail(TESTFAIL);
    }
    fprintf(stderr, " ok\n");
    done;
}

ok64 CFOLD05test() {
    sane(1);
    call(emit_singlewin);
    call(emit_twowin);
    call(fold_capped);
    done;
}

TEST(CFOLD05test);
