#ifndef TOK_MKDT_H
#define TOK_MKDT_H

#include "TOK.h"

con ok64 MKDTBAD = 0x1650d74b28d;
con ok64 MKDTFAIL = 0x59435d3ca495;

//  DOG-024: `depth` is the inline recursion level — a span's delimiters emit
//  as their own tokens and its body is re-lexed one level down, so nesting
//  needs no parser state; past MKDT_MAX_DEPTH a body stays plain text.
#define MKDT_MAX_DEPTH 16

typedef struct {
    u8cs data;
    TOKcb cb;
    void *ctx;
    int   depth;
} MKDTstate;

ok64 MKDTLexer(MKDTstate *state);

// Inline tokenizer (generated from MKDT.c.rl); emits G/H/L/S/P/W via cb.
ok64 MKDTInlineLexer(MKDTstate *state);

// --- Block-line classification ---

typedef enum {
    MKDT_MARK_NONE = 0,
    MKDT_MARK_QUOTE,  // >
    MKDT_MARK_ULIST,  // -
    MKDT_MARK_OLIST,  // N.
    MKDT_MARK_TODO,   // [ ] [x] [X]
} mkdtmark;

// One line's structural classification, per the StrictMark block grammar
// `(INDENT|QUOTE)* (LIST | LEAF)?`: three consecutive regions of the line.
// Every derived fact (quad counts, marker kind, heading level, fence width,
// …) is a stateless inquiry over these slices; nothing is cached (DOG-026).
typedef struct {
    u8cs quads;   // the structural prefix: indent / quote quads (DOG-024)
    u8cs mark;    // the list-marker or leaf-markup region, empty when none
    u8cs rest;    // the content: text / info string / value, to line end
} mkdtblock;

// --- Stateless inquiries over the recorded regions ---
//
// Each takes a region MKDTBlock delimited; empty/foreign slices answer 0/NO.
int MKDTquadsCount(u8csc quads);    // total prefix quads (one tab IS a quad)
int MKDTquadsQuotes(u8csc quads);   // count of quote quads in the prefix
int MKDTquadsDepth(u8csc quads);    // LEADING indent quads, before any quote
mkdtmark MKDTmarkList(u8csc mark);  // the list marker in `mark`, else NONE
int MKDTmarkHeading(u8csc mark);    // ATX header level 1..4, else 0
int MKDTmarkFence(u8csc mark);      // backtick-run width, else 0
b8 MKDTmarkHRule(u8csc mark);       // 3-4 dash ruler (blank rest verified)
b8 MKDTmarkRefDef(u8csc mark);      // [x]: reference definition
b8 MKDTmarkMeta(u8csc mark);        // `Key:` meta-pair quad (DOG-026)

// A decomposed inline span (ragel: the mkdtg machine, MKDTDecomposeSpan): the
// StrictMark inline grammar's emphasis/link/image forms split into their parts,
// so a renderer needs no second parse.  For a shortcut [page], label == text.
typedef struct {
    u8   kind;   // 'B' strong, 'I' emph, 'D' del, 'A' link, 'M' image, 0 none
    u8cs text;   // inner text / link text / alt text
    u8cs label;  // explicit label; for a shortcut it equals the bracket text
} mkdtspan;

// Decompose one inline span token (the 'G' token from MKDTInlineLexer); always
// OK, with kind 0 when the token is not a recognised span.
ok64 MKDTDecomposeSpan(mkdtspan *g, u8csc tok);

// The StrictMark block grammar (ragel: MKDTB): split one line into regions.
// Only exact shapes match — markers are 4-char-wide (a single '>'/'-' padded
// with spaces in any column, 1-3 digits then '.', or a [ ]/[x]/[X] todo);
// anything off-grammar (e.g. "-- ") leaves `mark` empty (a paragraph).
void MKDTBlock(u8csc line, mkdtblock *b);

#endif
