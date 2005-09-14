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
#include <assert.h>
#include <libcnt/log.h>
#include <libcnt/mem.h>
#include <string.h>
#include <errno.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <X11/forms.h>
#include <tiffio.h>
#include "mmodel_ui.h"
#include "mmodeler.h"
#include "libmicromodel/mcommander.h"
#include "libmicromodel/grid.h"
#include "libmicromodel/mml2bin.h"
#include "srcbuf.h"
#include "uv2triangles.h"

/* Data Definitions */

static unsigned current_command;
static struct {	// cannot be union because for Vecs both f and uc are used
	unsigned char uc;
	float f[3];
	unsigned long ul;
} current_params[MAX_NB_PARAMS];
FL_OBJECT *current_objects[MAX_NB_PARAMS][MAX_NB_OBJ_PER_TYPE];
char last_filename[PATH_MAX+1] = "default.mml";

/* Private Functions */

static void save(bool ascii) {
	const char *filename = fl_show_fselector("Choose the file", "./", ascii ? "*.mml":"*.bin", ascii ? last_filename:"default.bin");
	if (filename) {
		FILE *file = fopen(filename, ascii ? "w+":"w+b");
		if (!file) {
			log_warning(LOG_MUSTSEE, "Cannot open file '%s' : %s", filename, strerror(errno));
			return;
		}
		char *source = SrcBuf_get_text(mml);
		if (ascii) {
			fputs(source, file);
			reset_fileinfo(filename);
		} else {
			unsigned bin_size;
			mmlPatchSet *patchset;
			unsigned char *bin = mml2bin(source, &bin_size, &patchset);
			fwrite(patchset, 1, sizeof_patchset(patchset), file);
			fwrite(bin, 1, bin_size, file);
			mem_unregister(bin);
			mem_unregister(patchset);
		}
		fclose(file);
		modified = 0;
	}
}

static void load(void) {
	const char *filename = fl_show_fselector("Choose the file", "./", "*.mml", "");
	if (filename) {
		reset_to_mml(filename);
	}
}

static void loadTiff(void) {
	const char *filename = fl_show_fselector("Choose the file", "./", "*.tiff", "");
	if (filename) {
		strcpy(tiff_filename, filename);
		reset_tiff();
		reset_gcl();
	}
}

static void undo(void) {
	SrcBuf_undo(mml);
	reset_source();
	reset_grid();
	reset_dyn_choices();
	reset_gcl();
}

static void redo(void) {
	SrcBuf_redo(mml);
	reset_source();
	reset_grid();
	reset_dyn_choices();
	reset_gcl();
}

/*** callbacks and freeobj handles for form mmodel ***/

void menu_file_cb(FL_OBJECT *ob, long data) {
	switch (fl_get_menu(ob)) {
		case 1:	// load
			if (modified) {
				if (! fl_show_question("Are you sure you want to load a new MML and loose all you hard work ?", 0)) {
					return;
				}
			}
			load();
			break;
		case 2:	// save
			save(true);
			break;
		case 3:	// export
			save(false);
			break;
		case 4:	// load TIFF
			loadTiff();
			break;
		case 5:	// quit
			if (modified) {
				if (! fl_show_question("Are you sure you want to leave without saving ?", 0)) {
					return;
				}
			}
			fl_finish();
			if (Grid_get()) Grid_del();
			exit(EXIT_SUCCESS);
	}
}

void menu_edit_cb(FL_OBJECT *ob, long data) {
	switch (fl_get_menu(ob)) {
		case 1:	// undo
			undo();
			break;
		case 2:	// redo
			redo();
			break;
	}
}

void help_cb(FL_OBJECT *ob, long data) {
	fl_show_form(Help, FL_PLACE_FREE, FL_TRANSIENT, "Micro Modeler - Help");
}
void hide_help_cb(FL_OBJECT *ob, long data) {
	fl_hide_form(Help);
}

void args_cb(FL_OBJECT *ob, long data) {
	int old_modified = modified;
	accept_source_cb(ob, data);
	modified = old_modified;
}

void source_cb(FL_OBJECT *ob, long data) {
	accept_source_cb(ob, data);
}

