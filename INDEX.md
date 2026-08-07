#   dog — module index

dog is the beagle shared layer between abc and the verb dogs (sniff/keeper/graf/spot/bro): the `.be/` store conventions, the URI/branch vocabulary, the ULOG event log, the hunk wire format, and the ragel tokenizers that paint syntax. Naming follows abc; a `u8s` arg is consumed, a `u8sp` assigned. `DOG*FromBe` split a `.be`-anchor, `*Pup*` manage file stacks, `tok32` packs a tag + side. Prose in [DOG.md], [ULOG.md], [DEF.md], [BRCT.md].

##  Core conventions

###  DOG.h — URI grammar, `.be` layout, path hashes, puppies

The shared vocabulary every dog speaks: parse/canonicalise a CLI URI, split a `.be`-anchor path into repo/project/branch, hash a tree path, and drive the puppy file stacks. Full prose in [DOG.md].

 -  `DOGParseURI`/`DOGNormalizeArg`/`DOGPromoteBareword` — parse a CLI arg with dog rules (bare `word:path` → remote alias).
 -  `DOGCanonURI`/`DOGCanonURIFeed` — the chokepoint every ULOG/REFS writer uses to canonicalise + serialise a URI.
 -  `DOGIsProjector`/`ProjectorDog`/`IsTransport`/`IsGitTransport` — classify a URI scheme against the tables.
 -  `DOGDebangSlice`/`DOGDebang`/`DOGDebangFeed` — the `!` reader: tail-shed + OR the per-component force bits.
 -  `DOGRepoFromBe`/`DOGProjectFromBe`/`DOGBranchFromBe` — split a row-0 anchor path on `/.be/` into the wt root.
 -  `DOGRefDrain`/`QueryBranchOnly`/`StripProject`/`QueryProject`/`CanonQueryParse` — split a ref query.
 -  `DOGIsHashlet`/`DOGIsFullSha`/`DOGRefIsBranch` — syntactic tests: a 6..40-hex sha prefix, a full 40/64-hex object id.
 -  `DOGPathHash`/`DOGChildPathHash` (`ROOT`) — positional tree-node id: root is ron60("ROOT").
 -  `DOGPupOpenAll`/`OpenAside`/`Create`/`CreateAt`/`ThinTail`/`DropAt`/`DOGPupClose` — open/load/append/trim/drop one/close a puppy stack.
 -  `DOGPupCount`/`DOGPupCountAll`/`DOGPupData`/`DOGPupDataAll`/`DOGPupAllData`/`DOGPupSeqno` — read views over the stack.
 -  `dogpuplane` (`dogpupsync` + `dogpupmerge`) — the family's typed sort/dedup/collapse + compaction-merge roles, passed DOWN on every call; nothing about the lane is ever dicted.
 -  `DOGPupPut`/`DOGPupCommit`/`DOGPupLadder` — append a record to the `<dir>/<ext>` memtable; seal it into a run; keep the 1/8 ladder (every seal ends there).
 -  Run names are 10-char RON64 only: `DOGPupOpenAll` UNLINKS a `<ext>` file with any other seqno width (jab's retired 8-wide JS names), so the family recomputes.
 -  A run never overwrites a name: `DOGPupCreate` bumps its key past any file already there (RON64 is case-sensitive, APFS is not — two keys, one name); `DOGPupCreateAt` refuses instead, its key being the caller's.
 -  `DOGPupAllRuns` — query view: committed runs oldest→newest, then the memtable's PAST and DATA.
 -  `DOGutf8sFeedDate` — render a unix ts as a 7-column relative date (`12:34` / `Tue05` / `01Jan`) for status columns.
 -  `DOG_BE_NAME`/`DOG_REFS_NAME`/`DOG_WTLOG_NAME`/`DOG_CONFIG_NAME` — the source of truth for `.be` filenames.

###  DPATH.h — tree-entry names & branch-path normalization

Validate a single tree-entry segment and put branch paths into the one canonical form (`.be/` sharding) so they compare by byte equality.

 -  `DPATHu8sDrainSeg`/`DPATHVerify` — drain/verify one filename segment (no slashes, valid UTF-8, rejects).
 -  `DPATHBranchNormFeed` — feed the canonical branch form: trunk aliases → empty, else the body + a trailing `/`.
 -  `DPATHBranchAncestor`/`DPATHBranchLcaLen`/`DPATHBranchResolveRel` — ancestor (prefix) test.

###  HOME.h — per-process ambient workspace (`&HOME`)

`home` is cwd-derived ambient state, so the process holds exactly one live home — the `&HOME` singleton. The top of each dog's call chain opens it once and pairs it with one close; everything downstream reads `&HOME`.

 -  `HOMEOpen`/`HOMEOpenAt`/`HOMEClose` — populate / release `&HOME` from an anchor URI (path=root, query=branch).
 -  `HOMEFind`/`HOMEFindDogs`/`HOMEResolveSibling` — walk cwd up to the nearest `.be` anchor.
 -  `HOMEOpenBranch`/`SetCurBranch`/`WriteBranch`/`BranchVisible` — claim/retarget/read a branch + test scope.
 -  `HOMEBeDir`/`HOMEMakeBeDir`/`HOMEBranchDir`/`HOMEProjectExists` — compose `.be`/branch/shard dirs.
 -  `HOMEGetConfig`/`HOMEHost` — read a dotted key from `.be/config` (TOML).

###  CLI.h — argv → `cli`, flag access, output mode

Parse `dog [verb] [--flags] [URI...]` into a `cli` whose slices borrow argv; URIs are stored raw and parsed on demand.

 -  `CLIParse` (`cli`) — fill verb / interleaved `flags` / raw `uris` from argv against a verb-name and value-flag list.
 -  `CLIUriLen`/`CLIUriRawAt`/`CLIUriSetRaw`/`CLIUriParse`/`CLIUriAt` — count, read, rewrite.
 -  `CLIFlag`/`CLIHas`/`CLIAtURI` — read a flag's value, test a boolean flag.
 -  `CLISetHUNKMode` — resolve the process-global `HUNKMode` from `--tlv`/ `--color`/`--plain` (default keys off).

###  VERSN.h — compile-time build metadata (version / hash / date)

Build stamp every binary can report on `--version`. `dog/version.cmake` writes the three literals into a generated `VERSN_BUILD.h` (rewritten only on change, so only VERSN.c recompiles); off a git checkout the version/hash read "unknown".

 -  `VERSNVersion`/`VERSNHash`/`VERSNDate` — `git describe` / short hash / source date (ISO-8601 UTC) as const `u8cs` slices.
 -  `VERSNu8sFeed` — feed `<prog> <version> (<hash>) <date>` into a slice (empty `prog` skipped; propagates SNOROOM).
 -  `VERSNReport` — print that one-line banner to stdout; the `--version` convenience for a dog's MAIN.

##  Event log & reporting

###  ULOG.h — append-only URI event log + sidecar index

Each row is `<ron60-ms>\t<verb>\t<uri>\n`, strictly monotonic; a `wh128` sidecar (`<dir>/.<base>.idx`) indexes ts→offset with a 20-bit verb prefilter and a tail sentinel for stale detection. Format in [ULOG.md].

 -  `ULOGOpen`/`ULOGOpenRO`/`ULOGOpenBooked`/`ULOGClose` (`ULOGTORN`) — open rw / read-only / sized.
 -  `ULOGu8sFeed`/`ULOGu8sDrain` (`ulogrec`) — the stateless row codec; one parsed row carries ts, verb.
 -  `ULOGAppend`/`ULOGAppendAt` — append with a clock-clamped or explicit ts (`ULOGCLOCK` on a non-monotonic stamp).
 -  `ULOGRow`/`ULOGSeek`/`ULOGFind`/`ULOGHead`/`ULOGTail`/`ULOGCount`/ `ULOGHas` — random access by index.
 -  `ULOGFindLatest`/`FindVerb`/`eachLatest`/`eachLatestKey` — reverse scans: latest row per predicate/verb/key.
 -  `ULOGCompactLatest`/`ULOGTruncate` — rewrite keeping the latest row per key (atomic tmp+rename), or keep a prefix.
 -  `ULOGu8ssDrainHeap`/`ULOGMergeWalk` (`ULOGu8csZbyTs`/`ZbyUri`) — K-way heap merge + fan-out.
 -  `ULOGu8bScanWt`/`ULOGu8sRelFromFull` — walk a worktree emitting one `<mtime>\t<verb>\t<rel>?<mode>` row per leaf.
 -  `ULOGVerbColor`/`ULOGVerbTag` (`ULOG_VERB_COLORS`) — the shared verb→`ansi64` / verb→theme-tag palette every status.
 -  `HUNKu8sFeedRec`/`HUNKTablePrintRow` — the two `ulogrec` conveniences over HUNK's status/action table (live here as HUNK.h can't take a `ulogrec`, circular).

