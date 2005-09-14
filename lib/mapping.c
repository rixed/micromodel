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
#include <math.h>
#include <assert.h>
#include <float.h>
#include <libcnt/vec.h>
#include "gridsel.h"

/*
 * Data Definitions
 */

typedef struct Mapping {
	GridSel_mapping_type type;
	const Vec *pos;
	struct {
		double x, y;
	} offset, scale;
} Mapping;

/*
 * Private Functions
 */

static void Mapping_construct(Mapping *this, GridSel_mapping_type type, const Vec *pos, float scale_x, float scale_y, float offset_x, float offset_y) {
	assert(this);
	this->type = type;
	this->pos = pos;
	this->scale.x = scale_x;
	this->scale.y = scale_y;
	this->offset.x = offset_x;
	this->offset.y = offset_y;
}
static void Mapping_destruct(Mapping *this) {}

static void Mapping_get_projection(Mapping *this, Vertex *v, float *uv_x, float *uv_y, bool along_normals) {
	assert(this && v && uv_x && uv_y);
	const Vec *vp = Vertex_position(v);
	Vec Mp = *this->pos;
	Vec Mx = vec_x, My = vec_y, Mz = vec_z;
	double r, d;
	if (Mp.c[0] > Mp.c[1] && Mp.c[0] != 0.) {
		r = Mp.c[1]/Mp.c[0];
		Mx.c[1] = sqrt(1./(1.+r));
		Mx.c[0] = -Mx.c[1]*r;
	} else if (Mp.c[1] > Mp.c[0] && Mp.c[1] != 0.) {
		r = Mp.c[0]/Mp.c[1];
		Mx.c[0] = sqrt(1./(1.+r));
		Mx.c[1] = -Mx.c[0]*r;
	}
	if (Mx.c[0] > Mx.c[1]) {
		r = Mx.c[1]/Mx.c[0];
		d = Mp.c[0]*r-Mp.c[1];
		if (d != 0.) {	// otherwise default values are OK
			My.c[2] = sqrt(1./(1.+Mp.c[2]*Mp.c[2]*(1+r*r)/(d*d)));
			My.c[1] = My.c[2]*Mp.c[2]/d;
			My.c[0] = -My.c[1]*r;
		}
	} else {
		r = Mx.c[0]/Mx.c[1];
		d = Mp.c[1]*r-Mp.c[0];
		if (d != 0.) {
			My.c[2] = sqrt(1./(1.+Mp.c[2]*Mp.c[2]*(1+r*r)/(d*d)));
			My.c[0] = My.c[2]*Mp.c[2]/d;
			My.c[1] = -My.c[0]*r;
		}
	}
	double M_dist = Vec_norm(&Mz);
	if (M_dist > 0.) {
		Vec_scale3(&Mz, &Mp, -1./M_dist);
	}
	double vx = Vec_scalar(vp, &Mx);
	double vy = Vec_scalar(vp, &My);
	double vz = Vec_scalar(vp, &Mz);
	Vec n = vec_origin;
	if (along_normals) {
		const Vec *vn = Vertex_normal(v);
		Vec_coord_set(&n, 0, Vec_scalar(vn, &Mx));
		Vec_coord_set(&n, 1, Vec_scalar(vn, &My));
		Vec_coord_set(&n, 2, Vec_scalar(vn, &Mz));
	} else {
		switch (this->type) {
			case GridSel_PLANAR:
				Vec_coord_set(&n, 2, 1.);
				break;
			case GridSel_SPHERICAL:
				Vec_coord_set(&n, 2, vz);
				// pass
			case GridSel_CYLINDRIC:
				Vec_coord_set(&n, 0, vx);
				Vec_coord_set(&n, 1, vy);
				Vec_normalize(&n);
				break;
		}
	}
	switch (this->type) {
		case GridSel_PLANAR:
			if (Vec_coord(&n, 2) != 0) {
				double h = M_dist + vz;
				*uv_x = vx + h*Vec_coord(&n, 0)/Vec_coord(&n, 2);
				*uv_y = vy + h*Vec_coord(&n, 1)/Vec_coord(&n, 2);
			} else {
				*uv_x = 0;
				*uv_y = 0;
			}
			break;
		case GridSel_CYLINDRIC:
			{
				double a = Vec_coord(&n, 0)*Vec_coord(&n, 0)+Vec_coord(&n, 1)*Vec_coord(&n, 1);
				double b = 2*(vx*Vec_coord(&n, 0)+vy*Vec_coord(&n, 1));
				double c = vx*vx+vy*vy-M_dist*M_dist;
				double alpha = (sqrt(b*b-4*a*c) -b) / (2*a);
				double x = vx+Vec_coord(&n, 0)*alpha;
				double y = vy+Vec_coord(&n, 1)*alpha;
				double ang = acos(x/M_dist);
				if (y<0) ang = -ang;
				*uv_x = ang/M_PI;
				*uv_y = vz +Vec_coord(&n, 2)*alpha;
			}
			break;
		case GridSel_SPHERICAL:
			{
				double a = Vec_norm2(&n);
				double b = 2*(vx*Vec_coord(&n, 0)+vy*Vec_coord(&n, 1)+vz*Vec_coord(&n, 2));
				double c = vx*vx+vy*vy+vz*vz-M_dist*M_dist;
				double alpha = (sqrt(b*b-4*a*c) -b) / (2*a);
				double x = vx+Vec_coord(&n, 0)*alpha;
				double y = vy+Vec_coord(&n, 1)*alpha;
				double z = vz+Vec_coord(&n, 2)*alpha;
				double r = x*x+y*y;
				if (r > 0) {
					double ang = acos(x/sqrt(r));
					if (y<0) ang = -ang;
					*uv_x = ang/M_PI;
				} else {
					*uv_x = 0;
				}
				r += z*z;
				if (r > 0) {
					double ang = asin(z/sqrt(r));
					*uv_y = ang/(.5*M_PI);
				} else {
					*uv_y = 0;
				}
			}
			break;
		default:
			assert(0);
	}
	if (!isfinite(*uv_x)) *uv_x = 0.;
	if (!isfinite(*uv_y)) *uv_y = 0.;
	*uv_x += this->offset.x;
	*uv_y += this->offset.y;
	*uv_x *= this->scale.x;
	*uv_y *= this->scale.y;
}

