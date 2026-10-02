# Jace 1.3 validation summary

Verified on macOS Apple Silicon before import:
- Release, debug, and ASan/UBSan builds with -Werror.
- Full core, perft, quiescence differential, SEE, TT, book, UCI and XBoard tests.
- searchmoves, MultiPV and pondering tests on release and sanitizer builds.
- Portable protocol smoke test.
- Identical scores, node counts, PVs and best moves on 68 depth-5 positions
  compared with Jace 1.2 SEE Test.
- Four-game Fastchess integration check: all normal terminations, no warnings.

The promoted SEE-cache candidate scored 71 wins, 124 draws and 61 losses
against Jace 1.2 Analysis over 128 unique opening pairs (256 games), 60+1,
64 MB hash, MultiPV 1, Ponder false, Book false, TaperedEval true, concurrency 1.
Fastchess estimated +13.58 Elo +/-30.60; an Elo gain was not established.

Windows ARM64 and Ubuntu native verification of 1.3 remains outstanding.
Raw machine-specific logs and match PGNs are retained outside this repository.
See BUILDING.md for repeatable tests and see-profile/REPORT.md for profiling.
