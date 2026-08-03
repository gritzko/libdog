//  MKDTB — the StrictMark block grammar, in 4-char blocks.
//
//  Per wiki/StrictMark the block layer is a regular language: every line's
//  structural prefix is a run of indent (div) blocks — four spaces or one
//  tab, which is the same thing — followed by at most one marker, and the
//  whole-line leaf shapes (heading, code fence, ruler, reference definition,
//  meta pair) are likewise fixed.  This one machine owns all of it — no
//  hand-rolled line classification in MKDT.c.
//
//  Every marker is exactly 4 chars wide and self-delimiting — no gap space
//  outside the quad (DOG-024): a single '>' (quote) or '-' (bullet) padded with
//  spaces in any of the four columns, 1-3 digits then '.' filling the slot
//  (numbered), a -[ ]/-[v]/-[-]/-[x] todo, or a run of 1-4 '#' padded the same
//  way (header, level = the count of '#', so "####" and "  # " both qualify).
//  Anything off-grammar (e.g. "-- ", a two-dash run) leaves `mark` empty, so
//  the line is a paragraph.  Only the innermost markup may be shorter than a
//  quad, its trailing spaces implied: a 3-dash ruler and a 3-backtick fence.
//
//  The machine RECORDS three region slices and nothing else: `quads` (the
//  prefix), `mark` (the marker / leaf markup) and `rest` (the content).  Every
//  derived fact is a stateless inquiry in MKDT.c over those slices (DOG-026).
//  Boundaries are set by FINISHING actions (@), which fire on the transition
//  that consumes a token's last byte — so they fire even when the next byte is
//  ordinary content (the common case), unlike leaving actions (%), which do
//  not fire on a dead-end transition.  One line, one classification.
//
//  Build: ragel -C MKDTB.c.rl -o MKDTB.rl.c -L

#include "MKDT.h"
#include "abc/PRO.h"

%%{
    machine mkdtb;
    alphtype unsigned char;

    action quad_end   { qe = p + 1; }
    action mark_end   { me = p + 1; }
    #  the ruler / meta run ends are staged: the mark is recorded only once
    #  the trailing newline / gap space CONFIRMS the shape.
    action hr_run     { hre = p + 1; }
    action hr_set     { me = hre; }
    action mt_run     { mke = p + 1; }
    action mt_set     { me = mke; }

    sp     = ' ';
    tab    = 0x09;
    ws     = sp | tab | 0x0d;
    nl     = 0x0a;
    #  DOG-024: one tab IS four spaces, so it fills an indent quad on its
    #  own.  It pads nothing else: a marker quad is spaces or nothing.
    indent = (sp sp sp sp) | tab ;

    quote  = ('>' sp sp sp) | (sp '>' sp sp) | (sp sp '>' sp) | (sp sp sp '>') ;
    bullet = ('-' sp sp sp) | (sp '-' sp sp) | (sp sp '-' sp) | (sp sp sp '-') ;
    dig    = 0x30 .. 0x39 ;
    number = (dig '.' sp sp) | (sp dig '.' sp) | (sp sp dig '.')
           | (dig dig '.' sp) | (sp dig dig '.') | (dig dig dig '.') ;
    #  TODO marker: a dash then a bracketed one-char state, filling the quad
    #  (-[ ] not started, -[v]/-[V] done, -[-] blocked, -[x]/-[X] wontfix).
    #  Distinct from a bullet (a lone dash) and a ruler.  DOG-024: the quad
    #  self-delimits, so no gap space follows — "-[v]New" is a todo item.
    todo   = '-' '[' (sp | 'v' | 'V' | '-' | 'x' | 'X') ']' ;

    alnm   = dig | 0x41 .. 0x5a | 0x61 .. 0x7a ;
    refdef = '[' alnm ']' ':' @mark_end ;

    #  DOG-026: meta pair `KEY ':' GAP VALUE`; key+colon IS the quad, the gap
    #  is content.  `mt_set` fires on the gap, so `Due:x` stays a paragraph.
    #  The 3rd key char may be a digit: relation keys `On1:`, `On2:` … .
    meta   = (upper lower (lower|dig) ':' @mt_run) sp @mt_set ;

    #  ATX header (DOG-024): a run of 1-4 '#' padded with spaces to fill the
    #  quad, in any column, exactly like quote/bullet; the level is the count
    #  of '#'.  A full quad needs no gap space, so "####" is a level-4 header.
    head1  = ('#' sp sp sp) | (sp '#' sp sp) | (sp sp '#' sp) | (sp sp sp '#') ;
    head2  = ('#' '#' sp sp) | (sp '#' '#' sp) | (sp sp '#' '#') ;
    head3  = ('#' '#' '#' sp) | (sp '#' '#' '#') ;
    head4  = ('#' '#' '#' '#') ;
    heading = (head1 | head2 | head3 | head4) @mark_end ;

    #  Code fence: a run of >=3 backticks.  The mark is recorded by a
    #  finishing action so a lone 1-2 backtick run (inline code at line start)
    #  never matches; the blank-vs-info rest is an inquiry over `rest`.
    fence  = ('`' '`' '`' '`'*) @mark_end ;

    #  Ruler: 3-4 dashes, blank rest, terminating newline (3-dash short form).
    #  DOG-024: `mark` ends the dash run, so the ruler emits as one token.
    hrule  = ('-' '-' '-' '-'?) @hr_run ws* nl @hr_set ;

    #  DOG-024: the block stack is `(INDENT|QUOTE)* LIST? LEAF?` per
    #  wiki/StrictMark — a quote quad is a repeatable container like an
    #  indent, so `>    1. x` is a numbered item inside a quote, not a
    #  quote whose text happens to start with "1.".
    prefix = (indent @quad_end | quote @quad_end)* ;
    list   = bullet @mark_end | number @mark_end | todo @mark_end ;

    main := prefix ( list | heading | fence | hrule | refdef | meta )? ;
}%%

%% write data;

void MKDTBlock(u8csc line, mkdtblock *b) {
    a_dup(u8c, data, line);
    const unsigned char *p = (const unsigned char *)data[0];
    const unsigned char *pe = (const unsigned char *)data[1];
    const unsigned char *eof = pe;
    const unsigned char *qe = p, *me = p, *hre = p, *mke = p;
    int cs;
    %% write init;
    %% write exec;

    //  The one pointer→slice boundary: the machine's recorded region ends
    //  become the three slices.  No marker matched leaves `mark` empty.
    if (me < qe) me = qe;
    b->quads[0] = data[0];       b->quads[1] = (const u8c *)qe;
    b->mark[0] = (const u8c *)qe; b->mark[1] = (const u8c *)me;
    b->rest[0] = (const u8c *)me; b->rest[1] = data[1];
    (void)eof; (void)hre; (void)mke;
    (void)mkdtb_en_main; (void)mkdtb_error; (void)mkdtb_first_final;
}
