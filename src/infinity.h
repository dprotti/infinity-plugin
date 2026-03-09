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
#ifndef __INFINITY_INFINITY__
#define __INFINITY_INFINITY__

#include <atomic>
#include <deque>
#include <glib.h>
#include <mutex>
#include <random>
#include <thread>

#include "display.h"
#include "effects.h"
#include "input.h"
#include "music-player.h"

typedef struct _InfParameters {
    gint32 (*get_width)(void);
    void (*set_width)(gint32 width);
    gint32 (*get_height)(void);
    void (*set_height)(gint32 height);
    gint32 (*get_scale)(void);
    gint32 (*get_effect_interval)(void);
    gint32 (*get_color_interval)(void);
    gint32 (*get_max_fps)(void);
} InfParameters;

class Infinity {
public:
    Infinity(InfParameters *params, Player *player);
    ~Infinity();

    void finish();
    void render_multi_pcm(const float *data, int channels);

private:
    void renderer();
    void handle_key_event(InfinityKey key);
    void queue_key(InfinityKey key);
    void process_key_queue();
    gint64 calculate_frame_length_usecs(gint32 fps, int line);

    Display display_;
    std::mt19937 rng_;
    InfParameters *params_{nullptr};
    Player *player_{nullptr};

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

#ifdef INFINITY_DEBUG
    bool interactive_mode_{false};
#endif

    std::thread render_thread_;

    std::deque<InfinityKey> key_queue_;
    std::mutex key_mutex_;
};

#endif /* __INFINITY_INFINITY__ */
