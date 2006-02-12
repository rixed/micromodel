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
#ifndef GRID_H_041124
#define GRID_H_041124

typedef struct Grid Grid;

typedef struct GridSel GridSel;
typedef enum { GridSel_VERTEX=0, GridSel_EDGE, GridSel_FACET } GridSel_type;
typedef enum { GridSel_MIN=0, GridSel_MAX } GridSel_convert_type;
typedef enum { GridSel_PLANAR, GridSel_CYLINDRIC, GridSel_SPHERICAL } GridSel_mapping_type;

#include <stdbool.h>
#include <libcnt/vec.h>

int Grid_new(void);
void Grid_del(void);
void Grid_set(Grid *this);
Grid *Grid_get(void);
unsigned Grid_get_version(void);
void Grid_set_carac_size(unsigned new_size);
unsigned Grid_get_carac_size(void);

int Grid_tetrahedron(unsigned name);
int Grid_cube(unsigned name);
int Grid_octahedron(unsigned name);
int Grid_icosahedron(unsigned name);
int Grid_dodecahedron(unsigned name);
int Grid_triangle(unsigned name);
int Grid_square(unsigned name);

void Grid_size(unsigned *nb_vertices, unsigned *nb_edges, unsigned *nb_facets);

void Grid_scale(unsigned selection, Vec *center, double ratio);
void Grid_stretch(unsigned selection, Vec *center, Vec *axis, double ratio);
void Grid_shear(unsigned selection, Vec *center, Vec *axis, double ratio);
void Grid_translate(unsigned selection, Vec *translation, double ratio);
void Grid_rotate(unsigned selection, Vec *center, Vec *axis, double angle);

int Grid_extrude(unsigned selection, bool dir_vertex, Vec *direction, double ratio, unsigned result_selection);
int Grid_extrude_1by1(unsigned selection, bool dir_vertex, Vec *direction, double ratio, double scale, unsigned result_selection);
int Grid_cut(unsigned selection, unsigned nb_cuts, unsigned result_selection);
int Grid_connect(unsigned selection, unsigned result_selection, bool full_connect);
int Grid_bevel(unsigned selection, unsigned result_selection, double ratio);
int Grid_bevsmooth(unsigned selection, unsigned level);
int Grid_smooth(unsigned selection, unsigned level, double softness, unsigned result_selection);
int Grid_zap(unsigned selection);
int Grid_plane_cut(unsigned selection, unsigned result_selection, Vec *center, Vec *normal);
int Grid_separate(unsigned selection, unsigned result_selection); 
int Grid_mirror(unsigned selection, unsigned result_selection);

int Grid_new_selection(unsigned name, GridSel_type type);
int Grid_del_selection(unsigned name);
int Grid_empty_selection(unsigned name);
int Grid_addsingle_to_selection(unsigned name, unsigned index);
int Grid_subsingle_from_selection(unsigned name, unsigned index);
int Grid_toggle_selection(unsigned name);
int Grid_add_to_selection(unsigned name_dest, unsigned name_src);
int Grid_sub_from_selection(unsigned name_dest, unsigned name_src);
int Grid_convert_selection(unsigned name, GridSel_type type, GridSel_convert_type convert_type);
int Grid_propagate_selection(unsigned name, unsigned level);
int Grid_replace_in_selections(GridSel_type type, void *old_elmnt, void *new_elmnt);

int Grid_new_basis(unsigned name, unsigned father, Vec *position, Vec *x, Vec *y, Vec *z);
int Grid_del_basis(unsigned name);
int Grid_set_vertex_basis(unsigned vertex, unsigned basis, float ratio);
int Grid_set_selection_hardskin(unsigned selection, unsigned basis);
int Grid_set_selection_softskin(unsigned selection, unsigned basis);
int Grid_mapping(unsigned selection, GridSel_mapping_type type, const Vec *pos, float scale_x, float scale_y, float offset_x, float ofset_y, bool along_normals);
int Grid_set_uv(unsigned selection, float uv_x, float uv_y);
int Grid_set_instance(unsigned name, unsigned original, bool recursive);
bool Grid_basis_is_instance(unsigned name);
unsigned Grid_basis_original(unsigned name);

int Grid_new_color(unsigned name, float r, float g, float b);
int Grid_del_color(unsigned name);
int Grid_set_selection_color(unsigned selection, unsigned color);

#include <libmicromodel/facet.h>
#include <libmicromodel/edge.h>
#include <libmicromodel/vertex.h>
#include <libmicromodel/basis.h>
#include <libmicromodel/color.h>

Edge *Grid_edge_cut(Edge *edge, double ratio);
void Grid_replace_vertex(Vertex *v, Vertex *rep);
void Grid_replace_edge(Edge *e, Edge *rep);
void Grid_replace_facet(Facet *f, Vertex *rep);
Facet *Grid_get_facet(unsigned index);
Edge *Grid_get_edge(unsigned index);
Edge *Grid_edge_index_from_vertices(Vertex *v1, Vertex *v2);
Vertex *Grid_get_vertex(unsigned index);
void Grid_reset_vertices(void);
Vertex *Grid_each_vertex(void);
void Grid_reset_facets(void);
Facet *Grid_each_facet(void);
void Grid_reset_edges(void);
Edge *Grid_each_edge(void);
void Grid_reset_selections(void);
unsigned Grid_each_selection(void);
bool Grid_selected(unsigned name, void *elmnt);
unsigned Grid_selection_size(unsigned name);
void Grid_reset_selection(unsigned name);
void *Grid_each_selected(unsigned name);
GridSel_type Grid_get_selection_type(unsigned name);
Basis *Grid_get_basis(unsigned name);
void Grid_reset_bases(void);
unsigned Grid_each_basis(void);
unsigned Grid_get_basis_father(unsigned name);
const Vec *Grid_basis_axis(unsigned name, unsigned dim);
const Vec *Grid_basis_center(unsigned name);
Color *Grid_get_color(unsigned name);
void Grid_reset_colors(void);
unsigned Grid_each_color(void);

Vertex *Grid_vertex_new(const Vec *position, unsigned bi, float ratio, float uv_x, float uv_y);
Vertex *Grid_vertex_average_new(const Vertex *v1, const Vertex *v2, double ratio);
Edge *Grid_edge_new(Vertex *v1, Vertex *v2);
Facet *Grid_facet_new(unsigned size, Edge **edges, bool direct);

#endif
// vi:ts=3:sw=3
