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
#include "../config.h"
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <libcnt/cnt.h>
#include <libcnt/hash.h>
#include <libcnt/list.h>
#include <libcnt/vec_i.h>
#include <libcnt/log.h>
#include "libmicromodel/grid.h"
#include "libmicromodel/vertex.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/edge.h"
#include "libmicromodel/basis.h"
#include "libmicromodel/color.h"
#include "gridsel.h"
#include "grid.h"

#define GRID_VERSION 0

/* Data Definitions */

struct Grid {
	cntHash *selections;	// clefs unsigned, pour les selections, valeurs = GridSel
	cntHash *vertices;	// clefs = unsigned, valeurs = Vertex
	cntHash *edges;
	cntHash *facets;
	cntHash *bases;
	cntHash *colors;
	unsigned next_vertex, next_edge, next_facet, next_basis;
};

static Grid *this_grid = NULL;
static unsigned carac_size = 2000;

/* Private Functions */

static void Grid_destruct(void) {
	assert(this_grid);
	void *ptr;
	if (this_grid->selections) {
		cntHash_reset(this_grid->selections);
		cntHashkey key;
		while (cntHash_each(this_grid->selections, &key, NULL) ) {
			Grid_del_selection(key.i);
		}
		cntHash_del(this_grid->selections);
	}
	if (this_grid->vertices) {
		cntHash_reset(this_grid->vertices);
		while (cntHash_each(this_grid->vertices, NULL, &ptr)) {
			Vertex_destruct(ptr);
		}
		cntHash_del(this_grid->vertices);
	}
	if (this_grid->edges) {
		cntHash_reset(this_grid->edges);
		while (cntHash_each(this_grid->edges, NULL, &ptr)) {
			Edge_destruct(ptr);
		}
		cntHash_del(this_grid->edges);
	}
	if (this_grid->facets) {
		cntHash_reset(this_grid->facets);
		while (cntHash_each(this_grid->facets, NULL, &ptr)) {
			Facet_destruct(ptr);
		}
		cntHash_del(this_grid->facets);
	}
	if (this_grid->bases) {
		cntHash_reset(this_grid->bases);
		while (cntHash_each(this_grid->bases, NULL, &ptr)) {
			Basis_destruct(ptr);
		}
		cntHash_del(this_grid->bases);
	}
	if (this_grid->colors) {
		cntHash_reset(this_grid->colors);
		while (cntHash_each(this_grid->colors, NULL, &ptr)) {
			Color_destruct(ptr);
		}
		cntHash_del(this_grid->colors);
	}
}

static int Grid_construct(void) {	// build an empty (invalid) grid
	assert(this_grid);
	this_grid->selections = this_grid->vertices = this_grid->edges = this_grid->facets = NULL;
	this_grid->next_vertex = this_grid->next_edge = this_grid->next_facet = 0;
	this_grid->selections = cntHash_new(sizeof(GridSel), 50, 1, cntHash_INTKEYS, 0);
	if (! this_grid->selections) goto fail;
	this_grid->vertices = cntHash_new(sizeof(Vertex), Grid_get_carac_size(), 3, cntHash_INTKEYS, 0);
	if (! this_grid->vertices) goto fail;
	this_grid->edges = cntHash_new(sizeof(Edge), Grid_get_carac_size(), 3, cntHash_INTKEYS, 0);
	if (! this_grid->edges) goto fail;
	this_grid->facets = cntHash_new(sizeof(Facet), Grid_get_carac_size(), 3, cntHash_INTKEYS, 0);
	if (! this_grid->facets) goto fail;
	this_grid->bases = cntHash_new(sizeof(Basis), 25, 3, cntHash_INTKEYS, 0);
	if (! this_grid->bases) goto fail;
	this_grid->colors = cntHash_new(sizeof(Color), 35, 3, cntHash_INTKEYS, 0);
	if (! this_grid->colors) goto fail;
	return 1;
fail:
	Grid_destruct();
	return 0;
}

static void *get_elmnt_from_index(GridSel_type type, unsigned index) {
	assert(this_grid);
	switch (type) {
		// cannot use a table of function pointer because of return type that
		// are not required to be hardware compatible with void *... :-(
		case GridSel_VERTEX:
			return Grid_get_vertex(index);
		case GridSel_EDGE:
			return Grid_get_edge(index);
		case GridSel_FACET:
			return Grid_get_facet(index);
		default:
			assert(0);
	}
	return NULL;	// avoir warning
}

