# Pawn-evaluation cache experiment

Baseline: 18cd16bf76acdde576bd71b361c782ae3822d64c. Measurements: macOS ARM64,
2026-10-09, native cc/Apple clang, -O3 -DNDEBUG, sequential execution.

The fixed 4,096-entry direct-mapped table occupies approximately 128 KiB. Entries
store both complete pawn bitboards and base/middlegame/endgame pawn scores. Full
key comparisons prevent index collisions from returning another structure's score.
Zero initialization is sufficient; no allocation or incremental pawn hash is needed.
The table may persist across searches, positions and evaluation-mode changes since
its contents depend only on the pawn arrangement and fixed evaluation constants.
King shelter, mobility, material, phase blending and side-to-move are not cached.
Like the existing search state, this table is single-threaded; future SMP must use
thread-local tables or synchronization rather than sharing these writes unsafely.

Reproduce on macOS/Linux (the baseline commit must be available):

```
python3 tests/pawn_benchmark.py --baseline-ref 18cd16bf76acdde576bd71b361c782ae3822d64c --repeats 1
python3 tests/pawn_benchmark.py --baseline-ref 18cd16bf76acdde576bd71b361c782ae3822d64c --no-counters --repeats 9
```

Only evaluate.c is taken from the baseline; all other engine files must match.
The same 28 opening/tactical/endgame positions used for prior optimization tests
are searched at depths 6/6/8, with 64 MB hash, tapered evaluation on, MultiPV 1,
book/ponder off. Each process starts with an empty pawn cache; it persists across
positions within that process. The transposition table is cleared per position.
Timing order alternates between baseline and candidate.

Every iteration matched scores, nodes, PV and best move in every trial; total
final nodes were 3,415,798. Counters found 2,399,460 pawn-cache lookups and
2,316,457 hits (96.54%), leaving 83,003 pawn-score recomputations. Hit rates were
91.19% opening, 97.56% tactical and 99.54% endgame. See profile.json.

Nine uninstrumented trials reduced median total search time from 2,124 to 2,041 ms
(3.91%). Category median reductions: opening 4.41%, tactical 4.44%, endgame 0.63%.
See timing.json for all samples. These short local timings are not a universal
speed claim or an Elo estimate. No time-controlled match has yet been run.

The regression suite compares against the frozen pre-cache evaluator, including
both TaperedEval settings during seeded random legal play, mirrored positions,
make/unmake, pawnless positions, promotions, en-passant and deliberately colliding
pawn structures with different scores. The reference is tests/evaluate_reference.inc.
