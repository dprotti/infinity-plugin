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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <SDL.h>

#include "config.h"
#include "display.h"
#include "types.h"

#define wrap(a) (a < 0 ? 0 : (a > 255 ? 255 : a))
#define assign_max(p, a) (*p <= a ? *p = a : 0)

#define VIDEO_FLAGS ((Uint32)(SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE))

typedef struct sincos {
	gint32	i;
	gfloat *f;
} sincos_t;

static gint16 pcm_data[2][512];
G_LOCK_DEFINE_STATIC(pcm_data);

static gint32 width, height, scale;

/* Little optimization for cos/sin functions */
static sincos_t cosw = { 0, NULL };
static sincos_t sinw = { 0, NULL };

static vector_field_t *vector_field;

static SDL_Window *window;
static SDL_Renderer *sdl_renderer;
static SDL_Surface *screen;
static SDL_Texture *texture;

static SDL_Color color_table[NB_PALETTES][256];
static gint16 current_colors[256];

static byte *surface1;
static Player *player;

static gchar error_msg[256];
static gboolean initialized;

static gboolean allocate_screen_and_texture() {
	if (screen != NULL) {
		SDL_FreeSurface(screen);
	}
	screen = SDL_CreateRGBSurface(0, width, height, 16, 0, 0, 0, 0);
	if (screen == NULL) {
		g_snprintf(error_msg, 256, "Infinity cannot create display: %s", SDL_GetError());
		player->notify_critical_error(error_msg);
		return FALSE;
	}
	SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);
	if (texture != NULL) {
		SDL_DestroyTexture(texture);
	}
	texture = SDL_CreateTexture(sdl_renderer,
		                       SDL_PIXELFORMAT_RGB565,
                               SDL_TEXTUREACCESS_STREAMING,
                               width, height);
	if (texture == NULL) {
		g_snprintf(error_msg, 256, "Infinity cannot create SDL texture: %s", SDL_GetError());
		player->notify_critical_error(error_msg);
		return FALSE;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
	SDL_RenderSetLogicalSize(sdl_renderer, width, height);
	return TRUE;
}

static gboolean sdl_init()
{
	SDL_InitSubSystem(SDL_INIT_EVENTS);
	int res = SDL_CreateWindowAndRenderer(width, height, VIDEO_FLAGS, &window, &sdl_renderer);
	if (res < 0) {
		g_snprintf(error_msg, 256, "Infinity cannot initialize SDL: %s", SDL_GetError());
		player->notify_critical_error(error_msg);
		return FALSE;
	}
	return allocate_screen_and_texture();
}

static void generate_colors()
{
	gint32 i, k;
	gfloat colors[NB_PALETTES][2][3] = { { { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 } },
					     { { 2.0, 1.5, 0.0 }, { 0.0, 0.5, 2.0 } },
					     { { 0.0, 1.0, 2.0 }, { 0.0, 1.0, 0.0 } },
					     { { 0.0, 2.0, 1.0 }, { 0.0, 0.0, 1.0 } },
					     { { 2.0, 0.0, 0.0 }, { 0.0, 1.0, 1.0 } } };

	for (k = 0; k < NB_PALETTES; k++) {
		for (i = 0; i < 128; i++) {
			color_table[k][i].r = (Uint8)(colors[k][0][0] * i);
			color_table[k][i].g = (Uint8)(colors[k][0][1] * i);
			color_table[k][i].b = (Uint8)(colors[k][0][2] * i);
			color_table[k][i + 128].r = (Uint8)(colors[k][0][0] * 127 + colors[k][1][0] * i);
			color_table[k][i + 128].g = (Uint8)(colors[k][0][1] * 127 + colors[k][1][1] * i);
			color_table[k][i + 128].b = (Uint8)(colors[k][0][2] * 127 + colors[k][1][2] * i);
		}
	}
}

