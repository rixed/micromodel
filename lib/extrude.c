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
#include <libcnt/hash.h>
#include "libmicromodel/grid.h"
#include "libmicromodel/basis.h"
#include "libmicromodel/vertex.h"
#include "libmicromodel/facet.h"
#include "libmicromodel/edge.h"
#include "gridsel.h"

/* Data Definitions */

/* Private Functions */

// Extrude a single zone. A zone is formed by facets all joinable by a path included in the zone, a
// path beeing composed of facets sharing at least one edge (and *not* a single vertex).
static GridSel extrude_zone(GridSel *this, bool dir_vertex, Vec *direction, double ratio) {
	// 0) Inits
	// Assertions (selection valide)
	// Construire la sélection de Facets my_result
	// Sauvegarder les normales
	assert(this && this->type==GridSel_FACET);
	GridSel my_result;
	GridSel_construct(&my_result, GridSel_FACET);
	// 1) Détachement
	// Construire la sélection 'edges' par une conversion de 'this' vers le type Edge, MIN ou MAX sans importance
	// Boucler sur tous les edges de la sélection
	// 	Boucler sur S in (WEST, EAST)
	// 		F = Edge_get_facet(e, S), et F' = Edge_get_facet(e, !S)
	// 		Si F est dans la sélection, et que F' n'est pas dans la sélection
	// 			Pour v in (v1, v2) de edge
	// 				Si h{v} existe, récuperer h{v} = (v1/2_n, e1/2_n)
	// 				Sinon
	// 					Creer v1/2_n = duplication de v1/2
	// 					Creer e1/2_n de v1/2 à v1/2_n
	// 					Insérer dans h{v}
	// 			Creer e_n de v1_n à v2_n, avec uniquement F comme facets
	// 			Changer le edge de F de e à e_n
	// 			Creer la facet f_n (e, e2_n, e_n, e1_n) dont les sides dépendent du side S, et signaler cette f_n à ces 4 edges
	// 			Ajouter cette facet à my_result
	// Détruire h{v}
	GridSel edges;
	GridSel_convert(this, &edges, GridSel_EDGE, GridSel_MIN);
	GridSel_reset(&edges);
	Edge *e;
	cntHashkey key;
	typedef struct {
		Vertex *vertex;
		Edge *edge;
		Vec normal;
	} hValue;
	cntHash *h = cntHash_new(sizeof(hValue), 1+GridSel_size(&edges), 3, cntHash_PTRKEYS, 0);
	assert(h);
	while ( (e = GridSel_each(&edges)) ) {
		for (unsigned s=WEST; s<=EAST; s++) {
			Facet *f = Edge_get_facet(e, s);
			Facet *f_= Edge_get_facet(e,!s);
			assert(f);
			if (GridSel_selected(this, f) && !GridSel_selected(this, f_)) {
				Vertex *vn[2];
				Edge *en[2];
				for (unsigned p=SOUTH; p<=NORTH; p++) {
					Vertex *v = Edge_get_vertex(e, p);
					assert(v);
					key.ptr = v;
					hValue *value;
					if ( (value = cntHash_get(h, key)) ) {
						vn[p] = value->vertex;
						en[p] = value->edge;
					} else {
						vn[p] = Grid_vertex_new(Vertex_position(v), v->basis, v->skin_ratio, v->uv_x, v->uv_y);
						en[p] = Grid_edge_new(vn[p], v);
						hValue new_value = { .vertex=vn[p], .edge=en[p], .normal=*Vertex_normal(v) };
						if (!cntHash_put(h, key, &new_value)) assert(0);
						// Boucler sur toutes les connections du vertex v: tous les edges non sélectionnés vont maintenant pointer sur vn[p]
						// refait en passant les connections de v et de vn[p]
						// La création des edges en[x] et e_n se chargera de signaler leurs connexions propres
						for (unsigned c=0; c<Vertex_size(v); c++) {
							Edge *connected = Vertex_get_edge(v, c);
							if (GridSel_selected(&edges, connected)) continue;
							for (unsigned pp=SOUTH; pp<=NORTH; pp++) {
								if (Edge_get_vertex(connected, pp)==v) {
									Vertex_remove_connection(v, connected);
									Edge_set_vertex(connected, pp, vn[p]);
									Vertex_add_edge(vn[p], connected);
									c--;	// v lost this connection
									break;
								}
							}
						}
					}
				}
				Edge *e_n = Grid_edge_new(vn[SOUTH], vn[NORTH]);
				assert(e_n);
				Facet_change_edge(f_, e, e_n, 0);
				Edge *edges[4] = { e, en[s], e_n, en[!s] };
				GridSel_add(&my_result, Grid_facet_new(4, edges, true));
			}
		}
	}
	// 2) Déplacement
	// Si direction, construire le Vec dir = direction normalisé, multiplié par ratio
	// Construire la sélection de Vertex 'vertices' en convertissant 'edges' vers Vertex, MIN ou MAX sans importance
	// Boucler sur tous les vertices de la selection 'vertices'
	// 	Si direction, ajouter dir
	// 	Sinon, si dir_vertex, ajouter normale au vertex multipliée par ratio
	// 	Sinon, ajouter la normale à la face multipliée par ratio
	// 		(dans ce cas, une seule face utilise le vertex - pour le retrouver, prendre la première face non nulle du premier edge)
	if (direction && direction->c[0]==0. && direction->c[1]==0. && direction->c[2]==0.) {	// facility for mcommander
		direction = NULL;
	}
	GridSel vertices;
	GridSel_convert(&edges, &vertices, GridSel_VERTEX, GridSel_MAX);
	GridSel_destruct(&edges);
	Vertex *v;
	GridSel_reset(&vertices);
	if (direction) {
		Vec dir;
		if (direction) {
			dir = *direction;
			Vec_normalize(&dir);
			Vec_scale(&dir, ratio);
		}
		while ( (v=GridSel_each(&vertices)) ) {
			Vec_add(Vertex_position(v), &dir);
		}
	} else if (dir_vertex) {
		while ( (v=GridSel_each(&vertices)) ) {
			Vec dir;
			hValue *hv = cntHash_get(h, (cntHashkey){ .ptr = v });
			if (hv) {
				dir = hv->normal;
			} else {
				dir = *Vertex_normal(v);
			}
			Vec_scale(&dir, ratio);
			Vec_add(Vertex_position(v), &dir);
		}
	} else {
		Vec normals[GridSel_size(&vertices)];
		unsigned n = 0;
		while ( (v=GridSel_each(&vertices)) ) {
			normals[n] = vec_origin;
			for (unsigned i=0; i<Vertex_size(v); i++) {
				Facet *f = Vertex_get_facet(v, i);
				assert(f);
				if (GridSel_selected(this, f)) {
					Vec_add(normals+n, Facet_normal(f));
				}
			}
			Vec_normalize(normals+n);
			Vec_scale(normals+n, ratio);
			n++;
		}
		n = 0;
		GridSel_reset(&vertices);
		while ( (v=GridSel_each(&vertices)) ) {
			Vec_add(Vertex_position(v), normals+n);
			n++;
		}
	}
	// 3) Fin
	// Détruire les selections 'edges' et 'vertices'
	// Renvoyer my_result
	cntHash_del(h);
	GridSel_destruct(&vertices);
	return my_result;
}


