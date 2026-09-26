/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

// The core (core/include/tap/python/) includes CPython, which insists on preceding system headers.
#include "tap/python/processor.h"
#include "tap/python/runtime.h"
#include "tap/python/value.h"

// c74_min.h must be the FIRST min header in the translation unit: it defines
// C74_MIN_WITH_IMPLEMENTATION so the min wrapper's out-of-line statics get
// emitted here. If c74_min_api.h sneaks in first (e.g. via our helper headers),
// the include guards swallow those definitions and the external fails to link.
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "c74_min.h"
#include "tap.python_tilde_attribute.h"
#include "tap.python_tilde_cglue.h"
#include "tap.python_tilde_message.h"
#include "tap.python_tilde_package.h"

using namespace c74::min;
namespace runtime = tap::python;

class python : public object<python>, public vector_operator<> {
  public:
    MIN_DESCRIPTION{
        "Process audio with a Python class. Attributes and messages are generated from the class's type-annotated members, and the class's process() method runs on the audio signal. The source file is watched and hot-reloaded on save."};
    MIN_TAGS{"programming"};
    MIN_AUTHOR{"Tim Place"};
    MIN_RELATED{"js, node.script"};

    inlet<>  m_inlet{this, "(signal) input passed to the Python process() method"};
    outlet<> m_outlet_main{this, "(signal) output returned from the Python process() method", "signal"};

    argument<symbol> m_source_arg{
        this, "source",
        "Python source file in the package's python folder, without the .py extension. It must define a class of the same name."};

    // Max's filewatcher (created in the constructor) sends this message any
    // time our python source file is modified.
    message<> m_filechanged{this, "filechanged",
                            MIN_FUNCTION {
                                cout << "Source file update detected. Reloading." << endl;
                                try {
                                    update_source();
                                }
                                catch (const std::exception& e) {
                                    cerr << "reload failed: " << e.what() << endl;
                                }
                                return {};
                            }};

    // What went wrong on the audio thread (an exception in process(), a non-numeric or non-finite
    // result) is reported from here, on Max's main thread: the core calls report_ready on the audio
    // thread, which only sets this qelem.
    queue<> m_reports{this,
                      MIN_FUNCTION {
                          if (m_processor) {
                              try {
                                  m_processor->flush_reports();
                              }
                              catch (const std::exception& e) {
                                  cerr << "reporting failed: " << e.what() << endl;
                              }
                          }
                          return {};
                      }};

    // Called by min when the signal chain compiles, with the sample rate and the maximum vector size:
    // passed on to the class's prepare(), if it has one. (min finds it by the member name "dspsetup";
    // its m_dspsetup alternative is detected but then called as dspsetup, so it does not compile.)
    message<> dspsetup{this, "dspsetup",
                       MIN_FUNCTION {
                           if (m_processor) {
                               try {
                                   m_processor->prepare(static_cast<double>(args[0]),
                                                        static_cast<std::size_t>(static_cast<long>(args[1])));
                               }
                               catch (const std::exception& e) {
                                   cerr << "prepare failed: " << e.what() << endl;
                               }
                           }
                           return {};
                       }};

