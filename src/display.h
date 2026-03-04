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

#include <glib.h>

#include "compute.h"
#include "effects.h"
#include "music-player.h"

#define NB_PALETTES 5

/*
 * Initializes the display related structures and UI window.
 *
 * Returns true on success; and false otherwise.
 */
gboolean display_init(gint32 _width, gint32 _height, gint32 _scale, Player *player);

/*
 * Closes the display module.
 */
void display_quit(void);

/*
 * Change the size of the display to the new dimension
 * width x height.
 *
 * Returns true on success; and false otherwise.
 */
gboolean display_resize(gint32 width, gint32 height);

gboolean display_take_resize(gint32 *out_width, gint32 *out_height);
gboolean display_window_closed(void);
gboolean display_is_visible(void);

/*
 * Set data as the data PCM data of this module.
 *
 * This function makes a copy of data.
 *
 * Warning: be aware that this function locks a mutex.
 *
 * See display_quit().
 */
void display_set_pcm_data(const float *data, int channels);

void display_show(void);

void change_color(gint32 old_p, gint32 p, gint32 w);
void display_blur(guint32 effect_index);
void spectral(t_effect *current_effect);
void curve(t_effect *current_effect);

/*
 * Makes the plugin screen switch to full-screen mode.
 *
 * See display_init().
 */
void display_toggle_fullscreen(void);

void display_exit_fullscreen_if_needed(void);

void display_save_effect(t_effect *effect);
void display_load_random_effect(t_effect *effect);

void display_notify_resize(gint32 width, gint32 height);
void display_notify_close(void);
void display_notify_visibility(gboolean is_visible);

#endif /* __INFINITY_DISPLAY__ */
