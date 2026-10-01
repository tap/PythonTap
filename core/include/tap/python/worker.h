/// @file worker.h
/// @brief Worker mode (plan 2.5): a processor's process() on a thread of its own, a fixed number of
///        vectors behind the audio thread, which never takes the GIL.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// Host-independent. In direct mode the audio thread calls processor::process() and so takes the
// GIL: whatever else holds it — a reload compiling a large file, a message handler, a garbage
// collection — delays the audio. A worker moves that call to a thread of its own. The audio thread
// calls worker::process() once per vector, which copies the vector's inputs into a ring of slots
// and copies out the outputs of the vector `latency` vectors before it; the worker thread runs
// processor::process() on each slot in order — per sample or per vector, with the processor's
// channel matching — so the class contract does not change. The first `latency` vectors out are
// silence.
//
// A vector whose outputs are not ready when they are due (an underrun) is output as silence. The
// worker still processes it when it gets there, so the class sees every vector in order and the
// latency stays fixed; only that vector's output is lost. If the worker falls a whole ring behind
// (the latency plus a quarter of a second), the audio thread drops the inputs of new vectors until
// it catches up: the class sees a gap. Both are reported once per load, from the main thread
// (flush_reports()); the audio thread never prints, allocates, locks or calls Python.
//
// Threads: start(), stop(), flush_reports() and the destructor run on the host's main thread,
// never while it holds the GIL (stopping joins the worker, which may be waiting for it); process()
// on the audio thread, possibly at the same time as any of them (a host can run the old signal
// chain while it prepares the next). The audio thread borrows the ring for each vector by taking
// an atomic pointer, so start() and stop() wait at most one vector for it, and a vector that
// arrives while they hold it is silence.

#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "tap/python/processor.h"

namespace tap::python {

    class worker {
      public:
        /// The latency, in milliseconds, unless the host says otherwise (plan 2.5). A host that computes
        /// several vectors at a time (Max: one I/O vector's worth, back to back) leaves the worker only
        /// the latency beyond that burst to do them in, so this covers a 512-sample I/O vector at
        /// 44.1 kHz (11.6 ms) with room to spare: measured in Max, 4.7 ms of room was not enough.
        static constexpr double k_default_latency_ms = 30.0;

