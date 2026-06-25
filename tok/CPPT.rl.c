
/* #line 1 "CPPT.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "CPPT.h"

ok64 CPPTonComment (u8cs tok, CPPTstate* state);
ok64 CPPTonString (u8cs tok, CPPTstate* state);
ok64 CPPTonNumber (u8cs tok, CPPTstate* state);
ok64 CPPTonPreproc (u8cs tok, CPPTstate* state);
ok64 CPPTonWord (u8cs tok, CPPTstate* state);
ok64 CPPTonPunct (u8cs tok, CPPTstate* state);
ok64 CPPTonSpace (u8cs tok, CPPTstate* state);


/* #line 166 "CPPT.c.rl" */



/* #line 16 "CPPT.rl.c" */
static const char _CPPT_actions[] = {
	0, 1, 0, 1, 4, 1, 5, 1, 
	17, 1, 19, 1, 20, 1, 21, 1, 
	22, 1, 23, 1, 24, 1, 25, 1, 
	26, 1, 27, 1, 28, 1, 29, 1, 
	30, 1, 31, 1, 32, 1, 33, 1, 
	34, 1, 35, 1, 36, 1, 37, 1, 
	38, 1, 39, 1, 40, 1, 41, 1, 
	42, 1, 43, 1, 44, 1, 45, 1, 
	46, 1, 47, 1, 48, 1, 49, 1, 
	50, 1, 51, 1, 52, 1, 53, 1, 
	54, 2, 1, 18, 2, 2, 3, 2, 
	5, 6, 2, 5, 7, 2, 5, 8, 
	2, 5, 9, 2, 5, 10, 2, 5, 
	11, 2, 5, 12, 2, 5, 13, 2, 
	5, 14, 2, 5, 15, 2, 5, 16, 
	3, 0, 1, 18
};

static const short _CPPT_key_offsets[] = {
	0, 0, 2, 19, 25, 31, 37, 43, 
	49, 55, 61, 67, 82, 84, 101, 107, 
	113, 119, 125, 131, 137, 143, 149, 150, 
	152, 156, 158, 159, 161, 163, 165, 169, 
	171, 175, 177, 183, 185, 191, 199, 208, 
	214, 218, 220, 227, 234, 241, 248, 255, 
	262, 269, 276, 283, 290, 297, 304, 311, 
	318, 325, 332, 333, 366, 369, 370, 385, 
	393, 402, 411, 420, 429, 438, 449, 459, 
	468, 477, 486, 495, 504, 513, 523, 533, 
	542, 551, 560, 569, 578, 587, 596, 605, 
	614, 623, 632, 641, 650, 659, 668, 677, 
	686, 695, 703, 705, 707, 710, 713, 724, 
	733, 736, 737, 750, 757, 767, 778, 787, 
	796, 804, 817, 821, 823, 825, 827, 834, 
	838, 840, 842, 844, 858, 867, 871, 873, 
	875, 877, 878, 880, 881, 883, 894, 903, 
	915
};

