/// @file tap.python_glue.h
/// @brief The Max side every object over the core shares: its runtime, its attributes and messages.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// tap.python~ (a min object) and tap.python (a plain SDK class, plan 9.3) both map a Python class
// onto Max through the core (core/include/tap/python/), and what is the same for both lives here,
// once (plan 9.2): starting the interpreter against the package's runtime, with Python's output
// posted from Max's main thread; watching the class file; the names Max answers itself, and the
// guard that keeps a Python name from replacing one; and the Max attributes and messages made for
// the class's fields and methods, with the C trampolines Max calls for them.
//
// The trampolines are instantiated per host type (python_glue<Host>): each finds the object Max
// called through Host::self() and calls its attr_set(), attr_get() or message_gimme().

#pragma once

// The core includes CPython, which insists on preceding system headers.
#include "tap/python/processor.h"
#include "tap/python/runtime.h"
#include "tap/python/value.h"

// Then the standard library and Max's (this comment keeps clang-format from regrouping the two).
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "c74_min_api.h"
#include "tap.python_filewatch.h"
#include "tap.python_package.h"

namespace tap::python {

    // ---- the runtime ---------------------------------------------------------------------------

    namespace detail {

        /// Python's print() output and tracebacks, for every object.
        inline void console_line(const log_level level, const std::string_view text) {
            const std::string line{text};
            if (level == log_level::error) {
                c74::max::object_error(nullptr, "python: %s", line.c_str());
            }
            else {
                c74::max::object_post(nullptr, "python: %s", line.c_str());
            }
        }

        /// Whether this is Max's main thread: where Python's output may be posted at once. On any
        /// other thread — the audio thread, a worker, the scheduler — the core queues the line (plan 8.4).
        inline bool is_main_thread() {
            return c74::max::systhread_ismainthread() != 0;
        }

        /// Called on the printing thread when the core queued a line: sets the process-wide qelem that
        /// has the lines posted from the main thread. Real-time safe (qelem_set).
        inline void console_ready() {
            static c74::max::t_qelem* s_flush =
                c74::max::qelem_new(nullptr, reinterpret_cast<c74::max::method>(+[](void*) { flush_console(); }));
            c74::max::qelem_set(s_flush);
        }

    } // namespace detail

    /// What start_runtime() found: the package's script folder, or why Python cannot run.
    struct runtime_start {
        bool                  ok{};
        std::filesystem::path scripts_dir; ///< <package>/python
        std::string           error;       ///< what to tell the user, when not ok
    };

    /// Start the embedded interpreter against this package's runtime and python folder, with
    /// Python's output posted from Max's main thread (plan 8.4). Once per process: an object after
    /// the first finds it running. Main thread.
    inline runtime_start start_runtime() {
        runtime_start result;
        const auto    package = package_root();
        result.scripts_dir    = package / "python";
#ifdef TAP_PYTHON_HOME
        // Linux development builds embed a system CPython instead of a support/ runtime
        // (see this project's CMakeLists.txt)
        const std::filesystem::path home{TAP_PYTHON_HOME};
#else
        const auto home = runtime_home(package); // support/, or support/<platform> (plan 4.8)
#endif

        if (!std::filesystem::exists(home)) {
            result.error = "No Python runtime found at " + detail::utf8(home)
                           + " — run scripts/install-runtime from the package root to install it.";
            return result;
        }
        if (std::string error; !runtime_library_loadable(home, error)) {
            result.error = error;
            return result;
        }

        runtime_options options;
        options.home           = home;
        options.scripts_dir    = result.scripts_dir;
        options.console        = detail::console_line;
        options.is_main_thread = detail::is_main_thread; // Python's output posts from Max's main thread only (8.4)
        options.console_ready  = detail::console_ready;
        if (const auto status = initialize(options); !status.ok) {
            result.error = "failed to start Python from '" + detail::utf8(home) + "': " + status.error
                           + " (run scripts/install-runtime to install the runtime)";
            return result;
        }
        result.ok = true;
        return result;
    }

