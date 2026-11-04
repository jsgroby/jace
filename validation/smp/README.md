# Lazy SMP validation

All matches used the same release executable from 6f6f71b (PR27), macOS ARM64,
60+1, 64MB total hash per engine, MultiPV 1, book/ponder/tablebases disabled,
concurrency 1. Each opening was played twice with colors reversed. Independent
confirmation sets excluded positions used in earlier matches.

| Comparison | Games | Wins | Losses | Draws | Score | Fastchess Elo estimate |
|---|---:|---:|---:|---:|---:|---:|
| 2 vs 1 pilot | 256 | 76 | 54 | 126 | 54.30% | +29.93 ±29.70 |
| 2 vs 1 confirmation | 512 | 185 | 74 | 253 | 60.84% | +76.54 ±22.05 |
| 4 vs 2 pilot | 256 | 89 | 45 | 122 | 58.59% | +60.31 ±27.87 |
| 4 vs 2 confirmation | 512 | 172 | 101 | 239 | 56.93% | +48.49 ±21.44 |

All games terminated normally. Independent board replay verified every move,
final result, and reversed-color pair: respectively 36,005, 70,826, 35,829 and
70,976 played plies. Result summaries, replay checks and executable hashes are
stored alongside this report. Full PGNs remain in the local engine-testing
match directories named here; they are not included in this report.

These results support Threads=4 on the tested Mac when CPU capacity is available.
They are relative self-play results, not an absolute CCRL rating. Do not add the
Elo estimates to predict an untested 4-vs-1 result. x64 performance/scaling and
other time controls remain unmeasured; all five platforms pass correctness CI.
The early fixed-time screen had inconsistent serial/parallel final node reporting
and cannot establish a quantitative NPS speedup. Version 1.5 corrects that output;
repeat an isolated benchmark before quoting throughput gains.
