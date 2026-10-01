# Changelog

Changes to the Python class contract and to the object's behavior, newest first. Before 1.0,
breaking changes to the contract are allowed where they buy correctness (D5 in
`docs/PRODUCTION-PLAN.md`); each is recorded here.

## Unreleased

### Changed — nothing of yours posts from the audio thread

- **What `process()` prints, or warns, is posted from Max's main thread.** A `print()` or a numpy
  `RuntimeWarning` in `process()` used to post to the console from the audio thread (or the worker
  thread), taking a lock on the way. Now a complete line printed on any thread but Max's main one
  is queued, lock-free and without allocating, and posted from the main thread a moment later;
  lines are assembled per thread, so two threads printing pieces of a line no longer mix them. A
  flood is coalesced: 256 lines queue, the rest are dropped and counted in one line. Lines printed
  on the main thread post at once, as before. (Plan 8.4, audit A5.)

### Changed — worker mode never waits forever

- **A `process()` that does not return when the worker stops is interrupted, then abandoned.**
  Stopping the worker — DSP off, a chain rebuild, the object deleted — waits up to 100 ms, then
  raises `WorkerStopped` (a `BaseException`) into the thread: a pure-Python loop ends, that vector
  is silence, the console says so once per load, and the class's audio stays bound. If it still has
  not returned 250 ms later it is blocked in a call Python cannot interrupt, and the thread is
  abandoned: audio continues on a new worker, the console says the thread keeps costing a core
  until Max quits, and the object keeps its Python state alive for it. Before, stopping the worker
  waited for `process()` to return, so such a class froze Max when DSP toggled or the patch closed.
  The worker thread also runs on a 16 MiB stack, what CPython gives its own threads on macOS, where
  the default 512 KiB was too little for Python that recurses through a C boundary. (Plan 8.3,
  audit A1 and A6.)

### Changed — more names are reserved

- **A method or field named like a message Max sends with C arguments is not exposed**, with the
  console saying so: `dspstate`, `fileusage`, `patchlineupdate`, `inputchanged`,
  `multichanneloutputs`, `edclose`, `okclose`, `oksize`, `paint`, `dictionary`, `getplaystate`,
  `key`, the mouse and focus messages, `mousewheel`, and the `*_setup` names — every message min
  treats as `A_CANT`, plus Max's own — and, as a guard, any name the Max object already answers
  itself. A Python method of such a name used to be registered as an ordinary message, which Max
  then called with a `long` or a pointer: the crash that every save once caused through
  `filechanged`, in its general form. (Plan 8.2, audit A3.)

### Fixed

- **Samples computed before `process()` raised are sanitized.** A `process()` that returned NaN for
  part of a vector and then raised let those NaNs through; the contract says non-finite output is
  replaced with 0.0, and now it is on that path too. (Plan 8.2, audit A2.)
- **The documents say what the guards cover.** No Python exception, `sys.exit()` included, takes
  Max down; what never reaches Python's exception machinery — `os._exit()`, a crash in a C
  extension, code that never returns — is not caught, and the ReadMe now says so, as it says that
  helper modules load once per Max session and that a second embedded CPython in the same Max is
  unsupported. `assemble-package.py --merge` refuses a zip whose entries or symlinks would land
  outside the package. (Plan 8.1.)

## 0.10.0 — 2026-10-01

### Added — one package for every platform

- **Each release also has `PythonTap-<version>.zip`, for every platform at once**: the macOS and
  Windows externals, and each platform's runtime in `support/<platform>/`. The external uses
  `support/<platform>/` when its package has one — on macOS, each architecture its own — and
  `support/` otherwise, so the per-platform packages and a checkout work as before. 0.x releases
  are published as pre-releases rather than drafts. (Plan 4.8.)

### Added — worker mode

