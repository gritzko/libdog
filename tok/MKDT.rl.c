
/* #line 1 "MKDT.c.rl" */
//  The StrictMark INLINE machine.  A span may cross a newline, because a
//  paragraph is broken by a blank line, not by a line end (DOG-024): the
//  block layer hands this machine a whole paragraph run, exactly as the
//  renderers hand their joined paragraph buffer to it (`mark_para_flush`
//  in beagle/mark/MARK.c, `paraFlush` in be/verbs/mark/render.js).  The
//  span bodies therefore admit `\n`; each still ends at its first closer,
//  so nothing runs away past the paragraph.

#include "abc/INT.h"
#include "abc/PRO.h"
#include "MKDT.h"

ok64 MKDTonEmph (u8cs tok, MKDTstate* state);
ok64 MKDTonCode (u8cs tok, MKDTstate* state);
ok64 MKDTonLink (u8cs tok, MKDTstate* state);
ok64 MKDTonNumber (u8cs tok, MKDTstate* state);
ok64 MKDTonWord (u8cs tok, MKDTstate* state);
ok64 MKDTonKey (u8cs tok, MKDTstate* state);
ok64 MKDTonPunct (u8cs tok, MKDTstate* state);
ok64 MKDTonSpace (u8cs tok, MKDTstate* state);
ok64 MKDTonEscape (u8cs tok, MKDTstate* state);


/* #line 157 "MKDT.c.rl" */



/* #line 26 "MKDT.rl.c" */
static const char _MKDT_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 1, 18, 1, 
	19, 1, 20, 1, 21, 1, 22, 1, 
	23, 1, 24, 1, 25, 1, 26, 1, 
	27, 1, 28, 1, 29, 1, 30, 1, 
	31, 2, 2, 3, 2, 2, 4, 2, 
	2, 5, 2, 2, 6
};

static const unsigned char _MKDT_key_offsets[] = {
	0, 2, 4, 4, 5, 11, 12, 14, 
	14, 20, 22, 24, 24, 30, 31, 33, 
	33, 34, 36, 36, 73, 77, 78, 82, 
	84, 89, 91, 94, 100, 108, 110, 117, 
	119, 120, 123, 133, 141, 142, 146
};

static const unsigned char _MKDT_trans_keys[] = {
	92u, 93u, 92u, 93u, 91u, 48u, 57u, 65u, 
	90u, 97u, 122u, 93u, 42u, 92u, 48u, 57u, 
	65u, 70u, 97u, 102u, 48u, 57u, 92u, 93u, 
	48u, 57u, 65u, 90u, 97u, 122u, 93u, 92u, 
	95u, 96u, 92u, 126u, 10u, 32u, 33u, 42u, 
	46u, 48u, 63u, 91u, 92u, 95u, 96u, 126u, 
	127u, 0u, 8u, 9u, 13u, 14u, 31u, 34u, 
	35u, 36u, 37u, 38u, 47u, 49u, 57u, 58u, 
	64u, 65u, 90u, 93u, 94u, 97u, 122u, 123u, 
	125u, 9u, 32u, 11u, 13u, 91u, 32u, 42u, 
	9u, 13u, 48u, 57u, 46u, 88u, 120u, 48u, 
	57u, 48u, 57u, 46u, 48u, 57u, 48u, 57u, 
	65u, 70u, 97u, 102u, 45u, 95u, 48u, 57u, 
	65u, 90u, 97u, 122u, 48u, 57u, 95u, 48u, 
	57u, 65u, 90u, 97u, 122u, 92u, 93u, 91u, 
	32u, 9u, 13u, 32u, 95u, 9u, 13u, 48u, 
	57u, 65u, 90u, 97u, 122u, 92u, 95u, 48u, 
	57u, 65u, 90u, 97u, 122u, 96u, 32u, 126u, 
	9u, 13u, 128u, 191u, 0
};

static const char _MKDT_single_lengths[] = {
	2, 2, 0, 1, 0, 1, 2, 0, 
	0, 0, 2, 0, 0, 1, 2, 0, 
	1, 2, 0, 13, 2, 1, 2, 0, 
	3, 0, 1, 0, 2, 0, 1, 2, 
	1, 1, 2, 2, 1, 2, 0
};

