# Remaining SEE cost — Jace 1.3

28-position corpus: 12 openings, 8 directed tactical/exchange fixtures, 8 endgames.
Openings/tactics use iterative depth 6; endgames use depth 8 to improve timing
resolution. Depths differ, so totals are workload-specific, not representative
percentages for tournament play. Hash 64 MB, cleared per position, Book false,
TaperedEval true, MultiPV 1, Ponder false. Three uninstrumented runs plus one
instrumented run. Counts and whole-SEE elapsed time are collected only in
separate diagnostic binaries; production source has no profiling counters.

| Category | SEE calls | SEE time / instrumented search | Median ordinary search |
|---|---:|---:|---:|
| opening | 961,462 | 50.2% | 1028 ms |
| tactical | 1,580,737 | 41.8% | 1342 ms |
| endgame | 46,659 | 2.6% | 317 ms |
| all | 2,588,858 | 40.6% | 2692 ms |

All runs preserved every iterative score, node count, PV and best move.
No same-capture duplicate SEE calls were detected within the same node visit.
Across the corpus: 2,588,858 SEE calls, 4,630,743 king-attack checks scanning
284,171,830 squares, 232,259,456 recapture-candidate square visits, and
190,893,137 sliding-ray steps. Maximum reply recursion depth was 8.
437,929 initial candidate captures failed the internal king-safety check.

## Recommendation

First optimize SEE's `Attacked`: replace its full-board enumeration with
attacks tested outward from the king (pawn/knight/king offsets and sliding
rays). Preserve legality checks for pinned pieces, king captures, en passant
and promotion. Then measure and test against the current implementation as
an oracle before any strength match. Recapture enumeration in `Reply` is a
second target. Do not remove legality checks merely to gain speed.

This is a recommendation, not an implemented optimization or an Elo forecast.
The timing includes instrumentation overhead; operation counts establish
repeated work but do not independently assign CPU time to each helper.
The small, partially synthetic corpus is a limitation. Endgame SEE remains
small even at depth 8, so this optimization is primarily aimed at positions
with more pieces and captures.

## Reproduce

On macOS/Linux, run `python3 run.py /absolute/path/to/jace-baseline` from this
folder. It compiles isolated benchmark/profile executables and regenerates
the logs and summary.json. Requires a C11 compiler named cc and Python 3.
