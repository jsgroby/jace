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


UCI conveniences:
- `Clear Hash` is a button that clears the transposition table without changing
  the current position, move history, hash allocation, or other options.
- `Move Overhead` accepts 0–5000 milliseconds (default 50). It is the maximum
  reserve kept on the remaining game clock, capped at 5% of that clock to avoid
  consuming a large fraction of very short time controls. It caps the computed
  move budget at remaining time minus reserve; it is not subtracted from every
  normal move allocation. The default reproduces the previous time allocation.
  Fixed `go movetime`, depth/node limits and infinite analysis are unaffected.
  Pondering applies the reserved budget after `ponderhit`.
- Option names and boolean values are case-insensitive. Invalid option values
  are ignored. Unknown commands and `debug on/off` do not stop UCI searches.
  Debug mode defaults off and reports received commands using `info string`.
