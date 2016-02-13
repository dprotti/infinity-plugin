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
#ifndef __INFINITY_CONFIG__
#define __INFINITY_CONFIG__


#include <glib.h>


/*
 * This structure is intended to be used for every module
 * that want to get your own copy of the screen parameters,
 * it is not used by the config module itself.
 * Is responsibility of the modules that create his own
 * copy of the screen parameters to keep consistent that
 * parameters with each other copy, it is not responsibility
 * of the config module.
 */
typedef struct _t_screen_parameters {
	gint32	x, y;
	gint32	width;
	gint32	height;
	gint32	scale;
	gint32	bpp; /**< bytes per pixels. */
} t_screen_parameters;


/*
 * Read user preferences from ~/.xmms/config file
 */
void config_plugin_load_prefs(void);

void config_plugin_save_prefs(void);

void config_set_default_values(void);

/*
 * \return TRUE if config_plugin_load_prefs() or
 * config_set_default_values() has been called.
 */
gboolean config_is_initialized(void);

/*
 * Open a window to let the user choose and
 * save your options.
 */
void config_plugin_config_window(void);

void config_set_x(gint32 value);
void config_set_y(gint32 value);
void config_set_xres(gint32 value);
void config_set_yres(gint32 value);
void config_set_teff(gint32 value);
void config_set_tcol(gint32 value);
void config_set_sres(gint32 value);
void config_set_fps(gint32 value);
void config_set_show_title(gboolean value);

gint32 config_get_x(void);
gint32 config_get_y(void);
gint32 config_get_xres(void);
gint32 config_get_yres(void);
gint32 config_get_teff(void);
gint32 config_get_tcol(void);
gint32 config_get_sres(void);
gint32 config_get_fps(void);
gboolean config_get_show_title(void);


#endif /* __INFINITY_CONFIG__ */
