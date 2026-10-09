/// @file tap.python_spike.cpp
/// @brief THROWAWAY (plan 9.0): a plain SDK class, tap.python, in tap.python~'s binary.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// The tap.python plan (docs/TAP-PYTHON-PLAN.md, D11 and 9.0) needs answers only Max can give before
// any of 9.1 onward is built. This file is the smallest second class that can ask them: a plain SDK
// class (class_new / class_addmethod / class_register — no min) registered by tap.python~'s own
// ext_main after min's wrap_as_max_external(), in the same binary. It has no Python in it. Built
// only with -DTAP_PYTHON_SPIKE=ON (off by default, never in a release); the patchers that drive it
// are in runtime-tests/spike/, and what they showed is written into the plan under 9.0.
//
// What it has, each for a question of 9.0:
//   - a dumpout outlet, made first (so it is the rightmost) and stored in the obex, and
//     object_obex_dumpout() registered as the class's A_CANT `dumpout` method, as the SDK's examples
//     do; value outlets made after it, from the last to the first (`[tap.python <count>]`, default
//     2). The same class without that method is registered as tap.python.spike.nodumpout (Q3);
//   - `outlets <n>`: changes the value outlets between the box's dynlet_begin and dynlet_end —
//     outlet_delete for the surplus, outlet_insert_after the last value outlet for new ones;
//     `describe` posts what Max then has (outlet_nth against the pointers held here, and every patch
//     cord from the box); `fire` outputs the outlet's number from each value outlet and `fired`
//     from the dumpout, right to left;
//   - one instance attribute, `steps` (attribute_new + object_addattr), whose getter and setter
//     post when Max calls them; and instance methods (object_addmethod): `hello` (A_GIMME), `int`
//     (A_LONG), and `who` and `bang` (which the class has too). `[tap.python <count> bare]` adds none
//     of them;
//   - a class attribute, `level`, for comparison; `getdump <name>` (object_attr_getdump(), the
//     SDK's "send its value from the dumpout" — documented as taking the attribute's name, it
//     wants the get message: 9.0) and `attrmethod <message>` (what object_attr_method() resolves
//     the message to);
//   - a class-level `anything` that posts what it received;
//   - `probe <selector> [<scripting name>]`: what object_getmethod() answers for the selector on
//     this object, or on the named box's object in the same patcher (tap.python~, say), compared
//     with what it answers for a name no class can have, with this class's anything, and with
//     method_false as this module sees it;
//   - `thread <label>`: which thread the message came on (systhread_ismainthread/isaudiothread/
//     istimerthread), noted on that thread and posted from the main thread;
//   - `outmsg <text>` / `outsym <text>`: a string output as the message it names, or as
//     `symbol <text>`, from the first value outlet (no argument: the empty string).
//
// And a third class, tap.python.spike.recorder: `[tap.python.spike.recorder <label>]` posts every
// message it receives — its label, the selector, and each atom with its type — because Max's log
// (what run_spike.py reads) drops a [print]'s name, and [print] does not show an atom's type.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <thread>

#include "c74_max.h"

using namespace c74::max;

namespace {

    constexpr long k_max_outlets{8};

    /// The object. object_alloc() gives zeroed memory and runs no constructor, so: plain data only.
    struct spike {
        t_object    ob;
        void*       obex;
        void*       dumpout;
        void*       value_outlets[k_max_outlets]; // left to right
        long        value_count;
        t_atom_long steps;
        t_atom_long level; // the class attribute
    };

    /// The recorder.
    struct recorder {
        t_object  ob;
        t_symbol* label;
    };

    t_class* s_class{};           // tap.python, with the dumpout method
    t_class* s_nodumpout_class{}; // tap.python.spike.nodumpout, the same without it
    t_class* s_recorder_class{};

    /// The arguments as one string, as Max would show them.
    std::string text_of(const long argc, t_atom* argv) {
        if (argc <= 0 || argv == nullptr) {
            return {};
        }
        long  size{};
        char* text{};
        if (atom_gettext(argc, argv, &size, &text, OBEX_UTIL_ATOM_GETTEXT_SYM_NO_QUOTE) != MAX_ERR_NONE || !text) {
            return "?";
        }
        std::string result{text};
        sysmem_freeptr(text);
        return result;
    }

