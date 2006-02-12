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
#include "libmicromodel/mcommander.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <libcnt/list.h>
#include <libcnt/log.h>
#include "libmicromodel/grid.h"

/* Data Definitions */

static const char *type_names[MCom_NB_TYPES] = {
	"Vector", "Selection", "Index", "Integer", "Version", "Real", "Choice", "Geometry type", "Basis", "Color"
};
static bool ret_was_found;

#define NB_PRIMITIVES 7
static int tetrahedron(void);
static int cube(void);
static int octahedron(void);
static int icosahedron(void);
static int dodecahedron(void);
static int triangle(void);
static int square(void);

static int extrude(void);
static int extrude_1by1(void);
static int cut(void);
static int plane_cut(void);
static int connect(void);
static int separate(void);
static int bevel(void);
static int mirror(void);
static int bevsmooth(void);
static int smooth(void);
static int zap(void);

static int new_selection(void);
static int addsingle_to_selection(void);
static int subsingle_from_selection(void);
static int add_to_selection(void);
static int sub_from_selection(void);
static int empty_selection(void);
static int toggle_selection(void);
static int convert_selection(void);
static int propagate_selection(void);

static int scale(void);
static int stretch(void);
static int shear(void);
static int translate(void);
static int rotate(void);

static int new_basis(void);
static int hardskin(void);
static int softskin(void);
static int planmap(void);
static int cylmap(void);
static int sphermap(void);
static int planmap_ortho(void);
static int cylmap_ortho(void);
static int sphermap_ortho(void);
static int set_uv(void);
static int instanciate(void);
static int new_color(void);
static int paint(void);
static int ret(void);
static int version(void);

/* Note : Beware to order the instructions by descending strlen, otherwise the quick'n dirty strncpys of mml2bin may fail !
 *        (for extr vs extr1 !)
 */