static int add_or_sub_two_selections(unsigned name_dest, unsigned name_src, int add) {
	assert(this_grid);
	if (0==name_dest || 0==name_src) return 1;
	GridSel *sel_src = Grid_get_selection(name_src);
	GridSel *sel_dest = Grid_get_selection(name_dest);
	assert (sel_src && sel_dest);
	GridSel tmp; bool tmp_used = false;
	if (sel_src->type != sel_dest->type) {
		GridSel_convert(sel_src, &tmp, sel_dest->type, GridSel_MIN);
		tmp_used = true;
		sel_src = &tmp;
	}
	GridSel_add_or_sub(sel_dest, sel_src, add);
	if (tmp_used) GridSel_destruct(&tmp);
	return 1;
}

/*
 * Protected Functions
 */

GridSel *Grid_new_selection_(unsigned name, GridSel_type type) {
	assert(this_grid && name>0 && (type==GridSel_VERTEX || type==GridSel_EDGE || type==GridSel_FACET));
	cntHashkey key = { .i = name };
	if (cntHash_get(this_grid->selections, key)) {
		return NULL;
	}
	GridSel *sel = cntHash_put(this_grid->selections, key, NULL);
	assert(sel);
	if (! GridSel_construct(sel, type)) {
		cntHash_remove(this_grid->selections, key);
		return NULL;
	}
	return sel;
}

void output_selection(unsigned selection, GridSel *sel, unsigned result_selection, GridSel *my_result) {
	assert(selection>0 && sel && my_result);
	if (! result_selection) {
		GridSel_destruct(my_result);
	} else if (result_selection == selection) {
		GridSel_destruct(sel);
		*sel = *my_result;
	} else {
		GridSel *res = Grid_get_selection(result_selection);
		if (! res) {
			res = Grid_new_selection_(result_selection, my_result->type);
			assert(res);
		}
		GridSel_destruct(res);
		*res = *my_result;
	}
}

GridSel *Grid_get_selection(unsigned name) {
	assert(this_grid && name>0);
	return cntHash_get(this_grid->selections, (cntHashkey){ .i = name });
}

/*
 * Public Functions
 */

int Grid_new(void) {	// alloc an empty (invalid) Grid
	this_grid = mem_alloc(sizeof(*this_grid));
	if (! this_grid) return 0;
	if (! Grid_construct()) {
		mem_unregister(this_grid);
		return 0;
	}
	return 1;
}

void Grid_set(Grid *this) {
	assert(this);
	this_grid = this;
}
Grid *Grid_get(void) {
	return this_grid;
}
unsigned Grid_get_version(void) {
	return GRID_VERSION;
}
void Grid_set_carac_size(unsigned new_size) {
	carac_size = new_size;
}
unsigned Grid_get_carac_size(void) {
	return carac_size;
}

Vertex *Grid_vertex_new(const Vec *position, unsigned bi, float ratio, float uv_x, float uv_y) {
	// Add an unlinked vertex, return its number
	assert(this_grid && position && (bi>0 || ratio==0.));
	Vertex *v = cntHash_put(this_grid->vertices, (cntHashkey){ .i = this_grid->next_vertex }, NULL);
	assert(v);
	if (! Vertex_construct(v, this_grid->next_vertex, position, bi, ratio, uv_x, uv_y)) return NULL;
	this_grid->next_vertex ++;
	return v;
}
Vertex *Grid_vertex_average_new(const Vertex *v1, const Vertex *v2, double ratio) {
	assert(v1 && v2);
	Vertex *v = cntHash_put(this_grid->vertices, (cntHashkey){ .i = this_grid->next_vertex }, NULL);
	assert(v);
	if (! Vertex_construct_average(v, this_grid->next_vertex, v1, v2, ratio)) return NULL;
	this_grid->next_vertex ++;
	return v;
}
Edge *Grid_edge_new(Vertex *v1, Vertex *v2) {
	// Add an edge between two vertex. No facets are updated
	assert(this_grid && v1 && v2);
	Edge *e = cntHash_put(this_grid->edges, (cntHashkey){ .i = this_grid->next_edge }, NULL);
	assert(e);
	Edge_construct(e, this_grid->next_edge, v1, v2);
	this_grid->next_edge ++;
	return e;
}
Facet *Grid_facet_new(unsigned size, Edge **edges, bool direct) {
	// Add a facet, using given edges
	assert(this_grid);
	Facet *f = cntHash_put(this_grid->facets, (cntHashkey){ .i = this_grid->next_facet }, NULL);
	assert(f);
	Facet_construct(f, this_grid->next_facet, size, edges, direct);
	this_grid->next_facet ++;
	return f;
}

