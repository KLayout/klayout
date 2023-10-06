
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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


#if defined(HAVE_QT)
#  include "layTipDialog.h"
#  include "layEditorOptionsPages.h"
#endif

#include "layDispatcher.h"
#include "layLayoutViewBase.h"
#include "edtPlugin.h"
#include "edtConfig.h"
#include "edtService.h"
#include "edtServiceImpl.h"
#include "edtMainService.h"
#include "edtPartialService.h"
#if defined(HAVE_QT)
#  include "edtEditorOptionsPages.h"
#  include "edtRecentConfigurationPage.h"
#endif

#if defined(HAVE_QT)
#  include <QApplication>
#  include <QLayout>
#endif

namespace edt
{

#if defined(HAVE_QT)
edt::RecentConfigurationPage::ConfigurationDescriptor shape_cfg_descriptors[] =
{
  edt::RecentConfigurationPage::ConfigurationDescriptor ("", tl::to_string (tr ("Layer")), edt::RecentConfigurationPage::Layer),
};
#endif

#if defined(HAVE_QT)
static
void get_shape_editor_options_pages (std::vector<lay::EditorOptionsPage *> &ret, lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
{
  ret.push_back (new RecentConfigurationPage (view, dispatcher, "edit-recent-shape-param",
                        &shape_cfg_descriptors[0], &shape_cfg_descriptors[sizeof (shape_cfg_descriptors) / sizeof (shape_cfg_descriptors[0])]));
}
#else
static void get_shape_editor_options_pages () { }
#endif

static
void get_text_options (std::vector < std::pair<std::string, std::string> > &options)
{
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_string, "ABC"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_size, "0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_halign, "left"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_text_valign, "bottom"));
}

#if defined(HAVE_QT)
static
void get_text_editor_options_pages (std::vector<lay::EditorOptionsPage *> &ret, lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
{
  static edt::RecentConfigurationPage::ConfigurationDescriptor text_cfg_descriptors[] =
  {
    edt::RecentConfigurationPage::ConfigurationDescriptor ("", tl::to_string (tr ("Layer")), edt::RecentConfigurationPage::Layer),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_string, tl::to_string (tr ("Text")), edt::RecentConfigurationPage::Text),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_size, tl::to_string (tr ("Size")), edt::RecentConfigurationPage::Double),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_halign, tl::to_string (tr ("Hor. align")), edt::RecentConfigurationPage::Text),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_valign, tl::to_string (tr ("Vert. align")), edt::RecentConfigurationPage::Text)
  };

  ret.push_back (new RecentConfigurationPage (view, dispatcher, "edit-recent-text-param",
                        &text_cfg_descriptors[0], &text_cfg_descriptors[sizeof (text_cfg_descriptors) / sizeof (text_cfg_descriptors[0])]));
  ret.push_back (new edt::EditorOptionsText (view, dispatcher));
}
#else
static void get_text_editor_options_pages () { }
#endif

static 
void get_path_options (std::vector < std::pair<std::string, std::string> > &options)
{
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_width, "0.1"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_ext_type, "flush"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_ext_var_begin, "0.0"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_path_ext_var_end, "0.0"));
}

#if defined(HAVE_QT)
static
void get_path_editor_options_pages (std::vector<lay::EditorOptionsPage *> &ret, lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
{
  static edt::RecentConfigurationPage::ConfigurationDescriptor path_cfg_descriptors[] =
  {
    edt::RecentConfigurationPage::ConfigurationDescriptor ("", tl::to_string (tr ("Layer")), edt::RecentConfigurationPage::Layer),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_width, tl::to_string (tr ("Width")), edt::RecentConfigurationPage::Double),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_ext_type, tl::to_string (tr ("Ends")), edt::RecentConfigurationPage::Int),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_ext_var_begin, tl::to_string (tr ("Begin ext.")), edt::RecentConfigurationPage::Double),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_ext_var_end, tl::to_string (tr ("End ext.")), edt::RecentConfigurationPage::Double)
  };
  
  ret.push_back (new RecentConfigurationPage (view, dispatcher, "edit-recent-path-param",
                        &path_cfg_descriptors[0], &path_cfg_descriptors[sizeof (path_cfg_descriptors) / sizeof (path_cfg_descriptors[0])]));
  ret.push_back (new EditorOptionsPath (view, dispatcher));
}
#else
static void get_path_editor_options_pages () { }
#endif

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
  options.push_back (std::pair<std::string, std::string> (cfg_edit_pcell_show_parameter_names, "false"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_max_shapes_of_instances, "1000"));
  options.push_back (std::pair<std::string, std::string> (cfg_edit_show_shapes_of_instances, "true"));
}

