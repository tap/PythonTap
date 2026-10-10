# <picture><source media="(prefers-color-scheme: dark)" srcset=".github/icon-dark.svg"><img src=".github/icon-light.svg" width="40" height="40" alt="" align="top"></picture> tap.python

[![build](https://github.com/tap/PythonTap/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/tap/PythonTap/actions/workflows/build.yml)
[![Max 9](https://img.shields.io/badge/Max-9%2B-9cf)](https://cycling74.com/products/max)
[![Python 3.13](https://img.shields.io/badge/Python-3.13-3776AB?logo=python&logoColor=white)](https://www.python.org)
[![platforms](https://img.shields.io/badge/platforms-macOS%20universal%20%7C%20Windows%20x64-lightgrey)](#install)
[![license: MIT](https://img.shields.io/badge/license-MIT-green)](License.md)
[![book](https://img.shields.io/badge/docs-the%20book-orange)](https://tap.github.io/PythonTap/)

Write Max objects in Python. PythonTap embeds CPython 3.13 in a Max package and runs a Python class as a Max object:

- **`tap.python~`** runs it as an **audio object**: its `process()` method is called on the signal — once per signal vector with numpy arrays, or once per sample with floats.
- **`tap.python`** runs it as an **object without audio**: messages in, messages out — and what a method returns is what the object outputs.

Either way, the class's **type-annotated fields** become Max attributes, its **public methods** become Max messages, the file is **hot-reloaded** every time you save it, and Python's `print()` and tracebacks land in the **Max console** — no exception your code raises can take Max down.

```python
from attrs import define, field

@define
class mygain:
    gain: float = field(default = 0.5)

    def process(self, x: float) -> float:
        return x * self.gain
```

```
[tap.python~ mygain]   ← loads python/mygain.py: a 'gain' attribute, process() on the signal
```

**[Read the book](https://tap.github.io/PythonTap/)** for everything else: how a class maps onto Max, audio and worker mode, objects without audio, performance, errors and limits, the examples, and building and testing it yourself.

## Install

Requires **Max 9** or later, on macOS 11.0+ (Apple Silicon or Intel) or Windows 10 22H2+ / 11 (64-bit).

1. Download a package from the [releases page](https://github.com/tap/PythonTap/releases) — `PythonTap-<version>.zip` for every platform at once, or the zip for yours. The Python runtime (CPython 3.13 with `attrs` and `numpy`) is included.
2. Unzip it into your `Documents/Max 9/Packages` folder.
3. Releases are not code-signed yet. On a Mac, clear the download's quarantine once, or Max will not load the external:
   ```sh
   xattr -dr com.apple.quarantine ~/Documents/"Max 9"/Packages/PythonTap
   ```
   On Windows, SmartScreen may warn about the download.
4. Restart Max, and open **PythonTap Overview** from the Extras menu.

The book's [Installing](https://tap.github.io/PythonTap/installing.html) chapter says which zip is which, how to add Python packages to the runtime, and how to work from a clone of this repository.

## Quick start

1. Save the class above as `mygain.py` in the package's `python` folder (`Documents/Max 9/Packages/PythonTap/python/`). The file, the class and the object's argument share one name.
2. In a patcher: `[cycle~ 440]` → `[tap.python~ mygain]` → `[ezdac~]`. Turn the audio on.
3. Send the object `gain 0.1`, or give it `@gain 0.1` in its box, or connect an `attrui`.
4. Change `process()` — `return x * x * self.gain`, say — and save: within a couple of seconds the object reloads, without a gap in the audio, keeping its gain.

For an object without audio, give the class a method that returns something: with `def bang(self) -> float: return self.gain` added, a bang to `[tap.python mygain]` outputs the gain. The package's `python` folder has more [examples](https://tap.github.io/PythonTap/examples.html), and Max's Documentation window has the package's guide and three tutorials (Package Docs › PythonTap).

## More

- [The book](https://tap.github.io/PythonTap/) — the whole documentation, and the measurements.
- [CHANGELOG.md](CHANGELOG.md) — what changed in each release.
- [Building from source](https://tap.github.io/PythonTap/building.html) and [testing](https://tap.github.io/PythonTap/testing.html), for contributors; [the design](https://tap.github.io/PythonTap/design.html) and [docs/PRODUCTION-PLAN.md](docs/PRODUCTION-PLAN.md), the roadmap.

## License

MIT — see [License.md](License.md), which also lists the third-party components (Min-API, CPython, attrs, NumPy) and where their licenses live.
