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
#ifndef FACET_H_041125
#define FACET_H_041125

typedef struct Facet Facet;

#include <libmicromodel/vertex.h>
#include <libmicromodel/edge.h>
#include <libcnt/vec.h>
#include <stdarg.h>

unsigned Facet_size(const Facet *this);

Edge *Facet_get_edge(const Facet *this, unsigned order);
Vertex *Facet_get_vertex(const Facet *this, unsigned order);
Facet *Facet_get_facet(const Facet *this, unsigned order);

const Vec *Facet_normal(Facet *this);
void Facet_invalidate_normal(Facet *this);
const Vec *Facet_center(Facet *this);

#include <stdbool.h>

struct Facet {
	unsigned size;
	unsigned name;
	Vec normal;
	bool normal_ok;
	struct FacetEdge *first_facetEdge;
};

int Facet_construct(Facet *this, unsigned name, unsigned size, Edge **edges, bool direct);
int Facet_destruct(Facet *this);

void Facet_add_edge_next(Facet *this, Edge *restrict edge, Edge *restrict new);
void Facet_split(Facet *restrict this, Facet *restrict new_facet, Edge *edge);
void Facet_change_edge(Facet *this, Edge *restrict old, Edge *restrict new, int inverse_side);
int Facet_remove_if_flat(Facet *this);
void Facet_remove_edge(Facet *this, Edge *edge);
EdgeSide Facet_my_side(const Facet *this, Edge *edge);
void Facet_swallow_by_edge(Facet *this, Edge *edge);

#include <assert.h>
static inline unsigned Facet_name(Facet *this) {
	assert(this);
	return this->name;
}

#endif
// vi:ts=3:sw=3
