# JACE 1.4

This release gives the accepted main baseline a distinct UCI version and
packages it for macOS ARM64, Linux ARM64/x64, and Windows ARM64/x64.
Author: Jeffrey S. Groby. Source foundation: Bluefever Software's VICE.
License: MIT, with upstream notice retained.

## Changes included in the baseline

- Cached SEE work, deferred capture SEE, and pawn evaluation caching.
- Early-exit legal-move probing; the accepted benchmark showed about 8–10%
  less elapsed search time on the measured suites, with matching search results.
- Improved UCI control handling, Clear Hash and Move Overhead options.
- searchmoves, MultiPV and pondering support retained.
- Portable builds, Windows input handling fixes, architecture-specific Windows
  executable names, automated regression checks and packaged builds.
- XBoard removed; UCI, console mode and perft retained.

The release-preparation changes themselves affect version identification,
documentation and packaging, not search or evaluation.

## Evidence and limits

The last accepted legal-probe match completed 256 paired games at 60+1 with
77 wins, 112 draws and 67 losses: 51.95%, estimated +13.58 ±30.12 Elo.
That interval does not establish a playing-strength gain. The merge was
supported by repeatable speed measurements and correctness checks.
See validation/legal-probe and other validation reports for details.
No official CCRL rating is claimed for JACE 1.4.

Before publishing, all five platform jobs must pass on the release tag.
Windows runs portable smoke tests; Unix also runs ASan/UBSan regressions.
Packages include instructions, credits, license, build provenance and checksums.
No opening book is bundled in binary packages or Git source archives. macOS is unsigned; Linux builds target Ubuntu 24.04.

## Publication checklist

1. Review and merge the release-preparation PR after its CI checks pass.
2. Create the immutable tag v1.4 on the approved merge commit.
3. Wait for all five tag builds and the aggregate CI check to pass.
4. Inspect the draft release assets and test extracted packages.
5. Publish the draft release, then send its stable link to interested testers.

CCRL testing is voluntary; a release does not guarantee inclusion or a rating.