    /// Watch `<scripts_dir>/<name>.py`, telling `owner` of each save by its filechanged message (see
    /// tap.python_filewatch.h). Only a plain file name is watched: the core refuses anything else
    /// (e.g. "../x"), and so must the watcher — null without `error`. Null with `error` set when Max
    /// cannot find the file. Main thread.
    inline std::unique_ptr<file_watch> watch_source(c74::max::t_object* owner, const std::filesystem::path& scripts_dir,
                                                    const std::string& name, std::string& error) {
        if (name.find_first_of("/\\.:") != std::string::npos) {
            return nullptr;
        }
        const auto watched_file = scripts_dir / (name + ".py");
        const auto watched_utf8 = detail::utf8(watched_file); // Max's paths are UTF-8 (8.7)
        char       filename[c74::max::MAX_PATH_CHARS]{};
        std::strncpy(filename, watched_utf8.c_str(), c74::max::MAX_PATH_CHARS - 1);
        short              path_id{};
        c74::max::t_fourcc filetype{};
        if (c74::max::locatefile_extended(filename, &path_id, &filetype, nullptr, 0) != 0) {
            error = "Unable to watch " + watched_utf8 + " for changes.";
            return nullptr;
        }
        return std::make_unique<file_watch>(owner, path_id, filename);
    }

    // ---- the names Max answers ---------------------------------------------------------------------

    /// The names reserved_messages() adds for an audio object: Max's and min's audio messages, and
    /// tap.python~'s own attributes (worker mode's, plan 2.5).
    inline std::vector<std::string> audio_messages() {
        return {"dsp",
                "dsp64",
                "dspsetup",
                "dspstate",
                "inputchanged",
                "latency",
                "latencysamples",
                "mode",
                "multichanneloutputs",
                "signal"};
    }

    /// Messages and attributes the Max object handles itself, which a Python method or field must
    /// never replace: min's own class methods, the messages Max sends every object, the file
    /// watcher's filechanged (a Python method named filechanged would silently disable hot reload),
    /// and with `audio`, the audio_messages().
    ///
    /// Above all, every message Max sends with C arguments (plan 8.2, audit A3): a Python method
    /// of such a name would be registered with the A_GIMME trampoline, and Max calling it with a
    /// long or a pointer reads them as a symbol and an atom list — the crash 6.1 found for
    /// filechanged, in its general form. The names are those min treats as A_CANT
    /// (c74_min_message.h, message_type::cant; the MIN_WRAPPER_ADDMETHOD table in
    /// c74_min_object_wrapper.h) plus Max's own dspstate, inputchanged and multichanneloutputs.
    /// Anything the Max class already answers is reserved as well: python_members::answered_by_max().
    inline std::vector<std::string> reserved_messages(const bool audio) {
        std::vector<std::string> names{"anything",
                                       "appendtodictionary",
                                       "assist",
                                       "dblclick",
                                       "dictionary",
                                       "edclose",
                                       "filechanged",
                                       "fileusage",
                                       "focusgained",
                                       "focuslost",
                                       "getplaystate",
                                       "getvalueof",
                                       "inletinfo",
                                       "jitclass_setup",
                                       "key",
                                       "loadbang",
                                       "maxclass_setup",
                                       "maxob_setup",
                                       "mop_setup",
                                       "mousedoubleclick",
                                       "mousedown",
                                       "mousedrag",
                                       "mousedragdelta",
                                       "mouseenter",
                                       "mouseleave",
                                       "mousemove",
                                       "mouseup",
                                       "mousewheel",
                                       "mt_mousedown",
                                       "mt_mousedrag",
                                       "mt_mouseenter",
                                       "mt_mouseleave",
                                       "mt_mousemove",
                                       "mt_mouseup",
                                       "notify",
                                       "okclose",
                                       "oksize",
                                       "paint",
                                       "patchlineupdate",
                                       "preset",
                                       "savestate",
                                       "setup",
                                       "setvalueof"};
        if (audio) {
            const auto more = audio_messages();
            names.insert(names.end(), more.begin(), more.end());
            std::sort(names.begin(), names.end());
        }
        return names;
    }

    /// Whether object_getmethod() found a method: neither null nor `not_found`, what it answers for a
    /// name the object does not have — Max's method_false(), as the SDK documents. 1.0.0 tested for
    /// null alone, and 1.0.1 compared with the address of method_false as this module sees it, which
    /// on Windows is the external's own import thunk (the SDK declares it without dllimport), never
    /// what Max returns: either way every field and method of a class was "answered by Max", and the
    /// object had no attributes and no messages. So `not_found` comes from Max (not_found_method()).
    inline bool found_method(const c74::max::method found, const c74::max::method not_found) {
        return found != nullptr && found != not_found;
    }

