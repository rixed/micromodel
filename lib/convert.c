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
#include <assert.h>
#include <libcnt/hash.h>
#include <libcnt/log.h>
#include "libmicromodel/grid.h"
#include "gridsel.h"

/* Private Functions */

/* Public Functions */

void GridSel_convert(GridSel *restrict this, GridSel *restrict my_result, GridSel_type type, GridSel_convert_type convert_type) {
	assert(this && my_result && this!=my_result);
	cntHashkey key;
	if (
			(type == GridSel_VERTEX && this->type == GridSel_FACET) ||
			(type == GridSel_FACET && this->type == GridSel_VERTEX)
		) {
		GridSel tmp;
		GridSel_convert(this, &tmp, GridSel_EDGE, convert_type);
		GridSel_convert(&tmp, my_result, type, convert_type);
		GridSel_destruct(&tmp);
		return;
	} else {
		GridSel_construct(my_result, type);
		if (type == this->type) {	// simple copy
			GridSel_reset(this);
			while ( (key.ptr=GridSel_each(this)) ) {
				cntHash_put(my_result->elmnts, key, NULL);
			}
		} else {
			cntHash *h = cntHash_new(sizeof(int), 1+GridSel_size(this), 4, cntHash_PTRKEYS, 0);
			assert(h);
			static const int count_init = 1;
			switch (this->type) {
				case GridSel_VERTEX:
					{
						assert(type == GridSel_EDGE);
						GridSel_reset(this);
						Vertex *v;
						int *count_ptr;
						while ( (v=GridSel_each(this)) ) {
							for (unsigned i=0; i<Vertex_size(v); i++) {
								Edge *e = Vertex_get_edge(v, i);
								key.ptr=e;
								count_ptr = cntHash_get(h, key);
								if (!count_ptr) {
									cntHash_put(h, key, &count_init);
								} else {
									++*count_ptr;
								}
							}
						}
						cntHash_reset(h);
						void *ptr;
						while (cntHash_each(h, &key, &ptr)) {
							count_ptr = ptr;
							if (convert_type==GridSel_MIN && *count_ptr<2) continue;
							cntHash_put(my_result->elmnts, key, NULL);
						}
					}
					break;
				case GridSel_EDGE:
					switch (type) {
						case GridSel_VERTEX:
							{
								GridSel_reset(this);
								Edge *e;
								while ( (e=GridSel_each(this)) ) {
									for (unsigned i=SOUTH; i<=NORTH; i++) {
										Vertex *v = Edge_get_vertex(e, i);
										key.ptr=v;
										cntHash_put(my_result->elmnts, key, NULL);
									}
								}
							}
							break;
						case GridSel_FACET:
							{
								GridSel_reset(this);
								Edge *e;
								int *count_ptr;
								while ( (e=GridSel_each(this)) ) {
									for (unsigned i=WEST; i<=EAST; i++) {
										Facet *f = Edge_get_facet(e, i);
										key.ptr=f;
										count_ptr = cntHash_get(h, key);
										if (!count_ptr) {
											cntHash_put(h, key, &count_init);
										} else {
											++*count_ptr;
										}
									}
								}
								cntHash_reset(h);
								void *ptr;
								while (cntHash_each(h, &key, &ptr)) {
									count_ptr = ptr;
									if (convert_type==GridSel_MIN && *count_ptr<(signed)Facet_size(key.ptr)) continue;
									cntHash_put(my_result->elmnts, key, NULL);
								}
							}
							break;
						default:
							assert(0);
					}
					break;
				case GridSel_FACET:
					{
						assert(type == GridSel_EDGE);
						GridSel_reset(this);
						Facet *f;
						while ( (f=GridSel_each(this)) ) {
							for (unsigned i=0; i<Facet_size(f); i++) {
								Edge *e = Facet_get_edge(f, i);
								key.ptr=e;
								cntHash_put(my_result->elmnts, key, NULL);
							}
						}
					}
					break;
				default:
					assert(0);
			}
			cntHash_del(h);
		}
	}
}

void GridSel_propagate(GridSel *this, unsigned level) {
	assert(this);
	log_warning(LOG_DEBUG, "propagate this = %p", this);
	GridSel_type initial_type = this->type;
	GridSel tmp_result;
	while (level--) {
		GridSel_convert(this, &tmp_result, GridSel_VERTEX, GridSel_MAX /* unused */);
		GridSel_destruct(this);
		GridSel_convert(&tmp_result, this, GridSel_FACET, GridSel_MAX);
		GridSel_destruct(&tmp_result);
	}
	if (this->type != initial_type) {
		GridSel_convert(this, &tmp_result, initial_type, GridSel_MIN);
		GridSel_destruct(this);
		*this = tmp_result;
	}
}

