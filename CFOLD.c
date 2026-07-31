//  CFOLD (DIS-082): the append-only weave.  See CFOLD.h for the model.
//  Reader (codec, DFS walk, visibility, blame) then writer (fold, tomb,
//  chain terminator, per-commit index merge).
//
#include "CFOLD.h"

#include "abc/DIFF.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RAP.h"
#include "abc/TLV.h"
#include "dog/BRAM.h"
#include "dog/HUNK.h"
#include "dog/NEIL.h"

//  DIS-082: read one entry from an IMMUTABLE index view (the generated
//  csAt wants a consumable slice, and a parsed weave is const).
fun tokv32 cf_at(tokv32csc s, size_t i) { return s[0][i]; }

// ============================================================
//  Codec: 'V' container <-> the three streams
// ============================================================

ok64 CFOLDParse(cfold *w, u8csc blob) {
    sane(w);
    zerop(w);
    u8cs rest = {};
    u8csMv(rest, blob);
    u8   otype = 0;
    u8cs inner = {};
    call(TLVu8sDrain, rest, &otype, inner);
    if (otype != CFOLD_TLV) fail(CFOLDFAIL);
    while (u8csLen(inner) > 0) {
        u8   t = 0;
        u8cs v = {};
        call(TLVu8sDrain, inner, &t, v);
        switch (t) {
        case CFOLD_TLV_BODY: u8csMv(w->body, v); break;
        case CFOLD_TLV_CMT:  u8csMv(w->cmts, v); break;
        case CFOLD_TLV_IGN:  u8csMv(w->ign, v);  break;
        case CFOLD_TLV_IDX:
            if (u8csLen(v) % sizeof(tokv32)) fail(CFOLDFAIL);
            w->idx[0] = (tokv32c *)v[0];
            w->idx[1] = (tokv32c *)v[1];
            break;
        default: break;   // unknown sub-record: ignore (forward-compat)
        }
    }
    if (u8csLen(w->cmts) % CFOLD_CMT_SZ) fail(CFOLDFAIL);
    if (u8csLen(w->ign) % sizeof(u32)) fail(CFOLDFAIL);
    done;
}

static ok64 cfold_serialize(u8s into, cfold const *w) {
    sane(w);
    u8csc ebytes = {(u8c *)w->idx[0], (u8c *)w->idx[1]};
    size_t cap = u8csLen(w->body) + u8csLen(ebytes) + u8csLen(w->cmts)
               + u8csLen(w->ign) + 64;
    a_carve(u8, inner, cap);
    call(TLVu8sFeed, u8bIdle(inner), CFOLD_TLV_BODY, w->body);
    call(TLVu8sFeed, u8bIdle(inner), CFOLD_TLV_IDX, ebytes);
    call(TLVu8sFeed, u8bIdle(inner), CFOLD_TLV_CMT, w->cmts);
    call(TLVu8sFeed, u8bIdle(inner), CFOLD_TLV_IGN, w->ign);
    call(TLVu8sFeed, into, CFOLD_TLV, u8bDataC(inner));
    done;
}

u32 CFOLDNCommits(cfold const *w) {
    return (u32)(u8csLen(w->cmts) / CFOLD_CMT_SZ);
}

ok64 CFOLDCommitAt(cfcommit *out, cfold const *w, u32 i) {
    sane(out && w);
    if (i >= CFOLDNCommits(w)) fail(CFOLDNOCM);
    zerop(out);
    a_part(u8c, rec, w->cmts, (size_t)i * CFOLD_CMT_SZ, CFOLD_CMT_SZ);
    u8cs r = {};
    u8csMv(r, rec);
    call(u8sDrain64, r, &out->id);
    call(u8sDrain32, r, &out->start);
    call(u8sDrain32, r, &out->end);
    call(u8sDrain32, r, &out->L);
    call(u8sDrain32, r, &out->P);
    call(u8sDrain32, r, &out->ign_off);
    call(u8sDrain32, r, &out->ign_len);
    done;
}

//  Feed one 'C' record.  Fixed CFOLD_CMT_SZ bytes, LE throughout.
static ok64 cfold_feed_commit(u8b into, cfcommit const *c) {
    sane(c);
    call(u8sFeed64, u8bIdle(into), &c->id);
    call(u8sFeed32, u8bIdle(into), &c->start);
    call(u8sFeed32, u8bIdle(into), &c->end);
    call(u8sFeed32, u8bIdle(into), &c->L);
    call(u8sFeed32, u8bIdle(into), &c->P);
    call(u8sFeed32, u8bIdle(into), &c->ign_off);
    call(u8sFeed32, u8bIdle(into), &c->ign_len);
    done;
}

ok64 CFOLDFindCommit(u32 *out, cfold const *w, u64 id) {
    sane(out && w);
    u32 n = CFOLDNCommits(w);
    for (u32 i = 0; i < n; i++) {
        cfcommit c = {};
        call(CFOLDCommitAt, &c, w, i);
        if (c.id == id) { *out = i; done; }
    }
    fail(CFOLDNOCM);
}

//  BLAME: the commit whose appended body range covers `off`.  Ranges tile
//  [0, bodylen) in build order, so one binary search answers it — no
//  per-token inserter column exists or is needed.
ok64 CFOLDBlame(u32 *out, cfold const *w, u32 off) {
    sane(out && w);
    u32 lo = 0, hi = CFOLDNCommits(w);
    while (lo < hi) {
        u32     mid = lo + (hi - lo) / 2;
        cfcommit c   = {};
        call(CFOLDCommitAt, &c, w, mid);
        if (off < c.start)     hi = mid;
        else if (off >= c.end) lo = mid + 1;
        else { *out = mid; done; }
    }
    fail(CFOLDNOCM);
}

// ============================================================
//  Scope + the DFS walk
// ============================================================

//  What one commit can see: the body bound, its own build index (a tomb
//  counts when its deleting commit index is <= this and not ignored), and
//  the ignore-set — the commits already folded that this one never saw.
typedef struct {
    u32   L;
    u32   cidx;
    u32cs ign;
} cfscope;

static b8 cfscope_ignored(cfscope const *s, u32 ci) {
    $for(u32c, p, s->ign) if (*p == ci) return YES;
    return NO;
}

//  View a commit's ignore list as a u32 slice into the 'G' pool.
static void cfold_ign_of(u32cs out, cfold const *w, cfcommit const *c) {
    u32cs all = {(u32c *)w->ign[0], (u32c *)w->ign[1]};
    a_part(u32c, part, all, c->ign_off, c->ign_len);
    $mv(out, part);
}

