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
- The book (plan 9.7) and the ReadMe: every chapter is in book/src/SUMMARY.md, every file a chapter
  includes exists, and every relative link — in the book or the ReadMe — names a file that exists
  and, for a heading, one that page has. (mdBook checks none of that; a broken link is a 404.)

Standard library only. Exits non-zero, naming each problem, if anything is wrong.
"""

from __future__ import annotations

import json
import re
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
BOOK = ROOT / "book" / "src"
# Rendered by scripts/notebook-to-book.py at build time, so absent from a fresh checkout; CI's book
# job renders it and builds the book with warnings as errors.
GENERATED = {BOOK / "notebook.md"}

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


def anchor(heading: str) -> str:
    """The id mdBook and GitHub give a heading: lower case, punctuation dropped, spaces as hyphens."""
    heading = re.sub(r"<[^>]+>", "", heading).strip().lower()
    return re.sub(r"[^\w\- ]", "", heading).replace(" ", "-")


def anchors(path: Path) -> set[str]:
    text = re.sub(r"^```.*?^```", "", path.read_text(encoding="utf-8"), flags=re.M | re.S)
    return {anchor(heading) for heading in re.findall(r"^#+\s+(.*)$", text, re.M)}


def check_links(path: Path) -> None:
    text = re.sub(r"^```.*?^```", "", path.read_text(encoding="utf-8"), flags=re.M | re.S)
    for target in re.findall(r"\]\(([^)\s]+)\)|srcset=\"([^\"]+)\"|src=\"([^\"]+)\"", text):
        target = next(t for t in target if t)
        if target.startswith(("http:", "https:", "mailto:")):
            continue
        file, _, heading = target.partition("#")
        destination = (path.parent / file) if file else path
        if destination in GENERATED:
            continue
        if not destination.exists():
            problem(path, f"links to {target}, which does not exist")
        elif heading and destination.suffix == ".md" and heading not in anchors(destination):
            problem(path, f"links to {target}, a heading {destination.name} does not have")


def check_book() -> None:
    summary = (BOOK / "SUMMARY.md").read_text(encoding="utf-8")
    listed = set(re.findall(r"\]\(([^)]+\.md)\)", summary))
    for path in sorted(BOOK.glob("*.md")):
        if path.name != "SUMMARY.md" and path.name not in listed and path not in GENERATED:
            problem(path, "is not in SUMMARY.md")
    for name in sorted(listed - {p.name for p in BOOK.glob("*.md")} - {p.name for p in GENERATED}):
        problem(BOOK / "SUMMARY.md", f"lists {name}, which does not exist")
    for path in sorted(set(BOOK.glob("*.md")) - GENERATED):
        for include in re.findall(r"\{\{#include ([^}:\s]+)", path.read_text(encoding="utf-8")):
            if not (path.parent / include).exists():
                problem(path, f"includes {include}, which does not exist")
        check_links(path)
    check_links(ROOT / "ReadMe.md")


def main() -> int:
    check_pages()
    check_patchers()
    check_book()
    for line in problems:
        print(line, file=sys.stderr)
    if problems:
        print(f"{len(problems)} problem(s) in the hand-made documentation", file=sys.stderr)
        return 1
    print("the hand-made documentation is well-formed, and its links and patch cords hold")
    return 0


if __name__ == "__main__":
    sys.exit(main())
