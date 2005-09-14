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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/forms.h>
#include <libcnt/cnt.h>
#include <tiffio.h>
#include "mmodeler.h"
#include "grid2gcl.h"
#include "mmodel_ui.h"
#include "libmicromodel/mcommander.h"
#include "libmicromodel/mml2bin.h"
#include "libmicromodel/grid.h"
#include "srcbuf.h"

/* Picking */

static FL_OBJECT *pick_input = NULL;	// to poke the pick into
FL_OBJECT *uv_x_input = NULL;	// to write the selected UVs into
FL_OBJECT *uv_y_input = NULL;

static void pick(int fd, void *data) {
	assert(pick_input && data);
	FL_OBJECT *select_combo = data;
	char line[1024] = "\0";
	fgets(line, 1024, stdin);
	log_warning(LOG_DEBUG, "Read from stdin : '%s'", line);
	long int vi=-2, fi=-2;
	unsigned r;
	char ei[256] = "\0";
	// Look for the selection's type
	int selected_name = fl_get_choice(select_combo);
	if (3 == sscanf(line, "(pick World grid nil nil nil nil nil %ld (%[^)]) %ld)", &vi, ei, &fi)) {
		log_warning(LOG_DEBUG, "Recognized Edge pick : %ld, '%s', %ld", vi, ei, fi);
		if (selected_name && 0 != strcmp(MCom_query_backref_sel_name(selected_name-1), "edge")) {
			log_warning(LOG_DEBUG, "Edge does not match selection type");
			return;
		}
		unsigned long i0,i1;
		if (2 == sscanf(ei, "%lu %lu", &i0, &i1)) {
			log_warning(LOG_DEBUG, "translate to vertex names (%u, %u)", Vertex_name(get_vertex_from_gcl_order(i0)), Vertex_name(get_vertex_from_gcl_order(i1)));
			r = Edge_name(Grid_edge_index_from_vertices(get_vertex_from_gcl_order(i0), get_vertex_from_gcl_order(i1)));
			log_warning(LOG_DEBUG, "Recognized edge %u", r);
		} else {
			log_warning(LOG_DEBUG, "Cannot read edge");
			return;
		}
	} else {
		if (2 == sscanf(line, "(pick World grid nil nil nil nil nil %ld () %ld)", &vi, &fi)) {
			if (vi != -1) {
				if (selected_name && 0 != strcmp(MCom_query_backref_sel_name(selected_name-1), "vertex")) {
					log_warning(LOG_DEBUG, "Vertex does not match selection type");
					return;
				}
				r = Vertex_name(get_vertex_from_gcl_order(vi));
				log_warning(LOG_DEBUG, "Recognized Vertex %u", r);
			} else {
				if (selected_name && 0 != strcmp(MCom_query_backref_sel_name(selected_name-1), "facet")) {
					log_warning(LOG_DEBUG, "Facet does not match selection type");
					return;
				}
				r = Facet_name(get_facet_from_gcl_order(fi));
				log_warning(LOG_DEBUG, "Recognized Facet %u", r);
			}
		} else {
			log_warning(LOG_DEBUG, "Cannot recognize anything");
			return;
		}
	}
	sprintf(line, "%u", r);
	fl_set_input(pick_input, line);
}

#include <unistd.h>
static void start_picking(FL_OBJECT *obj, FL_OBJECT *select_combo) {
	pick_input = obj;
	fl_add_io_callback(STDIN_FILENO, FL_READ, pick, select_combo);
}
static void stop_picking(void) {
	if (pick_input) {
		fl_remove_io_callback(STDIN_FILENO, FL_READ, pick);
		pick_input = NULL;
	}
}
static void start_uv_picking(FL_OBJECT *obj_x, FL_OBJECT *obj_y) {
	uv_x_input = obj_x;
	uv_y_input = obj_y;
}
static void stop_uv_picking(void) {
	uv_x_input = uv_y_input = NULL;
}

/* Create the action form */

FL_FORM *form_action = NULL;
char modified = 0;

