/// @file tap.python.h
/// @brief tap.python: a Python class as a Max object without audio — what a method returns, it outputs.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// [tap.python name] loads python/name.py as tap.python~ does — the same loader, attributes, messages,
// hot reload and console, through the shared glue (tap.python_glue.h) — with the core's processor
// binding no audio (process() and prepare() are messages like any other), and outputs what each
// method returns: one value per outlet for a return hinted tuple[...], one outlet otherwise, and a
// dumpout at the right for attribute values (docs/TAP-PYTHON-PLAN.md, D7–D11; plan 9.3).
//
// It is a plain SDK class, not a min one (D11): registered by tap.python~'s ext_main after min's
// class (tap.python.cpp), found by Max through the package's init/tap.python.txt. Max allocates the
// box (t_tap_python, plain data); the object itself, control_object, lives behind a pointer in it.
//
// Threads (the plan's "Threads and the GIL"). A message runs on the thread it arrives on — Max's
// main thread, the scheduler thread under Overdrive, or the audio thread with Scheduler in Audio
// Interrupt on, which the object says once per session. The method runs under the GIL; its result
// is output after the GIL is released, under the object's own lock — a recursive mutex, because a
// patch can send a message back into the object from its own output — which also guards every change
// to the outlets on a reload. The lock is never held across a call into Python, and never taken
// with the GIL held (the guard, which runs inside load() with the GIL, takes no lock of the object's),
// so the two are always taken in one order: the object's lock, then (downstream) the GIL.

#pragma once

// The core includes CPython, which insists on preceding system headers.
#include "tap/python/processor.h"
#include "tap/python/runtime.h"
#include "tap/python/value.h"

// Then the standard library and Max's (this comment keeps clang-format from regrouping the two).
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "tap.python_glue.h"

namespace tap::python {

    class control_object;

    /// The Max object, as object_alloc() makes it: plain data, with no constructor run.
    struct t_tap_python {
        c74::max::t_object header;
        void*              obex;   // Max's, for the dumpout (class_obexoffset_set)
        control_object*    object; // the object itself
    };

    /// tap.python: the class file's methods as messages, what they return as output.
    class control_object {
      public:
        /// The object's Max class name, for an error in a trampoline.
        static constexpr const char* k_max_name = "tap.python";

        /// The most value outlets an object has: a return hinted tuple[...] wider than this is output
        /// as a list (the core's rule, at 64).
        static constexpr std::size_t k_max_outlets = 64;

        /// The selectors the class answers with forwarders of its own (tap.python.cpp): `anything`
        /// for every message nothing else answers, and int, float, bang and list, which Max never
        /// passes to a class's anything (plan 9.0). A Python method of one of these names is the
        /// class's to define: the guard must not take the forwarder for Max's (and a Python
        /// `anything` is called by the forwarder, never registered with Max).
        static bool forwarded_by_class(const std::string& name) {
            return name == "anything" || name == "int" || name == "float" || name == "bang" || name == "list";
        }

        /// The names a class may not use: every object's (8.2's list, less the audio ones, D7) less
        /// `anything`, which the class's forwarder passes to a Python `anything` — and `dumpout`, the
        /// class's own A_CANT method, which Max answers (plan 9.0).
        static std::vector<std::string> reserved_names() {
            auto names = reserved_messages(false);
            names.erase(std::remove(names.begin(), names.end(), "anything"), names.end());
            names.emplace_back("dumpout");
            std::sort(names.begin(), names.end());
            return names;
        }

        /// The object Max called (a trampoline's, or the class's own methods').
        static control_object* self(c74::max::t_object* x) { return reinterpret_cast<t_tap_python*>(x)->object; }