static const struct {
	int (*execute)(void);
	const char *name;
	const char *description;
	const char *instruction;
	unsigned nb_params;
	struct {
		MCom_param_type type;
		const char *name;
	} params[MAX_NB_PARAMS];
} commands[] = {
	{
		tetrahedron,
		"Tetra.",
		"This creates a new grid\nin the shape of a tetrahedron",
		"tetrahedron",
		1,
		{
			{ MCom_SEL, "To store the result" },
		}
	}, {
		cube,
		"Cube",
		"This creates a new grid\nin the shape of a cube",
		"cube",
		1,
		{
			{ MCom_SEL, "To store the result" },
		}
	}, {
		octahedron,
		"Octa.",
		"This creates a new grid\nin the shape of an octahedron",
		"octahedron",
		1,
		{
			{ MCom_SEL, "To store the result" },
		}
	}, {
		icosahedron,
		"Icosa.",
		"This creates a new grid\nin the shape of an icosahedron",
		"icosahedron",
		1,
		{
			{ MCom_SEL, "To store the result" },
		}
	}, {
		dodecahedron,
		"Dodec.",
		"This creates a new grid\nin the shape of an dodecahedron",
		"dodecahedron",
		1,
		{
			{ MCom_SEL, "To store the result" },
		}
	}, {
		triangle,
		"Triangle",
		"This creates a flat equilateral triangle pointing towards X",
		"triangle",
		1,
		{
			{ MCom_SEL, "To store the result" },
		}
	}, {
		square,
		"Square",
		"This creates a flat square\n",
		"square",
		1,
		{
			{ MCom_SEL, "To store the result" },
		}
	}, {
		extrude_1by1,
		"Extrude (disjoint)",
		"Extrude the selected facets\nfacets by facets",
		"extr1",
		6,
		{
			{ MCom_SEL, "Extrude this selection" },
			{ MCom_BOOL, "Along facets / vertices" },
			{ MCom_VEC, "Or Along this direction" },
			{ MCom_REAL, "Distance" },
			{ MCom_REAL, "Scale" },
			{ MCom_SEL, "To store created faces" },
		}
	}, {
		extrude,
		"Extrude",
		"Extrude the selected facets",
		"extr",
		5,
		{
			{ MCom_SEL, "Extrude this selection" },
			{ MCom_BOOL, "Along vertices / facets" },
			{ MCom_VEC, "Or Along this direction" },
			{ MCom_REAL, "Distance" },
			{ MCom_SEL, "To store created faces" },
		}
	}, {
		cut,
		"Cut",
		"Cut the given edges",
		"cut",
		3,
		{
			{ MCom_SEL, "The edges to cut" },
			{ MCom_INT, "How many cuts" },
			{ MCom_SEL, "To store new vertices" },
		}
	}, {
		plane_cut,
		"Cut by plane",
		"Cut the given selection\nby the given plane",
		"planecut",
		4,
		{
			{ MCom_SEL, "The selection to cut" },
			{ MCom_VEC, "Center of cutting plane" },
			{ MCom_VEC, "Normal to cutting plane" },
			{ MCom_SEL, "To store new vertices" },
		}
	}, {
		connect,
		"Connect",
		"Connect given vertices",
		"connect",
		3,
		{
			{ MCom_SEL, "The vertices to connect" },
			{ MCom_SEL, "To store new edges" },
			{ MCom_BOOL, "Full connection" },
		}
	}, {
		separate,
		"Separate",
		"Split the grid by every possible loops\nfound in the given edges",
		"separate",
		2,
		{
			{ MCom_SEL, "The edges where to split" },
			{ MCom_SEL, "To store new facets" },
		}
	}, {
		bevel,
		"Bevel",
		"Bevel the selection",
		"bevel",
		3,
		{
			{ MCom_SEL, "Bevel this selection" },
			{ MCom_REAL, "Ratio" },
			{ MCom_SEL, "To store created faces" },
		}
	}, {
		mirror,
		"Mirror",
		"Mirror through a facet.\nThe selection must include at least\none facet, the first will be the\nmirror.",
		"mirror",
		2,
		{
			{ MCom_SEL, "The Mirror" },
			{ MCom_SEL, "To store the new geometry" },
		}
	}, {
		smooth,
		"Smooth",
		"Smooth the selection",
		"smooth",
		4,
		{
			{ MCom_SEL, "Smooth this selection" },
			{ MCom_INT, "Level" },
			{ MCom_REAL, "Softness" },
			{ MCom_SEL, "To store resulting edges" },
		}
	}, {
		bevsmooth,
		"Atl. Smooth",
		"Smooth the selection by beveling",
		"bevsmooth",
		2,
		{
			{ MCom_SEL, "Smooth this selection" },
			{ MCom_INT, "Level" },
		}
	}, {
		zap,
		"Zap",
		"Zap the selection",
		"zap",
		1,
		{
			{ MCom_SEL, "Zap this selection" },
		}
	}, {
		scale,
		"Scale",
		"Scale selected vertices",
		"scale",
		3,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Center" },
			{ MCom_REAL, "Ratio" },
		}
	}, {
		stretch,
		"Stretch",
		"Stretch selected vertices",
		"stretch",
		4,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Center" },
			{ MCom_VEC, "Axis" },
			{ MCom_REAL, "Ratio" },
		}
	}, {
		shear,
		"Shear",
		"Shear selected vertices",
		"shear",
		4,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Center" },
			{ MCom_VEC, "Axis" },
			{ MCom_REAL, "Ratio" },
		}
	}, {
		translate,
		"Translate",
		"Translate selected vertices",
		"trans",
		3,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Displacement" },
			{ MCom_REAL, "Ratio" },
		}
	}, {
		rotate,
		"Rotate",
		"Rotate selected vertices",
		"rot",
		4,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Center" },
			{ MCom_VEC, "Axis" },
			{ MCom_REAL, "Angle (rad)" },
		}
	}, {
		new_selection,
		"Select",
		"Create a new selection",
		"newsel",
		1,	// Le premier argument de Grid_new_selection est automatique
		{ { MCom_GEOMTYPE, "Type of selection" } }
	}, {
		addsingle_to_selection,
		"Pick",
		"Add an element\nto the selection",
		"select",
		2,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_INDEX, "The element" },
		}
	}, {
		subsingle_from_selection,
		"UnPick",
		"Remove an element\nfrom the selection",
		"remove",
		2,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_INDEX, "The element" },
		}
	}, {
		add_to_selection,
		"Add",
		"Add a selection to another one",
		"add",
		2,
		{
			{ MCom_SEL, "The result" },
			{ MCom_SEL, "Selection to add" },
		}
	}, {
		sub_from_selection,
		"Sub",
		"Sub a selection from another one",
		"sub",
		2,
		{
			{ MCom_SEL, "The result" },
			{ MCom_SEL, "Selection to sub" },
		}
	}, {
		empty_selection,
		"Empty",
		"Empty the given selection",
		"empty",
		1,
		{ { MCom_SEL, "The selection" } }
	}, {
		toggle_selection,
		"Toggle",
		"Toggle the given selection",
		"toggle",
		1,
		{ { MCom_SEL, "The selection" } }
	}, {
		convert_selection,
		"Convert",
		"Convert a selection to\nanother geometry type",
		"convert",
		3,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_GEOMTYPE, "The new geomtry type" },
			{ MCom_BOOL, "Max conversion" },
		}
	}, {
		propagate_selection,
		"Propagate",
		"Propagate a selection over\nits borders",
		"propagate",
		2,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_INT, "Amount" },
		}
	}, {
		new_basis,
		"Basis",
		"Create a new Basis",
		"basis",
		5,
		{
			{ MCom_BASIS, "Father" },
			{ MCom_VEC, "Position" },
			{ MCom_VEC, "X Axis" },
			{ MCom_VEC, "Y Axis" },
			{ MCom_VEC, "Z Axis" },
		}
	}, {
		hardskin,
		"Hard Skin",
		"Attach all vertices of the\nselection to the given\nbasis",
		"hardskin",
		2,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_BASIS, "The basis" },
		}
	}, {
		softskin,
		"Soft Skin",
		"Attach all vertices of the\nselection to the given\nbasis and his father",
		"softskin",
		2,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_BASIS, "The basis" },
		}
	}, {
		planmap_ortho,
		"Plan Map (Ortho)",
		"Project a texture onto vertices of\nthe given selection,\nplanar projection",
		"planmapo",
		6,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Center of plan" },
			{ MCom_REAL, "X Scale" },
			{ MCom_REAL, "Y Scale" },
			{ MCom_REAL, "X Offset" },
			{ MCom_REAL, "Y Offset" },
		}
	}, {
		cylmap_ortho,
		"Cyl Map (Ortho)",
		"Project a texture onto vertices of\nthe given selection,\ncylindric projection",
		"cylmapo",
		6,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Axis of cylinder" },
			{ MCom_REAL, "X Scale" },
			{ MCom_REAL, "Y Scale" },
			{ MCom_REAL, "X Offset" },
			{ MCom_REAL, "Y Offset" },
		}
	}, {
		sphermap_ortho,
		"Spher Map (Ortho)",
		"Project a texture onto vertices of\nthe given selection,\nspherical projection",
		"sphermapo",
		6,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "North Pole of sphere" },
			{ MCom_REAL, "X Scale" },
			{ MCom_REAL, "Y Scale" },
			{ MCom_REAL, "X Offset" },
			{ MCom_REAL, "Y Offset" },
		}
	}, {
		planmap,
		"Plan Map",
		"Project a texture onto vertices of\nthe given selection,\nplanar projection",
		"planmap",
		6,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Center of plan" },
			{ MCom_REAL, "X Scale" },
			{ MCom_REAL, "Y Scale" },
			{ MCom_REAL, "X Offset" },
			{ MCom_REAL, "Y Offset" },
		}
	}, {
		cylmap,
		"Cyl Map",
		"Project a texture onto vertices of\nthe given selection,\ncylindric projection",
		"cylmap",
		6,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "Axis of cylinder" },
			{ MCom_REAL, "X Scale" },
			{ MCom_REAL, "Y Scale" },
			{ MCom_REAL, "X Offset" },
			{ MCom_REAL, "Y Offset" },
		}
	}, {
		sphermap,
		"Spher Map",
		"Project a texture onto vertices of\nthe given selection,\nspherical projection",
		"sphermap",
		6,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_VEC, "North Pole of sphere" },
			{ MCom_REAL, "X Scale" },
			{ MCom_REAL, "Y Scale" },
			{ MCom_REAL, "X Offset" },
			{ MCom_REAL, "Y Offset" },
		}
	}, {
		set_uv,
		"Set UV",
		"Set UV coordinates of all\nselected vertices to\ngiven values",
		"setuv",
		3,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_REAL, "U Coord" },
			{ MCom_REAL, "V Coord" },
		}
	}, {
		instanciate,
		"Instanciate",
		"Make a basis a mirror\nof another basis\nNone could be root",
		"instanciate",
		3,
		{
			{ MCom_BASIS, "Original" },
			{ MCom_BASIS, "Copy" },
			{ MCom_BOOL, "Recursively?" },
		}
	}, {
		new_color,
		"Color",
		"Create a new Color",
		"color",
		3,
		{
			{ MCom_REAL, "Red" },
			{ MCom_REAL, "Green" },
			{ MCom_REAL, "Blue" },
		}
	}, {
		paint,
		"Paint",
		"Apply a color to a selection",
		"paint",
		2,
		{
			{ MCom_SEL, "The selection" },
			{ MCom_COLOR, "The color" },
		}
	}, {
		ret,
		"Return",
		"Stop execution of MML instructions",
		"ret",
		0,
		{
			{ 0, NULL },
		}
	}, {
		version,
		"Version",
		"Check that the version of the grid generator match the one\nintended for this model",
		"version",
		1,
		{
			{ MCom_VERSION, "Version number" },
		}
	},
};

