#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
"""Render python/allpass-doc.ipynb, as committed (executed), into the book (plan 9.7).

    python3 scripts/notebook-to-book.py        # before `mdbook build book`; CI runs it

writes book/src/notebook.md — the notebook's markdown cells as they are, its code cells as Python
blocks, and their outputs: text as text blocks, images as files beside the page
(book/src/notebook/) — so that the book shows what the notebook computed, and cannot drift from it.
Both are generated, and ignored by git; never edit them. Standard library only.
"""

from __future__ import annotations

import base64
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NOTEBOOK = ROOT / "python" / "allpass-doc.ipynb"
PAGE = ROOT / "book" / "src" / "notebook.md"
IMAGES = ROOT / "book" / "src" / "notebook"
NOTE = ("*Rendered from [`python/allpass-doc.ipynb`](https://github.com/tap/PythonTap/blob/main/python/"
        "allpass-doc.ipynb), as committed — executed, its outputs those it computed.*")


def text(value) -> str:
    return "".join(value) if isinstance(value, list) else str(value)


def fenced(body: str, language: str = "") -> list[str]:
    body = body.rstrip("\n")
    fence = "```"
    while fence in body:
        fence += "`"
    return [f"{fence}{language}", body, fence, ""]


def render(notebook: dict) -> tuple[list[str], dict[str, bytes]]:
    lines: list[str] = []
    images: dict[str, bytes] = {}
    for index, cell in enumerate(notebook["cells"]):
        source = text(cell.get("source", ""))
        if cell["cell_type"] == "markdown":
            lines += [source.rstrip("\n"), ""]
            if index == 0:
                lines += [NOTE, ""]
            continue
        if cell["cell_type"] != "code" or not source.strip():
            continue
        lines += fenced(source, "python")
        for output in cell.get("outputs", []):
            kind = output.get("output_type")
            if kind == "stream":
                lines += fenced(text(output.get("text", "")), "text")
            elif kind in ("display_data", "execute_result"):
                data = output.get("data", {})
                if "image/png" in data:
                    name = f"cell-{index}-{len(images) + 1}.png"
                    images[name] = base64.b64decode(text(data["image/png"]))
                    lines += [f"![The output of the cell above](notebook/{name})", ""]
                elif "text/plain" in data:
                    lines += fenced(text(data["text/plain"]), "text")
            elif kind == "error":
                lines += fenced("\n".join(output.get("traceback", [])), "text")
    return lines, images


def main() -> int:
    notebook = json.loads(NOTEBOOK.read_text(encoding="utf-8"))
    if notebook.get("nbformat") != 4:
        print(f"error: {NOTEBOOK.name} is not an nbformat 4 notebook", file=sys.stderr)
        return 1
    lines, images = render(notebook)
    shutil.rmtree(IMAGES, ignore_errors=True)
    if images:
        IMAGES.mkdir(parents=True)
        for name, data in images.items():
            (IMAGES / name).write_bytes(data)
    PAGE.write_text("\n".join(lines).rstrip("\n") + "\n", encoding="utf-8")
    print(f"wrote {PAGE.relative_to(ROOT)} and {len(images)} image(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
