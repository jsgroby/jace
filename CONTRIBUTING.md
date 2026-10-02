# Development workflow

The accepted baseline is Jace 1.3. The repository is https://github.com/jsgroby/jace (verified private at import).

Preserve the existing Git history when importing this source. Review changes
against the remote default branch before committing; do not force-push or
replace repository history. Preserve any existing license and repository
configuration unless a specific change is reviewed.

Use main (or the existing default branch) for accepted changes and one topic
branch per fix or experiment. Tag the accepted 1.3 baseline v1.3.0 only after
checking whether that tag already exists. Preserve old baseline commits and
build baseline/candidate executables separately for strength tests.

Before merging engine changes, run the appropriate regression tests described
in BUILDING.md. Keep correctness fixes, portability work and search experiments
separate. Record match conditions, candidate and baseline commit IDs, opening
set checksum, results and uncertainty in a small report. Keep raw match logs
and platform-specific executables outside Git. Small PGN test fixtures belong
in Git; large match output does not.

Build locally on each platform. Never commit credentials, local GitHub auth
configuration, or compiler output. See .gitignore for generated-file exclusions.
