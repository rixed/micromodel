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
#include <assert.h>
#include <math.h>
#include "libmicromodel/vertex.h"
#include "libmicromodel/edge.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/grid.h"
#include "rules.h"
#include <libcnt/vec.h>

/* Data Definitions */

static cntShelf *vertexEdges = NULL;

/* Private Functions */

static void vertexEdges_end(void) {
	if (vertexEdges) {
		cntShelf_del(vertexEdges);
		vertexEdges = NULL;
	}
}

static int vertexEdges_init(void) {
	if (vertexEdges) return 1;
	vertexEdges = cntShelf_new(sizeof(VertexEdge), Grid_get_carac_size()*4);
	atexit(vertexEdges_end);
	return NULL != vertexEdges;
}

// Check internal data structure, not topology
static bool Vertex_is_valid(Vertex *this) {
	assert(this);
	// Check the VertexEdges list
	VertexEdge *ve = this->first_vertexEdge;
	for (unsigned i=0; i<this->size; i++) {
		ve = ve->next;	// TODO we should also check that the same edge or vertexEdge is not used twice
	}
	if (ve != this->first_vertexEdge) return false;
	return true;
}

/* Protected Functions */

int Vertex_construct(Vertex *this, unsigned name, const Vec *position, unsigned basis, float skin_ratio, float uv_x, float uv_y) {
	assert(this && position);
	if (!vertexEdges_init()) return 0;
	this->name = name;
	this->size = 0;
	this->first_vertexEdge = NULL;
	this->position = *position;
	this->normal_ok = false;
	Vertex_set_basis(this, basis, skin_ratio);
	Vertex_set_color(this, 0);
	this->uv_x = uv_x;
	this->uv_y = uv_y;
	assert(Vertex_is_valid(this));
	return 1;
}

int Vertex_destruct(Vertex *this) {
	assert(this);
	VertexEdge *first_ve = this->first_vertexEdge;
	if (!first_ve) return 1;
	VertexEdge *ve = first_ve;
	do {
		void *to_free = ve;
		ve = ve->next;
		cntShelf_free(vertexEdges, to_free);
	} while (ve != first_ve);
	return 1;
}

int Vertex_construct_average(Vertex *this, unsigned name, const Vertex *v1, const Vertex *v2, double ratio) {
	assert(this && v1 && v2);
	Vec pi;
	Vec_sub3(&pi, &v2->position, &v1->position);
	Vec_scale(&pi, ratio);
	Vec_add(&pi, &v1->position);
	float uv_x = v1->uv_x*ratio + v2->uv_x*(1.-ratio);
	float uv_y = v1->uv_y*ratio + v2->uv_y*(1.-ratio);
	if (v1->basis == v2->basis) {
		if (!Vertex_construct(this, name, &pi, v1->basis, v2->skin_ratio*ratio+v1->skin_ratio*(1.-ratio), uv_x, uv_y)) {
			return 0;
		}
	} else {
		if (!Vertex_construct(this, name, &pi, ratio <= .5 ? v1->basis:v2->basis, ratio <= .5 ? v1->skin_ratio:v2->skin_ratio, uv_x, uv_y)) {
			return 0;
		}
	}
	this->color = ratio <= .5 ? v1->color:v2->color;
	return 1;
}

void Vertex_set_basis(Vertex *this, unsigned basis, float skin_ratio) {
	assert(this);
	this->basis = basis;
	this->skin_ratio = skin_ratio;
	assert(this->basis>0 || skin_ratio==0);	// root has no father
}

void Vertex_add_edge(Vertex *this, Edge *edge) {
	assert(this && edge);
	this->normal_ok = false;
	VertexEdge *ve = this->first_vertexEdge;
	if (ve) do {
		if (ve->edge == edge) return;
		ve = ve->next;
	} while (ve != this->first_vertexEdge);
	VertexEdge *new_ve = cntShelf_alloc(vertexEdges);
	assert(new_ve);
	new_ve->edge = edge;
	if (0 == this->size) {
		this->first_vertexEdge = new_ve;
		new_ve->next = new_ve;
		this->size ++;
		assert(Vertex_is_valid(this));
		return;
	}
	ve = this->first_vertexEdge;
	new_ve->next = ve->next;
	ve->next = new_ve;
	this->size ++;
	// reorder everything
	VertexEdge *ve_;
	unsigned i, i_;
	assert(rule_v3(this));
	for (i=0, ve=this->first_vertexEdge; i<this->size-1; i++, ve=ve->next) {
		Facet *c_next;
		if (Edge_get_vertex(ve->edge, SOUTH) == this) {	// Im south, next facet is then WEST
			c_next = Edge_get_facet(ve->edge, WEST);
		} else {
			c_next = Edge_get_facet(ve->edge, EAST);
		}
		for (i_=i+1, ve_=ve->next; i_<this->size; i_++, ve_=ve_->next) {
			Facet *c2_previous;	
			if (Edge_get_vertex(ve_->edge, SOUTH) == this) {	// Im south, previous facet of c2 is then EAST
				c2_previous = Edge_get_facet(ve_->edge, EAST);
			} else {
				c2_previous = Edge_get_facet(ve_->edge, WEST);
			}
			if (c2_previous == c_next) {	// then c2 should come right after c
				if (i_ > i+1) {
					VertexEdge *ve_prev = ve->next;
					while (ve_prev->next != ve_) ve_prev = ve_prev->next;
					ve_prev->next = ve_->next;
					ve_->next = ve->next;
					ve->next = ve_;
				}
				break;
			}
		}
	}
	assert(Vertex_is_valid(this));
}

