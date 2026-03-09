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
#ifndef __INFINITY_DISPLAY__
#define __INFINITY_DISPLAY__

#include <functional>
#include <glib.h>
#include <string>

#include "compute.h"
#include "effects.h"
#include "ui.h"

#define NB_PALETTES 5  // TODO avoid macro

class Display {
public:
    Display(Player *_player, std::function<void(InfinityKey)> _queue_key);

    bool init(gint32 _width, gint32 _height, gint32 _scale);

    void quit();

    bool resize(gint32 _width, gint32 _height);

    bool take_resize(gint32 *out_w, gint32 *out_h);

    [[nodiscard]] bool window_closed() const;
    [[nodiscard]] bool is_visible() const;

    void set_pcm_data(const float *data, int channels);

    void show();

    void change_color(gint32 t2, gint32 t1, gint32 w);

    void blur(guint32 effect_index);

    struct SpectralResult {
        gint32 final_y1;
        gint32 final_y2;
    };

    SpectralResult draw_spectral_amplitudes(t_effect *effect, gint32 halfheight, gint32 halfwidth, gint32 shift);

    void draw_spectral_closing_line(
        t_effect *effect, gint32 y1, gint32 y2, gint32 halfheight, gint32 halfwidth, gint32 shift);

    void spectral(t_effect *current_effect);

    void plot_lissajous_phase(gfloat phase_offset_factor, gint32 &k, byte curve_color_index, gfloat amplitude);

    void curve(t_effect *effect);

    void toggle_fullscreen();
    void exit_fullscreen_if_needed();
    void save_effect(t_effect *effect);
    void load_random_effect(t_effect *effect);

    void notify_resize(gint32 w, gint32 h);
    void notify_close();
    void notify_visibility(gboolean v);

private:
    struct color_entry_t {
        guint8 r;
        guint8 g;
        guint8 b;
    };

    bool allocate_render_buffer();

    bool ui_init_window();

    void generate_colors();

    void display_surface();

    static void assign_max(byte *ptr, byte value) {
        if (*ptr <= value) {
            *ptr = value;
        }
    }
    inline void plot1(byte *surf, gint32 x, gint32 y, byte c) const {
        if (x > 0 && x < width - 3 && y > 0 && y < height - 3) {
            assign_max(&surf[static_cast<size_t>(x) + static_cast<size_t>(y) * width], c);
        }
    }

    inline void plot2(byte *surf, gint32 x, gint32 y, byte c) const {
        if (x > 0 && x < width - 3 && y > 0 && y < height - 3) {
            gint32 ty = y * width;
            assign_max(&surf[static_cast<size_t>(x) + ty], c);
            assign_max(&surf[static_cast<size_t>(x) + 1 + ty], c);
            assign_max(&surf[static_cast<size_t>(x) + ty + width], c);
            assign_max(&surf[static_cast<size_t>(x) + 1 + ty + width], c);
        }
    }

    void line(gint32 x1, gint32 y1, gint32 x2, gint32 y2, gint32 c);

    void regenerate_cosine_tables();
    bool regenerate_vector_field();

    void ui_quit_window();

    static constexpr gint32 PCM_SIZE = 512;

    gint32 width = 0;
    gint32 height = 0;
    gint32 scale = 0;
    Player *player = nullptr;
    DisplayCallbacks display_callbacks;

    std::mutex render_mutex;
    std::mutex resize_mutex;
    std::mutex pcm_mutex;

    std::vector<guint16> render_buffer;

    std::array<std::array<color_entry_t, 256>, NB_PALETTES> color_table;
    std::array<guint16, 256> current_colors{};

    std::vector<gfloat> cos_table;
    std::vector<gfloat> sin_table;

    vector_field_t *vector_field = nullptr;
    byte *surface1 = nullptr;

    std::array<std::array<gint16, PCM_SIZE>, 2> pcm_data{};

    gboolean initialized = false;
    gboolean pending_resize = false;
    gint32 pending_width = 0;
    gint32 pending_height = 0;
    gboolean window_closed_ = false;
    gboolean visible = true;

    std::string error_msg;
};

#endif /* __INFINITY_DISPLAY__ */
