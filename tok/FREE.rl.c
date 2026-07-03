
/* #line 1 "dog/tok/FREE.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "FREE.h"


/* #line 104 "dog/tok/FREE.c.rl" */



/* #line 8 "dog/tok/FREE.rl.c" */
static const char _FREE_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 1, 18, 1, 
	19, 1, 20, 1, 21, 1, 22, 1, 
	23, 1, 24, 1, 25, 1, 26, 1, 
	27, 2, 2, 3, 2, 2, 4, 2, 
	2, 5, 2, 2, 6
};

static const unsigned char _FREE_key_offsets[] = {
	0, 2, 4, 5, 11, 12, 14, 20, 
	22, 24, 30, 31, 33, 35, 39, 41, 
	43, 69, 73, 74, 78, 80, 85, 87, 
	90, 96, 110, 112, 121, 123, 124, 138, 
	148, 150
};

static const unsigned char _FREE_trans_keys[] = {
	10u, 93u, 10u, 93u, 91u, 48u, 57u, 65u, 
	90u, 97u, 122u, 93u, 10u, 42u, 48u, 57u, 
	65u, 70u, 97u, 102u, 48u, 57u, 10u, 93u, 
	48u, 57u, 65u, 90u, 97u, 122u, 93u, 10u, 
	95u, 10u, 96u, 32u, 126u, 9u, 13u, 10u, 
	126u, 10u, 126u, 10u, 32u, 33u, 42u, 46u, 
	48u, 91u, 95u, 96u, 126u, 0u, 8u, 9u, 
	13u, 14u, 47u, 49u, 57u, 58u, 64u, 65u, 
	90u, 92u, 94u, 123u, 127u, 9u, 32u, 11u, 
	13u, 91u, 32u, 42u, 9u, 13u, 48u, 57u, 
	46u, 88u, 120u, 48u, 57u, 48u, 57u, 46u, 
	48u, 57u, 48u, 57u, 65u, 70u, 97u, 102u, 
	45u, 95u, 0u, 47u, 48u, 57u, 58u, 64u, 
	65u, 90u, 91u, 96u, 123u, 127u, 48u, 57u, 
	96u, 0u, 47u, 58u, 64u, 91u, 94u, 123u, 
	127u, 10u, 93u, 91u, 32u, 95u, 0u, 8u, 
	9u, 13u, 14u, 47u, 58u, 64u, 91u, 96u, 
	123u, 127u, 10u, 95u, 0u, 47u, 58u, 64u, 
	91u, 96u, 123u, 127u, 10u, 96u, 126u, 0
};

static const char _FREE_single_lengths[] = {
	2, 2, 1, 0, 1, 2, 0, 0, 
	2, 0, 1, 2, 2, 2, 2, 2, 
	10, 2, 1, 2, 0, 3, 0, 1, 
	0, 2, 0, 1, 2, 1, 2, 2, 
	2, 1
};

static const char _FREE_range_lengths[] = {
	0, 0, 0, 3, 0, 0, 3, 1, 
	0, 3, 0, 0, 0, 1, 0, 0, 
	8, 1, 0, 1, 1, 1, 1, 1, 
	3, 6, 1, 4, 0, 0, 6, 4, 
	0, 0
};

static const unsigned char _FREE_index_offsets[] = {
	0, 3, 6, 8, 12, 14, 17, 21, 
	23, 26, 30, 32, 35, 38, 42, 45, 
	48, 67, 71, 73, 77, 79, 84, 86, 
	89, 93, 102, 104, 110, 113, 115, 124, 
	131, 134
};

static const char _FREE_indicies[] = {
	0, 0, 1, 0, 2, 1, 3, 0, 
	4, 4, 4, 0, 5, 0, 0, 7, 
	6, 9, 9, 9, 8, 11, 10, 0, 
	13, 12, 15, 15, 15, 14, 16, 14, 
	10, 18, 17, 0, 20, 19, 0, 0, 
	0, 21, 0, 22, 21, 0, 23, 21, 
	26, 25, 27, 28, 29, 30, 33, 34, 
	35, 37, 24, 25, 24, 31, 24, 32, 
	24, 24, 36, 25, 25, 25, 38, 40, 
	39, 39, 39, 39, 6, 42, 41, 44, 
	45, 45, 31, 43, 44, 46, 44, 31, 
	43, 9, 9, 9, 47, 49, 32, 48, 
	32, 48, 32, 48, 48, 36, 11, 50, 
	41, 41, 41, 41, 41, 36, 39, 39, 
	12, 52, 51, 48, 36, 17, 48, 17, 
	17, 17, 17, 53, 48, 54, 17, 17, 
	17, 17, 53, 39, 39, 19, 55, 39, 
	0
};

