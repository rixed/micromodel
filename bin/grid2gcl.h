/* This file is part of MicroModel.
 *
 * Copyright (C) 2005 Cedric Cellier.
 *
 * MicroModel is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * MicroModel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MicroModel; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef GRID2GCL_H_041124
#define GRID2GCL_H_041124

#include "libmicromodel/grid.h"

int grid2gcl(unsigned sel_name, unsigned basis_name, const char *tiff_filename);
void grid2gcl_end(void);
/** Return the Vertex associated with the GCL vertex order given.
 * Usefull for picking, when geomview return orders and we need vertices.
 */
Vertex *get_vertex_from_gcl_order(unsigned order);
Facet *get_facet_from_gcl_order(unsigned order);

#endif
// vi:ts=3:sw=3
