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
#include <float.h>
#include <math.h>
#include <libcnt/slist.h>
#include <libcnt/hash.h>
#include <libcnt/log.h>
#include "libmicromodel/grid.h"
#include "gridsel.h"

/*
 * Data Definitions
 */

struct elmnt {
	double ang_cost, dist2;
	Vertex *v1, *v2;
};
struct facet_n_vertex {
	Facet *f;
	Vertex *v;
};

/*
 * Private Functions
 */

static Edge *split_facet(Facet *facet, Vertex *v1, Vertex *v2) {
	assert(facet && v1 && v2 && v1!=v2);
	Edge *new_edge = Grid_edge_new(v1, v2);
	// allocate an empty facet
	Facet *new_facet = Grid_facet_new(0, NULL, true);
	Facet_split(facet, new_facet, new_edge);
	return new_edge;
}

static int comp(void *e1_, void *e2_) {
	struct elmnt *e1 = e1_, *e2 = e2_;
	if (e1->dist2 <= DBL_EPSILON) return -1;
	if (e2->dist2 <= DBL_EPSILON) return 1;
	double cost1 = e1->ang_cost + .2*sqrt(e1->dist2/e2->dist2);
	double cost2 = e2->ang_cost + .2*sqrt(e2->dist2/e1->dist2);
	if (cost1 <= cost2) return -1;
	return 1;
}

// tells weither segment ab intersect segment cd.
static bool intersect(const Vec *a, const Vec *b, const Vec *c, const Vec *d) {
	Vec ab, cd, ac;
	Vec_sub3(&ab, b, a);
	Vec_sub3(&cd, d, c);
	Vec_sub3(&ac, c, a);
	/* 3 usable equations :
	 * 0: alpha*ABx - beta*CDx = ACx
	 * 1: alpha*ABy - beta*CDy = ACy
	 * 2: alpha*ABz - beta*CDz = ACz
	 * that is : alpha*ab[i] - beta*cd[i] = ac[i]
	 * uses the 2 that have the biggest ab[i] for one and the biggest cd[i] for the other one
	 */
	int i1 = -1, i2 = -1, prev_i2 = -1;
	double max1 = 0, max2 = 0;
	for (int i=0; i<3; i++) {
		if (fabs(Vec_coord(&ab, i)) > max1) {
			max1 = fabs(Vec_coord(&ab, i));
			i1 = i;
		}
		if (fabs(Vec_coord(&cd, i)) > max2) {
			max2 = fabs(Vec_coord(&cd, i));
			prev_i2 = i2;
			i2 = i;
		}
	}
	if (i1 == i2) i2 = prev_i2;
	if (i1 == -1 || i2 == -1) return false;	// pathological cases (|ab|=0 or |cd|=0) does not intersect
	double ab_ratio = Vec_coord(&ab, i2)/Vec_coord(&ab, i1);
	double beta = (Vec_coord(&ac, i2) - Vec_coord(&ac, i1)*ab_ratio) / (Vec_coord(&cd, 1)*ab_ratio - Vec_coord(&cd, i2));
	if (!isfinite(beta) || beta < 0. || beta >  1.) return false;
	double alpha = (Vec_coord(&ac, i1) + beta*Vec_coord(&cd, i1)) / Vec_coord(&ab, i1);
	if (!isfinite(alpha) || alpha < 0. || alpha > 1.) return false;
	// check aligment
	int i3 = 0;
	while (i3 == i1 || i3 == i2) i3 ++;
	double check = alpha*Vec_coord(&ab, i3) - beta*Vec_coord(&cd, i3) - Vec_coord(&ac, i3);
	if (!isfinite(check) || fabs(check) > Vec_norm(&ab)*.05) return false;
	return true;
}

// tells weither segment [v1,v2] intersect any facet edges (except one using v1 or v2)
static bool facet_intersect(const Facet *facet, Vertex *v1, Vertex *v2) {
	assert(facet && v1 && v2 && v1 != v2);
	Vertex *v_prev, *v_next;
	const Vec *a = Vertex_position(v1);
	const Vec *b = Vertex_position(v2);
	unsigned fsize = Facet_size(facet);
	for (unsigned i=0; i<fsize; i++) {
		if (i) v_prev = v_next;
		else v_prev = Facet_get_vertex(facet, i);
		v_next = Facet_get_vertex(facet, i==fsize-1 ? 0:i+1);
		if (v_prev==v1 || v_next==v1 || v_prev==v2 || v_next==v2) continue;
		if (intersect(a, b, Vertex_position(v_prev), Vertex_position(v_next))) {
			return true;
		}
	}
	return false;
}

/*
 * Public Functions
 */

