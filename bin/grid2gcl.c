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
#include "config.h"
#include "grid2gcl.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <libcnt/hash.h>
#include <libcnt/list.h>
#include "libmicromodel/grid.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/edge.h"
#include "libmicromodel/vertex.h"
#include "libmicromodel/basis.h"
#include "libmicromodel/color.h"

/* Data Definitions */

cntHash *vertex2order = NULL;
cntList *order2vertex = NULL;
cntHash *order2facet = NULL;
static unsigned vertex_order, facet_order;

/* Private Functions */

static void hash_init(void) {
	vertex2order = cntHash_new(sizeof(unsigned), Grid_get_carac_size(), 4, cntHash_PTRKEYS, 0);
	assert(vertex2order);
	order2vertex = cntList_new(sizeof(Vertex *), Grid_get_carac_size());
	assert(order2vertex);
	order2facet = cntHash_new(sizeof(Facet *), Grid_get_carac_size(), 4, cntHash_INTKEYS, 0);
	assert(order2facet);
}

static void hash_free(void) {
	if (vertex2order) {
		cntHash_del(vertex2order);
		vertex2order = NULL;
	}
	if (order2vertex) {
		cntList_del(order2vertex);
		order2vertex = NULL;
	}
	if (order2facet) {
		cntHash_del(order2facet);
		order2facet = NULL;
	}
}

static unsigned order_of_vertex(Vertex *v) {
	// return the order of the given vertex when it was registered
	assert(v);
	assert(vertex2order);
	unsigned *o = cntHash_get(vertex2order, (cntHashkey){ .ptr = v });
	assert(o);
	return *o;
}

static void register_reset(void) {
	vertex_order = 0;
	facet_order = 0;
}
static void register_vertex(Vertex *v) {
	assert(v);
	cntHashkey key = { .ptr = v };
	assert(NULL == cntHash_get(vertex2order, key));
	cntHash_put(vertex2order, key, &vertex_order);
	cntList_set(order2vertex, vertex_order, &v);
	vertex_order++;
}
static void register_facet(Facet *f) {
	assert(f);
	cntHashkey key = { .i = facet_order };
	assert(NULL == cntHash_get(order2facet, key));
	cntHash_put(order2facet, key, &f);
	facet_order++;
}

static void print_vertex(Vertex *vertex) {
	assert(vertex);
	Vec *pos = Vertex_position(vertex);
	assert(pos);
	printf("%e %e %e\n", Vec_coord(pos, 0), Vec_coord(pos, 1), Vec_coord(pos, 2));
}

static void print_vertex_n_normal(Vertex *vertex) {
	assert(vertex);
	const Vec *pos = Vertex_position(vertex);
	const Vec *norm = Vertex_normal(vertex);
	assert(pos && norm);
	printf("%e %e %e %e %e %e %e %e\n",
			Vec_coord(pos, 0), Vec_coord(pos, 1), Vec_coord(pos, 2),
			Vec_coord(norm, 0), Vec_coord(norm, 1), Vec_coord(norm, 2),
			.5*(1+Vertex_uv_x(vertex)), .5*(1+Vertex_uv_y(vertex)));
}

static void print_vertex_n_normal_n_color(Vertex *vertex) {
	assert(vertex);
	const Vec *pos = Vertex_position(vertex);
	const Vec *norm = Vertex_normal(vertex);
	unsigned colorname = Vertex_color(vertex);
	assert(pos && norm);
	if (!colorname) {
		printf("%e %e %e %e %e %e .4 .4 .4 1 %e %e\n",
				Vec_coord(pos, 0), Vec_coord(pos, 1), Vec_coord(pos, 2),
				Vec_coord(norm, 0), Vec_coord(norm, 1), Vec_coord(norm, 2),
				.5*(1+Vertex_uv_x(vertex)), .5*(1+Vertex_uv_y(vertex)));
	} else {
		const Color *color = Grid_get_color(colorname);
		printf("%e %e %e %e %e %e %e %e %e 1 %e %e\n",
				Vec_coord(pos, 0), Vec_coord(pos, 1), Vec_coord(pos, 2),
				Vec_coord(norm, 0), Vec_coord(norm, 1), Vec_coord(norm, 2),
				color->r, color->g, color->b,
				.5*(1+Vertex_uv_x(vertex)), .5*(1+Vertex_uv_y(vertex)));
	}
}

static void print_facet(Facet *facet, const char *color) {
	assert(facet);
	unsigned nb_facetEdge = Facet_size(facet);
	printf("%u ", nb_facetEdge);
	for (unsigned j=0; j<nb_facetEdge; j++) {
		Vertex *vertex = Facet_get_vertex(facet, j);
		printf("%u ", order_of_vertex(vertex));
	}
	printf("%s\n", color);
}

static void print_basis(Basis *basis, unsigned color) {
	assert(basis && color<3);
	const char *colors[3][3] = {
		{ "1 0 0", "0 1 0", "0 0 1" },
		{ ".6 .4 .4", ".4 .6 .4", ".4 .4 .6" },
		{ "1 .4 .4", ".4 1 .4", ".4 .4 1" },
	};
	printf("{ appearance { linewidth 5 }\nVECT\n3 6 3\n2 2 2\n1 1 1\n");
	Vec p;
	for (unsigned a=0; a<3; a++) {
		printf("%e %e %e\n", Vec_coord(&basis->position, 0), Vec_coord(&basis->position, 1), Vec_coord(&basis->position, 2));
		Vec_add3(&p, &basis->position, &basis->axes[a]);
		printf("%e %e %e\n", Vec_coord(&p, 0), Vec_coord(&p, 1), Vec_coord(&p, 2));
	}
	for (unsigned c=0; c<3; c++) {
		printf("%s .8\n", colors[color][c]);
	}
	puts("}");
}

