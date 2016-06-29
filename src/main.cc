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
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>
#include <glib/gi18n.h>

extern "C" {
#include "config.h"
#include "renderer.h"
#include "types.h"
}

static const char about_text[] =
	"Infinity " PACKAGE_VERSION "\n\n"
	"Julien Carme, Duilio Protti\n\n"
	"https://dprotti.github.io/infinity-plugin";

static const PreferencesWidget prefs_fps[] = {
	WidgetLabel ("<b>Frames per second</b>"),
	WidgetSpin ("Max. :", WidgetInt (CFGID, "max_fps"), {15, 60, 1, "fps"}),
	WidgetLabel ("<b>How often change effect</b>"),
	WidgetSpin ("Every", WidgetInt (CFGID, "effect_time"), {50, 500, 5, "frames   "}),
	WidgetLabel ("<b>How often change colors</b>"),
	WidgetSpin ("Every", WidgetInt (CFGID, "palette_time"), {50, 500, 5, "frames   "})
};

static const PreferencesWidget prefs_widgets[] = {
	WidgetBox ({{prefs_fps}}),
	//WidgetSeparator (),
	//WidgetBox ({{...}})
};

static const PluginPreferences preferences = {{prefs_widgets}};

class InfinityPlugin : VisPlugin {
public:
	static constexpr PluginInfo info = {
		"Infinity",
		PACKAGE,
		about_text,
		& preferences
	};

	constexpr InfinityPlugin () : VisPlugin (info, Visualizer::MultiPCM) {}

	bool init ();
	void cleanup ();

	// No gtk window, SDL creates its own window.
	// void * get_gtk_widget ();

	void clear ();
	void render_multi_pcm (const float * pcm, int channels);

private:
	void load_settings ();
};

EXPORT InfinityPlugin aud_plugin_instance;

bool InfinityPlugin::init(void)
{
	g_message("Infinity commands:\n"
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
			"- F12:\t\tchange palette.");
	load_settings();
	renderer_init();
	return TRUE;
}

void InfinityPlugin::clear ()
{
	g_message("Infinity: clear()");
}

void InfinityPlugin::cleanup(void)
{
	g_message("Infinity: cleanup()");
	renderer_finish();
}

void InfinityPlugin::render_multi_pcm (const float * pcm, int channels) {
	renderer_render_multi_pcm(pcm, channels);
}

static const char * const defaults[] = {
	"width", "512",
	"height", "288",
	"effect_time", "100",
	"palette_time", "100",
	"scale_factor", "1",
	"max_fps", "30",
	"show_title", "true"
};

void InfinityPlugin::load_settings(void)
{
	aud_config_set_defaults (CFGID, defaults);
}
