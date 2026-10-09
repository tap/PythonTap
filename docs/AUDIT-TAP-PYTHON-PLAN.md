# Audit of the tap.python plan (2026-10-02)

An adversarial read of `docs/TAP-PYTHON-PLAN.md` as committed in `18f3c10`, before any of it is
built. It asks where the plan is wrong, unverified, inconsistent with itself or the codebase, or
would fail in a real Max the way 1.0.0 did (a guard that the mock kernel passed and Max broke: PR
#37). Two reviewers read it independently, and every finding kept here was checked against the
source by hand. Each is marked **verified** (shown against the code, the SDK headers or a run) or
**unverified** (plausible, but only Max can show it).

## Verdict

**Do not start 9.1 as planned.** The design's user-facing contract (return values as output,
hints as outlet counts, one inlet, messages on their own thread) survives the audit. Its
architecture does not. Two findings block:

- **The second external kills Max** (B1). Two externals each compile in their own copy of the
  header-only core, and the second one's start-up aborts the process.
- **The order of work repeats 1.0.0** (B2). The plan builds five PRs before anything runs in Max,
  yet most of its riskiest assumptions are behaviors only Max can show.

Six major findings change the design: outlets, concurrency, `anything`, Scheduler in Audio
Interrupt, announce-once, and the contract.

| | Blocker | Major | Minor |
|---|---:|---:|---:|
| Findings | 2 | 6 | 9 |

## Blockers

### B1. A second external aborts Max when it starts the interpreter (verified)

**The plan says** (D7, D11): "A second external in the same package, over the same core", with
"the same loader (a file saved once is executed once however many objects of either kind share
it)", and "the core gains one option".

**The problem.** The core is header-only, and its process-wide state lives in function-local
statics: `initialize()`'s `std::once_flag` (`runtime.h:791`), the support module's globals
(`support_globals()`), the console, the scripts folder, and `init_thread()`. Each external
compiles in its own copy of all of them, but both load the same libpython. The second external's
`call_once` runs again, and calls `PyImport_AppendInittab()` after `Py_Initialize()`.

**Reproduced here.** Two shared objects, each built with the core and `-fvisibility=hidden`, were
loaded into one process with `dlopen(RTLD_LOCAL)` and each called `initialize()`:

```
./ext_a.so: initialize ok=1
Fatal Python error: PyImport_AppendInittab: PyImport_AppendInittab() may not be called after Py_Initialize()
Aborted (exit 134)
```

**On each platform:**

- **Windows.** Each `.mxe64` is a DLL with its own statics, so this happens as shown: a patch
  with both objects aborts Max.
- **macOS.** It depends on whether dyld merges the two bundles' weak inline statics, and only Max
  can show which. If dyld does not merge them, Max aborts as above. If it does, the two externals
  share the core's state by accident, including each one's console sink and thread checks, which
  is no design either.
- **Every test binary links one copy of the core**, so no existing test can see this.

**Even without the abort**, a second support module means a second source cache. A shared file
would then be executed once per kind, and 6.7 (announce once) and 6.10 (report once) would break
across kinds.

**It also contradicts the ReadMe**, which calls two embedded copies sharing process-wide state
"untested and unsupported".

**Recommend.** Decide this in D11 before anything else:

- **(a) One core, two front ends** (preferred). The core becomes one shared library in the
  package (`support/` or beside the externals), which both externals link, so there is one copy of
  every static.
- **(b) Attach instead of re-initialize.** `initialize()` detects `Py_IsInitialized()` and attaches
  to state the first binary published in `sys.modules`: the support module, the console module and
  the sink. The second binary skips `AppendInittab`, pre-initialization and stream rebinding, and
  keeps its own thread-state handling.
- **(c) One external.** A single `tap.python~` binary also registers the `tap.python` class (a
  second `class_new` in its `ext_main`), if Max can find a class that is not named for its file.
  This is the smallest change if Max allows it; the 9.0 spike can say.

Whichever is chosen, add a core test that starts the runtime from two shared objects in one
process, and a runtime test with both objects in one patch, created in both orders.

### B2. The order of work repeats the 1.0.0 failure (verified, plan text)