static const char _MKDT_range_lengths[] = {
	0, 0, 0, 0, 3, 0, 0, 0, 
	3, 1, 0, 0, 3, 0, 0, 0, 
	0, 0, 0, 12, 1, 0, 1, 1, 
	1, 1, 1, 3, 3, 1, 3, 0, 
	0, 1, 4, 3, 0, 1, 1
};

static const unsigned char _MKDT_index_offsets[] = {
	0, 3, 6, 7, 9, 13, 15, 18, 
	19, 23, 25, 28, 29, 33, 35, 38, 
	39, 41, 44, 45, 71, 75, 77, 81, 
	83, 88, 90, 93, 97, 103, 105, 110, 
	113, 115, 118, 125, 131, 133, 137
};

static const char _MKDT_indicies[] = {
	2, 0, 1, 2, 3, 1, 1, 4, 
	0, 5, 5, 5, 0, 6, 0, 8, 
	9, 7, 7, 11, 11, 11, 10, 13, 
	12, 15, 16, 14, 14, 18, 18, 18, 
	17, 19, 17, 21, 22, 20, 20, 24, 
	23, 26, 27, 25, 25, 30, 29, 31, 
	33, 34, 35, 28, 38, 39, 40, 41, 
	43, 28, 28, 29, 28, 32, 28, 32, 
	36, 32, 37, 32, 42, 32, 44, 29, 
	29, 29, 45, 47, 46, 46, 46, 46, 
	7, 49, 48, 51, 52, 52, 36, 50, 
	51, 53, 51, 36, 50, 11, 11, 11, 
	54, 56, 37, 37, 37, 42, 55, 13, 
	57, 42, 42, 42, 42, 48, 15, 46, 
	14, 59, 58, 46, 46, 60, 55, 42, 
	55, 61, 61, 61, 20, 21, 62, 61, 
	61, 61, 20, 46, 23, 46, 63, 46, 
	25, 44, 64, 0
};

static const char _MKDT_trans_targs[] = {
	19, 1, 2, 3, 4, 5, 19, 6, 
	19, 7, 19, 27, 19, 29, 10, 11, 
	32, 19, 13, 19, 14, 15, 19, 16, 
	19, 17, 18, 19, 19, 20, 19, 21, 
	19, 22, 23, 24, 26, 28, 31, 33, 
	34, 36, 30, 37, 38, 19, 19, 0, 
	19, 23, 19, 25, 8, 19, 19, 19, 
	9, 19, 19, 12, 19, 35, 30, 19, 
	19
};

static const char _MKDT_trans_actions[] = {
	53, 0, 0, 0, 0, 0, 19, 0, 
	11, 0, 49, 0, 51, 0, 0, 0, 
	5, 47, 0, 17, 0, 0, 13, 0, 
	7, 0, 0, 15, 27, 0, 25, 5, 
	23, 5, 66, 5, 0, 5, 5, 0, 
	5, 5, 63, 5, 0, 43, 41, 0, 
	55, 60, 35, 0, 0, 33, 31, 39, 
	0, 37, 29, 0, 9, 5, 57, 21, 
	45
};

static const char _MKDT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const char _MKDT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const unsigned char _MKDT_eof_trans[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 
	11, 13, 1, 1, 18, 18, 13, 13, 
	1, 1, 1, 0, 46, 47, 47, 49, 
	51, 54, 51, 55, 56, 58, 49, 47, 
	59, 47, 56, 56, 47, 47, 65
};

static const int MKDT_start = 19;
static const int MKDT_first_final = 19;
static const int MKDT_error = -1;

static const int MKDT_en_main = 19;


/* #line 160 "MKDT.c.rl" */

