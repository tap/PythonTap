# Testing

## The core, and testing it on Linux

Everything that talks to CPython — starting the interpreter, loading and reloading the class, describing its attributes and messages, converting values, and running `process()` — lives in a host-independent core under `core/include/tap/python/` (plain C++20 + CPython, no Max). The external in `source/projects/` is a thin layer that maps the core onto Max. The core builds and tests on any platform with a CPython 3.13 — no Max, no runtime install:

```sh
cmake -S core -B build-core -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Any CPython 3.13 with its headers will do; if you have no `python3.13`, `uv python install 3.13` gets one, and `$(uv python find 3.13)` names it. The example tests need `attrs` and `numpy` importable by that interpreter (or in a folder named by `TAP_PYTHON_TEST_SITE`); without them they are skipped. `-DTAP_PYTHON_SANITIZE=address,undefined` or `=thread` builds the battery under sanitizers, as CI does.

The core's build also makes `core/bench`, which times `process()` as Max's audio thread calls it; `python3 scripts/update-perf-docs.py` builds it (Release), runs it, and rewrites the tables in [the performance chapter](performance.md) — with `--max`, the table measured in Max too. Measure on an idle machine; never edit those tables by hand.

The external and its mock-kernel unit test also build on Linux (Max does not run there, but its glue does), embedding the same CPython instead of a `support/` runtime:

```sh
cmake -S . -B build-linux -DPython3_EXECUTABLE=$(which python3.13)
cmake --build build-linux
ctest --test-dir build-linux --output-on-failure
```

## Runtime tests in Max

What only a real Max shows — the file watcher, attributes read through `getattr`, audio through the signal chain, `poly~`, the console, loading without a runtime — is tested by patchers that Max runs, with Cycling '74's max-test harness. On a Mac with Max 9, with the external built, the runtime installed and the package in `Packages`, quit Max and run:

```sh
python3 runtime-tests/run.py    # launches Max, runs every test patcher, quits it (~2 minutes)
```

It installs the harness into `Packages` as `max-test` (and leaves it there). `--session soak` runs the hour-long soak instead — audio through many instances while their files are saved every second, a sample-rate change, memory sampled each minute — and `--session perf` reads Max's CPU meter under load. See [runtime-tests/README.md](https://github.com/tap/PythonTap/blob/main/runtime-tests/README.md) for what they do and how to write a test.
