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
#include <libcnt/cnt.h>
#include "libmicromodel/mml2bin.h"
#include "libmicromodel/grid.h"

static const char pantin[] =
"cube	\\0\n"
"newsel	edge\n"
"select	\\1	8\n"
"select	\\1	11\n"
"cut	\\1	1	new\n"
"connect	\\2	\\0	\\0\n"
"empty	\\1\n"
"convert	\\1	facet	0\n"
"select	\\1	4\n"
"select	\\1	6\n"
"extr1	\\1	0	0,0,0	1.	1. \\0\n"
"extr	\\1	0	0,0,0	1.	\\0\n"
"extr	\\1	0	0,0,0	.2	\\0\n";

int main(void) {
	int ret = EXIT_FAILURE;
	if (!cnt_init(1024, LOG_DEBUG)) return ret;
	atexit(cnt_end);
	unsigned size;
	unsigned char *bin = mml2bin(pantin, &size, NULL);
	if (!bin) goto exit;
	mem_unregister(bin);
	printf("Resulting size : %u bytes\n", size);
	ret = EXIT_SUCCESS;
exit:
	return ret;
}
// vi:ts=3:sw=3
