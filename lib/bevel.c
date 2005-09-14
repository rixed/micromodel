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
#include <stdlib.h>
#include <assert.h>
#include <libcnt/vec.h>
#include <libcnt/log.h>
#include "libmicromodel/grid.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/edge.h"
#include "libmicromodel/vertex.h"
#include "gridsel.h"

/*
 * Data Definitions
 */

typedef struct {
	Edge *coedges[NB_SIDES];	// WEST of original edge, EAST of original edge
	Facet *cofacet;
} Coedges;

/*
 * Private Functions
 */

static Edge *bevel_new_edge(Vertex *south, Vertex *north) {
	assert(south && north);
	return Grid_edge_new(south, north);
}

static Facet *bevel_new_facet(unsigned nb_edges, Edge **edges) {
	return Grid_facet_new(nb_edges, edges, true);	// will signal to edges, wich will signal to vertex when fully qualified by 2 facets
}

static Vertex *bevel_get_covertex(Vertex *v, Edge *e, int dep, Vertex **covertices) {
	assert(v && e && covertices);
	int i;
	for (i=0; i<(signed)Vertex_size(v); i++) {
		if (Vertex_get_edge(v, i) == e) break;
	}
	assert(i<(signed)Vertex_size(v));
	i += dep;
	if (i<0) i += Vertex_size(v);
	else if (i>=(signed)Vertex_size(v)) i -= Vertex_size(v);
	if (covertices[i]) return covertices[i];
	if (dep>=0) return covertices[(i+1)%Vertex_size(v)];
	assert(0);
	return NULL;
}

static Edge *bevel_get_vertices_coedge(Vertex *v, Edge *e, Edge **coedges) {
	// le coedge N est celui qui arrive sur le vertex N, sauf si la cofacet du vertex est plate, auquel cas le second seg est une copie du premier
	// mais dans ce cas, on retourne donc forcément le bon coedge...
	assert(v && e && coedges);
	int i;
	for (i=0; i<(signed)Vertex_size(v); i++) {
		if (Vertex_get_edge(v, i) == e) break;
	}
	assert(i<(signed)Vertex_size(v));
	if (coedges[i]) return coedges[i];
	return coedges[(i+1)%(int)Vertex_size(v)];
}

static void bevel_facet_replace_edge(Facet *facet, Edge *old, Coedges *new) {
	if (! new->cofacet) return;	// some beveled edges are not actually bevelled (isolated single edge with vertices of size 2)
	Facet_change_edge(facet, old, new->coedges[Facet_my_side(facet, old)], 0);
}

static void bevel_facet_split(Facet *facet, unsigned idx, Vertex *vertex, Edge **coedges, Vertex **covertices) {
	Edge *previous = Facet_get_edge(facet, idx>0 ? idx-1 : Facet_size(facet)-1);
	unsigned prev_coi, next_coi;
	for (prev_coi=0; prev_coi<Vertex_size(vertex); prev_coi++) {
		if (Vertex_get_edge(vertex, prev_coi) == previous) break;
	}
	assert(prev_coi<Vertex_size(vertex));
	next_coi = (prev_coi > 0 ? prev_coi:Vertex_size(vertex)) - 1;	// Oui, le segment suivant dans la face précède celui là dans les connexions du vertex...
	// Normalement, la facet est toujours à l'est du coedge ; sauf dans le cas particulier où il n'y à que deux segments dans le covertex : le second référence alors le premier
	assert(coedges[prev_coi]->facets[covertices[prev_coi]==Edge_get_vertex(coedges[prev_coi], NORTH) ? EAST:WEST] == NULL);
	Edge_add_facet(coedges[prev_coi], facet, covertices[prev_coi]==Edge_get_vertex(coedges[prev_coi], NORTH) ? EAST:WEST);
	Facet_add_edge_next(facet, previous, coedges[prev_coi]);
}

static void bevel_edge_change_vertex(Edge *edge, EdgePole pole, Vertex *new) {
	Edge_set_vertex(edge, pole, new);
	Vertex_add_edge(new, edge);
}

/*
 * Public Functions
 */