static const unsigned char _CPPT_trans_keys[] = {
	34u, 92u, 34u, 39u, 63u, 85u, 92u, 110u, 
	114u, 117u, 120u, 48u, 55u, 97u, 98u, 101u, 
	102u, 116u, 118u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 9u, 32u, 36u, 95u, 100u, 
	101u, 105u, 108u, 112u, 117u, 119u, 65u, 90u, 
	97u, 122u, 39u, 92u, 34u, 39u, 63u, 85u, 
	92u, 110u, 114u, 117u, 120u, 48u, 55u, 97u, 
	98u, 101u, 102u, 116u, 118u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 46u, 48u, 57u, 
	43u, 45u, 48u, 57u, 48u, 57u, 42u, 42u, 
	47u, 48u, 57u, 48u, 57u, 43u, 45u, 48u, 
	57u, 48u, 57u, 43u, 45u, 48u, 57u, 48u, 
	57u, 36u, 95u, 65u, 90u, 97u, 122u, 48u, 
	49u, 48u, 57u, 65u, 70u, 97u, 102u, 80u, 
	112u, 48u, 57u, 65u, 70u, 97u, 102u, 39u, 
	80u, 112u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 43u, 45u, 
	48u, 57u, 48u, 57u, 32u, 34u, 40u, 41u, 
	92u, 9u, 13u, 32u, 34u, 40u, 41u, 92u, 
	9u, 13u, 32u, 34u, 40u, 41u, 92u, 9u, 
	13u, 32u, 34u, 40u, 41u, 92u, 9u, 13u, 
	32u, 34u, 40u, 41u, 92u, 9u, 13u, 32u, 
	34u, 40u, 41u, 92u, 9u, 13u, 32u, 34u, 
	40u, 41u, 92u, 9u, 13u, 32u, 34u, 40u, 
	41u, 92u, 9u, 13u, 32u, 34u, 40u, 41u, 
	92u, 9u, 13u, 32u, 34u, 40u, 41u, 92u, 
	9u, 13u, 32u, 34u, 40u, 41u, 92u, 9u, 
	13u, 32u, 34u, 40u, 41u, 92u, 9u, 13u, 
	32u, 34u, 40u, 41u, 92u, 9u, 13u, 32u, 
	34u, 40u, 41u, 92u, 9u, 13u, 32u, 34u, 
	40u, 41u, 92u, 9u, 13u, 32u, 34u, 40u, 
	41u, 92u, 9u, 13u, 40u, 32u, 34u, 35u, 
	36u, 38u, 39u, 42u, 43u, 45u, 46u, 47u, 
	48u, 58u, 60u, 61u, 62u, 76u, 82u, 85u, 
	94u, 95u, 117u, 124u, 9u, 13u, 33u, 37u, 
	49u, 57u, 65u, 90u, 97u, 122u, 32u, 9u, 
	13u, 61u, 9u, 32u, 36u, 95u, 100u, 101u, 
	105u, 108u, 112u, 117u, 119u, 65u, 90u, 97u, 
	122u, 36u, 95u, 48u, 57u, 65u, 90u, 97u, 
	122u, 36u, 95u, 101u, 48u, 57u, 65u, 90u, 
	97u, 122u, 36u, 95u, 102u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 105u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 110u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 101u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	108u, 110u, 114u, 48u, 57u, 65u, 90u, 97u, 
	122u, 36u, 95u, 105u, 115u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 102u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 100u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 105u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	114u, 48u, 57u, 65u, 90u, 97u, 122u, 36u, 
	95u, 111u, 48u, 57u, 65u, 90u, 97u, 122u, 
	36u, 95u, 114u, 48u, 57u, 65u, 90u, 97u, 
	122u, 36u, 95u, 102u, 110u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 100u, 110u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 101u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	100u, 48u, 57u, 65u, 90u, 97u, 122u, 36u, 
	95u, 99u, 48u, 57u, 65u, 90u, 97u, 122u, 
	36u, 95u, 108u, 48u, 57u, 65u, 90u, 97u, 
	122u, 36u, 95u, 117u, 48u, 57u, 65u, 90u, 
	97u, 122u, 36u, 95u, 100u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 114u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 97u, 48u, 
	57u, 65u, 90u, 98u, 122u, 36u, 95u, 103u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	109u, 48u, 57u, 65u, 90u, 97u, 122u, 36u, 
	95u, 97u, 48u, 57u, 65u, 90u, 98u, 122u, 
	36u, 95u, 110u, 48u, 57u, 65u, 90u, 97u, 
	122u, 36u, 95u, 97u, 48u, 57u, 65u, 90u, 
	98u, 122u, 36u, 95u, 114u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 110u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 105u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 110u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	103u, 48u, 57u, 65u, 90u, 97u, 122u, 36u, 
	95u, 48u, 57u, 65u, 90u, 97u, 122u, 38u, 
	61u, 43u, 61u, 45u, 61u, 62u, 46u, 48u, 
	57u, 39u, 69u, 76u, 101u, 108u, 48u, 57u, 
	68u, 70u, 100u, 102u, 39u, 68u, 70u, 76u, 
	100u, 102u, 108u, 48u, 57u, 42u, 47u, 61u, 
	10u, 39u, 46u, 66u, 69u, 88u, 95u, 98u, 
	101u, 120u, 48u, 55u, 56u, 57u, 39u, 46u, 
	69u, 95u, 101u, 48u, 57u, 69u, 76u, 101u, 
	108u, 48u, 57u, 68u, 70u, 100u, 102u, 39u, 
	69u, 76u, 101u, 108u, 48u, 57u, 68u, 70u, 
	100u, 102u, 39u, 68u, 70u, 76u, 100u, 102u, 
	108u, 48u, 57u, 39u, 68u, 70u, 76u, 100u, 
	102u, 108u, 48u, 57u, 36u, 95u, 48u, 57u, 
	65u, 90u, 97u, 122u, 39u, 46u, 69u, 76u, 
	85u, 95u, 101u, 108u, 117u, 48u, 55u, 56u, 
	57u, 76u, 85u, 108u, 117u, 85u, 117u, 76u, 
	108u, 76u, 108u, 39u, 76u, 85u, 108u, 117u, 
	48u, 49u, 76u, 85u, 108u, 117u, 85u, 117u, 
	76u, 108u, 76u, 108u, 39u, 46u, 76u, 80u, 
	85u, 108u, 112u, 117u, 48u, 57u, 65u, 70u, 
	97u, 102u, 39u, 68u, 70u, 76u, 100u, 102u, 
	108u, 48u, 57u, 76u, 85u, 108u, 117u, 85u, 
	117u, 76u, 108u, 76u, 108u, 58u, 60u, 61u, 
	62u, 61u, 62u, 34u, 36u, 39u, 82u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 34u, 36u, 
	95u, 48u, 57u, 65u, 90u, 97u, 122u, 34u, 
	36u, 39u, 56u, 82u, 95u, 48u, 57u, 65u, 
	90u, 97u, 122u, 61u, 124u, 0
};

