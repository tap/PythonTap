/// @file tap.python.cpp
/// @brief tap.python's Max class: registered by tap.python~'s ext_main, after min's (plan 9.3).
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// A plain SDK class (D11): class_new / class_addmethod / class_register in the binary min's
// tap.python~ lives in, which Max loads for [tap.python] through the package's init/tap.python.txt
// (`max objectfile tap.python tap.python~;`). Every function here is a C entry point Max calls:
// each finds the object (control_object, tap.python.h) and lets no C++ exception back into Max.

#include "tap.python.h"

#include <cstddef>
#include <exception>

namespace {

    using tap::python::control_object;
    using tap::python::guarded;
    using tap::python::t_tap_python;

    c74::max::t_class* s_class{};

    c74::max::t_object* as_object(t_tap_python* x) {
        return &x->header;
    }

    void tap_python_free(t_tap_python* x) {
        delete x->object;
        x->object = nullptr;
    }

    void tap_python_assist(t_tap_python* x, void* /*box*/, const long io, const long index, char* text) {
        guarded(as_object(x), control_object::k_max_name, [&] { x->object->assist(io, index, text); });
    }

    // Typed, as the file watcher sends it (tap.python_filewatch.h), and as a patcher may to force a
    // reload.
    void tap_python_filechanged(t_tap_python* x, c74::max::t_symbol* /*s*/, long /*ac*/, c74::max::t_atom* /*av*/) {
        guarded(as_object(x), control_object::k_max_name, [&] { x->object->update_source(); });
    }

    // The forwarders (tap.python.h, control_object::forward()).
    void tap_python_anything(t_tap_python* x, c74::max::t_symbol* s, const long ac, c74::max::t_atom* av) {
        guarded(as_object(x), control_object::k_max_name, [&] { x->object->forward(s, ac, av); });
    }

    void tap_python_list(t_tap_python* x, c74::max::t_symbol* /*s*/, const long ac, c74::max::t_atom* av) {
        guarded(as_object(x), control_object::k_max_name,
                [&] { x->object->forward(c74::max::gensym("list"), ac, av); });
    }

    void tap_python_int(t_tap_python* x, const c74::max::t_atom_long value) {
        guarded(as_object(x), control_object::k_max_name, [&] {
            c74::max::t_atom a;
            c74::max::atom_setlong(&a, value);
            x->object->forward(c74::max::gensym("int"), 1, &a);
        });
    }

    void tap_python_float(t_tap_python* x, const double value) {
        guarded(as_object(x), control_object::k_max_name, [&] {
            c74::max::t_atom a;
            c74::max::atom_setfloat(&a, value);
            x->object->forward(c74::max::gensym("float"), 1, &a);
        });
    }

    void tap_python_bang(t_tap_python* x) {
        guarded(as_object(x), control_object::k_max_name,
                [&] { x->object->forward(c74::max::gensym("bang"), 0, nullptr); });
    }

    template <typename Function>
    c74::max::method as_method(Function function) {
        return reinterpret_cast<c74::max::method>(function);
    }

} // namespace

void* tap_python_new(c74::max::t_symbol* /*s*/, const long argc, c74::max::t_atom* argv) {
    auto* x = static_cast<t_tap_python*>(c74::max::object_alloc(s_class));
    if (!x) {
        return nullptr;
    }
    x->object = nullptr;
    try {
        x->object = new control_object(as_object(x), argc, argv);
    }
    catch (const std::exception& e) {
        c74::max::object_error(as_object(x), "%s", e.what());
    }
    if (!x->object) {
        c74::max::object_free(x);
        return nullptr;
    }
    // @attribute arguments, now that the class's fields are attributes
    c74::max::attr_args_process(x, static_cast<short>(argc), argv);
    return x;
}

void tap_python_register() {
    if (s_class) {
        return;
    }
    auto* c = c74::max::class_new("tap.python", as_method(tap_python_new), as_method(tap_python_free),
                                  sizeof(t_tap_python), nullptr, c74::max::A_GIMME, 0);
    // get<attribute> reaches the dumpout only through this method (plan 9.0, 3)
    c74::max::class_addmethod(c, as_method(c74::max::object_obex_dumpout), "dumpout", c74::max::A_CANT, 0);
    c74::max::class_addmethod(c, as_method(tap_python_assist), "assist", c74::max::A_CANT, 0);
    c74::max::class_addmethod(c, as_method(tap_python_filechanged), "filechanged", c74::max::A_GIMME, 0);
    c74::max::class_addmethod(c, as_method(tap_python_int), "int", c74::max::A_LONG, 0);
    c74::max::class_addmethod(c, as_method(tap_python_float), "float", c74::max::A_FLOAT, 0);
    c74::max::class_addmethod(c, as_method(tap_python_bang), "bang", 0);
    c74::max::class_addmethod(c, as_method(tap_python_list), "list", c74::max::A_GIMME, 0);
    c74::max::class_addmethod(c, as_method(tap_python_anything), "anything", c74::max::A_GIMME, 0);
    c74::max::class_obexoffset_set(c, offsetof(t_tap_python, obex));
    c74::max::class_register(c74::max::gensym("box"), c);
    s_class = c;
}
