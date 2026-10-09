#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
"""Run the plan 9.0 spike in Max on a Mac and collect what the console said (THROWAWAY).

    cmake -S . -B build-spike -DTAP_PYTHON_SPIKE=ON && cmake --build build-spike
    python3 runtime-tests/spike/run_spike.py            # every question (~5 minutes)
    python3 runtime-tests/spike/run_spike.py --only q2   # sessions whose names contain q2

Each session starts a fresh Max (quit Max first), opens its patchers from patchers/ as the Finder
would (`open -a`: an "open document" event to the running Max), gives each a few seconds, and quits
(`max clean` first, so modified patchers ask nothing). Max is started, told to quit and waited for
over OSC through the max-test harness, as runtime-tests/run.py does (its helpers are used here). For
the run, Max's "Restore Windows on Launch" preference is off — otherwise a window left open at the
last quit (a help patcher with a tap.python~ in it) loads the external before the patcher under
test, and no Max is fresh — and the crash-recovery workspaces Max would reopen are moved aside to
runtime-tests/logs/spike-crash-recovery/. What the console said comes from Max's own log
(~/Library/Application Support/Cycling '74/Max 9/Logs/Max.log): the lines of interest are written
to results/macos.txt, the whole log to runtime-tests/logs/spike-<session>.maxlog.

The sessions: q1-control (init/tap.python.txt moved aside: [tap.python] alone must fail), q1-mapping
(with it: must load), q1-spike-first and q1-tilde-first (both objects, either order), q2-q6 (one
Max: outlets, the dumpout, dispatch, strings, then threads — which turns on Overdrive and Audio
Interrupt and starts audio for a second, and puts them back), and q7 (q7/'s vignettes, tutorials
and Extras patcher copied into the package's docs/ and extras/; after Max's file database reports
ready, the database is asked what it made of them). Everything moved or copied is put back, and
Max's preference files are restored to what they were before the run.

Never `max openfile` through the harness: its OSC dispatch calls every method as (symbol, argc,
argv), and Max's openfile takes two symbols — Max crashes (reading argc, 2, as a symbol).
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import sqlite3
import subprocess
import sys
import time
from pathlib import Path

SPIKE = Path(__file__).resolve().parent
sys.path.insert(0, str(SPIKE.parent))
import run as harness  # noqa: E402  (runtime-tests/run.py)

ROOT = harness.ROOT
PATCHERS = SPIKE / "patchers"
Q7 = SPIKE / "q7"
RESULTS = SPIKE / "results"
LOGS = harness.LOGS
INIT = ROOT / "init" / "tap.python.txt"
INIT_ASIDE = ROOT / "init" / "tap.python.txt.spike-aside"
MAX9 = Path.home() / "Library" / "Application Support" / "Cycling '74" / "Max 9"
MAX_LOG = MAX9 / "Logs" / "Max.log"
SETTINGS = MAX9 / "Settings"
CRASH_RECOVERY = MAX9 / "Crash Recovery"
PREFERENCES = ["maxpreferences.maxpref", "audioprefs.txt", "Core Audio@.txt", "recentitems.txt"]
SPIKE_MARK = "plan 9.0 spike build"  # in the external only when built with TAP_PYTHON_SPIKE

# What to keep from Max's log: the spike's lines, the questions' prints, and what the objects said.
INTERESTING = re.compile(r"spike|recorder|\bq[1-7]\b|tap\.python|No such object|python:|Loaded |"
                         r"doesn't understand|missing arguments|obex|error\]|crash", re.IGNORECASE)
NOISE = re.compile(r"RNBO|rnbo|j\.loader|dictionary_read|bach")
TRACE = re.compile(r"\] (->|<-) ")


def patcher(name: str) -> str:
    return f"tap.python.spike-{name}.maxpat"


# (session, its patchers, seconds to give each)
SESSIONS = [
    ("q1-control", ["q1-mapping"], 4),
    ("q1-mapping", ["q1-mapping"], 4),
    ("q1-spike-first", ["q1-spike-first"], 4),
    ("q1-tilde-first", ["q1-tilde-first"], 4),
    ("q2-q6", ["q2-outlets", "q3-dumpout", "q4-dispatch", "q6-strings", "q5-threads"], 6),
    ("q7", [], 0),
]


class Spike:
    def __init__(self, max_app: Path, packages: Path, verbose: bool):
        self.max_app = max_app
        self.packages = packages
        self.verbose = verbose
        self.copied: list[Path] = []
        self.made_folders: list[Path] = []
        self.report: list[str] = []

    # -- setting up and putting back ---------------------------------------------------------------

    def check(self) -> None:
        harness.check_prerequisites(self.max_app, self.packages)
        binary = harness.EXTERNAL / "Contents" / "MacOS" / "tap.python~"
        if SPIKE_MARK.encode() not in binary.read_bytes():
            raise harness.RunError("the external has no spike in it — cmake -S . -B build-spike "
                                   "-DTAP_PYTHON_SPIKE=ON && cmake --build build-spike")
        if INIT_ASIDE.exists():  # a run killed during q1-control
            INIT_ASIDE.rename(INIT)
        if not INIT.exists():
            raise harness.RunError(f"no {INIT.relative_to(ROOT)}")

    def back_up_preferences(self) -> Path:
        """Copy Max's preference files aside (restore_preferences() puts them back), then turn off
        Restore Windows on Launch for the run."""
        backup = LOGS / "spike-preferences-before"
        shutil.rmtree(backup, ignore_errors=True)
        backup.mkdir(parents=True)
        for name in PREFERENCES:
            if (SETTINGS / name).exists():
                shutil.copy2(SETTINGS / name, backup / name)
        preferences = SETTINGS / "maxpreferences.maxpref"
        if preferences.exists():
            document = json.loads(preferences.read_text())
            if document.get("preferences", {}).get("restorewindows"):
                document["preferences"]["restorewindows"] = 0
                preferences.write_text(json.dumps(document, indent=4) + "\n")
        return backup

    def clear_crash_recovery(self) -> None:
        """Move aside the workspaces Max would reopen at launch (kept in runtime-tests/logs/)."""
        aside = LOGS / "spike-crash-recovery"
        for workspace in CRASH_RECOVERY.glob("maxworkspace-*.txt"):
            aside.mkdir(parents=True, exist_ok=True)
            shutil.move(str(workspace), str(aside / workspace.name))

    def restore_preferences(self, backup: Path) -> None:
        for name in PREFERENCES:
            if (backup / name).exists():
                shutil.copy2(backup / name, SETTINGS / name)

    def install_q7(self) -> None:
        for source in sorted(p for p in Q7.rglob("*") if p.is_file()):
            destination = ROOT / source.relative_to(Q7)
            if destination.exists():
                raise harness.RunError(f"{destination} already exists")
            for folder in reversed(destination.relative_to(ROOT).parents[:-1]):
                if not (ROOT / folder).exists():
                    (ROOT / folder).mkdir()
                    self.made_folders.append(ROOT / folder)
            shutil.copyfile(source, destination)
            self.copied.append(destination)

    def remove_q7(self) -> None:
        for path in self.copied:
            path.unlink(missing_ok=True)
        for folder in reversed(self.made_folders):
            if folder.exists() and not any(folder.iterdir()):
                folder.rmdir()
        self.copied.clear()
        self.made_folders.clear()

    # -- one session ------------------------------------------------------------------------------

    def drain(self, session: harness.MaxSession, seconds: float) -> None:
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            session.receive(min(0.5, max(0.01, deadline - time.monotonic())))

    def session(self, name: str, patchers: list[str], seconds: float) -> None:
        print(f"\n== {name}")
        if name == "q1-control":
            INIT.rename(INIT_ASIDE)
        if name == "q7":
            self.install_q7()
        self.clear_crash_recovery()
        try:
            session = harness.MaxSession(self.max_app, f"spike-{name}", self.verbose)
            try:
                session.start()
                for which in patchers:
                    print(f"  opening {patcher(which)}")
                    subprocess.run(["open", "-a", str(self.max_app), str(PATCHERS / patcher(which))], check=True)
                    self.drain(session, seconds)
                if name == "q7":
                    print("  waiting for Max's file database")
                    session.wait_for("/db/ready", 900, poll="/db/ready?", every=5)
                    self.drain(session, 2.0)
                session.send("max clean")
                self.drain(session, 0.5)
            finally:
                session.quit()
        finally:
            if INIT_ASIDE.exists():
                INIT_ASIDE.rename(INIT)
        log = LOGS / f"spike-{name}.maxlog"
        shutil.copyfile(MAX_LOG, log)
        lines = [line.rstrip() for line in log.read_text(errors="replace").splitlines()
                 if INTERESTING.search(line) and not TRACE.search(line) and not NOISE.search(line)]
        # timestamps and thread ids stay: the thread is part of what q5 asks
        self.report.append(f"## session {name}\n")
        self.report.extend(lines)
        if name == "q7":
            self.report.extend(self.q7_database())
            self.remove_q7()
        self.report.append("")
        for line in lines:
            print(f"    {line}")

    def q7_database(self) -> list[str]:
        """What Max's file database made of q7's files: each one's kind and folder, or that it is
        not there. (Max's own record; what the Documentation window and the Extras menu show is
        still to be looked at.)"""
        databases = sorted((MAX9 / "Database").glob("*.maxdb"), key=lambda p: p.stat().st_size)
        if not databases:
            return ["(no file database found)"]
        copy = LOGS / "spike-q7.maxdb"
        shutil.copyfile(databases[-1], copy)
        lines = ["", f"Max's file database ({databases[-1].name}), for the files q7 put in the package:"]
        with sqlite3.connect(copy) as db:
            for path in self.copied:
                stem = path.name
                for suffix in (".maxvig.xml", ".maxtut.xml", ".maxpat"):
                    stem = stem.removesuffix(suffix)
                rows = db.execute("SELECT _name, _kind, _typedesc, _path FROM _things WHERE _name = ? "
                                  "OR _filename = ?", (stem, path.name)).fetchall()
                where = path.relative_to(ROOT)
                if rows:
                    for row in rows:
                        lines.append(f"  {where}: name '{row[0]}', kind {row[1]} ({row[2]}), folder {row[3]}")
                else:
                    lines.append(f"  {where}: NOT in the database")
        return lines

    def run(self, only: list[str]) -> int:
        self.check()
        harness.build_oscar()
        harness.install_harness(self.packages)
        backup = self.back_up_preferences()
        try:
            for name, patchers, seconds in SESSIONS:
                if only and not any(o in name for o in only):
                    continue
                self.session(name, patchers, seconds)
        finally:
            self.remove_q7()
            if INIT_ASIDE.exists():
                INIT_ASIDE.rename(INIT)
            self.restore_preferences(backup)
        version = subprocess.run(["defaults", "read", str(self.max_app / "Contents" / "Info.plist"),
                                  "CFBundleShortVersionString"], capture_output=True, text=True).stdout.strip()
        RESULTS.mkdir(exist_ok=True)
        output = RESULTS / "macos.txt"
        header = [f"# The plan 9.0 spike in Max {version}, macOS {platform_version()}",
                  "# (written by runtime-tests/spike/run_spike.py: the lines of interest from Max's log)", ""]
        output.write_text("\n".join(header + self.report) + "\n")
        print(f"\nwrote {output.relative_to(ROOT)}")
        return 0


def platform_version() -> str:
    return subprocess.run(["sw_vers", "-productVersion"], capture_output=True, text=True).stdout.strip() + \
        " " + subprocess.run(["uname", "-m"], capture_output=True, text=True).stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--max", type=Path, default=Path("/Applications/Max.app"), help="the Max application")
    parser.add_argument("--packages", type=Path, default=Path.home() / "Documents" / "Max 9" / "Packages",
                        help="Max's Packages folder")
    parser.add_argument("--only", action="append", default=[], help="run only the sessions whose names contain this")
    parser.add_argument("-v", "--verbose", action="store_true", help="print the harness's log as it runs")
    args = parser.parse_args()
    try:
        return Spike(args.max, args.packages, args.verbose).run(args.only)
    except harness.RunError as error:
        print(f"ERROR: {error}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
