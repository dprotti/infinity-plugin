/**
 * \file compute.h
 *
 * \brief This modules is responsible for calculate the screen that
 * will be showed.
 */

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
#ifndef __INFINITY_COMPUTE__
#define __INFINITY_COMPUTE__


#include "types.h"


#define NB_FCT 7
#define PI 3.14159


/**
 * Represents the interpollation information.
 */
typedef struct {
	guint32 coord;  /**< Coordinates of the top left pixel. */
	guint32 weight; /**< 32 bits = 4*8 = weights of the four corners */
} t_interpol;


/**
 * Represents a field of interpollation vectors.
 *
 * \warning Don't use width*height*sizeof(::t_interpol) for
 * to calculate the memory size of an object of this type
 * because this is actually not true.
 */
typedef struct {
	gint32		width;  /**< number of vectors */
	gint32		height; /**< length of each vector */
	t_interpol *	vector; /**< pointer to the vector field */
} vector_field_t;



/*
 * The constructor of the ::vector_field_t type.
 */
vector_field_t *compute_vector_field_new(int width, int height);

/**
 * The destructor of the ::vector_field_t type.
 *
 * @param vector_field Must be non NULL pointer to a
 * ::vector_field_t object.
 */
void compute_vector_field_destroy(vector_field_t *vector_field);

/**
 * It frees any allocated resource and mades clenaup work.
 */
void compute_quit(void);

/**
 * Initializes this module.
 *
 * For to do this the user configuration options are readed and
 * internals structures setting accordingly. If this options
 * change, the module must be reinitialized in order to the
 * changes take effect.
 */
void compute_init(void);

/**
 * Tells the module that the screen has been resized.
 *
 * \warning May be this will be deprecated.
 */
void compute_resize(gint32 width, gint32 height);

void compute_generate_vector_field(vector_field_t *vector_field);

byte *compute_surface(t_interpol *vector, gint32 width, gint32 height);
byte *compute_surface_mmx(t_interpol *vector, gint32 width, gint32 height);


#endif /* __INFINITY_COMPUTE__ */
