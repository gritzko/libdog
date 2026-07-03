#ifndef TOK_FREE_H
#define TOK_FREE_H

#include "TOK.h"

// FREE — reusable "free text" scanner for natural-language slices
// (comment bodies, docstrings, markdown paragraphs).  Splits a slice
// into space/punctuation-separated UTF-8 words, recognises issue
// keys (ABC-123) as one token, treats newlines as standalone
// whitespace tokens (never fused into a word).  No strict UTF-8
// validation — any 0x80..0xff byte runs as a word char.
//
// Native tags emitted by FREELexer:
//   S — word (ASCII alnum runs and/or hi-byte runs)
//   L — number (decimal, fractional, hex)
//   P — punctuation (one byte at a time)
//   W — whitespace (horizontal runs OR a single newline)
//
// FREEu8sFeed wraps FREELexer with an overlay tag — pass 'D' from a
// comment callback to retag every chunk as D, etc.  Pass 0 to keep
// native tags.
//
// DOG-006: FREELexer also matches StrictMark inline spans — code `x`
// (tag 'H'), emphasis/strike/link *x* _x_ ~~x~~ [x] (tag 'G').  These
// tags and 'F' (issue key) stay STICKY through the overlay so markup
// pops instead of graying out.  Comment DELIMITERS ("//", "/* */", …)
// must NOT reach the scanner: FREECommentFeed[N] emits them as plain
// 'D' and StrictMark-parses only the body (so "/*x*/" cannot false-span).

con ok64 FREEBAD  = 0x5e6e63b28d;
con ok64 FREEFAIL = 0x16f5e6e6ca495;

typedef struct {
    u8cs data;
    TOKcb cb;
    void *ctx;
} FREEstate;

ok64 FREELexer(FREEstate *state);

ok64 FREEu8sFeed(u8 overlay, u8cs slice, TOKcb cb, void *ctx);

// DOG-006: split a comment into delimiter(s) + body.  FREECommentFeed takes
// pre-carved slices (empty open/close allowed); FREECommentFeedN carves a
// token by fixed delimiter widths (line "//" -> olen 2, clen 0; block
// "/* */" -> 2, 2).  Delimiters emit as plain 'D'; the body is StrictMark-scanned.
ok64 FREECommentFeed(u8cs open, u8cs body, u8cs close, TOKcb cb, void *ctx);
ok64 FREECommentFeedN(u8cs tok, u32 olen, u32 clen, TOKcb cb, void *ctx);

#endif
