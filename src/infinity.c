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
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

#include "config.h"
#include "display.h"
#include "effects.h"
#include "infinity.h"
#include "input.h"
#include "types.h"

#define wrap(a)         (a < 0 ? 0 : (a > 255 ? 255 : a))
#define next_effect()   (t_last_effect++)
#define next_color()    (t_last_color++)

typedef gint32 t_color;
typedef gint32 t_num_effect;

static InfParameters * params;
static Player * player;

static gint32 width, height, scale;
static t_effect current_effect;
static t_color color, old_color, t_last_color;
static t_num_effect t_last_effect;

static gboolean must_resize;
static gboolean finished;
static gboolean resizing;
G_LOCK_DEFINE_STATIC(resizing);
static gboolean initializing = FALSE;
static gboolean quiting;
#ifdef INFINITY_DEBUG
static gboolean interactive_mode;
#endif

static GThread *thread;
static GAsyncQueue *key_queue;

static gpointer renderer(void *arg);
static void handle_key_event(InfinityKey key);
static void process_key_queue(void);

void infinity_init(InfParameters * _params, Player * _player)
{
	gint32 _try;

	if (initializing) {
		g_warning("Infinity: is already initializing...");
		_try = 0;
		while (initializing) {
			g_usleep(1000000);
			(void)sleep(1);
			if (_try++ > 10)
				return;
		}
	}
	initializing = TRUE;
	if (key_queue == NULL) {
		key_queue = g_async_queue_new();
	}
	params = _params;
	player = _player;
	width = params->get_width();
	height = params->get_height();
	scale = params->get_scale();

	if (! display_init(width, height, scale, player)) {
		g_critical("Infinity: cannot initialize display");
		initializing = FALSE;
		finished = TRUE;
		player->disable_plugin();
		return;
	}

	old_color = 0;
	color = 0;

	finished = FALSE;
	must_resize = FALSE;
	resizing = FALSE;
#ifdef INFINITY_DEBUG
	interactive_mode = FALSE;
#endif
	quiting = FALSE;

	display_load_random_effect(&current_effect);

	thread = g_thread_new("infinity_renderer", renderer, NULL);
}

void infinity_finish(void)
{
	gint32 _try;

	if (finished)
		return;
	if (initializing) {
		g_warning("The plugin have not yet initialized");
		_try = 0;
		while (initializing) {
			g_usleep(1000000);
			if (_try++ > 10)
				return;
		}
	}
	quiting = TRUE;
	finished = TRUE;
	if (thread != NULL) {
		g_thread_join(thread);
		thread = NULL;
	}
	if (key_queue != NULL) {
		g_async_queue_unref(key_queue);
		key_queue = NULL;
	}
	/*
	 * Take some time to let it know infinity_render_multi_pcm()
	 * that must not call display_set_pcm_data().
	 * If it do that while calling display_quit(),
	 * we could make Audacious crash, because display_quit
	 * destroy a mutex where display_set_pcm_data
	 * could be blocked.
	 *
	 * See display.h::display_set_pcm_data()
	 */
	g_usleep(1000000);
	display_quit();

	g_message("Infinity is shut down");
}

void infinity_render_multi_pcm(const float *data, int channels)
{
	if (!initializing && !quiting)
		display_set_pcm_data(data, channels);
}

void infinity_queue_key(InfinityKey key)
{
	if (key_queue == NULL) {
		return;
	}
	g_async_queue_push(key_queue, GINT_TO_POINTER(key));
}

