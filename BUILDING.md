# Jace 1.3 baseline

This baseline includes the promoted SEE cache and PV draw-termination fix.
The engine reports `Jace 1.3`, author `Jeffrey S. Groby`.
The executable is `jace-baseline` on macOS/Linux and `jace-baseline.exe` on Windows.
Keep previous engines in separate folders for comparison/rollback.

## macOS and Ubuntu

Use a C11 compiler, GNU Make, and Python 3. From the extracted source folder:

```
make clean
make CC=clang release   # macOS; CC=gcc is suitable on Ubuntu
make smoke             # portable UCI checks
make test              # full Unix suite, including ASan/UBSan
make release           # leave the release executable at the top level
```

`make test` currently uses `cc` for its standalone C tests and Unix pipe APIs.
It is supported on macOS/Linux, not Windows. On Windows, `make test` exits
with a clear explanation and nonzero status before any builds; it does not
report skipped tests as passing. `make smoke` is portable.
Always run `make clean` when switching compiler, architecture, or compiler flags.
The native compiler target determines the operating-system flags and executable suffix.
No architecture-specific instruction flags are required.

## Windows 11 ARM64

Open MSYS2 CLANGARM64 (the prompt/environment must be CLANGARM64):

```
pacman -Syu
```

Reopen CLANGARM64 if the update requests it, complete the update, then:

```
pacman -S --needed make mingw-w64-clang-aarch64-clang mingw-w64-clang-aarch64-python
cd /c/Chess/jace-baseline
make clean
make CC=clang LDFLAGS=-static release
make CC=clang LDFLAGS=-static PYTHON=python smoke
```

Adjust the source directory as needed. `clang --version` should show an
`aarch64` Windows target. Use `make` from MSYS2; the recipes require its shell.
The result is a native ARM64 Windows executable. A direct build also works:

```
clang -std=c11 -O3 -DNDEBUG -D_WIN32_WINNT=0x0600 -Wall -Wextra -Wpedantic *.c -static -o jace-baseline.exe
python tests/portable_smoke.py ./jace-baseline.exe
```

`_WIN32` is predefined by the Windows compiler, including ARM64 targets.
The old manually supplied `-DWIN32` is no longer needed.
Windows sanitizers are not part of this supported build recipe.

## Windows x64 (optional)

Use MSYS2 UCRT64 instead, install `make`, `mingw-w64-ucrt-x86_64-gcc`, and
`mingw-w64-ucrt-x86_64-python`. Use the same Windows commands with `CC=gcc`.
This produces an x64 executable, not a native ARM64 executable.

## Validation status

Local validation has passed on macOS and Ubuntu, including the full regression
suite. Portable smoke tests have also passed on Windows ARM64 and x64.
For a specific revision, check its GitHub Actions results: the workflow tests
macOS ARM64, Linux ARM64 and x64, and Windows ARM64 and x64 separately.
On Ubuntu, `uname -m` reports `aarch64` for ARM64 or `x86_64` for x64;
choose the corresponding Linux package. Building from source uses your native compiler.
A successful build alone is not a strength result or a substitute for match testing.

The portable smoke test checks UCI handshake, a depth-limited search,
searchmoves, MultiPV, readiness during search, stop, pondering and quit.
After it passes on each platform, use a short paired engine match there to
exercise clocks and sustained GUI/runner communication. Compare fixed-depth
scores and moves, not wall-clock speed across dissimilar hardware or VMs.

MSYS2 references:
https://www.msys2.org/docs/arm64/
https://www.msys2.org/docs/environments/

## Automated builds

See [CI.md](CI.md) for GitHub Actions checks, downloadable packages, and draft releases.
