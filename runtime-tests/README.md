# Runtime tests in Max

The unit tests (`core/tests/`, and the external's `_test.cpp` against min's mock kernel) never load
the objects into Max. These do — `tap.python~` and, since plan 9.5, `tap.python` (the
`tap.python.*` patchers): each is a patcher that Max runs, driven by Cycling '74's
[max-test](https://github.com/Cycling74/max-test) harness (MIT, the `max-test` submodule), checking
what only a real Max shows — the file watcher, `getattr` on the Python-defined attributes, audio
through the real signal chain, `poly~`, the console, and loading without a runtime. (Plan 6.1 and
6.4; the first run found a crash on every save of a class file, which no unit test could see.)

```sh
python3 runtime-tests/run.py              # everything (~2 minutes); exit status 0 when all pass
python3 runtime-tests/run.py --only reload -v
python3 runtime-tests/run.py --session soak   # the hour-long soak (plan 6.2)
python3 runtime-tests/run.py --session perf   # Max's CPU meter under load (plan 6.3, ~4 minutes)
python3 runtime-tests/run.py --package ~/Documents/Max\ 9/Packages/PythonTap   # an installed release
```

`--package` tests a package installed in `Packages` as `PythonTap` — a release zip unzipped there —
instead of this checkout: its external, its runtime and its `python/` folder; the harness, the
patchers and the fixtures still come from here (plan 6.5).

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
│   ├── without-runtime/              # run only by run.py, with support/ moved aside
│   ├── soak/                         # the hour-long soak, only with --session soak
│   └── perf/                         # the CPU measurement, only with --session perf
├── python/                 # the classes the tests load (maxtest_*.py)
├── max-test-config.json    # the harness's OSC ports
└── max-test/               # the harness (submodule, pinned)
```

## What the runner does

1. Checks the package: the external built from the current sources, a runtime, and the lines of
   `init/tap.python.txt` without which Max finds neither `tap.python` nor `mc.tap.python~`. Then
   backs up Max's preference files, turns off **Restore Windows on Launch** and moves aside the
   workspaces Max would reopen (into `logs/`) — a window reopened at launch would load the binary
   before the first test — and puts the preferences back when the run ends.
2. Builds the harness's `oscar` extension if needed, and installs the harness into `Packages` as
   `max-test` — a **copy**, not a link, because Max loads a package's extensions only from a real
   folder. The copy is marked `INSTALLED-BY-PYTHONTAP.txt`, replaced on every run, and left
   installed afterwards (Max then loads `oscar` at every launch; delete the folder to uninstall).
   Another `max-test` already in `Packages` is refused, not replaced.
3. Links `runtime-tests/` into `Packages` as `PythonTap-runtime-tests` (so Max finds the patchers
   by name), and copies `python/maxtest_*.py` into the package's `python/` folder (the object loads
   classes from there only). Both are removed afterwards.
4. **Session `without-runtime`**: moves `support/` aside and starts Max. The object must load and
   say the runtime is missing; then, with `support/` put back while Max runs, a new object must
   say to restart Max (on macOS the weak binding to libpython is fixed when the external loads).
   `support/` is always put back — and a run killed midway leaves it as `support.maxtest-aside/`,
   which the next run restores.
5. **Session `main`**: starts Max again and runs every `*.maxtest.maxpat` — `tap.python.load` first,
   so that the fresh Max must find `tap.python` through the package's `init/` mapping (plan 9.0).
6. Only when asked for: **session `soak`** (plan 6.2) runs `soak/tap.python~.soak.maxpat` for an
   hour, sampling Max's memory every minute, and writes a summary — memory, reloads, and what the
   patcher logged each minute — to `logs/soak.summary`; **session `perf`** (plan 6.3) runs
   `perf/tap.python~.perf.maxpat` and averages Max's CPU-meter readings into `logs/perf.json`,
   which `scripts/update-perf-docs.py --max` turns into the ReadMe's table.
7. Reads the results from the harness's SQLite database (in the installed `max-test`) and prints
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

- **Max's file watcher coalesces saves made close together**: saving once a second, the object
  was told of about half of them, the first some seconds late. A test that needs a reload at a
  given moment sends `filechanged` itself (the soak saves *and* sends it); one that tests the
  watcher leaves a couple of seconds after a save (`WATCH`).
- **`adstatus cpu` reports on its own when audio starts or restarts** — dozens of stale values at
  once. `Test.cpu_reading()` keeps only its answer to a bang.
- **A `poly~`'s `up`/`down` change its vector size too** (`down 2` halves it), and `down 1` sent
  after `up 2` undoes the `up`. Fine for per-sample checks; a block-path measurement at another
  rate is not like for like, so the perf patcher measures at the device's own rate.
- **A check that audio did not change passes vacuously if no audio ran**: pair each with a sample
  check (the soak's `phase-N-…-output`).
- **`tap.python`'s output is checked as `[tosymbol]` writes it** (`Test.output_is()`): one symbol for
  the whole message — `symbol C` (the word `symbol` kept), `1 0 1`, `note 60 100`, and floats with
  four decimals, `0.5000` — held in a `[zl.reg]` until the check compares it with a quoted
  `[sel "…"]`. A `bang` makes `[tosymbol]` repeat its last symbol, so a bang output is not checked
  this way. An output that never came fails the check: the harness records an assertion that never
  answers as a failure ("never returned results"). `Test.output_count()` and `Test.count_is()` check
  how many messages an outlet sent — nothing, for `None`.
- **Text logged with spaces breaks the harness's SQL** (its log line goes between double quotes, and
  Max quotes a symbol with a space): log codes, not text — `[atoi]`, as the probe that found the
  formats above did.
- **Overdrive is `; max preempt 1`, Scheduler in Audio Interrupt `; dsp takeover 1`**; the threads
  test reads both through `[adstatus]` first and puts them back at the end.

The fixtures' docstrings say what each is for. Open a patcher in Max to watch a test run; its
`test.*` objects only record anything when the harness opened it.

## Not covered here

`tap.python`'s once-per-session notice that Scheduler in Audio Interrupt runs Python on the audio
thread is a warning, which `[error]` does not see: the glue test checks it, and the threads test only
that the messages run there correctly. Some of the runbook's checks still need a person: that a `bool` attribute shows as a toggle in the
inspector, that an `attrui` displaying a removed attribute looks right (the tests check only that
nothing breaks), and whether the audio *device* drops out while a reload holds the GIL (the tests
check every sample the object outputs, not the driver). Changing the sample rate in Audio Status is
covered by `poly~`'s `up` instead, which does not touch the audio device. Windows is not covered:
the runner is macOS-only. The perf session measures at whatever sample rate the audio device is
set to — set it in Audio Status first if you want another.
