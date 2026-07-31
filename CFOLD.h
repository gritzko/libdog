#ifndef DOG_CFOLD_H
#define DOG_CFOLD_H

//  CFOLD (DIS-082): the APPEND-ONLY weave.  One file's whole DAG history
//  as three streams, sitting ALONGSIDE the columnar dog/WEAVE — deletion is
//  an appended INDEX ENTRY, never a mutation of the deleted token, so the
//  BODY is byte-identical forever once written.
//
//    body     ('B')  u8    every token's bytes ever inserted, in append
//                          order; a commit appends only what it inserts.
//                          STRICTLY APPEND-ONLY.
//    index    ('E')  tokv32 entries (parent, child), PARENT-SORTED.  Not
//                          append-only: a commit's new entries are sorted
//                          and MERGED into the array in one sequential pass,
//                          so the index is rewritten per commit BY DESIGN.
//    commits  ('C')  one fixed-size record per commit, in build order:
//                          (id, body range [start,end), L, P, ignore ref).
//    ignore   ('G')  u32 pool of commit-index lists the 'C' records point
//                          into, so a commit record stays fixed-size.
//
//  A token's IDENTITY, causal rank and blame key are all ONE number: its
//  body offset.  No anchor hash, no inserter column, no remover list, no
//  side bits.  Offsets are ARRIVAL-LOCAL (a weave is derived data, never
//  exchanged); only the RGA sibling tie-break reads the canonical commit id.
//
//  ORDER is a preorder DFS of the causal tree: emit a token, then recurse
//  into its children.  Siblings are contiguous in the index (it is
//  parent-sorted), so a fork-free chain walks as a pure linear scan.
//
//  VISIBILITY at commit c: a token is present iff its body offset < L_c and
//  its inserting commit is not in c's ignore-set; it is ALIVE iff no tomb
//  child of it is itself present at c.  Linear history has an empty
//  ignore-set, so presence is one integer compare.

#include "abc/BUF.h"
#include "abc/INT.h"
#include "abc/S.h"
#include "dog/HUNK.h"
#include "dog/tok/TOK.h"

con ok64 CFOLDFAIL = 0xc3d854d3ca495;    // malformed stream
con ok64 CFOLDBIG  = 0x30f61534b490;     // body past the 16 MiB tok32 cap
con ok64 CFOLDNOCM = 0xc3d854d5d8316;    // no such commit
con ok64 CFOLDTORN = 0xc3d854d7586d7;    // an entry names a token that is not there
con ok64 CFOLDPLEN = 0xc3d854d655397;    // walk produced != the recorded P

// ---------------------------------------------------------------------
//  tokv32 — a PAIR of tok32, 64 bits, fixed layout.  The index element.
// ---------------------------------------------------------------------
//  `par` NAMES the CT parent by body offset under one fixed tag, so all
//  children of one token share a byte-identical `par`, the whole 32-bit
//  word orders by parent offset, and siblings stay contiguous.  `chi` is
//  the entry itself; its TAG says what kind of entry this is.
typedef struct tokv32 {
    tok32 par;
    tok32 chi;
} tokv32;

//  Parent-major order.  Both halves compare as whole tok32 words, so the
//  tag lands in the high bits and children of one parent stay contiguous.
fun b8 tokv32Z(tokv32 const *a, tokv32 const *b) {
    if (a->par != b->par) return a->par < b->par;
    return a->chi < b->chi;
}

#define X(M, name) M##tokv32##name
#include "abc/Bx.h"
#include "abc/QSORTx.h"
#undef X

// ---------------------------------------------------------------------
//  Reserved entry tags (the 5-bit tok32 tag field; 'A'..'`')
// ---------------------------------------------------------------------
//  The syntax tags a tokenizer emits are D G L H R P S W U N C F; A, B, E
//  and T are free, so they carry the four structural roles.  A syntax tag
//  in `chi` means an ordinary INSERTed token.
#define CFOLD_TAG_ROOT 'A'   // the file-start sentinel (tok32 value 0)
#define CFOLD_TAG_PAR  'B'   // a `par` reference: offset of the parent token
#define CFOLD_TAG_TERM 'E'   // chain terminator: chi offset = the chain end
#define CFOLD_TAG_TOMB 'T'   // tombstone: chi offset = the DELETING COMMIT INDEX

//  The parent every file-start token hangs off.  tok32Pack('A',0) == 0, so
//  the root group sorts first and a real parent (tag 'B') never collides.
#define CFOLD_ROOT ((tok32)0)

//  Name the token at body offset `off` as a parent.
fun tok32 CFOLDPar(u32 off) { return tok32Pack(CFOLD_TAG_PAR, off); }

// ---------------------------------------------------------------------
//  'V' outer TLV container; 'B'/'E'/'C'/'G' sub-records
// ---------------------------------------------------------------------
#define CFOLD_TLV      'V'
#define CFOLD_TLV_BODY 'B'
#define CFOLD_TLV_IDX  'E'
#define CFOLD_TLV_CMT  'C'
#define CFOLD_TLV_IGN  'G'

//  One 'C' record, 32 bytes on the wire (LE): id, start, end, L, P,
//  ignore offset (in u32 units into 'G'), ignore length.
#define CFOLD_CMT_SZ 32

typedef struct {
    u64 id;      // canonical commit id (hi64 of the sha1) — the ONLY
                 // canonical thing the order ever reads
    u32 start;   // body range this commit appended, [start, end)
    u32 end;
    u32 L;       // body length at this commit (== end)
    u32 P;       // MATERIALIZED length at this commit (allocation + check)
    u32 ign_off; // index into the 'G' pool, in u32 units
    u32 ign_len; // how many commit indices
} cfcommit;