static ok64 cfold_scope(cfscope *sc, cfold const *w, u32 c) {
    sane(sc);
    cfcommit ac = {};
    call(CFOLDCommitAt, &ac, w, c);
    zerop(sc);
    sc->L    = ac.L;
    sc->cidx = c;
    cfold_ign_of(sc->ign, w, &ac);
    done;
}

//  Lower bound of the sibling group whose parent is `par`.  `hint` is the
//  entry the caller expects the group to start at: inside a fork-free chain
//  the next entry IS the group, so the walk never binary-searches at all
//  (cfstat.bsearch stays 0 — the format's efficiency claim, made testable).
static u32 cfold_group(cfold const *w, tok32 par, u32 hint, cfstat *st) {
    u32 n = (u32)$len(w->idx);
    if (st) st->groups++;
    if (hint < n && cf_at(w->idx, hint).par == par) return hint;
    if (st) st->bsearch++;
    u32 lo = 0, hi = n;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2;
        if (cf_at(w->idx, mid).par < par) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static u32 cfold_group_end(cfold const *w, tok32 par, u32 lo) {
    u32 n = (u32)$len(w->idx), hi = lo;
    while (hi < n && cf_at(w->idx, hi).par == par) hi++;
    return hi;
}

//  Is this entry's token PRESENT at `sc` (inserted by a commit the scope
//  can see)?  One integer compare unless the ignore-set is non-empty.
static ok64 cfold_present(b8 *out, cfold const *w, cfscope const *sc,
                           tokv32 e) {
    sane(out);
    *out = NO;
    u8 tag = tok32Tag(e.chi);
    if (tag == CFOLD_TAG_TERM || tag == CFOLD_TAG_TOMB) done;
    u32 o = tok32Offset(e.chi);
    if (o >= sc->L) done;
    if ($len(sc->ign) > 0) {
        u32 ci = 0;
        call(CFOLDBlame, &ci, w, o);
        if (cfscope_ignored(sc, ci)) done;
    }
    *out = YES;
    done;
}

//  CANONICAL TOPO RANK.  The sibling tie-break must be BOTH causal (a later
//  insertion at one anchor sorts first) and canonical (clones must render
//  the same bytes), and "ancestry first, else id DESC" is neither — it is
//  not even transitive, so it cannot order a sibling group at all.  One
//  deterministic topological order over the commit table gives both: walk
//  the DAG the ignore-sets encode (anc(j) == [0,j) minus ignore(j)), always
//  taking the READY commit with the SMALLEST canonical id.  Build order is
//  arrival-local; this rank is not.
static ok64 cfold_ranks(u32s rank, cfold const *w) {
    sane(1);
    u32 n = CFOLDNCommits(w);
    if (n == 0) done;
    a_carve(u32, pendb, (size_t)n);
    a_carve(u8,  emitb, (size_t)n);
    a_carve(u64, idb,   (size_t)n);
    a_dup(u32, pend, u32bIdle(pendb));
    a_dup(u8,  emit, u8bIdle(emitb));
    a_dup(u64, ids,  u64bIdle(idb));
    for (u32 j = 0; j < n; j++) {
        cfcommit c = {};
        call(CFOLDCommitAt, &c, w, j);
        if (c.ign_len > j) fail(CFOLDTORN);
        ids[0][j]  = c.id;
        pend[0][j] = j - c.ign_len;
        emit[0][j] = 0;
    }
    for (u32 step = 0; step < n; step++) {
        u32 best = n;
        for (u32 j = 0; j < n; j++) {
            if (emit[0][j] || pend[0][j] != 0) continue;
            if (best == n || ids[0][j] < ids[0][best]) best = j;
        }
        if (best == n) fail(CFOLDTORN);          // not a DAG
        emit[0][best] = 1;
        rank[0][best] = step;
        for (u32 j = best + 1; j < n; j++) {
            if (emit[0][j]) continue;
            cfcommit c = {};
            call(CFOLDCommitAt, &c, w, j);
            u32cs ig = {};
            cfold_ign_of(ig, w, &c);
            b8 hid = NO;
            $for(u32c, p, ig) if (*p == best) { hid = YES; break; }
            if (!hid) pend[0][j]--;
        }
    }
    done;
}

//  A walk sees the weave through 1..32 SCOPES at once (a diff needs two, a
//  produce one); a token is walked when it is present in ANY of them, and
//  the callback gets per-scope present/alive bit masks (bit s = scope s).
#define CFOLD_SCOPES_MAX 32

//  Everything one walk needs: the weave, the scopes, the rank table computed
//  once for it, and the optional counters.
typedef struct {
    cfold const *w;
    cfscope const *scs;
    u32           nsc;
    u32cs         rank;
    cfstat       *st;
} cfwalk;

//  Bit s set iff the token is PRESENT at scope s.
static ok64 cfold_present_mask(u32 *out, cfold const *w, cfscope const *scs,
                               u32 nsc, tokv32 e) {
    sane(out);
    *out = 0;
    for (u32 s = 0; s < nsc; s++) {
        b8 ok = NO;
        call(cfold_present, &ok, w, &scs[s], e);
        if (ok) *out |= (1u << s);
    }
    done;
}

//  The RGA sibling key: the inserting commit's canonical topo rank and the
//  token's body offset (its per-commit ordinal).
typedef struct { u32 rank, off; } cfrgakey;

//  A LATER commit's insertion sits closer to the shared anchor, so rank
//  DESC; within one commit, ordinal ASC.  A total order, so a sibling group
//  can be stepped through without an order array.
fun b8 cfrgakeyLt(cfrgakey const *a, cfrgakey const *b) {
    if (a->rank != b->rank) return a->rank > b->rank;
    return a->off < b->off;
}

static ok64 cfold_rga_key(cfrgakey *out, cfwalk const *k, tokv32 e) {
    sane(out);
    zerop(out);
    out->off = tok32Offset(e.chi);
    u32 ci = 0;
    call(CFOLDBlame, &ci, k->w, out->off);
    if (ci >= (u32)$len(k->rank)) fail(CFOLDTORN);
    out->rank = k->rank[0][ci];
    done;
}

//  One pending sibling group.  `prev` is the entry last visited from it, so
//  the next pick is the RGA-minimum member strictly after it — no order
//  array, and a single-child group costs one presence test.
typedef struct { u32 lo, hi, prev; b8 started; } cfframe;

//  Frames overlay the iterator's caller-owned u32 area, 4 words apiece.
_Static_assert(sizeof(cfframe) == 4 * sizeof(u32), "cfframe is 4 u32s");

//  DIS-082: unused directly, but Sx.h's generated FindGE requires a Z.
fun b8 cfframeZ(cfframe const *a, cfframe const *b) { return a->lo < b->lo; }

#define X(M, name) M##cfframe##name
#include "abc/Bx.h"
#undef X

//  Pick the next present child of `f` in RGA order; *got = NO when the
//  group is exhausted.  `prev` plus a total order replaces a visited set.
static ok64 cfold_pick(u32 *out, b8 *got, cfwalk const *k, cfframe const *f) {
    sane(out && got);
    *got = NO;
    u32 m = 0;
    //  Fork-free fast path: a one-child group is visited once, at the cost
    //  of one presence test — no blame lookup, no ordering work.
    if (f->hi == f->lo + 1) {
        if (f->started) done;
        call(cfold_present_mask, &m, k->w, k->scs, k->nsc,
             cf_at(k->w->idx, f->lo));
        if (m) { *out = f->lo; *got = YES; }
        done;
    }
    cfrgakey best = {}, pk = {}, pv = {};
    if (f->started) call(cfold_rga_key, &pv, k, cf_at(k->w->idx, f->prev));
    for (u32 i = f->lo; i < f->hi; i++) {
        tokv32 e = cf_at(k->w->idx, i);
        call(cfold_present_mask, &m, k->w, k->scs, k->nsc, e);
        if (!m) continue;
        call(cfold_rga_key, &pk, k, e);
        if (f->started && !cfrgakeyLt(&pv, &pk)) continue;   // already visited
        if (*got && !cfrgakeyLt(&pk, &best)) continue;
        best = pk;
        *out = i;
        *got = YES;
    }
    done;
}

//  Per-token callback: `off`..`end` is the token's body range; `present`
//  and `alive` carry one bit per scope (alive ⊆ present: no visible tomb).
//  A tombed token is still WALKED (its children keep their place in
//  document order), just not emitted.
typedef ok64 (*cfwalkcb)(void *ctx, u32 off, u32 end, u8 tag,
                         u32 present, u32 alive);

//  Iterator state words in the caller-owned u32 area (CFOLDIterMem
//  elements): [0] phase, [1] depth, [2..2+ncommits) the topo ranks, then
//  the DFS frames, one cfframe (4 words) apiece.
#define CFOLD_ITER_FRESH 0u
#define CFOLD_ITER_LIVE  1u
#define CFOLD_ITER_DONE  2u

//  One step of the preorder DFS at scopes `scs[0..nsc)` — THE walk; every
//  reader loops over it.  A token's END comes from its chain terminator
//  when it has one, else from the smallest body offset among its INSERT
//  children — always its chain successor, because the successor was
//  appended immediately after it and every later child of the same token
//  lands further along the append-only body.
static ok64 cfold_iter_next(cfoldtok *out, u32 *present, u32 *alive, b8 *got,
                            cfold const *w, cfscope const *scs, u32 nsc,
                            u32s mem, cfstat *st) {
    sane(out && got && nsc >= 1 && nsc <= CFOLD_SCOPES_MAX);
    *got = NO;
    u32 n  = (u32)$len(w->idx);
    u32 nc = CFOLDNCommits(w);
    if ((size_t)$len(mem) < CFOLDIterMem(w)) fail(CFOLDFAIL);
    u32     *phase = &mem[0][0], *depth = &mem[0][1];
    u32s     rank  = {mem[0] + 2, mem[0] + 2 + nc};
    cfframe *stk   = (cfframe *)(mem[0] + 2 + nc);
    if (*phase == CFOLD_ITER_DONE) done;
    if (*phase == CFOLD_ITER_FRESH) {
        if (n == 0) { *phase = CFOLD_ITER_DONE; done; }
        call(cfold_ranks, rank, w);
        u32 rlo = cfold_group(w, CFOLD_ROOT, 0, st);
        stk[0]  = (cfframe){rlo, cfold_group_end(w, CFOLD_ROOT, rlo), 0, NO};
        *depth  = 1;
        *phase  = CFOLD_ITER_LIVE;
    }
    cfwalk k = {.w = w, .scs = scs, .nsc = nsc, .st = st};
    k.rank[0] = rank[0];
    k.rank[1] = rank[0] + nc;

    while (*depth > 0) {
        cfframe *f  = &stk[*depth - 1];
        u32     pk  = 0;
        b8      gp  = NO;
        call(cfold_pick, &pk, &gp, &k, f);
        if (!gp) { (*depth)--; continue; }
        f->started = YES;
        f->prev    = pk;

        tokv32 e   = cf_at(w->idx, pk);
        u32    off = tok32Offset(e.chi);
        u8     tag = tok32Tag(e.chi);
        tok32  par = CFOLDPar(off);
        u32    glo = cfold_group(w, par, pk + 1, st);
        u32    ghi = cfold_group_end(w, par, glo);

        //  One scan of the group settles the token's END and its liveness.
        u32 pm = 0;
        call(cfold_present_mask, &pm, w, scs, nsc, e);
        u32 am = pm;
        b8  hasterm = NO, hasmin = NO;
        u32 term = 0, mino = 0;
        for (u32 i = glo; i < ghi; i++) {
            tokv32 c  = cf_at(w->idx, i);
            u8     ct = tok32Tag(c.chi);
            u32    co = tok32Offset(c.chi);
            if (ct == CFOLD_TAG_TERM) { hasterm = YES; term = co; continue; }
            if (ct == CFOLD_TAG_TOMB) {
                for (u32 s = 0; s < nsc; s++)
                    if (co <= scs[s].cidx && !cfscope_ignored(&scs[s], co))
                        am &= ~(1u << s);
                continue;
            }
            if (!hasmin || co < mino) { hasmin = YES; mino = co; }
        }
        u32 end = hasterm ? term : mino;
        if (!hasterm && !hasmin) fail(CFOLDTORN);
        if (end < off || end > (u32)u8csLen(w->body)) fail(CFOLDTORN);
        if (*depth > n) fail(CFOLDTORN);
        stk[(*depth)++] = (cfframe){glo, ghi, 0, NO};

        if (st) { st->present++; if (am) st->emitted++; }
        out->off   = off;
        out->end   = end;
        out->tag   = tag;
        out->alive = (am & 1u) != 0;
        if (present) *present = pm;
        if (alive)   *alive   = am;
        *got = YES;
        done;
    }
    *phase = CFOLD_ITER_DONE;
    done;
}

ok64 CFOLDIterNext(cfoldtok *out, b8 *got, cfold const *w, u32 rev, u32s mem) {
    sane(out && got);
    cfscope sc = {};
    call(cfold_scope, &sc, w, rev);
    call(cfold_iter_next, out, NULL, NULL, got, w, &sc, 1, mem, NULL);
    done;
}

//  The internal callback walk is a loop over the iterator (scratch state on
//  BASS), so there is exactly ONE walking mechanism.
static ok64 cfold_walk(cfold const *w, cfscope const *scs, u32 nsc,
                        cfwalkcb cb, void *ctx, cfstat *st) {
    sane(w && cb);
    a_carve(u32, memb, CFOLDIterMem(w));
    a_dup(u32, mem, u32bIdle(memb));
    mem[0][0] = CFOLD_ITER_FRESH;
    mem[0][1] = 0;
    for (;;) {
        cfoldtok t  = {};
        u32 pm = 0, am = 0;
        b8  got = NO;
        call(cfold_iter_next, &t, &pm, &am, &got, w, scs, nsc, mem, st);
        if (!got) break;
        call(cb, ctx, t.off, t.end, t.tag, pm, am);
    }
    done;
}

// ============================================================
//  Produce / Alive
// ============================================================

typedef struct { u8bp out; cfold const *w; } cfemit_ctx;

static ok64 cfold_emit_cb(void *vctx, u32 off, u32 end, u8 tag,
                          u32 present, u32 alive) {
    sane(vctx);
    (void)tag; (void)present;
    cfemit_ctx *c = vctx;
    if (!(alive & 1)) done;
    a_part(u8c, seg, c->w->body, off, end - off);
    call(u8bFeed, c->out, seg);
    done;
}

static ok64 cfold_render(cfold const *w, cfscope const *sc, u8b out,
                          cfstat *st) {
    sane(w && out);
    u8bReset(out);
    cfemit_ctx c = {.out = out, .w = w};
    call(cfold_walk, w, sc, 1, cfold_emit_cb, &c, st);
    done;
}

ok64 CFOLDProduce(cfold const *w, u32 c, u8b out, cfstat *st) {
    sane(w && out);
    cfcommit ac = {};
    call(CFOLDCommitAt, &ac, w, c);
    cfscope sc = {};
    call(cfold_scope, &sc, w, c);
    call(cfold_render, w, &sc, out, st);
    if ((u32)u8bDataLen(out) != ac.P) fail(CFOLDPLEN);
    done;
}

ok64 CFOLDAlive(cfold const *w, u8b out) {
    sane(w && out);
    u32 n = CFOLDNCommits(w);
    u8bReset(out);
    if (n == 0) done;
    call(CFOLDProduce, w, n - 1, out, NULL);
    done;
}

// ============================================================
//  Emit: HUNK-record diff between two revs (the DOG-004 contract,
//  ported off the retired columnar WEAVE).  `from`/`to` are commit
//  indices — no scope bitmaps: a rev's visibility is already stored.
//  There is no capped fallback here: a body past the 16 MiB tok32 cap
//  cannot be FOLDED in the first place (CFOLDBIG), so every parseable
//  weave decodes.  Fenced merge renders are retired (DIS-080): the
//  merged bytes are just CFOLDProduce at the merge commit.
// ============================================================

#define CFOLD_FULL_HUNK_MAX (1UL << 20)
#define CFOLD_CTX_LINES 3

//  Flat per-token decode in DOCUMENT ORDER, classified from->to: 'I'
//  inserted (alive in `to` only), 'D' deleted (alive in `from` only),
//  ' ' context (both); tokens alive in neither are dropped here, so the
//  emit loops never see them.  Carves persist in the CALLER's frame —
//  invoke cfold_emit_decode DIRECTLY, never via call().
typedef struct {
    u32  n;      // visible tokens
    u8c *text;   // their bytes, concatenated in document order
    u32 *end;    // cumulative end offset per token (into `text`)
    u8  *cls;    // 'I' / 'D' / ' '
    u32  tlen;   // total text length
} cfdec;

typedef struct { cfold const *w; u8bp tx; u32bp end; u8bp cls; } cfdec_ctx;

static ok64 cfold_dec_cb(void *vctx, u32 off, u32 end, u8 tag,
                         u32 present, u32 alive) {
    sane(vctx);
    (void)tag; (void)present;
    cfdec_ctx *c = vctx;
    b8 af = (alive & 1u) != 0, at = (alive & 2u) != 0;
    if (!af && !at) done;
    a_part(u8c, seg, c->w->body, off, end - off);
    call(u8bFeed, c->tx, seg);
    call(u32bFeed1, c->end, (u32)u8bDataLen(c->tx));
    call(u8bFeed1, c->cls, at && !af ? 'I' : (af && !at ? 'D' : ' '));
    done;
}

static ok64 cfold_emit_decode(cfold const *w, u32 from, u32 to, cfdec *d) {
    sane(w && d);
    zerop(d);
    u32 nent = (u32)$len(w->idx);
    a_carve(u8,  tx,  u8csLen(w->body) + 1);
    a_carve(u32, end, (size_t)nent + 1);
    a_carve(u8,  cls, (size_t)nent + 1);
    cfscope scs[2] = {};
    call(cfold_scope, &scs[0], w, from);
    call(cfold_scope, &scs[1], w, to);
    cfdec_ctx c = {.w = w, .tx = tx, .end = end, .cls = cls};
    call(cfold_walk, w, scs, 2, cfold_dec_cb, &c, NULL);
    d->n    = (u32)u8bDataLen(cls);
    d->text = u8bDataHead(tx);
    d->end  = (u32 *)u32bDataHead(end);
    d->cls  = (u8 *)u8bDataHead(cls);
    d->tlen = (u32)u8bDataLen(tx);
    done;
}

fun u32 cfdec_lo(cfdec const *d, u32 i) { return i ? d->end[i - 1] : 0; }
fun u32 cfdec_hi(cfdec const *d, u32 i) { return d->end[i]; }

//  Overlay real syntax tags onto a diff hunk's side-only tok stream.
//  A hunk re-slices arbitrary token runs, so `text` is re-tokenised for
//  syntax and `out` (reset first) gets the union segmentation: each
//  emitted tok32 takes the syntax tag of its covering syntax token and
//  the diff side of its covering side segment.  Best-effort; on a
//  hiccup the neutral 'S' tag stands so the body always renders.
static ok64 cfold_overlay_syntax(u32b out, u8csc text, u8csc ext,
                                 tok32cs sides) {
    sane(out != NULL);
    u32bReset(out);
    u32 ns = (u32)$len(sides);
    if (ns == 0 || $empty(text)) done;

    a_carve(u32, syn, (size_t)($len(text) + 16));
    u32 nt = 0;
    if (!$empty(ext) && TOKKnownExt(ext) &&
        HUNKu32bTokenize(syn, text, ext) == OK)
        nt = (u32)u32bDataLen(syn);
    tok32c *st = (tok32c *)u32bDataHead(syn);
    tok32c *sd = sides[0];

    u32 tlen = (u32)$len(text);
    u32 pos = 0, i = 0, j = 0;
    while (pos < tlen) {
        u8  side = (i < ns) ? tok32Side(sd[i]) : TOK_SIDE_EQ;
        u8  tag  = (j < nt) ? tok32Tag(st[j])  : 'S';
        u32 a = (i < ns) ? tok32Offset(sd[i]) : tlen;
        u32 b = (j < nt) ? tok32Offset(st[j]) : tlen;
        u32 nb = a < b ? a : b;
        if (nb <= pos) break;   // defensive: strictly-increasing offsets
        call(u32bFeed1, out, tok32PackSide(tag, side, nb));
        if (a == nb && i < ns) i++;
        if (b == nb && j < nt) j++;
        pos = nb;
    }
    done;
}

//  Compose a hunk URI `<scheme><name>?<navver>#L<lineno>` into `uri`
//  (reset first), preserving DIFF-003/004 behaviour: a non-empty scheme
//  is prepended; `navver` rides as a query; the start line is 1-based.
static ok64 cfold_emit_uri(u8b uri, u8cs scheme, u8cs name, u8cs navver,
                           u32 lineno) {
    sane(uri != NULL);
    u8bReset(uri);
    if (!u8csEmpty(scheme)) call(u8bFeed, uri, scheme);
    call(u8bFeed, uri, name);
    if (!u8csEmpty(navver)) {
        call(u8bFeed1, uri, '?');
        call(u8bFeed, uri, navver);
    }
    u8csc empty = {NULL, NULL};
    call(HUNKu8sMakeURI, u8bIdle(uri), empty, empty, lineno + 1);
    done;
}

//  Emit one hunk (uri/text/sides) through `cb`: overlay syntax onto the
//  side-only stream, then hand a borrowed `hunk` to the callback.
static ok64 cfold_emit_flush(HUNKcb cb, void *ctx, u8b uri,
                             u8b text, u32b sides, u32b combined, u8cs ext) {
    sane(cb != NULL);
    a_dup(u8c, htext, u8bDataC(text));
    tok32cs sd = {(tok32c *)u32bDataHead(sides),
                  (tok32c *)u32bDataHead(sides) + u32bDataLen(sides)};
    call(cfold_overlay_syntax, combined, htext, ext, sd);
    hunk hk = {};
    hk.uri[0]  = u8bDataHead(uri);
    hk.uri[1]  = u8bDataHead(uri) + u8bDataLen(uri);
    hk.text[0] = u8bDataHead(text);
    hk.text[1] = u8bDataHead(text) + u8bDataLen(text);
    hk.toks[0] = (tok32c *)u32bDataHead(combined);
    hk.toks[1] = (tok32c *)u32bDataHead(combined) + u32bDataLen(combined);
    call(cb, &hk, ctx);
    done;
}

//  Windowed diff: emit only changed-line windows (CFOLD_CTX_LINES of
//  context) as `diff:<name>?<navver>#L<line>` hunks.
ok64 CFOLDEmitDiff(cfold const *w, u8cs name, u8cs navver,
                   u32 from, u32 to, HUNKcb cb, void *ctx) {
    sane(w && cb != NULL);
    if (CFOLDEmpty(w)) done;

    cfdec d = {};
    { ok64 r = cfold_emit_decode(w, from, to, &d); if (r != OK) return r; }
    u32 ntok = d.n;
    if (ntok == 0) done;
    u8c *text = d.text;

    //  Mark changed lines: a line carrying an 'I'/'D' token (and its
    //  immediate predecessor line) is changed.  total_lines_est bounds it.
    u32 total_lines_est = 1;
    for (u32 b = 0; b < d.tlen; b++) if (text[b] == '\n') total_lines_est++;
    a_carve(u8, changed, total_lines_est + 4);
    u8bReset(changed);
    for (u32 z = 0; z < total_lines_est; z++) call(u8bFeed1, changed, 0);
    u8 *cmark = (u8 *)u8bDataHead(changed);
    u32 cur_line = 0;
    for (u32 i = 0; i < ntok; i++) {
        u32 lo = cfdec_lo(&d, i), hi = cfdec_hi(&d, i);
        u32 nl = 0;
        for (u32 b = lo; b < hi; b++) if (text[b] == '\n') nl++;
        if (d.cls[i] == 'I' || d.cls[i] == 'D') {
            if (cur_line > 0) cmark[cur_line - 1] = 1;
            for (u32 l = cur_line; l <= cur_line + nl && l < total_lines_est; l++)
                cmark[l] = 1;
        }
        cur_line += nl;
    }
    u32 total_lines = cur_line + 1;
    if (total_lines > total_lines_est) total_lines = total_lines_est;

    //  Coalesce changed lines into [lo,hi] windows with context.
    a_carve(u32, windows, (total_lines + 4) * 2);
    u32 *wbuf = (u32 *)u32bIdleHead(windows);
    u32 nwin = 0;
    { u32 i = 0;
      while (i < total_lines) {
        if (!cmark[i]) { i++; continue; }
        u32 cluster_first = i, cluster_last = i;
        i++;
        while (i < total_lines) {
            if (cmark[i]) { cluster_last = i; i++; continue; }
            u32 j = i;
            while (j < total_lines && !cmark[j] &&
                   j - cluster_last <= 2 * CFOLD_CTX_LINES) j++;
            if (j < total_lines && cmark[j]) { cluster_last = j; i = j + 1; }
            else break;
        }
        u32 lo = (cluster_first > CFOLD_CTX_LINES)
                 ? cluster_first - CFOLD_CTX_LINES : 0;
        u32 hi = cluster_last + CFOLD_CTX_LINES;
        if (hi >= total_lines) hi = total_lines - 1;
        wbuf[nwin * 2] = lo; wbuf[nwin * 2 + 1] = hi; nwin++;
      } }
    if (nwin == 0) done;

    a_carve(u8,  outtext,  (size_t)d.tlen + 1);
    a_carve(u32, outtoks,  (size_t)ntok + 1);
    a_carve(u32, combined, 2 * (size_t)ntok + (size_t)d.tlen + 16);
    a_carve(u8,  outuri,   u8csLen(name) + u8csLen(navver) + 64);
    u8cs ext = {};
    PATHu8sExt(ext, name);
    a_cstr(dscheme, "diff:");

    u32 wi = 0, win_lo = wbuf[0], win_hi = wbuf[1];
    cur_line = 0;
    b8 hunk_open = NO;

    #define FLUSH() do { if (hunk_open) {                                 \
        call(cfold_emit_uri, outuri, dscheme, name, navver, win_lo);      \
        call(cfold_emit_flush, cb, ctx, outuri, outtext, outtoks,         \
             combined, ext);                                              \
        u8bReset(outtext); u32bReset(outtoks); hunk_open = NO; } } while (0)

    for (u32 i = 0; i < ntok; i++) {
        u32 lo = cfdec_lo(&d, i), hi = cfdec_hi(&d, i);
        u32 nl = 0;
        for (u32 b = lo; b < hi; b++) if (text[b] == '\n') nl++;
        while (wi < nwin && cur_line > win_hi) {
            FLUSH();
            wi++;
            if (wi < nwin) { win_lo = wbuf[wi * 2]; win_hi = wbuf[wi * 2 + 1]; }
        }
        if (wi >= nwin) break;
        if (cur_line >= win_lo && cur_line <= win_hi) {
            a_part(u8c, tb, ((u8csc){d.text, d.text + d.tlen}), lo, hi - lo);
            call(u8bFeed, outtext, tb);
            u8 side = (d.cls[i] == 'I') ? TOK_SIDE_IN
                    : (d.cls[i] == 'D') ? TOK_SIDE_RM : TOK_SIDE_EQ;
            call(u32bFeed1, outtoks,
                 tok32PackSide('S', side, (u32)u8bDataLen(outtext)));
            hunk_open = YES;
        }
        cur_line += nl;
    }
    FLUSH();
    #undef FLUSH
    done;
}