    /// What object_getmethod() answers for a name `object` does not have, asked of Max for one no
    /// class can have (it has spaces): Max's method_false(), whatever its address looks like from
    /// here — on a class with a class-level anything too (plan 9.0). Main thread.
    inline c74::max::method not_found_method(c74::max::t_object* object) {
        return c74::max::object_getmethod(object, c74::max::gensym("tap.python~ answers no such name"));
    }

    // ---- atoms ----------------------------------------------------------------------------------

    /// Max atoms as core values, keeping each atom's own type (the core coerces to the
    /// hinted type with the same rules as atom_getlong/atom_getfloat/atom_getsym).
    inline std::vector<value> to_values(const long ac, const c74::max::t_atom* av) {
        std::vector<value> values;
        values.reserve(static_cast<std::size_t>(ac));
        for (long i = 0; i < ac; ++i) {
            const auto* atom = av + i;
            switch (c74::max::atom_gettype(atom)) {
            case c74::max::A_LONG:
                values.emplace_back(static_cast<std::int64_t>(c74::max::atom_getlong(atom)));
                break;
            case c74::max::A_FLOAT:
                values.emplace_back(static_cast<double>(c74::max::atom_getfloat(atom)));
                break;
            default:
                values.emplace_back(std::string{c74::max::atom_getsym(atom)->s_name});
                break;
            }
        }
        return values;
    }

    // ---- the trampolines Max calls ------------------------------------------------------------------

    /// Max calls the trampolines below from C, so no C++ exception may cross back into it.
    template <typename Fn>
    c74::max::t_max_err guarded(c74::max::t_object* x, const char* object_name, Fn&& fn) noexcept {
        try {
            fn();
        }
        catch (const std::exception& e) {
            c74::max::object_error(x, "%s: %s", object_name, e.what());
        }
        catch (...) {
            c74::max::object_error(x, "%s: unknown error", object_name);
        }
        return c74::max::MAX_ERR_NONE;
    }

    /// The C functions Max calls for the attributes and messages made for a Python class (by
    /// python_attr and python_message), one set per host type. `Host` provides
    /// - `static Host* self(c74::max::t_object* x)`: the object Max called;
    /// - `static constexpr const char* k_max_name`: its Max class name, for an error;
    /// - `attr_set(name, argc, argv)`, `attr_get(name, argc, argv)` and `message_gimme(name, ac, av)`,
    ///   the name a `c74::max::t_symbol*` (or anything made from one).
    template <class Host>
    struct python_glue {
        static c74::max::t_max_err attr_set(c74::max::t_object* x, c74::max::t_object* maxattr, const long argc,
                                            const c74::max::t_atom* argv) {
            return guarded(x, Host::k_max_name, [&] { Host::self(x)->attr_set(attribute_name(maxattr), argc, argv); });
        }

        static c74::max::t_max_err attr_get(c74::max::t_object* x, c74::max::t_object* maxattr, long* argc,
                                            c74::max::t_atom** argv) {
            return guarded(x, Host::k_max_name, [&] { Host::self(x)->attr_get(attribute_name(maxattr), argc, argv); });
        }

        static c74::max::t_max_err mess_int(c74::max::t_object* x, const long value) {
            return guarded(x, Host::k_max_name, [&] {
                c74::max::t_atom a;
                c74::max::atom_setlong(&a, value);
                Host::self(x)->message_gimme(c74::max::gensym("int"), 1, &a);
            });
        }

        static c74::max::t_max_err mess_float(c74::max::t_object* x, const double value) {
            return guarded(x, Host::k_max_name, [&] {
                c74::max::t_atom a;
                c74::max::atom_setfloat(&a, value);
                Host::self(x)->message_gimme(c74::max::gensym("float"), 1, &a);
            });
        }

        static c74::max::t_max_err mess_symbol(c74::max::t_object* x, c74::max::t_symbol* value) {
            return guarded(x, Host::k_max_name, [&] {
                c74::max::t_atom a;
                c74::max::atom_setsym(&a, value);
                Host::self(x)->message_gimme(c74::max::gensym("symbol"), 1, &a);
            });
        }