**The plan says:** 9.1 to 9.5 land first, and "The Mac session that runs them is the phase's last
item" (9.6). The production plan adds that Phase 9 can run "after 8.8, or beside it: nothing in it
touches what 8.8 checks."

**The problem.** The plan marks one thing "to check in Max" (the dumpout). Yet B1, M1, M2, M3 and
M4 below are all behaviors only Max shows. And 9.2 moves `answered_by_max()`, `found_method()` and
the file watcher into shared code: exactly what 8.8, still open, has to verify. The plan's own
record of 1.0.0 says "a tag with a Mac session outstanding ships what only Max can show."

**Recommend.** Two changes:

- **Run 8.8 (with PR #37's fix) before 9.2** moves the guard.
- **Add 9.0, a Max spike, before 9.1.** A throwaway second external, built as B1's chosen design,
  next to `tap.python~` in one patch. In Max, confirm:
  - both objects start, in either creation order;
  - `outlet_insert_after` places an outlet before the dumpout, and patch cords survive it;
  - `get<attr>` reaches an obex-stored dumpout;
  - how a class-level `anything` and instance methods and attributes take turns in dispatch (M3);
  - what `object_getmethod()` answers for an unknown name on a class with `anything`;
  - which thread a `metro`-driven message runs on with Overdrive on, and with Scheduler in Audio
    Interrupt on.

  Write the answers into the plan before 9.1.

## Major

### M1. min's outlets cannot do what "Ports" describes (verified)

**The plan says:** value outlets "plus a last `outlet<> m_dumpout{this, "dumpout"}` stored in the
obex", new outlets by `outlet_insert_after`, "min's lists following". It leaves open "whether min's
`outlet<>` named `dumpout` is enough".

**The evidence** is in min's outlet header and the SDK:

- **Order.** An `outlet<>` pushes itself onto the object's list as it is constructed
  (`c74_min_outlet.h:312`). A member `m_dumpout` is constructed before the constructor body adds
  the class's outlets (as `tap.python~`'s `describe_ports()` does). So the dumpout would be the
  second outlet, not the last.
- **No names.** The string is a description; min has no outlet names. Only
  `object_obex_store(x, _sym_dumpout, …)` makes `get<attr>` work, as the SDK's own example shows
  (`ext_obex.h`, at `object_obex_store`). That settles the plan's open question.
- **No Max outlet behind a later `outlet<>`.** min makes the Max outlet in a private `create()`,
  called only once, by `create_outlets()` at instantiation (`c74_min_outlet.h:438`). An `outlet<>`
  added on a reload keeps a null instance, and sending through it calls `outlet_list(nullptr, …)`.
  `tap.python~` gets away with this because min never sends through its signal outlets. A control
  outlet would crash.

**Recommend.** Specify the ports outright:

- Make the value outlets, then the dumpout, in the constructor, in that order.
- Store the dumpout with `object_obex_store()` once the Max outlets exist (min's `setup` stage or
  later).
- Send through raw outlet pointers taken from `outlet_nth()` and `outlet_insert_after()`, never
  through `outlet<>::send()`. Keep min's list only for assist text, as `tap.python~` does.

### M2. A reload can change outlets while another thread is outputting (verified gap; Max side unverified)

**The plan says:** the `outlet_*` calls come after the GIL is released, and "a message that arrives
on the scheduler thread while a reload runs waits for the GIL".

**The problem.** Once the GIL is released, nothing orders a message's output against a reload's
port changes. The reload runs on the main thread, without the GIL, and makes these changes:

- `outlet_delete` and `outlet_insert_after` on Max's outlets;
- edits to min's outlet vector;
- `object_deletemethod` and `object_addmethod` for the class's messages.

Under Overdrive, a scheduler-thread message can compute output for three outlets, release the GIL,
and index an outlet the main thread has just deleted: a use-after-free. The SDK documents nothing
about `outlet_insert_after` or `outlet_delete` (`ext_proto.h:465-469`, declarations only), let
alone their thread safety.

**Recommend.** Add a per-object lock, held while a message maps its output onto outlets and while
a reload changes ports. Under it, a message checks that its outlet count is still current and
drops the output (once, reported) if not. Never hold it across a call into Python. Add a glue test
that races a widening reload against a second thread's messages.

