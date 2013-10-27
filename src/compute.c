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
#include <glib.h>

#include "config.h"
#include "compute.h"
#include "infconfig.h"
#ifdef MMX_DETECTION
#include "mmx.h"
#endif

typedef struct t_coord {
	gint32 x, y;
} t_coord;

typedef struct t_complex {
	gfloat x, y;
} t_complex;

static t_screen_parameters scr_par;

static byte *surface1;
static byte *surface2;

static inline t_complex fct(t_complex a, guint32 n, gint32 p1, gint32 p2)   /* p1 et p2:0-4 */
{
	t_complex b;
	gfloat fact;
	gfloat an;
	gfloat circle_size;
	gfloat speed;
	gfloat co, si;

	a.x -= scr_par.width / 2;
	a.y -= scr_par.height / 2;

	switch (n) {
	case 0:
		an = 0.025 * (p1 - 2) + 0.002;
		co = cos(an);
		si = sin(an);
		circle_size = scr_par.height * 0.25;
		speed = (gfloat)2000 + p2 * 500;
		b.x = (co * a.x - si * a.y);
		b.y = (si * a.x + co * a.y);
		fact = -(sqrt(b.x * b.x + b.y * b.y) - circle_size) / speed + 1;
		b.x *= fact;
		b.y *= fact;
		break;
	case 1:
		an = 0.015 * (p1 - 2) + 0.002;
		co = cos(an);
		si = sin(an);
		circle_size = scr_par.height * 0.45;
		speed = (gfloat)4000 + p2 * 1000;
		b.x = (co * a.x - si * a.y);
		b.y = (si * a.x + co * a.y);
		fact = (sqrt(b.x * b.x + b.y * b.y) - circle_size) / speed + 1;
		b.x *= fact;
		b.y *= fact;
		break;
	case 2:
		an = 0.002;
		co = cos(an);
		si = sin(an);
		circle_size = scr_par.height * 0.25;
		speed = (gfloat)400 + p2 * 100;
		b.x = (co * a.x - si * a.y);
		b.y = (si * a.x + co * a.y);
		fact = -(sqrt(b.x * b.x + b.y * b.y) - circle_size) / speed + 1;
		b.x *= fact;
		b.y *= fact;
		break;
	case 3:
		an = (sin(sqrt(a.x * a.x + a.y * a.y) / 20) / 20) + 0.002;
		co = cos(an);
		si = sin(an);
		circle_size = scr_par.height * 0.25;
		speed = (gfloat)4000;
		b.x = (co * a.x - si * a.y);
		b.y = (si * a.x + co * a.y);
		fact = -(sqrt(b.x * b.x + b.y * b.y) - circle_size) / speed + 1;
		b.x *= fact;
		b.y *= fact;
		break;
	case 4:
		an = 0.002;
		co = cos(an);
		si = sin(an);
		circle_size = scr_par.height * 0.25;
		speed = sin(sqrt(a.x * a.x + a.y * a.y) / 5) * 3000 + 4000;
		b.x = (co * a.x - si * a.y);
		b.y = (si * a.x + co * a.y);
		fact = -(sqrt(b.x * b.x + b.y * b.y) - circle_size) / speed + 1;
		b.x *= fact;
		b.y *= fact;
		break;
	case 5:
		b.x = a.x * 1.02;
		b.y = a.y * 1.02;
		break;
	case 6:
		an = 0.002;
		co = cos(an);
		si = sin(an);
		circle_size = scr_par.height * 0.25;
		fact = 1 + cos(atan(a.x / (a.y + 0.00001)) * 6) * 0.02;
		b.x = (co * a.x - si * a.y);
		b.y = (si * a.x + co * a.y);
		b.x *= fact;
		b.y *= fact;
		break;
	default:
		b.x = (gfloat)0.0;
		b.y = (gfloat)0.0;
	}
	b.x += scr_par.width / 2;
	b.y += scr_par.height / 2;
	if (b.x < 0.0)
		b.x = 0.0;
	if (b.y < 0.0)
		b.y = 0.0;
	if (b.x > (gfloat)scr_par.width - 1)
		b.x = (gfloat)scr_par.width - 1;
	if (b.y > (gfloat)scr_par.height - 1)
		b.y = (gfloat)scr_par.height - 1;
	return b;
}

/* We are trusting here on vector_field != NULL !!! */
static inline void compute_generate_sector(guint32 g, guint32 f, guint32 p1, guint32 p2,
					   guint32 debut, guint32 step, vector_field_t *vector_field)
{
	const guint32 width = (guint32)vector_field->width;
	const guint32 height = (guint32)vector_field->height;
	const guint32 prop_transmitted = 249;
	const guint32 b_add = g * width * height;
	t_interpol *vector = vector_field->vector;
	guint32 fin = debut + step;
	guint32 cx, cy;

	if (fin > height)
		fin = height;
	for (cy = debut; cy < fin; cy++) {
		for (cx = 0; cx < width; cx++) {
			t_complex a;
			gfloat fpy;
			guint32 rw, lw, add;
			guint32 w1, w2, w3, w4;
			guint32 x, y;

			a.x = (gfloat)cx;
			a.y = (gfloat)cy;
			a = fct(a, f, p1, p2);
			add = cx + cy * width;
			x = (guint32)(a.x);
			y = (guint32)(a.y);
			vector[b_add + add].coord = (x << 16) | y;

			fpy = a.y - floor(a.y);
			rw = (guint32)((a.x - floor(a.x)) * prop_transmitted);
			lw = prop_transmitted - rw;
			w4 = (guint32)(fpy * rw);
			w2 = rw - w4;
			w3 = (guint32)(fpy * lw);
			w1 = lw - w3;
			vector[b_add + add].weight = \
				(w1 << 24) | (w2 << 16) | (w3 << 8) | w4;
		}
	}
}

