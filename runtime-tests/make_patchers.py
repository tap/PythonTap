#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
"""Generate tap.python~'s runtime-test patchers (plan 6.1) for the max-test harness.

    python3 runtime-tests/make_patchers.py      # rewrites runtime-tests/patchers/
    python3 runtime-tests/make_patchers.py --soak-minutes 3   # a short soak, to try it (don't commit)

Each test is written here as a script — wait, send a message, sample the signal, read an
attribute, count console errors — and generated as a max-test patcher: it starts itself from a
loadbang, runs its steps through a chain of [delay] and [t b b …] objects, checks results with
test.assert, and ends with test.terminate. Commit the generated patchers with this file; never
edit them by hand. Run them with runtime-tests/run.py (see runtime-tests/README.md), or open one
in Max to watch it (its test.* objects record nothing unless the harness opened it).

The Python classes the tests load are in runtime-tests/python/ (run.py copies them into the
package's python/ folder for the run); the tests also load the shipped examples.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

TESTS = Path(__file__).resolve().parent
PATCHERS = TESTS / "patchers"

APPVERSION = {"major": 9, "minor": 0, "revision": 8, "architecture": "x64", "modernui": 1}

# How long to let things settle, in milliseconds. A sample is taken on the next signal vector and
# reported through the scheduler; the file watcher needs a moment to notice a save.
SETTLE = 150
WATCH = 2500
FINISH = 500  # before test.terminate stops audio: the last samples must have been taken
LOGGED = 50  # console errors logged per test
WATCHDOG = 60000  # a test that has not ended by now is ended, failing whatever it had not checked
SOAK_MINUTES = 60  # plan 6.2: an hour

HOST = "maxtest.host"  # the poly~ abstraction that hosts a [tap.python~ #1] (see write_host)


class Patcher:
    """A patcher's boxes and patch cords, laid out in columns."""

    COLUMN_WIDTH = 250

    def __init__(self, title: str, description: str):
        self.title = title
        self.boxes: list[dict] = []
        self.lines: list[dict] = []
        self.next_y: dict[int, float] = {}
        self.count = 0
        self.comment(f"{title}\n\n{description}", column=0, width=self.COLUMN_WIDTH * 2 - 20, lines=4)

    def _place(self, column: int, height: float) -> tuple[float, float]:
        y = self.next_y.get(column, 20.0)
        self.next_y[column] = y + height + 8
        return 20.0 + column * self.COLUMN_WIDTH, y

    def box(self, text: str, inlets: int = 1, outlets: int = 1, column: int = 1, outlettype=None,
            maxclass: str = "newobj", width: float = 0, **extra) -> str:
        self.count += 1
        box_id = f"obj-{self.count}"
        x, y = self._place(column, 22)
        box = {
            "id": box_id,
            "maxclass": maxclass,
            "numinlets": inlets,
            "numoutlets": outlets,
            "outlettype": outlettype if outlettype is not None else [""] * outlets,
            "patching_rect": [x, y, width or max(40.0, 7.0 * len(text) + 16), 22.0],
        }
        if maxclass in ("newobj", "message"):
            box["text"] = text
        box.update(extra)
        self.boxes.append({"box": box})
        return box_id

    def message(self, text: str, column: int = 1) -> str:
        return self.box(text, inlets=2, outlets=1, column=column, maxclass="message")

    def comment(self, text: str, column: int = 1, width: float = 230, lines: int = 1) -> str:
        self.count += 1
        box_id = f"obj-{self.count}"
        height = 20.0 * lines
        x, y = self._place(column, height)
        self.boxes.append({"box": {"id": box_id, "maxclass": "comment", "numinlets": 1, "numoutlets": 0,
                                   "patching_rect": [x, y, width, height], "text": text,
                                   "linecount": lines}})
        return box_id

    def connect(self, source: str, outlet: int, destination: str, inlet: int = 0) -> None:
        self.lines.append({"patchline": {"source": [source, outlet], "destination": [destination, inlet]}})

    def document(self) -> dict:
        width = self.COLUMN_WIDTH * (max(self.next_y) + 1) + 40
        height = max(self.next_y.values()) + 40
        return {"patcher": {
            "fileversion": 1,
            "appversion": APPVERSION,
            "classnamespace": "box",
            "rect": [60.0, 80.0, width, min(height, 900.0)],
            "default_fontsize": 12.0,
            "default_fontname": "Arial",
            "gridsize": [15.0, 15.0],
            "description": self.title,
            "boxes": self.boxes,
            "lines": self.lines,
        }}


