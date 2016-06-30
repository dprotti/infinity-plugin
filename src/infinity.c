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

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
/*#include <SDL/SDL_syswm.h>*/

#include "config.h"
#include "display.h"
#include "effects.h"
#include "infinity.h"
#include "types.h"

#if MMX_DETECTION
#include "cputest.h"
#endif

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
static SDL_mutex *resizing_mutex;
static gboolean initializing = FALSE;
static gboolean visible;
static gboolean quiting;
static gboolean mode_interactif;
static gboolean first_xevent;
static gchar *current_title;
static GTimer *title_timer;

static SDL_Thread *thread;

static gint32 event_filter(const SDL_Event *event);
static void check_events();
static int renderer(void *);
static void set_title(void);

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
	resizing_mutex = SDL_CreateMutex();
	mode_interactif = FALSE;
	visible = TRUE;
	quiting = FALSE;
	first_xevent = TRUE;

	current_title = g_strdup("Infinity");
	set_title();
	title_timer = g_timer_new();
	g_timer_start(title_timer);
	display_load_random_effect(&current_effect);

	(void)SDL_EventState((Uint8)SDL_ALLEVENTS, SDL_IGNORE);
	(void)SDL_EventState((Uint8)SDL_VIDEORESIZE, SDL_ENABLE);
	(void)SDL_EventState((Uint8)SDL_ACTIVEEVENT, SDL_ENABLE);
	(void)SDL_EventState((Uint8)SDL_KEYDOWN, SDL_ENABLE);
	(void)SDL_EventState((Uint8)SDL_QUIT, SDL_ENABLE);

	SDL_SetEventFilter(event_filter);

	thread = SDL_CreateThread(renderer, NULL);
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
	SDL_DestroyMutex(resizing_mutex);
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
	g_usleep(10000 * SDL_TIMESLICE);
	display_quit();
	g_timer_destroy(title_timer);

	player->disable_plugin();

	g_message("Infinity: Closing...");
}

void infinity_render_multi_pcm(const float *data, int channels)
{
	if (!initializing && !quiting)
		display_set_pcm_data(data, channels);
}

/*
 * Private functions
 */
static gint32 event_filter(const SDL_Event *event)
{
	if (!event) {
		g_warning("Infinity: SDL_Event is NULL");
		return 0;
	}

	switch (event->type) {
	case SDL_VIDEORESIZE:
		g_return_val_if_fail(SDL_LockMutex(resizing_mutex) >= 0, 0);
		if (resizing) {
			g_return_val_if_fail(SDL_UnlockMutex(resizing_mutex) >= 0, 0);
			/*
			 * VIDEORESIZE event dropped from event queue
			 */
			return 0;
		} else {
			g_return_val_if_fail(SDL_UnlockMutex(resizing_mutex) >= 0, 0);
			return 1;
		}
		g_assert_not_reached();
		break;
	case SDL_ACTIVEEVENT:
		if (event->active.state & SDL_APPACTIVE) {
			if (event->active.gain) {
				visible = TRUE;
				return 0;
			} else {
				visible = FALSE;
				return 0;
			}
		}
		break;
	case SDL_QUIT:
		// ignore it. let handle it in check_events()
		break;
	default:
		break;
	}

	return 1;
}