#define NB_COMMANDS (sizeof(commands)/sizeof(*commands))

static const char *selType_names[3] = { "vertex", "edge", "facet" };

struct {
	unsigned char command;
	unsigned char param;
	unsigned int was_backref:MAX_NB_PARAMS;
	union {	// We do need all those integer unsigned type because we use memcpy to store them. Storing all in an unsigned long would turn the memcopy into a nightmare of endianess
		unsigned char uc;	// backref, integer, bool, geomtype
		float real;
		Vec vec;
		unsigned long l;	// index
		unsigned short us;	// version
	} params[MAX_NB_PARAMS];
} current;

cntList *backref_vecs = NULL;
cntList *backref_sels = NULL;
cntList *backref_reals = NULL;
cntList *backref_bases = NULL;
cntList *backref_colors = NULL;
static unsigned next_sel_name, next_basis_name, next_color_name;
static unsigned nb_backref_sels, nb_backref_vecs, nb_backref_reals, nb_backref_bases, nb_backref_colors;	// count the backrefs that were created in a command

/* Private Functions */

// tools

static void free_all(void) {
	if (backref_vecs) cntList_del(backref_vecs), backref_vecs = NULL;
	if (backref_sels) cntList_del(backref_sels), backref_sels = NULL;
	if (backref_reals) cntList_del(backref_reals), backref_reals = NULL;
	if (backref_bases) cntList_del(backref_bases), backref_bases = NULL;
	if (backref_colors) cntList_del(backref_colors), backref_colors = NULL;
}

