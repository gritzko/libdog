#  Capture build metadata into VERSN_BUILD.h, rewriting the file ONLY
#  when a value changes — so a commit recompiles just dog/VERSN.c, not
#  the world.  Run as `cmake -DSRC=.. -DOUT=.. -DFALLBACK_DATE=.. -P`.
#
#  version : `git describe --tags --always --dirty` ("unknown" off-git)
#  hash    : short commit hash                       ("unknown" off-git)
#  date    : the commit's author date (stable per commit, no per-build
#            churn); falls back to FALLBACK_DATE (a configure-time UTC
#            stamp that honors SOURCE_DATE_EPOCH) outside a git checkout.
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${SRC}
    OUTPUT_VARIABLE V OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${SRC}
    OUTPUT_VARIABLE H OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(
    COMMAND git show -s --format=%cI HEAD
    WORKING_DIRECTORY ${SRC}
    OUTPUT_VARIABLE D OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT V)
    set(V "unknown")
endif()
if(NOT H)
    set(H "unknown")
endif()
if(NOT D)
    set(D "${FALLBACK_DATE}")
endif()
set(NEW
"#ifndef DOG_VERSN_BUILD_H
#define DOG_VERSN_BUILD_H
#define VERSN_BUILD_VERSION \"${V}\"
#define VERSN_BUILD_HASH \"${H}\"
#define VERSN_BUILD_DATE \"${D}\"
#endif
")
if(EXISTS ${OUT})
    file(READ ${OUT} OLD)
    if("${OLD}" STREQUAL "${NEW}")
        return()
    endif()
endif()
file(WRITE ${OUT} "${NEW}")