        /// Make the object in `owner`, a box being created: start the interpreter, load the class
        /// file the arguments name (before any @attribute; none: python/default.py), and make the
        /// outlets — the dumpout, then a value outlet for each the class's return hints need (one if
        /// it does not load) — and the attributes and messages for its fields and methods. Main
        /// thread, from the class's new; reports what goes wrong, and never throws but for memory.
        control_object(c74::max::t_object* owner, const long argc, const c74::max::t_atom* argv)
            : m_owner{owner} {
            const auto start = start_runtime();
            if (!start.ok) {
                log(log_level::error, start.error);
            }
            else {
                m_scripts_dir = start.scripts_dir;
                m_source      = source_name(argc, argv);
                m_processor   = std::make_unique<processor>(
                    m_source, [this](const log_level level, const std::string_view text) { log(level, text); },
                    reserved_names(), std::function<void()>{},
                    [this](const std::string& name) {
                        return !forwarded_by_class(name) && m_members.answered_by_max(m_owner, name);
                    },
                    false);
            }

            const bool loaded = m_processor && m_processor->load();

            // Max orders a box's outlets by creation, right to left: the dumpout first, then the value
            // outlets from the last to the first (plan 9.0, 2)
            m_dumpout = c74::max::outlet_new(m_owner, nullptr);
            c74::max::object_obex_store(m_owner, c74::max::gensym("dumpout"),
                                        static_cast<c74::max::t_object*>(m_dumpout));
            const auto count = loaded ? (std::min)(m_processor->outlet_count(), k_max_outlets) : std::size_t{1};
            m_value_outlets.resize(count);
            for (auto n = count; n-- > 0;) {
                m_value_outlets[n] = c74::max::outlet_new(m_owner, nullptr);
            }

            if (loaded) {
                m_members.create_attributes(m_owner, *m_processor);
                m_members.create_messages(m_owner, *m_processor, k_forwarded);
            }

            if (m_processor) {
                std::string watch_error;
                m_file_watch = watch_source(m_owner, m_scripts_dir, m_source, watch_error);
                if (!watch_error.empty()) {
                    log(log_level::error, watch_error);
                }
            }

            m_notices = c74::max::qelem_new(m_owner, reinterpret_cast<c74::max::method>(post_notices_of));
        }

        ~control_object() {
            m_file_watch.reset();
            if (m_notices) {
                c74::max::qelem_free(m_notices);
            }
            m_processor.reset(); // releases the Python objects under the GIL
            // Max frees the outlets and the attributes the object added
        }

        control_object(const control_object&)            = delete;
        control_object& operator=(const control_object&) = delete;

        /// Load the class file again (the file watcher's filechanged, or a patcher's), and make the
        /// attributes, messages and outlets match the class: on a failure, they stay as they were, and
        /// the object outputs nothing until a save fixes it. Main thread.
        void update_source() {
            if (!m_processor || !m_processor->load()) {
                return; // the processor has reported why
            }
            // the attributes first, outside the lock (the glue's maps have their own): Max may call an
            // attribute's getter, so Python, while object_addattr() adds it
            m_members.create_attributes(m_owner, *m_processor);
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            m_members.create_messages(m_owner, *m_processor, k_forwarded);
            adapt_outlets();
        }

        /// A message for one of the class's methods: call it, and output what it returns. Main,
        /// scheduler or audio thread.
        void message_gimme(const c74::max::t_symbol* name, const long ac, const c74::max::t_atom* av) {
            if (!m_processor || !m_processor->loaded()) {
                return;
            }
            notice_audio_thread();
            send(m_processor->call_with_output(name->s_name, to_values(ac, av)));
        }

        /// Max's attribute setter and getter, for the attributes made for the class's fields.
        void attr_set(const c74::max::t_symbol* name, const long argc, const c74::max::t_atom* argv) {
            notice_audio_thread();
            m_members.set_attribute(m_processor.get(), name->s_name, argc, argv,
                                    [this](const log_level level, const std::string_view text) { log(level, text); });
        }

        void attr_get(const c74::max::t_symbol* name, long* argc, c74::max::t_atom** argv) {
            m_members.get_attribute(m_processor.get(), name->s_name, argc, argv);
        }

