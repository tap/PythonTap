# tap.python — a Python class as a Max object without audio: design and plan

`tap.python~` runs a Python class as an audio object. `tap.python` runs one as an ordinary Max
object: messages in, messages out, no signal. This is its design — the decisions (D7–D11,
continuing `PRODUCTION-PLAN.md`'s D1–D6), what a class looks like, how the core and the package
change — and the plan to build it (Phase 9 of the production plan, which points here).

Drafted 2026-10-02 against 1.0.0 and audited the same day (`AUDIT-TAP-PYTHON-PLAN.md`: two
blockers, six major findings, nine minor; its addendum of 2026-10-09 records what 1.0.1 and 1.0.2
changed). **Revised 2026-10-09 against 1.0.2** for every finding; the [revision record](#revision-record)
at the end says what each changed. Nothing is built yet. Tick the items below (with the PR) as they
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
| D7 | What `tap.python` is | **A second Max object in the same package, over the same core, with the same class contract minus audio, plus output.** Same `python/` folder, same loader (a file saved once is executed once however many objects of either kind share it), same attributes, messages, hot reload, console and error guards. A class written for one object loads in the other: in `tap.python`, `process()` and `prepare()` are ordinary methods (sending `process 0.5` calls it and outputs the result — a way to test a filter sample by sample); in `tap.python~`, a method's return value is dropped, as now. |
| D8 | How output works | **A method's return value is output from the object's outlets, by its type; its return *hint* fixes how many outlets, at bind time.** No injected outlet API in 2.0 (nothing for the class to import, so it still runs in a notebook): `None` outputs nothing; a number or a string one atom; a sequence a list; a method hinted `-> tuple[A, B]` outputs one value per outlet, right to left. The object has as many outlets as the widest return hint among its methods (at least one), plus a dumpout outlet at the right, as Max objects with attributes do; a save that changes the count changes the outlets in place. The rules are in [Output](#output-what-a-method-returns). |
| D9 | Inlets | **One inlet in the first release.** Messages are the methods; state is the attributes (`steps 8`, `@pulses 3`, attrui), which is what a right inlet is for in most Max objects. More inlets are a later option (how, in [Later](#later-not-planned)), not a 1.x promise. |
| D10 | Threads | **A message runs on the thread it arrives on, holding the GIL while Python runs, and outputs on that thread after the GIL is released.** That is Max's main thread, the scheduler thread under Overdrive — or, with Scheduler in Audio Interrupt on, the audio thread itself, which the object says once per session and the ReadMe states as a limit. As every ordinary Max object does, and as `tap.python~`'s messages already do. No worker. The limits, in [Threads](#threads-and-the-gil). |
| D11 | Code layout | **One binary registers both Max classes; the second is a plain SDK class, not a min class; the core gains one option.** *Why one binary (decided 2026-10-09, audit B1):* the core is header-only and keeps its process-wide state in function-local statics, so two externals would each start the interpreter — the second `PyImport_AppendInittab()` after `Py_Initialize()` aborts Max (reproduced with two shared objects in one process) — and, as 1.0.1's Windows failure showed, a function's address is not its identity across a DLL boundary. *Why a plain SDK class:* min keeps one file-scope `this_class` per translation unit and returns early from a second `wrap_as_max_external()` (`c74_min_api.h:325`, `c74_min_object_wrapper.h:780`); a second min class in a second translation unit would work by accident of internal linkage and violate the one-definition rule the moment a test instantiated its templates elsewhere. A plain class (`class_new`/`class_addmethod`/`class_register` in the same `ext_main`, after min's) is what the shared glue already speaks — `object_addattr`, `object_addmethod`, the nobox file watcher — and what the outlets need (M1). Max finds the object through the package's `init/` mapping (`objectfile`), with a stub external as the fallback; the 9.0 spike settles which. The core's `processor` takes `bind_audio` (true for `tap.python~`): with it off, `process` and `prepare` are plain methods and nothing is prepared. The shared glue moves into templates on the host object (`python_glue<Host>`) so both classes use one copy. |

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
  messages get it too; it is a change to what such a parameter received before (the empty
  symbol), recorded as one (9.1).
- **Output** — below.
- **Hot reload, errors, console** — as `tap.python~`: a save reloads keeping attribute values; a
  broken save is reported once and the object outputs nothing until a save fixes it; an exception
  in a method prints its traceback and the message outputs nothing; `sys.exit()` is reported, not
  honored; what is below Python (`os._exit()`, a crashing extension, an endless loop — which
  freezes the thread the message came on) is outside the guard (8.1).
- **Reserved names** — Max's and min's own (8.2's list and the guard), without the audio ones:
  `dsp`, `dsp64`, `dspsetup`, `dspstate`, `inputchanged`, `multichanneloutputs`, `signal`, `mode`,
  `latency` and `latencysamples` are free; `anything` is taken by the forwarder above, as a method
  a class may define. The guard asks Max what it answers for a name no class can have and
  compares every lookup with that (1.0.2's `not_found_method()`), and leaves out everything the
  object itself registered — its messages, its attributes, its dumpout and its forwarder — since
  each answers its name in Max (1.0.1's reload bug).

### Output: what a method returns

A method's return value goes out of the object, converted by the rules below, before the message
returns. The rules reproduce how Max objects and `js` behave, so a patch sees what it expects.

| The method returns | The object outputs |
|---|---|
| `None` | nothing |
| `bool`, or numpy's `bool_` | an int, 0 or 1 |
| an `int` (anything with `__index__`: `np.int64`, an `IntEnum`) | an int (`t_atom_long`, 64-bit; a value past it is reported) |
| a `float` (anything with `__float__` and no `__index__`: `np.float32`) | a float; a non-finite value passes through as the atom Max can carry (`tap.python~` zeroes them for the audio's sake; a control value is the class's to make) |
| a `str` | the message named by it, with no arguments (`"hello world"` is one symbol, as in `js`) — *or* `symbol <s>`: the 9.0 spike decides against real downstream objects (`route`, `sel`, `prepend`, a message box's `$1`), and says what happens to the selectors `int`, `float`, `list`, `symbol`, `bang` and `""` |
| a sequence: a `list`, a `tuple` without a `tuple[…]` hint, a `range`, or a 1-D `np.ndarray` | a list, each element an atom by the rules above; if the first element is a `str`, the message it names with the rest as arguments (Max's own rule: `["note", 60, 100]` outputs `note 60 100`); an empty sequence outputs nothing; an element that is not an atom (`None`, a nested sequence) is reported and the list is not output |
| a sequence of *n* values, from a method hinted `-> tuple[…]` with *n* members | one value per outlet, outlets *n*…1, right to left, each by these rules (`None` in a slot outputs nothing from that outlet); a result that is not a sequence of exactly *n* values is reported |
| anything else: a `dict`, a `set`, `bytes`, a generator, a 2-D array, an object | reported, with its type; nothing output (a `dict` as a Max dictionary is a later item) |

The *hint* decides the outlet count when the class loads; the *value* fills them when the method
runs. A method hinted `-> tuple[int, str]` has two outlets, and so does the object if no method is
hinted wider; a method hinted `-> list[float]`, `-> np.ndarray`, `-> float` or not at all has one,
and an unhinted method that happens to return a tuple outputs it as a list from the first outlet.
An unhinted tuple return is a list, a hinted one is several outlets: this is the one place where
the hint changes what a value does, so the ReadMe says it in those words. A `tuple[…]` hint of
unsaid length (`tuple`, `tuple[int, ...]`) cannot name an outlet count: the method keeps one
outlet and outputs the tuple as a list, and the console says so once — rather than hiding the
method, so that a class written for `tap.python~` still loads whole (D7). The count comes from
the hint as an object, or as a string with the same depth-aware split `_split_union` uses
(`'tuple[list[int], dict[str, int]]'` is two outlets, not three).

A `print()` in a method posts to the console at once from the main thread and a moment later from
any other (8.4); it is not output.

### Examples

`default.py` stays shared by both objects and gains a `bang` that returns the gain, so
`[tap.python]` with no argument outputs something. New examples, each a thing Max users write by
hand today, each with the comment `default.py` already carries — once `def list(...)` or
`def int(...)` is defined, a later annotation in the same class body using `list[...]` or `int`
names the method, so such methods come last:

- `euclid.py` (above): a Euclidean rhythm from two attributes, a list out of a bang.
- `scale.py`: a list in, scaled and offset by attributes with numpy, a list out — `def list(self,
  values: np.ndarray) -> np.ndarray`.
- `note_name.py`: an int in, two outlets out — `def int(self, note: int) -> tuple[str, int]`
  returns the pitch class and the octave, so `60` outputs `C` and `4`.

## Threads and the GIL

A message takes a `gil_lock` on the thread it arrives on (`processor::call()` already does),
converts the arguments, calls the method, and converts the result to atoms, all under the GIL;
the `outlet_*` calls come after the GIL is released. Not for reentrancy — `gil_lock` is
`PyGILState_Ensure`, which nests, so a downstream object sending a message back into this one (a
loop through `trigger`) simply takes it again on the same thread — but because an outlet call
runs the whole downstream chain before it returns, and holding the GIL across it would keep every
other Python thread (a `tap.python~`'s audio thread in direct mode) waiting for a cascade of Max
objects that has nothing to do with Python.

**Output against a reload (audit M2).** A reload runs on the main thread and, after `load()`
returns the GIL, changes the object's outlets and messages. A message on the scheduler thread may
be outputting at that moment. So the object has one lock of its own, a recursive mutex, held
across the mapping of a result onto outlets *and the sends* (recursive, because a feedback loop
re-enters on the same thread), and across every port change (`outlet_delete`,
`outlet_insert_after`, the outlet vector, `object_deletemethod`/`object_addmethod`). It is never
held across a call into Python. Under it a message checks that the outlet count it computed for is
still current, and drops the output, reported once per load, if not. Pinned by a glue test that
races a widening reload against a second thread's messages (TSan on the Linux leg).

**The honest limits:**

- A long computation in a message from a `metro` holds Max's scheduler for its duration under
  Overdrive. The patch can send it through `deferlow` to run on the main thread instead.
- **Scheduler in Audio Interrupt** makes the scheduler thread the audio thread, so a `metro`-driven
  message then runs Python *on the audio thread*, and waits there for the GIL behind a reload (a
  few milliseconds) — a dropout. `tap.python~`'s worker mode does not help, because the work
  itself is on the audio thread; CLAUDE.md's "the audio thread never takes the GIL" holds for a
  patch without a `tap.python`. The object detects it (`systhread_isaudiothread()`) and says so
  once per session; the ReadMe states it as a limit; an `@defer` attribute running messages on
  the main thread is the later remedy ([Later](#later-not-planned)).
- The 0.5 ms switch interval (2.6) bounds how long a *bytecode* loop keeps the GIL from a waiting
  thread; a single long C call (a numpy operation on a large array) holds it throughout, as 2.6
  says itself.
- Output from a thread Python started (`threading.Thread` in the class) is not supported: a method
  runs and returns on Max's thread, and there is no outlet to reach from anywhere else — stated as
  a limit, with the later item that would change it.

## The core

`core/include/tap/python/` grows, host-independent and tested on Linux first, as everything else
did (D6):

- **`value.h`** — beside `value` (one atom): `output_item`, a message to output — a selector or
  none, and its atoms (what `outlet_anything` / `outlet_list` / `outlet_int` take) — and
  `output`, the per-outlet items of one call. The conversion from a Python object to an
  `output_item` lives in the core (the table above; `np.bool_` recognized by its `__mro__` name as
  `hint_kind` does, without importing numpy), with its errors as diagnostics through the
  processor's log, so the Max side only maps items onto `outlet_*` calls.
- **`processor.h`** — `message_info` gains `return_count` (from `describe()`'s `return_shape`,
  made depth-aware for string hints); `outlet_count()` is the widest among the messages (at least
  one); a new **`call_with_output()`** returns the converted `output` (empty for `None` or a
  failure) beside the existing `bool call()`, which `tap.python~` keeps (C++ cannot overload on the
  return type alone); the constructor's `bind_audio` — off, `process`/`prepare` are bound as
  messages, `has_process()` is false, `prepare()` is a no-op; the unsaid-length tuple rule applies
  only with `bind_audio` off; argument conversion for a last parameter hinted `list[…]` or
  `np.ndarray`, which needs the support module's `describe()` to report a list hint's element kind.
- **Announce-once per kind (audit M5).** The class diagnostics that depend on the object kind — a
  name reserved by one host and not the other, the tuple rule — are announced once per *(file,
  kind)*, not once per file: the loader keeps what it announced for each kind against the source
  it executed (`announce_due(name, kind)`), and a processor announces what its kind owes even when
  the other kind ran the save. The `Loaded` line stays once per save, from whichever ran it, and
  says what that object bound — "4 messages, 2 outlets" or "process() bound, one call per vector".
- **Tests** — `test_output.cpp`: every row of the table above from a fixture whose methods return
  each kind; the outlet count from the hints, object and string, nested; an unhinted tuple as a
  list against a hinted one as outlets; an unsaid-length tuple hint as a list with one notice; the
  `list[…]`/`np.ndarray` parameter — **with a test of today's behavior first** (the empty symbol),
  then the change; `bind_audio` off making `process` a message and leaving `tap.python~`'s battery
  untouched; two processors of different kinds on one file, each announcing what it owes; a method
  raising outputs nothing and reports; a call from a second thread racing a reload on the first.

## The Max object

`source/projects/tap.python_tilde/` gains the second class beside the first — `tap.python.h`,
registered by the project's own `ext_main` in `tap.python_tilde.cpp` after min's
`wrap_as_max_external<python>()` — a plain SDK class (D11):

- **The class.** `class_new("tap.python", new, free, sizeof(tap_python), nullptr, A_GIMME, 0)`,
  `assist` (`A_CANT`), `filechanged` (typed, as the watcher sends it), the standard messages the
  class's Python methods take through `python_message`, and a class-level `anything` forwarder;
  `class_register(CLASS_BOX, c)`. The struct holds the `t_object`, the processor (`bind_audio`
  off), the outlet pointers, the dumpout, the lock, the file watch and the attribute and message
  maps — the same members as the min object, through the shared glue.
- **How Max finds it.** The package's `init/tap.python.txt` maps the object name to the file:
  `max objectfile tap.python tap.python~;` — the mapping Max's own `init/` text files use for
  objects that live in a file of another name. *The spike confirms it on both platforms.* If Max
  will not map it, the fallback is a stub external `tap.python.mxo` / `.mxe64` with no core in it,
  whose `ext_main` has Max load `tap.python~`'s file (which registers both classes) and returns;
  the spike tries that too if needed. `assemble-package.py` ships `init/`.
- **Ports (audit M1).** In `new`, `outlet_new()` for the dumpout first (Max orders outlets by
  creation, right to left), stored with `object_obex_store(x, _sym_dumpout, …)` as the SDK's own
  example does, then the value outlets from last to first. The object sends through the raw
  pointers it holds, never through min. On a reload that changes the count, between the box's
  `dynlet_begin` and `dynlet_end`: `outlet_delete` for the surplus, `outlet_insert_after` the last
  value outlet for the new ones (never `outlet_append`, which would land them after the dumpout),
  under the object's lock; without a box, the object keeps its outlets and says so once.
- **Messages and output.** `message_gimme()` calls `call_with_output()` and, under the lock, maps
  each `output_item` onto `outlet_int`/`outlet_float`/`outlet_anything`/`outlet_list` on the right
  outlet, right to left.
- **`anything` (audit M3).** A class-level forwarder registered in `class_new`, so it exists for
  every instance and the object, not Max's dispatch order, decides what happens: a selector is
  tried as one of the class's messages, then as an attribute set, then as `get<name>` (output from
  the dumpout), then as the class's Python `anything` if it has one, and otherwise posts that the
  object does not understand it, in Max's own words. The Python `anything` is never registered as
  an instance method. *Unverified until the spike:* whether Max consults the object's instance
  methods and attributes before a class-level `anything` at all (8.8 showed an added attribute's
  name is found by `object_getmethod()`, which is consistent with it); the forwarder is correct
  either way.
- **The guard.** `answered_by_max()` as 1.0.2 has it — compare with what Max answers for a name no
  class can have — leaving out the object's own messages, attributes, dumpout and forwarder. If
  Max answers unknown names with the forwarder on a class that has one, the sentinel *is* the
  forwarder and unknown names still read as "not found".
- **Reserved names.** 8.2's list less the audio names; `anything` is answered by the forwarder and
  is not reserved by the guard (the sentinel sees to it), and a glue test asserts a class's
  `anything` is exposed.
- **Threads.** The lock above; `systhread_isaudiothread()` on each message for the once-per-session
  notice; the console through the shared qelem (8.4).
- **The rest** is the shared glue, templated on the host: the file watcher (6.1's nobox helper,
  registered once for both hosts), attributes (3.4's reconciliation), messages, the trampolines
  (`python_glue<Host>::self(t_object*)` instead of `wrapper_find_self`), the package paths.
- **The reference page.** min writes `tap.python~`'s from `MIN_DESCRIPTION`; a plain SDK class has
  no such generator, so `docs/tap.python.maxref.xml` is written by hand from the same source of
  truth, the contract above, and kept in step by review (CLAUDE.md's "never hand-edit" is about
  the page min generates). The help patcher is by hand in Max, as 5.2 was.
- **Glue test** (mock kernel, with the stubs made faithful — audit m5): `outlet_nth` returning the
  mock's real outlet ids, `object_obex_store`/`object_obex_dumpout` and `outlet_insert_after`
  stubbed and recorded. `[tap.python euclid]` (made through the class's own `new`, as Max would)
  has one inlet, two outlets (one plus dumpout); a bang's list is in `object_getoutput(x, 0)`; a
  `note_name` int fills outlet 1 then outlet 0; `getsteps` reaches the dumpout; a `tuple[…]` save
  that widens the class records the dynlet calls; the forwarder's order with a class that has
  `anything` and with one that does not; the reload race under TSan.

## The package

- **Build.** The second class compiles into `tap.python~`'s module: a second header and its TU in
  the same CMake target; the runtime, link and rpath rules are unchanged. The glue test target
  includes both. Linux (the mock kernel), macOS and Windows as now.
- **CI.** `build.yml`: the data-import check (4.2), the bundle identifier, `lipo`/`otool` and the
  rpath checks and the Windows delay-load check are unchanged (one external); `linux-max-glue`
  gains ASan/UBSan and TSan rows, as `linux-core` has, for the lock and the reload race; one
  script checks the hand-made artifacts (the pages' XML, the help patchers' JSON, `init/` in the
  assembled package). `style.yml`: the new
  TU and header in the clang-tidy list, and `source/shared/` — if the glue moves there — in
  clang-format's file list (today it lists `source/projects/` and `core/` only).
- **Packaging.** `assemble-package.py` ships `init/` beside `help`, `docs` and `python`;
  `package-info.json.in`'s description names both objects. One external per platform, as now; the
  icon is TapHouse's and guarded (v6), and is the package's, not an object's.
- **Docs.** The ReadMe: a `tap.python` section after the audio one — the loading line, the output
  table, the limits (D10, Scheduler in Audio Interrupt, no output from Python's own threads, no
  dictionaries yet), and a performance sentence backed by the bench row 9.1 adds. In Max's own
  documentation system (9.4): `docs/tap.python.maxref.xml` by hand (above); a vignette and three
  tutorials with patchers in `docs/`; a *PythonTap Overview* patcher in `extras/`, named as the
  landing patcher; help patchers with tabs for both objects. CLAUDE.md: the second class, the
  shared glue, D11's reasons. `CHANGELOG.md`: 2.0.0 — a new object; the `list[…]` parameter
  recorded as a change to what `tap.python~` passes such a parameter (the empty symbol before),
  pinned by a test of the old behavior first; whether that needs 2.0 under the CHANGELOG's rule
  is the maintainer's call, and the plan's position is that a hint which never carried a value is
  not a contract a class could have relied on. The book (9.7) follows the release.
- **Runtime tests in Max** (`runtime-tests/`): `make_patchers.py` gains a non-signal `python()`
  box; new patchers `tap.python.*.maxtest.maxpat`: load (no argument, each example, through the
  `init/` mapping in a fresh Max); every output row through `[print]`-free checks (a list into
  `test.assert`, an `anything` through `route`, two outlets in order through `trigger`); `getsteps`
  from the dumpout; a save that widens the return hint changes the outlets and the new one's cord
  carries (2.4's `channels` test, for control outlets); one file shared by a `tap.python~` and a
  `tap.python`, saved: one `Loaded` line, both reload, each kind's diagnostics once; a message from
  a `metro` under Overdrive outputs correctly and in order; `sys.exit()` and an exception reported,
  the object alive; `anything` forwarded; a class with `anything` and attributes, the attributes
  still set and read. Windows has no harness: the glue test runs there in CI, and the spike and
  the release session run the package by hand.

## Plan — Phase 9 of the production plan

One PR each, in this order; each lands against tests that fail before it. **9.0 comes first,
before any code the others depend on, and runs on a Mac and on Windows** — three releases in a
row (1.0.0, 1.0.1, 1.0.1's Windows package) shipped what only a host platform could show, and the
third was Windows-only.

- [ ] **9.0 The spike, in Max on both platforms.** A throwaway second class in `tap.python~`'s
  binary, enough to answer what only Max can, written into this plan before 9.1:
  - a plain SDK class registered beside min's in one `ext_main` loads, and both objects work in
    one patch created in either order (D11);
  - `init/tap.python.txt`'s `objectfile` mapping makes `[tap.python]` load the file in a fresh
    Max — or the stub external does;
  - `outlet_insert_after` places an outlet before the dumpout, and patch cords survive it (M1);
  - `get<attr>` reaches the obex-stored dumpout (M1);
  - the dispatch order of a class-level `anything` against the object's instance methods and
    attributes, and what `object_getmethod()` answers for an unknown name on such a class (M3);
  - which thread a `metro`-driven message runs on with Overdrive on, and with Scheduler in Audio
    Interrupt on (M4);
  - `symbol` or `anything` for a `str` return, against `route`, `sel`, `prepend` and a message
    box (m2);
  - where Max picks up a vignette (`.maxvig.xml`) and a tutorial (`.maxtut.xml`) dropped into
    the package's `docs/`, and a patcher in `extras/`, for 9.4.
- [ ] **9.1 The core: output, outlets and the audio option.** `value.h`'s `output_item`/`output`
  and the Python-to-output conversion; `message_info::return_count` (depth-aware for strings),
  `outlet_count()`, `call_with_output()`; `bind_audio`, gating the tuple rule; the
  `list[…]`/`np.ndarray` final parameter with the old behavior pinned first; announce-once per
  kind; the `Loaded` line per binding. `test_output.cpp` and fixtures, with the output table
  exercised as one data-driven test over its edges (64-bit ints at the limit, non-finite floats,
  numpy scalars of every family, 0-d and 2-D arrays, nested sequences, a `tuple[…]` hint of each
  length against results of the wrong length), and `test_examples.cpp` running the three new
  examples on known inputs. A `core/bench` row for a message call with a list of 64 atoms, written
  by `scripts/update-perf-docs.py` with the others, so the ReadMe's performance sentence is
  measured. No Max code changes; `tap.python~`'s battery and glue test unchanged and green; the
  audio bench numbers unchanged (the audio path does not touch the new code).
- [ ] **9.2 The shared glue.** `tap.python_tilde_{attribute,message,cglue,filewatch,package}.h`
  become `python_glue<Host>` (the trampolines instantiated per host through `Host::self()`), in
  `source/shared/tap/python_max/` or beside the object; `reserved_messages()` parameterized by the
  audio names. A pure move: `tap.python~`'s behavior, tests and the data-import check unchanged.
- [ ] **9.3 The object.** `tap.python.h` and its TU in the project, registered by the project's
  `ext_main`; ports with the dumpout and the lock; output mapping; the `anything` forwarder; dynamic
  outlets on reload; the guard and the reserved names; the Scheduler-in-Audio-Interrupt notice,
  with the audio-thread predicate injected as the main-thread one is (8.4) so the glue test can
  drive it; `init/tap.python.txt` (or the stub); the glue test with faithful stubs, **and
  `linux-max-glue` gaining ASan/UBSan and TSan rows** — the lock and the reload race live in the
  glue, which CI builds without sanitizers today; the examples (`euclid.py`, `scale.py`,
  `note_name.py`, `default.py`'s `bang`); `assemble-package.py` and `package-info.json.in`, with a
  CI check that the assembled package lists `init/tap.python.txt`; CHANGELOG 2.0.0 started.
- [ ] **9.4 Documentation, in Max's own system and the ReadMe.** The ReadMe section, the output
  table and the limits; CLAUDE.md. For the Documentation window: `docs/tap.python.maxref.xml` by
  hand, with see-also links between the two pages; a vignette, *Writing Max objects in Python* —
  the ReadMe's "Writing a class" and the threads and performance sections, rewritten for the
  window, covering both objects; and three tutorials with their patchers — a gain per sample and
  then per vector, a filter with `prepare()`, a control object (`euclid`). For the Extras menu, a
  *PythonTap Overview* patcher, named as the package's landing patcher in `package-info.json.in`
  (its `homepatcher` is empty today). The help patchers with tabs: `tap.python`'s from the start,
  and `tap.python~`'s gaining the numpy, allpass and `mc.` tabs 5.2 asked for. Where Max reads the
  vignette and tutorial files from inside `docs/` is what 9.0 found. CI checks for what is written
  by hand: the reference page, the vignette and the tutorials well-formed XML, and the help
  patchers' JSON as 5.2 checked by hand (ids unique, every line connects), in one script. All
  checked open in Max in 9.6.
- [ ] **9.5 Runtime tests.** The patchers above in `make_patchers.py`; `run.py` aware of the
  second object (the `init/` mapping in the installed package, the `--package` mode, the
  reference-page rule for `tap.python~`'s page only). The soak session (6.2) gains a
  `tap.python` driven by a `metro` while its file is saved every second: every output checked,
  memory flat, the console counts exact.
- [ ] **9.6 The release session, Mac and Windows.** Build, run the whole runtime suite on the Mac,
  check the help patcher and re-save it, commit the page Max writes for `tap.python~`, the hand
  checks the tests cannot make (`get<attr>` through the dumpout in a patcher, attrui on a
  `tap.python`, a `tap.python~` and a `tap.python` on one file saved while audio runs); on
  Windows, the package by hand: both objects load through the mapping, attributes and messages
  work, a reload keeps them. Then tag `v2.0.0`.
- [ ] **9.7 The book.** After 2.0.0, the family's shape: an mdBook under `book/` (TapHouse's icon
  rule already knows `book/book.toml` and guards `book/theme/favicon.*`), built and published by
  CI. Chapters from the ReadMe's sections — install, writing a class, audio, worker mode, control
  objects, performance, errors and limits, building, testing — plus the examples, the notebook
  rendered, and the design decisions for contributors. Example code is pulled into the pages by
  mdBook's include directive straight from `python/*.py`, and the performance tables are written
  into the book by `scripts/update-perf-docs.py` beside the ReadMe's, so neither can drift from
  what ships. The ReadMe then shrinks to the front door — what it is, install, a quick start, and
  links — so that each fact has one home. *Decide in the PR:* whether the vignette and the book
  share source (one Markdown rendered two ways) or the vignette stays the short form.

## Later (not planned)

Written down so they are decided rather than rediscovered; none is promised by 2.0.

- **Dictionaries.** A `dict` return as a Max dictionary out (`dictionary <name>` through a
  `t_dictionary` the object owns), and a `dictionary` message in as a `dict` argument. The natural
  next step, and a real design: ownership of the named dictionary, nested values, and `jit`-style
  `dictobj` registration.
- **`@defer`.** Run every message on the main thread (`defer_low`), as `js` effectively does: the
  remedy for Scheduler in Audio Interrupt, and for a heavy method under Overdrive. `deferlow` in
  the patch does the same today; add it when the notice above is seen in practice.
- **Timers.** A class that wants to run on its own — a sequencer — needs a clock. Two shapes: an
  attribute `@interval` ms calling a method `tick()` on the scheduler thread (nothing to import,
  consistent with the rest), or a `self`-side API (a `schedule(ms, method)` injected at
  construction, which breaks "runs in a notebook"). The first fits; measure a Python call per tick
  against Max's own `metro` before promising timing.
- **More inlets.** `proxy_new()` per extra inlet and `proxy_getinlet()` to tell which one a message
  came in; a mapping would be `inlets: ClassVar[int]` plus the inlet number as a first argument to
  `anything`, or a method per inlet. Not until a use needs it: attributes cover the cold-inlet
  idiom.
- **Output from Python's own threads.** A queue the class could post to from a `threading.Thread`,
  drained by a qelem on the main thread — `node.script`'s shape. It needs the injected API the
  timers item weighs.
- **`buffer~` and `jit.matrix` as numpy arrays.** Attractive and Max-specific: a `buffer~` name as
  an attribute, its samples as an `np.ndarray` view under `buffer_locksamples()`. The locking rules
  make it its own design.

## Revision record

*2026-10-09, against 1.0.2, for `AUDIT-TAP-PYTHON-PLAN.md` and its addendum:*

- **B1 → D11 rewritten:** one binary, two classes; and, because min allows one class per
  translation unit, the second is a plain SDK class. Max finds it through `init/`'s `objectfile`
  mapping, with a stub external as the fallback.
- **B2 → 9.0:** a Max spike on both platforms before 9.1, with the questions listed; 8.8 has run.
- **M1 → Ports:** raw outlets created dumpout-first, the dumpout in the obex, sends through the
  object's own pointers, `outlet_insert_after` on reload.
- **M2 → Threads:** the object's recursive lock across output and port changes, with a race test.
- **M3 → `anything`:** a class-level forwarder that fixes the order itself; the Python `anything`
  never an instance method; the guard as 1.0.2 has it, leaving out everything the object registers.
- **M4 → D10 and Threads:** Scheduler in Audio Interrupt named, detected and stated as a limit;
  `@defer` the later remedy; the C-call caveat on the switch interval.
- **M5 → The core:** announce-once per (file, kind).
- **M6 → The core and Docs:** the tuple rule gated on `bind_audio`; an unsaid-length tuple output
  as a list rather than hidden; the `list[…]` parameter recorded as a change with the old
  behavior pinned first, and the version question put to the maintainer.
- **m1–m9:** the output table's sequence types enumerated and `np.bool_` placed; the `str` return
  decided by the spike; `call_with_output()`; depth-aware string hints; faithful mock stubs; the
  file watcher registered once (one binary); the shadowing comment in the examples; D10 reworded
  to match the Threads section; non-finite floats pass through on the plan's own reasoning, the
  `js` claim dropped.
- **Addendum:** the guard rule from 1.0.1 and 1.0.2 (compare with what Max answers; leave out what
  the object registers), Windows in the spike and the release session.

*2026-10-09, after the maintainer's review — documentation and test coverage:*

- **Documentation** was reference pages and help files only. 9.4 now covers Max's own system — a
  vignette, three tutorials with patchers, an Extras overview and landing patcher, help tabs —
  with CI checks for what is written by hand, and 9.0 finds where Max reads the files from. A
  new 9.7, the family's mdBook, follows 2.0.0, with example code and performance tables pulled
  from what ships so the book cannot drift.
- **Test coverage** had four gaps: no sanitizer rows for the Max glue, where the new lock lives
  (9.3); a performance sentence with no measurement behind it — a bench row (9.1); no soak for
  the new object (9.5); and hand-made artifacts with no check — the pages, the help patchers, the
  `init/` file (9.3, 9.4). Also added: the output table as a data-driven test over its edges and
  core tests for the new examples (9.1), and the audio-thread predicate injected so the
  Scheduler-in-Audio-Interrupt notice is testable without Max (9.3). Windows stays by hand: the
  runtime harness is macOS only, written down as a limit rather than fixed here.
