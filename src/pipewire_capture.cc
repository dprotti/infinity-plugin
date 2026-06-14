// Copyright (c) 2026 Duilio Protti
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pipewire_capture.h"
#include "capture_config.h"
#include "display.h"

#include <glib.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

struct PipeWireCapture::Impl {
    Display& display;
    CaptureConfig config;
    std::atomic<bool> running{false};
    std::thread capture_thread;

    pw_main_loop* loop = nullptr;
    pw_context* context = nullptr;
    pw_core* core = nullptr;
    pw_stream* stream = nullptr;

    Impl(Display& d, const CaptureConfig& c)
        : display(d), config(c) {}

    static void on_state_changed(void* userdata, enum pw_stream_state old,
                                 enum pw_stream_state state, const char *error) {
        auto* impl = static_cast<Impl*>(userdata);
        if (!impl) return;

        g_message("PipeWire stream state: %s -> %s (error: %s)",
                  pw_stream_state_as_string(old),
                  pw_stream_state_as_string(state),
                  error ? error : "none");

        if (state == PW_STREAM_STATE_ERROR) {
            g_critical("PipeWire stream error: %s", error ? error : "unknown");
        } else if (state == PW_STREAM_STATE_STREAMING) {
            g_message("✅ PipeWire stream is streaming. Visualizations will now react to audio.");
        }
    }

    static void on_process(void* userdata) {
        Impl* impl = static_cast<Impl*>(userdata);
        if (!impl || !impl->running.load()) return;

        pw_buffer* buf = pw_stream_dequeue_buffer(impl->stream);
        if (buf == nullptr) return;

        spa_data* d = &buf->buffer->datas[0];
        if (d->data == nullptr) {
            pw_stream_queue_buffer(impl->stream, buf);
            return;
        }

        const float* samples = static_cast<const float*>(d->data);
        uint32_t n_frames = buf->buffer->datas[0].chunk->size / (sizeof(float) * 2);

        if (n_frames > 0) {
            impl->display.set_pcm_data(samples, 2);
        }

        pw_stream_queue_buffer(impl->stream, buf);
    }

    static void on_param_changed(void*, uint32_t id, const struct spa_pod* param) {
        if (id != SPA_PARAM_Format || param == nullptr) return;
        g_message("PipeWire: format negotiated (param_changed SPA_PARAM_Format received)");
    }

    static struct pw_stream_events stream_events;
};

struct pw_stream_events PipeWireCapture::Impl::stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = on_state_changed,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = on_param_changed,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = on_process,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr,
};

PipeWireCapture::PipeWireCapture(Display& display, const CaptureConfig& config)
    : impl_(std::make_unique<Impl>(display, config)) {}

PipeWireCapture::~PipeWireCapture() {
    stop();
}

bool PipeWireCapture::start() {
    if (impl_->running.load()) return true;

    g_message("PipeWireCapture: initializing...");

    pw_init(nullptr, nullptr);

    impl_->loop = pw_main_loop_new(nullptr);
    if (!impl_->loop) {
        g_critical("PipeWire: failed to create main loop");
        return false;
    }

    impl_->context = pw_context_new(pw_main_loop_get_loop(impl_->loop), nullptr, 0);
    if (!impl_->context) {
        g_critical("PipeWire: failed to create context");
        return false;
    }

    impl_->core = pw_context_connect(impl_->context, nullptr, 0);
    if (!impl_->core) {
        g_critical("PipeWire: failed to connect to core");
        return false;
    }

    impl_->stream = pw_stream_new_simple(
        pw_main_loop_get_loop(impl_->loop),
        "infinity-visualizer",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Capture",
            PW_KEY_MEDIA_ROLE, "Music",
            PW_KEY_NODE_NAME, "infinity-visualizer",
            PW_KEY_STREAM_CAPTURE_SINK, "true",
            "node.suspend-on-idle", "false",
            "node.pause-on-idle", "false",
            nullptr),
        &Impl::stream_events,
        impl_.get());

    if (!impl_->stream) {
        g_critical("PipeWire: failed to create stream");
        return false;
    }

    spa_audio_info_raw audio_info{};
    audio_info.format = SPA_AUDIO_FORMAT_F32;
    audio_info.channels = 2;
    audio_info.rate = impl_->config.sample_rate;

    const struct spa_pod *params[1] = {nullptr};
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(nullptr, 0);
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audio_info);

    int res = pw_stream_connect(impl_->stream,
                                PW_DIRECTION_INPUT,
                                PW_ID_ANY,
                                static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                                             PW_STREAM_FLAG_MAP_BUFFERS |
                                                             PW_STREAM_FLAG_RT_PROCESS),
                                params, 1);

    if (res < 0) {
        g_critical("PipeWire: pw_stream_connect failed: %s", g_strerror(-res));
        return false;
    }

    g_message("PipeWire: stream connected, waiting for STREAMING state...");

    impl_->running.store(true);
    impl_->capture_thread = std::thread([this]() {
        // The PipeWire handshake is multi-step: the server sends format
        // negotiation events (param_changed) *after* the initial PAUSED
        // state. pw_stream_set_active() only works once a format has been
        // agreed upon. We pump the loop one iteration at a time until the
        // stream reaches STREAMING or we hit a 3-second timeout, then fall
        // through to pw_main_loop_run() which handles the ongoing capture.
        struct pw_loop* loop = pw_main_loop_get_loop(impl_->loop);
        using Clock = std::chrono::steady_clock;
        auto deadline = Clock::now() + std::chrono::seconds(3);

        while (Clock::now() < deadline) {
            // Pump one iteration with a 50ms timeout so we don't spin.
            pw_loop_iterate(loop, 50 /*ms*/);

            if (!impl_->stream) break;

            enum pw_stream_state st = pw_stream_get_state(impl_->stream, nullptr);

            if (st == PW_STREAM_STATE_STREAMING) {
                // Already streaming (can happen if the server was fast).
                break;
            }
            if (st == PW_STREAM_STATE_PAUSED) {
                // Format has been negotiated; request transition to STREAMING.
                // If the server isn't ready yet this is a no-op and we retry
                // on the next iteration.
                pw_stream_set_active(impl_->stream, true);
            }
            if (st == PW_STREAM_STATE_ERROR) {
                g_critical("PipeWire: stream error during handshake, giving up");
                break;
            }
        }

        pw_main_loop_run(impl_->loop);
    });

    return true;
}

void PipeWireCapture::stop() {
    if (!impl_->running.load()) return;
    impl_->running.store(false);

    g_message("PipeWireCapture: shutting down...");

    if (impl_->stream) pw_stream_destroy(impl_->stream);
    if (impl_->loop) pw_main_loop_quit(impl_->loop);
    if (impl_->capture_thread.joinable()) impl_->capture_thread.join();
    if (impl_->core) pw_core_disconnect(impl_->core);
    if (impl_->context) pw_context_destroy(impl_->context);
    if (impl_->loop) pw_main_loop_destroy(impl_->loop);

    pw_deinit();
    g_message("PipeWireCapture: shutdown complete");
}