/* Public function */

int grid2gcl(unsigned sel_name, unsigned basis_name, const char *tiff_filename) {
	unsigned nb_vertices, nb_edges, nb_facets;
	bool show_facet_sel = sel_name>0 && Grid_get_selection_type(sel_name)==GridSel_FACET;
	hash_free();
	hash_init();
	register_reset();
	puts("(geometry grid { LIST ");
	if (Grid_get()) {
		printf("{ appearance { normscale .3 texture { file %s } }\n", tiff_filename);
		Grid_size(&nb_vertices, &nb_edges, &nb_facets);
		if (basis_name>0) {
			puts("appearance { -face }");
		}
		if (show_facet_sel) {
			puts("STNOFF");
		} else {
			puts("STCNOFF");
		}
		printf("%u %u %u\n", nb_vertices, nb_facets, nb_edges);
		unsigned i;
		/* vertices */
		Grid_reset_vertices();
		for (i=0; i<nb_vertices; i++) {
			Vertex *vertex = Grid_each_vertex();
			assert(vertex);
			register_vertex(vertex);
			if (show_facet_sel) {
				print_vertex_n_normal(vertex);
			} else {
				print_vertex_n_normal_n_color(vertex);
			}
		}
		assert(Grid_each_vertex() == NULL);
		/* Facets */
		Grid_reset_facets();
		for (i=0; i<nb_facets; i++) {
			Facet *facet = Grid_each_facet();
			register_facet(facet);
			print_facet(facet, show_facet_sel && Grid_selected(sel_name, facet) ? "1. 0. 0." : "");
		}
		assert(Grid_each_facet() == NULL);
		puts("}");
		/* Selection */
		if (sel_name>0) switch (Grid_get_selection_type(sel_name)) {
			case GridSel_VERTEX:
				{
					unsigned nb_v = Grid_selection_size(sel_name);
					if (nb_v > 0) {
						printf("{ appearance { linewidth 10 }\nVECT\n%d %d 1\n", nb_v, nb_v);
						for (unsigned i=0; i<nb_v; i++) printf("1 ");
						printf("\n1 ");
						for (unsigned i=1; i<nb_v; i++) printf("0 ");
						printf("\n");
						Grid_reset_selection(sel_name);
						Vertex *v;
						while ( (v=Grid_each_selected(sel_name)) ) {
							print_vertex(v);
						}
						printf("1. 0. 0. .8\n}\n");
					}
				}
				break;
			case GridSel_EDGE:
				if (0 == basis_name) {
					unsigned nb_e = Grid_selection_size(sel_name);
					if (nb_e > 0) {
						printf("{ appearance { linewidth 6 }\nVECT\n%d %d 1\n", nb_e, nb_e*2);
						for (unsigned i=0; i<nb_e; i++) printf("2 ");
						printf("\n1 ");
						for (unsigned i=1; i<nb_e; i++) printf("0 ");
						printf("\n");
						Grid_reset_selection(sel_name);
						Edge *e;
						while ( (e=Grid_each_selected(sel_name)) ) {
							print_vertex(Edge_get_vertex(e, SOUTH));
							print_vertex(Edge_get_vertex(e, NORTH));
						}
						printf("1. 0. 0. .8\n}\n");
					}
				}
				break;
			case GridSel_FACET:	/* already done */
				break;
		}
		/* Basis */
		if (basis_name>0) {
			Basis *basis = Grid_get_basis(basis_name);
			print_basis(basis, 0);
			unsigned father = Grid_get_basis_father(basis_name);
			if (father) {
				print_basis(Grid_get_basis(father), 1);
			}
			if (basis->is_instance) {
				print_basis(Grid_get_basis(basis->instance_of), 2);
			}
			if (nb_vertices>0) {
				printf("{ appearance { linewidth 7 }\nVECT\n%d %d %d\n", nb_vertices, nb_vertices, nb_vertices);
				for (unsigned i=0; i<nb_vertices; i++) printf("1 ");
				printf("\n");
				for (unsigned i=0; i<nb_vertices; i++) printf("1 ");
				printf("\n");
				Vertex *v;
				Grid_reset_vertices();
				while ( (v=Grid_each_vertex()) ) {
					print_vertex(v);
				}
				Grid_reset_vertices();
				while ( (v=Grid_each_vertex()) ) {
					unsigned bi = Vertex_basis(v);
					if (bi>0) {
						if (bi == basis_name) {
							if (0. == v->skin_ratio) {
								puts("0 1. 0 .7");
							} else {
								printf("0 %e 1 .7\n", 1.-v->skin_ratio);
							}
						} else if (Grid_get_basis_father(bi) == basis_name) {
							puts(".7 .7 .7 .7");
						}
					} else {
						puts(".1 .1 .1 .1");
					}
				}
				puts("}");
			}
		}
	} // if Grid_get()
	printf("})\n(bbox-draw grid no)\n");
	fflush(stdout);
	return 1;
}

Vertex *get_vertex_from_gcl_order(unsigned order) {
	assert(order2vertex);
	Vertex **v = cntList_get(order2vertex, order);
	assert(v);
	return *v;
}

Facet *get_facet_from_gcl_order(unsigned order) {
	assert(order2facet);
	Facet  **f = cntHash_get(order2facet, (cntHashkey){ .i = order });
	assert(f);
	return *f;
}

void grid2gcl_end(void) {
	hash_free();
}

// vi:ts=3:sw=3