GridSel GridSel_connect(GridSel *sel, GridSel *restrict_to_facets, bool full_connect) {
	assert(sel && sel->type == GridSel_VERTEX);
	GridSel my_result;
	GridSel_construct(&my_result, GridSel_EDGE);
	// compute distances
	unsigned nb_vertices = GridSel_size(sel), total_vert_size = 0;
	cntSList *slist = cntSList_new(sizeof(struct elmnt), 1+nb_vertices*nb_vertices/8, comp);
	assert(slist);
	GridSel departures;
	GridSel_dup(&departures, sel);
	while (GridSel_size(&departures)) {
		GridSel arrivals;
		GridSel_dup(&arrivals, &departures);
		GridSel_reset(&arrivals);
		Vertex *p0 = GridSel_each(&arrivals);
		assert(p0);
		total_vert_size += Vertex_size(p0);
		// loop on all neighboring vertices of all neighboring facets
		for (unsigned fi = 0; fi < Vertex_size(p0); fi++) {
			Facet *f = Vertex_get_facet(p0, fi);
			if (restrict_to_facets && !GridSel_selected(restrict_to_facets, f)) continue;
			for (unsigned pi = 0; pi < Facet_size(f); pi++) {
				Vertex *p = Facet_get_vertex(f, pi);
				if (p == p0 || !GridSel_selected(&arrivals, p) || vertices_are_connected(p, p0)) continue;
				// Compute the cost of this cut
				double max_cos1 = Vertex_max_cos_to(p0, p, f);
				double max_cos2 = Vertex_max_cos_to(p, p0, f);
				if (max_cos2 > max_cos1) max_cos1 = max_cos2;
				if (max_cos1 >= 1.-DBL_EPSILON) continue;
				struct elmnt e = {
					-1./(max_cos1-1.-DBL_EPSILON)-1.,
					distance2_between_vertices(p, p0),
					p0, p
				};
				cntSList_insert(slist, &e);
				GridSel_remove(&arrivals, p);
			}
		}
		GridSel_remove(&departures, p0);
		GridSel_destruct(&arrivals);
	}
	GridSel_destruct(&departures);
	// Do cut
	cntHash *uniq_fv, *orig_facets;
	cntShelf *binkeys;
	if (! full_connect) {
		uniq_fv = cntHash_new(0, total_vert_size +1, 3, cntHash_BINKEYS, sizeof(struct facet_n_vertex));
		orig_facets = cntHash_new(sizeof(Facet *), nb_vertices, 3, cntHash_PTRKEYS, 0);
		binkeys = cntShelf_new(sizeof(struct facet_n_vertex), total_vert_size);
		assert(uniq_fv && orig_facets && binkeys);
	}
	while (cntSList_size(slist)) {
		struct elmnt *e = cntSList_get(slist, 0);
		assert(e);
		Facet *facet = vertices_are_connectable(e->v1, e->v2);
		if ( facet && !facet_intersect(facet, e->v1, e->v2)) {
			Facet **orig_facet;
			struct facet_n_vertex fv1, fv2;
			if (!full_connect) {
				orig_facet = cntHash_get(orig_facets, (cntHashkey){ .ptr=facet });
				if (!orig_facet) orig_facet = &facet;
				fv1.f = fv2.f = *orig_facet;
				fv1.v = e->v1;
				fv2.v = e->v2;
			}
			if (full_connect || (
				! cntHash_get(uniq_fv, (cntHashkey){ .bin=&fv1 }) &&
				! cntHash_get(uniq_fv, (cntHashkey){ .bin=&fv2 })
			)) {
				Edge *edge = split_facet(facet, e->v1, e->v2);
				Facet *new_facet = Edge_get_facet(edge, WEST);
				if (new_facet == facet) new_facet = Edge_get_facet(edge, EAST);
				GridSel_add(&my_result, edge);
				if (restrict_to_facets) {	// add new facet to the restriction
					GridSel_add(restrict_to_facets, new_facet);
				}
				if (!full_connect) {
					cntHash_put(orig_facets, (cntHashkey){ .ptr=new_facet }, orig_facet);
					void *key = cntShelf_put(binkeys, &fv1);
					cntHash_put(uniq_fv, (cntHashkey){ .bin=key }, NULL);
					key = cntShelf_put(binkeys, &fv2);
					cntHash_put(uniq_fv, (cntHashkey){ .bin=key }, NULL);
				}
			}
		}
		cntSList_remove(slist, 0);
	}
	if (!full_connect) {
		cntShelf_del(binkeys);
		cntHash_del(orig_facets);
		cntHash_del(uniq_fv);
	}
	cntSList_del(slist);
	return my_result;
}


