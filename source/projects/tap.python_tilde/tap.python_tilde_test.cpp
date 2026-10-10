/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "c74_min_unittest.h" // required unit-test header (defines main via Catch)

// The mock kernel does not implement these Max functions, which the object
// references for its dynamically generated attributes/messages and its file
// watcher, or implements one too thinly (attr_args_offset). Provide stubs so the
// test binary links (the headers declare them with C linkage). With these stubs the Max-side attribute and method
// registrations fail harmlessly, so the tests below drive the object's own
// attribute and message handlers directly, as Max's dispatch would.
// What Max does with an attribute the object adds, which the stubs below model only while a
// scenario asks (the test process makes many objects in turn, and the others rely on the
// registration failing quietly): the object then answers the attribute's name, so
// object_getmethod() finds it. 1.0.1's guard took the previous load's attributes for Max's own
// and reserved every field on a reload (found in the Mac session, plan 8.8).
namespace attributes {
    bool                            modeled{};
    std::deque<std::string>         made;  // attribute_new's, by address (a deque keeps them in place)
    std::unordered_set<std::string> added; // object_addattr's, until object_deleteattr

    void model(const bool on) {
        modeled = on;
        made.clear();
        added.clear();
    }
} // namespace attributes

// What the objects say in the Max console (object_post, object_warn, object_error), recorded while a
// scenario listens: the mock kernel prints them to its own std::cout and std::cerr, which a test
// cannot redirect on Windows, where the kernel is a DLL. Printed as the mock prints them, too.
namespace console {
    std::mutex  lock;
    std::string said;
    bool        listening{};

    void say(std::ostream& stream, const char* format, va_list arguments) {
        char text[4096];
        std::vsnprintf(text, sizeof text, format, arguments);
        stream << text;
        const std::lock_guard<std::mutex> guard{lock};
        if (listening) {
            said += text;
            said += '\n';
        }
    }
} // namespace console

// How many times a qelem was set (tap.python's notice from the audio thread sets one).
namespace qelems {
    std::atomic<int> set{};
} // namespace qelems

namespace c74 {
    namespace max {
        extern "C" {
        t_object* attribute_new(const char* name, t_symbol*, long, method, method) {
            if (!attributes::modeled) {
                return nullptr;
            }
            attributes::made.emplace_back(name);
            return reinterpret_cast<t_object*>(&attributes::made.back());
        }
        t_max_err object_addattr(void*, t_object* attribute) {
            if (!attributes::modeled || attribute == nullptr) {
                return MAX_ERR_GENERIC;
            }
            attributes::added.insert(*reinterpret_cast<std::string*>(attribute));
            return MAX_ERR_NONE;
        }
        t_max_err object_attr_addattr_parse(t_object*, const char*, const char*, t_symbol*, long, const char*) {
            return attributes::modeled ? MAX_ERR_NONE : MAX_ERR_GENERIC;
        }
        t_max_err object_addmethod(t_object*, method, const char*, ...) {
            return MAX_ERR_NONE;
        }
        t_max_err object_deletemethod(t_object*, t_symbol*) {
            return MAX_ERR_NONE;
        }
        void* filewatcher_new(t_object*, const short, const char*) {
            return nullptr;
        }
        t_max_err object_method_typed(void*, t_symbol*, long, t_atom*, t_atom*) {
            return MAX_ERR_NONE;
        }
        // The mock's says 0 — no arguments at all — so an object could not be given its class file.
        // As the SDK documents it: how many atoms come before the first @attribute.
        long attr_args_offset(short ac, t_atom* av) {
            for (short i = 0; i < ac; ++i) {
                if (atom_gettype(av + i) == A_SYM && atom_getsym(av + i)->s_name[0] == '@') {
                    return i;
                }
            }
            return ac;
        }
        void filewatcher_start(void*) {}
        // What Max's own does, which the mock's (always null) does not: method_false() for a name
        // the object does not answer. 1.0.0 took that for "found" and reserved every Python name
        // (plan 8.2's guard, found broken in Max; fixed in 1.0.1). The pretend class answers the
        // names min registers for this object, plus one a test's class defines on purpose.
        t_atom_long method_false(void*) {
            return 0;
        }
        // What the pretend Max answers for a name the object does not have: its own method_false(),
        // which is not the method_false whose address this module takes. On Windows that address is
        // the external's import thunk (the SDK declares method_false without dllimport), so 1.0.1,
        // comparing with it, took every name for found and reserved it (found on Windows). The body
        // is its own, so that no linker folds it into method_false.
        t_atom_long max_method_false(void*) {
            static volatile int s_calls;
            s_calls = s_calls + 1;
            return 0;
        }
        // The method the pretend class answers with. Its body must be its own: a linker that folds
        // identical functions (MSVC's /OPT:ICF, on by default in Release; lld's --icf=all) gave a
        // `return 0` here method_false's address, so every name read as unanswered on Windows.
        void* answered_method(void*) {
            static int s_marker;
            return &s_marker;
        }
        method object_getmethod(void*, t_symbol* s) {
            static const char* const k_answered[] = {"dsp64",       "assist",   "notify",
                                                     "filechanged", "dspsetup", "maxtest_host_answers"};
            for (const auto* name : k_answered) {
                if (s == gensym(name)) {
                    return reinterpret_cast<method>(answered_method);
                }
            }
            if (attributes::added.count(s->s_name) != 0) { // an attribute the object added answers its name
                return reinterpret_cast<method>(answered_method);
            }
            return reinterpret_cast<method>(max_method_false);
        }
        void* qelem_new(void*, method) {
            static int s_qelem;
            return &s_qelem;
        }
        void qelem_set(void*) {
            ++qelems::set;
        }
        t_max_err object_deleteattr(void*, t_symbol* name) {
            attributes::added.erase(name->s_name);
            return MAX_ERR_NONE;
        }
        void qelem_free(void*) {}
        }
    } // namespace max
} // namespace c74

// Max's dynamic inlets and outlets, which the mock kernel lacks: every object is in a pretend box,
// and what the object asks of it is recorded, so the tests can check what it does and that min's
// own lists follow.
namespace dynlets {
    int                      box{};           // the pretend box (its address)
    long                     signal_inlets{}; // the last dsp_resize, 0 if none
    long                     appended{};      // outlets appended
    std::vector<long>        deleted;         // the indices of the outlets deleted, in order (by outlet_nth)
    std::vector<std::string> calls;           // what was asked of the box and of the outlets, in order
    bool                     chain_broken{};