ok64 MKDTInlineLexer(MKDTstate* state) {

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

    
/* #line 184 "MKDT.rl.c" */
	{
	cs = MKDT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 178 "MKDT.c.rl" */
    
/* #line 190 "MKDT.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_acts = _MKDT_actions + _MKDT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 207 "MKDT.rl.c" */
		}
	}

	_keys = _MKDT_trans_keys + _MKDT_key_offsets[cs];
	_trans = _MKDT_index_offsets[cs];

	_klen = _MKDT_single_lengths[cs];
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

	_klen = _MKDT_range_lengths[cs];
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
	_trans = _MKDT_indicies[_trans];
_eof_trans:
	cs = _MKDT_trans_targs[_trans];

	if ( _MKDT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _MKDT_actions + _MKDT_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 2:
/* #line 1 "NONE" */
	{te = p+1;}
	break;
	case 3:
/* #line 38 "MKDT.c.rl" */
	{act = 4;}
	break;
	case 4:
/* #line 56 "MKDT.c.rl" */
	{act = 11;}
	break;
	case 5:
/* #line 62 "MKDT.c.rl" */
	{act = 15;}
	break;
	case 6:
/* #line 74 "MKDT.c.rl" */
	{act = 16;}
	break;
	case 7:
/* #line 44 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonCode(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 8:
/* #line 86 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonEscape(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 9:
/* #line 38 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonEmph(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 10:
/* #line 38 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonEmph(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 11:
/* #line 38 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonEmph(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 12:
/* #line 50 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonLink(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 50 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonLink(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 74 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 74 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 80 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 74 "MKDT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 50 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonLink(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 56 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 56 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 56 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 68 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonKey(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 62 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 74 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 80 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 62 "MKDT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 50 "MKDT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonLink(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 56 "MKDT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 62 "MKDT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 30:
/* #line 74 "MKDT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 31:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 4:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonEmph(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 11:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 15:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 16:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = MKDTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 516 "MKDT.rl.c" */
		}
	}

_again:
	_acts = _MKDT_actions + _MKDT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 527 "MKDT.rl.c" */
		}
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _MKDT_eof_trans[cs] > 0 ) {
		_trans = _MKDT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 179 "MKDT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < MKDT_first_final)
        o = MKDTBAD;

    return o;
}

//  ---- inline span decomposer (folded in from the former mark/MARKG) ----
//
//  MKDTInlineLexer isolates an emphasis/link/image span as one 'G' token; this
//  second machine splits that span into (kind, text, label) so a renderer emits
//  <strong>/<em>/<del>/<a>/<img> without re-scanning.  The explicit label l is
//  one symbol; a shortcut [page] carries none, so it keys on the bracket text.


/* #line 218 "MKDT.c.rl" */



/* #line 560 "MKDT.rl.c" */
static const char _mkdtg_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 2, 
	0, 1, 2, 1, 4, 2, 1, 5, 
	2, 1, 6, 2, 1, 7, 2, 3, 
	7, 2, 3, 8, 3, 0, 1, 4, 
	3, 0, 1, 5, 3, 0, 1, 6, 
	3, 0, 1, 7
};

static const char _mkdtg_key_offsets[] = {
	0, 0, 5, 6, 8, 10, 10, 11, 
	17, 18, 20, 22, 22, 24, 26, 26, 
	32, 33, 35, 37, 37, 39, 41, 41, 
	41
};

static const unsigned char _mkdtg_trans_keys[] = {
	33u, 42u, 91u, 95u, 126u, 91u, 92u, 93u, 
	92u, 93u, 91u, 48u, 57u, 65u, 90u, 97u, 
	122u, 93u, 42u, 92u, 42u, 92u, 92u, 93u, 
	92u, 93u, 48u, 57u, 65u, 90u, 97u, 122u, 
	93u, 92u, 95u, 92u, 95u, 92u, 126u, 92u, 
	126u, 91u, 0
};

static const char _mkdtg_single_lengths[] = {
	0, 5, 1, 2, 2, 0, 1, 0, 
	1, 2, 2, 0, 2, 2, 0, 0, 
	1, 2, 2, 0, 2, 2, 0, 0, 
	1
};

static const char _mkdtg_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 3, 
	0, 0, 0, 0, 0, 0, 0, 3, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const char _mkdtg_index_offsets[] = {
	0, 0, 6, 8, 11, 14, 15, 17, 
	21, 23, 26, 29, 30, 33, 36, 37, 
	41, 43, 46, 49, 50, 53, 56, 57, 
	58
};