class Test:
    """A runtime test: a patcher plus a script of steps started by loadbang.

    Each step waits, then bangs its actions in order. The actions are boxes that do something
    when banged: a message box sending to the object under test, or one of the checks below,
    each of which ends in a test.assert named for what it promises.
    """

    def __init__(self, filename: str, description: str, audio: bool = True, watchdog: int = WATCHDOG):
        self.filename = filename
        self.audio = audio
        self.watchdog = watchdog
        self.patcher = Patcher(filename.split(".maxtest")[0].split(".maxpat")[0], description)
        self.steps: list[tuple[int, list[str]]] = []
        self.errors = ""  # the [error 1] box, made on first use
        self.error_counters: dict[str, str] = {}

    # -- things to test ---------------------------------------------------------------------

    def obj(self, text: str, inlets: int = 1, outlets: int = 1, signal: bool = False, column: int = 2) -> str:
        return self.patcher.box(text, inlets, outlets, column=column,
                                outlettype=["signal"] * outlets if signal else None)

    def python(self, arguments: str, column: int = 2) -> str:
        """A [tap.python~ …] box (one signal inlet, one signal outlet)."""
        return self.obj(f"tap.python~ {arguments}".strip(), signal=True, column=column)

    def signal(self, value: float, column: int = 2) -> str:
        return self.obj(f"sig~ {value}", signal=True, column=column)

    def host(self, python_class: str = "", voices: int = 1, column: int = 2) -> str:
        """A [poly~ maxtest.host] running `voices` instances of [tap.python~ python_class]. With no
        class, nothing is loaded until the actions from load() run: the object is then created
        after the patcher has loaded and its patch cords exist, so what its constructor prints is
        caught — but such a poly~ has no signal outlets to connect."""
        text = f"poly~ {HOST} {voices} args {python_class}" if python_class else "poly~"
        return self.obj(text, inlets=1, outlets=1, signal=True, column=column)

    def load(self, host: str, python_class: str, voices: int = 1) -> list[str]:
        """Actions loading `voices` instances of [tap.python~ python_class] into `host`."""
        return [self.send(f"args {python_class}", host),
                self.send(f"voices {voices}", host),
                self.send(f"patchername {HOST}", host)]

    # -- actions -----------------------------------------------------------------------------

    def send(self, text: str, destination: str, inlet: int = 0) -> str:
        """A message box sending `text` to `destination`; bang it to send."""
        box = self.patcher.message(text, column=1)
        self.patcher.connect(box, 0, destination, inlet)
        return box

    def dsp(self, on: bool = True) -> str:
        return self.patcher.message("; dsp start" if on else "; dsp stop", column=1)

    def _assert(self, name: str, source: str, outlet: int = 0) -> str:
        box = self.patcher.box(f"test.assert {name}", column=3)
        self.patcher.connect(source, outlet, box)
        return box

    def sample_equals(self, name: str, source: str | None, expected: float = 0.0, operand=None) -> str:
        """Check that `source`'s signal is `expected` (within 2 ulp) on the next vector; or, given
        `operand` (a box, outlet), whatever that last sent. With no source, connect the signals
        to check to the returned box: they sum."""
        sample = self.patcher.box("test.sample~ @autorun 0", column=3)
        equals = self.patcher.box(f"test.equals {expected}", inlets=2, column=3)
        if source:
            self.patcher.connect(source, 0, sample)
        self.patcher.connect(sample, 0, equals)
        if operand:  # as a float: test.equals takes only floats
            to_float = self.patcher.box("t f", column=3)
            self.patcher.connect(operand[0], operand[1], to_float)
            self.patcher.connect(to_float, 0, equals, 1)
        self._assert(name, equals)
        return sample

    def sample_compare(self, name: str, source: str, compare: str) -> str:
        """Check `source`'s signal on the next vector with a Max comparison, e.g. ">= 1."."""
        sample = self.patcher.box("test.sample~ @autorun 0", column=3)
        test = self.patcher.box(compare, inlets=2, column=3)
        self.patcher.connect(source, 0, sample)
        self.patcher.connect(sample, 0, test)
        self._assert(name, test)
        return sample

    def attribute_equals(self, name: str, target: str, attribute: str, expected) -> str:
        """Check the value of `target`'s attribute through [getattr], as a patch would read it."""
        # read when banged only: listening, it would answer again whenever a message set the attribute
        query = self.patcher.box(f"getattr {attribute} @listen 0", outlets=3, column=3)
        self.patcher.connect(query, 1, target)
        if isinstance(expected, str):
            select = self.patcher.box(f"sel {expected}", inlets=2, outlets=2, column=3)
            yes = self.patcher.message("1", column=3)
            no = self.patcher.message("0", column=3)
            self.patcher.connect(query, 0, select)
            self.patcher.connect(select, 0, yes)
            self.patcher.connect(select, 1, no)
            self.patcher.connect(no, 0, self._assert(name, yes))
        else:
            # an int attribute reads as an int, which test.equals (float only) does not take
            test = self.patcher.box(f"== {expected}" if isinstance(expected, int) else f"test.equals {expected}",
                                    inlets=2, column=3)
            self.patcher.connect(query, 0, test)
            self._assert(name, test)
        return query

    def attribute_compare(self, name: str, target: str, attribute: str, compare: str) -> str:
        """Check `target`'s attribute through [getattr] with a Max comparison, e.g. ">= 10"."""
        query = self.patcher.box(f"getattr {attribute} @listen 0", outlets=3, column=3)
        test = self.patcher.box(compare, inlets=2, column=3)
        self.patcher.connect(query, 1, target)
        self.patcher.connect(query, 0, test)
        self._assert(name, test)
        return query

    def no_change(self, name: str, source, value: float) -> tuple[str, str]:
        """Watch `source` (a box, or several whose signals sum) sample by sample: returns (start,
        check) actions. Bang `start` to begin watching; bang `check` to assert that every sample
        since was exactly `value`."""
        differs = self.patcher.box(f"!=~ {value}", inlets=2, column=3, outlettype=["signal"])
        for each in source if isinstance(source, list) else []:
            self.patcher.connect(each, 0, differs)
        source = None if isinstance(source, list) else source
        peak = self.patcher.box("peakamp~", inlets=2, column=3)
        start = self.patcher.box("t b b", outlets=2, column=3)
        check = self.patcher.box("t b", column=3)
        gate = self.patcher.box("gate", inlets=2, column=3)
        opened = self.patcher.message("1", column=3)
        zero = self.patcher.box("== 0", inlets=2, column=3)
        if source:
            self.patcher.connect(source, 0, differs)
        self.patcher.connect(differs, 0, peak)
        # starting: read (and so reset) the peak with the gate closed, then open it
        self.patcher.connect(start, 1, peak)
        self.patcher.connect(start, 0, opened)
        self.patcher.connect(opened, 0, gate)
        self.patcher.connect(check, 0, peak)
        self.patcher.connect(peak, 0, gate, 1)
        self.patcher.connect(gate, 0, zero)
        self._assert(name, zero)
        return start, check

    def metro(self, interval_ms: int, *targets: str) -> tuple[str, str]:
        """A [metro] banging `targets` (in order): returns (start, stop) actions."""
        metro = self.patcher.box(f"metro {interval_ms}", inlets=2, column=1)
        fan = self.patcher.box("t " + " ".join(["b"] * len(targets)), outlets=len(targets), column=1)
        self.patcher.connect(metro, 0, fan)
        for index, target in enumerate(targets):
            self.patcher.connect(fan, len(targets) - 1 - index, target)
        return self.send("1", metro), self.send("0", metro)

    def log_value(self, label: str, source: str, outlet: int = 0) -> None:
        """Log what `source` outputs into the test's results as "<label> <value>" (run.py prints
        these for the soak test)."""
        prefix = self.patcher.box(f"prepend {label}", inlets=2, column=4)
        log = self.patcher.box("test.log measure", outlets=0, column=4)
        self.patcher.connect(source, outlet, prefix)
        self.patcher.connect(prefix, 0, log)

    def log_attribute(self, label: str, target: str, attribute: str) -> str:
        """An action logging `target`'s attribute; bang it to log."""
        query = self.patcher.box(f"getattr {attribute} @listen 0", outlets=3, column=4)
        self.patcher.connect(query, 1, target)
        self.log_value(label, query)
        return query

    def cpu_reading(self, label: str) -> str:
        """An action logging Max's DSP CPU (adstatus cpu) as "<label> <percent>". adstatus also
        reports on its own when audio starts or restarts, so only the answer to this bang is kept."""
        trigger = self.patcher.box("t b b b", outlets=3, column=4)
        opened = self.patcher.message("1", column=4)
        closed = self.patcher.message("0", column=4)
        gate = self.patcher.box("gate", inlets=2, column=4)
        cpu = self.patcher.box("adstatus cpu", outlets=2, column=4)
        self.patcher.connect(trigger, 2, opened)
        self.patcher.connect(trigger, 1, cpu)
        self.patcher.connect(trigger, 0, closed)
        self.patcher.connect(opened, 0, gate)
        self.patcher.connect(closed, 0, gate)
        self.patcher.connect(cpu, 0, gate, 1)
        self.log_value(label, gate)
        return trigger

    def _errors(self) -> str:
        """The [error 1] listening to the Max console: every error posted while the patcher is open."""
        if not self.errors:
            self.errors = self.patcher.box("error 1", column=4)
        return self.errors

    def _log_errors(self) -> None:
        """Put the first LOGGED console errors into the test's log, for run.py to print when the
        test fails. The harness writes a log line into SQL between double quotes, and Max quotes a
        symbol that has a space in it, so the error is made plain first: into one symbol, its
        double quotes (Python source in a traceback has them) replaced, and back into atoms. Else
        the database error that causes is posted as an error, logged, and fails again; the cap
        stops any other such loop."""
        text = self.patcher.box("tosymbol", column=4)
        quotes = self.patcher.box("regexp \\\" @substitute '", outlets=5, column=4)
        atoms = self.patcher.box("fromsymbol", column=4)
        trigger = self.patcher.box("t l b", outlets=2, column=4)
        counter = self.patcher.box("counter 1 1000000", inlets=3, outlets=4, column=4)
        room = self.patcher.box(f"<= {LOGGED}", inlets=2, column=4)
        gate = self.patcher.box("gate 1 1", inlets=2, column=4)
        log = self.patcher.box("test.log console-error", outlets=0, column=4)
        self.patcher.connect(self._errors(), 0, text)
        self.patcher.connect(text, 0, quotes)
        self.patcher.connect(quotes, 0, atoms)  # substituted
        self.patcher.connect(quotes, 3, atoms)  # no quote to replace
        self.patcher.connect(atoms, 0, trigger)
        self.patcher.connect(trigger, 1, counter)
        self.patcher.connect(counter, 0, room)
        self.patcher.connect(room, 0, gate)
        self.patcher.connect(trigger, 0, gate, 1)
        self.patcher.connect(gate, 0, log)

    def _error_counter(self, pattern: str) -> str:
        """A [counter] of Max console errors matching the regular expression `pattern` ("" for
        every error), counted from when the patcher opened."""
        if pattern in self.error_counters:
            return self.error_counters[pattern]
        source, outlet = self._errors(), 0
        if pattern:
            source = self.patcher.box(f"regexp {pattern}", outlets=5, column=4)
            self.patcher.connect(self._errors(), 0, source)
            outlet = 2  # the matched substrings: output only on a match
        trigger = self.patcher.box("t b", column=4)
        counter = self.patcher.box("counter 1 1000000", inlets=3, outlets=4, column=4)
        self.patcher.connect(source, outlet, trigger)
        self.patcher.connect(trigger, 0, counter)
        self.error_counters[pattern] = counter
        return counter

    def errors_are(self, name: str, compare: str, pattern: str = "") -> str:
        """Check the number of console errors (matching `pattern`) so far: `compare` is a Max
        comparison and its operand, e.g. "== 0" or ">= 1"."""
        counter = self._error_counter(pattern)
        count = self.patcher.box("i", inlets=2, column=4)
        test = self.patcher.box(compare, inlets=2, column=4)
        self.patcher.connect(counter, 0, count, 1)
        self.patcher.connect(count, 0, test)
        self._assert(name, test)
        return count

    def count_errors(self, pattern: str) -> None:
        """Start counting console errors matching `pattern` when the patcher opens."""
        self._error_counter(pattern)

    # -- the script --------------------------------------------------------------------------

    def step(self, *actions, wait: int = SETTLE) -> None:
        """Wait, then bang `actions` (boxes, or lists of them) in order. A sample is taken on the
        next signal vector, which can be computed well after the scheduler moves on, so wait at
        least SETTLE between a check and whatever changes what it checks."""
        flat: list[str] = []
        for action in actions:
            flat.extend(action if isinstance(action, list) else [action])
        self.steps.append((wait, flat))

    def write(self) -> Path:
        terminate = self.patcher.box("test.terminate", outlets=0, column=0)
        self.step(terminate, wait=FINISH)
        self._log_errors()
        loadbang = self.patcher.box("loadbang", column=0)
        previous = loadbang
        if self.audio:
            # start audio, and the script once it is running (only the first time: a test may
            # restart it); if it never starts, audio-started fails and the watchdog ends the test
            trigger = self.patcher.box("t b b", outlets=2, column=0)
            start = self.patcher.message("; dsp start", column=0)
            watchdog = self.patcher.box(f"delay {self.watchdog}", inlets=2, column=0)
            state = self.patcher.box("dspstate~", outlets=4, column=0)
            running = self.patcher.box("sel 1", inlets=2, outlets=2, column=0)
            once = self.patcher.box("onebang 1", inlets=2, outlets=2, column=0)
            started = self.patcher.message("1", column=0)
            self.patcher.connect(loadbang, 0, trigger)
            self.patcher.connect(trigger, 1, watchdog)
            self.patcher.connect(trigger, 0, start)
            self.patcher.connect(watchdog, 0, terminate)
            self.patcher.connect(state, 0, running)
            self.patcher.connect(running, 0, once)
            self.patcher.connect(once, 0, started)
            self._assert("audio-started", started)
            previous = once
        for wait_ms, actions in self.steps:
            delay = self.patcher.box(f"delay {wait_ms}", inlets=2, column=0)
            trigger = self.patcher.box("t " + " ".join(["b"] * (len(actions) + 1)),
                                       outlets=len(actions) + 1, column=0)
            self.patcher.connect(previous, 0, delay)
            self.patcher.connect(delay, 0, trigger)
            # a trigger fires right to left: the actions in order, then the next step
            for index, action in enumerate(actions):
                self.patcher.connect(trigger, len(actions) - index, action)
            previous = trigger
        path = PATCHERS / self.filename
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(self.patcher.document(), indent="\t") + "\n")
        return path