void Grid_del(void) {
	assert(this_grid);
	Grid_destruct();
	mem_unregister(this_grid);
	this_grid = NULL;
}
void Grid_size(unsigned *nb_vertices, unsigned *nb_edges, unsigned *nb_facets) {
	if (nb_vertices) *nb_vertices = this_grid ? cntHash_size(this_grid->vertices) : 0;
	if (nb_edges) *nb_edges = this_grid ? cntHash_size(this_grid->edges) : 0;
	if (nb_facets) *nb_facets = this_grid ? cntHash_size(this_grid->facets) : 0;
}

void Grid_replace_vertex(Vertex *v, Vertex *rep) {
	assert(this_grid && v);
	Vertex_destruct(v);
	int ret = cntHash_remove_by_address(this_grid->vertices, v);
	assert(ret);
	Grid_replace_in_selections(GridSel_VERTEX, v, rep);
}

void Grid_replace_edge(Edge *e, Edge *rep) {
	assert(this_grid && e);
	Edge_destruct(e);
	int ret = cntHash_remove_by_address(this_grid->edges, e);
	assert(ret);
	Grid_replace_in_selections(GridSel_EDGE, e, rep);
}

void Grid_replace_facet(Facet *f, Vertex *rep) {
	assert(this_grid && f);
	Facet_destruct(f);
	int ret = cntHash_remove_by_address(this_grid->facets, f);
	assert(ret);
	Grid_replace_in_selections(GridSel_FACET, f, rep);
}

Edge *Grid_edge_cut(Edge *edge, double ratio) {
	// edge stay the south part ; return the new edge, at north.
	if (! this_grid) return NULL;
	assert(edge);
	Vertex *vS = Edge_get_vertex(edge, SOUTH);
	Vertex *vN = Edge_get_vertex(edge, NORTH);
	Vertex *vi = Grid_vertex_average_new(vS, vN, ratio);
	assert(vi);
	Edge *new = Grid_edge_new(vi, vN);
	assert(new);
	Edge_cut(edge, new, vi);
	return new;
}

/* Homotetic Functions */

static void scale(Vec *pos, Vec *axis, double ratio) {
	Vec_scale(pos, ratio);
}
static void stretch(Vec *pos, Vec *axis, double ratio) {
	double d = Vec_scalar(pos, axis);
	Vec disp = *axis;
	Vec_scale(&disp, d*(ratio - 1));
	Vec_add(pos, &disp);
}
static void shear(Vec *pos, Vec *axis, double ratio) {
	double d = Vec_scalar(pos, axis);
	Vec a = *axis;
	Vec_scale(&a, d);
	Vec b;
	Vec_sub3(&b, pos, &a);	// b and axis are perpendicullar
	Vec_normalize(&b);
	double e = Vec_scalar(pos, &b);
	Vec disp = *axis;
	Vec_scale(&disp, e*ratio);
	Vec_add(pos, &disp);
}
static void rotate(Vec *pos, Vec *axis, double angle) {
	Vec m0, m1, m2;
	Vec new_pos;
	double c = cos(angle);
	double s = sin(angle);
	Vec_construct(&m0,
		c + (1-c)*Vec_coord(axis, 0)*Vec_coord(axis, 0),
		(1-c)*Vec_coord(axis, 0)*Vec_coord(axis, 1) - s*Vec_coord(axis, 2),
		(1-c)*Vec_coord(axis, 0)*Vec_coord(axis, 2) + s*Vec_coord(axis, 1)
	);
	Vec_construct(&m1,
		(1-c)*Vec_coord(axis, 0)*Vec_coord(axis, 1) + s*Vec_coord(axis, 2),
		c + (1-c)*Vec_coord(axis, 1)*Vec_coord(axis, 1),
		(1-c)*Vec_coord(axis, 1)*Vec_coord(axis, 2) - s*Vec_coord(axis, 0)
	);
	Vec_construct(&m2,
		(1-c)*Vec_coord(axis, 0)*Vec_coord(axis, 2) - s*Vec_coord(axis, 1),
		(1-c)*Vec_coord(axis, 1)*Vec_coord(axis, 2) + s*Vec_coord(axis, 0),
		c + (1-c)*Vec_coord(axis, 2)*Vec_coord(axis, 2)
	);
	Vec_construct(&new_pos,
		Vec_scalar(&m0, pos),
		Vec_scalar(&m1, pos),
		Vec_scalar(&m2, pos)
	);
	Vec_destruct(pos);
	*pos = new_pos;
	Vec_destruct(&m0);
	Vec_destruct(&m1);
	Vec_destruct(&m2);
}
static void translate(Vec *pos, Vec *axis, double ratio) {
	Vec_add(pos, axis);
}

