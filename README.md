# JACE 1.4

JACE is a single-threaded UCI chess engine developed by Jeffrey S. Groby,
based on Bluefever Software's VICE tutorial engine. It needs a chess GUI
(such as HIARCS Chess Explorer) to provide a graphical board.

## Download and play

Published binaries are available on the [Releases page](https://github.com/jsgroby/jace/releases)
when a release has been published. Choose the package matching your operating
system and CPU. See [PLAYING.md](PLAYING.md) for installation, options, and
platform requirements. A standalone executable is sufficient; no opening book,
neural-network file, or tablebase is required.

| Platform | Executable |
|---|---|
| macOS ARM64 (Apple Silicon) | `jace-baseline` |
| Linux ARM64 or x64 (separate packages) | `jace-baseline` |
| Windows ARM64 | `jace-windows-arm64.exe` |
| Windows x64 | `jace-windows-x64.exe` |

For source builds and tests, see [BUILDING.md](BUILDING.md).
For automated packages and the release process, see [CI.md](CI.md).

## Engine features

- 120-square mailbox board with pawn bitboards.
- Iterative-deepening alpha-beta search, principal variation search,
  aspiration windows, quiescence, null-move pruning, and late move reductions.
- Transposition table; capture ordering using MVV/LVA and static exchange
  evaluation (SEE); killer, history, and counter-move ordering.
- Tapered evaluation, pawn evaluation caching, and cached SEE work.
- Early-exit legal-move probing for terminal-position checks.
- UCI `searchmoves`, MultiPV (1–16), pondering, adjustable hash,
  Clear Hash, and Move Overhead.
- Optional Polyglot opening books; console mode and perft diagnostics.

JACE searches on one CPU thread. XBoard/WinBoard is not supported.
For engine comparisons, use MultiPV 1, Ponder false, Book false,
TaperedEval true, and equal hash allocations for both engines.

## Release and validation

See [RELEASE_NOTES.md](RELEASE_NOTES.md) for the 1.4 release summary and
[validation/](validation/) for regression and performance evidence.
This release packages the accepted baseline; it does not introduce a new
search or evaluation experiment. Benchmark speedups and short self-play
matches are not an established CCRL rating.

## Credits and license

JACE is distributed under the [MIT license](LICENSE), retaining Bluefever
Software's copyright notice. See [CREDITS.md](CREDITS.md) for source origins
and Polyglot attribution. Historical development notes remain in UPGRADE.md.
Author and version are defined in defs.h as ENGINE_AUTHOR and ENGINE_VERSION.