# ---- the tests ----------------------------------------------------------------------------------


def load() -> Test:
    t = Test("tap.python~.load.maxtest.maxpat",
             "Loading: with no argument (python/default.py), the numpy examples, and a class file with "
             "an attribute set by its argument; the standard int and float messages. Closing the "
             "patcher unloads them all.")
    source = t.signal(1.0)
    default = t.python("")
    block = t.python("numpy_gain @gain 0.25")
    gain = t.python("maxtest_gain @gain 0.5")
    allpass = t.python("numpy_allpass")
    for py in (default, block, gain, allpass):
        t.patcher.connect(source, 0, py)
    t.step(t.sample_equals("default-loads-with-unity-gain", default, 1.0),
           t.sample_equals("numpy-block-path-applies-gain", block, 0.25),
           t.sample_equals("numpy-allpass-passes-dc-at-unity", allpass, 1.0),
           t.sample_equals("argument-sets-attribute", gain, 0.5),
           t.attribute_equals("attribute-reads-back", gain, "gain", 0.5))
    t.step(t.send("float 0.5", default))
    t.step(t.sample_equals("float-message-calls-method", default, 0.5))
    t.step(t.send("int 2", default))
    t.step(t.sample_equals("int-message-calls-method", default, 2.0))
    t.step(t.send("greet max", default))
    t.step(t.errors_are("console-clean", "== 0"))
    return t


