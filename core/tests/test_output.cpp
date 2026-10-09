/// @file test_output.cpp
/// @brief tap.python's output (plan 9.1): what a method returns, converted for the host's outlets;
///        the outlet count from the return hints; the audio option; list parameters; what each kind
///        of object is told of a class they share.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <catch2/generators/catch_generators.hpp>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    std::string got(processor& p) {
        return std::get<std::string>(*p.get_attribute("got", value_type::symbol));
    }

    output_item number(const std::int64_t i) {
        return {std::nullopt, {i}};
    }

    output_item real(const double d) {
        return {std::nullopt, {d}};
    }

    output_item symbol(const std::string& s) {
        return {std::string{"symbol"}, {s}};
    }

    output_item list(std::vector<value> atoms) {
        return {std::string{"list"}, std::move(atoms)};
    }

    output_item message(const std::string& selector, std::vector<value> atoms) {
        return {selector, std::move(atoms)};
    }

    std::vector<value> word(const std::string& text) {
        return {text};
    }

    const message_info* find_message(const std::vector<message_info>& messages, const std::string& name) {
        for (const auto& m : messages) {
            if (m.name == name) {
                return &m;
            }
        }
        return nullptr;
    }

    /// Whether numpy is importable: its rows and fixtures need it. CI requires it.
    bool numpy_importable() {
        bool found = false;
        {
            gil_lock  lock;
            PyObject* numpy = PyImport_ImportModule("numpy");
            found           = numpy != nullptr;
            Py_XDECREF(numpy);
            PyErr_Clear();
        }
        if (std::getenv("TAP_PYTHON_TEST_REQUIRE_EXAMPLES")) {
            REQUIRE(found);
        }
        return found;
    }

    /// One row of the output table: what give(case) outputs from the one outlet, or what the report says.
    struct row {
        const char*                name;
        std::optional<output_item> item;  ///< nothing, when not set and there is no error
        const char*                error; ///< a fragment of the report, when the value does not convert
        bool                       numpy;
    };

    constexpr auto k_max = (std::numeric_limits<std::int64_t>::max)();
    constexpr auto k_min = std::numeric_limits<std::int64_t>::lowest();

    const std::vector<row> k_rows = {
        {"none", std::nullopt, nullptr, false},
        {"true", number(1), nullptr, false},
        {"false", number(0), nullptr, false},
        {"int", number(60), nullptr, false},
        {"negative", number(-5), nullptr, false},
        {"int_max", number(k_max), nullptr, false},
        {"int_min", number(k_min), nullptr, false},
        {"int_over", std::nullopt,
         "give() returned 9223372036854775808 (past a 64-bit integer), which an outlet cannot carry", false},
        {"int_under", std::nullopt,
         "give() returned -9223372036854775809 (past a 64-bit integer), which an outlet cannot carry", false},
        {"int_enum", number(3), nullptr, false},
        {"float", real(1.5), nullptr, false},
        {"inf", real(std::numeric_limits<double>::infinity()), nullptr, false},
        {"str", symbol("C"), nullptr, false},
        {"empty_str", symbol(""), nullptr, false},
        {"digits", symbol("60"), nullptr, false},
        {"spaced", symbol("hello world"), nullptr, false},
        {"list", list({std::int64_t{1}, 2.5, std::string{"x"}}), nullptr, false},
        {"message", message("note", {std::int64_t{60}, std::int64_t{100}}), nullptr, false},
        {"one_word", message("start", {}), nullptr, false},
        {"empty_list", std::nullopt, nullptr, false},
        {"tuple", list({std::int64_t{1}, std::int64_t{2}}), nullptr, false},
        {"range", list({std::int64_t{0}, std::int64_t{1}, std::int64_t{2}}), nullptr, false},
        {"bools", list({std::int64_t{1}, std::int64_t{0}}), nullptr, false},
        {"holding_none", std::nullopt, "give() returned a list holding None (item 2), which an outlet cannot carry",
         false},
        {"nested", std::nullopt, "give() returned a list holding a list (item 2)", false},
        {"big_in_list", std::nullopt,
         "give() returned a list holding 18446744073709551616 (past a 64-bit integer) (item 1)", false},
        {"dict", std::nullopt, "give() returned a dict, which an outlet cannot carry — nothing output", false},
        {"set", std::nullopt, "give() returned a set", false},
        {"bytes", std::nullopt, "give() returned a bytes", false},
        {"generator", std::nullopt, "give() returned a generator", false},
        {"object", std::nullopt, "give() returned an object", false},
        {"np_bool", number(1), nullptr, true},
        {"np_int64", number(7), nullptr, true},
        {"np_uint8", number(200), nullptr, true},
        {"np_uint64_max", std::nullopt, "give() returned 18446744073709551615 (past a 64-bit integer)", true},
        {"np_float32", real(0.5), nullptr, true},
        {"np_float64", real(0.25), nullptr, true},
        {"np_array", list({1.0, 2.0}), nullptr, true},
        {"np_int_array", list({std::int64_t{1}, std::int64_t{2}}), nullptr, true},
        {"np_bool_array", list({std::int64_t{1}, std::int64_t{0}}), nullptr, true},
        {"np_str_array", message("a", {std::string{"b"}}), nullptr, true},
        {"np_0d", real(5.0), nullptr, true},
        {"np_0d_int", number(3), nullptr, true},
        {"np_2d", std::nullopt, "give() returned a 2-D np.ndarray, which an outlet cannot carry (a 1-D one is a list)",
         true},
        {"np_empty", std::nullopt, nullptr, true},
        {"np_in_list", list({0.5, std::int64_t{3}, std::int64_t{0}}), nullptr, true},
        {"array_in_list", std::nullopt, "give() returned a list holding a numpy.ndarray (item 1)", true},
    };

} // namespace

