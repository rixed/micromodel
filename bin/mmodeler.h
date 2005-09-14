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
#ifndef MMODELER_H_041214
#define MMODELER_H_041214

#include <X11/Xlib.h>
#include <X11/forms.h>
#include <tiffio.h>
#include "libmicromodel/mcommander.h"
#include "srcbuf.h"

#define MAX_NB_OBJ_PER_TYPE 4
extern FL_OBJECT *current_objects[MAX_NB_PARAMS][MAX_NB_OBJ_PER_TYPE];
extern char last_filename[];
extern FL_FORM *form_action;
extern char modified;
extern SrcBuf *mml;
extern char tiff_filename[];
extern uint32 *tiff_data;
extern uint32 tiff_size;
extern XImage tiff_ximage;
extern FL_OBJECT *uv_x_input;
extern FL_OBJECT *uv_y_input;

void reset_gcl(void);
void reset_tiff(void);
void reset_dyn_choices(void);
void reset_fileinfo(const char *filename);
int reset_to_mml(const char *filename);
int load_mml(const char *buf);
char *get_source(void);
void reset_source(void);
void reset_grid(void);
void delete_action_form(void);
FL_FORM *create_form_action(unsigned command);

/* Additionnal callbacks */

void action_cb(FL_OBJECT *ob, long data);
void cancel_action_cb(FL_OBJECT *ob, long data);
void accept_action_cb(FL_OBJECT *ob, long data);
void action_grid_cb(FL_OBJECT *ob, long data);
void action_x_cb(FL_OBJECT *ob, long data);
void action_y_cb(FL_OBJECT *ob, long data);
void action_z_cb(FL_OBJECT *ob, long data);
void action_vecbr_cb(FL_OBJECT *ob, long data);
void action_sel_cb(FL_OBJECT *ob, long data);
void action_basis_cb(FL_OBJECT *ob, long data);
void action_color_cb(FL_OBJECT *ob, long data);
void action_index_cb(FL_OBJECT *ob, long data);
void action_int_cb(FL_OBJECT *ob, long data);
void action_real_cb(FL_OBJECT *ob, long data);
void action_bool_cb(FL_OBJECT *ob, long data);
void action_geomtype_cb(FL_OBJECT *ob, long data);
void action_maptype_cb(FL_OBJECT *ob, long data);
void mapdisplay_redraw(void);
int mapdisplay_expose(FL_OBJECT *ob, Window win, int w, int h, XEvent *xev, void *ud);
int mapdisplay_click(FL_OBJECT *ob, Window win, int w, int h, XEvent *xev, void *ud);

#endif
// vi:ts=3:sw=3
