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
#include <libaudcore/drct.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

extern "C" {
#include "config.h"
#include "infinity.h"
#include "types.h"
}

#define CFGID "infinity"

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

static gint32 get_width() {
	return aud_get_int(CFGID, "width");
}

static void set_width(gint32 width) {
	aud_set_int(CFGID, "width", width);
}

static gint32 get_height() {
	return aud_get_int(CFGID, "height");
}

static void set_height(gint32 height) {
	aud_set_int(CFGID, "height", height);
}

static gint32 get_scale() {
	return aud_get_int(CFGID, "scale_factor");
}

static gint32 get_effect_interval() {
	return aud_get_int(CFGID, "effect_time");
}

static gint32 get_color_interval() {
	return aud_get_int(CFGID, "palette_time");
}

static gboolean must_show_title() {
	return aud_get_bool(CFGID, "show_title");
}

static gint32 get_max_fps() {
	return aud_get_int(CFGID, "max_fps");
}

static InfParameters params;

static void init_params() {
	params.get_width = get_width;
	params.set_width = set_width;
	params.get_height = get_height;
	params.set_height = set_height;
	params.get_scale = get_scale;
	params.get_effect_interval = get_effect_interval;
	params.get_color_interval = get_color_interval;
	params.must_show_title = must_show_title;
	params.get_max_fps = get_max_fps;
};

static gboolean is_playing() {
	return aud_drct_get_playing() && aud_drct_get_ready();
}

static gchar* get_title() {
	String title = aud_playlist_get_title(aud_playlist_get_playing());
	return (gchar*) title.to_raw();
}

static void play() {
	aud_drct_play();
}

static void pause() {
	aud_drct_pause();
}

static void stop() {
	aud_drct_stop();
}

static void previous() {
	aud_drct_pl_prev();
}

static void next() {
	aud_drct_pl_next();
}

static void seek(gint32 usecs) {
	aud_drct_seek(aud_drct_get_time() + usecs);
}

static void adjust_volume(gint delta) {
	gint volume = aud_drct_get_volume_main();
	g_message("Increasing volume to %d", volume + 5);
	aud_drct_set_volume_main(volume + delta);
}

static void disable_plugin() {
	PluginHandle * plugin = aud_plugin_lookup_basename("libinfinite");
	aud_plugin_enable(plugin, false);
}

static Player player = {
	.is_playing = is_playing,
	.get_title = get_title,
	.play = play,
	.pause = pause,
	.stop = stop,
	.previous = previous,
	.next = next,
	.seek = seek,
	.adjust_volume = adjust_volume,
	.disable_plugin = disable_plugin
};

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
	init_params();
	infinity_init(&params, &player);

	return TRUE;
}

void InfinityPlugin::clear ()
{
	g_message("Infinity: clear()");
}

void InfinityPlugin::cleanup(void)
{
	g_message("Infinity: cleanup()");
	infinity_finish();
}

void InfinityPlugin::render_multi_pcm (const float * pcm, int channels) {
	infinity_render_multi_pcm(pcm, channels);
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