// ---- what a method returns, as the outlets output it ---------------------------------------------

SCENARIO("What a method returns is output by the table's rules (plan 9.1)") {
    ensure_runtime();
    const bool  numpy = numpy_importable();
    log_capture log;
    processor   p{"outputs", log.sink(), {}, {}, {}, false};
    REQUIRE(p.load());

    for (const auto& r : k_rows) {
        if (r.numpy && !numpy) {
            continue;
        }
        INFO("case " << r.name);
        log.clear();
        const auto out = p.call_with_output("give", word(r.name));
        if (r.error) {
            CHECK(out.empty());
            CHECK(log.contains(r.error, log_level::error));
        }
        else if (r.item) {
            CHECK(out == output{*r.item});
            CHECK(log.lines().empty());
        }
        else {
            CHECK(out.empty());
            CHECK(log.lines().empty());
        }
    }

    THEN("a non-finite float passes through, as the atom Max can carry") {
        const auto out = p.call_with_output("give", word("nan"));
        REQUIRE(out.size() == 1);
        REQUIRE(out[0]);
        CHECK_FALSE(out[0]->selector);
        CHECK(std::isnan(std::get<double>(out[0]->atoms.at(0))));
        if (numpy) {
            const auto from_numpy = p.call_with_output("give", word("np_nan"));
            REQUIRE(from_numpy.size() == 1);
            CHECK(std::isnan(std::get<double>(from_numpy[0]->atoms.at(0))));
        }
    }
}

