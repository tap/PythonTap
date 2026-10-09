/// @file tap.python_filewatch.h
/// @brief Watches the class file, and delivers a save to the object as its filechanged message.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// Max's file watcher calls its owner's `filechanged` method directly, with the C signature
// (t_object*, char* filename, short path) that the Max SDK documents for an A_CANT method. The
// object's own filechanged is a min message<>, which min registers with its A_GIMME wrapper —
// called that way, the wrapper reads the file name as a t_symbol* and crashes Max, on every save
// (plan 6.1 found it: the mock kernel has no file watcher). A message a patcher sends cannot reach
// an A_CANT method, so filechanged cannot simply be re-registered as one without losing the
// documented "send filechanged to reload". Instead the watcher belongs to this small nobox object,
// whose filechanged has the SDK's signature and passes the event on as a typed message.

#pragma once

#include "c74_min_api.h"

namespace tap::python {

    namespace detail {

        struct file_watch_object {
            c74::max::t_object  header;
            c74::max::t_object* target; // the object told of each save
        };

        /// Called by Max's file watcher, on the main thread.
        inline void file_watch_changed(file_watch_object* self, char* /*filename*/, short /*path*/) {
            c74::max::object_method_typed(self->target, c74::max::gensym("filechanged"), 0, nullptr, nullptr);
        }

        /// The nobox class, registered on first use (main thread).
        inline c74::max::t_class* file_watch_class() {
            static c74::max::t_class* s_class = [] {
                auto* c = c74::max::class_new("tap.python~.filewatch", nullptr, nullptr, sizeof(file_watch_object),
                                              nullptr, 0, 0);
                c74::max::class_addmethod(c, reinterpret_cast<c74::max::method>(file_watch_changed), "filechanged",
                                          c74::max::A_CANT, 0);
                c74::max::class_register(c74::max::gensym("nobox"), c);
                return c;
            }();
            return s_class;
        }

    } // namespace detail

    /// A running watch on one file, telling `target` of each save by sending it `filechanged`.
    /// Main thread only, for construction and destruction.
    class file_watch {
      public:
        /// @param target    the object to send filechanged to; must outlive the watch
        /// @param path      the file's folder, as locatefile_extended() returns it
        /// @param filename  the file's name in that folder
        file_watch(c74::max::t_object* target, const short path, const char* filename) {
            m_owner = static_cast<detail::file_watch_object*>(c74::max::object_alloc(detail::file_watch_class()));
            if (!m_owner) {
                return;
            }
            m_owner->target = target;
            m_watcher       = c74::max::filewatcher_new(&m_owner->header, path, filename);
            if (m_watcher) {
                c74::max::filewatcher_start(m_watcher);
            }
        }

        ~file_watch() {
            if (m_watcher) {
                c74::max::object_free(m_watcher); // first: it must not call a freed owner
            }
            if (m_owner) {
                c74::max::object_free(m_owner);
            }
        }

        file_watch(const file_watch&)            = delete;
        file_watch& operator=(const file_watch&) = delete;

        /// Whether the file is being watched.
        bool watching() const noexcept { return m_watcher != nullptr; }

      private:
        detail::file_watch_object* m_owner{};
        void*                      m_watcher{};
    };

} // namespace tap::python