static void apply_homotecy(unsigned selection, Vec *center, Vec *axis, double ratio, void (*homotecy)(Vec *, Vec *, double)) {
	assert(this_grid && homotecy);
	if (! selection) return;
	GridSel *sel = Grid_get_selection(selection);
	assert(sel);
	GridSel_apply_homotecy(sel, center, axis, ratio, homotecy);
}

void Grid_scale(unsigned selection, Vec *center, double ratio) {
	assert(center);
	apply_homotecy(selection, center, NULL, ratio, scale);
}

void Grid_stretch(unsigned selection, Vec *center, Vec *axis, double ratio) {
	assert(center && axis);
	Vec norm_axis = *axis;
	Vec_normalize(&norm_axis);
	apply_homotecy(selection, center, &norm_axis, ratio, stretch);
}

void Grid_shear(unsigned selection, Vec *center, Vec *axis, double ratio) {
	assert(center && axis);
	Vec norm_axis = *axis;
	Vec_normalize(&norm_axis);
	apply_homotecy(selection, center, &norm_axis, ratio, shear);
}

void Grid_rotate(unsigned selection, Vec *center, Vec *axis, double angle) {
	assert(center && axis);
	Vec norm_axis = *axis;
	Vec_normalize(&norm_axis);
	apply_homotecy(selection, center, &norm_axis, angle, rotate);
}

void Grid_translate(unsigned selection, Vec *disp, double ratio) {
	assert(disp);
	Vec long_disp = *disp;
	Vec_scale(&long_disp, ratio);
	apply_homotecy(selection, NULL, &long_disp, ratio, translate);
}

/* Building Functions */

int Grid_extrude(unsigned name, bool dir_vertex, Vec *direction, double ratio, unsigned result_selection) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	GridSel my_result = GridSel_extrude(sel, dir_vertex, direction, ratio);
	output_selection(name, sel, result_selection, &my_result);
	return 1;
}

int Grid_extrude_1by1(unsigned name, bool dir_vertex, Vec *direction, double ratio, double scale_ratio, unsigned result_selection) {
	// meme chose, mais sur une selection qui fait une face par une face
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel && sel->type==GridSel_FACET);
	GridSel my_result, single_facet;
	GridSel_construct(&my_result, GridSel_FACET);
	GridSel_construct(&single_facet, GridSel_FACET);
	GridSel_reset(sel);
	Facet *facet;
	while ( (facet=GridSel_each(sel)) ) {
		GridSel_clear(&single_facet);
		GridSel_add(&single_facet, facet);
		GridSel this_result = GridSel_extrude(&single_facet, dir_vertex, direction, ratio);
		GridSel_add_or_sub(&my_result, &this_result, true);
		if (scale_ratio != 1.) {
			Vec center;
			GridSel_add(&single_facet, facet);
			GridSel_center(&single_facet, &center);
			GridSel_apply_homotecy(&single_facet, &center, NULL, scale_ratio, scale);
			Vec_destruct(&center);
		}
		GridSel_destruct(&this_result);
	}
	GridSel_destruct(&single_facet);
	output_selection(name, sel, result_selection, &my_result);
	return 1;
}

int Grid_cut(unsigned selection, unsigned nb_cuts, unsigned result_selection) {
	if (! this_grid || ! selection || ! nb_cuts) return 0;
	GridSel *sel = Grid_get_selection(selection);
	assert(sel);
	GridSel my_result;
	switch (sel->type) {
		case GridSel_VERTEX:
			return 0;
		case GridSel_EDGE:
			GridSel_construct(&my_result, GridSel_VERTEX);
			GridSel_reset(sel);
			Edge *edge;
			while ( (edge = GridSel_each(sel)) ) {
				for (unsigned i=0; i<nb_cuts; i++) {
					Grid_edge_cut(edge, 1.-1./(nb_cuts+1-i));	// cut given edge at north
					GridSel_add(&my_result, Edge_get_vertex(edge, NORTH));
				}
			}
			break;
		case GridSel_FACET:
			return 0;
		default:
			assert(0);
	}
	output_selection(selection, sel, result_selection, &my_result);
	return 1;
}