    /// A string argument, as a class's Python method would return it: a number's text for a number,
    /// so that `outmsg 60` is the string "60"; the empty string without one.
    std::string string_argument(const long argc, t_atom* argv) {
        if (argc <= 0 || argv == nullptr) {
            return {};
        }
        char buffer[64];
        switch (atom_gettype(argv)) {
        case A_LONG:
            snprintf_zero(buffer, sizeof buffer, "%lld", static_cast<long long>(atom_getlong(argv)));
            return buffer;
        case A_FLOAT:
            snprintf_zero(buffer, sizeof buffer, "%g", atom_getfloat(argv));
            return buffer;
        default:
            return atom_getsym(argv)->s_name;
        }
    }

    // ---- the instance attribute and methods ------------------------------------------------

    t_max_err steps_get(spike* x, t_object* /*attr*/, long* argc, t_atom** argv) {
        char alloc{};
        if (atom_alloc(argc, argv, &alloc) != MAX_ERR_NONE) {
            return MAX_ERR_OUT_OF_MEM;
        }
        atom_setlong(*argv, x->steps);
        object_post(&x->ob, "spike: steps getter called (instance attribute) -> %lld",
                    static_cast<long long>(x->steps));
        return MAX_ERR_NONE;
    }

    t_max_err steps_set(spike* x, t_object* /*attr*/, const long argc, t_atom* argv) {
        if (argc > 0 && argv) {
            x->steps = atom_getlong(argv);
        }
        object_post(&x->ob, "spike: steps setter called (instance attribute) <- %s", text_of(argc, argv).c_str());
        return MAX_ERR_NONE;
    }

    void instance_hello(spike* x, t_symbol* s, const long argc, t_atom* argv) {
        object_post(&x->ob, "spike: instance method %s called (object_addmethod, A_GIMME): %s", s->s_name,
                    text_of(argc, argv).c_str());
    }

    void instance_int(spike* x, const t_atom_long value) {
        object_post(&x->ob, "spike: instance method int called (object_addmethod, A_LONG): %lld",
                    static_cast<long long>(value));
    }

    void instance_who(spike* x) {
        object_post(&x->ob, "spike: who: the INSTANCE method (object_addmethod) answered");
    }

    void class_who(spike* x) {
        object_post(&x->ob, "spike: who: the CLASS method (class_addmethod) answered");
    }

    void instance_bang(spike* x) {
        object_post(&x->ob, "spike: bang: the INSTANCE method (object_addmethod) answered");
    }

    void class_bang(spike* x) {
        object_post(&x->ob, "spike: bang: the CLASS method (class_addmethod) answered");
    }

    /// object_attr_getdump(): the SDK's way to have an attribute send its value from the dumpout.
    void spike_getdump(spike* x, t_symbol* attribute) {
        object_post(&x->ob, "spike: getdump %s: calling object_attr_getdump()", attribute->s_name);
        object_attr_getdump(x, attribute, 0, nullptr);
    }

    /// What object_attr_method() resolves a message to: an attribute's get or set function, or nothing.
    void spike_attrmethod(spike* x, t_symbol* message) {
        void*      attr{};
        long       get{};
        const auto found = object_attr_method(x, message, &attr, &get);
        object_post(&x->ob, "spike: attrmethod '%s': object_attr_method %s, attribute %s, get %ld%s", message->s_name,
                    found ? "found a method" : "found nothing", attr ? "found" : "none", get,
                    found == reinterpret_cast<method>(steps_get)   ? " (steps' own getter)"
                    : found == reinterpret_cast<method>(steps_set) ? " (steps' own setter)"
                                                                   : "");
    }

    // ---- the class -----------------------------------------------------------------------------

    void spike_anything(spike* x, t_symbol* s, const long argc, t_atom* argv) {
        object_post(&x->ob, "spike: class anything received: selector '%s', %ld argument(s): %s", s->s_name, argc,
                    text_of(argc, argv).c_str());
    }

