#include "MKDT.h"

#include "abc/PRO.h"
#include "dog/tok/MDBLK.h"

// Inline ragel lexer (MKDT.rl.c, generated from MKDT.c.rl)
ok64 MKDTInlineLexer(MKDTstate *state);

// --- Inline callbacks ---

//  DOG-024: a span is markup + text, not one blob.  The delimiters emit as
//  their own 'G' tokens and the body is re-lexed one level down, so a nested
//  `_x_` inside `*a _x_ b*` is just more tokens and the words between them
//  diff and search on their own.  `mkdtg` (MKDTDecomposeSpan) does the
//  splitting — the same machine, no hand-cut bytes.
static ok64 MKDTEmitSpan(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb == NULL) done;
    mkdtspan g = {};
    if (state->depth >= MKDT_MAX_DEPTH || MKDTDecomposeSpan(&g, tok) != OK
        || g.kind == 0 || g.text[0] == NULL) {
        call(state->cb, 'G', tok, state->ctx);   //  too deep / off-shape
        done;
    }
    u8cs open = {tok[0], g.text[0]};
    u8cs close = {g.text[1], tok[1]};
    if (!u8csEmpty(open)) call(state->cb, 'G', open, state->ctx);
    if (!u8csEmpty(g.text)) {
        MKDTstate ist = {
            .data = {g.text[0], g.text[1]},
            .cb = state->cb,
            .ctx = state->ctx,
            .depth = state->depth + 1,
        };
        call(MKDTInlineLexer, &ist);
    }
    if (!u8csEmpty(close)) call(state->cb, 'G', close, state->ctx);
    done;
}

ok64 MKDTonEmph(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    return MKDTEmitSpan(tok, state);
}

//  A code span splits its backticks off too, but its body is verbatim — one
//  'H' token, never re-lexed (DOG-024).
ok64 MKDTonCode(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb == NULL) done;
    if (u8csLen(tok) < 3) {                      //  degenerate, keep it whole
        call(state->cb, 'H', tok, state->ctx);
        done;
    }
    u8cs open = {tok[0], tok[0] + 1};
    u8cs body = {tok[0] + 1, tok[1] - 1};
    u8cs close = {tok[1] - 1, tok[1]};
    call(state->cb, 'G', open, state->ctx);
    call(state->cb, 'H', body, state->ctx);
    call(state->cb, 'G', close, state->ctx);
    done;
}

ok64 MKDTonLink(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    return MKDTEmitSpan(tok, state);
}

ok64 MKDTonNumber(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('L', tok, state->ctx);
    done;
}

ok64 MKDTonWord(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('S', tok, state->ctx);
    done;
}

//  Issue key ABC-123: one filename-tagged token so the pager can navigate it
//  (BRO-012).  Same 'F' tag MDTonKey emits for `.md`.
ok64 MKDTonKey(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('F', tok, state->ctx);
    done;
}

ok64 MKDTonPunct(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('P', tok, state->ctx);
    done;
}

ok64 MKDTonSpace(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('W', tok, state->ctx);
    done;
}

//  Backslash escape \<punct>: emit the literal run with the leading
//  backslash dropped, as plain punctuation text (kind 'P'). The escaped
//  opener has already cancelled its bracketing role in the lexer, so the
//  whole run reaches HTML verbatim (escaped), never as an inline span.
ok64 MKDTonEscape(u8cs tok, MKDTstate *state) {
    sane($ok(tok) && state != NULL);
    a_dup(u8c, lit, tok);
    u8csUsed(lit, 1);  //  drop the leading backslash
    if (state->cb) return state->cb('P', lit, state->ctx);
    done;
}

// --- Stateless inquiries over the MKDTBlock regions (DOG-026) ---

//  Consume one prefix quad off `q` — four chars or ONE tab (DOG-024), so
//  counting is a walk, never len/4.  *quote := the quad carries a '>'.
static b8 MKDTQuadStep(u8cs q, b8 *quote) {
    *quote = NO;
    if (u8csEmpty(q)) return NO;
    if (*u8csHead(q) == '\t') return u8csUsed1(q) == OK ? YES : NO;
    a_dup(u8c, quad, q);
    if (u8csUsed(q, 4) != OK) return NO;
    quad[1] = q[0];
    *quote = u8csFind(quad, '>') == OK ? YES : NO;
    return YES;
}

int MKDTquadsCount(u8csc quads) {
    a_dup(u8c, q, quads);
    b8 quote;
    int n = 0;
    while (MKDTQuadStep(q, &quote)) n++;
    return n;
}