#include <float.h>
int Grid_plane_cut(unsigned selection, unsigned result_selection, Vec *center, Vec *normal) {
	if (! this_grid || ! selection) return 0;
	GridSel *sel = Grid_get_selection(selection);
	assert(sel);
	GridSel my_result;
	GridSel_construct(&my_result, GridSel_VERTEX);
	GridSel edge_sel;
	GridSel_convert(sel, &edge_sel, GridSel_EDGE, GridSel_MAX);
	GridSel_reset(&edge_sel);
	Edge *edge;
	while ( (edge = GridSel_each(&edge_sel)) ) {
		Vec ps, pn;
		Vec_sub3(&ps, Vertex_position(Edge_get_vertex(edge, SOUTH)), center);
		Vec_sub3(&pn, Vertex_position(Edge_get_vertex(edge, NORTH)), center);
		double ss = Vec_scalar(&ps, normal);
		double sn = Vec_scalar(&pn, normal);
		const double ss_sn = ss*sn;
		// when ss_sn is 'close enought' to zero, we do not cut but we add to the selection anyway so that a loop would always result from the planecut
		if (ss_sn < -DBL_EPSILON) {
			Grid_edge_cut(edge, fabs(ss/(ss-sn)));
		}
		if (ss_sn < DBL_EPSILON) {
			GridSel_add(&my_result, Edge_get_vertex(edge, NORTH));
		}
	}
	GridSel_destruct(&edge_sel);
	output_selection(selection, sel, result_selection, &my_result);
	return 1;
}

int Grid_connect(unsigned selection, unsigned result_selection, bool full_connect) {
	if (! this_grid || ! selection) return 0;
	GridSel *sel = Grid_get_selection(selection);
	if (! sel) return 0;
	if (sel->type != GridSel_VERTEX) {
		log_warning(LOG_IMPORTANT, "Cannot connect non-vertex selection");
		return 0;
	}
	GridSel my_result = GridSel_connect(sel, NULL, full_connect);
	output_selection(selection, sel, result_selection, &my_result);
	return 1;
}

int Grid_zap(unsigned selection) {
	if (! this_grid || ! selection) return 0;
	GridSel *sel = Grid_get_selection(selection);
	if (! sel) return 0;
	GridSel tmp_sel;
	switch (sel->type) {
		case GridSel_FACET:
			GridSel_convert(sel, &tmp_sel, GridSel_VERTEX, GridSel_MIN);
			GridSel_destruct(sel);
			*sel = tmp_sel;	// copied, so its like tmp_sel was never constructed
			// pass
		case GridSel_VERTEX:
			GridSel_reset(sel);
			Vertex *v;
			while ( (v=GridSel_each(sel)) ) {
				Vertex_zap(v);
			};
			break;
		case GridSel_EDGE:
			GridSel_reset(sel);
			Edge *e;
			while ( (e=GridSel_each(sel)) ) {
				Edge_zap(e);
			};
			break;
	}
	GridSel_clear(sel);
	return 1;
}

int Grid_bevel(unsigned name, unsigned result_selection, double ratio) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	if (! sel) return 0;
	GridSel my_result;
	switch (sel->type) {
		case GridSel_VERTEX: ;
			GridSel empty_sel;
			GridSel_construct(&empty_sel, GridSel_EDGE);
			my_result = GridSel_bevel(&empty_sel, sel, ratio);
			GridSel_destruct(&empty_sel);
			break;
		case GridSel_EDGE:
			my_result = GridSel_bevel(sel, NULL, ratio);
			break;
		case GridSel_FACET: ;
			GridSel tmp_edges;
			GridSel_convert(sel, &tmp_edges, GridSel_EDGE, GridSel_MIN);
			my_result = GridSel_bevel(&tmp_edges, NULL, ratio);
			GridSel_destruct(&tmp_edges);
			break;
		default:
			assert(0);
	}
	output_selection(name, sel, result_selection, &my_result);
	return 1;
}

