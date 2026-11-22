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

## King-pressure experiment (2026-11-22)

The accepted starting point is main ad0a3c5 (PR #35). This experiment freezes all
21 accepted scalar weights and fits only the five names beginning with `King`:
knight, bishop, rook and queen attacks into the enemy king's clipped 3x3 zone,
and coordinated attackers. Hits are collected on the existing mobility rays,
independently of the pawn-safe mobility count. Friendly occupied target squares
and squares beyond blockers are excluded. Pinned attackers still count as
positional pressure; this is not a legal-move or tactical proof.

At least two distinct pieces must attack the zone. Coordination uses
`(min(attackers,4)-1)^2`. Queenless pressure is halved with integer rounding.
The result enters only the middlegame side of tapered evaluation. Existing pawn
shelter is untouched. Zero weights reproduce the accepted evaluation.

`make king-safety-test` compares the implementation against an independent
zone-to-piece ray tracer with zero and nonzero weights, mirrored colors, legacy
evaluation, blockers, board edges, queenless positions and extra attackers.
The test also runs on 4,713 targeted/random legal positions. CI runs it on all
five platforms. Local sanitizer runs use `make MODE=sanitize king-safety-test`.

For broader training samples, prepare.py supports `--allow-captures` and repeated
`--test-source FILE` options. The latter restricts test data to fresh PGN sources;
older test-opening groups are excluded, not reassigned to training. Grouping and
cross-split deduplication remain in effect. Then filter all three partitions:

```sh
make release tuning-tools
python3 tools/tuning/filter_stability.py --input /path/to/raw \
  --output /path/to/filtered --exe build/release/eval-stability
python3 tools/tuning/tune.py --only-prefix King \
  --data /path/to/filtered --output /path/to/fit
```

The stability helper runs baseline quiescence with the five pressure terms
explicitly disabled, keeping samples within 25 cp of baseline static evaluation.
It rejects searches reaching the 10,000-node cap. It does not label positions with
candidate scores or change game outcomes. A node cap is a filtering guard, not a
search setting for the match. Data is still drawn from local engine games and is
not a broad independent chess corpus.

The pilot retains 25,208 training, 3,223 validation, and 605 fresh test positions.
The selected weights are 2/2/2/4 for N/B/R/Q hits and 6 for coordination. Relative
MSE reductions are only about 0.15% validation and 0.03% held out: weak evidence,
not a strength claim. The direct-evaluator benchmark costs about 6.2% more than
the accepted baseline; this is not an estimate of whole-search slowdown.
A fresh paired 60+1 match is required before any promotion.
