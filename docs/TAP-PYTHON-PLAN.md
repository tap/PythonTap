# tap.python — a Python class as a Max object without audio: design and plan

`tap.python~` runs a Python class as an audio object. `tap.python` runs one as an ordinary Max
object: messages in, messages out, no signal. This is its design — the decisions (D7–D11,
continuing `PRODUCTION-PLAN.md`'s D1–D6), what a class looks like, how the core and the package
change — and the plan to build it (Phase 9 of the production plan, which points here).

Drafted 2026-10-02 against 1.0.0 and audited the same day (`AUDIT-TAP-PYTHON-PLAN.md`: two
blockers, six major findings, nine minor; its addendum of 2026-10-09 records what 1.0.1 and 1.0.2
changed). **Revised 2026-10-09 against 1.0.2** for every finding; the [revision record](#revision-record)
at the end says what each changed. 9.0 to 9.3 are built — the spike ran in Max on a Mac
and on Windows: its answers are under [9.0](#what-90-found), and the design below follows them. Tick the
items below (with the PR) as they land, and keep the design current where a PR decides differently.

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
| D11 | Code layout | **One binary registers both Max classes; the second is a plain SDK class, not a min class; the core gains one option.** *Why one binary (decided 2026-10-09, audit B1):* the core is header-only and keeps its process-wide state in function-local statics, so two externals would each start the interpreter — the second `PyImport_AppendInittab()` after `Py_Initialize()` aborts Max (reproduced with two shared objects in one process) — and, as 1.0.1's Windows failure showed, a function's address is not its identity across a DLL boundary. *Why a plain SDK class:* min keeps one file-scope `this_class` per translation unit and returns early from a second `wrap_as_max_external()` (`c74_min_api.h:325`, `c74_min_object_wrapper.h:780`); a second min class in a second translation unit would work by accident of internal linkage and violate the one-definition rule the moment a test instantiated its templates elsewhere. A plain class (`class_new`/`class_addmethod`/`class_register` in the same `ext_main`, after min's) is what the shared glue already speaks — `object_addattr`, `object_addmethod`, the nobox file watcher — and what the outlets need (M1). Max finds the object through the package's `init/` mapping (`objectfile`), with a stub external as the fallback; the 9.0 spike found the mapping works, on a Mac and on Windows, so no stub is needed. The core's `processor` takes `bind_audio` (true for `tap.python~`): with it off, `process` and `prepare` are plain methods and nothing is prepared. The shared glue moves into templates on the host object (`python_glue<Host>`) so both classes use one copy. |

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
  symbol), recorded as one (9.1) — and the reason the release is 2.0.0 (decided 2026-10-09).
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
| a `str` | `symbol <s>`: one symbol, whatever it holds (`"hello world"` is one symbol, `"60"` the symbol and not the number). *Decided by the 9.0 spike, in Max:* output as the message it names, the strings `list`, `int`, `float`, `symbol`, `bang` and `""` are not data — dropped, an error in every receiver, a real bang — while `symbol <s>` reaches `sel`, a message box's `$1` and every other receiver as itself ([what 9.0 found](#what-90-found), 6). To output the message a string names, return it in a list (`["start"]`, the next row) |
| a sequence: a `list`, a `tuple` without a `tuple[…]` hint, a `range`, or a 1-D `np.ndarray` (a 0-d one is its value) | a list, each element an atom by the rules above; if the first element is a `str`, the message it names with the rest as arguments (Max's own rule: `["note", 60, 100]` outputs `note 60 100`); an empty sequence outputs nothing; an element that is not an atom (`None`, a nested sequence) is reported and the list is not output |
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
  returns the pitch class and the octave, so `60` outputs `4` and `symbol C`.

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
  patch without a `tap.python`. The object detects it (`systhread_isaudiothread()`, which 9.0
  found true for exactly that case) and says so once per session; the ReadMe states it as a limit; an `@defer` attribute running messages on
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
  none, and its atoms (what `outlet_anything` / `outlet_list` / `outlet_int` take: no selector is
  one number, the selector `list` a list, any other the message it names, a `str` being `symbol
  <s>`) — and `output`, the per-outlet items of one call (empty: nothing at all). The conversion from a Python object to an
  `output_item` lives in the core (the table above; `np.bool_` recognized by its `__mro__` name as
  `hint_kind` does, without importing numpy), with its errors as diagnostics through the
  processor's log, so the Max side only maps items onto `outlet_*` calls.
- **`processor.h`** — `message_info` gains `return_count` and `spreads` (from `describe()`'s
  `return_shape`, made depth-aware for string hints; a `tuple[…]` of more than 64 members is a list,
  said once) and `rest`/`rest_type` for a last parameter hinted `list[…]` or `np.ndarray`;
  `outlet_count()` is the widest among the messages (at least one); a new **`call_with_output()`** returns the converted `output` (empty for `None` or a
  failure) beside the existing `bool call()`, which `tap.python~` keeps (C++ cannot overload on the
  return type alone); the constructor's `bind_audio` — off, `process`/`prepare` are bound as
  messages, `has_process()` is false, `prepare()` is a no-op; the unsaid-length tuple rule applies
  only with `bind_audio` off; argument conversion for a last parameter hinted `list[…]` or
  `np.ndarray`, which needs the support module's `describe()` to report a list hint's element kind.
- **Announce-once per kind (audit M5).** The class diagnostics that depend on the object kind — a
  name reserved by one host and not the other, the tuple rule — are announced once per *(file,
  kind)*, not once per file: the loader keeps what it announced for each kind against the source
  it executed (`announce_due(name, kind)`, the kinds `audio` and `control`), and a processor
  announces what its kind owes even when the other kind ran the save. The `Loaded` line stays once
  per save, from whichever ran it, and says what that object bound — "4 messages, 2 outlets" or
  "process() bound, one call per vector". As built (9.1): a name its host reserves, everything
  `process()`'s binding says, a keyword-only parameter and the tuple rules are per kind; what is
  true of the class whatever binds it — a class that will not load or instantiate, a hint that does
  not resolve, an attribute whose type changed — stays once per save.
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
  `class_register(CLASS_BOX, c)`. The struct holds the `t_object`, the obex and a pointer to the
  object itself (`control_object`, as built), which holds the processor (`bind_audio` off), the
  outlet pointers, the dumpout, the lock, the file watch and the attribute and message maps — the
  same members as the min object, through the shared glue.
- **How Max finds it.** The package's `init/tap.python.txt` maps the object name to the file:
  `max objectfile tap.python tap.python~;` — the mapping Max's own `init/` text files use for
  objects that live in a file of another name. *Verified by 9.0 on a Mac and on Windows:* a fresh
  Max finds `[tap.python]` through it, and says `tap.python: No such object` without it. So the
  fallback the plan held in reserve — a stub external `tap.python.mxo` / `.mxe64` whose `ext_main`
  has Max load `tap.python~`'s file — is not needed. `assemble-package.py` ships `init/` (since
  9.0).
- **Ports (audit M1).** In `new`, `outlet_new()` for the dumpout first (Max orders outlets by
  creation, right to left), stored with `object_obex_store(x, _sym_dumpout, …)` as the SDK's own
  example does, then the value outlets from last to first. **And the class registers
  `object_obex_dumpout` as its `dumpout` method** (`class_addmethod(c, (method)object_obex_dumpout,
  "dumpout", A_CANT, 0)`, as Cycling '74's SDK examples do): 9.0 found that without it `get<attr>`
  is dropped without a word. The object sends through the raw pointers it holds, never through
  min. On a reload that changes the count, between the box's `dynlet_begin` and `dynlet_end`:
  `outlet_delete` for the surplus, `outlet_insert_after` the last value outlet for the new ones
  (never `outlet_append`, which would land them after the dumpout), under the object's lock;
  without a box, the object keeps its outlets and says so once. 9.0 verified that this keeps the
  dumpout last and the cords of every outlet that stays, the dumpout's included.
- **Messages and output.** `message_gimme()` calls `call_with_output()` and, under the lock, maps
  each `output_item` onto `outlet_int`/`outlet_float`/`outlet_anything`/`outlet_list` on the right
  outlet, right to left.
- **`anything` (audit M3).** A class-level forwarder registered in `class_new`, so it exists for
  every instance and the object, not Max's dispatch order, decides what happens: a selector is
  tried as one of the class's messages, then as an attribute set, then as `get<name>` (output from
  the dumpout), then as the class's Python `anything` if it has one, and otherwise posts that the
  object does not understand it, in Max's own words. The Python `anything` is never registered as
  an instance method. *Verified by 9.0:* Max tries the object's instance attributes (set and
  `get<attr>`) and instance methods before a class-level `anything`, and an instance method before
  a class method of the same name, so the forwarder sees only what nothing else answers. But
  **`int`, `float`, `bang` and `list` never reach a class-level `anything`** — Max says it does not
  understand them (a list goes to an `int` method with its first atom if there is one) — so the
  class also registers `int`, `float`, `bang` and `list` methods that forward the same way; a
  Python method of one of those names, registered per instance as `tap.python~` does, answers
  before them. `symbol foo` reaches `anything` with the selector `symbol` (or a Python `symbol`).
- **The guard.** `answered_by_max()` as 1.0.2 has it — compare with what Max answers for a name no
  class can have — leaving out the object's own messages, attributes, dumpout and forwarder.
  *Verified by 9.0:* on a class with a class-level `anything`, `object_getmethod()` still answers
  an unknown name with `method_false()`, the sentinel's answer — not the forwarder — so 1.0.2's
  comparison works unchanged; it answers `anything` with the forwarder, and finds attributes,
  `get<attr>` and class methods, while methods added with `object_addmethod()` answer as unknown
  (as on `tap.python~`).
- **Reserved names.** 8.2's list less the audio names, plus `dumpout` (the object's own, `A_CANT`,
  which Max answers — 9.0); `anything` is answered by the forwarder and is not reserved by the
  guard (it leaves the forwarder out), and a glue test asserts a class's `anything` is exposed.
