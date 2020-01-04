
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "layTipDialog.h"
#include "layAbstractMenuProvider.h"
#include "edtPlugin.h"
#include "edtConfig.h"
#include "edtService.h"
#include "edtServiceImpl.h"
#include "edtMainService.h"
#include "edtPartialService.h"
#include "edtEditorOptionsPages.h"

#include <QApplication>

namespace edt
{

static 
void get_text_options (std::vector < std::pair<std::string, std::string> > &options)
{
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_string, "ABC"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_size, "0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_halign, "left"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_valign, "bottom"));
}

static 
void get_text_editor_options_pages (std::vector<edt::EditorOptionsPage *> &ret, lay::PluginRoot *)
{
  ret.push_back (new edt::EditorOptionsText ());
}

static 
void get_path_options (std::vector < std::pair<std::string, std::string> > &options)
{
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_width, "0.1"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_ext_type, "flush"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_ext_var_begin, "0.0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_ext_var_end, "0.0"));
}

static 
void get_path_editor_options_pages (std::vector<edt::EditorOptionsPage *> &ret, lay::PluginRoot *)
{
  ret.push_back (new EditorOptionsPath ());
}

static 
void get_inst_options (std::vector < std::pair<std::string, std::string> > &options)
{
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_cell_name, ""));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_angle, "0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_mirror, "false"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_array, "false"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_scale, "1.0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_rows, "1"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_row_x, "0.0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_row_y, "0.0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_columns, "1"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_column_x, "0.0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_column_y, "0.0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_inst_place_origin, "false"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_max_shapes_of_instances, "1000"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_show_shapes_of_instances, "true"));
}

static 
void get_inst_editor_options_pages (std::vector<edt::EditorOptionsPage *> &ret, lay::PluginRoot *root)
{
  ret.push_back (new EditorOptionsInst (root));
}

template <class Svc>
class PluginDeclaration
  : public PluginDeclarationBase
{
public:
  PluginDeclaration (const std::string &title, const std::string &mouse_mode, 
                     void (*option_get_f) (std::vector < std::pair<std::string, std::string> > &) = 0,
                     void (*pages_f) (std::vector <edt::EditorOptionsPage *> &, lay::PluginRoot *) = 0)
    : m_title (title), m_mouse_mode (mouse_mode), mp_option_get_f (option_get_f), mp_pages_f (pages_f)
  {
    //  .. nothing yet ..
  }

  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    if (mp_option_get_f != 0) {
      (*mp_option_get_f) (options);
    }
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> & /*menu_entries*/) const
  {
    //  .. nothing yet ..
  }

  virtual void get_editor_options_pages (std::vector<edt::EditorOptionsPage *> &pages, lay::PluginRoot *root) const
  {
    if (mp_pages_f != 0) {
      size_t nstart = pages.size ();
      (*mp_pages_f) (pages, root);
      while (nstart < pages.size ()) {
        pages [nstart++]->set_plugin_declaration (this);
      }
    }
  }

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::PluginRoot *, lay::LayoutView *view) const
  {
    Svc *service = new Svc (manager, view);
    service->set_plugin_declaration (this);
    return service;
  }

  virtual bool implements_editable (std::string &title) const
  {
    title = m_title;
    return true;
  }
  
  virtual bool implements_mouse_mode (std::string &title) const
  {
    title = m_mouse_mode;
    return true;
  }

