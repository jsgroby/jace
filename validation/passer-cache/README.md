# Cache eligible passed-pawn squares

Baseline: main 7de35cdfdd99378916d5c299ab8717ec4d7b2af7.

Store one combined bitboard of eligible advanced, frontmost passed pawns in each pawn-cache score. Populate it on a cache miss using the existing passed-pawn and doubled-file checks. King-proximity evaluation iterates this bitboard and still computes both king distances on every evaluation. No king-dependent value enters the pawn-only cache. Full white/black pawn-bitboard comparisons continue to reject index collisions.

The candidate must preserve evaluation scores and fixed-depth search results exactly. The independent slow evaluation oracle is unchanged. Regression coverage includes a cache-slot collision between structures with different eligible passers, king-only moves and undo, pawn changes, promotions, en passant, mirror symmetry, both evaluation modes, and seeded random legal games.

On the tested macOS build the pawn cache grows by 64 KiB (4096 entries). A larger entry and returned score structure can offset the saved scans, so measure rather than assume a speedup.

Reproduce the strict search comparison:

    python3 tests/passer_cache_benchmark.py --baseline-dir /path/to/baseline --repeats 7 --depth-offset 1

This requires identical non-evaluation engine sources and asserts every completed iteration's score, node count, depth, selective depth, PV and final best move matches baseline across all repetitions. Timings are workload-dependent; no Elo gain is inferred from them. A deeper comparison uses --depth-offset 2.

Initial 28-position comparison: identical 13,235,914 nodes and all search outputs; median summed time 5,345ms baseline versus 5,299ms candidate (0.86% lower). Deeper timing, evaluation-only timing and cross-platform checks are recorded in the PR as they complete. Do not merge solely on this small timing difference.