static const char _CPPT_single_lengths[] = {
	0, 2, 9, 0, 0, 0, 0, 0, 
	0, 0, 0, 11, 2, 9, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 0, 
	2, 0, 1, 2, 0, 0, 2, 0, 
	2, 0, 2, 0, 0, 2, 3, 0, 
	2, 0, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 1, 23, 1, 1, 11, 2, 
	3, 3, 3, 3, 3, 5, 4, 3, 
	3, 3, 3, 3, 3, 4, 4, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 2, 2, 2, 1, 1, 5, 7, 
	3, 1, 9, 5, 4, 5, 7, 7, 
	2, 9, 4, 2, 2, 2, 5, 4, 
	2, 2, 2, 8, 7, 4, 2, 2, 
	2, 1, 2, 1, 2, 5, 3, 6, 
	2
};

static const char _CPPT_range_lengths[] = {
	0, 0, 4, 3, 3, 3, 3, 3, 
	3, 3, 3, 2, 0, 4, 3, 3, 
	3, 3, 3, 3, 3, 3, 0, 1, 
	1, 1, 0, 0, 1, 1, 1, 1, 
	1, 1, 2, 1, 3, 3, 3, 3, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 0, 5, 1, 0, 2, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 0, 0, 1, 1, 3, 1, 
	0, 0, 2, 1, 3, 3, 1, 1, 
	3, 2, 0, 0, 0, 0, 1, 0, 
	0, 0, 0, 3, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 3, 3, 3, 
	0
};