    void reset() {
        signal_inlets = 0;
        appended      = 0;
        deleted.clear();
        calls.clear();
        chain_broken = false;
    }

    std::string id(const void* outlet) {
        return std::to_string(reinterpret_cast<std::intptr_t>(outlet));
    }
} // namespace dynlets

// What an object stores in its obex (tap.python's dumpout), which the mock kernel does not keep.
namespace obex {
    std::map<std::pair<void*, c74::max::t_symbol*>, c74::max::t_object*> stored;
} // namespace obex

// Which thread a message comes on: the audio thread only while a scenario says so (Scheduler in
// Audio Interrupt, plan 9.0).
namespace audio_thread {
    std::atomic<bool> now{};
} // namespace audio_thread

// The outlets the mock kernel made for each object, newest first (its order), with the ones
// tap.python inserted on a reload made by the same outlet_new: so an outlet's messages are found by
// what Max gave the object, whatever the mock's order (see outlet_insert_after below).
namespace outlets {
    std::map<void*, std::vector<void*>> inserted; // per object, in order

    /// The mock's index of `outlet` among `ids`, every outlet the object was given: the mock keeps
    /// them newest first, and its ids grow with each outlet made.
    int index_of(const std::vector<void*>& ids, const void* outlet) {
        int index = 0;
        for (const auto* other : ids) {
            index += reinterpret_cast<std::intptr_t>(other) > reinterpret_cast<std::intptr_t>(outlet) ? 1 : 0;
        }
        return index;
    }
} // namespace outlets

namespace c74 {
    namespace max {
        extern "C" {
        t_max_err object_obex_lookup(void* x, t_symbol* key, t_object** val) {
            if (key == gensym("#B")) {
                *val = reinterpret_cast<t_object*>(&dynlets::box);
                return MAX_ERR_NONE;
            }
            const auto found = obex::stored.find({x, key});
            if (found == obex::stored.end()) {
                return MAX_ERR_GENERIC;
            }
            *val = found->second;
            return MAX_ERR_NONE;
        }
        t_max_err object_obex_store(void* x, t_symbol* key, t_object* val) {
            obex::stored[{x, key}] = val;
            return MAX_ERR_NONE;
        }
        // As Max's: the message from the outlet the object stored as its dumpout.
        void object_obex_dumpout(void* x, t_symbol* s, long argc, t_atom* argv) {
            const auto found = obex::stored.find({x, gensym("dumpout")});
            if (found != obex::stored.end()) {
                outlet_anything(found->second, s, static_cast<short>(argc), argv);
            }
        }
        void class_obexoffset_set(t_class*, long) {}
        // The SDK's object_method() is this on 64-bit; the mock's answers nothing. The pretend box
        // records what it is asked (dynlet_begin, dynlet_end), and every other receiver answers
        // nothing, as the mock's.
        void* object_method_imp(void* x, void* sym, void*, void*, void*, void*, void*, void*, void*, void*) {
            if (x == &dynlets::box) {
                dynlets::calls.emplace_back(static_cast<t_symbol*>(sym)->s_name);
            }
            return nullptr;
        }
        void dsp_resize(t_pxobject*, long nsignals) {
            dynlets::signal_inlets = nsignals;
        }
        void* outlet_append(t_object*, t_symbol*, t_symbol*) {
            ++dynlets::appended;
            return nullptr;
        }
        void* outlet_nth(t_object*, long n) {
            return reinterpret_cast<void*>(static_cast<std::intptr_t>(n + 1)); // never null
        }
        void outlet_delete(void* x) {
            dynlets::deleted.push_back(static_cast<long>(reinterpret_cast<std::intptr_t>(x) - 1));
            dynlets::calls.push_back("delete " + dynlets::id(x));
        }
        // Max's inserts the outlet after `previous`; the mock kernel has no insertion, so the outlet
        // is made by its outlet_new (newest first, the mock's order), and the call recorded: what the
        // object asked for is what the tests check, and the new outlet carries messages like any.
        void* outlet_insert_after(t_object* x, t_symbol*, t_symbol*, void* previous) {
            void* outlet = outlet_new(x, nullptr);
            outlets::inserted[x].push_back(outlet);
            dynlets::calls.push_back("insert after " + dynlets::id(previous));
            return outlet;
        }
        short systhread_isaudiothread() {
            return audio_thread::now.load() ? 1 : 0;
        }
        // in place of the mock kernel's, which print the same: so that a test can hear them
        void object_post(t_object*, const char* format, ...) {
            va_list arguments;
            va_start(arguments, format);
            console::say(std::cout, format, arguments);
            va_end(arguments);
        }
        void object_warn(t_object*, const char* format, ...) {
            va_list arguments;
            va_start(arguments, format);
            console::say(std::cout, format, arguments);
            va_end(arguments);
        }
        void object_error(t_object*, const char* format, ...) {
            va_list arguments;
            va_start(arguments, format);
            console::say(std::cerr, format, arguments);
            va_end(arguments);
        }
        t_dspchain* dspchain_fromobject(t_object*) {
            static int s_chain;
            return reinterpret_cast<t_dspchain*>(&s_chain);
        }
        void dspchain_setbroken(t_dspchain*) {
            dynlets::chain_broken = true;
        }
        }
    } // namespace max
} // namespace c74

#include "tap.python_tilde.cpp" // include the object source so we can instantiate it

// after it, whose c74_min.h must come first: tap.python, whose class is tap.python.cpp, compiled
// beside this test
#include "tap.python.h"

// The Max glue, end to end through the mock kernel. The test binary lands in <package>/tests/,
// and package_root() resolves the package from there, so the object starts the real embedded
// interpreter (the support/ runtime on macOS and Windows, a system CPython 3.13 on Linux) and
// loads python/default.py. A build always has a runtime (configuring fails without one), so a
// runtime that does not come up here is a failure, not a skip. The Python-side behavior is
// pinned in far more detail by the core battery (core/tests/); these tests cover what the core
// cannot see: atoms in and out, the attribute and message plumbing, and the perform routine.

namespace {

    /// Run one 64-sample vector of a constant `input` through the object.
    std::vector<double> render(python& object, const double input) {
        std::vector<double> in(64, input);
        std::vector<double> out(64, 12345.0); // a sentinel, so untouched samples show
        double*             inp[1]  = {in.data()};
        double*             outp[1] = {out.data()};
        audio_bundle        ina{inp, 1, static_cast<long>(in.size())};
        audio_bundle        outa{outp, 1, static_cast<long>(out.size())};
        object(ina, outa);
        return out;
    }

