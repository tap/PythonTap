/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

// C-style trampolines registered on the Max class for the attributes and
// messages that are created dynamically from the user's Python class.
// Implementations are in tap.python_tilde.cpp.

c74::max::t_max_err python_attr_set(c74::max::t_object* x, c74::max::t_object* maxattr, const long argc,
                                    const c74::max::t_atom* argv);
c74::max::t_max_err python_attr_get(c74::max::t_object* x, c74::max::t_object* maxattr, long* argc,
                                    c74::max::t_atom** argv);

c74::max::t_max_err python_mess_int(c74::max::t_object* x, long value);
c74::max::t_max_err python_mess_float(c74::max::t_object* x, double value);
c74::max::t_max_err python_mess_symbol(c74::max::t_object* x, c74::max::t_symbol* value);
c74::max::t_max_err python_mess_bang(c74::max::t_object* x);
c74::max::t_max_err python_mess_gimme(c74::max::t_object* x, c74::max::t_symbol* name, long ac, c74::max::t_atom* av);