- **Threads.** The lock above; `systhread_isaudiothread()` on each message for the once-per-session
  notice; the console through the shared qelem (8.4).
- **The rest** is the shared glue, templated on the host: the file watcher (6.1's nobox helper,
  registered once for both hosts), attributes (3.4's reconciliation), messages, the trampolines
  (`python_glue<Host>::self(t_object*)` instead of `wrapper_find_self`), the package paths.
- **The reference page.** min writes `tap.python~`'s from `MIN_DESCRIPTION`; a plain SDK class has
  no such generator, so `docs/tap.python.maxref.xml` is written by hand from the same source of
  truth, the contract above, and kept in step by review (CLAUDE.md's "never hand-edit" is about
  the page min generates). The help patcher is by hand in Max, as 5.2 was.
- **Glue test** (mock kernel, with the stubs made faithful — audit m5): `object_obex_store`/
  `object_obex_dumpout`, `outlet_insert_after` and the box's dynlets stubbed and recorded (the
  object holds its outlet pointers and never asks `outlet_nth`, as built), and the class's
  `dumpout` method asserted registered (9.0). `[tap.python euclid]` (made through the class's own `new`, as Max would)
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
  pinned by a test of the old behavior first. *Decided 2026-10-09 (the maintainer):* that is a
  contract change under the CHANGELOG's rule, so the release that carries `tap.python` is **2.0.0**,
  not 1.1 — the rule is applied as written, however unlikely a class relied on a hint that carried
  the empty symbol. The book (9.7) follows the release.
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

- [x] **9.0 The spike, in Max on both platforms.** A throwaway second class in `tap.python~`'s
  binary, enough to answer what only Max can, written into this plan before 9.1. **Run on a Mac
  (2026-10-09, #43) and on Windows (2026-10-09, #45): every question answered, the same on both —
  the verdicts below, the evidence in [What 9.0 found](#what-90-found).** Verified in Max 9.1.5
  (3db35fa476d) on macOS 15.7.9 x86_64 and on Windows 11 x64 (a Parallels VM):
  - a plain SDK class registered beside min's in one `ext_main` loads, and both objects work in
    one patch created in either order (D11) — **yes**;
  - `init/tap.python.txt`'s `objectfile` mapping makes `[tap.python]` load the file in a fresh
    Max — or the stub external does — **the mapping does, on both; no stub is needed**;
  - `outlet_insert_after` places an outlet before the dumpout, and patch cords survive it (M1) —
    **yes, and `outlet_delete` removes exactly the outlets it is given, with their cords**;
  - `get<attr>` reaches the obex-stored dumpout (M1) — **only if the class also registers
    `object_obex_dumpout` as its `dumpout` method; without it, `get<attr>` is dropped silently**
    (Ports now says so);
  - the dispatch order of a class-level `anything` against the object's instance methods and
    attributes, and what `object_getmethod()` answers for an unknown name on such a class (M3) —
    **instance attributes and methods first; `int`, `float`, `bang` and `list` never reach
    `anything`; an unknown name is answered with `method_false()`, as on `tap.python~`** (the
    forwarder and the guard now say so);
  - which thread a `metro`-driven message runs on with Overdrive on, and with Scheduler in Audio
    Interrupt on (M4) — **the main thread (Overdrive off), the scheduler thread (on), the audio
    thread (on, with Audio Interrupt and audio running: `systhread_isaudiothread()` is 1)**;
  - `symbol` or `anything` for a `str` return, against `route`, `sel`, `prepend` and a message
    box (m2) — *the maintainer's choice (2026-10-09): the spike decides* — **`symbol <s>`** (the
    output table now says so, and why);
  - where Max picks up a vignette (`.maxvig.xml`) and a tutorial (`.maxtut.xml`) dropped into
    the package's `docs/`, and a patcher in `extras/`, for 9.4 — **anywhere under `docs/`: Max's
    file database records each, and the Documentation window lists the package under Package
    Docs; an `extras/` patcher is in the Extras menu** (seen on the Mac; which of the window's
    tabs shows which file is 9.4's to look at, when it writes them).
- [x] **9.1 The core: output, outlets and the audio option** (#46). `value.h`'s `output_item`/`output`
  and the Python-to-output conversion; `message_info::return_count` (depth-aware for strings),
  `outlet_count()`, `call_with_output()`; `bind_audio`, gating the tuple rule; the
  `list[…]`/`np.ndarray` final parameter with the old behavior pinned first; announce-once per
  kind; the `Loaded` line per binding. `test_output.cpp` and fixtures, with the output table
  exercised as one data-driven test over its edges (64-bit ints at the limit, non-finite floats,
  numpy scalars of every family, 0-d and 2-D arrays, nested sequences, a `tuple[…]` hint of each
  length against results of the wrong length), and `test_examples.cpp` running the three new
  examples on known inputs. A `core/bench` row for a message call with a list of 64 atoms, written
  by `scripts/update-perf-docs.py` with the others, so the ReadMe's performance sentence is
  measured. *(9.1: the row and the script's sentence are in; the ReadMe's table is regenerated on
  the next measurement on an idle machine, 9.6 at the latest — on 2026-10-09, with macOS's indexing
  running, a message with 64 numbers in and out measured 5.6 µs for the bridge alone and 15 µs for
  `scale.py`, and `main` and the branch measured alike back to back.)* No Max code changes; `tap.python~`'s battery and glue test unchanged and green; the
  audio bench numbers unchanged (the audio path does not touch the new code).
- [x] **9.2 The shared glue** (#47). `tap.python_tilde_{attribute,message,cglue,filewatch,package}.h`
  become `python_glue<Host>` (the trampolines instantiated per host through `Host::self()`), in
  `source/shared/tap/python_max/` or beside the object; `reserved_messages()` parameterized by the
  audio names. A pure move: `tap.python~`'s behavior, tests and the data-import check unchanged.
  *(Done beside the object, as one binary builds both: `tap.python_glue.h` holds `python_attr`,
  `python_message` and `python_members` (the attributes and messages made for a class, their
  reconciliation, attribute access and the guard) on the host, the trampolines, `start_runtime()`,
  `watch_source()` and `to_values()`; the file watcher and package paths are
  `tap.python_{filewatch,package}.h`. The same 53 reserved names in the same order; the glue test
  pins what an audio object reserves beyond every object's; every runtime test passes in Max 9.1.5.
  For 9.3: `reserved_messages(false)` still has 8.2's `anything`, which `tap.python`, exposing a
  class's `anything`, takes out — and adds `dumpout`.)*
- [x] **9.3 The object** (#48). `tap.python.h` and its TU in the project, registered by the project's
  `ext_main` (replacing the 9.0 spike's TU and its CMake option); ports with the dumpout, its
  `dumpout` method and the lock; output mapping (a `str` as `symbol <s>`); the `anything` forwarder
  with its class-level `int`, `float`, `bang` and `list` (9.0); dynamic outlets on reload; the
  guard and the reserved names (`dumpout` among them); the Scheduler-in-Audio-Interrupt notice,
  with the audio-thread predicate injected as the main-thread one is (8.4) so the glue test can
  drive it; `init/tap.python.txt` (added by 9.0); the glue test with faithful stubs, **and
  `linux-max-glue` gaining ASan/UBSan and TSan rows** — the lock and the reload race live in the
  glue, which CI builds without sanitizers today; the examples (`euclid.py`, `scale.py`,
  `note_name.py`, `default.py`'s `bang`); `package-info.json.in`, and a CI check that the assembled
  package lists `init/tap.python.txt` (9.0 made `assemble-package.py` ship `init/`); CHANGELOG
  2.0.0 started. *(As built: `t_tap_python`, what Max allocates, holds the `t_object`, the obex and
  a pointer to `control_object`, which holds the rest — the outlets, the lock, the processor, the
  members. The object keeps its outlet pointers and never asks `outlet_nth`, so the stubs made
  faithful are `object_obex_store`/`object_obex_dumpout`, `outlet_insert_after` (the mock kernel
  cannot insert: the new outlet is made by its `outlet_new`, and the call recorded) and the box's
  `dynlet_begin`/`dynlet_end` (through `object_method_imp`, which the SDK's `object_method` is on
  64-bit); `tap.python~`'s `outlet_nth` stub is unchanged. The audio-thread notice is driven
  through the kernel's `systhread_isaudiothread`, stubbed as the mock lacks it, rather than an
  injected predicate, and posted by a qelem. The mock keeps no order across outlets, so
  note_name's "outlet 1 then outlet 0" is the code's and 9.5's to show in Max (through `trigger`).
  The glue's attribute and message maps took a lock of their own (a latent race in `tap.python~`
  too, in the CHANGELOG), and its includes put CPython first again. UBSan's `vptr` check is left
  out of the glue test's sanitizer build: min calls a member before constructing the object, on
  purpose. Verified in Max 9.1.5 on the Mac with a scratch patcher, not committed — `tap.python`
  through `init/`, euclid's list, `getsteps` from the dumpout, note_name's two outlets, default's
  bang, scale through numpy — and every `tap.python~` runtime test. And on Windows 11 (the
  Parallels VM, MSVC), the glue test passing and the same checks in Max 9.1.5 read from its log:
  `tap.python` through `init/`, euclid's list and `steps 5` from the dumpout, note_name's `4` and
  then `symbol C` — right to left, which the mock cannot show — default's `1.`, scale's `2. 4. 6.`,
  and Max's `doesn't understand "nonesuch"` from the forwarder. The Windows glue test found the one
  fault: its console checks redirected `std::cout`, which a DLL's kernel does not share; the test
  now stubs `object_post`/`object_warn`/`object_error` to hear them.)*
- [ ] **9.4 Documentation, in Max's own system and the ReadMe.** The ReadMe section, the output
  table and the limits; CLAUDE.md. For the Documentation window: `docs/tap.python.maxref.xml` by
  hand, with see-also links between the two pages; a vignette, *Writing Max objects in Python* —
  the ReadMe's "Writing a class" and the threads and performance sections, rewritten for the
  window, covering both objects; and three tutorials with their patchers — a gain per sample and
  then per vector, a filter with `prepare()`, a control object (`euclid`). For the Extras menu, a
  *PythonTap Overview* patcher, named as the package's landing patcher in `package-info.json.in`
  (its `homepatcher` is empty today). The help patchers with tabs: `tap.python`'s from the start,
  and `tap.python~`'s gaining the numpy, allpass and `mc.` tabs 5.2 asked for. 9.0 found Max
  records a vignette and a tutorial anywhere under `docs/` (so `docs/vignettes/` and
  `docs/tutorials/<name>-tut/`, as Cycling '74's packages lay them out), lists the package in the
  Documentation window's Package Docs, and shows an `extras/` patcher in the Extras menu; still to
  see which of the window's tabs lists which file, and which folder `homepatcher` reads (Max's
  Packages page says `patchers/`). CI checks for what is written
  by hand: the reference page, the vignette and the tutorials well-formed XML, and the help
  patchers' JSON as 5.2 checked by hand (ids unique, every line connects), in one script. All
  checked open in Max in 9.6.
- [ ] **9.5 Runtime tests.** The patchers above in `make_patchers.py`; `run.py` aware of the
  second object (the `init/` mapping in the installed package, the `--package` mode, the
  reference-page rule for `tap.python~`'s page only). What 9.0's runner learned: a fresh Max
  needs Restore Windows on Launch off (a restored window loads the binary first); patchers are
  opened with an "open document" event, never `max openfile` through the harness's OSC (it crashes
  Max); and Max's log drops a `[print]`'s name. The soak session (6.2) gains a
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
  links — so that each fact has one home. *Decided 2026-10-09 (the maintainer):* the vignette is
  the short form, written separately — a few screens in Max's own conventions that point to the
  ReadMe and the book for the contract's facts — not generated from the book's source.

## What 9.0 found

Verified 2026-10-09 in Max 9.1.5 (3db35fa476d) on macOS 15.7.9, x86_64, and in the same Max on
Windows 11 Home (10.0.26200, x64, in a Parallels VM), with the spike built into `tap.python~`'s
binary (`-DTAP_PYTHON_SPIKE=ON`, off by default; `tap.python_spike.cpp`) — on Windows by MSVC
(Visual Studio 2026, CMake 4.3.1) with no warnings, the glue test passing. The patchers and a runner
for each platform (`run_spike.py`, `run_spike_windows.ps1`) are in `runtime-tests/spike/`; the lines
of Max's log behind every answer below are `runtime-tests/spike/results/macos.txt` and
`windows.txt`. **Every answer is the same on both platforms**; where Windows says more, it is said
below. The
spike's `tap.python` is a plain SDK class with no Python in it: a dumpout stored in the obex, value
outlets, an instance attribute and instance methods, a class attribute, a class-level `anything`,
and messages that ask Max things; a second class, `tap.python.spike.recorder`, posts each message
it receives with its selector and atom types (Max's log drops a `[print]`'s name).

**1. One binary, two classes, found through `init/` (D11).** In a fresh Max, a patcher holding only
`[tap.python]`: with `init/tap.python.txt` (`max objectfile tap.python tap.python~;`) Max loads the
`tap.python~` file, whose `ext_main` registers `tap.python~` (min) and then `tap.python`, and the
object is made; with the file moved aside, `tap.python: No such object`. Both objects in one patch,
each created first in its own fresh Max: both load, the spike's outlets output, and `tap.python~`
(`default.py`) answers `greet`. The same on Windows — the first build of a plain SDK class beside
min's in the `.mxe64` — so the stub external is not needed. A Max is fresh only if
nothing loads the binary first: with **Restore Windows on Launch** on, a window left open at the
last quit (`tap.python~`'s help patcher, here) is reopened at launch and loads it — 9.5's test of
the mapping must see to that, as the spike's runner does.

**2. Ports (M1).** The dumpout made first and the value outlets after it, last to first, give value
outlets 0…n−1 and the dumpout last (`outlet_nth` against the pointers held). From two value
outlets, with a cord from each and from the dumpout: `outlet_insert_after` the last value outlet,
between the box's `dynlet_begin` and `dynlet_end`, makes a third value outlet before the dumpout;
the dumpout's cord moves with it (now from outlet 3) and outlets 0 and 1 keep theirs; a cord made
to the new outlet by scripting carries its output. Narrowing to one with `outlet_delete` on the
last two removes exactly those and their cords, leaving the dumpout's (now from outlet 1); widening
again adds an outlet with no cord, before the dumpout. The obex's dumpout stays the one made first
throughout, and `object_obex_dumpout` reaches it after every change.

**3. `get<attr>` (M1) — a change to Ports.** Storing the dumpout in the obex is not enough. With
the class also registering `object_obex_dumpout` as its `dumpout` method (`A_CANT`), as Cycling
'74's SDK examples do (`dbviewer.c`; `dict.edit.cpp` in max-devkit), `getsteps` calls the instance
attribute's own getter and outputs `steps 5` from the dumpout (`steps 8` after `steps 8`), and a
class attribute answers `getlevel` the same way. Without that method, `get<attr>` on either kind of
attribute is **dropped silently**: no getter call, no output, no error, and the class's `anything`
never sees it — although `object_attr_method()` resolves `getsteps` to the getter and
`object_getmethod()` finds it. On Windows the method is registered by the address the module sees,
its import thunk (1.0.1's lesson), and works the same: Max calls through it. In passing: `object_attr_getdump()`, which its header says takes the
attribute's name, strips three characters from the name it is given (`steps` came out as `ps`) —
it wants the message, `getsteps`. The object has no use for it once the `dumpout` method exists.

**4. `anything` and dispatch (M3).** On a class with a class-level `anything`:

- *Instance attributes and methods come first.* `steps 8` calls the instance attribute's setter,
  `getsteps` its getter; `hello 1 2` calls the method added with `object_addmethod()`; and where
  the class and the instance both have a method of one name — `who`, and `bang`, a standard message
  — the instance's answers. On the same class with none of them, `steps 8`, `getsteps` and
  `hello 1 2` reach `anything`, and `who` and `bang` the class's methods.
- *`int`, `float`, `bang` and `list` never reach a class-level `anything`.* Without a method of
  the name, Max posts `doesn't understand "float"` (or `"int"`, `"bang"`); a list (`1 2 3`, `list
  4 5`) goes to an `int` method with its first atom if there is one, and is otherwise not
  understood as `int`. `symbol foo` reaches `anything` with the selector `symbol`; any other
  selector reaches it with its arguments. So the forwarder needs class-level `int`, `float`, `bang`
  and `list` methods beside `anything` (the `anything` item of the Max object now says so).
- *`object_getmethod()`* answers an unknown name with `method_false()` — the same as for a name no
  class can have, and equal to `method_false` as the module sees it on the Mac — on such a class
  exactly as on `tap.python~`, which has no `anything`. So 1.0.2's guard works unchanged. It
  answers `anything` with the class's `anything`; it finds instance and class attributes and their
  `get<attr>` (`steps`, `getsteps`, `level`, `getlevel`), class methods (`who`, `outlets`,
  `dumpout`, `dsp64` on `tap.python~`); and it answers methods added with `object_addmethod()` —
  the spike's `hello` and `int`, `default.py`'s `greet` and `int` on `tap.python~` — with
  `method_false()`, as unknown (`zgetfn()` finds them). `dumpout` is answered, so it joins the
  reserved names. On Windows every one of these answers is the same; only Max's `method_false()`
  is not the module's `&method_false` there (its import thunk), as 1.0.2 assumes — so the guard
  works unchanged on both.

**5. Threads (M4).** A `[metro 100]` into a message box into the object; `systhread_*` noted on the
thread the message came on:

| Setting | The message ran on | `ismainthread` | `isaudiothread` | `istimerthread` |
|---|---|---|---|---|
| (`loadbang`, for reference) | the main thread | 1 | 0 | 1 |
| Overdrive off | the main thread | 1 | 0 | 1 |
| Overdrive on | the scheduler thread | 0 | 0 | 1 |
| Overdrive and Scheduler in Audio Interrupt on, audio running | the audio thread | 0 | 1 | 1 |
| the same, audio stopped | a scheduler thread | 0 | 0 | 1 |

So `systhread_isaudiothread()` is true exactly when Python would run on the audio thread, and is
the test for the Threads section's notice; `systhread_istimerthread()` is 1 in every case (with
Overdrive off the main thread services the scheduler) and tells nothing. Max's log shows three
different posting threads for the last three rows. The patcher checks that audio really ran
(`dspstate~`) and puts both settings back as it found them. Windows (MME, 44.1 kHz): the same table,
row for row.

**6. A `str` return (m2) — decided: `symbol <s>`.** Each string was output from one outlet as the
message it names (`outlet_anything(o, gensym(s), 0, nullptr)`) and as `symbol <s>`, into `[route
C 60]`, `[sel C 60]`, `[prepend got]` and a message box `[$1(`:

| The string | As the message it names | As `symbol <s>` |
|---|---|---|
| `C` | `route` and `sel` match (a bang from `C`); `prepend` → `got C`; `$1` → `C` | `sel` matches; `route` does **not** (rejects `symbol C`); `prepend` → `got symbol C`; `$1` → `C` |
| `hello world` | rejected by both, as the message `hello world`; `prepend` → `got "hello world"` | rejected by both as `symbol "hello world"`; `$1` → `hello world` |
| `60`, `1.5` | the selector is the symbol `60`: matches neither `route 60` nor `sel 60`; `$1` → the symbol | the same: rejected as `symbol 60`; never the number |
| `list` | **nothing reaches any receiver** | arrives as `symbol list` everywhere; `$1` re-emits it as `list` — nothing |
| `int`, `float` | **`missing arguments for message "int"` from every receiver**, nothing received | arrives as `symbol int` everywhere; `$1` → the error |
| `symbol` | the message box errors; `route` passes on `symbol` with no argument, `sel` `symbol ""` | arrives as `symbol symbol` |
| `bang` | **a real bang**: the message box re-sends its last contents (here `1.5`) | arrives as `symbol bang`; `$1` → a bang |
| `""` | a message with an empty selector, passed on by every receiver | arrives as `symbol ""` |

As the message it names, six strings — `list`, `int`, `float`, `symbol`, `bang` and the empty one —
are not data: dropped, an error in every receiver, a real bang. As `symbol <s>` every string
reaches every receiver as itself, `sel` matches it, and a message box's `$1` gives back the bare
word (only there do those six behave as messages again, as they would typed into the box). Max's
own text objects output a single symbol (`sprintf symout`, `tosymbol`, `regexp @tosymbol`, by their
reference pages), and `fromsymbol` is Max's documented way back to a message. So a `str` is output
as `symbol <s>`. The costs, for the ReadMe: `[route C]` does not match it (use `[sel C]`, which
matches either, or `[route symbol]` first), and `[prepend]` keeps the word `symbol` (to display a
string, `[set $1(`). A class that means a message returns it in a list — `["start"]` — which names
the message by the sequence row, so both are expressible and the safe one is the default.

**7. Max's documentation system (for 9.4).** Vignettes, tutorials and an Extras patcher were copied
into the package and Max was started; once its file database reported ready, it held:

| Placed at | Recorded as |
|---|---|
| `docs/*.maxvig.xml`, `docs/vignettes/`, `docs/topics/` | `vignette` (XML Vignette file), each in its folder |
| `docs/*.maxtut.xml`, `docs/tutorials/`, `docs/tutorials/<name>/` | `tutorial` (XML Tutorial file), each in its folder |
| `docs/tutorials/<name>/*.maxpat` (a tutorial's patcher) | `patcher`, so the tutorial's `openfile` finds it by name |
| `extras/PythonTap Spike Extras.maxpat` | `patcher` in `Package:/PythonTap/extras` |

So Max reads the files from anywhere under `docs/`; 9.4 can lay them out as Cycling '74's packages
do (`docs/vignettes/`, `docs/tutorials/<name>-tut/`). And in Max itself, with them in place: the
Extras menu lists **PythonTap Spike Extras** among the packages' entries, and the Documentation
window's Package Docs lists **PythonTap** beside the packages that ship vignettes or tutorials
(bach, cage, Ease, FrameLib, HIRT, Jamoma, Link, Node for Max, RNBO, Zero), with its Topics,
Guides and Tutorials tabs. Which tab lists which of the files was not looked at (macOS refused the
UI scripting mid-way); 9.4 sees it when it writes the real ones. On Windows the database was still
scanning when Max was quit — in this VM Windows' Documents folder is the Mac's, so Max indexes
every one of the Mac's packages over the share — so `windows.txt` says the query is incomplete;
nothing here is platform-specific. And Max's Packages page describes `homepatcher` as a patcher in
the package's `patchers/` folder, where 9.4 plans the overview in `extras/`: 9.4 checks which
`homepatcher` finds, or ships it in both.

**For later items, learned on the way.** The harness's OSC dispatch (`oscar`) calls every method
as `(symbol, argc, argv)`: `max openfile` — Max's, typed with two symbols — crashed Max reading
`argc` as a symbol; the runner opens patchers with an "open document" event instead (9.5). Max's
own log (`Logs/Max.log`) has every console line with its thread, but not a `[print]`'s name (9.5).
And methods added with `object_addmethod()` are not found by `object_getmethod()` (4 above) —
CLAUDE.md said an object's own messages answer their names, as its attributes do; only the
attributes do, which the guard was right to leave out either way (CLAUDE.md now says so; the
comment on `answered_by_max()` said the same, and 9.2's move corrected it).

**On Windows, learned on the way** (for 9.5 and 9.6, which run Windows by hand). A patcher opened
together with Max (`Max.exe "<patcher>"`, as Explorer opens one) had its scheduler held for
seconds after loading, so every `delay` in it fired at once: the Windows runner opens the patcher
under test from a starter patcher once Max has settled. The glue test needs `support\` on `PATH`,
as CI gives it; without it the test stops at a modal "python313.dll was not found". And a first
Q5 run, before the patcher checked `dspstate~`, showed no audio thread at all — the evidence that
audio ran is what makes that row mean anything.

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

*2026-10-09, after the maintainer's review — documentation and test coverage; and the three
decisions put to the maintainer: the release is 2.0.0 (the `list[…]` parameter is a contract
change, the CHANGELOG's rule applied as written); the `str` return is the spike's to decide;
the vignette is written separately as the short form:*

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

*2026-10-09, the 9.0 spike in Max on a Mac (Max 9.1.5; #43; Windows still to run) — answers in
[What 9.0 found](#what-90-found):*

- **D11 / How Max finds it:** the `init/` mapping verified; `init/tap.python.txt` added and shipped.
- **Ports:** the class registers `object_obex_dumpout` as its `dumpout` method, without which
  `get<attr>` is silently dropped; `outlet_insert_after`/`outlet_delete` verified against the
  dumpout and the cords.
- **`anything`:** instance attributes and methods win over a class-level `anything`, and an
  instance method over a class method of its name; `int`, `float`, `bang` and `list` never reach
  `anything`, so the class registers forwarders for them too.
- **The guard:** unknown names are answered with `method_false()` on a class with `anything` too —
  1.0.2's comparison stands; `dumpout` joins the reserved names.
- **Threads:** the main, scheduler and audio threads as D10 says; `systhread_isaudiothread()`
  detects Scheduler in Audio Interrupt.
- **Output:** a `str` is `symbol <s>` (the maintainer left it to the spike); a list names a
  message.
- **9.4 and 9.5:** where Max reads vignettes, tutorials and extras; what the runner learned.

*2026-10-09, the 9.0 spike on Windows (Max 9.1.5 in a Windows 11 VM; #45) — 9.0 done:*

- **Every answer the same as on the Mac:** the `init/` mapping (so no stub), the plain SDK class in
  the `.mxe64`, the ports and the dumpout method (registered through the import thunk), the
  dispatch and the guard's sentinel, the threads, the strings. 9.0 is ticked.
- **Q7 on the Mac, in Max's own windows:** the Extras menu lists an `extras/` patcher, and Package
  Docs lists the package; which tab lists which file is left to 9.4.
- **Q5's patcher now checks that audio ran** (`dspstate~`), after a run that could not tell.
- **9.5/9.6 notes:** how to drive Max on Windows (a starter patcher, `support\` on `PATH`).
