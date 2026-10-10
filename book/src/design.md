# The design

For contributors: how PythonTap is put together, and why. The decisions are recorded with their
reasons in the repository — [`docs/PRODUCTION-PLAN.md`](https://github.com/tap/PythonTap/blob/main/docs/PRODUCTION-PLAN.md) (D1–D6, and
the phases that built 1.0) and [`docs/TAP-PYTHON-PLAN.md`](https://github.com/tap/PythonTap/blob/main/docs/TAP-PYTHON-PLAN.md) (D7–D11, the
second object) — and [`CLAUDE.md`](https://github.com/tap/PythonTap/blob/main/CLAUDE.md) keeps the rules a change must not break. This chapter is the map.

## The decisions

| | Decision | In short |
|---|---|---|
| D1 | Where `process()` runs | On the audio thread, once per vector (the block path) or per sample; `@mode worker` moves it to a thread of its own, the audio thread then never taking the GIL. |
| D1a | Choosing the block path | By type hint: `np.ndarray` parameters, once per vector; `float`, once per sample. |
| D2 | How a class is loaded | By file path, `python/<name>.py`, compiled and executed by the loader as a fresh module — never imported, no bytecode written. |
| D3 | The runtime | Bundled in every release: CPython, attrs and numpy in `support/`, per platform. |
| D4 | The Python version | Pinned to 3.13; a move is its own change, with tests. |
| D5 | Compatibility | Before 1.0, breaking changes allowed for correctness; from 1.0, a change to the class contract is a major version (2.0.0 was one). |
| D6 | Architecture | A host-independent core plus a thin Max wrapper. |
| D7–D11 | `tap.python` | The same core, folder, loader and contract without audio; a method's return value is the output, its return hint the outlet count; one inlet; a message runs on the thread it arrives on; one binary registers both Max classes, the second a plain SDK class found through `init/`. |

## The layers

- **The core** (`core/include/tap/python/`, namespace `tap::python`) — everything that talks to
  CPython, in plain C++20 with no Max: the one process-wide interpreter and its thread states, the
  loader, class introspection, the value model and its coercions (which reproduce Max's own
  `atom_getlong`/`atom_getfloat`/`atom_getsym`), message and attribute dispatch, `process()` per
  sample or per vector, the worker, and the conversion of what a method returns into output. Its
  Catch2 battery runs on Linux, under ASan/UBSan and TSan too.
- **The shared glue** (`source/projects/tap.python_tilde/tap.python_glue.h`) — what both Max objects
  need from Max: starting the runtime from the package, the file watcher, atom conversion, the names
  Max keeps and the guard against a class replacing one, and the attributes and messages made for a
  class, with the C trampolines Max calls, instantiated per host type.
- **The two objects** — `tap.python~`, a min object (`tap.python_tilde.h`), and `tap.python`, a plain
  SDK class (`tap.python.h`, `tap.python.cpp`), both registered by the one binary's `ext_main`.

## The rules that hold it together

- The interpreter starts once per process and is **never finalized**; every entry point takes a
  `gil_lock`. Holding the GIL does not serialize threads — CPython hands it over every switch
  interval, even mid-reload — so a reload builds the new binding completely and swaps it in with no
  Python call in between, and every caller takes its own references first.
- **Nothing prints on the audio thread**: problems there are recorded and printed later from the
  main thread.
- **No CPython data symbols** in the core or the wrapper: the Windows external delay-loads
  `python313.dll`, which MSVC refuses if the module imports CPython data. CI checks by symbol name.
- **What is true of a class is said once**, by the object that ran its file; what is true of an
  instance, per instance.
- **Honest limits are pinned, not hidden**: a known limit gets a test that states it, and the
  documentation never claims more than the tests show.

## Where the documentation comes from

This book is built from `book/` by CI and published on every change to `main`. The examples are
included from `python/` as they ship, the performance tables are written by
`scripts/update-perf-docs.py`, and the notebook is rendered from `python/allpass-doc.ipynb` at build
time — so none of them can drift from what ships. The object's reference pages, guide, tutorials and
help files, in Max's own documentation system, are in `docs/` and `help/`.