int MKDTquadsQuotes(u8csc quads) {
    a_dup(u8c, q, quads);
    b8 quote;
    int n = 0;
    while (MKDTQuadStep(q, &quote)) n += quote ? 1 : 0;
    return n;
}

int MKDTquadsDepth(u8csc quads) {
    a_dup(u8c, q, quads);
    b8 quote;
    int n = 0;
    while (MKDTQuadStep(q, &quote) && !quote) n++;
    return n;
}

//  The mark region is grammar-delimited by MKDTBlock, so the shapes are told
//  apart by their bytes alone; a foreign / empty slice answers 0 / NO.
mkdtmark MKDTmarkList(u8csc mark) {
    if (u8csLen(mark) != 4) return MKDT_MARK_NONE;
    int dash = 0, dot = 0, brk = 0, dig = 0, sp = 0;
    $for(u8c, p, mark) {
        if (*p == '-') dash++;
        else if (*p == '.') dot++;
        else if (*p == '[') brk++;
        else if (*p >= '0' && *p <= '9') dig++;
        else if (*p == ' ') sp++;
    }
    if (brk == 1)                //  -[x] is a todo, [x]: a reference
        return $at(mark, 0) == '-' ? MKDT_MARK_TODO : MKDT_MARK_NONE;
    if (dot == 1 && dig > 0) return MKDT_MARK_OLIST;
    if (dash == 1 && sp == 3) return MKDT_MARK_ULIST;
    return MKDT_MARK_NONE;       //  header / ruler / meta / fence quads
}

int MKDTmarkHeading(u8csc mark) {
    int n = 0;
    $for(u8c, p, mark) n += (*p == '#') ? 1 : 0;
    return n;
}

int MKDTmarkFence(u8csc mark) {
    if (u8csEmpty(mark) || $at(mark, 0) != '`') return 0;
    return (int)u8csLen(mark);   //  the mark IS the backtick run
}

b8 MKDTmarkHRule(u8csc mark) {
    if (u8csLen(mark) < 3) return NO;
    $for(u8c, p, mark) if (*p != '-') return NO;
    return YES;                  //  3-4 dashes; a bullet quad has spaces
}

b8 MKDTmarkRefDef(u8csc mark) {
    return !u8csEmpty(mark) && $at(mark, 0) == '[' ? YES : NO;
}

b8 MKDTmarkMeta(u8csc mark) {
    if (u8csEmpty(mark)) return NO;
    u8c h = $at(mark, 0);     //  `Key:` starts uppercase, nothing else does
    return h >= 'A' && h <= 'Z' ? YES : NO;
}

//  DOG-024: block markup emits ONE 'R' token per 4-char quad — indents, the
//  marker, the header run.  Only the innermost may be short (3-dash ruler,
//  3-backtick fence), its trailing spaces implied.
static ok64 MKDTEmitQuads(MKDTstate *state, u8csc markup) {
    sane(state != NULL);
    a_dup(u8c, rest, markup);
    if (state->cb == NULL) done;
    while (!u8csEmpty(rest)) {
        u64 len = u8csLen(rest);
        //  A quad is four chars — but one tab IS an indent quad, in a
        //  single byte, so it never swallows the three chars after it.
        u64 take = (*u8csHead(rest) == '\t') ? 1 : (len < 4 ? len : 4);
        a_dup(u8c, quad, rest);
        call(u8csUsed, rest, take);
        quad[1] = rest[0];   // the quad ends where the remainder begins
        call(state->cb, 'R', quad, state->ctx);
    }
    done;
}

// Emit heading: the "#..." markup quad as R, then the content through inline.
static ok64 MKDTEmitHeading(MKDTstate *state, const mkdtblock *b) {
    sane(state != NULL && b != NULL);
    call(MKDTEmitQuads, state, b->quads);
    call(MKDTEmitQuads, state, b->mark);

    a_dup(u8c, body, b->rest);
    b8 has_nl = !u8csEmpty(body) && *u8csLast(body) == '\n';
    if (has_nl) u8csShed1(body);

    if (!u8csEmpty(body)) {
        MKDTstate ist = {
            .data = {body[0], body[1]}, .cb = state->cb, .ctx = state->ctx};
        call(MKDTInlineLexer, &ist);
    }

    //  DOG-024: a newline is whitespace on a heading row too — it used to
    //  emit as 'S', the one line-end in StrictMark that read as text.
    if (has_nl && state->cb) {
        u8cs nl = {body[1], b->rest[1]};
        call(state->cb, 'W', nl, state->ctx);
    }
    done;
}

