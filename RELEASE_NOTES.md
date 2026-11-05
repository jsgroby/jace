# JACE 1.5.1 — release preparation

Author: Jeffrey S. Groby. MIT licensed, based on Bluefever Software's VICE.
This candidate adds documented parallel-search and tablebase support to the
published 1.4 baseline. It is not a published release yet.

Version 1.5 was withheld after package verification found a sanitizer binary in
the Mac archive. Version 1.5.1 explicitly packages and smoke-tests the staged
release executable on every platform.

## Changes since 1.4

- Lazy SMP with Threads 1–4, default 1; private search state and synchronized TT.
- Syzygy WDL/DTZ support through Fathom. Tables are external and optional.
- Adaptive time management and passed-pawn king-proximity evaluation.
- Quiet startup while retaining console and perft diagnostics.
- Automatic discovery of local Syzygy test files, with explicit TABLES override.
- Consistent final UCI node/tbhits totals immediately before bestmove for both
  serial and parallel search, including work from interrupted iterations.

Only the final reporting placement and version identification change engine code
in this release-preparation PR; search decisions and default Threads stay intact.
MultiPV >1 and console mode use one search thread. Hash is shared across workers.

## Evidence and limits

The independent 512-game confirmations at 60+1, 64MB hash on macOS ARM64 gave
Threads 2 vs 1: 185 wins, 74 losses, 253 draws (60.84%, +76.54 ±22.05 Elo);
Threads 4 vs 2: 172 wins, 101 losses, 239 draws (56.93%, +48.49 ±21.44 Elo).
All games, moves and color-reversed pairs were verified. See validation/smp.
Use Threads=4 on the tested Mac with available CPU capacity. Default 1 remains
portable. Physical x64 scaling and other time controls remain to be measured.
No absolute CCRL rating or universal speedup is claimed.

## Publication checklist

1. Review and merge this preparation PR after all five platform jobs and CI pass.
2. Tag the approved merge commit v1.5.1 when authorized.
3. Wait for all five tag builds and aggregate CI; inspect the draft release.
4. Verify five platform archives, checksums, version/provenance and extracted
   executable smoke tests before publishing with approval.

Targets: macOS ARM64, Linux ARM64/x64, Windows ARM64/x64. Packages include
instructions, MIT notices (including Fathom), provenance and checksums. No books
or tablebase files are bundled. macOS binaries are unsigned; Linux targets
Ubuntu 24.04. GUI SyzygyPath is separate from build-test table discovery.