static void display_surface()
{
	gint32 i, j;
	gint16 *pdest;
	byte *psrc;
	gboolean screen_locked = FALSE;

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0) {
			g_critical("Infinity cannot lock SDL surface: %s", SDL_GetError());
			return;
		}
		screen_locked = TRUE;
	}

	psrc = surface1;
	for (i = 0; i < height; i++) {
		pdest = (gint16 *)(screen->pixels + i * screen->pitch);
		for (j = 0; j < width; j++)
			*pdest++ = current_colors[*psrc++];
	}
	SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);

	if (screen_locked)
		SDL_UnlockSurface(screen);
	else {
		SDL_RenderClear(sdl_renderer);
		SDL_RenderCopy(sdl_renderer, texture, NULL, NULL);
		SDL_RenderPresent(sdl_renderer);
	}
}

#define plot1(x, y, c) \
\
	if ((x) > 0 && (x) < width - 3 && (y) > 0 && (y) < height - 3) \
		assign_max(&(surface1)[(x) + (y) * width], (c)) \
\

#define plot2(x, y, c) \
	{ \
		gint32 ty; \
		if ((x) > 0 && (gint32)(x) < width - 3 && (y) > 0 && (gint32)(y) < height - 3) { \
			ty = (gint32)(y) * width; \
			assign_max((&(surface1)[(gint32)(x) + ty]), (c)); \
			assign_max((&(surface1)[(gint32)(x) + 1 + ty]), (c)); \
			assign_max((&(surface1)[(gint32)(x) + ty + width]), (c)); \
			assign_max((&(surface1)[(gint32)(x) + 1 + ty + width]), (c)); \
		} \
	} \

#define SWAP(x, y) \
	x ^= y; \
	y ^= x; \
	x ^= y;

static void line(gint32 x1, gint32 y1, gint32 x2, gint32 y2, gint32 c)
{
	gint32 dx, dy, cxy, dxy;

	/* calculate the distances */
	dx = abs(x1 - x2);
	dy = abs(y1 - y2);
	cxy = 0;
	if (dy > dx) {
		/* Follow Y axis */
		if (y1 > y2) {
			SWAP(y1, y2);
			SWAP(x1, x2);
		}
		if (x1 > x2)
			dxy = -1;
		else
			dxy = 1;
		for (y1 = y1; y1 < y2; y1++) {
			cxy += dx;
			if (cxy >= dy) {
				x1 += dxy;
				cxy -= dy;
			}
			plot1(x1, y1, (byte)c);
		}
	} else {
		/* Follow X axis */
		if (x1 > x2) {
			SWAP(x1, x2);
			SWAP(y1, y2);
		}
		if (y1 > y2)
			dxy = -1;
		else
			dxy = 1;

		for (x1 = x1; x1 < x2; x1++) {
			cxy += dy;
			if (cxy >= dx) {
				y1 += dxy;
				cxy -= dx;
			}
			plot1(x1, y1, (byte)c);
		}
	}
}

static void sdl_quit() {
	if (screen != NULL)
		SDL_FreeSurface(screen);
	screen = NULL;
	if (window != NULL)
		SDL_DestroyWindow(window);
	window = NULL;
}

gboolean display_init(gint32 _width, gint32 _height, gint32 _scale, Player *_player)
{
	width = _width;
	height = _height;
	scale = _scale;
	player = _player;

	if (! effects_load_effects(player)) {
		return FALSE;
	}
	if (! sdl_init()) {
		sdl_quit();
		return FALSE;
	}
	compute_init(width, height, scale);
	generate_colors();
	vector_field = compute_vector_field_new(width, height);
	compute_generate_vector_field(vector_field);
	initialized = TRUE;
	return TRUE;
}

void display_quit(void)
{
	if (! initialized)
		return;
	compute_vector_field_destroy(vector_field);
	compute_quit();
	sdl_quit();
	// do not call SDL_Quit() since there is another plugin that use
	// SDL inside Audacious.
	// Do not call neither SDL_QuitSubsystem() just in case
	//SDL_QuitSubSystem(SDL_INIT_EVENTS);
	initialized = FALSE;
}

gboolean display_resize(gint32 _width, gint32 _height)
{
	width = _width;
	height = _height;

	gboolean screen_ok = allocate_screen_and_texture();
	compute_vector_field_destroy(vector_field);
	vector_field = compute_vector_field_new(width, height);
	compute_resize(width, height);
	compute_generate_vector_field(vector_field);
	return screen_ok;
}