###  HUNK.h — the code-hunk wire format, renderers, and status table

A `hunk` is `{ts, verb, uri, text, toks}` DATA; the renderers own all presentation. Three output modes are chosen once into the global `HUNKMode`; emit sites call `HUNKu8sFeedOut`.  BE-007 folded the old `dog/ROWS` builder in here as a plain status/action TABLE — NO row/table types, just a process-global accumulator (existing `Bu8`/`Bu32`/`hunk`) armed by `HUNKTableOpen` and serialized one row at a time.

 -  `hunk`/`hunkZ`/`HUNKcb` — the record (location is one `uri`; `toks` are `tok32` with tag + side), ordering, callback.
 -  `HUNKu8sFeed`/`HUNKu8sDrain` (`HUNKTOKLEN`/`HUNKTOKOOB`) — the nested-TLV codec.
 -  `HUNKu8sFeedOut`/`FeedText`/`FeedColor`/`FeedHtml` (`HUNKMode`) — render via the global mode, or forced.
 -  `HUNKu8sFeedBanner` — the ONE header drawer (BRO-002): the abbreviated date + verb + uri banner for every hunk (Plain).
 -  `HUNKu8sRebaseURI`/`HUNKu8sRelay` — prefix a hunk URI's path with a submodule mount (empty child uri stays bannerless, POST-022).
 -  `HUNKTableOpen`/`HUNKTableClose`/`HUNKTableFd` — arm / flush an mmap-backed module table; mode-keyed stream-or-buffer (`batch`).
 -  `HUNKu8sFeedRow`/`HUNKTableSummary` (`HUNK_NAV_*`) — serialize ONE event row / a summary tail; nav is a plain `ron60` scheme.
 -  `HUNKTableText`/`HUNKTableToks` — the active table's buffers for a rich multi-tok summary tail (`be status`).
 -  `HUNKu32bTokenize`/`u32sClip`/`u8sMakeURI`/`u8sFragSplit` — tokenize, clip toks, compose/split a `path#symbol:line` URI.