### M3. `anything` and the guard rest on Max dispatch the plan has not checked (unverified)

**The plan says:** a min `message<> m_anything` forwards unknown selectors, and "the guard excludes
it by name".

**What is known:**

- min registers `anything` as a class method (`c74_min_object_wrapper.h:580`).
- The Python methods are instance methods (`object_addmethod`), and the fields are instance
  attributes. The SDK says of instance methods: "these methods are private -- instance methods are
  not actually fully implemented at this time" (`ext_obex.h:2296`).
- Today no class has both, because `tap.python~` reserves `anything`.

**What is not known:**

- whether Max tries instance methods and instance attributes (`steps 8`, `getsteps`) before a
  class's `anything`;
- whether `object_getmethod()` answers an unknown name with the `anything` method on such a class.
  If it does, the guard reserves every name: 1.0.0 again, and excluding `anything` by name does not
  help.
- The test kernel from PR #37 answers from a fixed list, so it cannot reveal either.

**Recommend.**

- Do not give the class a min `anything`. Register `anything` per instance, and only when the
  Python class defines it, so classes without one keep exactly `tap.python~`'s dispatch.
- Make the forwarder proof against dispatch order. Before calling the Python `anything`, it tries
  the selector as a message, then as an attribute set, then as `get<name>`.
- Make the guard ignore an answer that is the object's own forwarder.
- Settle the dispatch order in the 9.0 spike.

### M4. D10 leaves out Scheduler in Audio Interrupt (omission verified; Max behavior unverified)

**The plan says:** a message runs "on Max's main thread, or the scheduler thread under Overdrive",
and "worker mode (2.5) is the answer when both are in one patch and the computation is heavy".

**The problem.** With Scheduler in Audio Interrupt on, the scheduler runs on the audio thread. A
`metro`-driven `tap.python` method then runs Python there, and waits for the GIL behind a reload
on the main thread, which holds it for milliseconds.

- **Worker mode does not help**, because the heavy work itself is on the audio thread.
- **CLAUDE.md's guarantee breaks.** CLAUDE.md promises that in worker mode the audio thread "never
  takes the GIL". That stops being true for any patch that also has a `tap.python`.
- **The plan, the production plan and the ReadMe never mention the setting.** The SDK has
  `systhread_isaudiothread()` (`ext_systhread.h:180`) to detect it.

**Recommend.**

- **State it as an honest limit**, with its consequence: dropouts while Python runs or waits.
- **Decide what the object does about it:** warn once, or defer such messages to the main thread
  (an attribute, or the default).
- **Correct D10's advice.**

### M5. Announce-once hides diagnostics when the two objects apply different rules (verified)

**The plan says** of announce-once only that the `Loaded` line "describes whichever ran the save".

**The problem.** Every class diagnostic goes through `announce()`, which only the processor that
ran the file speaks (`m_announcing = executed`, `processor.h:187`). The two kinds disagree on
exactly what those diagnostics report:

- `mode`, `latency`, `latencysamples` and `anything` are reserved only in `tap.python~`;
- methods with an unsaid-length tuple hint are dropped only in `tap.python`.

So if a `tap.python` runs the save, a `tap.python~` on the same file silently loses a field named
`mode`, and the reverse happens too.

**Recommend.** Key announce-once on the file and the object kind, or have each kind announce what
differs. Pin it with two processors whose reserved names and `bind_audio` differ.

### M6. 9.1 changes `tap.python~`'s contract while saying it does not (verified)

**The plan says** 9.1 leaves "`tap.python~`'s battery and glue test unchanged", and the CHANGELOG
should say the `list[…]` parameter "only adds".

**The evidence:**

- **List parameters work differently today.** `hint_kind(list[float])` is `'list'`, which
  `value_type_from_hint` maps to a symbol (`value.h`). So today `foo 1` passes `""` to a parameter
  hinted `list[float]`, and an `np.ndarray` parameter receives a `str`. Taking all the remaining
  atoms changes what existing classes receive and how many arguments they accept.
