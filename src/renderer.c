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
#include <gtk/gtk.h>
#include <dbus/dbus.h>

#include <audacious/audctrl.h>
#include <audacious/playlist.h>
#include <audacious/drct.h>

#include <audacious/dbus.h>


#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
/*#include <SDL/SDL_syswm.h>*/

#include "config.h"
#include "gettext.h"
#include "renderer.h"
#include "infconfig.h"
#include "effects.h"
#include "display.h"

#if MMX_DETECTION
#include "cputest.h"
#endif

#define wrap(a)         (a < 0 ? 0 : (a > 255 ? 255 : a))
#define next_effect()   (t_last_effect++)
#define next_color()    (t_last_color++)

typedef struct t_general_parameters {
	gint32	t_between_effects;
	gint32	t_between_colors;
} t_general_parameters;

typedef gint32 t_color;
typedef gint32 t_num_effect;

static t_screen_parameters scr_par;

static t_effect current_effect;
static t_general_parameters gen_par;
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

static DBusGConnection *connection = NULL;
static DBusGProxy *dbus_proxy = NULL;

static gint32 event_filter(const SDL_Event *event);
static void check_events();
static int renderer(void *);
#if MMX_DETECTION
static int renderer_mmx(void *);
#endif
static void set_title(void);

/*
 * Public functions
 */
void renderer_init(void)
{
	GError *error = NULL;
	gint32 try;

	if (initializing) {
		g_warning(_("We are already initializing"));
		try = 0;
		while (initializing) {
			g_usleep(1000000);
			(void)sleep(1);
			if (try++ > 10)
				return;
		}
	}
	initializing = TRUE;
	scr_par.width = config_get_xres();
	scr_par.height = config_get_yres();
	scr_par.scale = config_get_sres();

	gen_par.t_between_effects = config_get_teff();
	gen_par.t_between_colors = config_get_tcol();

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

	display_init();
	current_title = g_strdup("Infinity");
	set_title();
	title_timer = g_timer_new();
	g_timer_start(title_timer);
	display_load_random_effect(&current_effect);

	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	dbus_proxy = dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
					       AUDACIOUS_DBUS_PATH,
					       AUDACIOUS_DBUS_INTERFACE);

	(void)SDL_EventState((Uint8)SDL_ALLEVENTS, SDL_IGNORE);
	(void)SDL_EventState((Uint8)SDL_VIDEORESIZE, SDL_ENABLE);
	(void)SDL_EventState((Uint8)SDL_ACTIVEEVENT, SDL_ENABLE);
	(void)SDL_EventState((Uint8)SDL_KEYDOWN, SDL_ENABLE);
	(void)SDL_EventState((Uint8)SDL_QUIT, SDL_ENABLE);

	SDL_SetEventFilter(event_filter);

#if MMX_DETECTION
	if (mm_support_check_and_show() != 0)
		thread = SDL_CreateThread(renderer_mmx, NULL);
	else
#endif
	thread = SDL_CreateThread(renderer, NULL);
}

