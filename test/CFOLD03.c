//
//  CFOLD03 (DIS-082, was WEAVE03/DIS-045) — blank-line vs code-line EOL
//  merge mis-anchor, on the append-only weave and REAL C content (the
//  other CFOLD tests feed one-char lines; this one runs the tokenizer on
//  code).  patch/15-ancestor-skip step 2: ours and theirs fold onto the
//  SAME F2 base, so the F2 spine — its blank line `\n` included — is
//  shared identity; theirs's added `divmod` line carries a trailing `\n`
//  with content identical to that spine blank.  The RENDERED MERGE of a
//  pure merge commit must still anchor the insert on the right side.
//
#include "dog/CFOLD.h"

#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

#define F2 \
    "#include <stdio.h>\n" \
    "\n" \
    "int add(int x, int y) { return x + y; }\n" \
    "int sub(int x, int y) { return x - y; }\n" \
    "int mul(int x, int y) { return x * y; }\n" \
    "\n" \
    "const char *greet = \"hi\";\n" \
    "const char *bye = \"bye\";\n" \
    "\n" \
    "int main(void) { return 0; }\n"
#define F3 \
    "#include <stdio.h>\n" \
    "\n" \
    "int add(int x, int y) { return x + y; }\n" \
    "int sub(int x, int y) { return x - y; }\n" \
    "int mul(int x, int y) { return x * y; }\n" \
    "int divmod(int a, int b) { return a / b; }\n" \
    "\n" \
    "const char *greet = \"hi\";\n" \
    "const char *bye = \"bye\";\n" \
    "\n" \
    "int main(void) { return 0; }\n"
#define WANT1 \
    "#include <stdio.h>\n" \
    "\n" \
    "int add(int x, int y) { return x + y; }\n" \
    "int sub(int x, int y) { return x - y; }\n" \
    "int mul(int x, int y) { return x * y; }\n" \
    "\n" \
    "const char *greet = \"hello\";\n" \
    "const char *bye = \"farewell\";\n" \
    "\n" \
    "int main(void) { return 0; }\n"
#define WANT2 \
    "#include <stdio.h>\n" \
    "\n" \
    "int add(int x, int y) { return x + y; }\n" \
    "int sub(int x, int y) { return x - y; }\n" \
    "int mul(int x, int y) { return x * y; }\n" \
    "int divmod(int a, int b) { return a / b; }\n" \
    "\n" \
    "const char *greet = \"hello\";\n" \
    "const char *bye = \"farewell\";\n" \
    "\n" \
    "int main(void) { return 0; }\n"

#define LIT(s) {(u8c *)(s), (u8c *)(s) + sizeof(s) - 1}

static void show(char const *lbl, u8c *p, size_t n) {
    fprintf(stderr, "  %s (%zu):\n----\n", lbl, n);
    for (size_t k = 0; k < n; k++) fputc(p[k], stderr);
    fprintf(stderr, "----\n");
}

static ok64 check_rev(char const *lbl, cfold const *w, u32 c, u8csc want) {
    sane(1);
    a_carve(u8, out, 1UL << 16);
    call(CFOLDProduce, w, c, out, NULL);
    size_t gl = u8bDataLen(out), wl = (size_t)$len(want);
    if (gl != wl || (wl && memcmp(u8bDataHead(out), want[0], wl))) {
        fprintf(stderr, " %s MISMATCH (%zu vs %zu)\n", lbl, gl, wl);
        show("got", u8bDataHead(out), gl);
        show("want", (u8c *)want[0], wl);
        fail(TESTFAIL);
    }
    done;
}

//  base F2, then ours(WANT1) and theirs(F3) CONCURRENT on it, then a pure
//  merge commit; the merge's render must equal 08.lib.want_step2.c.
static ok64 dis045_blank_eol(void) {
    sane(1);
    fprintf(stderr, "  dis045_blank_eol ...");
    a_cstr(cext, "c");
    enum { C_F2 = 4, C_OURS = 7, C_THEIRS = 5, C_MERGE = 9 };
    u8csc vF2 = LIT(F2), vW1 = LIT(WANT1), vF3 = LIT(F3), vW2 = LIT(WANT2);
    a_carve(u64, anc, 8);

    u8 *wb[4][4] = {};
    ok64 ar;
    for (u32 i = 0; i < 4; i++)
        if ((ar = u8bAcquire(ABC_BASS, wb[i], 1UL << 18)) != OK) return ar;
    cfold w = {};

    call(CFOLDFold, u8bIdle(wb[0]), NULL, vF2, cext, C_F2, u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[0]));

    u64bReset(anc); call(u64bFeed1, anc, C_F2);
    call(CFOLDFold, u8bIdle(wb[1]), &w, vW1, cext, C_OURS, u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[1]));

    u64bReset(anc); call(u64bFeed1, anc, C_F2);   // concurrent with ours
    call(CFOLDFold, u8bIdle(wb[2]), &w, vF3, cext, C_THEIRS, u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[2]));

    call(check_rev, "ours tip", &w, 1, vW1);
    call(check_rev, "theirs tip", &w, 2, vF3);

    u64bReset(anc);
    call(u64bFeed1, anc, C_F2);
    call(u64bFeed1, anc, C_OURS);
    call(u64bFeed1, anc, C_THEIRS);
    call(CFOLDMerge, u8bIdle(wb[3]), &w, C_MERGE, u64bDataC(anc));
    call(CFOLDParse, &w, u8bDataC(wb[3]));
    call(check_rev, "step2 merged", &w, 3, vW2);

    fprintf(stderr, " ok\n");
    done;
}

ok64 CFOLD03test() {
    sane(1);
    call(dis045_blank_eol);
    done;
}

TEST(CFOLD03test);
