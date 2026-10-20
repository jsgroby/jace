# Passed-pawn king proximity experiment

Baseline: main b4430ee5cac63a16b94db41563be07e3f262bad8.

Add an endgame-only adjustment for advanced passed pawns, outside the pawn-only cache. For each frontmost passer on relative ranks 4 through 7, compare both kings' Chebyshev distances to the pawn's next square. Multiply enemy distance minus friendly distance by 2, 4, 6 or 8 centipawns respectively, and blend through the existing endgame phase. Pawns on relative ranks 2 and 3 receive no adjustment. Rear doubled pawns are excluded. Classical evaluation and search are unchanged.

This is a positional heuristic, not a pawn-race solver: it does not prove that a king can reach the square through other pieces, and it does not model opposition or move order. Coefficients are deliberately modest and have not been fitted to match results. Strength improvement requires a paired match and, if promising, independent confirmation.

Validation includes an independent slow reference implementation, expected endgame score differences, blocking kings, edge files, doubled and non-passed pawns, absent pawns, mirror symmetry, classical/full-middlegame suppression and king-only make/unmake with unchanged pawn keys. Existing randomized evaluation/cache differential and sanitizer tests remain enabled.

Fixed-depth comparison (not a strength test):

    python3 tests/passer_benchmark.py --baseline-dir /path/to/baseline --repeats 3 --depth-offset 1

The baseline must share all non-evaluation engine source files. Search results may differ between versions; each version must remain deterministic across repeats.

Keep this experiment separate from baseline pending cross-platform CI and match review.
