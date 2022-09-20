/// @file
/// @copyright  Copyright 2022 Timothy Place. All rights reserved.
/// @license           Use of this source code is governed by the MIT License found in the License.md file.

#pragma once


c74::max::t_max_err python_attr_set(c74::max::t_object* x, c74::max::t_object* maxattr, const long argc, const c74::max::t_atom* argv);
c74::max::t_max_err python_attr_get(c74::max::t_object* x, c74::max::t_object* maxattr, long* argc, c74::max::t_atom** argv);

c74::max::t_max_err python_mess_int(c74::max::t_object* x, long value);
c74::max::t_max_err python_mess_float(c74::max::t_object* x, double value);
c74::max::t_max_err python_mess_symbol(c74::max::t_object* x, c74::max::t_symbol* value);
c74::max::t_max_err python_mess_bang(c74::max::t_object* x);
c74::max::t_max_err python_mess_clear(c74::max::t_object* x);
c74::max::t_max_err python_mess_gimme(c74::max::t_object* x, c74::max::t_symbol* name, long ac, c74::max::t_atom* av);

