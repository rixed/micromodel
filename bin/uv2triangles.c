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
#include <assert.h>
#include <GL/gl.h>
#include "uv2triangles.h"
#include "libmicromodel/grid.h"
#include "libmicromodel/vertex.h"

/*
 * Data Definitions
 */

/*
 * Private Functions
 */

/*
 * Public Functions
 */

int uv2triangles(unsigned sel_name) {
	if (!Grid_get()) return 0;
	Grid_reset_vertices();
	Vertex *v1;
	glDisable(GL_TEXTURE_2D);
	glColor3f(1., 1., 1.);
	glBegin(GL_LINES);
	while ( (v1 = Grid_each_vertex()) ) {
		assert(v1);
		for (unsigned i=0; i<Vertex_size(v1); i++) {
			Vertex *v2 = Vertex_get_vertex(v1, i);
			if (v1 < v2) {
				glVertex3f(Vertex_uv_x(v1), Vertex_uv_y(v1), 0.);
				glVertex3f(Vertex_uv_x(v2), Vertex_uv_y(v2), 0.);
			}
		}
	}
	glEnd();
	// tracer la selection
	if (sel_name) {
		glColor4f(1., .1, .1, .5);
		Grid_reset_selection(sel_name);
		switch (Grid_get_selection_type(sel_name)) {
			case GridSel_VERTEX:
				{
					double delta = .005;
					Vertex *v;
					while ( (v = Grid_each_selected(sel_name)) ) {
						glBegin(GL_LINE_LOOP);
						glVertex3f(Vertex_uv_x(v)-delta, Vertex_uv_y(v)-delta, 0.);
						glVertex3f(Vertex_uv_x(v)-delta, Vertex_uv_y(v)+delta, 0.);
						glVertex3f(Vertex_uv_x(v)+delta, Vertex_uv_y(v)+delta, 0.);
						glVertex3f(Vertex_uv_x(v)+delta, Vertex_uv_y(v)-delta, 0.);
						glEnd();
					}
				}
				break;
			case GridSel_EDGE:
				glLineWidth(2.);
				glBegin(GL_LINES);
				Edge *e;
				while ( (e = Grid_each_selected(sel_name)) ) {
					glVertex3f(Vertex_uv_x(Edge_get_vertex(e, SOUTH)), Vertex_uv_y(Edge_get_vertex(e, SOUTH)), 0.);
					glVertex3f(Vertex_uv_x(Edge_get_vertex(e, NORTH)), Vertex_uv_y(Edge_get_vertex(e, NORTH)), 0.);
				}
				glEnd();
				break;
			case GridSel_FACET:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_POLYGON);
				Facet *f;
				while ( (f = Grid_each_selected(sel_name)) ) {
					for (unsigned i=0; i<Facet_size(f); i++) {
						Vertex *v = Facet_get_vertex(f, i);
						glVertex3f(Vertex_uv_x(v), Vertex_uv_y(v), 0.);
					}
				}
				glEnd();
				glDisable(GL_BLEND);
				break;
		}
	}
	return 1;
}

