# Early-exit legal-move probe

Baseline: be2787554d943db82d0fc58500f09573af08ed6a (merged UCI improvements).

The prior HasLegalMove generated and scored every pseudo-legal move before
trying them. The candidate shares the same enumeration body with
GenerateAllMoves, but its move sinks try each move immediately and stop at
the first legal move. Full generation keeps its original scores and order.
Pawn-square order is snapshotted because promotion make/unmake can reorder
the pawn list, including after a rejected move. All four promotion choices
are retained in their original order.

## Baseline profile

28 opening, tactical and endgame positions, depths 6/6/8:
1,944,175 probes generated 48,986,496 moves and tried 2,405,350.
That is 25.20 generated versus 1.24 tried per call. Instrumented probe time
was 13.67% of instrumented search time (11.17% opening, 13.42% tactical,
19.81% endgame). Timer/counter overhead affects these percentages; use the
separate uninstrumented trials for performance conclusions.

## Uninstrumented timing

Native macOS ARM64, O3/NDEBUG, 64 MB hash, book/ponder off, MultiPV 1,
tapered evaluation on. Each suite alternates baseline/candidate order.

- Nine depth-6/6/8 trials: median 2,102 ms to 1,897 ms, 9.75% less time.
  3,415,798 nodes per suite.
- Five depth-7/7/9 trials: median 5,868 ms to 5,398 ms, 8.01% less time.
  13,235,887 nodes per suite.
- Deeper categories: opening 9.55%, tactical 8.75%, endgame 5.13% less time.

All iterations matched scores, depths, selective depths, nodes, PVs and best
moves in every trial. This is speed evidence, not an Elo claim.

## Correctness

Strict release build with -Werror and full ASan/UBSan regression suite passed
locally. A frozen pre-change generator verifies move ordering and scores;
4,860 directed/mirrored/seeded-play positions verify legal-probe results and
complete resulting board state, including piece-list order and undo slots.
Cases include mate, stalemate, pins, en passant, castling and promotions.
Existing perft, SEE, quiescence, evaluation, TT, UCI and console tests passed.

## Reproduction

From the repository root:

    make CC=clang CFLAGS=-Werror release
    make CC=clang test
    python3 tests/legal_profile.py --baseline-ref be2787554d943db82d0fc58500f09573af08ed6a
    python3 tests/legal_benchmark.py --baseline-ref be2787554d943db82d0fc58500f09573af08ed6a --repeats 9
    python3 tests/legal_benchmark.py --baseline-ref be2787554d943db82d0fc58500f09573af08ed6a --repeats 5 --depth-offset 1

The scripts write generated binaries/logs under build/. Timing arrays and
baseline profiling totals are preserved alongside this report.