    bool all_equal(const std::vector<double>& samples, const double expected) {
        for (const auto s : samples) {
            if (s != expected) {
                return false;
            }
        }
        return true;
    }

    c74::max::t_atom float_atom(const double value) {
        c74::max::t_atom a{};
        c74::max::atom_setfloat(&a, value);
        return a;
    }

    c74::max::t_atom long_atom(const long value) {
        c74::max::t_atom a{};
        c74::max::atom_setlong(&a, value);
        return a;
    }

    c74::max::t_atom symbol_atom(const char* value) {
        c74::max::t_atom a{};
        c74::max::atom_setsym(&a, c74::max::gensym(value));
        return a;
    }

    /// The object's attribute getter, as Max calls it (the object allocates the atom).
    c74::max::t_atom get(python& object, const char* name) {
        long              argc = 0;
        c74::max::t_atom* argv = nullptr;
        object.attr_get(c74::min::symbol{name}, &argc, &argv);
        c74::max::t_atom result = *argv;
        c74::max::sysmem_freeptr(argv);
        return result;
    }

} // namespace

SCENARIO("The object finds its package and starts the embedded interpreter") {
    ext_main(nullptr);
    test_wrapper<python> an_instance;
    python&              my_object = an_instance;

    THEN("the package root is the folder above the test binary") {
        REQUIRE(std::filesystem::exists(tap::python::package_root() / "python" / "default.py"));
    }
    THEN("the interpreter is running") {
        REQUIRE(Py_IsInitialized());
    }
    THEN("python/default.py passes audio at unity gain") {
        REQUIRE(all_equal(render(my_object, 0.5), 0.5));
    }
}

SCENARIO("Attributes convert Max atoms to and from the class's field types") {
    ext_main(nullptr);
    test_wrapper<python> an_instance;
    python&              my_object = an_instance;
    REQUIRE(Py_IsInitialized());

    WHEN("a float attribute is set with a float atom") {
        const auto a = float_atom(0.25);
        my_object.attr_set(c74::min::symbol{"gain"}, 1, &a);
        THEN("the class sees it") {
            REQUIRE(all_equal(render(my_object, 1.0), 0.25));
        }
        THEN("the getter returns it as a float atom") {
            const auto got = get(my_object, "gain");
            REQUIRE(c74::max::atom_gettype(&got) == c74::max::A_FLOAT);
            REQUIRE(c74::max::atom_getfloat(&got) == 0.25);
        }
    }

    WHEN("a float attribute is set with a long atom") {
        const auto a = long_atom(2);
        my_object.attr_set(c74::min::symbol{"gain"}, 1, &a);
        THEN("it converts, as atom_getfloat does") {
            REQUIRE(all_equal(render(my_object, 1.0), 2.0));
        }
    }

    WHEN("a float attribute is set with a symbol atom") {
        const auto a = symbol_atom("loud");
        my_object.attr_set(c74::min::symbol{"gain"}, 1, &a);
        THEN("it reads as 0.0, as atom_getfloat does") {
            REQUIRE(all_equal(render(my_object, 1.0), 0.0));
        }
    }

    WHEN("an attribute is set with more than one atom") {
        const c74::max::t_atom atoms[2] = {float_atom(0.5), float_atom(0.25)};
        my_object.attr_set(c74::min::symbol{"gain"}, 2, atoms);
        THEN("the set is refused") {
            REQUIRE(all_equal(render(my_object, 1.0), 1.0));
        }
    }

    WHEN("an attribute the class does not have is read") {
        const auto got = get(my_object, "nonexistent");
        THEN("the getter returns its default of 0.0") {
            REQUIRE(c74::max::atom_getfloat(&got) == 0.0);
        }
    }
}

SCENARIO("Messages reach the class's methods with their arguments converted") {
    ext_main(nullptr);
    test_wrapper<python> an_instance;
    python&              my_object = an_instance;
    REQUIRE(Py_IsInitialized());

    WHEN("a float message is sent") {
        const auto a = float_atom(0.25);
        my_object.message_gimme(c74::min::symbol{"float"}, 1, &a);
        THEN("default.float() sets the gain") {
            REQUIRE(all_equal(render(my_object, 1.0), 0.25));
        }
    }

    WHEN("an int message is sent") {
        const auto a = long_atom(3);
        my_object.message_gimme(c74::min::symbol{"int"}, 1, &a);
        THEN("default.int() receives it converted to its float hint") {
            REQUIRE(all_equal(render(my_object, 1.0), 3.0));
        }
    }

    WHEN("a message arrives with the wrong number of arguments") {
        my_object.message_gimme(c74::min::symbol{"float"}, 0, nullptr);
        THEN("it is refused and nothing changes") {
            REQUIRE(all_equal(render(my_object, 1.0), 1.0));
        }
    }

    WHEN("a message the class does not define arrives") {
        const auto a = float_atom(0.25);
        my_object.message_gimme(c74::min::symbol{"nonexistent"}, 1, &a);
        THEN("it is ignored") {
            REQUIRE(all_equal(render(my_object, 1.0), 1.0));
        }
    }
}

SCENARIO("The object has as many signal inlets and outlets as its class's process() declares (plan 2.4)") {
    ext_main(nullptr);
    // test_wrapper takes no arguments: make [tap.python~ stereo_width] as Max would
    const auto argument = symbol_atom("stereo_width");
    auto*      wrapped  = c74::min::wrapper_new<python>(c74::min::symbol("dummy"), 1, &argument);
    REQUIRE(wrapped);
    python& my_object = wrapped->m_min_object;

    THEN("two of each, the second inlet named for process()'s parameter") {
        REQUIRE(my_object.inlets().size() == 2);
        CHECK(my_object.outlets().size() == 2);
        CHECK(my_object.inlets()[1]->description().find("right") != std::string::npos);
    }
    THEN("each channel is processed") {
        std::vector<double> left(64, 0.75);
        std::vector<double> right(64, 0.25);
        std::vector<double> out_left(64, 12345.0);
        std::vector<double> out_right(64, 12345.0);
        double*             in[2]  = {left.data(), right.data()};
        double*             out[2] = {out_left.data(), out_right.data()};
        audio_bundle        input{in, 2, 64};
        audio_bundle        output{out, 2, 64};
        my_object(input, output);
        CHECK(all_equal(out_left, 0.75)); // width 1: as it was
        CHECK(all_equal(out_right, 0.25));
    }
    c74::max::object_free(wrapped);
}