    python(const atoms& args = {}) {
        if (maxobj() == NULL) {
            return; // this occurs during dummy construction
        }

        const auto package = runtime::package_root();
        m_scripts_dir      = package / "python";
#ifdef TAP_PYTHON_HOME
        // Linux development builds embed a system CPython instead of a support/ runtime
        // (see this object's CMakeLists.txt)
        const std::filesystem::path home{TAP_PYTHON_HOME};
#else
        const auto home = package / "support";
#endif

        if (!std::filesystem::exists(home)) {
            cerr << "No Python runtime found at " << home.string()
                 << " — run scripts/install-runtime from the package root to install it." << endl;
            return;
        }

        const auto status = runtime::initialize({home, m_scripts_dir, console_line});
        if (!status.ok) {
            cerr << "failed to start Python from '" << home.string() << "': " << status.error
                 << " (run scripts/install-runtime to install the runtime)" << endl;
            return;
        }

        if (args.empty()) {
            m_python_source = "default";
        }
        else {
            m_python_source = to_string(args);
        }

        m_processor = std::make_unique<runtime::processor>(
            m_python_source,
            [this](const runtime::log_level level, const std::string_view text) {
                if (level == runtime::log_level::error) {
                    cerr << std::string{text} << endl;
                }
                else {
                    cout << std::string{text} << endl;
                }
            },
            reserved_messages(), [this] { m_reports.set(); });

        update_source();

        // watch the source file for changes (delivered as our 'filechanged' message) — only a plain
        // file name: the core refuses anything else (e.g. "../x"), and so must the watcher
        if (m_python_source.find_first_of("/\\.:") != std::string::npos) {
            return;
        }
        const auto watched_file = m_scripts_dir / (m_python_source + ".py");
        char       filename[c74::max::MAX_PATH_CHARS]{};
        std::strncpy(filename, watched_file.string().c_str(), c74::max::MAX_PATH_CHARS - 1);
        short              path_id{};
        c74::max::t_fourcc filetype{};
        if (c74::max::locatefile_extended(filename, &path_id, &filetype, nullptr, 0) == 0) {
            m_filewatcher = c74::max::filewatcher_new(maxobj(), path_id, filename);
            if (m_filewatcher) {
                c74::max::filewatcher_start(m_filewatcher);
            }
        }
        else {
            cerr << "Unable to watch " << watched_file.string() << " for changes." << endl;
        }
    }

    ~python() {
        if (m_filewatcher) {
            c74::max::object_free(m_filewatcher);
        }
        m_processor.reset(); // releases the Python objects under the GIL
    }

    /// (Re)import the user's module, instantiate its class, and rebuild the Max
    /// attributes and messages from the class's type hints. Called from the Max
    /// main thread (constructor and file watcher).
    void update_source() {
        if (!m_processor || !m_processor->load()) {
            return; // the processor has reported why, and outputs silence until the next reload
        }
        create_attributes();
        create_messages();
    }

    /// Dispatch a Max message to the bound Python method.
    /// Runs on the Max main or scheduler thread.
    void message_gimme(const symbol name, const long ac, const c74::max::t_atom* av) {
        if (!m_processor || !m_processor->loaded()) {
            return;
        }
        m_processor->call(name.c_str(), to_values(ac, av));
    }

    void attr_set(const symbol& name, const long argc, const c74::max::t_atom* argv) {
        if (!m_processor || !m_processor->loaded()) {
            return;
        }
        if (argc != 1) {
            cerr << "Attributes with more than 1 arg not supported" << endl;
            return;
        }

        auto found = m_python_attributes.find(name.c_str());
        if (found == m_python_attributes.end()) {
            return;
        }

        // the core converts to the field's hinted type (a bool field gets a bool)
        m_processor->set_attribute(name.c_str(), to_values(argc, argv).front());
    }

