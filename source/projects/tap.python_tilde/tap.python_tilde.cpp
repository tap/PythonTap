/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#include "tap.python_tilde.h"

// The trampolines Max calls for the attributes and messages made for a Python class are
// python_glue<python>'s (tap.python_glue.h), instantiated by the object's members.

// The binary registers two classes (docs/TAP-PYTHON-PLAN.md, D11): tap.python~ (min's), then
// tap.python, a plain SDK class (tap.python.cpp), which Max finds here through the package's
// init/tap.python.txt.
void tap_python_register();

void ext_main(void* r) {
    c74::min::wrap_as_max_external<python>("python", __FILE__, r);
    tap_python_register();
}