namespace {

    /// A class with these process() parameters (per sample), returning this many values.
    std::string shaped_class(const std::vector<std::string>& inputs, const std::size_t outputs) {
        std::string parameters;
        std::string sum = "0.0";
        for (const auto& name : inputs) {
            parameters += ", " + name + ": float";
            sum += " + " + name;
        }
        std::string hint   = "float";
        std::string values = sum;
        if (outputs > 1) {
            hint = "tuple[float";
            for (std::size_t i = 1; i < outputs; ++i) {
                hint += ", float";
                values += ", " + sum;
            }
            hint += "]";
        }
        return "class maxtest_mock_shape:\n    def process(self" + parameters + ") -> " + hint + ":\n        return "
               + values + "\n";
    }

    void write_file(const std::filesystem::path& path, const std::string& text) {
        std::ofstream{path, std::ios::binary | std::ios::trunc} << text;
    }

} // namespace

SCENARIO("A save that changes process()'s inputs or outputs changes the object's inlets and outlets in place "
         "(plan 2.4)") {
    ext_main(nullptr);
    // a fixture in the package's python folder, where python/maxtest_*.py is ignored by git
    const auto file = tap::python::package_root() / "python" / "maxtest_mock_shape.py";
    write_file(file, shaped_class({"left", "right"}, 2));
    const auto argument = symbol_atom("maxtest_mock_shape");
    auto*      wrapped  = c74::min::wrapper_new<python>(c74::min::symbol("dummy"), 1, &argument);
    REQUIRE(wrapped);
    python& my_object = wrapped->m_min_object;
    REQUIRE(my_object.inlets().size() == 2);
    REQUIRE(my_object.outlets().size() == 2);
    dynlets::reset();

    WHEN("a save gives it three inputs and one output") {
        write_file(file, shaped_class({"a", "b", "c"}, 1));
        my_object.update_source();
        THEN("Max's signal inlets become three and the last outlet is deleted") {
            CHECK(dynlets::signal_inlets == 3);
            CHECK(dynlets::deleted == std::vector<long>{1});
            CHECK(dynlets::appended == 0);
            CHECK(dynlets::chain_broken);
        }
        THEN("min's lists follow, each inlet named for its parameter") {
            REQUIRE(my_object.inlets().size() == 3);
            CHECK(my_object.outlets().size() == 1);
            CHECK(my_object.inlets()[1]->description().find("b: input 2") != std::string::npos);
            CHECK(my_object.inlets()[2]->description().find("c: input 3") != std::string::npos);
        }
    }

    WHEN("a save gives it no inputs and three outputs") {
        write_file(file, shaped_class({}, 3));
        my_object.update_source();
        THEN("one signal inlet stays, for messages, and an outlet is appended") {
            CHECK(dynlets::signal_inlets == 1);
            CHECK(dynlets::appended == 1);
            CHECK(dynlets::deleted.empty());
            CHECK(my_object.inlets().size() == 1);
            CHECK(my_object.outlets().size() == 3);
        }
    }

    WHEN("a save renames an input but keeps the counts") {
        write_file(file, shaped_class({"mid", "side"}, 2));
        my_object.update_source();
        THEN("Max's inlets and outlets are left alone, and the inlet's help names the new parameter") {
            CHECK(dynlets::signal_inlets == 0);
            CHECK(dynlets::appended == 0);
            CHECK(dynlets::deleted.empty());
            CHECK_FALSE(dynlets::chain_broken);
            REQUIRE(my_object.inlets().size() == 2);
            CHECK(my_object.inlets()[1]->description().find("side: input 2") != std::string::npos);
        }
    }

    WHEN("a save breaks the class") {
        write_file(file, "class maxtest_mock_shape:\n    def process(self, x: float) -> float\n");
        my_object.update_source();
        THEN("the inlets and outlets stay as they were") {
            CHECK(dynlets::signal_inlets == 0);
            CHECK(my_object.inlets().size() == 2);
            CHECK(my_object.outlets().size() == 2);
        }
    }

    c74::max::object_free(wrapped);
    std::filesystem::remove(file);
}

SCENARIO("Methods named like messages Max sends with C arguments are not exposed, and the promised ones are "
         "(plan 8.2)") {
    ext_main(nullptr);
    // the test's kernel must tell its answers apart, or what follows tests the linker; and its
    // "not found" must not be the method_false this module can take the address of (Windows)
    const auto not_found = c74::max::object_getmethod(nullptr, c74::max::gensym("maxtest_no_such_name"));
    REQUIRE(c74::max::object_getmethod(nullptr, c74::max::gensym("maxtest_host_answers")) != not_found);
    REQUIRE(not_found != reinterpret_cast<c74::max::method>(c74::max::method_false));
    const auto file = tap::python::package_root() / "python" / "maxtest_mock_reserved.py";
    write_file(file, "class maxtest_mock_reserved:\n"
                     // a field, and a method, which must be exposed whatever the guard answers
                     "    gain: float = 0.75\n"
                     // what Max calls with C arguments: a Python method of the name would crash Max
                     "    def dspstate(self, on: int) -> None:\n        pass\n"
                     "    def fileusage(self) -> None:\n        pass\n"
                     "    def patchlineupdate(self) -> None:\n        pass\n"
                     "    def inputchanged(self) -> None:\n        pass\n"
                     // a name only the guard reserves: the test's object_getmethod answers it as
                     // Max's would for a method the class registered
                     "    def maxtest_host_answers(self) -> None:\n        pass\n"
                     // what the ReadMe promises stays a message, whatever the guard answers
                     "    def int(self, n: int) -> None:\n        pass\n"
                     "    def float(self, x: float) -> None:\n        pass\n"
                     "    def symbol(self, s: str) -> None:\n        pass\n"
                     "    def bang(self) -> None:\n        pass\n"
                     "    def list(self, *args: float) -> None:\n        pass\n"
                     "    def greet(self, name: str) -> None:\n        pass\n"
                     "    def process(self, x: float) -> float:\n        return x\n");
    const auto argument = symbol_atom("maxtest_mock_reserved");
    auto*      wrapped  = c74::min::wrapper_new<python>(c74::min::symbol("dummy"), 1, &argument);
    REQUIRE(wrapped);
    python& my_object = wrapped->m_min_object;

    THEN("the reserved names are not registered, and every promised name is") {
        CHECK(my_object.python_message_names()
              == std::vector<std::string>{"bang", "float", "greet", "int", "list", "symbol"});
    }
    THEN("the field is an attribute: a name Max does not answer is not reserved by the guard (1.0.1)") {
        const auto got = get(my_object, "gain");
        CHECK(c74::max::atom_gettype(&got) == c74::max::A_FLOAT);
        CHECK(c74::max::atom_getfloat(&got) == 0.75);
    }
    THEN("found means neither null nor what Max answers for a name it does not have (1.0.2)") {
        const auto answered = c74::max::object_getmethod(nullptr, c74::max::gensym("maxtest_host_answers"));
        CHECK(runtime::found_method(nullptr, not_found) == false);
        CHECK(runtime::found_method(not_found, not_found) == false);
        CHECK(runtime::found_method(answered, not_found) == true);
        CHECK(runtime::not_found_method(my_object.maxobj()) == not_found); // asked of Max, not taken from &method_false
    }
    THEN("audio is bound regardless") {
        CHECK(all_equal(render(my_object, 0.5), 0.5));
    }
    c74::max::object_free(wrapped);
    std::filesystem::remove(file);
}

