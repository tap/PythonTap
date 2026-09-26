# License

The MIT License

Copyright 2022-2026 Timothy Place

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## Third-party software

This package builds on, and at runtime uses, the following third-party software. None of it is contained in this repository: the build pulls in the Min-API (with the Max SDK inside it) as a git submodule, and `scripts/install-runtime.*` installs the Python runtime and packages locally. A release package bundles all of it, and carries a copy of every one of these licenses in its `licenses/` folder (collected by `scripts/assemble-package.py` from exactly what the package contains, with an index in `licenses/README.md`).

- **Min-API** (git submodule at `source/min-api`, compiled into the external) — Copyright The Min-API Authors, MIT License. See `source/min-api/License.md`.
- **Max SDK** (inside the Min-API, at `source/min-api/max-sdk-base`, compiled into the external) — Copyright Cycling '74, MIT-style license. See `source/min-api/max-sdk-base/LICENSE.md`.
- **CPython** (the runtime in `support/`, built by the [python-build-standalone](https://github.com/astral-sh/python-build-standalone) project) — PSF License Agreement, plus the licenses of the components it bundles, all in its `LICENSE.txt` (`support/lib/python3.13/LICENSE.txt` on macOS, `support/LICENSE.txt` on Windows). python-build-standalone documents the licenses of the libraries it links in (OpenSSL, SQLite, and others) at https://gregoryszorc.com/docs/python-build-standalone/main/running.html#licensing.
- **attrs** (installed in the runtime) — MIT License.
- **NumPy** (installed in the runtime) — BSD 3-Clause License. Its wheels bundle OpenBLAS and LAPACK (BSD 3-Clause) and, where OpenBLAS needs it, the GCC runtime libraries (`libgfortran`, `libquadmath`: GPL 3 with the GCC Runtime Library Exception, and LGPL 2.1); their license texts ship inside the installed package, in `numpy-*.dist-info/licenses/`.
- **pip** (part of the runtime) — MIT License, with the licenses of the libraries it vendors in its `dist-info/licenses/`.

If you redistribute a package that bundles the `support/` runtime yourself, include those license texts too; `python3 scripts/assemble-package.py --licenses-only --output <folder>` collects them from the runtime you installed.
