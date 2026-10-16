# One-ply continuation-history experiment

Baseline: JACE 1.4, commit 323bd241934ae39ce71a6f11d3678095d1322f56.
Candidate is unpromoted; no evaluation, pruning, LMR condition or time-management changes.

A 13 x 64 x 13 x 64 int16 table (1,384,448 bytes) indexes previous
piece/destination and current piece/destination. The previous piece is the
piece now on that square, including after a promotion. Null moves provide no
context. History resets at each search like existing history/counter-moves.
Only ordinary quiet ordering receives the learned score; hash, promotion,
capture, killer and counter-move priorities remain intact. Quiet check evasions
can use the ordering information, but quiescence does not train the table.

On a quiet beta cutoff, bounded depth-squared gravity updates reward that
reply and penalize preceding legal quiet moves actually searched at that node.
Bounds are +/-16384, with update magnitude at most 512. Existing history and
counter-move updates are unchanged. This implementation was written for JACE;
no third-party implementation was copied.

## Local validation

macOS ARM64 strict release build (-Werror) and complete ASan/UBSan regression
suite passed. New tests cover context selection, null moves, promotions,
priority ordering, positive/negative saturation, per-search reset and actual
search training. Existing UCI, perft, make/unmake, SEE and evaluation checks pass.

## Fixed-depth benchmark

Run from this checkout:

```
python3 tests/continuation_benchmark.py --baseline-dir /path/to/jace-v1.4 --repeats 3 --depth-offset 1
```

28 existing opening/tactical/endgame positions, depth 7/7/9, 64 MB hash,
three alternating uninstrumented runs after the regression suite finished.
Each engine's search output was deterministic across runs. Relative to baseline:

- Overall median elapsed time: 5262 -> 5051 ms (4.01% less).
- Nodes: 13,235,887 -> 12,108,730 (8.51% fewer).
- Tactical time: 12.57% less; opening time: 1.64% more; endgame time: 2.68% more.
- Three opening best moves changed; no tactical/endgame best moves changed.

This is a small suite and search behavior differs: these numbers are not a
strength claim or a guaranteed speedup. See benchmark.json for raw timings.

## Playing-strength test

A 256-game paired fastchess match against JACE 1.4 is prepared: 128 unique
opening positions, reversed colors, 60+1, 64 MB hash, MultiPV 1, Book/Ponder
false, TaperedEval true, Move Overhead 50, concurrency 1. Results pending.
An independent confirmation is required before recommending promotion.
