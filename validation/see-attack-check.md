# Direct SEE attack checks (experimental)

Baseline: e118fdf6aa8eaa4d2f4b7668f730873d7f10b743, Jace 1.3 plus Windows
pipe and Ubuntu warning fixes. Only SEE's king-attack query is optimized;
recapture selection, legality policy, material values and search are unchanged.

The new query checks pawn/knight/king offsets and the first occupied square
along each sliding ray, instead of scanning all 64 squares and testing each
opponent piece. Pinned defenders still count as pseudo-attackers for king safety.

## Correctness

Full regression suite including ASan/UBSan passed on macOS. Differential tests
against the frozen prior implementation matched 20,853 candidate captures and
647,040 attack queries across fixtures and seeded legal playouts. Strict Clang
and GCC 16 release builds and GCC portable smoke passed on macOS. The separate
68-position depth-5 comparison matched scores, nodes, PVs and best moves.
Native Windows and Ubuntu checks of this candidate remain outstanding.

## Speed

28 positions: 12 openings and 8 tactical/exchange fixtures at iterative depth 6,
8 endgames at depth 8; 64 MB cleared hash, Book false, TaperedEval true,
MultiPV 1, Ponder false. Five alternating runs, same compiler flags and source
except SEE. Every iterative score, node count, PV and best move matched.

| Category | Median baseline ms | Median candidate ms | Reduction |
|---|---:|---:|---:|
| Openings | 1033 | 767 | 25.8% |
| Tactical | 1344 | 1074 | 20.1% |
| Endgames | 318 | 314 | 1.3% |
| Total | 2693 | 2159 | 19.8% |

Total nodes: 3,415,798 per run. Total speed increase is about 24.7% in nodes/sec.
This small, partly synthetic corpus does not establish a playing-strength gain;
the small endgame timing difference should not be treated as significant.
Reproduce on macOS with `python3 tests/see_benchmark.py` (requires clang).
Outputs go to build/see-benchmark. Core differential tests run via make test.

## Match

A 256-game, 128-opening paired match at 60+1 is planned against the exact
baseline above: hash 64 MB, MultiPV 1, Book/Ponder off, tapered evaluation on,
one game at a time. Keep this branch experimental until results are reviewed.
