#ifndef DOG_GIT_PIDX_H
#define DOG_GIT_PIDX_H

//  PIDX: pack-log index-EMIT.  GIT-010.
//
//  The pack-log does NOT own or maintain an index.  These functions are
//  ENTRY PRODUCERS: given a caller-provided `Bwh128 out`, they drop one
//  `(sha -> offset)` entry per object into it; sort / merge / persist /
//  query stay entirely the caller's (an abc.index wh128 lane, the keeper
//  puppy registry, ...).  Mirrors the `tok.parse(..., out?)` shape — emit
//  into the caller's region, hold nothing (JABC rule #4).
//
//  Entry layout (the canonical wh128 lane shared with keeper + abc.index):
//    key = WHIFFKeyPack(type, hashlet60(sha))   hashlet60[60] | type[4]
//    val = offset                               byte offset of the record
//  `val` is the bare in-pack offset (flags=0, file_id=0): a single-pack
//  emitter has no file_id; the caller re-packs file_id (keepPackVal) when
//  it merges this run into a multi-pack registry.
//
//  SINGLE-pack, single-threaded, NO keeper coupling, NO fork.  The heavy
//  multi-pack / ingest / parallel path stays in keeper/UNPK; PIDXScan is
//  the small shared core modelled on UNPKIndex minus all of that.

#include "abc/INT.h"
#include "abc/S.h"
#include "dog/WHIFF.h"   //  Bwh128 + WHIFFKeyPack + WHIFFHashlet60
#include "dog/git/SHA1.h"

con ok64 PIDXFAIL = 0x6e2349e3ca495;   //  scan / emit failure (bad pack, no room)

//  Compute a git object's SHA-1 over the loose-object framing
//  "<type> <len>\0<content>".  `type` is a git pack/object type (1..4);
//  out-of-range types are still framed via the empty type name.  This is
//  the dog/git twin of keeper's KEEPObjSha (no keeper coupling).
void PIDXObjSha(sha1 *out, u8 type, u8csc content);

//  Build the canonical wh128 index entry for one resolved object:
//    key = WHIFFKeyPack(type, hashlet60(sha)),  val = offset.
wh128 PIDXEntry(u8 type, sha1cp sha, u64 offset);

//  SCAN-EMIT: walk a single OFS-only pack, resolve every object to its
//  full bytes, git-sha it, and emit one wh128 entry per object into `out`.
//  `pack` is the whole pack-byte slice (12-byte header through the last
//  record, NOT the trailing pack checksum — pass the logical extent the
//  caller tracks, e.g. the watermark).  `base` / `delta` are caller-owned
//  resolve scratch with the same sizing contract as PACKResolveOfs (`base`
//  holds the largest inflated object, `delta` the largest delta stream
//  plus one apply result).  `out` is grown one entry at a time (the caller
//  may pre-Reserve to the object count).  REF_DELTA records make the scan
//  return PIDXFAIL (an OFS-only log carries no sha-addressed bases — the
//  same loud backstop as PACKResolveOfs).  No malloc, no refs, no index.
ok64 PIDXScan(u8cs pack, Bwh128 out, u8s base, u8s delta);

//  INDEX-ON-APPEND: emit the entry for the object the caller JUST fed,
//  hashing the full content it already holds (no resolve — cheap).
//  `type` is the git object type (1..4), `content` the object's full
//  uncompressed bytes, `offset` the record's start offset in the log.
//  Append the `(sha, offset)` entry to `out`.
ok64 PIDXFeedEmit(Bwh128 out, u8 type, u8csc content, u64 offset);

#endif
