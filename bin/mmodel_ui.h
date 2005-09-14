/** Header file generated with fdesign on Thu Jun 30 06:59:01 2005.**/

#ifndef FD_MicroModeler_h_
#define FD_MicroModeler_h_

/** Callbacks, globals and object handlers **/
extern void menu_file_cb(FL_OBJECT *, long);
extern void menu_edit_cb(FL_OBJECT *, long);
extern void help_cb(FL_OBJECT *, long);
extern void args_cb(FL_OBJECT *, long);
extern void select_sel_cb(FL_OBJECT *, long);
extern void select_basis_cb(FL_OBJECT *, long);
extern void select_color_cb(FL_OBJECT *, long);
extern void source_cb(FL_OBJECT *, long);
extern void accept_source_cb(FL_OBJECT *, long);
extern void mapview_cb(FL_OBJECT *, long);

extern void hide_help_cb(FL_OBJECT *, long);



/**** Forms and Objects ****/
extern FL_FORM *MicroModeler;

extern FL_OBJECT
        *action_box,
        *infotext_file,
        *infotext_sizes,
        *args,
        *select_sel,
        *select_basis,
        *select_color,
        *source_textbox,
        *infotext_mapfile,
        *mapview;

extern FL_FORM *Help;

extern FL_FORM *MapView;

extern FL_OBJECT
        *mapinfotext,
        *coordinfotext,
        *mapdisplay;


/**** Creation Routine ****/
extern void create_the_forms(void);

#endif /* FD_MicroModeler_h_ */
