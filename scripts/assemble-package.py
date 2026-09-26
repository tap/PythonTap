#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
"""Assemble the PythonTap Max package for release (plan 4.5, D3), with every license (plan 4.6).

Run after building, with the runtime installed in support/ (scripts/install-runtime.*):

    python3 scripts/assemble-package.py --platform macos --output dist

writes dist/PythonTap/, a Max package ready to zip: the externals for that platform, the help
patcher, the reference page, the example scripts, the bundled runtime and packages, the package's
own documents, and a licenses/ folder holding a copy of every third-party license that ships, with
an index (licenses/README.md). Nothing is downloaded; it packages what the build left in the tree.
It fails, rather than producing an incomplete package, if anything required is missing.

    python3 scripts/assemble-package.py --licenses-only --support support --output dist

collects just the licenses (how CI exercises it on Linux, against any installed runtime).

Standard library only, so the runtime's own interpreter can run it.
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parent.parent
PACKAGE_NAME = "PythonTap"

# The external's file name per platform, as the build writes it into externals/.
EXTERNALS = {
    "macos": "tap.python~.mxo",
    "windows": "tap.python~.mxe64",
}

# Package content copied as-is (relative to the package root).
DOCUMENTS = ["ReadMe.md", "License.md", "CHANGELOG.md", "icon.png", "package-info.json"]
FOLDERS = ["help", "docs", "python"]

# Never shipped from the copied folders.
IGNORED = shutil.ignore_patterns("__pycache__", "*.pyc", ".ipynb_checkpoints", ".DS_Store")


class AssemblyError(Exception):
    pass


@dataclass
class License:
    component: str  # what it covers, for the index
    path: str  # where the copy lives, relative to licenses/
    note: str = ""


def copy_file(source: Path, destination: Path) -> None:
    if not source.is_file():
        raise AssemblyError(f"missing {source}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def find_cpython_license(support: Path) -> Path:
    """CPython's LICENSE.txt: support/LICENSE.txt on Windows, support/lib/python3.x/ on macOS."""
    candidates = [support / "LICENSE.txt", *sorted(support.glob("lib/python3.*/LICENSE.txt"))]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise AssemblyError(f"no CPython LICENSE.txt in {support} — is the runtime installed?")


def site_packages(support: Path) -> list[Path]:
    """The runtime's site-packages folder(s): Lib/site-packages on Windows, lib/python3.x/ on macOS."""
    found = [p for p in [support / "Lib" / "site-packages", *support.glob("lib/python3.*/site-packages")]
             if p.is_dir()]
    return found


def distribution_name(dist_info: Path) -> str:
    """'numpy-2.5.3.dist-info' -> 'numpy-2.5.3', from its METADATA when present."""
    metadata = dist_info / "METADATA"
    name = version = None
    if metadata.is_file():
        for line in metadata.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith("Name:") and name is None:
                name = line.split(":", 1)[1].strip()
            elif line.startswith("Version:") and version is None:
                version = line.split(":", 1)[1].strip()
            elif not line.strip():
                break  # end of the headers
    if name and version:
        return f"{name}-{version}"
    return dist_info.name.removesuffix(".dist-info")


def collect_licenses(support: Path, destination: Path) -> list[License]:
    """Copy every license that ships into `destination` and return the index entries."""
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir(parents=True)
    licenses: list[License] = []

    def add(component: str, source: Path, relative: str, note: str = "") -> None:
        copy_file(source, destination / relative)
        licenses.append(License(component, relative, note))

    # Compiled into the external.
    add("Min-API (Cycling '74's C++ API for Max externals)", PACKAGE_ROOT / "source/min-api/License.md",
        "min-api/License.md")
    add("Max SDK (Cycling '74), via Min-API", PACKAGE_ROOT / "source/min-api/max-sdk-base/LICENSE.md",
        "max-sdk/LICENSE.md")

    # The bundled runtime.
    add("CPython", find_cpython_license(support), "cpython/LICENSE.txt",
        "Includes the licenses of the components CPython itself bundles. The runtime is built by "
        "python-build-standalone (https://github.com/astral-sh/python-build-standalone), which "
        "documents the licenses of the libraries it links in (OpenSSL, SQLite, and others) at "
        "https://gregoryszorc.com/docs/python-build-standalone/main/running.html#licensing.")

    # Every package installed in the runtime: each wheel's own license files (PEP 639's
    # dist-info/licenses/, which is where numpy lists what it bundles — OpenBLAS, LAPACK, the GCC
    # runtime — or, for older wheels, LICENSE*/COPYING*/NOTICE* in the dist-info itself).
    packages = site_packages(support)
    if not packages:
        raise AssemblyError(f"no site-packages in {support} — is the runtime installed?")
    for folder in packages:
        for dist_info in sorted(folder.glob("*.dist-info")):
            name = distribution_name(dist_info)
            files = [p for p in (dist_info / "licenses").rglob("*") if p.is_file()] \
                if (dist_info / "licenses").is_dir() else []
            files += [p for p in dist_info.iterdir()
                      if p.is_file() and p.name.upper().startswith(("LICENSE", "LICENCE", "COPYING", "NOTICE"))]
            if not files:
                raise AssemblyError(f"{dist_info.name} carries no license file; check it by hand before shipping")
            for file in files:
                relative = file.relative_to(dist_info / "licenses") if file.is_relative_to(dist_info / "licenses") \
                    else Path(file.name)
                add(f"Python package {name}", file, f"python-packages/{name}/{relative.as_posix()}")

    write_index(destination, licenses)
    return licenses