def attributes_and_messages() -> Test:
    t = Test("tap.python~.attributes-and-messages.maxtest.maxpat",
             "Annotated fields as attributes, one of each hint kind (float, int, bool, str, "
             "Optional), set by message and read by getattr; methods as messages, called by "
             "signature; a call with the wrong arguments is reported and changes nothing.")
    source = t.signal(1.0)
    py = t.python("maxtest_types")
    t.patcher.connect(source, 0, py)
    t.step(t.sample_equals("defaults-apply", py, 1.0),
           t.attribute_equals("int-default", py, "count", 3),
           t.attribute_equals("bool-default-reads-1", py, "on", 1),
           t.attribute_equals("str-default", py, "label", "start"))
    t.step(t.send("gain 2.", py), t.send("count 7.9", py), t.send("label hello", py))
    t.step(t.sample_equals("float-attribute-set", py, 2.0),
           t.attribute_equals("float-attribute-reads-back", py, "gain", 2.0),
           t.attribute_equals("int-attribute-truncates-like-atom_getlong", py, "count", 7),
           t.attribute_equals("str-attribute-set", py, "label", "hello"))
    t.step(t.send("on 0", py))
    t.step(t.sample_equals("bool-attribute-off", py, 0.0),
           t.attribute_equals("bool-attribute-reads-0", py, "on", 0))
    t.step(t.send("on 1", py), t.send("add 0.25", py))
    t.step(t.sample_equals("float-method-called", py, 2.25))
    t.step(t.send("add 0.25", py))
    t.step(t.sample_equals("method-called-again", py, 2.5))
    t.step(t.send("reset", py), t.send("pair 4 3.", py), t.send("rename world", py))
    t.step(t.sample_equals("no-argument-and-two-argument-methods", py, 3.0),
           t.attribute_equals("method-sets-int-attribute", py, "count", 4),
           t.attribute_equals("symbol-method-called", py, "label", "world"))
    t.step(t.send("bang", py), t.send("limit 1.5", py))
    t.step(t.attribute_equals("bang-message-calls-method", py, "count", 5),
           t.sample_equals("optional-attribute-set", py, 1.5),
           t.errors_are("console-clean", "== 0"))
    t.step(t.send("pair 1", py))
    t.step(t.errors_are("wrong-arguments-reported", ">= 1"),
           t.attribute_equals("wrong-arguments-change-nothing", py, "count", 5),
           t.sample_equals("still-running-after-bad-call", py, 1.5))
    return t