- **The element type is missing.** `hint_kind` does not report it, so the support module must
  change too. The plan does not say so.
- **The tuple rule has no gate.** Not exposing a method with an unsaid-length tuple return must be
  limited to `tap.python`. Today `bind_message` ignores return hints (`processor.h:1390`), so an
  ungated change removes messages from `tap.python~` classes.
- **The CHANGELOG's policy** (`CHANGELOG.md:5`): "From 1.0.0, a change to the class contract is a
  major version."

**Recommend.**

- **Gate the tuple rule on `bind_audio`.**
- **Record the list parameter as a change, with a test pinning today's behavior first** (the
  house rule). Either call the release 2.0, or argue in the plan that a hint that never worked is
  not contract. Then the CHANGELOG states that, rather than "only adds".
- **Output an unsaid-length tuple as a list** from the first outlet rather than hiding the method,
  which keeps D7's "a class written for one object loads in the other".

## Minor

- **m1. The output table contradicts itself and numpy (verified).**
  - The sequence row covers "any non-str iterable", which includes the `dict` that the last row
    reports. It also includes `bytes`, an unordered `set`, and one-shot generators.
  - `np.bool_` has no `__index__` in numpy 2.5.3 but does convert with `float()` (checked: `1.0`).
    So by the table it is output as a float, against the `bool` row and the ReadMe's rule that
    numpy's bool counts as `bool`.
  - **Recommend:** list the accepted sequence types, and add `np.bool_` to the bool row.
