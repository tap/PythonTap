# tap.python — a Python class as a Max object without audio: design and plan

`tap.python~` runs a Python class as an audio object. `tap.python` runs one as an ordinary Max
object: messages in, messages out, no signal. This is its design — the decisions (D7–D11,
continuing `PRODUCTION-PLAN.md`'s D1–D6), what a class looks like, how the core and the package
change — and the plan to build it (Phase 9 of the production plan, which points here). Drafted
2026-10-02 against 1.0.0; nothing in it is built yet. Tick the items below (with the PR) as they
land, and keep the design current where a PR decides differently.

## What it is for

In Max, most objects are not audio objects: they transform messages — a list comes in, a list goes
out; a bang asks for the next value; an int becomes a name. Max gives three ways to write such an
object in a language: `js`, `node.script`, and a C external. `tap.python` is a fourth, with what the
`tap.python~` contract already gives — Python's numeric stack (numpy, and whatever is installed into
the runtime), attributes and messages from type hints, hot reload on save, tracebacks in the
console, and no way for the class to take Max down — and one thing more that an audio object does
not need: **what a method returns is what the object outputs.**

```python
from attrs import define, field

@define
class euclid:
    steps: int = field(default = 8)
    pulses: int = field(default = 3)

    def bang(self) -> list[int]:
        return [int((i * self.pulses) % self.steps < self.pulses) for i in range(self.steps)]
```

```
[tap.python euclid]   ← loads python/euclid.py; bang outputs a list of 0s and 1s; @steps and @pulses
```

Two things it is not. It is not a replacement for `js`: no access to the patcher, the Max API or
UI; it is for computing. And it is not an async object like `node.script`: a message runs to
completion, on the thread that sent it, and its result is output before the message returns — the
ordinary Max object model, which is what makes it composable with `trigger`, `metro` and the rest.

## Decisions

