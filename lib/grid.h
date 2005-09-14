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
#ifndef GRID_H_PRIV_050622
#define GRID_H_PRIV_050622

#include "libmicromodel/grid.h"
#include "gridsel.h"

GridSel *Grid_get_selection(unsigned name);
GridSel *Grid_new_selection_(unsigned name, GridSel_type type);
void output_selection(unsigned selection, GridSel *sel, unsigned result_selection, GridSel *my_result);

#endif