        /// What the class's forwarders receive: a message nothing registered on the object answered
        /// (anything), and int, float, bang and list when the class has no method of the name. Tried
        /// in order as one of the class's messages, an attribute to set, get<attribute> (its value
        /// from the dumpout), and the class's own `anything(selector, *args)`; otherwise Max's own
        /// words. In Max the first three are answered before the forwarder (plan 9.0); here they are
        /// for whatever reaches it anyway — a message sent by object_method_typed(), say.
        void forward(const c74::max::t_symbol* selector, const long ac, const c74::max::t_atom* av) {
            const std::string name{selector->s_name};
            if (name != "anything" && m_members.has_message(name)) {
                message_gimme(selector, ac, av);
                return;
            }
            if (m_members.attribute_type(name)) {
                attr_set(selector, ac, av);
                return;
            }
            if (name.size() > 3 && name.compare(0, 3, "get") == 0 && m_members.attribute_type(name.substr(3))) {
                output_attribute(name.substr(3));
                return;
            }
            if (m_members.has_message("anything")) {
                if (!m_processor || !m_processor->loaded()) {
                    return;
                }
                notice_audio_thread();
                auto args = to_values(ac, av);
                args.insert(args.begin(), value{name});
                send(m_processor->call_with_output("anything", args));
                return;
            }
            c74::max::object_error(m_owner, "doesn't understand \"%s\"", selector->s_name);
        }

        /// The help text for the inlet (io 1) or an outlet (io 2).
        void assist(const long io, const long index, char* text) const {
            constexpr std::size_t k_size = 500; // ASSIST_MAX_STRING_LEN
            if (io == 1) {
                std::snprintf(text, k_size, "Messages: the Python class's methods, and its attributes");
                return;
            }
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            const auto                                  count = static_cast<long>(m_value_outlets.size());
            if (index >= count) {
                std::snprintf(text, k_size, "dumpout: attribute values (get<name>)");
            }
            else if (count == 1) {
                std::snprintf(text, k_size, "What a method returns");
            }
            else {
                std::snprintf(text, k_size,
                              "Value %ld of what a method hinted tuple[...] returns, or what another returns",
                              index + 1);
            }
        }

        /// Post what the audio thread asked for (the qelem's function, main thread).
        void post_notices() {
            if (m_notice_due.exchange(false)) {
                c74::max::object_warn(m_owner,
                                      "Scheduler in Audio Interrupt is on: messages to tap.python run Python on the "
                                      "audio thread, where a long method, a reload or another object's Python can "
                                      "interrupt the audio. Send them through [deferlow] to run them on the main "
                                      "thread. (Said once per session.)");
            }
        }

        /// How many value outlets the object has, the dumpout not counted (for the tests).
        std::size_t value_outlet_count() const {
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            return m_value_outlets.size();
        }

        /// The names of the Max messages made for the class's methods (for the tests).
        std::vector<std::string> python_message_names() const { return m_members.message_names(); }

        /// The outlets, as Max gave them (for the tests).
        void* value_outlet(const std::size_t n) const {
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            return n < m_value_outlets.size() ? m_value_outlets[n] : nullptr;
        }
        void* dumpout_outlet() const { return m_dumpout; }

      private:
        static inline const std::vector<std::string> k_forwarded{"anything"};

        c74::max::t_object*            m_owner;
        std::string                    m_source;
        std::filesystem::path          m_scripts_dir;
        std::unique_ptr<processor>     m_processor;
        std::unique_ptr<file_watch>    m_file_watch;
        python_members<control_object> m_members;
        mutable std::recursive_mutex   m_lock;          // the outlets, and every output through them
        std::vector<void*>             m_value_outlets; // left to right
        void*                          m_dumpout{};
        bool                           m_outlets_differ{};   // without a box: said once
        std::uint64_t                  m_dropped_reported{}; // the load a dropped output was last reported for
        c74::max::t_qelem*             m_notices{};          // posts the audio thread's notice
        std::atomic<bool>              m_notice_due{};