###  CFOLD.h — the APPEND-ONLY weave (DIS-082), replaces the columnar WEAVE.h

One file's whole DAG history; DELETION IS AN APPENDED ENTRY, never a mutation of the deleted token, so the `'B'` BODY is byte-identical forever and a fold never re-emits history. Three streams replace the old six columns — `'B'` body bytes in append order, `'E'` a `tokv32` index, `'C'` fixed-size commit records, `'G'` the ignore-set pool. A token's identity, causal rank and blame key are all ONE number, its body offset; there is no anchor hash, inserter, remover list or side bit. Offsets are ARRIVAL-LOCAL (a weave is derived data, never exchanged); only the RGA sibling tie-break reads the canonical commit id. Two DIFFERENT properties, both asserted by `test/CFOLD01.c`: the BODY is append-only, the INDEX is parent-SORTED and REWRITTEN per commit by a one-pass merge. `test/CFOLD02.c` is the oracle harness — whole commit DAGs, every rev materialized back byte-for-byte, plus a randomized DAG sweep; `fuzz/CFOLD.c` fuzzes the same recovery on the `DWEAVE` corpus format; `test/CFOLD03.c` replays the DIS-045 blank-EOL merge on real C; `test/CFOLD04.c` is the DIS-047 scaling bench.

 -  `tokv32`/`tokv32Z`/`CFOLDPar` (`CFOLD_TAG_ROOT`/`PAR`/`TERM`/`TOMB`) — the 64-bit (parent, child) index record; the child's TAG says INSERT (a syntax tag), TOMB or chain TERMINATOR, `par` names the parent by body offset so siblings are contiguous.
 -  `CFOLDParse` (`CFOLD_TLV*`, `CFOLDFAIL`/`BIG`/`NOCM`/`TORN`/`PLEN`) — zero-copy view over a `'V'` blob.
 -  `cfold`/`cfcommit`/`CFOLDNCommits`/`CFOLDCommitAt`/`CFOLDFindCommit` — the streams and one commit's `(id, body range, L, P, ignore ref)`.
 -  `CFOLDProduce`/`CFOLDAlive`/`cfstat` — DFS materialization at any commit, checked against the recorded `P`; `cfstat.bsearch` counts the walk's binary searches, so a fork-free chain is MEASURABLY a linear scan.
 -  `CFOLDBlame` — body offset → commit, one binary search over the `'C'` range table; `why:` needs no per-token inserter.
 -  `CFOLDFold`/`CFOLDFoldStat`/`cffold` — fold one commit (append body + entries, merge the sorted batch into the index); the stat block reports the append-only bracket and the one-pass merge step count.
 -  `CFOLDMerge` — a merge that carries no content: appends NOTHING, takes the later `L` and the INTERSECTED ignore-set (a commit hides only when NO parent saw it).
 -  `CFOLDEmitDiff`/`CFOLDEmitFull` — windowed / whole-file diff between two commit INDICES as HUNK records (DIFF-003/004 URIs); byte-parity with the retired WEAVE emitters (`test/CFOLD05.c`); fenced merge renders are retired (DIS-080) — merged bytes are `CFOLDProduce` at the merge commit; no DIFF-007 coarse fallback since a >16 MiB body is refused at fold time (`CFOLDBIG`).
 -  CLI: `dogcfold FILE...` folds each file as a successive linear commit, reports both streams per commit, and materializes every rev back.

