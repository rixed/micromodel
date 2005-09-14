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
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libcnt/cnt.h>
#include "libmicromodel/grid.h"

static int build_pantin(void) {
	int ret = 0;
	if (!Grid_cube(0)) goto ret1;
#	define NONE 0
#	define S0 1
#	define S1 2
	if ( ! (
		Grid_new_selection(S0, GridSel_EDGE) &&
		Grid_addsingle_to_selection(S0, 8) &&
		Grid_addsingle_to_selection(S0, 11) &&
		Grid_cut(S0, 1, S1) &&	// for shoulder -> S1 of VERTEX type
		Grid_connect(S1, NONE, false) &&
		// extrude both the leg and the arm
		Grid_empty_selection(S0) &&
		Grid_convert_selection(S0, GridSel_FACET, GridSel_MIN) &&
		Grid_addsingle_to_selection(S0, 4) &&
		Grid_addsingle_to_selection(S0, 6) &&
		Grid_extrude_1by1(S0, false, NULL, 1., 0.9, NONE) &&
		Grid_extrude(S0, false, NULL, 1., NONE) &&
		Grid_extrude(S0, false, NULL, 0.2, NONE)
	)) {
		Grid_del();
		goto ret1;
	}
	ret = 1;
ret1:
	return ret;
}

int main(void) {
	int ret = EXIT_FAILURE;
	if (!cnt_init(1024, LOG_DEBUG)) return ret;
	atexit(cnt_end);
	if (!build_pantin()) goto exit;
	Grid_del();
	ret = EXIT_SUCCESS;
exit:
	return ret;
}
// vi:ts=3:sw=3
