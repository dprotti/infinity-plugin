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

#include <glib.h>

typedef struct _InfParameters {
    gint32  (*get_x_origin)     (void);
    gint32  (*get_y_origin)     (void);
    gint32  (*get_width)        (void);
    void    (*set_width)        (gint32 width);
    gint32  (*get_height)       (void);
    void    (*set_height)       (gint32 height);
    gint32  (*get_scale)        (void);
    gint32  (*get_bpp)          (void);  /* bytes per pixels. */
    gint32  (*get_effect_interval) (void);
    gint32  (*get_color_interval) (void);
    gint32  (*must_show_title)  (void);
    gint32  (*get_max_fps)      (void);
} InfParameters;

typedef struct _Player {

    gboolean (*is_playing)      (void);
    gchar*  (*get_title)        (void);

    void    (*play)             (void);
    void    (*pause)            (void);
    void    (*stop)             (void);
    void    (*previous)         (void);
    void    (*next)             (void);
    void    (*seek)             (gint32 usecs);
    void    (*adjust_volume)    (gint delta);

    void    (*disable_plugin)   (void);  /* tell the player that this plugin has stopped */

} Player;

/*
 * Initializes rendering process.
 *
 * Reads configuration parameters and launches a thread where most of the
 * plugin job gets done.
 */
void infinity_init(InfParameters * params, Player * player);

/*
 * Closes rendering process.
 */
void infinity_finish(void);

/*
 * Expected to be called periodically by the player to provide actual PCM data.
 */
void infinity_render_multi_pcm(const float *data, int channels);

#endif /* __INFINITY_INFINITY__ */
