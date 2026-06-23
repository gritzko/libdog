#  Capture build metadata into VERSN_BUILD.h, rewriting the file ONLY
#  when a value changes — so a commit recompiles just dog/VERSN.c, not
#  the world.  Run as `cmake -DSRC=.. -DOUT=.. -DFALLBACK_DATE=.. -P`.
#
#  version : `git describe --tags --always --dirty`, else the beagle tip
#            hashlet (`be log`) when this tree is a `.be` store not a git
#            checkout; "unknown" when neither is available.
#  hash    : short commit hash (git), else the same beagle tip hashlet.
#  date    : the commit's author date (stable per commit, no per-build
#            churn); falls back to FALLBACK_DATE (a configure-time UTC
#            stamp that honors SOURCE_DATE_EPOCH) off a git checkout.
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
#  be fallback: a beagle `.be` store has no `.git`, so the probes above
#  are empty.  Read the current tip from `be log --plain` at the worktree
#  root (dog/'s parent); its first 6+hex token is the tip hashlet.  Prefer
#  the build's own `be` (BEBIN, present on an incremental build), else
#  `be` on PATH; with neither, the value just stays "unknown".  Beagle is
#  tagless, so version == the hashlet; the date column is relative, so the
#  date stays FALLBACK_DATE.
if(NOT H)
    set(_be "${BEBIN}")
    if(NOT EXISTS "${_be}")
        set(_be "be")
    endif()
    get_filename_component(_wt "${SRC}" DIRECTORY)
    execute_process(
        COMMAND "${_be}" log --plain
        WORKING_DIRECTORY "${_wt}"
        OUTPUT_VARIABLE _log OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string(REGEX MATCH "[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]+"
           _hl "${_log}")
    if(_hl)
        set(H "${_hl}")
        set(V "${_hl}")
    endif()
endif()
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
