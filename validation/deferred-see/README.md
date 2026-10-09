# Deferred capture SEE experiment

Baseline: 5b3ca6df1f671afb6fcaaed50345a7da3186fc7e. Measured on macOS ARM64,
2026-10-08, native cc/Apple clang, -O3 -DNDEBUG, one process at a time.
The 28 existing positions cover openings, tactics and endgames. Search depth is
6 for the first 20 positions and 8 for the eight endgames. Hash is cleared for
each position; 64 MB, tapered evaluation on, book/ponder off, MultiPV 1.

Reproduce on macOS or Linux from the repository (baseline commit must be available):

```
python3 tests/ordering_benchmark.py --baseline-ref 5b3ca6df1f671afb6fcaaed50345a7da3186fc7e
python3 tests/ordering_benchmark.py --baseline-ref 5b3ca6df1f671afb6fcaaed50345a7da3186fc7e --no-counters --repeats 9
```

Only search.c is taken from the baseline revision; all other engine files must
match the baseline for this comparison. Timing runs alternate execution order.
The script verifies scores, cumulative nodes, principal variations and best moves
at every reported iteration. All matched in all trials (3,415,798 final nodes).

SEE calls fell from 2,588,858 to 2,427,691 (6.23%). By category: opening 7.75%,
tactical 5.07%, endgame 14.00%. Five instrumented trials are in counted.json.

Nine trials without SEE counters are in timing.json. Median total search time
fell from 2,206 ms to 2,166 ms (1.81%). Category median reductions were 2.15%,
1.64% and 1.24%, respectively. These are short local benchmarks with millisecond
resolution and timing noise, not a universal speed claim or an Elo estimate.

Local strict release build, portable UCI smoke and full ASan/UBSan regression
suite passed, including ordering equivalence for generated/missing hash moves,
promotions, en-passant and castling positions. The candidate contains no futility
pruning change. A time-controlled match remains separate validation.
