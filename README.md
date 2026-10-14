# JACE 1.3

GUI connections use UCI. XBoard/WinBoard is no longer supported; configure
your GUI to use UCI. The interactive console (type `jace`) and perft remain available.

Current baseline: promoted SEE caching from Jace 1.2 SEE Test, plus a fix
that stops printed analysis lines at threefold repetition, the fifty-move
threshold, or insufficient material. Both ordinary and restricted/MultiPV
line builders are covered. Search draw policy and evaluation are unchanged.

See BUILDING.md for macOS, Ubuntu, and Windows ARM64 instructions.
Author and version are defined in defs.h as ENGINE_AUTHOR and ENGINE_VERSION.

The SEE candidate scored 71 wins, 124 draws, and 61 losses against Jace 1.2
Analysis in 256 paired games at 60+1. Its speed improvement was verified;
that match did not establish a statistically conclusive Elo improvement.

The baseline retains searchmoves, MultiPV (1–16), pondering, tapered evaluation,
and optional Polyglot book support. For strength comparisons use MultiPV 1,
Ponder false, Book false, TaperedEval true and equal hash allocations.

The full Unix test suite is `make test`; the cross-platform protocol smoke
suite is `make smoke`. Build a release executable again after debug builds.

See validation/ for the draw fix patch, test results and SEE profiling report.
Prior historical development notes are retained in UPGRADE.md.
