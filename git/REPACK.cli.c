//  dogrepack — the CLI driver over REPACKRun (KEEP-006, JAB-020).
//
//    dogrepack <pack-file|-> <shard-dir> [cap]
//
//  Does exactly what the `git.pack` binding does: open the source, map the
//  input buffer and the index region, run once, print the stats.  `-` reads
//  stdin, so a pack piped from `git upload-pack` and a pack file take the
//  same path through the loop.

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "dog/git/REPACK.h"

#define RP_IDX_SLOTS (1ull << 25)   //  33.5 M index entries of VA, lazily faulted
#define RP_BUF_MIN   (1ull << 20)   //  input-buffer floor for small-cap runs

static ok64 rp_progress(void *user, repack_stat const *st) {
    (void)user;
    fprintf(stderr, "  %llu/%llu objects, log %u at %llu bytes\n",
            (unsigned long long)st->objects, (unsigned long long)st->total,
            st->log0 + st->logs - 1, (unsigned long long)st->log_len);
    return OK;
}

ok64 repack_cli() {
    sane($arglen >= 3);
    a$rg(a1, 1);
    a$rg(a2, 2);
    a_cstr(dash, "-");
    int fd = 0;
    if (!u8csEq(a1, dash)) {
        a_path(src, a1);
        call(FILEOpen, &fd, $path(src), O_RDONLY);
    }
    u64 cap = REPACK_LOG_MAX;
    if ($arglen > 3) {
        a$rg(a3, 3);
        a_path(capstr, a3);
        cap = strtoull((char const *)capstr[0], NULL, 10);
    }
    //  Optional first file id: a second ingest into the same shard must not
    //  reopen 0000000000.keeper over the first one's log.
    u32 log0 = 0;
    if ($arglen > 4) {
        a$rg(a4, 4);
        a_path(l0str, a4);
        log0 = (u32)strtoul((char const *)l0str[0], NULL, 10);
    }

    Bu8 buf = {};
    Bwh128 idx = {};
    //  The input buffer must hold one whole record; the cap is the bound
    //  that matters (a record that outgrows a log can't be stored anyway),
    //  but keep a floor so a tiny --cap test still has room to read.
    call(u8bMap, buf, (size_t)(cap > RP_BUF_MIN ? cap : RP_BUF_MIN));
    call(wh128bMap, idx, RP_IDX_SLOTS);
    repack_conf conf = {.cap = cap, .log0 = log0, .every = 100000,
                        .watch = rp_progress, .user = NULL};
    repack_stat st = {};
    a_path(shard, a2);
    try(REPACKRun, fd, buf, $path(shard), &conf, idx, &st);
    fprintf(stderr, "repack: %llu objects (%llu raw, %llu ofs, %llu ref), "
            "%llu index entries, %u logs, %llu bytes\n",
            (unsigned long long)st.objects, (unsigned long long)st.raw,
            (unsigned long long)st.ofs, (unsigned long long)st.ref,
            (unsigned long long)st.index_n, st.logs,
            (unsigned long long)st.out_bytes);
    wh128bUnMap(idx);
    u8bUnMap(buf);
    if (fd > 0) FILEClose(&fd);
    done;
}

MAIN(repack_cli);