- **m2. A `str` return as a bare selector (design; Max behavior unverified).**
  - `"list"`, `"int"`, `"float"` and `""` become malformed zero-argument messages.
  - `"60"` becomes a selector, not a number.
  - The example `note_name.py` outputs the pitch class `C` as a message named `C`, where most Max
    objects that output a name send `symbol C`.
  - **Recommend:** decide between `anything` and `symbol` in the 9.0 spike, against real
    downstream objects (`route`, `sel`, `prepend`, a message box's `$1`), and say what happens to
    the reserved selectors.
- **m3. `call()` cannot be overloaded on its return type (verified).** The new `call()` would have
  the same parameters as the existing `bool call(std::string_view, std::span<const value>)`
  (`processor.h:403`). It needs another name, such as `call_with_output()`.
- **m4. Outlet counts from string hints count nested commas (verified).** `return_shape` splits a
  string hint on every comma (`runtime.h:559`). Checked:
  - the hint `'tuple[list[int], dict[str, int]]'` gives 3;
  - the same hint as an object gives 2.

  Outlet counts become patch cords, so this needs the depth-aware split that `_split_union`
  already has.
- **m5. The planned mock tests cannot see what they claim (verified).**
  - The glue test's `outlet_nth` stub returns a made-up pointer (`n + 1`), while the mock records
    output by real outlet id. Output through such a pointer would land on another object's outlet,
    or nowhere.
  - `object_obex_store`, `object_obex_dumpout` and `outlet_insert_after` are not stubbed at all.
  - **Recommend:** make the stubs faithful (CLAUDE.md's rule since PR #37).
- **m6. The file watcher's class name would be registered twice (unverified).** A "pure move"
  keeps `class_new("tap.python~.filewatch", …)` and its `nobox` registration in both binaries.
  Name it per host, or let B1's single core own it.
- **m7. The examples invite the class-body shadowing trap (verified).** Once `def list(...)` or
  `def int(...)` is defined, a later annotation in the same class using `list[...]` or `int`
  refers to the method. On 3.13: `TypeError: 'function' object is not subscriptable`.
  `default.py` already warns about this. The ReadMe section and the new examples must too.
- **m8. D10 contradicts the Threads section, and overstates the GIL (verified).**
  - D10 says a message holds the GIL "until it returns, and outputs on that thread". The Threads
    section says the outlet calls come after the GIL is released.
  - D10's "waits for the GIL in 0.5 ms slices" holds only for Python bytecode. A single long C
    call (a numpy operation) holds the GIL throughout, as the production plan's 2.6 says itself
    ("what a switch cannot interrupt is a single C call").
- **m9. An unsourced claim about `js` (verified, plan text).** "Non-finite values pass through, as
  `js` passes them" cites nothing, and `tap.python~` zeroes non-finite output. Either cite Max's
  documentation for `js`, or choose on the merits: Max's own number boxes and arithmetic objects
  can propagate NaN downstream.

## What the plan should change, in order

1. **Fix 1.0.0 and run 8.8 first:** PR #37, then the Mac session.
2. **Rewrite D11 around one core** (B1's option a, b or c), and record the decision.
3. **Add 9.0, the Max spike** (B2), and write its answers into the plan:
   - two objects in either order;
   - outlet insertion before a dumpout;
   - `get<attr>` through the dumpout;
   - `anything` dispatch and `object_getmethod()` on such a class;
   - the threads under Overdrive and under Scheduler in Audio Interrupt;
   - `symbol` or `anything` for a `str` return.
4. **Respecify Ports** (M1) and add the per-object output lock (M2).
5. **Redesign `anything`** as an instance method registered only when defined, with a forwarder
   that does not depend on dispatch order (M3).
6. **Add Scheduler in Audio Interrupt to D10 and the ReadMe's limits** (M4).
7. **Key announce-once per object kind** (M5).
8. **Gate or record the contract changes**, with tests pinning today's behavior first (M6).
9. **Fix the minor items** in the text and the planned tests.

## Addendum, 2026-10-09: after 1.0.1 and 1.0.2

The audit above was written against 1.0.0 with PR #37's fix. Since then the Mac session (8.8) ran,
1.0.1 was tagged and then superseded, and 1.0.2 was released. What that changed, and did not:

- **The verdict and every finding stand.** Nothing since has answered a Max-only question the audit
  raised, and the two blockers are untouched.
- **B1 is reinforced.** 1.0.1 failed on Windows because the external's `&method_false` is its own
  import thunk, not the function Max returns: a function's address is not its identity across a
  DLL boundary. Two externals, each with its own copy of the core, would carry two of every
  function the core hands to CPython as well as two of every static. One core is the first
  decision to make, as B1 says.
- **B2's prerequisite is met, and the spike needs Windows too.** 8.8 ran on 2026-10-07 (the whole
  suite passes; two items stay by hand), so "run 8.8 before 9.2" is done. But 1.0.0, 1.0.1 and the
  Windows package of 1.0.1 each shipped something only a host platform could show, and the third
  was Windows-only. The 9.0 spike runs on a Mac *and* on Windows, before 9.1.
- **M3's guard half is likely resolved by 1.0.2's design**, which asks Max what it answers for a
  nonsense name and compares every lookup with that (`not_found_method()`). If a class-level
  `anything` makes Max answer every unknown name with the forwarder, the sentinel is the forwarder
  too, so unknown names still read as "not found"; if Max answers `method_false()` regardless, the
  sentinel is that. Either way the guard no longer needs to know what "not found" looks like. The
  other half of M3, the dispatch order between `anything`, instance methods and instance
  attributes, is still unverified. One piece of evidence from 8.8: an attribute the object adds
  with `object_addattr` *is* found by `object_getmethod()` under its name (why a reload reserved
  every field in 1.0.1), so attribute names take part in the object's own lookup. That is
  consistent with attributes winning over `anything`; it is not proof, and stays a spike item.
- **A new rule for the plan's guard, from 1.0.1:** everything the object itself registers answers
  its name — messages, attributes, and for `tap.python` the dumpout and any per-object `anything`
  forwarder — and the guard must leave all of it out, as `answered_by_max()` now leaves out its
  own messages and attributes.
- **Stale references in the text above:** "run 8.8 before 9.2" (done); the glue test's kernel
  "answers from a fixed list" (it now also models added attributes and a Max-side "not found"
  that is not the module's `method_false`); and `found_method()` "treats null and
  `method_false()` alike", which describes 1.0.1 — 1.0.2 compares with what Max answers. The
  plan's 9.2 moves the 1.0.2 code.
- **The icon is now guarded.** TapHouse v6's drift check compares each Max package's `icon.png`
  with its own render, byte for byte; `tap.python`'s package would share it (one package), so
  nothing new is needed for Phase 9.
