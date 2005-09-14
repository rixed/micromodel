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
#ifndef VERTEX_H_041125
#define VERTEX_H_041125

typedef struct Vertex Vertex;

#include <libmicromodel/edge.h>
#include <libmicromodel/facet.h>
#include <libcnt/vec.h>

unsigned Vertex_size(const Vertex *this);
Edge *Vertex_get_edge(const Vertex *this, unsigned order);
Facet *Vertex_get_facet(const Vertex *this, unsigned order);
Vertex *Vertex_get_vertex(const Vertex *this, unsigned order);
EdgePole Vertex_my_pole(const Vertex *this, unsigned order);

double distance2_between_vertices(Vertex *v1, Vertex *v2);
Edge *vertices_are_connected(Vertex *v1, Vertex *v2);
Facet *vertices_are_connectable(Vertex *v1, Vertex *v2);	// or connected
double Vertex_max_cos_to(Vertex *this, Vertex *to, Facet *f);
int Vertex_zap(Vertex *this);

#include <stdbool.h>
#include <libcnt/list.h>
#include <libcnt/vec_i.h>

typedef struct VertexEdge {
	Edge *edge;
	struct VertexEdge *next;
} VertexEdge;

struct Vertex {
	unsigned name;
	unsigned size;
	Vec position;
	unsigned basis;
	unsigned color;
	float skin_ratio;	// 0 -> fully in basis, 1 -> fully in basis' father
	float uv_x, uv_y;	// mapping coordinates in the range [-1,1] (when not looping)
	Vec normal;
	bool normal_ok;
	VertexEdge *first_vertexEdge;
};

int Vertex_construct(Vertex *this, unsigned name, const Vec *position, unsigned basis, float skin_ratio, float uv_x, float uv_y);
int Vertex_destruct(Vertex *this);
int Vertex_construct_average(Vertex *this, unsigned name, const Vertex *v1, const Vertex *v2, double ratio);
void Vertex_set_basis(Vertex *this, unsigned basis, float skin_ratio);

void Vertex_add_edge(Vertex *this, Edge *edge);
void Vertex_change_connection(Vertex *this, Edge *old, Edge *new);
void Vertex_move_connections(Vertex *this, Vertex *dest, Edge *from, Edge *to);
void Vertex_remove_connection(Vertex *this, Edge *edge);
const Vec *Vertex_normal(Vertex *this);

#include <assert.h>
static inline unsigned Vertex_basis(Vertex *this) {
	assert(this);
	return this->basis;
}
static inline float Vertex_skin_ratio(Vertex *this) {
	assert(this);
	return this->skin_ratio;
}
static inline Vec *Vertex_position(Vertex *this) {
	assert(this);
	return &this->position;
}
static inline unsigned Vertex_color(Vertex *this) {
	assert(this);
	return this->color;
}
static inline float Vertex_uv_x(Vertex *this) {
	assert(this);
	return this->uv_x;
}
static inline float Vertex_uv_y(Vertex *this) {
	assert(this);
	return this->uv_y;
}
static inline void Vertex_set_color(Vertex *this, unsigned color) {
	assert(this);
	this->color = color;
}
static inline unsigned Vertex_name(Vertex *this) {
	assert(this);
	return this->name;
}
static inline void Vertex_set_uv_mapping(Vertex *this, float uv_x, float uv_y) {
	assert(this);
	this->uv_x = uv_x;
	this->uv_y = uv_y;
}

#endif
// vi:ts=3:sw=3
