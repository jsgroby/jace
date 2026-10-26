# Adaptive time-management experiment

Baseline: main 7de35cdfdd99378916d5c299ab8717ec4d7b2af7. This candidate changes clock allocation only; it does not include the rejected history-penalty or blocked-passer experiments.

For single-PV UCI clock searches, retain the old allocation as the nominal soft budget: remaining time / moves to go (default 30) + 75% of increment, capped by remaining time minus the existing overhead reserve. The hard budget is twice nominal, capped at one quarter of available clock when more than one move remains before the time control. With movestogo 1, it is capped by the full available clock. Minimum budgets remain 1ms. Clock polling retains the existing 256-node interval; the deadline is not a real-time operating-system guarantee.

After a fully completed iteration at depth 4 or later:

- Use 75% of nominal after three consecutive comparisons with the same best move and a score change of at most 20cp.
- Use 150% when the latest best move changes or its score drops by at least 35cp.
- Otherwise use 100%.

Never exceed the hard budget. Soft decisions do not interrupt an iteration; the hard deadline still can, retaining the previous complete result. Aspiration retries must finish before a soft decision. Coefficients are deliberately simple, fixed before match results, and not claimed optimal.

Fixed movetime, MultiPV clock allocation, untimed depth/node searches and console timing retain their existing behavior. Pondering emits no bestmove before ponderhit/stop and starts both soft and hard clocks at ponderhit. Reported UCI search time continues to include pondering work as before. Book handling and root restrictions are unchanged.

## Validation

Strict Clang build and full macOS regression suite passed, including ASan/UBSan, perft, reference evaluation/SEE, UCI, book, console and portable smoke tests. New tests cover soft-decision boundaries, instability extension, hard caps, non-adaptive modes, budget arithmetic over thousands of clock/increment combinations, large integer inputs, both sides' clocks, MultiPV, tiny clocks and adaptive ponderhit.

The fixed-depth comparison on 28 opening/tactical/endgame positions at depths 7/7/9 exactly matched baseline at every completed iteration: scores, PVs, depths, nodes, selective depth and bestmove. This verifies unchanged search behavior on that corpus, not playing strength. Reproduce from the project workspace with:

```sh
python3 jace-adaptive-time/tests/time_equivalence.py --baseline-dir jace-passer-kings --repeats 1 --depth-offset 1
```

## Strength gate

A four-game 5+0.1 pilot precedes a 256-game paired match at 60+1, Hash64, concurrency1, MultiPV1, Book/Ponder off and Move Overhead50. Use 128 fresh unique opening positions, with colors reversed, excluding prior match positions. Review clock forfeits, abnormal endings and legal replay as well as score. A promising result needs a fresh-opening confirmation before promotion. No Elo gain is established and main is unchanged.

The initial 5+0.1 pilot exposed one candidate time forfeit (6ms overrun) after its clock fell to roughly 100ms. The initial twice-nominal cap could spend nearly all of that clock when increment dominated the allocation. The revised quarter-available-clock cap prevents that drain when multiple moves remain. Original pilot artifacts are retained under engine-testing/adaptive-time-20261022/initial-pilot; they are excluded from strength testing. This is a safety correction before the main match, not outcome-based coefficient tuning.
