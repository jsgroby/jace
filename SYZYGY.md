# Optional Syzygy endgame tables

JACE includes the MIT-licensed Fathom probing library. No separate Fathom DLL,
shared library, installation or compilation is required. The executable still
runs without any table files. Fathom's full license is in vendor/fathom/LICENSE
and in executable packages as LICENSE-Fathom.txt.

## Configuration

Obtain standard chess Syzygy files separately: `.rtbw` for win/draw/loss and
`.rtbz` for distance to a zeroing move (pawn move or capture). Start with complete
3–5-piece sets, including smaller material classes needed after captures and
promotions. Piece counts include kings. Put them in a directory you can read.

In HIARCS's engine configuration, set **SyzygyPath** to that directory and leave
**SyzygyProbeLimit** at 5. The engine's default path is empty (disabled).
The GUI may group tablebase paths with its shared hash/tablebase settings.

UCI example (replace the path with your own):

```
setoption name SyzygyPath value /path/to/syzygy
setoption name SyzygyProbeLimit value 5
isready
```

Multiple directories use `:` on macOS/Linux and `;` on Windows. No shell quotes
are required around a UCI path containing ordinary spaces. `SyzygyPath` reports
the largest material size discovered; this does not guarantee a complete set.
Set the path to `<empty>` or the probe limit to 0 to disable probing. Limits
0–7 are accepted, though the initial validation fixtures cover three pieces.
Path or limit changes clear JACE's transposition table.

## Behavior

At a covered root, DTZ ranks legal moves while respecting the fifty-move clock.
JACE also checks immediate repetition/draw claims against its own move history.
`searchmoves` restrictions and MultiPV are applied to those ranked moves.
The output has depth 0, one-move PVs and a nonzero `tbhits` count. Win/loss scores
are +/-20000 centipawns, not a prediction of mate distance. Cursed wins and
blessed losses are treated as draws under the fifty-move rule. DTZ-selected
moves may look unnatural because they prioritize preserving the outcome and
progress toward a pawn move or capture.

Root probing requires the relevant DTZ and WDL data. If it fails, JACE searches
normally. During search, WDL probing is conservative: only non-check positions
with halfmove clock zero are probed, after existing draw checks. Missing data,
positions above the probe limit and positions with castling rights fall back
to ordinary search. Table files are read locally, never queried online.

Completed tablebase results still wait for `ponderhit`/`stop` during pondering
and for `stop` during infinite analysis. Root results need not consume the full
`movetime` budget. Cold disk reads can add latency, so use local storage for
clock-sensitive play. JACE remains single-threaded; the Fathom build is configured
accordingly and must be revisited before SMP.

## Validation

`make smoke` always checks missing/disabled table fallback. For full real-file
checks, run `python3 tests/fetch_syzygy.py` first (Windows: `python`), then run
`make smoke` and the normal platform tests. Downloads are small, verified against
fixed Git blob hashes, ignored by Git and excluded from packages. CI performs
these real-file checks on all five supported platforms. The tests cover root
probing, WDL search, restrictions, MultiPV, fifty-move/mate precedence,
promotions, pondering and infinite mode. The fixture source is python-chess:
https://github.com/niklasf/python-chess/tree/master/data/syzygy/regular
