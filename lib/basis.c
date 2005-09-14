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
#include "libmicromodel/basis.h"
#include <stdbool.h>
#include <assert.h>
#include "libmicromodel/grid.h"

int Basis_construct(Basis *this, unsigned father, Vec *pos, Vec *x, Vec *y, Vec *z) {
	assert(this && x && y && z && pos);
	this->father = father;
	this->position = *pos;
	this->axes[0] = *x;
	this->axes[1] = *y;
	this->axes[2] = *z;
	this->is_instance = false;
	return 1;
}
void Basis_destruct(Basis *this) {}
const Vec *Basis_position(Basis *this) {
	assert(this);
	return &this->position;
}
void Basis_remove_instance(Basis *this) {
	assert(this);
	this->is_instance = false;
}
Basis *Basis_get_father(Basis *this) {
	assert(this);
	Basis *father;
	if (this->father) {
		father = Grid_get_basis(this->father);
		assert(father);
	}
	return father;
}

// vi:ts=3:sw=3