    void spike_assist(spike* x, void* /*box*/, const long io, const long index, char* text) {
        if (io == ASSIST_INLET) {
            snprintf_zero(text, 256, "messages");
        }
        else if (index == x->value_count) {
            snprintf_zero(text, 256, "dumpout");
        }
        else {
            snprintf_zero(text, 256, "value outlet %ld", index);
        }
    }

    t_object* find_box(spike* x, t_symbol* varname) {
        t_object* patcher{};
        if (object_obex_lookup(x, gensym("#P"), &patcher) != MAX_ERR_NONE || !patcher) {
            return nullptr;
        }
        for (auto* box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box)) {
            if (jbox_get_varname(box) == varname) {
                return box;
            }
        }
        return nullptr;
    }

    std::string box_name(t_object* box) {
        auto* name = jbox_get_varname(box);
        if (name && name->s_name[0]) {
            return name->s_name;
        }
        return std::string{"["} + jbox_get_maxclass(box)->s_name + "]";
    }

    /// What Max now has: each outlet (as outlet_nth answers) against the pointers held here, the
    /// dumpout the obex holds, and every patch cord from this box.
    void spike_describe(spike* x) {
        const auto count = outlet_count(&x->ob);
        object_post(&x->ob, "spike: describe: outlet_count %ld (%ld value outlet(s) held + dumpout)", count,
                    x->value_count);
        for (long n = 0; n < count; ++n) {
            auto*       outlet = outlet_nth(&x->ob, n);
            std::string what   = "not one held here";
            if (outlet == x->dumpout) {
                what = "the dumpout";
            }
            for (long k = 0; k < x->value_count; ++k) {
                if (outlet == x->value_outlets[k]) {
                    what = "value outlet " + std::to_string(k);
                }
            }
            object_post(&x->ob, "spike: describe:   outlet_nth(%ld) is %s", n, what.c_str());
        }
        t_object* stored{};
        object_obex_lookup(x, gensym("dumpout"), &stored);
        object_post(&x->ob, "spike: describe: the obex's dumpout is %s",
                    stored == x->dumpout ? "the one made first" : "NOT the one made first");

        t_object* patcher{};
        t_object* box{};
        object_obex_lookup(x, gensym("#P"), &patcher);
        object_obex_lookup(x, gensym("#B"), &box);
        if (!patcher || !box) {
            object_post(&x->ob, "spike: describe: no patcher or box");
            return;
        }
        long cords{};
        for (auto* line = jpatcher_get_firstline(patcher); line; line = jpatchline_get_nextline(line)) {
            if (jpatchline_get_box1(line) == box) {
                ++cords;
                object_post(&x->ob, "spike: describe:   cord from outlet %ld to %s inlet %ld",
                            jpatchline_get_outletnum(line), box_name(jpatchline_get_box2(line)).c_str(),
                            jpatchline_get_inletnum(line));
            }
        }
        object_post(&x->ob, "spike: describe: %ld cord(s) from this box", cords);
    }

    /// The value outlets become `count`, in place, as a reload that changes a class's outlet count
    /// will: surplus ones deleted from the right, new ones inserted after the last value outlet —
    /// never appended, which would put them after the dumpout.
    void spike_outlets(spike* x, const t_atom_long requested) {
        const auto count = std::clamp<long>(static_cast<long>(requested), 1, k_max_outlets);
        t_object*  box{};
        if (object_obex_lookup(x, gensym("#B"), &box) != MAX_ERR_NONE || !box) {
            object_post(&x->ob, "spike: outlets: no box; outlets unchanged");
            return;
        }
        object_post(&x->ob, "spike: outlets: %ld -> %ld value outlet(s)", x->value_count, count);
        object_method(box, gensym("dynlet_begin"));
        while (x->value_count > count) {
            --x->value_count;
            outlet_delete(x->value_outlets[x->value_count]);
            x->value_outlets[x->value_count] = nullptr;
        }
        while (x->value_count < count) {
            auto* previous                   = x->value_outlets[x->value_count - 1];
            x->value_outlets[x->value_count] = outlet_insert_after(&x->ob, nullptr, nullptr, previous);
            ++x->value_count;
        }
        object_method(box, gensym("dynlet_end"));
    }

    /// Each value outlet outputs its number, then the dumpout `fired`, right to left as Max objects do.
    void spike_fire(spike* x) {
        object_post(&x->ob, "spike: fire");
        object_obex_dumpout(x, gensym("fired"), 0, nullptr);
        for (auto n = x->value_count - 1; n >= 0; --n) {
            outlet_int(static_cast<t_outlet*>(x->value_outlets[n]), n);
        }
    }

    void spike_probe(spike* x, t_symbol* /*s*/, const long argc, t_atom* argv) {
        if (argc < 1 || atom_gettype(argv) != A_SYM) {
            object_post(&x->ob, "spike: probe <selector> [<scripting name>]");
            return;
        }
        auto*       selector = atom_getsym(argv);
        t_object*   target   = &x->ob;
        std::string where    = std::string{"this "} + object_classname(&x->ob)->s_name;
        if (argc > 1) {
            auto* box = find_box(x, atom_getsym(argv + 1));
            if (!box) {
                object_post(&x->ob, "spike: probe: no box named %s", atom_getsym(argv + 1)->s_name);
                return;
            }
            target = jbox_get_object(box);
            where  = box_name(box) + " (" + object_classname(target)->s_name + ")";
        }
        const auto found     = object_getmethod(target, selector);
        const auto not_found = object_getmethod(target, gensym("tap.python spike: no class has this name"));
        const auto anything  = reinterpret_cast<method>(spike_anything);
        const auto falsefn   = reinterpret_cast<method>(method_false);
        object_post(&x->ob,
                    "spike: probe '%s' on %s: object_getmethod %s; %s what it answers for a name no class has "
                    "(which is %s, %s method_false as this module sees it); zgetfn %s",
                    selector->s_name, where.c_str(),
                    !found              ? "null"
                    : found == anything ? "is this class's anything"
                                        : "is a function",
                    found == not_found ? "SAME AS" : "differs from",
                    !not_found              ? "null"
                    : not_found == anything ? "this class's anything"
                                            : "a function",
                    not_found == falsefn ? "equal to" : "not equal to",
                    !zgetfn(target, selector) ? "null" : "non-null");
    }

    /// Noted on the thread the message came on, posted from the main thread (nothing is posted from
    /// the audio thread here).
    void thread_report(spike* x, t_symbol* /*s*/, const short argc, t_atom* argv) {
        if (argc < 5) {
            return;
        }
        object_post(&x->ob, "spike: thread '%s': main %lld, audio %lld, timer %lld, thread id %lld",
                    atom_getsym(argv)->s_name, static_cast<long long>(atom_getlong(argv + 1)),
                    static_cast<long long>(atom_getlong(argv + 2)), static_cast<long long>(atom_getlong(argv + 3)),
                    static_cast<long long>(atom_getlong(argv + 4)));
    }

    void spike_thread(spike* x, t_symbol* /*s*/, const long argc, t_atom* argv) {
        t_atom report[5];
        atom_setsym(report, argc > 0 ? atom_getsym(argv) : gensym("?"));
        atom_setlong(report + 1, systhread_ismainthread());
        atom_setlong(report + 2, systhread_isaudiothread());
        atom_setlong(report + 3, systhread_istimerthread());
        atom_setlong(report + 4,
                     static_cast<t_atom_long>(std::hash<std::thread::id>{}(std::this_thread::get_id()) % 1000000));
        defer_low(x, reinterpret_cast<method>(thread_report), nullptr, 5, report);
    }

    /// A string result output as the message it names, with no arguments.
    void spike_outmsg(spike* x, t_symbol* /*s*/, const long argc, t_atom* argv) {
        const auto text = string_argument(argc, argv);
        object_post(&x->ob, "spike: q6 ---- as a message: '%s'", text.c_str());
        outlet_anything(static_cast<t_outlet*>(x->value_outlets[0]), gensym(text.c_str()), 0, nullptr);
    }

    /// A string result output as `symbol <text>`.
    void spike_outsym(spike* x, t_symbol* /*s*/, const long argc, t_atom* argv) {
        const auto text = string_argument(argc, argv);
        object_post(&x->ob, "spike: q6 ---- as symbol: '%s'", text.c_str());
        t_atom atom;
        atom_setsym(&atom, gensym(text.c_str()));
        outlet_anything(static_cast<t_outlet*>(x->value_outlets[0]), gensym("symbol"), 1, &atom);
    }

    void* spike_new(t_symbol* s, const long argc, t_atom* argv) {
        const bool nodumpout = s == gensym("tap.python.spike.nodumpout");
        auto*      x         = static_cast<spike*>(object_alloc(nodumpout ? s_nodumpout_class : s_class));
        if (!x) {
            return nullptr;
        }
        const auto offset = attr_args_offset(static_cast<short>(argc), argv);
        long       count  = 2;
        if (offset > 0 && atom_gettype(argv) == A_LONG) {
            count = std::clamp<long>(static_cast<long>(atom_getlong(argv)), 1, k_max_outlets);
        }
        const bool bare = offset > 1 && atom_gettype(argv + 1) == A_SYM && atom_getsym(argv + 1) == gensym("bare");

        // Max orders outlets by creation, right to left: the dumpout first, then the value outlets
        // from the last to the first.
        x->dumpout = outlet_new(x, nullptr);
        object_obex_store(x, gensym("dumpout"), static_cast<t_object*>(x->dumpout));
        for (auto n = count - 1; n >= 0; --n) {
            x->value_outlets[n] = outlet_new(x, nullptr);
        }
        x->value_count = count;

        x->steps = 8;
        x->level = 1;
        if (!bare) {
            auto* attr = attribute_new("steps", gensym("long"), 0, reinterpret_cast<method>(steps_get),
                                       reinterpret_cast<method>(steps_set));
            if (object_addattr(x, attr) != MAX_ERR_NONE) {
                object_error(&x->ob, "spike: object_addattr failed");
            }
            if (object_addmethod(&x->ob, reinterpret_cast<method>(instance_hello), "hello", A_GIMME, 0) != MAX_ERR_NONE
                || object_addmethod(&x->ob, reinterpret_cast<method>(instance_int), "int", A_LONG, 0) != MAX_ERR_NONE
                || object_addmethod(&x->ob, reinterpret_cast<method>(instance_who), "who", 0) != MAX_ERR_NONE
                || object_addmethod(&x->ob, reinterpret_cast<method>(instance_bang), "bang", 0) != MAX_ERR_NONE) {
                object_error(&x->ob, "spike: object_addmethod failed");
            }
        }
        attr_args_process(x, static_cast<short>(argc), argv);

        object_post(&x->ob, "spike: new %s%s: %ld value outlet(s) + dumpout, steps %lld; main thread %d", s->s_name,
                    bare ? " (bare: no instance attribute or methods)" : "", count, static_cast<long long>(x->steps),
                    systhread_ismainthread());
        return x;
    }

    void spike_free(spike* /*x*/) {} // Max frees the outlets and the instance attribute

    // ---- the recorder ----------------------------------------------------------------------------

    /// One atom with its type: 60 (int), 1.5 (float), C (symbol), "" (symbol) for the empty one.
    std::string typed(t_atom* atom) {
        char buffer[64];
        switch (atom_gettype(atom)) {
        case A_LONG:
            snprintf_zero(buffer, sizeof buffer, "%lld (int)", static_cast<long long>(atom_getlong(atom)));
            return buffer;
        case A_FLOAT:
            snprintf_zero(buffer, sizeof buffer, "%g (float)", atom_getfloat(atom));
            return buffer;
        case A_SYM:
            return std::string{"'"} + atom_getsym(atom)->s_name + "' (symbol)";
        default:
            return "(another type)";
        }
    }

    void record(recorder* x, const char* selector, const long argc, t_atom* argv) {
        std::string atoms;
        for (long n = 0; n < argc; ++n) {
            atoms += (n ? ", " : "") + typed(argv + n);
        }
        object_post(&x->ob, "recorder %s: selector '%s', %ld atom(s)%s%s", x->label->s_name, selector, argc,
                    argc ? ": " : "", atoms.c_str());
    }

    void recorder_bang(recorder* x) {
        record(x, "bang", 0, nullptr);
    }

    void recorder_int(recorder* x, const t_atom_long value) {
        t_atom atom;
        atom_setlong(&atom, value);
        record(x, "int", 1, &atom);
    }

    void recorder_float(recorder* x, const double value) {
        t_atom atom;
        atom_setfloat(&atom, value);
        record(x, "float", 1, &atom);
    }

    void recorder_anything(recorder* x, t_symbol* s, const long argc, t_atom* argv) {
        record(x, s->s_name, argc, argv);
    }

    void* recorder_new(t_symbol* /*s*/, const long argc, t_atom* argv) {
        auto* x = static_cast<recorder*>(object_alloc(s_recorder_class));
        if (x) {
            x->label = argc > 0 && atom_gettype(argv) == A_SYM ? atom_getsym(argv) : gensym("?");
        }
        return x;
    }

} // namespace

