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
#include <stdbool.h>
#include <stdlib.h>
#include "libmicromodel/grid.h"
#include "libmicromodel/basis.h"
#include "libmicromodel/vertex.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/edge.h"
#include "gridsel.h"

/* Data Definitions */

/* Private Functions */

/* Public Functions */

int GridSel_construct(GridSel *this, GridSel_type type) {
	assert(this && (type == GridSel_VERTEX || type == GridSel_EDGE || type == GridSel_FACET));
	if (! (this->elmnts = cntHash_new(0, 50, 1, cntHash_PTRKEYS, 0)) ) {
		return 0;
	}
	this->type = type;
	return 1;
}
int GridSel_dup(GridSel *this, GridSel *source) {
	assert(this);
	GridSel_convert(source, this, source->type, GridSel_MIN);
	return 1;
}
int GridSel_destruct(GridSel *this) {
	assert(this);
	cntHash_del(this->elmnts);
	return 1;
}
void GridSel_reset(GridSel *this) {
	assert(this);
	cntHash_reset(this->elmnts);
}
void *GridSel_each(GridSel *this) {
	assert(this);
	cntHashkey key;
	if (cntHash_each(this->elmnts, &key, NULL)) {
		return key.ptr;
	} else {
		return NULL;
	}
}

unsigned GridSel_size(GridSel *this) {
	assert(this);
	return cntHash_size(this->elmnts);
}

bool GridSel_selected(GridSel *this, void *elmnt) {
	assert(this && elmnt);
	return cntHash_get(this->elmnts, (cntHashkey){ .ptr = elmnt });
}

void GridSel_center(GridSel *sel, Vec *dest) {
	assert(sel && dest);
	GridSel tmp; bool tmp_used = false;
	if (sel->type != GridSel_VERTEX) {
		GridSel_convert(sel, &tmp, GridSel_VERTEX, GridSel_MIN);
		tmp_used = true;
		sel = &tmp;
	}
	GridSel_reset(sel);
	Vec_construct(dest, 0,0,0);
	unsigned nb_vertices = 0;
	Vertex *v;
	while ( (v=GridSel_each(sel)) ) {
		const Vec *pos = Vertex_position(v);
		Vec_add(dest, pos);
		nb_vertices ++;
	}
	Vec_scale(dest, 1./nb_vertices);
	if (tmp_used) GridSel_destruct(&tmp);
}

void GridSel_apply_homotecy(GridSel *sel, Vec *center, Vec *axis, double ratio, void (*homotecy)(Vec *, Vec *, double)) {
	assert(homotecy && sel);
	GridSel tmp; bool tmp_used = false;
	if (sel->type != GridSel_VERTEX) {
		GridSel_convert(sel, &tmp, GridSel_VERTEX, GridSel_MIN);
		tmp_used = true;
		sel = &tmp;
	}
	GridSel_reset(sel);
	Vertex *v;
	while ( (v=GridSel_each(sel)) ) {
		Vec *pos = Vertex_position(v);
		if (center) Vec_sub(pos, center);
		homotecy(pos, axis, ratio);
		if (center) Vec_add(pos, center);
		for (unsigned f=0; f<Vertex_size(v); f++) {
			Facet *facet = Vertex_get_facet(v, f);
			if (facet) Facet_invalidate_normal(facet);
		}
	}
	if (tmp_used) GridSel_destruct(&tmp);
}

int GridSel_set_hardskin(GridSel *this, unsigned basis) {
	assert(this);
	Basis_remove_instance(Grid_get_basis(basis));
	GridSel tmp; bool tmp_used = false;
	if (this->type != GridSel_VERTEX) {
		GridSel_convert(this, &tmp, GridSel_VERTEX, GridSel_MIN);
		tmp_used = true;
		this = &tmp;
	}
	GridSel_reset(this);
	Vertex *v;
	while ( (v=GridSel_each(this)) ) {
		Vertex_set_basis(v, basis, 0.);
	}
	if (tmp_used) GridSel_destruct(&tmp);
	return 1;
}

int GridSel_set_softskin(GridSel *this, unsigned bi) {
	assert(this);
	if (0==bi) return GridSel_set_hardskin(this, 0);
	Basis_remove_instance(Grid_get_basis(bi));
	GridSel tmp; bool tmp_used = false;
	if (this->type != GridSel_VERTEX) {
		GridSel_convert(this, &tmp, GridSel_VERTEX, GridSel_MIN);
		tmp_used = true;
		this = &tmp;
	}
	Basis *basis = Grid_get_basis(bi);
	assert(basis);
	Basis *father = Basis_get_father(basis);
	GridSel_reset(this);
	Vertex *v;
	while ( (v=GridSel_each(this)) ) {
		double d1, d2;
		Vec dep;
		Vec_sub3(&dep, Vertex_position(v), Basis_position(basis));
		d1 = Vec_norm2(&dep);
		if (father) {
			Vec_sub3(&dep, Vertex_position(v), Basis_position(father));
			d2 = Vec_norm2(&dep);
		} else {
			d2 = Vec_norm2(Vertex_position(v));
		}
		Vertex_set_basis(v, bi, d1/(d1+d2));
	}
	if (tmp_used) GridSel_destruct(&tmp);
	return 1;
}

int GridSel_set_color(GridSel *this, unsigned color) {
	assert(this);
	GridSel tmp; bool tmp_used = false;
	if (this->type != GridSel_VERTEX) {
		GridSel_convert(this, &tmp, GridSel_VERTEX, GridSel_MIN);
		tmp_used = true;
		this = &tmp;
	}
	GridSel_reset(this);
	Vertex *v;
	while ( (v=GridSel_each(this)) ) {
		Vertex_set_color(v, color);
	}
	if (tmp_used) GridSel_destruct(&tmp);
	return 1;
}

void GridSel_add_or_sub(GridSel *dest, GridSel *src, bool add) {
	GridSel_reset(src);
	void *elmnt;
	while ( (elmnt = GridSel_each(src)) ) {
		if (add) GridSel_add(dest, elmnt);
		else GridSel_remove(dest, elmnt);
	}
}

void GridSel_toggle_selection(GridSel *this) {
	assert(this);
	switch (this->type) {
		case GridSel_VERTEX:
			Grid_reset_vertices();
			Vertex *v;
			while ( (v=Grid_each_vertex())) GridSel_toggle(this, v);
			break;
		case GridSel_EDGE:
			Grid_reset_edges();
			Edge *e;
			while ( (e=Grid_each_edge())) GridSel_toggle(this, e);
			break;
		case GridSel_FACET:
			Grid_reset_facets();
			Facet *f;
			while ( (f=Grid_each_facet())) GridSel_toggle(this, f);
			break;
		default:
			assert(0);
	}
}



// vi:ts=3:sw=3
