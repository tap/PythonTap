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
- [ ] **2.4 Multiple inlets/outlets** — `process` arguments define signal inlets and a tuple
  return defines outlets, fixed at construction.
- [ ] **2.5 Worker mode (D1)** — `@mode worker`: Python on a worker thread, lock-free FIFO,
  latency reported to Max, underrun → silence.
- [ ] **2.6 Shorter reload stalls** — compile outside the swap and hold the GIL only for the swap.

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
- [ ] **3.8 Namespace** — move `python_attr`, `python_message` and the trampolines into
  `tap::python`; correct TapHouse's README, which says PythonTap has no namespace of its own.
  *The move is done (and the headers lost their file-scope `using namespace`); the TapHouse README
  correction is a separate PR in tap/taphouse.*

## Phase 4 — build, supply chain, distribution

- [x] **4.1 CMake** — the macOS arch detection cached with `FORCE` and never re-detected: it now
  remembers its own value and re-detects on every configure while `CMAKE_OSX_ARCHITECTURES` still
  holds it, so a reinstalled runtime (native ↔ universal) is picked up by an existing build folder;
  a value you set is kept but refused at configure if wider than `lipo -archs` of the runtime.
  (C++20 on the test target landed with Phase 0.) `BUILD_WITH_INSTALL_RPATH` on the external, so
  only the `@loader_path` rpath is embedded — CI now fails on any absolute rpath.
- [ ] **4.2 Loadable without a runtime** — weak-link libpython on macOS, `/DELAYLOAD` the DLL on
  Windows, so the "run install-runtime" message can actually appear. *Split out of the 4.1–4.4 PR:*
  CI can check the link shape but not that Max then loads the external, so it lands on its own,
  with a Phase 6 check in Max.
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
- [ ] **4.5 Release workflow (D3)** — tag-triggered; assembles the full package per platform;
  signs/notarizes every Mach-O (Developer ID + notarytool) and Authenticode-signs Windows binaries
  **when the secrets exist, skipping cleanly otherwise**; attaches zips and checksums.
- [ ] **4.6 Licenses** — add the Max SDK, correct Min-API's copyright holder, list numpy's bundled
  OpenBLAS/gfortran licenses, and copy every license file into the package.

## Phase 5 — documentation and examples

- [x] **5.1** Rewrite `docs/tap.python~.maxref.xml` against the real contract. *Done — at the
  source:* min regenerates the page from the object's metadata whenever the external is newer than
  it (so a hand edit is lost on the next build), so the contract now lives in `MIN_DESCRIPTION`
  and the argument and `filechanged` descriptions, and the committed page is the one min's own
  `doc_generate` writes from them (run against the mock kernel on Linux). The Python-defined
  attributes and messages depend on each user's class, so the page describes them generically.
- [ ] **5.2** Rebuild `help/tap.python~.maxhelp` in Max 9 — basics, messages, allpass, hot
  reload, errors, `mc.` usage. *Partly done by hand:* the patcher gained a title, the class
  contract, message boxes for `greet`/`float`/`int`/`filechanged`, and pointers to the
  `numpy_gain` and `allpass` examples (JSON checked: ids unique, every line connects). *Still
  wanted, in Max:* open, check the layout, re-save in Max 9, and consider tabs for the numpy and
  allpass examples and `mc.` usage.
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

- [ ] **6.1 Runtime tests** with the `max-test` harness (as TapTools-Max does): load/unload,
  attributes/messages, reload under audio, 20 instances, `sys.exit()`/exception/NaN. Needs a
  licensed Max — a local on-Mac gate, not CI.
- [ ] **6.2 Stress/soak** — an hour of audio with a reload every second; many instances of one
  module; a sample-rate change mid-run.
- [ ] **6.3 Performance budget** — CPU per sample (per-sample path) and per vector (block path) at
  48/96 kHz, recorded in the ReadMe as measured numbers.

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

## External prerequisites

- Apple Developer ID Application certificate + notarization credentials, and a Windows signing
  certificate, as CI secrets — not yet available; release artifacts are unsigned until then.
- A Mac with a licensed Max for Phase 6.
