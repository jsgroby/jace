# Conservative principal-variation LMR experiment

Baseline: main b4430ee5cac63a16b94db41563be07e3f262bad8.

At wide-window nodes (beta - alpha > 1), subtract one ply from the usual late-move reduction. Existing one-ply reductions become zero and existing two-ply reductions become one. Null-window nodes retain their original reductions. Eligibility and full-depth re-search rules are unchanged.

Hypothesis: spending more search on later quiet moves while establishing the principal line may improve tactical reliability enough to offset the additional nodes. This is an experiment, not a verified strength improvement.

Local validation: strict Clang release build and full macOS regression suite passed, including ASan/UBSan, perft, protocol and portable smoke tests.

Fixed-depth benchmark:

    python3 tests/pv_lmr_benchmark.py --baseline-dir /path/to/baseline --repeats 3 --depth-offset 1

The baseline must have identical non-search engine sources. This measures cost and deterministic behavior, not playing strength. Equal-node and repeated equal-time comparisons additionally use 28 benchmark positions and eight history-preserving positions selected from the Sjeng losses. Those eight positions have no independently verified best-move answer key; do not tune repeatedly to them or count them as a tactical pass rate.

Cross-platform CI and strength validation are required before promotion. Keep this PR in draft during the experiment.