static void init_backref_sizes(void) {
	assert(backref_vecs);
	nb_backref_sels = cntList_size(backref_sels);
	nb_backref_vecs = cntList_size(backref_vecs);
	nb_backref_reals = cntList_size(backref_reals);
	nb_backref_bases = cntList_size(backref_bases);
	nb_backref_colors = cntList_size(backref_colors);
}

static void init_backrefs(void) {
	if (!backref_vecs) {
		backref_vecs = cntList_new(sizeof(Vec), 20);
		cntList_push(backref_vecs, &vec_origin);
		cntList_push(backref_vecs, &vec_x);
		cntList_push(backref_vecs, &vec_y);
		cntList_push(backref_vecs, &vec_z);
		backref_sels = cntList_new(sizeof(unsigned), 10);
		unsigned zero = 0;
		cntList_push(backref_sels, &zero);
		next_sel_name = 1;
		backref_reals = cntList_new(sizeof(float), 20);
		backref_bases = cntList_new(sizeof(unsigned), 10);
		cntList_push(backref_bases, &zero);
		next_basis_name = 1;
		backref_colors = cntList_new(sizeof(unsigned), 10);
		cntList_push(backref_colors, &zero);
		next_color_name = 1;
		assert(backref_vecs && backref_sels && backref_reals && backref_bases && backref_colors);
		atexit(free_all);
		init_backref_sizes();
	}
}

static int param_is_valid(MCom_param_type type) {
	assert(type < MCom_NB_TYPES);
	return
		current.command != UCHAR_MAX &&
		current.param < commands[current.command].nb_params &&
		type == commands[current.command].params[current.param].type;
}

static void add_sel_to_backrefs(unsigned sel) {
	init_backrefs();
	cntList_push(backref_sels, &sel);
}
static void add_basis_to_backrefs(unsigned basis) {
	init_backrefs();
	cntList_push(backref_bases, &basis);
}
static void add_color_to_backrefs(unsigned color) {
	init_backrefs();
	cntList_push(backref_colors, &color);
}
static unsigned get_sel(unsigned p) {
	assert(current.was_backref & (1<<p));
	return *(unsigned *)cntList_get(backref_sels, current.params[p].uc);
}
static bool get_boolean(unsigned p) {
	assert(!(current.was_backref & (1<<p)));
	return current.params[p].uc;
}
static Vec *get_vec(unsigned p) {
	if (current.was_backref & (1<<p)) {
		return (Vec *)cntList_get(backref_vecs, current.params[p].uc);
	} else {
		return &current.params[p].vec;
	}
}
static double get_real(unsigned p) {
	if (current.was_backref & (1<<p)) {
		return *(float *)cntList_get(backref_reals, current.params[p].uc);
	} else {
		return current.params[p].real;
	}
}
static unsigned get_integer(unsigned p) {
	assert(!(current.was_backref & (1<<p)));
	return current.params[p].uc;
}
static GridSel_type get_geomtype(unsigned p) {
	assert(!(current.was_backref & (1<<p)));
	return current.params[p].uc;
}
static unsigned get_index(unsigned p) {
	assert(!(current.was_backref & (1<<p)));
	return current.params[p].l;
}
static unsigned get_basis(unsigned p) {
	assert(current.was_backref & (1<<p));
	return *(unsigned *)cntList_get(backref_bases, current.params[p].uc);
}
static unsigned get_color(unsigned p) {
	assert(current.was_backref & (1<<p));
	return *(unsigned *)cntList_get(backref_colors, current.params[p].uc);
}

// wrappers to Grid

