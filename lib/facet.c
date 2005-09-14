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
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <libcnt/shelf.h>
#include <libcnt/log.h>
#include "libmicromodel/grid.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/edge.h"
#include "libmicromodel/vertex.h"
#include "rules.h"

/* Data Definitions */

static cntShelf *facetEdges = NULL;

typedef struct FacetEdge {
	Edge *edge;
	struct FacetEdge *next;
} FacetEdge;

/* Private Functions */

static void facetEdges_end(void) {
	if (facetEdges) {
		cntShelf_del(facetEdges);
		facetEdges = NULL;
	}
}

static int facetEdges_init(void) {
	if (facetEdges) return 1;
	facetEdges = cntShelf_new(sizeof(FacetEdge), Grid_get_carac_size()*4);
	atexit(facetEdges_end);
	return NULL != facetEdges;
}

// Check internal data structure, not topology
static bool Facet_is_valid(Facet *this) {
	assert(this);
	// Check the FacetEdges list
	FacetEdge *fe = this->first_facetEdge;
	for (unsigned i=0; i<this->size; i++) {
		fe = fe->next;	// TODO we should also check that the same edge or facetEdge is not used twice
	}
	if (fe != this->first_facetEdge) return false;
	return true;
}

static void Facet_sub_hear(Facet *this, unsigned start, unsigned stop, Edge *edge) {
	// Replace every edges from order start to stop by the Edge edge
	assert(this && start!=stop && start<this->size && stop<this->size && edge);
	this->normal_ok = false;
	FacetEdge *new_fe = cntShelf_alloc(facetEdges);
	assert(new_fe);
	new_fe->edge = edge;
	unsigned i = 0;
	FacetEdge *fe = this->first_facetEdge, *next_fe;
	if (start < stop) {
		for ( ; i < this->size; i++, fe=next_fe) {
			next_fe = fe->next;
			if (i+1 == start || (this->size-1 == i && 0 == start)) {
				fe->next = new_fe;
			} else if (i >= start && i < stop) {
				cntShelf_free(facetEdges, fe);
			} else if (i == stop) {
				new_fe->next = fe;
			} else if (i > stop && start > 0) {
				break;
			}
		}
		this->size -= stop - start - 1;
	} else {
		for ( ; i < this->size; i++, fe=next_fe) {
			next_fe = fe->next;
			if (i < stop || i >= start) {
				cntShelf_free(facetEdges, fe);
			} else if (i == stop) {
				new_fe->next = fe;
			} else if (i+1 == start) {
				fe->next = new_fe;
			}
		}
		this->size = start - stop + 1;
	}
	this->first_facetEdge = new_fe;
	assert(Facet_is_valid(this));
}

/* Public Functions */

EdgeSide Facet_my_side(const Facet *this, Edge *edge) {
	assert(this && edge);
	if (this == Edge_get_facet(edge, WEST)) {
		return WEST;
	} else {
		assert(Edge_get_facet(edge, EAST) == this);
		return EAST;
	}
}

int Facet_construct(Facet *this, unsigned name, unsigned size, Edge **edges, bool direct) {
	// The edges are given in direct order, and must have their right vertices already ;
	// they are informed of their new relationship with this facet.
	assert(this);
	if (!facetEdges_init()) return 0;
	this->name = name;
	this->size = size;
	this->normal_ok = false;
	FacetEdge *previous = NULL;
	Edge *first_edge = NULL;
	if (!size) return 1;	// as a special case, we accept empty facets
	int i_start, i_end, i_inc;
	if (direct) {
		i_start = 0;
		i_end = size;
		i_inc = 1;
	} else {
		i_start = size-1;
		i_end = -1;
		i_inc = -1;
	}
	for (int i=i_start; i != i_end+i_inc; i += i_inc) {
		Edge *edge;
		FacetEdge *fe;
		if (i != i_end) {
			edge = edges[i];
			fe = cntShelf_alloc(facetEdges);
			assert(fe);
			fe->edge = edge;
		} else {	// looping
			edge = first_edge;
			fe = this->first_facetEdge;
			assert(previous);
		}
		if (!previous) {
			first_edge = edge;
			this->first_facetEdge = fe;
		} else {
			previous->next = fe;
			EdgeSide previous_side;
			/* So, what side was previous on ?
			 * Edges are passed in direct order, so that If last vertex of previous segment = a vertex of current = NORTH,
			 * then previous was directed toward direct rotation, and so facet is WEST side.
			 */
			if (
				Edge_get_vertex(previous->edge, NORTH) == Edge_get_vertex(edge, SOUTH) ||
				Edge_get_vertex(previous->edge, NORTH) == Edge_get_vertex(edge, NORTH)
			) {
				previous_side = WEST;
			} else {
				assert(
					Edge_get_vertex(previous->edge, SOUTH) == Edge_get_vertex(edge, SOUTH) ||
					Edge_get_vertex(previous->edge, SOUTH) == Edge_get_vertex(edge, NORTH)
				);
				previous_side = EAST;
			}
			Edge_add_facet(previous->edge, this, previous_side);
		}
		previous = fe;
	}
	assert(Facet_is_valid(this));
	return 1;
}

