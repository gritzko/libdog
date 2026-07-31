//
//  CFOLD fuzz (DIS-082) — DAG content recovery on the append-only
//  dog/CFOLD.  Same input format as the retired fuzz/WEAVE.c, so the
//  existing DWEAVE corpus drives this one unchanged:
//
//    <parent-refs><space><content>\n   per revision, RON64 bytes,
//    parent refs are 0-based indices of PRECEDING lines, content non-empty,
//    single root (only line 0 may have no parents).
//
//  Each content byte b becomes the line "b\n" ('_' becomes a BLANK line),
//  so repeated bytes are IDENTICAL tokens — the positional ambiguity that
//  exposed the DIS-003/043/045 holdouts.  Commit ids are SplitMix64 of the
//  line index: ARBITRARY and non-monotonic, so a base routinely outranks
//  its edits (the DIS-044 toggle a monotonic counter hides).
//
//  PROPERTY: for every line a, CFOLDProduce(A, a) out of the ONE weave
//  must equal lineform(Ca).  The input content IS the oracle; a mismatch
//  is a real finding, never a property to weaken.
//
#include "dog/CFOLD.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/RON.h"
#include "abc/TEST.h"

#define AW_MAX_LINES   24u
#define AW_MAX_CONTENT 64u
#define AW_FUZZ_MAX    4096u
#define AW_WEAVE_CAP   (1UL << 19)

typedef struct {
    u8cs content;
    u32  par[AW_MAX_LINES];
    u32  npar;
} aw_line;

static u64 aw_cid(u32 i) {
    u64 x = (u64)i + 0x9E3779B97F4A7C15ULL;
    x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27; x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

static void aw_closure(aw_line const *ln, u32 start, u8 *anc) {
    anc[start] = 1;
    for (u32 i = start + 1; i-- > 0;) {
        if (!anc[i]) continue;
        for (u32 k = 0; k < ln[i].npar; k++) anc[ln[i].par[k]] = 1;
    }
}

static ok64 aw_parse(u8cs input, aw_line *ln, u32 *nlines) {
    sane($ok(input));
    u32  count = 0;
    u8cp p = input[0], e = input[1];
    while (p < e) {
        if (count >= AW_MAX_LINES) return CFOLDFAIL;
        u8cp eol = p;
        while (eol < e && *eol != '\n') eol++;
        u8cp sp = p;
        while (sp < eol && *sp != ' ') sp++;
        if (sp == eol) return CFOLDFAIL;
        aw_line *l = &ln[count];
        l->npar = 0;
        for (u8cp r = p; r < sp; r++) {
            u8 v = RON64_REV[*r];
            if (v == 0xff || v >= count) return CFOLDFAIL;
            if (l->npar >= AW_MAX_LINES) return CFOLDFAIL;
            l->par[l->npar++] = v;
        }
        u8cp cb = sp + 1;
        if ((u32)(eol - cb) > AW_MAX_CONTENT || cb == eol) return CFOLDFAIL;
        for (u8cp r = cb; r < eol; r++)
            if (RON64_REV[*r] == 0xff) return CFOLDFAIL;
        if (count > 0 && l->npar == 0) return CFOLDFAIL;
        l->content[0] = cb;
        l->content[1] = eol;
        count++;
        p = (eol < e) ? eol + 1 : eol;
    }
    if (count == 0) return CFOLDFAIL;
    *nlines = count;
    done;
}

//  Each byte b -> the line "b\n"; '_' -> a BLANK line (DIS-045 teeth).
static ok64 aw_lineform(u8b dst, u8csc src) {
    sane(1);
    u8bReset(dst);
    $for(u8c, c, src) {
        if (*c != '_') call(u8bFeed1, dst, *c);
        call(u8bFeed1, dst, (u8)'\n');
    }
    done;
}

static ok64 aw_run(aw_line const *ln, u32 n) {
    sane(n > 0);
    a_cstr(ext, "c");
    u8 *nbuf[AW_MAX_LINES][4] = {};
    ok64 ar;
    a_carve(u8, lf, 4 * AW_MAX_CONTENT + 16);
    a_carve(u64, anc, AW_MAX_LINES + 1);
    cfold A = {};

    for (u32 i = 0; i < n; i++) {
        if ((ar = u8bAcquire(ABC_BASS, nbuf[i], AW_WEAVE_CAP)) != OK) return ar;
        call(aw_lineform, lf, ln[i].content);
        u8csc v = {u8bDataHead(lf), u8bDataHead(lf) + u8bDataLen(lf)};
        u8 cl[AW_MAX_LINES] = {};
        aw_closure(ln, i, cl);
        u64bReset(anc);
        for (u32 j = 0; j < i; j++)
            if (cl[j]) call(u64bFeed1, anc, aw_cid(j));
        call(CFOLDFold, u8bIdle(nbuf[i]), i ? &A : NULL, v, ext, aw_cid(i),
             u64bDataC(anc));
        call(CFOLDParse, &A, u8bDataC(nbuf[i]));
    }

    a_carve(u8, nb, 1UL << 16);
    for (u32 a = 0; a < n; a++) {
        call(CFOLDProduce, &A, a, nb, NULL);
        call(aw_lineform, lf, ln[a].content);
        must(u8bDataLen(nb) == u8bDataLen(lf), "rev length off the oracle");
        must(!u8bDataLen(nb) ||
                 !memcmp(u8bDataHead(nb), u8bDataHead(lf), u8bDataLen(nb)),
             "rev bytes off the oracle");
    }
    done;
}

FUZZ(u8, CFOLDfuzz) {
    sane(1);
    if ($empty(input) || $len(input) > AW_FUZZ_MAX) done;
    static _Thread_local aw_line lines[AW_MAX_LINES];
    u32 n = 0;
    if (aw_parse(input, lines, &n) != OK) done;
    call(aw_run, lines, n);
    done;
}
