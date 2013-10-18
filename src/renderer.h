/**
 * \file renderer.h
 *
 * \brief This module control the rendering process and his interaction
 * with the events that affects the application.
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
#ifndef	__INFINITY_RENDERER__
#define __INFINITY_RENDERER__

#include <audacious/plugin.h>
#include <glib.h>


/**
 * Initializes the rendering process.
 *
 * It reads the plugin's configuration parameters and launchs
 * a thread on which almost all the job of the plugin will be
 * done.
 */
void renderer_init (void);

/**
 * Closes the rendering process.
 */
void renderer_finish (void);

/*
 * Set a reference to a structure where there is the
 * information about the plugin.
 *
 * @param vplugin Must be a non NULL reference to an
 * Audacity VisPlugin structure properly initialized.
 *
 * \see <xmms/plugin.h>
 */
void renderer_set_plugin_info (VisPlugin *vplugin);

/**
 * Copy the actual PCM data from Audacity.
 *
 * It is supposed that this function is called periodically
 * by Audacity copying the PCM data which corresponds to the 
 * current sound played.
 */
void renderer_set_pcm_data (gint16 data[2][512]);


#endif /* __INFINITY_RENDERER__ */
