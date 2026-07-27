#ifndef DOG_GIT_REPACK_H
#define DOG_GIT_REPACK_H

//  REPACK: stream-ingest a git pack into size-capped keeper pack logs
//  (KEEP-006, JAB-020).  The source is CONSUMED FROM AN fd, never stored
//  and never seeked -- a pack file and a socket are the same thing here.
//  ONE call repacks a whole fetch: no session handle, no step protocol.
//  The input buffer, the index region and the log directory are all the
//  caller's (JABC rule #4: emit into the caller's region, hold nothing).
//
//  Per record: parse the self-delimiting object header, inflate ONCE (that
//  also yields the record extent -- a zlib stream carries no length),
//  resolve any delta against OUR ALREADY-WRITTEN LOG (never by seeking back
//  into the source), re-emit the record with its base reference rewritten
//  (OFS when the base landed in this log, REF when it landed in an earlier
//  one) copying the zlib payload VERBATIM, and drop one index entry.

#include "abc/PATH.h"
#include "abc/S.h"
#include "dog/WHIFF.h"

con u64 REPACK_LOG_MAX = (1ull << 31) - 1;   //  KEEP-006 pack-log cap

con ok64 REPACKFAIL = 0x6ce64a3143ca495;   //  io / internal failure
con ok64 REPACKTORN = 0x6ce64a3147586d7;   //  the source ended mid-record
con ok64 REPACKBIG  = 0x1b39928c50b490;    //  a record outgrows buf or the cap
con ok64 REPACKBASE = 0x6ce64a3142ca70e;   //  a delta base was never seen
con ok64 REPACKROOM = 0x6ce64a3146d8616;   //  the caller's index region is full
con ok64 REPACKLOGS = 0x6ce64a31455841c;   //  more logs than REPACK_MAX_LOGS
con ok64 REPACKHDR  = 0x1b39928c51135b;    //  no pack header at the front

//  Earlier logs stay mapped for the whole run (a delta base may live in
//  one), so this bounds address space, not memory: 64 * 2 GiB of VA.
#define REPACK_MAX_LOGS 64

typedef struct {
    u64 objects, total;       //  repacked so far / the pack header's count
    u64 raw, ofs, ref;        //  emitted record kinds
    u64 in_bytes, out_bytes;  //  consumed from the source / written to logs
    u32 log0, logs;           //  first file id / logs opened so far
    u64 log_len, index_n;     //  current log's fill / index entries emitted
} repack_stat;

//  Progress: invoked every `every` objects with the live counters.  A
//  non-OK return aborts the run and becomes REPACKRun's status.
typedef ok64 (*repack_watch)(void *user, repack_stat const *st);

typedef struct {
    u64 cap;              //  per-log byte cap, 0 = REPACK_LOG_MAX
    u32 log0;             //  first `NNNNNNNNNN.keeper` file id in the shard
    u64 every;            //  progress granularity in objects, 0 = silent
    repack_watch watch;
    void *user;
} repack_conf;

//  Repack the pack arriving on `fd` into `<shard>/NNNNNNNNNN.keeper` logs.
//
//  `buf` is the caller's input buffer: DATA is the head of the stream it
//  already read (the pack header, plus whatever the pkt-line reader ate
//  with it), IDLE is where further reads land.  REPACK never grows it, so
//  it must hold one whole record -- size it at the log cap, since a record
//  that doesn't fit a log cannot be stored anyway (REPACKBIG).  Its DATA
//  and IDLE heads advance in place, so the caller sees what was consumed.
//
//  `idx` collects index entries in emission order (sorting, run naming and
//  persistence stay the caller's): one per object, plus one PACK summary
//  per log --
//    object:  key = WHIFFKeyPack(type, hashlet60(sha))
//             val = wh64Pack(1, file_id, record offset)
//    PACK:    key = wh64Pack(0xF, file_id, 12)
//             val = (record count << 32) | (log bytes - 12)
ok64 REPACKRun(int fd, u8b buf, path8sc shard, repack_conf const *conf,
               Bwh128 idx, repack_stat *st);

#endif