static unsigned obj_add(unsigned command, unsigned p, int add, int x,int y,int w, int def) {
	FL_OBJECT *obj;
	unsigned h;
	static unsigned last_select_obj_in_param;	// for the picking to retrieve the type of selected selection
	unsigned nbo = 0;
	switch (MCom_query_param_type(command, p)) {
		case MCom_VEC:
			h = 60;
			if (add) {
				obj = fl_add_input(FL_FLOAT_INPUT, x,y, w/3,30, "");
				fl_set_object_callback(obj, action_x_cb, p);
				current_objects[p][nbo++] = obj;
				obj = fl_add_input(FL_FLOAT_INPUT, x+w/3,y, w/3,30, "");
				fl_set_object_callback(obj, action_y_cb, p);
				current_objects[p][nbo++] = obj;
				obj = fl_add_input(FL_FLOAT_INPUT, x+(2*w)/3,y, w/3,30, "");
				fl_set_object_callback(obj, action_z_cb, p);
				current_objects[p][nbo++] = obj;
				obj = fl_add_choice(FL_NORMAL_CHOICE2, x,y+30, w,30, "");
				unsigned nb_vecs = MCom_query_nb_backrefs(MCom_VEC);
				for (unsigned i=0; i<nb_vecs; i++) {
					fl_addto_choice(obj, MCom_query_backref_name(MCom_VEC, i));
				}
				fl_addto_choice(obj, "new");
				fl_set_object_callback(obj, action_vecbr_cb, p);
				current_objects[p][nbo++] = obj;
			}
			break;
		case MCom_SEL:
			h = 30;
			if (add) {
				obj = fl_add_choice(FL_NORMAL_CHOICE2, x,y, w,30, "");
				fl_set_object_callback(obj, action_sel_cb, p);
				unsigned nb_sels = MCom_query_nb_backrefs(MCom_SEL);
				for (unsigned i=0; i<nb_sels; i++) {
					fl_addto_choice(obj, MCom_query_backref_name(MCom_SEL, i));
				}
				fl_addto_choice(obj, "new");
				if (def) fl_set_choice(obj, def);
				last_select_obj_in_param = p;
				current_objects[p][nbo++] = obj;
			}
			break;
		case MCom_BASIS:
			h = 30;
			if (add) {
				obj = fl_add_choice(FL_NORMAL_CHOICE2, x,y, w,30, "");
				fl_set_object_callback(obj, action_basis_cb, p);
				unsigned nb_bases = MCom_query_nb_backrefs(MCom_BASIS);
				for (unsigned i=0; i<nb_bases; i++) {
					fl_addto_choice(obj, MCom_query_backref_name(MCom_BASIS, i));
				}
				if (def) fl_set_choice(obj, def);
				current_objects[p][nbo++] = obj;
			}
			break;
		case MCom_COLOR:
			h = 30;
			if (add) {
				obj = fl_add_choice(FL_NORMAL_CHOICE2, x,y, w,30, "");
				fl_set_object_callback(obj, action_color_cb, p);
				unsigned nb_colors = MCom_query_nb_backrefs(MCom_COLOR);
				for (unsigned i=0; i<nb_colors; i++) {
					fl_addto_choice(obj, MCom_query_backref_name(MCom_COLOR, i));
				}
				if (def) fl_set_choice(obj, def);
				current_objects[p][nbo++] = obj;
			}
			break;
		case MCom_INDEX:
			h = 30;
			if (add) {
				obj = fl_add_input(FL_INT_INPUT, x,y, w,30, "");
				fl_set_object_callback(obj, action_index_cb, p);
				current_objects[p][nbo++] = obj;
				start_picking(obj, current_objects[last_select_obj_in_param][0]);
			}
			break;
		case MCom_INT:
			h = 30;
			if (add) {
				obj = fl_add_input(FL_INT_INPUT, x,y, w,30, "");
				fl_set_object_callback(obj, action_int_cb, p);
				current_objects[p][nbo++] = obj;
			}
			break;
		case MCom_VERSION:
			h = 30;
			if (add) {
				static char version[16];
				snprintf(version, 16, "%u", Grid_get_version());
				obj = fl_add_text(FL_NORMAL_TEXT, x,y, w,30, version);
				current_objects[p][nbo++] = obj;
			}
			break;
		case MCom_REAL:
			h = 30;
			if (add) {
				obj = fl_add_input(FL_FLOAT_INPUT, x,y, w,30, "");
				fl_set_object_callback(obj, action_real_cb, p);
				current_objects[p][nbo++] = obj;
				if (p == 2) {	// Nasty trick to plug the uv_picker
					start_uv_picking(current_objects[p-1][0], obj);
				}
			}
			break;
		case MCom_BOOL:
			h = 30;
			if (add) {
				obj = fl_add_checkbutton(FL_PUSH_BUTTON, x,y, w,30, "");
				fl_set_object_callback(obj, action_bool_cb, p);
				current_objects[p][nbo++] = obj;
			}
			break;
		case MCom_GEOMTYPE:
			h = 30;
			if (add) {
				obj = fl_add_choice(FL_NORMAL_CHOICE2, x,y, w,30, "");
				fl_set_object_callback(obj, action_geomtype_cb, p);
				for (unsigned i=0; i<3; i++) {
					fl_addto_choice(obj, MCom_query_selType_name(i));
				}
				current_objects[p][nbo++] = obj;
			}
			break;
		default:
			assert(0);
	}
	if (nbo<MAX_NB_OBJ_PER_TYPE) current_objects[p][nbo] = NULL;
	return h;
}

