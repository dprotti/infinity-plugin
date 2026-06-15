// Copyright (c) 2026 Duilio Protti
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pulseaudio_capture.h"
#include "capture_config.h"
#include "display.h"

#include <glib.h>
#include <pulse/pulseaudio.h>

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct PulseAudioCapture::Impl {
    Display&      display;
    CaptureConfig config;

    std::atomic<bool> running{false};
    std::atomic<bool> shutting_down{false};
    std::thread       capture_thread;

    // PulseAudio async objects – all owned by the mainloop thread.
    pa_threaded_mainloop* mainloop = nullptr;
    pa_context*           context  = nullptr;
    pa_stream*            stream   = nullptr;

    // Filled by the server-info callback; empty until then.
    std::string default_sink_monitor;

    Impl(Display& d, const CaptureConfig& c) : display(d), config(c) {}

    // ------------------------------------------------------------------
    // pa_context state callback
    // ------------------------------------------------------------------
    static void on_context_state(pa_context* ctx, void* userdata) {
        auto* impl = static_cast<Impl*>(userdata);
        pa_context_state_t state = pa_context_get_state(ctx);

        if (state == PA_CONTEXT_READY) {
            g_message("PulseAudio: context ready, querying default sink...");
            // Ask for server info so we can learn the default sink name.
            pa_context_get_server_info(ctx, on_server_info, impl);
        } else if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
            if (!impl->shutting_down.load()) {
                g_critical("PulseAudio: context failed or terminated");
            }
            pa_threaded_mainloop_signal(impl->mainloop, 0);
        }
    }

    // ------------------------------------------------------------------
    // pa_context_get_server_info callback
    // ------------------------------------------------------------------
    static void on_server_info(pa_context* ctx, const pa_server_info* info, void* userdata) {
        auto* impl = static_cast<Impl*>(userdata);
        if (!info || !info->default_sink_name) {
            g_critical("PulseAudio: could not get server info");
            pa_threaded_mainloop_signal(impl->mainloop, 0);
            return;
        }

        // The monitor source for a sink is always "<sink_name>.monitor".
        impl->default_sink_monitor = std::string(info->default_sink_name) + ".monitor";
        g_message("PulseAudio: targeting monitor source: %s",
                  impl->default_sink_monitor.c_str());

        // Now create the recording stream.
        pa_sample_spec spec{};
        spec.format   = PA_SAMPLE_FLOAT32LE;
        spec.rate     = static_cast<uint32_t>(impl->config.sample_rate);
        spec.channels = static_cast<uint8_t>(impl->config.channels);

        impl->stream = pa_stream_new(ctx, "infinity-visualizer", &spec, nullptr);
        if (!impl->stream) {
            g_critical("PulseAudio: pa_stream_new() failed");
            pa_threaded_mainloop_signal(impl->mainloop, 0);
            return;
        }

        pa_stream_set_read_callback(impl->stream, on_stream_read, impl);
        pa_stream_set_state_callback(impl->stream, on_stream_state, impl);

        // Use a modest latency; we just need steady chunks for visualization.
        pa_buffer_attr attr{};
        attr.maxlength = static_cast<uint32_t>(-1);
        attr.fragsize  = static_cast<uint32_t>(
            pa_usec_to_bytes(30000 /*µs*/, &spec)); // ~30 ms chunks

        int r = pa_stream_connect_record(
            impl->stream,
            impl->default_sink_monitor.c_str(),
            &attr,
            static_cast<pa_stream_flags_t>(
                PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE));

        if (r < 0) {
            g_critical("PulseAudio: pa_stream_connect_record() failed: %s",
                       pa_strerror(pa_context_errno(ctx)));
            pa_stream_unref(impl->stream);
            impl->stream = nullptr;
            pa_threaded_mainloop_signal(impl->mainloop, 0);
        }
    }

    // ------------------------------------------------------------------
    // pa_stream state callback
    // ------------------------------------------------------------------
    static void on_stream_state(pa_stream* s, void* userdata) {
        auto* impl = static_cast<Impl*>(userdata);
        pa_stream_state_t state = pa_stream_get_state(s);

        if (state == PA_STREAM_READY) {
            g_message("PulseAudio: stream READY – visualizations will react to audio");
            pa_threaded_mainloop_signal(impl->mainloop, 0);
        } else if (state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED) {
            if (!impl->shutting_down.load()) {
                g_critical("PulseAudio: stream failed or terminated: %s",
                        pa_strerror(pa_context_errno(pa_stream_get_context(s))));
            }
            pa_threaded_mainloop_signal(impl->mainloop, 0);
        }
    }

    // ------------------------------------------------------------------
    // pa_stream read callback – called whenever audio data is available
    // ------------------------------------------------------------------
    static void on_stream_read(pa_stream* s, size_t /*nbytes*/, void* userdata) {
        auto* impl = static_cast<Impl*>(userdata);
        if (!impl->running.load()) return;

        const void* data = nullptr;
        size_t      bytes = 0;

        if (pa_stream_peek(s, &data, &bytes) < 0) return;

        if (data && bytes > 0) {
            const float* samples = static_cast<const float*>(data);
            int n_frames = static_cast<int>(bytes / (sizeof(float) * impl->config.channels));
            if (n_frames > 0) {
                impl->display.set_pcm_data(samples, impl->config.channels);
            }
        }

        // Always drop even on a hole (data == nullptr) to advance the read pointer.
        pa_stream_drop(s);
    }
};

