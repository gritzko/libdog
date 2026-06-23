#ifndef DOG_VERSN_H
#define DOG_VERSN_H

#include "abc/S.h"   // u8cs / u8s / u8csc, U8SFMT; ok64 via 01.h

//  Build metadata captured at compile time.  The three values are
//  filled by dog/VERSN.c from the generated VERSN_BUILD.h (`git
//  describe` / short hash / source date — see dog/version.cmake), or
//  read "unknown" when the tree was built outside a git checkout.
//  Only VERSN.c recompiles when a value changes (the generated header
//  is rewritten only when it differs); the rest of the tree is untouched.
//
//  ABC consumers feed/compare the slices directly (no strlen); a dog's
//  MAIN prints the one-line banner with VERSNReport on `--version`.

extern u8 const *VERSNVersion[2];  // `git describe --tags --always --dirty`
extern u8 const *VERSNHash[2];     // short commit hash (or "unknown")
extern u8 const *VERSNDate[2];     // source date, ISO-8601 UTC

//  Feed the one-line banner `<prog> <version> (<hash>) <date>` into
//  `into` (an empty `prog` is skipped).  All-or-nothing per component:
//  propagates SNOROOM from the underlying feed if it won't fit.
ok64 VERSNu8sFeed(u8s into, u8csc prog);

//  Print that banner to stdout — the `--version` convenience for every
//  dog's MAIN.  Always OK.
ok64 VERSNReport(u8csc prog);

#endif
