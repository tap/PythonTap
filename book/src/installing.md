# Installing

## Requirements

- **Max 9** or later (macOS 11.0+ on Intel or Apple Silicon, Windows 10 22H2+ / Windows 11, 64-bit).

Download a package from the [releases page](https://github.com/tap/PythonTap/releases) — the Python runtime (CPython 3.13 with `attrs` and `numpy`) is included:

- `PythonTap-<version>.zip` — every platform at once: Apple Silicon and Intel Macs and Windows, each with its own runtime (in `support/macos-arm64`, `support/macos-x86_64` and `support/windows-x64`). The largest; the one to use when a patch travels between machines.
- `PythonTap-<version>-macos-arm64.zip` — Apple Silicon Macs.
- `PythonTap-<version>-macos-x86_64.zip` — Intel Macs, and Apple Silicon Macs running Max under Rosetta.
- `PythonTap-<version>-windows-x64.zip` — Windows.

Unzip it into your `Documents/Max 9/Packages` folder, restart Max, and open **PythonTap Overview** from the Extras menu — or the help patcher of `tap.python~` or `tap.python`, or the package's guide and tutorials in the Documentation window. Each zip has a `.sha256` beside it, and the release's `SHA256SUMS` lists them all. The package's `licenses/` folder holds the license of everything it ships.

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

## From a clone of this repository

Max finds the package only in its `Packages` folder, so clone it there (or clone it anywhere and symlink it in, e.g. `ln -s ~/src/PythonTap ~/Documents/"Max 9"/Packages/PythonTap`), with its submodules:

```sh
cd ~/Documents/"Max 9"/Packages
git clone --recursive https://github.com/tap/PythonTap.git
```

A clone has no runtime: install it into the package's `support` folder (from [python-build-standalone](https://github.com/astral-sh/python-build-standalone)), then build (see [Building from source](building.md)):

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