        /// A latency in milliseconds as whole vectors of `vector_size` at `sample_rate`: rounded up,
        /// and at least one.
        static std::size_t latency_vectors(const double milliseconds, const double sample_rate,
                                           const std::size_t vector_size) {
            if (!(milliseconds > 0.0) || !(sample_rate > 0.0) || vector_size == 0 || !std::isfinite(milliseconds)
                || !std::isfinite(sample_rate)) {
                return 1;
            }
            const double vectors = milliseconds / 1000.0 * sample_rate / static_cast<double>(vector_size);
            return std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(vectors - 1e-9)));
        }

        /// @param target       the processor to run; it must outlive the worker
        /// @param log          receives the worker's reports (main thread)
        /// @param report_ready called on the audio thread when something needs reporting; the host
        ///                     must then call flush_reports() from its main thread. Must be
        ///                     real-time safe, as the processor's is (it may be the same callback).
        /// @param thread_setup called on each worker thread as it starts, with the vector period in
        ///                     seconds (0 if the sample rate is unknown): where the host gives the
        ///                     thread the scheduling an audio thread has. Without it, a busy machine
        ///                     can hold an ordinary thread off for longer than the latency.
        explicit worker(processor& target, log_function log = {}, std::function<void()> report_ready = {},
                        std::function<void(double)> thread_setup = {})
            : m_target{target}
            , m_log{std::move(log)}
            , m_report_ready{std::move(report_ready)}
            , m_thread_setup{std::move(thread_setup)} {}

        ~worker() { stop(); }

        worker(const worker&)            = delete;
        worker& operator=(const worker&) = delete;

        /// Start the worker, or restart it with new settings: a ring for `inputs` and `outputs` host
        /// channels of `vector_size` frames, `latency` vectors deep (at least 1), with room for the
        /// worker to fall a quarter of a second behind at `sample_rate` (at least 16 vectors).
        /// Whatever the previous worker had not processed is discarded. Returns once the new thread
        /// is running with its Python thread state, which it takes the GIL to make: main thread,
        /// never holding the GIL.
        void start(const std::size_t inputs, const std::size_t outputs, const std::size_t vector_size,
                   const std::size_t latency, const double sample_rate) {
            stop();
            const auto vectors = std::max<std::size_t>(1, latency);
            const auto frames  = std::max<std::size_t>(1, vector_size);
            const auto backlog =
                std::isfinite(sample_rate) && sample_rate > 0.0
                    ? static_cast<std::size_t>(std::ceil(k_backlog_seconds * sample_rate / static_cast<double>(frames)))
                    : std::size_t{0};
            auto next =
                std::make_unique<ring>(inputs, outputs, frames, vectors, vectors + std::max(k_min_backlog, backlog));
            ring*      r = next.get();
            const auto period =
                std::isfinite(sample_rate) && sample_rate > 0.0 ? static_cast<double>(frames) / sample_rate : 0.0;
            r->thread = std::thread{[this, r, period] {
                if (m_thread_setup) {
                    m_thread_setup(period);
                }
                if (Py_IsInitialized()) {
                    gil_lock lock; // the thread's Python thread state, made now rather than on its first vector
                }
                r->ready.store(true, std::memory_order_release);
                r->ready.notify_one();
                run(*r);
            }};
            r->ready.wait(false, std::memory_order_acquire); // ready before the audio thread can give it work
            m_owned   = std::move(next);
            m_latency = vectors * frames;
            m_ring.store(r, std::memory_order_release);
        }

        /// Stop the worker and join its thread; process() outputs silence until the next start().
        /// Main thread.
        void stop() {
            if (!m_owned) {
                return;
            }
            // take the ring back from the audio thread, which holds it for one vector at most
            ring* r = m_ring.exchange(nullptr, std::memory_order_acq_rel);
            while (!r) {
                std::this_thread::yield();
                r = m_ring.exchange(nullptr, std::memory_order_acq_rel);
            }
            r->stopping.store(true, std::memory_order_release);
            r->signal.fetch_add(1, std::memory_order_release);
            r->signal.notify_one();
            r->thread.join();
            m_owned.reset();
            m_latency = 0;
        }

        /// True between start() and stop(). Main thread.
        bool running() const noexcept { return m_owned != nullptr; }

        /// The latency in samples: the vectors it was started with times the vector size; 0 when
        /// stopped. Main thread.
        std::size_t latency_samples() const noexcept { return m_latency; }

        /// How many vectors the worker has processed (or passed over, dropped) since start(). Main
        /// thread; for tests and diagnostics.
        std::uint64_t processed() const noexcept { return m_owned ? m_owned->done.load(std::memory_order_acquire) : 0; }

        /// The audio thread's side, once per vector: queue this vector's inputs and output those of
        /// the vector `latency` before (silence if the worker has not finished it). Inputs the host
        /// does not give read as silence, outputs it does not take are dropped, and the class's
        /// own matching applies on the worker. Never blocks, allocates, or takes the GIL.
        void process(const double* const* inputs, const std::size_t input_count, double* const* outputs,
                     const std::size_t output_count, const std::size_t frame_count) {
            ring* r = m_ring.exchange(nullptr, std::memory_order_acquire);
            if (!r) {
                silence(outputs, output_count, 0, frame_count);
                return;
            }
            const auto frames = std::min(frame_count, r->vector_size);

            // this vector's inputs, unless the worker is a whole ring behind
            const auto k = r->written.load(std::memory_order_relaxed); // written only here
            if (k - r->done.load(std::memory_order_acquire) < r->slot_count) {
                const auto index = k % r->slot_count;
                for (std::size_t c = 0; c < r->inputs; ++c) {
                    double* to = r->input(index, c);
                    if (c < input_count && inputs[c]) {
                        std::copy_n(inputs[c], frames, to);
                    }
                    else {
                        std::fill_n(to, frames, 0.0);
                    }
                }
                r->slots[index].frames.store(frames, std::memory_order_relaxed);
                r->slots[index].in_seq.store(k, std::memory_order_release);
            }
            else {
                record(m_dropped, m_dropped_load);
            }
            r->written.store(k + 1, std::memory_order_release);
            r->signal.fetch_add(1, std::memory_order_release);
            r->signal.notify_one();

            // the outputs of the vector `latency` before (the first `latency` vectors have none)
            std::size_t given = 0; // frames of output given; the rest is silence
            if (k >= r->latency) {
                const auto m     = k - r->latency;
                const auto index = m % r->slot_count;
                auto&      slot  = r->slots[index];
                if (r->done.load(std::memory_order_acquire) > m && slot.out_seq.load(std::memory_order_acquire) == m) {
                    given = std::min(slot.frames.load(std::memory_order_relaxed), frame_count);
                    for (std::size_t c = 0; c < output_count; ++c) {
                        if (c < r->outputs) {
                            std::copy_n(r->output(index, c), given, outputs[c]);
                        }
                        else {
                            std::fill_n(outputs[c], given, 0.0);
                        }
                    }
                }
                else if (slot.in_seq.load(std::memory_order_relaxed) == m) {
                    record(m_late, m_late_load); // its inputs were queued: the worker is late
                }
            }
            silence(outputs, output_count, given, frame_count);

            m_ring.store(r, std::memory_order_release);
        }

        /// Print what the audio thread recorded since the last flush: vectors output as silence
        /// because the worker was late, and inputs dropped because it fell a whole ring behind.
        /// Main thread.
        void flush_reports() {
            if (const auto late = m_late.exchange(0, std::memory_order_acq_rel); late != 0) {
                log(log_level::error, "process() was late for " + std::to_string(late)
                                          + " vector(s) in worker mode — output as silence, the latency kept "
                                            "(reported once per load)");
            }
            if (const auto dropped = m_dropped.exchange(0, std::memory_order_acq_rel); dropped != 0) {
                log(log_level::error, "process() fell too far behind in worker mode — the input of "
                                          + std::to_string(dropped)
                                          + " vector(s) was dropped (reported once per load)");
            }
        }

      private:
        static constexpr std::uint64_t k_none            = std::numeric_limits<std::uint64_t>::max();
        static constexpr double        k_backlog_seconds = 0.25;
        static constexpr std::size_t   k_min_backlog     = 16;

        struct slot {
            std::atomic<std::uint64_t> in_seq{k_none};  // the vector whose inputs it holds
            std::atomic<std::uint64_t> out_seq{k_none}; // the vector whose outputs it holds
            std::atomic<std::size_t>   frames{};
        };

        /// One run of the worker: the slots, and the thread that processes them.
        struct ring {
            ring(const std::size_t input_channels, const std::size_t output_channels, const std::size_t frames,
                 const std::size_t vectors, const std::size_t count)
                : inputs{input_channels}
                , outputs{output_channels}
                , vector_size{frames}
                , latency{vectors}
                , slot_count{count}
                , input_samples(count * input_channels * frames)
                , output_samples(count * output_channels * frames)
                , slots(count) {}

            double* input(const std::size_t index, const std::size_t channel) {
                return input_samples.data() + ((index * inputs) + channel) * vector_size;
            }
            double* output(const std::size_t index, const std::size_t channel) {
                return output_samples.data() + ((index * outputs) + channel) * vector_size;
            }

            std::size_t                inputs;
            std::size_t                outputs;
            std::size_t                vector_size;
            std::size_t                latency;
            std::size_t                slot_count;
            std::vector<double>        input_samples;
            std::vector<double>        output_samples;
            std::vector<slot>          slots;
            std::atomic<std::uint64_t> written{}; // vectors the audio thread has queued
            std::atomic<std::uint64_t> done{};    // vectors the worker has finished (or passed over)
            std::atomic<std::uint64_t> signal{};  // bumped to wake the worker
            std::atomic<bool>          stopping{};
            std::atomic<bool>          ready{}; // the thread has started and has its Python thread state
            std::thread                thread;
        };

        processor&                  m_target;
        log_function                m_log;
        std::function<void()>       m_report_ready;
        std::function<void(double)> m_thread_setup;

        std::atomic<ring*>    m_ring{};    // the running ring, while the audio thread is not using it
        std::unique_ptr<ring> m_owned;     // the running ring (main thread)
        std::size_t           m_latency{}; // in samples (main thread)

        std::atomic<std::uint64_t> m_late{};               // vectors output as silence since the last flush
        std::atomic<std::uint64_t> m_dropped{};            // vectors whose inputs were dropped
        std::atomic<std::uint64_t> m_late_load{k_none};    // the load for which late vectors were last reported
        std::atomic<std::uint64_t> m_dropped_load{k_none}; // the same, for dropped ones

        /// The worker thread: process each queued vector in order, sleeping while there is none.
        void run(ring& r) {
            std::vector<const double*> in(r.inputs);
            std::vector<double*>       out(r.outputs);
            std::uint64_t              next = 0;
            for (;;) {
                const auto signalled = r.signal.load(std::memory_order_acquire);
                if (r.stopping.load(std::memory_order_acquire)) {
                    return;
                }
                if (next == r.written.load(std::memory_order_acquire)) {
                    r.signal.wait(signalled, std::memory_order_acquire);
                    continue;
                }
                const auto index = next % r.slot_count;
                auto&      slot  = r.slots[index];
                if (slot.in_seq.load(std::memory_order_acquire) == next) { // not dropped
                    for (std::size_t c = 0; c < r.inputs; ++c) {
                        in[c] = r.input(index, c);
                    }
                    for (std::size_t c = 0; c < r.outputs; ++c) {
                        out[c] = r.output(index, c);
                    }
                    const auto frames = slot.frames.load(std::memory_order_relaxed);
                    try {
                        m_target.process(in.data(), r.inputs, out.data(), r.outputs, frames);
                    }
                    catch (...) { // an exception must not end the thread (and with it, the host)
                        silence(out.data(), r.outputs, 0, frames);
                    }
                    slot.out_seq.store(next, std::memory_order_release);
                }
                ++next;
                r.done.store(next, std::memory_order_release);
            }
        }

        /// Count one late or dropped vector (audio thread), and ask for a report if it is the first
        /// since the class was last loaded; later ones are counted into that report until it is
        /// flushed, and then not again until the next load.
        void record(std::atomic<std::uint64_t>& count, std::atomic<std::uint64_t>& reported_load) {
            const auto load = m_target.load_count();
            if (reported_load.load(std::memory_order_relaxed) == load) {
                // reported already: count it into the report if that is still pending (never into one
                // flushed meanwhile, which would carry it over to the next load's)
                auto pending = count.load(std::memory_order_relaxed);
                while (pending != 0 && !count.compare_exchange_weak(pending, pending + 1, std::memory_order_relaxed)) {
                }
                return;
            }
            reported_load.store(load, std::memory_order_relaxed);
            count.fetch_add(1, std::memory_order_release);
            if (m_report_ready) {
                m_report_ready();
            }
        }

        void log(const log_level level, const std::string& text) const {
            if (m_log) {
                m_log(level, text);
            }
        }

        /// Zero `count` channels of `out` from `first_frame` to `frame_count`.
        static void silence(double* const* out, const std::size_t count, const std::size_t first_frame,
                            const std::size_t frame_count) {
            for (std::size_t c = 0; c < count; ++c) {
                if (first_frame < frame_count) {
                    std::fill(out[c] + first_frame, out[c] + frame_count, 0.0);
                }
            }
        }
    };

} // namespace tap::python
