/// @file test_processor.cpp
/// @brief Loading a class, describing it, dispatching to it, processing audio, and every failure path.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    const attribute_info* find_attribute(const processor& p, const std::string& name) {
        for (const auto& a : p.attributes()) {
            if (a.name == name) {
                return &a;
            }
        }
        return nullptr;
    }

    bool has_message(const processor& p, const std::string& name) {
        for (const auto& m : p.messages()) {
            if (m.name == name) {
                return true;
            }
        }
        return false;
    }

    std::vector<value_type> message_types(const processor& p, const std::string& name) {
        for (const auto& m : p.messages()) {
            if (m.name == name) {
                return m.argument_types;
            }
        }
        return {};
    }

} // namespace

SCENARIO("An unloaded processor outputs silence and ignores everything") {
    ensure_runtime();
    processor p{"gain"};

    CHECK_FALSE(p.loaded());
    CHECK_FALSE(p.has_process());
    CHECK(all_equal(render(p, 0.5), 0.0));
    CHECK_FALSE(p.set_attribute("level", 0.5));
    CHECK_FALSE(p.get_attribute("level", value_type::real).has_value());
    CHECK_FALSE(p.call("set_level", std::vector<value>{0.5}));
}

SCENARIO("Loading a class describes its attributes and messages") {
    ensure_runtime();
    log_capture log;
    processor   p{"gain", log.sink()};

    REQUIRE(p.load());
    CHECK(p.loaded());
    CHECK(p.has_process());
    CHECK_FALSE(p.block_mode()); // process(self, x: float): once per sample

    THEN("annotated public fields become attributes, typed by their hints, in declaration order") {
        const auto& attributes = p.attributes();
        REQUIRE(attributes.size() == 3);
        CHECK(attributes[0].name == "level");
        CHECK(attributes[0].type == value_type::real);
        CHECK(attributes[1].name == "count");
        CHECK(attributes[1].type == value_type::integer);
        CHECK(attributes[2].name == "label");
        CHECK(attributes[2].type == value_type::symbol);
        CHECK(find_attribute(p, "_private") == nullptr);
    }

    THEN("public methods other than process() become messages, sorted by name") {
        std::vector<std::string> names;
        for (const auto& m : p.messages()) {
            names.push_back(m.name);
        }
        CHECK(names == std::vector<std::string>{"bump", "fail", "pair", "rename", "say", "set_level"});
        CHECK_FALSE(has_message(p, "process"));
        CHECK_FALSE(has_message(p, "_hidden"));
    }

    THEN("message argument types follow the parameter hints, in order") {
        CHECK(message_types(p, "set_level") == std::vector<value_type>{value_type::real});
        CHECK(message_types(p, "bump") == std::vector<value_type>{value_type::integer});
        CHECK(message_types(p, "rename") == std::vector<value_type>{value_type::symbol});
        CHECK(message_types(p, "pair") == std::vector<value_type>{value_type::integer, value_type::real});
        CHECK(message_types(p, "fail").empty());
    }
}

SCENARIO("process() runs the class's process() once per sample") {
    ensure_runtime();
    processor p{"gain"};
    REQUIRE(p.load());

    CHECK(all_equal(render(p, 0.5), 0.5));

    WHEN("the input and output buffers alias") {
        std::vector<double> buffer{0.1, 0.2, 0.3};
        REQUIRE(p.set_attribute("level", 2.0));
        p.process(buffer.data(), buffer.data(), buffer.size());
        THEN("each sample is read before it is overwritten") {
            CHECK(buffer == std::vector<double>{0.2, 0.4, 0.6});
        }
    }
}

SCENARIO("Attributes round-trip through the instance") {
    ensure_runtime();
    processor p{"gain"};
    REQUIRE(p.load());

    REQUIRE(p.set_attribute("level", 0.25));
    CHECK(std::get<double>(*p.get_attribute("level", value_type::real)) == 0.25);
    CHECK(all_equal(render(p, 1.0), 0.25));

    REQUIRE(p.set_attribute("count", std::int64_t{7}));
    CHECK(std::get<std::int64_t>(*p.get_attribute("count", value_type::integer)) == 7);

    REQUIRE(p.set_attribute("label", std::string{"loud"}));
    CHECK(std::get<std::string>(*p.get_attribute("label", value_type::symbol)) == "loud");

    WHEN("an attribute does not exist on the instance") {
        THEN("reading it reports nothing") {
            CHECK_FALSE(p.get_attribute("missing", value_type::real).has_value());
        }
    }

    WHEN("an attribute holds a value that does not convert to the requested type") {
        REQUIRE(p.set_attribute("label", std::string{"text"}));
        THEN("it reads as nothing — never an error sentinel like -1") {
            CHECK_FALSE(p.get_attribute("label", value_type::real).has_value());
        }
    }
}

