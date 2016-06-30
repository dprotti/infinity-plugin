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
#ifndef __INFINITY_PLAYER__
#define __INFINITY_PLAYER__

/*
 * Interface that Infinity uses to talk with the hosting music player.
 */
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

    void    (*notify_critical_error) (const gchar *message);
    void    (*disable_plugin)   (void);  /* tell the player that this plugin has stopped */

} Player;

#endif /* __INFINITY_PLAYER__ */