static int tetrahedron(void) {
	return Grid_tetrahedron(get_sel(0));
}
static int cube(void) {
	return Grid_cube(get_sel(0));
}
static int octahedron(void) {
	return Grid_octahedron(get_sel(0));
}
static int icosahedron(void) {
	return Grid_icosahedron(get_sel(0));
}
static int dodecahedron(void) {
	return Grid_dodecahedron(get_sel(0));
}
static int triangle(void) {
	return Grid_triangle(get_sel(0));
}
static int square(void) {
	return Grid_square(get_sel(0));
}
static int extrude(void) {
	return Grid_extrude(get_sel(0), get_boolean(1), get_vec(2), get_real(3), get_sel(4));
}
static int extrude_1by1(void) {
	return Grid_extrude_1by1(get_sel(0), get_boolean(1), get_vec(2), get_real(3), get_real(4), get_sel(5));
}
static int cut(void) {
	if (get_integer(1) < 1) return 0;
	return Grid_cut(get_sel(0), get_integer(1), get_sel(2));
}
static int plane_cut(void) {
	return Grid_plane_cut(get_sel(0), get_sel(3), get_vec(1), get_vec(2));
}
static int connect(void) {
	return Grid_connect(get_sel(0), get_sel(1), get_boolean(2));
}
static int separate(void) {
	return Grid_separate(get_sel(0), get_sel(1));
}
static int bevel(void) {
	return Grid_bevel(get_sel(0), get_sel(2), get_real(1));
}
static int mirror(void) {
	return Grid_mirror(get_sel(0), get_sel(1));
}
static int smooth(void) {
	return Grid_smooth(get_sel(0), get_integer(1), get_real(2), get_sel(3));
}
static int bevsmooth(void) {
	return Grid_bevsmooth(get_sel(0), get_integer(1));
}
static int zap(void) {
	return Grid_zap(get_sel(0));
}
static int new_selection(void) {
	return Grid_new_selection(next_sel_name, get_geomtype(0));
}
static int addsingle_to_selection(void) {
	if (!get_sel(0)) return 0;
	return Grid_addsingle_to_selection(get_sel(0), get_index(1));
}
static int subsingle_from_selection(void) {
	return Grid_subsingle_from_selection(get_sel(0), get_index(1));
}
static int add_to_selection(void) {
	return Grid_add_to_selection(get_sel(0), get_sel(1));
}
static int sub_from_selection(void) {
	return Grid_sub_from_selection(get_sel(0), get_sel(1));
}
static int empty_selection(void) {
	return Grid_empty_selection(get_sel(0));
}
static int toggle_selection(void) {
	return Grid_toggle_selection(get_sel(0));
}
static int convert_selection(void) {
	return Grid_convert_selection(get_sel(0), get_geomtype(1), get_boolean(2) ? GridSel_MAX : GridSel_MIN);
}
static int propagate_selection(void) {
	return Grid_propagate_selection(get_sel(0), get_integer(1));
}
static int scale(void) {
	Grid_scale(get_sel(0), get_vec(1), get_real(2));
	return 1;
}
static int stretch(void) {
	Grid_stretch(get_sel(0), get_vec(1), get_vec(2), get_real(3));
	return 1;
}
static int shear(void) {
	Grid_shear(get_sel(0), get_vec(1), get_vec(2), get_real(3));
	return 1;
}
static int translate(void) {
	Grid_translate(get_sel(0), get_vec(1), get_real(2));
	return 1;
}
static int rotate(void) {
	Grid_rotate(get_sel(0), get_vec(1), get_vec(2), get_real(3));
	return 1;
}
static int new_basis(void) {
	return Grid_new_basis(next_basis_name, get_basis(0), get_vec(1), get_vec(2), get_vec(3), get_vec(4));
}
static int hardskin(void) {
	return Grid_set_selection_hardskin(get_sel(0), get_basis(1));
}
static int softskin(void) {
	return Grid_set_selection_softskin(get_sel(0), get_basis(1));
}
static int planmap(void) {
	return Grid_mapping(get_sel(0), GridSel_PLANAR, get_vec(1), get_real(2), get_real(3), get_real(4), get_real(5), true);
}
static int cylmap(void) {
	return Grid_mapping(get_sel(0), GridSel_CYLINDRIC, get_vec(1), get_real(2), get_real(3), get_real(4), get_real(5), true);
}
static int sphermap(void) {
	return Grid_mapping(get_sel(0), GridSel_SPHERICAL, get_vec(1), get_real(2), get_real(3), get_real(4), get_real(5), true);
}
static int planmap_ortho(void) {
	return Grid_mapping(get_sel(0), GridSel_PLANAR, get_vec(1), get_real(2), get_real(3), get_real(4), get_real(5), false);
}
static int cylmap_ortho(void) {
	return Grid_mapping(get_sel(0), GridSel_CYLINDRIC, get_vec(1), get_real(2), get_real(3), get_real(4), get_real(5), false);
}
static int sphermap_ortho(void) {
	return Grid_mapping(get_sel(0), GridSel_SPHERICAL, get_vec(1), get_real(2), get_real(3), get_real(4), get_real(5), false);
}
static int set_uv(void) {
	return Grid_set_uv(get_sel(0), get_real(1), get_real(2));
}
static int instanciate(void) {
	if (get_basis(0) == get_basis(1) || get_basis(0)==0 || get_basis(1)==0) return 0;
	return Grid_set_instance(get_basis(1), get_basis(0), get_boolean(2));
}
static int new_color(void) {
	return Grid_new_color(next_color_name, get_real(0), get_real(1), get_real(2));
}
static int paint(void) {
	return Grid_set_selection_color(get_sel(0), get_color(1));
}
static int ret(void) {
	ret_was_found = true;
	return 1;
}
static int version(void) {
	unsigned v = get_integer(0);
	if (v != Grid_get_version()) {
		log_warning(LOG_MUSTSEE, "Version mismatch : want %u, but actual version is %u", v, Grid_get_version());
		return 0;
	}
	return 1;
}