    void attr_get(const symbol& name, long* argc, c74::max::t_atom** argv) {
        if ((*argc) != 1 || !(*argv)) { // otherwise use memory passed in
            if (*argc && *argv) {
                c74::max::sysmem_freeptr(*argv);
                *argv = NULL;
            }
            *argc = 1;
            *argv = reinterpret_cast<c74::max::t_atom*>(c74::max::sysmem_newptr(sizeof(c74::max::t_atom) * (*argc)));
        }
        c74::max::atom_setfloat(*argv, 0.0); // default in case anything below fails

        if (!m_processor || !m_processor->loaded()) {
            return;
        }

        auto found = m_python_attributes.find(name.c_str());
        if (found == m_python_attributes.end()) {
            return;
        }

        // nothing to read (None, or a value that does not convert): the type's empty value
        const auto type = found->second->value_type();
        if (type == runtime::value_type::integer || type == runtime::value_type::boolean) {
            c74::max::atom_setlong(*argv, 0);
        }
        else if (type == runtime::value_type::symbol) {
            c74::max::atom_setsym(*argv, c74::max::gensym(""));
        }

        const auto result = m_processor->get_attribute(name.c_str(), type);
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

    /// The audio perform routine: the core calls the Python process() once per
    /// sample, holding the GIL for the vector, and outputs silence while unbound.
    void operator()(audio_bundle input, audio_bundle output) {
        if (!m_processor) {
            output.clear();
            return;
        }
        try {
            m_processor->process(input.samples(0), output.samples(0), static_cast<std::size_t>(input.frame_count()));
        }
        catch (...) { // never let an exception unwind into the audio driver
            output.clear();
        }
    }

  private:
    string                                                                    m_python_source{};
    std::filesystem::path                                                     m_scripts_dir{};
    void*                                                                     m_filewatcher{};
    std::unique_ptr<runtime::processor>                                       m_processor;
    std::unordered_map<std::string, std::unique_ptr<runtime::python_message>> m_python_messages;
    std::unordered_map<std::string, std::unique_ptr<runtime::python_attr>>    m_python_attributes;

    /// Messages the Max object handles itself, which a Python method must never replace: min's
    /// own class methods, the messages Max sends every object, and this object's file watcher
    /// (a Python method named filechanged would silently disable hot reload).
    static std::vector<std::string> reserved_messages() {
        return {"anything", "appendtodictionary", "assist",     "dblclick",  "dsp",      "dsp64",
                "dspsetup", "filechanged",        "getvalueof", "inletinfo", "loadbang", "notify",
                "preset",   "savestate",          "setvalueof", "signal"};
    }

    /// Python's print() output and tracebacks, for every instance.
    static void console_line(const runtime::log_level level, const std::string_view text) {
        const std::string line{text};
        if (level == runtime::log_level::error) {
            c74::max::object_error(nullptr, "python: %s", line.c_str());
        }
        else {
            c74::max::object_post(nullptr, "python: %s", line.c_str());
        }
    }

    /// Max atoms as core values, keeping each atom's own type (the core coerces to the
    /// hinted type with the same rules as atom_getlong/atom_getfloat/atom_getsym).
    static std::vector<runtime::value> to_values(const long ac, const c74::max::t_atom* av) {
        std::vector<runtime::value> values;
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

    /// Make the Max attributes match the class's annotated fields: remove the ones the class no
    /// longer has, recreate the ones whose type changed, add the new ones. (The core has already
    /// carried the values of those that stayed over to the new instance.)
    void create_attributes() {
        const auto& current = m_processor->attributes();
        for (auto it = m_python_attributes.begin(); it != m_python_attributes.end();) {
            const auto found = std::find_if(current.begin(), current.end(),
                                            [&](const runtime::attribute_info& a) { return a.name == it->first; });
            if (found == current.end() || found->type != it->second->value_type()) {
                it->second->remove();
                it = m_python_attributes.erase(it);
            }
            else {
                ++it;
            }
        }
        for (const auto& attribute : current) {
            if (m_python_attributes.find(attribute.name) == m_python_attributes.end()) {
                m_python_attributes[attribute.name] =
                    std::make_unique<runtime::python_attr>(maxobj(), attribute.name, attribute.type);
            }
        }
    }

    /// Replace the previous incarnation's Max messages with the class's current methods.
    void create_messages() {
        for (auto& element : m_python_messages) {
            c74::max::object_deletemethod(maxobj(), c74::max::gensym(element.first.c_str()));
        }
        m_python_messages.clear();

        for (const auto& message : m_processor->messages()) { // never names an attribute
            m_python_messages[message.name] = std::make_unique<runtime::python_message>(maxobj(), message.name);
        }
    }
};