void Vertex_change_connection(Vertex *this, Edge *old, Edge *new) {
	assert(this && old && new);
	this->normal_ok = false;
	unsigned i;
	VertexEdge *ve = this->first_vertexEdge;
	assert(ve);
	for (i=0; i<this->size; i++, ve=ve->next) {
		if (ve->edge == old) break;
	}
	assert(i<this->size);
	ve->edge = new;
	assert(Vertex_is_valid(this));
}

// Edges are signaled
void Vertex_move_connections(Vertex *this, Vertex *dest, Edge *from, Edge *to) {
	assert(this && dest && from && to && rule_v1(this));
	this->normal_ok = false;
	unsigned i = 0;
	VertexEdge *ve = this->first_vertexEdge;
	assert(ve);
	while (ve->edge != from && i++ < this->size) ve = ve->next;
	assert(i < this->size);
	VertexEdge *previous = ve;
	ve = ve->next;	// keep from and to here
	i = 0;
	while (ve->edge != to && i++ < this->size) {
		Edge_change_vertex(ve->edge, this, dest);
		Vertex_add_edge(dest, ve->edge);
		previous->next = ve->next;
		if (this->first_vertexEdge==ve) this->first_vertexEdge=previous;
		cntShelf_free(vertexEdges, ve);
		ve = previous->next;
		this->size --;
		assert(this->size>2);
	}
	assert(i < this->size);
}

void Vertex_remove_connection(Vertex *this, Edge *edge) {
	assert(this && edge);
	this->normal_ok = false;
	VertexEdge *ve = this->first_vertexEdge;
	assert(ve);
	while (ve->next->edge != edge) ve = ve->next;
	if (this->first_vertexEdge == ve->next) this->first_vertexEdge = ve;
	VertexEdge *to_free = ve->next;
	ve->next = ve->next->next;
	cntShelf_free(vertexEdges, to_free);
	if (! --this->size) this->first_vertexEdge = NULL;
	assert(Vertex_is_valid(this));
	if (0 == this->size) {	// doom it (usefull for zap
		Grid_replace_vertex(this, NULL);
	}
}

/* Public Functions */

unsigned Vertex_size(const Vertex *this) {
	assert(this);
	return this->size;
}

Edge *Vertex_get_edge(const Vertex *this, unsigned order) {
	assert(this && order<Vertex_size(this));
	VertexEdge *ve = this->first_vertexEdge;
	assert(ve);
	for ( ; order-- > 0 ; ) ve = ve->next;
	return ve->edge;
}

Vertex *Vertex_get_vertex(const Vertex *this, unsigned order) {
	assert(this && order<Vertex_size(this));
	Edge *e = Vertex_get_edge(this, order);
	Vertex *v = Edge_get_vertex(e, SOUTH);
	if (this!=v) {
		assert(Edge_get_vertex(e, NORTH) == this);
		return v;
	} else {
		return Edge_get_vertex(e, NORTH);
	}
}

Facet *Vertex_get_facet(const Vertex *this, unsigned order) {
	assert(this && order<Vertex_size(this));
	Edge *e = Vertex_get_edge(this, order);
	EdgePole my_pole = Vertex_my_pole(this, order);
	EdgeSide side;
	if (my_pole == SOUTH) side = WEST;
	else side = EAST;
	return Edge_get_facet(e, side);
}

EdgePole Vertex_my_pole(const Vertex *this, unsigned order) {
	assert(this && order<Vertex_size(this));
	Edge *e = Vertex_get_edge(this, order);
	if (this == Edge_get_vertex(e, SOUTH)) {
		return SOUTH;
	} else {
		assert(this == Edge_get_vertex(e, NORTH));
		return NORTH;
	}
}

