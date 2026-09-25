/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#include "tap.python_tilde.h"

using namespace c74::min;
using namespace c74::max;

namespace {

    /// Max calls the trampolines below from C, so no C++ exception may cross back into it.
    template <typename Fn>
    c74::max::t_max_err guarded(c74::max::t_object* x, Fn&& fn) noexcept {
        try {
            fn();
        }
        catch (const std::exception& e) {
            c74::max::object_error(x, "tap.python~: %s", e.what());
        }
        catch (...) {
            c74::max::object_error(x, "tap.python~: unknown error");
        }
        return c74::max::MAX_ERR_NONE;
    }

} // namespace

c74::max::t_max_err python_attr_set(c74::max::t_object* x, c74::max::t_object* maxattr, const long argc,
                                    const c74::max::t_atom* argv) {
    return guarded(x, [&] {
        auto attr_name{static_cast<const c74::max::t_symbol*>(c74::max::object_method(maxattr, k_sym_getname))};
        auto self = &(wrapper_find_self<python>(x))->m_min_object;
        self->attr_set(attr_name, argc, argv);
    });
}

c74::max::t_max_err python_attr_get(c74::max::t_object* x, c74::max::t_object* maxattr, long* argc,
                                    c74::max::t_atom** argv) {
    return guarded(x, [&] {
        auto attr_name{static_cast<const c74::max::t_symbol*>(c74::max::object_method(maxattr, k_sym_getname))};
        auto self = &(wrapper_find_self<python>(x))->m_min_object;
        self->attr_get(attr_name, argc, argv);
    });
}

c74::max::t_max_err python_mess_int(c74::max::t_object* x, long value) {
    return guarded(x, [&] {
        auto             self = &(wrapper_find_self<python>(x))->m_min_object;
        c74::max::t_atom a;
        c74::max::atom_setlong(&a, value);
        self->message_gimme(gensym("int"), 1, &a);
    });
}

c74::max::t_max_err python_mess_float(c74::max::t_object* x, double value) {
    return guarded(x, [&] {
        auto             self = &(wrapper_find_self<python>(x))->m_min_object;
        c74::max::t_atom a;
        c74::max::atom_setfloat(&a, value);
        self->message_gimme(k_sym_float, 1, &a);
    });
}

c74::max::t_max_err python_mess_symbol(c74::max::t_object* x, c74::max::t_symbol* value) {
    return guarded(x, [&] {
        auto             self = &(wrapper_find_self<python>(x))->m_min_object;
        c74::max::t_atom a;
        c74::max::atom_setsym(&a, value);
        self->message_gimme(gensym("symbol"), 1, &a);
    });
}

c74::max::t_max_err python_mess_bang(c74::max::t_object* x) {
    return guarded(x, [&] {
        auto self = &(wrapper_find_self<python>(x))->m_min_object;
        self->message_gimme(k_sym_bang, 0, nullptr);
    });
}

c74::max::t_max_err python_mess_gimme(c74::max::t_object* x, c74::max::t_symbol* name, long ac, c74::max::t_atom* av) {
    return guarded(x, [&] {
        auto self = &(wrapper_find_self<python>(x))->m_min_object;
        self->message_gimme(name, ac, av);
    });
}

MIN_EXTERNAL(python);
