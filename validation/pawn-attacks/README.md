# Pawn attack maps for evaluation

Baseline: dbfe5a6d3eeffa93cb26a6bced420333ca951cf6. Native macOS ARM64,
2026-10-13, cc/Apple clang, -O3 -DNDEBUG, sequential alternating benchmark order.

Compute one white and one black pawn-attack bitboard per tapered evaluation.
Mobility uses a membership test instead of two mailbox pawn lookups for every
reachable square. Source-file masks prevent a/h-file wraparound. Maps are local
to evaluation; the existing pawn cache and its entry size are unchanged. The
classical evaluation path does not calculate these unused maps. This experiment
did not benchmark storing maps inside the pawn cache.

Reproduce with the existing evaluation benchmark (only evaluate.c may differ
between baseline and candidate engine sources):

```
python3 tests/pawn_benchmark.py --baseline-ref dbfe5a6d3eeffa93cb26a6bced420333ca951cf6 --no-counters --repeats 9
python3 tests/pawn_benchmark.py --baseline-ref dbfe5a6d3eeffa93cb26a6bced420333ca951cf6 --no-counters --repeats 5 --depth-offset 1
```

All scores, nodes, PVs and best moves matched at every iteration in every run.
28 positions, Hash 64 MB, TaperedEval on, MultiPV 1, book and ponder off.
The transposition table is cleared per position; pawn cache persists within each
process as in the previous benchmark.

| Depths (opening/tactical/endgame) | Trials per build | Baseline median | Candidate median | Time reduction | Nodes |
|---|---:|---:|---:|---:|---:|
| 6/6/8 | 9 | 2,090 ms | 2,056 ms | 1.63% | 3,415,798 |
| 7/7/9 | 5 | 5,828 ms | 5,736 ms | 1.58% | 13,235,887 |

Raw sample arrays are in timing-depth6-8.json and timing-depth7-9.json.
Deeper category reductions: opening 2.19%, tactical 0.37%, endgame 1.44%.
These are small local timing gains, not a universal speed claim or Elo estimate.

Strict release build, portable smoke, and full ASan/UBSan regression tests passed
locally. Existing frozen-evaluator comparisons cover both evaluation modes and
random legal positions; added edge-file fixtures exercise mobility around pawn
attacks without wraparound, including mirrored positions and make/unmake.
No timed strength match has been run for this candidate.
