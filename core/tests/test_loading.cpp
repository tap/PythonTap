/// @file test_loading.cpp
/// @brief Phase 3a of the production plan: file-based loading, shared reloads, and attribute
///        values that survive a reload.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <cstdint>
#include <string>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    std::size_t count_lines(const std::string& fragment) {
        std::size_t n = 0;
        for (const auto& line : console().lines()) {
            if (line.text.find(fragment) != std::string::npos) {
                ++n;
            }
        }
        return n;
    }

} // namespace

// 3.5 — the source is <scripts_dir>/<name>.py, loaded by path

SCENARIO("A class file named like a standard-library module neither shadows it nor is shadowed by it") {
    ensure_runtime();
    processor p{"json"};
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 1.0), 7.0)); // our json.py
    CHECK(run("import json\nassert hasattr(json, 'dumps'), 'the standard library was shadowed'"));
}

SCENARIO("The source name must name a file in the script folder") {
    ensure_runtime();
    log_capture log;

    WHEN("it names a module that exists only on sys.path") {
        processor p{"os", log.sink()};
        CHECK_FALSE(p.load()); // not the standard library's os
        CHECK(log.contains("No file " + (scripts_dir() / "os.py").string(), log_level::error));
    }
    WHEN("it is not a Python identifier") {
        for (const char* name : {"../gain", "two words", "", "1abc", "gain.py"}) {
            processor p{name, log.sink()};
            CHECK_FALSE(p.load());
        }
        CHECK(log.contains("is not a valid Python name", log_level::error));
    }
}

// 3.6 / 3.6a — one execution per save, whoever loads it; no stale bytecode

SCENARIO("Every instance of one file shares a single execution per save") {
    ensure_runtime();
    write_script("shared_module", "print('executing shared_module v1')\n"
                                  "class shared_module:\n"
                                  "    def process(self, x: float) -> float:\n"
                                  "        return 1.0\n");
    console().clear();
    processor a{"shared_module"};
    processor b{"shared_module"};
    REQUIRE(a.load());
    REQUIRE(b.load());
    CHECK(count_lines("executing shared_module") == 1);

    WHEN("the file is saved, and every instance's watcher reloads it") {
        write_script("shared_module", "print('executing shared_module v2')\n"
                                      "class shared_module:\n"
                                      "    def process(self, x: float) -> float:\n"
                                      "        return 2.0\n");
        console().clear();
        REQUIRE(a.load());
        REQUIRE(b.load());
        THEN("it runs once, and both run the new code") {
            CHECK(count_lines("executing shared_module v2") == 1);
            CHECK(all_equal(render(a, 0.0), 2.0));
            CHECK(all_equal(render(b, 0.0), 2.0));
        }
    }
}

SCENARIO("A second save within the same second, of the same size, is still picked up") {
    // the source loader's .pyc check is mtime (1 s) + size; these two versions are the same size
    // and are written back to back
    ensure_runtime();
    write_script("quick_save", "class quick_save:\n"
                               "    def process(self, x: float) -> float:\n"
                               "        return 2.0\n");
    processor p{"quick_save"};
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 0.0), 2.0));

    write_script("quick_save", "class quick_save:\n"
                               "    def process(self, x: float) -> float:\n"
                               "        return 3.0\n");
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 0.0), 3.0));
}

SCENARIO("A reload starts from a fresh module: names deleted from the file are gone") {
    ensure_runtime();
    write_script("fresh_module", "OFFSET = 1.0\n"
                                 "class fresh_module:\n"
                                 "    def process(self, x: float) -> float:\n"
                                 "        return globals().get('OFFSET', 0.0)\n");
    processor p{"fresh_module"};
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 0.0), 1.0));

    write_script("fresh_module", "class fresh_module:\n"
                                 "    def process(self, x: float) -> float:\n"
                                 "        return globals().get('OFFSET', 0.0)  # OFFSET deleted\n");
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 0.0), 0.0)); // importlib.reload would have kept OFFSET = 1.0
}

// 3.4 — attribute values survive a reload

SCENARIO("Attribute values carry over a reload") {
    ensure_runtime();
    log_capture log;
    write_script("carried", "class carried:\n"
                            "    level: float = 1.0\n"
                            "    steps: int = 1\n"
                            "    gone: float = 1.0\n"
                            "    def process(self, x: float) -> float:\n"
                            "        return self.level\n");
    processor p{"carried", log.sink()};
    REQUIRE(p.load());
    REQUIRE(p.set_attribute("level", 5.0));
    REQUIRE(p.set_attribute("steps", std::int64_t{4}));

    write_script("carried", "class carried:\n"
                            "    level: float = 2.0\n"
                            "    steps: float = 0.5\n"
                            "    added: int = 9\n"
                            "    def process(self, x: float) -> float:\n"
                            "        return self.level\n");
    REQUIRE(p.load());

    THEN("a value set from outside wins over the new default") {
        CHECK(std::get<double>(*p.get_attribute("level", value_type::real)) == 5.0);
        CHECK(all_equal(render(p, 0.0), 5.0));
    }
    THEN("an attribute whose type changed starts from its new default, with a note") {
        CHECK(std::get<double>(*p.get_attribute("steps", value_type::real)) == 0.5);
        CHECK(log.contains("attribute 'steps' changed type", log_level::info));
    }
    THEN("the description matches the new class: removed attributes are gone, new ones present") {
        std::vector<std::string> names;
        for (const auto& a : p.attributes()) {
            names.push_back(a.name);
        }
        CHECK(names == std::vector<std::string>{"level", "steps", "added"});
    }
}

SCENARIO("Attribute values survive a failed reload and come back when it is fixed") {
    ensure_runtime();
    write_script("carried_broken", "class carried_broken:\n"
                                   "    level: float = 1.0\n"
                                   "    def process(self, x: float) -> float:\n"
                                   "        return self.level\n");
    processor p{"carried_broken"};
    REQUIRE(p.load());
    REQUIRE(p.set_attribute("level", 5.0));

    write_script("carried_broken", "class carried_broken:\n    def process(self, x: float) -> float\n");
    CHECK_FALSE(p.load());
    CHECK(all_equal(render(p, 0.0), 0.0));

    write_script("carried_broken", "class carried_broken:\n"
                                   "    level: float = 1.0\n"
                                   "    def process(self, x: float) -> float:\n"
                                   "        return self.level  # fixed\n");
    REQUIRE(p.load());
    CHECK(all_equal(render(p, 0.0), 5.0));
}

SCENARIO("A value the new class rejects is reported, and the new default stays") {
    ensure_runtime();
    log_capture log;
    write_script("carried_rejected", "class carried_rejected:\n"
                                     "    level: float = 1.0\n"
                                     "    def process(self, x: float) -> float:\n"
                                     "        return self.level\n");
    processor p{"carried_rejected", log.sink()};
    REQUIRE(p.load());
    REQUIRE(p.set_attribute("level", 5.0));

    write_script("carried_rejected", "class carried_rejected:\n"
                                     "    level: float = 1.0\n"
                                     "    def __setattr__(self, name, value):\n"
                                     "        if name == 'level' and value > 2.0:\n"
                                     "            raise ValueError('level must be at most 2.0')\n"
                                     "        object.__setattr__(self, name, value)\n"
                                     "    def process(self, x: float) -> float:\n"
                                     "        return self.level\n");
    console().clear();
    REQUIRE(p.load());
    CHECK(log.contains("could not carry attribute 'level' over the reload", log_level::error));
    CHECK(console().contains("ValueError: level must be at most 2.0", log_level::error));
    CHECK(all_equal(render(p, 0.0), 1.0));
}