/*
 * Public Functions
 */

void GridSel_mapping(GridSel *this, GridSel_mapping_type type, const Vec *pos, float scale_x, float scale_y, float offset_x, float offset_y, bool along_normals) {
	assert(this && pos);
	GridSel vertices;
	GridSel_convert(this, &vertices, GridSel_VERTEX, GridSel_MIN);
	Mapping mapping;
	Mapping_construct(&mapping, type, pos, scale_x, scale_y, offset_x, offset_y);
	GridSel_reset(&vertices);
	Vertex *v;
	while ( (v=GridSel_each(&vertices)) ) {
		float uv_x, uv_y;
		Mapping_get_projection(&mapping, v, &uv_x, &uv_y, along_normals);
		Vertex_set_uv_mapping(v, uv_x, uv_y);
	}
	Mapping_destruct(&mapping);
	GridSel_destruct(&vertices);
}

void GridSel_set_uv(GridSel *this, float uv_x, float uv_y) {
	assert(this);
	GridSel vertices;
	GridSel_convert(this, &vertices, GridSel_VERTEX, GridSel_MIN);
	GridSel_reset(&vertices);
	Vertex *v;
	while ( (v=GridSel_each(&vertices)) ) {
		Vertex_set_uv_mapping(v, uv_x, uv_y);
	}
	GridSel_destruct(&vertices);
}

// vi:ts=3:sw=3