static const short _CPPT_index_offsets[] = {
	0, 0, 3, 17, 21, 25, 29, 33, 
	37, 41, 45, 49, 63, 66, 80, 84, 
	88, 92, 96, 100, 104, 108, 112, 114, 
	116, 120, 122, 124, 127, 129, 131, 135, 
	137, 141, 143, 148, 150, 154, 160, 167, 
	171, 175, 177, 184, 191, 198, 205, 212, 
	219, 226, 233, 240, 247, 254, 261, 268, 
	275, 282, 289, 291, 320, 323, 325, 339, 
	345, 352, 359, 366, 373, 380, 389, 397, 
	404, 411, 418, 425, 432, 439, 447, 455, 
	462, 469, 476, 483, 490, 497, 504, 511, 
	518, 525, 532, 539, 546, 553, 560, 567, 
	574, 581, 587, 590, 593, 596, 599, 608, 
	617, 621, 623, 635, 642, 650, 659, 668, 
	677, 683, 695, 700, 703, 706, 709, 716, 
	721, 724, 727, 730, 742, 751, 756, 759, 
	762, 765, 767, 770, 772, 775, 784, 791, 
	801
};

static const unsigned char _CPPT_indicies[] = {
	2, 3, 1, 1, 1, 1, 4, 1, 
	1, 1, 5, 6, 1, 1, 1, 1, 
	0, 7, 7, 7, 0, 8, 8, 8, 
	0, 9, 9, 9, 0, 5, 5, 5, 
	0, 10, 10, 10, 0, 11, 11, 11, 
	0, 6, 6, 6, 0, 1, 1, 1, 
	0, 13, 13, 14, 14, 15, 16, 17, 
	18, 19, 20, 21, 14, 14, 12, 23, 
	24, 22, 22, 22, 22, 25, 22, 22, 
	22, 26, 27, 22, 22, 22, 22, 0, 
	28, 28, 28, 0, 29, 29, 29, 0, 
	30, 30, 30, 0, 26, 26, 26, 0, 
	31, 31, 31, 0, 32, 32, 32, 0, 
	27, 27, 27, 0, 22, 22, 22, 0, 
	34, 33, 36, 35, 37, 37, 38, 35, 
	38, 35, 41, 40, 41, 42, 40, 43, 
	0, 45, 44, 46, 46, 47, 44, 47, 
	44, 48, 48, 49, 0, 49, 0, 50, 
	50, 50, 50, 0, 51, 0, 52, 52, 
	52, 0, 55, 55, 54, 54, 54, 53, 
	56, 55, 55, 54, 54, 54, 53, 54, 
	54, 54, 53, 57, 57, 58, 53, 58, 
	0, 59, 59, 61, 59, 59, 59, 60, 
	59, 59, 63, 59, 59, 59, 62, 59, 
	59, 63, 59, 59, 59, 64, 59, 59, 
	63, 59, 59, 59, 65, 59, 59, 63, 
	59, 59, 59, 66, 59, 59, 63, 59, 
	59, 59, 67, 59, 59, 63, 59, 59, 
	59, 68, 59, 59, 63, 59, 59, 59, 
	69, 59, 59, 63, 59, 59, 59, 70, 
	59, 59, 63, 59, 59, 59, 71, 59, 
	59, 63, 59, 59, 59, 72, 59, 59, 
	63, 59, 59, 59, 73, 59, 59, 63, 
	59, 59, 59, 74, 59, 59, 63, 59, 
	59, 59, 75, 59, 59, 63, 59, 59, 
	59, 76, 59, 59, 63, 59, 59, 59, 
	77, 63, 59, 79, 1, 81, 82, 83, 
	22, 80, 84, 85, 86, 87, 88, 89, 
	90, 80, 91, 92, 93, 92, 80, 82, 
	94, 95, 79, 80, 43, 82, 82, 78, 
	79, 79, 96, 34, 0, 13, 13, 14, 
	14, 15, 16, 17, 18, 19, 20, 21, 
	14, 14, 97, 14, 14, 14, 14, 14, 
	0, 14, 14, 99, 14, 14, 14, 98, 
	14, 14, 18, 14, 14, 14, 98, 14, 
	14, 100, 14, 14, 14, 98, 14, 14, 
	101, 14, 14, 14, 98, 14, 14, 102, 
	14, 14, 14, 98, 14, 14, 103, 104, 
	105, 14, 14, 14, 98, 14, 14, 106, 
	101, 14, 14, 14, 98, 14, 14, 102, 
	14, 14, 14, 98, 14, 14, 107, 14, 
	14, 14, 98, 14, 14, 106, 14, 14, 
	14, 98, 14, 14, 108, 14, 14, 14, 
	98, 14, 14, 109, 14, 14, 14, 98, 
	14, 14, 102, 14, 14, 14, 98, 14, 
	14, 110, 111, 14, 14, 14, 98, 14, 
	14, 113, 114, 14, 14, 14, 112, 14, 
	14, 106, 14, 14, 14, 98, 14, 14, 
	113, 14, 14, 14, 98, 14, 14, 115, 
	14, 14, 14, 98, 14, 14, 116, 14, 
	14, 14, 98, 14, 14, 117, 14, 14, 
	14, 98, 14, 14, 101, 14, 14, 14, 
	98, 14, 14, 118, 14, 14, 14, 98, 
	14, 14, 119, 14, 14, 14, 98, 14, 
	14, 120, 14, 14, 14, 98, 14, 14, 
	121, 14, 14, 14, 98, 14, 14, 102, 
	14, 14, 14, 98, 14, 14, 114, 14, 
	14, 14, 98, 14, 14, 122, 14, 14, 
	14, 98, 14, 14, 123, 14, 14, 14, 
	98, 14, 14, 124, 14, 14, 14, 98, 
	14, 14, 125, 14, 14, 14, 98, 14, 
	14, 126, 14, 14, 14, 98, 14, 14, 
	102, 14, 14, 14, 98, 82, 82, 82, 
	82, 82, 127, 34, 34, 128, 34, 34, 
	128, 34, 34, 128, 130, 36, 129, 132, 
	134, 133, 134, 133, 36, 133, 133, 131, 
	37, 133, 133, 133, 133, 133, 133, 38, 
	131, 40, 135, 34, 128, 136, 135, 138, 
	139, 141, 142, 143, 144, 141, 142, 143, 
	140, 43, 137, 138, 139, 142, 144, 142, 
	43, 137, 147, 146, 147, 146, 45, 146, 
	146, 145, 148, 147, 146, 147, 146, 45, 
	146, 146, 145, 46, 146, 146, 146, 146, 
	146, 146, 47, 145, 48, 150, 150, 150, 
	150, 150, 150, 49, 149, 50, 50, 50, 
	50, 50, 137, 138, 139, 142, 152, 153, 
	144, 142, 152, 153, 140, 43, 151, 154, 
	155, 154, 155, 151, 155, 155, 151, 156, 
	156, 151, 155, 155, 151, 141, 158, 159, 
	158, 159, 51, 157, 160, 161, 160, 161, 
	157, 161, 161, 157, 162, 162, 157, 161, 
	161, 157, 143, 164, 165, 55, 166, 165, 
	55, 166, 52, 52, 52, 163, 57, 168, 
	168, 168, 168, 168, 168, 58, 167, 169, 
	170, 169, 170, 163, 170, 170, 163, 171, 
	171, 163, 170, 170, 163, 34, 128, 172, 
	173, 128, 34, 174, 34, 172, 128, 1, 
	82, 22, 93, 82, 82, 82, 82, 127, 
	175, 82, 82, 82, 82, 82, 127, 1, 
	82, 22, 92, 93, 82, 82, 82, 82, 
	127, 34, 34, 128, 0
};