/* Protected Functions */

/* Public Functions */

// command API

void MCom_reset(void) {
	ret_was_found = false;
	current.command = UCHAR_MAX;
	free_all();
	if (Grid_get()) Grid_del();
}

int MCom_begin(unsigned char command) {
	if (command >= NB_COMMANDS) {
		log_warning(LOG_DEBUG, "Asked to begin invalid command");
		return 0;
	}
	init_backrefs();
	init_backref_sizes();
	current.command = command;
	current.param = 0;
	current.was_backref = 0;
	return 1;
}
int MCom_param_backref(unsigned char backref) {
	if (param_is_valid(MCom_VEC)) {
		if (backref >= nb_backref_vecs) return 0;
	} else if (param_is_valid(MCom_SEL)) {
		if (backref == nb_backref_sels) {
			add_sel_to_backrefs(next_sel_name ++);
		} else if (backref > nb_backref_sels) {
			return 0;
		}
	} else if (param_is_valid(MCom_BASIS)) {
		if (backref >= nb_backref_bases) return 0;
	} else if (param_is_valid(MCom_COLOR)) {
		if (backref >= nb_backref_colors) return 0;
	} else if (param_is_valid(MCom_REAL)) {
		if (backref >= nb_backref_reals) return 0;
	} else {
		return 0;
	}
	current.params[current.param].uc = backref;
	current.was_backref |= (1<<current.param);
	current.param ++;
	return 1;
}
int MCom_param_vec(float x, float y, float z) {
	if (!param_is_valid(MCom_VEC)) return 0;
	Vec_construct(&current.params[current.param].vec, x,y,z);
	cntList_push(backref_vecs, &current.params[current.param].vec);
	current.param ++;
	return 1;
}
int MCom_param_index(unsigned long index) {
	if (!param_is_valid(MCom_INDEX)) return 0;
	current.params[current.param].l = index;
	current.param ++;
	return 1;
}
int MCom_param_int(unsigned char i) {
	if (!param_is_valid(MCom_INT)) return 0;
	current.params[current.param].uc = i;
	current.param ++;
	return 1;
}
int MCom_param_version(unsigned short v) {
	if (!param_is_valid(MCom_VERSION)) return 0;
	current.params[current.param].us = v;
	current.param ++;
	return 1;
}
int MCom_param_real(float r) {
	if (!param_is_valid(MCom_REAL)) return 0;
	current.params[current.param].real = r;
	cntList_push(backref_reals, &current.params[current.param].real);
	current.param ++;
	return 1;
}
int MCom_param_bool(unsigned char n) {
	if (!param_is_valid(MCom_BOOL)) return 0;
	current.params[current.param].uc = n;
	current.param ++;
	return 1;
}
int MCom_param_geomtype(unsigned char t) {
	if (!param_is_valid(MCom_GEOMTYPE)) return 0;
	current.params[current.param].uc = t;
	current.param ++;
	return 1;
}
int MCom_end(void) {
	if (current.command == UCHAR_MAX || current.param != commands[current.command].nb_params) {
		log_warning(LOG_DEBUG, "Asking to end incomplete command");
		return 0;
	}
	int (* const exec)(void) = commands[current.command].execute;
	
	int ret = exec();
	if (ret) {	// some instructions create new objects accessible through backrefs.
		if (exec == new_selection) {
			add_sel_to_backrefs(next_sel_name ++);
		} else if (exec == new_basis) {
			add_basis_to_backrefs(next_basis_name ++);
		} else if (exec == new_color) {
			add_color_to_backrefs(next_color_name ++);
		}
	}
	init_backref_sizes();
	return ret;
}

