
/* #line 1 "MKDTB.c.rl" */
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


/* #line 106 "MKDTB.c.rl" */



/* #line 32 "MKDTB.rl.c" */
static const char _mkdtb_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13
};

static const char _mkdtb_key_offsets[] = {
	0, 0, 6, 12, 16, 18, 19, 20, 
	21, 23, 24, 26, 27, 30, 31, 32, 
	34, 35, 37, 38, 40, 43, 44, 49, 
	53, 59, 60, 63, 64, 65, 71, 72, 
	73, 74, 75, 84, 84
};

static const unsigned char _mkdtb_trans_keys[] = {
	32u, 35u, 45u, 62u, 48u, 57u, 32u, 35u, 
	45u, 62u, 48u, 57u, 32u, 35u, 45u, 62u, 
	32u, 35u, 32u, 46u, 32u, 32u, 35u, 32u, 
	32u, 35u, 32u, 46u, 48u, 57u, 32u, 32u, 
	32u, 35u, 32u, 32u, 35u, 32u, 32u, 35u, 
	32u, 45u, 91u, 45u, 9u, 10u, 13u, 32u, 
	45u, 9u, 10u, 13u, 32u, 32u, 45u, 86u, 
	88u, 118u, 120u, 93u, 46u, 48u, 57u, 32u, 
	32u, 48u, 57u, 65u, 90u, 97u, 122u, 93u, 
	58u, 96u, 96u, 9u, 32u, 35u, 45u, 62u, 
	91u, 96u, 48u, 57u, 96u, 0
};

static const char _mkdtb_single_lengths[] = {
	0, 4, 4, 4, 2, 1, 1, 1, 
	2, 1, 2, 1, 1, 1, 1, 2, 
	1, 2, 1, 2, 3, 1, 5, 4, 
	6, 1, 1, 1, 1, 0, 1, 1, 
	1, 1, 7, 0, 1
};

static const char _mkdtb_range_lengths[] = {
	0, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 0, 0, 3, 0, 0, 
	0, 0, 1, 0, 0
};

static const unsigned char _mkdtb_index_offsets[] = {
	0, 0, 6, 12, 17, 20, 22, 24, 
	26, 29, 31, 34, 36, 39, 41, 43, 
	46, 48, 51, 53, 56, 60, 62, 68, 
	73, 80, 82, 85, 87, 89, 93, 95, 
	97, 99, 101, 110, 111
};

static const char _mkdtb_indicies[] = {
	0, 2, 3, 5, 4, 1, 6, 7, 
	8, 10, 9, 1, 11, 12, 13, 14, 
	1, 12, 15, 1, 13, 1, 16, 1, 
	14, 1, 17, 18, 1, 12, 1, 15, 
	19, 1, 8, 1, 20, 9, 1, 16, 
	1, 10, 1, 21, 22, 1, 17, 1, 
	23, 24, 1, 15, 1, 19, 25, 1, 
	3, 26, 27, 1, 28, 1, 29, 30, 
	29, 29, 31, 1, 29, 30, 29, 29, 
	1, 32, 32, 32, 32, 32, 32, 1, 
	33, 1, 34, 4, 1, 20, 1, 5, 
	1, 35, 35, 35, 1, 36, 1, 37, 
	1, 38, 1, 39, 1, 11, 40, 41, 
	42, 44, 45, 46, 43, 1, 1, 39, 
	1, 0
};

static const char _mkdtb_trans_targs[] = {
	2, 0, 8, 11, 12, 14, 3, 4, 
	5, 6, 7, 34, 35, 35, 34, 35, 
	35, 9, 10, 35, 13, 16, 17, 18, 
	19, 35, 21, 24, 22, 23, 35, 23, 
	25, 35, 27, 30, 31, 35, 33, 36, 
	1, 15, 20, 26, 28, 29, 32
};

static const char _mkdtb_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 11, 5, 3, 13, 
	7, 0, 0, 15, 0, 0, 0, 0, 
	0, 17, 0, 0, 23, 0, 25, 23, 
	0, 9, 0, 0, 0, 27, 0, 21, 
	0, 0, 0, 0, 0, 0, 19
};

static const int mkdtb_start = 34;
static const int mkdtb_first_final = 34;
static const int mkdtb_error = 0;

static const int mkdtb_en_main = 34;


/* #line 109 "MKDTB.c.rl" */

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
    
/* #line 146 "MKDTB.rl.c" */
	{
	cs = mkdtb_start;
	}

/* #line 129 "MKDTB.c.rl" */
    
/* #line 149 "MKDTB.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _mkdtb_trans_keys + _mkdtb_key_offsets[cs];
	_trans = _mkdtb_index_offsets[cs];

	_klen = _mkdtb_single_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _mkdtb_range_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _mkdtb_indicies[_trans];
	cs = _mkdtb_trans_targs[_trans];

	if ( _mkdtb_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _mkdtb_actions + _mkdtb_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 33 "MKDTB.c.rl" */
	{
        if (b->quotes == 0) b->depth += 1;   //  depth counts the LEADING indents
        b->quads += 1;
        b->content = (const u8c *)(p + 1);
    }
	break;
	case 1:
/* #line 38 "MKDTB.c.rl" */
	{
        b->quads += 1;
        b->quotes += 1;
        if (b->quotes == 1) b->qend = (const u8c *)(p + 1);
        b->content = (const u8c *)(p + 1);
    }
	break;
	case 2:
/* #line 44 "MKDTB.c.rl" */
	{ b->marker = MKDT_MARK_ULIST; b->content = (const u8c *)(p + 1); }
	break;
	case 3:
/* #line 45 "MKDTB.c.rl" */
	{ b->marker = MKDT_MARK_OLIST; b->content = (const u8c *)(p + 1); }
	break;
	case 4:
/* #line 46 "MKDTB.c.rl" */
	{ b->marker = MKDT_MARK_TODO;  b->content = (const u8c *)(p + 1); }
	break;
	case 5:
/* #line 47 "MKDTB.c.rl" */
	{ b->heading = 1; b->content = (const u8c *)(p + 1); }
	break;
	case 6:
/* #line 48 "MKDTB.c.rl" */
	{ b->heading = 2; b->content = (const u8c *)(p + 1); }
	break;
	case 7:
/* #line 49 "MKDTB.c.rl" */
	{ b->heading = 3; b->content = (const u8c *)(p + 1); }
	break;
	case 8:
/* #line 50 "MKDTB.c.rl" */
	{ b->heading = 4; b->content = (const u8c *)(p + 1); }
	break;
	case 9:
/* #line 51 "MKDTB.c.rl" */
	{ fs = p; }
	break;
	case 10:
/* #line 52 "MKDTB.c.rl" */
	{ b->fence = (int)(p + 1 - fs); b->content = (const u8c *)(p + 1); }
	break;
	case 11:
/* #line 53 "MKDTB.c.rl" */
	{ hre = p + 1; }
	break;
	case 12:
/* #line 54 "MKDTB.c.rl" */
	{ b->hrule = YES; b->content = (const u8c *)hre; }
	break;
	case 13:
/* #line 55 "MKDTB.c.rl" */
	{ b->refdef = YES; b->content = (const u8c *)(p + 1); }
	break;
/* #line 273 "MKDTB.rl.c" */
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

/* #line 130 "MKDTB.c.rl" */

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