SCENARIO("Every object reserves the names Max sends it; an audio object, the audio ones as well (plan 9.2)") {
    const auto every = runtime::reserved_messages(false);
    const auto audio = runtime::reserved_messages(true);
    const auto has   = [](const std::vector<std::string>& names, const std::string& name) {
        return std::find(names.begin(), names.end(), name) != names.end();
    };

    THEN("both are sorted, and every object reserves what Max sends with C arguments, and filechanged") {
        CHECK(std::is_sorted(every.begin(), every.end()));
        CHECK(std::is_sorted(audio.begin(), audio.end()));
        for (const auto* name : {"anything", "assist", "fileusage", "filechanged", "notify", "patchlineupdate"}) {
            CHECK(has(every, name));
        }
    }
    THEN("an audio object reserves every object's names and exactly the audio ones besides") {
        auto expected = every;
        for (const auto& name : runtime::audio_messages()) {
            CHECK_FALSE(has(every, name));
            expected.push_back(name);
        }
        std::sort(expected.begin(), expected.end());
        CHECK(audio == expected);
        for (const auto* name : {"dsp64", "dspsetup", "dspstate", "inputchanged", "mode", "latency"}) {
            CHECK(has(audio, name));
        }
    }
}

SCENARIO("With @mode worker, process() runs on a thread of its own, @latency milliseconds behind (plan 2.5)") {
    ext_main(nullptr);
    test_wrapper<python> an_instance;
    python&              my_object = an_instance;
    REQUIRE(Py_IsInitialized());

    my_object.m_mode    = c74::min::symbol("worker");
    my_object.m_latency = 4.0;                        // three 64-sample vectors at 48 kHz
    my_object.dspsetup(c74::min::atoms{48000.0, 64}); // as min does when the chain compiles
    THEN("the delay it adds is reported in samples") {
        CHECK(static_cast<int>(my_object.m_latencysamples) == 3 * 64);
    }
    THEN("the output is the input three vectors later, the first three silence") {
        for (int k = 0; k < 6; ++k) {
            const auto out = render(my_object, static_cast<double>(k + 1));
            CHECK(all_equal(out, k < 3 ? 0.0 : static_cast<double>(k - 3 + 1)));
            std::this_thread::sleep_for(std::chrono::milliseconds{50}); // the worker's turn
        }
    }
    WHEN("the mode is set back to direct and the chain compiles") {
        my_object.m_mode = c74::min::symbol("direct");
        my_object.dspsetup(c74::min::atoms{48000.0, 64});
        THEN("there is no delay") {
            CHECK(static_cast<int>(my_object.m_latencysamples) == 0);
            CHECK(all_equal(render(my_object, 0.5), 0.5));
        }
    }
    WHEN("the mode is set to something else") {
        my_object.m_mode = c74::min::symbol("fast");
        THEN("it is direct") {
            CHECK(my_object.m_mode.get() == c74::min::symbol("direct"));
        }
    }
    WHEN("the latency is set below 0") {
        my_object.m_latency = -1.0;
        my_object.dspsetup(c74::min::atoms{48000.0, 64});
        THEN("it is held to 0, which is one vector") {
            CHECK(static_cast<double>(my_object.m_latency) == 0.0);
            CHECK(static_cast<int>(my_object.m_latencysamples) == 64);
        }
    }
}

SCENARIO("The runtime is support/<platform> in a package that carries every platform's, else support/ (plan 4.8)") {
    const auto package = std::filesystem::temp_directory_path() / "tap-python-runtime-home-test";
    std::filesystem::remove_all(package);
    std::filesystem::create_directories(package / "support");

    THEN("a single-platform package's runtime is support/") {
        CHECK(runtime::runtime_home(package) == package / "support");
    }
    WHEN("support/ holds a folder for this platform") {
        std::filesystem::create_directories(package / "support" / runtime::runtime_platform());
        THEN("that is the runtime") {
            CHECK(runtime::runtime_home(package) == package / "support" / runtime::runtime_platform());
        }
    }
    WHEN("support/ holds only other platforms' folders") {
        std::filesystem::create_directories(package / "support" / "another-platform");
        THEN("the runtime is still support/") {
            CHECK(runtime::runtime_home(package) == package / "support");
        }
    }
    std::filesystem::remove_all(package);
}

SCENARIO("A reload keeps the class's attributes: those the object added are its own, not Max's (1.0.1)") {
    ext_main(nullptr);
    attributes::model(true);
    // a fixture in the package's python folder, where python/maxtest_*.py is ignored by git
    const auto file   = tap::python::package_root() / "python" / "maxtest_mock_fields.py";
    const auto source = std::string{"class maxtest_mock_fields:\n    level: float = 1.0\n\n"
                                    "    def process(self, x: float) -> float:\n        return x * self.level\n"};
    write_file(file, source);
    const auto argument = symbol_atom("maxtest_mock_fields");
    auto*      wrapped  = c74::min::wrapper_new<python>(c74::min::symbol("dummy"), 1, &argument);
    REQUIRE(wrapped);
    python& my_object = wrapped->m_min_object;
    REQUIRE(attributes::added.count("level") == 1); // the object answers "level" now, as in Max

    const auto half = float_atom(0.5);
    my_object.attr_set(c74::min::symbol{"level"}, 1, &half);
    const auto before = get(my_object, "level");
    REQUIRE(c74::max::atom_getfloat(&before) == 0.5);

    WHEN("a save changes the file and it reloads") {
        write_file(file, source + "    # saved again\n");
        my_object.update_source();
        THEN("the field is still an attribute, with its value") {
            const auto got = get(my_object, "level");
            CHECK(c74::max::atom_getfloat(&got) == 0.5);
            CHECK(attributes::added.count("level") == 1);
        }
    }

    c74::max::object_free(wrapped);
    std::filesystem::remove(file);
    attributes::model(false);
}