def write_index(destination: Path, licenses: list[License]) -> None:
    lines = [
        "# Third-party licenses",
        "",
        "PythonTap itself is MIT licensed (see `License.md` in the package root). It is built with, and",
        "ships, the third-party software below; this folder holds a copy of each license, collected",
        "by `scripts/assemble-package.py` from exactly what this package contains.",
        "",
    ]
    groups: dict[str, list[License]] = {}
    for entry in licenses:
        groups.setdefault(entry.component, []).append(entry)
    for component, entries in groups.items():
        lines += [f"## {component}", ""]
        if entries[0].note:
            lines += [entries[0].note, ""]
        lines += [f"- [`{e.path}`]({e.path})" for e in entries]
        lines.append("")
    (destination / "README.md").write_text("\n".join(lines), encoding="utf-8")


def check_version(expected: str | None) -> str:
    """The package version from the generated package-info.json, checked against `expected`."""
    info = PACKAGE_ROOT / "package-info.json"
    if not info.is_file():
        raise AssemblyError("package-info.json is missing — configure with CMake first (it is generated)")
    version = json.loads(info.read_text(encoding="utf-8"))["version"]
    if expected is not None and version != expected:
        raise AssemblyError(f"package-info.json says version {version}, but the release is {expected} "
                            "(min takes the version from the latest git tag; check out with tags)")
    return version


def assemble(platform: str, output: Path, expected_version: str | None) -> Path:
    version = check_version(expected_version)
    support = PACKAGE_ROOT / "support"
    if not support.is_dir():
        raise AssemblyError("support/ is missing — install the runtime first (scripts/install-runtime.*)")

    package = output / PACKAGE_NAME
    if package.exists():
        shutil.rmtree(package)
    package.mkdir(parents=True)

    external = PACKAGE_ROOT / "externals" / EXTERNALS[platform]
    if not external.exists():
        raise AssemblyError(f"missing {external} — build the external first")
    (package / "externals").mkdir()
    if external.is_dir():
        shutil.copytree(external, package / "externals" / external.name, symlinks=True)
    else:
        copy_file(external, package / "externals" / external.name)

    for folder in FOLDERS:
        shutil.copytree(PACKAGE_ROOT / folder, package / folder, symlinks=True, ignore=IGNORED)
    for document in DOCUMENTS:
        copy_file(PACKAGE_ROOT / document, package / document)

    # The runtime, as installed (symlinks kept: the macOS runtime uses them), minus caches.
    shutil.copytree(support, package / "support", symlinks=True, ignore=IGNORED)

    licenses = collect_licenses(support, package / "licenses")
    print(f"assembled {package} (version {version}, {len(licenses)} license files)")
    return package


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--platform", choices=sorted(EXTERNALS), help="the platform whose externals to package")
    parser.add_argument("--output", type=Path, required=True, help="folder to write PythonTap/ (or licenses/) into")
    parser.add_argument("--version", help="fail unless package-info.json has this version (e.g. 0.9.0)")
    parser.add_argument("--licenses-only", action="store_true", help="only collect the licenses")
    parser.add_argument("--support", type=Path, default=PACKAGE_ROOT / "support",
                        help="the runtime to collect licenses from (with --licenses-only)")
    args = parser.parse_args()

    try:
        if args.licenses_only:
            licenses = collect_licenses(args.support, args.output / "licenses")
            print(f"collected {len(licenses)} license files into {args.output / 'licenses'}")
        else:
            if not args.platform:
                parser.error("--platform is required (unless --licenses-only)")
            assemble(args.platform, args.output, args.version)
    except AssemblyError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
