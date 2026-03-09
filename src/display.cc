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
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <mutex>
#include <vector>

#include "config.h"
#include "display.h"
#include "music-player.h"
#include "types.h"

Display::Display(Player *_player, std::function<void(InfinityKey)> _queue_key)
    : player(_player) {
    assert(player);
    assert(_queue_key);

    DisplayCallbacks callbacks = {[this](gint32 width, gint32 height) { notify_resize(width, height); },
        [this]() { notify_close(); },
        [this](gboolean is_visible) { notify_visibility(is_visible); },
        _queue_key};
    display_callbacks = callbacks;
}

bool Display::init(gint32 _width, gint32 _height, gint32 _scale) {
    width = _width;
    height = _height;
    scale = _scale;
    pending_resize = false;
    window_closed_ = false;
    visible = true;

    if (!effects_load_effects(player)) {
        return false;
    }
    if (!ui_init_window()) {
        ui_quit_window();
        return false;
    }
    compute_init(width, height, scale);
    generate_colors();
    vector_field = compute_vector_field_new(width, height);
    compute_generate_vector_field(vector_field);
    regenerate_cosine_tables();
    initialized = true;
    return true;
}

void Display::quit() {
    if (!initialized) {
        return;
    }
    std::lock_guard<std::mutex> lock(render_mutex);
    if (vector_field) {
        compute_vector_field_destroy(vector_field);
        vector_field = nullptr;
    }
    compute_quit();
    ui_quit_window();
    initialized = false;
}

bool Display::resize(gint32 _width, gint32 _height) {
    std::lock_guard<std::mutex> lock(render_mutex);
    width = _width;
    height = _height;
    regenerate_cosine_tables();
    const bool ok1 = allocate_render_buffer();
    const bool ok2 = regenerate_vector_field();
    return ok1 && ok2;
}

bool Display::take_resize(gint32 *out_w, gint32 *out_h) {
    std::lock_guard<std::mutex> lock(resize_mutex);
    if (pending_resize) {
        *out_w = pending_width;
        *out_h = pending_height;
        pending_resize = false;
        return true;
    }
    return false;
}

bool Display::window_closed() const {
    return window_closed_;
}
bool Display::is_visible() const {
    return visible;
}

void Display::set_pcm_data(const float *data, int channels) {
    if (channels != 2) {
        g_critical("Unsupported number of channels (%d)\n", channels);
        return;
    }
    std::lock_guard<std::mutex> lock(pcm_mutex);
    for (gint32 i = 0; i < PCM_SIZE; ++i) {
        float l = data[2 * i], r = data[2 * i + 1];
        gint32 sl = static_cast<gint32>(l * 32767.0f);
        gint32 sr = static_cast<gint32>(r * 32767.0f);
        pcm_data[1][i] = static_cast<gint16>(std::clamp(sl, -32768, 32767));
        pcm_data[0][i] = static_cast<gint16>(std::clamp(sr, -32768, 32767));
    }
}

void Display::show() {
    std::lock_guard<std::mutex> lock(render_mutex);
    if (surface1) {
        display_surface();
    }
}

void Display::change_color(gint32 t2, gint32 t1, gint32 w) {
    for (gint32 i = 0; i < 255; ++i) {
        gint32 r = ((color_table[t1][i].r * w + color_table[t2][i].r * (256 - w)) >> 11);
        gint32 g = ((color_table[t1][i].g * w + color_table[t2][i].g * (256 - w)) >> 10);
        gint32 b = ((color_table[t1][i].b * w + color_table[t2][i].b * (256 - w)) >> 11);
        current_colors[i] = static_cast<guint16>((r << 11) + (g << 5) + b);
    }
}

void Display::blur(guint32 effect_index) {
    std::lock_guard<std::mutex> lock(render_mutex);
    const guint32 wh = static_cast<guint32>(vector_field->width) * static_cast<guint32>(vector_field->height);
    effect_index %= NB_FCT;
    surface1 = compute_surface(vector_field->vector + effect_index * wh, width, height);
    display_surface();
}

static constexpr gint32 STEP = 4;

