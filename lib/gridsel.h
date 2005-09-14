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
#ifndef GRIDSEL_H_050228
#define GRIDSEL_H_050228

#include <libcnt/vec.h>
#include "libmicromodel/grid.h"

int GridSel_construct(GridSel *this, GridSel_type type);
int GridSel_destruct(GridSel *this);
int GridSel_dup(GridSel *this, GridSel *source);
void GridSel_reset(GridSel *this);
void *GridSel_each(GridSel *this);
unsigned GridSel_size(GridSel *this);
// my_result must not be constructed
void GridSel_convert(GridSel *restrict this, GridSel *restrict my_result, GridSel_type type, GridSel_convert_type convert_type);
void GridSel_propagate(GridSel *this, unsigned level);
bool GridSel_selected(GridSel *this, void *elmnt);
void GridSel_center(GridSel *this, Vec *dest);
void GridSel_apply_homotecy(GridSel *this, Vec *center, Vec *axis, double ratio, void (*homotecy)(Vec *, Vec *, double));
int GridSel_set_hardskin(GridSel *this, unsigned basis);
int GridSel_set_softskin(GridSel *this, unsigned bi);
int GridSel_set_color(GridSel *this, unsigned color);
void GridSel_add_or_sub(GridSel *this, GridSel *src, bool add);
void GridSel_toggle_selection(GridSel *this);
GridSel GridSel_connect(GridSel *this, GridSel *restrict_to_facets, bool full_connect);
GridSel GridSel_extrude(GridSel *this, bool dir_vertex, Vec *direction, double ratio);
GridSel GridSel_bevel(GridSel *beveled_edges, GridSel *additionnal_vertices, double ratio);
// Returns the new and old edges
GridSel GridSel_smooth(GridSel *this, unsigned level, double softness);
// Returns the new facets
GridSel GridSel_separate(GridSel *this);
// Mapping application
void GridSel_mapping(GridSel *this, GridSel_mapping_type type, const Vec *pos, float scale_x, float scale_y, float offset_x, float offset_y, bool along_normals);
// Set all selected vertices to these uv coords
void GridSel_set_uv(GridSel *this, float uv_x, float uv_y);

#include <libcnt/hash.h>

struct GridSel {
	cntHash *elmnts;	// clefs = void *, pas de valeur
	GridSel_type type;
};

#include <stdlib.h>
#include <assert.h>
static inline void *GridSel_add(GridSel *this, void *element) {
	assert(this);
	return cntHash_put(this->elmnts, (cntHashkey){ .ptr = element }, NULL);
}
static inline int GridSel_remove(GridSel *this, void *element) {
	assert(this);
	return cntHash_remove(this->elmnts, (cntHashkey){ .ptr = element});
}
static inline int GridSel_toggle(GridSel *this, void *element) {
	assert(this);
	if (cntHash_get(this->elmnts, (cntHashkey){ .ptr = element})) {
		return GridSel_remove(this, element);
	} else {
		return NULL != GridSel_add(this, element);
	}
}
static inline void GridSel_clear(GridSel *this) {
	assert(this);
	cntHash_clear(this->elmnts);
}

#endif
// vi:ts=3:sw=3