int Grid_bevsmooth(unsigned name, unsigned level) {
	if (! this_grid || ! name) return 0;
	Grid_convert_selection(name, GridSel_FACET, GridSel_MAX);
	while (level>0) {
		Grid_bevel(name, name, 0.3);
		Grid_convert_selection(name, GridSel_EDGE, GridSel_MIN);
		Grid_convert_selection(name, GridSel_FACET, GridSel_MIN);
		level --;
	}
	return 1;
}

int Grid_smooth(unsigned name, unsigned level, double softness, unsigned result_selection) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	if (! sel) return 0;
	GridSel my_result = GridSel_smooth(sel, level, softness);
	output_selection(name, sel, result_selection, &my_result);
	return 1;
}

int Grid_separate(unsigned name, unsigned result_selection) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	if (! sel) return 0;
	GridSel my_result = GridSel_separate(sel);
	output_selection(name, sel, result_selection, &my_result);
	return 1;
}

/* Selection Manipulation */

int Grid_new_selection(unsigned name, GridSel_type type) {
	return NULL != Grid_new_selection_(name, type);
}

int Grid_del_selection(unsigned name) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	(void)GridSel_destruct(sel);
	cntHash_remove(this_grid->selections, (cntHashkey){ .i = name });
	return 0;
}

int Grid_empty_selection(unsigned name) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	GridSel_clear(sel);
	return 1;
}

int Grid_addsingle_to_selection(unsigned name, unsigned index) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	cntHashkey key = { .ptr = get_elmnt_from_index(sel->type, index) };
	if (key.ptr) {
		cntHash_put(sel->elmnts, key, NULL);
		return 1;
	} else {
		return 0;
	}
}

int Grid_subsingle_from_selection(unsigned name, unsigned index) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	cntHashkey key = { .ptr = get_elmnt_from_index(sel->type, index) };
	if (key.ptr) {
		cntHash_remove(sel->elmnts, key);
		return 1;
	} else {
		return 0;
	}
}

int Grid_add_to_selection(unsigned name_dest, unsigned name_src) {
	return add_or_sub_two_selections(name_dest, name_src, 1);
}

int Grid_sub_from_selection(unsigned name_dest, unsigned name_src) {
	return add_or_sub_two_selections(name_dest, name_src, 0);
}

int Grid_toggle_selection(unsigned name) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	GridSel_toggle_selection(sel);
	return 1;
}

int Grid_convert_selection(unsigned name, GridSel_type type, GridSel_convert_type convert_type) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	if (type == sel->type) return 1;
	GridSel my_result;
	GridSel_convert(sel, &my_result, type, convert_type);
	GridSel_destruct(sel);
	*sel = my_result;
	return 1;
}

int Grid_propagate_selection(unsigned name, unsigned level) {
	if (! this_grid || ! name) return 0;
	GridSel *sel = Grid_get_selection(name);
   assert(sel);
	GridSel_propagate(sel, level);
	return 1;
}

int Grid_replace_in_selections(GridSel_type type, void *old_elmnt, void *new_elmnt) {
	if (! this_grid) return 0;
	cntHash_reset(this_grid->selections);
	void *ptr;
	while (cntHash_each(this_grid->selections, NULL, &ptr)) {
		GridSel *const sel = ptr;
		if (sel->type == type && GridSel_selected(sel, old_elmnt)) {
			GridSel_remove(sel, old_elmnt);
			if (new_elmnt) GridSel_add(sel, new_elmnt);
		}
	}
	return 1;
}

/* Bases Functions  */

int Grid_new_basis(unsigned name, unsigned father, Vec *pos, Vec *x, Vec *y, Vec *z) {
	assert(pos && x && y && z);
	if (! this_grid || ! name) return 0;
	cntHashkey key = { .i = name };
	if (cntHash_get(this_grid->bases, key)) {
		return 0;
	}
	Basis *basis = cntHash_put(this_grid->bases, key, NULL);
	assert(basis);
	if (! Basis_construct(basis, father, pos, x,y,z)) {
		cntHash_remove(this_grid->bases, key);
		return 0;
	}
	return 1;
}
int Grid_del_basis(unsigned name) {
	if (! this_grid || ! name) return 0;
	Basis *basis = Grid_get_basis(name);
	assert(basis);
	Basis_destruct(basis);
	cntHashkey key = { .i = name };
	cntHash_remove(this_grid->bases, key);
	cntHash_reset(this_grid->bases);
	void *ptr;
	while (cntHash_each(this_grid->bases, &key, &ptr)) {
		Basis * const other_basis = ptr;
		if (other_basis->father == name) {
			other_basis->father = 0;
		}
		if (other_basis->is_instance && other_basis->instance_of == name) {
			other_basis->is_instance = false;
		}
	}
	Grid_reset_vertices();
	Vertex *v;
	while ( (v=Grid_each_vertex()) ) {
		if (Vertex_basis(v) == name) Vertex_set_basis(v, 0, 0.);
	}
	return 1;
}