//  Whole-file diff: every visible token, change-tagged.  `scheme` (e.g.
//  `diff:` or empty for `cat:`) selects the renderer path (DIFF-003).
ok64 CFOLDEmitFull(cfold const *w, u8cs name, u8cs scheme, u8cs navver,
                   u32 from, u32 to, HUNKcb cb, void *ctx) {
    sane(w && cb != NULL);
    if (CFOLDEmpty(w)) done;

    cfdec d = {};
    { ok64 r = cfold_emit_decode(w, from, to, &d); if (r != OK) return r; }
    u32 ntok = d.n;
    if (ntok == 0) done;

    a_carve(u8,  outtext,  (size_t)d.tlen + 1);
    a_carve(u32, outtoks,  (size_t)ntok + 1);
    a_carve(u32, combined, 2 * (size_t)ntok + (size_t)d.tlen + 16);
    a_carve(u8,  outuri,   u8csLen(name) + u8csLen(navver) + 64);
    u8cs ext = {};
    PATHu8sExt(ext, name);

    b8 hunk_open = NO;
    u32 hunk_start_line = 0, cur_line = 0;

    #define FLUSH() do { if (hunk_open) {                                    \
        call(cfold_emit_uri, outuri, scheme, name, navver, hunk_start_line); \
        call(cfold_emit_flush, cb, ctx, outuri, outtext, outtoks,            \
             combined, ext);                                                 \
        u8bReset(outtext); u32bReset(outtoks); hunk_open = NO;               \
        hunk_start_line = cur_line; } } while (0)

    for (u32 i = 0; i < ntok; i++) {
        u32 lo = cfdec_lo(&d, i), hi = cfdec_hi(&d, i);
        u32 nl = 0;
        for (u32 b = lo; b < hi; b++) if (d.text[b] == '\n') nl++;
        if (hunk_open && u8bDataLen(outtext) + (hi - lo) > CFOLD_FULL_HUNK_MAX)
            FLUSH();
        a_part(u8c, tb, ((u8csc){d.text, d.text + d.tlen}), lo, hi - lo);
        call(u8bFeed, outtext, tb);
        u8 side = (d.cls[i] == 'I') ? TOK_SIDE_IN
                : (d.cls[i] == 'D') ? TOK_SIDE_RM : TOK_SIDE_EQ;
        call(u32bFeed1, outtoks,
             tok32PackSide('S', side, (u32)u8bDataLen(outtext)));
        hunk_open = YES;
        cur_line += nl;
    }
    FLUSH();
    #undef FLUSH
    done;
}