def reload() -> Test:
    t = Test("tap.python~.reload.maxtest.maxpat",
             "Reloading: by the filechanged message, and by saving the file (maxtest_editor edits "
             "it, and the object's file watcher notices). Attribute values are kept; a field an "
             "attrui shows can be removed; a file that fails to load silences the object, and "
             "fixing it brings the audio — and the values — back.")
    source = t.signal(1.0)
    py = t.python("maxtest_reload")
    editor = t.python("maxtest_editor", column=1)
    t.patcher.connect(source, 0, py)
    shown = t.patcher.box("", maxclass="attrui", outlets=1, column=2, attr="extra", width=160)
    t.patcher.connect(shown, 0, py)
    t.count_errors("Failed.to.load")
    t.step(t.send("gain 0.5", py))
    t.step(t.sample_equals("gain-applies", py, 0.5))
    t.step(t.send("filechanged", py))
    t.step(t.sample_equals("filechanged-reload-keeps-value", py, 0.5),
           t.attribute_equals("filechanged-reload-keeps-attribute", py, "gain", 0.5))
    t.step(t.send("scale maxtest_reload 3.", editor))
    t.step(t.sample_equals("saving-the-file-reloads-it", py, 1.5),
           t.attribute_equals("saved-reload-keeps-attribute", py, "gain", 0.5), wait=WATCH)
    t.step(t.send("remove_field maxtest_reload extra", editor))
    t.step(t.send("gain 1.", py), wait=WATCH)
    t.step(t.sample_equals("works-after-removing-a-field-an-attrui-shows", py, 3.0),
           t.errors_are("console-clean", "== 0"))
    t.step(t.send("corrupt maxtest_reload", editor))
    t.step(t.sample_equals("failed-reload-silences", py, 0.0),
           t.errors_are("failed-reload-reported-once", "== 1", "Failed.to.load"), wait=WATCH)
    t.step(t.send("restore maxtest_reload", editor))
    t.step(t.sample_equals("fixing-the-file-restores-audio-and-values", py, 3.0), wait=WATCH)
    return t


def reload_under_audio() -> Test:
    t = Test("tap.python~.reload-under-audio.maxtest.maxpat",
             "Reloading every 50 ms for 3 s while audio runs, through the block path (numpy_gain) "
             "and the per-sample path (default): a reload that succeeds never interrupts the "
             "audio, so every sample must stay exactly 1. (Whether the audio device drops out "
             "while a reload holds the GIL is for a listening check: plan 2.6.)")
    source = t.signal(1.0)
    block = t.python("numpy_gain")
    sample = t.python("default")
    t.patcher.connect(source, 0, block)
    t.patcher.connect(source, 0, sample)
    watch_block = t.no_change("block-path-never-interrupted", block, 1.0)
    watch_sample = t.no_change("per-sample-path-never-interrupted", sample, 1.0)
    start, stop = t.metro(50, t.send("filechanged", block), t.send("filechanged", sample))
    t.step(watch_block[0], watch_sample[0], start)
    t.step(stop, wait=3000)
    t.step(watch_block[1], watch_sample[1],
           t.sample_equals("block-path-output-after", block, 1.0),
           t.sample_equals("per-sample-path-output-after", sample, 1.0),
           t.errors_are("console-clean", "== 0"))
    return t


def many_instances() -> Test:
    t = Test("tap.python~.many-instances.maxtest.maxpat",
             "Twenty instances of one class on each path, sharing the interpreter and the loaded "
             "file (their outputs sum into one inlet), then instances created and destroyed while "
             "audio runs: a poly~ switching between 20 voices and 1, ten times.")
    source = t.signal(1.0)
    total_sample = t.sample_equals("twenty-per-sample-instances-sum", None, 2.5)
    total_block = t.sample_equals("twenty-block-instances-sum", None, 2.5)
    for index in range(20):
        for name, total in (("maxtest_gain", total_sample), ("numpy_gain", total_block)):
            py = t.python(f"{name} @gain 0.125")
            t.patcher.connect(source, 0, py)
            t.patcher.connect(py, 0, total)
    host = t.host("maxtest_gain", voices=20)
    t.patcher.connect(source, 0, host)
    t.step(total_sample, total_block)
    for _ in range(10):
        t.step(t.send("voices 1", host), wait=200)
        t.step(t.send("voices 20", host), wait=200)
    t.step(t.sample_equals("instances-survive-churn", host, 20.0),
           t.errors_are("console-clean", "== 0"), wait=500)
    return t


def channels() -> Test:
    t = Test("tap.python~.channels.maxtest.maxpat",
             "Several inputs and outputs (plan 2.4): process()'s parameters are the object's signal inlets "
             "and its return hint its outlets — stereo_width's two of each, a generator's none (one inlet "
             "still, for messages). A save that changes how many changes the object's inlets and outlets "
             "in place, keeping the patch cords of those that stay: new ones take cords and carry signal, "
             "and the cords of those removed go with them.")
    left, right = t.signal(0.8), t.signal(0.2)
    width = t.obj("tap.python~ stereo_width", inlets=2, outlets=2, signal=True)
    t.patcher.connect(left, 0, width, 0)
    t.patcher.connect(right, 0, width, 1)
    generator = t.python("maxtest_generator")
    a, b = t.signal(0.3), t.signal(0.6)
    # named, so that thispatcher can connect cords to the inlet and outlet a save adds
    shape = t.patcher.box("tap.python~ maxtest_shape", 2, 2, column=2, outlettype=["signal"] * 2, varname="shape")
    t.patcher.connect(a, 0, shape, 0)
    t.patcher.connect(b, 0, shape, 1)
    t.patcher.box("sig~ 0.9", 1, 1, column=2, outlettype=["signal"], varname="third_input")
    third_output = t.patcher.box("+~ 0.", inlets=2, column=3, outlettype=["signal"], varname="third_output")
    scripting = t.obj("thispatcher")
    editor = t.python("maxtest_editor", column=1)

    def outlet(box: str, n: int) -> str:  # a box passing outlet n of `box` on, to sample
        through = t.patcher.box("+~ 0.", inlets=2, column=3, outlettype=["signal"])
        t.patcher.connect(box, n, through)
        return through

    width_left, width_right = outlet(width, 0), outlet(width, 1)
    shape_first, shape_second = outlet(shape, 0), outlet(shape, 1)
    t.step(t.sample_equals("stereo-left-at-width-1", width_left, 0.8),
           t.sample_equals("stereo-right-at-width-1", width_right, 0.2),
           t.sample_equals("generator-without-inputs", generator, 0.25),
           t.sample_equals("second-input-to-second-output", shape_second, 0.6))
    t.step(t.send("width 0", width), t.send("level 0.5", generator))
    t.step(t.sample_equals("mono-left-at-width-0", width_left, 0.5),
           t.sample_equals("mono-right-at-width-0", width_right, 0.5),
           t.sample_equals("generator-takes-messages", generator, 0.5))
    t.step(t.send("reshape maxtest_shape 3 3", editor), t.send("filechanged", shape))
    t.step(t.sample_equals("grown-keeps-first-cords", shape_first, 0.3),
           t.sample_equals("grown-keeps-second-cords", shape_second, 0.6))
    t.step(t.send("script connect third_input 0 shape 2", scripting),
           t.send("script connect shape 2 third_output 0", scripting))
    t.step(t.sample_equals("added-inlet-to-added-outlet", third_output, 0.9))
    t.step(t.send("reshape maxtest_shape 1 1", editor), t.send("filechanged", shape))
    t.step(t.sample_equals("shrunk-keeps-first-cords", shape_first, 0.3),
           t.sample_equals("removed-outlet-takes-its-cord", shape_second, 0.0),
           t.sample_equals("removed-outlets-take-their-cords", third_output, 0.0),
           t.errors_are("console-clean", "== 0"), wait=WATCH)
    return t