SCENARIO("A return hinted tuple[...] outputs one value per outlet; an unhinted tuple is a list") {
    ensure_runtime();
    log_capture log;
    processor   p{"outputs", log.sink(), {}, {}, {}, false};
    REQUIRE(p.load());

    CHECK(p.call_with_output("two", word("pair")) == output{number(60), symbol("C")});
    CHECK(p.call_with_output("two", word("list_pair")) == output{number(60), symbol("C")});
    CHECK(p.call_with_output("two", word("slot_list"))
          == output{list({std::int64_t{1}, std::int64_t{2}}), symbol("x")});
    CHECK(p.call_with_output("one", word("one")) == output{number(5)});
    CHECK(p.call_with_output("give", word("pair")) == output{list({std::int64_t{60}, std::string{"C"}})});
    CHECK(p.call_with_output("give", word("tuple")) == output{list({std::int64_t{1}, std::int64_t{2}})});

    THEN("None in a slot outputs nothing from that outlet, and None in every slot nothing at all") {
        CHECK(p.call_with_output("two", word("none_slot")) == output{std::nullopt, symbol("x")});
        CHECK(p.call_with_output("two", word("all_none")).empty());
    }
    THEN("a result that is not that many values is reported, and nothing is output") {
        CHECK(p.call_with_output("two", word("three")).empty());
        CHECK(
            log.contains("two() is hinted to return 2 values, one per outlet, but returned 3 value(s) — nothing output",
                         log_level::error));
        CHECK(p.call_with_output("two", word("int")).empty());
        CHECK(
            log.contains("two() is hinted to return 2 values, one per outlet, but returned an int", log_level::error));
        CHECK(p.call_with_output("one", word("int")).empty());
        CHECK(log.contains("one() is hinted to return 1 value, one per outlet, but returned an int", log_level::error));
    }
    THEN("a value in a slot that does not convert is reported, and nothing is output") {
        CHECK(p.call_with_output("two", word("slot_dict")).empty());
        CHECK(log.contains("two() returned a dict, which an outlet cannot carry as its value 2", log_level::error));
    }
    THEN("a tuple hint that does not say how many values outputs a list from one outlet") {
        CHECK(p.call_with_output("unsaid", word("tuple")) == output{list({std::int64_t{1}, std::int64_t{2}})});
    }
}

SCENARIO("A method that raises outputs nothing, and its traceback is printed") {
    ensure_runtime();
    console().clear();
    processor p{"outputs", {}, {}, {}, {}, false};
    REQUIRE(p.load());
    CHECK(p.call_with_output("boom", {}).empty());
    CHECK(console().contains("boom() raised on purpose", log_level::error));
    CHECK(p.call_with_output("no_such_method", {}).empty());
}

// ---- how many outlets -------------------------------------------------------------------------------

SCENARIO("The outlet count is the widest return hint, as an object or as a string (plan 9.1, audit m4)") {
    ensure_runtime();
    const auto* name = GENERATE("wide", "wide_strings");
    processor   p{name, {}, {}, {}, {}, false};
    REQUIRE(p.load());

    const auto  messages = p.messages();
    const auto* nested   = find_message(messages, "nested");
    const auto* triple   = find_message(messages, "triple");
    const auto* single   = find_message(messages, "single");
    REQUIRE(nested);
    REQUIRE(triple);
    REQUIRE(single);
    CHECK(nested->return_count == 2); // tuple[list[int], dict[str, int]]: two, however it is written
    CHECK(nested->spreads);
    CHECK(triple->return_count == 3);
    CHECK(single->return_count == 1);
    CHECK_FALSE(single->spreads);
    CHECK(p.outlet_count() == 3);
    CHECK(p.call_with_output("nested", {}) == output{list({std::int64_t{1}, std::int64_t{2}}), number(3)});
}

SCENARIO("An object without audio announces what it bound: its messages and outlets") {
    ensure_runtime();
    log_capture log;
    processor   p{"outputs", log.sink(), {}, {}, {}, false};
    forget_loaded("outputs"); // this load runs the file, so logs what is true of the class
    REQUIRE(p.load());
    CHECK(p.outlet_count() == 2);
    CHECK(log.contains("Loaded outputs.py: 5 messages, 2 outlets", log_level::info));
    CHECK(log.contains("unsaid() is hinted to return a tuple without saying how many values, so it is output as a list "
                       "from one outlet",
                       log_level::info));

    AND_WHEN("the same file is loaded by an audio object") {
        log_capture audio_log;
        processor   audio{"outputs", audio_log.sink()};
        forget_loaded("outputs");
        REQUIRE(audio.load());
        THEN("it says nothing of tuples, whose returns it drops") {
            CHECK_FALSE(audio_log.contains("unsaid()", log_level::info));
        }
    }
}

// ---- the audio option -----------------------------------------------------------------------------

