# Nonblocking shared hash-store experiment

Baseline main: cb32a871fe1b641b24511c72967d7a929c725e8c (merged SEE optimization).

Shared StoreHashEntry tries its stripe mutex once and returns without modifying
an entry if acquisition fails. ReadHashEntry still acquires its mutex normally.
Unshared single-thread stores are unchanged. Native try-lock implementations
use pthread_mutex_trylock on Unix and TryEnterCriticalSection on Windows.

This deliberately discards some cache updates. It can change parallel search
results, move ordering and strength. Keep experimental until match results are
reviewed; throughput alone is not proof of a strength improvement.

## Validation

Full macOS ARM64 ASan/UBSan suite, native smoke, SMP ThreadSanitizer and real
Syzygy tests passed. Targeted tests prove a busy store returns without altering
the entry, then verify uncontended and serial stores still succeed. The shared
collision stress test seeds one valid entry before workers start so it tests
snapshot integrity without assuming every attempted write succeeds.

Three alternating single-thread fixed-depth passes over 28 positions matched
every iteration's score, PV, nodes, seldepth and best move exactly.

## Preliminary four-thread benchmark

Apple M1 Max; Hash 64 MB; 12 positions (4 opening, 2 tactical, 6 endgame),
1500 ms per search, five alternating passes. Book/ponder/Syzygy off.
Final aggregate nodes divided by command-to-bestmove wall time:

| Category | Change in median aggregate NPS |
|---|---:|
| Opening | +1.37% |
| Tactical | +2.41% |
| Endgame | +4.73% |
| All | +3.58% |

Candidate throughput was higher in each paired pass. Sum of completed depths
across each 12-position pass: baseline 156,155,153,154,154; candidate
155,155,156,154,156. This is a rough progress indicator, not a strength measure.
All 120 returned moves independently checked legal.

Reproduce from candidate checkout:

```sh
python3 validation/hash-trywrite/benchmark.py --baseline /absolute/path/to/baseline
python3 validation/see-enumeration/benchmark.py --baseline-dir /path/to/baseline-source --repeats 3 --depth-offset 1
```

## Match gate

256-game screen versus current main, four threads each, 60+1, 64 MB hash,
128 fresh reversed-color opening pairs, concurrency one, books/ponder/Syzygy
off. No automatic promotion; a small match cannot guarantee no strength loss.
