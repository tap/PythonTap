#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
"""Assemble the PythonTap Max package for release (plan 4.5, D3), with every license (plan 4.6).

Run after building, with the runtime installed in support/ (scripts/install-runtime.*):

    python3 scripts/assemble-package.py --platform macos --output dist

writes dist/PythonTap/, a Max package ready to zip: the externals for that platform, the help
patcher, the reference page, the example scripts, the init/ files Max reads at launch, the bundled
runtime and packages, the package's own documents, and a licenses/ folder holding a copy of every
third-party license that ships, with an index (licenses/README.md). Nothing is downloaded; it packages what the build left in the tree.
It fails, rather than producing an incomplete package, if anything required is missing.

    python3 scripts/assemble-package.py --licenses-only --support support --output dist

collects just the licenses (how CI exercises it on Linux, against any installed runtime).

    python3 scripts/assemble-package.py --merge macos-arm64=PythonTap-1.0.0-macos-arm64.zip \
        macos-x86_64=PythonTap-1.0.0-macos-x86_64.zip windows-x64=PythonTap-1.0.0-windows-x64.zip \
        --output dist

makes one package for every platform (plan 4.8) from single-platform ones: every platform's
externals, each runtime in support/<platform> (where the external looks first), each platform's
licenses in licenses/<platform>, and the rest — which must be the same in all of them — once.

Standard library only, so the runtime's own interpreter can run it.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import stat
import sys
import tempfile
import zipfile
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
# init/: text files Max reads at launch — tap.python.txt maps the object tap.python to the file
# tap.python~, which registers both classes (docs/TAP-PYTHON-PLAN.md, D11; plan 9.0), and
# mc.tap.python~ to Max's MC wrapper around it. extras/: the PythonTap Overview, in the Extras menu
# and the package's home patcher (plan 9.4).
FOLDERS = ["help", "docs", "python", "init", "extras"]

# What a package must hold, checked once it is assembled or merged, so that a release cannot lack
# it: above all init/tap.python.txt, with the mappings without which Max finds neither tap.python in
# tap.python~'s binary ("tap.python: No such object", plan 9.0) nor mc.tap.python~ (plan 9.4) —
# with the lines each must say.
REQUIRED = {
    "init/tap.python.txt": ["max objectfile tap.python tap.python~;",
                            "max objectfile mc.tap.python~ mc.wrapper~ tap.python~;"],
    "python/default.py": [],
    "extras/PythonTap Overview.maxpat": [],
}

# Never shipped from the copied folders.
# maxtest_*.py: the runtime tests' fixtures, copied into python/ while runtime-tests/run.py runs
# PRODUCTION-PLAN.md, *-PLAN.md, AUDIT-*.md: the development roadmap, plans and audits in docs/, beside the
# reference pages Max reads
IGNORED = shutil.ignore_patterns("__pycache__", "*.pyc", ".ipynb_checkpoints", ".DS_Store", "maxtest_*",
                                 "PRODUCTION-PLAN.md", "*-PLAN.md", "AUDIT-*.md")


# Each platform's folder for its runtime in a package for every platform (plan 4.8); the external
# names its own (runtime_platform() in tap.python_package.h).
RUNTIME_PLATFORMS = ["macos-arm64", "macos-x86_64", "windows-x64"]

# What differs between single-platform packages, and is merged rather than copied once.
PER_PLATFORM = {"externals", "support", "licenses"}


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
    check_complete(package)
    print(f"assembled {package} (version {version}, {len(licenses)} license files)")
    return package


def check_complete(package: Path) -> None:
    """Fail unless `package` holds everything REQUIRED, each saying what it must."""
    for name, lines in REQUIRED.items():
        path = package / name
        if not path.is_file():
            raise AssemblyError(f"the package has no {name}")
        said = path.read_text(encoding="utf-8").splitlines()
        for line in lines:
            if line not in said:
                raise AssemblyError(f"the package's {name} does not say {line!r}")


def same_content(a: Path, b: Path) -> bool:
    """Whether two files or folders hold the same files with the same contents, line endings aside
    (a Windows checkout has CRLF where a Mac's has LF)."""
    if a.is_file() or b.is_file():
        return a.is_file() and b.is_file() and \
            a.read_bytes().replace(b"\r\n", b"\n") == b.read_bytes().replace(b"\r\n", b"\n")
    files_a = sorted(p.relative_to(a) for p in a.rglob("*") if p.is_file())
    files_b = sorted(p.relative_to(b) for p in b.rglob("*") if p.is_file())
    return files_a == files_b and all(same_content(a / f, b / f) for f in files_a)


def extract(archive: Path, destination: Path) -> None:
    """Unzip a single-platform release zip as the platform that made it would: entries named with
    backslashes (as some Windows zip writers make them) as folders, and the macOS runtime's symlinks
    and executable bits as they were — which Python's zipfile leaves out.

    Nothing is written outside `destination`: an entry whose path leaves it (through `..`, or
    through a symlink an earlier entry made) is refused, as is a symlink whose target is absolute
    or resolves outside it — a crafted zip must fail, not write elsewhere."""
    root = destination.resolve()

    def inside(path: Path) -> bool:
        return path.resolve().is_relative_to(root)  # resolve() follows the symlinks made so far

    with zipfile.ZipFile(archive) as zip_file:
        for info in zip_file.infolist():
            name = info.filename.replace("\\", "/")
            if name.startswith("__MACOSX/"):
                continue  # resource forks
            target = destination / name
            if ".." in Path(name).parts or Path(name).is_absolute() or not inside(target):
                raise AssemblyError(f"{archive.name}: entry {info.filename!r} would land outside {destination}")
            mode = info.external_attr >> 16 if info.create_system == 3 else 0  # Unix
            if name.endswith("/"):
                target.mkdir(parents=True, exist_ok=True)
            elif stat.S_ISLNK(mode):
                link = zip_file.read(info).decode("utf-8")
                if Path(link).is_absolute() or not inside(target.parent / link):
                    raise AssemblyError(f"{archive.name}: symlink {info.filename!r} -> {link!r} points outside "
                                        f"{destination}")
                target.parent.mkdir(parents=True, exist_ok=True)
                os.symlink(link, target)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(zip_file.read(info))
                if mode & 0o777:
                    target.chmod(mode & 0o777)


def merge(packages: dict[str, Path], output: Path) -> Path:
    """One package for every platform (plan 4.8) from single-platform ones, each a PythonTap/ folder
    or a release zip holding one, keyed by its runtime platform."""
    with tempfile.TemporaryDirectory() as unpacked:
        folders: dict[str, Path] = {}
        for platform, source in packages.items():
            if source.suffix == ".zip":
                extract(source, Path(unpacked) / platform)
                source = Path(unpacked) / platform / PACKAGE_NAME
            folders[platform] = source
        return merge_folders(folders, output)


def merge_folders(packages: dict[str, Path], output: Path) -> Path:
    unknown = sorted(set(packages) - set(RUNTIME_PLATFORMS))
    if unknown:
        raise AssemblyError(f"unknown platform(s) {', '.join(unknown)}; expected {', '.join(RUNTIME_PLATFORMS)}")
    for platform, source in packages.items():
        if not (source / "support").is_dir() or not (source / "externals").is_dir():
            raise AssemblyError(f"{source} is not a single-platform package (no support/ or externals/)")
    # the shared content comes from a Mac package where there is one: its text has LF line endings
    order = sorted(packages, key=lambda platform: not platform.startswith("macos"))
    base = packages[order[0]]

    package = output / PACKAGE_NAME
    if package.exists():
        shutil.rmtree(package)
    package.mkdir(parents=True)

    for entry in sorted(base.iterdir()):
        if entry.name in PER_PLATFORM:
            continue
        for platform in order[1:]:
            if not same_content(entry, packages[platform] / entry.name):
                raise AssemblyError(f"{entry.name} differs between the {order[0]} and {platform} packages")
        if entry.is_dir():
            shutil.copytree(entry, package / entry.name, symlinks=True)
        else:
            shutil.copy2(entry, package / entry.name)

    (package / "externals").mkdir()
    for platform in order:
        for external in sorted((packages[platform] / "externals").iterdir()):
            destination = package / "externals" / external.name
            if destination.exists():
                continue  # the same universal macOS external, built on each Mac's runner: one is enough
            if external.is_dir():
                shutil.copytree(external, destination, symlinks=True)
            else:
                shutil.copy2(external, destination)

    lines = ["# Third-party licenses", "",
             "This package carries a runtime for each platform, so each platform's licenses are",
             "collected separately — what ships for it:", ""]
    for platform in RUNTIME_PLATFORMS:
        if platform not in packages:
            continue
        shutil.copytree(packages[platform] / "support", package / "support" / platform, symlinks=True)
        shutil.copytree(packages[platform] / "licenses", package / "licenses" / platform)
        lines.append(f"- [{platform}]({platform}/README.md)")
    (package / "licenses" / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    check_complete(package)
    print(f"merged {package} ({', '.join(p for p in RUNTIME_PLATFORMS if p in packages)})")
    return package


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--platform", choices=sorted(EXTERNALS), help="the platform whose externals to package")
    parser.add_argument("--output", type=Path, required=True, help="folder to write PythonTap/ (or licenses/) into")
    parser.add_argument("--version", help="fail unless package-info.json has this version (e.g. 0.9.0)")
    parser.add_argument("--licenses-only", action="store_true", help="only collect the licenses")
    parser.add_argument("--support", type=Path, default=PACKAGE_ROOT / "support",
                        help="the runtime to collect licenses from (with --licenses-only)")
    parser.add_argument("--merge", nargs="+", metavar="PLATFORM=PATH",
                        help="merge single-platform packages (PythonTap/ folders or release zips) into one "
                             f"for every platform (platforms: {', '.join(RUNTIME_PLATFORMS)})")
    args = parser.parse_args()

    try:
        if args.merge:
            packages: dict[str, Path] = {}
            for item in args.merge:
                platform, _, path = item.partition("=")
                if not path:
                    parser.error(f"--merge takes PLATFORM=PATH, not {item!r}")
                packages[platform] = Path(path)
            merge(packages, args.output)
        elif args.licenses_only:
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