def worker(latency_ms: float | None = None, quiet_windows: int = 0) -> Test:
    t = Test("tap.python~.worker.maxtest.maxpat",
             "Worker mode (plan 2.5), at its default latency: with @mode worker the output is the input "
             "delayed by @latencysamples, sample for sample what delay~ gives, and stays so through a "
             "reload; a class that stalls for 0.2 s is late, reported once, and comes back at the same "
             "latency; @mode direct takes the delay away.")
    ramp = t.obj("phasor~ 50", signal=True)  # a new value every sample, so any delay shows
    py = t.python("maxtest_stall @mode worker" + (f" @latency {latency_ms}" if latency_ms is not None else ""))
    delay = t.obj("delay~ 48000 0", inlets=2, signal=True)
    late = t.obj("-~", inlets=2, signal=True)  # the object's output less the input, delayed
    now = t.obj("-~", inlets=2, signal=True)  # the object's output less the input
    for source, destination in ((ramp, py), (ramp, delay), (py, late), (py, now)):
        t.patcher.connect(source, 0, destination)
    t.patcher.connect(delay, 0, late, 1)
    t.patcher.connect(ramp, 0, now, 1)
    delay_by_latency = t.patcher.box("getattr latencysamples @listen 0", outlets=3, column=3)
    t.patcher.connect(delay_by_latency, 1, py)
    t.patcher.connect(delay_by_latency, 0, delay, 1)
    t.count_errors("late.for")

    watch = t.no_change("delayed-by-latencysamples", late, 0.0)
    after = t.no_change("same-latency-after-stall", late, 0.0)
    windows = [t.no_change(f"quiet-window-{i}", late, 0.0) for i in range(quiet_windows)]
    # each step waits, then acts (so a watch starts a step after what it depends on)
    t.step(t.attribute_compare("latency-reported-in-samples", py, "latencysamples", "> 0"), delay_by_latency,
           t.log_attribute("latencysamples", py, "latencysamples"))
    t.step(watch[0])
    t.step(watch[1], wait=1000)
    for start, check in windows:  # (for measuring: none in the committed test)
        t.step(start)
        t.step(check, wait=1000)
    t.step(t.send("stall 0.2", py))
    t.step(t.errors_are("stall-reported-once", "== 1", "late.for"), after[0], wait=1000)
    t.step(after[1], wait=1000)
    t.step(t.send("filechanged", py))
    t.step(t.sample_equals("delayed-after-reload", late, 0.0), wait=500)
    t.step(t.send("mode direct", py))
    t.step(t.attribute_equals("direct-reports-no-latency", py, "latencysamples", 0),
           t.sample_equals("direct-has-no-delay", now, 0.0),
           t.errors_are("console-only-the-stall", "== 1"), wait=500)
    return t


def announce_once() -> Test:
    t = Test("tap.python~.announce-once.maxtest.maxpat",
             "What is true of a class is said once per run of its file, however many objects share it "
             "(plan 6.7): five instances of a class with a reserved method name, reporting it once on "
             "load, once more after the file changes, and not at all when an unchanged file reloads; "
             "and a save that breaks the file is reported once, not by each (plan 6.10).",
             audio=False)
    host = t.host()  # loaded by the first step, so that what the objects print is caught
    editor = t.python("maxtest_editor", column=1)
    pattern = "is.reserved"
    t.count_errors(pattern)
    t.step(t.load(host, "maxtest_reserved", voices=5), t.send("target 0", host))
    t.step(t.errors_are("said-once-for-five-objects", "== 1", pattern))
    t.step(t.send("bump maxtest_reserved", editor), t.send("filechanged", host))
    t.step(t.errors_are("said-once-more-after-a-change", "== 2", pattern))
    t.step(t.send("filechanged", host))
    t.step(t.errors_are("unchanged-reload-says-nothing", "== 2", pattern), wait=WATCH)
    # 6.10: a save that breaks the file is reported once, not by each of the five
    t.count_errors("Failed.to.load")
    t.step(t.send("corrupt maxtest_reserved", editor))  # the file watcher reloads them all
    t.step(t.errors_are("broken-save-reported-once-for-five-objects", "== 1", "Failed.to.load"), wait=WATCH)
    t.step(t.send("restore maxtest_reserved", editor))
    return t


