/* Form definition file generated with fdesign. */

#include <GL/glx.h>
#include <X11/forms.h>
#include <stdlib.h>
#include "mmodel_ui.h"

FL_FORM *MicroModeler;

FL_OBJECT
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

static FL_PUP_ENTRY fdmenu_File_0[] =
{ 
    /*  itemtext   callback  shortcut   mode */
    { "Load",	0,	"l",	 FL_PUP_NONE},
    { "Save",	0,	"s",	 FL_PUP_NONE},
    { "Export",	0,	"e",	 FL_PUP_NONE},
    { "Load TIFF",	0,	"t",	 FL_PUP_NONE},
    { "Quit",	0,	"q",	 FL_PUP_NONE},
    {0}
};

static FL_PUP_ENTRY fdmenu_Edit_1[] =
{ 
    /*  itemtext   callback  shortcut   mode */
    { "Undo",	0,	"u",	 FL_PUP_NONE},
    { "Redo",	0,	"r",	 FL_PUP_NONE},
    {0}
};

void create_form_MicroModeler(void)
{
  FL_OBJECT *obj;

  if (MicroModeler)
     return;

  MicroModeler = fl_bgn_form(FL_NO_BOX,420,820);
  obj = fl_add_box(FL_FLAT_BOX,0,0,420,820,"");
  obj = fl_add_box(FL_FRAME_BOX,0,380,420,90,"");
  action_box = obj = fl_add_box(FL_FRAME_BOX,0,150,420,230,"");
  obj = fl_add_box(FL_FRAME_BOX,0,30,420,120,"");
  obj = fl_add_frame(FL_ENGRAVED_FRAME,0,0,420,30,"");
  obj = fl_add_menu(FL_PULLDOWN_MENU,0,0,60,30,"File");
    fl_set_object_shortcut(obj,"#f",1);
    fl_set_object_boxtype(obj,FL_NO_BOX);
    fl_set_object_callback(obj,menu_file_cb,0);
    fl_set_menu_entries(obj, fdmenu_File_0);
  obj = fl_add_menu(FL_PULLDOWN_MENU,60,0,60,30,"Edit");
    fl_set_object_shortcut(obj,"#e",1);
    fl_set_object_boxtype(obj,FL_NO_BOX);
    fl_set_object_callback(obj,menu_edit_cb,0);
    fl_set_menu_entries(obj, fdmenu_Edit_1);
  obj = fl_add_button(FL_NORMAL_BUTTON,360,0,60,30,"Help");
    fl_set_button_shortcut(obj,"#h",1);
    fl_set_object_boxtype(obj,FL_NO_BOX);
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_callback(obj,help_cb,0);
  infotext_file = obj = fl_add_text(FL_NORMAL_TEXT,0,30,420,20,"file:");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
  infotext_sizes = obj = fl_add_text(FL_NORMAL_TEXT,0,50,420,20,"size:");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
  args = obj = fl_add_input(FL_MULTILINE_INPUT,40,90,380,60,"args:");
    fl_set_object_boxtype(obj,FL_BORDER_BOX);
    fl_set_object_callback(obj,args_cb,0);
  select_sel = obj = fl_add_choice(FL_NORMAL_CHOICE2,70,380,280,30,"Selection :");
    fl_set_object_shortcut(obj,"#s",1);
    fl_set_object_boxtype(obj,FL_SHADOW_BOX);
    fl_set_object_callback(obj,select_sel_cb,0);
  select_basis = obj = fl_add_choice(FL_NORMAL_CHOICE2,70,410,280,30,"Basis :");
    fl_set_object_shortcut(obj,"#b",1);
    fl_set_object_boxtype(obj,FL_SHADOW_BOX);
    fl_set_object_callback(obj,select_basis_cb,0);
  select_color = obj = fl_add_choice(FL_NORMAL_CHOICE2,70,440,280,30,"Color :");
    fl_set_object_shortcut(obj,"#c",1);
    fl_set_object_boxtype(obj,FL_SHADOW_BOX);
    fl_set_object_callback(obj,select_color_cb,0);
  obj = fl_add_box(FL_FRAME_BOX,0,470,420,350,"");
  source_textbox = obj = fl_add_input(FL_MULTILINE_INPUT,0,490,420,300,"Source");
    fl_set_object_lalign(obj,FL_ALIGN_TOP);
    fl_set_object_callback(obj,source_cb,0);
  obj = fl_add_button(FL_NORMAL_BUTTON,300,790,120,30,"Accept");
    fl_set_button_shortcut(obj,"^A",1);
    fl_set_object_boxtype(obj,FL_SHADOW_BOX);
    fl_set_object_callback(obj,accept_source_cb,0); add_action_buttons();
  infotext_mapfile = obj = fl_add_text(FL_NORMAL_TEXT,0,70,360,20,"texture:");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
  mapview = obj = fl_add_lightbutton(FL_PUSH_BUTTON,360,70,60,20,"view");
    fl_set_object_callback(obj,mapview_cb,0);
  fl_end_form();

}
/*---------------------------------------*/

FL_FORM *Help;


void create_form_Help(void)
{
  FL_OBJECT *obj;

  if (Help)
     return;

  Help = fl_bgn_form(FL_NO_BOX,520,370);
  obj = fl_add_box(FL_FLAT_BOX,0,0,520,370,"");
  obj = fl_add_text(FL_NORMAL_TEXT,0,0,520,30,"Micro Modeler");
    fl_set_object_boxtype(obj,FL_FRAME_BOX);
    fl_set_object_color(obj,FL_DARKER_COL1,FL_BLACK);
    fl_set_object_lcolor(obj,FL_WHITE);
    fl_set_object_lsize(obj,FL_NORMAL_SIZE);
    fl_set_object_lalign(obj,FL_ALIGN_TOP|FL_ALIGN_INSIDE);
    fl_set_object_lstyle(obj,FL_BOLD_STYLE+FL_EMBOSSED_STYLE);
  obj = fl_add_text(FL_NORMAL_TEXT,30,50,460,280,"MicroModeler is a modeling tool for precise and concise 3D modeling\n\nhomepage : http://happyleptic.org/micromodel.php\n\nSend bugs to : rixed@happyleptic.org\n\n\n(if you haven't run this as a geomview plugin, you might have trouble using it)");
    fl_set_object_lalign(obj,FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
  obj = fl_add_button(FL_RETURN_BUTTON,430,330,80,30,"Hide");
    fl_set_object_boxtype(obj,FL_SHADOW_BOX);
    fl_set_object_callback(obj,hide_help_cb,0);
  fl_end_form();

}
/*---------------------------------------*/

FL_FORM *MapView;

FL_OBJECT
        *mapinfotext,
        *coordinfotext,
        *mapdisplay;

void create_form_MapView(void)
{
  FL_OBJECT *obj;

  if (MapView)
     return;

  MapView = fl_bgn_form(FL_NO_BOX,580,590);
  obj = fl_add_box(FL_UP_BOX,0,0,580,590,"");
  mapinfotext = obj = fl_add_text(FL_NORMAL_TEXT,10,560,270,20,"info:");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
  coordinfotext = obj = fl_add_text(FL_NORMAL_TEXT,300,560,270,20,"coord:");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
  mapdisplay = obj = fl_add_glcanvas(FL_NORMAL_CANVAS,10,10,560,550,"");
    fl_set_object_boxtype(obj,FL_BORDER_BOX);
  fl_end_form();

}
/*---------------------------------------*/

void create_the_forms(void)
{
  create_form_MicroModeler();
  create_form_Help();
  create_form_MapView();
}