SCENARIO("Messages call the bound methods with coerced arguments") {
    ensure_runtime();
    log_capture log;
    processor   p{"gain", log.sink()};
    REQUIRE(p.load());

    WHEN("a float message is sent") {
        REQUIRE(p.call("set_level", std::vector<value>{0.5}));
        THEN("the method ran") {
            CHECK(all_equal(render(p, 1.0), 0.5));
        }
    }

    WHEN("an int argument arrives as a real") {
        REQUIRE(p.call("bump", std::vector<value>{2.9}));
        THEN("it is truncated, as atom_getlong does") {
            CHECK(std::get<std::int64_t>(*p.get_attribute("count", value_type::integer)) == 2);
        }
    }

    WHEN("several arguments are sent") {
        REQUIRE(p.call("pair", std::vector<value>{std::int64_t{3}, std::int64_t{2}}));
        THEN("each is coerced to its own parameter's type") {
            CHECK(std::get<std::int64_t>(*p.get_attribute("count", value_type::integer)) == 3);
            CHECK(all_equal(render(p, 1.0), 2.0));
        }
    }

    WHEN("a symbol argument is sent") {
        REQUIRE(p.call("rename", std::vector<value>{std::string{"quiet"}}));
        THEN("it arrives as a str") {
            CHECK(std::get<std::string>(*p.get_attribute("label", value_type::symbol)) == "quiet");
        }
    }

    WHEN("a method prints") {
        console().clear();
        REQUIRE(p.call("say", std::vector<value>{std::string{"hi"}}));
        THEN("the output reaches the console") {
            CHECK(console().contains("say: hi", log_level::info));
        }
    }

    WHEN("the argument count is wrong") {
        CHECK_FALSE(p.call("set_level", std::vector<value>{}));
        THEN("the call is refused with a diagnostic and has no effect") {
            CHECK(log.contains("set_level: expected 1 argument(s), got 0", log_level::error));
            CHECK(all_equal(render(p, 1.0), 1.0));
        }
    }

    WHEN("the message does not exist") {
        THEN("the call is refused silently") {
            CHECK_FALSE(p.call("nonexistent", std::vector<value>{}));
            CHECK_FALSE(p.call("process", std::vector<value>{1.0}));
        }
    }

    WHEN("the method raises") {
        console().clear();
        CHECK_FALSE(p.call("fail", std::vector<value>{}));
        THEN("the traceback reaches the console and the processor stays loaded") {
            CHECK(console().contains("RuntimeError: fail() was called", log_level::error));
            CHECK(p.loaded());
            CHECK(all_equal(render(p, 1.0), 1.0));
        }
    }
}

SCENARIO("A source file that does not exist leaves the processor unloaded") {
    ensure_runtime();
    log_capture log;
    processor   p{"does_not_exist", log.sink()};

    CHECK_FALSE(p.load());
    CHECK(log.contains("No file " + (scripts_dir() / "does_not_exist.py").string(), log_level::error));
    CHECK(all_equal(render(p, 1.0), 0.0));
}

SCENARIO("A module without the named class leaves the processor unloaded") {
    ensure_runtime();
    log_capture log;
    processor   p{"wrong_class", log.sink()};

    CHECK_FALSE(p.load());
    CHECK(log.contains("No class named 'wrong_class' in wrong_class.py", log_level::error));
    CHECK(all_equal(render(p, 1.0), 0.0));
}

SCENARIO("A class whose constructor raises leaves the processor unloaded") {
    ensure_runtime();
    log_capture log;
    processor   p{"bad_constructor", log.sink()};

    console().clear();
    CHECK_FALSE(p.load());
    CHECK(log.contains("Failed to instantiate the Python class bad_constructor", log_level::error));
    CHECK(console().contains("RuntimeError: constructor failed on purpose", log_level::error));
    CHECK(all_equal(render(p, 1.0), 0.0));
}

SCENARIO("A class without process() is loaded but silent") {
    ensure_runtime();
    processor p{"no_process"};

    REQUIRE(p.load());
    CHECK_FALSE(p.has_process());
    CHECK(p.attributes().size() == 1);
    CHECK(has_message(p, "poke"));
    CHECK(all_equal(render(p, 1.0), 0.0));
}