static const unsigned char _CPPT_trans_targs[] = {
	59, 1, 59, 2, 3, 7, 10, 4, 
	5, 6, 8, 9, 59, 11, 63, 64, 
	69, 77, 66, 85, 90, 91, 12, 59, 
	13, 14, 18, 21, 15, 16, 17, 19, 
	20, 59, 59, 59, 102, 25, 103, 59, 
	26, 27, 59, 107, 59, 109, 31, 110, 
	33, 111, 112, 118, 123, 59, 38, 40, 
	39, 41, 124, 59, 43, 59, 44, 59, 
	45, 46, 47, 48, 49, 50, 51, 52, 
	53, 54, 55, 56, 57, 58, 59, 60, 
	61, 62, 97, 98, 99, 100, 101, 104, 
	106, 129, 130, 132, 133, 134, 135, 136, 
	59, 59, 59, 65, 67, 68, 63, 70, 
	72, 74, 71, 73, 75, 76, 78, 81, 
	59, 79, 80, 82, 83, 84, 86, 87, 
	88, 89, 92, 93, 94, 95, 96, 59, 
	59, 59, 22, 59, 23, 59, 24, 105, 
	59, 59, 28, 108, 113, 35, 32, 36, 
	34, 59, 59, 30, 29, 59, 59, 59, 
	114, 116, 115, 59, 117, 59, 119, 121, 
	120, 59, 122, 59, 37, 125, 127, 59, 
	59, 126, 59, 128, 61, 131, 59, 42
};