static void handle_key_event(InfinityKey key)
{
	switch (key) {
	case INFINITY_KEY_RIGHT:
		if (player->is_playing())
			player->seek(5000);
		break;
	case INFINITY_KEY_LEFT:
		if (player->is_playing())
			player->seek(-5000);
		break;
	case INFINITY_KEY_UP:
		player->adjust_volume(5);
		break;
	case INFINITY_KEY_DOWN:
		player->adjust_volume(-5);
		break;
	case INFINITY_KEY_PREV:
		player->previous();
		break;
	case INFINITY_KEY_PLAY:
		player->play();
		break;
	case INFINITY_KEY_PAUSE:
		player->pause();
		break;
	case INFINITY_KEY_STOP:
		player->stop();
		break;
	case INFINITY_KEY_NEXT:
		player->next();
		break;
	case INFINITY_KEY_FULLSCREEN:
		display_toggle_fullscreen();
		break;
	case INFINITY_KEY_EXIT_FULLSCREEN:
		display_exit_fullscreen_if_needed();
		break;
	case INFINITY_KEY_NEXT_PALETTE:
		if (t_last_color > 32) {
			t_last_color = 0;
			old_color = color;
			color = (color + 1) % NB_PALETTES;
		}
		break;
	case INFINITY_KEY_NEXT_EFFECT:
		display_load_random_effect(&current_effect);
		t_last_effect = 0;
		break;
	case INFINITY_KEY_TOGGLE_INTERACTIVE:
#ifdef INFINITY_DEBUG
		interactive_mode = !interactive_mode;
		g_message("Infinity %s interactive mode", interactive_mode ? "entered" : "leaved");
#endif
		break;
	default:
		break;
	}
}

static void process_key_queue(void)
{
	if (key_queue == NULL) {
		return;
	}
	for (;;) {
		gpointer key_ptr = g_async_queue_try_pop(key_queue);
		if (key_ptr == NULL) {
			break;
		}
		handle_key_event((InfinityKey)GPOINTER_TO_INT(key_ptr));
	}
}

// log calling line to improve bug reports
static gint64 calculate_frame_length_usecs(gint32 fps, int line) {
	gint64 frame_length = (gint64)(((1.0 / fps) * 1000000));
	g_message("Infinity[%d]: setting maximum rate at ~%d frames/second", line, fps);
	return frame_length;
}

static gpointer renderer(void *arg)
{
	gint64 now, render_time, t_begin;
	gint32 frame_length;
	gint32 fps, new_fps;
	gint32 t_between_effects, t_between_colors;

	fps = params->get_max_fps();
	frame_length = calculate_frame_length_usecs(fps, __LINE__);
	t_between_effects = params->get_effect_interval();
	t_between_colors = params->get_color_interval();
	initializing = FALSE;
	for (;; ) { /* ever... */
		if (display_window_closed()) {
			player->disable_plugin();
			break;
		}
		if (!display_is_visible()) {
			if (finished)
				break;
			g_usleep(3 * frame_length);
			continue;
		}
		process_key_queue();
		if (display_take_resize(&width, &height)) {
			G_LOCK(resizing);
			resizing = TRUE;
			G_UNLOCK(resizing);
			must_resize = TRUE;
		}
		if (finished)
			break;
		if (must_resize) {
			if (! display_resize(width, height)) {
				player->disable_plugin();
				break;
			}
			params->set_width(width);
			params->set_height(height);
			must_resize = FALSE;
			G_LOCK(resizing);
			resizing = FALSE;
			G_UNLOCK(resizing);
		}
		t_begin = g_get_monotonic_time();
		display_blur(width * height * current_effect.num_effect);
		spectral(&current_effect);
		curve(&current_effect);
		if (t_last_color <= 32)
			change_color(old_color, color, t_last_color * 8);
		next_color();
		next_effect();
		if (t_last_effect % t_between_effects == 0) {
#ifdef INFINITY_DEBUG
			if (!interactive_mode) {
				display_load_random_effect(&current_effect);
				t_last_effect = 0;
				t_between_effects = params->get_effect_interval();
			}
#else
			display_load_random_effect(&current_effect);
			t_last_effect = 0;
			t_between_effects = params->get_effect_interval();
#endif
		}
		if (t_last_color % t_between_colors == 0) {
#ifdef INFINITY_DEBUG
			if (!interactive_mode) {
				old_color = color;
				color = rand() % NB_PALETTES;
				t_last_color = 0;
				t_between_colors = params->get_color_interval();
			}
#else
			old_color = color;
			color = rand() % NB_PALETTES;
			t_last_color = 0;
			t_between_colors = params->get_color_interval();
#endif
		}

		new_fps = params->get_max_fps();
		if (new_fps != fps) {
			fps = new_fps;
			frame_length = calculate_frame_length_usecs(fps, __LINE__);
		}

		now = g_get_monotonic_time();
		render_time = now - t_begin;
		if (render_time < frame_length) {
			g_usleep(frame_length - render_time);
		}
	}

	return NULL;
}