int Grid_set_selection_hardskin(unsigned selection, unsigned basis) {
	if (! this_grid || ! selection) return 0;
	return GridSel_set_hardskin(Grid_get_selection(selection), basis);
}

int Grid_set_selection_softskin(unsigned selection, unsigned bi) {
	if (! this_grid || ! selection) return 0;
	return GridSel_set_softskin(Grid_get_selection(selection), bi);
}

int Grid_mapping(unsigned selection, GridSel_mapping_type type, const Vec *pos, float scale_x, float scale_y, float offset_x, float offset_y, bool along_normals) {
	if (! this_grid || ! selection) return 0;
	GridSel_mapping(Grid_get_selection(selection), type, pos, scale_x, scale_y, offset_x, offset_y, along_normals);
	return 1;
}

int Grid_set_uv(unsigned selection, float uv_x, float uv_y) {
	if (! this_grid || ! selection) return 0;
	GridSel_set_uv(Grid_get_selection(selection), uv_x, uv_y);
	return 1;
}

int Grid_set_instance(unsigned name, unsigned original, bool recursive) {
	if (name == original) return 0;
	Basis *basis = Grid_get_basis(name);
	assert(basis && (original==0 || Grid_get_basis(original)));
	Grid_reset_vertices();
	Vertex *v;
	while ( (v=Grid_each_vertex()) ) {
		if (Vertex_basis(v) == name) Vertex_set_basis(v, 0, 0.);
	}
	basis->is_instance = true;
	basis->recursive_instance = recursive;
	basis->instance_of = original;
	return 1;
}

bool Grid_basis_is_instance(unsigned name) {
	Basis *basis = Grid_get_basis(name);
	assert(basis);
	return basis->is_instance;
}

unsigned Grid_basis_original(unsigned name) {
	Basis *basis = Grid_get_basis(name);
	assert(basis);
	return basis->instance_of;
}

/* Colors functions */

int Grid_new_color(unsigned name, float r, float g, float b) {
	if (! this_grid || ! name) return 0;
	cntHashkey key = { .i = name };
	if (cntHash_get(this_grid->colors, key)) {
		return 0;
	}
	Color *color = cntHash_put(this_grid->colors, key, NULL);
	assert(color);
	if (! Color_construct(color, r,g,b)) {
		cntHash_remove(this_grid->colors, key);
		return 0;
	}
	return 1;
}

int Grid_del_color(unsigned name) {
	if (! name) return 0;
	Color *color = Grid_get_color(name);
	assert(color);
	Color_destruct(color);
	cntHashkey key = { .i = name };
	cntHash_remove(this_grid->colors, key);
	Grid_reset_vertices();
	Vertex *v;
	while ( (v=Grid_each_vertex()) ) {
		if (Vertex_color(v) == name) Vertex_set_color(v, 0);
	}
	return 1;
}

int Grid_set_selection_color(unsigned selection, unsigned color) {
	if (! selection) return 0;
	return GridSel_set_color(Grid_get_selection(selection), color);
}

/* Extraction Functions */

Facet *Grid_get_facet(unsigned index) {
	assert(this_grid);
	return cntHash_get(this_grid->facets, (cntHashkey){ .i = index });
}

Edge *Grid_get_edge(unsigned index) {
	assert(this_grid);
	return cntHash_get(this_grid->edges, (cntHashkey){ .i = index });
}

Edge *Grid_edge_index_from_vertices(Vertex *v1, Vertex *v2) {
	assert(v1 && v2);
	unsigned size = Vertex_size(v1);
	for (unsigned i=0; i<size; i++) {
		if (Vertex_get_vertex(v1, i) == v2) {
			return Vertex_get_edge(v1, i);
		}
	}
	return NULL;
}

Vertex *Grid_get_vertex(unsigned index) {
	assert(this_grid);
	return cntHash_get(this_grid->vertices, (cntHashkey){ .i = index });
}

