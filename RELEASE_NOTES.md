# JACE 1.5.2

JACE 1.5.2 gives the accepted baseline a distinct version for external testing,
including CCRL submission. Author: Jeffrey S. Groby, United States.
Open source under MIT; based on Bluefever Software's VICE tutorial engine,
with AI-assisted development using OpenAI ChatGPT/Codex. See CREDITS.md.

## Changes since 1.5.1

- Faster SEE attacker enumeration from the exchange square, preserving the
  original equal-value ordering and king-safety checks (PR #31).
- macOS CI moved to the supported macOS 15 ARM runner.
- Updated UCI version identification and release documentation to 1.5.2.

The nonblocking hash-store experiment (PR #32) and direct attack-detection
experiment (PR #33) were closed without merging and are not included.
This release introduces no further search or evaluation changes beyond the
already accepted baseline cb32a871fe1b641b24511c72967d7a929c725e8c.

## Validation and strength

The accepted SEE change matched 20,853 reference capture scores and 647,040
attacker sets. Seven alternating fixed-depth benchmark passes on Apple M1 Max
preserved search scores, principal variations and node counts while reducing
median search time by 11.43%. A 256-game paired match against 1.5.1 at 60+1,
four threads and 64 MB hash scored 76 wins, 56 losses and 124 draws (53.91%;
+27.20 +/-31.63 Elo). All games ended normally and were independently verified.
The interval includes zero: this is not a conclusive strength gain or an
absolute CCRL rating. Evidence is in validation/see-enumeration and PR #31.

JACE remains independently unrated. Earlier local opponent testing suggests
only a very provisional 2000-2200 Elo range; it does not calibrate this release
across thread counts, hardware or time controls.

## Downloads and configuration

Packages target Windows x64/ARM64, Linux x64/ARM64 and macOS ARM64. Choose the
archive matching the test machine; Windows x64 uses jace-windows-x64.exe.
Each archive includes instructions, copyright/license notices, build provenance
and SHA-256 checksums. The staged release executable is smoke-tested before
packaging. macOS binaries are unsigned; Linux builds target Ubuntu 24.04.

Lazy SMP supports Threads 1-4 (default 1), with Hash shared across workers.
For matches use MultiPV=1, Ponder=false, Book=false and TaperedEval=true, and
explicitly set Threads and Hash to the tester's chosen configuration.
MultiPV greater than 1 and console mode search serially. Optional Syzygy WDL/DTZ
probing uses Fathom; set SyzygyPath to separately downloaded table files.
Books and tablebase data are not bundled or required to run the engine.

Source and downloads: https://github.com/jsgroby/jace/releases/tag/v1.5.2