int MCom_binexec(unsigned char *bin, unsigned max_size, unsigned *read_size) {
	assert(bin && max_size>0);
	unsigned offset = 0;
	unsigned command = bin[offset++];
	if (!MCom_begin(command)) return 0;
	unsigned was_backref = bin[offset++];
	for (unsigned param=0; param<commands[command].nb_params; param++) {
		int ret;
		unsigned long ul;
		unsigned short us;
		float f[3];
		if (was_backref & (1<<param)) {
			assert(offset < max_size);
			ret = MCom_param_backref(bin[offset++]);
		} else switch (commands[command].params[param].type) {
			case MCom_SEL:
				ret = MCom_param_backref(cntList_size(backref_sels));
				break;
			case MCom_INT:
				assert(offset < max_size);
				ret = MCom_param_int(bin[offset++]);
				break;
			case MCom_VERSION:
				assert(offset + sizeof(unsigned short) < max_size);
				memcpy(&us, bin+offset, sizeof(us));
				ret = MCom_param_version(us);
				offset += sizeof(us);
				break;
			case MCom_BOOL:
				assert(offset < max_size);
				ret = MCom_param_bool(bin[offset++]);
				break;
			case MCom_GEOMTYPE:
				assert(offset < max_size);
				ret = MCom_param_geomtype(bin[offset++]);
				break;
			case MCom_INDEX:
				assert(offset + sizeof(unsigned long) <= max_size);
				memcpy(&ul, bin+offset, sizeof(ul));
				ret = MCom_param_index(ul);
				offset += sizeof(ul);
				break;
			case MCom_VEC:
				assert(offset + sizeof(float)*3 <= max_size);
				memcpy(f+0, bin+offset+sizeof(*f)*0, sizeof(*f));
				memcpy(f+1, bin+offset+sizeof(*f)*1, sizeof(*f));
				memcpy(f+2, bin+offset+sizeof(*f)*2, sizeof(*f));
				ret = MCom_param_vec(f[0], f[1], f[2]);
				offset += sizeof(*f)*3;
				break;
			case MCom_REAL:
				assert(offset + sizeof(float) <= max_size);
				memcpy(f, bin+offset, sizeof(*f));
				ret = MCom_param_real(*f);
				offset += sizeof(*f);
				break;
			case MCom_BASIS:
			case MCom_COLOR:
			default:
				assert(0);
		}
		if (!ret) return 0;
	}
	if (ret_was_found) {
		if (read_size) *read_size = max_size;
		return 1;
	} else {
		if (read_size) *read_size = offset;
		return MCom_end();
	}
}

unsigned MCom_binexec_all(unsigned char *bin, unsigned bin_size) {
	unsigned s = 0;
	while (s < bin_size) {
		unsigned r;
		if (!MCom_binexec(bin+s, bin_size-s, &r)) {
			break;
		}
		s += r;
	}
	return s;
}

// modeler helper

