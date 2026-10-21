# Direct passed-pawn blockade experiment

Baseline: main 7de35cdfdd99378916d5c299ab8717ec4d7b2af7.

An advanced frontmost passed pawn receives a modest endgame discount when an opposing non-king piece occupies its next square. The discount is one quarter of the existing rank-based passer bonus (integer centipawns: 5/8/15/25 on relative ranks 4/5/6/7), blended through the existing endgame phase. King blockades and friendly obstructions are excluded; king proximity remains unchanged. Rear doubled pawns, low-rank pawns and non-passers are excluded. The term runs outside the pawn-only cache, within the existing passer loop. Search and classical evaluation are unchanged.

This tests a positional hypothesis, not a solved rule about blockades. An enemy piece can be exchanged or displaced, and this heuristic does not prove permanence. The small coefficient is not fitted to the test games.

A descriptive audit of the two recent king-proximity matches found 53,132 current-candidate positions, with 1,600 occurrences of non-king blockaders ahead of advanced frontmost passers (441 knights, 569 bishops, 418 rooks, 172 queens), plus 623 king blockades. Occurrences are not unique games and include won/drawn positions. Twelve adjacent score-drop examples were retained; these do not establish causality. For example, confirmation game 347 had a white c6 passer blocked by a knight on c7, with reported score falling +1.12 to -0.43. Other examples involve repetition/fifty-move horizons and are not evidence that a blockade penalty solves them.

Validation includes an independent slow board-scan penalty oracle, all four non-king blockader types, king/friendly exclusions, rear doubled pawns, non-passers, early ranks, promotion, color symmetry, classical/full-phase suppression and moving/undoing a blockader on unchanged pawn structure. Existing randomized evaluation and cache differential tests remain enabled.

Fixed-depth comparison:

    python3 tests/blocked_passer_benchmark.py --baseline-dir /path/to/baseline --repeats 3 --depth-offset 1

Results may differ between engines, but each must be deterministic across repeats. These benchmarks and hand-selected examples cannot establish playing strength. Keep this experiment in draft until CI and a paired match are reviewed.
