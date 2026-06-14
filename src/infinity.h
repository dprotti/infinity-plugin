/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#pragma once

#include <atomic>
#include <deque>
#include <glib.h>
#include <memory>
#include <mutex>
#include <random>
#include <thread>

#include "display.h"
#include "effects.h"
#include "input.h"

#include "config.h"
#if CAPTURE_BACKEND_PULSEAUDIO
#  include "pulseaudio_capture.h"
   using CaptureBackend = PulseAudioCapture;
#else
#  include "pipewire_capture.h"
   using CaptureBackend = PipeWireCapture;
#endif

// Audio capture backend selected at build time (see meson_options.txt).
enum class CaptureBknd { PipeWire, PulseAudio };

struct StandaloneParams {
    gint32 width           = 1280;
    gint32 height          = 720;
    gint32 scale           = 1;
    gint32 effect_interval = 300;
    gint32 color_interval  = 150;
    gint32 max_fps         = 30;
    gint32 sample_rate     = 44100;
};

class Infinity {
public:
    explicit Infinity(const StandaloneParams& params = {});
    ~Infinity();

    void finish();

    [[nodiscard]] bool window_closed() const { return display_.window_closed(); }

private:
    void renderer();
    void handle_key_event(InfinityKey key);
    void queue_key(InfinityKey key);
    void process_key_queue();
    [[nodiscard]] gint64 calculate_frame_length_usecs(gint32 fps, int line);

    Display display_;
    std::unique_ptr<CaptureBackend> capture_;
    StandaloneParams params_;
    std::mt19937 rng_;

    gint32 width_{0};
    gint32 height_{0};
    gint32 scale_{0};

    t_effect current_effect_{};
    t_color color_{0};
    t_color old_color_{0};
    t_num_effect t_last_color_{0};
    t_num_effect t_last_effect_{0};

    bool must_resize_{false};
    bool finished_{false};
    bool resizing_{false};
    std::mutex resize_mutex_;

    std::atomic<bool> initializing_{false};
    std::atomic<bool> quiting_{false};

    std::thread render_thread_;

    std::deque<InfinityKey> key_queue_;
    std::mutex key_mutex_;
};