| # | Decision | Choice |
|---|---|---|
| D7 | What `tap.python` is | **A second external in the same package, over the same core, with the same class contract minus audio, plus output.** Same `python/` folder, same loader (a file saved once is executed once however many objects of either kind share it), same attributes, messages, hot reload, console and error guards. A class written for one object loads in the other: in `tap.python`, `process()` and `prepare()` are ordinary methods (sending `process 0.5` calls it and outputs the result — a way to test a filter sample by sample); in `tap.python~`, a method's return value is dropped, as now. |
| D8 | How output works | **A method's return value is output from the object's outlets, by its type; its return *hint* fixes how many outlets, at bind time.** No injected outlet API in 1.x (nothing for the class to import, so it still runs in a notebook): `None` outputs nothing; a number or a string one atom; a sequence a list; a method hinted `-> tuple[A, B]` outputs one value per outlet, right to left. The object has as many outlets as the widest return hint among its methods (at least one), plus a dumpout outlet at the right, as Max objects with attributes do; a save that changes the count changes the outlets in place, as 2.4 does for `tap.python~`. The rules are in [Output](#output-what-a-method-returns). |
| D9 | Inlets | **One inlet in the first release.** Messages are the methods; state is the attributes (`steps 8`, `@pulses 3`, attrui), which is what a right inlet is for in most Max objects. More inlets are a later option (how, in [Later](#later-not-planned)), not a 1.x promise. |
| D10 | Threads | **A message runs on the thread it arrives on — Max's main thread, or the scheduler thread under Overdrive — holding the GIL until it returns, and outputs on that thread.** As every ordinary Max object does, and as `tap.python~`'s messages already do. No worker. The honest limits: a long computation in a message from a `metro` holds Max's scheduler for its duration (send it through `deferlow` to run on the main thread instead); and while a message runs, a `tap.python~` in direct mode waits for the GIL in 0.5 ms slices (2.6) — worker mode (2.5) is the answer when both are in one patch and the computation is heavy. |
| D11 | Code layout | **The Max glue the two objects share moves out of `tap.python_tilde/` into `source/shared/`, templated on the host object; the core gains one option.** The dynamic attribute and message registration, the C trampolines, the file watcher and the package paths are the same for both; min's `SUBDIRLIST` makes a target per folder of `source/projects/` only, so a shared folder beside it is safe. The core's `processor` takes `bind_audio` (true for `tap.python~`): with it off, `process` and `prepare` are plain methods and nothing is prepared. Behavior-preserving for `tap.python~`: its tests do not change. |

## The class contract

Everything the ReadMe's *Writing a class* says holds, less the *Audio*, *Worker mode* and *Audio
settings* paragraphs, plus output:

- **Loading.** `[tap.python name]` loads `python/name.py` and instantiates `class name`; no
  argument loads `default`. File-based loading (D2), helpers (8.5), UTF-8 (6.6), one execution per
  save shared by every object of either kind (3.6, 6.7), one report per broken save (6.10).
- **Attributes** — as `tap.python~` (3.2, 8.6). With a dumpout outlet, `getsteps` outputs
  `steps 8` from it, as a Max object with attributes does.
- **Messages** — public methods, called by signature (3.1). `int`, `float`, `symbol`, `bang` and
  `list` answer those standard messages; a method named `anything` answers every message the class
  has no method for, with the selector as its first argument: `def anything(self, selector: str,
  *args)`. (In `tap.python~`, `anything` is reserved; here the object forwards it.) A parameter
  hinted `list[float]`, `list[int]`, `list[str]` or `np.ndarray`, last in the signature, takes all
  the remaining atoms as one list (or a float64 array) — `def list(self, values: np.ndarray) ->
  np.ndarray` is a list in and a list out. This last mapping lands in the core, so `tap.python~`'s
  messages get it too.
- **Output** — below.
- **Hot reload, errors, console** — as `tap.python~`: a save reloads keeping attribute values; a
  broken save is reported once and the object outputs nothing until a save fixes it; an exception
  in a method prints its traceback and the message outputs nothing; `sys.exit()` is reported, not
  honored; what is below Python (`os._exit()`, a crashing extension, an endless loop — which
  freezes the thread the message came on) is outside the guard (8.1).
- **Reserved names** — Max's and min's own (8.2's list and the `answered_by_max()` guard), without
  the audio ones: `dsp`, `dsp64`, `dspsetup`, `dspstate`, `inputchanged`, `multichanneloutputs`,
  `signal`, `mode`, `latency` and `latencysamples` are free; `anything` is taken by the forwarder
  above, as a method a class may define.

### Output: what a method returns

A method's return value goes out of the object, converted by the rules below, before the message
returns. The rules reproduce how Max objects and `js` behave, so a patch sees what it expects.

| The method returns | The object outputs |
|---|---|
| `None` | nothing |
| `bool` | an int, 0 or 1 |
| an `int` (anything with `__index__`: `np.int64`, an `IntEnum`) | an int (`t_atom_long`, 64-bit; a value past it is reported) |
| a `float` (anything with `__float__` and no `__index__`: `np.float32`) | a float (non-finite values pass through, as `js` passes them) |
| a `str` | the message named by it, with no arguments (`"hello world"` is one symbol, as in `js`) |
| a sequence (`list`, a `tuple` with no `tuple[…]` hint, a 1-D `np.ndarray`, any non-`str` iterable) | a list, each element an atom by the rules above; if the first element is a `str`, the message it names with the rest as arguments (Max's own rule: `["note", 60, 100]` outputs `note 60 100`); an empty sequence outputs nothing; an element that is not an atom (`None`, a nested sequence) is reported and the list is not output |
| a sequence of *n* values, from a method hinted `-> tuple[…]` with *n* members | one value per outlet, outlets *n*…1, right to left, each by these rules (`None` in a slot outputs nothing from that outlet); a result that is not a sequence of exactly *n* values is reported |
| anything else (a `dict`, an object) | reported, with its type; nothing output (a `dict` as a Max dictionary is a later item) |

The *hint* decides the outlet count when the class loads; the *value* fills them when the method
runs. A method hinted `-> tuple[int, str]` has two outlets, and so does the object if no method is
hinted wider; a method hinted `-> list[float]`, `-> np.ndarray`, `-> float` or not at all has one,
and an unhinted method that happens to return a tuple outputs it as a list from the first outlet.
An unhinted tuple return is a list, a hinted one is several outlets: this is the one place where
the hint changes what a value does, so the ReadMe says it in those words. A `tuple[…]` hint of
unsaid length (`tuple`, `tuple[int, ...]`) cannot name an outlet count: the method is not exposed,
and the console says why (as 2.4 does for `process()`).

A `print()` in a method posts to the console at once from the main thread and a moment later from
the scheduler thread (8.4); it is not output.

### Examples

`default.py` stays shared by both objects and gains a `bang` that returns the gain, so
`[tap.python]` with no argument outputs something. New examples, each a thing Max users write by
hand today:

- `euclid.py` (above): a Euclidean rhythm from two attributes, a list out of a bang.
- `scale.py`: a list in, scaled and offset by attributes with numpy, a list out — `def list(self,
  values: np.ndarray) -> np.ndarray`.
- `note_name.py`: an int in, two outlets out — `def int(self, note: int) -> tuple[str, int]`
  returns the pitch class and the octave, so `60` outputs `C` and `4`.

## Threads and the GIL

Nothing new is load-bearing, and that is the point of D10. A message takes a `gil_lock` on the
thread it arrives on (`processor::call()` already does), converts the arguments, calls the method, and
converts the result to atoms, all under the GIL; the `outlet_*` calls come after the GIL is
released. Not for reentrancy — `gil_lock` is `PyGILState_Ensure`, which nests, so a downstream
object sending a message back into this one (a loop through `trigger`) simply takes it again on the
same thread — but because an outlet call runs the whole downstream chain before it returns, and
holding the GIL across it would keep every other Python thread (a `tap.python~`'s audio thread in
direct mode) waiting for a cascade of Max objects that has nothing to do with Python. Reloads stay
on the main thread; a message that arrives on the scheduler thread while a reload runs waits for
the GIL, as a `tap.python~` message does now.
Output from a thread Python started (`threading.Thread` in the class) is not supported: a method
runs and returns on Max's thread, and there is no outlet to reach from anywhere else — stated in
the ReadMe as an honest limit, with the later item that would change it.

## The core

`core/include/tap/python/` grows, host-independent and tested on Linux first, as everything else
did (D6):

- **`value.h`** — beside `value` (one atom): `output_item`, a message to output — a selector or
  none, and its atoms (what `outlet_anything` / `outlet_list` / `outlet_int` take) — and
  `output`, the per-outlet items of one call. The conversion from a Python object to an
  `output_item` lives in the core (the rules above), with its errors as diagnostics through the
  processor's log, so the Max side only maps items onto `outlet_*` calls.
- **`processor.h`** — `message_info` gains `return_count` (from `describe()`'s `return_shape`,
  which the signature already carries); `outlet_count()` is the widest among the messages (at least
  one); `call()` returns the converted `output` (empty for `None` or a failure) instead of a bool,
  with a `call()` overload keeping the old shape for `tap.python~`; a `processor_options` (or a
  constructor flag) `bind_audio` — off, `process`/`prepare` are bound as messages, `has_process()`
  is false, `prepare()` is a no-op; argument conversion for a last parameter hinted `list[…]` or
  `np.ndarray`. The `Loaded` line (6.7) says what the loading object bound — "4 messages, 2
  outlets" for a `tap.python`, "process() bound, one call per vector" for a `tap.python~` — so
  with a file shared by both kinds it describes whichever ran the save; an honest limit of 6.7's
  rule, written down.
- **Tests** — `test_output.cpp`: every row of the table above, from a fixture class whose methods
  return each kind; the outlet count from the hints; an unhinted tuple as a list against a hinted
  one as outlets; an unsaid-length tuple hint not exposed; the `list[…]`/`np.ndarray` parameter;
  `bind_audio` off making `process` a message; a method raising outputs nothing and reports; a
  call from a second thread racing a reload on the first (as `test_threads.cpp` does for audio).

## The Max object

`source/projects/tap.python/` — `tap.python.h`, `tap.python.cpp`, `tap.python_test.cpp` — a min
`object<>` without `vector_operator<>`:

- **Ports.** One `inlet<>`; `outlet<>`s for the class's `outlet_count()` plus a last `outlet<>
  m_dumpout{this, "dumpout"}` stored in the obex (`object_obex_store(maxobj(), gensym("dumpout"),
  …)`) so that `get<attr>` outputs from it. On a reload that changes the count, dynamic outlets as
  2.4 does, without `dsp_resize`: between the box's `dynlet_begin`/`dynlet_end`, `outlet_delete`
  for the surplus and `outlet_insert_after` the last value outlet for the new ones (never
  `outlet_append`, which would land them after the dumpout), min's lists following; without a box,
  the object keeps its outlets and says so once. *To check in Max:* that min's `outlet<>` named
  `dumpout` is enough for `get<attr>` or whether the obex store is needed (min stores one for jit
  objects only, `max_jit_class_wrap_standard`).
- **Messages.** The shared `python_message` registration; `message_gimme()` calls the processor and
  maps each `output_item` onto `outlet_int`/`outlet_float`/`outlet_anything`/`outlet_list` on the
  right outlet, right to left. A min `message<> m_anything{this, "anything", …}` forwards an
  unknown selector with its atoms to the class's `anything` method when it has one, and otherwise
  posts that the object does not understand it (Max's own wording).
- **Reserved names.** 8.2's list less the audio names, and the `answered_by_max()` guard; `anything`
  is answered by the object (the forwarder), so it must not be reserved by the guard — the guard
  excludes it by name, and a glue test asserts a class's `anything` is exposed.
- **The rest** is the shared glue: the file watcher (`filechanged`, 6.1's nobox helper), the console
  (8.4's qelem), the package paths, attributes (3.4's reconciliation), `reserved_messages()` and
  `answered_by_max()`. `MIN_DESCRIPTION` is the contract (5.1): min writes
  `docs/tap.python.maxref.xml` from it.
- **Glue test** (mock kernel): `[tap.python euclid]` has one inlet, two outlets (one plus dumpout);
  a bang's list is in `object_getoutput(maxobj, 0)` (the mock records outlet sequences); a
  `note_name` int fills outlet 1 then outlet 0; a str return arrives as an `anything`; `getsteps`
  reaches the dumpout (if the mock routes it; else a runtime test); a `tuple[…]` save that widens
  the class records the dynlet calls as 2.4's test does.

## The package

- **Build.** The runtime discovery block of `tap.python_tilde/CMakeLists.txt` (support/, weak link,
  delay-load, rpaths per slice, bundle identifier, the `.mxo` touch) moves to
  `source/cmake/embedded-python.cmake`, included by both objects' `CMakeLists.txt`; each object's
  stays a page. Both externals build and test on Linux (the mock kernel), macOS and Windows.
- **CI.** `build.yml`: the data-import check (4.2), the bundle identifier, `lipo`/`otool` and the
  rpath checks, and the Windows delay-load check run over both externals (a loop over
  `externals/`, so a third would need nothing). `style.yml`: both objects' TUs in the clang-tidy
  list and its header filter, and `source/shared/` added to clang-format's file list (today it
  lists `source/projects/` and `core/` only). `scripts/tidy.sh` is TapHouse's and takes a repo's
  own TUs — unchanged.
- **Packaging.** `assemble-package.py`'s `EXTERNALS` becomes a list per platform; `--merge` already
  copies every external it finds. `package-info.json.in`'s description names both objects. The
  release zips carry both; nothing else in `release.yml` names an external.
- **Docs.** The ReadMe: a `tap.python` section after the audio one — the loading line, the output
  table, the honest limits (D10, threads, no output from Python's own threads, no dictionaries
  yet) — and its performance note is one sentence: the cost is the method's Python, plus the
  `call()` bridge measured once by `core/bench`. `help/tap.python.maxhelp`, by hand in Max, with
  the three examples. `docs/tap.python.maxref.xml` from min (6.8's rule: commit Max's). CLAUDE.md:
  the second object in the layout and the shared glue. `CHANGELOG.md`: 1.1.0 — a new object, no
  change to `tap.python~`'s contract beyond the `list[…]` parameter, which only adds.
- **Runtime tests in Max** (`runtime-tests/`): `make_patchers.py` gains a non-signal `python()`
  box; new patchers `tap.python.*.maxtest.maxpat`: load (no argument, each example); every output
  row through `[print]`-free checks (a list into `test.assert`, an `anything` through `route`, two
  outlets in order through `trigger`); `getsteps` from the dumpout; a save that widens the return
  hint changes the outlets and the new one's cord carries (2.4's `channels` test, for control
  outlets); one file shared by a `tap.python~` and a `tap.python`, saved: one `Loaded` line, both
  reload; a message from a `metro` under Overdrive (the scheduler thread) outputs correctly and in
  order; `sys.exit()` and an exception reported, the object alive; `anything` forwarded. The Mac
  session that runs them is the phase's last item, with the help patcher.

## Plan — Phase 9 of the production plan

One PR each, in this order; each lands against tests that fail before it.

- [ ] **9.1 The core: output, outlets and the audio option.** `value.h`'s `output_item`/`output`
  and the Python-to-output conversion; `message_info::return_count`, `outlet_count()`, `call()`
  returning the output; `bind_audio`; the `list[…]`/`np.ndarray` final parameter (for both
  objects); the `Loaded` line per binding. `test_output.cpp` and fixtures. No Max code changes;
  `tap.python~`'s battery and glue test unchanged and green, the bench numbers unchanged (the
  audio path does not touch the new code).
- [ ] **9.2 The shared glue.** `tap.python_tilde_{attribute,message,cglue,filewatch,package}.h`
  move to `source/shared/tap/python_max/` as `python_glue<Host>` (the trampolines instantiated in
  each object's `.cpp` through `wrapper_find_self<Host>`); the runtime CMake block to
  `source/cmake/embedded-python.cmake`; `style.yml`'s list and header filter follow. Pure move:
  `tap.python~`'s behavior, tests and the data-import check unchanged. *Decide in the PR:* whether
  `reserved_messages()` and `answered_by_max()` move too, parameterized by the audio names, or each
  object keeps its list (the audio names are the only difference).
- [ ] **9.3 The object.** `source/projects/tap.python/`: ports with the dumpout, output mapping,
  `anything` forwarding, dynamic outlets on reload, the reserved names, `MIN_DESCRIPTION`; the
  glue test; the examples (`euclid.py`, `scale.py`, `note_name.py`, `default.py`'s `bang`); the
  CI checks over both externals; `assemble-package.py` and `package-info.json.in`. CHANGELOG 1.1.0
  started.
- [ ] **9.4 Documentation.** The ReadMe section and the output table; CLAUDE.md; the help patcher
  (JSON by hand, as 5.2 was, checked in Max in 9.6); the reference page from min against the mock
  kernel (5.1's way), to be replaced by Max's in 9.6.
- [ ] **9.5 Runtime tests.** The patchers above, in `make_patchers.py`, and `run.py` aware of the
  second external (its `EXTERNAL` check, the `--package` mode, the reference-page rule for both
  pages).
- [ ] **9.6 The Mac session.** Build, run the whole runtime suite (both objects), check the help
  patcher and re-save it, commit the pages Max writes, the hand checks the tests cannot make
  (`get<attr>` through the dumpout in a patcher, attrui on a `tap.python`, a `tap.python~` and a
  `tap.python` on one file saved while audio runs), then tag `v1.1.0`.

## Later (not planned)

Written down so they are decided rather than rediscovered; none is promised by 1.1.

- **Dictionaries.** A `dict` return as a Max dictionary out (`dictionary <name>` through a
  `t_dictionary` the object owns), and a `dictionary` message in as a `dict` argument. The natural
  next step, and a real design: ownership of the named dictionary, nested values, and `jit`-style
  `dictobj` registration.
- **Timers.** A class that wants to run on its own — a sequencer — needs a clock. Two shapes: an
  attribute `@interval` ms calling a method `tick()` on the scheduler thread (nothing to import,
  consistent with the rest), or a `self`-side API (a `schedule(ms, method)` injected at
  construction, which breaks "runs in a notebook"). The first fits; measure a Python call per tick
  against Max's own `metro` before promising timing.
- **More inlets.** min's `inlet<>` list makes proxies, and `proxy_getinlet()` says which one a
  message came in; a mapping would be `inlets: ClassVar[int]` plus the inlet number as a first
  argument to `anything`, or a method per inlet. Not until a use needs it: attributes cover the
  cold-inlet idiom.
- **Output from Python's own threads.** A queue the class could post to from a `threading.Thread`,
  drained by a qelem on the main thread — `node.script`'s shape. It needs the injected API the
  timers item weighs.
- **`buffer~` and `jit.matrix` as numpy arrays.** Attractive and Max-specific: a `buffer~` name as
  an attribute, its samples as an `np.ndarray` view under `buffer_locksamples()`. The locking rules
  make it its own design.
- **Deferring to the main thread.** An `@defer` attribute running every message on the main
  thread (`defer_low`), as `js` effectively does. `deferlow` in the patch does the same today; add
  it only if users ask.
