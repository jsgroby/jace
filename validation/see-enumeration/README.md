# SEE attacker enumeration experiment

Baseline: v1.5.1, bcc954ea4ff5c9e0b91fcf99c07fc6fec0357007.
Candidate changes only SEE attacker enumeration, plus regression coverage.

Reply enumerates pawn, knight, king and first occupied slider-ray attackers
from the target square. A local bitboard visits them in ascending square order,
preserving equal-value selection. Enumeration is rebuilt after each exchange,
so newly exposed sliders are included. Existing legality, king-safety,
promotion, en-passant and decline-exchange behavior is unchanged.

## Correctness

macOS ARM64 full regression suite passed with strict warnings and ASan/UBSan.
20,853 capture SEE scores matched the original reference implementation.
647,040 target/side queries compared complete attacker sets with the reference
full-board enumeration; existing king attack queries also passed.
Tests include seeded legal play, pins, promotions, en passant and king captures.
Real Syzygy-table, protocol, perft and search regression tests passed.

## Fixed-depth timing

Apple M1 Max, local compiler, identical -O3 builds. Single thread, Hash 64 MB,
book off, 28 opening/tactical/endgame positions, depths 7/7/9. Seven alternating
baseline/candidate passes. Every search iteration matched score, principal
variation, depth, selective depth and node count; every best move matched.
Each pass searched exactly 13,235,914 nodes per engine.

| Positions | Baseline median ms | Candidate median ms | Time reduction |
|---|---:|---:|---:|
| Opening | 1585 | 1262 | 20.38% |
| Tactical | 2355 | 2094 | 11.08% |
| Endgame | 1513 | 1494 | 1.26% |
| All | 5467 | 4842 | 11.43% |

Overall throughput improvement is approximately 12.91%. Endgame differences
are small relative to timing variation; do not interpret them as a proven gain.
These timings establish a local speed improvement, not an Elo estimate or
proof of exact equivalence for every possible position.

Reproduce from the candidate checkout with a baseline checkout alongside it:

```sh
python3 validation/see-enumeration/benchmark.py --baseline-dir ../jace-baseline --repeats 7 --depth-offset 1
```

Raw output is written under build/time-equivalence-1. Summary is checked in here.

## Match plan

256 games against the v1.5.1 release executable: 128 fresh opening positions,
reversed colors, 60+1, four threads each, Hash 64 MB, MultiPV 1, book/ponder/
Syzygy off, concurrency 1. Keep as an experiment pending completed match review
and cross-platform CI; no strength improvement is claimed yet.