void Grid_reset_vertices(void) {
	assert(this_grid);
	cntHash_reset(this_grid->vertices);
}

Vertex *Grid_each_vertex(void) {
	assert(this_grid);
	void *ptr;
	if (cntHash_each(this_grid->vertices, NULL, &ptr)) {
		return ptr;
	} else {
		return NULL;
	}
}

void Grid_reset_facets(void) {
	assert(this_grid);
	cntHash_reset(this_grid->facets);
}

Facet *Grid_each_facet(void) {
	assert(this_grid);
	void *ptr;
	if (cntHash_each(this_grid->facets, NULL, &ptr)) {
		return ptr;
	} else {
		return NULL;
	}
}

void Grid_reset_edges(void) {
	assert(this_grid);
	cntHash_reset(this_grid->edges);
}

Edge *Grid_each_edge(void) {
	assert(this_grid);
	void *ptr;
	if (cntHash_each(this_grid->edges, NULL, &ptr)) {
		return ptr;
	} else {
		return NULL;
	}
}

void Grid_reset_selections(void) {
	assert(this_grid);
	cntHash_reset(this_grid->selections);
}

unsigned Grid_each_selection(void) {
	assert(this_grid);
	cntHashkey key;
	if (cntHash_each(this_grid->selections, &key, NULL)) {
		return key.i;
	} else {
		return 0;
	}
}

bool Grid_selected(unsigned name, void *elmnt) {
	assert(this_grid);
	if (! name) return false;
	GridSel *sel = Grid_get_selection(name);
	if (! sel) return 0;
	return GridSel_selected(sel, elmnt);
}

unsigned Grid_selection_size(unsigned name) {
	if (! name) return 0;
	assert(this_grid);
	GridSel *sel = Grid_get_selection(name);
	if (! sel) return 0;
	return GridSel_size(sel);
}

void Grid_reset_selection(unsigned name) {
	assert(this_grid);
	if (! name) return;
	GridSel *sel = Grid_get_selection(name);
	if (! sel) return;
	cntHash_reset(sel->elmnts);
}

void *Grid_each_selected(unsigned name) {
	assert(this_grid);
	if (! name) return NULL;
	GridSel *sel = Grid_get_selection(name);
	if (! sel) return NULL;
	return GridSel_each(sel);
}

GridSel_type Grid_get_selection_type(unsigned name) {
	assert(this_grid && name>0);
	GridSel *sel = Grid_get_selection(name);
	assert(sel);
	return sel->type;
}

Basis *Grid_get_basis(unsigned name) {
	assert(this_grid && name>0);
	return cntHash_get(this_grid->bases, (cntHashkey){ .i = name });
}

void Grid_reset_bases(void) {
	assert(this_grid);
	cntHash_reset(this_grid->bases);
}

unsigned Grid_each_basis(void) {
	assert(this_grid);
	cntHashkey key;
	if (cntHash_each(this_grid->bases, &key, NULL)) {
		return key.i;
	} else {
		return 0;
	}
}
unsigned Grid_get_basis_father(unsigned name) {
	assert(this_grid && name>0);
	Basis *basis = Grid_get_basis(name);
	assert(basis);
	return basis->father;
}
const Vec *Grid_basis_axis(unsigned name, unsigned dim) {
	assert(this_grid && dim<3);
	if (0 == name) {
		static const Vec *root_axis[3] = { &vec_x, &vec_y, &vec_z };
		return root_axis[dim];
	}
	Basis *basis = Grid_get_basis(name);
	assert(basis);
	return &basis->axes[dim];
}
const Vec *Grid_basis_center(unsigned name) {
	assert(this_grid);
	if (0 == name) return &vec_origin;
	Basis *basis = Grid_get_basis(name);
	assert(basis);
	return &basis->position;
}

Color *Grid_get_color(unsigned name) {
	assert(this_grid && name>0);
	return cntHash_get(this_grid->colors, (cntHashkey){ .i = name });
}
void Grid_reset_colors(void) {
	assert(this_grid);
	cntHash_reset(this_grid->colors);
}
unsigned Grid_each_color(void) {
	assert(this_grid);
	cntHashkey key;
	if (cntHash_each(this_grid->colors, &key, NULL)) {
		return key.i;
	} else {
		return 0;
	}
}

// vi:ts=3:sw=3
