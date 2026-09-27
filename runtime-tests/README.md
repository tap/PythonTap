# Runtime tests in Max

The unit tests (`core/tests/`, and the external's `_test.cpp` against min's mock kernel) never load
the object into Max. These do: each is a patcher that Max runs, driven by Cycling '74's
[max-test](https://github.com/Cycling74/max-test) harness (MIT, the `max-test` submodule), checking
what only a real Max shows — the file watcher, `getattr` on the Python-defined attributes, audio
through the real signal chain, `poly~`, the console, and loading without a runtime. (Plan 6.1 and
6.4; the first run found a crash on every save of a class file, which no unit test could see.)

```sh
python3 runtime-tests/run.py              # everything (~2 minutes); exit status 0 when all pass
python3 runtime-tests/run.py --only reload -v
```

Needs macOS, Max 9 in `/Applications` (`--max` for another), the external built, the runtime
installed, and the package in Max's `Packages` folder as `PythonTap` (see the ReadMe). Quit Max
first: the runner launches its own, drives it over OSC (UDP 4791/4792 on localhost) and quits it.
Unlicensed Max runs patchers, which is all this needs.

```
runtime-tests/
├── run.py                  # the runner
├── make_patchers.py        # generates patchers/ — the tests are written here
├── patchers/               # generated; never edit by hand
│   ├── maxtest.host.maxpat           # the poly~ abstraction hosting a [tap.python~ #1]
│   ├── *.maxtest.maxpat              # the tests run with the runtime installed
│   └── without-runtime/              # run only by run.py, with support/ moved aside
├── python/                 # the classes the tests load (maxtest_*.py)
├── max-test-config.json    # the harness's OSC ports
└── max-test/               # the harness (submodule, pinned)
```

## What the runner does

1. Builds the harness's `oscar` extension if needed, and installs the harness into `Packages` as
   `max-test` — a **copy**, not a link, because Max loads a package's extensions only from a real
   folder. The copy is marked `INSTALLED-BY-PYTHONTAP.txt`, replaced on every run, and left
   installed afterwards (Max then loads `oscar` at every launch; delete the folder to uninstall).
   Another `max-test` already in `Packages` is refused, not replaced.
2. Links `runtime-tests/` into `Packages` as `PythonTap-runtime-tests` (so Max finds the patchers
   by name), and copies `python/maxtest_*.py` into the package's `python/` folder (the object loads
   classes from there only). Both are removed afterwards.
3. **Session `without-runtime`**: moves `support/` aside and starts Max. The object must load and
   say the runtime is missing; then, with `support/` put back while Max runs, a new object must
   say to restart Max (on macOS the weak binding to libpython is fixed when the external loads).
   `support/` is always put back — and a run killed midway leaves it as `support.maxtest-aside/`,
   which the next run restores.
4. **Session `main`**: starts Max again and runs every `*.maxtest.maxpat`.
5. Reads the results from the harness's SQLite database (in the installed `max-test`) and prints
   them. Max's standard output goes to `runtime-tests/logs/` — only min's own lines appear there;
   the console errors seen during a failing test are printed from its log instead.

## Writing a test

Tests are Python functions in `make_patchers.py`, each a script of steps — wait, send a message,
sample the signal, read an attribute, count console errors — that becomes a max-test patcher
(loadbang → a chain of `[delay]` and `[t b b …]` → checks into `test.assert` → `test.terminate`).
Regenerate with `python3 runtime-tests/make_patchers.py` and commit the patchers with it. What the
first runs taught, built into the helpers:

- **A sample is taken on the next signal vector**, and Max computes vectors in bursts per I/O
  buffer — well after the scheduler has moved on. So every step waits (150 ms by default) before
  changing what an earlier check measures.
- **Audio takes a while to start**; audio tests begin their script when `dspstate~` says DSP is on
  (the `audio-started` assertion), with a watchdog that ends a test that never gets there.
- **An object in the patcher runs its constructor before any patch cord exists**, so what it
  prints then is lost to `[error 1]`. To check that, host it in a `poly~` loaded by a later step
  (`Test.host()` with no class, then `Test.load()`) — though such a `poly~` has no outlets.
- **`getattr` listens by default** and would answer again whenever a message sets the attribute;
  the checks use `@listen 0`. `test.equals` takes only floats: ints are compared with `[== n]`.
- **Console errors are logged** into each test's results (at most 50), for the runner to print
  when it fails: made plain first, because the harness writes its log into SQL between double
  quotes, and a quote in the text makes the database post an error that would be logged again.

The fixtures' docstrings say what each is for. Open a patcher in Max to watch a test run; its
`test.*` objects only record anything when the harness opened it.

## Not covered here

Some of the runbook's checks still need a person: that a `bool` attribute shows as a toggle in the
inspector, that an `attrui` displaying a removed attribute looks right (the tests check only that
nothing breaks), and whether the audio *device* drops out while a reload holds the GIL (the tests
check every sample the object outputs, not the driver). Changing the sample rate in Audio Status is
covered by `poly~`'s `up` instead, which does not touch the audio device. Windows is not covered:
the runner is macOS-only.