        /// The class file's name: the arguments before the first @attribute — one symbol, read as it
        /// is (anything else, as text, is not a valid name, and the core says so) — or "default".
        static std::string source_name(const long argc, const c74::max::t_atom* argv) {
            const auto count =
                c74::max::attr_args_offset(static_cast<short>(argc), const_cast<c74::max::t_atom*>(argv));
            if (count <= 0) {
                return "default";
            }
            if (count == 1 && c74::max::atom_gettype(argv) == c74::max::A_SYM) {
                return c74::max::atom_getsym(argv)->s_name;
            }
            std::string text;
            for (long i = 0; i < count; ++i) {
                const auto* atom = argv + i;
                text += i == 0 ? "" : " ";
                switch (c74::max::atom_gettype(atom)) {
                case c74::max::A_LONG:
                    text += std::to_string(c74::max::atom_getlong(atom));
                    break;
                case c74::max::A_FLOAT:
                    text += std::to_string(c74::max::atom_getfloat(atom));
                    break;
                default:
                    text += c74::max::atom_getsym(atom)->s_name;
                    break;
                }
            }
            return text;
        }

        static void post_notices_of(t_tap_python* x) {
            if (x->object) {
                x->object->post_notices();
            }
        }

        /// What concerns this object, to the Max console with the object named.
        void log(const log_level level, const std::string_view text) const {
            const std::string line{text};
            if (level == log_level::error) {
                c74::max::object_error(m_owner, "%s", line.c_str());
            }
            else {
                c74::max::object_post(m_owner, "%s", line.c_str());
            }
        }

        /// With Scheduler in Audio Interrupt on, a message from the scheduler runs on the audio thread
        /// (plan 9.0, 5): say so once per session, from the main thread. Real-time safe.
        void notice_audio_thread() {
            static std::atomic<bool> s_noticed{};
            if (c74::max::systhread_isaudiothread() && !s_noticed.exchange(true)) {
                m_notice_due.store(true);
                c74::max::qelem_set(m_notices);
            }
        }

        /// Output a call's result, right to left: each value from its outlet, an empty slot nothing.
        /// A result for more outlets than the object has — computed against a class that a reload
        /// narrowed meanwhile — is dropped, and said once per load.
        void send(const output& out) {
            if (out.empty()) {
                return;
            }
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            if (out.size() > m_value_outlets.size()) {
                const auto loads = m_processor ? m_processor->load_count() : 0;
                if (m_dropped_reported != loads) {
                    m_dropped_reported = loads;
                    log(log_level::error, "a result for " + std::to_string(out.size())
                                              + " outlets arrived while the "
                                                "class reloaded, and the object has "
                                              + std::to_string(m_value_outlets.size())
                                              + " — not output (said once per load)");
                }
                return;
            }
            for (auto n = out.size(); n-- > 0;) {
                if (out[n]) {
                    emit(m_value_outlets[n], *out[n]);
                }
            }
        }