###  BRAM.h, NEIL.h — token-level diff core (patience + EDL cleanup)

The two passes that sharpen a raw token-level Myers edit list (`e32g` EDL): line-coherent anchoring, then whitespace/boundary cleanup. Both work on packed `u32`/`u64` tokens over `abc/DIFF` gauges with no graf/keeper dependency; relocated from graf/ per DOG-002 so `dog/WEAVE` and other dogs can weave. Consumed by `graf/WEAVE.c` and legacy `graf/JOIN.c`.

 -  `BRAMu64s` (BRAM.h) — Bram Cohen patience diff over u64 token-hash arrays: anchor on unique lines, recurse between.
 -  `NEILCleanup`/`NEILShift`/`NEILCanon` (NEIL.h) — semantic EDL cleanup: drop false EQs, slide, collapse to INS+DEL.
 -  `NEILIsWS`/`NEIL_MAX_KILL`/`NEILBAD` (NEIL.h) — the whitespace-token test, the killable-EQ knob, the error code.

###  THEME.h — tag-letter colour palette

A 32-slot `ansi64` table indexed by tag letter (`tag - 'A'`); the renderer ORs an fg-tag and a bg-tag lookup into each cell's SGR.

 -  `theme`/`THEMEActive`/`THEMEAt` — the palette type, the always-non-NULL active pointer.
 -  `THEMESelect` (`THEME_16`/`THEME_DARK`/`THEME_LIGHT`) — pick a palette by name (falling back to `$BRO_THEME` then "16").
 -  `THEME_BANNER` (`THEME_BANNER_FG`/`_BG`) — the fixed black-on-pale-yellow SGR the one hunk-banner drawer uses.

###  WHIFF.h — packed tagged words & SHA-1 hashlets

`wh64` packs `off40 | id20 | type4` (hashlet in the top bits so entries sort by hashlet first); `wh128` is a `(key,val)` pair. Backs the ULOG index, keeper LSM index, and graf entries.

 -  `wh64`/`wh64Pack`/`wh64Type`/`wh64Id`/`wh64Off` (`wh64Z`) — pack / unpack the three fields and the value comparator.
 -  `wh128`/`wh128Z`/`wh128hash`/`wh128hashEq` (+ `wh128cs*`) — the pair, its full-field ordering.
 -  `WHIFFHashlet40`/`Hashlet60`/`HexFeed40`/`HexFeed60`/`HexHashlet40`/`HexHashlet60` — the 40/60-bit hashlet twins.

##  git interoperability (`dog/git/`)

###  SHA1.h — git SHA-1 type, hashing (sha1dc), and `sha1hex`

The git `sha1` (20 bytes) plus collision-detecting hashing and the 40-hex text form. CODE-019 folded the hex-text struct in here beside `sha1`.

 -  `sha1`/`sha1Z`/`sha1empty` — the 20-byte record, its memcmp ordering, and the all-zero test.
 -  `SHA1Sum`/`SHA1Open`/`SHA1Feed`/`SHA1Close` (`SHA1state`) — one-shot/streaming hashing over `dog/sha1dc`.
 -  `sha1FromBin`/`FromHex`/`Drain`/`slice`/`SHA1u8sFeedHex`/`FeedHashlet`/`a_sha1hex` — decode + render sha1 hex.
 -  `sha1hex`/`hexZ`/`hexeq`/`hexFromSha1`/`sha1FromSha1hex`/`hexFromHex`/`hexSlice` — the 40-hex key struct.