inline void display_set_pcm_data(const float *data, int channels)
{
	if (channels != 2) {
		g_critical("Unsupported number of channels (%d)\n", channels);
		return;
	}
	G_LOCK(pcm_data);
	// TODO check this out, different types here...
	memcpy(pcm_data, data, 2 * 512 * sizeof(gint16));
	G_UNLOCK(pcm_data);
}

void change_color(gint32 t2, gint32 t1, gint32 w)
{
	gint32 i;
	gint32 r, g, b;

	for (i = 0; i < 255; i++) {
		r = ((color_table[t1][i].r * w + color_table[t2][i].r * (256 - w)) >> 11);
		g = ((color_table[t1][i].g * w + color_table[t2][i].g * (256 - w)) >> 10);
		b = ((color_table[t1][i].b * w + color_table[t2][i].b * (256 - w)) >> 11);
		current_colors[i] = (r << 11) + (g << 5) + b;
	}
}

inline void display_blur(guint32 vector_index)
{
	surface1 = compute_surface(&(vector_field->vector[vector_index]),
				   vector_field->width, vector_field->height);
	display_surface();
}

#if MMX_DETECTION
inline void display_blur_mmx(guint32 vector_index)
{
	surface1 = compute_surface_mmx(&(vector_field->vector[vector_index]),
				       vector_field->width, vector_field->height);
	display_surface();
}
#endif

void spectral(t_effect *current_effect)
{
	gint32 i, halfheight, halfwidth;
	gfloat old_y1, old_y2;
	gfloat y1, y2;
	const gint32 density_lines = 5;
	const gint32 step = 4;
	const gint32 shift = (current_effect->spectral_shift * height) >> 8;

	G_LOCK(pcm_data);
	y1 = (gfloat)((((pcm_data[0][0] + pcm_data[1][0]) >> 9) * current_effect->spectral_amplitude * height) >> 12);
	y2 = (gfloat)((((pcm_data[0][0] + pcm_data[1][0]) >> 9) * current_effect->spectral_amplitude * height) >> 12);
	if (cosw.i != width || sinw.i != width) {
		g_free(cosw.f);
		g_free(sinw.f);
		sinw.f = cosw.f = NULL;
		sinw.i = cosw.i = 0;
	}
	if (cosw.i == 0 || cosw.f == NULL) {
		gfloat halfPI = (gfloat)PI / 2;
		cosw.i = width;
		if (cosw.f != NULL)
			g_free(cosw.f);
		cosw.f = g_malloc(sizeof(gfloat) * width);
		for (i = 0; i < width; i += step)
			cosw.f[i] = cos((gfloat)i / width * PI + halfPI);
	}
	if (sinw.i == 0 || sinw.f == NULL) {
		gfloat halfPI = (gfloat)PI / 2;
		sinw.i = width;
		if (sinw.f != NULL)
			g_free(sinw.f);
		sinw.f = g_malloc(sizeof(gfloat) * width);
		for (i = 0; i < width; i += step)
			sinw.f[i] = sin((gfloat)i / width * PI + halfPI);
	}
	if (current_effect->mode_spectre == 3) {
		if (y1 < 0.0)
			y1 = 0.0;
		if (y2 < 0.0)
			y2 = 0.0;
	}
	halfheight = height >> 1;
	halfwidth = width >> 1;
	for (i = step; i < width; i += step) {
		old_y1 = y1;
		old_y2 = y2;
		y1 = (gfloat)(((pcm_data[1][(i << 9) / width / density_lines] >> 8) *
			       current_effect->spectral_amplitude * height) >> 12);
		y2 = (gfloat)(((pcm_data[0][(i << 9) / width / density_lines] >> 8) *
			       current_effect->spectral_amplitude * height) >> 12);
		/* end CS */
		switch (current_effect->mode_spectre) {
		case 0:
			line(i - step, halfheight + shift + old_y2,
			     i, halfheight + shift + y2,
			     current_effect->spectral_color);
			break;
		case 1:
			line(i - step, halfheight + shift + old_y1,
			     i, halfheight + shift + y1,
			     current_effect->spectral_color);
			line(i - step, halfheight - shift + old_y2,
			     i, halfheight - shift + y2,
			     current_effect->spectral_color);
			break;
		case 2:
			line(i - step, halfheight + shift + old_y1,
			     i, halfheight + shift + y1,
			     current_effect->spectral_color);
			line(i - step, halfheight - shift + old_y1,
			     i, halfheight - shift + y1,
			     current_effect->spectral_color);
			line(halfwidth + shift + old_y2, i - step,
			     halfwidth + shift + y2, i,
			     current_effect->spectral_color);
			line(halfwidth - shift + old_y2, i - step,
			     halfwidth - shift + y2, i,
			     current_effect->spectral_color);
			break;
		case 3:
			if (y1 < 0.0)
				y1 = 0.0;
			if (y2 < 0.0)
				y2 = 0.0;
		case 4:
			line(halfwidth + cosw.f[i - step] * (shift + old_y1),
			     halfheight + sinw.f[i - step] * (shift + old_y1),
			     halfwidth + cosw.f[i] * (shift + y1),
			     halfheight + sinw.f[i] * (shift + y1),
			     current_effect->spectral_color);
			line(halfwidth - cosw.f[i - step] * (shift + old_y2),
			     halfheight + sinw.f[i - step] * (shift + old_y2),
			     halfwidth - cosw.f[i] * (shift + y2),
			     halfheight + sinw.f[i] * (shift + y2),
			     current_effect->spectral_color);
			break;
		}
	}
	G_UNLOCK(pcm_data);
	if (current_effect->mode_spectre == 3 || current_effect->mode_spectre == 4) {
		line(halfwidth + cosw.f[width - step] * (shift + y1),
		     halfheight + sinw.f[width - step] * (shift + y1),
		     halfwidth - cosw.f[width - step] * (shift + y2),
		     halfheight + sinw.f[width - step] * (shift + y2),
		     current_effect->spectral_color);
	}
}

