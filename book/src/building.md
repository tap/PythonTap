# Building from source

Requires CMake 3.19+ and a C++20 compiler (Xcode 12+ / Visual Studio 2019+). The build links against the embedded runtime, so install it first (see [Installing](installing.md)).

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release    # externals land in externals/
ctest --test-dir build                  # unit tests (mock kernel, no Max needed)
```

On macOS the build matches the architectures of the installed runtime: a universal `libpython` (installed with `./scripts/install-runtime.sh --universal`, as CI does) gives a **universal** (arm64 + x86_64) external — required for anything you ship — while a plain native install gives a faster native-only build for local iteration. Reinstalling the runtime (native ↔ universal) is picked up by an existing build folder on the next configure. An explicit `-DCMAKE_OSX_ARCHITECTURES=...` overrides the default, but configure refuses one wider than the runtime (the link would fail).

The runtime and packages are pinned in `scripts/runtime.lock` and `scripts/requirements.lock`; to move a pin, run `python3 scripts/update-locks.py` with the new versions (`--help` lists them), review the diff, and commit it.

```sh
./scripts/install-runtime.sh --universal   # universal libpython → universal external (ship this)
./scripts/install-runtime.sh               # native libpython → native external (fast iteration)
```

On Windows, configure with `cmake -S . -B build -A x64`.

## Making a release

Push a tag `vMAJOR.MINOR.PATCH`: `.github/workflows/release.yml` builds and tests on each platform (both Mac architectures on their own runners), assembles the package with `scripts/assemble-package.py` (the platform's externals, help, docs, examples, the runtime, and `licenses/`), zips it with SHA256 checksums, merges the three into one package for every platform (`assemble-package.py --merge`: each runtime in `support/<platform>`, each platform's licenses in `licenses/<platform>`), and attaches everything to a release — published as a pre-release for a 0.x version, and a **draft** to review and publish from 1.0 on. The package version comes from the tag (min reads it from git; the assembly checks they agree). Running the workflow by hand builds the zips as workflow artifacts without a release. It signs and notarizes the Mac packages and signs the Windows binaries when the signing secrets it lists are set, and skips signing, with a warning, when they are not.