###  GIT.h — git object & refname parsers

Pull-mode drains for tree / commit / ident bytes, and the single translation point between be-style refs and git's `refs/heads/…` wire form.

 -  `GITParseRef`/`GITFeedRef`/`GITTypeName` (`gitref_kind`) — parse a refname into (kind, name) + emit its wire form.
 -  `GITu8sDrainTree`/`GITu8sFileSplit` — drain one tree entry (mode+name, raw SHA-1) and split the `<mode> <name>` field.
 -  `GITu8sDrainCommit`/`ParseCommit`/`CommitTree`/`GITu8sIdent` (`git_commit`) — drain/parse commit headers + tree + ident.
 -  `GIT_FIELD_*`/`GIT_REFS_*`/`GIT_PKT_*`/`GIT_TYPE_*` — the wire fixed strings (incl. beagle-only `foster`/`picked`).

###  PACK.h, DELT.h, ZINF.h — packfile, delta, zlib

The packfile codec, the git delta instruction parser/applier, and the zlib inflate/deflate wrappers under them.

 -  `PACKu8sFeedHdr`/`DrainHdr`/`DrainObjHdr`/`FeedObjHdr`/`PACKInflate` — write/parse the header + object entry.
 -  `PACKu8sFeedObj` — GIT-002 raw|OFS_DELTA object writer (caller log + base bytes/offset); shared by keeper + JABC; never REF_DELTA.
 -  `PACKRecordEnd` — GIT-007 record extent (offset → next record start, via inflate into BASS scratch); shared by keeper/JABC walk.
 -  `PACKResolveOfs` — GIT-004 OFS-only delta-chase (offset → inflated object); shared by keeper get + JABC; `PACKREF` on a stray REF.
 -  `PACKResolve(pack, off, base, delta, find, user, out, type)` — KEEP-006: the same chase plus a REF_DELTA arm; `find` (`pack_ref_find`: sha → base pack slice + offset) resumes the chase in whatever log holds the base, so a rotated log's cross-log bases resolve. Every hop remembers its own pack; `find == NULL` IS `PACKResolveOfs`.
 -  `DELTApply`/`DELTEncode` — apply a delta stream to a base.
 -  `ZINFInflate`/`ZINFDeflate` — the slice-consuming zlib wrappers (`into` advances by produced, source by consumed).

###  PIDX.h — pack-log index-EMIT (GIT-010)

