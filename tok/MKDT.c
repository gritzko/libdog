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

// --- Block-line classifiers (thin wrappers over the MKDTB block grammar) ---

int MKDTFenceOpen(u8csc line) {
    mkdtblock b;
    MKDTBlock(line, &b);
    return (b.fence == 3 || b.fence == 4) ? b.fence : 0;
}

b8 MKDTFenceClose(u8csc line, int flen) {
    mkdtblock b;
    MKDTBlock(line, &b);
    return (b.fence >= flen && b.fence_blank) ? YES : NO;
}

int MKDTHeadingLevel(u8csc line) {
    mkdtblock b;
    MKDTBlock(line, &b);
    return b.heading;
}

b8 MKDTHRule(u8csc line) {
    mkdtblock b;
    MKDTBlock(line, &b);
    return b.hrule;
}

b8 MKDTRefDef(u8csc line) {
    mkdtblock b;
    MKDTBlock(line, &b);
    return b.refdef;
}

int MKDTIndentDepth(u8csc line) {
    mkdtblock b;
    MKDTBlock(line, &b);
    return b.depth;
}

//  One marker per line, the shape the renderers read: a quoted line reports
//  QUOTE and hands back the text after its first quote quad, whatever the
//  block stack holds beyond it (DOG-024 keeps the full stack in `mkdtblock`).
mkdtmark MKDTLineMarker(u8csc line, int depth, u8c **markend) {
    (void)depth;
    mkdtblock b;
    MKDTBlock(line, &b);
    if (b.quotes > 0) {
        *markend = (u8c *)b.qend;
        return MKDT_MARK_QUOTE;
    }
    *markend = (u8c *)b.content;
    return b.marker;
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
static ok64 MKDTEmitHeading(MKDTstate *state, u8csc line) {
    u8c *e = (u8c *)line[1];
    mkdtblock b;
    MKDTBlock(line, &b);
    u8c *p = (u8c *)b.content;   // first content byte after the header markup

    u8cs prefix = {line[0], p};
    if (!$empty(prefix)) {
        ok64 o = MKDTEmitQuads(state, prefix);
        if (o != OK) return o;
    }

    u8c *ce = e;
    b8 has_nl = NO;
    u8cs body = {p, e};
    if (!u8csEmpty(body) && *u8csLast(body) == '\n') { ce--; has_nl = YES; }

    if (p < ce) {
        MKDTstate ist = {.data = {p, ce}, .cb = state->cb, .ctx = state->ctx};
        ok64 o = MKDTInlineLexer(&ist);
        if (o != OK) return o;
    }

    //  DOG-024: a newline is whitespace on a heading row too — it used to
    //  emit as 'S', the one line-end in StrictMark that read as text.
    if (has_nl && state->cb) {
        u8cs nl = {ce, e};
        ok64 o = state->cb('W', nl, state->ctx);
        if (o != OK) return o;
    }
    return OK;
}

//  DOG-024: a fence line is markup too — the backtick run emits as one 'R'
//  token (3 backticks being the short innermost form), the info string 'H'.
static ok64 MKDTEmitFence(MKDTstate *state, u8csc line, const mkdtblock *b) {
    sane(state != NULL && b != NULL);
    u8cs mark = {line[0], (u8c *)b->content};
    call(MKDTEmitQuads, state, mark);
    if (b->content < line[1] && state->cb != NULL) {
        u8cs rest = {(u8c *)b->content, (u8c *)line[1]};
        call(state->cb, b->fence_blank ? 'W' : 'H', rest, state->ctx);
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
    u8c const *sol[MKDT_PARA_LINES];    //  continuation line starts
    u8c const *txt[MKDT_PARA_LINES];    //  where each one's content begins
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
    while (p->i < p->n && p->sol[p->i] < hi) {
        u8c const *s = p->sol[p->i], *c = p->txt[p->i];
        p->i += 1;
        if (lo < s && p->out->cb) {     //  a span straddling it splits here:
            u8cs head = {lo, s};        //  markup stays markup, the span
            call(p->out->cb, tag, head, p->out->ctx);   //  resumes after it
        }
        u8cs mark = {s, c};
        call(MKDTEmitQuads, p->out, mark);
        if (hi <= c) { p->skip = c; done; }
        lo = c;
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
        u8c *sol = (u8c *)line[0];   // start of line
        u8c *cur = (u8c *)line[1];   // end of line (past the '\n')
        mkdtblock b;
        MKDTBlock(line, &b);
        b8 blank = MDBLKu8csAllBlank(line);

        //  A line with no marker and no leaf shape continues the open run —
        //  its markup prefix, if any, is restored by the filter.  A blank
        //  line, a marker or a leaf closes the run.
        b8 runs_on = !in_fence && !blank && b.fence == 0 && !b.hrule
                  && !b.refdef && b.heading == 0
                  && b.marker == MKDT_MARK_NONE && para[0] != NULL
                  && b.quads == want_quads && b.quotes == want_quotes
                  && filt.n < MKDT_PARA_LINES;
        if (runs_on) {
            if ((u8c *)b.content > sol) {
                filt.sol[filt.n] = sol;
                filt.txt[filt.n] = b.content;
                filt.n += 1;
            }
            para[1] = cur;
            continue;
        }
        {
            ok64 o = MKDTFlushPara(state, para, &filt);
            if (o != OK) { state->data[0] = scan[0]; return o; }
        }

        if (in_fence) {
            //  A closing fence is markup (quads); anything else is code body.
            if (b.fence >= fence_len && b.fence_blank) {
                in_fence = NO;
                ok64 o = MKDTEmitFence(state, line, &b);
                if (o != OK) { state->data[0] = scan[0]; return o; }
                continue;
            }
            if (state->cb) {
                ok64 o = state->cb('H', line, state->ctx);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
            continue;
        }

        if (b.fence == 3 || b.fence == 4) {
            in_fence = YES;
            fence_len = b.fence;
            ok64 o = MKDTEmitFence(state, line, &b);
            if (o != OK) { state->data[0] = scan[0]; return o; }
        } else if (b.hrule) {
            //  DOG-024: the ruler is one token (a 3-dash run implies its 4th
            //  space); the blank rest and the newline stay whitespace.
            u8cs mark = {sol, (u8c *)b.content};
            ok64 o = MKDTEmitQuads(state, mark);
            if (o != OK) { state->data[0] = scan[0]; return o; }
            if (b.content < cur && state->cb) {
                u8cs blank = {(u8c *)b.content, cur};
                o = state->cb('W', blank, state->ctx);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
        } else if (b.refdef) {
            //  DOG-024: `[x]:` is the markup quad; the url/title is content.
            u8cs mark = {sol, (u8c *)b.content};
            ok64 o = MKDTEmitQuads(state, mark);
            if (o != OK) { state->data[0] = scan[0]; return o; }
            if (b.content < cur) {
                MKDTstate ist = {
                    .data = {(u8c *)b.content, cur},
                    .cb = state->cb,
                    .ctx = state->ctx,
                };
                o = MKDTInlineLexer(&ist);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
        } else if (b.heading > 0) {
            ok64 o = MKDTEmitHeading(state, line);
            if (o != OK) { state->data[0] = scan[0]; return o; }
        } else {
            // Paragraph / list / quote / div: the prefix quads as R, then the
            // text opens a paragraph run the next lines may continue.
            u8c *text_start = (u8c *)b.content;
            if (text_start > sol) {
                u8cs markup = {sol, text_start};
                ok64 o = MKDTEmitQuads(state, markup);
                if (o != OK) { state->data[0] = scan[0]; return o; }
            }
            if (text_start < cur) {
                if (blank) {          //  a blank line opens nothing
                    MKDTstate ist = {
                        .data = {text_start, cur},
                        .cb = state->cb,
                        .ctx = state->ctx,
                    };
                    ok64 o = MKDTInlineLexer(&ist);
                    if (o != OK) { state->data[0] = scan[0]; return o; }
                } else {
                    para[0] = text_start;
                    para[1] = cur;
                    //  a continuation sits one quad deeper than a marker line
                    want_quads = b.quads
                               + (b.marker == MKDT_MARK_NONE ? 0 : 1);
                    want_quotes = b.quotes;
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
