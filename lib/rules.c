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
/* Rules :
 *
 * v1) a vertex must be connected to at least 2 distinct edges ;
 * v2) two adjascent connections to a vertex must be successive edges of the same facet, counter-clockwise ;
 * v3) all connections to a vertex must see this vertex as connected (reciprocity vertex <-> edge) ;
 *
 * e1) an edge must connect two distinct vertices ;
 * e2) an edge must be used by 2 distinct facets ;
 * e3) any connected vertices must see the edge as connected (reciprocity edge <-> vertex) ;
 * e4) any neighboring facets must see the edge as their (reciprocity edge <-> facet) ;
 *
 * f1) a facet must be composed of at least 2 distincts edges ;
 * f2) two sucessive edges of a facet must share a vertice ;
 * f3) the used edges must be used by the facet ;
 *
 */

#include <assert.h>
#include <stdbool.h>
#include "libmicromodel/vertex.h"
#include "libmicromodel/edge.h"
#include "libmicromodel/facet.h"
#include "rules.h"

// On vertices

bool rule_v1(const Vertex *v) {
	return v && Vertex_size(v)>=2;
}

bool rule_v2(const Vertex *v) {
	assert(v);
	for (unsigned i=0; i<Vertex_size(v); i++) {
		Edge *e[2] = {
			Vertex_get_edge(v, i),
			Vertex_get_edge(v, i+1==Vertex_size(v) ? 0:i+1)
		};
		Facet *f = Vertex_get_facet(v, i);
		unsigned j;
		for (j=0; j<Facet_size(f); j++) {
			if (Facet_get_edge(f, j) == e[0]) break;
		}
		if (j==Facet_size(f) || Facet_get_edge(f, j!=0 ? j-1:Facet_size(f)-1)!=e[1]) return false;
	}
	return true;
}

bool rule_v3(const Vertex *v) {
	assert(v);
	for (unsigned i=0; i<Vertex_size(v); i++) {
		Edge *e = Vertex_get_edge(v, i);
		if (Edge_get_vertex(e, SOUTH)!=v && Edge_get_vertex(e, NORTH)!=v) return false;
	}
	return true;
}

// On edges

bool rule_e1(const Edge *e) {
	assert(e);
	Vertex *v0 = Edge_get_vertex(e, SOUTH);
	Vertex *v1 = Edge_get_vertex(e, NORTH);
	return v0 && v1 && v0 != v1;
}

bool rule_e2(const Edge *e) {
	assert(e);
	Facet *f0 = Edge_get_facet(e, WEST);
	Facet *f1 = Edge_get_facet(e, EAST);
	return f0 && f1 && f0 != f1;
}

bool rule_e3(const Edge *e) {
	assert(e);
	Vertex *v;
	for (EdgePole s=SOUTH; s<NB_POLES; s++) {
		v = Edge_get_vertex(e, s);
		if (v) {
			unsigned i;
			for (i=0; i<Vertex_size(v); i++) {
				if (Vertex_get_edge(v, i) == e) break;
			}
			if (i==Vertex_size(v)) return false;
		}
	}
	return true;
}

bool rule_e4(const Edge *e) {
	assert(e);
	Facet *f;
	for (EdgeSide s=WEST; s<NB_SIDES; s++) {
		f = Edge_get_facet(e, s);
		if (f) {
			unsigned i;
			for (i=0; i<Facet_size(f); i++) {
				if (Facet_get_edge(f, i) == e) break;
			}
			if (i==Facet_size(f)) return false;
		}
	}
	return true;
}

// On Facets

bool rule_f1(const Facet *f) {
	assert(f);
	return Facet_size(f)>=2;
}

bool rule_f2(const Facet *f) {
	assert(f);
	for (unsigned i=0; i<Facet_size(f); i++) {
		Edge *e[2] = {
			Facet_get_edge(f, i),
			Facet_get_edge(f, i+1==Facet_size(f) ? 0:i+1)
		};
		if (
			Edge_get_vertex(e[0], SOUTH) != Edge_get_vertex(e[1], SOUTH) &&
			Edge_get_vertex(e[0], SOUTH) != Edge_get_vertex(e[1], NORTH) &&
			Edge_get_vertex(e[0], NORTH) != Edge_get_vertex(e[1], SOUTH) &&
			Edge_get_vertex(e[0], NORTH) != Edge_get_vertex(e[1], NORTH)
		) return false;
	}
	return true;
}

bool rule_f3(const Facet *f) {
	assert(f);
	for (unsigned i=0; i<Facet_size(f); i++) {
		Edge *e = Facet_get_edge(f, i);
		if (Edge_get_facet(e, WEST)!=f && Edge_get_facet(e, EAST)!=f) return false;
	}
	return true;
}

