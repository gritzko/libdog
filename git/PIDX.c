//  PIDX: pack-log index-emit.  See PIDX.h for the contract.  GIT-010.
//
//  PIDXScan models the single-pack core of keeper/UNPK.c::UNPKIndex but
//  DROPS the keeper coupling, the fork/parallelism, the REF_DELTA waiter
//  forest and the ingest re-emit (those stay in keeper).  It walks one
//  OFS-only pack record-by-record (PACKRecordEnd), resolves each object to
//  full bytes (PACKResolveOfs — the SAME chase keeper's native get runs),
//  git-shas it, and emits one wh128 entry into the caller's buffer.

#include "PIDX.h"

#include "GIT.h"     //  GITTypeName (the loose-object framing word)
#include "PACK.h"    //  PACKDrainHdr / PACKRecordEnd / PACKResolveOfs

#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/PRO.h"

void PIDXObjSha(sha1 *out, u8 type, u8csc content) {
    //  "<type> <len>\0" + content, hashed — git's loose-object framing.
    //  The dog/git twin of keeper's KEEPObjSha (no keeper coupling).
    a_pad(u8, hdr, 64);
    u8cs tname = {};
    GITTypeName(tname, type);
    u8bFeed(hdr, tname);
    u8bPrintf(hdr, " %llu", (unsigned long long)u8csLen(content));
    u8bFeed1(hdr, 0);

    SHA1state ctx;
    SHA1Open(&ctx);
    SHA1Feed(&ctx, u8bDataC(hdr));
    SHA1Feed(&ctx, content);
    SHA1Close(&ctx, out);
}

wh128 PIDXEntry(u8 type, sha1cp sha, u64 offset) {
    wh128 e = {
        .key = WHIFFKeyPack(type, WHIFFHashlet60(sha)),
        .val = offset,
    };
    return e;
}

//  Grow-then-push one entry (mirrors UNPK's unpk_push: Breserve rounds up
//  so single-slot growth isn't quadratic).  The caller may pre-Reserve to
//  the object count; one entry is emitted per resolved object.
static ok64 pidx_push(Bwh128 out, wh128 const *e) {
    sane(out);
    call(wh128bReserve, out, 1);
    return wh128bPush(out, e);
}

ok64 PIDXFeedEmit(Bwh128 out, u8 type, u8csc content, u64 offset) {
    sane(out);
    sha1 sha = {};
    PIDXObjSha(&sha, type, content);
    wh128 e = PIDXEntry(type, &sha, offset);
    return pidx_push(out, &e);
}

//  PACK-001: resolve the record at *off, git-sha it, emit its entry, advance.
static ok64 pidx_emit_at(u8cs pack, u64 *off, Bwh128 out, u8s base, u8s delta) {
    sane(off && out && u8csOK(pack) && u8sOK(base) && u8sOK(delta));
    //  Resolve to full bytes (chases the OFS chain bottom-up).  A REF_DELTA
    //  surfaces as PACKREF — the loud OFS-only backstop (no sha-addressed base).
    u8cs body = {};
    u8 type = 0;
    call(PACKResolveOfs, pack, *off, base, delta, body, &type);
    sha1 sha = {};
    a_dup(u8c, content, body);
    PIDXObjSha(&sha, type, content);
    wh128 e = PIDXEntry(type, &sha, *off);
    call(pidx_push, out, &e);
    //  Advance to the next record (header + the whole zlib stream).
    u64 end = 0;
    call(PACKRecordEnd, pack, *off, &end);
    if (end <= *off) return PIDXFAIL;   //  no forward progress -> corrupt
    *off = end;
    done;
}

ok64 PIDXScan(u8cs pack, u64 from_off, Bwh128 out, u8s base, u8s delta) {
    sane(u8csOK(pack) && out && u8sOK(base) && u8sOK(delta));

    pack_hdr hdr = {};
    a_dup(u8c, hp, pack);
    call(PACKDrainHdr, hp, &hdr);   //  validates magic, reads object count

    u64 packlen = (u64)u8csLen(pack);
    if (from_off) {
        //  PACK-001: tail scan from a known boundary to EOF (hdr.count is total).
        u64 off = from_off;
        while (off < packlen) call(pidx_emit_at, pack, &off, out, base, delta);
    } else {
        u64 off = 12;   //  whole-pack: bounded by the header's object count
        for (u32 i = 0; i < hdr.count; i++) {
            if (off >= packlen) return PIDXFAIL;
            call(pidx_emit_at, pack, &off, out, base, delta);
        }
    }
    done;
}
