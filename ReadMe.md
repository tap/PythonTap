# tap.python

[![build](https://github.com/tap/tap.python/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/tap/tap.python/actions/workflows/build.yml)
[![Max 9](https://img.shields.io/badge/Max-9%2B-9cf)](https://cycling74.com/products/max)
[![Python 3.13](https://img.shields.io/badge/Python-3.13-3776AB?logo=python&logoColor=white)](https://www.python.org)
[![platforms](https://img.shields.io/badge/platforms-macOS%20universal%20%7C%20Windows%20x64-lightgrey)](#requirements)
[![license: MIT](https://img.shields.io/badge/license-MIT-green)](License.md)

Write Max objects in Python.

`tap.python~` embeds a CPython interpreter inside a Max external and runs a Python class as an audio object:

- The class's **`process()` method** is called on the audio signal.
- The class's **type-annotated attributes** become Max attributes (`gain 0.5` in the object box, `getgain`, attrui — it all works).
- The class's **public methods** become Max messages, with arguments converted according to their type hints.
- The source file is **watched and hot-reloaded** every time you save it, so you can live-code DSP with Max running.
- Python's `print()` output and tracebacks land in the **Max console**.

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
- An internet connection for the one-time runtime install below.

## Installation

1. Place this package in your `Documents/Max 9/Packages` folder.
2. Install the embedded Python runtime (CPython 3.13, from [python-build-standalone](https://github.com/astral-sh/python-build-standalone)) into the package's `support` folder:

   **macOS** — in Terminal, from the package root:
   ```sh
   ./scripts/install-runtime.sh
   ```

   **Windows** — in PowerShell, from the package root:
   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts\install-runtime.ps1
   ```

   The script also installs the Python packages used by the examples (`attrs`, `numpy`). To add more packages later:
   ```sh
   ./support/bin/python3 -m pip install <package>      # macOS
   .\support\python.exe -m pip install <package>       # Windows
   ```

3. Restart Max and open the `tap.python~` help patcher.

## Writing a class

Python sources live in the package's `python` folder. `[tap.python~ name]` imports `python/name.py` and instantiates the class `name` defined in it (the file, class, and argument must share the same name; with no argument, `default` is loaded).

- **Attributes** — class-level annotated fields (e.g. via `attrs`) become Max attributes. `int` maps to a Max `long`, `float` to `float64`, anything else is treated as a symbol. Names starting with `_` are private and skipped.
- **Messages** — public methods become Max messages. Argument type hints (`int`, `float`, `str`) drive the conversion from Max atoms. Methods named `int`, `float`, `symbol`, and `bang` map to those standard Max messages.
- **Audio** — a method `process(self, x: float) -> float` is called once per sample with the input sample and must return the output sample. One signal inlet and one signal outlet, single-channel; wrap the object in `mc.` for multichannel. (Returning a tuple for multiple outputs is not supported yet.)
- **Hot reload** — saving the `.py` file reloads it in place: attributes and messages are rebuilt and audio resumes with the new code. If the file has an error, the object prints the traceback to the Max console and outputs silence until the next successful reload.

### A note on performance

`process()` is called once per sample from the audio thread, holding Python's global interpreter lock. That is fantastic for sketching and live-coding an algorithm, and expensive for production use — expect meaningful CPU load at high sample rates, and audio dropouts while a reload is in progress. All `tap.python~` instances share one interpreter, so heavy Python work in one instance can steal time from the others. A vectorized `process(numpy array)` path is on the roadmap.

## Building from source

Requires CMake 3.19+, a C++20 compiler (Xcode 12+ / Visual Studio 2019+), and the runtime installed as above (the build links against it — on CI or for building universal macOS binaries, use `./scripts/install-runtime.sh --universal`).

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release    # externals land in externals/
ctest --test-dir build                  # unit tests (mock kernel, no Max needed)
```

On Windows, configure with `cmake -S . -B build -A x64`.

## License

MIT — see [License.md](License.md), which also lists the third-party components (Min-API, CPython, attrs, NumPy) and where their licenses live.
