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

#include <SDL.h>
#include <SDL_thread.h>

#include "config.h"
#include "display.h"
#include "effects.h"
#include "infinity.h"
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
static gboolean visible;
static gboolean quiting;
static gboolean interactive_mode;
static gboolean windowevent_close_received;

static SDL_Thread *thread;

static void check_events();
static int renderer(void *);

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
	interactive_mode = FALSE;
	windowevent_close_received = FALSE;
	visible = TRUE;
	quiting = FALSE;

	display_load_random_effect(&current_effect);

	thread = SDL_CreateThread(renderer, "infinity_renderer", NULL);
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
	SDL_WaitThread(thread, NULL);
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

#ifdef INFINITY_DEBUG
static void handle_interactive_mode() {
	gint32 i;
	gint32 sx, sy;
	const byte *keystate = SDL_GetKeyboardState(NULL);

	SDL_GetMouseState(&sx, &sy);
	current_effect.spectral_shift = sx;
	if (keystate[SDL_SCANCODE_A])
		current_effect.curve_color = wrap(current_effect.curve_color - 32);
	if (keystate[SDL_SCANCODE_Z])
		current_effect.curve_color = wrap(current_effect.curve_color + 32);
	if (keystate[SDL_SCANCODE_Q])
		current_effect.spectral_color = wrap(current_effect.spectral_color - 32);
	if (keystate[SDL_SCANCODE_S])
		current_effect.spectral_color = wrap(current_effect.spectral_color + 32);
	for (i = 0; i < 10; i++)
		if (keystate[SDL_SCANCODE_F1 + i])
			current_effect.num_effect = i % NB_FCT;
	if (keystate[SDL_SCANCODE_D])
		current_effect.spectral_amplitude = (current_effect.spectral_amplitude - 1);
	if (keystate[SDL_SCANCODE_F])
		current_effect.spectral_amplitude = (current_effect.spectral_amplitude + 1);
	if (keystate[SDL_SCANCODE_E])
		current_effect.curve_amplitude = (current_effect.curve_amplitude - 1);
	if (keystate[SDL_SCANCODE_R])
		current_effect.curve_amplitude = (current_effect.curve_amplitude + 1);
	if (keystate[SDL_SCANCODE_M])
		display_save_effect(&current_effect);
	if (keystate[SDL_SCANCODE_W])
		current_effect.mode_spectre = (current_effect.mode_spectre + 1) % 5;
}
#endif /* INFINITY_DEBUG */

static void handle_window_event(SDL_Event *event) {
	switch (event->window.event) {
		case SDL_WINDOWEVENT_SHOWN:
			visible = TRUE;
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			visible = TRUE;
			break;
		case SDL_WINDOWEVENT_HIDDEN:
			visible = FALSE;
			break;
		/*case SDL_WINDOWEVENT_MOVED:
			SDL_Log("Window %d moved to %d,%d",
					event->window.windowID, event->window.data1,
					event->window.data2);
			break;*/
		case SDL_WINDOWEVENT_RESIZED:
			G_LOCK(resizing);
			resizing = TRUE;
			G_UNLOCK(resizing);
			width = event->window.data1;
			height = event->window.data2;
			g_message("Infinity: screen resize to %dx%d pixels", width, height);
			must_resize = TRUE;
			break;
		/*case SDL_WINDOWEVENT_SIZE_CHANGED:
			SDL_Log("Window %d size changed to %dx%d",
					event->window.windowID, event->window.data1,
					event->window.data2);
			break;*/
		case SDL_WINDOWEVENT_MINIMIZED:
			visible = FALSE;
			break;
		/*case SDL_WINDOWEVENT_MAXIMIZED:
			SDL_Log("Window %d maximized", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_RESTORED:
			SDL_Log("Window %d restored", event->window.windowID);
			break;*/
		case SDL_WINDOWEVENT_CLOSE:
			windowevent_close_received = TRUE;
			player->disable_plugin();
			break;
		default:
			break;
		}
}

static void check_events()
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			if (! windowevent_close_received)
				player->disable_plugin();
			break;
		case SDL_WINDOWEVENT:
			handle_window_event(&event); break;
		case SDL_KEYDOWN:
			// TODO check how this may work in a Mac keyboard
			switch (event.key.keysym.sym) {
			case SDLK_RIGHT:
				if (player->is_playing())
					player->seek(5000);
				break;
			case SDLK_LEFT:
				if (player->is_playing())
					player->seek(-5000);
				break;
			case SDLK_UP:
				player->adjust_volume(5); break;
			case SDLK_DOWN:
				player->adjust_volume(-5); break;
			case SDLK_z:
				player->previous(); break;
			case SDLK_x:
				player->play(); break;
			case SDLK_c:
				player->pause(); break;
			case SDLK_v:
				player->stop(); break;
			case SDLK_b:
				player->next(); break;
			case SDLK_F11:
				display_toggle_fullscreen(); break;
			case SDLK_ESCAPE:
				display_exit_fullscreen_if_needed(); break;
			case SDLK_F12:
				if (t_last_color > 32) {
					t_last_color = 0;
					old_color = color;
					color = (color + 1) % NB_PALETTES;
				}
				break;
			case SDLK_SPACE:
				display_load_random_effect(&current_effect);
				t_last_effect = 0;
				break;
#ifdef INFINITY_DEBUG
			case SDLK_RETURN:
				interactive_mode = !interactive_mode;
				g_message("Infinity %s interactive mode", interactive_mode ? "entered" : "leaved");
				break;
#endif
			default:
				break;
			}
			break; /* SDLK_KEYDOWN */
		default:
			break;
		}
	}
#ifdef INFINITY_DEBUG
	if (interactive_mode)
		handle_interactive_mode();
#endif  /* INFINITY_DEBUG */
}

// log calling line to improve bug reports
static gint64 calculate_frame_length_usecs(gint32 fps, int line) {
	gint64 frame_length = (gint64)(((1.0 / fps) * 1000000));
	g_message("Infinity[%d]: setting maximum rate at ~%d frames/second", line, fps);
	return frame_length;
}

static int renderer(void *arg)
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
		if (!visible) {
			check_events();
			if (finished)
				break;
			g_usleep(3 * frame_length);
			continue;
		}
		check_events();
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

	return 0;
}
