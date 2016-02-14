/**
 * \file main.c
 *
 * \brief The main module of Infinity plugin.
 *
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
#include <string.h>
#include <audacious/plugin.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "config.h"
#include "gettext.h"
#include "infconfig.h"
#include "renderer.h"

static const char infinity_about[] =
	N_("Infinity Visualization Plugin for Audacious\n\n"
		"Version " PACKAGE_VERSION "\n\n"
		"https://github.com/dprotti/infinity-plugin");

static bool_t plugin_init(void);
static void   plugin_close(void);
static void   clear(void);

/*
 * Registers plugin with Audacious.
 */
AUD_VIS_PLUGIN(
	.name = "Infinity",
	.init = plugin_init,
	.cleanup = plugin_close,
	.take_message = NULL,
	.about			= NULL,
	.about_text		= infinity_about,
	.configure = config_plugin_config_window,
                        //.playback_stop		= NULL,

                        /* reset internal state and clear display */
	.clear = clear,

        /* 512 frames of a single-channel PCM signal */
	.render_mono_pcm = NULL,

        /* 512 frames of an interleaved multi-channel PCM signal */
	.render_multi_pcm = renderer_render_multi_pcm,

        /* intensity of frequencies 1/512, 2/512, ..., 256/512 of sample rate */
	.render_freq = NULL,

        /* GtkWidget * (* get_widget) (void); */
	.get_widget = NULL
	);

static void clear(void)
{
	g_message("TODO implement clear()");
}

static bool_t plugin_init(void)
{
#if ENABLE_NLS
	(void)setlocale(LC_MESSAGES, "");
	(void)bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	(void)textdomain(GETTEXT_PACKAGE);
#endif

	g_message(_("Infinity commands:\n"
		    "- Space:\tchange effect.\n"
		    "- Tab:\t\ttoggle full-screen.\n"
		    "- Up/Down:\tup/down main volume.\n"
		    "- Left/Right:\treward/forward actual played song, if any.\n"
		    "- z:\t\tprevious song.\n"
		    "- x:\t\tplay.\n"
		    "- c:\t\tpause.\n"
		    "- v:\t\tstop.\n"
		    "- b:\t\tnext song.\n"
		    "- Enter:\tswitch to interactive mode.\n\t\t(works only if infinity was configured with --enable-debug option)\n"
		    "- F11:\t\tscreenshot.\n"
		    "- F12:\t\tchange palette."));
	config_plugin_load_prefs();
	renderer_init();
	return TRUE;
}

static void plugin_close(void)
{
	config_plugin_save_prefs();
	renderer_finish();
}