#if defined(HAVE_QT)
static
void get_inst_editor_options_pages (std::vector<lay::EditorOptionsPage *> &ret, lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
{
  static edt::RecentConfigurationPage::ConfigurationDescriptor inst_cfg_descriptors[] =
  {
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_lib_name, tl::to_string (tr ("Library")), edt::RecentConfigurationPage::CellLibraryName),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_cell_name, tl::to_string (tr ("Cell")), edt::RecentConfigurationPage::CellDisplayName),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_angle, tl::to_string (tr ("Angle")), edt::RecentConfigurationPage::Double),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_mirror, tl::to_string (tr ("Mirror")), edt::RecentConfigurationPage::Bool),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_scale, tl::to_string (tr ("Scale")), edt::RecentConfigurationPage::Double),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_array, tl::to_string (tr ("Array")), edt::RecentConfigurationPage::ArrayFlag),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_rows, tl::to_string (tr ("Rows")), edt::RecentConfigurationPage::IntIfArray),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_row_x, tl::to_string (tr ("Row step (x)")), edt::RecentConfigurationPage::DoubleIfArray),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_row_y, tl::to_string (tr ("Row step (y)")), edt::RecentConfigurationPage::DoubleIfArray),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_columns, tl::to_string (tr ("Columns")), edt::RecentConfigurationPage::IntIfArray),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_column_x, tl::to_string (tr ("Column step (x)")), edt::RecentConfigurationPage::DoubleIfArray),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_column_y, tl::to_string (tr ("Column step (y)")), edt::RecentConfigurationPage::DoubleIfArray),
    edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_pcell_parameters, tl::to_string (tr ("PCell parameters")), edt::RecentConfigurationPage::PCellParameters)
  };

  ret.push_back (new RecentConfigurationPage (view, dispatcher, "edit-recent-inst-param",
                        &inst_cfg_descriptors[0], &inst_cfg_descriptors[sizeof (inst_cfg_descriptors) / sizeof (inst_cfg_descriptors[0])]));
  ret.push_back (new EditorOptionsInstPCellParam (view, dispatcher));
  ret.push_back (new EditorOptionsInst (view, dispatcher));
}
#else
static void get_inst_editor_options_pages () { }
#endif

template <class Svc>
class PluginDeclaration
  : public PluginDeclarationBase
{
public:
#if defined(HAVE_QT)
  PluginDeclaration (const std::string &title, const std::string &mouse_mode, 
                     void (*option_get_f) (std::vector < std::pair<std::string, std::string> > &) = 0,
                     void (*pages_f) (std::vector <lay::EditorOptionsPage *> &, lay::LayoutViewBase *, lay::Dispatcher *) = 0)
    : m_title (title), m_mouse_mode (mouse_mode), mp_option_get_f (option_get_f), mp_pages_f (pages_f)
  {
    //  .. nothing yet ..
  }
#else
  PluginDeclaration (const std::string &title, const std::string &mouse_mode,
                     void (*option_get_f) (std::vector < std::pair<std::string, std::string> > &) = 0,
                     void (*pages_f) () = 0)
    : m_title (title), m_mouse_mode (mouse_mode), mp_option_get_f (option_get_f), mp_pages_f (pages_f)
  {
    //  .. nothing yet ..
  }
#endif

  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    if (mp_option_get_f != 0) {
      (*mp_option_get_f) (options);
    }
  }

#if defined(HAVE_QT)
  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0;
  }
#endif

  virtual void get_menu_entries (std::vector<lay::MenuEntry> & /*menu_entries*/) const
  {
    //  .. nothing yet ..
  }

