#include "abc/INT.h"
#include "abc/PRO.h"
#include "FREE.h"
#include "MKDT.h"

//  DOG-024: a span is markup + text here too — the delimiters emit as their
//  own 'G' tokens and the body is re-lexed one level down.  The split runs
//  through `mkdtg` (MKDTDecomposeSpan), the same machine StrictMark uses, so
//  a comment and a .mkd page tokenize their markup identically.
static ok64 FREEEmitSpan(u8cs tok, FREEstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb == NULL) done;
    mkdtspan g = {};
    if (state->depth >= FREE_MAX_DEPTH || MKDTDecomposeSpan(&g, tok) != OK
        || g.kind == 0 || g.text[0] == NULL) {
        call(state->cb, 'G', tok, state->ctx);
        done;
    }
    u8cs open = {tok[0], g.text[0]};
    u8cs close = {g.text[1], tok[1]};
    if (!u8csEmpty(open)) call(state->cb, 'G', open, state->ctx);
    if (!u8csEmpty(g.text)) {
        FREEstate ist = {
            .data = {g.text[0], g.text[1]},
            .cb = state->cb,
            .ctx = state->ctx,
            .depth = state->depth + 1,
        };
        call(FREELexer, &ist);
    }
    if (!u8csEmpty(close)) call(state->cb, 'G', close, state->ctx);
    done;
}

//  A code span splits its backticks off; the body is verbatim 'H'.
static ok64 FREEEmitCode(u8cs tok, FREEstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb == NULL) done;
    if (u8csLen(tok) < 3) {
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

%%{

machine FREE;

alphtype unsigned char;

any8        = (0x00..0xff);
ws_h        = [ \t\r\f\v];
dgt         = [0-9];
xdgt        = [0-9a-fA-F];
ascii_alpha = [a-zA-Z_];
ascii_alnum = [a-zA-Z_0-9];
uc          = [A-Z];
ucnum       = [A-Z0-9_];
hibyte      = (0x80..0xff);
nl          = [\n];
nws         = any8 - [ \t\r\n\f\v];

wordstart   = ascii_alpha | hibyte;
wordcont    = ascii_alnum | hibyte;

action on_word {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('S', tok, state->ctx); if (o!=OK) fbreak; }
}
action on_key {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('F', tok, state->ctx); if (o!=OK) fbreak; }
}
action on_number {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('L', tok, state->ctx); if (o!=OK) fbreak; }
}
action on_punct {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('P', tok, state->ctx); if (o!=OK) fbreak; }
}
action on_space {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('W', tok, state->ctx); if (o!=OK) fbreak; }
}
#  DOG-006: StrictMark inline spans inside comment/prose bodies.  Code -> 'H',
#  emphasis/strike/link -> 'G'; overlay keeps them sticky so markup pops.
action on_code {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = FREEEmitCode(tok, state);
    if (o!=OK) fbreak;
}
action on_emph {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = FREEEmitSpan(tok, state);
    if (o!=OK) fbreak;
}
action on_link {
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = FREEEmitSpan(tok, state);
    if (o!=OK) fbreak;
}

main := |*

    # ---- issue keys (ABC-123) — wins over plain word by longest match ----
    uc ucnum* "-" dgt+                                   => on_key;

    # ---- StrictMark inline (DOG-006): code > strong > emph > strike > link.
    # No backslash-escape rule here — comments must not eat "\n"/"\t" bytes.
    "`" ( any8 - [`\n] )+ "`"                            => on_code;
    "*" (nws - [*]) (any8 - [*\n])* "*"                  => on_emph;
    "_" (nws - [_]) (any8 - [_\n])* "_"                  => on_emph;
    "~" (nws - [~]) ( any8 - [~] | [~] (any8 - [~]) )* "~" => on_emph;
    "[" (any8 - [\]\n])+ "][" [0-9A-Za-z] "]"           => on_link;
    "![" (any8 - [\]\n])+ "][" [0-9A-Za-z] "]"          => on_link;
    "[" (any8 - [\]\n])+ "]"                             => on_link;

    # ---- numbers ----
    "0" [xX] xdgt+                                       => on_number;
    dgt+ "." dgt*                                        => on_number;
    "." dgt+                                             => on_number;
    dgt+                                                 => on_number;

    # ---- words (ASCII + hi-byte runs, no strict UTF-8) ----
    wordstart wordcont*                                  => on_word;

    # ---- horizontal whitespace runs ----
    ws_h+                                                => on_space;

    # ---- newline: one byte, never fused with anything ----
    [\n]                                                 => on_space;

    # ---- catch-all: any other single byte → punctuation ----
    any8                                                 => on_punct;

*|;

}%%

%%write data;

ok64 FREELexer(FREEstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    int act = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u8c *ts = NULL;
    u8c *te = NULL;
    ok64 o = OK;

    u8cs tok = {p, p};

    %% write init;
    %% write exec;

    state->data[0] = p;
    if (o==OK && cs < FREE_first_final)
        o = FREEBAD;

    return o;
}