The pack-log owns NO index; these ENTRY PRODUCERS emit `(sha→offset)` wh128 rows into a caller `Bwh128` (the `tok.parse(…, out?)` shape — hold nothing, JABC rule #4). Sort/merge/persist/query stay the caller's (an abc.index wh128 lane, the keeper puppy registry). Entry layout: `key = WHIFFKeyPack(type, hashlet60)`, `val = offset` (bare; caller re-packs file_id when merging). Shared by keeper + the `git.pack` JABC binding.

 -  `PIDXScan(pack, from_off, out, base, delta)` — walk one OFS-only pack (`PACKRecordEnd`), resolve each object (`PACKResolveOfs`), git-sha it (`PIDXObjSha`), emit one entry per object; the small single-pack, single-threaded core modelled on `keeper/UNPK.c::UNPKIndex` minus the keeper coupling/fork/REF-waiters/ingest. `PACKREF` on a stray REF. PACK-001: `from_off>0` tail-scans from a byte boundary to slice end (an OFS base earlier in the pack still resolves); `0` = whole pack from the 12-byte header (bounded by `hdr.count`).
 -  `PIDXScanRef(pack, from_off, out, base, delta, find, user)` — KEEP-006: the same scan over a log that MAY carry REF_DELTA records (a rotated repack log re-anchors every base left behind in an earlier log by sha); `find` is `PACKResolve`'s base finder. `PIDXScan` is this with `find == NULL`. A rescan in file-id order can answer from what it already emitted — a REF base always sits in a log BEFORE the one citing it.
 -  `PIDXFeedEmit` — index-on-append: git-sha the content the caller JUST fed (no resolve) and append its `(sha, offset)` entry.
 -  `PIDXObjSha`/`PIDXEntry` — the loose-object framing sha (dog/git twin of keeper's KEEPObjSha) + the canonical wh128 entry builder.

###  REPACK.h — stream-ingest a git pack into capped logs (KEEP-006, JAB-020)

ONE call repacks a whole fetch: the source is CONSUMED from an fd (a file and a socket are the same thing here), never stored and never seeked. Per record — parse, inflate once, resolve the delta against OUR ALREADY-WRITTEN log, re-emit with the base reference rewritten (OFS when the base landed in this log, REF when in an earlier one) copying the zlib payload verbatim, emit one index entry, rotate at the cap. Input buffer, index region and log dir are the caller's. Read the logs back with `PIDXScanRef` (`dogscan`), never with the OFS-only scan.

 -  `REPACKRun(fd, buf, shard, conf, idx, st)` (`repack_conf`/`repack_stat`/`repack_watch`) — the whole loop; `REPACK_LOG_MAX` = the 2 GiB log cap, `REPACK_MAX_LOGS` bounds the logs one run may open.
 -  `REPACKLogPath(path, shard, id)` — `<shard>/NNNNNNNNNN.keeper`, the store's zero-padded log name; readers name logs with the writer's namer, never by re-deriving the padding.
 -  `REPACKFAIL`/`TORN`/`BIG`/`BASE`/`ROOM`/`LOGS`/`HDR` — io, source ended mid-record, record outgrows buf/cap, base never seen, caller's index full, too many logs, no pack header.
 -  CLIs: `dogrepack <pack|-> <shard> [cap]` writes a shard, `dogscan <shard> [--print]` reads one back (its finder is just the entries the scan has already emitted, sorted by hashlet).

###  PKT.h, CFG.h, SUBS.h, IGNO.h — protocol & config

git pkt-line framing, the gitconfig-family parser, the `.gitmodules` submodule parser, and the `.gitignore` matcher.

 -  `PKTu8sDrain`/`PKTu8sFeed`/`PKTu8sFeedFlush` (`PKTFLUSH`/`PKTDELIM`) — drain/emit one pkt-line.
 -  `CFGu8sFeed`/`CFGu8sDrain` (`CFGstate`) — the gitconfig pull parser: `Feed` advances to the next section/assignment.
 -  `SUBSu8sParse`/`SUBSu8sFind`/`SUBSu8bSynth` (`subs_cb`) — drain `.gitmodules` sections firing `cb(path,url)`.
 -  `IGNOLoad`/`IGNOMatch`/`IGNOFree` (`igno`/`igno_set`/`igno_pat`) — load the hierarchical `.gitignore` chain (anchor dir up to `$HOME`/`/`, crossing sub boundaries), match with git precedence (negate / anchor / dir-only).

###  sha1dc/ — vendored SHA-1 collision detection

`dog/sha1dc/sha1.h` and `ubc_check.h` are the upstream sha1collisiondetection library, vendored. `SHA1.h` builds `SHA1Sum`/`SHA1Open…` on its `SHA1DCInit`/`SHA1DCUpdate`/`SHA1DCFinal`/`SHA1DCSetSafeHash` C API; do not edit, external dependency.

##  Tokenizers (`dog/tok/`)

###  TOK.h — the tokenizer framework & `tok32`

The shared dispatch and the packed token every tokenizer emits. A `tok32` packs a 5-bit syntax tag (`tag - 'A'`), a 1-bit display-scratch custom bit, a 2-bit diff side, and a 24-bit end offset.

 -  `tok32`/`tok32Pack`/`Offset`/`Side`/`Tag`/`SetSide`/`SetCustom`/`tok32Val` (`tok32Z`) — build/decode a token.
 -  `TOKLexer`/`SplitText`/`KnownExt`/`SameLexer`/`TOKExtAt` (`TOKstate`/`TOKcb`) — dispatch by ext, split a blob.

###  KEYW.h, DEF.h, FREE.h, BRCT.h, MDBLK.h — tokenizer infra

The per-language `.c` lexers share these: a keyword-set probe, a definition-marking second pass, a free-text scanner, bracket matching, and the Markdown block-cursor helpers.

 -  `KEYWOpen`/`KEYWHas` (`keyw`) — the 256-slot open-addressing keyword set each language builds once.
 -  `DEFMark` (`DEF_TAG`='N', `CALL_TAG`='C') — second pass over a tok array: enrich tags, run per-language NFA patterns.
 -  `FREELexer`/`FREEu8sFeed` (`FREEstate`) — natural-language scanner (comment bodies, docstrings); also lexes StrictMark inline spans (code->`H`, emph/strike/link->`G`), kept sticky with `F` through the overlay (DOG-006).
 -  `FREECommentFeed`/`FREECommentFeedN` — split a comment delimiter (own `D` token, never parsed) from the StrictMark-scanned body; `N` takes fixed widths (line `//`->2,0; block `/* */`->2,2).
 -  `BRCTMatch`/`BRCTInner`/`BRCTOuter`/`BRCTCheck`/`BRCTDepth` — bracket matching: partner, region, balance, depth.
 -  `MDBLKu8csSkipSpaces`/`SkipIndents`/`Run`/`AllBlank`/`DrainLine` — the slice-cursor Markdown block idioms.

###  Tag alphabet

Tags a `tok32` carries (painted by `dog/THEME.h`): `D` comment, `G` string, `L` number, `H` preproc, `R` keyword, `P` punctuation, `S` default, `W` whitespace, `U` URI. The `DEF` pass adds `N` defined name and `C` call; `F` is a filename/path column. `T` is the StrictMark meta-pair key quad (`Who:`, DOG-026) — one distinct kind per indexable leaf, so an index keys on the tag, not a line regex.

###  Per-language tokenizers — roster

Each `*T.h` is one ragel-generated `<LANG>Lexer` registered in `dog/tok/TOK.c`'s `TOK_TABLE` by extension. Languages (from that table):

 -  Systems: `CT`→C, `CPPT`→C++, `GOT`→Go, `RST`→Rust, `DT`→D, `ZIGT`, `ODNT`, `SOLT`, `VERT`→Verilog, `GLST`→GLSL.
 -  JVM / .NET: `JAT`→Java, `KTT`→Kotlin, `SCLT`→Scala, `CST`→C#, `FSHT`→F#, `CLJT`→Clojure.
 -  Scripting: `PYT`, `JST`, `TST`, `RBT`→Ruby, `PRLT`→Perl, `LUAT`, `PHPT`, `SHT`→Bash, `PWST`, `RT`→R, `JLT`→Julia.
 -  Functional / ML: `HST`→Haskell, `MLT`→OCaml, `ELXT`→Elixir, `ERLT`→Erlang, `NIMT`→Nim, `GLMT`→Gleam, `AGDT`→Agda.
 -  Mobile / app: `SWFT`→Swift, `DARTT`→Dart.
 -  Markup / web: `HTMT`→HTML, `CSST`→CSS, `SCSST`→SCSS, `TYST`→Typst, `LAXT`→LaTeX.
 -  Data / config: `JSONT`→JSON, `YMLT`→YAML, `TOMLT`→TOML, `SQLT`→SQL, `GQLT`→GraphQL, `PRTT`→Protobuf, `HCLT`, `NIXT`.
 -  Build / ops: `CMKT`→CMake, `MAKT`→Makefile, `DKFT`→Docker, `VIMT`→VimL, `FORT`→Fortran, `LLT`→LLVM IR.
 -  Prose: `MDT`→Markdown (two-pass), `MKDT`→StrictMark, `TXTT`→plain text / reStructuredText (the fallback).

To regenerate a lexer: `ragel -C XXT.c.rl -o XXT.rl.c -L`.

To eyeball one: `doghili FILE` (`tok/HILI.cli.c`) reprints the source with every token painted by `dog/THEME.h` and every other token underlined, plus a ruler under each row naming the tag of each char (`.` = more of the same token, lowercase = continued from the previous line). `dogtok FILE` (`tok/TOK.cli.c`) dumps the same stream as `tag<TAB>token` lines instead.

##  Test harness

###  test/TESTBE.h — hermetic-store test setup

`TESTBEmkdtemp`/`TESTBErmrf` create and tear down a `.be`-free scratch dir under `/tmp` so a C test's `HOMEOpen` cwd-walk can never escape into a real `$HOME/.be`; `TESTBEShieldTmp` loudly removes a stray leaked `/tmp/.be`.

[DOG.md]: ./DOG.md
[ULOG.md]: ./ULOG.md
[README.md]: ./README.md
[DEF.md]: ./tok/DEF.md
[BRCT.md]: ./tok/BRCT.md