/* Public Functions */

// Subdivide this into zones (see above what is a zone if you mind),
// and then extrude each one.
GridSel GridSel_extrude(GridSel *this, bool dir_vertex, Vec *direction, double ratio) {
	assert(this && this->type==GridSel_FACET);
	GridSel my_result;
	GridSel_construct(&my_result, GridSel_FACET);
	GridSel facets;
	GridSel_convert(this, &facets, GridSel_FACET, GridSel_MAX);
	while (GridSel_size(&facets)) {
		GridSel zone, zone_toadd, zone_added;
		GridSel_construct(&zone, GridSel_FACET);
		GridSel_construct(&zone_toadd, GridSel_FACET);
		GridSel_construct(&zone_added, GridSel_FACET);
		GridSel_reset(&facets);
		Facet *fzone = GridSel_each(&facets);
		GridSel_remove(&facets, fzone);
		GridSel_add(&zone, fzone);
		GridSel_add(&zone_added, fzone);
		GridSel_reset(&zone_added);
		Facet *f;
		do {
			while ( (f = GridSel_each(&zone_added)) ) {
				for (unsigned i=0; i<Facet_size(f); i++) {
					Facet *neigh = Facet_get_facet(f, i);
					if (GridSel_selected(&facets, neigh)) {
						GridSel_remove(&facets, neigh);
						GridSel_add(&zone_toadd, neigh);
					}
				}
			}
			GridSel_clear(&zone_added);
			GridSel_reset(&zone_toadd);
			while ( (f = GridSel_each(&zone_toadd)) ) {
				GridSel_add(&zone, f);
				GridSel_add(&zone_added, f);
			}
			GridSel_clear(&zone_toadd);
		} while (GridSel_size(&zone_added));
		GridSel tmp_result = extrude_zone(&zone, dir_vertex, direction, ratio);
		GridSel_add_or_sub(&my_result, &tmp_result, true);
		GridSel_destruct(&tmp_result);
		GridSel_destruct(&zone);
		GridSel_destruct(&zone_toadd);
		GridSel_destruct(&zone_added);
	}
	GridSel_destruct(&facets);
	return my_result;
}

// vi:ts=3:sw=3
