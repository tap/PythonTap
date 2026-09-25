/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.
///
/// Where this external lives: all runtime paths are relative to the Max package that contains the
/// external binary, never hardcoded. (The interpreter itself is the host-independent core in
/// core/include/tap/python/.)

#pragma once

#include <filesystem>

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

} // namespace tap::python