        static c74::max::t_max_err mess_bang(c74::max::t_object* x) {
            return guarded(x, Host::k_max_name,
                           [&] { Host::self(x)->message_gimme(c74::max::gensym("bang"), 0, nullptr); });
        }

        static c74::max::t_max_err mess_gimme(c74::max::t_object* x, c74::max::t_symbol* name, const long ac,
                                              c74::max::t_atom* av) {
            return guarded(x, Host::k_max_name, [&] { Host::self(x)->message_gimme(name, ac, av); });
        }

      private:
        static const c74::max::t_symbol* attribute_name(c74::max::t_object* maxattr) {
            return static_cast<const c74::max::t_symbol*>(c74::max::object_method(maxattr, c74::min::k_sym_getname));
        }
    };

    // ---- the attributes and messages made for a class ---------------------------------------------

    /// A Max attribute dynamically added to an object, mirroring an annotated attribute of the user's
    /// Python class. Instances are owned via unique_ptr in python_members; one persists across reloads
    /// while the class keeps the field with the same type, and is removed otherwise.
    template <class Host>
    class python_attr {
      public:
        python_attr(c74::max::t_object* owner, const std::string& name, const tap::python::value_type type)
            : m_owner{owner}
            , m_name{name}
            , m_value_type{type} {
            switch (m_value_type) {
            case value_type::integer:
            case value_type::boolean:
                m_type = c74::min::k_sym_long;
                break;
            case value_type::real:
                m_type = c74::min::k_sym_float64;
                break;
            case value_type::symbol:
            case value_type::any:
                m_type = c74::min::k_sym_symbol;
                break;
            }

            m_attrobj = c74::max::attribute_new(m_name.c_str(), m_type, 0,
                                                reinterpret_cast<c74::max::method>(python_glue<Host>::attr_get),
                                                reinterpret_cast<c74::max::method>(python_glue<Host>::attr_set));
            auto err  = c74::max::object_addattr(m_owner, m_attrobj);
            if (err) {
                c74::max::object_error(m_owner, "failed to add attribute for python member '%s'", m_name.c_str());
                return;
            }
            err = c74::max::object_attr_addattr_parse(m_owner, m_name.c_str(), "dynamicattr", c74::min::k_sym_long, 0,
                                                      "1");
            if (err) {
                c74::max::object_error(m_owner, "failed to make dynamic attribute for %s", m_name.c_str());
            }
            if (m_value_type == value_type::boolean) {
                // a bool field shows as a toggle in the inspector and attrui
                c74::max::object_attr_addattr_parse(m_owner, m_name.c_str(), "style", c74::min::k_sym_symbol, 0,
                                                    "onoff");
            }
        }

        python_attr(const python_attr&)            = delete;
        python_attr& operator=(const python_attr&) = delete;

        /// Detach the attribute from the Max object and free it (the class no longer has the field,
        /// or its type changed). Not called at object destruction: Max frees instance attributes then.
        void remove() {
            if (m_attrobj) {
                c74::max::object_deleteattr(m_owner, c74::max::gensym(m_name.c_str()));
                m_attrobj = nullptr;
            }
        }

        /// The Max attribute type (long, float64 or symbol).
        c74::min::symbol type() const { return m_type; }

        /// The core value type the attribute was created with; fixed for the attribute's lifetime.
        tap::python::value_type value_type() const { return m_value_type; }

      private:
        c74::max::t_object*     m_owner;
        std::string             m_name;
        tap::python::value_type m_value_type;
        c74::min::symbol        m_type;
        c74::max::t_object*     m_attrobj{};
    };

