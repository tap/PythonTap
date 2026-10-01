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
- Python's `print()` output and tracebacks land in the **Max console** — and no exception your code raises, `sys.exit()` included, can take Max down.

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

Download a package from the [releases page](https://github.com/tap/PythonTap/releases) — the Python runtime (CPython 3.13 with `attrs` and `numpy`) is included:

- `PythonTap-<version>.zip` — every platform at once: Apple Silicon and Intel Macs and Windows, each with its own runtime (in `support/macos-arm64`, `support/macos-x86_64` and `support/windows-x64`). The largest; the one to use when a patch travels between machines.
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
In the package for every platform, the runtime is in `support/<platform>/` — `./support/macos-arm64/bin/python3`, say, or `.\support\windows-x64\python.exe` — and a package you add goes into that platform's runtime only. If you use [uv](https://docs.astral.sh/uv/), `uv pip install --python support/bin/python3 <package>` (or `support\python.exe` on Windows) does the same, faster; nothing in the package needs uv.

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

Python sources live in the package's `python` folder. `[tap.python~ name]` loads `python/name.py` and instantiates the class `name` defined in it (the file, class, and argument must share the same name, which must be a valid Python identifier; with no argument, `default` is loaded). The file is loaded by its path, never through `import`, so a file named like a standard-library module (`random.py`, `json.py`) works and does not shadow that module for anyone else. Other files in the `python` folder can be imported by your class as helper modules (the folder is on `sys.path`, after the standard library) — they are imported once per Max session, as any module is, so a save of a helper is not picked up until Max restarts; what you are live-coding belongs in the class file. Text is UTF-8: the interpreter runs in Python's UTF-8 mode, so `open()` reads and writes UTF-8 unless you pass another `encoding`, whatever the machine's locale.

- **Attributes** — class-level annotated fields (e.g. via `attrs`) become Max attributes. `int` maps to a Max `long`, `float` to `float64`, `bool` to an on/off `long` (your field receives a real `bool`), anything else to a symbol; `Optional[X]` and `X | None` count as `X`. Names starting with `_` are private, and `ClassVar`s are not fields. If a hint cannot be resolved (say a name imported only under `TYPE_CHECKING`), the console says so and the annotation is read as written.
- **Messages** — public methods (including classmethods and staticmethods) become Max messages, called according to their signature: parameters with defaults are optional, `*args` takes any number of arguments, and a keyword-only parameter must have a default (a method with a required keyword-only parameter is not exposed). Argument hints (`int`, `float`, `bool`, `str`) drive the conversion from Max atoms; an unannotated parameter receives the atom as it is (an `int`, `float` or `str`). Methods named `int`, `float`, `symbol`, and `bang` map to those standard Max messages. Names Max or the object handle themselves (`filechanged`, `dsp64`, `notify`, `assist`, `loadbang`, `dblclick`, `anything`, …, and the object's own attributes `mode`, `latency` and `latencysamples`) are not exposed — as methods, or as fields; the console names any skipped this way so you can rename it.
- **Audio** — a method `process()` runs on the signal, in one of two forms chosen by its type hint:
  - `process(self, x: np.ndarray) -> np.ndarray` is called **once per signal vector** with a numpy array of the input and must return an array of the same length (any numeric dtype, or a list; it is converted). This is the form to use for anything that must run in real time — see `python/numpy_gain.py`, and `python/numpy_allpass.py` for a filter with feedback. The input array is reused from one call to the next: copy it if you want to keep it.
  - `process(self, x: float) -> float` is called **once per sample** — simplest for sketching; the call is cheap, but every line of Python in it runs once per sample (see [the performance note](#a-note-on-performance)).

  Its parameters are the object's signal inlets and its return hint its outlets: `process(self, left: np.ndarray, right: np.ndarray) -> tuple[np.ndarray, np.ndarray]` makes an object with two of each (see `python/stereo_width.py`), and `process(self) -> float`, with no inputs, a generator — the first inlet is always there, for messages. The inputs are all `np.ndarray` or all per sample, and a tuple return must say how many values it has (`tuple[float, float]`). A save that changes how many changes the object's inlets and outlets to match, keeping the patch cords of those that stay. Wrap the object in `mc.` to run one instance per channel of a multichannel signal. Output that is not a number, or not finite (NaN, infinity), is replaced with 0.0 and reported once in the Max console, as is a return with the wrong number of values.
- **Worker mode** — by default `process()` runs on Max's audio thread, so whatever else holds Python's interpreter — a reload, a message, another instance — holds up the audio while it does. With `@mode worker` it runs on a thread of its own instead, `@latency` milliseconds behind the audio (30 by default, rounded up to whole signal vectors): the audio thread then only copies vectors to and from that thread, and never waits for Python. Max computes a whole I/O vector's worth of signal vectors at once, so `@latency` must be longer than the I/O vector (Options > Audio Status: 512 samples is 11.6 ms at 44.1 kHz) — what is left over is the time Python has to keep up; raise it with a larger I/O vector. If Python falls further behind, the vectors it is late for are output as silence and the console says so, once per load; the delay stays the same. The read-only `@latencysamples` gives the delay in samples, for aligning other signal paths (a `delay~`, say); in direct mode it is 0. The worker thread has the real-time scheduling of an audio thread. `@mode` and `@latency` take effect as soon as they are set, by rebuilding the signal chain. Stopping the worker (DSP off, a chain rebuild, the object deleted) waits up to 100 ms for the vector in progress; a `process()` that has not returned by then is interrupted with a `WorkerStopped` exception (a `BaseException`, so `except Exception:` does not swallow it), that vector is silence, the console says so once, and the class stays bound; one that still has not returned after a further 250 ms is blocked in a call Python cannot interrupt (`time.sleep`, a long C call) and is abandoned: audio continues on a new worker, while the abandoned thread keeps costing a core until Max quits.
- **Audio settings** — an optional method `prepare(self, sample_rate: float, vector_size: int) -> None` is called with Max's sample rate and vector size before the object processes any audio, again whenever they change, and on every reload before the new code runs. `python/allpass.py` uses it to size its delay line.
- **Hot reload** — saving the `.py` file reloads it in place, as a fresh module (names you deleted from the file are gone): the attributes and messages follow the new class, and audio resumes with the new code. Until the new code is ready the object keeps running the old one, so a successful reload swaps in without a gap in the audio. Attribute values carry over — set from the patcher or by your own code — for every attribute the new class still has with the same type; an attribute whose type changed starts from its new default, and one you removed disappears from the object. If the file has an error, the object prints the traceback to the Max console and outputs silence until the next successful reload (and the attribute values come back with it); a save that breaks a file shared by many objects is reported once, not by each, while an object created later with the file still broken says so again. Every object using the same file shares one execution of it per save, and the console says so once — `Loaded name.py: process() bound, one call per sample` (or per vector), from whichever object ran the file — along with anything true of the class, such as a method skipped for its name; a save that changes nothing reloads silently. Errors that belong to one object, such as an exception in its constructor, are still reported by each. Max's file watcher notices a save within a couple of seconds, but it coalesces saves made in quick succession — a script saving once a second, say — so the object then reloads for only some of them, and can lag behind until they stop; sending the object `filechanged` reloads it at once.
- **Errors** — an exception raised by your code (in `process()`, a message, an attribute setter, the constructor or at import) prints its traceback to the Max console and never takes Max down. That includes `sys.exit()`, which is reported like any other exception rather than quitting Max. What the object cannot catch is what never reaches Python's exception machinery: a process exit below it (`os._exit()`, `os.abort()`), a crash in a C extension (a broken wheel, `ctypes`), or code that never returns — an endless loop in a message or the constructor freezes Max's main thread, and in `process()` it stalls the audio thread (in worker mode, only the worker thread, which is interrupted or abandoned when the worker stops — see above). Another Max external that embeds its own CPython alongside this one is untested and unsupported: two interpreters in one Max share process-wide state neither expects to.

### A note on performance

`process()` runs on the audio thread, holding Python's global interpreter lock. The per-sample form (`x: float`) makes one Python call per sample. The call itself is cheap — a fraction of a percent of a core, in the measurements below — but every line of Python in it runs 48,000 or 96,000 times a second, so the cost is your code's: `allpass.py`, a few lines of indexing and arithmetic, costs more than twenty times the call — and `numpy_allpass.py`, the same filter written a vector at a time, a small fraction of it. It is fantastic for sketching and live-coding an algorithm. The numpy form (`x: np.ndarray`) makes one call per signal vector, and numpy then works on the whole vector at C speed: it is the one to use when the algorithm has real work in it and has to keep up. Either way, in the default direct mode, the audio thread waits while other Python code holds the interpreter — a reload, a message, or another instance: all `tap.python~` instances share one interpreter, so heavy Python work in one can steal time from the others. [Worker mode](#writing-a-class) (`@mode worker`) takes Python off the audio thread, at the cost of a fixed delay. The object has Python hand the interpreter to a waiting thread after half a millisecond — a tenth of CPython's default — so a reload, which holds it for a few milliseconds, delays the audio a little at a time instead of all at once: measured with `core/bench`'s reload benchmark (96 kHz, 512-sample buffers, a save every 100 ms), no buffer was late at 0.5 ms, where CPython's default left some late in most runs. Errors in `process()` are printed from Max's main thread, never from the audio thread.

What `process()` costs, measured by `core/bench` — which calls it exactly as Max's audio thread does, one vector at a time — as the share of one CPU core it needs:

<!-- perf:begin (generated by scripts/update-perf-docs.py; do not edit) -->

| `process()` | Cost (at 48 kHz) | CPU at 48 kHz | CPU at 96 kHz |
|---|---|---:|---:|
| returns its input (the bridge alone) — per sample, vectors of 64 | 45 ns per sample | 0.22% | 0.43% |
| `default.py`, a gain (an attrs class) — per sample, vectors of 64 | 58 ns per sample | 0.28% | 0.57% |
| `allpass.py`, a Schroeder allpass filter — per sample, vectors of 64 | 1.2 µs per sample | 6% | 12% |
| returns its input (the bridge alone) — per vector of 64 | 0.23 µs per vector | 0.017% | 0.034% |
| returns its input (the bridge alone) — per vector of 512 | 0.52 µs per vector | 0.0049% | 0.0097% |
| `numpy_gain.py`, a gain — per vector of 64 | 0.97 µs per vector | 0.073% | 0.15% |
| `numpy_gain.py`, a gain — per vector of 512 | 1.5 µs per vector | 0.014% | 0.028% |
| `numpy_allpass.py`, the same allpass filter — per vector of 64 | 7 µs per vector | 0.53% | 0.68% |
| `numpy_allpass.py`, the same allpass filter — per vector of 512 | 31 µs per vector | 0.29% | 0.34% |

`numpy_allpass.py` computes exactly what `allpass.py` does — the core battery checks it sample for sample — and in 64-sample vectors needs 11× less at 48 kHz (0.53% of a core against 6%) and 18× less at 96 kHz (0.68% of a core against 12%). It computes a vector in stretches no longer than the delay, one numpy expression each: the default 1 ms delay is 48 samples at 48 kHz, so a 64-sample vector takes two, and 96 at 96 kHz, so it takes one — a delay shorter than the vector costs more.

The share of one core: each row timed once per round for seven rounds, interleaved, keeping its fastest; every median was within 32% of it. Measured 2026-09-30 on Intel(R) Core(TM) i9-8950HK CPU @ 2.90GHz, macOS 15.7.9 (x86_64), CPython 3.13.14, numpy 2.5.3, load average 2.4.

<!-- perf:end -->

In Max itself, measured by Max's own CPU meter — which covers everything the audio thread does around `process()` too — beside what `core/bench` predicts:

<!-- perf-max:begin (generated by scripts/update-perf-docs.py --max; do not edit) -->

| In Max at 96 kHz | Instances | Max's CPU meter | Each | `core/bench`, each |
|---|---:|---:|---:|---:|
| `default.py`, per sample | 26 | 22% | 0.84% | 0.55% |
| `numpy_gain.py`, per vector of 64 | 26 | 7% | 0.28% | 0.14% |
| `allpass.py`, per sample | 1 | 18% | 18% | 12% |
| `numpy_allpass.py`, per vector of 64 | 26 | 28% | 1.1% | 0.69% |

Max's own DSP CPU meter (`adstatus cpu`): the mean of ten readings a second apart with the instances running in a `poly~`, less the reading with no object (0.0%); *each* divides by the number running. The audio device ran at 96 kHz with 64-sample signal vectors and a 512-sample I/O vector. The meter reads in whole percent. Measured 2026-09-30 with Max 9.1.5, on Intel(R) Core(TM) i9-8950HK CPU @ 2.90GHz, macOS 15.7.9 (x86_64), CPython 3.13.14, numpy 2.5.3.

<!-- perf-max:end -->

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

Push a tag `vMAJOR.MINOR.PATCH`: `.github/workflows/release.yml` builds and tests on each platform (both Mac architectures on their own runners), assembles the package with `scripts/assemble-package.py` (the platform's externals, help, docs, examples, the runtime, and `licenses/`), zips it with SHA256 checksums, merges the three into one package for every platform (`assemble-package.py --merge`: each runtime in `support/<platform>`, each platform's licenses in `licenses/<platform>`), and attaches everything to a release — published as a pre-release for a 0.x version, and a **draft** to review and publish from 1.0 on. The package version comes from the tag (min reads it from git; the assembly checks they agree). Running the workflow by hand builds the zips as workflow artifacts without a release. It signs and notarizes the Mac packages and signs the Windows binaries when the signing secrets it lists are set, and skips signing, with a warning, when they are not.

### The core, and testing it on Linux

Everything that talks to CPython — starting the interpreter, loading and reloading the class, describing its attributes and messages, converting values, and running `process()` — lives in a host-independent core under `core/include/tap/python/` (plain C++20 + CPython, no Max). The external in `source/projects/` is a thin layer that maps the core onto Max. The core builds and tests on any platform with a CPython 3.13 — no Max, no runtime install:

```sh
cmake -S core -B build-core -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Any CPython 3.13 with its headers will do; if you have no `python3.13`, `uv python install 3.13` gets one, and `$(uv python find 3.13)` names it. The example tests need `attrs` and `numpy` importable by that interpreter (or in a folder named by `TAP_PYTHON_TEST_SITE`); without them they are skipped. `-DTAP_PYTHON_SANITIZE=address,undefined` or `=thread` builds the battery under sanitizers, as CI does.

The core's build also makes `core/bench`, which times `process()` as Max's audio thread calls it; `python3 scripts/update-perf-docs.py` builds it (Release), runs it, and rewrites the tables in [the performance note](#a-note-on-performance) — with `--max`, the table measured in Max too. Measure on an idle machine; never edit those tables by hand.

The external and its mock-kernel unit test also build on Linux (Max does not run there, but its glue does), embedding the same CPython instead of a `support/` runtime:

```sh
cmake -S . -B build-linux -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-linux
ctest --test-dir build-linux --output-on-failure
```

### Runtime tests in Max

What only a real Max shows — the file watcher, attributes read through `getattr`, audio through the signal chain, `poly~`, the console, loading without a runtime — is tested by patchers that Max runs, with Cycling '74's max-test harness. On a Mac with Max 9, with the external built, the runtime installed and the package in `Packages`, quit Max and run:

```sh
python3 runtime-tests/run.py    # launches Max, runs every test patcher, quits it (~2 minutes)
```

It installs the harness into `Packages` as `max-test` (and leaves it there). `--session soak` runs the hour-long soak instead — audio through many instances while their files are saved every second, a sample-rate change, memory sampled each minute — and `--session perf` reads Max's CPU meter under load. See [runtime-tests/README.md](runtime-tests/README.md) for what they do and how to write a test.

## License

MIT — see [License.md](License.md), which also lists the third-party components (Min-API, CPython, attrs, NumPy) and where their licenses live.
