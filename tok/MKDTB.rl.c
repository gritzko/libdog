
/* #line 1 "MKDTB.c.rl" */
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


/* #line 98 "MKDTB.c.rl" */



/* #line 35 "MKDTB.rl.c" */
static const char _mkdtb_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5
};

static const char _mkdtb_key_offsets[] = {
	0, 0, 6, 12, 16, 18, 19, 20, 
	21, 23, 24, 27, 28, 30, 33, 34, 
	39, 43, 49, 50, 53, 54, 56, 60, 
	61, 62, 68, 69, 70, 71, 72, 83, 
	83
};

static const unsigned char _mkdtb_trans_keys[] = {
	32u, 35u, 45u, 62u, 48u, 57u, 32u, 35u, 
	45u, 62u, 48u, 57u, 32u, 35u, 45u, 62u, 
	32u, 35u, 32u, 46u, 32u, 32u, 35u, 32u, 
	46u, 48u, 57u, 32u, 32u, 35u, 32u, 45u, 
	91u, 45u, 9u, 10u, 13u, 32u, 45u, 9u, 
	10u, 13u, 32u, 32u, 45u, 86u, 88u, 118u, 
	120u, 93u, 46u, 48u, 57u, 32u, 97u, 122u, 
	48u, 57u, 97u, 122u, 58u, 32u, 48u, 57u, 
	65u, 90u, 97u, 122u, 93u, 58u, 96u, 96u, 
	9u, 32u, 35u, 45u, 62u, 91u, 96u, 48u, 
	57u, 65u, 90u, 96u, 0
};

static const char _mkdtb_single_lengths[] = {
	0, 4, 4, 4, 2, 1, 1, 1, 
	2, 1, 1, 1, 2, 3, 1, 5, 
	4, 6, 1, 1, 1, 0, 0, 1, 
	1, 0, 1, 1, 1, 1, 7, 0, 
	1
};

static const char _mkdtb_range_lengths[] = {
	0, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 0, 1, 2, 0, 
	0, 3, 0, 0, 0, 0, 2, 0, 
	0
};

static const unsigned char _mkdtb_index_offsets[] = {
	0, 0, 6, 12, 17, 20, 22, 24, 
	26, 29, 31, 34, 36, 39, 43, 45, 
	51, 56, 63, 65, 68, 70, 72, 75, 
	77, 79, 83, 85, 87, 89, 91, 101, 
	102
};

static const char _mkdtb_indicies[] = {
	0, 2, 3, 5, 4, 1, 6, 7, 
	8, 10, 9, 1, 11, 12, 12, 11, 
	1, 12, 12, 1, 12, 1, 12, 1, 
	11, 1, 8, 7, 1, 8, 1, 8, 
	9, 1, 10, 1, 3, 2, 1, 3, 
	13, 14, 1, 15, 1, 16, 17, 16, 
	16, 18, 1, 16, 17, 16, 16, 1, 
	19, 19, 19, 19, 19, 19, 1, 12, 
	1, 3, 4, 1, 5, 1, 20, 1, 
	21, 21, 1, 22, 1, 23, 1, 24, 
	24, 24, 1, 25, 1, 12, 1, 26, 
	1, 27, 1, 11, 28, 29, 30, 32, 
	34, 35, 31, 33, 1, 1, 27, 1, 
	0
};

static const char _mkdtb_trans_targs[] = {
	2, 0, 8, 9, 10, 11, 3, 4, 
	5, 6, 7, 30, 31, 14, 17, 15, 
	16, 31, 16, 18, 22, 23, 24, 31, 
	26, 27, 29, 32, 1, 12, 13, 19, 
	20, 21, 25, 28
};

static const char _mkdtb_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 3, 0, 0, 5, 
	0, 7, 5, 0, 0, 0, 9, 11, 
	0, 0, 0, 3, 0, 0, 0, 0, 
	0, 0, 0, 0
};

static const int mkdtb_start = 30;
static const int mkdtb_first_final = 30;
static const int mkdtb_error = 0;

static const int mkdtb_en_main = 30;


/* #line 101 "MKDTB.c.rl" */

void MKDTBlock(u8csc line, mkdtblock *b) {
    a_dup(u8c, data, line);
    const unsigned char *p = (const unsigned char *)data[0];
    const unsigned char *pe = (const unsigned char *)data[1];
    const unsigned char *eof = pe;
    const unsigned char *qe = p, *me = p, *hre = p, *mke = p;
    int cs;
    
/* #line 133 "MKDTB.rl.c" */
	{
	cs = mkdtb_start;
	}

/* #line 110 "MKDTB.c.rl" */
    
/* #line 136 "MKDTB.rl.c" */
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
/* #line 36 "MKDTB.c.rl" */
	{ qe = p + 1; }
	break;
	case 1:
/* #line 37 "MKDTB.c.rl" */
	{ me = p + 1; }
	break;
	case 2:
/* #line 40 "MKDTB.c.rl" */
	{ hre = p + 1; }
	break;
	case 3:
/* #line 41 "MKDTB.c.rl" */
	{ me = hre; }
	break;
	case 4:
/* #line 42 "MKDTB.c.rl" */
	{ mke = p + 1; }
	break;
	case 5:
/* #line 43 "MKDTB.c.rl" */
	{ me = mke; }
	break;
/* #line 227 "MKDTB.rl.c" */
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

/* #line 111 "MKDTB.c.rl" */

    //  The one pointer→slice boundary: the machine's recorded region ends
    //  become the three slices.  No marker matched leaves `mark` empty.
    if (me < qe) me = qe;
    b->quads[0] = data[0];       b->quads[1] = (const u8c *)qe;
    b->mark[0] = (const u8c *)qe; b->mark[1] = (const u8c *)me;
    b->rest[0] = (const u8c *)me; b->rest[1] = data[1];
    (void)eof; (void)hre; (void)mke;
    (void)mkdtb_en_main; (void)mkdtb_error; (void)mkdtb_first_final;
}
