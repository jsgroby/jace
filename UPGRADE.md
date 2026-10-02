Historical notes below; README.md describes the current SEE experiment.

Historical notes follow. README.md describes Jace 1.2 Analysis, its build name,
new features and current validation.

Historical baseline notes follow. README.md describes the current interface
update, executable name, and verification.

This file describes the inherited baseline update. See README.md for the
quiescence experiment, current executable name, validation and benchmark results.

JACE reviewed update
===================

This source tree implements the first prioritized batches from the review of
jace-main.zip. It is a separate working copy; the uploaded source was not edited.

Build and run
-------------

Run `make` to build the release executable `jace`. Run it from this directory to
use the included `performance.bin` opening book. The engine supports UCI, XBoard,
and its existing console interface.

- `make release`: optimized build without debug board checks.
- `make debug`: debug checks and symbols.
- `make debug-sanitize`: debug checks plus AddressSanitizer/UBSan.
- `make test`: builds release/sanitizer variants and runs regression tests.
- `make CFLAGS=-Werror release debug debug-sanitize`: treats warnings as errors.

Each mode has its own build directory. The last explicitly built mode is copied
to `./jace`; `make test` leaves that executable as the release build. A C11
compiler, make, and Python 3 are needed for tests. Tested with Apple clang 21 on
macOS ARM64. Other platforms have not been validated in this update.

Correctness changes
-------------------

- Fixed the out-of-bounds access in CheckBoard and separated release/debug modes.
- Root best moves are independent of TT/PV reconstruction. Interrupted searches
  return a legal fallback, and terminal roots return UCI `bestmove 0000`.
- Bounded aspiration retries, separate mate/infinity values, mate-formatted UCI
  scores, aligned ply limits, and reserved history space for long games.
- Quiescence recognizes checkmate/stalemate, searches every check evasion, and
  includes quiet promotions. Stand-pat and SEE pruning are prohibited in check;
  checking captures and promotions are protected from SEE pruning.
- Replaced SEE with compact-board legal recapture simulation, corrected exchange
  accounting, promotion gains, en passant removal, pins and king safety.
  SEE remains an exchange heuristic, not a complete tactical search.
- Parse FEN halfmove clocks, validate board structure and basic legality, reject
  invalid input without replacing the current board, and guard repetition scans
  when the entire pre-FEN game history is unavailable.
- Repetition keys normalize unavailable/pinned en passant rights. Repetition
  scanning stops at synthetic null moves.
- TT score reuse checks the halfmove clock and reversible-history context; hash
  moves remain reusable. Replacement now ages entries between searches and
  protects deeper same-context bounds from shallower non-exact stores.
- Exact dead-material recognition replaces broad draw assumptions.
- Monotonic 64-bit timing, positive short-time budgets, UCI node limits, buffered
  commands, readiness responses during search, and clean EOF handling.
- XBoard White-mate result, fixed-time handling, increment units, and initialized
  timing/control state corrected.
- Book loading checks allocation/record lengths, closes files, decodes portable
  big-endian records, validates moves, handles castling encoding, and performs
  weighted selection without a fixed 32-move array. Lookup uses binary search.
- Deterministic 64-bit Zobrist key generation and warning cleanup.

Strength-oriented changes
-------------------------

- Capture ordering separates good and losing exchanges; quiet promotions receive
  priority. Counter-moves no longer accidentally outrank the hash move or good
  captures through additive score bonuses.
- LMR excludes checking moves, promotions, and killers. Null-move pruning is
  restricted to null-window nodes and kept away from the fifty-move boundary.
- Quiet cutoff history receives bounded updates; counter-moves learn quiet
  cutoffs. Full continuation history and failed-quiet penalties remain future
  experiments.
- Phase-blended king evaluation, differentiated middlegame/endgame weights for
  mobility, king cover, connected/doubled pawns and passed-pawn refinements.
  Existing common material/PST/file terms are retained in both phases.

The new evaluation is enabled by default. For controlled comparisons, send:

    setoption name TaperedEval value false

This selects the corrected original evaluation with the SAME corrected search,
not the buggy uploaded engine. Setting it to true re-enables the new evaluation.
Changing this option clears the TT. For reproducible search comparisons, also use:

    setoption name Book value false

Validation
----------

Release, debug, and sanitizer builds pass with the project's full warning set
and `-Werror`. Regression tests cover:

- Four reference perft counts: 197281, 97862, 43238, and 9467, with hash and board
  restoration checked after each traversed move.
- Nine directed SEE cases: defended/free captures, a pawn capturing a queen,
  en passant x-rays, capture promotion, recapture promotion, pinned defenders,
  legal king recaptures, and prohibited king recaptures.
- Quiescence mates, stalemates, quiet promotions and mate precedence at the
  fifty-move threshold.
- Halfmove FEN parsing, clock-sensitive TT reuse, TT mate normalization, repetition
  and null-move boundaries, legal/unavailable/pinned EP identity, and depth/history
  capacity boundaries.
- Forty deterministic random legal playouts, each up to 120 plies, with board/hash
  checks, full undo, and evaluation mirror symmetry in both evaluation modes.
- Actual UCI release and sanitizer processes: normal searches, one-node/one-ms
  limits, terminal roots, readiness and batched stop commands, invalid positions
  and moves, and switching evaluations.
- Synthetic book files with 40 matching entries and Polyglot castling.
- Actual XBoard mate-result and fixed-time commands.

No controlled match gauntlet or statistical Elo test has been run. In particular,
the new evaluation weights and selective-search changes are not proven Elo gains.
The stricter TT context policy and exact stalemate checks prioritize correctness
and may cost speed; profile and match-test them before further optimization.

Deferred intentionally: aggressive futility/late-move pruning, additional
null-move verification, NNUE, SMP, and representation rewrites. UCI pondering and
`searchmoves` are not implemented. This is a tested development update, not a
claim of complete protocol compliance or a measured rating increase.