typedef struct {
    u8cs     body;  // 'B'
    tokv32cs idx;   // 'E'
    u8cs     cmts;  // 'C', CFOLD_CMT_SZ-byte records
    u8cs     ign;   // 'G', u32 LE
} cfold;

fun b8 CFOLDEmpty(cfold const *w) { return (u32)$len(w->idx) == 0; }

// --- codec -----------------------------------------------------------
ok64 CFOLDParse(cfold *w, u8csc blob);          // zero-copy view over 'V'
u32  CFOLDNCommits(cfold const *w);
ok64 CFOLDCommitAt(cfcommit *out, cfold const *w, u32 i);
//  Commit id -> build index.  CFOLDNOCM when the id was never folded.
ok64 CFOLDFindCommit(u32 *out, cfold const *w, u64 id);

// --- read ------------------------------------------------------------
//  Walk statistics — the format's efficiency claim, made measurable:
//  `bsearch` counts the binary searches the walk needed, so a fork-free
//  chain can be ASSERTED to stay a linear scan (bsearch == 0).
typedef struct {
    u32 groups;   // sibling groups looked up
    u32 bsearch;  // of those, how many needed a binary search
    u32 emitted;  // tokens whose bytes were emitted
    u32 present;  // tokens present in scope (alive or tombed)
} cfstat;

//  Materialize the file as commit index `c` saw it.  Asserts the produced
//  length against the recorded P_c (CFOLDPLEN on a mismatch).  `st` may be
//  NULL.
ok64 CFOLDProduce(cfold const *w, u32 c, u8b out, cfstat *st);
//  The last-folded commit's view.
ok64 CFOLDAlive(cfold const *w, u8b out);
//  BLAME: which commit appended the token at body offset `off`?  One binary
//  search over the 'C' range table — no per-token inserter column.
ok64 CFOLDBlame(u32 *out, cfold const *w, u32 off);
//  ITERATION: the preorder DFS walk, one token per call, in document order.
//  The iterator state (topo ranks + the DFS frame stack) lives in a CALLER-
//  OWNED u32 area of CFOLDIterMem(w) elements, ZEROED before the first call —
//  so a stateless binding can persist it across calls, and every reader
//  (produce, emit, blame views) is a loop over this one mechanism.  A token's
//  `off`/`end` are BODY offsets — the identity — so `CFOLDBlame(off)` names
//  its inserter; `alive` clear means a visible tomb (walked, not rendered).
typedef struct {
    u32 off, end;   // body range
    u8  tag;        // stored syntax tag
    b8  alive;      // no tomb of it is visible at the rev
} cfoldtok;

fun size_t CFOLDIterMem(cfold const *w) {
    return 2 + CFOLDNCommits(w) + 4 * ((size_t)$len(w->idx) + 1);
}
//  One step at commit index `rev`; *got = NO once the walk is exhausted.
ok64 CFOLDIterNext(cfoldtok *out, b8 *got, cfold const *w, u32 rev, u32s mem);

//  Windowed and whole-file diff between two revs, emitted as HUNK records
//  (DIFF-003/004: `name`, `scheme`, `navver` URIs).  `from`/`to` are commit
//  INDICES — a rev's visibility is stored, so no scope bitmaps exist.
//  Fenced conflict renders are retired (DIS-080): merged bytes are just
//  CFOLDProduce at the merge commit.
ok64 CFOLDEmitDiff(cfold const *w, u8cs name, u8cs navver,
                   u32 from, u32 to, HUNKcb cb, void *ctx);
ok64 CFOLDEmitFull(cfold const *w, u8cs name, u8cs scheme, u8cs navver,
                   u32 from, u32 to, HUNKcb cb, void *ctx);

// --- write -----------------------------------------------------------
//  Fold one commit in.  `ancestors` lists the ids of the new commit's
//  ancestors (its whole causal closure, itself excluded) — the same shape
//  WEAVEScope takes, because the weave stores no parent edges.  Everything
//  already folded and NOT named there lands in the new commit's ignore-set,
//  which is exactly the INTERSECTION rule for a merge: a commit hides only
//  when no parent saw it.
//
//  Appends `blob`'s new tokens to the body, appends one INDEX ENTRY per
//  inserted token plus a chain terminator, and one TOMB entry per token the
//  diff dropped.  `w` NULL/empty starts a fresh weave.
ok64 CFOLDFold(u8s into, cfold const *w, u8csc blob, u8csc ext,
                u64 commit, u64csc ancestors);

//  What one fold did, so the two stream properties can be ASSERTED rather
//  than eyeballed: `body_before`/`body_after` bracket the append-only body,
//  and `merge_steps` must equal `entries_old + entries_new` — the per-commit
//  index merge touches each element exactly once, one sequential pass.
typedef struct {
    u32 entries_old;
    u32 entries_new;
    u32 merge_steps;
    u32 body_before;
    u32 body_after;
} cffold;

ok64 CFOLDFoldStat(u8s into, cfold const *w, u8csc blob, u8csc ext,
                    u64 commit, u64csc ancestors, cffold *fs);
//  A merge that carries no content of its own: appends NOTHING to the body
//  and NOTHING to the index, only a 'C' record taking the later L and the
//  intersected ignore-set.
ok64 CFOLDMerge(u8s into, cfold const *w, u64 commit, u64csc ancestors);

#endif
