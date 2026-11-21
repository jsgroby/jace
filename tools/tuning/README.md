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

## Staged follow-up

1. Test existing-weight tuning against v1.5.2.
2. Add an explicit king-ring attack/coordination feature separately, with zero
   default contribution and independent tests for mirrors, blockers and phase.
   Preserve the existing shelter term and avoid counting it twice.
3. Use training/validation for fitting the new feature; reserve fresh held-out
   data for its final assessment. The present quiet-only dataset needs broader
   king-attack coverage before making claims about that feature.
4. Match the king-safety candidate against the accepted result of stage 1, then
   confirm any gain on fresh openings and against other opponents.

The earlier Sjeng loss review found search-horizon sensitivity as well as king
exposure. King safety remains an experiment, not a proven cure for those losses.
