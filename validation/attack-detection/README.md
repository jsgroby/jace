# Direct piece-code attack detection

Baseline: main cb32a871fe1b641b24511c72967d7a929c725e8c.

RawSqAttacked selects the attacking side's knight, rook, bishop, queen and
king codes once and compares mailbox occupants directly. This removes repeated
piece-type and color-array lookups. Ray traversal, pawn checks, attack order,
king-safety semantics and scratch-board handling remain unchanged. No skipped
hash stores or changes from the rejected PR #32 are included.

## Evidence

Five baseline profiles (four threads, five seconds each, Apple M1 Max) found
RawSqAttacked at 13.8-14.7% of top-of-stack samples in opening/tactical positions,
and 26.5-29.5% in three endgames. Sample shares are not exact CPU-time estimates.

775,040 raw attack queries matched the original implementation, covering every
target square and both sides in seeded legal play plus 1,000 sparse randomized
scratch mailboxes. The original function remains a test oracle. Full native
ASan/UBSan regression tests passed, including perft, SEE, repetition/en passant,
protocol, console and real Syzygy tests.

## Exact fixed-depth benchmark

Seven alternating baseline/candidate passes, 28 positions, one thread, Hash
64 MB, book off, depths 7 opening/tactical and 9 endgame. Every iteration matched
score, principal variation, depth, selective depth and node count; every final
best move matched. Each engine searched 13,235,914 nodes per pass.

| Category | Baseline median ms | Candidate median ms | Time reduction |
|---|---:|---:|---:|
| Opening | 1250 | 1199 | 4.08% |
| Tactical | 2088 | 1980 | 5.17% |
| Endgame | 1507 | 1411 | 6.37% |
| All | 4868 | 4597 | 5.57% |

Overall throughput gain is about 5.90%. All seven paired total-time comparisons
favored the candidate. This is a local speed result, not an Elo estimate or a
proof covering every possible input.

Reproduce from candidate checkout:

```sh
python3 validation/see-enumeration/benchmark.py --baseline-dir /path/to/baseline-source --repeats 7 --depth-offset 1
```

## Acceptance

Keep this an experiment pending all five platform jobs plus aggregate CI and
a 256-game paired match against current main: 128 fresh openings with reversed
colors, four threads each, 60+1, Hash 64 MB, books/ponder/Syzygy off, concurrency
one. Main and the published release remain unchanged until explicit approval.

Native release smoke and SMP/real-Syzygy ThreadSanitizer checks also passed.