private:
  std::string m_title;
  std::string m_mouse_mode;

  void (*mp_option_get_f) (std::vector < std::pair<std::string, std::string> > &options);
  void (*mp_pages_f) (std::vector <edt::EditorOptionsPage *> &, lay::PluginRoot *);
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl1 (
  new edt::PluginDeclaration<edt::PolygonService> (tl::to_string (QObject::tr ("Polygons")), "polygon:edit_mode\t" + tl::to_string (QObject::tr ("Polygon")) + "<:polygon.png>" + tl::to_string (QObject::tr ("{Create a polygon}"))), 
  4010, 
  "edt::Service(Polygons)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl2 (
  new edt::PluginDeclaration<edt::BoxService> (tl::to_string (QObject::tr ("Boxes")), "box:edit_mode\t" + tl::to_string (QObject::tr ("Box")) + "\t<:box.png>" + tl::to_string (QObject::tr ("{Create a box}"))), 
  4011, 
  "edt::Service(Boxes)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl3 (
  new edt::PluginDeclaration<edt::TextService> (tl::to_string (QObject::tr ("Texts")), "text:edit_mode\t" + tl::to_string (QObject::tr ("Text")) + "\t<:text.png>" + tl::to_string (QObject::tr ("{Create a text object}")), &get_text_options, &get_text_editor_options_pages), 
  4012, 
  "edt::Service(Texts)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl4 (
  new edt::PluginDeclaration<edt::PathService> (tl::to_string (QObject::tr ("Paths")), "path:edit_mode\t" + tl::to_string (QObject::tr ("Path")) + "\t<:path.png>" + tl::to_string (QObject::tr ("{Create a path}")), &get_path_options, &get_path_editor_options_pages), 
  4013, 
  "edt::Service(Paths)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl5 (
  new edt::PluginDeclaration<edt::InstService> (tl::to_string (QObject::tr ("Instances")), "instance:edit_mode\t" + tl::to_string (QObject::tr ("Instance")) + "\t<:instance.png>" + tl::to_string (QObject::tr ("{Create a cell instance}")), &get_inst_options, &get_inst_editor_options_pages), 
  4020, 
  "edt::Service(CellInstances)"
);

class MainPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  MainPluginDeclaration (const std::string &title)
    : mp_root (0), m_title (title), mp_obj_prop_dialog (0)
  {
    //  .. nothing yet ..
  }

  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_edit_top_level_selection, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_hier_copy_mode, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_grid, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_snap_to_objects, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_move_angle_mode, "any"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_connect_angle_mode, "any"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_combine_mode, "add"));
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);

    menu_entries.push_back (lay::MenuEntry ("edt::hier_group", "zoom_menu.end"));
    menu_entries.push_back (lay::MenuEntry ("edt::descend", "descend", "zoom_menu.end", tl::to_string (QObject::tr ("Descend")) + "(Ctrl+D)"));
    menu_entries.push_back (lay::MenuEntry ("edt::ascend", "ascend", "zoom_menu.end", tl::to_string (QObject::tr ("Ascend")) + "(Ctrl+A)"));

    menu_entries.push_back (lay::MenuEntry ("edit_options_group:edit_mode", "edit_menu.end"));
    menu_entries.push_back (lay::MenuEntry ("edt::edit_options", "edit_options:edit_mode", "edit_menu.end", tl::to_string (QObject::tr ("Editor Options")) + "(F3)"));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_make_array", "make_array:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Make Array"))));
    menu_entries.push_back (lay::MenuEntry ("selection_group:edit_mode", "edit_menu.selection_menu.end"));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_change_layer", "change_layer:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Change Layer"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_tap", "tap:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Tap")) + "(T)"));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_align", "align:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Align"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_round_corners", "round_corners:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Round Corners"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_size", "size:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Size Shapes"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_union", "union:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Merge Shapes"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_intersection", "intersection:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Intersection - Others With First"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_difference", "difference:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Subtraction - Others From First"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_separate", "separate:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Separate - First into Inside/Outside Others"))));
    menu_entries.push_back (lay::MenuEntry ("hier_group:edit_mode", "edit_menu.selection_menu.end"));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_flatten_insts", "flatten_insts:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Flatten Instances"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_resolve_arefs", "resolve_arefs:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Resolve Arrays"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_move_hier_up", "move_hier_up:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Move Up In Hierarchy"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_make_cell", "make_cell:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Make Cell"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_make_cell_variants", "make_cell_variants:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Make Cell Variants"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_convert_to_pcell", "convert_to_pcell:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Convert To PCell"))));
    menu_entries.push_back (lay::MenuEntry ("edt::sel_convert_to_cell", "convert_to_cell:edit_mode", "edit_menu.selection_menu.end", tl::to_string (QObject::tr ("Convert To Static Cell"))));

    menu_entries.push_back (lay::MenuEntry ("edt::combine_mode", "combine_mode:edit_mode", "@toolbar.end_modes", tl::to_string (QObject::tr ("Combine{Select background combination mode}"))));
  }

  bool configure (const std::string &name, const std::string &value)
  {
    if (name == cfg_edit_combine_mode) {
      combine_mode_type cm = CM_Add;
      CMConverter ().from_string (value, cm);
      update_menu (cm);
    }

    return false;
  }

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::PluginRoot *root, lay::LayoutView *view) const
  {
    return new edt::MainService (manager, view, root);
  }

  virtual bool implements_editable (std::string & /*title*/) const
  {
    return false;
  }
  
  virtual bool implements_mouse_mode (std::string & /*title*/) const
  {
    return false;
  }

  virtual void initialize (lay::PluginRoot *root)
  {
    lay::AbstractMenuProvider *mp = lay::AbstractMenuProvider::instance ();
    if (! mp) {
      return;
    }

    mp_root = root;

    //  create the editor options dialog
    m_prop_dialog_pages.push_back (new edt::EditorOptionsGeneric ());

    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      const PluginDeclarationBase *pd_base = dynamic_cast<const PluginDeclarationBase *> (&*cls);
      if (pd_base) {
        pd_base->get_editor_options_pages (m_prop_dialog_pages, root);
      }
    }

    mp_obj_prop_dialog = new edt::EditorOptionsPages (m_prop_dialog_pages, root);

    for (std::vector<edt::EditorOptionsPage *>::const_iterator op = m_prop_dialog_pages.begin (); op != m_prop_dialog_pages.end (); ++op) {
      if ((*op)->plugin_declaration () != 0) {
        (*op)->activate (false);
      }
    }

    //  add entries to the combine mode dialog
    lay::Action combine_menu = mp->menu ()->action ("@toolbar.combine_mode");

    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_add",   *mp->create_config_action (tl::to_string (QObject::tr ("Add<:/cm_add.png>{Add shapes}")),   cfg_edit_combine_mode, CMConverter ().to_string (CM_Add)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_merge", *mp->create_config_action (tl::to_string (QObject::tr ("Merge<:/cm_merge.png>{Merge shapes with background}")), cfg_edit_combine_mode, CMConverter ().to_string (CM_Merge)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_erase", *mp->create_config_action (tl::to_string (QObject::tr ("Erase<:/cm_erase.png>{Erase shape from background}")), cfg_edit_combine_mode, CMConverter ().to_string (CM_Erase)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_mask",  *mp->create_config_action (tl::to_string (QObject::tr ("Mask<:/cm_mask.png>{Mask background with shape}")),  cfg_edit_combine_mode, CMConverter ().to_string (CM_Mask)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_diff",  *mp->create_config_action (tl::to_string (QObject::tr ("Diff<:/cm_diff.png>{Compute difference of shape with background}")),  cfg_edit_combine_mode, CMConverter ().to_string (CM_Diff)));

    update_menu (CM_Add);
  }

  void update_menu (combine_mode_type cm)
  {
    lay::AbstractMenuProvider *mp = lay::AbstractMenuProvider::instance ();
    if (! mp) {
      return;
    }

    lay::Action combine_menu = mp->menu ()->action ("@toolbar.combine_mode");

    if (cm == CM_Add) {
      combine_menu.set_title (tl::to_string (QObject::tr ("Add")));
      combine_menu.set_icon (":/cm_add.png");
    } else if (cm == CM_Merge) {
      combine_menu.set_title (tl::to_string (QObject::tr ("Merge")));
      combine_menu.set_icon (":/cm_merge.png");
    } else if (cm == CM_Erase) {
      combine_menu.set_title (tl::to_string (QObject::tr ("Erase")));
      combine_menu.set_icon (":/cm_erase.png");
    } else if (cm == CM_Mask) {
      combine_menu.set_title (tl::to_string (QObject::tr ("Mask")));
      combine_menu.set_icon (":/cm_mask.png");
    } else if (cm == CM_Diff) {
      combine_menu.set_title (tl::to_string (QObject::tr ("Diff")));
      combine_menu.set_icon (":/cm_diff.png");
    }
  }

  virtual void uninitialize (lay::PluginRoot *)
  {
    if (mp_obj_prop_dialog) {
      delete mp_obj_prop_dialog;
      mp_obj_prop_dialog = 0;
    }
  }

  virtual void config_finalize ()
  {
    if (mp_obj_prop_dialog && mp_obj_prop_dialog->isVisible ()) {
      mp_obj_prop_dialog->setup ();
    }
  }

  void show_dialog () const
  {
    if (mp_obj_prop_dialog) {
      if (! mp_obj_prop_dialog->isVisible ()) {
        mp_obj_prop_dialog->setup ();
        mp_obj_prop_dialog->show ();
      }
      mp_obj_prop_dialog->activateWindow ();
      mp_obj_prop_dialog->raise ();
    }
  }

  void activate (const lay::PluginDeclaration *pd, bool active) const
  {
    for (std::vector<edt::EditorOptionsPage *>::const_iterator op = m_prop_dialog_pages.begin (); op != m_prop_dialog_pages.end (); ++op) {
      if ((*op)->plugin_declaration () == pd) {
        (*op)->activate (active);
      }
    }
  }

  void initialized (lay::PluginRoot *root)
  {
    lay::AbstractMenuProvider *mp = lay::AbstractMenuProvider::instance ();
    if (! mp) {
      return;
    }

    lay::Action combine_menu = mp->menu ()->action ("@toolbar.combine_mode");

    //  Do some more initialization here.

    combine_mode_type cm = CM_Add;
    root->config_get (cfg_edit_combine_mode, cm, CMConverter ());
    if (cm != CM_Add && combine_menu.is_visible ()) {
      lay::TipDialog td (QApplication::activeWindow (), 
                    tl::to_string (QObject::tr ("The background combination mode of the shape editor is set to some other mode than 'Add'.\n"
                                                "This can be confusing, because a shape may not be drawn as expected.\n\nTo switch back to normal mode, choose 'Add' for the background combination mode in the toolbar.")), 
                    "has-non-add-edit-combine-mode");
      if (td.exec_dialog ()) {
        //  Don't bother the user with more dialogs.
        return;
      }
    }
  }

private:
  lay::PluginRoot *mp_root;
  std::string m_title;
  edt::EditorOptionsPages *mp_obj_prop_dialog;
  std::vector<edt::EditorOptionsPage *> m_prop_dialog_pages;
};

