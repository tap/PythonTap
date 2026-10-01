# tap.python~ — production-readiness plan

Drafting record for taking `tap.python~` from a working sketch to a shippable 1.0. It grew out of
a four-dimension audit (threading/RT safety, CPython C-API use, build/CI/supply chain,
tests/docs/conventions) in September 2026; the findings are summarized per phase below with
file:line anchors as of commit `b6dc003` (since Phase 0.1, the CPython-facing code those anchors
point at lives in `core/include/tap/python/processor.h` and `runtime.h`). Keep this file current as phases land — tick items and
note the PR that closed them.

## The bar

1. **User Python cannot take Max down.** No crash, no process exit, no hang from anything a user
   script does (`sys.exit()`, exceptions, non-numeric returns, NaN, pathological names).
2. **The audio thread is protected.** No crash, no allocation or console posting on the hot path,
   and a defined, documented behavior (silence, not a stall) when Python misbehaves.
3. **Every documented behavior is pinned by a test** — unit (mock kernel) where possible, in-Max
   runtime tests where not.
4. **A reproducible, verifiable package ships from CI**, with pinned and hash-checked dependencies.
5. **Behavior is validated in a real Max**, not only against the mock kernel.

## Decisions (settled)

| # | Decision | Choice |
|---|---|---|
| D1 | Where `process()` runs | **Direct now, worker later.** A block path called once per vector on the audio thread (zero latency); the per-sample path stays as a documented slow path; an opt-in worker-thread mode (`@mode worker`, lock-free FIFO, ≥1 vector latency, audio thread never takes the GIL) follows. |
| D1a | Opting into the block path | **By type hint.** `process(self, x: np.ndarray) -> np.ndarray` is called per vector; `process(self, x: float) -> float` per sample. Detected at bind time. |
| D2 | How `[tap.python~ name]` loads code | **File-based.** `python/<name>.py` is loaded by path (`importlib.util.spec_from_file_location`) under a private module name (`_tap_python_user.<name>`); `name` must be an identifier; `python/` is *appended* to `sys.path` so sibling-helper imports keep working without shadowing the stdlib. |
| D3 | How users get the runtime | **Bundled in the release.** CI assembles a complete per-platform package (externals, `support/` with CPython + attrs + numpy, `python/`, help, docs, licenses). `scripts/install-runtime.*` stays for source builds. Signing/notarization steps are wired but **skip cleanly until credentials exist** (none yet). |
| D4 | Python version policy | **Pin 3.13; upgrade deliberately.** One CPython minor per release; a move (e.g. to 3.14's deferred annotations) is its own PR with tests. No free-threaded or subinterpreter builds until numpy supports them. |
| D5 | Compatibility before 1.0 | **Breaking changes to the class contract are allowed** where they buy correctness (reserved names, file-based loading, signature dispatch). Each is recorded in a `CHANGELOG.md`; the shipped examples are updated in the same PR. |
| D6 | Architecture | **A host-independent core plus a thin Max wrapper**, the family's kernel/wrapper split (TapTools / TapTools-Max). Everything that talks to CPython — interpreter start-up, thread state, module loading, class introspection, value conversion, `process()` binding and reload, exception handling — lives in `core/` (`tap::python`, plain C++20 + CPython, no Max or min-api). The external maps the core's attribute and message descriptions onto `object_addattr`/`object_addmethod` and owns only Max concerns (package paths, the file watcher, atoms). Linux is the first test platform *for the core*: real audio/main threads and sanitizers in CI and in cloud sessions. A plugin front end (CLAP or VST3) is optional later work over the same core (Phase 7), not a test vehicle — the Max glue still needs its own tests. |

## Phase 0 — the core split and a test foundation that can fail

Every later phase needs proof, and today the one unit test passes on either branch (below). The
core split (D6) makes the CPython layer buildable and testable on Linux, so fixes can be
reproduced and pinned there first.

- [x] **0.1 Extract the core (D6).** `core/include/tap/python/` — `runtime.h` (initialization,
  `gil_lock`, the console module), `value.h` (the Max-atom value model and its coercion rules),
  `processor.h` (load/reload, introspection, attribute/message dispatch, `process()`). The
  extraction is behavior-preserving except where noted in its PR; the known bugs are fixed in
  Phase 1 against tests that reproduce them.
- [x] **0.2 Linux core battery.** `core/tests/` (Catch2, the family's FetchContent pin) against
  CPython 3.13 from `find_package(Python3)`: runtime start-up and the console, the coercion table,
  load/reload/error paths, attribute and message dispatch, `process()`, a real audio thread racing
  main-thread messages, and the shipped examples. A `linux-core` CI job, plus ASan/UBSan and TSan
  rows. The Max test target moved to C++20 with it (the core needs `std::span`).
  *Found on the way:* a newly created processor gets a module this process already imported,
  even if its file changed since (the first load goes through `sys.modules`) — pinned as an honest
  limit, fixed by file-based loading (3.5).
- [x] **0.3 A package root the Max test can find.** Under `MIN_TEST`, `package_root()` is the
  folder above the test binary (`<repo>/tests/`) on every platform; it also makes the loader's path
  absolute (a binary launched as `./name` resolved to an empty root). Before this, it walked five
  levels up from the test binary on macOS and landed outside the repo, so the macOS job never
  started Python. *Also:* the external and its mock-kernel test now build and run on Linux against
  a system CPython 3.13 (`linux-max-glue` in CI) — the spike this item used to be succeeded.
- [x] **0.4 No either-way assertions.** Every build has a runtime (configure fails without one), so
  the Max test now requires the interpreter to start and `python/default.py` to run.
- [x] **0.5 Max-glue coverage** — `attr_set`/`attr_get` round-trips through float, long and symbol
  atoms, multi-atom refusal, and the `float`/`int` messages and arity refusal through
  `message_gimme` (the C trampolines that forward to it are not driven directly yet; their
  per-type conversion is covered by the core coercion tests). Reserved-name refusal is tested
  with 1.3.
- [x] **0.6 `CLAUDE.md`** — runtime prerequisite, the class contract, GIL/thread rules, the
  core/wrapper split, TapHouse sync rules, and what must move together (maxref, help patcher,
  notebook, this plan).

## Phase 1 — crash and safety fixes

Each fix landed against a core test that reproduced the bug first (`core/tests/test_safety.cpp`):
before the fixes, `sys.exit(3)` in a message ended the test process with status 3, and reloading
while audio ran segfaulted in 5 of 5 runs.

| # | Fix | Pinned by |
|---|---|---|
| ✅ 1.1 | `report_exception()` (`runtime.h`) replaces every `PyErr_Print()`: it displays the exception via `PyErr_DisplayException` — a `SystemExit` is reported, never honored, and `sys.last_exc` is not retained | `sys.exit()` in a message, in `process()`, in an attribute setter, at module top level and in the constructor → the process survives and the traceback reaches the console |
| ✅ 1.2 | The reload/perform race: `load()` builds the new binding (instance, process function, attributes, messages) completely, then swaps it in with no Python call in between; `process()` holds its own references for the whole vector and calls through `PyObject_Vectorcall` (no shared args tuple); `call()`/`set_attribute()`/`get_attribute()` hold their own references and copies before anything that could yield the GIL. A successful reload no longer silences the audio | 200 reloads against a slow `process()` with a 1 µs switch interval: no crash, no silent or partial vector (40/40 runs; ASan/UBSan and TSan clean) |
| ✅ 1.3 | Reserved message names: the processor takes a host list and refuses those methods with a diagnostic; the external passes Max's (`filechanged`, `dsp64`, `notify`, `assist`, `anything`, …) | Methods named like reserved host messages are not exposed |
| ✅ 1.4 | `gil_lock` gives each thread one Python thread state for its lifetime (`detail::thread_state_keeper`), instead of `PyGILState_Ensure`/`Release` creating and freeing one per vector | `threading.local` on the audio thread survives across vectors |
| ✅ 1.5 | Conversions NULL-checked before use; a strong reference to the class across `__init__`; every C trampoline, the file-watcher handler and the perform routine catch C++ exceptions | Unit tests; review |
| ✅ 1.6 | `m_instance`/`m_process_function` are `std::atomic<PyObject*>` (written only under the GIL), so the audio thread's lock-free check and `loaded()` are well-defined | TSan on the `linux-core` leg |

## Phase 2 — the real-time model

- [x] **2.1 Nothing prints on the audio thread.** process() raising, a non-numeric or non-finite
  result, and a block of the wrong length are recorded on the audio thread (the exception object
  and flags, no allocation or printing) and the host's `report_ready` callback fires — in Max a
  `queue<>` (qelem) — so `flush_reports()` prints them from the main thread, each kind once per
  load. Non-finite output is zeroed. *Honest limit:* the per-sample path still allocates a Python
  float per sample (CPython's free list); the block path allocates nothing per sample on our side
  (what the user's numpy code allocates is theirs).
- [x] **2.2 Block path (D1a).** A first parameter hinted `np.ndarray` binds process() per vector:
  a processor-owned `np.zeros(vs)` input array, reused (its buffer held, so its memory cannot
  move), sized at `prepare()` and resized only if a vector arrives in another size; the result is
  read through the buffer protocol, or `np.ascontiguousarray(…, float64)` for other dtypes and
  lists; a wrong length or `None` is reported and silenced. Rebuilding the buffer while audio runs
  is swap-safe (200 `prepare()` calls against a running block path: no corrupt vector, TSan
  clean).
- [x] **2.3 `prepare(self, sample_rate, vector_size)`**, from min's `dspsetup`; also called on each
  newly loaded instance before it is published, so no vector ever runs on an unprepared instance.
  `allpass.py` uses it (and its α range is now open, per 5.4).
- [x] **2.4 Multiple inlets/outlets** — `process` arguments define signal inlets and a tuple
  return defines outlets. *Decided (2026-09-30):* when a save changes the
  counts, the object adapts its inlets and outlets in place with Max's dynamic inlets and outlets
  (`dynlet_begin`/`dynlet_end` around `dsp_resize` and `outlet_append`/`outlet_delete`, as
  Cycling '74 describes in its developer forum) rather than asking for the object to be
  re-created. Three steps: *(a) the core — done:* `processor::process()` takes N input and M
  output channels; the positional parameters are the inputs (all `np.ndarray` or all per sample;
  none makes a generator), the return hint the outputs (`tuple[float, float]` for two; a tuple of
  unsaid length, `*args` or mixed hints are reported and not bound); `input_count()` and
  `output_count()` tell the host; the host's channels are matched to the class's (a missing input
  reads as silence, an extra output is silent); a result of the wrong length is silenced and
  reported once per load. *(b) the Max object's inlets and outlets from the class at creation —
  done:* the constructor adds an `inlet<>` per input after the first (its help naming the
  parameter: `processor::input_names()`) and an `outlet<>` per output after the first, before min
  makes the Max ports from its lists; after a reload that changes the counts it says so, once, and
  the core matches the channels. New example `stereo_width.py` (two in, two out; `width`). Mock test:
  `[tap.python~ stereo_width]` has two of each and processes both (the test file now stands in a
  faithful `attr_args_offset`, and the object reads a one-symbol argument directly, not through
  `atom_gettext` — the mock's gives nothing); runtime test `channels` (stereo_width, a generator, a
  save that changes the shape). *(c) adapting them on reload (dynlets) — done:* after a reload that
  changes the counts, `adapt_ports()` finds the object's box (`#B`) and, between `dynlet_begin`
  and `dynlet_end`, sets the signal inlets with `dsp_resize` and deletes or appends outlets at the
  end (`outlet_nth`/`outlet_delete`, `outlet_append`); min's own inlet and outlet lists follow at
  once (its dsp64 reads a connection count for each, its assist the help text, now renamed with
  the parameters on every reload); then `dspchain_setbroken(dspchain_fromobject())` has the signal
  chain rebuilt. Without a box the object keeps its ports and says so, as in (b). Mock test: the
  dynlet calls recorded against a pretend box, for a save that grows, shrinks, renames and breaks
  the class; runtime test `channels`: grown from two to three, the old cords carry on and cords
  that `thispatcher` connects to the new inlet and outlet carry signal (this check fails against
  (b)); shrunk to one, the removed outlets' cords go with them.
- [ ] **2.5 Worker mode (D1)** — `@mode worker`: Python on a worker thread, lock-free FIFO,
  latency reported to Max, underrun → silence. *Design (decided 2026-09-30):*
  - **What it buys.** In direct mode the audio thread takes the GIL, so anything else holding it —
    a reload compiling a large file (2.6's limit), a message handler, a GC pass — delays the
    buffer. In worker mode the audio thread never takes the GIL or calls CPython: it copies
    vectors in and out of a queue, and Python runs on a thread of its own, a fixed latency behind.
  - **In the core** (D6), beside `processor`: a `worker` that owns the thread and a ring of slots,
    each one vector of every host input and output channel. The audio thread writes vector *k*'s
    inputs into a slot and reads vector *k − L*'s outputs; the worker takes slots in order and runs
    the existing `processor::process()` on them — per sample or per vector, with 2.4's channel
    matching — so the class contract does not change. Slot states are atomics (single producer,
    single consumer, no locks); the audio thread wakes the worker with a C++20 atomic notify, which
    never blocks. The worker's thread is created and joined on the main thread.
  - **Latency** *L* is whole vectors, set by `@latency` (in vectors, default 2), and reported in
    samples (*L* × vector size) by the read-only `@latencysamples`, which a patch can read to align
    other paths. Max has no documented call for an MSP object to report latency to the host, so
    none is used; the output is primed with *L* vectors of silence.
  - **Underruns.** A slot not done when its outputs are due is output as silence and counted; the
    worker still processes it when it gets there (so the class's state stays continuous) and
    discards its outputs, catching up through the backlog, so the latency stays *L*. If it falls
    a whole ring behind (the ring holds *L* + a margin), the oldest inputs are dropped, which the
    class sees as a gap. Both are reported once per load from the main thread (`flush_reports()`),
    never printed on the audio thread.
  - **`prepare()` and the mode.** The ring is sized when the chain compiles (`dspsetup`: vector
    size, and the object's inlet and outlet counts), with the worker stopped and restarted around
    it; `prepare()` runs as now, on the main thread. `@mode` and `@latency` take effect at the next
    compile — setting them marks the chain broken (as 2.4 does), so that is at once while audio
    runs.
  - **Reloads, attributes and messages** are unchanged: they take the GIL on the main or scheduler
    thread, and the worker picks up a new binding at its next vector, as the audio thread does
    now. An attribute change is heard *L* vectors later.
  - **Tests.** Core battery (Linux, under TSan too): worker output equals direct output delayed by
    *L* vectors, per sample and per vector, several channels; a worker stalled by a class that
    sleeps underruns to silence, is reported once, and comes back at the same latency; reload
    under a running worker; resizing in `prepare()`; destruction joins the thread. Runtime test in
    Max: `@mode worker` against `delay~` of *L* vectors, a reload under audio, and
    `@latencysamples`.
  - **Decided:** latency in whole vectors, default 2; an underrun is silence with the latency kept;
    one Python call per host vector — batching several per call would cut the per-call overhead
    further (worker mode's other win) at more latency, a later option (`@block`) if measurements
    show it pays.
- [x] **2.6 Shorter reload stalls** — compile outside the swap and hold the GIL only for the swap.
  *Measured first* (`core/bench/reload_bench.cpp`: an audio thread with Core Audio's real-time
  scheduling computes 512-sample buffers of 64-sample vectors at 96 kHz while the main thread saves
  and reloads the class every 100 ms): a reload of the allpass examples holds the GIL for about
  3.5 ms, and with CPython's 5 ms switch interval the audio thread waited for all of it — late
  buffers in most runs, the worst 45 ms (the reloading thread descheduled while holding the GIL).
  *Done:* the runtime sets a 0.5 ms switch interval (`runtime_options::switch_interval`): no late
  buffer in three runs, the 99th percentile 0.7–2 ms, the median unchanged. Splitting `load()` into
  phases was not needed for files this size: what a switch cannot interrupt is a single C call —
  compiling the file, above all — which is short for them; a very large class file would still
  compile in one piece (2.5, worker mode, is the answer to that).

## Phase 3 — the type bridge and class contract

- [x] **3.1 Signature-based dispatch** (`inspect.signature`): required/optional arity, defaults,
  `*args`, keyword-only parameters; unannotated parameters convert by incoming atom type. Today
  arity is the count of *annotated* parameters (`tap.python_tilde.h:190, 522-540`). *Done:* the
  support module's `describe()` (inspect.signature + hints) gives positional types, the required
  count and `*args`; messages call the bound method, so classmethods/staticmethods work; a required
  keyword-only parameter keeps a method from being exposed; `methods()` uses
  `inspect.getattr_static`, so describing a class runs no property getter.
- [x] **3.2 Types** — `bool` → long attribute; unwrap `Optional`/`Union` via
  `typing.get_origin`/`get_args` (today they fall through to symbol and store `""`);
  `list[float]` → list attribute; 64-bit ints (`PyLong_FromLongLong`, `t_atom_long`) so Windows
  doesn't truncate; getters never report `-1` as a value. *Done,* except `list[float]` attributes
  (still symbols — a list attribute needs its own Max-side design); `ClassVar` is not a field.
- [x] **3.3 Hint failures are reported**, with a per-field fallback via
  `inspect.get_annotations`, instead of silently producing no attributes
  (`tap.python_tilde.h:366-368, 437-440`). *Done:* the exception is displayed and the annotations
  are read as written (a string hint by name; `Optional[...]`/`| None` unwrapped).
- [x] **3.4 Reload reconciles attributes** — delete removed fields, re-type changed ones, and
  carry the patcher's current values onto the new instance (`tap.python_tilde.h:392-395, 424`).
  *Done:* the core snapshots the attribute values before a load (kept across a failed one) and
  sets them on the new instance before it is published — same name and type only; a value the new
  class rejects is reported. The external removes Max attributes the class dropped
  (`object_deleteattr`) and recreates retyped ones.
- [x] **3.5 File-based loading (D2)**; the source argument must be an identifier. *Done:* a loader
  created at start-up compiles `<scripts>/<name>.py` itself and executes it as a fresh
  `_tap_python_<name>` module registered in `sys.modules` (typing/attrs resolve annotations there);
  the scripts folder is appended to `sys.path`. Fixes the pinned "new processor reuses a stale
  module" limit.
- [x] **3.6 One reload per module** shared by all instances of that file, debounced (today each
  instance's watcher re-executes the module). *Done:* the loader caches modules by source bytes, so
  the first instance to load a save executes it and the rest reuse it — no timer needed.
- [x] **3.6a Bytecode staleness on fast saves.** Reload goes through the normal source loader,
  whose `.pyc` check is mtime (1 s resolution) + size, so two same-size saves within a second can
  reload stale bytecode. Disable bytecode writing for user scripts, or load them uncached (D2).
  *Done:* the loader compiles the source directly; no `.pyc` is read or written for class files.
- [x] **3.7 Console streams** — a real `flush()`, plus `encoding`/`errors`/`isatty` for libraries
  that probe them (`_runtime.h:184-194`). *Done:* `io.TextIOBase` subclasses; `s#` so NULs pass.
- [x] **3.8 Namespace** — move `python_attr`, `python_message` and the trampolines into
  `tap::python`; correct TapHouse's README, which says PythonTap has no namespace of its own.
  *The move is done (and the headers lost their file-scope `using namespace`), and the core also
  exports the conventional `tap::python` CMake alias (keeping `tap::python_core`); the TapHouse
  README correction — adding PythonTap to its namespace table — is a separate PR in tap/taphouse.*
  *Done there too:* TapHouse's README lists `tap::python` (tap/taphouse `349b1a4`).

## Phase 4 — build, supply chain, distribution

- [x] **4.1 CMake** — the macOS arch detection cached with `FORCE` and never re-detected: it now
  remembers its own value and re-detects on every configure while `CMAKE_OSX_ARCHITECTURES` still
  holds it, so a reinstalled runtime (native ↔ universal) is picked up by an existing build folder;
  a value you set is kept but refused at configure if wider than `lipo -archs` of the runtime.
  (C++20 on the test target landed with Phase 0.) `BUILD_WITH_INSTALL_RPATH` on the external, so
  only the `@loader_path` rpath is embedded — CI now fails on any absolute rpath.
- [x] **4.2 Loadable without a runtime** — libpython is weakly linked on macOS and delay-loaded
  on Windows, and the object checks the runtime before its first Python call, printing what is
  missing and how to install it. MSVC refuses to delay-load a DLL whose *data* a module imports, and
  the core used four (`Py_None`, `PyExc_RuntimeError`, `PyFunction_Type`, `PyMethod_Type`); it now
  reaches them only through function calls (`Py_GetConstantBorrowed`, a builtins lookup,
  attributes), and `scripts/check-data-imports.sh` gates the Linux build on that. On macOS the check
  reads the weak binding itself (dyld never binds it later), so a runtime installed while Max runs
  needs a restart; on Windows the DLL is loaded by full path from `support/`, so the next object
  picks it up. CI checks the link shape on both (`LC_LOAD_WEAK_DYLIB`, delay-load dependencies);
  that Max then loads the external without a runtime is a Phase 6 check.
- [x] **4.3 Installers** — `scripts/runtime.lock` (per-triple asset + SHA256) and
  `scripts/requirements.lock` (attrs/numpy wheels, `--require-hashes --only-binary :all:`), both
  generated by `scripts/update-locks.py`; no more trusting the release's own `SHA256SUMS` or
  unpinned PyPI, and no `pip --upgrade pip`. Both installers build the new runtime in a scratch
  folder, move the old one aside and restore it on any failure; the `.ps1` checks
  `$LASTEXITCODE` after every native command; the `.sh` detects Apple Silicon under Rosetta
  (`hw.optional.arm64`), rejects unknown flags and refuses non-macOS. Exercised on Linux with
  stand-ins for the Mac tools: both archives verified, a pip failure rolled back to the previous
  runtime, a tampered hash stopped before touching `support/`, a stale `support.previous` left
  alone.
- [x] **4.4 CI hardening** — third-party actions pinned by commit SHA; `permissions: contents:
  read`; superseded runs cancelled; `support/` cached on the lock files and installer; the Linux
  jobs install the examples' packages from the same lock; the tidy gate fails on a clang-tidy
  failure, not only on a finding (checked with a broken invocation); tidy against 3.13 headers
  landed with Phase 0.
- [x] **4.5 Release workflow (D3)** — `release.yml`, on a `vX.Y.Z` tag (or by hand, artifacts
  only): builds and tests per platform, assembles the package with `scripts/assemble-package.py`,
  zips it with SHA256 checksums, and attaches everything to a *draft* release. The runtime is per
  architecture (only libpython is universal, for linking), so macOS ships two packages — arm64 and
  x86_64, each with the universal external — built on runners of their own architecture, which also
  runs the tests on Intel. Signing (`scripts/release/sign-macos.sh`: every Mach-O, hardened runtime,
  timestamp, then notarytool + staple; `sign-windows.ps1`: Authenticode on every unsigned binary)
  runs when the secrets exist and is skipped with a warning when they do not (none exist yet); the
  scripts are parse-checked on every run. *Unexercised until the first tag:* the workflow itself,
  and signing until there are credentials — a Phase 6 item.
- [x] **4.6 Licenses** — `License.md` now lists the Max SDK, credits the Min-API to its authors,
  names numpy's bundled OpenBLAS/LAPACK/GCC runtime licenses, and fixes the Windows path of
  CPython's `LICENSE.txt`. Release packages carry `licenses/`: a copy of every license file that
  ships — Min-API, the Max SDK, CPython's, and each installed package's own (PEP 639
  `dist-info/licenses/`, where numpy lists what it bundles) — with an index, collected from the
  package's actual contents; CI runs the collection on Linux.
- [ ] **4.7 uv for development and release tooling** — *Discussed (2026-09-30):* use uv where it
  replaces work we do by hand; keep the shipped runtime a python-build-standalone archive pinned
  by SHA256 in `runtime.lock`. For uv: `uv pip compile --universal --generate-hashes` in place of
  most of `update-locks.py`'s PyPI handling; `uv pip install --python-platform <triple> --target …`
  to install any platform's wheels from any machine (what 4.8 needs); `uv python install 3.13` for
  the Linux fast loop's CPython with headers; `uv run` with inline script metadata for the
  scripts. Against using it for the runtime: `uv python install` pins only through the uv
  version, installs in its own layout, and may mark the interpreter externally managed (to
  check) — and the work that matters (the `@rpath` install name, re-signing, the universal
  libpython) is ours either way. Users never need uv; the ReadMe may mention
  `uv pip install --python support/bin/python3 …` beside pip.
- [ ] **4.8 One package for every platform on each tag** — *Decided (2026-09-30):* a tag also
  attaches a single `PythonTap-<version>.zip` holding every platform's external and runtime, and
  v0.x tags publish as pre-releases automatically (1.0 and later stay drafts until signing
  exists). Needs one runtime per platform side by side — `support/macos-arm64/`,
  `support/macos-x86_64/`, `support/windows-x64/` (one folder cannot hold both: Windows' `Lib/`
  and the Mac's `lib/` collide on a case-insensitive disk) — with the macOS external choosing by
  the architecture it runs as (each slice of the universal binary can carry its own rpath) and the
  Windows external loading `python313.dll` from its folder by full path before the first
  delay-loaded call (Max adds only `support/` itself to the DLL search path); and a last
  `release.yml` job that merges the platform builds into one `PythonTap/` and attaches it with
  its checksum (about 130 MB, against 37–55 MB per platform zip today).

## Phase 5 — documentation and examples

- [x] **5.1** Rewrite `docs/tap.python~.maxref.xml` against the real contract. *Done — at the
  source:* min regenerates the page from the object's metadata whenever the external is newer than
  it (so a hand edit is lost on the next build), so the contract now lives in `MIN_DESCRIPTION`
  and the argument and `filechanged` descriptions, and the committed page is the one min's own
  `doc_generate` writes from them (run against the mock kernel on Linux). The Python-defined
  attributes and messages depend on each user's class, so the page describes them generically.
- [x] **5.2** Rebuild `help/tap.python~.maxhelp` in Max 9 — basics, messages, allpass, hot
  reload, errors, `mc.` usage. *Done by hand:* the patcher gained a title, the class contract,
  message boxes for `greet`/`float`/`int`/`filechanged`, and pointers to the `numpy_gain` and
  `allpass` examples (JSON checked: ids unique, every line connects). *Checked in Max 9
  (2026-09-29):* the layout sits right and every message box works; kept as committed rather than
  re-saved. Tabs for the numpy and allpass examples and `mc.` usage would still be welcome.
- [x] **5.3** `ReadMe.md` — `@gain` not `gain`; exactly what reload keeps and resets; the threading
  and performance model; the block path. *Done* across Phases 1–3 and this pass.
- [x] **5.4** Examples — `allpass.py` takes its sample rate from `prepare()` and its α range is
  open (Phase 2); `numpy_gain.py` is the block example; `default.py` puts its `int`/`float`
  methods last (after `def float`, `float` in the class body is the method) with a comment saying
  why; `allpass-doc.ipynb` is rewritten as an executed document of the example — what the object
  exposes, `prepare()` then per-sample `process()`, unity energy and a flat magnitude response,
  the delay following the sample rate, α rejected at 1 — on CPython 3.13 with the locked attrs and
  numpy.
- [x] **5.5** `CHANGELOG.md` recording each contract change (D5) — started with Phase 1.

## Phase 6 — validation in a real Max

**Runbook for a session on a Mac with Max 9.** Up to Phase 5 everything was built and tested on
Linux (core battery under sanitizers, the external against min's mock kernel) and in CI (macOS and
Windows builds, link-shape checks). The first Mac session (2026-09-27, Max 9.0.8, Intel) set up the
package, added the runtime tests (6.1) and ran them green; they found a crash on every save of a
class file, now fixed. A second session (2026-09-29) made the hand checks of steps 2 and 4: all
passed; a third ran the soak (6.2) and measured performance (6.3). To continue:

1. *Set up.* Clone into (or symlink into) `~/Documents/Max 9/Packages/PythonTap` with
   `--recursive`, run `./scripts/install-runtime.sh` (native: fastest to build; add `--universal`
   only to check a universal build), then `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release &&
   cmake --build build && ctest --test-dir build`. The external lands in `externals/`. When Max
   loads an external newer than `docs/tap.python~.maxref.xml`, min rewrites the page from the
   object's metadata (6.8: the build dates the `.mxo` for it, and `run.py` says when it happened);
   commit Max's page when it differs. (A symlinked package works for externals, but Max loads a package's
   *extensions* only from a real folder — why `run.py` installs the harness as a copy.)
2. *5.2 — the help patcher.* Open `help/tap.python~.maxhelp`: its new boxes were added by hand
   (as JSON, in Max's layout), so check they sit sensibly and every message box works, then
   re-save it in Max 9 and commit. Tabs for the numpy and allpass examples and `mc.` are welcome.
   *Done 2026-09-29:* the layout and every message box check out; kept as committed (5.2).
3. *Run the runtime tests:* quit Max, `python3 runtime-tests/run.py` (see
   `runtime-tests/README.md`). It covers 6.1 and the macOS half of 6.4 below; `--session soak`
   (an hour) and `--session perf` cover 6.2 and 6.3.
4. *Behavior the tests cannot show* — check by hand and note the result here:
   - a `bool` field's attribute shows as on/off (a toggle in the inspector and in attrui);
   - removing a field while an attrui displays it *looks* right (the tests check that nothing
     breaks and the object keeps working);
   - saving the file repeatedly while audio runs gives no audible dropout beyond the swap (the tests
     check every sample the object outputs, not whether the audio device underruns while a reload
     holds the GIL — plan 2.6);
   - changing the sample rate in Audio Status calls `prepare()` again (the tests change it through
     `poly~`'s `up`, which does not touch the device).

   *Checked 2026-09-29, Max 9 on macOS: all four as described* — the toggle shows, the attrui
   removal looks right, no audible dropout while saving under audio, and `prepare()` runs again on
   a sample-rate change in Audio Status. Repeat them on the release packages (step 6).
5. *6.4 on Windows:* with `support/` renamed aside, create `[tap.python~]`: one console line naming
   the missing runtime; rename it back and create another: it should work without a restart.
6. *6.5 — the first release.* Tag a pre-1.0 version (`v0.9.0`), which runs `release.yml` for the
   first time, including the `macos-15-intel` runner; install each draft zip into `Packages/`
   (clearing quarantine per the ReadMe) and repeat step 4's quick checks. *v0.9.0 is tagged and
   its draft built; the Intel zip is checked (see 6.5).* For each other zip: unzip it into
   `Packages/` as `PythonTap` in place of the checkout (move the checkout's link out of
   `Packages/` meanwhile), then `python3 runtime-tests/run.py --package ~/Documents/Max\ 9/Packages/PythonTap`.

- [x] **6.1 Runtime tests** with the `max-test` harness — `runtime-tests/`: Cycling '74's max-test as
  a submodule, the test patchers generated by `make_patchers.py` from scripts of steps, fixtures in
  `runtime-tests/python/`, and `run.py`, which installs the harness, launches Max, drives it over
  OSC and reports from the harness's database (a local gate on a Mac, not CI). Nine patchers, 75
  assertions: loading (no argument, numpy, `@`-arguments, `int`/`float`), every attribute kind set
  and read through `getattr`, messages by signature (and a bad call reported), reload by message
  and by saving through the file watcher (values kept, a field removed under an attrui, a broken
  file silencing and its fix restoring audio and values), reload every 50 ms for 3 s on both
  paths with every sample checked, 20 instances on each path plus `poly~` churn between 20 voices
  and 1, `sys.exit()`/NaN/exception each reported once, and `prepare()` before the first
  `process()` and again on a sample-rate and a vector-size change. *Found:* every save of a class
  file crashed Max — min registered `filechanged` with its A_GIMME wrapper, which Max's file
  watcher calls with C arguments; the watcher now belongs to a nobox helper that forwards the save
  as a typed message (`tap.python_tilde_filewatch.h`). *Also found* 6.6.
- [x] **6.2 Stress/soak** — an hour of audio with a reload every second; many instances of one
  module; a sample-rate change mid-run. *Done:* `runtime-tests/run.py --session soak` (the patcher
  is generated with the others; not in the quick suite). Passed on 2026-09-29: 24 per-sample
  instances of one class in a `poly~`, one more at top level and 8 block-path instances of
  another, the device at 96 kHz; every second both files saved with a new revision and every
  instance told to reload; the `poly~` at twice the rate (`up 2`) for the middle twenty minutes.
  Every output sample exact in all three phases, the console clean, 3,600 saves running the module
  3,600 times (147,684 object reloads), Python's tracked objects flat (18.8–22.1 thousand, no
  trend). Max's memory grew 70 MB (615 → 685 MB, steadily): not the object's — the core alone
  stays flat over 75,000 reloads, and Max grows as much from `[print]` posting the same number of
  lines, cleared window or not — but Max's console keeping the two lines each object posts per
  reload (6.7). *Found on the way:* Max's file watcher coalesces saves made close together (saving
  once a second, about half are delivered, some seconds late) — now in the ReadMe.
- [x] **6.3 Performance budget** — CPU per sample (per-sample path) and per vector (block path) at
  48/96 kHz, recorded in the ReadMe as measured numbers. *Done:* `core/bench` times
  `processor::process()` as the audio thread calls it — interleaved rounds, each measurement's
  fastest kept and its median reported — and `scripts/update-perf-docs.py` writes the ReadMe's
  table from it; `--max` adds Max's own CPU meter under load (`run.py --session perf`) beside the
  benchmark's prediction. Measured: the per-sample bridge costs about 70 ns a call (a third of a
  percent of a core at 48 kHz), so a per-sample class's cost is its Python code (`allpass.py`,
  more than twenty times that); the block path's call is under a microsecond per vector. In Max the
  per-sample path matches the benchmark and the block path reads about half again more. Taken on
  a busy machine (the note records the load average); regenerate on an idle one. *Then:*
  `numpy_allpass.py`, the allpass written per vector — identical output, checked sample for sample
  in the core battery — measured beside `allpass.py`: 11× less CPU at 48 kHz and 18× less at 96 kHz
  in 64-sample vectors, and about 16× less by Max's own meter.
- [ ] **6.4 Loading without a runtime** (4.2) — with `support/` moved aside the external loads and
  says what is missing, on macOS and Windows; installing the runtime then works after a Max restart
  (macOS) or for the next object (Windows). *macOS: done* — `run.py`'s without-runtime session
  checks both halves, including that a runtime put back while Max runs still asks for a restart.
  *Windows: by hand* (runbook step 5).
- [ ] **6.5 The first release** (4.5) — tag a pre-1.0 version, check the draft's three zips install
  and run from `Packages/` on Apple Silicon, Intel (or Rosetta) and Windows; later, with
  credentials, that signed and notarized packages load without the quarantine step. *Tagged
  `v0.9.0` (2026-09-30):* `release.yml`'s first run passed all four jobs and made the draft (three
  zips, unsigned). *Intel checked:* the x86_64 zip, its checksum verified and unzipped into
  `Packages/`, passes the whole runtime suite on an Intel Mac with Max 9.1.5 —
  `runtime-tests/run.py --package` tests an installed package. *Still to check:* the arm64 zip on
  Apple Silicon, the Windows zip on Windows, and the quarantine step (a zip fetched with `gh` is
  not quarantined). *Found:* the release attached `SHA256SUMS` but not the per-zip `.sha256` the
  ReadMe promises (`release.yml` fixed; the draft's added by hand); the external's bundle
  identifier is min's template, unexpanded — `com.74objects.${PRODUCT_NAME:rfc1034identifier}` —
  to fix before signing and notarizing; and `docs/PRODUCTION-PLAN.md` ships inside the package.
  *Both fixed since:* max-sdk-base leaves the identifier for Xcode to expand, which no other
  generator does (every sibling Max package ships it the same way), so the object's CMakeLists
  expands it — `com.74objects.tap.python-tilde`; `assemble-package.py` leaves the plan out.
  *Observed, not explained:* with the release installed, Max's file database took 8–11 minutes to
  report ready at each launch (seconds with the linked checkout; Max had also just been updated to
  9.1.5 and rebuilt its database) — the runner no longer waits for it, as tests opened by name do
  not need it; and Max rewrote the installed package's reference page, which it never did through
  the linked checkout (a clue for 6.8).
- [x] **6.6 Text files default to ASCII** — found by 6.1: the interpreter is configured with
  `PyConfig_InitIsolatedConfig`, whose pre-configuration leaves the locale unconfigured and UTF-8
  mode off, so in Max `open()` and `Path.read_text()` without `encoding=` decode as ASCII, whatever
  `LANG` says (as in any host that leaves the C locale in place); a class reading a UTF-8 file
  fails with `UnicodeDecodeError`. Proposed:
  pre-initialize with UTF-8 mode on (PEP 686 makes it the default from Python 3.15), pinned first by
  a core test — a contract change for the CHANGELOG. *Done:* `initialize()` pre-initializes the
  isolated configuration with `utf8_mode = 1`; `test_runtime.cpp` checks the flag and that
  `open()` reads and writes UTF-8 without an `encoding`, and failed before the change.
- [x] **6.7 Console lines per reload** — found by 6.2: every instance posts two lines on every
  reload ("Source file update detected. Reloading." and "Audio process() bound: …"), so a save
  with 25 instances of a class posts 50. Max's console keeps every line, even after the window is
  cleared (about 0.24 KB each, measured), so an extreme reload rate grows Max's memory — 70 MB
  over the soak's 147,684 reloads, where a realistic rate (20 instances, a save a minute) costs
  about half a megabyte an hour. Proposed: one line per save of a file, from whichever instance reloads
  first, and the binding line only when it changes; a user-visible change for the CHANGELOG.
  *Done:* the processor whose `load()` runs the file (the loader reports it) posts one line,
  `Loaded name.py: …` with how `process()` is bound, and announces the class's diagnostics
  (hints, reserved names, an unbindable `process()`) with it; the others sharing the file post
  nothing, and the external's own reload line is gone. Errors particular to an instance are
  unchanged. Pinned by a core test (two processors, a change, an unchanged reload) and a runtime
  test (five objects in Max: the class's diagnostic once per run of the file).
- [x] **6.8 The reference page from Max** — runbook step 1 expects min to rewrite
  `docs/tap.python~.maxref.xml` when Max loads an external newer than it; in the Mac sessions it
  did not. Max's standard output had "file not found" and "failed to get date modified" lines at
  start-up — probably min's `doc_update` failing to resolve a path, not yet shown to come from this
  object. Find out why, and whether the committed page (generated against the mock kernel) is the
  one Max would write. *Done:* min's `doc_update` dates the external by its `.mxo` folder, and a
  rebuild changes only the files inside it — the checkout's folder dated from its first build
  (2026-08-05), older than the page, so the page never looked stale; a freshly unzipped release
  has a new folder, which is why Max rewrote the installed package's page. The macOS build now
  touches the folder after each link (a Windows `.mxe64` is one file, dated by its link already),
  and `run.py` says when Max has rewritten the page, to commit it. Max's page matches the committed
  one but for the description, which predated 2.4's text — now committed. The start-up lines are
  not this object's: they still appear while its page is written.
- [x] **6.9 The help patcher and `numpy_allpass.py`** — the help patcher points to the `numpy_gain`
  and `allpass` examples but not to `numpy_allpass`, the one that shows what the block path is for;
  add it in Max (and re-save), perhaps with the measured comparison. *Done:* the examples note names
  `numpy_allpass` as the same filter per vector, to swap in and watch the patcher's CPU meter (the
  ReadMe's tables are the measured comparison); and a new note says what 2.4 made true —
  `process()`'s parameters are the inlets, its return hint the outlets, `stereo_width` has two of
  each, and a save that changes them changes the object. Checked open in Max 9.1.5.
- [x] **6.10 One traceback per broken save** — a file that fails to load is not cached, so every
  object sharing it runs it and prints the traceback: 25 objects, 25 tracebacks. 6.7 left this
  alone, because the obvious fix — remember the failing source and stay quiet — would also hide
  the error from an object created later (a patch reopened with the file still broken). Needs a
  rule that reports once per save without that. *Done:* the loader still runs the file on every
  load, but marks a failure of the same source it reported less than two seconds ago
  (`_REPORT_WINDOW`), and the processor stays quiet for a marked failure — a save, or a patch
  opening many objects, reports once; an object made later reports again. Pinned by a core test
  (two processors; another broken save; a later object) and the runtime test `announce-once` (a
  broken save, five objects in Max, one report — five before the change).

## Phase 7 — plugin front ends (optional, post-1.0)

- [ ] **7.1** A CLAP (MIT, simpler) or VST3 wrapper over the core. Needs its own answer to fixed
  parameter lists — e.g. a fixed bank of N parameters the Python class declares — and to sharing
  one interpreter with other plugins that embed Python in the same host process.

## Sequencing (one PR each)

1. Phase 0.1–0.2 — the core split and the Linux battery; then 0.3–0.6.
2. Phase 1 — crash/safety fixes.
3. Phase 4.1–4.4 — build/installers/CI (independent; can run in parallel with 2).
4. Phase 2.1–2.3 — RT hygiene, block path, `prepare()`.
5. Phase 3 — type bridge (likely split: dispatch + types; reload reconciliation; file-based loading).
6. Phase 5 — docs/examples once the contract has settled.
7. Phase 4.5–4.6 — release packaging and licenses.
8. Phase 2.4–2.6 — multichannel, worker mode, reload stalls (features; may follow 1.0).
9. Phase 6 — in-Max validation before tagging 1.0.
10. Phase 4.7–4.8 — uv for the tooling, then one package for every platform on each tag.

## External prerequisites

- Apple Developer ID Application certificate + notarization credentials, and a Windows signing
  certificate, as CI secrets — not yet available; release artifacts are unsigned until then.
- A Mac with a licensed Max for Phase 6.