GridSel GridSel_bevel(GridSel *beveled_edges, GridSel *additionnal_vertices, double ratio) {
	/* Algorithm :
	 * for each beveled vertex,
	 * 	for each connection,
	 * 		if the connection is not a beveled edge,
	 * 			cut the edge with a co-vertex (dos not actually cut existing edge, juste position the new vertices as if)
	 * 		else, if the next edge is beveled also,
	 * 			add a co-vertex as the sum of the 2 co-vertices that would have been created if we were cutting beveled edges
	 * 	build a facet with thoses co-vertices (new edges, not related to existing facets)
	 * for each beveled edges,
	 * 	build two co-edges with the corresponding co-vertex of each pole of the edge,
	 * 	build a facet with thoses co-edges and edges from facets build from vertices (related to each others)
	 * for each beveled facets,
	 * 	for each vertex of the facet,
	 * 		if the vertex is beveled, but none of the corresponding edges are,
	 * 			change the termination of the 2 edges so that they end on the co-vertices, and insert the co-edge betwee the two. unlink the beveled vertex from thoses 2 edges (delete it if it does not hold any connection left)
	 * 		else
	 * 			replace the beveled edge by the corresponding co-edge, unlinking cutted edges.
	 */
	GridSel my_result;
	GridSel_construct(&my_result, GridSel_FACET);
	GridSel beveled_facets, beveled_vertices;
	GridSel_convert(beveled_edges, &beveled_vertices, GridSel_VERTEX, GridSel_MIN);
	if (additionnal_vertices) GridSel_add_or_sub(&beveled_vertices, additionnal_vertices, 1);
	GridSel_convert(&beveled_vertices, &beveled_facets, GridSel_FACET, GridSel_MAX);
	
	Vertex *v;
	cntHash *covertices = NULL, *coedges = NULL;
	//
	// Build covertices
	//
	// get max number of covertices
	unsigned max_nb_covertices = 0;
	GridSel_reset(&beveled_vertices);
	while ( (v=GridSel_each(&beveled_vertices)) ) {
		unsigned size = Vertex_size(v);
		if (size > max_nb_covertices) max_nb_covertices = size;
	}
	typedef struct {
		Facet *cofacet;
		Vertex *covertices[max_nb_covertices];
		Edge *coedges[max_nb_covertices];
	} Covertices;
	covertices = cntHash_new(sizeof(Covertices), GridSel_size(&beveled_vertices)+1, 3, cntHash_PTRKEYS, 0);
	assert(covertices);

	GridSel_reset(&beveled_vertices);
	while ( (v=GridSel_each(&beveled_vertices)) ) {
		unsigned v_size = Vertex_size(v);
		Covertices *cov = cntHash_put(covertices, (cntHashkey){ .ptr = v }, NULL);
		assert(cov);
		unsigned nb_covs = 0;
		for (unsigned i=0; i<v_size; i++) {
			Edge *next_e, *e = Vertex_get_edge(v, i);
			Vertex *v_l = Edge_get_vertex(e, SOUTH);
			if (v_l == v) v_l = Edge_get_vertex(e, NORTH);
			if (!GridSel_selected(beveled_edges, e)) {
				cov->covertices[i] = Grid_vertex_average_new(v, v_l, ratio);
			} else if (GridSel_selected(beveled_edges, next_e=Vertex_get_edge(v, (i+1)%(int)v_size))) {
				Vertex *next_v_l = Edge_get_vertex(next_e, SOUTH);
				if (next_v_l == v) next_v_l = Edge_get_vertex(next_e, NORTH);
				Vertex temp;
				Vertex_construct_average(&temp, 0, v_l, next_v_l, 0.5);	// FIXME: si v_l, v et next_v_l sont alignés, c'est pas génial (meme si topologiquement c'est OK).
				cov->covertices[i] = Grid_vertex_average_new(v, &temp, 2.*ratio);
			} else {
				cov->covertices[i] = NULL;
				continue;
			}
			nb_covs ++;
		}
		// build coedges
		Vertex *first_cov = NULL, *last_cov = NULL;
		unsigned last_i, first_i;
		for (unsigned i=0; i<v_size; i++) {
			Vertex *this_cov = cov->covertices[i];
			cov->coedges[i] = NULL;
			if (this_cov) {
				if (!first_cov) {
					first_cov = this_cov;
					first_i = i;	// can be 0 or 1
				}
				if (last_cov) {
					cov->coedges[i] = bevel_new_edge(last_cov, this_cov);
					last_i = i;
				}
				last_cov = this_cov;
			}
		}
		assert(last_cov && first_cov);
		if (first_cov != last_cov) {
			cov->coedges[first_i] = nb_covs>2 ? bevel_new_edge(last_cov, first_cov) : cov->coedges[last_i];
		}
		// build vertex cofacet
		cov->cofacet = NULL;
		if (nb_covs>2) {
			Edge *edges[nb_covs];
			unsigned e_idx = 0;
			for (unsigned i=0; i<v_size; i++) {
				if (cov->coedges[i]) edges[e_idx++] = cov->coedges[i];
			}
			assert(e_idx == nb_covs);
			cov->cofacet = bevel_new_facet(nb_covs, edges);
			GridSel_add(&my_result, cov->cofacet);
		}
	}
	//
	// Build Coedges
	//
	coedges = cntHash_new(sizeof(Coedges), GridSel_size(beveled_edges)+1, 3, cntHash_PTRKEYS, 0);
	if (!coedges) goto free_n_quit;
	GridSel_reset(beveled_edges);
	Edge *e;
	while ( (e=GridSel_each(beveled_edges)) ) {
		Coedges *coe = cntHash_put(coedges, (cntHashkey){ .ptr = e }, NULL);
		Vertex *v[NB_POLES] = { Edge_get_vertex(e, SOUTH), Edge_get_vertex(e, NORTH) };
		Covertices *south_cov = cntHash_get(covertices, (cntHashkey){ .ptr=v[SOUTH] });
		Covertices *north_cov = cntHash_get(covertices, (cntHashkey){ .ptr=v[NORTH] });
		Vertex *covSW = bevel_get_covertex(v[SOUTH], e, +0, south_cov->covertices);
		Vertex *covNW = bevel_get_covertex(v[NORTH], e, -1, north_cov->covertices);
		Vertex *covSE = bevel_get_covertex(v[SOUTH], e, -1, south_cov->covertices);
		Vertex *covNE = bevel_get_covertex(v[NORTH], e, +0, north_cov->covertices);
		if (covSW != covSE || covNW != covNE) {
			coe->coedges[WEST] = bevel_new_edge(covSW, covNW);
			coe->coedges[EAST] = bevel_new_edge(covSE, covNE);
			Edge *edges[4];
			unsigned nb_edges = 0;
			edges[nb_edges++] = coe->coedges[WEST];
			Edge *fe = bevel_get_vertices_coedge(v[SOUTH], e, south_cov->coedges);
			if (fe) edges[nb_edges++] = fe;
			edges[nb_edges++] = coe->coedges[EAST];
			fe = bevel_get_vertices_coedge(v[NORTH], e, north_cov->coedges);
			if (fe) edges[nb_edges++] = fe;
			coe->cofacet = bevel_new_facet(nb_edges, edges);
			GridSel_add(&my_result, coe->cofacet);
		} else {	// This isolated edge cannot be beveled.
			// Destroy the coedge alltogether
			cntHash_remove(coedges, (cntHashkey){ .ptr = e });
			// The same goes for the covertices
			cntHash_remove(covertices, (cntHashkey){ .ptr = v[SOUTH] });
			cntHash_remove(covertices, (cntHashkey){ .ptr = v[NORTH] });
			// Destroy the vertices that will never be linked to anything
			Grid_replace_vertex(covSW, NULL);
			Grid_replace_vertex(covNW, NULL);
		}
	}
	GridSel_destruct(&beveled_vertices);
	//
	// Build Cofacets
	//
	Facet *f;
	GridSel_reset(&beveled_facets);
	while ( (f=GridSel_each(&beveled_facets)) ) {
		Edge *left_e = Facet_get_edge(f, Facet_size(f)-1);
		Coedges *left_coe = cntHash_get(coedges, (cntHashkey){ .ptr = left_e });
		for (unsigned i=0; i<Facet_size(f); i++) {
			Edge *right_e = Facet_get_edge(f, i);
			Coedges *right_coe = cntHash_get(coedges, (cntHashkey){ .ptr = right_e });
			Vertex *orig_v = Facet_get_vertex(f, i);
			Covertices *cov = cntHash_get(covertices, (cntHashkey){ .ptr = orig_v });
			if (left_coe) {
				bevel_facet_replace_edge(f, left_e, left_coe);
			} else if (cov && !right_coe) {
				bevel_facet_split(f, i, orig_v, cov->coedges, cov->covertices);
				if (i) i++;	// we just added an edge after previous
			}
			left_e = right_e;
			left_coe = right_coe;
		}
	}
	GridSel_reset(&beveled_facets);
	while ( (f=GridSel_each(&beveled_facets)) ) {
		Edge *left_e = Facet_get_edge(f, Facet_size(f)-1);
		EdgePole left_pole = Facet_my_side(f, left_e)==WEST ? NORTH:SOUTH;
		for (unsigned i=0; i<Facet_size(f); i++) {
			Edge *right_e = Facet_get_edge(f, i);
			EdgePole right_pole = Facet_my_side(f, right_e)==WEST ? SOUTH:NORTH;
			Vertex *left_v = Edge_get_vertex(left_e, left_pole);
			Vertex *right_v = Edge_get_vertex(right_e, right_pole);
			if (left_v != right_v) {
				if (cntHash_get(covertices, (cntHashkey){ .ptr = left_v })) {
					bevel_edge_change_vertex(left_e, left_pole, right_v);
				} else {
					assert(cntHash_get(covertices, (cntHashkey){ .ptr = right_v }));
					bevel_edge_change_vertex(right_e, right_pole, left_v);
				}
			}
			left_e = right_e;
			left_pole = ! right_pole;
		}
	}
	//
	// Remove pending vertices
	//
	cntHash_reset(covertices);
	cntHashkey key;
	while (cntHash_each(covertices, &key, NULL)) {
		Grid_replace_vertex(key.ptr, NULL);
	}
	cntHash_reset(coedges);
	while (cntHash_each(coedges, &key, NULL)) {
		Grid_replace_edge(key.ptr, NULL);
	}
free_n_quit:
	if (covertices) {
		cntHash_del(covertices);
	}
	if (coedges) {
		cntHash_del(coedges);
	}
	GridSel_destruct(&beveled_facets);
	return my_result;
}

// vi:ts=3:sw=3
