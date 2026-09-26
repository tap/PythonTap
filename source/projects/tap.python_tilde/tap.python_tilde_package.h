/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.
///
/// Where this external lives: all runtime paths are relative to the Max package that contains the
/// external binary, never hardcoded. (The interpreter itself is the host-independent core in
/// core/include/tap/python/.)

#pragma once

// CPython (named by the runtime check below) must precede the standard headers
#include "tap/python/runtime.h"

// standard library
#include <filesystem>
#include <string>

#ifdef WIN_VERSION
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace tap::python {

    /// Full path of this external's binary on disk, found from the address of one of
    /// our own functions. This works no matter where the user installed the package.
    inline std::filesystem::path external_binary_path() {
#ifdef WIN_VERSION
        HMODULE module{};
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&external_binary_path), &module);
        wchar_t buffer[4096]{};
        GetModuleFileNameW(module, buffer, 4096);
        return std::filesystem::path{buffer};
#else
        Dl_info info{};
        dladdr(reinterpret_cast<void*>(&external_binary_path), &info);
        return std::filesystem::path{info.dli_fname};
#endif
    }

    /// Root of the Max package containing this external.
    /// mac:  <package>/externals/tap.python~.mxo/Contents/MacOS/tap.python~
    /// win:  <package>/externals/tap.python~.mxe64
    /// test: <package>/tests/tap.python_tilde_test (min-object-unittest.cmake's output folder),
    ///       on every platform
    inline std::filesystem::path package_root() {
        // the loader may report a relative path (e.g. a binary launched as ./name)
        std::error_code ec;
        auto            binary = std::filesystem::absolute(external_binary_path(), ec);
        if (ec) {
            binary = external_binary_path();
        }
#if defined(MIN_TEST)
        return binary.parent_path().parent_path();
#elif defined(MAC_VERSION)
        return binary.parent_path().parent_path().parent_path().parent_path().parent_path();
#else
        return binary.parent_path().parent_path();
#endif
    }

    /// Whether the CPython library the external was built against can be used, asked before the
    /// first Python call. The external links it weakly on macOS and delay-loads it on Windows (see
    /// this object's CMakeLists.txt), so that it loads without a runtime and can say what is
    /// missing — instead of Max refusing to load it at all. On failure `error` says what to do.
    inline bool runtime_library_loadable([[maybe_unused]] const std::filesystem::path& home,
                                         [[maybe_unused]] std::string&                 error) {
#if defined(WIN_VERSION) && defined(TAP_PYTHON_DLL)
        // Load it from the package by full path (its own dependencies from its folder); the
        // delay-load helper then finds the loaded module by name on the first Python call. A runtime
        // installed while Max is running is picked up by the next object created.
        const auto dll = home / TAP_PYTHON_DLL;
        if (LoadLibraryExW(dll.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH) == nullptr) {
            error = "could not load " + dll.string() + " (Windows error " + std::to_string(GetLastError())
                    + ") — run scripts/install-runtime.ps1 from the package root to install the runtime";
            return false;
        }
#elif defined(MAC_VERSION)
        // A weakly linked library that was missing when the external loaded leaves our imports null,
        // and dyld never binds them later, so test the binding itself rather than the file. (Read
        // through a volatile, so the compiler cannot assume a function's address is non-null.)
        auto* volatile entry = &Py_InitializeFromConfig;
        if (entry == nullptr) {
            error = "the Python runtime was not found in " + (home / "lib").string()
                    + " when Max loaded tap.python~ — run scripts/install-runtime.sh from the package root, "
                      "then restart Max";
            return false;
        }
#endif
        return true;
    }

} // namespace tap::python