// ---------------------------------------------------------------------------
// PulseAudioCapture public API
// ---------------------------------------------------------------------------

PulseAudioCapture::PulseAudioCapture(Display& display, const CaptureConfig& config)
    : impl_(std::make_unique<Impl>(display, config)) {}

PulseAudioCapture::~PulseAudioCapture() { stop(); }

bool PulseAudioCapture::start() {
    if (impl_->running.load()) return true;

    g_message("PulseAudioCapture: initializing...");

    impl_->mainloop = pa_threaded_mainloop_new();
    if (!impl_->mainloop) {
        g_critical("PulseAudio: pa_threaded_mainloop_new() failed");
        return false;
    }

    pa_mainloop_api* api = pa_threaded_mainloop_get_api(impl_->mainloop);

    impl_->context = pa_context_new(api, "infinity-visualizer");
    if (!impl_->context) {
        g_critical("PulseAudio: pa_context_new() failed");
        pa_threaded_mainloop_free(impl_->mainloop);
        impl_->mainloop = nullptr;
        return false;
    }

    pa_context_set_state_callback(impl_->context, Impl::on_context_state, impl_.get());

    // Lock before connect so we don't miss the signal in on_stream_state.
    pa_threaded_mainloop_lock(impl_->mainloop);

    if (pa_context_connect(impl_->context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        g_critical("PulseAudio: pa_context_connect() failed: %s",
                   pa_strerror(pa_context_errno(impl_->context)));
        pa_threaded_mainloop_unlock(impl_->mainloop);
        pa_context_unref(impl_->context);
        impl_->context = nullptr;
        pa_threaded_mainloop_free(impl_->mainloop);
        impl_->mainloop = nullptr;
        return false;
    }

    pa_threaded_mainloop_start(impl_->mainloop);

    // Wait until the stream reaches READY (or fails).
    // on_stream_state signals the mainloop when either happens.
    pa_threaded_mainloop_wait(impl_->mainloop);
    pa_threaded_mainloop_unlock(impl_->mainloop);

    if (!impl_->stream || pa_stream_get_state(impl_->stream) != PA_STREAM_READY) {
        g_critical("PulseAudioCapture: failed to reach READY state");
        stop();
        return false;
    }

    impl_->running.store(true);
    return true;
}

void PulseAudioCapture::stop() {
    if (!impl_->mainloop) return;

    impl_->running.store(false);
    impl_->shutting_down.store(true);

    pa_threaded_mainloop_lock(impl_->mainloop);

    if (impl_->stream) {
        pa_stream_disconnect(impl_->stream);
        pa_stream_unref(impl_->stream);
        impl_->stream = nullptr;
    }
    if (impl_->context) {
        pa_context_disconnect(impl_->context);
        pa_context_unref(impl_->context);
        impl_->context = nullptr;
    }

    pa_threaded_mainloop_unlock(impl_->mainloop);
    pa_threaded_mainloop_stop(impl_->mainloop);
    pa_threaded_mainloop_free(impl_->mainloop);
    impl_->mainloop = nullptr;

    g_message("PulseAudioCapture: shutdown complete");
}