SCENARIO("process() raising silences the rest of the vector and unbinds until the next load") {
    ensure_runtime();
    log_capture      log;
    std::atomic<int> notified{0};
    processor        p{"raises_in_process", log.sink(), {}, [&] { ++notified; }};
    REQUIRE(p.load());
    log.clear(); // drop load()'s own "Loaded" line

    console().clear();
    const auto out = render(p, 0.5, 8);

    THEN("the samples before the exception pass, the rest are silent") {
        CHECK(out == std::vector<double>{0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    }
    THEN("nothing is printed on the audio thread; the host is told a report is waiting") {
        CHECK(console().lines().empty());
        CHECK(log.lines().empty());
        CHECK(notified.load() == 1);
    }
    THEN("flushing the reports prints the traceback and a diagnostic, once") {
        p.flush_reports();
        CHECK(console().contains("ValueError: process() failed on purpose", log_level::error));
        CHECK(log.contains("process() raised an exception", log_level::error));
        log.clear();
        p.flush_reports();
        CHECK(log.lines().empty());
    }
    THEN("later vectors are silent without calling Python again") {
        CHECK_FALSE(p.has_process());
        CHECK(p.loaded());
        CHECK(all_equal(render(p, 0.5), 0.0));
    }
    THEN("a reload re-arms it") {
        REQUIRE(p.load());
        CHECK(p.has_process());
        CHECK(all_equal(render(p, 0.5, 2), 0.5));
    }
}

SCENARIO("process() returning a tuple of unsaid length is not bound") {
    ensure_runtime();
    log_capture log;
    processor   p{"tuple_return", log.sink()};

    REQUIRE(p.load());
    CHECK_FALSE(p.has_process());
    CHECK(log.contains("process() returns a tuple without saying how many values", log_level::error));
    CHECK(all_equal(render(p, 1.0), 0.0));
}

// The reload scenarios are linear (no sibling sections): Catch2 re-runs a scenario once per
// section, and each re-run would start from the file the previous one left behind.

SCENARIO("load() reloads the module from disk") {
    ensure_runtime();
    write_script("reload_changes", "class reload_changes:\n"
                                   "    level: float = 2.0\n"
                                   "    def process(self, x: float) -> float:\n"
                                   "        return x * self.level\n");
    processor p{"reload_changes"};
    REQUIRE(p.load());
    REQUIRE(p.set_attribute("level", 5.0));
    CHECK(all_equal(render(p, 1.0), 5.0));

    write_script("reload_changes", "class reload_changes:\n"
                                   "    level: float = 3.0\n"
                                   "    width: int = 1\n"
                                   "    def process(self, x: float) -> float:\n"
                                   "        return -x * self.level\n"
                                   "    def poke(self) -> None:\n"
                                   "        pass\n");
    REQUIRE(p.load());

    // the new class runs, with the value set from outside carried over (3.4) rather than the new
    // default of 3.0
    CHECK(all_equal(render(p, 1.0), -5.0));
    // the attributes and messages describe the new class
    REQUIRE(p.attributes().size() == 2);
    CHECK(p.attributes()[1].name == "width");
    CHECK(has_message(p, "poke"));
}

SCENARIO("A reload that fails goes silent until the source is fixed") {
    ensure_runtime();
    log_capture log;
    write_script("reload_breaks", "class reload_breaks:\n"
                                  "    def process(self, x: float) -> float:\n"
                                  "        return x\n");
    processor p{"reload_breaks", log.sink()};
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 1.0), 1.0));

    write_script("reload_breaks", "class reload_breaks:\n    def process(self, x: float) -> float\n");
    console().clear();
    CHECK_FALSE(p.load());
    CHECK(log.contains("Failed to load " + (scripts_dir() / "reload_breaks.py").string(), log_level::error));
    CHECK(console().contains("SyntaxError", log_level::error));
    CHECK_FALSE(p.loaded());
    CHECK(all_equal(render(p, 1.0), 0.0));

    write_script("reload_breaks", "class reload_breaks:\n"
                                  "    def process(self, x: float) -> float:\n"
                                  "        return x * 4.0  # fixed\n");
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 1.0), 4.0));
}

SCENARIO("A new processor after an edit runs the edited file") {
    // Fixed by file-based loading (3.5); it used to get the module already in sys.modules: in Max,
    // delete every instance of a script, edit it, create a new instance — and run the old code.
    ensure_runtime();
    write_script("cached_module", "class cached_module:\n"
                                  "    def process(self, x: float) -> float:\n"
                                  "        return x\n");
    {
        processor first{"cached_module"};
        REQUIRE(first.load());
    }

    write_script("cached_module", "class cached_module:\n"
                                  "    def process(self, x: float) -> float:\n"
                                  "        return x * 10.0  # edited\n");
    processor second{"cached_module"};
    REQUIRE(second.load());
    CHECK(all_equal(render(second, 1.0), 10.0));
}