def faults() -> Test:
    t = Test("tap.python~.faults.maxtest.maxpat",
             "sys.exit() in a message is reported, not obeyed; NaN from process() is output as 0 "
             "and reported once; an exception in process() is reported once, from the main "
             "thread, and silences the object — ignoring messages — until the file is reloaded. Its "
             "class also has methods named dspstate, patchlineupdate, inputchanged and fileusage, "
             "which Max calls with C arguments (plan 8.2): they must not be exposed, so toggling DSP "
             "and connecting a cord with it loaded must pass (fileusage is Build Collective's: by hand).")
    source = t.signal(1.0)
    py = t.patcher.box("tap.python~ maxtest_faults", 1, 1, column=2, outlettype=["signal"], varname="faults")
    t.patcher.connect(source, 0, py)
    t.patcher.box("sig~ 1.", 1, 1, column=2, outlettype=["signal"], varname="second_source")
    scripting = t.obj("thispatcher")
    for pattern in ("SystemExit", "non-finite", "audio.disabled"):
        t.count_errors(pattern)
    t.step(t.sample_equals("passes-signal", py, 1.0))
    t.step(t.send("exit 3", py))
    t.step(t.errors_are("sys-exit-reported", "== 1", "SystemExit"),
           t.sample_equals("still-running-after-sys-exit", py, 1.0))
    t.step(t.send("nan", py))
    t.step(t.sample_equals("nan-output-as-zero", py, 0.0),
           t.errors_are("nan-reported-once", "== 1", "non-finite"), wait=1000)
    t.step(t.send("heal", py))
    t.step(t.sample_equals("recovers-from-nan-without-reload", py, 1.0))
    t.step(t.send("fail", py))
    t.step(t.sample_equals("exception-silences", py, 0.0),
           t.errors_are("exception-reported-once", "== 1", "audio.disabled"), wait=1000)
    t.step(t.send("heal", py))
    t.step(t.sample_equals("messages-ignored-until-reload", py, 0.0))
    t.step(t.send("filechanged", py))
    t.step(t.sample_equals("reload-restores-audio", py, 1.0))
    t.step(t.dsp(False))
    t.step(t.dsp(True), wait=100)
    t.step(t.sample_equals("survives-dsp-toggle-with-c-argument-names", py, 1.0), wait=1000)
    t.step(t.send("script connect second_source 0 faults 0", scripting))
    t.step(t.sample_equals("survives-a-new-cord-with-c-argument-names", py, 2.0))  # both sources summed
    return t


def prepare() -> Test:
    t = Test("tap.python~.prepare.maxtest.maxpat",
             "prepare(sample_rate, vector_size) runs before the first process(), and again when "
             "either changes: the object runs in a poly~, whose up and vs messages change what it "
             "sees without touching the audio device. maxtest_prepare outputs what it was told, "
             "selected by its input (1 rate, 2 vector size, 3 prepare calls, 4 early samples).")
    select = t.patcher.box("sig~ 0.", column=2, outlettype=["signal"])
    host = t.host("maxtest_prepare")
    t.patcher.connect(select, 0, host)
    state = t.patcher.box("dspstate~", outlets=4, column=2)
    doubled = t.patcher.box("* 2.", inlets=2, column=2)
    t.patcher.connect(state, 1, doubled)
    t.step(t.send("resampling 0", host))  # so that a constant passes the resampling unchanged
    t.step(t.send("1.", select), state)
    t.step(t.sample_equals("told-the-sample-rate", host, operand=(state, 1)))
    t.step(t.send("2.", select))
    t.step(t.sample_equals("told-the-vector-size", host, operand=(state, 2)))
    t.step(t.send("4.", select))
    t.step(t.sample_equals("no-process-before-prepare", host, 0.0))
    t.step(t.send("3.", select))
    t.step(t.sample_compare("prepared", host, ">= 1."))
    t.step(t.send("up 2", host), t.dsp(False))
    t.step(t.dsp(True), wait=100)
    t.step(t.send("1.", select), state, wait=1500)
    t.step(t.sample_equals("told-the-new-sample-rate", host, operand=(doubled, 0)))
    t.step(t.send("up 1", host), t.send("vs 16", host), t.dsp(False))
    t.step(t.dsp(True), wait=100)
    t.step(t.send("2.", select), wait=1500)
    t.step(t.sample_equals("told-the-new-vector-size", host, 16.0))
    t.step(t.send("3.", select))
    t.step(t.sample_compare("prepared-again-on-each-change", host, ">= 3."),
           t.errors_are("console-clean", "== 0"))
    return t


def soak(minutes: float) -> Test:
    """Plan 6.2, run by `run.py --session soak`: not a .maxtest, so the quick suite skips it."""
    duration = int(minutes * 60_000)
    third = duration // 3
    saves = duration // 1000
    t = Test("soak/tap.python~.soak.maxpat",
             f"The soak (plan 6.2): {minutes:g} minutes of audio through 24 per-sample instances of one class "
             "(in a poly~), one more at top level, and 8 block-path instances of another; every second "
             "maxtest_editor saves both files with a new revision and every instance is told to reload. A "
             "third of the way through the poly~ changes its sample rate (up 2), and back at two thirds. "
             "Every output sample must stay exact in each phase, the console clean, and the saves must have run "
             "the module again. Logged: Max's DSP CPU after 30 quiet seconds (no reloads, to compare with "
             "core/bench) and then each minute, with the module's executions and Python's object count.",
             watchdog=duration + 180_000)
    source = t.signal(1.0)
    host = t.host("maxtest_soak", voices=24)
    census = t.python("maxtest_soak")
    blocks = [t.python("maxtest_soak_block") for _ in range(8)]
    editor = t.python("maxtest_editor", column=1)
    for py in (host, census, *blocks):
        t.patcher.connect(source, 0, py)

    # each second: a real save of both files (so the module runs again), then every instance reloads —
    # the file watcher coalesces saves this close together, so it is not left to deliver them
    reload_all = t.patcher.message("filechanged", column=1)
    for py in (host, census, *blocks):
        t.patcher.connect(reload_all, 0, py)
    saves_start, saves_stop = t.metro(1000, t.send("bump maxtest_soak", editor),
                                      t.send("bump maxtest_soak_block", editor), reload_all)
    minute_start, minute_stop = t.metro(60_000, t.send("census", census),
                                        t.log_attribute("executions", census, "executions"),
                                        t.log_attribute("objects", census, "objects"), t.cpu_reading("cpu"))
    phases = [(t.no_change(f"phase-{n}-poly-per-sample-never-interrupted", host, 24.0),
               t.no_change(f"phase-{n}-block-path-never-interrupted", blocks, 8.0)) for n in (1, 2, 3)]

    def audio_flowed(n: int) -> list[str]:  # the watchers pass vacuously if no audio ran
        return [t.sample_equals(f"phase-{n}-poly-output", host, 24.0),
                t.sample_equals(f"phase-{n}-block-output", blocks[0], 1.0)]

    t.step(t.send("resampling 0", host), t.send("target 0", host))  # constants pass unchanged; to every voice
    t.step(t.cpu_reading("cpu-quiet"), wait=30_000)  # 33 objects running, nothing else going on
    t.step(phases[0][0][0], phases[0][1][0], saves_start, minute_start)
    t.step(audio_flowed(1), wait=third - 200)
    t.step(phases[0][0][1], phases[0][1][1], t.send("up 2", host), t.dsp(False))
    t.step(t.dsp(True), wait=100)
    t.step(phases[1][0][0], phases[1][1][0], wait=1500)
    t.step(audio_flowed(2), wait=third - 1800)
    t.step(phases[1][0][1], phases[1][1][1], t.send("up 1", host), t.dsp(False))
    t.step(t.dsp(True), wait=100)
    t.step(phases[2][0][0], phases[2][1][0], wait=1500)
    t.step(audio_flowed(3), wait=third - 1800)
    t.step(phases[2][0][1], phases[2][1][1], saves_stop, minute_stop)
    t.step(t.send("census", census), t.log_attribute("executions", census, "executions"),
           t.log_attribute("objects", census, "objects"),
           t.attribute_compare("saves-ran-the-module-again", census, "executions", f">= {saves * 9 // 10}"),
           t.errors_are("console-clean", "== 0"), wait=WATCH)
    return t