static const char _mkdtg_trans_targs[] = {
	2, 9, 12, 17, 20, 0, 3, 0, 
	5, 6, 4, 5, 6, 4, 4, 7, 
	0, 8, 8, 8, 0, 23, 0, 23, 
	11, 10, 23, 11, 10, 10, 14, 24, 
	13, 14, 24, 13, 13, 16, 16, 16, 
	0, 23, 0, 19, 23, 18, 19, 23, 
	18, 18, 22, 23, 21, 22, 23, 21, 
	21, 0, 15, 0, 0
};

static const char _mkdtg_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 7, 1, 0, 3, 0, 0, 0, 
	0, 5, 5, 5, 0, 25, 0, 28, 
	1, 1, 10, 0, 0, 0, 1, 40, 
	1, 0, 19, 0, 0, 5, 5, 5, 
	0, 22, 0, 1, 32, 1, 0, 13, 
	0, 0, 1, 36, 1, 0, 16, 0, 
	0, 0, 0, 0, 0
};

static const int mkdtg_start = 1;
static const int mkdtg_first_final = 23;
static const int mkdtg_error = 0;

static const int mkdtg_en_main = 1;


/* #line 221 "MKDT.c.rl" */

ok64 MKDTDecomposeSpan(mkdtspan *g, u8csc tok) {
    a_dup(u8c, data, tok);

    int cs;
    u8c *p = (u8c *)data[0];
    u8c *pe = (u8c *)data[1];
    u8c *eof = pe;
    u8c *txt0 = NULL, *txt1 = NULL, *lbl0 = NULL, *lbl1 = NULL;
    u8 kind = 0;

    
/* #line 645 "MKDT.rl.c" */
	{
	cs = mkdtg_start;
	}

/* #line 233 "MKDT.c.rl" */
    
/* #line 648 "MKDT.rl.c" */
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
	_keys = _mkdtg_trans_keys + _mkdtg_key_offsets[cs];
	_trans = _mkdtg_index_offsets[cs];

	_klen = _mkdtg_single_lengths[cs];
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

	_klen = _mkdtg_range_lengths[cs];
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
	cs = _mkdtg_trans_targs[_trans];

	if ( _mkdtg_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _mkdtg_actions + _mkdtg_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 198 "MKDT.c.rl" */
	{ txt0 = (u8c *)p; }
	break;
	case 1:
/* #line 199 "MKDT.c.rl" */
	{ txt1 = (u8c *)p; }
	break;
	case 2:
/* #line 200 "MKDT.c.rl" */
	{ lbl0 = (u8c *)p; }
	break;
	case 3:
/* #line 201 "MKDT.c.rl" */
	{ lbl1 = (u8c *)p; }
	break;
	case 4:
/* #line 202 "MKDT.c.rl" */
	{ kind = 'B'; }
	break;
	case 5:
/* #line 203 "MKDT.c.rl" */
	{ kind = 'I'; }
	break;
	case 6:
/* #line 204 "MKDT.c.rl" */
	{ kind = 'D'; }
	break;
	case 7:
/* #line 205 "MKDT.c.rl" */
	{ kind = 'A'; }
	break;
	case 8:
/* #line 206 "MKDT.c.rl" */
	{ kind = 'M'; }
	break;
/* #line 747 "MKDT.rl.c" */
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

/* #line 234 "MKDT.c.rl" */

    if (cs < mkdtg_first_final) {
        g->kind = 0;
        return OK;
    }
    g->kind = kind;
    g->text[0] = txt0 ? txt0 : (u8c *)data[1];
    g->text[1] = txt1 ? txt1 : (u8c *)data[1];
    if (lbl0 != NULL) {
        g->label[0] = lbl0;
        g->label[1] = lbl1;
    } else {
        //  shortcut: key the link on its bracket text
        g->label[0] = g->text[0];
        g->label[1] = g->text[1];
    }
    (void)eof;
    return OK;
}