void select_sel_cb(FL_OBJECT *ob, long data) {
	reset_gcl();
}
void select_basis_cb(FL_OBJECT *ob, long data) {
	reset_gcl();
}
void select_color_cb(FL_OBJECT *ob, long data) {
	reset_gcl();
}

void action_cb(FL_OBJECT *ob, long data) {
	delete_action_form();
	unsigned command = data;
	// create a new form
	current_command = command;
	form_action = create_form_action(command);
	if (form_action) {
		fl_show_form(form_action, FL_PLACE_FREE, FL_FULLBORDER, MCom_query_command_name(command));
	}
}

void mapview_cb(FL_OBJECT *ob, long data) {
	static bool is_shown = false;
	if (is_shown) {
		fl_activate_glcanvas(mapdisplay);
		fl_hide_form(MapView);
		is_shown = false;
	} else {
		fl_show_form(MapView, FL_PLACE_FREE, FL_TRANSIENT, "UV Mapping");
		is_shown = true;
		reset_tiff();	// will load the texture
	}
}

void mapdisplay_redraw(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	glShadeModel(GL_FLAT);
	glEnable(GL_TEXTURE_2D);
	if (tiff_data) {
		glBegin(GL_QUADS);
		glVertex3f(-1.,-1.,0.);
		glTexCoord2f(1.,0.);
		glVertex3f(1.,-1.,0.);
		glTexCoord2f(1.,1.);
		glVertex3f(1.,1.,0.);
		glTexCoord2f(0.,1.);
		glVertex3f(-1.,1.,0.);
		glTexCoord2f(0.,0.);
		glEnd();
	}
	int choice_sel = fl_get_choice(select_sel);
	if (choice_sel>0) choice_sel--;
	uv2triangles(choice_sel);
	glFinish();
}

int mapdisplay_expose(FL_OBJECT *ob, Window win, int w, int h, XEvent *xev, void *ud) {
	glViewport(0,0, (GLint)w, (GLint)h);
	glMatrixMode(GL_PROJECTION);
	glOrtho(-1.,1.,-1.,1.,-1.,1.);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	mapdisplay_redraw();
	return 0;
}

int mapdisplay_click(FL_OBJECT *ob, Window win, int w, int h, XEvent *xev, void *ud) {
	static char infotxt[45] = "coord: ";
	double x = ((double)xev->xbutton.x/w)*2.-1.;
	double y = (1.-(double)xev->xbutton.y/h)*2.-1.;
	snprintf(infotxt+7, sizeof(infotxt)-7, "%f, %f", x, y);
	fl_set_object_label(coordinfotext, infotxt);
	if (uv_x_input) {
		snprintf(infotxt+7, sizeof(infotxt)-7, "%f", x);
		fl_set_input(uv_x_input, infotxt+7);
	}
	if (uv_y_input) {
		snprintf(infotxt+7, sizeof(infotxt)-7, "%f", y);
		fl_set_input(uv_y_input, infotxt+7);
	}
	return 1;
}

/*** callbacks and freeobj handles for form source ***/

void accept_source_cb(FL_OBJECT *ob, long data) {
	if (!load_mml(fl_get_input(source_textbox))) {
		fl_show_alert("Invalid MML syntax", "", "Fix it to proceed", 1);
	} else {
		modified = 1;
	}
}

/* Callbacks for form_action */