SCENARIO("Without bind_audio, process() and prepare() are messages like any other (plan 9.1)") {
    ensure_runtime();
    processor p{"gain", {}, {}, {}, {}, false};
    REQUIRE(p.load());
    CHECK_FALSE(p.binds_audio());
    CHECK_FALSE(p.has_process());
    CHECK(find_message(p.messages(), "process") != nullptr);
    CHECK(p.call_with_output("process", std::vector<value>{0.5}) == output{real(0.5)});
    CHECK(all_equal(render(p, 1.0), 0.0)); // no audio callback: silence

    AND_WHEN("the class has a prepare()") {
        processor prepared{"prepared", {}, {}, {}, {}, false};
        REQUIRE(prepared.load());
        prepared.prepare(48000.0, 64); // nothing to tell: it is a message
        CHECK(prepared.call_with_output("process", std::vector<value>{0.0}) == output{real(0.0)});
        CHECK(prepared.call_with_output("prepare", std::vector<value>{44100.0, std::int64_t{32}}).empty());
        CHECK(prepared.call_with_output("process", std::vector<value>{0.0}) == output{real(44100.0)});
    }
    AND_WHEN("an audio object loads the same class") {
        processor audio{"gain"};
        REQUIRE(audio.load());
        THEN("process() is its audio callback, as before") {
            CHECK(audio.binds_audio());
            CHECK(audio.has_process());
            CHECK(find_message(audio.messages(), "process") == nullptr);
            CHECK(all_equal(render(audio, 0.5), 0.5));
        }
    }
}

// ---- what each kind of object is told ---------------------------------------------------------------

SCENARIO("Objects of two kinds sharing a file are each told once what is true of the class as they bind it "
         "(plan 9.1, audit M5)") {
    ensure_runtime();
    const bool control_first = GENERATE(true, false);
    forget_loaded("kinds");
    log_capture control_log;
    log_capture audio_log;
    processor   control{"kinds", control_log.sink(), {}, {}, {}, false};
    processor   audio{"kinds", audio_log.sink(), {"dsp"}};
    if (control_first) {
        REQUIRE(control.load()); // runs the file
        REQUIRE(audio.load());   // shares that execution
    }
    else {
        REQUIRE(audio.load());
        REQUIRE(control.load());
    }

    THEN("the Loaded line is said once, by the object that ran the file, for what it bound") {
        auto& runner = control_first ? control_log : audio_log;
        auto& other  = control_first ? audio_log : control_log;
        CHECK(runner.contains(control_first ? "Loaded kinds.py: 3 messages, 1 outlet"
                                            : "Loaded kinds.py: process() bound, one call per sample",
                              log_level::info));
        CHECK_FALSE(other.contains("Loaded kinds.py", log_level::info));
    }
    THEN("each kind says what its own binding owes, whichever ran the file") {
        CHECK(audio_log.contains("dsp() is reserved by the host", log_level::error));
        CHECK_FALSE(control_log.contains("dsp() is reserved", log_level::error));
        CHECK(control_log.contains("spread() is hinted to return a tuple without saying how many values",
                                   log_level::info));
        CHECK_FALSE(audio_log.contains("spread()", log_level::info));
    }
    THEN("another object of either kind on the same save says none of it again") {
        log_capture again;
        processor   second_control{"kinds", again.sink(), {}, {}, {}, false};
        processor   second_audio{"kinds", again.sink(), {"dsp"}};
        REQUIRE(second_control.load());
        REQUIRE(second_audio.load());
        CHECK(again.lines().empty());
    }
}

// ---- list parameters ------------------------------------------------------------------------------

