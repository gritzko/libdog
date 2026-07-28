//  doghili — print a file with every token hili'd, so a tokenizer's
//  output can be eyeballed against the source it came from (DOG-024).
//
//      doghili [--plain|--color] [--no-ruler] [--theme NAME]
//              [--ext EXT] FILE
//
//  Every token wears its THEME colour, and every OTHER token an
//  underline, so a boundary shows even between two same-tag tokens
//  (the `#   ` / `-[v]` quads a `S`-tagged word runs straight into).
//  Under each source row a ruler names the tag of every char: the
//  tag letter at the token's first char, `.` for more of the same
//  token, the lowercase tag for a token continued from the previous
//  line.  Reading the letters off a row gives the `TOK01Case` tag
//  sequence.
//
//  One source char = one output column, so the ruler always sits
//  under the char it describes: a tab prints as `→`, a newline as
//  `↵`, any other control byte as `?`, and a multi-byte UTF-8 char
//  spends one ruler slot (its lead byte's tag).

#include "TOK.h"

#include "abc/ANSI.h"
#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/THEME.h"

#define HILI_OUT_CAP (1UL << 16)
#define HILI_ROW_CAP (1UL << 20)
#define HILI_SRC_CAP (1UL << 24)
#define HILI_GUTTER  5

typedef struct {
    int    fd;
    u8bp   out;      //  stdout buffer, flushed by threshold
    u8bp   ruler;    //  one tag char per source char of the open line
    u32    lineno;
    ansi64 cur;      //  SGR currently open in `out`
    b8     alt;      //  underline every other token
    b8     color;
    b8     rule;     //  draw the ruler rows
    b8     bol;      //  nothing written on the open line yet
} hili;

static ok64 hili_flush(hili *h) {
    sane(h != NULL);
    while (u8bDataLen(h->out) > 0) call(FILEFlushThreshold, h->fd, h->out, 1);
    u8bReset(h->out);
    done;
}

static ok64 hili_room(hili *h, size_t need) {
    sane(h != NULL);
    if (u8bIdleLen(h->out) < need) call(hili_flush, h);
    done;
}

//  Close whatever SGR is open; the reset also ends the underline, so a
//  hili'd trailing space never bleeds past the newline glyph.
static ok64 hili_off(hili *h) {
    sane(h != NULL);
    if (h->cur == ANSI_DEFAULT) done;
    call(hili_room, h, 64);
    call(ANSIu8sFeedReset, u8bIdle(h->out), h->cur);
    h->cur = ANSI_DEFAULT;
    done;
}

static ok64 hili_sgr(hili *h, ansi64 want) {
    sane(h != NULL);
    if (!h->color || want == h->cur) done;
    if (want == ANSI_DEFAULT) return hili_off(h);
    call(hili_room, h, 64);
    call(ANSIu8sFeedDelta, u8bIdle(h->out), want, h->cur);
    h->cur = want;
    done;
}

//  Feed a whole slice out, flushing as often as it takes.
static ok64 hili_spill(hili *h, u8csc what) {
    sane(h != NULL);
    a_dup(u8 const, d, what);
    while (!$empty(d)) {
        call(hili_room, h, 1);
        call(u8sDrain, u8bIdle(h->out), d);
    }
    done;
}

//  `  123 │ ` in grey, or the same width blank for a ruler row.
static ok64 hili_gutter(hili *h, u32 no) {
    sane(h != NULL);
    call(hili_room, h, 64);
    call(hili_sgr, h, h->color ? THEMEAt('Q') : ANSI_DEFAULT);
    a_pad(u8, num, 16);
    if (no > 0) call(utf8sFeed10, num_idle, (u64)no);
    for (size_t i = u8bDataLen(num); i < HILI_GUTTER; ++i)
        call(u8bFeed1, h->out, ' ');
    if (no > 0) call(u8bFeed, h->out, u8bDataC(num));
    a_cstr(bar, " │ ");
    call(hili_spill, h, bar);
    done;
}

//  One ruler slot for one source char; a line longer than the row
//  buffer just stops collecting (the content row stays whole).
static void hili_mark(hili *h, u8 c) {
    if (u8bIdleLen(h->ruler) > 0) u8bFeed1(h->ruler, c);
}

//  Close the content row, draw the ruler row under it, open the next.
static ok64 hili_eol(hili *h) {
    sane(h != NULL);
    call(hili_off, h);
    call(hili_room, h, 8);
    call(u8bFeed1, h->out, '\n');
    if (h->rule) {
        call(hili_gutter, h, 0);
        call(hili_sgr, h, h->color ? THEMEAt('D') : ANSI_DEFAULT);
        call(hili_spill, h, u8bDataC(h->ruler));
        call(hili_off, h);
        call(hili_room, h, 8);
        call(u8bFeed1, h->out, '\n');
    }
    u8bReset(h->ruler);
    h->lineno += 1;
    h->bol = YES;
    done;
}

