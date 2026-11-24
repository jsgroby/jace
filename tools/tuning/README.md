# Offline evaluation tuning

This tool tunes existing evaluation weights using the actual C evaluator and
integer rounding. It adds no runtime UCI options and no mutable production state.
The first experiment exposes 21 scalar weights: isolated/doubled/connected pawns,
open files, bishop pair, pawn shield and piece mobility. Piece values, piece-square
tables, passed-pawn ranks, search and king proximity remain fixed in this pilot.

Build the standalone tools in release mode:

```sh
make CFLAGS=-Werror release tuning-tools
python3 tools/tuning/test.py
python3 -m venv .venv-tuning
.venv-tuning/bin/pip install -r tools/tuning/requirements.txt
.venv-tuning/bin/python tools/tuning/prepare.py --output /path/to/new-data /path/to/games.pgn
python3 tools/tuning/tune.py --data /path/to/new-data --output /path/to/new-result
```

On Windows use the appropriate Python executable and pass `--exe
build/windows-x64/release/eval-tune.exe` (or windows-arm64) to tune.py.
The Python scripts are offline tools; installed engines need neither Python nor
the training dataset. Do not feed arbitrary unvalidated FENs to the C helper:
prepare.py produces legal positions from replayed PGNs.

## Data and measurement

Only normally terminated games with results are included. Sampled positions must
be beyond ply 23, have more than six pieces, and have no legal captures, promotions
or check. Sampling is deterministic with at most eight positions per game and at
least eight plies between samples. This deliberately conservative quiet-position
filter reduces tactical noise, but it can underrepresent tactical/king-attack
positions. Improve dataset coverage before expanding the parameter count greatly.

Games with the same opening position are assigned together to train/validation/
test (80/10/10). Repeated games are removed; repeated positions are reduced to one
observation, and positions occurring across partitions are removed altogether.
Manifests record source and output hashes. Outcomes are White's result (0/0.5/1).
The tool converts side-to-move evaluation to White's perspective.

Calibrate logistic K on training only. Coordinate descent fits the training set
with bounded weights and a fixed weak penalty for moving far from the baseline.
A predetermined six-sweep schedule is used; validation chooses the checkpoint.
The held-out test is evaluated only after that choice. Do not use its result to
pick another configuration or repeatedly tune against the same test set.

No pawn cache is shared between weight requests: the helper evaluates without a
cache. Production cache equivalence remains covered by tests/core.c's independent
uncached implementation using the same weight file.

A lower prediction error is NOT an Elo result. Build the generated eval_params.def
in an isolated candidate, run correctness tests, and play paired fresh-opening
matches against the frozen release. Do not change main or replace a release
based only on fitted loss. Archive dataset manifest, starting weights, evaluator
hash, tuning output, candidate hash and match command together.

## Passed-pawn phase/rank experiment (2026-11-24)

Start from accepted main ad0a3c5 / PR #35. Rejected king-pressure and placement
experiments are excluded. Keep the 21 accepted scalar weights, piece-square
tables, material values, search and passed-pawn king proximity fixed.

Fit twelve bonuses: relative ranks 2..7 for middlegame and endgame. Initial values
exactly reproduce the old tapered score: MG [3,5,10,18,30,50], EG
[5,10,20,35,60,100]. The old odd-value rounding is preserved (e.g. 5-5/2=3).
Only the frontmost pawn on a doubled file receives the tapered passer bonus.
Classical evaluation retains the original PawnPassed array; no new blocker
penalty or passer-eligibility rule is introduced. Scores remain in the pawn-only
cache because both bonus sequences depend only on pawn placement.

Use `--only-prefix Pass --monotonic-prefix PassMG --monotonic-prefix PassEG` to
freeze the accepted weights and constrain each rank sequence to be nondecreasing.
This is a small bounded fit, not a claim that the optimizer finds a global optimum.
Normal builds use immutable tables; only the standalone helper has mutable weights.

The independent uncached reference and test_passers.py check phase, rank, color,
front/rear doubled pawns and opposing-pawn blockage. The helper compares compiled
and mutable evaluation; core tests continue to check pawn-cache equivalence,
mirrors and classical evaluation.

Supporting data tools permit captures followed by a frozen-baseline quiescence
filter (25 cp threshold, 10,000-node cap), full-opening holdouts, and older PGNs
without termination tags only when final-board replay proves their results.
Those offline tools do not add the rejected engine evaluation terms.

```sh
make CFLAGS=-Werror release tuning-test
# Prepare PGNs in the pinned python-chess environment, reserving all fresh groups.
.venv-tuning/bin/python tools/tuning/prepare.py --allow-captures \
  --verify-missing-termination --holdout-pgn /path/to/fresh.pgn \
  --test-source /path/to/fresh.pgn --output /path/to/raw \
  /path/to/older.pgn /path/to/fresh.pgn
python3 tools/tuning/filter_stability.py --input /path/to/raw \
  --output /path/to/data --exe build/release/eval-stability
python3 tools/tuning/tune.py --only-prefix Pass \
  --monotonic-prefix PassMG --monotonic-prefix PassEG \
  --data /path/to/data --output /path/to/fit
```

The accepted order is placement (PR #37 closed without merging after a flat
confirmation), passed-pawn tuning (current), then search diagnostics based on
missed tactics. Keep each candidate separate and require match evidence before
promotion. Local game outcomes and prediction loss are not an absolute Elo rating.
