# Changelog

Changes to the Python class contract and to the object's behavior, newest first. Before 1.0,
breaking changes to the contract are allowed where they buy correctness (D5 in
`docs/PRODUCTION-PLAN.md`); each is recorded here.

## Unreleased

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
