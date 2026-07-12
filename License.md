# License

The MIT License

Copyright 2022-2026 Timothy Place

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## Third-party software

This package builds on, and at runtime uses, the following third-party software. None of it is contained in this repository; the build pulls in the Min-API as a git submodule and `scripts/install-runtime.*` installs the Python runtime and packages locally.

- **Min-API** (git submodule at `source/min-api`) — Copyright Cycling '74, MIT License. See `source/min-api/License.md`.
- **CPython** (installed into `support/` by the install script, built by the [python-build-standalone](https://github.com/astral-sh/python-build-standalone) project) — PSF License Agreement. See `support/lib/python3.*/LICENSE.txt` after installation (`support/Lib/LICENSE.txt` on Windows), or https://docs.python.org/3/license.html. The runtime bundles OpenSSL and other components; python-build-standalone documents their licenses at https://gregoryszorc.com/docs/python-build-standalone/main/running.html#licensing.
- **attrs** (installed by the install script) — MIT License.
- **NumPy** (installed by the install script) — BSD 3-Clause License.

If you redistribute a built package that bundles the `support/` runtime (for example a release zip), you must include the corresponding license texts from the runtime you bundle.
