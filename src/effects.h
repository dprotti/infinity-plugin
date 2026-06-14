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
#pragma once

#include <glib.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gint32 num_effect;
    gint32 x_curve;
    gint32 curve_color;
    gint32 curve_amplitude;
    gint32 spectral_amplitude;
    gint32 spectral_color;
    gint32 mode_spectre;
    gint32 spectral_shift;
} t_effect;

void effects_append_effect(t_effect *effect);
gboolean effects_load_effects(void);
void effects_load_random_effect(t_effect *effect);

#ifdef __cplusplus
} // extern "C"
#endif
