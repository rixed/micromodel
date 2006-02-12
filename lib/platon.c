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
#include "libmicromodel/grid.h"
#include "gridsel.h"
#include "grid.h"

/* Private Functions */

static int add_from_geometry(unsigned name, unsigned nb_vertices, const float (*vertices)[3], unsigned nb_edges, const unsigned short (*edges)[2], unsigned nb_facets, unsigned nb_edges_per_facet, const unsigned short *facets) {
	assert(nb_vertices+nb_facets-nb_edges == 2);
	if (!Grid_get()) Grid_new();
	if (!Grid_get()) return 0;
	Vec n;
	GridSel *sel = NULL;
	if (name > 0) {
		sel = Grid_get_selection(name);
		if (!sel) {
			sel = Grid_new_selection_(name, GridSel_FACET);	// by default, use FACET type
			assert(sel);
		}
	}
	Vertex *vi[nb_vertices];
	for (unsigned v=0; v<nb_vertices; v++) {
		Vec_construct(&n, vertices[v][0], vertices[v][1], vertices[v][2]);
		vi[v] = Grid_vertex_new(&n, 0,0.,0,0);
		if (sel && GridSel_VERTEX == sel->type) GridSel_add(sel, vi[v]);
	}
	Edge *ed[nb_edges];
	for (unsigned e=0; e<nb_edges; e++) {
		ed[e] = Grid_edge_new(vi[edges[e][0]], vi[edges[e][1]]);
		if (sel && GridSel_EDGE == sel->type) GridSel_add(sel, ed[e]);
	}
	Edge *facetedges[nb_edges_per_facet];
	for (unsigned f=0, fe=0; f<nb_facets; f++) {
		for (unsigned e=0; e<nb_edges_per_facet; e++, fe++) {
			facetedges[e] = ed[facets[fe]];
		}
		Facet *f = Grid_facet_new(nb_edges_per_facet, facetedges, true);
		if (sel && GridSel_FACET == sel->type) GridSel_add(sel, f);
	}
	return 1;
}

/* Public Functions */

#include <stdio.h>
int Grid_tetrahedron(unsigned name) {
#	define a .5
	static const float vertices[][3] = {
		{ a, a, a }, {-a, a,-a }, { a,-a,-a }, {-a,-a, a },
	};
	static const unsigned short edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 0 }, { 3, 0 }, { 3, 1 }, { 3, 2 },
	};
	static const unsigned short facets[] = {
		0,2,1, 5,2,3, 3,0,4, 4,1,5,
	};
#	undef a
	return add_from_geometry(name, sizeof(vertices)/sizeof(*vertices), vertices, sizeof(edges)/sizeof(*edges), edges, 4, 3, facets);
}

int Grid_cube(unsigned name) {
#	define a .5
	static const float vertices[][3] = {
		{-a,-a,-a }, { a,-a,-a } , { a, a,-a }, {-a, a,-a },
		{-a,-a, a }, { a,-a, a }, { a, a, a }, {-a, a, a },
	};
	static const unsigned short edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
		{ 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
	};
	static const unsigned short facets[] = {
		0,9,4,8, 1,10,5,9, 2,11,6,10, 3,8,7,11, 0,3,2,1, 4,5,6,7,
	};
#	undef a
	return add_from_geometry(name, sizeof(vertices)/sizeof(*vertices), vertices, sizeof(edges)/sizeof(*edges), edges, 6, 4, facets);
}

int Grid_octahedron(unsigned name) {
#	define a .5
	static const float vertices[][3] = {
		{ a, 0, 0 }, { 0, a, 0 }, { 0, 0, a }, {-a, 0, 0 },
		{ 0,-a, 0 }, { 0, 0,-a },
	};
	static const unsigned short edges[][2] = {
		{ 0, 1 }, { 1, 3 }, { 3, 4 }, { 4, 0 }, { 2, 0 }, { 2, 1 },
		{ 2, 3 }, { 2, 4 }, { 5, 0 }, { 5, 1 }, { 5, 3 }, { 5, 4 },
	};
	static const unsigned short facets[] = {
		0,5,4, 1,6,5, 2,7,6, 3,4,7, 0,8,9, 1,9,10, 2,10,11, 3,11,8,
	};
#	undef a
	return add_from_geometry(name, sizeof(vertices)/sizeof(*vertices), vertices, sizeof(edges)/sizeof(*edges), edges, 8, 3, facets);
}