#if defined(HAVE_QT)
  virtual void get_editor_options_pages (std::vector<lay::EditorOptionsPage *> &pages, lay::LayoutViewBase *view, lay::Dispatcher *root) const
  {
    if (mp_pages_f != 0) {
      size_t nstart = pages.size ();
      (*mp_pages_f) (pages, view, root);
      while (nstart < pages.size ()) {
        pages [nstart++]->set_plugin_declaration (this);
      }
    }
  }
#endif

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::Dispatcher *, lay::LayoutViewBase *view) const
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
    if (! m_mouse_mode.empty ()) {
      title = m_mouse_mode;
      return true;
    } else {
      return false;
    }
  }

private:
  std::string m_title;
  std::string m_mouse_mode;

  void (*mp_option_get_f) (std::vector < std::pair<std::string, std::string> > &options);
#if defined(HAVE_QT)
  void (*mp_pages_f) (std::vector <lay::EditorOptionsPage *> &, lay::LayoutViewBase *, lay::Dispatcher *);
#else
  void (*mp_pages_f) ();
#endif
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl1 (
  new edt::PluginDeclaration<edt::PolygonService> (tl::to_string (tr ("Polygons")), "polygon:edit_mode\t" + tl::to_string (tr ("Polygon")) + "<:polygon_24px.png>" + tl::to_string (tr ("{Create a polygon}")), 0, &get_shape_editor_options_pages),
  4010, 
  "edt::Service(Polygons)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl2 (
  new edt::PluginDeclaration<edt::BoxService> (tl::to_string (tr ("Boxes")), "box:edit_mode\t" + tl::to_string (tr ("Box")) + "\t<:box_24px.png>" + tl::to_string (tr ("{Create a box}")), 0, &get_shape_editor_options_pages),
  4011, 
  "edt::Service(Boxes)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl3 (
  new edt::PluginDeclaration<edt::TextService> (tl::to_string (tr ("Texts")), "text:edit_mode\t" + tl::to_string (tr ("Text")) + "\t<:text_24px.png>" + tl::to_string (tr ("{Create a text object}")), &get_text_options, &get_text_editor_options_pages),
  4012, 
  "edt::Service(Texts)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl4 (
  new edt::PluginDeclaration<edt::PathService> (tl::to_string (tr ("Paths")), "path:edit_mode\t" + tl::to_string (tr ("Path")) + "\t<:path_24px.png>" + tl::to_string (tr ("{Create a path}")), &get_path_options, &get_path_editor_options_pages),
  4013, 
  "edt::Service(Paths)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl5 (
  new edt::PluginDeclaration<edt::PointService> (tl::to_string (tr ("Points")), std::string (), 0, &get_shape_editor_options_pages),
  4014,
  "edt::Service(Points)"
);
static tl::RegisteredClass<lay::PluginDeclaration> config_decl6 (
  new edt::PluginDeclaration<edt::InstService> (tl::to_string (tr ("Instances")), "instance:edit_mode\t" + tl::to_string (tr ("Instance")) + "\t<:instance_24px.png>" + tl::to_string (tr ("{Create a cell instance}")), &get_inst_options, &get_inst_editor_options_pages),
  4020, 
  "edt::Service(CellInstances)"
);

template <class Service>
bool is_enabled ()
{
  for (auto p = tl::Registrar<lay::PluginDeclaration>::begin (); p != tl::Registrar<lay::PluginDeclaration>::end (); ++p) {
    auto pd = dynamic_cast<const edt::PluginDeclaration<Service> *> (p.operator-> ());
    if (pd) {
      return pd->editable_enabled ();
    }
  }
  return false;
}

bool polygons_enabled () { return is_enabled<edt::PolygonService> (); }
bool paths_enabled () { return is_enabled<edt::PathService> (); }
bool boxes_enabled () { return is_enabled<edt::BoxService> (); }
bool points_enabled () { return is_enabled<edt::PointService> (); }
bool texts_enabled () { return is_enabled<edt::TextService> (); }
bool instances_enabled () { return is_enabled<edt::InstService> (); }

class MainPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  MainPluginDeclaration (const std::string &title)
    : mp_root (0), m_title (title)
  {
    //  .. nothing yet ..
  }

  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_edit_top_level_selection, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_hier_copy_mode, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_grid, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_snap_to_objects, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_snap_objects_to_grid, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_move_angle_mode, "any"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_connect_angle_mode, "any"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_combine_mode, "add"));
  }

