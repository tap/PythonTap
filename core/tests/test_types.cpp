/// @file test_types.cpp
/// @brief Phase 3b of the production plan: signature-based dispatch, bool/Optional hints, hint
///        errors, and attribute reads that never invent a value.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <cstdint>
#include <string>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    const message_info* find_message(const std::vector<message_info>& messages, const std::string& name) {
        for (const auto& m : messages) {
            if (m.name == name) {
                return &m;
            }
        }
        return nullptr;
    }

    std::string label(processor& p) {
        return std::get<std::string>(*p.get_attribute("label", value_type::symbol));
    }

    double gain(processor& p) {
        return std::get<double>(*p.get_attribute("gain", value_type::real));
    }

} // namespace

SCENARIO("Field hints map through Optional and X | None, bool is a type of its own, ClassVar is not a field") {
    ensure_runtime();
    console().clear();
    processor p{"typed"};
    REQUIRE(p.load());

    std::vector<std::string> names;
    for (const auto& a : p.attributes()) {
        names.push_back(a.name);
    }
    CHECK(names == std::vector<std::string>{"flag", "gain", "count", "label", "made"});
    CHECK(p.attributes()[0].type == value_type::boolean);
    CHECK(p.attributes()[1].type == value_type::real);    // Optional[float]
    CHECK(p.attributes()[2].type == value_type::integer); // int | None

    THEN("describing the class never runs a property getter") {
        CHECK_FALSE(console().contains("property getter ran", log_level::error));
    }
}

SCENARIO("A bool attribute receives a real bool") {
    ensure_runtime();
    processor p{"typed"};
    REQUIRE(p.load());

    REQUIRE(p.set_attribute("flag", std::int64_t{1}));
    CHECK(std::get<std::int64_t>(*p.get_attribute("flag", value_type::boolean)) == 1);
    REQUIRE(p.call("untyped", std::vector<value>{std::int64_t{0}, std::string{"s"}})); // any: as the atom
    REQUIRE(p.call("set_flag", std::vector<value>{std::int64_t{1}}));
    CHECK(label(p) == "bool");
}

SCENARIO("Optional numeric attributes take numbers") {
    ensure_runtime();
    processor p{"typed"};
    REQUIRE(p.load());
    REQUIRE(p.set_attribute("gain", 0.5));
    CHECK(all_equal(render(p, 1.0), 0.5));
    REQUIRE(p.set_attribute("count", std::int64_t{4}));
    CHECK(std::get<std::int64_t>(*p.get_attribute("count", value_type::integer)) == 4);
}

SCENARIO("Messages are dispatched by the method's signature") {
    ensure_runtime();
    log_capture log;
    processor   p{"typed", log.sink()};
    REQUIRE(p.load());
    const auto messages = p.messages();

    THEN("defaults make arguments optional") {
        const auto* scale = find_message(messages, "scale");
        REQUIRE(scale);
        CHECK(scale->argument_types.size() == 2);
        CHECK(scale->required == 1);
        REQUIRE(p.call("scale", std::vector<value>{3.0}));
        CHECK(gain(p) == 6.0);
        REQUIRE(p.call("scale", std::vector<value>{3.0, 3.0}));
        CHECK(gain(p) == 9.0);
        CHECK_FALSE(p.call("scale", std::vector<value>{}));
        CHECK(log.contains("scale: expected 1 to 2 argument(s), got 0", log_level::error));
        CHECK_FALSE(p.call("scale", std::vector<value>{1.0, 2.0, 3.0}));
    }
    THEN("*args takes any number, each converted to its hint") {
        const auto* many = find_message(messages, "many");
        REQUIRE(many);
        CHECK(many->variadic == value_type::real);
        REQUIRE(p.call("many", std::vector<value>{std::int64_t{1}, 2.5, std::int64_t{3}}));
        CHECK(gain(p) == 6.5);
        REQUIRE(p.call("many", std::vector<value>{}));
        CHECK(gain(p) == 0.0);
    }
    THEN("unannotated parameters take the value as the atom carried it") {
        REQUIRE(p.call("untyped", std::vector<value>{std::int64_t{1}, std::string{"a"}}));
        CHECK(label(p) == "int:str");
        REQUIRE(p.call("untyped", std::vector<value>{0.5, std::int64_t{2}}));
        CHECK(label(p) == "float:int");
    }
    THEN("a keyword-only parameter with a default is left at its default") {
        REQUIRE(p.call("kwonly_ok", std::vector<value>{std::int64_t{4}}));
        CHECK(std::get<std::int64_t>(*p.get_attribute("count", value_type::integer)) == 5);
    }
    THEN("a keyword-only parameter without a default keeps the method from being a message") {
        CHECK(find_message(messages, "kwonly_required") == nullptr);
        CHECK(log.contains("kwonly_required() has a keyword-only parameter 'k' without a default", log_level::error));
    }
    THEN("classmethods and staticmethods are called as themselves") {
        REQUIRE(p.call("make", std::vector<value>{std::string{"by class"}}));
        CHECK(std::get<std::string>(*p.get_attribute("made", value_type::symbol)) == "by class");
        REQUIRE(p.call("helper", std::vector<value>{std::int64_t{2}}));
        CHECK(std::get<std::string>(*p.get_attribute("made", value_type::symbol)) == "helper 2.0");
    }
}

SCENARIO("Attribute reads never invent a value") {
    ensure_runtime();
    processor p{"typed"};
    REQUIRE(p.load());

    WHEN("an Optional attribute holds None") {
        REQUIRE(p.call("clear_gain", std::vector<value>{}));
        THEN("it reads as nothing") {
            CHECK_FALSE(p.get_attribute("gain", value_type::real).has_value());
        }
    }
    WHEN("an int attribute holds a float, and a str attribute a number") {
        REQUIRE(p.call("odd_values", std::vector<value>{}));
        THEN("they convert as the Max types would: truncated, and through str()") {
            CHECK(std::get<std::int64_t>(*p.get_attribute("count", value_type::integer)) == 2);
            CHECK(label(p) == "5");
        }
    }
}

SCENARIO("Hints that cannot be resolved are reported, and the annotations as written are used") {
    ensure_runtime();
    log_capture log;
    console().clear();
    processor p{"bad_hints", log.sink()};
    REQUIRE(p.load());

    CHECK(console().contains("NameError", log_level::error));
    CHECK(log.contains("could not resolve the type hints of class bad_hints", log_level::error));

    REQUIRE(p.attributes().size() == 2);
    CHECK(p.attributes()[0].name == "level");
    CHECK(p.attributes()[0].type == value_type::real); // 'float', read by name
    CHECK(p.attributes()[1].type == value_type::symbol);

    REQUIRE(p.call("set_level", std::vector<value>{0.5}));
    CHECK(all_equal(render(p, 1.0), 0.5));
}