        /// One output item from `outlet`: a number by itself, a list, or a message. Holds the lock.
        void emit(void* outlet, const output_item& item) {
            if (item.atoms.size() > static_cast<std::size_t>((std::numeric_limits<short>::max)())) {
                log(log_level::error, "a list of " + std::to_string(item.atoms.size())
                                          + " atoms is longer than a Max message can be (32767) — not output");
                return;
            }
            std::vector<c74::max::t_atom> atoms(item.atoms.size());
            for (std::size_t i = 0; i < item.atoms.size(); ++i) {
                const auto& atom = item.atoms[i];
                if (const auto* integer = std::get_if<std::int64_t>(&atom)) {
                    c74::max::atom_setlong(&atoms[i], static_cast<c74::max::t_atom_long>(*integer));
                }
                else if (const auto* real = std::get_if<double>(&atom)) {
                    c74::max::atom_setfloat(&atoms[i], *real);
                }
                else {
                    c74::max::atom_setsym(&atoms[i], c74::max::gensym(std::get<std::string>(atom).c_str()));
                }
            }
            const auto count = static_cast<short>(atoms.size());
            if (!item.selector) { // one number
                if (count == 1 && c74::max::atom_gettype(atoms.data()) == c74::max::A_LONG) {
                    c74::max::outlet_int(outlet, c74::max::atom_getlong(atoms.data()));
                }
                else if (count == 1 && c74::max::atom_gettype(atoms.data()) == c74::max::A_FLOAT) {
                    c74::max::outlet_float(outlet, c74::max::atom_getfloat(atoms.data()));
                }
                else {
                    c74::max::outlet_list(outlet, nullptr, count, atoms.data());
                }
            }
            else if (*item.selector == "list") {
                c74::max::outlet_list(outlet, nullptr, count, atoms.data());
            }
            else {
                c74::max::outlet_anything(outlet, c74::max::gensym(item.selector->c_str()), count, atoms.data());
            }
        }

        /// get<name> through the forwarder: the attribute's value from the dumpout, as Max outputs it.
        void output_attribute(const std::string& name) {
            long              argc = 0;
            c74::max::t_atom* argv = nullptr;
            m_members.get_attribute(m_processor.get(), name.c_str(), &argc, &argv);
            {
                const std::lock_guard<std::recursive_mutex> lock{m_lock};
                c74::max::outlet_anything(m_dumpout, c74::max::gensym(name.c_str()), static_cast<short>(argc), argv);
            }
            c74::max::sysmem_freeptr(argv);
        }

        /// Make the value outlets match what the reloaded class's return hints need, in place: Max's
        /// dynamic outlets, changed between the box's dynlet_begin and dynlet_end, keep the patch
        /// cords of every outlet that stays, the dumpout's included — the surplus deleted from the
        /// right, new ones inserted after the last value outlet, never appended (which would put them
        /// after the dumpout): plan 9.0, 2. An object without a box (made by object_new) keeps its
        /// outlets and says so, once, until they match again. Holds the lock; main thread.
        void adapt_outlets() {
            const auto wanted = (std::min)(m_processor->outlet_count(), k_max_outlets);
            const auto have   = m_value_outlets.size();
            if (wanted == have) {
                m_outlets_differ = false;
                return;
            }
            c74::max::t_object* box{};
            if (c74::max::object_obex_lookup(m_owner, c74::max::gensym("#B"), &box) != c74::max::MAX_ERR_NONE || !box) {
                if (!m_outlets_differ) {
                    log(log_level::info, "the class now has " + std::to_string(wanted) + " outlet(s); this object has "
                                             + std::to_string(have) + " — re-create it to change them");
                }
                m_outlets_differ = true;
                return;
            }

            c74::max::object_method(box, c74::max::gensym("dynlet_begin"));
            while (m_value_outlets.size() > wanted) {
                c74::max::outlet_delete(m_value_outlets.back());
                m_value_outlets.pop_back();
            }
            while (m_value_outlets.size() < wanted) {
                m_value_outlets.push_back(
                    c74::max::outlet_insert_after(m_owner, nullptr, nullptr, m_value_outlets.back()));
            }
            c74::max::object_method(box, c74::max::gensym("dynlet_end"));
            m_outlets_differ = false;
        }
    };

} // namespace tap::python

/// Register tap.python with Max (tap.python.cpp): from tap.python~'s ext_main, after min's class.
void tap_python_register();

/// The class's new, as Max calls it for a box [tap.python <arguments>] (tap.python.cpp; the tests
/// make their objects through it).
void* tap_python_new(c74::max::t_symbol* s, long argc, c74::max::t_atom* argv);