#if defined(HAVE_QT)
  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0;
  }
#endif

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);

    menu_entries.push_back (lay::separator ("edt::hier_group", "zoom_menu.end"));
    menu_entries.push_back (lay::menu_item ("edt::descend", "descend", "zoom_menu.end", tl::to_string (tr ("Descend")) + "(Ctrl+D)"));
    menu_entries.push_back (lay::menu_item ("edt::ascend", "ascend", "zoom_menu.end", tl::to_string (tr ("Ascend")) + "(Ctrl+A)"));

    menu_entries.push_back (lay::menu_item ("edt::sel_make_array", "make_array:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Make Array"))));
    menu_entries.push_back (lay::separator ("selection_group:edit_mode", "edit_menu.selection_menu.end"));
    menu_entries.push_back (lay::menu_item ("edt::sel_change_layer", "change_layer:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Change Layer"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_tap", "tap", "edit_menu.selection_menu.end", tl::to_string (tr ("Tap")) + "(T)"));
    menu_entries.push_back (lay::menu_item ("edt::sel_align", "align:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Align"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_distribute", "distribute:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Distribute"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_round_corners", "round_corners:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Round Corners"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_size", "size:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Size Shapes"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_union", "union:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Merge Shapes"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_intersection", "intersection:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Intersection - Others With First"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_difference", "difference:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Subtraction - Others From First"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_separate", "separate:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Separate - First into Inside/Outside Others"))));
    menu_entries.push_back (lay::separator ("hier_group:edit_mode", "edit_menu.selection_menu.end"));
    menu_entries.push_back (lay::menu_item ("edt::sel_flatten_insts", "flatten_insts:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Flatten Instances"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_resolve_arefs", "resolve_arefs:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Resolve Arrays"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_move_hier_up", "move_hier_up:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Move Up In Hierarchy"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_make_cell", "make_cell:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Make Cell"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_make_cell_variants", "make_cell_variants:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Make Cell Variants"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_convert_to_pcell", "convert_to_pcell:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Convert To PCell"))));
    menu_entries.push_back (lay::menu_item ("edt::sel_convert_to_cell", "convert_to_cell:edit_mode", "edit_menu.selection_menu.end", tl::to_string (tr ("Convert To Static Cell"))));
    menu_entries.push_back (lay::separator ("hier_group:edit_info", "edit_menu.selection_menu.end"));
    menu_entries.push_back (lay::menu_item ("edt::sel_area_perimeter", "area_perimeter", "edit_menu.selection_menu.end", tl::to_string (tr ("Area and Perimeter"))));

    menu_entries.push_back (lay::menu_item ("edt::combine_mode", "combine_mode:edit_mode", "@toolbar.end_modes", tl::to_string (tr ("Combine{Select background combination mode}"))));
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

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::Dispatcher *root, lay::LayoutViewBase *view) const
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

#if defined(HAVE_QT)
  virtual void get_editor_options_pages (std::vector<lay::EditorOptionsPage *> &pages, lay::LayoutViewBase *view, lay::Dispatcher *dispatcher) const
  {
    //  NOTE: we do not set plugin_declaration which makes the page unspecific
    EditorOptionsGeneric *generic_opt = new EditorOptionsGeneric (view, dispatcher);
    pages.push_back (generic_opt);
  }
#endif

  virtual void initialize (lay::Dispatcher *root)
  {
    lay::Dispatcher *mp = lay::Dispatcher::instance ();
    if (! mp || ! mp->has_ui ()) {
      return;
    }

    mp_root = root;

#if defined(HAVE_QT)
    //  add entries to the combine mode dialog
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_add",   new lay::ConfigureAction (tl::to_string (tr ("Add<:/cm_add.png>{Add shapes}")),   cfg_edit_combine_mode, CMConverter ().to_string (CM_Add)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_merge", new lay::ConfigureAction (tl::to_string (tr ("Merge<:/cm_merge.png>{Merge shapes with background}")), cfg_edit_combine_mode, CMConverter ().to_string (CM_Merge)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_erase", new lay::ConfigureAction (tl::to_string (tr ("Erase<:/cm_erase.png>{Erase shape from background}")), cfg_edit_combine_mode, CMConverter ().to_string (CM_Erase)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_mask",  new lay::ConfigureAction (tl::to_string (tr ("Mask<:/cm_mask.png>{Mask background with shape}")),  cfg_edit_combine_mode, CMConverter ().to_string (CM_Mask)));
    mp->menu ()->insert_item ("@toolbar.combine_mode.end", "combine_mode_diff",  new lay::ConfigureAction (tl::to_string (tr ("Diff<:/cm_diff.png>{Compute difference of shape with background}")),  cfg_edit_combine_mode, CMConverter ().to_string (CM_Diff)));

    update_menu (CM_Add);
#endif
  }

  void update_menu (combine_mode_type cm)
  {
#if defined(HAVE_QT)
    lay::Dispatcher *mp = lay::Dispatcher::instance ();
    if (! mp || ! mp->has_ui ()) {
      return;
    }

    lay::Action *combine_menu = mp->menu ()->action ("@toolbar.combine_mode");

    if (cm == CM_Add) {
      combine_menu->set_title (tl::to_string (tr ("Add")));
      combine_menu->set_icon (":/cm_add_24px.png");
    } else if (cm == CM_Merge) {
      combine_menu->set_title (tl::to_string (tr ("Merge")));
      combine_menu->set_icon (":/cm_merge_24px.png");
    } else if (cm == CM_Erase) {
      combine_menu->set_title (tl::to_string (tr ("Erase")));
      combine_menu->set_icon (":/cm_erase_24px.png");
    } else if (cm == CM_Mask) {
      combine_menu->set_title (tl::to_string (tr ("Mask")));
      combine_menu->set_icon (":/cm_mask_24px.png");
    } else if (cm == CM_Diff) {
      combine_menu->set_title (tl::to_string (tr ("Diff")));
      combine_menu->set_icon (":/cm_diff_24px.png");
    }
#endif
  }

  void initialized (lay::Dispatcher *root)
  {
    lay::Dispatcher *mp = lay::Dispatcher::instance ();
    if (! mp || ! mp->has_ui ()) {
      return;
    }

#if defined(HAVE_QT)

    //  generate a warning if the combine mode isn't "Add"

    combine_mode_type cm = CM_Add;
    root->config_get (cfg_edit_combine_mode, cm, CMConverter ());

    lay::Action *combine_menu = mp->menu ()->action ("@toolbar.combine_mode");

    if (cm != CM_Add && combine_menu->is_visible ()) {
      lay::TipDialog td (QApplication::activeWindow (), 
                    tl::to_string (tr ("The background combination mode of the shape editor is set to some other mode than 'Add'.\n"
                                       "This can be confusing, because a shape may not be drawn as expected.\n\nTo switch back to normal mode, choose 'Add' for the background combination mode in the toolbar.")),
                    "has-non-add-edit-combine-mode");
      if (td.exec_dialog ()) {
        //  Don't bother the user with more dialogs.
        return;
      }
    }

#endif
  }

private:
  lay::Dispatcher *mp_root;
  std::string m_title;
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl_main (new edt::MainPluginDeclaration (tl::to_string (tr ("Instances and shapes"))), 4000, "edt::MainService");

void
commit_recent (lay::LayoutViewBase *view)
{
#if defined(HAVE_QT)
  lay::EditorOptionsPages *eo_pages = view->editor_options_pages ();
  if (!eo_pages) {
    return;
  }

  for (std::vector<lay::EditorOptionsPage *>::const_iterator op = eo_pages->pages ().begin (); op != eo_pages->pages ().end (); ++op) {
    if ((*op)->active ()) {
      (*op)->commit_recent (view);
    }
  }
#endif
}

class PartialPluginDeclaration
  : public PluginDeclarationBase
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

  virtual void get_editor_options_pages (std::vector<lay::EditorOptionsPage *> & /*pages*/, lay::LayoutViewBase * /*view*/, lay::Dispatcher * /*root*/) const
  {
    //  .. no specific ones ..
  }

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::Dispatcher *root, lay::LayoutViewBase *view) const
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
  new edt::PartialPluginDeclaration (tl::to_string (tr ("Partial shapes")), "partial:edit_mode\t" + tl::to_string (tr ("Partial{Edit points and edges of shapes}")) + "<:partial_24px.png>"),
  4030, 
  "edt::PartialService"
);

}