static ok64 hili_cb(u8 tag, u8cs tok, void *ctx) {
    hili *h = (hili *)ctx;
    sane(h != NULL && $ok(tok));

    ansi64 want = ANSI_DEFAULT;
    if (h->color) {
        want = THEMEAt(tag);
        if (h->alt) want |= ANSI64_FLAG(ANSI_UNDERLINE);
    }
    h->alt = !h->alt;

    //  `head` is the ruler letter owed to the next char: the tag at the
    //  token's first char, its lowercase after a line break inside the
    //  token, `.` for every other char of the run.
    u8 head = tag;
    for (u8c const *p = tok[0]; p < tok[1]; ++p) {
        u8 c = *p;
        if (h->bol) {
            call(hili_gutter, h, h->lineno);
            h->bol = NO;
        }
        call(hili_sgr, h, want);
        call(hili_room, h, 8);
        if (c == '\n') {
            a_cstr(nl, "↵");
            call(hili_spill, h, nl);
            hili_mark(h, head);
            call(hili_eol, h);
            if (p + 1 < tok[1])           //  the token runs on
                head = (u8)(tag - 'A' + 'a');
            continue;
        }
        if (c == '\t') {
            //  A tab IS an indent quad (DOG-024), so spend its four
            //  columns — the grid stays honest and so does the ruler.
            a_cstr(tb, "→   ");
            call(hili_spill, h, tb);
            hili_mark(h, head);
            hili_mark(h, '.');
            hili_mark(h, '.');
            hili_mark(h, '.');
            head = '.';
            continue;
        }
        if (c < 0x20 || c == 0x7f) {
            call(u8bFeed1, h->out, '?');
        } else {
            call(u8bFeed1, h->out, c);
        }
        if ((c & 0xc0) != 0x80) {         //  utf8 tails take no slot
            hili_mark(h, head);
            head = '.';
        }
    }
    done;
}

//  The extension drives the lexer choice; `--ext` overrides it for a
//  file whose name says nothing (a scratch dump, a `README`).
static void hili_ext(u8csp out, u8csc path) {
    out[0] = NULL;
    out[1] = NULL;
    u8c const *p = path[1];
    while (p > path[0]) {
        --p;
        if (*p == '.') {
            out[0] = p + 1;
            out[1] = path[1];
            return;
        }
        if (*p == '/') return;
    }
}

static u8 _srcbuf[HILI_SRC_CAP];

static ok64 hilicli() {
    sane($arglen >= 2);

    u8cs file = {};
    u8cs ext = {};
    char const *theme = NULL;
    b8 color = ANSIIsTTY();
    b8 rule = YES;

    a_cstr(f_plain, "--plain");
    a_cstr(f_color, "--color");
    a_cstr(f_norule, "--no-ruler");
    a_cstr(f_theme, "--theme");
    a_cstr(f_ext, "--ext");
    for (size_t i = 1; i < $arglen; ++i) {
        a$rg(a, i);
        if (u8csEq(a, f_plain)) {
            color = NO;
        } else if (u8csEq(a, f_color)) {
            color = YES;
        } else if (u8csEq(a, f_norule)) {
            rule = NO;
        } else if (u8csEq(a, f_theme) && i + 1 < $arglen) {
            a$rg(v, ++i);
            theme = (char const *)v[0];
        } else if (u8csEq(a, f_ext) && i + 1 < $arglen) {
            a$rg(v, ++i);
            $mv(ext, v);
        } else if (u8csLen(a) > 0 && *a[0] == '-') {
            fail(BADARG);
        } else {
            $mv(file, a);
        }
    }
    if (u8csLen(file) == 0) fail(BADARG);
    if (ext[0] == NULL) hili_ext(ext, file);
    if (ext[0] == NULL) fail(TOKBAD);
    call(THEMESelect, theme);

    a_pad(u8, path, 4096);
    call(u8sFeed, path_idle, file);
    PATHu8bTerm(path);

    u8 *idle[2] = {_srcbuf, _srcbuf + sizeof(_srcbuf)};
    u8 *start = idle[0];
    int fd;
    call(FILEOpen, &fd, $path(path), O_RDONLY);
    call(FILEdrainall, idle, fd);
    call(FILEClose, &fd);

    aBpad2(u8, out, HILI_OUT_CAP);
    a_carve(u8, ruler, HILI_ROW_CAP);
    hili h = {
        .fd = STDOUT_FILENO,
        .out = outbuf,
        .ruler = ruler,
        .lineno = 1,
        .color = color,
        .rule = rule,
        .bol = YES,
    };

    TOKstate state = {
        .data = {start, idle[0]},
        .cb = hili_cb,
        .ctx = &h,
    };
    call(TOKLexer, &state, ext);
    if (!h.bol) call(hili_eol, &h);       //  no trailing newline in the file
    call(hili_off, &h);
    call(hili_flush, &h);
    done;
}

MAIN(hilicli);