//  DOG-024: a fence line is markup too — the backtick run emits as one 'R'
//  token (3 backticks being the short innermost form), the info string 'H'.
static ok64 MKDTEmitFence(MKDTstate *state, const mkdtblock *b) {
    sane(state != NULL && b != NULL);
    call(MKDTEmitQuads, state, b->quads);
    call(MKDTEmitQuads, state, b->mark);
    if (!u8csEmpty(b->rest) && state->cb != NULL) {
        a_dup(u8c, rest, b->rest);
        call(state->cb, MDBLKu8csAllBlank(rest) ? 'W' : 'H', rest, state->ctx);
    }
    done;
}

#define MKDT_PARA_LINES 256   //  continuation lines one paragraph run may hold

//  DOG-024: a paragraph is broken by a blank line, not by a line end, so the
//  inline machine runs over the whole run and a span crosses the wrap — the
//  reading the renderers get from their joined paragraph buffer.  A
//  continuation line's markup prefix sits inside that run; this filter puts it
//  back as 'R' quads and splits whatever token covers it, so a span that
//  crosses a wrap emits as two same-tag pieces around the markup.
typedef struct {
    MKDTstate *out;                     //  downstream cb/ctx
    u8cs       pre[MKDT_PARA_LINES];    //  each continuation line's markup prefix
    int        n;                       //  how many are recorded
    int        i;                       //  the next one expected
    u8c const *skip;                    //  drop bytes up to here (prefix tail)
} mkdtpara;

static ok64 mkdt_para_cb(u8 tag, u8cs tok, void *ctx) {
    mkdtpara *p = (mkdtpara *)ctx;
    sane(p != NULL && $ok(tok));
    u8c const *lo = tok[0], *hi = tok[1];

    if (p->skip != NULL) {              //  inside a prefix already emitted
        if (hi <= p->skip) done;
        lo = p->skip;
        p->skip = NULL;
    }
    while (p->i < p->n && p->pre[p->i][0] < hi) {
        a_dup(u8c, mark, p->pre[p->i]);
        p->i += 1;
        if (lo < mark[0] && p->out->cb) {   //  a span straddling it splits here:
            u8cs head = {lo, mark[0]};      //  markup stays markup, the span
            call(p->out->cb, tag, head, p->out->ctx);   //  resumes after it
        }
        call(MKDTEmitQuads, p->out, mark);
        if (hi <= mark[1]) { p->skip = mark[1]; done; }
        lo = mark[1];
    }
    if (lo < hi && p->out->cb) {
        u8cs rest = {lo, hi};
        call(p->out->cb, tag, rest, p->out->ctx);
    }
    done;
}

static ok64 MKDTFlushPara(MKDTstate *state, u8cs para, mkdtpara *f) {
    sane(state != NULL && f != NULL);
    f->i = 0;
    f->skip = NULL;
    if (!u8csEmpty(para)) {
        f->out = state;
        MKDTstate ist = {
            .data = {para[0], para[1]},
            .cb = mkdt_para_cb,
            .ctx = f,
        };
        call(MKDTInlineLexer, &ist);
    }
    para[0] = para[1] = NULL;
    f->n = 0;
    f->i = 0;
    f->skip = NULL;
    done;
}

// --- Block-level lexer (drives the MKDTB grammar, one classification/line) ---

