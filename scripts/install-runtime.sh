#!/bin/bash
# Copyright 2022-2026 Timothy Place. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.
#
# Install the embedded Python runtime for tap.python~ into <package>/support.
#
# Downloads a relocatable CPython from python-build-standalone
# (https://github.com/astral-sh/python-build-standalone), verifies it against
# the release's SHA256SUMS, unpacks it into support/, and installs the Python
# packages used by the examples (attrs, numpy).
#
# Usage:
#   ./scripts/install-runtime.sh              # native arch (what users run)
#   ./scripts/install-runtime.sh --universal  # ALSO lipo a universal libpython
#                                             # for building universal externals (CI)
#
# Pinned via the env vars below; override to upgrade:
#   PBS_RELEASE=20260623 PYTHON_SERIES=3.13 ./scripts/install-runtime.sh

set -euo pipefail

PBS_RELEASE="${PBS_RELEASE:-20260623}"
PYTHON_SERIES="${PYTHON_SERIES:-3.13}"
PIP_PACKAGES=(attrs numpy)

PACKAGE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SUPPORT="$PACKAGE_ROOT/support"
URL_PREFIX="https://github.com/astral-sh/python-build-standalone/releases/download/$PBS_RELEASE"

case "$(uname -m)" in
    arm64|aarch64) NATIVE_TRIPLE="aarch64-apple-darwin"; OTHER_TRIPLE="x86_64-apple-darwin" ;;
    x86_64)        NATIVE_TRIPLE="x86_64-apple-darwin";  OTHER_TRIPLE="aarch64-apple-darwin" ;;
    *) echo "error: unsupported architecture $(uname -m)" >&2; exit 1 ;;
esac

UNIVERSAL=0
if [[ "${1:-}" == "--universal" ]]; then
    UNIVERSAL=1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "==> Fetching asset manifest for python-build-standalone $PBS_RELEASE"
curl -fsSL "$URL_PREFIX/SHA256SUMS" -o "$WORK/SHA256SUMS"

# Resolve the exact cpython-3.13.x asset name for a target triple from the manifest.
asset_for() {
    local triple="$1"
    # each release carries exactly one patch version per series, so unique-sort suffices
    grep -oE "cpython-${PYTHON_SERIES//./\\.}\.[0-9]+\+${PBS_RELEASE}-${triple}-install_only\.tar\.gz" "$WORK/SHA256SUMS" \
        | sort -u | tail -1
}

fetch_verified() {
    local asset="$1" dest="$2"
    echo "==> Downloading $asset"
    curl -fSL --progress-bar "$URL_PREFIX/$asset" -o "$dest"
    local expected actual
    expected="$(grep " $asset\$" "$WORK/SHA256SUMS" | head -1 | awk '{print $1}')"
    actual="$(shasum -a 256 "$dest" | awk '{print $1}')"
    if [[ -z "$expected" || "$expected" != "$actual" ]]; then
        echo "error: SHA256 mismatch for $asset" >&2
        exit 1
    fi
}

NATIVE_ASSET="$(asset_for "$NATIVE_TRIPLE")"
if [[ -z "$NATIVE_ASSET" ]]; then
    echo "error: no cpython $PYTHON_SERIES install_only asset for $NATIVE_TRIPLE in release $PBS_RELEASE" >&2
    exit 1
fi

fetch_verified "$NATIVE_ASSET" "$WORK/native.tar.gz"

if [[ -e "$SUPPORT" ]]; then
    echo "==> Removing existing runtime at $SUPPORT"
    rm -rf "$SUPPORT"
fi

echo "==> Installing runtime into $SUPPORT"
mkdir -p "$WORK/native"
tar -xzf "$WORK/native.tar.gz" -C "$WORK/native"    # unpacks to a top-level python/ dir
mv "$WORK/native/python" "$SUPPORT"

# The external links libpython by @rpath and carries an rpath pointing at
# <package>/support/lib, so give the dylib an @rpath install name. Re-sign
# afterwards: install_name_tool invalidates the signature, and arm64 macOS
# refuses to load unsigned code.
LIBPYTHON="$(ls "$SUPPORT"/lib/libpython"$PYTHON_SERIES"*.dylib | head -1)"
echo "==> Setting @rpath install name on $(basename "$LIBPYTHON")"
install_name_tool -id "@rpath/$(basename "$LIBPYTHON")" "$LIBPYTHON"
codesign --force --sign - "$LIBPYTHON"

if [[ "$UNIVERSAL" == 1 ]]; then
    # For building a universal (arm64 + x86_64) external we need a libpython
    # with both slices to link against. The stdlib and site-packages stay
    # native-arch: Max 9 always runs natively, so only the link step needs fat.
    OTHER_ASSET="$(asset_for "$OTHER_TRIPLE")"
    fetch_verified "$OTHER_ASSET" "$WORK/other.tar.gz"
    mkdir -p "$WORK/other"
    tar -xzf "$WORK/other.tar.gz" -C "$WORK/other"
    OTHER_LIB="$WORK/other/python/lib/$(basename "$LIBPYTHON")"
    echo "==> Creating universal $(basename "$LIBPYTHON")"
    lipo -create "$LIBPYTHON" "$OTHER_LIB" -output "$WORK/libpython-universal.dylib"
    mv "$WORK/libpython-universal.dylib" "$LIBPYTHON"
    install_name_tool -id "@rpath/$(basename "$LIBPYTHON")" "$LIBPYTHON"
    codesign --force --sign - "$LIBPYTHON"
fi

echo "==> Installing Python packages: ${PIP_PACKAGES[*]}"
"$SUPPORT/bin/python3" -m pip install --quiet --no-warn-script-location --upgrade pip
"$SUPPORT/bin/python3" -m pip install --quiet --no-warn-script-location "${PIP_PACKAGES[@]}"

echo "==> Done. $("$SUPPORT/bin/python3" --version) installed at $SUPPORT"