- **`@mode worker` runs `process()` on a thread of its own**, `@latency` milliseconds behind the
  audio (30 by default, rounded up to whole signal vectors; it must exceed Max's I/O vector), so
  that nothing else Python does — a
  reload, a message, another instance — can hold up the audio thread, which only copies vectors to
  and from it. Vectors the worker is late for are output as silence, reported once per load, and
  the delay stays the same; the read-only `@latencysamples` gives it in samples. The worker thread
  has the real-time scheduling of an audio thread. `@mode direct`, the default, is as before.
  (Plan 2.5.) *Changed with it:* the object's own attributes `mode`, `latency` and
  `latencysamples` are reserved — a class's field or method of one of those names is not exposed,
  and the console says so (before, fields were never checked against reserved names).

### Added — several inputs and outputs

- **process() can take several signal inputs and return several outputs.** Its parameters are the
  object's signal inlets and its return hint its outlets: `process(self, left: np.ndarray, right:
  np.ndarray) -> tuple[np.ndarray, np.ndarray]` makes `[tap.python~ …]` with two of each, and
  `process(self) -> float` a generator (with one inlet still, for messages). The inputs are all
  `np.ndarray` or all per sample; a tuple return must say how many values it has. A return with the
  wrong number of values is output as silence and reported once. New example: `stereo_width.py`.
  A save that changes how many changes the object's inlets and outlets in place, keeping the patch
  cords of those that stay. (Plan 2.4.) *Changed with it:*
  `process()` taking `*args` is now reported and not bound (before, it was bound and its extra
  arguments never arrived).

### Changed — a broken file is reported once

- **A save that breaks a class file is reported once, however many objects share it.** Every
  object used to run the file and print its traceback: a syntax error with 25 objects printed 25
  tracebacks. Now a failure of the same source already reported in the last two seconds — the
  same save, or a patch opening many objects at once — is not reported again, while an object
  created later with the file still broken says why it is silent. Every load still runs the file,
  in case something it imports has been fixed. (Plan 6.10.)

### Changed — reloads and the audio thread

- **A reload no longer holds up the audio for its whole length.** The interpreter now hands itself
  to a waiting thread after 0.5 ms instead of CPython's 5 ms, so the audio thread waits at most
  about that long at a time while another thread — a reload, above all, which takes a few
  milliseconds — runs Python. Measured at 96 kHz with 512-sample buffers and a save every 100 ms,
  CPython's default made a buffer late in most runs (the worst took 45 ms); at 0.5 ms none was.
  (Plan 2.6.)

### Fixed — the package

- **The macOS external's bundle identifier** is `com.74objects.tap.python-tilde`; it shipped as an
  unexpanded template (`com.74objects.${PRODUCT_NAME:rfc1034identifier}`), which signing and
  notarizing would have refused. The development plan no longer ships inside the package, and the
  reference page now follows the object's descriptions (Max rewrites it from them).

## 0.9.0 — 2026-09-30

### Changed — the console on reload

- **One line per save, however many objects share the file.** Every object used to post two lines
  on every reload ("Source file update detected. Reloading." and "Audio process() bound: …"), so a
  save with 25 objects of one class posted 50 — and Max keeps every line it is sent. Now the object
  that runs the changed file posts one, `Loaded name.py: process() bound, one call per sample` (or
  per vector, or that nothing is bound), and what is true of the class — a type-hint error, a
  method skipped for its name, a `process()` that cannot be bound — is reported with it, once. A
  reload of an unchanged file posts nothing. Errors particular to one object are still reported by
  each. (Plan 6.7, found by the soak test.)

### Changed — text encoding

- **Text files are UTF-8 by default.** The interpreter now runs in Python's UTF-8 mode (PEP 540,
  Python's own default from 3.15), so `open()`, `Path.read_text()` and the like read and write
  UTF-8 unless given another `encoding`. Before, they used ASCII — whatever the machine's locale —
  and a class reading a UTF-8 file failed with `UnicodeDecodeError`. (Plan 6.6, found by the
  runtime tests.)

### Added — examples

- **`numpy_allpass.py`**: the allpass filter of `allpass.py`, processed a vector at a time with
  numpy — the same output, sample for sample, at a small fraction of the cost (the ReadMe's
  performance note compares the two).

### Fixed

- **Saving the class file no longer crashes Max.** Max's file watcher calls the object's
  `filechanged` method with C arguments (a file name and a folder), and the object registered that
  message in a form that reads its first argument as a Max symbol — so every save while the object
  existed crashed Max. The watcher now belongs to a small helper object that passes each save on
  as an ordinary `filechanged` message; sending `filechanged` from a patcher works as before.
  (Found by the first runtime test in Max; the unit tests' mock kernel has no file watcher.)

### Added — releases

- **Release packages with the runtime bundled** (D3): `PythonTap-<version>-macos-arm64.zip`,
  `-macos-x86_64.zip` and `-windows-x64.zip`, built by `release.yml` from a version tag into a draft
  release, with SHA256 checksums. Each carries a `licenses/` folder with every third-party license it
  ships. Not code-signed until signing credentials are configured.
- **The object loads without a runtime** and says in the Max console what is missing and how to
  install it, instead of Max failing to load the external. (libpython is weakly linked on macOS and
  delay-loaded on Windows.)

### Changed — documentation and examples

- **The reference page is generated from the object's own metadata** and now states the real
  contract (the argument, attributes from annotated fields, signature-called messages, the two
  `process()` forms, `prepare()`, hot reload, errors). Edit the descriptions in
  `tap.python_tilde.h`, not the XML: min regenerates the page whenever the external is newer.
- **`default.py`'s `int` message is annotated `int` and works as a number.** Its hint was written
  `float` after `def float`, so in the class body it named the method, not the type, and the
  argument arrived as a symbol. Its `int` and `float` methods now come last, with a comment saying
  why.
- **`allpass-doc.ipynb`** is rewritten as an executed document of the `allpass` example: what the
  object exposes, running it with `prepare()` as the object does, its unity energy and flat
  magnitude response, the delay following the sample rate, and the rejected coefficient.
- **The help patcher** explains the class contract and gains message boxes for the example's
  messages and for `filechanged`.

### Changed — types and messages

- **Messages follow the method's signature.** Parameters with defaults are optional, `*args`
  accepts any number of arguments, and unannotated parameters receive the value as the atom carried
  it. Previously the argument count had to equal the number of *annotated* parameters, so defaults
  and `*args` did not work and an unannotated parameter made a method uncallable. A method with a
  keyword-only parameter without a default is no longer exposed (it could never be called).
- **`bool` fields and arguments** are Max on/off longs and arrive in Python as `bool` (they were
  symbols, so `flag 1` stored `""`). **`Optional[X]` / `X | None`** count as `X` (they were symbols).
  `ClassVar` annotations are no longer attributes.
- **Classmethods and staticmethods** are called correctly (a classmethod received the instance as
  `cls`). Describing a class no longer runs its property getters.
- **Unresolvable type hints are reported** and read as written, instead of silently leaving the
  class with no attributes.
- **Attribute reads never invent a value**: `None`, or a value that does not convert, reads as the
  type's empty value instead of `-1`; an `int` attribute holding a float truncates; a symbol
  attribute holding a non-string reads through `str()`.
- **`sys.stdout`/`sys.stderr`** are proper text streams (`encoding`, `isatty()`, `fileno()` raising
  `io.UnsupportedOperation`); `flush()` forwards a partial line, and text containing NUL characters
  is written instead of raising.

### Changed (breaking) — loading and reloading

- **Class files are loaded by path**, as the private module `_tap_python_<name>`, instead of being
  imported through `sys.path`. The argument must be a valid Python identifier (e.g. `../x` or
  `two words` are refused), and it only ever names a file in the `python` folder:
  `[tap.python~ os]` no longer imports the standard library's `os`. A class file may now be named
  like a standard-library module without shadowing it.
- **The `python` folder moved to the end of `sys.path`**, so helper modules there are still
  importable but can no longer shadow the standard library or installed packages.
- **Attribute values carry over a reload** (they used to reset to the class defaults), including
  through a failed reload that you then fix. Attributes removed from the class are removed from the
  Max object, and one whose type changed is recreated with the new type (previously both stayed, with
  the old type).

### Fixed — loading and reloading

- A new object created after its file was edited ran the old code if another object had loaded that
  file earlier in the session.
- Two saves within the same second, of the same size, could reload stale compiled bytecode.
- A reload kept module-level names that had been deleted from the file.
- With several objects on one file, each save executed the module once per object; it now executes
  once.

### Added

- **numpy block processing.** A `process(self, x: np.ndarray) -> np.ndarray` is called once per
  signal vector with a (reused) numpy array, instead of once per sample. The result may be any
  numeric dtype or a list; it is converted. New example: `python/numpy_gain.py`.
- **`prepare(self, sample_rate: float, vector_size: int)`**, called with Max's audio settings before
  audio starts, whenever they change, and on every reload before the new code runs.

### Changed

- `python/allpass.py` takes its sample rate from `prepare()`; its `fs` attribute is gone (it
  assumed 48 kHz unless set by hand). `alpha` must now be strictly between -1 and 1 (±1 put the
  filter's pole on the unit circle).
- Problems in `process()` are printed from Max's main thread, not the audio thread. A non-numeric
  result is now reported (once per load) instead of silently output as 0.0, and non-finite output
  (NaN, infinity) is replaced with 0.0 and reported once per load.

### Installation

- The runtime installers verify the CPython download against a SHA256 committed in
  `scripts/runtime.lock`, and install attrs and numpy at the versions and hashes pinned in
  `scripts/requirements.lock` (previously: the release's own checksum file, and whatever PyPI
  served that day). `scripts/update-locks.py` moves the pins.
- Re-running an installer no longer deletes the runtime first: the old one is kept until the new
  one is complete and restored if anything fails. The Windows installer now stops on a failed
  `tar` or `pip` instead of reporting success. On an Apple Silicon Mac, a Terminal running under
  Rosetta now installs the arm64 runtime Max needs. `install-runtime.sh` rejects unknown options.
- The `PBS_RELEASE`/`PYTHON_SERIES` environment overrides (and the `.ps1` parameters) are gone:
  the pins live in the lock files.

### Changed (breaking)

- **Reserved method names.** A method named like a message Max or the object handles itself —
  `anything`, `appendtodictionary`, `assist`, `dblclick`, `dsp`, `dsp64`, `dspsetup`,
  `filechanged`, `getvalueof`, `inletinfo`, `loadbang`, `notify`, `preset`, `savestate`,
  `setvalueof`, `signal` — is no longer exposed as a Max message; the console names it. Before,
  such a method replaced the object's own handler (a method named `filechanged` silently disabled
  hot reload). Rename the method to expose it.

### Fixed

- `sys.exit()` (or `raise SystemExit`) anywhere in user code — a message, `process()`, an attribute
  setter, the constructor, or module top level — no longer quits Max; it is reported like any
  other exception.
- A hot reload landing in the middle of an audio vector could crash Max. The audio vector now
  finishes on the code it started with, and a successful reload swaps in without interrupting the
  audio (it no longer outputs silence while the new code loads).
- The audio thread keeps one Python thread state for its lifetime instead of creating and
  destroying one every vector, so per-thread Python state (`threading.local`, decimal contexts)
  survives between vectors, and a vector no longer costs an allocation and an interpreter-wide lock.
- Integer attribute values and message arguments cross as 64-bit on every platform (on Windows they
  were truncated to 32 bits).