Display::SpectralResult Display::draw_spectral_amplitudes(
    t_effect *effect, gint32 halfheight, gint32 halfwidth, gint32 shift) {

    const gint32 density_lines = 5;
    // Initial values
    gfloat y1 = (gfloat)((((pcm_data[0][0] + pcm_data[1][0]) >> 9) * effect->spectral_amplitude * height) >> 12);
    gfloat y2 = y1;

    if (effect->mode_spectre == 3) {
        if (y1 < 0.0f) {
            y1 = 0.0f;
        }
        if (y2 < 0.0f) {
            y2 = 0.0f;
        }
    }

    for (gint32 i = STEP; i < width; i += STEP) {
        gfloat old_y1 = y1;
        gfloat old_y2 = y2;

        gint32 bin = (i << 9) / width / density_lines;
        bin = std::min(bin, PCM_SIZE - 1);

        y1 = (gfloat)(((pcm_data[1][bin] >> 8) * effect->spectral_amplitude * height) >> 12);
        y2 = (gfloat)(((pcm_data[0][bin] >> 8) * effect->spectral_amplitude * height) >> 12);

        switch (effect->mode_spectre) {
        case 0:
            line(i - STEP,
                halfheight + shift + (gint32)old_y2,
                i,
                halfheight + shift + (gint32)y2,
                effect->spectral_color);
            break;

        case 1:
            line(i - STEP,
                halfheight + shift + (gint32)old_y1,
                i,
                halfheight + shift + (gint32)y1,
                effect->spectral_color);
            line(i - STEP,
                halfheight - shift + (gint32)old_y2,
                i,
                halfheight - shift + (gint32)y2,
                effect->spectral_color);
            break;

        case 2:
            line(i - STEP,
                halfheight + shift + (gint32)old_y1,
                i,
                halfheight + shift + (gint32)y1,
                effect->spectral_color);
            line(i - STEP,
                halfheight - shift + (gint32)old_y1,
                i,
                halfheight - shift + (gint32)y1,
                effect->spectral_color);
            line(halfwidth + shift + (gint32)old_y2,
                i - STEP,
                halfwidth + shift + (gint32)y2,
                i,
                effect->spectral_color);
            line(halfwidth - shift + (gint32)old_y2,
                i - STEP,
                halfwidth - shift + (gint32)y2,
                i,
                effect->spectral_color);
            break;

        case 3:
            if (y1 < 0.0f) {
                y1 = 0.0f;
            }
            if (y2 < 0.0f) {
                y2 = 0.0f;
            }
            [[fallthrough]];

        case 4:
            line(halfwidth + (gint32)(cos_table[i - STEP] * (shift + old_y1)),
                halfheight + (gint32)(sin_table[i - STEP] * (shift + old_y1)),
                halfwidth + (gint32)(cos_table[i] * (shift + y1)),
                halfheight + (gint32)(sin_table[i] * (shift + y1)),
                effect->spectral_color);

            line(halfwidth - (gint32)(cos_table[i - STEP] * (shift + old_y2)),
                halfheight + (gint32)(sin_table[i - STEP] * (shift + old_y2)),
                halfwidth - (gint32)(cos_table[i] * (shift + y2)),
                halfheight + (gint32)(sin_table[i] * (shift + y2)),
                effect->spectral_color);
            break;
        }
    }

    return {(gint32)y1, (gint32)y2};
}

void Display::draw_spectral_closing_line(
    t_effect *effect, gint32 y1, gint32 y2, gint32 halfheight, gint32 halfwidth, gint32 shift) {

    if (effect->mode_spectre != 3 && effect->mode_spectre != 4) {
        return;
    }

    line(halfwidth + (gint32)(cos_table[width - STEP] * (shift + y1)),
        halfheight + (gint32)(sin_table[width - STEP] * (shift + y1)),
        halfwidth - (gint32)(cos_table[width - STEP] * (shift + y2)),
        halfheight + (gint32)(sin_table[width - STEP] * (shift + y2)),
        effect->spectral_color);
}

void Display::spectral(t_effect *current_effect) {
    const gint32 shift = (current_effect->spectral_shift * height) >> 8;
    const gint32 halfheight = height >> 1;
    const gint32 halfwidth = width >> 1;

    SpectralResult res;
    {
        std::lock_guard<std::mutex> lock(pcm_mutex);
        res = draw_spectral_amplitudes(current_effect, halfheight, halfwidth, shift);
    }
    draw_spectral_closing_line(current_effect, res.final_y1, res.final_y2, halfheight, halfwidth, shift);
}

void Display::plot_lissajous_phase(gfloat phase_offset_factor, gint32 &k, byte curve_color_index, gfloat amplitude) {

    constexpr gint32 curve_iterations = 64;
    constexpr gfloat initial_v = 80.0f;
    constexpr gfloat vr = 0.001f;

    // Approximate golden-ratio influenced scaling factors (historical tuning)
    constexpr gfloat phase_scale_a = 1.34f;
    constexpr gfloat phase_scale_b = 0.93f;
    constexpr gfloat freq_factor = 1.756f;

    for (gint32 i = 0; i < curve_iterations; ++i) {
        gfloat x = std::cos(static_cast<gfloat>(k) / (initial_v + initial_v * phase_offset_factor * phase_scale_a))
                   * height * amplitude;
        gfloat y = std::sin(static_cast<gfloat>(k)
                            / (freq_factor * (initial_v + initial_v * phase_offset_factor * phase_scale_b)))
                   * height * amplitude;
        gfloat angle = static_cast<gfloat>(k) * vr;
        plot2(surface1,
            x * std::cos(angle) + y * std::sin(angle) + width / 2.0f,
            x * std::sin(angle) - y * std::cos(angle) + height / 2.0f,
            curve_color_index);

        ++k;
    }
}

void Display::curve(t_effect *effect) {
    std::lock_guard<std::mutex> lock(render_mutex);

    gint32 k = effect->x_curve;
    const gfloat amp = static_cast<gfloat>(effect->curve_amplitude) / 256.0f;
    const auto color_palette_index = static_cast<byte>(effect->curve_color);

    plot_lissajous_phase(0.0f, k, color_palette_index, amp);
    plot_lissajous_phase(1.0f, k, color_palette_index, amp);

    effect->x_curve = k;
}