/*
 * TODO current_effect->curve_color must be a byte. This is related to
 * t_effect typo.
 */
void curve(t_effect *current_effect)
{
	gint32 i, j, k;
	gfloat v, vr;
	gfloat x, y;
	gfloat amplitude = (gfloat)current_effect->curve_amplitude / 256;

	for (j = 0; j < 2; j++) {
		v = 80.0;
		vr = 0.001;
		k = current_effect->x_curve;
		for (i = 0; i < 64; i++) {
			x = cos((gfloat)(k) / (v + v * j * 1.34)) * height * amplitude;
			y = sin((gfloat)(k) / (1.756 * (v + v * j * 0.93))) * height * amplitude;
			plot2(x * cos((gfloat)k * vr) + y * sin((gfloat)k * vr) + width / 2,
			      x * sin((gfloat)k * vr) - y * cos((gfloat)k * vr) + height / 2,
			      (byte)current_effect->curve_color);
			k++;
		}
	}
	current_effect->x_curve = k;
}

void display_toggle_fullscreen(void)
{
	Uint32 is_fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
	if (SDL_SetWindowFullscreen(window, is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN) < 0)
		g_warning("Infinity cannot change fullscreen mode: %s", SDL_GetError());
	SDL_ShowCursor(is_fullscreen);
}

void display_set_title(const gchar *title) {
	g_return_if_fail(window != NULL);
	SDL_SetWindowTitle(window, title);
}

void display_save_screen(void)
{
	gchar name[256];

	(void)snprintf(name, 255, "screenshot%i%s", rand() % 1000000, ".bmp");
	name[255] = '\0';
	if (SDL_SaveBMP(screen, name) < 0)
		g_warning("Error while saving file %s: %s", name, SDL_GetError());
	else
		g_message("saved");
}

inline void display_save_effect(t_effect *effect)
{
	effects_append_effect(effect);
}


inline void display_load_random_effect(t_effect *effect)
{
	effects_load_random_effect(effect);
}