int Facet_destruct(Facet *this) {
	assert(this);
	FacetEdge *first_fe = this->first_facetEdge;
	if (!first_fe) return 1;
	FacetEdge *fe = first_fe;
	do {
		void *to_free = fe;
		fe = fe->next;
		cntShelf_free(facetEdges, to_free);
	} while (fe != first_fe);
	return 1;
}

void Facet_add_edge_next(Facet *this, Edge *restrict edge, Edge *restrict new) {
	assert(this && edge && new);
	this->normal_ok = false;
	unsigned i;
	FacetEdge *previous_fe = NULL, *fe = this->first_facetEdge;
	for (i=0; i<this->size; i++) {
		if (fe->edge == edge) break;
		previous_fe = fe;
		fe = fe->next;
	}
	assert(i<this->size);
	FacetEdge *new_fe = cntShelf_alloc(facetEdges);
	assert(new_fe);
	new_fe->edge = new;
	// did new comes before or after edge ?
	Vertex *v = Facet_get_vertex(this, i);	// first - for us - vertex of edge
	if (Edge_get_vertex(new, SOUTH) == v || Edge_get_vertex(new, NORTH) == v) {	// new is before edge
		new_fe->next = fe;
		if (!previous_fe) {
			for (i=0, previous_fe=this->first_facetEdge; i<this->size-1; i++) previous_fe = previous_fe->next;
			this->first_facetEdge = new_fe;
		}
		previous_fe->next = new_fe;
	} else {	// new is after edge
		new_fe->next = fe->next;
		fe->next = new_fe;
	}
	this->size ++;
	assert(Facet_is_valid(this));
}

void Facet_split(Facet *restrict this, Facet *restrict new_facet, Edge *edge) {
	// The new facet is build EASTSIDE from edge
	assert(this && new_facet && edge);
	assert(Facet_is_valid(this));
	this->normal_ok = false;
	// construct new_facet as an empty facet
	new_facet->size = 1;
	new_facet->first_facetEdge = cntShelf_alloc(facetEdges);
	new_facet->first_facetEdge->edge = edge;
	new_facet->first_facetEdge->next = new_facet->first_facetEdge;	// enought for Facet_add_edge_next
	// get vertices
	Vertex *v1 = Edge_get_vertex(edge, SOUTH);
	Vertex *v2 = Edge_get_vertex(edge, NORTH);
	assert(v1 && v2);
	// look for v1 in this
	unsigned i;
	for (i=0; i<this->size; i++) {
		if (Facet_get_vertex(this, i) == v1) break;
	}
	assert(i < this->size);
	// until v2, add the edges to new_facet
	Vertex *v = v1;
	Edge *e = edge;
	unsigned i1 = i;
	do {
		Edge *e_ = Facet_get_edge(this, i);
		assert(e_);
		Edge_change_facet(e_, this, new_facet);
		Facet_add_edge_next(new_facet, e, e_);
		e = e_;
		if (++ i >= this->size) i = 0;
		v = Facet_get_vertex(this, i);
	} while (v != v2);
	// Now that's done, signal this and new_facet to edge (wich will signal to vertices)
	Edge_add_facet(edge, this, WEST);
	Edge_add_facet(edge, new_facet, EAST);
	// remove those edges from this facet, closing with edge
	Facet_sub_hear(this, i1, i, edge);
	assert(Facet_is_valid(this));
	assert(Facet_is_valid(new_facet));
}

