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
#ifndef BASIS_H_041220
#define BASIS_H_041220

#include <libcnt/vec.h>

typedef struct Basis Basis;

int Basis_construct(Basis *this, unsigned father, Vec *pos, Vec *x, Vec *y, Vec *z);
void Basis_destruct(Basis *this);
const Vec *Basis_position(Basis *this);
void Basis_remove_instance(Basis *this);
Basis *Basis_get_father(Basis *this);

#include <stdbool.h>

struct Basis {
	unsigned father;
	Vec axes[3];
	Vec position;
	bool is_instance, recursive_instance;
	unsigned instance_of;
};

#endif
// vi:ts=3:sw=3