    /// A Max message dynamically added to an object for a method of the user's Python class. It only
    /// registers the Max method; the Python side (the bound function and its signature) is owned by
    /// the core's processor, which the host's message_gimme() dispatches to by name. Instances are
    /// owned via unique_ptr in python_members and rebuilt on every reload.
    template <class Host>
    class python_message {
      public:
        python_message(c74::max::t_object* owner, const std::string& name)
            : m_owner{owner}
            , m_name{name} {
            if (m_owner == nullptr) {
                return; // this occurs during dummy construction
            }

            using glue = python_glue<Host>;
            c74::max::t_max_err err{};

            if (m_name == "int") {
                err = c74::max::object_addmethod(m_owner, reinterpret_cast<c74::max::method>(glue::mess_int),
                                                 m_name.c_str(), c74::max::A_LONG, 0);
            }
            else if (m_name == "float") {
                err = c74::max::object_addmethod(m_owner, reinterpret_cast<c74::max::method>(glue::mess_float),
                                                 m_name.c_str(), c74::max::A_FLOAT, 0);
            }
            else if (m_name == "symbol") {
                err = c74::max::object_addmethod(m_owner, reinterpret_cast<c74::max::method>(glue::mess_symbol),
                                                 m_name.c_str(), c74::max::A_SYM, 0);
            }
            else if (m_name == "bang") {
                err = c74::max::object_addmethod(m_owner, reinterpret_cast<c74::max::method>(glue::mess_bang),
                                                 m_name.c_str(), 0);
            }
            else {
                err = c74::max::object_addmethod(m_owner, reinterpret_cast<c74::max::method>(glue::mess_gimme),
                                                 m_name.c_str(), c74::max::A_GIMME, 0);
            }

            if (err) {
                c74::max::object_error(m_owner, "failed to add message for python method '%s'", m_name.c_str());
            }
        }

        python_message(const python_message&)            = delete;
        python_message& operator=(const python_message&) = delete;

      private:
        c74::max::t_object* m_owner;
        std::string         m_name;
    };

    /// The Max attributes and messages one object made for its class's fields and methods, kept in
    /// step with the class across reloads; the attribute access and the guard that go with them.
    ///
    /// The maps change on the main thread, on a reload, while attributes are read and set from the
    /// main or the scheduler thread (and a host's forwarder looks names up from either): a lock of
    /// its own guards them, held for the lookups and the changes and never across a call into
    /// Python — what a lookup finds is copied out first. Recursive, because Max may call an
    /// attribute's getter from inside object_addattr() on the thread making it.
    template <class Host>
    class python_members {
      public:
        /// Make the Max attributes match the class's annotated fields: remove the ones the class no
        /// longer has, recreate the ones whose type changed, add the new ones. (The core has already
        /// carried the values of those that stayed over to the new instance.) Main thread.
        void create_attributes(c74::max::t_object* owner, const processor& p) {
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            const auto&                                 current = p.attributes();
            for (auto it = m_attributes.begin(); it != m_attributes.end();) {
                const auto found = std::find_if(current.begin(), current.end(),
                                                [&](const attribute_info& a) { return a.name == it->first; });
                if (found == current.end() || found->type != it->second->value_type()) {
                    it->second->remove();
                    it = m_attributes.erase(it);
                }
                else {
                    ++it;
                }
            }
            for (const auto& attribute : current) {
                if (m_attributes.find(attribute.name) == m_attributes.end()) {
                    m_attributes[attribute.name] =
                        std::make_unique<python_attr<Host>>(owner, attribute.name, attribute.type);
                }
            }
        }

        /// Replace the previous incarnation's Max messages with the class's current methods. A method
        /// named in `forwarded` is not registered with Max: the host answers it through a forwarder of
        /// its own (tap.python's anything, plan 9.3), and has_message() still knows it. Main thread.
        void create_messages(c74::max::t_object* owner, const processor& p,
                             const std::vector<std::string>& forwarded = {}) {
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            for (auto& element : m_messages) {
                c74::max::object_deletemethod(owner, c74::max::gensym(element.first.c_str()));
            }
            m_messages.clear();
            m_forwarded.clear();

            for (const auto& message : p.messages()) { // never names an attribute
                if (std::find(forwarded.begin(), forwarded.end(), message.name) != forwarded.end()) {
                    m_forwarded.insert(message.name);
                }
                else {
                    m_messages[message.name] = std::make_unique<python_message<Host>>(owner, message.name);
                }
            }
        }

        /// The names of the Max messages made for the class's methods, sorted (for the tests).
        std::vector<std::string> message_names() const {
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            std::vector<std::string>                    names;
            names.reserve(m_messages.size());
            for (const auto& element : m_messages) {
                names.push_back(element.first);
            }
            std::sort(names.begin(), names.end());
            return names;
        }