// Before 9.1 a parameter hinted list[float] was not a list to the bridge: it took one atom, coerced to
// a symbol — a number became the empty symbol, as atom_getsym() says — and more atoms were refused.
// That changed for tap.python~'s messages too: recorded in CHANGELOG.md (2.0.0).
SCENARIO("A last message parameter hinted list[...] takes the atoms left after the others (plan 9.1)") {
    ensure_runtime();
    const bool  bind_audio = GENERATE(true, false); // tap.python~ and tap.python alike
    log_capture log;
    processor   p{"list_params", log.sink(), {}, {}, {}, bind_audio};
    REQUIRE(p.load());

    REQUIRE(p.call("take", std::vector<value>{1.5}));
    CHECK(got(p) == "[1.5]");
    REQUIRE(p.call("take", std::vector<value>{1.0, std::int64_t{2}, 3.0}));
    CHECK(got(p) == "[1.0, 2.0, 3.0]");
    REQUIRE(p.call("take", std::vector<value>{}));
    CHECK(got(p) == "[]");

    THEN("each atom converts to the list's element hint, as a single parameter's would") {
        REQUIRE(p.call("ints", std::vector<value>{1.7, std::int64_t{2}}));
        CHECK(got(p) == "[1, 2]");
        REQUIRE(p.call("words", std::vector<value>{std::string{"a"}, std::int64_t{1}}));
        CHECK(got(p) == "['a', '']");
        REQUIRE(p.call("anything_list", std::vector<value>{std::int64_t{1}, 2.5, std::string{"x"}}));
        CHECK(got(p) == "[1, 2.5, 'x']");
        REQUIRE(p.call("written", std::vector<value>{4.0, std::int64_t{5}}));
        CHECK(got(p) == "[4, 5]");
    }
    THEN("the parameters before it take one atom each, and it the rest") {
        REQUIRE(p.call("after", std::vector<value>{std::int64_t{1}}));
        CHECK(got(p) == "(1, [])");
        REQUIRE(p.call("after", std::vector<value>{std::int64_t{1}, std::int64_t{2}, std::int64_t{3}}));
        CHECK(got(p) == "(1, [2.0, 3.0])");
        CHECK_FALSE(p.call("after", std::vector<value>{}));
        CHECK(log.contains("after: expected at least 1 argument(s), got 0", log_level::error));
    }
    THEN("with a default and no atoms left, it keeps its default") {
        REQUIRE(p.call("optional", std::vector<value>{}));
        CHECK(got(p) == "None");
        REQUIRE(p.call("optional", std::vector<value>{std::int64_t{1}}));
        CHECK(got(p) == "[1.0]");
    }
    THEN("a list parameter followed by *args is not last, and takes one atom as before") {
        REQUIRE(p.call("not_last", std::vector<value>{std::int64_t{1}, std::int64_t{2}}));
        CHECK(got(p) == "('', (2,))");
    }
}

SCENARIO("A last message parameter hinted np.ndarray takes them as a float64 array: a list in, a list out") {
    ensure_runtime();
    if (!numpy_importable()) {
        SKIP("numpy is not importable by the embedded interpreter");
    }
    processor p{"array_params", {}, {}, {}, {}, false};
    REQUIRE(p.load());
    REQUIRE(p.call("take", std::vector<value>{std::int64_t{1}, 2.5}));
    CHECK(got(p) == "('ndarray', 'float64', [1.0, 2.5])");
    REQUIRE(p.call("take", std::vector<value>{}));
    CHECK(got(p) == "('ndarray', 'float64', [])");
    CHECK(p.call_with_output("scaled", std::vector<value>{2.0, std::int64_t{1}, std::int64_t{2}, 3.0})
          == output{list({2.0, 4.0, 6.0})});
}

// ---- threads --------------------------------------------------------------------------------------

SCENARIO("A message called from another thread while the class reloads outputs one binding's result or the other's") {
    ensure_runtime();
    const std::string narrow = "class race_output:\n"
                               "    def value(self) -> int:\n"
                               "        return 3\n";
    const std::string wide   = "class race_output:\n"
                               "    def value(self) -> tuple[int, int]:\n"
                               "        return 1, 2\n";
    write_script("race_output", narrow);
    processor p{"race_output", {}, {}, {}, {}, false};
    REQUIRE(p.load());

    std::atomic<bool> stop{false};
    std::atomic<int>  calls{0};
    std::atomic<int>  unexpected{0};
    std::thread       caller{[&] {
        while (!stop.load()) {
            const auto out = p.call_with_output("value", {});
            if (out != output{number(3)} && out != output{number(1), number(2)}) {
                ++unexpected;
            }
            ++calls;
        }
    }};
    int               reloads = 0;
    for (int i = 0; i < 40; ++i) {
        write_script("race_output", i % 2 == 0 ? wide : narrow);
        reloads += p.load() ? 1 : 0;
        CHECK(p.outlet_count() == (i % 2 == 0 ? 2u : 1u));
    }
    stop = true;
    caller.join();
    CHECK(reloads == 40);
    CHECK(calls.load() > 0);
    CHECK(unexpected.load() == 0);
}
