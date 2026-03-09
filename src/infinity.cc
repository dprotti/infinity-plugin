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
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "infinity.h"
#include "types.h"

void Infinity::queue_key(InfinityKey key) {
    std::lock_guard<std::mutex> lock(key_mutex_);
    key_queue_.push_back(key);
}

Infinity::~Infinity() {
    finish();
}

Infinity::Infinity(InfParameters *_params, Player *_player)
    : display_(_player, [this](InfinityKey key) { queue_key(key); }),
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

    params_ = _params;
    player_ = _player;
    width_ = params_->get_width();
    height_ = params_->get_height();
    scale_ = params_->get_scale();

    if (!display_.init(width_, height_, scale_)) {
        g_critical("Infinity: cannot initialize display");
        initializing_.store(false);
        finished_ = true;
        player_->disable_plugin();
        return;
    }

    old_color_ = 0;
    color_ = 0;

    finished_ = false;
    must_resize_ = false;
    resizing_ = false;
#ifdef INFINITY_DEBUG
    interactive_mode_ = false;
#endif
    quiting_ = false;

    display_.load_random_effect(&current_effect_);

    render_thread_ = std::thread(&Infinity::renderer, this);
    initializing_.store(false);
}

void Infinity::finish() {
    gint32 _try = 0;

    if (finished_) {
        return;
    }
    if (initializing_.load()) {
        g_warning("The plugin have not yet initialized");
        while (initializing_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (_try++ > 10) {
                return;
            }
        }
    }
    quiting_ = true;
    finished_ = true;

    if (render_thread_.joinable()) {
        render_thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(key_mutex_);
        key_queue_.clear();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    display_.quit();

    g_message("Infinity shuts down");
}

void Infinity::render_multi_pcm(const float *data, int channels) {
    if (!initializing_.load() && !quiting_) {
        display_.set_pcm_data(data, channels);
    }
}

void Infinity::handle_key_event(InfinityKey key) {
    switch (key) {
    case INFINITY_KEY_RIGHT: {
        if (player_->is_playing()) {
            player_->seek(5000);
        }
        break;
    }
    case INFINITY_KEY_LEFT: {
        if (player_->is_playing()) {
            player_->seek(-5000);
        }
        break;
    }
    case INFINITY_KEY_UP: {
        player_->adjust_volume(5);
        break;
    }
    case INFINITY_KEY_DOWN: {
        player_->adjust_volume(-5);
        break;
    }
    case INFINITY_KEY_PREV: {
        player_->previous();
        break;
    }
    case INFINITY_KEY_PLAY: {
        player_->play();
        break;
    }
    case INFINITY_KEY_PAUSE: {
        player_->pause();
        break;
    }
    case INFINITY_KEY_STOP: {
        player_->stop();
        break;
    }
    case INFINITY_KEY_NEXT: {
        player_->next();
        break;
    }
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
    case INFINITY_KEY_TOGGLE_INTERACTIVE: {
#ifdef INFINITY_DEBUG
        interactive_mode_ = !interactive_mode_;
        g_message("Infinity %s interactive mode", interactive_mode_ ? "entered" : "leaved");
#endif
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

    gint32 fps = params_->get_max_fps();
    gint64 frame_length = calculate_frame_length_usecs(fps, __LINE__);
    gint32 t_between_effects = params_->get_effect_interval();
    gint32 t_between_colors = params_->get_color_interval();

    for (;;) {
        if (display_.window_closed()) {
            player_->disable_plugin();
            break;
        }
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
            if (!display_.resize(width_, height_)) {
                player_->disable_plugin();
                break;
            }
            params_->set_width(width_);
            params_->set_height(height_);
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
#ifdef INFINITY_DEBUG
            if (!interactive_mode_) {
                display_.load_random_effect(&current_effect_);
                t_last_effect_ = 0;
                t_between_effects = params_->get_effect_interval();
            }
#else
            display_.load_random_effect(&current_effect_);
            t_last_effect_ = 0;
            t_between_effects = params_->get_effect_interval();
#endif
        }

        if (t_last_color_ % t_between_colors == 0) {
#ifdef INFINITY_DEBUG
            if (!interactive_mode_) {
                old_color_ = color_;
                color_ = std::uniform_int_distribution<int>(0, NB_PALETTES - 1)(rng_);
                t_last_color_ = 0;
                t_between_colors = params_->get_color_interval();
            }
#else
            old_color_ = color_;
            color_ = std::uniform_int_distribution<int>(0, NB_PALETTES - 1)(rng_);
            t_last_color_ = 0;
            t_between_colors = params_->get_color_interval();
#endif
        }

        gint32 new_fps = params_->get_max_fps();
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