void delete_action_form(void) {
	if (form_action) {
		stop_picking();
		stop_uv_picking();
		fl_hide_form(form_action);
		fl_free_form(form_action);
		form_action = NULL;
	}
}

FL_FORM *create_form_action(unsigned command) {
	static const unsigned w=202;
	unsigned tot_h=0;
	unsigned p, nb_params = MCom_query_nb_params(command);
	for (p=0; p<nb_params; p++) {
		tot_h += 30+obj_add(command, p, 0, 0,0,0,0);
	}
	FL_FORM *form = fl_bgn_form(FL_UP_BOX, w,1+60+tot_h+30+1);
	FL_OBJECT *obj;
	fl_add_text(FL_NORMAL_TEXT, 1,1, w-2,60, MCom_query_command_description(command));
	unsigned h = 61;
	unsigned nb_sel = 0;
	for (unsigned p=0; p<nb_params; p++) {
		fl_add_text(FL_NORMAL_TEXT, 1,h, w-2,30, MCom_query_param_name(command, p));
		h += 30;
		int choice = 0;
		switch (MCom_query_param_type(command, p)) {
			case MCom_VEC:
			case MCom_INDEX:
			case MCom_INT:
			case MCom_VERSION:
			case MCom_REAL:
			case MCom_BOOL:
			case MCom_GEOMTYPE:
				break;
			case MCom_BASIS:
				choice = fl_get_choice(select_basis);	// allways set to selected basis
				break;
			case MCom_COLOR:
				choice = fl_get_choice(select_color);	// allways set to selected basis
				break;
			case MCom_SEL:
				if (!nb_sel) {	// default to viewed selection
					choice = fl_get_choice(select_sel);
				}
				nb_sel++;
				break;
			default:
				assert(0);
		}
		h += obj_add(command, p, 1, 1,h, w-2, choice);
	}
	obj = fl_add_button(FL_NORMAL_BUTTON, 1,h, 78,30, "Cancel");
	fl_set_object_callback(obj, cancel_action_cb, 0);
	obj = fl_add_button(FL_RETURN_BUTTON, w-80,h, 78,30, "Accept");
	fl_set_object_callback(obj, accept_action_cb, command);
	fl_end_form();
	return form;
}

/* Reset the source textbox */

SrcBuf *mml = NULL;

static void free_mml(void) {
	if (mml) {
		SrcBuf_del(mml);
		mml = NULL;
	}
}

void reset_source(void) {
	char *txt = SrcBuf_get_text(mml);
	fl_set_input(source_textbox, txt);
}

/* Reset the dynamic choice box */

