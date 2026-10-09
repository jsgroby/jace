# Automated builds and packages

The Build and test workflow runs on pull requests, main pushes, version-tag pushes,
and manual dispatch. It does not run strength matches or promote candidates.

| Package | Hosted runner | Validation |
|---|---|---|
| linux-x64 | Ubuntu 24.04 | Strict release build, smoke, full ASan/UBSan regression suite |
| linux-arm64 | Ubuntu 24.04 ARM64 | Strict release build, smoke, full ASan/UBSan regression suite |
| macos-arm64 | macOS 14 ARM64 | Strict release build, smoke, full ASan/UBSan regression suite |
| windows-x64 | Windows 2022, MSYS2 UCRT64 | Strict static release build, portable smoke |
| windows-arm64 | Windows 11 ARM, MSYS2 CLANGARM64 | Strict static release build, portable smoke |

Windows full sanitizer tests remain unsupported. macOS binaries are unsigned and
not notarized. Linux binaries target Ubuntu 24.04's runtime, not every older Linux.
The Polyglot book is not bundled; users may supply their own performance.bin.

After a successful platform job, download its package from the workflow run's
Artifacts section (requires repository access). Artifacts expire after 14 days.
Windows archives contain `jace-windows-arm64.exe` or `jace-windows-x64.exe`;
Unix archives contain `jace-baseline`. Each archive contains the executable, documentation, BUILD.json (source commit,
engine version, target, compiler, and run ID), and SHA256SUMS. Each archive also
has a separate checksum file. Unix tar archives preserve executable permissions.

## Repository setup

Enable GitHub Actions in Settings > Actions > General. Allow the actions used in
build.yml if the repository has an allowlist. The workflow pins action commits;
review and update these periodically. No personal token or extra secret is needed.
Private-repository jobs consume the account's Actions allowance; review billing
and spending limits before enabling recurring runs.

Once the first complete run passes, add the `CI` status check to the rule protecting
main: Settings > Rules > Rulesets (or Branches > branch protection), require pull
requests and status checks, select `CI`, and require branches to be up to date.
Availability/enforcement on private repositories depends on the GitHub plan.
Workflow files alone do not enable branch protection. Do not make the tag-only
Draft release job a required pull-request check.

## Release process

Merge an approved version first, update ENGINE_VERSION and release notes as needed,
then deliberately push a new `v` tag (for example v1.3.1). Do not move an existing
tag. All platforms are rebuilt and tested from the tag before a draft GitHub Release
is created with the packages. Review and publish that draft manually. Re-running
may replace draft assets but refuses to replace a published release.

Tags name packages; BUILD.json also records the actual compiled ENGINE_VERSION.
A public download requires public distribution access; a release in this private
repository remains private. The current tree contains no top-level license file.
Packaging copies LICENSE/COPYING/NOTICE documents when present, but does not invent
licensing terms. Resolve attribution and licensing documentation before public
redistribution. A draft release is not a claim that distribution is cleared.