// ---- tap.python (plan 9.3) ------------------------------------------------------------------------
//
// Each object is made through the class's own new, as Max makes a box, and messages reach it through
// the class's methods as Max dispatches them. The mock kernel registers no instance methods
// (object_addmethod is stubbed above), so a message for one of the class's Python methods reaches it
// through the class's forwarders here — which try the class's messages first for that reason.

namespace {

    using tap::python::control_object;

    std::map<void*, std::vector<void*>> made; // the outlets each object was made with

    /// [tap.python <name>], made as Max makes a box.
    c74::max::t_object* make_control(const char* name) {
        auto  argument = symbol_atom(name);
        auto* x        = static_cast<c74::max::t_object*>(tap_python_new(c74::max::gensym("tap.python"), 1, &argument));
        if (x) {
            auto& ids = made[x];
            ids.push_back(control_object::self(x)->dumpout_outlet());
            for (std::size_t n = 0; control_object::self(x)->value_outlet(n); ++n) {
                ids.push_back(control_object::self(x)->value_outlet(n));
            }
        }
        return x;
    }

    /// Free the object, and forget what the stubs kept for it (its address may be the next one's).
    void free_control(c74::max::t_object* x) {
        c74::max::object_free(x);
        made.erase(x);
        outlets::inserted.erase(x);
        for (auto it = obex::stored.begin(); it != obex::stored.end();) {
            it = it->first.first == x ? obex::stored.erase(it) : std::next(it);
        }
    }

    /// One message as text: "int 4", "symbol C", "1 0 1" (a list), "steps 5".
    std::string text_of(const c74::max::t_atom_vector& message) {
        std::string text;
        for (const auto& atom : message) {
            text += text.empty() ? "" : " ";
            switch (c74::max::atom_gettype(&atom)) {
            case c74::max::A_LONG:
                text += std::to_string(c74::max::atom_getlong(&atom));
                break;
            case c74::max::A_FLOAT: {
                char number[64];
                std::snprintf(number, sizeof number, "%g", c74::max::atom_getfloat(&atom));
                text += number;
                break;
            }
            default:
                text += c74::max::atom_getsym(&atom)->s_name;
                break;
            }
        }
        return text;
    }

    /// What `outlet`, as Max gave it to the object, has output so far.
    std::vector<std::string> output_of(c74::max::t_object* x, const void* outlet) {
        auto ids = made[x];
        ids.insert(ids.end(), outlets::inserted[x].begin(), outlets::inserted[x].end());
        std::vector<std::string> texts;
        for (const auto& message : *c74::max::object_getoutput(x, outlets::index_of(ids, outlet))) {
            texts.push_back(text_of(message));
        }
        return texts;
    }

    std::vector<std::string> value_output(c74::max::t_object* x, const std::size_t n) {
        return output_of(x, control_object::self(x)->value_outlet(n));
    }

    std::vector<std::string> dumpout_output(c74::max::t_object* x) {
        return output_of(x, control_object::self(x)->dumpout_outlet());
    }

    /// A message, as Max dispatches it to the class: its int, float, bang or list method, or its
    /// anything for any other selector — each called with the type it was registered with.
    void send(c74::max::t_object* x, const char* selector, std::vector<c74::max::t_atom> atoms = {}) {
        using tap::python::t_tap_python;
        auto*             object = reinterpret_cast<t_tap_python*>(x);
        const std::string name{selector};
        const auto        method = [&](const char* of) { return c74::max::zgetfn(x, c74::max::gensym(of)); };
        if (name == "int") {
            reinterpret_cast<void (*)(t_tap_python*, c74::max::t_atom_long)>(method("int"))(
                object, c74::max::atom_getlong(atoms.data()));
        }
        else if (name == "float") {
            reinterpret_cast<void (*)(t_tap_python*, double)>(method("float"))(object,
                                                                               c74::max::atom_getfloat(atoms.data()));
        }
        else if (name == "bang") {
            reinterpret_cast<void (*)(t_tap_python*)>(method("bang"))(object);
        }
        else {
            reinterpret_cast<void (*)(t_tap_python*, c74::max::t_symbol*, long, c74::max::t_atom*)>(
                method(name == "list" ? "list" : "anything"))(object, c74::max::gensym(selector),
                                                              static_cast<long>(atoms.size()), atoms.data());
        }
    }

    /// What the objects say in the console while `fn` runs.
    template <class Fn>
    std::string console_of(Fn&& fn) {
        {
            const std::lock_guard<std::mutex> guard{console::lock};
            console::said.clear();
            console::listening = true;
        }
        fn();
        const std::lock_guard<std::mutex> guard{console::lock};
        console::listening = false;
        return console::said;
    }

    bool contains(const std::string& text, const std::string& part) {
        return text.find(part) != std::string::npos;
    }

    std::filesystem::path fixture(const char* name) {
        return tap::python::package_root() / "python" / (std::string{name} + ".py");
    }

} // namespace

SCENARIO("tap.python is registered beside tap.python~, with its dumpout and its forwarders (plan 9.3)") {
    ext_main(nullptr);
    auto* x = make_control("default");
    REQUIRE(x);

    THEN("the class registers object_obex_dumpout as its dumpout method, which get<attr> needs (plan 9.0)") {
        CHECK(c74::max::zgetfn(x, c74::max::gensym("dumpout"))
              == reinterpret_cast<c74::max::method>(c74::max::object_obex_dumpout));
    }
    THEN("the forwarders, the file watcher's filechanged and assist are the class's") {
        for (const auto* name : {"anything", "int", "float", "bang", "list", "filechanged", "assist"}) {
            CHECK(c74::max::zgetfn(x, c74::max::gensym(name)) != nullptr);
        }
    }
    THEN("the dumpout is stored in the obex, and is the rightmost outlet") {
        c74::max::t_object* stored{};
        REQUIRE(c74::max::object_obex_lookup(x, c74::max::gensym("dumpout"), &stored) == c74::max::MAX_ERR_NONE);
        CHECK(stored == control_object::self(x)->dumpout_outlet());
        CHECK(control_object::self(x)->value_outlet_count() == 1);
        CHECK(outlets::index_of(made[x], stored) == 1); // made first: after the value outlet
    }
    THEN("with no argument it loads python/default.py, whose bang outputs the gain") {
        send(x, "bang");
        CHECK(value_output(x, 0) == std::vector<std::string>{"float 1"});
        send(x, "float", {float_atom(0.25)});
        send(x, "bang");
        CHECK(value_output(x, 0) == std::vector<std::string>{"float 1", "float 0.25"});
    }
    THEN("process() is a message like any other, and outputs what it returns") {
        send(x, "process", {float_atom(2.0)});
        CHECK(value_output(x, 0) == std::vector<std::string>{"float 2"});
    }
    free_control(x);
}

