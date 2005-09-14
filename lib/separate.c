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
#include <assert.h>
#include <libcnt/log.h>
#include "gridsel.h"

/* Private Functions */

static void separate_loop(Edge **loop, unsigned loop_size, GridSel *new_facets) {
	assert(loop && loop_size>=3 && new_facets);
	Edge *coloop[loop_size];
	Vertex *last_v[NB_POLES] = { NULL }, *last_cov[NB_POLES] = { NULL }, *very_first_v = NULL, *very_first_cov = NULL;
	EdgeSide sep_side = EAST;	// separate the EAST of the first edge
	bool loop_is_direct = true;
	for (unsigned e=0; e<loop_size; e++) {
		Vertex *v[NB_POLES];
		Vertex *cov[NB_POLES];
		EdgePole first_pole = NB_POLES;
		for (EdgePole pole = 0; pole < NB_POLES; pole++) {
			v[pole] = Edge_get_vertex(loop[e], pole);
			static const EdgePole possible_matches[3] = { NB_POLES, NORTH, SOUTH };
			const unsigned match_i = (v[pole]==last_v[SOUTH])*2 + (v[pole]==last_v[NORTH]);
			assert(match_i<sizeof(possible_matches)/sizeof(*possible_matches));
			const EdgePole match_pole = possible_matches[match_i];
			if (match_pole != NB_POLES) {
				cov[pole] = last_cov[match_pole];
				first_pole = pole;
				if (match_pole == pole) {	// inversion of the direction of successive edges
					sep_side = !sep_side;	// so, I separate from the other side
				}
				if (e == 1) {
					if (match_pole == NORTH) loop_is_direct = false;
					very_first_cov = last_cov[!match_pole];
					very_first_v = last_v[!match_pole];
				}
			} else {	// new vertex. when e==0, both vertices are new, and so we first separate from EAST.
				if (e < loop_size-1) {
					cov[pole] = Grid_vertex_new(Vertex_position(v[pole]), Vertex_basis(v[pole]),Vertex_skin_ratio(v[pole]),Vertex_uv_x(v[pole]),Vertex_uv_y(v[pole]));	// TODO : faire un Vertex_dup, Grid_vertex_dup ?
				} else {	// for last edge, we must loop rather than creating a new edge
					assert(very_first_cov);
					cov[pole] = very_first_cov;
					v[pole] = very_first_v;
				}
			}
		}
		Facet *f = Edge_get_facet(loop[e], sep_side);
		coloop[e] = Grid_edge_new(cov[SOUTH], cov[NORTH]);
		Facet_change_edge(f, loop[e], coloop[e], 0);
		if (e > 0) {
			assert(first_pole != NB_POLES);
			if ((first_pole == SOUTH && sep_side == EAST) || (first_pole == NORTH && sep_side == WEST)) {
				Vertex_move_connections(v[first_pole], cov[first_pole], loop[e-1], loop[e]);
				if (e == loop_size-1) {
					Vertex_move_connections(v[!first_pole], cov[!first_pole], loop[e], loop[0]);
				}
			} else {
				Vertex_move_connections(v[first_pole], cov[first_pole], loop[e], loop[e-1]);
				if (e == loop_size-1) {
					Vertex_move_connections(v[!first_pole], cov[!first_pole], loop[0], loop[e]);
				}
			}
		}
		for (EdgePole pole = 0; pole < NB_POLES; pole++) {
			last_v[pole] = v[pole];
			last_cov[pole] = cov[pole];
		}
	}
	Facet *f1 = Grid_facet_new(loop_size, loop, loop_is_direct);
	Facet *f2 = Grid_facet_new(loop_size, coloop, ! loop_is_direct);
	assert(f1 && f2);
	GridSel_add(new_facets, f1);
	GridSel_add(new_facets, f2);
}

/* Public Functions */

GridSel GridSel_separate(GridSel *this) {
	assert(this);
	GridSel my_result;
	GridSel_construct(&my_result, GridSel_FACET);
	if (this->type != GridSel_EDGE) return my_result;
	while (GridSel_size(this)) {
		Edge *loop[GridSel_size(this)];
		unsigned loop_size = 0;
		Edge *edge;
		bool again;
		do {
			again = false;
			GridSel_reset(this);
			while ( (edge = GridSel_each(this)) ) {
				if (!loop_size || edges_are_connected(edge, loop[loop_size-1])) {
					again = true;
					GridSel_remove(this, edge);
					loop[loop_size++] = edge;
					if (loop_size>2 && edges_are_connected(loop[0], loop[loop_size-1])) {
						separate_loop(loop, loop_size, &my_result);
						goto next_loop;
					}
				}
			}
		} while (again);
next_loop: ;
	}
	log_warning(LOG_DEBUG, "Returning %u facets from separation", GridSel_size(&my_result));
	return my_result;
}

