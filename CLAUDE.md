# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

**PythonTap** — the `tap.python~` Max package: a Max external that embeds CPython 3.13 and runs a
user's Python class as an audio object. `[tap.python~ name]` imports `python/name.py`, instantiates
`class name`, turns its annotated public fields into Max attributes and its public methods into Max
messages, calls its `process()` on the signal, and hot-reloads on save. `ReadMe.md` is the user-facing
contract; **`docs/PRODUCTION-PLAN.md` is the authoritative roadmap** — its settled decisions (D1–D6),
its phases, and the audit findings behind them. Tick its items (with the PR) as they land.

## Layout (D6: a host-independent core plus a thin Max wrapper)

- **`core/include/tap/python/`** — everything that talks to CPython, in namespace `tap::python`, plain
  C++20 + CPython with no Max or min-api: `runtime.h` (one process-wide interpreter, `gil_lock`,
  routing `print()`/tracebacks to a host sink), `value.h` (the Max-atom value model and its coercion
  rules, which reproduce `atom_getlong`/`atom_getfloat`/`atom_getsym`), `processor.h` (load/reload,
  class introspection, attribute and message dispatch, `prepare()`, and `process()` — per sample, or
  per vector when the input is hinted `np.ndarray` — plus `flush_reports()`). New CPython-facing
  behavior goes here, never in the wrapper.
- **`core/tests/`** — the core's Catch2 battery against CPython 3.13, with Python fixtures in
  `core/tests/python/` and the shipped examples copied alongside. Runs on Linux, including under
  ASan/UBSan and TSan.
- **`source/projects/tap.python_tilde/`** — the Min external: package paths, the file watcher, atom
  conversion, and mapping the core's descriptions onto `object_addattr`/`object_addmethod`. Its
  `_test.cpp` drives the Max glue through min-api's mock kernel.
- **`python/`** — the user's script folder in the package (and the examples: `default.py`,
  `allpass.py`, `allpass-doc.ipynb`).
- **`scripts/install-runtime.{sh,ps1}`** — installs python-build-standalone CPython 3.13 plus attrs and
  numpy into `support/` (gitignored), verified against `scripts/runtime.lock` (per-platform archive
  SHA256s) and `scripts/requirements.lock` (pip `--require-hashes`, wheels only). Never hand-edit the
  locks: `scripts/update-locks.py` regenerates them (moving a pin is deliberate — D4). CI installs from
  the same locks, and caches `support/` keyed on them. The macOS/Windows build links against it.

## Build & test

The fast loop is Linux, no Max and no runtime install needed — just a CPython 3.13 with headers:

```sh
# the core battery
cmake -S core -B build-core -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-core && ctest --test-dir build-core --output-on-failure
# add -DTAP_PYTHON_SANITIZE=address,undefined (or thread) for the sanitizer builds

# the external + its mock-kernel test, embedding the same CPython (Linux only; Max doesn't run here)
git submodule update --init --recursive
cmake -S . -B build-linux -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-linux && ctest --test-dir build-linux --output-on-failure
```

The example tests need `attrs` and `numpy` importable by that interpreter (or in a folder named by
`TAP_PYTHON_TEST_SITE`); they skip without them, and CI sets `TAP_PYTHON_TEST_REQUIRE_EXAMPLES` so a
missing dependency fails instead. The shipping targets are macOS (universal) and Windows x64:
`./scripts/install-runtime.sh --universal` (or the `.ps1`), then `cmake -S . -B build` — configure fails
without `support/`. The Max unit test resolves the package from its binary's folder (`<repo>/tests/`),
so on every platform it runs the real interpreter and `python/default.py`.

CI (`build.yml`): `linux-core` (release, asan-ubsan, tsan), `linux-max-glue`, `macos` (universal +
`lipo`/`otool` checks, including no absolute rpath), `windows`. `style.yml`: TapHouse drift check,
clang-format, clang-tidy (a clang-tidy failure or crash fails the gate, not just a finding). Workflows
run with `contents: read`, pin third-party actions by commit SHA (tag noted beside it), and cancel
superseded runs.

## Threads and the GIL (load-bearing)

- The interpreter is initialized once per process and **never finalized** (re-initializing is unsafe
  for numpy); all instances share it. After start-up the GIL is released and every entry point takes a
  `gil_lock`.
- `processor::load()` and destruction run on Max's main thread; attribute and message calls on the main
  or scheduler thread; `process()` on the audio thread. Holding the GIL does **not** serialize them —
  CPython hands the GIL to a waiting thread every switch interval (5 ms), mid-`process()` or mid-reload,
  and even an allocation can run a GC finalizer that yields it. So `load()` builds the new binding
  completely and swaps it in with no Python call in between, and every caller takes its own references
  (and copies) of what it uses before running anything that could yield. Keep it that way.
- `gil_lock` gives each thread one long-lived Python thread state (`detail::thread_state_keeper`);
  don't call `PyGILState_Ensure` directly.
- **Nothing prints on the audio thread.** Problems in `process()` are recorded (exception object,
  flags) and the host's `report_ready` callback fires — the external sets a `queue<>` — and
  `flush_reports()` prints them on the main thread. Anything new on the audio path follows suit.
- Report user exceptions with `tap::python::report_exception()`, **never `PyErr_Print()`**: for a
  `SystemExit` it calls `Py_Exit()` and quits Max. Never finalize the interpreter, and never let a C++
  exception cross a Max callback (the trampolines are wrapped in `guarded()`).

## Conventions

- **Honest limits are pinned, not hidden.** A known bug or limit gets a test that states it (named
  for the promise, with the plan item that will change it); fixes land against a test that reproduces
  the bug first. The core battery is where that happens.
- **Style:** `STYLE.md`, `.clang-format`, `.clang-tidy`, `.pre-commit-config.yaml`, `scripts/tidy.sh`
  and the SessionStart hook are canonical TapHouse copies — never hand-edit them (CI drift-checks).
  New files use the STYLE.md §3 SPDX banner. clang-tidy compiles with a clang front end; treat it as a
  second compiler. `style.yml` lints the external's TUs at C++20 against 3.13 headers and the core's
  tests through its compile database.
- **C++20** everywhere; the root `CMakeLists.txt` forces it on every object and `_test` target (Min pins
  C++17), as TapTools-Max does.
- **Keep in sync when behavior changes:** `ReadMe.md`, `docs/tap.python~.maxref.xml`,
  `help/tap.python~.maxhelp`, the examples and their notebook, and the plan.
- **Implement from documentation and published sources only** — the CPython C-API docs, the Max SDK
  docs — never by reverse-engineering another product.