static void check_events()
{
	SDL_Event event;

	/*XEvent *xevent;
	 * XWindowChanges changes;
	 * XWindowAttributes attr;
	 * XSetWindowAttributes s_attr;*/

	if (params->must_show_title()) {
		if (g_timer_elapsed(title_timer, NULL) > 1.0) {
			if (player->is_playing()) {
				if (current_title)
					g_free(current_title);
				current_title = g_strdup(player->get_title());
			} else {
				if (current_title)
					g_free(current_title);
				current_title = g_strdup("Infinity");
			}
			set_title();
			g_timer_reset(title_timer);
		}
	}

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		/*case SDL_SYSWMEVENT:
		 * g_message ("Infinity: SDL_SYSWMEVENT");
		 * if (event.syswm.msg != NULL) {
		 * if (event.syswm.msg->subsystem == SDL_SYSWM_X11) {
		 * xevent = &(event.syswm.msg->event.xevent);
		 * if (xevent == NULL)
		 * continue;
		 * if (first_xevent) {
		 * changes.x = config_get_x();
		 * changes.y = config_get_y();
		 * XConfigureWindow (xevent->xany.display,
		 * xevent->xany.window,
		 * CWX | CWY, &changes);
		 * first_xevent = FALSE;
		 * g_message ("Infinity: window moved to (%d,%d)",
		 * changes.x, changes.y);
		 * } else {
		 * XGetWindowAttributes (xevent->xany.display,
		 * xevent->xany.window,
		 * &attr);
		 * g_message ("Infinity: GetWindowAttributes (%d,%d)",
		 * attr.x, attr.y);
		 * }
		 * }
		 * }
		 * break;*/
		case SDL_QUIT:
			player->disable_plugin();
			break;
		case SDL_VIDEORESIZE:
			g_return_if_fail(SDL_LockMutex(resizing_mutex) >= 0);
			resizing = TRUE;
			g_return_if_fail(SDL_UnlockMutex(resizing_mutex) >= 0);
			width = event.resize.w;
			height = event.resize.h;
			g_message("Infinity: Screen resized to %dx%d pixels^2", width, height);
			must_resize = TRUE;
			break;
		case SDL_KEYDOWN:
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
				player->adjust_volume(5);
				break;
			case SDLK_DOWN:
				player->adjust_volume(-5);
				break;
			case SDLK_TAB:
				display_toggle_fullscreen();
				break;
			case SDLK_z:
				player->previous();
				break;
			case SDLK_x:
				player->play();
				break;
			case SDLK_c:
				player->pause();
				break;
			case SDLK_v:
				player->stop();
				break;
			case SDLK_b:
				player->next();
				break;
			case SDLK_F11:
				display_save_screen();
				break;
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
				mode_interactif = !mode_interactif;
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
	if (mode_interactif) {
		gint32 i;
		gint32 sx, sy;
		byte *keystate;

		keystate = SDL_GetKeyState(NULL);
		SDL_GetMouseState(&sx, &sy);
		current_effect.spectral_shift = sx;
		if (keystate[SDLK_a])
			current_effect.curve_color = wrap(current_effect.curve_color - 32);
		if (keystate[SDLK_z])
			current_effect.curve_color = wrap(current_effect.curve_color + 32);
		if (keystate[SDLK_q])
			current_effect.spectral_color = wrap(current_effect.spectral_color - 32);
		if (keystate[SDLK_s])
			current_effect.spectral_color = wrap(current_effect.spectral_color + 32);
		for (i = 0; i < 10; i++)
			if (keystate[SDLK_F1 + i])
				current_effect.num_effect = i % NB_FCT;
		if (keystate[SDLK_d])
			current_effect.spectral_amplitude = (current_effect.spectral_amplitude - 1);
		if (keystate[SDLK_f])
			current_effect.spectral_amplitude = (current_effect.spectral_amplitude + 1);
		if (keystate[SDLK_e])
			current_effect.curve_amplitude = (current_effect.curve_amplitude - 1);
		if (keystate[SDLK_r])
			current_effect.curve_amplitude = (current_effect.curve_amplitude + 1);
		if (keystate[SDLK_m])
			display_save_effect(&current_effect);
		if (keystate[SDLK_w])
			current_effect.mode_spectre = (current_effect.mode_spectre + 1) % 5;
	}
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
	gint32 has_mmx = 0;

	fps = params->get_max_fps();
	frame_length = calculate_frame_length_usecs(fps, __LINE__);
	t_between_effects = params->get_effect_interval();
	t_between_colors = params->get_color_interval();
#if MMX_DETECTION
	has_mmx = mm_support_check_and_show();
#endif
	initializing = FALSE;
	for (;; ) { /* ever... */
		if (!visible) {
			check_events();
			if (finished)
				break;
			g_usleep(3000 * frame_length);
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
			g_return_val_if_fail(SDL_LockMutex(resizing_mutex) >= 0, -1);
			resizing = FALSE;
			g_return_val_if_fail(SDL_UnlockMutex(resizing_mutex) >= 0, -1);
		}
		t_begin = g_get_monotonic_time();
		if (has_mmx)
			display_blur_mmx(width * height * current_effect.num_effect);
		else
			display_blur(width * height * current_effect.num_effect);
		spectral(&current_effect);
		curve(&current_effect);
		if (t_last_color <= 32)
			change_color(old_color, color, t_last_color * 8);
		next_color();
		next_effect();
		if (t_last_effect % t_between_effects == 0) {
#ifdef INFINITY_DEBUG
			if (!mode_interactif) {
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
			if (!mode_interactif) {
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

static void set_title(void)
{
	SDL_WM_SetCaption(current_title, "Infinity");
}
