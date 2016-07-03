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
 * Initializes the display related structures, including the SDL library.
 *
 * Warning: must be called before any SDL operation and must not be
 * called when SDL was already started.
 *
 * Returns true on success; and false otherwise.
 */
gboolean display_init(gint32 _width, gint32 _height, gint32 _scale, Player *player);

/*
 * Closes the display module.
 *
 * It also closes the SDL library.
 *
 * \warning If you call this from a multithreaded application,
 * any thread must be calling display_set_pcm_data() while
 * inside this function, because display_set_pcm_data()
 * could be bloqued on a mutex, mutex that display_quit()
 * will destroy!
 *
 * See display_init().
 */
void display_quit(void);

/*
 * Change the size of the display to the new dimension
 * width x height.
 *
 * Returns true on success; and false otherwise.
 */
gboolean display_resize(gint32 width, gint32 height);

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
void display_blur(guint32 vector_index);
void display_blur_mmx(guint32 vector_index);
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

#endif /* __INFINITY_DISPLAY__ */
