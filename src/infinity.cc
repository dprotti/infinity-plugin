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
#include "config.h"
#include "infinity.h"
#include "types.h"

#include <glib.h>
#include <chrono>

Infinity::Infinity(const StandaloneParams& params)
    : display_([this](InfinityKey key) { queue_key(key); }),
      params_(params),
      rng_(std::random_device{}()) {

    gint32 _try = 0;
    if (initializing_.load()) {
        while (initializing_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (_try++ > 10) {
                g_critical("Infinity: failed to initialize!");
                return;
            }
        }
    }
    initializing_.store(true);

    width_ = params.width;
    height_ = params.height;
    scale_ = params.scale;

    if (!effects_load_effects()) {
        g_critical("Failed to load effects");
    }

    if (!display_.init(width_, height_, scale_)) {
        g_critical("Infinity: cannot initialize display");
        initializing_.store(false);
        finished_ = true;
        return;
    }

    capture_ = std::make_unique<CaptureBackend>(display_, CaptureConfig{params_.sample_rate, 2});
    if (!capture_->start()) {
        g_critical("Failed to start audio capture");
    }

    finished_ = false;
    must_resize_ = false;
    resizing_ = false;
    quiting_ = false;

    display_.load_random_effect(&current_effect_);

    render_thread_ = std::thread(&Infinity::renderer, this);
    initializing_.store(false);
}

Infinity::~Infinity() {
    finish();
}

void Infinity::finish() {
    if (finished_) { return; }

    quiting_ = true;
    finished_ = true;

    if (capture_) { capture_->stop(); }

    if (render_thread_.joinable()) {
        render_thread_.join();
    }

    display_.quit();

    g_message("Infinity shuts down");
}

void Infinity::queue_key(InfinityKey key) {
    std::lock_guard<std::mutex> lock(key_mutex_);
    key_queue_.push_back(key);
}

void Infinity::handle_key_event(InfinityKey key) {
    switch (key) {
    case INFINITY_KEY_FULLSCREEN: {
        display_.toggle_fullscreen();
        break;
    }
    case INFINITY_KEY_EXIT_FULLSCREEN: {
        display_.exit_fullscreen_if_needed();
        break;
    }
    case INFINITY_KEY_NEXT_PALETTE: {
        if (t_last_color_ > 32) {
            t_last_color_ = 0;
            old_color_ = color_;
            color_ = (color_ + 1) % NB_PALETTES;
        }
        break;
    }
    case INFINITY_KEY_NEXT_EFFECT: {
        display_.load_random_effect(&current_effect_);
        t_last_effect_ = 0;
        break;
    }
    default: {
        break;
    }
    }
}

void Infinity::process_key_queue() {
    std::deque<InfinityKey> keys_to_process;
    {
        std::lock_guard<std::mutex> lock(key_mutex_);
        if (key_queue_.empty()) {
            return;
        }
        keys_to_process.swap(key_queue_);
    }
    for (auto key : keys_to_process) {
        handle_key_event(key);
    }
}

gint64 Infinity::calculate_frame_length_usecs(gint32 fps, int line) {
    gint64 frame_length = static_cast<gint64>(((1.0 / fps) * 1000000));
    g_message("Infinity[%d]: setting maximum rate at ~%d frames/second", line, fps);
    return frame_length;
}

void Infinity::renderer() {
    using Clock = std::chrono::steady_clock;
    using Microseconds = std::chrono::microseconds;

    gint32 fps = params_.max_fps;
    gint64 frame_length = calculate_frame_length_usecs(fps, __LINE__);
    gint32 t_between_effects = params_.effect_interval;
    gint32 t_between_colors = params_.color_interval;

    for (;;) {
        if (!display_.is_visible()) {
            if (finished_) {
                break;
            }
            std::this_thread::sleep_for(Microseconds(3 * frame_length));
            continue;
        }

        process_key_queue();

        if (display_.take_resize(&width_, &height_)) {
            {
                std::lock_guard<std::mutex> lock(resize_mutex_);
                resizing_ = true;
            }
            must_resize_ = true;
        }

        if (finished_) {
            break;
        }

        if (must_resize_) {
            params_.width = width_;
            params_.height = height_;
            must_resize_ = false;
            {
                std::lock_guard<std::mutex> lock(resize_mutex_);
                resizing_ = false;
            }
        }

        auto t_begin = Clock::now();

        display_.blur(current_effect_.num_effect);
        display_.spectral(&current_effect_);
        display_.curve(&current_effect_);

        if (t_last_color_ <= 32) {
            display_.change_color(old_color_, color_, t_last_color_ * 8);
        }
        ++t_last_color_;
        ++t_last_effect_;

        if (t_last_effect_ % t_between_effects == 0) {
            display_.load_random_effect(&current_effect_);
            t_last_effect_ = 0;
            t_between_effects = params_.effect_interval;
        }

        if (t_last_color_ % t_between_colors == 0) {
            old_color_ = color_;
            color_ = std::uniform_int_distribution<int>(0, NB_PALETTES - 1)(rng_);
            t_last_color_ = 0;
            t_between_colors = params_.color_interval;
        }

        gint32 new_fps = params_.max_fps;
        if (new_fps != fps) {
            fps = new_fps;
            frame_length = calculate_frame_length_usecs(fps, __LINE__);
        }

        auto now = Clock::now();
        auto render_time = std::chrono::duration_cast<Microseconds>(now - t_begin).count();

        if (render_time < frame_length) {
            std::this_thread::sleep_for(Microseconds(frame_length - render_time));
        }
    }
}