// ============================================================
//  Writer: tokenize, diff, append
// ============================================================

//  Line-coherent tokens: lex with the shared HUNK tokenizer, then split
//  every token at '\n' so the diff's line anchors work, and cover any tail
//  the lexer left (unknown ext) as plain 'S' lines.
typedef struct { u32b *toks; u32b *ends; u8b *tags; u64b *hashes; } cftok_out;

static ok64 atok_emit(cftok_out *o, u8csc blob, u8 tag, u32 lo, u32 hi) {
    sane(o);
    u32 p = lo;
    while (p < hi) {
        u32 q = p;
        while (q < hi && blob[0][q] != '\n') q++;
        u32 seg = (q < hi) ? q + 1 : hi;
        call(u32bFeed1, *o->toks, tok32Pack(tag, seg));
        if (o->ends) call(u32bFeed1, *o->ends, seg);
        if (o->tags) call(u8bFeed1, *o->tags, tag);
        if (o->hashes) {
            a_part(u8c, s, blob, p, seg - p);
            call(u64bFeed1, *o->hashes, RAPHash(s));
        }
        p = seg;
    }
    done;
}

static ok64 cfold_tokenize(cftok_out *o, u8csc blob, u8csc ext) {
    sane(o);
    if ($empty(blob)) done;
    a_carve(u32, raw, u8csLen(blob) + 16);
    call(HUNKu32bTokenize, raw, blob, ext);
    u32c *rp  = (u32c *)u32bDataHead(raw);
    u32   nr  = (u32)u32bDataLen(raw);
    u32   cur = 0;
    for (u32 i = 0; i < nr; i++) {
        u32 end = tok32Offset(rp[i]);
        if (end > (u32)u8csLen(blob)) end = (u32)u8csLen(blob);
        if (end <= cur) continue;
        call(atok_emit, o, blob, tok32Tag(rp[i]), cur, end);
        cur = end;
    }
    if (cur < (u32)u8csLen(blob))
        call(atok_emit, o, blob, 'S', cur, (u32)u8csLen(blob));
    done;
}

