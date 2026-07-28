//  MKDTB — the StrictMark block grammar, in 4-char blocks.
//
//  Per wiki/StrictMark the block layer is a regular language: every line's
//  structural prefix is a run of indent (div) blocks — four spaces or one
//  tab, which is the same thing — followed by at most one marker, and the whole-line leaf shapes (heading, code fence,
//  ruler, reference definition) are likewise fixed.  This one machine owns all
//  of it — there is no hand-rolled line classification left in MKDT.c.
//
//  Every marker is exactly 4 chars wide and self-delimiting — no gap space
//  outside the quad (DOG-024): a single '>' (quote) or '-' (bullet) padded with
//  spaces in any of the four columns, 1-3 digits then '.' filling the slot
//  (numbered), a -[ ]/-[v]/-[-]/-[x] todo, or a run of 1-4 '#' padded the same
//  way (header, level = the count of '#', so "####" and "  # " both qualify).
//  Anything off-grammar (e.g. "-- ", a two-dash run) leaves marker NONE, so the
//  line is a paragraph.  Only the innermost markup may be shorter than a quad,
//  its trailing spaces implied: a 3-dash ruler and a 3-backtick code fence.
//
//  Fields are set by FINISHING actions (@), which fire on the transition that
//  consumes a token's last byte — so they fire even when the next byte is
//  ordinary content (the common case), unlike leaving actions (%), which do
//  not fire on a dead-end transition.  `content` is set to p+1 (one past the
//  matched byte) = the first content byte.  One line, one classification.
//
//  Build: ragel -C MKDTB.c.rl -o MKDTB.rl.c -L

#include "MKDT.h"
#include "abc/PRO.h"

%%{
    machine mkdtb;
    alphtype unsigned char;

    action indent_end {
        if (b->quotes == 0) b->depth += 1;   //  depth counts the LEADING indents
        b->quads += 1;
        b->content = (const u8c *)(p + 1);
    }
    action quote_end  {
        b->quads += 1;
        b->quotes += 1;
        if (b->quotes == 1) b->qend = (const u8c *)(p + 1);
        b->content = (const u8c *)(p + 1);
    }
    action ulist_end  { b->marker = MKDT_MARK_ULIST; b->content = (const u8c *)(p + 1); }
    action olist_end  { b->marker = MKDT_MARK_OLIST; b->content = (const u8c *)(p + 1); }
    action todo_end   { b->marker = MKDT_MARK_TODO;  b->content = (const u8c *)(p + 1); }
    action h1_end     { b->heading = 1; b->content = (const u8c *)(p + 1); }
    action h2_end     { b->heading = 2; b->content = (const u8c *)(p + 1); }
    action h3_end     { b->heading = 3; b->content = (const u8c *)(p + 1); }
    action h4_end     { b->heading = 4; b->content = (const u8c *)(p + 1); }
    action f_start    { fs = p; }
    action f_end      { b->fence = (int)(p + 1 - fs); b->content = (const u8c *)(p + 1); }
    action hr_run     { hre = p + 1; }
    action hr_set     { b->hrule = YES; b->content = (const u8c *)hre; }
    action rd_set     { b->refdef = YES; b->content = (const u8c *)(p + 1); }

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
    refdef = '[' alnm ']' ':' @rd_set ;

    #  ATX header (DOG-024): a run of 1-4 '#' padded with spaces to fill the
    #  quad, in any column, exactly like quote/bullet; the level is the count
    #  of '#'.  A full quad needs no gap space, so "####" is a level-4 header.
    head1  = ('#' sp sp sp) | (sp '#' sp sp) | (sp sp '#' sp) | (sp sp sp '#') ;
    head2  = ('#' '#' sp sp) | (sp '#' '#' sp) | (sp sp '#' '#') ;
    head3  = ('#' '#' '#' sp) | (sp '#' '#' '#') ;
    head4  = ('#' '#' '#' '#') ;
    heading = head1 @h1_end | head2 @h2_end | head3 @h3_end | head4 @h4_end ;

    #  Code fence: a run of >=3 backticks (the wrapper accepts only 3/4).  The
    #  width/content are set by a finishing action so a lone 1-2 backtick run
    #  (inline code at line start) never matches and never touches content; the
    #  blank-vs-info rest is decided in C from the recorded run end.
    fence  = ('`' '`' '`' '`'*) >f_start @f_end ;

    #  Ruler: 3-4 dashes, blank rest, terminating newline (3-dash short form).
    #  DOG-024: `content` ends the dash run, so the ruler emits as one token.
    hrule  = ('-' '-' '-' '-'?) @hr_run ws* nl @hr_set ;

    #  DOG-024: the block stack is `(INDENT|QUOTE)* LIST? LEAF?` per
    #  wiki/StrictMark — a quote quad is a repeatable container like an
    #  indent, so `>    1. x` is a numbered item inside a quote, not a
    #  quote whose text happens to start with "1.".
    prefix = (indent @indent_end | quote @quote_end)* ;
    list   = bullet @ulist_end | number @olist_end | todo @todo_end ;

    main := prefix ( list | heading | fence | hrule | refdef )? ;
}%%

%% write data;

void MKDTBlock(u8csc line, mkdtblock *b) {
    b->depth = 0;
    b->quads = 0;
    b->quotes = 0;
    b->qend = NULL;
    b->marker = MKDT_MARK_NONE;
    b->heading = 0;
    b->fence = 0;
    b->fence_blank = NO;
    b->hrule = NO;
    b->refdef = NO;
    a_dup(u8c, data, line);
    const unsigned char *p = (const unsigned char *)data[0];
    const unsigned char *pe = (const unsigned char *)data[1];
    const unsigned char *eof = pe;
    const unsigned char *hre = p, *fs = p;
    b->content = (const u8c *)p;   // default: content begins after the indents
    int cs;
    %% write init;
    %% write exec;

    //  A fence closes a block iff the rest after its backtick run is blank; an
    //  info string makes it an opener.  `content` is the byte past the run.
    if (b->fence >= 3) {
        b->fence_blank = YES;
        for (const unsigned char *q = (const unsigned char *)b->content; q < pe;
             ++q)
            if (*q != ' ' && *q != '\t' && *q != '\r' && *q != '\n') {
                b->fence_blank = NO;
                break;
            }
    }
    (void)eof; (void)hre; (void)fs;
    (void)mkdtb_en_main; (void)mkdtb_error; (void)mkdtb_first_final;
}
