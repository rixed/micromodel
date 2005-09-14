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
#include "libmicromodel/edge.h"
#include "libmicromodel/vertex.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/grid.h"
#include "rules.h"

/* Private Functions */

/* Public Functions */

int Edge_construct(Edge *this, unsigned name, Vertex *v1, Vertex *v2) {
	assert(this && v1 && v2);
	this->name = name;
	this->v[SOUTH] = v1;
	this->v[NORTH] = v2;
	this->facets[0] = this->facets[1] = NULL;
	this->normal_ok = false;
	return 1;
}

int Edge_destruct(Edge *this) {
	assert(this);
	return 1;
}

void Edge_add_facet(Edge *this, Facet *facet, EdgeSide side) {
	assert(this && facet && (side==WEST || side==EAST));
	this->normal_ok = false;
	this->facets[side] = facet;
	if (this->facets[!side]) {
		Vertex_add_edge(this->v[0], this);
		Vertex_add_edge(this->v[1], this);
	}
}

void Edge_change_facet(Edge *this, Facet *from, Facet *to) {
	assert(this && from && to);
	this->normal_ok = false;
	if (this->facets[WEST] == from) {
		assert(this->facets[EAST]!=from && this->facets[EAST]!=to);
		this->facets[WEST] = to;
	} else {
		assert(this->facets[EAST]==from && this->facets[WEST]!=to);
		this->facets[EAST] = to;
	}
}

/* Does NOT signal to vertices */
void Edge_change_vertex(Edge *this, Vertex *from, Vertex *to) {
	assert(this && from && to);
	this->normal_ok = false;
	if (this->v[SOUTH] == from) {
		assert(this->v[NORTH]!=from && this->v[SOUTH]!=to);
		this->v[SOUTH] = to;
	} else {
		assert(this->v[NORTH]==from && this->v[SOUTH]!=to);
		this->v[NORTH] = to;
	}
}

/* Does NOT signal to vertices */
void Edge_set_vertex(Edge *this, EdgePole pole, Vertex *new) {
	assert(this && new);
	assert(this->v[pole]);
	this->normal_ok = false;
	this->v[pole] = new;
}

void Edge_remove_facet(Edge *this, Facet *facet) {
	assert(this && facet);
	this->normal_ok = false;
	if (this->facets[WEST] == facet) {
		this->facets[WEST] = NULL;
	} else {
		assert(this->facets[EAST] == facet);
		this->facets[EAST] = NULL;
	}
}

void Edge_cut(Edge *this, Edge *new, Vertex *v) {
	assert(this && v);
	assert(rule_e3(this));
	this->normal_ok = false;
	new->facets[WEST] = this->facets[WEST];
	new->facets[EAST] = this->facets[EAST];
	// update connections
	Vertex_change_connection(this->v[NORTH], this, new);
	this->v[NORTH] = v;
	// init V's connections
	Vertex_add_edge(v, this);
	Vertex_add_edge(v, new);
	// update Facet West
	Facet *f = Edge_get_facet(this, WEST);
	Facet_add_edge_next(f, this, new);
	// update Facet East
	f = Edge_get_facet(this, EAST);
	Facet_add_edge_next(f, this, new);
}

void Edge_zap(Edge *this) {
	assert(this);
	assert(rule_e1(this) && rule_e2(this) && rule_e3(this) && rule_e4(this));
	/* 2 cases : if one of the vertice of the edge is of size 2, then merge the edge with the next.
	 * if not, merge the 2 facets together.
	 */
	for (EdgePole p=SOUTH; p<NB_POLES; p++) {
		if (Vertex_size(this->v[p]) == 2) {
			// merge with the other edge by zapping the vertex
			Vertex_zap(this->v[p]);
			return;
		}
	}
	// merge facets
	Facet_swallow_by_edge(this->facets[WEST], this);
}

Facet *Edge_get_facet(const Edge *this, EdgeSide side) {
	assert(this && (side==WEST || side==EAST));
	return this->facets[side];
}

Vertex *Edge_get_vertex(const Edge *this, EdgePole pole) {
	assert(this && (pole == NORTH || pole == SOUTH));
	return this->v[pole];
}

const Vec *Edge_normal(Edge *this) {
	assert(this);
	assert(rule_e2(this));
	if (this->normal_ok) return &this->normal;
	Vec_add3(&this->normal, Facet_normal(this->facets[0]), Facet_normal(this->facets[1]));
	Vec_normalize(&this->normal);
	this->normal_ok = true;
	return &this->normal;
}

double Edge_length(Edge *this) {
	assert(this);
	return Vec_dist(Vertex_position(this->v[0]), Vertex_position(this->v[1]));
}

/* Friends */

bool edges_are_connected(Edge *e0, Edge *e1) {
	assert(e0 && e1);
	return 
		(e0->v[SOUTH]!=NULL && (e0->v[SOUTH]==e1->v[SOUTH] || e0->v[SOUTH]==e1->v[NORTH])) ||
		(e0->v[NORTH]!=NULL && (e0->v[NORTH]==e1->v[SOUTH] || e0->v[NORTH]==e1->v[NORTH]));
}

// vi:ts=3:sw=3
