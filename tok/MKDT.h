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

// --- Block-line classifiers ---
//
// These wrap the line-shape decisions MKDTLexer makes internally and are
// exposed so renderers (e.g. the `mark` dog) can recover block structure
// without re-deriving it.  The StrictMark grammar itself is unchanged.
// Each takes one source line (with or without its trailing '\n').

int MKDTFenceOpen(u8csc line);             // opening fence width 3/4, else 0
b8 MKDTFenceClose(u8csc line, int flen);   // closes a fence of width flen
int MKDTHeadingLevel(u8csc line);          // ATX level 1..4, else 0
b8 MKDTHRule(u8csc line);                  // "----" horizontal ruler
b8 MKDTRefDef(u8csc line);                 // "[x]: …" reference definition
int MKDTIndentDepth(u8csc line);           // count of leading 4-space blocks

typedef enum {
    MKDT_MARK_NONE = 0,
    MKDT_MARK_QUOTE,  // >
    MKDT_MARK_ULIST,  // -
    MKDT_MARK_OLIST,  // N.
    MKDT_MARK_TODO,   // [ ] [x] [X]
} mkdtmark;

// One line's full structural classification, per the StrictMark block grammar:
// the block stack `(INDENT|QUOTE)* LIST? LEAF?` — a run of indent/quote quads,
// then at most one list marker, OR a whole-line leaf shape (heading / code
// fence / ruler / reference definition).  `marker` never carries QUOTE: a
// quote is a container, counted in `quotes` (MKDTLineMarker keeps reporting
// QUOTE for the renderers that read one marker per line).
typedef struct {
    int        depth;        // count of LEADING indent quads (before a quote)
    int        quads;        // total prefix quads, indent or quote (DOG-024)
    int        quotes;       // count of quote quads in the prefix (DOG-024)
    const u8c *qend;         // end of the first quote quad, NULL without one
    mkdtmark   marker;       // the list marker after the prefix, else NONE
    int        heading;      // ATX header level 1..4, else 0
    int        fence;        // backtick-run width (wrapper accepts only 3/4)
    b8         fence_blank;  // the backtick run has a blank rest (a close fence)
    b8         hrule;        // 3-4 dash ruler with a blank rest
    b8         refdef;       // [x]: reference definition
    const u8c *content;      // first content byte after indents + marker/header
} mkdtblock;

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

// The StrictMark block grammar (ragel: MKDTB): classify one line.  Only exact
// shapes match — markers are 4-char-wide (a single '>'/'-' padded with spaces
// in any column, 1-3 digits then '.', or a [ ]/[x]/[X] todo); headers need the
// gap space; anything off-grammar (e.g. "-- ") leaves marker NONE (a paragraph).
void MKDTBlock(u8csc line, mkdtblock *b);

// Classify the block marker in the 4-char group after `depth` indents.
// *markend := end of the 4-char marker slot on a hit, else line[0]+depth*4.
mkdtmark MKDTLineMarker(u8csc line, int depth, u8c **markend);

#endif
