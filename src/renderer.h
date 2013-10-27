/**
 * \file renderer.h
 *
 * \brief Controls the rendering process and its interaction with application
 * events.
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
#ifndef __INFINITY_RENDERER__
#define __INFINITY_RENDERER__

#include <glib.h>

/**
 * Initializes rendering process.
 *
 * Reads configuration parameters and launchs a thread where most of the
 * plugin job gets done.
 */
void renderer_init(void);

/**
 * Closes rendering process.
 */
void renderer_finish(void);

/**
 * Copies PCM data from Audacity.
 *
 * Called periodically by Audacity with actual PCM data.
 */
void renderer_render_multi_pcm(const float *data, int channels);

#endif /* __INFINITY_RENDERER__ */