int Vertex_zap(Vertex *this) {
	// Merge this vertex with its closest neighbor
	assert(this);
	//assert(rule_v1(this));
	double best_dist;
	int best_neig = -1;
	for (unsigned i=0; i<Vertex_size(this); i++) {
		Vertex *v_neig = Vertex_get_vertex(this, i);
		double dist = Vec_dist(Vertex_position(this), Vertex_position(v_neig));
		if (-1 == best_neig || dist < best_dist) {
			// not allowable if v_neig and any other linked vertex are linked with each other 'secretly'
			unsigned j;
			for (j=0; j<Vertex_size(this); j++) {
				if ((j<=i && i-j<2) || (j>i && j-i<2)) continue;
				Vertex *other = Vertex_get_vertex(this, j);
				if (vertices_are_connected(v_neig, other)) break;
			}
			if (j == Vertex_size(this)) {
				best_dist = dist;
				best_neig = i;
			}
		}
	}
	if (-1 == best_neig) {	// not found
		return 0;
	}
	Vertex *best_v = Vertex_get_vertex(this, best_neig);
	Edge *best_e = Vertex_get_edge(this, best_neig);
	assert(rule_e2(best_e));
	Facet *f[2] = {
		Edge_get_facet(best_e, WEST),
		Edge_get_facet(best_e, EAST)
	};
	Facet_remove_edge(f[0], best_e);
	Facet_remove_edge(f[1], best_e);
	Vertex_remove_connection(best_v, best_e);
	// Facet_remove_edge can lead to other edge removal. Thus, best_neig is not usable any more.
	for (unsigned i=0; i<Vertex_size(this); i++) {
		Edge *e = Vertex_get_edge(this, i);
		if (e == best_e) continue;
		assert(Edge_get_vertex(e, SOUTH) == this || Edge_get_vertex(e, NORTH) == this);
		Edge_change_vertex(e, this, best_v);
		Vertex_add_edge(best_v, e);
	}
	Facet_remove_if_flat(f[0]);
	Facet_remove_if_flat(f[1]);
	Grid_replace_edge(best_e, NULL);
	Grid_replace_vertex(this, best_v);
	return 1;
}

const Vec *Vertex_normal(Vertex *this) {
	assert(this);
	assert(rule_v1(this));
	if (this->normal_ok) return &this->normal;
	this->normal = vec_origin;
	if (Vertex_size(this)<2) return &this->normal;
	if (Vertex_size(this)==2) return Edge_normal(Vertex_get_edge(this, 0));
	const Vec *pos_ref = Vertex_position(this);
	Vec pos[2];
	pos[1] = *Vertex_position(Vertex_get_vertex(this, 0));
	Vec_sub(&pos[1], pos_ref);
	Vec_normalize(&pos[1]);
	for (unsigned i=0; i<Vertex_size(this); i++) {
		pos[0] = pos[1];
		pos[1] = *Vertex_position(Vertex_get_vertex(this, i<Vertex_size(this)-1 ? i+1:0));
		Vec_sub(&pos[1], pos_ref);
		Vec_normalize(&pos[1]);
		Vec tmp;
		Vec_product(&tmp, &pos[0], &pos[1]);
		Vec_add(&this->normal, &tmp);
	}
	Vec_normalize(&this->normal);
	this->normal_ok = true;
	return &this->normal;
}

double distance2_between_vertices(Vertex *v1, Vertex *v2) {
	assert(v1 && v2);
	Vec p1;
	Vec_sub3(&p1, Vertex_position(v2), Vertex_position(v1));
	return Vec_norm2(&p1);
}

Edge *vertices_are_connected(Vertex *v1, Vertex *v2) {
	unsigned v1_size = Vertex_size(v1);
	unsigned o1;
	for (o1 = 0; o1 < v1_size; o1++) {
		Vertex *v = Vertex_get_vertex(v1, o1);
		assert(v);
		if (v == v2) return Vertex_get_edge(v1, o1);
	}
	return NULL;
}

Facet *vertices_are_connectable(Vertex *v1, Vertex *v2) {
	unsigned v1_size = Vertex_size(v1);
	unsigned v2_size = Vertex_size(v2);
	Facet *f1 = NULL;
	unsigned o1, o2;
	assert(rule_v2(v1) && rule_v2(v2));
	for (o1 = 0; o1 < v1_size; o1++) {
		f1 = Vertex_get_facet(v1, o1);
		for (o2 = 0; o2 < v2_size; o2++) {
			Facet *f2 = Vertex_get_facet(v2, o2);
			if (f1 == f2) return f1;
		}
	}
	return NULL;
}

double Vertex_max_cos_to(Vertex *this, Vertex *to, Facet *f) {
	assert(this && to);
	unsigned o;
	for (o=0; o<Vertex_size(this); o++) {
		if (f == Vertex_get_facet(this, o)) break;
	}
	Vertex *before, *after;
	before = Vertex_get_vertex(this, o);
	after = Vertex_get_vertex(this, (o+1)%Vertex_size(this));
	assert(before && after);
	Vec v0, v1, v2;
	Vec_sub3(&v0, Vertex_position(before), Vertex_position(this));
	Vec_sub3(&v1, Vertex_position(to), Vertex_position(this));
	Vec_sub3(&v2, Vertex_position(after), Vertex_position(this));
	Vec_normalize(&v0);
	Vec_normalize(&v1);
	Vec_normalize(&v2);
	double cos1 = Vec_scalar(&v0, &v1);
	double cos2 = Vec_scalar(&v1, &v2);
	return cos1 > cos2 ? cos1 : cos2;
}

// vi:ts=3:sw=3
