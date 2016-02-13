/**
 * \file display.h
 *
 * \brief This module implements the necessary functions to draw
 * to the screen.
 */

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

#include <SDL/SDL.h>
#include <glib.h>

#include "compute.h"
#include "effects.h"

#define NB_PALETTES 5

/**
 * Initializes the display related structures.
 *
 * This function reads the user configuration options
 * and set his internal data structures accordingly.
 * It also initializes the SDL library.
 *
 * \warning Because this function initializes the SDL
 * library, must be called before any SDL operation and
 * must not be called when SDL was already started.
 */
void display_init(void);

/**
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
 * \see display_init().
 */
void display_quit(void);

/**
 * Change the size of the display to the new dimension
 * \a width x \a height.
 *
 * It is supposed that this must be called when the display
 * screen is resized (i.e. when is resized the window where
 * the screen is embedded in).
 */
void display_resize(gint32 width, gint32 height);

/**
 * Set \a data as the data PCM data of this module.
 *
 * This function makes a copy of \a data.
 *
 * \warning Be aware that this function locks a mutex.
 *
 * \see display_quit().
 */
void display_set_pcm_data(const float *data, int channels);

void display_show(void);

void change_color(gint32 old_p, gint32 p, gint32 w);
void display_blur(guint32 vector_index);
void display_blur_mmx(guint32 vector_index);
void spectral(t_effect *current_effect);
void curve(t_effect *current_effect);

/**
 * Makes the plugin screen switch to full screen mode.
 *
 * \see display_init().
 */
void display_toggle_fullscreen(void);

void display_save_screen(void);
void display_save_effect(t_effect *effect);
void display_load_random_effect(t_effect *effect);

#endif /* __INFINITY_DISPLAY__ */