void reset_dyn_choices(void) {
	fl_freeze_form(MicroModeler);
	{	// init select_sel
		static unsigned former_nb_sels = 0;
		int old_choice = fl_get_choice(select_sel);
		unsigned nb_sels = MCom_query_nb_backrefs(MCom_SEL);
		fl_clear_choice(select_sel);
		for (unsigned i=0; i<nb_sels; i++) {
			fl_addto_choice(select_sel, MCom_query_backref_name(MCom_SEL, i));
		}
		if (old_choice > (int)nb_sels) old_choice = 0;
		else if (nb_sels != former_nb_sels) {	// to automatically select any new entry
			former_nb_sels = nb_sels;
			old_choice = nb_sels;
		}
		fl_set_choice(select_sel, old_choice);
	} {	// init select_basis
		static unsigned former_nb_bases = 0;
		int old_choice = fl_get_choice(select_basis);
		unsigned nb_bases = MCom_query_nb_backrefs(MCom_BASIS);
		fl_clear_choice(select_basis);
		for (unsigned i=0; i<nb_bases; i++) {
			fl_addto_choice(select_basis, MCom_query_backref_name(MCom_BASIS, i));
		}
		if (old_choice > (int)nb_bases) old_choice = 0;
		else if (nb_bases != former_nb_bases) {
			former_nb_bases = nb_bases;
			old_choice = nb_bases;
		}
		fl_set_choice(select_basis, old_choice);
	} {	// init select_color
		static unsigned former_nb_colors = 0;
		int old_choice = fl_get_choice(select_color);
		unsigned nb_colors = MCom_query_nb_backrefs(MCom_COLOR);
		fl_clear_choice(select_color);
		for (unsigned i=0; i<nb_colors; i++) {
			fl_addto_choice(select_color, MCom_query_backref_name(MCom_COLOR, i));
		}
		if (old_choice > (int)nb_colors) old_choice = 0;
		else if (nb_colors != former_nb_colors) {
			former_nb_colors = nb_colors;
			old_choice = nb_colors;
		}
		fl_set_choice(select_color, old_choice);
	} {	// reset info
		char sizes[256];
		unsigned nb_vertices, nb_edges, nb_facets;
		Grid_size(&nb_vertices, &nb_edges, &nb_facets);
		snprintf(sizes, sizeof(sizes), "size: %u vertices, %u edges, %u facets", nb_vertices, nb_edges, nb_facets);
		fl_set_object_label(infotext_sizes, sizes);
	}
	fl_unfreeze_form(MicroModeler);
}

/* Reset the drawing of the grid */

uint32 *tiff_data;
uint32 tiff_size;	// With = Height = tiff_size
char tiff_filename[PATH_MAX+1] = "defaultmap.tiff";

void reset_gcl(void) {
	// get selected selection
	int choice_sel = fl_get_choice(select_sel);
	if (choice_sel>0) choice_sel--;	// as first backref is NULL
	int choice_basis = fl_get_choice(select_basis);
	if (choice_basis>0) choice_basis--;
	grid2gcl(choice_sel, choice_basis, tiff_filename);
	if (fl_form_is_visible(MapView)) {
	//	fl_redraw_object(mapdisplay);	this does not generate an expose event for canvas :-<
		fl_activate_glcanvas(mapdisplay);
		mapdisplay_redraw();
	}
}

void reset_tiff(void) {
	// reset the infotext
	static char infotxt[45] = "texture: ";
	strncpy(infotxt+9, tiff_filename, sizeof(infotxt)-9-1);
	fl_set_object_label(infotext_mapfile, infotxt);
	// reread the fill texture
	if (fl_form_is_visible(MapView)) {
		TIFF* tif = TIFFOpen(tiff_filename, "r");
		if (tif) {
			uint32 w, h;
			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
			if (w == h) {
				if (tiff_data) _TIFFfree(tiff_data);
				tiff_data = (uint32*)_TIFFmalloc(w*h*sizeof(uint32));
				if (tiff_data) {
					tiff_size = w;
					if (TIFFReadRGBAImage(tif, w, h, tiff_data, 0)) {
						fl_activate_glcanvas(mapdisplay);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tiff_size, tiff_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, tiff_data);
					} else {
						_TIFFfree(tiff_data);
						tiff_data = NULL;
						fl_show_messages("I couldn't read your tiff file.\n(Actually, libtiff couldn't)\n\nYou should try with a real tiff file, textures r0x !");
					}
				} else {
					fl_show_messages("Your picture is that big that I couldn't allocate memory for it\n(At least, libtiff couldn't)");
				}
			} else {
				fl_show_messages("Your texture must have its width = its height");
			}
			TIFFClose(tif);
		} else {
			fl_show_messages("I couldn't open your tiff file.\n(Actually, libtiff couldn't)\n\nBad perms, certainly.");
		}
	} // fl_form_is_visible(MapView)
}