SCENARIO("tap.python outputs what a method returns, from the outlets its return hints give it (plan 9.3)") {
    ext_main(nullptr);

    GIVEN("euclid.py: a bang outputs a list, from its one outlet") {
        auto* x = make_control("euclid");
        REQUIRE(x);
        CHECK(control_object::self(x)->value_outlet_count() == 1);
        send(x, "bang");
        CHECK(value_output(x, 0) == std::vector<std::string>{"1 0 0 1 0 0 1 0"});

        WHEN("an attribute is set by its message, and read back by get<attribute>") {
            send(x, "steps", {long_atom(5)});
            send(x, "bang");
            send(x, "getsteps");
            THEN("the list follows it, and the value comes from the dumpout") {
                CHECK(value_output(x, 0) == std::vector<std::string>{"1 0 0 1 0 0 1 0", "1 0 1 0 1"});
                CHECK(dumpout_output(x) == std::vector<std::string>{"steps 5"});
            }
        }
        free_control(x);
    }

    GIVEN("note_name.py: an int outputs the octave from the right outlet and the name, as a symbol, from the left") {
        auto* x = make_control("note_name");
        REQUIRE(x);
        CHECK(control_object::self(x)->value_outlet_count() == 2);
        send(x, "int", {long_atom(60)});
        send(x, "int", {long_atom(70)});
        CHECK(value_output(x, 0) == std::vector<std::string>{"symbol C", "symbol A#"});
        CHECK(value_output(x, 1) == std::vector<std::string>{"int 4", "int 4"});
        CHECK(dumpout_output(x).empty());
        free_control(x);
    }

    GIVEN("scale.py: a list in, through numpy, a list out") {
        auto* x = make_control("scale");
        REQUIRE(x);
        send(x, "factor", {float_atom(2.0)});
        send(x, "offset", {long_atom(1)});
        send(x, "list", {long_atom(1), float_atom(2.0), long_atom(3)});
        CHECK(value_output(x, 0) == std::vector<std::string>{"3 5 7"});
        free_control(x);
    }
}

SCENARIO("A save that widens or narrows the return hints changes tap.python's outlets in place (plan 9.3)") {
    ext_main(nullptr);
    const auto file = fixture("maxtest_mock_control_outlets");
    const auto one = std::string{"class maxtest_mock_control_outlets:\n    def bang(self) -> int:\n        return 1\n"};
    const auto two = std::string{"class maxtest_mock_control_outlets:\n"
                                 "    def bang(self) -> tuple[int, int]:\n        return 1, 2\n"};
    write_file(file, one);
    auto* x = make_control("maxtest_mock_control_outlets");
    REQUIRE(x);
    auto&       object  = *control_object::self(x);
    void* const first   = object.value_outlet(0);
    void* const dumpout = object.dumpout_outlet();
    REQUIRE(object.value_outlet_count() == 1);
    dynlets::reset();

    WHEN("a save gives a method a tuple[int, int] return") {
        write_file(file, two);
        object.update_source();
        THEN("an outlet is inserted after the last value outlet, between the box's dynlet_begin and dynlet_end") {
            CHECK(dynlets::calls
                  == std::vector<std::string>{"dynlet_begin", "insert after " + dynlets::id(first), "dynlet_end"});
            CHECK(object.value_outlet_count() == 2);
            CHECK(object.value_outlet(0) == first);
            CHECK(object.dumpout_outlet() == dumpout);
        }
        THEN("the new outlet carries its value") {
            send(x, "bang");
            CHECK(value_output(x, 0) == std::vector<std::string>{"int 1"});
            CHECK(value_output(x, 1) == std::vector<std::string>{"int 2"});
        }

        AND_WHEN("a save narrows it again") {
            void* const second = object.value_outlet(1);
            dynlets::reset();
            write_file(file, one);
            object.update_source();
            THEN("the surplus outlet is deleted, and nothing else") {
                CHECK(dynlets::calls
                      == std::vector<std::string>{"dynlet_begin", "delete " + dynlets::id(second), "dynlet_end"});
                CHECK(object.value_outlet_count() == 1);
                CHECK(object.value_outlet(0) == first);
            }
        }
    }

    WHEN("a save keeps the count") {
        write_file(file, one + "    # saved again\n");
        object.update_source();
        THEN("the outlets are left alone") {
            CHECK(dynlets::calls.empty());
            CHECK(object.value_outlet_count() == 1);
        }
    }

    WHEN("a save breaks the class") {
        write_file(file, "class maxtest_mock_control_outlets:\n    def bang(self) -> int\n");
        object.update_source();
        THEN("the outlets stay as they were, and a bang outputs nothing") {
            CHECK(dynlets::calls.empty());
            send(x, "bang");
            CHECK(value_output(x, 0).empty());
        }
    }

    free_control(x);
    std::filesystem::remove(file);
}