ok64 MKDTLexer(MKDTstate *state) {
    sane($ok(state->data) && state != NULL);

    a_dup(u8c, scan, state->data);
    b8 in_fence = NO;
    int fence_len = 0;
    u8cs para = {};              // the open paragraph run, NULL when none
    mkdtpara filt = {};          // its continuation lines' markup prefixes
    int want_quads = 0;          // the prefix a continuation line must carry:
    int want_quotes = 0;         //   the opener's, plus one for its marker

    u8cs line = {};
    while (MDBLKu8csDrainLine(scan, line) == OK) {
        mkdtblock b;
        MKDTBlock(line, &b);
        b8 blank = MDBLKu8csAllBlank(line);
        int fence = MKDTmarkFence(b.mark);

        //  A line with no marker and no leaf shape (an empty mark region)
        //  continues the open run — its markup prefix, if any, is restored
        //  by the filter.  A blank line, a marker or a leaf closes the run.
        b8 runs_on = !in_fence && !blank && u8csEmpty(b.mark)
                  && para[0] != NULL
                  && MKDTquadsCount(b.quads) == want_quads
                  && MKDTquadsQuotes(b.quads) == want_quotes
                  && filt.n < MKDT_PARA_LINES;
        if (runs_on) {
            if (!u8csEmpty(b.quads)) {
                u8csMv(filt.pre[filt.n], b.quads);
                filt.n += 1;
            }
            para[1] = b.rest[1];
            continue;
        }
        {
            ok64 o = MKDTFlushPara(state, para, &filt);
            if (o != OK) { state->data[0] = scan[0]; return o; }
        }

        if (in_fence) {
            //  A closing fence is markup (quads); anything else is code body.
            if (fence >= fence_len && MDBLKu8csAllBlank(b.rest)) {
                in_fence = NO;
                ok64 o = MKDTEmitFence(state, &b);
                if (o != OK) { state->data[0] = scan[0]; return o; }
                continue;
            }
            if (state->cb) {
                ok64 o = state->cb('H', line, state->ctx);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
            continue;
        }

        if (fence == 3 || fence == 4) {
            in_fence = YES;
            fence_len = fence;
            ok64 o = MKDTEmitFence(state, &b);
            if (o != OK) { state->data[0] = scan[0]; return o; }
        } else if (MKDTmarkHRule(b.mark)) {
            //  DOG-024: the ruler is one token (a 3-dash run implies its 4th
            //  space); the blank rest and the newline stay whitespace.
            ok64 o = MKDTEmitQuads(state, b.quads);
            if (o == OK) o = MKDTEmitQuads(state, b.mark);
            if (o != OK) { state->data[0] = scan[0]; return o; }
            if (!u8csEmpty(b.rest) && state->cb) {
                o = state->cb('W', b.rest, state->ctx);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
        } else if (MKDTmarkRefDef(b.mark)) {
            //  DOG-024: `[x]:` is the markup quad; the url/title is content.
            ok64 o = MKDTEmitQuads(state, b.quads);
            if (o == OK) o = MKDTEmitQuads(state, b.mark);
            if (o != OK) { state->data[0] = scan[0]; return o; }
            if (!u8csEmpty(b.rest)) {
                MKDTstate ist = {
                    .data = {b.rest[0], b.rest[1]},
                    .cb = state->cb,
                    .ctx = state->ctx,
                };
                o = MKDTInlineLexer(&ist);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
        } else if (MKDTmarkMeta(b.mark)) {
            //  DOG-026: the `Who:` quad is the meta-pair key marker ('T');
            //  the value is ONE verbatim token — no inline layer runs in it.
            ok64 o = MKDTEmitQuads(state, b.quads);
            if (o != OK) { state->data[0] = scan[0]; return o; }
            if (state->cb) {
                o = state->cb('T', b.mark, state->ctx);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
            a_dup(u8c, val, b.rest);
            b8 has_nl = !u8csEmpty(val) && *u8csLast(val) == '\n';
            if (has_nl) u8csShed1(val);
            if (state->cb && !u8csEmpty(val)) {
                o = state->cb('S', val, state->ctx);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
            if (state->cb && has_nl) {
                u8cs nl = {val[1], b.rest[1]};
                o = state->cb('W', nl, state->ctx);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
        } else if (MKDTmarkHeading(b.mark) > 0) {
            ok64 o = MKDTEmitHeading(state, &b);
            if (o != OK) { state->data[0] = scan[0]; return o; }
        } else {
            // Paragraph / list / quote / div: the prefix quads as R, then the
            // text opens a paragraph run the next lines may continue.
            ok64 o = MKDTEmitQuads(state, b.quads);
            if (o == OK) o = MKDTEmitQuads(state, b.mark);
            if (o != OK) { state->data[0] = scan[0]; return o; }
            if (!u8csEmpty(b.rest)) {
                if (blank) {          //  a blank line opens nothing
                    MKDTstate ist = {
                        .data = {b.rest[0], b.rest[1]},
                        .cb = state->cb,
                        .ctx = state->ctx,
                    };
                    o = MKDTInlineLexer(&ist);
                    if (o != OK) { state->data[0] = scan[0]; return o; }
                } else {
                    u8csMv(para, b.rest);
                    //  a continuation sits one quad deeper than a marker line
                    want_quads = MKDTquadsCount(b.quads)
                               + (MKDTmarkList(b.mark) == MKDT_MARK_NONE ? 0 : 1);
                    want_quotes = MKDTquadsQuotes(b.quads);
                }
            }
        }
    }
    {
        ok64 o = MKDTFlushPara(state, para, &filt);
        if (o != OK) { state->data[0] = scan[0]; return o; }
    }

    state->data[0] = scan[0];
    return OK;
}