void Facet_change_edge(Facet *this, Edge *restrict old, Edge *restrict new, int inverse_side) {
	assert(this && old && new);
	this->normal_ok = false;
	FacetEdge *fe = this->first_facetEdge;
	assert(fe);
	unsigned i;
	for (i=0; i<this->size; i++) {
		if (fe->edge == old) {
			EdgeSide side = Facet_my_side(this, old);
			Edge_remove_facet(old, this);
			fe->edge = new;
			Edge_add_facet(new, this, inverse_side ? !side : side);
			break;
		}
		fe = fe->next;
	}
	assert(i<this->size);
	assert(Facet_is_valid(this));
}

int Facet_remove_if_flat(Facet *this) {
	assert(this);
	if (this->size > 2) return 0;
	Edge *old_e = Facet_get_edge(this, 0);
	Edge *new_e = Facet_get_edge(this, 1);
	Facet_change_edge(Facet_get_facet(this, 0), old_e, new_e, Edge_get_vertex(old_e, SOUTH) != Edge_get_vertex(new_e, SOUTH));
	Vertex_remove_connection(Edge_get_vertex(old_e, SOUTH), old_e);
	Vertex_remove_connection(Edge_get_vertex(old_e, NORTH), old_e);
	Grid_replace_edge(old_e, new_e);
	Grid_replace_facet(this, NULL);
	return 1;
}

void Facet_remove_edge(Facet *this, Edge *edge) {
	assert(this && edge);
	assert(rule_f1(this));
	this->normal_ok = false;
	FacetEdge *fe = this->first_facetEdge;
	if (fe->edge == edge) this->first_facetEdge = fe->next;
	unsigned i;
	for (i=0; i<this->size; i++, fe=fe->next) {
		if (fe->next->edge == edge) {
			FacetEdge *tmp = fe->next->next;
			cntShelf_free(facetEdges, fe->next);
			fe->next = tmp;
			break;
		}
	}
	assert(i < this->size);
	this->size --;
	assert(Facet_is_valid(this));
}

unsigned Facet_size(const Facet *this) {
	assert(this);
	return this->size;
}


Edge *Facet_get_edge(const Facet *this, unsigned order) {
	assert(this && order<this->size);
	FacetEdge *fe = this->first_facetEdge;
	for (unsigned i=0; i<order; i++) {
		fe = fe->next;
	}
	return fe->edge;
}

Vertex *Facet_get_vertex(const Facet *this, unsigned order) {
	assert(this && order < this->size);
	FacetEdge *fe = this->first_facetEdge;
	for (unsigned i=0; i<order; i++) {
		fe = fe->next;
	}
	return Edge_get_vertex(fe->edge, Edge_get_facet(fe->edge, WEST)==this ? SOUTH : NORTH);
}

Facet *Facet_get_facet(const Facet *this, unsigned order) {
	assert(this && order < this->size);
	Edge *edge = Facet_get_edge(this, order);
	return Edge_get_facet(edge, !Facet_my_side(this, edge));
}

const Vec *Facet_normal(Facet *this) {
	assert(this);
	if (this->normal_ok) return &this->normal;
	this->normal_ok = true;
	this->normal = vec_origin;
	if (Facet_size(this)<3) return &this->normal;
	const Vec *pos_ref = Vertex_position(Facet_get_vertex(this, 0));
	Vec pos[2];
	pos[1] = *Vertex_position(Facet_get_vertex(this, 1));
	Vec_sub(&pos[1], pos_ref);
	Vec_normalize(&pos[1]);
	for (unsigned i=2; i<Facet_size(this); i++) {
		pos[0] = pos[1];
		pos[1] = *Vertex_position(Facet_get_vertex(this, i));
		Vec_sub(&pos[1], pos_ref);
		Vec_normalize(&pos[1]);
		Vec tmp;
		Vec_product(&tmp, &pos[0], &pos[1]);
		Vec_add(&this->normal, &tmp);
	}
	Vec_normalize(&this->normal);
	return &this->normal;
}

const Vec *Facet_center(Facet *this) {
	assert(this);
	static Vec center;
	Vec_construct(&center, 0.,0.,0.);
	for (unsigned i=0; i<Facet_size(this); i++) {
		Vec_add(&center, Vertex_position(Facet_get_vertex(this, i)));
	}
	Vec_scale(&center, 1./Facet_size(this));
	return &center;
}

void Facet_swallow_by_edge(Facet *this, Edge *edge) {
	assert(this && edge && rule_f3(this));
	Facet *other = Edge_get_facet(edge, WEST);
	if (other == this) other = Edge_get_facet(edge, EAST);
	// TODO
}

// vi:ts=3:sw=3
