# Playing with JACE

Download and extract the archive for your operating system and CPU from
https://github.com/jsgroby/jace/releases. Install the executable as a **UCI**
engine in your chess GUI. The executable reports `Jace 1.4`, author
`Jeffrey S. Groby`. Select it in the GUI to play or analyze.

## Platform requirements

- macOS: the supplied build is for Apple Silicon (ARM64), built on macOS 14.
  It is unsigned and not notarized. If macOS blocks it, use Apple's normal
  approval flow for a download you trust, or build from source. There is no
  Intel macOS package in this release matrix.
- Linux: choose ARM64 (`uname -m` shows `aarch64`) or x64 (`x86_64`). Packages
  are built on Ubuntu 24.04; older distributions may need a source build.
- Windows: choose ARM64 for native ARM Windows or x64 for Intel/AMD Windows.
  The executables are statically linked by the build workflow; no MSYS2
  development installation is needed to play.

Keep the extracted Unix executable permission; if necessary use
`chmod +x jace-baseline`. The engine has no graphical board of its own.

## Recommended playing settings

- Hash: 64 MB by default. 256 MB is reasonable when memory permits; use the
  same allocation for both engines in a comparison.
- MultiPV: 1 for games; increase it for multiple analysis lines.
- Ponder: false for controlled tests; enable only when your GUI supports it.
- TaperedEval: true.
- Book: false for controlled tests. No book is bundled. For optional book
  use, supply a permitted Polyglot book named `performance.bin` in the engine's
  working directory and enable Book. The working directory is GUI-dependent.

JACE is single-threaded and has no Threads option. No NNUE file or tablebase
is required. GUI-managed books are independent of JACE's Book option.

## UCI conveniences

Clear Hash clears the transposition table without changing position, history,
hash allocation, or other options.

Move Overhead accepts 0–5000 milliseconds, default 50. It reserves at most
that amount of the remaining game clock, capped at 5% of the clock. It caps
the computed budget at remaining time minus reserve; it is not subtracted
from every normal move allocation. Fixed movetime, depth/node limits, and
infinite analysis are unaffected. Pondering applies the budget after ponderhit.

Option names and boolean values are case-insensitive. Invalid option values
are ignored. Unknown commands and debug on/off do not stop searches.
Debug is off by default and reports received commands using info string.

## Verify a download

Each archive has an adjacent `.sha256` checksum, and contains SHA256SUMS
and BUILD.json with the source commit, version, target, compiler, and CI run.
The external checksum verifies the archive; internal checksums verify its files.
For example, use `shasum -a 256 archive.tar.gz` on macOS,
`sha256sum archive.tar.gz` on Linux, or PowerShell
`Get-FileHash archive.zip -Algorithm SHA256` on Windows, then compare the
result with the corresponding `.sha256` file.

See BUILDING.md for source builds and the portable UCI smoke test.
