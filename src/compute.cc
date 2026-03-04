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
#include <glib.h>
#include <immintrin.h> // For AVX2 intrinsics
#include <math.h>

#include "compute.h"
#include "config.h"
#include "types.h"

typedef struct t_coord
{
    gint32 x, y;
} t_coord;

typedef struct t_complex
{
    gfloat x, y;
} t_complex;

static gint32 width, height, scale;

static byte* surface1;
static byte* surface2;

static bool avx2_available = false;

static inline t_complex
fct(t_complex a, guint32 n, gint32 p1, gint32 p2) /* p1 et p2:0-4 */
{
    t_complex b;
    gfloat fact;
    gfloat an;
    gfloat circle_size;
    gfloat speed;
    gfloat co, si;

    a.x -= width / 2;
    a.y -= height / 2;

    switch (n)
    {
    case 0:
        an = 0.025 * (p1 - 2) + 0.002;
        co = cos(an);
        si = sin(an);
        circle_size = height * 0.25;
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
        circle_size = height * 0.45;
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
        circle_size = height * 0.25;
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
        circle_size = height * 0.25;
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
        circle_size = height * 0.25;
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
        // circle_size = height * 0.25;
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
    b.x += width / 2;
    b.y += height / 2;
    if (b.x < 0.0)
        b.x = 0.0;
    if (b.y < 0.0)
        b.y = 0.0;
    if (b.x > (gfloat)width - 1)
        b.x = (gfloat)width - 1;
    if (b.y > (gfloat)height - 1)
        b.y = (gfloat)height - 1;
    return b;
}

/* We are trusting here on vector_field != NULL !!! */
static inline void compute_generate_sector(
    guint32 g, guint32 f, guint32 p1, guint32 p2, guint32 debut, guint32 step,
    vector_field_t* vector_field)
{
    const guint32 width = (guint32)vector_field->width;
    const guint32 height = (guint32)vector_field->height;
    const guint32 prop_transmitted = 249;
    const guint32 b_add = g * width * height;
    t_interpol* vector = vector_field->vector;
    guint32 fin = debut + step;
    guint32 cx, cy;

    if (fin > height)
        fin = height;
    for (cy = debut; cy < fin; cy++)
    {
        for (cx = 0; cx < width; cx++)
        {
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
            vector[b_add + add].weight =
                (w1 << 24) | (w2 << 16) | (w3 << 8) | w4;
        }
    }
}

void compute_init(gint32 _width, gint32 _height, gint32 _scale)
{
    width = _width;
    height = _height;
    scale = _scale;

    surface1 = (byte*)g_malloc((gulong)(width + 1) * (height + 1));
    surface2 = (byte*)g_malloc((gulong)(width + 1) * (height + 1));

    avx2_available = __builtin_cpu_supports("avx2");
}

void compute_resize(gint32 _width, gint32 _height)
{
    width = _width;
    height = _height;
    g_free(surface1);
    g_free(surface2);
    surface1 = (byte*)g_malloc0((gulong)(width + 1) * (height + 1));
    surface2 = (byte*)g_malloc0((gulong)(width + 1) * (height + 1));
}

vector_field_t* compute_vector_field_new(gint32 width, gint32 height)
{
    vector_field_t* field;

    field = g_new0(vector_field_t, 1);
    field->vector = g_new0(t_interpol, width * height * NB_FCT);
    field->width = width;
    field->height = height;
    return field;
}

void compute_vector_field_destroy(vector_field_t* vector_field)
{
    g_return_if_fail(vector_field != NULL);

    g_free(vector_field->vector);
    g_free(vector_field);
}

void compute_quit()
{
    g_free(surface1);
    g_free(surface2);
}

void compute_generate_vector_field(vector_field_t* vector_field)
{
    guint32 f, i, _height;

    g_return_if_fail(vector_field != NULL);
    g_return_if_fail(vector_field->height >= 0);

    _height = (guint32)vector_field->height;

    for (f = 0; f < NB_FCT; f++)
        for (i = 0; i < _height; i += 10)
            compute_generate_sector(f, f, 2, 2, i, 10, vector_field);
}

static inline void
scalar_compute_surface(t_interpol* vector, gint32 width, gint32 height)
{
    gint32 add_dest = 0;
    for (gint32 j = 0; j < height; ++j)
    {
        for (gint32 i = 0; i < width; ++i)
        {
            t_interpol* interpol = &vector[add_dest];
            guint32 add_src =
                (interpol->coord & 0xFFFF) * width + (interpol->coord >> 16);
            byte* ptr_pix = &surface1[add_src];
            guint32 color = ((guint32)(*(ptr_pix)) * (interpol->weight >> 24) +
                             (guint32)(*(ptr_pix + 1)) *
                                 ((interpol->weight & 0xFFFFFF) >> 16) +
                             (guint32)(*(ptr_pix + width)) *
                                 ((interpol->weight & 0xFFFF) >> 8) +
                             (guint32)(*(ptr_pix + width + 1)) *
                                 (interpol->weight & 0xFF)) >>
                            8;
            surface2[add_dest] = (byte)(color > 255 ? 255 : color);
            ++add_dest;
        }
    }
}

static const __m256i const_255 = _mm256_set1_epi32(255);
static const __m256i const_1 = _mm256_set1_epi32(1);
static const __m256i const_ff = _mm256_set1_epi32(0xFF);
static const __m256i const_3 = _mm256_set1_epi32(3);
static const __m256i const_neg3 = _mm256_set1_epi32(~3);
static const __m256i pack_mask = _mm256_setr_epi8(
    0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

static inline void
simd_compute_surface(t_interpol* vector, gint32 width, gint32 height)
{
    gint32 add_dest = 0;
    __m256i width_v = _mm256_set1_epi32(width);
    for (gint32 j = 0; j < height; ++j)
    {
        gint32 i = 0;
        for (; i + 8 <= width; i += 8)
        {
            t_interpol* interpol = &vector[add_dest];
            // Extract to arrays for loading into vectors
            gint32 x[8], y[8], a[8], b[8], c[8], d[8];
            for (gint32 k = 0; k < 8; ++k)
            {
                guint32 coord = interpol[k].coord;
                guint32 wgt = interpol[k].weight;
                x[k] = coord >> 16;
                y[k] = coord & 0xFFFF;
                a[k] = wgt >> 24;
                b[k] = (wgt >> 16) & 0xFF;
                c[k] = (wgt >> 8) & 0xFF;
                d[k] = wgt & 0xFF;
            }
            __m256i x_v = _mm256_loadu_si256(
                (__m256i*)x); // Assuming aligned, but use loadu for safety
            __m256i y_v = _mm256_loadu_si256((__m256i*)y);
            __m256i a_v = _mm256_loadu_si256((__m256i*)a);
            __m256i b_v = _mm256_loadu_si256((__m256i*)b);
            __m256i c_v = _mm256_loadu_si256((__m256i*)c);
            __m256i d_v = _mm256_loadu_si256((__m256i*)d);
            // add_src_v = y * width + x
            __m256i add_src_v =
                _mm256_add_epi32(_mm256_mullo_epi32(y_v, width_v), x_v);
            // Gather helper for one pixel position
            auto gather_byte = [&](__m256i offsets_v) -> __m256i
            {
                __m256i base_addr_v = _mm256_add_epi32(add_src_v, offsets_v);
                __m256i byte_offsets = _mm256_and_si256(base_addr_v, const_3);
                __m256i aligned_addr_v =
                    _mm256_and_si256(base_addr_v, const_neg3);
                // Gather 32-bit words
                __m256i words =
                    _mm256_i32gather_epi32((int*)surface1, aligned_addr_v, 1);
                // Shift right by (byte_offset * 8) to extract the byte
                __m256i shifts = _mm256_slli_epi32(byte_offsets, 3);
                __m256i bytes = _mm256_srlv_epi32(words, shifts);
                return _mm256_and_si256(bytes, const_ff);
            };
            __m256i tl_v = gather_byte(_mm256_setzero_si256());
            __m256i tr_v = gather_byte(const_1);
            __m256i bl_v = gather_byte(width_v);
            __m256i br_v = gather_byte(_mm256_add_epi32(width_v, const_1));
            // sum = tl*a + tr*b + bl*c + br*d
            __m256i sum1 = _mm256_add_epi32(
                _mm256_mullo_epi32(tl_v, a_v), _mm256_mullo_epi32(tr_v, b_v));
            __m256i sum2 = _mm256_add_epi32(
                _mm256_mullo_epi32(bl_v, c_v), _mm256_mullo_epi32(br_v, d_v));
            __m256i sum_v = _mm256_add_epi32(sum1, sum2);
            // color = sum >> 8, clamp to 0-255
            __m256i color_v = _mm256_srli_epi32(sum_v, 8);
            color_v = _mm256_min_epi32(color_v, const_255);
            color_v = _mm256_max_epi32(
                color_v,
                _mm256_setzero_si256()); // Assuming no negative, but for safety
            __m256i packed = _mm256_shuffle_epi8(color_v, pack_mask);
            // Extract low and high 128-bit lanes
            __m128i low_lane = _mm256_castsi256_si128(
                packed); // [c0, c1, c2, c3, 0, 0, ..., 0]
            __m128i high_lane = _mm256_extracti128_si256(
                packed, 1); // [c4, c5, c6, c7, 0, 0, ..., 0]
            // Align high lane to positions 4-7 by left-shifting 4 bytes
            __m128i shifted_high = _mm_slli_si128(
                high_lane, 4); // [0, 0, 0, 0, c4, c5, c6, c7, 0, ..., 0]
            // Merge
            __m128i eight_bytes = _mm_or_si128(
                low_lane,
                shifted_high); // [c0, c1, c2, c3, c4, c5, c6, c7, 0, ..., 0]
            // Store first 8 bytes
            _mm_storel_epi64((__m128i*)(surface2 + add_dest), eight_bytes);
            add_dest += 8;
        }
        // Scalar remainder for this row
        for (; i < width; ++i)
        {
            t_interpol* interpol = &vector[add_dest];
            guint32 add_src =
                (interpol->coord & 0xFFFF) * width + (interpol->coord >> 16);
            byte* ptr_pix = &surface1[add_src];
            guint32 color = ((guint32)(*(ptr_pix)) * (interpol->weight >> 24) +
                             (guint32)(*(ptr_pix + 1)) *
                                 ((interpol->weight & 0xFFFFFF) >> 16) +
                             (guint32)(*(ptr_pix + width)) *
                                 ((interpol->weight & 0xFFFF) >> 8) +
                             (guint32)(*(ptr_pix + width + 1)) *
                                 (interpol->weight & 0xFF)) >>
                            8;
            surface2[add_dest] = (byte)(color > 255 ? 255 : color);
            ++add_dest;
        }
    }
}

byte* compute_surface(t_interpol* vector, gint32 width, gint32 height)
{
    // -- Temporary disable AVX2 until obtaining smoothness similar or better
    // than scalar version in 4K fullscreen -- bool use_simd = avx2_available &&
    // (width >= 1280) && (height >= 720);

    // if (use_simd)
    // {
    //     simd_compute_surface(vector, width, height);
    // }
    // else
    // {
    scalar_compute_surface(vector, width, height);
    // }

    // Swap surfaces
    byte* ptr_swap = surface2;
    surface2 = surface1;
    surface1 = ptr_swap;

    return surface1;
}