void Display::toggle_fullscreen() {
    ui_toggle_fullscreen();
}
void Display::exit_fullscreen_if_needed() {
    ui_exit_fullscreen_if_needed();
}
void Display::save_effect(t_effect *effect) {
    effects_append_effect(effect);
}
void Display::load_random_effect(t_effect *effect) {
    effects_load_random_effect(effect);
}

void Display::notify_resize(gint32 w, gint32 h) {
    std::lock_guard<std::mutex> lock(resize_mutex);
    pending_resize = true;
    pending_width = w;
    pending_height = h;
    visible = true;
}

void Display::notify_close() {
    window_closed_ = true;
}
void Display::notify_visibility(gboolean v) {
    visible = v;
}

bool Display::allocate_render_buffer() {
    render_buffer.assign(static_cast<size_t>(width) * height, 0);
    if (render_buffer.size() != static_cast<size_t>(width) * height) {
        error_msg = "Infinity cannot allocate render buffer";
        if (player) {
            player->notify_critical_error(error_msg.c_str());
        }
        return false;
    }
    return true;
}

bool Display::ui_init_window() {
    if (!ui_init(width, height, display_callbacks)) {
        error_msg = "Infinity cannot initialize UI window";
        if (player) {
            player->notify_critical_error(error_msg.c_str());
        }
        return false;
    }
    return allocate_render_buffer();
}

void Display::generate_colors() {
    std::array<std::array<std::array<gfloat, 3>, 2>, NB_PALETTES> colors{{{{{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}},
        {{{2.0f, 1.5f, 0.0f}, {0.0f, 0.5f, 2.0f}}},
        {{{0.0f, 1.0f, 2.0f}, {0.0f, 1.0f, 0.0f}}},
        {{{0.0f, 2.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}},
        {{{2.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}}}}};
    for (gint32 k = 0; k < NB_PALETTES; ++k) {
        for (gint32 i = 0; i < 128; ++i) {
            auto &ct = color_table[k];
            ct[i].r = static_cast<guint8>(colors[k][0][0] * i);
            ct[i].g = static_cast<guint8>(colors[k][0][1] * i);
            ct[i].b = static_cast<guint8>(colors[k][0][2] * i);
            ct[i + 128].r = static_cast<guint8>(colors[k][0][0] * 127 + colors[k][1][0] * i);
            ct[i + 128].g = static_cast<guint8>(colors[k][0][1] * 127 + colors[k][1][1] * i);
            ct[i + 128].b = static_cast<guint8>(colors[k][0][2] * 127 + colors[k][1][2] * i);
        }
    }
}

void Display::display_surface() {
    for (gint32 i = 0; i < height; ++i) {
        guint16 *pdest = render_buffer.data() + static_cast<size_t>(i) * width;
        const byte *psrc = surface1 + static_cast<size_t>(i) * width;
        for (gint32 j = 0; j < width; ++j) {
            *pdest++ = current_colors[static_cast<size_t>(*psrc++)];
        }
    }
    ui_present(render_buffer.data(), width, height);
}

void Display::line(gint32 x1, gint32 y1, gint32 x2, gint32 y2, gint32 c) {
    gint32 dx = std::abs(x1 - x2);
    gint32 dy = std::abs(y1 - y2);
    gint32 cxy = 0;
    if (dy > dx) {
        if (y1 > y2) {
            std::swap(y1, y2), std::swap(x1, x2);
        }
        gint32 dxy = (x1 > x2) ? -1 : 1;
        for (; y1 < y2; ++y1) {
            cxy += dx;
            if (cxy >= dy) {
                x1 += dxy;
                cxy -= dy;
            }
            plot1(surface1, x1, y1, static_cast<byte>(c));
        }
    } else {
        if (x1 > x2) {
            std::swap(x1, x2), std::swap(y1, y2);
        }
        gint32 dxy = (y1 > y2) ? -1 : 1;
        for (; x1 < x2; ++x1) {
            cxy += dy;
            if (cxy >= dx) {
                y1 += dxy;
                cxy -= dx;
            }
            plot1(surface1, x1, y1, static_cast<byte>(c));
        }
    }
}

void Display::regenerate_cosine_tables() {
    size_t sz = static_cast<size_t>(width + height + 1);
    cos_table.resize(sz);
    sin_table.resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        cos_table[i] = std::cos(static_cast<gfloat>(i) / 64.0f);
        sin_table[i] = std::sin(static_cast<gfloat>(i) / 64.0f);
    }
}

bool Display::regenerate_vector_field() {
    if (vector_field) {
        compute_vector_field_destroy(vector_field);
    }
    vector_field = compute_vector_field_new(width, height);
    if (!vector_field) {
        g_critical("Failed to allocate vector_field on resize to %dx%d", width, height);
        return false;
    }
    compute_resize(width, height);
    compute_generate_vector_field(vector_field);
    return true;
}

void Display::ui_quit_window() {
    render_buffer.clear();
    ui_quit();
}