unsigned char MCom_query_nb_commands(void) {
	return NB_COMMANDS;
}
unsigned char MCom_query_nb_groups(void) {
	return 5;
}
unsigned char MCom_query_sizeof_group(unsigned char group) {
	static const unsigned char sizeof_group[] = { NB_PRIMITIVES, 11, 5, 9, 15 };
	assert(group < sizeof(sizeof_group)/sizeof(*sizeof_group));
	return sizeof_group[group];
}
const char *MCom_query_group_name(unsigned char group) {
	assert(group < MCom_query_nb_groups());
	static const char *group_name[] = { "New", "Transformations", "Deformations", "Selections", "Misc." };
	return group_name[group];
}
const char *MCom_query_command_name(unsigned char command) {
	assert(command < NB_COMMANDS);
	return commands[command].name;
}
const char *MCom_query_command_description(unsigned char command) {
	assert(command < NB_COMMANDS);
	return commands[command].description;
}
const char *MCom_query_command_instruction(unsigned char command) {
	assert(command < NB_COMMANDS);
	return commands[command].instruction;
}
const char *MCom_query_selType_name(unsigned t) {
	assert(t<sizeof(selType_names)/sizeof(*selType_names));
	return selType_names[t];
}
unsigned char MCom_query_nb_params(unsigned char command) {
	assert(command < NB_COMMANDS);
	return commands[command].nb_params;
}
MCom_param_type MCom_query_param_type(unsigned char command, unsigned char param) {
	assert(command < NB_COMMANDS && param < commands[command].nb_params);
	return commands[command].params[param].type;
}
const char *MCom_query_type_name(MCom_param_type type) {
	assert(type < MCom_NB_TYPES);
	return type_names[type];
}
const char *MCom_query_param_name(unsigned char command, unsigned char param) {
	assert(command < NB_COMMANDS && param < commands[command].nb_params);
	return commands[command].params[param].name;
}
unsigned MCom_query_nb_backrefs(MCom_param_type type) {
	// Return the number of backrefs AT THE START OF THE COMMAND. Do not count backrefs that were added during the command.
	assert(type < MCom_NB_TYPES);
	init_backrefs();
	switch (type) {
		case MCom_VEC:
			return nb_backref_vecs;
		case MCom_SEL:
			return nb_backref_sels;
		case MCom_REAL:
			return nb_backref_reals;
		case MCom_BASIS:
			return nb_backref_bases;
		case MCom_COLOR:
			return nb_backref_colors;
		default:
			return 0;
	}
}
const char *MCom_query_backref_name(MCom_param_type type, unsigned backref) {
	assert(type < MCom_NB_TYPES);
	init_backrefs();
	static char tmp_name[50];
	unsigned *name;
	switch (type) {
		case MCom_VEC: ;
			Vec *vec = cntList_get(backref_vecs, backref);
			assert(vec);
			snprintf(tmp_name, sizeof(tmp_name), "%#g, %#g, %#g", (double)vec->c[0], (double)vec->c[1], (double)vec->c[2]);
			break;
		case MCom_SEL:
			name = cntList_get(backref_sels, backref);
			assert(name);
			snprintf(tmp_name, sizeof(tmp_name), "sel%02d, %u of type %s", *name, Grid_selection_size(*name), MCom_query_backref_sel_name(backref));
			break;
		case MCom_REAL: ;
			float *r = cntList_get(backref_reals, backref);
			assert(r);
			snprintf(tmp_name, sizeof(tmp_name), "%#g", (double)*r);
			break;
		case MCom_BASIS:
			name = cntList_get(backref_bases, backref);
			assert(name);
			if (*name > 0) {
				snprintf(tmp_name, sizeof(tmp_name), "basis%02d", *name);
			} else {
				return "ROOT";
			}
			break;
		case MCom_COLOR:
			name = cntList_get(backref_colors, backref);
			assert(name);
			if (*name > 0) {
				snprintf(tmp_name, sizeof(tmp_name), "color%02d", *name);
			} else {
				return "NONE";
			}
			break;
		default:
			return 0;
	}
	return tmp_name;
}
const char *MCom_query_backref_sel_name(unsigned backref) {
	init_backrefs();
	unsigned *name;
	name = cntList_get(backref_sels, backref);
	assert(name);
	if (*name > 0) return MCom_query_selType_name(Grid_get_selection_type(*name));
	else return "NONE";
}
const char *MCom_last_cmd_geta(void) {
	static char geta_buf[MAX_MML_LINELEN];
	geta_buf[0] = '\0';
	if (current.command == UCHAR_MAX) return geta_buf;
	strcat(geta_buf, commands[current.command].instruction);
	char str_tmp[MAX_MML_LINELEN];
	for (unsigned param = 0; param < commands[current.command].nb_params; param++) {
		strcat(geta_buf, "\t");
		if (current.was_backref & (1<<param)) {
			sprintf(str_tmp, "\\%u", (unsigned)current.params[param].uc);
			strcat(geta_buf, str_tmp);
		} else switch (commands[current.command].params[param].type) {
			case MCom_SEL:
				strcat(geta_buf, "new");	// theres only 1 sel thats not a backref : "new"
				break;
			case MCom_INT:
			case MCom_BOOL:
				sprintf(str_tmp, "%u", (unsigned)current.params[param].uc);
				strcat(geta_buf, str_tmp);
				break;
			case MCom_VERSION:
				sprintf(str_tmp, "%u", (unsigned)current.params[param].us);
				strcat(geta_buf, str_tmp);
				break;
			case MCom_INDEX:
				sprintf(str_tmp, "%lu", current.params[param].l);
				strcat(geta_buf, str_tmp);
				break;
			case MCom_GEOMTYPE:
				strcat(geta_buf, selType_names[current.params[param].uc]);
				break;
			case MCom_VEC:
				sprintf(str_tmp, "%g,%g,%g", Vec_coord(&current.params[param].vec, 0), Vec_coord(&current.params[param].vec, 1), Vec_coord(&current.params[param].vec, 2));
				strcat(geta_buf, str_tmp);
				break;
			case MCom_REAL:
				sprintf(str_tmp, "%g", (double)current.params[param].real);
				strcat(geta_buf, str_tmp);
				break;
			case MCom_BASIS:
			case MCom_COLOR:
			default:
				assert(0);
		}
	}
	return geta_buf;
}
const unsigned char *Mcom_last_cmd_get(unsigned *size) {
	static unsigned char get_buf[MAX_BIN_LINELEN];
	get_buf[0] = current.command;
	get_buf[1] = current.was_backref;
	unsigned offset = 2;
	for (unsigned param = 0; param < commands[current.command].nb_params; param++) {
		if (current.was_backref & (1<<param)) {
			get_buf[offset++] = current.params[param].uc;
		} else switch (commands[current.command].params[param].type) {
			case MCom_SEL:
				// no output
				break;
			case MCom_INT:
			case MCom_BOOL:
			case MCom_GEOMTYPE:
				get_buf[offset++] = current.params[param].uc;
				break;
			case MCom_VERSION:
				memcpy(get_buf+offset, &current.params[param].us, sizeof(unsigned short));
				offset += sizeof(unsigned short);
				break;
			case MCom_INDEX:
				memcpy(get_buf+offset, &current.params[param].l, sizeof(unsigned long));
				offset += sizeof(unsigned long);
				break;
			case MCom_VEC:
				for (unsigned c=0; c<3; c++) {
					float f = Vec_coord(&current.params[param].vec, c);
					memcpy(get_buf+offset, &f, sizeof(float));
					offset += sizeof(float);
				}
				break;
			case MCom_REAL:
				memcpy(get_buf+offset, &current.params[param].real, sizeof(float));
				offset += sizeof(float);
				break;
			case MCom_BASIS:
			case MCom_COLOR:
			default:
				assert(0);
		}
	}
	if (size) *size = offset;
	return get_buf;
}
// vi:ts=3:sw=3
