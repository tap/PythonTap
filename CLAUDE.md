# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

**PythonTap** — the `tap.python~` Max package: a Max external that embeds CPython 3.13 and runs a
user's Python class as an audio object. `[tap.python~ name]` loads `python/name.py`, instantiates
`class name`, turns its annotated public fields into Max attributes and its public methods into Max
messages, calls its `process()` on the signal, and hot-reloads on save. Since 2.0.0 the package has a
second object, **`tap.python`**, a Python class as a Max object without audio (what a method returns
is what it outputs), in the same binary: designed in `docs/TAP-PYTHON-PLAN.md` (decisions D7–D11).
**The book (`book/`, published at https://tap.github.io/PythonTap/) is the user-facing contract**;
`ReadMe.md` is only the front door — what it is, install, a quick start, links — so each fact has
one home (plan 9.7). **`docs/PRODUCTION-PLAN.md` is the authoritative roadmap** — its settled
decisions (D1–D6), its phases, and the audit findings behind them. Tick its items (with the PR) as
they land.

## Layout (D6: a host-independent core plus a thin Max wrapper)

- **`core/include/tap/python/`** — everything that talks to CPython, in namespace `tap::python`, plain
  C++20 + CPython with no Max or min-api: `runtime.h` (one process-wide interpreter, `gil_lock`,
  routing `print()`/tracebacks to a host sink, and a small Python support module: `load_script()` —
  class files are compiled from source and executed by path as `_tap_python_<name>`, cached by source
  bytes, never imported — plus the introspection (`class_hints`, `methods`, `describe`: hint kinds,
  signatures) that `processor.h` builds its descriptions from), `value.h` (the Max-atom value model and its coercion
  rules, which reproduce `atom_getlong`/`atom_getfloat`/`atom_getsym`), `processor.h` (load/reload,
  class introspection, attribute and message dispatch, `prepare()`, and `process()` — per sample, or
  per vector when the input is hinted `np.ndarray` — plus `flush_reports()`; and, with `bind_audio`
  off for `tap.python`, `call_with_output()`, which converts what a method returns for the outlets
  by the plan's output table, and `outlet_count()` from the return hints: plan 9.1), `worker.h` (worker
  mode, plan 2.5: `process()` on a thread of its own, a fixed number of vectors behind an audio
  thread that only copies through a lock-free ring and never takes the GIL). New CPython-facing
  behavior goes here, never in the wrapper. CMake target `tap::python` (`core/CMakeLists.txt`).
- **`core/tests/`** — the core's Catch2 battery against CPython 3.13, with Python fixtures in
  `core/tests/python/` and the shipped examples copied alongside. Runs on Linux, including under
  ASan/UBSan and TSan.
- **`core/bench/`** — `tap_python_bench` times `processor::process()` as Max's audio thread calls it
  (plan 6.3); `tap_python_reload_bench` times the audio buffers while the class reloads, with the
  audio thread scheduled as Core Audio's are (plan 2.6: why the runtime sets a 0.5 ms switch
  interval). `scripts/update-perf-docs.py` builds it (Release), runs it, and
  rewrites the book's generated performance tables (`book/src/performance.md`) — with `--max`, Max's CPU meter too, through
  `runtime-tests/run.py --session perf`. Never hand-edit those tables; measure on an idle machine.
