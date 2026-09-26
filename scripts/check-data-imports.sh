#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
#
# Fail if a built module imports any *data* symbol (a variable, not a function) from libpython.
#
# The Windows external delay-loads python3xx.dll so that it loads without a runtime and can say what
# is missing (plan 4.2), and MSVC cannot delay-load a DLL whose data a module imports (LNK1194). So
# the core reaches Py_None, PyExc_* and the type objects through function calls only (see runtime.h).
# This check runs on the Linux build, where the same headers produce the same references, so a
# regression shows up here with the symbol's name instead of as a Windows link error.
#
# usage: scripts/check-data-imports.sh <module> <libpython.so>
set -euo pipefail

if [ $# -ne 2 ]; then
    echo "usage: $0 <module> <libpython.so>" >&2
    exit 2
fi
module="$1"
libpython="$2"

imports=$(nm -D --undefined-only "$module" | awk '{print $NF}' | sed 's/@.*//' | sort -u)
data=$(nm -D --defined-only "$libpython" | awk '$2 ~ /^[BDRV]$/ {print $3}' | sed 's/@.*//' | sort -u)
found=$(comm -12 <(echo "$imports") <(echo "$data"))

if [ -n "$found" ]; then
    echo "$module imports CPython data symbols, which prevents delay-loading python3xx.dll on Windows:" >&2
    echo "$found" | sed 's/^/  /' >&2
    echo "Reach them through function calls instead (see core/include/tap/python/runtime.h)." >&2
    exit 1
fi
count=$(echo "$imports" | grep -c '^_\?Py' || true)
echo "$module: no CPython data imports ($count CPython functions)"
