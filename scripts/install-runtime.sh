#!/bin/bash
# Copyright 2022-2026 Timothy Place. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.
#
# Install the embedded Python runtime for tap.python~ into <package>/support (macOS).
#
# Downloads a relocatable CPython from python-build-standalone
# (https://github.com/astral-sh/python-build-standalone), verifies it against the SHA256
# committed in scripts/runtime.lock, unpacks it into support/, and installs the Python packages
# used by the examples (attrs, numpy) from scripts/requirements.lock, hashes enforced. What gets
# installed is exactly what those files name; to upgrade, regenerate them with
# scripts/update-locks.py and commit the result.
#
# Re-running replaces the runtime. The previous one is kept until the new one is fully
# installed and restored if anything fails. Packages you added yourself with pip are not
# carried over; reinstall them afterwards.
#
# Usage:
#   ./scripts/install-runtime.sh              # this Mac's architecture (what users run)
#   ./scripts/install-runtime.sh --universal  # ALSO lipo a universal libpython, for building
#                                             # universal externals (CI; see ReadMe)

set -euo pipefail

usage() {
    sed -n '/^# Usage:/,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

UNIVERSAL=0
for arg in "$@"; do
    case "$arg" in
        --universal) UNIVERSAL=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "error: unknown option '$arg'" >&2; usage >&2; exit 2 ;;
    esac
done

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "error: this installer is for macOS; on Windows run scripts/install-runtime.ps1." >&2
    echo "       (On Linux, development builds embed a system CPython 3.13 — see ReadMe.md.)" >&2
    exit 1
fi

PACKAGE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SUPPORT="$PACKAGE_ROOT/support"
PREVIOUS="$PACKAGE_ROOT/support.previous"
RUNTIME_LOCK="$PACKAGE_ROOT/scripts/runtime.lock"
REQUIREMENTS_LOCK="$PACKAGE_ROOT/scripts/requirements.lock"

# Apple Silicon reports x86_64 from uname -m under Rosetta; the runtime must match what
# Max runs natively, so ask the hardware.
if [[ "$(sysctl -n hw.optional.arm64 2>/dev/null || echo 0)" == "1" ]]; then
    NATIVE_TRIPLE="aarch64-apple-darwin"; OTHER_TRIPLE="x86_64-apple-darwin"
else
    NATIVE_TRIPLE="x86_64-apple-darwin";  OTHER_TRIPLE="aarch64-apple-darwin"
fi

PBS_RELEASE="$(awk '$1 == "release" { print $2 }' "$RUNTIME_LOCK")"
URL_PREFIX="https://github.com/astral-sh/python-build-standalone/releases/download/$PBS_RELEASE"

# The pinned asset name and SHA256 for a target triple, from runtime.lock.
lock_field() { # <triple> <field: 2 = asset, 3 = sha256>
    awk -v t="$1" -v f="$2" '$1 == t { print $f }' "$RUNTIME_LOCK"
}

WORK="$(mktemp -d)"
SWAPPED=0   # this run moved the old runtime aside
INSTALLED=0
cleanup() {
    local status=$?
    rm -rf "$WORK"
    # roll back to the previous runtime if we moved it aside and did not finish
    if [[ "$SWAPPED" == 1 && "$INSTALLED" != 1 && -e "$PREVIOUS" ]]; then
        echo "==> Install failed; restoring the previous runtime" >&2
        rm -rf "$SUPPORT"
        mv "$PREVIOUS" "$SUPPORT"
    fi
    exit $status
}
trap cleanup EXIT

fetch_verified() { # <triple> <destination>
    local asset expected actual
    asset="$(lock_field "$1" 2)"
    expected="$(lock_field "$1" 3)"
    if [[ -z "$asset" || -z "$expected" ]]; then
        echo "error: scripts/runtime.lock has no entry for $1" >&2
        exit 1
    fi
    echo "==> Downloading $asset"
    curl -fSL --progress-bar "$URL_PREFIX/$asset" -o "$2"
    actual="$(shasum -a 256 "$2" | awk '{print $1}')"
    if [[ "$expected" != "$actual" ]]; then
        echo "error: SHA256 mismatch for $asset" >&2
        echo "       expected $expected (scripts/runtime.lock)" >&2
        echo "       got      $actual" >&2
        exit 1
    fi
}

# Download, verify and prepare everything in the scratch folder before touching support/.
fetch_verified "$NATIVE_TRIPLE" "$WORK/native.tar.gz"
mkdir -p "$WORK/native"
tar -xzf "$WORK/native.tar.gz" -C "$WORK/native"    # unpacks to a top-level python/ dir

# The external links libpython by @rpath and carries an rpath pointing at
# <package>/support/lib, so give the dylib an @rpath install name. Re-sign
# afterwards: install_name_tool invalidates the signature, and arm64 macOS
# refuses to load unsigned code.
LIBPYTHON="$(ls "$WORK"/native/python/lib/libpython3.*.dylib | head -1)"
LIBNAME="$(basename "$LIBPYTHON")"

if [[ "$UNIVERSAL" == 1 ]]; then
    # For building a universal (arm64 + x86_64) external we need a libpython
    # with both slices to link against. The stdlib and site-packages stay
    # native-arch: Max 9 always runs natively, so only the link step needs fat.
    fetch_verified "$OTHER_TRIPLE" "$WORK/other.tar.gz"
    mkdir -p "$WORK/other"
    tar -xzf "$WORK/other.tar.gz" -C "$WORK/other"
    echo "==> Creating universal $LIBNAME"
    lipo -create "$LIBPYTHON" "$WORK/other/python/lib/$LIBNAME" -output "$WORK/libpython-universal.dylib"
    mv "$WORK/libpython-universal.dylib" "$LIBPYTHON"
fi

echo "==> Setting @rpath install name on $LIBNAME"
install_name_tool -id "@rpath/$LIBNAME" "$LIBPYTHON"
codesign --force --sign - "$LIBPYTHON"

# Swap: keep the previous runtime until the new one is complete (the trap restores it).
rm -rf "$PREVIOUS"
if [[ -e "$SUPPORT" ]]; then
    echo "==> Moving the existing runtime aside"
    mv "$SUPPORT" "$PREVIOUS"
    SWAPPED=1
fi
echo "==> Installing runtime into $SUPPORT"
mv "$WORK/native/python" "$SUPPORT"

echo "==> Installing Python packages from scripts/requirements.lock"
"$SUPPORT/bin/python3" -m pip install --quiet --no-warn-script-location --disable-pip-version-check \
    --require-hashes --only-binary :all: -r "$REQUIREMENTS_LOCK"

INSTALLED=1
rm -rf "$PREVIOUS"
echo "==> Done. $("$SUPPORT/bin/python3" --version) installed at $SUPPORT"
