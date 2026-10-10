#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
"""Check the documentation written by hand (plan 9.4): what no build makes, and no test runs.

    python3 scripts/check-docs.py

- Every page in docs/ — the reference pages, the guide and topic (vignettes), the tutorials — is
  well-formed XML with the root element its kind needs; a reference page names its object, and a
  tutorial's patcher is beside it.
- Every link between them resolves: a guide's or tutorial's link to a tutorial or a vignette names
  one that exists, an openfilelink names a file in the package, and a tutorial's previous and next
  exist.
- Every patcher — the help files, the tutorials' patchers, the Overview — is JSON in which, in each
  patcher and subpatcher, box ids are unique and every patch cord joins two boxes that exist, from
  an outlet the source has to an inlet the destination has.

Standard library only. Exits non-zero, naming each problem, if anything is wrong.
"""

from __future__ import annotations

import json
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"

problems: list[str] = []


def problem(path: Path, what: str) -> None:
    problems.append(f"{path.relative_to(ROOT)}: {what}")


def parse(path: Path) -> ElementTree.Element | None:
    try:
        return ElementTree.parse(path).getroot()
    except ElementTree.ParseError as error:
        problem(path, f"not well-formed XML ({error})")
        return None


def stem(path: Path, suffix: str) -> str:
    return path.name[: -len(suffix)]


def check_pages() -> None:
    tutorials = {stem(p, ".maxtut.xml"): p for p in DOCS.rglob("*.maxtut.xml")}
    vignettes = {stem(p, ".maxvig.xml"): p for p in DOCS.rglob("*.maxvig.xml")}
    files = {p.name for folder in ("help", "extras", "docs") for p in (ROOT / folder).rglob("*") if p.is_file()}

    for path in sorted(DOCS.glob("*.maxref.xml")):
        root = parse(path)
        if root is None:
            continue
        name = stem(path, ".maxref.xml")
        if root.tag != "c74object" or root.get("name") != name:
            problem(path, f"the root must be <c74object name=\"{name}\">")
        if root.find("digest") is None or not "".join(root.find("digest").itertext()).strip():
            problem(path, "has no digest")

    pages = list(vignettes.values()) + list(tutorials.values())
    for path in sorted(pages):
        root = parse(path)
        if root is None:
            continue
        tutorial = path.name.endswith(".maxtut.xml")
        if root.tag != ("chapter" if tutorial else "vignette"):
            problem(path, f"the root must be <{'chapter' if tutorial else 'vignette'}>")
        if tutorial:
            openfile = root.find("openfile")
            if openfile is None or not (path.parent / openfile.get("patch", "")).is_file():
                problem(path, "its <openfile patch=…> names no patcher beside it")
            for element in ("previous", "next", "parent"):
                for link in root.iter(element):
                    if link.get("name") not in tutorials:
                        problem(path, f"<{element} name=\"{link.get('name')}\"> names no tutorial")
        for link in root.iter("link"):
            kind, name = link.get("type"), link.get("name")
            if kind == "tutorial" and name not in tutorials:
                problem(path, f"links to the tutorial {name}, which does not exist")
            if kind in ("vignette", "topic") and name not in vignettes:
                problem(path, f"links to the vignette {name}, which does not exist")
        for link in root.iter("openfilelink"):
            if link.get("filename") not in files:
                problem(path, f"<openfilelink filename=\"{link.get('filename')}\"> names no file in the package")


def check_patcher(path: Path, patcher: dict, where: str) -> None:
    boxes: dict[str, dict] = {}
    for entry in patcher.get("boxes", []):
        box = entry.get("box", {})
        box_id = box.get("id")
        if box_id in boxes:
            problem(path, f"{where}: the box id {box_id} is used twice")
        boxes[box_id] = box
        if "patcher" in box:
            check_patcher(path, box["patcher"], f"{where} > [{box.get('text', box_id)}]")
    for entry in patcher.get("lines", []):
        line = entry.get("patchline", {})
        (source, outlet), (destination, inlet) = line.get("source", [None, 0]), line.get("destination", [None, 0])
        if source not in boxes or destination not in boxes:
            problem(path, f"{where}: a patch cord joins {source} and {destination}, which are not both boxes")
            continue
        if outlet >= boxes[source].get("numoutlets", 0):
            problem(path, f"{where}: a patch cord leaves outlet {outlet} of {source}, which has "
                          f"{boxes[source].get('numoutlets', 0)}")
        if inlet >= boxes[destination].get("numinlets", 0):
            problem(path, f"{where}: a patch cord reaches inlet {inlet} of {destination}, which has "
                          f"{boxes[destination].get('numinlets', 0)}")


def check_patchers() -> None:
    paths = sorted(list((ROOT / "help").glob("*.maxhelp")) + list(DOCS.rglob("*.maxpat"))
                   + list((ROOT / "extras").glob("*.maxpat")))
    for path in paths:
        try:
            document = json.loads(path.read_text(encoding="utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            problem(path, f"not JSON ({error})")
            continue
        if "patcher" not in document:
            problem(path, "has no patcher")
            continue
        check_patcher(path, document["patcher"], "the patcher")


def main() -> int:
    check_pages()
    check_patchers()
    for line in problems:
        print(line, file=sys.stderr)
    if problems:
        print(f"{len(problems)} problem(s) in the hand-made documentation", file=sys.stderr)
        return 1
    print("the hand-made documentation is well-formed, and its links and patch cords hold")
    return 0


if __name__ == "__main__":
    sys.exit(main())