        /// Whether the class has a method `name`: registered with Max, or forwarded by the host.
        /// Any thread.
        bool has_message(const std::string& name) const {
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            return m_messages.find(name) != m_messages.end() || m_forwarded.find(name) != m_forwarded.end();
        }

        /// The type of the attribute `name`, if the class has one of that name. Any thread.
        std::optional<value_type> attribute_type(const std::string& name) const {
            const std::lock_guard<std::recursive_mutex> lock{m_lock};
            const auto                                  found = m_attributes.find(name);
            if (found == m_attributes.end()) {
                return std::nullopt;
            }
            return found->second->value_type();
        }

        /// Whether `owner` already answers `name` itself — a method its class registered (min's
        /// dsp64, assist, the reserved names), which Max would call before anything added for a
        /// Python method, or with C arguments. The attributes and messages made here for the previous
        /// incarnation's fields and methods are the object's own, not Max's, and must not make the
        /// class's names reserved on a reload: still registered while load() runs, an attribute
        /// answers its name — 1.0.1 took them for Max's, so a reload lost every field — and a message
        /// added with object_addmethod() answers as unknown (plan 9.0), so is left out either way.
        /// Main thread.
        bool answered_by_max(c74::max::t_object* owner, const std::string& name) const {
            if (has_message(name) || attribute_type(name)) {
                return false;
            }
            return found_method(c74::max::object_getmethod(owner, c74::max::gensym(name.c_str())),
                                not_found_method(owner));
        }

        /// Set the attribute `name` from Max's atoms: the core converts to the field's hinted type (a
        /// bool field gets a bool). `log` hears what is wrong. Main or scheduler thread.
        void set_attribute(processor* p, const char* name, const long argc, const c74::max::t_atom* argv,
                           const log_function& log) const {
            if (!p || !p->loaded()) {
                return;
            }
            if (argc != 1) {
                log(log_level::error, "Attributes with more than 1 arg not supported");
                return;
            }
            if (!attribute_type(name)) {
                return;
            }
            p->set_attribute(name, to_values(argc, argv).front());
        }

        /// Read the attribute `name` into Max's atoms, allocating them as an attribute getter must
        /// unless Max passed one: one atom of the attribute's type, the type's empty value when there
        /// is nothing to read (None, or a value that does not convert). Main or scheduler thread.
        void get_attribute(processor* p, const char* name, long* argc, c74::max::t_atom** argv) const {
            if ((*argc) != 1 || !(*argv)) { // otherwise use memory passed in
                if (*argc && *argv) {
                    c74::max::sysmem_freeptr(*argv);
                    *argv = nullptr;
                }
                *argc = 1;
                *argv =
                    reinterpret_cast<c74::max::t_atom*>(c74::max::sysmem_newptr(sizeof(c74::max::t_atom) * (*argc)));
            }
            c74::max::atom_setfloat(*argv, 0.0); // default in case anything below fails

            if (!p || !p->loaded()) {
                return;
            }
            const auto found = attribute_type(name);
            if (!found) {
                return;
            }

            // nothing to read (None, or a value that does not convert): the type's empty value
            const auto type = *found;
            if (type == value_type::integer || type == value_type::boolean) {
                c74::max::atom_setlong(*argv, 0);
            }
            else if (type == value_type::symbol) {
                c74::max::atom_setsym(*argv, c74::max::gensym(""));
            }

            const auto result = p->get_attribute(name, type);
            if (!result) {
                return;
            }
            if (const auto* i = std::get_if<std::int64_t>(&*result)) {
                c74::max::atom_setlong(*argv, static_cast<c74::max::t_atom_long>(*i));
            }
            else if (const auto* d = std::get_if<double>(&*result)) {
                c74::max::atom_setfloat(*argv, *d);
            }
            else {
                c74::max::atom_setsym(*argv, c74::max::gensym(std::get<std::string>(*result).c_str()));
            }
        }

      private:
        mutable std::recursive_mutex                                           m_lock;
        std::unordered_map<std::string, std::unique_ptr<python_message<Host>>> m_messages;
        std::unordered_map<std::string, std::unique_ptr<python_attr<Host>>>    m_attributes;
        std::unordered_set<std::string>                                        m_forwarded;
    };

} // namespace tap::python