void renderer_finish(void)
{
	gint32 try;

	if (initializing) {
		g_warning(_("The plugin have not yet initialized"));
		try = 0;
		while (initializing) {
			g_usleep(1000000);
			if (try++ > 10)
				return;
		}
	}
	quiting = TRUE;
	finished = TRUE;
	SDL_WaitThread(thread, NULL);
	SDL_DestroyMutex(resizing_mutex);
	/*
	 * Take some time to let it know renderer_render_multi_pcm()
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
	g_object_unref(dbus_proxy);
	g_message("Infinity: Closing...");
}

void renderer_render_multi_pcm(const float *data, int channels)
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
		config_plugin_save_prefs();
		break;
	default:
		break;
	}

	return 1;
}

static gint disable_func(gpointer data)
{
	renderer_finish();
	return FALSE;
}

static void check_events()
{
	SDL_Event event;
	gint volume;

	/*XEvent *xevent;
	 * XWindowChanges changes;
	 * XWindowAttributes attr;
	 * XSetWindowAttributes s_attr;*/

	if (config_get_show_title()) {
		if (g_timer_elapsed(title_timer, NULL) > 1.0) {
			if (aud_drct_get_playing() && aud_drct_get_ready()) {
				if (current_title)
					g_free(current_title);
				current_title = g_strdup(aud_playlist_entry_get_title(aud_playlist_get_playing(), aud_playlist_get_position(aud_playlist_get_playing()), FALSE));
				set_title();
			} else {
				if (current_title)
					g_free(current_title);
				current_title = g_strdup("Infinity");
				set_title();
			}
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
			GDK_THREADS_ENTER();
			(void)gtk_idle_add(disable_func, NULL);
			GDK_THREADS_LEAVE();
			break;
		case SDL_VIDEORESIZE:
			g_return_if_fail(SDL_LockMutex(resizing_mutex) >= 0);
			resizing = TRUE;
			g_return_if_fail(SDL_UnlockMutex(resizing_mutex) >= 0);
			scr_par.width = event.resize.w;
			scr_par.height = event.resize.h;
			g_message("Infinity: Screen resized to %dx%d pixels^2",
				  scr_par.width, scr_par.height);
			must_resize = TRUE;
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_RIGHT:
				if (aud_drct_get_playing() && aud_drct_get_ready())
					aud_drct_seek(aud_drct_get_time() + 5000);
				break;
			case SDLK_LEFT:
				if (aud_drct_get_playing() && aud_drct_get_ready())
					aud_drct_seek(aud_drct_get_time() - 5000);
				break;
			case SDLK_UP:
				aud_drct_get_volume_main(&volume);
				g_message(_("Increasing volume to %d"), volume + 5);
				aud_drct_set_volume_main(volume + 5);
				break;
			case SDLK_DOWN:
				aud_drct_get_volume_main(&volume);
				g_message(_("Decreasing volume to %d"), volume - 5);
				aud_drct_set_volume_main(volume - 5);
				break;
			case SDLK_TAB:
				display_toggle_fullscreen();
				break;
			case SDLK_z:
				aud_drct_pl_prev();
				break;
			case SDLK_x:
				aud_drct_play();
				break;
			case SDLK_c:
				aud_drct_pause();
				break;
			case SDLK_v:
				aud_drct_stop();
				break;
			case SDLK_b:
				aud_drct_pl_next();
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

static int renderer(void *arg)
{
	gint32 render_time, now;
	gint32 frame_length;
	gint32 idle_time;
	gint32 fps, new_fps;

	/* We suppose here that config module have been initialized */
	fps = config_get_fps();
	frame_length = (gint32)((1.0 / config_get_fps()) * 1000);
	g_message("Infinity[%d]: setting maximum rate at ~%d frames/second", __LINE__, fps);
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
			display_resize(scr_par.width, scr_par.height);
			config_set_xres(scr_par.width);
			config_set_yres(scr_par.height);
			must_resize = FALSE;
			g_return_val_if_fail(SDL_LockMutex(resizing_mutex) >= 0, -1);
			resizing = FALSE;
			g_return_val_if_fail(SDL_UnlockMutex(resizing_mutex) >= 0, -1);
		}
		render_time = (gint32)SDL_GetTicks();
		display_blur(scr_par.width * scr_par.height * current_effect.num_effect);
		spectral(&current_effect);
		curve(&current_effect);
		if (t_last_color <= 32)
			change_color(old_color, color, t_last_color * 8);
		next_color();
		next_effect();
		if (t_last_effect % gen_par.t_between_effects == 0) {
#ifdef INFINITY_DEBUG
			if (!mode_interactif) {
				display_load_random_effect(&current_effect);
				t_last_effect = 0;
			}
#else
			display_load_random_effect(&current_effect);
			t_last_effect = 0;
#endif
		}
		if (t_last_color % gen_par.t_between_colors == 0) {
#ifdef INFINITY_DEBUG
			if (!mode_interactif) {
				old_color = color;
				color = rand() % NB_PALETTES;
				t_last_color = 0;
			}
#else
			old_color = color;
			color = rand() % NB_PALETTES;
			t_last_color = 0;
#endif
		}

		new_fps = config_get_fps();
		if (new_fps != fps) {
			fps = new_fps;
			frame_length = (gint32)(((1.0 / fps) * 1000));
			g_message("Infinity[%d]: setting maximum rate at ~%d frames/second", __LINE__, fps);
		}

		now = (gint32)SDL_GetTicks();
		if ((idle_time = (now - render_time)) < frame_length)
			g_usleep(idle_time * 900);
	}

	return 0;
}

#if MMX_DETECTION
static int renderer_mmx(void *arg)
{
	gint32 render_time, now;
	gint32 frame_length;
	gint32 idle_time;
	gint32 fps, new_fps;

	/* We suppose here that config module have been initialized */
	fps = config_get_fps();
	frame_length = ((1.0 / fps) * 1000);
	g_message("Infinity[%d]: setting maximum rate at ~%d frames/second", __LINE__, fps);
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
			display_resize(scr_par.width, scr_par.height);
			config_set_xres(scr_par.width);
			config_set_yres(scr_par.height);
			must_resize = FALSE;
			g_return_val_if_fail(SDL_LockMutex(resizing_mutex) >= 0, -1);
			resizing = FALSE;
			g_return_val_if_fail(SDL_UnlockMutex(resizing_mutex) >= 0, -1);
		}
		render_time = SDL_GetTicks();
		display_blur_mmx(scr_par.width * scr_par.height * current_effect.num_effect);
		spectral(&current_effect);
		curve(&current_effect);
		if (t_last_color <= 32)
			change_color(old_color, color, t_last_color * 8);
		next_color();
		next_effect();
		if (t_last_effect % gen_par.t_between_effects == 0) {
#ifdef INFINITY_DEBUG
			if (!mode_interactif) {
				display_load_random_effect(&current_effect);
				t_last_effect = 0;
			}
#else
			display_load_random_effect(&current_effect);
			t_last_effect = 0;
#endif
		}
		if (t_last_color % gen_par.t_between_colors == 0) {
#ifdef INFINITY_DEBUG
			if (!mode_interactif) {
				old_color = color;
				color = rand() % NB_PALETTES;
				t_last_color = 0;
			}
#else
			old_color = color;
			color = rand() % NB_PALETTES;
			t_last_color = 0;
#endif
		}

		new_fps = config_get_fps();
		if (new_fps != fps) {
			fps = new_fps;
			frame_length = ((1.0 / fps) * 1000);
			g_message("Infinity[%d]: setting maximum rate at ~%d frames/second", __LINE__, fps);
		}

		now = SDL_GetTicks();
		if ((idle_time = (now - render_time)) < frame_length)
			g_usleep(idle_time * 900);
	}

	return 0;
}
#endif /* MMX_DETECTION */

static void set_title(void)
{
	SDL_WM_SetCaption(current_title, "Infinity");
}