/* Load a MML file and reset modeler */

void reset_grid(void) {
	MCom_reset();
	unsigned bin_size;
	mmlPatchSet *patchset;
	unsigned char *bin = mml2bin(SrcBuf_get_text(mml), &bin_size, &patchset);
	if (!bin) return;
	if (!patchbina(bin, patchset, fl_get_input(args))) {
		log_warning(LOG_IMPORTANT, "Cannot fully patch the bin (*AND* you should replace this with a nice dialog box)");
	}
	if (MCom_binexec_all(bin, bin_size) < bin_size) {
		log_warning(LOG_IMPORTANT, "Cannot exec the bin (*AND* you should replace this with a nice dialog box)");
	}
	mem_unregister(bin);
	mem_unregister(patchset);
}

int load_mml(const char *buf) {
	if (buf) {
		if (!SrcBuf_set_text(mml, buf)) return 0;
	} else {
		SrcBuf_set_text(mml, "");
	}
	reset_grid();
	reset_dyn_choices();
	reset_source();
	reset_gcl();
	modified = 0;
	return 1;
}

void reset_fileinfo(const char *filename) {
	static char infotxt[45] = "file: ";
	strncpy(infotxt+6, filename, sizeof(infotxt)-6-1);
	fl_set_object_label(infotext_file, infotxt);
	strcpy(last_filename, filename);
}

int reset_to_mml(const char *filename) {
	FILE *file = fopen(filename, "r");
	if (!file) file = fopen(filename, "w+");
	if (!file) {
		log_warning(LOG_MUSTSEE, "Cannot open file '%s': %s", filename, strerror(errno));
		return 0;
	}
	unsigned offset = 0, buf_size = 1024;
	char *buf = mem_alloc(buf_size);
	if (!buf) return 0;
	while (!feof(file)) {
		if (offset-1 >= buf_size) {
			unsigned new_size = buf_size*2;
			char *tmp = mem_realloc(buf, new_size);
			if (!tmp) {
				log_warning(LOG_MUSTSEE, "file too big : '%s'", filename);
error_1:
				mem_unregister(buf);
				fclose(file);
				return 0;
			}
			buf = tmp;
			buf_size = new_size;
		}
		offset += fread(buf+offset, 1, buf_size-offset, file);
		if (ferror(file)) {
			log_warning(LOG_MUSTSEE, "error reading file '%s'", filename);
			goto error_1;
		}
	}
	buf[offset++] = '\0';
	char *tmp = mem_realloc(buf, offset);
	if (tmp) buf = tmp;
	if (!load_mml(buf)) {
		log_warning(LOG_MUSTSEE, "Invalid MML file?");
		goto error_1;
	}
	mem_unregister(buf);
	fclose(file);
	reset_fileinfo(filename);
	return 1;
}

/* Add action buttons - add this at the end of create_form_MicroModeler before the end_form */

void add_action_buttons(void) {
#	define ACTION_HEIGHT 25
	FL_Coord y=0, box_x, box_y, box_w, box_h;
	fl_get_object_geometry(action_box, &box_x, &box_y, &box_w, &box_h);
	for (unsigned command=0, group=0; group<MCom_query_nb_groups(); group++) {
		FL_Coord x = 0;
		for (unsigned groupcom=0; groupcom<MCom_query_sizeof_group(group); groupcom++, command++) {
			const char *command_name = MCom_query_command_name(command);
			FL_Coord w = 8*strlen(command_name);
			if ( x > 0 && x+w > box_w ) {
				x = 0;
				y += ACTION_HEIGHT;
			}
			FL_OBJECT *obj = fl_add_button(FL_NORMAL_BUTTON, box_x+x,box_y+y, w,ACTION_HEIGHT, command_name);
			fl_set_object_boxtype(obj, FL_BORDER_BOX);
			fl_set_object_callback(obj, action_cb, command);
			x += w;
		}
		y += ACTION_HEIGHT+8;
	}
#	undef ACTION_HEIGHT
}

/* Command line */

