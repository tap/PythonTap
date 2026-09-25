# tap.python~ — production-readiness plan

Drafting record for taking `tap.python~` from a working sketch to a shippable 1.0. It grew out of
a four-dimension audit (threading/RT safety, CPython C-API use, build/CI/supply chain,
tests/docs/conventions) in September 2026; the findings are summarized per phase below with
file:line anchors as of commit `b6dc003`. Keep this file current as phases land — tick items and
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

## Phase 0 — a test foundation that can fail

Every later phase needs proof, and today the one unit test passes on either branch.

- [ ] **0.1 Injectable package root.** Under `MIN_TEST`, let the test set the package root and
  scripts folder (e.g. `TAP_PYTHON_PACKAGE_ROOT`). Today `package_root()`
  (`tap.python_tilde_runtime.h:52-59`) walks five levels up from the test binary on macOS and
  lands outside the repo, so the macOS job never starts Python.
- [ ] **0.2 No either-way assertions.** When `support/` exists, `REQUIRE(Py_IsInitialized())`;
  drop the silence branch in `tap.python_tilde_test.cpp:68-81`.
- [ ] **0.3 Linux test leg (spike).** Try building the mock-kernel tests against the system
  `python3-dev` (the tidy job already uses it). If min-api's mock kernel builds on Linux, this
  gives fast CI, ThreadSanitizer, and a local loop in cloud sessions; if not, record why.
- [ ] **0.4 Coverage of the public surface.** `attr_set`/`attr_get` round-trip, `message_gimme`
  (int/float/symbol/bang, wrong arity), reload from a temp scripts dir (syntax error → silence,
  fix → audio resumes, `process` raises → silence until reload, non-number return, missing class).
- [ ] **0.5 `pytest` for the Python side** — the examples and any conversion logic that can be
  exercised outside Max.
- [ ] **0.6 `CLAUDE.md`** — runtime prerequisite, the class contract, GIL/thread rules, TapHouse
  sync rules, and what must move together (maxref, help patcher, notebook, this plan).

## Phase 1 — crash and safety fixes