# (class, instances) measured by the perf patcher, at the audio device's own sample rate
PERF_LOADS = [("default", 26), ("numpy_gain", 26), ("allpass", 1), ("numpy_allpass", 26)]
PERF_READINGS = 10


def perf() -> Test:
    """Plan 6.3, run by `run.py --session perf`: Max's own DSP CPU meter (adstatus cpu) with no
    object, then with instances of the shipped examples in a poly~, at the audio device's sample
    rate and vector size (a poly~'s up or down would change its vector size too, so the rate is
    left to the device). Logged as "cpu <class> <instances> <percent>"; run.py averages the
    readings into logs/perf.json."""
    t = Test("perf/tap.python~.perf.maxpat",
             "Max's DSP CPU meter (plan 6.3): no object, then instances of default.py, numpy_gain.py, "
             "allpass.py and numpy_allpass.py in a poly~, at the audio device's sample rate. Each figure is "
             f"{PERF_READINGS} readings a second apart, after the load has run for 5 s.",
             watchdog=600_000)
    source = t.signal(0.5)
    host = t.host()
    t.patcher.connect(source, 0, host)
    state = t.patcher.box("dspstate~", outlets=4, column=4)
    t.log_value("rate", state, 1)
    t.log_value("vector", state, 2)
    t.log_value("io", state, 3)

    def readings(label: str) -> list[list[str]]:
        return [[t.cpu_reading(label)] for _ in range(PERF_READINGS)]

    t.step(state)
    for reading in readings("cpu none 0"):
        t.step(reading, wait=1000)
    for name, count in PERF_LOADS:
        t.step(t.load(host, name, voices=count))
        first, *rest = readings(f"cpu {name} {count}")
        t.step(first, wait=5000)
        for reading in rest:
            t.step(reading, wait=1000)
    t.step(t.errors_are("console-clean", "== 0"))
    return t


def without_runtime() -> Test:
    t = Test("without-runtime/tap.python~.without-runtime.maxpat",
             "Run by run.py with support/ moved aside before Max started (plan 4.2, 6.4): the object "
             "must load anyway and say that the runtime is missing — not fail to load.", audio=False)
    t.count_errors("No.Python.runtime.found")
    host = t.host()
    t.step(t.load(host, "maxtest_gain"))
    t.step(t.errors_are("says-the-runtime-is-missing", "== 1", "No.Python.runtime.found"))
    return t


def without_runtime_restart() -> Test:
    t = Test("without-runtime/tap.python~.without-runtime-restart.maxpat",
             "Run by run.py after putting support/ back while Max runs (plan 4.2, 6.4): on macOS the "
             "external was loaded without its runtime, and dyld never binds a weak import later, so a "
             "new object must say to restart Max — the documented limit, pinned here.", audio=False)
    t.count_errors("then.restart.Max")
    host = t.host()
    t.step(t.load(host, "maxtest_gain"))
    t.step(t.errors_are("says-to-restart-max", "== 1", "then.restart.Max"))
    return t


TESTS_TO_WRITE = [load, attributes_and_messages, reload, reload_under_audio, many_instances, channels, announce_once,
                  faults,
                  prepare, worker,
                  without_runtime, without_runtime_restart]


def write_host() -> Path:
    """The poly~ abstraction: [in~ 1] and [in 1] into a [tap.python~ #1], out through [out~ 1]."""
    p = Patcher(HOST, "Hosts one [tap.python~ #1] for poly~ (see Test.host in make_patchers.py).")
    signal_in = p.box("in~ 1", outlets=2, column=1, outlettype=["signal", ""])
    messages = p.box("in 1", inlets=0, column=1)
    python = p.box("tap.python~ #1", column=1, outlettype=["signal"])
    signal_out = p.box("out~ 1", outlets=0, column=1)
    p.connect(signal_in, 0, python)
    p.connect(messages, 0, python)
    p.connect(python, 0, signal_out)
    path = PATCHERS / f"{HOST}.maxpat"
    path.write_text(json.dumps(p.document(), indent="\t") + "\n")
    return path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--soak-minutes", type=float, default=SOAK_MINUTES,
                        help=f"how long the soak runs (default {SOAK_MINUTES}; commit only the default)")
    args = parser.parse_args()
    for existing in PATCHERS.rglob("*.maxpat"):
        existing.unlink()
    print(f"wrote {write_host().relative_to(TESTS.parent)}")
    for test in [make() for make in TESTS_TO_WRITE] + [soak(args.soak_minutes), perf()]:
        path = test.write()
        print(f"wrote {path.relative_to(TESTS.parent)}")


if __name__ == "__main__":
    main()