static const char _CPPT_trans_actions[] = {
	79, 0, 9, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 71, 0, 108, 0, 
	0, 0, 0, 0, 0, 0, 0, 11, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 77, 27, 69, 5, 0, 5, 75, 
	0, 0, 7, 102, 67, 5, 0, 5, 
	0, 99, 0, 93, 90, 65, 0, 0, 
	0, 0, 87, 73, 1, 120, 0, 81, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 29, 0, 
	117, 5, 0, 0, 0, 0, 5, 5, 
	102, 0, 0, 0, 111, 5, 111, 0, 
	63, 53, 51, 0, 0, 0, 105, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	49, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 55, 
	59, 61, 0, 43, 0, 23, 0, 0, 
	31, 47, 0, 5, 96, 0, 0, 0, 
	0, 41, 21, 0, 0, 45, 25, 39, 
	0, 0, 0, 19, 0, 37, 0, 0, 
	0, 17, 0, 35, 0, 0, 0, 33, 
	13, 0, 15, 0, 114, 0, 57, 0
};

static const char _CPPT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 84, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const char _CPPT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const short _CPPT_eof_trans[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 13, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 34, 36, 
	36, 36, 40, 40, 1, 45, 45, 45, 
	1, 1, 1, 1, 1, 54, 54, 54, 
	54, 1, 60, 60, 60, 60, 60, 60, 
	60, 60, 60, 60, 60, 60, 60, 60, 
	60, 60, 60, 0, 97, 1, 98, 1, 
	99, 99, 99, 99, 99, 99, 99, 99, 
	99, 99, 99, 99, 99, 99, 113, 99, 
	99, 99, 99, 99, 99, 99, 99, 99, 
	99, 99, 99, 99, 99, 99, 99, 99, 
	99, 128, 129, 129, 129, 130, 132, 132, 
	129, 137, 138, 138, 146, 146, 146, 150, 
	138, 152, 152, 152, 152, 152, 158, 158, 
	158, 158, 158, 164, 168, 164, 164, 164, 
	164, 129, 129, 175, 129, 128, 128, 128, 
	129
};

static const int CPPT_start = 59;
static const int CPPT_first_final = 59;
static const int CPPT_error = 0;

static const int CPPT_en_main = 59;


/* #line 169 "CPPT.c.rl" */

ok64 CPPTLexer(CPPTstate* state) {

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

    u8cs rsd = {NULL, NULL};        // DOG-007: raw-string delimiter capture

    u8cs tok = {p, p};

    
/* #line 479 "CPPT.rl.c" */
	{
	cs = CPPT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 189 "CPPT.c.rl" */
    
/* #line 485 "CPPT.rl.c" */
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
	_acts = _CPPT_actions + _CPPT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 4:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 504 "CPPT.rl.c" */
		}
	}

	_keys = _CPPT_trans_keys + _CPPT_key_offsets[cs];
	_trans = _CPPT_index_offsets[cs];

	_klen = _CPPT_single_lengths[cs];
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

	_klen = _CPPT_range_lengths[cs];
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
	_trans = _CPPT_indicies[_trans];
