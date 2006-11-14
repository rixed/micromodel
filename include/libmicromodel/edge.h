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
#ifndef EDGE_H_041125
#define EDGE_H_041125

typedef struct Edge Edge;
typedef enum { WEST=0, EAST, NB_SIDES } EdgeSide;
typedef enum { SOUTH=0, NORTH, NB_POLES } EdgePole;

#include <libmicromodel/facet.h>
#include <libmicromodel/vertex.h>
#include <libcnt/vec.h>

Facet *Edge_get_facet(const Edge *this, EdgeSide side);
Vertex *Edge_get_vertex(const Edge *this, EdgePole pole);

const Vec *Edge_normal(Edge *this);
double Edge_length(Edge *this);

#include <stdbool.h>

struct Edge {
	unsigned name;
	Vertex *v[2];
	Facet *facets[2];	// WEST, EAST
	Vec normal;
	bool normal_ok;
};

int Edge_construct(Edge *this, unsigned name, Vertex *v1, Vertex *v2);
int Edge_destruct(Edge *this);
void Edge_add_facet(Edge *this, Facet *facet, EdgeSide side);
void Edge_change_facet(Edge *this, Facet *from, Facet *to);
void Edge_change_vertex(Edge *this, Vertex *from, Vertex *to);
void Edge_remove_facet(Edge *this, Facet *facet);
void Edge_set_vertex(Edge *this, EdgePole pole, Vertex *new);
void Edge_cut(Edge *this, Edge *new, Vertex *v);
bool edges_are_connected(Edge *e0, Edge *e1);
void Edge_zap(Edge *this);

#include <assert.h>
static inline unsigned Edge_name(Edge *this) {
	assert(this);
	return this->name;
}

#endif
// vi:ts=3:sw=3