SCENARIO("tap.python's forwarders: the class's messages, its attributes, get<attribute>, then its own anything "
         "(plan 9.3)") {
    ext_main(nullptr);

    GIVEN("a class with an anything method") {
        const auto file = fixture("maxtest_mock_control_anything");
        write_file(file, "class maxtest_mock_control_anything:\n"
                         "    steps: int = 8\n\n"
                         "    def hello(self, n: int) -> int:\n        return n + 1\n\n"
                         "    def anything(self, selector: str, *args) -> list:\n"
                         "        return [selector, *args]\n");
        auto* x = make_control("maxtest_mock_control_anything");
        REQUIRE(x);

        THEN("its anything is not registered with Max: the class's forwarder calls it") {
            CHECK(control_object::self(x)->python_message_names() == std::vector<std::string>{"hello"});
        }
        THEN("a message the class has a method for goes to that method") {
            send(x, "hello", {long_atom(1)});
            CHECK(value_output(x, 0) == std::vector<std::string>{"int 2"});
        }
        THEN("an attribute's name sets it, and get<name> outputs it from the dumpout") {
            send(x, "steps", {long_atom(3)});
            send(x, "getsteps");
            CHECK(dumpout_output(x) == std::vector<std::string>{"steps 3"});
            CHECK(value_output(x, 0).empty());
        }
        THEN("any other message reaches anything, with its selector first") {
            send(x, "foo", {long_atom(1), symbol_atom("two")});
            send(x, "symbol", {symbol_atom("hi")});
            send(x, "bang");
            CHECK(value_output(x, 0) == std::vector<std::string>{"foo 1 two", "symbol hi", "bang"});
        }
        free_control(x);
        std::filesystem::remove(file);
    }

    GIVEN("a class without one") {
        const auto file = fixture("maxtest_mock_control_plain");
        write_file(file, "class maxtest_mock_control_plain:\n    def hello(self) -> int:\n        return 1\n");
        auto* x = make_control("maxtest_mock_control_plain");
        REQUIRE(x);

        THEN("a message it has no method for is not understood, in Max's own words, and nothing is output") {
            const auto said = console_of([&] {
                send(x, "foo", {long_atom(1)});
                send(x, "int", {long_atom(1)});
            });
            CHECK(contains(said, "doesn't understand \"foo\""));
            CHECK(contains(said, "doesn't understand \"int\""));
            CHECK(value_output(x, 0).empty());
        }
        free_control(x);
        std::filesystem::remove(file);
    }
}

SCENARIO("tap.python's reserved names: every object's, less anything, plus dumpout; the audio ones are free "
         "(plan 9.3)") {
    ext_main(nullptr);
    const auto names = control_object::reserved_names();
    const auto has   = [&](const char* name) { return std::find(names.begin(), names.end(), name) != names.end(); };
    CHECK(has("dumpout"));
    CHECK(has("filechanged"));
    CHECK(has("assist"));
    CHECK_FALSE(has("anything"));
    for (const auto& name : tap::python::audio_messages()) {
        CHECK_FALSE(has(name.c_str()));
    }

    const auto file = fixture("maxtest_mock_control_reserved");
    write_file(file, "class maxtest_mock_control_reserved:\n"
                     "    def dumpout(self) -> None:\n        pass\n"
                     "    def assist(self) -> None:\n        pass\n"
                     "    def dsp(self) -> None:\n        pass\n"
                     "    def mode(self) -> None:\n        pass\n"
                     "    def signal(self) -> None:\n        pass\n"
                     "    def anything(self, selector: str, *args) -> None:\n        pass\n"
                     "    def int(self, n: int) -> None:\n        pass\n"
                     "    def list(self, *args: float) -> None:\n        pass\n"
                     "    def bang(self) -> None:\n        pass\n");
    auto* x = make_control("maxtest_mock_control_reserved");
    REQUIRE(x);
    THEN("dumpout and assist are not exposed; the audio names, and int, list and bang, are registered "
         "despite the class's forwarders of those names; anything is forwarded") {
        CHECK(control_object::self(x)->python_message_names()
              == std::vector<std::string>{"bang", "dsp", "int", "list", "mode", "signal"});
    }
    free_control(x);
    std::filesystem::remove(file);
}

SCENARIO("A message on the audio thread (Scheduler in Audio Interrupt) is said once per session, from the main "
         "thread (plan 9.3)") {
    ext_main(nullptr);
    auto* x = make_control("default");
    REQUIRE(x);
    const auto before = qelems::set.load();

    send(x, "bang");
    CHECK(qelems::set.load() == before); // not on the audio thread: nothing to say

    audio_thread::now = true;
    send(x, "bang");
    send(x, "bang");
    audio_thread::now = false;
    CHECK(qelems::set.load() == before + 1); // set once, from the audio thread: real-time safe

    // what the qelem does on the main thread
    const auto first  = console_of([&] { control_object::self(x)->post_notices(); });
    const auto second = console_of([&] { control_object::self(x)->post_notices(); });
    CHECK(contains(first, "Scheduler in Audio Interrupt"));
    CHECK(second.empty());
    CHECK(value_output(x, 0).size() == 3); // the messages ran regardless
    free_control(x);
}

SCENARIO("A reload that widens and narrows the outlets races a second thread's messages safely (plan 9.3)") {
    ext_main(nullptr);
    const auto file = fixture("maxtest_mock_control_race");
    const auto one  = std::string{"class maxtest_mock_control_race:\n    def bang(self) -> int:\n        return 1\n"};
    const auto two  = std::string{"class maxtest_mock_control_race:\n"
                                  "    def bang(self) -> tuple[int, int]:\n        return 1, 2\n"};
    write_file(file, one);
    auto* x = make_control("maxtest_mock_control_race");
    REQUIRE(x);
    auto& object = *control_object::self(x);
    // once each way first, so that every symbol either thread asks the mock kernel for exists (its
    // gensym is not thread-safe, as Max's is), and both sources are cached
    write_file(file, two);
    object.update_source();
    write_file(file, one);
    object.update_source();
    const auto* bang = c74::max::gensym("bang");

    std::atomic<bool> done{};
    std::atomic<long> sent{};
    std::thread       messages{[&] {
        while (!done.load()) {
            object.message_gimme(bang, 0, nullptr);
            sent.fetch_add(1);
        }
    }};
    // (the console is not captured here: both threads post to it)
    for (int i = 0; i < 40; ++i) {
        write_file(file, i % 2 == 0 ? two : one);
        object.update_source();
    }
    done = true;
    messages.join();
    CHECK(sent.load() > 0);
    CHECK(object.value_outlet_count() == 1);
    // every value came out whole, from the outlet of its value: the first's 1, and 2 from each
    // second outlet a widening inserted — never a value for an outlet the object did not have then
    std::size_t output{};
    std::size_t misplaced{};
    for (const auto& text : value_output(x, 0)) {
        ++output;
        misplaced += text == "int 1" ? 0 : 1;
    }
    for (const auto* outlet : outlets::inserted[x]) {
        for (const auto& text : output_of(x, outlet)) {
            ++output;
            misplaced += text == "int 2" ? 0 : 1;
        }
    }
    CHECK(output > 0);
    CHECK(misplaced == 0);
    free_control(x);
    std::filesystem::remove(file);
}