_eof_trans:
	cs = _CPPT_trans_targs[_trans];

	if ( _CPPT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _CPPT_actions + _CPPT_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 80 "CPPT.c.rl" */
	{ rsd[0] = (u8c*)p; }
	break;
	case 1:
/* #line 81 "CPPT.c.rl" */
	{ rsd[1] = (u8c*)p; }
	break;
	case 5:
/* #line 1 "NONE" */
	{te = p+1;}
	break;
	case 6:
/* #line 47 "CPPT.c.rl" */
	{act = 6;}
	break;
	case 7:
/* #line 47 "CPPT.c.rl" */
	{act = 7;}
	break;
	case 8:
/* #line 47 "CPPT.c.rl" */
	{act = 8;}
	break;
	case 9:
/* #line 47 "CPPT.c.rl" */
	{act = 9;}
	break;
	case 10:
/* #line 47 "CPPT.c.rl" */
	{act = 12;}
	break;
	case 11:
/* #line 47 "CPPT.c.rl" */
	{act = 13;}
	break;
	case 12:
/* #line 53 "CPPT.c.rl" */
	{act = 14;}
	break;
	case 13:
/* #line 53 "CPPT.c.rl" */
	{act = 15;}
	break;
	case 14:
/* #line 59 "CPPT.c.rl" */
	{act = 17;}
	break;
	case 15:
/* #line 65 "CPPT.c.rl" */
	{act = 18;}
	break;
	case 16:
/* #line 65 "CPPT.c.rl" */
	{act = 19;}
	break;
	case 17:
/* #line 35 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 82 "CPPT.c.rl" */
	{te = p+1;{
    size_t n = $size(rsd);
    u8c* q = (u8c*)rsd[1] + 1;       // first body byte, just past the `(`
    u8c* end = NULL;
    while (q + n + 1 < pe) {
        u8cs run = {q + 1, q + 1 + n};
        if (*q == ')' && q[n+1] == '"' && $eq(rsd, run)) {
            end = q + n + 2;
            break;
        }
        ++q;
    }
    if (end == NULL) { o = CPPTBAD; {p++; goto _out; } }
    tok[0] = (u8c*)ts;
    tok[1] = end;
    o = CPPTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
    {p = (( end))-1;}          // resume the scanner just past the close
    {cs = 59;goto _again;}
}}
	break;
	case 19:
/* #line 41 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 41 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 47 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 47 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 47 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 47 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 47 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 47 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 47 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 65 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 65 "CPPT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 30:
/* #line 35 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 31:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 32:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 33:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 34:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 35:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 36:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 37:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 38:
/* #line 47 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 39:
/* #line 53 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 40:
/* #line 53 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 41:
/* #line 65 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 42:
/* #line 59 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 43:
/* #line 65 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 44:
/* #line 65 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 45:
/* #line 65 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 46:
/* #line 71 "CPPT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 47:
/* #line 47 "CPPT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 48:
/* #line 47 "CPPT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 49:
/* #line 47 "CPPT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 50:
/* #line 65 "CPPT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 51:
/* #line 59 "CPPT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 52:
/* #line 65 "CPPT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 53:
/* #line 65 "CPPT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 54:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 0:
	{{cs = 0;goto _again;}}
	break;
	case 6:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 7:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 8:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 9:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 12:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 13:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 14:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 15:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 17:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 18:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 19:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CPPTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 1017 "CPPT.rl.c" */
		}
	}

_again:
	_acts = _CPPT_actions + _CPPT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 2:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
	case 3:
/* #line 1 "NONE" */
	{act = 0;}
	break;
/* #line 1031 "CPPT.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _CPPT_eof_trans[cs] > 0 ) {
		_trans = _CPPT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 190 "CPPT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < CPPT_first_final)
        o = CPPTBAD;

    return o;
}