namespace {

    /// The spike class, registered under `name`. With `dumpout`, it registers object_obex_dumpout() as its
    /// A_CANT dumpout method, as the SDK's own examples do beside storing the outlet in the obex
    /// (dbviewer.c, dict.edit.cpp); the 9.0 spike found that get<attr> needs it (Q3).
    t_class* spike_class(const char* name, const bool dumpout) {
        auto* c = class_new(name, reinterpret_cast<method>(spike_new), reinterpret_cast<method>(spike_free),
                            sizeof(spike), nullptr, A_GIMME, 0);
        if (dumpout) {
            class_addmethod(c, reinterpret_cast<method>(object_obex_dumpout), "dumpout", A_CANT, 0);
        }
        class_addmethod(c, reinterpret_cast<method>(spike_assist), "assist", A_CANT, 0);
        class_addmethod(c, reinterpret_cast<method>(spike_outlets), "outlets", A_LONG, 0);
        class_addmethod(c, reinterpret_cast<method>(spike_fire), "fire", 0);
        class_addmethod(c, reinterpret_cast<method>(spike_describe), "describe", 0);
        class_addmethod(c, reinterpret_cast<method>(spike_probe), "probe", A_GIMME, 0);
        class_addmethod(c, reinterpret_cast<method>(spike_thread), "thread", A_GIMME, 0);
        class_addmethod(c, reinterpret_cast<method>(spike_outmsg), "outmsg", A_GIMME, 0);
        class_addmethod(c, reinterpret_cast<method>(spike_outsym), "outsym", A_GIMME, 0);
        class_addmethod(c, reinterpret_cast<method>(spike_getdump), "getdump", A_SYM, 0);
        class_addmethod(c, reinterpret_cast<method>(spike_attrmethod), "attrmethod", A_SYM, 0);
        class_addmethod(c, reinterpret_cast<method>(class_who), "who", 0);
        class_addmethod(c, reinterpret_cast<method>(class_bang), "bang", 0);
        class_addmethod(c, reinterpret_cast<method>(spike_anything), "anything", A_GIMME, 0);
        class_addattr(c, attr_offset_new("level", gensym("long"), 0, nullptr, nullptr, calcoffset(spike, level)));
        class_obexoffset_set(c, calcoffset(spike, obex));
        class_register(gensym("box"), c);
        return c;
    }

} // namespace

/// Called by tap.python~'s ext_main, after min has registered tap.python~ (tap.python_tilde.cpp).
void tap_python_spike_register() {
    if (s_class) {
        return;
    }
    s_class           = spike_class("tap.python", true);
    s_nodumpout_class = spike_class("tap.python.spike.nodumpout", false);

    auto* r = class_new("tap.python.spike.recorder", reinterpret_cast<method>(recorder_new), nullptr, sizeof(recorder),
                        nullptr, A_GIMME, 0);
    class_addmethod(r, reinterpret_cast<method>(recorder_bang), "bang", 0);
    class_addmethod(r, reinterpret_cast<method>(recorder_int), "int", A_LONG, 0);
    class_addmethod(r, reinterpret_cast<method>(recorder_float), "float", A_FLOAT, 0);
    class_addmethod(r, reinterpret_cast<method>(recorder_anything), "list", A_GIMME, 0);
    class_addmethod(r, reinterpret_cast<method>(recorder_anything), "anything", A_GIMME, 0);
    class_register(gensym("box"), r);
    s_recorder_class = r;
    post("spike: tap.python registered by tap.python~'s ext_main, after tap.python~ (plan 9.0 spike build)");
}