/*
 * Public functions
 */
void compute_init(void)
{
	scr_par.width = config_get_xres();
	scr_par.height = config_get_yres();
	scr_par.scale = config_get_sres();

	surface1 = (byte *)g_malloc((gulong)(scr_par.width + 1) * (scr_par.height + 1));
	surface2 = (byte *)g_malloc((gulong)(scr_par.width + 1) * (scr_par.height + 1));
}

void compute_resize(gint32 width, gint32 height)
{
	scr_par.width = width;
	scr_par.height = height;
	g_free(surface1);
	g_free(surface2);
	surface1 = (byte *)g_malloc((gulong)(scr_par.width + 1) * (scr_par.height + 1));
	surface2 = (byte *)g_malloc((gulong)(scr_par.width + 1) * (scr_par.height + 1));
}

vector_field_t *compute_vector_field_new(gint32 width, gint32 height)
{
	vector_field_t *field;

	field = g_new0(vector_field_t, 1);
	field->vector = g_new0(t_interpol, width * height * NB_FCT);
	field->width = width;
	field->height = height;
	return field;
}

void compute_vector_field_destroy(vector_field_t *vector_field)
{
	g_return_if_fail(vector_field != NULL);

	g_free(vector_field->vector);
	g_free(vector_field);
	vector_field = NULL;
}

void compute_quit()
{
	g_free(surface1);
	g_free(surface2);
}

void compute_generate_vector_field(vector_field_t *vector_field)
{
	guint32 f, i, height;

	g_return_if_fail(vector_field != NULL);
	g_return_if_fail(vector_field->height >= 0);

	height = (guint32)vector_field->height;

	for (f = 0; f < NB_FCT; f++)
		for (i = 0; i < height; i += 10)
			compute_generate_sector(f, f, 2, 2, i, 10, vector_field);
}

inline byte *compute_surface(t_interpol *vector, gint32 width, gint32 height)
{
	gint32 i, j;
	gint32 add_dest = 0;
	guint32 add_src;
	t_interpol *interpol;
	register byte *ptr_pix;
	guint32 color;
	byte *ptr_swap;

	for (j = 0; j < height; j++)
		for (i = 0; i < width; i++) {
			interpol = &vector[add_dest];
			add_src = (interpol->coord & 0xFFFF) * width + (interpol->coord >> 16);
			ptr_pix = &((byte *)surface1)[add_src];
			color = ((guint32)(*(ptr_pix)) * (interpol->weight >> 24)
				 + (guint32)(*(ptr_pix + 1)) * ((interpol->weight & 0xFFFFFF) >> 16)
				 + (guint32)(*(ptr_pix + width)) * ((interpol->weight & 0xFFFF) >> 8)
				 + (guint32)(*(ptr_pix + width + 1)) * (interpol->weight & 0xFF)) >> 8;
			if (color > 255)
				surface2[add_dest] = (byte)255;
			else
				surface2[add_dest] = (byte)color;
			add_dest++;
		}
	ptr_swap = surface2;
	surface2 = surface1;
	surface1 = ptr_swap;

	return surface1;
}

#if MMX_DETECTION
inline byte *compute_surface_mmx(t_interpol *vector, gint32 width, gint32 height)
{
	/*@unused@*/
	volatile mmx_t mm0, mm1, mm2;
	volatile mmx_t offsets, r;
	t_interpol *interpol;
	gint32 i, j, color;
	gint32 add_dest = 0;
	guint32 add_src;
	register byte *ptr_pix;
	byte *ptr_swap;

	for (j = 0; j < height; j++)
		for (i = 0; i < width; i++) {
			interpol = &vector[add_dest];
			add_src = (interpol->coord & 0xFFFF) * width + (interpol->coord >> 16);
			ptr_pix = &((byte *)surface1)[add_src];
			((guint16 *)&offsets)[0] = (guint16) * (ptr_pix + width + 1);
			((guint16 *)&offsets)[1] = (guint16) * (ptr_pix + width);
			((guint16 *)&offsets)[2] = (guint16) * (ptr_pix + 1);
			((guint16 *)&offsets)[3] = (guint16) * (ptr_pix);
			/* MMX mode entry */
			movd_m2r(interpol->weight, mm1);
			movq_m2r(offsets, mm2);
			pxor_r2r(mm0, mm0);
			punpcklbw_r2r(mm0, mm1);
			pmaddwd_r2r(mm1, mm2);
			movq_r2m(mm2, r);
			emms();
			/* MMX mode exit */
			color = (((gint32 *)&r)[0] + ((gint32 *)&r)[1]) >> 8;
			if (color > 255)
				surface2[add_dest] = 255;
			else
				surface2[add_dest] = (byte)color;
			add_dest++;
		}
	ptr_swap = surface1;
	surface1 = surface2;
	surface2 = ptr_swap;
	return surface1;
}
#endif /* MMX_DETECTION */