static const char _FREE_trans_targs[] = {
	16, 1, 2, 3, 4, 16, 5, 16, 
	16, 24, 16, 26, 8, 29, 16, 10, 
	16, 11, 16, 12, 16, 14, 15, 16, 
	16, 17, 16, 18, 19, 20, 21, 23, 
	25, 28, 30, 32, 27, 33, 16, 16, 
	0, 16, 20, 16, 22, 6, 16, 16, 
	16, 7, 16, 16, 9, 31, 27, 13
};

static const char _FREE_trans_actions[] = {
	45, 0, 0, 0, 0, 17, 0, 9, 
	41, 0, 43, 0, 0, 5, 39, 0, 
	15, 0, 11, 0, 7, 0, 0, 13, 
	21, 0, 19, 5, 5, 58, 5, 0, 
	5, 5, 5, 5, 55, 5, 35, 37, 
	0, 47, 52, 31, 0, 0, 29, 27, 
	33, 0, 23, 25, 0, 5, 49, 0
};

static const char _FREE_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const char _FREE_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const unsigned char _FREE_eof_trans[] = {
	1, 1, 1, 1, 1, 1, 9, 11, 
	1, 15, 15, 11, 1, 1, 1, 1, 
	0, 39, 40, 40, 42, 44, 47, 44, 
	48, 49, 51, 42, 40, 52, 49, 49, 
	40, 40
};

static const int FREE_start = 16;
static const int FREE_first_final = 16;
static const int FREE_error = -1;

static const int FREE_en_main = 16;


/* #line 107 "dog/tok/FREE.c.rl" */

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

    
/* #line 161 "dog/tok/FREE.rl.c" */
	{
	cs = FREE_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 125 "dog/tok/FREE.c.rl" */
    
/* #line 167 "dog/tok/FREE.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_acts = _FREE_actions + _FREE_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 184 "dog/tok/FREE.rl.c" */
		}
	}

	_keys = _FREE_trans_keys + _FREE_key_offsets[cs];
	_trans = _FREE_index_offsets[cs];

	_klen = _FREE_single_lengths[cs];
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

	_klen = _FREE_range_lengths[cs];
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
	_trans = _FREE_indicies[_trans];
_eof_trans:
	cs = _FREE_trans_targs[_trans];

	if ( _FREE_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _FREE_actions + _FREE_trans_actions[_trans];
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
/* #line 58 "dog/tok/FREE.c.rl" */
	{act = 4;}
	break;
	case 4:
/* #line 36 "dog/tok/FREE.c.rl" */
	{act = 11;}
	break;
	case 5:
/* #line 26 "dog/tok/FREE.c.rl" */
	{act = 13;}
	break;
	case 6:
/* #line 41 "dog/tok/FREE.c.rl" */
	{act = 16;}
	break;
	case 7:
/* #line 53 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('H', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 8:
/* #line 58 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 9:
/* #line 58 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 10:
/* #line 58 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 11:
/* #line 63 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 12:
/* #line 63 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 13:
/* #line 46 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('W', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 14:
/* #line 41 "dog/tok/FREE.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('P', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 15:
/* #line 31 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('F', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 16:
/* #line 63 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 17:
/* #line 36 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('L', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 18:
/* #line 36 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('L', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 19:
/* #line 36 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('L', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 20:
/* #line 26 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('S', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 21:
/* #line 46 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('W', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 22:
/* #line 41 "dog/tok/FREE.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('P', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 23:
/* #line 63 "dog/tok/FREE.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 24:
/* #line 36 "dog/tok/FREE.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('L', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 25:
/* #line 26 "dog/tok/FREE.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('S', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 26:
/* #line 41 "dog/tok/FREE.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('P', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}}
	break;
	case 27:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 4:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('G', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}
	break;
	case 11:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('L', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}
	break;
	case 13:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('S', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}
	break;
	case 16:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    if (state->cb) { o = state->cb('P', tok, state->ctx); if (o!=OK) {p++; goto _out; } }
}
	break;
	}
	}
	break;
/* #line 437 "dog/tok/FREE.rl.c" */
		}
	}

_again:
	_acts = _FREE_actions + _FREE_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 448 "dog/tok/FREE.rl.c" */
		}
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _FREE_eof_trans[cs] > 0 ) {
		_trans = _FREE_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 126 "dog/tok/FREE.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < FREE_first_final)
        o = FREEBAD;

    return o;
}
