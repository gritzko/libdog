#include "FREE.h"

#include "abc/PRO.h"

// Inline ragel lexer (FREE.rl.c, generated from FREE.c.rl)
ok64 FREELexer(FREEstate *state);

typedef struct {
    u8 overlay;
    TOKcb cb;
    void *ctx;
} FREE_overlay_ctx;

static ok64 FREE_overlay_cb(u8 nat, u8cs tok, void *ctx) {
    FREE_overlay_ctx *o = (FREE_overlay_ctx *)ctx;
    // Sticky tags pop through the overlay: 'F' issue key, and (DOG-006)
    // StrictMark markup 'G' (emphasis/link) and 'H' (code span) — so they
    // stay lit instead of vanishing into comment gray.
    b8 sticky = (nat == 'F' || nat == 'G' || nat == 'H');
    u8 tag = (o->overlay && !sticky) ? o->overlay : nat;
    return o->cb ? o->cb(tag, tok, o->ctx) : OK;
}

ok64 FREEu8sFeed(u8 overlay, u8cs slice, TOKcb cb, void *ctx) {
    sane($ok(slice));
    FREE_overlay_ctx oc = {overlay, cb, ctx};
    FREEstate st = {
        .data = {slice[0], slice[1]},
        .cb = cb ? FREE_overlay_cb : NULL,
        .ctx = &oc,
    };
    return FREELexer(&st);
}

// DOG-006: emit comment delimiters as plain 'D' (never StrictMark-parsed),
// then FREE-scan the body (issue keys + inline markup) under a 'D' overlay.
ok64 FREECommentFeed(u8cs open, u8cs body, u8cs close, TOKcb cb, void *ctx) {
    sane($ok(body));
    if (!cb) done;
    if (!u8csEmpty(open)) { ok64 o = cb('D', open, ctx); if (o != OK) return o; }
    { ok64 o = FREEu8sFeed('D', body, cb, ctx); if (o != OK) return o; }
    if (!u8csEmpty(close)) { ok64 o = cb('D', close, ctx); if (o != OK) return o; }
    done;
}

// DOG-006: fixed-width delimiter split (line "//" -> 2,0; block "/* */" -> 2,2).
// A short/malformed token (olen+clen > len) falls back to a plain body feed.
ok64 FREECommentFeedN(u8cs tok, u32 olen, u32 clen, TOKcb cb, void *ctx) {
    sane($ok(tok));
    if (!cb) done;
    size_t n = u8csLen(tok);
    if ((size_t)olen + (size_t)clen > n) return FREEu8sFeed('D', tok, cb, ctx);
    a_head(u8c, open, tok, olen);
    a_tail(u8c, close, tok, clen);
    a_part(u8c, body, tok, olen, n - olen - clen);
    return FREECommentFeed(open, body, close, cb, ctx);
}