void
show_editor_options_dialog ()
{
  //  look for the plugin declaration and show the dialog
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    const MainPluginDeclaration *main_pd = dynamic_cast<const MainPluginDeclaration *> (&*cls);
    if (main_pd) {
      main_pd->show_dialog ();
      break;
    }
  }
}

void 
activate_service (const lay::PluginDeclaration *pd, bool active)
{
  //  look for the plugin declaration and show the dialog
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    const MainPluginDeclaration *main_pd = dynamic_cast<const MainPluginDeclaration *> (&*cls);
    if (main_pd) {
      main_pd->activate (pd, active);
      break;
    }
  }
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl20 (new edt::MainPluginDeclaration (tl::to_string (QObject::tr ("Instances and shapes"))), 4000, "edt::MainService");

class PartialPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  PartialPluginDeclaration (const std::string &title, const std::string &mouse_mode)
    : m_title (title), m_mouse_mode (mouse_mode)
  {
    //  .. nothing yet ..
  }

  virtual void get_options (std::vector < std::pair<std::string, std::string> > & /*options*/) const
  {
    //  .. nothing yet ..
  }

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::PluginRoot *root, lay::LayoutView *view) const
  {
    return new edt::PartialService (manager, view, root);
  }

  virtual bool implements_editable (std::string &title) const
  {
    title = m_title;
    return true;
  }
  
  virtual bool implements_mouse_mode (std::string &title) const
  {
    title = m_mouse_mode;
    return true;
  }

private:
  std::string m_title;
  std::string m_mouse_mode;
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl30 (
  new edt::PartialPluginDeclaration (tl::to_string (QObject::tr ("Partial shapes")), "partial:edit_mode\t" + tl::to_string (QObject::tr ("Partial{Edit points and edges of shapes}")) + "<:partial.png>"), 
  4030, 
  "edt::PartialService"
);

}

