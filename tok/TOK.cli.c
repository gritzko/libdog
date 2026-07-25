#include "TOK.h"

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

//  Output is buffered by hand: one `u8bFeed` per unescaped run instead
//  of one `fputc` per byte.  Tokens average ~2 bytes, so the win is in
//  not paying a locked stdio call five times per token.
typedef struct {
    int fd;
    u8bp out;
} tokout;

static ok64 TOKOutFlush(tokout *o) {
    sane(o != NULL);
    while (u8bDataLen(o->out) > 0) call(FILEFlushThreshold, o->fd, o->out, 1);
    u8bReset(o->out);
    done;
}

static ok64 TOKOutFeed(tokout *o, u8csc what) {
    sane(o != NULL);
    if (u8csLen(what) > u8bIdleLen(o->out)) call(TOKOutFlush, o);
    if (u8csLen(what) > u8bIdleLen(o->out)) {  // longer than the buffer
        a_dup(u8 const, d, what);
        call(FILEFeedAll, o->fd, d);
        done;
    }
    call(u8bFeed, o->out, what);
    done;
}

//  Fallback for a token too large to fit the buffer worst-case: feed the
//  unescaped runs one slice at a time, flushing as needed.
static ok64 toktok_slow(u8 tag, u8cs tok, void *ctx) {
    tokout *o = (tokout *)ctx;
    sane(o != NULL && $ok(tok));

    u8 head[2] = {tag, '\t'};
    a$(u8 const, h, head);
    call(TOKOutFeed, o, h);

    //  Feed the bytes between escapes as whole runs; only \n \r \t \\
    //  interrupt a run.
    u8c const *p = tok[0];
    u8c const *run = p;
    while (p < tok[1]) {
        u8 esc;
        switch (*p) {
            case '\n': esc = 'n'; break;
            case '\r': esc = 'r'; break;
            case '\t': esc = 't'; break;
            case '\\': esc = '\\'; break;
            default: ++p; continue;
        }
        if (p > run) {
            u8cs pre = {run, p};
            call(TOKOutFeed, o, pre);
        }
        u8 pair[2] = {'\\', esc};
        a$(u8 const, e, pair);
        call(TOKOutFeed, o, e);
        run = ++p;
    }
    if (p > run) {
        u8cs tail = {run, p};
        call(TOKOutFeed, o, tail);
    }

    u8 eol[1] = {'\n'};
    a$(u8 const, n, eol);
    call(TOKOutFeed, o, n);
    done;
}

//  The hot path: one room check per token, then a raw byte loop straight
//  into the idle region.  Escaping at most doubles a token, so 2*len+3
//  bytes of room is always enough; anything larger goes the slow way.
static ok64 toktok_cb(u8 tag, u8cs tok, void *ctx) {
    tokout *o = (tokout *)ctx;
    sane(o != NULL && $ok(tok));

    size_t need = 3 + 2 * (size_t)(tok[1] - tok[0]);
    if (need > u8bIdleLen(o->out)) {
        call(TOKOutFlush, o);
        if (need > u8bIdleLen(o->out)) return toktok_slow(tag, tok, ctx);
    }

    u8 *w0 = *u8bIdle(o->out);
    u8 *w = w0;
    *w++ = tag;
    *w++ = '\t';
    for (u8c const *p = tok[0]; p < tok[1]; ++p) {
        switch (*p) {
            case '\n': *w++ = '\\'; *w++ = 'n'; break;
            case '\r': *w++ = '\\'; *w++ = 'r'; break;
            case '\t': *w++ = '\\'; *w++ = 't'; break;
            case '\\': *w++ = '\\'; *w++ = '\\'; break;
            default: *w++ = *p; break;
        }
    }
    *w++ = '\n';
    call(u8bFed, o->out, (size_t)(w - w0));
    done;
}

static u8 _srcbuf[1 << 24];

ok64 toktok() {
    sane($arglen >= 2);
    a$rg(file, 1);

    a_pad(u8, path, 4096);
    call(u8sFeed, path_idle, file);
    PATHu8bTerm(path);

    // Find the extension
    u8cs ext = {NULL, NULL};
    u8c const *scan = file[1];
    while (scan > file[0]) {
        --scan;
        if (*scan == '.') {
            ext[0] = (u8c*)(scan + 1);
            ext[1] = (u8c*)file[1];
            break;
        }
        if (*scan == '/') break;
    }
    if (ext[0] == NULL) fail(TOKBAD);

    u8 *idle[2] = {_srcbuf, _srcbuf + sizeof(_srcbuf)};
    int fd;
    call(FILEOpen, &fd, $path(path), O_RDONLY);
    u8 *start = idle[0];
    call(FILEdrainall, idle, fd);
    call(FILEClose, &fd);

    aBpad2(u8, out, 1 << 16);
    tokout o = {.fd = STDOUT_FILENO, .out = outbuf};

    TOKstate state = {
        .data = {start, idle[0]},
        .cb = toktok_cb,
        .ctx = &o,
    };
    call(TOKLexer, &state, ext);
    call(TOKOutFlush, &o);
    done;
}

MAIN(toktok);
