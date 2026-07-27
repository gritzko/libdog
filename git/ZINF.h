#ifndef KEEPER_ZINF_H
#define KEEPER_ZINF_H

//  ZINF: zlib inflate/deflate wrappers.

#include "abc/BUF.h"

con ok64 ZINFFAIL   = 0x8d25cf3ca495;
con ok64 ZINFINIT   = 0x8d25cf49749d;
con ok64 ZINFTOOBIG = 0x8d25cf75860b490;
//  MORE: the input ran out before Z_STREAM_END -- the stream is not corrupt,
//  it is TRUNCATED.  A streaming caller refills and retries the same record
//  (inflate is deterministic and consumed nothing it must un-consume); only
//  ZINFFAIL means damaged bytes.  Without this the two are indistinguishable
//  and a streaming reader cannot tell "need more" from "give up".
con ok64 ZINFMORE   = 0x8d25cf5986ce;

//  Inflate: into consumes from zipped (into head advances by produced,
//  zipped head by consumed).
ok64 ZINFInflate(u8s into, u8cs zipped);

//  Deflate: into consumes from plain (into head advances by produced,
//  plain head by consumed).
ok64 ZINFDeflate(u8s into, u8cs plain);

#endif