SCENARIO("A file's load is announced once, by the processor that ran it (plan 6.7)") {
    ensure_runtime();
    // dsp64 is reserved by the host: a diagnostic about the class, not about one instance
    write_script("announce_once", "class announce_once:\n"
                                  "    def dsp64(self) -> None:\n"
                                  "        pass\n\n"
                                  "    def process(self, x: float) -> float:\n"
                                  "        return x\n");
    log_capture first_log;
    log_capture second_log;
    processor   first{"announce_once", first_log.sink(), {"dsp64"}};
    processor   second{"announce_once", second_log.sink(), {"dsp64"}};
    REQUIRE(first.load());
    REQUIRE(second.load());

    THEN("the processor that ran the file says so once, with how process() is called; the other is quiet") {
        CHECK(first_log.contains("Loaded announce_once.py: process() bound, one call per sample", log_level::info));
        CHECK(first_log.contains("dsp64() is reserved", log_level::error));
        CHECK(second_log.lines().empty());
    }
    WHEN("the file changes, and the other processor reloads first") {
        first_log.clear();
        second_log.clear();
        write_script("announce_once", "import numpy as np\n\n"
                                      "class announce_once:\n"
                                      "    def process(self, x: np.ndarray) -> np.ndarray:\n"
                                      "        return x\n");
        REQUIRE(second.load());
        REQUIRE(first.load());
        THEN("that one announces the new class") {
            CHECK(second_log.contains("Loaded announce_once.py: process() bound, one call per vector (numpy)",
                                      log_level::info));
            CHECK(first_log.lines().empty());
            CHECK(first.block_mode());
            CHECK(second.block_mode());
        }
    }
    WHEN("both reload a file that did not change") {
        first_log.clear();
        second_log.clear();
        REQUIRE(first.load());
        REQUIRE(second.load());
        THEN("neither says anything") {
            CHECK(first_log.lines().empty());
            CHECK(second_log.lines().empty());
        }
    }
}

SCENARIO("A file that fails to load is reported once per save, not once per object (plan 6.10)") {
    ensure_runtime();
    write_script("broken_once", "class broken_once:\n"
                                "    def process(self, x: float) -> float:\n"
                                "        return x\n\n"
                                "def (\n");
    forget_loaded("broken_once");
    const auto syntax_errors = [] {
        const auto lines = console().lines();
        return std::count_if(lines.begin(), lines.end(), [](const log_capture::line& l) {
            return l.level == log_level::error && l.text.find("SyntaxError:") != std::string::npos;
        });
    };
    log_capture first_log;
    log_capture second_log;
    processor   first{"broken_once", first_log.sink()};
    processor   second{"broken_once", second_log.sink()};
    console().clear();
    CHECK_FALSE(first.load());
    CHECK_FALSE(second.load());

    THEN("the first to load it says why, once; the other is quiet — and silent") {
        CHECK(first_log.contains("Failed to load", log_level::error));
        CHECK(second_log.lines().empty());
        CHECK(syntax_errors() == 1);
        CHECK(all_equal(render(second, 0.5), 0.0));
    }
    WHEN("the file is saved broken another way") {
        first_log.clear();
        second_log.clear();
        console().clear();
        write_script("broken_once", "class broken_once:\n    pass\n\n)\n");
        CHECK_FALSE(second.load());
        CHECK_FALSE(first.load());
        THEN("that save is reported, once") {
            CHECK(second_log.contains("Failed to load", log_level::error));
            CHECK(first_log.lines().empty());
            CHECK(syntax_errors() == 1);
        }
    }
    WHEN("an object is made later, with the file still broken") {
        const auto set_window = [](const double seconds) { // "later": past the loader's window
            gil_lock  lock;
            PyObject* value = PyFloat_FromDouble(seconds);
            PyDict_SetItemString(detail::support_globals(), "_REPORT_WINDOW", value);
            Py_DECREF(value);
        };
        set_window(0.0);
        log_capture later_log;
        processor   later{"broken_once", later_log.sink()};
        CHECK_FALSE(later.load());
        set_window(2.0);
        THEN("it says why it is silent") {
            CHECK(later_log.contains("Failed to load", log_level::error));
        }
    }
}
