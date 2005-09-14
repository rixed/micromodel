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
#include <math.h>
#include <assert.h>
#include "libmicromodel/grid.h"
#include "gridsel.h"

/* Private Functions */

static double compute_height(const Vec *x, const Vec *y, const Vec *n) {
	double cx = -Vec_scalar(n, y);
	double cy = Vec_scalar(n, x);
	double ang = acos(cx);
	if (cy < 0) ang = 2*M_PI - ang;
	return .75-1.5*ang/(2*M_PI);
}

static Edge *cut(Edge *edge, GridSel *new_vertices, GridSel *facets, double softness) {
	// Keep values that will be usefull to displace the new vertex
	Vec y = *Edge_normal(edge);
	Vertex *extrem1 = Edge_get_vertex(edge, SOUTH);
	Vertex *extrem2 = Edge_get_vertex(edge, NORTH);
	double len = Edge_length(edge);
	// cut
	Edge *e = Grid_edge_cut(edge, .5);
	assert(e);
	Vertex *new_vertex = Edge_get_vertex(edge, NORTH);
	GridSel_add(new_vertices, new_vertex);
	// displacement if not constrained
	EdgeSide side;
	for (side = 0; side < NB_SIDES; side++) {
		Facet *f = Edge_get_facet(edge, side);
		if (! GridSel_selected(facets, f)) break;
	}
	if (side == NB_SIDES) {
		Vec x = *Vertex_position(extrem1);
		Vec_sub(&x, Vertex_position(new_vertex));
		Vec_normalize(&x);
		double h = compute_height(&x, &y, Vertex_normal(extrem1));
		Vec_scale(&x, -1);
		h += compute_height(&x, &y, Vertex_normal(extrem2));
		Vec_scale(&y, softness*h*.5*len);
		Vec_add(Vertex_position(new_vertex), &y);
	}
	return e;
}

/* Public Functions */

GridSel GridSel_smooth(GridSel *this, unsigned level, double softness) {
	assert(this);
	GridSel edges;
	GridSel_convert(this, &edges, GridSel_EDGE, GridSel_MIN);
	while (level > 0) {
		GridSel new_edges, new_vertices;
		GridSel_construct(&new_vertices, GridSel_VERTEX);
		GridSel_construct(&new_edges, GridSel_EDGE);
		GridSel_reset(&edges);
		Edge *edge;
		while ( (edge = GridSel_each(&edges)) ) {
			GridSel_add(&new_edges, cut(edge, &new_vertices, this, softness));
		}
		GridSel_add_or_sub(&edges, &new_edges, true);
		GridSel_destruct(&new_edges);
		new_edges = GridSel_connect(&new_vertices, this, true);	// connect will add created facets in this
		GridSel_add_or_sub(&edges, &new_edges, true);
		GridSel_destruct(&new_edges);
		GridSel_destruct(&new_vertices);
		level--;
	}
	return edges;
}