//  Pre-state collection: every token the new commit's parents can see, in
//  document order, with its body range — the diff baseline.
typedef struct {
    cfold const *w;
    u32b *off, *end, *tk;
    u8b  *tag, *tx;
    u64b *hash;
    u32   cum;
} apre_ctx;

static ok64 apre_cb(void *vctx, u32 off, u32 end, u8 tag,
                    u32 present, u32 alive) {
    sane(vctx);
    (void)present;
    apre_ctx *c = vctx;
    if (!(alive & 1)) done;
    a_part(u8c, seg, c->w->body, off, end - off);
    call(u32bFeed1, *c->off, off);
    call(u32bFeed1, *c->end, end);
    call(u8bFeed1, *c->tag, tag);
    call(u8bFeed, *c->tx, seg);
    c->cum += end - off;
    call(u32bFeed1, *c->tk, tok32Pack(tag, c->cum));
    call(u64bFeed1, *c->hash, RAPHash(seg));
    done;
}

//  Fold one commit.  `has_content` NO is a pure merge: nothing is appended
//  to the body or the index, only a 'C' record taking the later L and the
//  INTERSECTED ignore-set (everything no parent saw).
static ok64 cfold_fold(u8s into, cfold const *w, u8csc blob, u8csc ext,
                        u64 commit, u64csc ancestors, b8 has_content,
                        cffold *fs) {
    sane(into != NULL);
    if (fs) zerop(fs);
    cfold e = {};
    if (w) e = *w;
    u32 nc      = CFOLDNCommits(&e);
    u32 cidx    = nc;
    u32 bodylen = (u32)u8csLen(e.body);
    u32 nent    = (u32)$len(e.idx);

    //  Ignore-set: every commit already folded that is not in this one's
    //  causal closure.  For a merge that IS the intersection — a commit
    //  hides only when NEITHER parent saw it, and the earlier parent's
    //  unseen tail is simply not in its closure either.
    a_carve(u32, ign_new, (size_t)nc + 1);
    for (u32 j = 0; j < nc; j++) {
        cfcommit c = {};
        call(CFOLDCommitAt, &c, &e, j);
        b8 seen = NO;
        $for(u64c, a, ancestors) if (*a == c.id) { seen = YES; break; }
        if (!seen) call(u32bFeed1, ign_new, j);
    }

    //  The new commit's ignore list appended to the 'G' pool.
    u32 ign_off = (u32)(u8csLen(e.ign) / sizeof(u32));
    u32 ign_len = (u32)u32bDataLen(ign_new);
    a_carve(u8, o_ign, u8csLen(e.ign) + (size_t)ign_len * sizeof(u32) + 8);
    call(u8bFeed, o_ign, e.ign);
    a_dup(u32c, ign_slice, u32bDataC(ign_new));
    $for(u32c, p, ign_slice) {
        u32 v = *p;
        call(u8sFeed32, u8bIdle(o_ign), &v);
    }

    //  --- the diff baseline: the file as this commit's parents saw it ---
    a_carve(u32, al_off, (size_t)nent + 2);
    a_carve(u32, al_end, (size_t)nent + 2);
    a_carve(u8,  al_tag, (size_t)nent + 2);
    a_carve(u32, al_tk,  (size_t)nent + 2);
    a_carve(u64, al_h,   (size_t)nent + 2);
    a_carve(u8,  al_tx,  (size_t)bodylen + 2);
    if (nent > 0) {
        cfscope pre = {.L = bodylen, .cidx = cidx};
        $mv(pre.ign, ign_slice);
        apre_ctx pc = {.w = &e, .off = &al_off, .end = &al_end, .tk = &al_tk,
                       .tag = &al_tag, .tx = &al_tx, .hash = &al_h};
        call(cfold_walk, &e, &pre, 1, apre_cb, &pc, NULL);
    }

    //  --- the new content's tokens ---
    a_carve(u32, nw_tk,  u8csLen(blob) + 16);
    a_carve(u32, nw_end, u8csLen(blob) + 16);
    a_carve(u8,  nw_tag, u8csLen(blob) + 16);
    a_carve(u64, nw_h,   u8csLen(blob) + 16);
    if (has_content) {
        cftok_out to = {.toks = &nw_tk, .ends = &nw_end, .tags = &nw_tag,
                       .hashes = &nw_h};
        call(cfold_tokenize, &to, blob, ext);
    }

    u64 olen = u64bDataLen(al_h);
    u64 nlen = u64bDataLen(nw_h);

    //  --- output streams ---
    a_carve(u8, o_body, (size_t)bodylen + u8csLen(blob) + 2);
    call(u8bFeed, o_body, e.body);
    //  One entry per inserted token, one terminator per chain (a chain is
    //  at least one token), one tomb per dropped token.
    a_carve(tokv32, o_new, 2 * (size_t)nlen + (size_t)olen + 2);

    u32c *noff = (u32c *)u32bDataHead(nw_end);
    u8c  *ntag = (u8c *)u8bDataHead(nw_tag);
    u32c *aoff = (u32c *)u32bDataHead(al_off);

    tok32 prevpar   = CFOLD_ROOT;
    b8    chain     = NO;
    u32   chain_off = 0;

    #define CLOSE_CHAIN() do { if (chain) {                                  \
        tokv32 _t = {CFOLDPar(chain_off),                                   \
                     tok32Pack(CFOLD_TAG_TERM, (u32)u8bDataLen(o_body))};   \
        call(tokv32bFeed1, o_new, _t); chain = NO; } } while (0)
    #define KEEP(I) do { CLOSE_CHAIN(); prevpar = CFOLDPar(aoff[(I)]);       \
        } while (0)
    #define DROP(I) do { CLOSE_CHAIN();                                       \
        tokv32 _t = {CFOLDPar(aoff[(I)]),                                    \
                     tok32Pack(CFOLD_TAG_TOMB, cidx)};                       \
        call(tokv32bFeed1, o_new, _t); } while (0)
    #define ADD(J) do { u32 _lo = (J) ? noff[(J) - 1] : 0, _hi = noff[(J)];    \
        u32 _o = (u32)u8bDataLen(o_body);                                     \
        if (_o + (_hi - _lo) > TOK_OFF_MASK) fail(CFOLDBIG);                 \
        a_part(u8c, _seg, blob, _lo, _hi - _lo);                              \
        call(u8bFeed, o_body, _seg);                                          \
        tokv32 _t = {prevpar, tok32Pack(ntag[(J)], _o)};                      \
        call(tokv32bFeed1, o_new, _t);                                        \
        prevpar = CFOLDPar(_o); chain = YES; chain_off = _o; } while (0)

    if (!has_content) {
        /* pure merge: nothing appended */
    } else if (olen == 0) {
        for (u32 j = 0; j < (u32)nlen; j++) ADD(j);
    } else if (nlen == 0) {
        for (u32 i = 0; i < (u32)olen; i++) DROP(i);
    } else {
        u64 work_sz = DIFFWorkSize(olen, nlen);
        u64 edl_sz  = DIFFEdlMaxEntries(olen, nlen);
        a_carve(i32, work,   work_sz ? work_sz : 1);
        a_carve(u32, edlbuf, edl_sz ? edl_sz : 1);
        a_dup(u64c, oh, u64bDataC(al_h));
        a_dup(u64c, nh, u64bDataC(nw_h));
        e32g edlg = {edlbuf[0], edlbuf[3], edlbuf[0]};
        i32s ws   = {i32bHead(work), i32bTerm(work)};
        ok64 diff_o = BRAMu64s(edlg, ws, oh, nh);
        if (diff_o != OK) {
            call(BRAMFallbackEdl, edlg, (u32)olen, (u32)nlen);
            call(NEILCanon, edlg);
        } else {
            a_dup(u32c, at_view, u32bDataC(al_tk));
            a_dup(u32c, nt_view, u32bDataC(nw_tk));
            a_dup(u8c,  at_text, u8bDataC(al_tx));
            NEILCleanup(edlg, at_view, nt_view, at_text, blob);
            NEILShift  (edlg, at_view, nt_view, at_text, blob);
        }
        e32c *ep = edlbuf[0];
        e32c *ee = edlg[0];
        u32   oi = 0, ni = 0;
        while (ep < ee) {
            u32 op = DIFF_OP(*ep), len = DIFF_LEN(*ep);
            if (op == DIFF_EQ) {
                for (u32 k = 0; k < len; k++) { KEEP(oi); oi++; ni++; }
                ep++;
                continue;
            }
            u32 sum_ins = 0, sum_del = 0;
            while (ep < ee && DIFF_OP(*ep) != DIFF_EQ) {
                u32 l = DIFF_LEN(*ep);
                if (DIFF_OP(*ep) == DIFF_INS) sum_ins += l; else sum_del += l;
                ep++;
            }
            //  Insertions anchor on the LAST SURVIVOR before the hole
            //  (/wiki/Dirty): a plain abutment merges clean, and only an
            //  insertion INTERIOR to a removed run gets a tombed parent.
            for (u32 k = 0; k < sum_ins; k++) { ADD(ni); ni++; }
            for (u32 k = 0; k < sum_del; k++) { DROP(oi); oi++; }
        }
    }
    CLOSE_CHAIN();
    #undef CLOSE_CHAIN
    #undef KEEP
    #undef DROP
    #undef ADD

    //  --- index: sort this commit's batch, merge it in, one pass ---
    tokv32bSort(o_new);
    u32 nnew = (u32)tokv32bDataLen(o_new);
    a_carve(tokv32, o_idx, (size_t)nent + (size_t)nnew + 1);
    {
        a_dup(tokv32c, batch, tokv32bDataC(o_new));
        u32 a = 0, b = 0, steps = 0;
        while (a < nent || b < nnew) {
            steps++;
            b8 takea;
            if      (a >= nent) takea = NO;
            else if (b >= nnew) takea = YES;
            else {
                tokv32 x = cf_at(e.idx, a), y = cf_at(batch, b);
                takea = !tokv32Z(&y, &x);
            }
            if (takea) { call(tokv32bFeed1, o_idx, cf_at(e.idx, a)); a++; }
            else       { call(tokv32bFeed1, o_idx, cf_at(batch, b)); b++; }
        }
        if (fs) { fs->entries_old = nent; fs->entries_new = nnew;
                  fs->merge_steps = steps; }
    }

    //  --- commit record; P is filled in after the render it describes ---
    u32 newlen = (u32)u8bDataLen(o_body);
    if (newlen > TOK_OFF_MASK) fail(CFOLDBIG);
    a_carve(u8, o_cmts, u8csLen(e.cmts) + CFOLD_CMT_SZ + 8);
    call(u8bFeed, o_cmts, e.cmts);
    cfcommit rec = {.id = commit, .start = bodylen, .end = newlen, .L = newlen,
                   .P = 0, .ign_off = ign_off, .ign_len = ign_len};
    call(cfold_feed_commit, o_cmts, &rec);

    cfold nw = {};
    u8csMv(nw.body, u8bDataC(o_body));
    tokv32csMv(nw.idx, tokv32bDataC(o_idx));
    u8csMv(nw.cmts, u8bDataC(o_cmts));
    u8csMv(nw.ign, u8bDataC(o_ign));

    {
        cfscope sc = {};
        call(cfold_scope, &sc, &nw, cidx);
        a_carve(u8, mat, (size_t)newlen + 2);
        call(cfold_render, &nw, &sc, mat, NULL);
        u32 plen = (u32)u8bDataLen(mat);
        a_part(u8, pslot, u8bData(o_cmts),
               (size_t)cidx * CFOLD_CMT_SZ + 20, 4);
        call(u8sFeed32, pslot, &plen);
    }

    if (fs) { fs->body_before = bodylen; fs->body_after = newlen; }
    call(cfold_serialize, into, &nw);
    done;
}

ok64 CFOLDFold(u8s into, cfold const *w, u8csc blob, u8csc ext,
                u64 commit, u64csc ancestors) {
    return cfold_fold(into, w, blob, ext, commit, ancestors, YES, NULL);
}

ok64 CFOLDFoldStat(u8s into, cfold const *w, u8csc blob, u8csc ext,
                    u64 commit, u64csc ancestors, cffold *fs) {
    return cfold_fold(into, w, blob, ext, commit, ancestors, YES, fs);
}

ok64 CFOLDMerge(u8s into, cfold const *w, u64 commit, u64csc ancestors) {
    u8csc none = {NULL, NULL};
    return cfold_fold(into, w, none, none, commit, ancestors, NO, NULL);
}
