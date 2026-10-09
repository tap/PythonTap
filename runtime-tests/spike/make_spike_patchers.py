#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
"""Generate the plan 9.0 spike's patchers and documentation fixtures (THROWAWAY).

    python3 runtime-tests/spike/make_spike_patchers.py    # rewrites patchers/ and q7/ here

The spike (docs/TAP-PYTHON-PLAN.md, 9.0) is a throwaway second class, tap.python, built into
tap.python~'s binary with -DTAP_PYTHON_SPIKE=ON (source/projects/tap.python_tilde/
tap.python_spike.cpp). Each patcher here asks Max one of 9.0's questions: it runs itself from a
loadbang and says what it saw in the Max console — the spike object's own lines start "spike:",
and every [print] is named for the question (q2-…, q6-…). Open one in Max to run it by hand (on
Windows, say), or let run_spike.py open them all on a Mac and collect the console. What reaches an
outlet is shown by a [tap.python.spike.recorder <label>], the spike's second class, which posts its
label, the selector and each atom with its type (Max's log drops a [print]'s name, and [print] shows
no types); it must come after a [tap.python] or [tap.python~] in the patcher, which loads the
binary that registers it. q7/ holds a
minimal vignette, tutorials and an Extras patcher, which run_spike.py copies into the package's
docs/ and extras/ for a run (copy them by hand on Windows). Commit what this writes with it; never
edit the output by hand.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

SPIKE = Path(__file__).resolve().parent
sys.path.insert(0, str(SPIKE.parent))
from make_patchers import Patcher  # noqa: E402  (runtime-tests/make_patchers.py)

PATCHERS = SPIKE / "patchers"
Q7 = SPIKE / "q7"


class Script:
    """A patcher whose steps run from a loadbang: each step is a [delay] into message boxes."""

    def __init__(self, name: str, description: str):
        self.name = name
        self.patcher = Patcher(name, description)
        # the description across the top, sized to its text; every column starts below it
        lines = len(description) // 80 + 3
        header = self.patcher.boxes[0]["box"]
        header["patching_rect"][3] = 16.0 * lines
        header["linecount"] = lines
        self.patcher.next_y = {column: 40.0 + 16.0 * lines for column in range(4)}
        self.loadbang = self.patcher.box("loadbang", inlets=1, outlets=1, column=0, outlettype=["bang"])
        self.print_spike = self.patcher.box("print spike", inlets=1, outlets=0, column=0)

    def at(self, ms: int, target: str, *messages: str, inlet: int = 0) -> None:
        """At `ms` after loading, send each message (a message box each, in order) to `target` —
        or to nobody, for a message box that sends with a semicolon (target "")."""
        trigger = self.patcher.box(f"delay {ms}", inlets=2, outlets=1, column=0, outlettype=["bang"])
        self.patcher.connect(self.loadbang, 0, trigger)
        if len(messages) > 1:
            outlets = len(messages)
            t = self.patcher.box("t " + " ".join(["b"] * outlets), outlets=outlets, column=0,
                                 outlettype=["bang"] * outlets)
            self.patcher.connect(trigger, 0, t)
            for n, text in enumerate(messages):
                box = self.patcher.message(text, column=0)
                self.patcher.connect(t, outlets - 1 - n, box)  # right to left: the first message first
                if target:
                    self.patcher.connect(box, 0, target, inlet)
        else:
            box = self.patcher.message(messages[0], column=0)
            self.patcher.connect(trigger, 0, box)
            if target:
                self.patcher.connect(box, 0, target, inlet)

    def say(self, ms: int, text: str) -> None:
        self.at(ms, self.print_spike, text)

    def spike(self, arguments: str = "", varname: str = "spike", value_outlets: int = 2, column: int = 1) -> str:
        return self.patcher.box(f"tap.python {arguments}".strip(), inlets=1, outlets=value_outlets + 1,
                                column=column, varname=varname)

    def tilde(self, varname: str = "tilde", column: int = 1) -> str:
        return self.patcher.box("tap.python~", inlets=1, outlets=1, column=column, outlettype=["signal"],
                                varname=varname)

    def recorder(self, label: str, column: int = 2, varname: str = "") -> str:
        """A [tap.python.spike.recorder label]: posts every message it receives, with the label."""
        extra = {"varname": varname} if varname else {}
        return self.patcher.box(f"tap.python.spike.recorder {label}", inlets=1, outlets=0, column=column, **extra)

    def write(self) -> Path:
        PATCHERS.mkdir(exist_ok=True)
        path = PATCHERS / f"{self.name}.maxpat"
        path.write_text(json.dumps(self.patcher.document(), indent="\t") + "\n")
        return path


BEGIN = 100  # ms after loading, the first step
END = "end"


def q1_mapping() -> Script:
    s = Script("tap.python.spike-q1-mapping",
               "Q1 (D11): a patcher with only [tap.python], opened in a FRESH Max (quit Max first). With the "
               "package's init/tap.python.txt (max objectfile tap.python tap.python~) the console says "
               "'spike: tap.python registered by tap.python~'s ext_main' and then 'spike: new tap.python'; "
               "without it, 'tap.python: No such object'.")
    s.spike(value_outlets=2)
    return s


def q1_order(spike_first: bool) -> Script:
    order = "spike-first" if spike_first else "tilde-first"
    s = Script(f"tap.python.spike-q1-{order}",
               f"Q1 (D11): both objects in one patch, [{'tap.python' if spike_first else 'tap.python~'}] "
               "created first (it is first in the file), opened in a FRESH Max. Both must load, and both work: "
               "the spike's 'fire' reaches q1-spike, and tap.python~ (default.py) answers 'greet q1' with "
               "'python: hello q1, from python!'.")
    if spike_first:
        spike = s.spike()
        tilde = s.tilde()
    else:
        tilde = s.tilde()
        spike = s.spike()
    out = s.recorder("q1-spike")
    s.patcher.connect(spike, 0, out)
    s.patcher.connect(spike, 1, out)
    s.say(BEGIN, f"q1 {order} begin")
    s.at(BEGIN + 100, spike, "fire")
    s.at(BEGIN + 200, tilde, "greet q1")
    s.say(BEGIN + 400, f"q1 {order} {END}")
    return s


def q2_outlets() -> Script:
    s = Script("tap.python.spike-q2-outlets",
               "Q2 (M1): outlet_insert_after between the box's dynlet_begin and dynlet_end puts a new outlet "
               "before the dumpout, and the cords of the outlets that stay survive; outlet_delete removes the "
               "right one. 'describe' lists outlet_nth against what the object holds and every cord from the "
               "box; 'fire' outputs each value outlet's number (q2-outlet-N) and 'fired' from the dumpout "
               "(q2-dumpout). After 'outlets 3' the new outlet 2 is cord-ed to q2-new by scripting.")
    spike = s.spike("2", value_outlets=2)
    out0 = s.recorder("q2-outlet-0")
    out1 = s.recorder("q2-outlet-1")
    dump = s.recorder("q2-dumpout")
    s.recorder("q2-new", varname="newprint")
    s.patcher.connect(spike, 0, out0)
    s.patcher.connect(spike, 1, out1)
    s.patcher.connect(spike, 2, dump)
    thispatcher = s.patcher.box("thispatcher", inlets=1, outlets=2, column=2)
    t = BEGIN
    s.say(t, "q2 begin: two value outlets and the dumpout")
    s.at(t + 50, spike, "describe", "fire")
    s.say(t + 150, "q2 widen: outlets 3")
    s.at(t + 200, spike, "outlets 3", "describe")
    s.at(t + 300, thispatcher, "script connect spike 2 newprint 0")
    s.at(t + 400, spike, "describe", "fire")
    s.say(t + 500, "q2 narrow: outlets 1")
    s.at(t + 550, spike, "outlets 1", "describe", "fire")
    s.say(t + 650, "q2 widen again: outlets 2")
    s.at(t + 700, spike, "outlets 2", "describe", "fire")
    s.say(t + 900, f"q2 {END}")
    return s


def q3_dumpout() -> Script:
    s = Script("tap.python.spike-q3-dumpout",
               "Q3 (M1): get<attr> through the obex-stored dumpout. 'getsteps' should output 'steps 5' and, "
               "after 'steps 8', 'steps 8' to q3-dumpout — unless the class's anything takes it (the console "
               "would say 'spike: class anything received'). Compared with the class attribute 'level' "
               "(getlevel), with object_attr_getdump() called directly (getdump), and with what "
               "object_attr_method() resolves each message to (attrmethod) — on [tap.python], whose class "
               "registers object_obex_dumpout() as its dumpout method as the SDK's examples do, and on "
               "[tap.python.spike.nodumpout], the same class without it.")
    spike = s.spike("1 @steps 5", value_outlets=1)
    nodumpout = s.patcher.box("tap.python.spike.nodumpout 1 @steps 5", inlets=1, outlets=2, column=1,
                              varname="nodumpout")
    for box, label in ((spike, "q3-dumpout"), (nodumpout, "q3-nodumpout-dumpout")):
        s.patcher.connect(box, 0, s.recorder(label.replace("dumpout", "outlet-0")))
        s.patcher.connect(box, 1, s.recorder(label))
    t = BEGIN
    for box, name in ((spike, "tap.python"), (nodumpout, "tap.python.spike.nodumpout")):
        s.say(t, f"q3 {name}: the instance attribute steps")
        s.at(t + 50, box, "getsteps", "steps 8", "getsteps")
        s.say(t + 100, f"q3 {name}: the class attribute level")
        s.at(t + 150, box, "getlevel", "level 3", "getlevel")
        s.say(t + 200, f"q3 {name}: object_attr_getdump and object_attr_method")
        s.at(t + 250, box, "getdump steps", "getdump level", "attrmethod getsteps", "attrmethod steps",
             "attrmethod getlevel", "attrmethod level", "probe getsteps", "probe dumpout")
        t += 400
    s.say(t, f"q3 {END}")
    return s


def q4_dispatch() -> Script:
    s = Script("tap.python.spike-q4-dispatch",
               "Q4 (M3): with a class-level anything, does 'steps 8' set the instance attribute or reach "
               "anything; does an instance method (object_addmethod: hello, int) win over anything; what reaches "
               "anything (float, bang, list, symbol, an unknown selector)? Then 'probe' asks object_getmethod() "
               "for names on the spike (which has anything) and on tap.python~ (which has none), against what each "
               "answers for a name no class has. 'who' and 'bang' are both class and instance methods: which answers? "
               "The bare spike has no instance attribute or methods: what reaches its anything?")
    spike = s.spike("1", value_outlets=1)
    bare = s.spike("1 bare", varname="bare", value_outlets=1)
    s.tilde()
    dump = s.recorder("q4-dumpout")
    out0 = s.recorder("q4-outlet-0")
    s.patcher.connect(spike, 0, out0)
    s.patcher.connect(spike, 1, dump)
    s.patcher.connect(bare, 1, dump)
    t = BEGIN
    dispatch = ["steps 8", "getsteps", "hello 1 2", "who", "5", "1.5", "bang", "1 2 3", "list 4 5", "symbol foo",
                "foo 1 2"]
    s.say(t, "q4 begin: dispatch on the spike")
    s.at(t + 50, spike, *dispatch)
    s.say(t + 100, "q4 dispatch on the bare spike")
    s.at(t + 150, bare, *dispatch)
    s.say(t + 200, "q4 probes on the spike (it has a class-level anything)")
    s.at(t + 250, spike, "probe foo", "probe anything", "probe steps", "probe getsteps", "probe level",
         "probe getlevel", "probe hello", "probe who", "probe int", "probe float", "probe bang", "probe list",
         "probe symbol", "probe outlets")
    s.say(t + 400, "q4 probes on tap.python~ (no anything)")
    s.at(t + 450, spike, "probe foo tilde", "probe anything tilde", "probe gain tilde", "probe greet tilde",
         "probe int tilde", "probe dsp64 tilde", "probe bang tilde")
    s.say(t + 600, f"q4 {END}")
    return s


def q5_threads() -> Script:
    s = Script("tap.python.spike-q5-threads",
               "Q5 (M4): which thread a metro-driven message runs on with Overdrive off, Overdrive on, and "
               "Overdrive plus Scheduler in Audio Interrupt on — with audio on, then off. Each 'spike: thread' "
               "line gives systhread_ismainthread, isaudiothread and istimerthread on the thread the message "
               "came on; q5-dsp-running says whether audio really ran. Starts audio for a few seconds (silence). "
               "Puts Overdrive and Audio Interrupt back as they "
               "were (q5-was-…) at the end.")
    spike = s.spike("1", value_outlets=1)
    p = s.patcher

    def adstatus(setting: str) -> tuple[str, str, str]:
        status = p.box(f"adstatus {setting}", inlets=2, outlets=2, column=2, outlettype=["", "int"])
        was = p.box("i", inlets=2, outlets=1, column=2, outlettype=["int"])
        shown = s.recorder(f"q5-{setting}")
        p.connect(status, 1, shown)
        return status, was, shown

    overdrive, overdrive_was, _ = adstatus("overdrive")
    takeover, takeover_was, _ = adstatus("takeover")
    # the first report (just after loading) is the state to put back: open gates keep it in the [i]s,
    # and close after it
    gate_overdrive = p.box("gate 1 1", inlets=2, outlets=1, column=2)
    gate_takeover = p.box("gate 1 1", inlets=2, outlets=1, column=2)
    p.connect(overdrive, 1, gate_overdrive, 1)
    p.connect(gate_overdrive, 0, overdrive_was, 1)
    p.connect(takeover, 1, gate_takeover, 1)
    p.connect(gate_takeover, 0, takeover_was, 1)
    was_overdrive = s.recorder("q5-was-overdrive")
    was_takeover = s.recorder("q5-was-takeover")
    p.connect(overdrive_was, 0, was_overdrive)
    p.connect(takeover_was, 0, was_takeover)
    p.connect(overdrive_was, 0, overdrive)  # restoring: the old value into adstatus sets it
    p.connect(takeover_was, 0, takeover)

    def report(ms: int) -> None:
        s.at(ms, overdrive, "bang")
        s.at(ms + 10, takeover, "bang")

    def metro_phase(start: int, label: str, ms: int = 600) -> None:
        metro = p.box("metro 100", inlets=2, outlets=1, column=1, outlettype=["bang"])
        message = p.message(f"thread {label}", column=1)
        p.connect(metro, 0, message)
        p.connect(message, 0, spike)
        s.say(start - 50, f"q5 phase {label}")
        s.at(start, metro, "1")
        s.at(start + ms, metro, "0")

    t = BEGIN
    s.at(0, spike, "thread loadbang")
    s.at(t, overdrive, "bang")
    s.at(t + 10, takeover, "bang")
    s.at(t + 20, gate_overdrive, "0")
    s.at(t + 20, gate_takeover, "0")
    s.at(t + 100, "", "; max preempt 0; dsp takeover 0")
    report(t + 200)
    metro_phase(t + 300, "overdrive-off")
    s.at(t + 1000, "", "; max preempt 1")
    report(t + 1100)
    metro_phase(t + 1250, "overdrive-on")
    # whether audio really runs: the audio-running phase means nothing without it (dspstate~ says 1
    # when the audio starts, 0 when it stops), and an audio driver can take a while to start
    dsp = p.box("dspstate~", inlets=1, outlets=4, column=2, outlettype=["int", "float", "int", "int"])
    p.connect(dsp, 0, s.recorder("q5-dsp-running"))
    s.at(t + 1950, "", "; dsp takeover 1; dsp start")
    report(t + 2050)
    metro_phase(t + 4200, "overdrive-on-audio-interrupt-on-audio-running")
    s.at(t + 4900, "", "; dsp stop")
    metro_phase(t + 5100, "overdrive-on-audio-interrupt-on-audio-stopped")
    s.say(t + 5800, "q5 putting Overdrive and Audio Interrupt back")
    s.at(t + 5850, overdrive_was, "bang")
    s.at(t + 5860, takeover_was, "bang")
    report(t + 6000)
    s.say(t + 6200, f"q5 {END}")
    return s


def q6_strings() -> Script:
    texts = ["C", '"hello world"', "60", "1.5", "list", "int", "float", "symbol", "bang"]
    s = Script("tap.python.spike-q6-strings",
               "Q6 (m2): a str result output as the message it names (outmsg) and as 'symbol <s>' (outsym), "
               "into route, sel, prepend and a message box's $1. Each case is announced by a 'spike: q6 ----' "
               "line; the q6-… prints after it are what each downstream object did with it. The empty string "
               "comes last.")
    spike = s.spike("1", value_outlets=1)
    p = s.patcher
    raw = s.recorder("q6-raw")
    route = p.box("route C 60", inlets=2, outlets=3, column=2)
    select = p.box("sel C 60", inlets=2, outlets=3, column=2, outlettype=["bang", "bang", ""])
    prepend = p.box("prepend got", inlets=1, outlets=1, column=2)
    dollar = p.message("$1", column=2)
    p.connect(spike, 0, raw)
    p.connect(spike, 0, route)
    p.connect(spike, 0, select)
    p.connect(spike, 0, prepend)
    p.connect(spike, 0, dollar)
    for n, name in enumerate(["C", "60", "reject"]):
        p.connect(route, n, s.recorder(f"q6-route-{name}", column=3))
    for n, name in enumerate(["C", "60", "reject"]):
        p.connect(select, n, s.recorder(f"q6-sel-{name}", column=3))
    p.connect(prepend, 0, s.recorder("q6-prepend", column=3))
    p.connect(dollar, 0, s.recorder("q6-msgbox", column=3))
    s.say(BEGIN, "q6 begin")
    t = BEGIN + 50
    for kind in ("outmsg", "outsym"):
        s.at(t, spike, *[f"{kind} {text}" for text in texts], kind)  # the last: no argument, the empty string
        t += 150
    s.say(t + 100, f"q6 {END}")
    return s


# ---- q7: Max's documentation system --------------------------------------------------------------

VIGNETTE = """<?xml version="1.0" encoding="UTF-8"?>
<?xml-stylesheet type="text/xsl" href="_c74_vig.xsl"?>
<vignette name="PythonTap spike vignette ({where})" package="PythonTap">
<h1>PythonTap spike vignette ({where})</h1>
<p>A placeholder from the tap.python plan's 9.0 spike (runtime-tests/spike/ in the PythonTap repository),
placed at <b>{where}</b> inside the package. If this page is listed in the Documentation window, Max reads
vignettes from there.</p>
</vignette>
"""

TUTORIAL = """<?xml version="1.0" encoding="UTF-8"?>
<?xml-stylesheet href="./_c74_tut.xsl" type="text/xsl"?>
<chapter name="PythonTap spike tutorial ({where})">
<openfile name="PythonTap spike tutorial" patch="pythontap-spike-tutorial.maxpat"/>
<header>PythonTap spike tutorial ({where})</header>
<body>A placeholder from the tap.python plan's 9.0 spike (runtime-tests/spike/ in the PythonTap repository),
placed at {where} inside the package. If this tutorial is listed in the Documentation window, Max reads
tutorials from there; its Open Tutorial button opens pythontap-spike-tutorial.maxpat.</body>
</chapter>
"""

# (the file's path inside the package, what to call the place)
Q7_VIGNETTES = [
    ("docs/pythontap-spike-vig-docs.maxvig.xml", "docs/"),
    ("docs/vignettes/pythontap-spike-vig-vignettes.maxvig.xml", "docs/vignettes/"),
    ("docs/topics/pythontap-spike-vig-topics.maxvig.xml", "docs/topics/"),
]
Q7_TUTORIALS = [
    ("docs/pythontap-spike-tut-docs.maxtut.xml", "docs/"),
    ("docs/tutorials/pythontap-spike-tut-tutorials.maxtut.xml", "docs/tutorials/"),
    ("docs/tutorials/pythontap-tut/pythontap-spike-tut-nested.maxtut.xml", "docs/tutorials/pythontap-tut/"),
]
Q7_TUTORIAL_PATCHER = "docs/tutorials/pythontap-tut/pythontap-spike-tutorial.maxpat"
Q7_EXTRAS = "extras/PythonTap Spike Extras.maxpat"


def write_q7() -> list[Path]:
    written = []
    for relative, where in Q7_VIGNETTES:
        path = Q7 / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(VIGNETTE.format(where=where))
        written.append(path)
    for relative, where in Q7_TUTORIALS:
        path = Q7 / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(TUTORIAL.format(where=where))
        written.append(path)
    for relative, title, text in (
            (Q7_TUTORIAL_PATCHER, "pythontap-spike-tutorial",
             "The spike tutorial's patcher (plan 9.0): opened by the tutorial's Open Tutorial button."),
            (Q7_EXTRAS, "PythonTap Spike Extras",
             "Q7 (plan 9.0): a patcher in the package's extras/ folder. If it is in Max's Extras menu, Max "
             "lists a package's extras/ there.")):
        path = Q7 / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(Patcher(title, text).document(), indent="\t") + "\n")
        written.append(path)
    return written


SCRIPTS = [q1_mapping, lambda: q1_order(True), lambda: q1_order(False), q2_outlets, q3_dumpout, q4_dispatch,
           q5_threads, q6_strings]


def main() -> None:
    for existing in PATCHERS.glob("*.maxpat"):
        existing.unlink()
    for make in SCRIPTS:
        print(f"wrote {make().write().relative_to(SPIKE.parent.parent)}")
    for path in write_q7():
        print(f"wrote {path.relative_to(SPIKE.parent.parent)}")


if __name__ == "__main__":
    main()
