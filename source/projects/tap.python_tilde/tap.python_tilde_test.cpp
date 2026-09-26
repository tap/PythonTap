/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#include <vector>

#include "c74_min_unittest.h" // required unit-test header (defines main via Catch)

// The mock kernel does not implement these Max functions, which the object
// references for its dynamically generated attributes/messages and its file
// watcher. Provide inert stubs so the test binary links (the headers declare
// them with C linkage). With these stubs the Max-side attribute and method
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