- **`source/projects/tap.python_tilde/`** — one binary, two Max classes (D11), its `ext_main`
  registering both: the Min external `tap.python~` (`tap.python_tilde.h`) and, after it, `tap.python`,
  a plain SDK class (`tap.python.h`, the object; `tap.python.cpp`, the class and the C functions Max
  calls), which Max finds through the package's `init/tap.python.txt` (`max objectfile tap.python
  tap.python~;`). Both stand on the shared glue (plan 9.2): `tap.python_glue.h` — starting the
  runtime, atom conversion, the reserved names (`reserved_messages(audio)`) and the guard, and
  mapping the core's descriptions onto `object_addattr`/`object_addmethod`, with the C trampolines
  instantiated per host type (`python_glue<Host>`, through `Host::self()`) — plus the package paths
  (`tap.python_package.h`) and the file watcher (`tap.python_filewatch.h`). Its `_test.cpp` drives
  the Max glue of both through min-api's mock kernel, with stubs for what that kernel lacks made
  faithful to the SDK (the obex's dumpout, `outlet_insert_after`, the box's dynlets).
- **Documentation in Max's own system** (plan 9.4) — `docs/tap.python~.maxref.xml` (min's, never by
  hand) and `docs/tap.python.maxref.xml` (by hand: a plain SDK class has no generator); the package
  topic in `docs/topics/`, the guide *Writing Max Objects in Python* in `docs/vignettes/`, and three
  tutorials with their patchers in `docs/tutorials/pythontap-tut/` — the Documentation window's
  Package Docs › PythonTap lists them under Topics, Guides and Tutorials, by those folders; the help
  patchers `help/tap.python~.maxhelp` and `help/tap.python.maxhelp`, in tabs (`[p name]` with
  `showontab`, and an empty `[p ?]` that Max fills); and `extras/PythonTap Overview.maxpat`, in the
  Extras menu and the package's home patcher (`package-info.json.in`). The Documentation window
  lists `tap.python` as a reference page, not an object — it has no file of its own, as Max's
  `init`-mapped objects (`mc.abs~`) have not. `scripts/check-docs.py` (CI) checks them all: the XML
  well-formed, every link resolving, every patch cord whole.
- **`book/`** — the book (plan 9.7), mdBook 0.4.40: `book/src/SUMMARY.md` and a chapter per
  topic. The examples and the CHANGELOG are included as they are (`{{#include}}`), never copied; the
  notebook page is rendered from `python/allpass-doc.ipynb` by `scripts/notebook-to-book.py` before
  `mdbook build book` (generated, gitignored); the performance tables are written by
  `scripts/update-perf-docs.py`. `scripts/check-docs.py` checks every chapter is listed and every
  include and relative link (the ReadMe's too) resolves. CI's `book` job builds it, warnings as
  errors; `book-pages.yml` publishes it from `main`. The favicons in `book/theme/` are TapHouse
  copies (`sync.sh --icon PythonTap`), drift-checked like the rest.
- **`init/tap.python.txt`** — read by Max at launch: `tap.python` and `mc.tap.python~` are mapped to
  the `tap.python~` binary (the second through Max's MC wrapper, as Max's own `init/` maps
  `mc.cycle~`); `assemble-package.py` fails a package without both lines.
- **`python/`** — the user's script folder in the package (and the examples: `default.py`,
  `numpy_gain.py`, `allpass.py`, `numpy_allpass.py` — the same filter per vector, checked equal
  sample for sample by the core battery — `stereo_width.py`, two inputs and two outputs, and
  `allpass-doc.ipynb`; and for `tap.python`, without audio: `euclid.py`, `scale.py`, `note_name.py`,
  and `default.py`'s `bang`, which outputs the gain).
- **`runtime-tests/`** — tests that run inside a real Max (plan 6.1): Cycling '74's max-test harness
  (submodule), patchers generated by `make_patchers.py` (edit the generator, never the patchers),
  fixtures (`python/maxtest_*.py`, copied into `python/` for a run) and `run.py`, which installs the
  harness into Max's Packages, launches Max, drives it over OSC and reports — `tap.python~`'s tests
  and `tap.python`'s (`tap.python.*`, plan 9.5; `tap.python.load` opens first, in a Max with Restore
  Windows on Launch turned off for the run, so that `init/` must find the object). macOS, local only —
  `runtime-tests/README.md` has what the first runs taught about writing one. Two sessions run only
  when asked for: `soak` (plan 6.2, an hour) and `perf` (plan 6.3). `runtime-tests/spike/` is the
  record of the 9.0 spike for `tap.python` — its patchers, runners and Max's logs; the spike's class
  was removed when 9.3 built the real one, so running it again means checking out a commit before
  9.3 — and its answers are in `docs/TAP-PYTHON-PLAN.md` under 9.0.
- **`scripts/install-runtime.{sh,ps1}`** — installs python-build-standalone CPython 3.13 plus attrs and
  numpy into `support/` (gitignored), verified against `scripts/runtime.lock` (per-platform archive
  SHA256s) and `scripts/requirements.lock` (pip `--require-hashes`, wheels only). Never hand-edit the
  locks: `scripts/update-locks.py` regenerates them (moving a pin is deliberate — D4). CI installs from
  the same locks, and caches `support/` keyed on them. The macOS/Windows build links against it —
  weakly on macOS, delay-loaded on Windows, so the external loads without it and says so.
- **`scripts/assemble-package.py`** — builds the release package (platform externals, help, docs,
  examples, runtime, and `licenses/`: every third-party license that ships, collected from the
  actual contents); **`scripts/release/`** — the macOS/Windows signing scripts `release.yml` runs
  when its secrets exist.

## Build & test

The fast loop is Linux, no Max and no runtime install needed — just a CPython 3.13 with headers
(without one, `uv python install 3.13`, then `-DPython3_EXECUTABLE=$(uv python find 3.13)`):

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

In Max, on a Mac (quit Max first; ~2 minutes; see `runtime-tests/README.md`):

```sh
python3 runtime-tests/run.py
```

CI (`build.yml`): `linux-core` and `linux-max-glue` (each release, asan-ubsan, tsan — the glue
test's `TAP_PYTHON_SANITIZE`, as the core's; the release row also runs `scripts/check-docs.py`), `book`, `macos` (universal + `lipo`/`otool` checks, including
no absolute rpath, and the package assembled, which fails without `init/tap.python.txt`), `windows`. `style.yml`: TapHouse drift check,
clang-format, clang-tidy (a clang-tidy failure or crash fails the gate, not just a finding). Workflows
run with `contents: read`, pin third-party actions by commit SHA (tag noted beside it), and cancel
superseded runs. `release.yml`: on a `vX.Y.Z` tag, builds, tests, packages (macOS per architecture,
on runners of that architecture — the runtime is per arch, only libpython is universal), signs when
the secrets exist, merges the three into one package for every platform (`assemble-package.py
--merge`: each runtime in `support/<platform>`, where the external looks before `support/`), and
attaches all the zips + SHA256s to a release — a pre-release for 0.x, a draft from 1.0.
`book-pages.yml`: on a push to `main` that changes the book or what it includes, builds it and
deploys it to GitHub Pages (`pages: write`, `id-token: write` — the one workflow that writes).

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
- **`tap.python`'s messages** run on the thread they arrive on — the main thread, the scheduler
  thread under Overdrive, or the audio thread with Scheduler in Audio Interrupt on (said once per
  session, from the main thread by a qelem; plan 9.0 found `systhread_isaudiothread()` true exactly
  then). The method runs under the GIL; its result is output after the GIL is released, under the
  object's own recursive lock, which also guards every change to its outlets on a reload, and a
  result for more outlets than the object has then is dropped and said once per load. The lock is
  never held across the object's own call into Python, and never taken with the GIL held — the
  guard runs inside `load()` with the GIL and takes none — so the order is always the object's lock,
  then (downstream) the GIL. The shared glue's attribute and message maps have a lock of their own,
  held for lookups and changes only.
- **Worker mode** (`@mode worker`, plan 2.5): `process()` runs on the worker's own thread instead,
  and the audio thread only copies vectors through `worker`'s lock-free ring — no GIL, no
  allocation, no Python. The worker thread must have audio-thread scheduling (the wrapper's
  `audio_thread_scheduling`, through the core's `thread_setup` hook): an ordinary thread was late
  even at 21 ms of latency on a busy machine. `worker::start()`/`stop()` join threads, so never call
  them holding the GIL.
- **Nothing prints on the audio thread.** Problems in `process()` are recorded (exception object,
  flags) and the host's `report_ready` callback fires — the external sets a `queue<>` — and
  `flush_reports()` prints them on the main thread. Anything new on the audio path follows suit.
- **No CPython data symbols in the core or the wrapper** — `Py_None`, `Py_True`/`Py_False`,
  `PyExc_*`, `Py*_Type`, and the macros that expand to them (`Py_RETURN_NONE`, `PyFloat_Check`,
  `PyMethod_Check`, …). The Windows external delay-loads python3xx.dll, which MSVC refuses (LNK1194)
  if the module imports CPython data. Use the function forms (`detail::none()`,
  `detail::set_runtime_error()`, `Py_GetConstantBorrowed`, attribute lookups); CI's
  `scripts/check-data-imports.sh` fails the Linux build, by symbol name, on any regression.
- Max calls some methods directly with C arguments (`A_CANT`): the file watcher's `filechanged`
  is one. min registers a `message<>` whose name it does not special-case with its `A_GIMME` wrapper,
  which such a call crashes — so
  the watcher is owned by a nobox helper with the SDK's signature (`tap.python_filewatch.h`).
  Check the SDK's calling convention before exposing a Max-called method as a `message<>`.
  And read the SDK's return contract before testing a Max call's result: `object_getmethod()`
  answers `method_false()`, a function, not null, for a name an object does not have — a null
  test reserved every Python name in 1.0.0. **Never compare a function pointer Max returns with
  the address of a function this module imports:** across a DLL boundary the module's
  `&method_false` is its own import thunk (the SDK declares it without `dllimport`), so 1.0.1's
  comparison reserved every name again, on Windows only. The glue asks Max what it answers for a
  name no class can have (`not_found_method()`) and compares with that (`found_method()`, 1.0.2).
  And what the object itself registers can answer its name too — its *attributes* do, and the guard
  must leave them out, or a reload reserves every field (1.0.1, found in Max); its messages, added
  with `object_addmethod()`, answer as unknown (the 9.0 spike, on both platforms), and are left out
  as well. The mock kernel is thinner than Max: when a stub decides a behavior, make the test's stub
  faithful to the SDK (as `attr_args_offset` and `object_getmethod` are — the glue test's kernel
  answers an unknown name with a function the module cannot take the address of, as Windows does,
  and an added attribute's name, as Max does), and give each fake function a body of its own: MSVC's
  Release link folds identical functions (`/OPT:ICF`), which once gave a fake "found" method
  `method_false()`'s address on Windows only. Three releases in a row shipped what only a host
  platform could show, and the third was Windows-only: a change to Max glue is verified in Max on
  **both** platforms before it is tagged.
- **Say what is true of a class once, what is true of an instance per instance.** Many objects can
  share one class file; only the processor whose `load()` ran the file (`load_script` says so)
  announces the class — its `Loaded` line and its diagnostics — through `announce()` (plan 6.7).
  Use `log()` only for what concerns one instance. A file that fails to load is reported once too:
  the loader marks a failure of the same source it reported under two seconds ago (6.10). Tests
  that load a file again and check what was announced or reported call `forget_loaded()` first.
- Report user exceptions with `tap::python::report_exception()`, **never `PyErr_Print()`**: for a
  `SystemExit` it calls `Py_Exit()` and quits Max. Never finalize the interpreter, and never let a C++
  exception cross a Max callback (the trampolines are wrapped in `guarded()`).

## Conventions

- **Honest limits are pinned, not hidden.** A known bug or limit gets a test that states it (named
  for the promise, with the plan item that will change it); fixes land against a test that reproduces
  the bug first. The core battery is where that happens. Limits no test can pin are written in the
  book's Errors and limits chapter and stay there: the guards catch every Python *exception*, not a
  process exit below Python (`os._exit()`), a crash in a C extension, or code that never returns;
  helper modules load once per session; a second embedded CPython in the same Max is unsupported.
  Never let a document claim more than that.
- **Style:** `STYLE.md`, `.clang-format`, `.clang-tidy`, `.pre-commit-config.yaml`, `scripts/tidy.sh`
  and the SessionStart hook are canonical TapHouse copies — never hand-edit them (CI drift-checks).
  New files use the STYLE.md §3 SPDX banner. clang-tidy compiles with a clang front end; treat it as a
  second compiler. `style.yml` lints the external's TUs at C++20 against 3.13 headers and the core's
  tests through its compile database.
- **C++20** everywhere; the root `CMakeLists.txt` forces it on every object and `_test` target (Min pins
  C++17), as TapTools-Max does.
- **Keep in sync when behavior changes:** the book's chapters (and `ReadMe.md`, if its quick start
  or install steps change), the object's min metadata
  (`MIN_DESCRIPTION`, argument and message descriptions — min regenerates
  `docs/tap.python~.maxref.xml` from them when Max loads an external newer than the page, so never
  hand-edit the page: rebuild, run Max once — `runtime-tests/run.py` says when the page was
  rewritten — and commit it), `docs/tap.python.maxref.xml` (by hand), the guide and tutorials in
  `docs/`, both help patchers and the Overview — checked open in Max, not only by
  `scripts/check-docs.py` — the examples and their notebook (committed executed), and the plan.
- **Implement from documentation and published sources only** — the CPython C-API docs, the Max SDK
  docs — never by reverse-engineering another product.
