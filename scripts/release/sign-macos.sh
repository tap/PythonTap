#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
#
# Sign every Mach-O in an assembled package with a Developer ID, for notarization (plan 4.5).
#
# usage: scripts/release/sign-macos.sh <package-dir> <signing-identity>
#
# Signs inside out — every loose Mach-O file first (the runtime's dylibs, extension modules and
# executables), then each .mxo bundle — all with the hardened runtime and a secure timestamp, as
# notarization requires. The runtime's executables get scripts/release/python.entitlements, so the
# bundled python3 can still load extension modules users install with pip (signed by nobody).
# The external itself needs no entitlements: it runs inside Max, under Max's.
set -euo pipefail

if [ $# -ne 2 ]; then
    echo "usage: $0 <package-dir> <signing-identity>" >&2
    exit 2
fi
package="$1"
identity="$2"
entitlements="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/python.entitlements"

sign() { # <file> [extra codesign args...]
    local file="$1"; shift
    codesign --force --timestamp --options runtime --sign "$identity" "$@" "$file"
}

count=0
while IFS= read -r -d '' file; do
    description="$(file -b "$file")"
    case "$description" in
        Mach-O*executable*) sign "$file" --entitlements "$entitlements" ;;
        Mach-O*)            sign "$file" ;;
        *)                  continue ;;
    esac
    count=$((count + 1))
done < <(find "$package" -type f -not -path "*.mxo/*" -print0)

bundles=0
while IFS= read -r -d '' bundle; do
    sign "$bundle"
    codesign --verify --strict --verbose=2 "$bundle"
    bundles=$((bundles + 1))
done < <(find "$package" -type d -name "*.mxo" -print0)

echo "signed $count Mach-O files and $bundles bundles in $package as $identity"
