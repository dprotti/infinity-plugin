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
#ifndef __INFINITY_TYPES__
#define __INFINITY_TYPES__

#include <glib.h> /* gint32 */

#ifdef __cplusplus
#    include <cstdint>
#else
#    include <stdint.h>
#endif

#ifdef __cplusplus
using byte = std::uint8_t;
#else
typedef uint8_t byte;
#endif

#ifdef __cplusplus
using t_color = gint32;
using t_num_effect = gint32;
#else
typedef gint32 t_color;
typedef gint32 t_num_effect;
#endif

#endif /* __INFINITY_TYPES__ */
