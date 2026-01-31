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
#include "config.h"
#include "display.h"
#include "types.h"
#include "ui.h"

#define wrap(a) (a < 0 ? 0 : (a > 255 ? 255 : a))
#define assign_max(p, a) (*p <= a ? *p = a : 0)

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

static guint16 *render_buffer;

typedef struct {
	guint8 r;
	guint8 g;
	guint8 b;
} color_entry_t;

static color_entry_t color_table[NB_PALETTES][256];
static gint16 current_colors[256];

static byte *surface1;
static Player *player;

static gchar error_msg[256];
static gboolean initialized;
static gboolean pending_resize;
static gint32 pending_width;
static gint32 pending_height;
static gboolean window_closed;
static gboolean visible;

static gboolean allocate_render_buffer() {
	g_free(render_buffer);
	render_buffer = g_new0(guint16, width * height);
	if (render_buffer == NULL) {
		g_snprintf(error_msg, 256, "Infinity cannot allocate render buffer");
		player->notify_critical_error(error_msg);
		return FALSE;
	}
	return TRUE;
}

static gboolean ui_init_window()
{
	if (! ui_init(width, height)) {
		g_snprintf(error_msg, 256, "Infinity cannot initialize UI window");
		player->notify_critical_error(error_msg);
		return FALSE;
	}
	return allocate_render_buffer();
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
			color_table[k][i].r = (guint8)(colors[k][0][0] * i);
			color_table[k][i].g = (guint8)(colors[k][0][1] * i);
			color_table[k][i].b = (guint8)(colors[k][0][2] * i);
			color_table[k][i + 128].r = (guint8)(colors[k][0][0] * 127 + colors[k][1][0] * i);
			color_table[k][i + 128].g = (guint8)(colors[k][0][1] * 127 + colors[k][1][1] * i);
			color_table[k][i + 128].b = (guint8)(colors[k][0][2] * 127 + colors[k][1][2] * i);
		}
	}
}

static void display_surface()
{
	gint32 i, j;
	byte *psrc;

	psrc = surface1;
	for (i = 0; i < height; i++) {
		guint16 *pdest = render_buffer + (i * width);
		for (j = 0; j < width; j++) {
			*pdest++ = current_colors[*psrc++];
		}
	}
	ui_present(render_buffer, width, height);
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
		for (; y1 < y2; y1++) {
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

		for (; x1 < x2; x1++) {
			cxy += dy;
			if (cxy >= dx) {
				y1 += dxy;
				cxy -= dx;
			}
			plot1(x1, y1, (byte)c);
		}
	}
}

static void ui_quit_window() {
	g_free(render_buffer);
	render_buffer = NULL;
	ui_quit();
}

gboolean display_init(gint32 _width, gint32 _height, gint32 _scale, Player *_player)
{
	width = _width;
	height = _height;
	scale = _scale;
	player = _player;
	pending_resize = FALSE;
	window_closed = FALSE;
	visible = TRUE;

	if (! effects_load_effects(player)) {
		return FALSE;
	}
	if (! ui_init_window()) {
		ui_quit_window();
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
	ui_quit_window();
	initialized = FALSE;
}

gboolean display_resize(gint32 _width, gint32 _height)
{
	width = _width;
	height = _height;

	gboolean screen_ok = allocate_render_buffer();
	compute_vector_field_destroy(vector_field);
	vector_field = compute_vector_field_new(width, height);
	compute_resize(width, height);
	compute_generate_vector_field(vector_field);
	ui_resize(width, height);
	return screen_ok;
}

gboolean display_take_resize(gint32 *out_width, gint32 *out_height)
{
	if (!pending_resize) {
		return FALSE;
	}
	pending_resize = FALSE;
	*out_width = pending_width;
	*out_height = pending_height;
	return TRUE;
}

gboolean display_window_closed(void)
{
	return window_closed;
}

gboolean display_is_visible(void)
{
	return visible;
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
	ui_toggle_fullscreen();
}

void display_exit_fullscreen_if_needed(void) {
	ui_exit_fullscreen_if_needed();
}

inline void display_save_effect(t_effect *effect)
{
	effects_append_effect(effect);
}

inline void display_load_random_effect(t_effect *effect)
{
	effects_load_random_effect(effect);
}

void display_notify_resize(gint32 _width, gint32 _height)
{
	pending_resize = TRUE;
	pending_width = _width;
	pending_height = _height;
	visible = TRUE;
}

void display_notify_close(void)
{
	window_closed = TRUE;
}

void display_notify_visibility(gboolean is_visible)
{
	visible = is_visible;
}