| # | Fix | Anchor | Pinned by |
|---|---|---|---|
| 1.1 | One `report_exception()` replacing all `PyErr_Print()` calls: `SystemExit` is reported, never honored (CPython's `PyErr_Print` calls `Py_Exit`), tracebacks formatted via `traceback`, no `sys.last_exc` retention | `tap.python_tilde.h:133,145,165,221,257,332` | `sys.exit()` in a method, in `process()`, at module top level → process survives |
| 1.2 | Reload/perform race: the eval loop drops the GIL every 5 ms, so `update_source()` can clear `m_process_fn`/`m_process_args` mid-vector and the next `PyTuple_SetItem(nullptr, …)` segfaults. Take strong local refs per vector, call via `PyObject_Vectorcall` (no shared args tuple), build the new binding fully before swapping | `tap.python_tilde.h:115-128, 318-347` | Slow `process()` + reload in a loop never crashes |
| 1.3 | Reserved message names (`filechanged`, `dsp64`, `notify`, `assist`, `anything`, `loadbang`, `dblclick`, …) are refused with a warning | `tap.python_tilde.h:419`, `_message.h` | A `filechanged` method is rejected; reload still works |
| 1.4 | Long-lived Python thread state for the audio and scheduler threads instead of `PyGILState_Ensure`/`Release` creating and freeing one per vector | `_runtime.h:64-76` | `threading.local` survives across vectors |
| 1.5 | NULL-check every conversion before `PyTuple_SetItem`; hold a strong ref to the class across `__init__`; `try/catch` in every C trampoline | `tap.python_tilde.h:153-170, 203-213`; `.cpp` | Unit tests |
| 1.6 | Atomic `m_process_fn`/`m_instance` for the pre-GIL fast-path reads | `tap.python_tilde.h:177,228,274,313` | TSan on the Linux leg (if 0.3 lands) |

## Phase 2 — the real-time model

- [ ] **2.1 Nothing prints or allocates on the audio thread.** Errors raise an atomic flag and a
  `qelem`/`defer` reports them once from the main thread; non-finite output is zeroed and counted;
  a non-numeric return is reported once (today it silently becomes 0.0 every sample).
- [ ] **2.2 Block path (D1a).** Preallocated in/out numpy arrays wrapping the signal vectors,
  one call per vector; the per-sample path stays, documented as slow.
- [ ] **2.3 `prepare(self, sr, vs)` hook** from `dspsetup`, so classes know the sample rate and
  vector size (the allpass example assumes 48 kHz today).
- [ ] **2.4 Multiple inlets/outlets** — `process` arguments define signal inlets and a tuple
  return defines outlets, fixed at construction.
- [ ] **2.5 Worker mode (D1)** — `@mode worker`: Python on a worker thread, lock-free FIFO,
  latency reported to Max, underrun → silence.
- [ ] **2.6 Shorter reload stalls** — compile outside the swap and hold the GIL only for the swap.

## Phase 3 — the type bridge and class contract

- [ ] **3.1 Signature-based dispatch** (`inspect.signature`): required/optional arity, defaults,
  `*args`, keyword-only parameters; unannotated parameters convert by incoming atom type. Today
  arity is the count of *annotated* parameters (`tap.python_tilde.h:190, 522-540`).
- [ ] **3.2 Types** — `bool` → long attribute; unwrap `Optional`/`Union` via
  `typing.get_origin`/`get_args` (today they fall through to symbol and store `""`);
  `list[float]` → list attribute; 64-bit ints (`PyLong_FromLongLong`, `t_atom_long`) so Windows
  doesn't truncate; getters never report `-1` as a value.
- [ ] **3.3 Hint failures are reported**, with a per-field fallback via
  `inspect.get_annotations`, instead of silently producing no attributes
  (`tap.python_tilde.h:366-368, 437-440`).
- [ ] **3.4 Reload reconciles attributes** — delete removed fields, re-type changed ones, and
  carry the patcher's current values onto the new instance (`tap.python_tilde.h:392-395, 424`).
- [ ] **3.5 File-based loading (D2)**; the source argument must be an identifier.
- [ ] **3.6 One reload per module** shared by all instances of that file, debounced (today each
  instance's watcher re-executes the module).
- [ ] **3.7 Console streams** — a real `flush()`, plus `encoding`/`errors`/`isatty` for libraries
  that probe them (`_runtime.h:184-194`).
- [ ] **3.8 Namespace** — move `python_attr`, `python_message` and the trampolines into
  `tap::python`; correct TapHouse's README, which says PythonTap has no namespace of its own.

## Phase 4 — build, supply chain, distribution

- [ ] **4.1 CMake** — the macOS arch detection caches with `FORCE` and never re-detects
  (`CMakeLists.txt:26-42`): re-detect while the value still equals the auto value, and fail when
  the requested archs are wider than `lipo -archs` of the runtime. Force C++20 on the test target
  (as TapTools-Max does). `BUILD_WITH_INSTALL_RPATH` so no absolute CI path is embedded.
- [ ] **4.2 Loadable without a runtime** — weak-link libpython on macOS, `/DELAYLOAD` the DLL on
  Windows, so the "run install-runtime" message can actually appear.
- [ ] **4.3 Installers** — commit per-triple SHA256s next to `PBS_RELEASE`; a hashed
  `requirements.lock` installed with `--require-hashes`; `$LASTEXITCODE` checks in the `.ps1`;
  extract to a temp dir and swap instead of deleting `support/` first; detect Apple Silicon under
  Rosetta (`sysctl hw.optional.arm64`); reject unknown flags; refuse non-macOS in the `.sh`.
- [ ] **4.4 CI hardening** — actions pinned by SHA, `permissions: contents: read`, concurrency
  cancellation, runtime download cache, no `|| true` swallowing tidy failures, tidy against 3.13
  headers.
- [ ] **4.5 Release workflow (D3)** — tag-triggered; assembles the full package per platform;
  signs/notarizes every Mach-O (Developer ID + notarytool) and Authenticode-signs Windows binaries
  **when the secrets exist, skipping cleanly otherwise**; attaches zips and checksums.
- [ ] **4.6 Licenses** — add the Max SDK, correct Min-API's copyright holder, list numpy's bundled
  OpenBLAS/gfortran licenses, and copy every license file into the package.

## Phase 5 — documentation and examples

- [ ] **5.1** Rewrite `docs/tap.python~.maxref.xml` against the real contract (it documents a
  nonexistent `greeting` attribute and a stale digest/see-also).
- [ ] **5.2** Rebuild `help/tap.python~.maxhelp` in Max 9 — basics, messages, allpass, hot
  reload, errors, `mc.` usage.
- [ ] **5.3** `ReadMe.md` — `@gain` not `gain`; exactly what reload keeps and resets; the threading
  and performance model; the block path.
- [ ] **5.4** Examples — `default.py` stops shadowing `float` in its own annotations;
  `allpass.py` uses `prepare(sr)` and an open α range (−1, 1); add a numpy block example;
  re-execute `allpass-doc.ipynb` (its outputs predate the current allpass and come from 3.8).
- [ ] **5.5** `CHANGELOG.md` recording each contract change (D5).

## Phase 6 — validation in a real Max

- [ ] **6.1 Runtime tests** with the `max-test` harness (as TapTools-Max does): load/unload,
  attributes/messages, reload under audio, 20 instances, `sys.exit()`/exception/NaN. Needs a
  licensed Max — a local on-Mac gate, not CI.
- [ ] **6.2 Stress/soak** — an hour of audio with a reload every second; many instances of one
  module; a sample-rate change mid-run.
- [ ] **6.3 Performance budget** — CPU per sample (per-sample path) and per vector (block path) at
  48/96 kHz, recorded in the ReadMe as measured numbers.

## Sequencing (one PR each)

1. Phase 0 — tests that can fail.
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