void cancel_action_cb(FL_OBJECT *ob, long data) {
	delete_action_form();
}
void accept_action_cb(FL_OBJECT *ob, long command) {
	log_warning(LOG_DEBUG, "Run command %ld", command);
	assert(ob && command>=0 && command<MCom_query_nb_commands());
	MCom_begin(command);
	unsigned nb_params = MCom_query_nb_params(command);
	for (unsigned p=0; p<nb_params; p++) {
		for (unsigned po=0; po<MAX_NB_OBJ_PER_TYPE && current_objects[p][po]; po++) {	// flush those damned inputs
			fl_call_object_callback(current_objects[p][po]);
		}
		switch (MCom_query_param_type(command, p)) {
			case MCom_SEL:
			case MCom_BASIS:
			case MCom_COLOR:
				MCom_param_backref(current_params[p].uc);
				break;
			case MCom_VEC:
				if (current_params[p].uc == MCom_query_nb_backrefs(MCom_VEC)) {
					MCom_param_vec(current_params[p].f[0], current_params[p].f[1], current_params[p].f[2]);
				} else {
					MCom_param_backref(current_params[p].uc);
				}
				break;
			case MCom_INDEX:
				MCom_param_index(current_params[p].ul);
				break;
			case MCom_INT:
				MCom_param_int(current_params[p].uc);
				break;
			case MCom_VERSION:
				MCom_param_version(Grid_get_version());
				break;
			case MCom_REAL:
				MCom_param_real(current_params[p].f[0]);
				break;
			case MCom_BOOL:
				MCom_param_bool(current_params[p].uc);
				break;
			case MCom_GEOMTYPE:
				MCom_param_geomtype(current_params[p].uc);
				break;
			default:
				assert(0);
		}
	}
	if (!MCom_end()) {
		fl_show_alert("Invalid command", "", "The command failed to execute, because you did something wrong", 1);
	} else {
		modified = 1;
		if (! SrcBuf_append(mml, MCom_last_cmd_geta())) log_fatal("Cannot append to source view ?");
		reset_source();
		reset_dyn_choices();
		reset_gcl();
	}
	delete_action_form();
}
void action_x_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	current_params[p].f[0] = atof(fl_get_input(ob));
	log_warning(LOG_DEBUG, "Set param#%ld(X) to %f", p, (double)current_params[p].f[0]);
}
void action_y_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	current_params[p].f[1] = atof(fl_get_input(ob));
	log_warning(LOG_DEBUG, "Set param#%ld(Y) to %f", p, (double)current_params[p].f[1]);
}
void action_z_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	current_params[p].f[2] = atof(fl_get_input(ob));
	log_warning(LOG_DEBUG, "Set param#%ld(Z) to %f", p, (double)current_params[p].f[2]);
}
void action_vecbr_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	int c = fl_get_choice(ob);
	if (c>0) {
		current_params[p].uc = c-1;
		log_warning(LOG_DEBUG, "Set param#%ld(Vec) to backref %u", p, (unsigned)current_params[p].uc);
	}
}
void action_sel_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	int c = fl_get_choice(ob);
	if (c>0) {
		c --;
		current_params[p].uc = c;
		if ((unsigned)c<MCom_query_nb_backrefs(MCom_SEL)) {
			log_warning(LOG_DEBUG, "Set param#%ld(Sel) to backref %u", p, (unsigned)current_params[p].uc);
		} else {
			log_warning(LOG_DEBUG, "Sel param#%ld(Sel) to new", p);
		}
	}
}
void action_basis_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	int c = fl_get_choice(ob);
	if (c>0) {
		c --;
		current_params[p].uc = c;
		log_warning(LOG_DEBUG, "Set param#%ld(Basis) to backref %u", p, (unsigned)current_params[p].uc);
	}
}
void action_color_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	int c = fl_get_choice(ob);
	if (c>0) {
		c --;
		current_params[p].uc = c;
		log_warning(LOG_DEBUG, "Set param#%ld(Color) to backref %u", p, (unsigned)current_params[p].uc);
	}
}
void action_index_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	current_params[p].ul = strtoul(fl_get_input(ob), NULL, 10);
	log_warning(LOG_DEBUG, "Set param#%ld(Index) to %lu", p, current_params[p].ul);
}
void action_int_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	current_params[p].uc = strtoul(fl_get_input(ob), NULL, 10);
	log_warning(LOG_DEBUG, "Set param#%ld(Int) to %u", p, (unsigned)current_params[p].uc);
}
void action_real_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	current_params[p].f[0] = atof(fl_get_input(ob));
	log_warning(LOG_DEBUG, "Set param#%ld(Real) to %f", p, (double)current_params[p].f[0]);
}
void action_bool_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	current_params[p].uc = fl_get_button(ob)>0;
	log_warning(LOG_DEBUG, "Set param#%ld(Bool) to %u", p, (unsigned)current_params[p].uc);
}
void action_geomtype_cb(FL_OBJECT *ob, long p) {
	assert(ob && p < MAX_NB_PARAMS);
	int c = fl_get_choice(ob);
	if (c>0) {
		current_params[p].uc = c-1;
		log_warning(LOG_DEBUG, "Set param#%ld(Geomtype) to %u", p, (unsigned)current_params[p].uc);
	}
}

// vi:ts=3:sw=3