int Grid_icosahedron(unsigned name) {
#	define a .5
#	define b .30901699437494742410
	static const float vertices[][3] = {
		{ 0, b,-a }, { b, a, 0 }, {-b, a, 0 }, { 0, b, a },
		{ 0,-b, a }, {-a, 0, b }, { 0,-b,-a }, { a, 0,-b },
		{ a, 0, b }, {-a, 0,-b }, { b,-a, 0 }, {-b,-a, 0 },
	};
	static const unsigned short edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 0, 2 }, { 1, 3 }, { 2, 3 }, { 3, 4 },
		{ 3, 5 }, { 4, 5 }, { 3, 8 }, { 4, 8 }, { 0, 6 }, { 0, 7 },
		{ 6, 7 }, { 0, 9 }, { 9, 6 }, { 4, 10 }, { 4, 11 }, { 10, 11 },
		{ 6, 11 }, { 6, 10 }, { 2, 5 }, { 2, 9 }, { 5, 9 }, { 11, 9 },
		{ 11, 5 }, { 1, 7 },  { 1, 8 }, { 7, 8 }, { 10, 8 }, { 10, 7 },
	};
	static const unsigned short facets[] = {
		2,1,0, 3,1,4, 6,7,5, 5,9,8, 11,12,10, 10,14,13, 16,17,15, 19,17,18, 21,22,20, 24,22,23,
		26,27,25, 29,27,28, 4,20,6, 8,26,3, 13,21,2, 0,25,11, 18,23,14, 12,29,19, 7,24,16, 15,28,9,
	};
#	undef b
#	undef a
	return add_from_geometry(name, sizeof(vertices)/sizeof(*vertices), vertices, sizeof(edges)/sizeof(*edges), edges, 20, 3, facets);
}

int Grid_dodecahedron(unsigned name) {
#	define a .19098300562505257590
#	define b .30901699437494742410
#	define c .5
	static const float vertices[][3] = {
		{ a, 0, c }, {-a, 0, c }, {-b, b, b }, { 0, c, a },
		{ b, b, b }, { b,-b, b }, { 0,-c, a }, {-b,-b, b },
		{ a, 0,-c }, {-a, 0,-c }, {-b,-b,-b }, { 0,-c,-a },
		{ b,-b,-b }, { b, b,-b }, { 0, c,-a }, {-b, b,-b },
		{ c, a, 0 }, {-c, a, 0 }, {-c,-a, 0 }, { c,-a, 0 },
	};
	static const unsigned short edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 0 }, { 0, 5 },
		{ 5, 6 }, { 6, 7 }, { 7, 1 }, { 8, 9 }, { 9, 10 }, { 10, 11 },
		{ 11, 12 }, { 12, 8 }, { 8, 13 }, { 13, 14 }, { 14, 15 }, { 15, 9 },
		{ 14, 3 }, { 4, 16 }, { 16, 13 }, { 15, 17 }, { 17, 2 }, { 11, 6 },
		{ 7, 18 }, { 18, 10 }, { 12, 19 }, { 19, 5 }, { 16, 19 }, { 17, 18 },
	};
	static const unsigned short facets[] = {
		4,3,2,1,0, 8,7,6,5,0, 13,12,11,10,9, 17,16,15,14,9, 18,3,19,20,15, 18,16,21,22,2,
		23,7,24,25,11, 23,12,26,27,6, 19,4,5,27,28, 26,13,14,20,28, 21,17,10,25,29, 24,8,1,22,29,
	};
#	undef c
#	undef b
#	undef a
	return add_from_geometry(name, sizeof(vertices)/sizeof(*vertices), vertices, sizeof(edges)/sizeof(*edges), edges, 12, 5, facets);
}

int Grid_triangle(unsigned name) {	// not a platonic solid, but ...
#	define a .86602540378443864676
	static const float vertices[][3] = {
		{ 1, 0, 0 }, {-.5, a, 0 } , {-.5,-a, 0 },
	};
	static const unsigned short edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 0 },
	};
	static const unsigned short facets[] = {
		0,1,2, 2,1,0,
	};
#	undef a
	return add_from_geometry(name, sizeof(vertices)/sizeof(*vertices), vertices, sizeof(edges)/sizeof(*edges), edges, 2, 3, facets);
}

int Grid_square(unsigned name) {	// not a platonic solid, neither ...
	static const float vertices[][3] = {
		{ 1, 1, 0 }, {-1, 1, 0 } , {-1,-1, 0 }, { 1,-1, 0},
	};
	static const unsigned short edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }
	};
	static const unsigned short facets[] = {
		0,1,2,3, 3,2,1,0,
	};
	return add_from_geometry(name, sizeof(vertices)/sizeof(*vertices), vertices, sizeof(edges)/sizeof(*edges), edges, 2, 4, facets);
}