static void syntax(void) {
	puts(
		"mmodeler [-h] [ -i input_file ]\n"
		"\n"
		"  -h, --help              : this help\n"
		"  -q, --quiet             : reduce verbosity\n"
		"  -d, --debug             : increase verbosity\n"
		"  -i file, --input=file   : a mml file you want to edit/create\n"
		"  -t file, --texture=file : a tiff file to use as the texture\n"
		"  -p patches, --patch=patches : the patch string to apply\n"
		"                           (mmodel.mml by default)\n"
	);
}

static void missing_parameter(void) {
	fprintf(stderr, "Missing parameter\n");
	syntax();
	exit(EXIT_FAILURE);
}

static void bad_syntax(void) {
	fprintf(stderr, "Bad syntax\n");
	syntax();
	exit(EXIT_FAILURE);
}

static void bad_getopt(int c) {
	fprintf(stderr, "?? getopt returned character code %d ??\n", c);
	exit(EXIT_FAILURE);
}

int main(int nb_args, char *argv[]) {
	log_warn_level log_level = LOG_OPTIONAL;
	const char *filename = NULL;
	const char *init_patches = NULL;
	/* Command line */
	while (1) {
		static struct option long_options[] = {
			{ "input", required_argument, NULL, 'i' },
			{ "patch", required_argument, NULL, 'p' },
			{ "texture", required_argument, NULL, 't' },
			{ "help", no_argument, NULL, 'h' },
			{ "quiet", no_argument, NULL, 'q' },
			{ "debug", no_argument, NULL, 'd' },
			{ 0,0,0,0 },
		};
		int c = getopt_long(nb_args, argv, "i:p:t:hqd", long_options, NULL);
		switch (c) {
			case -1:
				goto end_opts;
			case 'i':
				filename = optarg;
				break;
			case 'p':
				init_patches = optarg;
				break;
			case 't':
				strcpy(tiff_filename, optarg);
				break;
			case 'h':
				syntax();
				return EXIT_SUCCESS;
			case 'q':
				log_level = LOG_IMPORTANT;
				break;
			case 'd':
				log_level = LOG_DEBUG;
				break;
			case ':':
				missing_parameter();
				break;
			case '?':
				bad_syntax();
				break;
			default:
				bad_getopt(c);
				break;
		}
	}
end_opts:
	/* First init */
	cnt_init(140000, log_level);
	atexit(cnt_end);
	atexit(grid2gcl_end);
	Grid_set_carac_size(4000);
	mml = SrcBuf_new(5000);
	if (!mml) log_fatal("Cannot set up source buffer ?");
	atexit(free_mml);
	MCom_reset();
	/* XForms init */
	log_warning(LOG_DEBUG, "Init XForms");
	fl_initialize(&nb_args, argv, 0, 0, 0);
	int attributes[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_ALPHA_SIZE, 1,
		GLX_DEPTH_SIZE, 0,
		GLX_STENCIL_SIZE, 0,
		GLX_ACCUM_RED_SIZE, 0,
		GLX_ACCUM_GREEN_SIZE, 0,
		GLX_ACCUM_BLUE_SIZE, 0,
		GLX_ACCUM_ALPHA_SIZE, 0,
		None
	};
	fl_set_glcanvas_defaults(attributes);
	create_the_forms();
	fl_add_canvas_handler(mapdisplay, Expose, mapdisplay_expose, NULL);
   fl_add_canvas_handler(mapdisplay, ButtonPress, mapdisplay_click, 0);
	/* Final init */
	if (filename) {
		log_warning(LOG_DEBUG, "Reset to '%s'", filename);
		if (!reset_to_mml(filename)) {
			log_warning(LOG_MUSTSEE, "Cannot load file '%s'", filename);
			return EXIT_FAILURE;
		}
	} else {
		load_mml(NULL);
	}
	if (init_patches) {
		fl_set_input(args, init_patches);
	}
	reset_tiff();
	/* Geomview init */
	puts("(interest (pick world grid nil nil nil nil nil * * *))");
	puts("(event-pick on)");
	fflush(stdout);
	/* Start the show */
	fl_show_form(MicroModeler,FL_PLACE_CENTER,FL_FULLBORDER,"Micro-Modeler");
	fl_do_forms();
	if (tiff_data) _TIFFfree(tiff_data);
	fl_finish();
	return EXIT_SUCCESS;
}

// vi:ts=3:sw=3
