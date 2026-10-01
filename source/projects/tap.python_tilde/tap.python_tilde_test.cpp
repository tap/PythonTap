/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "c74_min_unittest.h" // required unit-test header (defines main via Catch)

// The mock kernel does not implement these Max functions, which the object
// references for its dynamically generated attributes/messages and its file
// watcher, or implements one too thinly (attr_args_offset). Provide stubs so the
// test binary links (the headers declare them with C linkage). With these stubs the Max-side attribute and method
// registrations fail harmlessly, so the tests below drive the object's own
// attribute and message handlers directly, as Max's dispatch would.
namespace c74 {
    namespace max {
        extern "C" {
        t_object* attribute_new(const char*, t_symbol*, long, method, method) {
            return nullptr;
        }
        t_max_err object_addattr(void*, t_object*) {
            return MAX_ERR_GENERIC;
        }
        t_max_err object_attr_addattr_parse(t_object*, const char*, const char*, t_symbol*, long, const char*) {
            return MAX_ERR_GENERIC;
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
        void  filewatcher_start(void*) {}
        void* qelem_new(void*, method) {
            static int s_qelem;
            return &s_qelem;
        }
        void      qelem_set(void*) {}
        t_max_err object_deleteattr(void*, t_symbol*) {
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
    long              signal_inlets{}; // the last dsp_resize, 0 if none
    long              appended{};      // outlets appended
    std::vector<long> deleted;         // the indices of the outlets deleted, in order
    bool              chain_broken{};

    void reset() {
        signal_inlets = 0;
        appended      = 0;
        deleted.clear();
        chain_broken = false;
    }
} // namespace dynlets

namespace c74 {
    namespace max {
        extern "C" {
        t_max_err object_obex_lookup(void*, t_symbol* key, t_object** val) {
            static int s_box;
            if (key != gensym("#B")) {
                return MAX_ERR_GENERIC;
            }
            *val = reinterpret_cast<t_object*>(&s_box);
            return MAX_ERR_NONE;
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
    const auto file = tap::python::package_root() / "python" / "maxtest_mock_reserved.py";
    write_file(file, "class maxtest_mock_reserved:\n"
                     // what Max calls with C arguments: a Python method of the name would crash Max
                     "    def dspstate(self, on: int) -> None:\n        pass\n"
                     "    def fileusage(self) -> None:\n        pass\n"
                     "    def patchlineupdate(self) -> None:\n        pass\n"
                     "    def inputchanged(self) -> None:\n        pass\n"
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
    THEN("audio is bound regardless") {
        CHECK(all_equal(render(my_object, 0.5), 0.5));
    }
    c74::max::object_free(wrapped);
    std::filesystem::remove(file);
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
