# tap.python

[![build](https://github.com/tap/PythonTap/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/tap/PythonTap/actions/workflows/build.yml)
[![Max 9](https://img.shields.io/badge/Max-9%2B-9cf)](https://cycling74.com/products/max)
[![Python 3.13](https://img.shields.io/badge/Python-3.13-3776AB?logo=python&logoColor=white)](https://www.python.org)
[![platforms](https://img.shields.io/badge/platforms-macOS%20universal%20%7C%20Windows%20x64-lightgrey)](#requirements)
[![license: MIT](https://img.shields.io/badge/license-MIT-green)](License.md)

Write Max objects in Python.

`tap.python~` embeds a CPython interpreter inside a Max external and runs a Python class as an audio object:

- The class's **`process()` method** is called on the audio signal — once per signal vector with numpy arrays, or once per sample with floats.
- The class's **type-annotated attributes** become Max attributes (`@gain 0.5` in the object box, `gain 0.5`, `getgain`, attrui — it all works).
- The class's **public methods** become Max messages, called according to their signatures, with arguments converted according to their type hints.
- The source file is **watched and hot-reloaded** every time you save it, keeping attribute values, so you can live-code DSP with Max running.
- Python's `print()` output and tracebacks land in the **Max console** — and nothing your code does, `sys.exit()` included, can take Max down.

```python
from attrs import define, field

@define
class default:
    gain: float = field(default = 1.0)

    def process(self, x: float) -> float:
        return x * self.gain
```

```
[tap.python~ default]   ← loads python/default.py, exposes a 'gain' attribute
```

## Requirements

- **Max 9** or later (macOS 11.0+ on Intel or Apple Silicon, Windows 10 22H2+ / Windows 11, 64-bit).

## Installation

Download the package for your platform from the [releases page](https://github.com/tap/PythonTap/releases) — the Python runtime (CPython 3.13 with `attrs` and `numpy`) is included:

- `PythonTap-<version>-macos-arm64.zip` — Apple Silicon Macs.
- `PythonTap-<version>-macos-x86_64.zip` — Intel Macs, and Apple Silicon Macs running Max under Rosetta.
- `PythonTap-<version>-windows-x64.zip` — Windows.

Unzip it into your `Documents/Max 9/Packages` folder, restart Max, and open the `tap.python~` help patcher. Each zip has a `.sha256` beside it, and the release's `SHA256SUMS` lists them all. The package's `licenses/` folder holds the license of everything it ships.

Releases are not code-signed yet. On a Mac, a downloaded unsigned external is quarantined and Max will not load it; clear the quarantine once after unzipping:
```sh
xattr -dr com.apple.quarantine ~/Documents/"Max 9"/Packages/PythonTap
```
On Windows, SmartScreen may warn about the download.

To add more Python packages to the bundled runtime:
```sh
./support/bin/python3 -m pip install <package>      # macOS, from the package folder
.\support\python.exe -m pip install <package>       # Windows
```

### From a clone of this repository

Max finds the package only in its `Packages` folder, so clone it there (or clone it anywhere and symlink it in, e.g. `ln -s ~/src/PythonTap ~/Documents/"Max 9"/Packages/PythonTap`), with its submodules:

```sh
cd ~/Documents/"Max 9"/Packages
git clone --recursive https://github.com/tap/PythonTap.git
```

A clone has no runtime: install it into the package's `support` folder (from [python-build-standalone](https://github.com/astral-sh/python-build-standalone)), then build (see [Building from source](#building-from-source)):

**macOS** — in Terminal, from the package root:
```sh
./scripts/install-runtime.sh
```

**Windows** — in PowerShell, from the package root:
```powershell
powershell -ExecutionPolicy Bypass -File scripts\install-runtime.ps1
```

The script verifies the download against the SHA256 committed in `scripts/runtime.lock`, and installs the Python packages used by the examples (`attrs`, `numpy`) at the versions and hashes pinned in `scripts/requirements.lock`. Re-running it replaces the runtime safely: the previous one is kept until the new one is complete and restored if anything fails — but packages you added yourself are not carried over.

Without a runtime the object still loads, and says in the Max console what is missing. On a Mac, restart Max after installing the runtime; on Windows the next `tap.python~` you create picks it up.

## Writing a class

Python sources live in the package's `python` folder. `[tap.python~ name]` loads `python/name.py` and instantiates the class `name` defined in it (the file, class, and argument must share the same name, which must be a valid Python identifier; with no argument, `default` is loaded). The file is loaded by its path, never through `import`, so a file named like a standard-library module (`random.py`, `json.py`) works and does not shadow that module for anyone else. Other files in the `python` folder can be imported by your class as helper modules (the folder is on `sys.path`, after the standard library).

- **Attributes** — class-level annotated fields (e.g. via `attrs`) become Max attributes. `int` maps to a Max `long`, `float` to `float64`, `bool` to an on/off `long` (your field receives a real `bool`), anything else to a symbol; `Optional[X]` and `X | None` count as `X`. Names starting with `_` are private, and `ClassVar`s are not fields. If a hint cannot be resolved (say a name imported only under `TYPE_CHECKING`), the console says so and the annotation is read as written.
- **Messages** — public methods (including classmethods and staticmethods) become Max messages, called according to their signature: parameters with defaults are optional, `*args` takes any number of arguments, and a keyword-only parameter must have a default (a method with a required keyword-only parameter is not exposed). Argument hints (`int`, `float`, `bool`, `str`) drive the conversion from Max atoms; an unannotated parameter receives the atom as it is (an `int`, `float` or `str`). Methods named `int`, `float`, `symbol`, and `bang` map to those standard Max messages. Names Max or the object handle themselves (`filechanged`, `dsp64`, `notify`, `assist`, `loadbang`, `dblclick`, `anything`, …) are not exposed; the console names any method skipped this way so you can rename it.
- **Audio** — a method `process()` runs on the signal, in one of two forms chosen by its type hint:
  - `process(self, x: np.ndarray) -> np.ndarray` is called **once per signal vector** with a numpy array of the input and must return an array of the same length (any numeric dtype, or a list; it is converted). This is the form to use for anything that must run in real time — see `python/numpy_gain.py`. The input array is reused from one call to the next: copy it if you want to keep it.
  - `process(self, x: float) -> float` is called **once per sample** — simplest for sketching, and expensive.

  One signal inlet and one signal outlet, single-channel; wrap the object in `mc.` for multichannel. (Returning a tuple for multiple outputs is not supported yet.) Output that is not a number, or not finite (NaN, infinity), is replaced with 0.0 and reported once in the Max console.
- **Audio settings** — an optional method `prepare(self, sample_rate: float, vector_size: int) -> None` is called with Max's sample rate and vector size before the object processes any audio, again whenever they change, and on every reload before the new code runs. `python/allpass.py` uses it to size its delay line.
- **Hot reload** — saving the `.py` file reloads it in place, as a fresh module (names you deleted from the file are gone): the attributes and messages follow the new class, and audio resumes with the new code. Until the new code is ready the object keeps running the old one, so a successful reload swaps in without a gap in the audio. Attribute values carry over — set from the patcher or by your own code — for every attribute the new class still has with the same type; an attribute whose type changed starts from its new default, and one you removed disappears from the object. If the file has an error, the object prints the traceback to the Max console and outputs silence until the next successful reload (and the attribute values come back with it). Every object using the same file shares one execution of it per save.
- **Errors** — an exception raised by your code (in `process()`, a message, an attribute setter, the constructor or at import) prints its traceback to the Max console; it never takes Max down. That includes `sys.exit()`, which is reported like any other exception rather than quitting Max.

### A note on performance

`process()` runs on the audio thread, holding Python's global interpreter lock. The per-sample form (`x: float`) makes one Python call per sample — fantastic for sketching and live-coding an algorithm, and expensive: expect meaningful CPU load at high sample rates. The numpy form (`x: np.ndarray`) makes one call per signal vector and is the one to use when it has to keep up. Either way the audio thread waits while other Python code holds the interpreter — a reload, a message, or another instance: all `tap.python~` instances share one interpreter, so heavy Python work in one can steal time from the others. Errors in `process()` are printed from Max's main thread, never from the audio thread.

## Building from source

Requires CMake 3.19+ and a C++20 compiler (Xcode 12+ / Visual Studio 2019+). The build links against the embedded runtime, so install it first (see [Installation](#installation)).

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

### Making a release

Push a tag `vMAJOR.MINOR.PATCH`: `.github/workflows/release.yml` builds and tests on each platform (both Mac architectures on their own runners), assembles the package with `scripts/assemble-package.py` (the platform's externals, help, docs, examples, the runtime, and `licenses/`), zips it with SHA256 checksums, and attaches everything to a **draft** release to review and publish. The package version comes from the tag (min reads it from git; the assembly checks they agree). Running the workflow by hand builds the zips as workflow artifacts without a release. It signs and notarizes the Mac packages and signs the Windows binaries when the signing secrets it lists are set, and skips signing, with a warning, when they are not.

### The core, and testing it on Linux

Everything that talks to CPython — starting the interpreter, loading and reloading the class, describing its attributes and messages, converting values, and running `process()` — lives in a host-independent core under `core/include/tap/python/` (plain C++20 + CPython, no Max). The external in `source/projects/` is a thin layer that maps the core onto Max. The core builds and tests on any platform with a CPython 3.13 — no Max, no runtime install:

```sh
cmake -S core -B build-core -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

The example tests need `attrs` and `numpy` importable by that interpreter (or in a folder named by `TAP_PYTHON_TEST_SITE`); without them they are skipped. `-DTAP_PYTHON_SANITIZE=address,undefined` or `=thread` builds the battery under sanitizers, as CI does.

The external and its mock-kernel unit test also build on Linux (Max does not run there, but its glue does), embedding the same CPython instead of a `support/` runtime:

```sh
cmake -S . -B build-linux -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-linux
ctest --test-dir build-linux --output-on-failure
```

## License

MIT — see [License.md](License.md), which also lists the third-party components (Min-API, CPython, attrs, NumPy) and where their licenses live.
