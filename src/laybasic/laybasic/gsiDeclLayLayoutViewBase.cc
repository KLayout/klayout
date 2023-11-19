
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


#include "gsiDecl.h"
#include "gsiSignals.h"
#include "gsiEnums.h"
#include "rdb.h"
#include "layLayoutViewBase.h"
#include "layDitherPattern.h"
#include "layLineStyles.h"
#include "dbSaveLayoutOptions.h"
#include "dbLayoutToNetlist.h"
#include "dbLayoutVsSchematic.h"
#include "tlStream.h"

namespace gsi
{

static void remove_stipple (lay::LayoutViewBase *view, unsigned int index)
{
  lay::DitherPattern pattern (view->dither_pattern ());

  if (index >= (unsigned int) std::distance (pattern.begin (), pattern.begin_custom ()) && 
      index < (unsigned int) std::distance (pattern.begin (), pattern.end ())) {
    lay::DitherPatternInfo p;
    pattern.replace_pattern (index, p); 
    pattern.renumber ();
    view->set_dither_pattern (pattern);
  }
}

static void clear_stipples (lay::LayoutViewBase *view)
{
  lay::DitherPattern no_stipples;
  view->set_dither_pattern (no_stipples);
}

static unsigned int add_stipple1 (lay::LayoutViewBase *view, const std::string &name, const std::vector<unsigned int> &data, unsigned int bits)
{
  lay::DitherPattern pattern (view->dither_pattern ());

  lay::DitherPatternInfo p;
  p.set_name (name);
  if (bits > 0 && ! data.empty ()) {
    p.set_pattern (&(*data.begin ()), std::min (32u, bits), std::min (32u, (unsigned int) data.size ()));
  }
  unsigned int index = pattern.add_pattern (p);

  view->set_dither_pattern (pattern);

  return index;
}

static unsigned int add_stipple2 (lay::LayoutViewBase *view, const std::string &name, const std::string &s)
{
  lay::DitherPattern pattern (view->dither_pattern ());

  lay::DitherPatternInfo p;
  p.from_string (s);
  p.set_name (name);
  unsigned int index = pattern.add_pattern (p);

  view->set_dither_pattern (pattern);

  return index;
}

static std::string get_stipple (lay::LayoutViewBase *view, unsigned int index)
{
  lay::DitherPattern pattern (view->dither_pattern ());
  return pattern.pattern (index).to_string ();
}

static void remove_line_style (lay::LayoutViewBase *view, unsigned int index)
{
  lay::LineStyles styles (view->line_styles ());

  if (index >= (unsigned int) std::distance (styles.begin (), styles.begin_custom ()) &&
      index < (unsigned int) std::distance (styles.begin (), styles.end ())) {
    lay::LineStyleInfo p;
    styles.replace_style (index, p);
    styles.renumber ();
    view->set_line_styles (styles);
  }
}

static void clear_line_styles (lay::LayoutViewBase *view)
{
  lay::LineStyles no_styles;
  view->set_line_styles (no_styles);
}

static unsigned int add_line_style1 (lay::LayoutViewBase *view, const std::string &name, unsigned int data, unsigned int bits)
{
  lay::LineStyles styles (view->line_styles ());

  lay::LineStyleInfo s;
  s.set_name (name);
  s.set_pattern (data, std::min (32u, bits));
  unsigned int index = styles.add_style (s);

  view->set_line_styles (styles);

  return index;
}

static unsigned int add_line_style2 (lay::LayoutViewBase *view, const std::string &name, const std::string &str)
{
  lay::LineStyles styles (view->line_styles ());

  lay::LineStyleInfo s;
  s.from_string (str);
  s.set_name (name);
  unsigned int index = styles.add_style (s);

  view->set_line_styles (styles);

  return index;
}

static std::string get_line_style (lay::LayoutViewBase *view, unsigned int index)
{
  return view->line_styles ().style (index).to_string ();
}

static void transaction (lay::LayoutViewBase *view, const std::string &desc)
{
  view->manager ()->transaction (desc);
}

static void commit (lay::LayoutViewBase *view)
{
  view->manager ()->commit ();
}

static void clear_transactions (lay::LayoutViewBase *view)
{
  view->manager ()->clear ();
}

static bool transacting (lay::LayoutViewBase *view)
{
  return view->manager ()->transacting ();
}

static db::DCplxTrans viewport_trans (const lay::LayoutViewBase *view)
{
  return view->viewport ().trans ();
}

static int viewport_width (const lay::LayoutViewBase *view)
{
  return view->viewport ().width ();
}

static int viewport_height (const lay::LayoutViewBase *view)
{
  return view->viewport ().height ();
}

static std::vector<lay::LayoutViewBase::cell_path_type> selected_cells_paths (const lay::LayoutViewBase *view, int cv_index)
{
  std::vector<lay::LayoutViewBase::cell_path_type> p;
  view->selected_cells_paths (cv_index, p);
  return p;
}

static unsigned int create_rdb (lay::LayoutViewBase *view, const std::string &name)
{
  rdb::Database *db = new rdb::Database ();
  db->set_name (name);
  return view->add_rdb (db);
}

static unsigned int create_l2ndb (lay::LayoutViewBase *view, const std::string &name)
{
  db::LayoutToNetlist *db = new db::LayoutToNetlist ();
  db->set_name (name);
  return view->add_l2ndb (db);
}

static unsigned int create_lvsdb (lay::LayoutViewBase *view, const std::string &name)
{
  db::LayoutVsSchematic *db = new db::LayoutVsSchematic ();
  db->set_name (name);
  return view->add_l2ndb (db);
}

static db::LayoutVsSchematic *get_lvsdb (lay::LayoutViewBase *view, unsigned int index)
{
  db::LayoutToNetlist *db = view->get_l2ndb (index);
  return dynamic_cast<db::LayoutVsSchematic *> (db);
}

static unsigned int add_lvsdb (lay::LayoutViewBase *view, db::LayoutVsSchematic *lvsdb)
{
  return view->add_l2ndb (lvsdb);
}

static unsigned int replace_lvsdb (lay::LayoutViewBase *view, unsigned int db_index, db::LayoutVsSchematic *lvsdb)
{
  return view->replace_l2ndb (db_index, lvsdb);
}


//  this binding returns a const pointer which is not converted into a copy by RBA
static lay::LayerPropertiesNodeRef insert_layer1 (lay::LayoutViewBase *view, const lay::LayerPropertiesConstIterator &iter, const lay::LayerProperties &props)
{
  const lay::LayerPropertiesNode *lp = dynamic_cast<const lay::LayerPropertiesNode *> (&props);
  if (lp) {
    return lay::LayerPropertiesNodeRef (lay::LayerPropertiesConstIterator (&view->insert_layer (iter, *lp)));
  } else {
    return lay::LayerPropertiesNodeRef (lay::LayerPropertiesConstIterator (&view->insert_layer (iter, props)));
  }
}

static void replace_layer_node2 (lay::LayoutViewBase *view, unsigned int index, const lay::LayerPropertiesConstIterator &iter, const lay::LayerProperties &props)
{
  const lay::LayerPropertiesNode *lp = dynamic_cast<const lay::LayerPropertiesNode *> (&props);
  if (lp) {
    view->replace_layer_node (index, iter, *lp);
  } else {
    view->replace_layer_node (index, iter, props);
  }
}

static void replace_layer_node1 (lay::LayoutViewBase *view, const lay::LayerPropertiesConstIterator &iter, const lay::LayerProperties &props)
{
  const lay::LayerPropertiesNode *lp = dynamic_cast<const lay::LayerPropertiesNode *> (&props);
  if (lp) {
    view->replace_layer_node (iter, *lp);
  } else {
    view->replace_layer_node (iter, props);
  }
}

static lay::LayerPropertiesNodeRef insert_layer2 (lay::LayoutViewBase *view, unsigned int index, const lay::LayerPropertiesConstIterator &iter, const lay::LayerProperties &props)
{
  const lay::LayerPropertiesNode *lp = dynamic_cast<const lay::LayerPropertiesNode *> (&props);
  if (lp) {
    return lay::LayerPropertiesNodeRef (lay::LayerPropertiesConstIterator (&view->insert_layer (index, iter, *lp)));
  } else {
    return lay::LayerPropertiesNodeRef (lay::LayerPropertiesConstIterator (&view->insert_layer (index, iter, props)));
  }
}

static void delete_layers1 (lay::LayoutViewBase *view, const std::vector<lay::LayerPropertiesConstIterator> &iters)
{
  std::vector<lay::LayerPropertiesConstIterator> sorted (iters);
  std::sort (sorted.begin (), sorted.end (), lay::CompareLayerIteratorBottomUp ());
  for (std::vector<lay::LayerPropertiesConstIterator>::iterator s = sorted.begin (); s != sorted.end (); ++s) {
    view->delete_layer (*s);
  }
}

static unsigned int show_layout1 (lay::LayoutViewBase *view, db::Layout *layout, bool add_cellview)
{
  //  the layout gets held by the LayoutHandle object
  layout->keep ();
  lay::LayoutHandle *handle = lay::LayoutHandle::find_layout (layout);
  if (! handle) {
    handle = new lay::LayoutHandle (layout, std::string ());
  }
  return view->add_layout (handle, add_cellview);
}

static unsigned int show_layout2 (lay::LayoutViewBase *view, db::Layout *layout, std::string &tech, bool add_cellview)
{
  //  the layout gets held by the LayoutHandle object
  layout->keep ();
  lay::LayoutHandle *handle = lay::LayoutHandle::find_layout (layout);
  if (! handle) {
    handle = new lay::LayoutHandle (layout, std::string ());
  }
  handle->set_tech_name (tech);
  return view->add_layout (handle, add_cellview);
}

static unsigned int show_layout3 (lay::LayoutViewBase *view, db::Layout *layout, std::string &tech, bool add_cellview, bool initialize_layers)
{
  //  the layout gets held by the LayoutHandle object
  layout->keep ();
  lay::LayoutHandle *handle = lay::LayoutHandle::find_layout (layout);
  if (! handle) {
    handle = new lay::LayoutHandle (layout, std::string ());
  }
  handle->set_tech_name (tech);
  return view->add_layout (handle, add_cellview, initialize_layers);
}

static void delete_layers2 (lay::LayoutViewBase *view, unsigned int index, const std::vector<lay::LayerPropertiesConstIterator> &iters)
{
  std::vector<lay::LayerPropertiesConstIterator> sorted (iters);
  std::sort (sorted.begin (), sorted.end (), lay::CompareLayerIteratorBottomUp ());
  for (std::vector<lay::LayerPropertiesConstIterator>::iterator s = sorted.begin (); s != sorted.end (); ++s) {
    view->delete_layer (index, *s);
  }
}

static void save_as1 (lay::LayoutViewBase *view, unsigned int index, const std::string &filename, const db::SaveLayoutOptions &options)
{
  view->save_as (index, filename, tl::OutputStream::OM_Auto, options, true, 0);
}

static void save_as2 (lay::LayoutViewBase *view, unsigned int index, const std::string &filename, bool /*gzip*/, const db::SaveLayoutOptions &options)
{
  //  because set_format_from_name always returns true now, we ignore the gzip option -
  //  it's probably used only in that context.
  view->save_as (index, filename, tl::OutputStream::OM_Auto, options, true, 0);
}

static tl::PixelBuffer get_pixels_with_options (lay::LayoutViewBase *view, unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, const db::DBox &target_box)
{
  return view->get_pixels_with_options (width, height, linewidth, oversampling, resolution, tl::Color (), tl::Color (), tl::Color (), target_box);
}

static tl::BitmapBuffer get_pixels_with_options_mono (lay::LayoutViewBase *view, unsigned int width, unsigned int height, int linewidth, const db::DBox &target_box)
{
  return view->get_pixels_with_options_mono (width, height, linewidth, tl::Color (), tl::Color (), tl::Color (), target_box);
}

static void save_image_with_options (lay::LayoutViewBase *view, const std::string &fn, unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, const db::DBox &target_box, bool monochrome)
{
  view->save_image_with_options (fn, width, height, linewidth, oversampling, resolution, tl::Color (), tl::Color (), tl::Color (), target_box, monochrome);
}

#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
static QImage get_image_with_options (lay::LayoutViewBase *view, unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, const db::DBox &target_box, bool monochrome)
{
  return view->get_image_with_options (width, height, linewidth, oversampling, resolution, tl::Color (), tl::Color (), tl::Color (), target_box, monochrome);
}

static QWidget *widget (lay::LayoutViewBase *view)
{
  return view->widget ();
}

#endif

static std::vector<std::string> 
get_config_names (lay::LayoutViewBase *view)
{
  std::vector<std::string> names;
  view->get_config_names (names);
  return names;
}

static void
send_key_press_event (lay::LayoutViewBase *view, unsigned int key, unsigned int buttons)
{
  view->canvas ()->send_key_press_event (key, buttons);
}

static void
send_mouse_move_event (lay::LayoutViewBase *view, const db::DPoint &pt, unsigned int buttons)
{
  view->canvas ()->send_mouse_move_event (pt, buttons);
}

static void
send_leave_event (lay::LayoutViewBase *view)
{
  view->canvas ()->send_leave_event ();
}

static void
send_enter_event (lay::LayoutViewBase *view)
{
  view->canvas ()->send_enter_event ();
}

static void
send_mouse_press_event (lay::LayoutViewBase *view, const db::DPoint &pt, unsigned int buttons)
{
  view->canvas ()->send_mouse_press_event (pt, buttons);
}

static void
send_mouse_double_clicked_event (lay::LayoutViewBase *view, const db::DPoint &pt, unsigned int buttons)
{
  view->canvas ()->send_mouse_double_clicked_event (pt, buttons);
}

static void
send_mouse_release_event (lay::LayoutViewBase *view, const db::DPoint &pt, unsigned int buttons)
{
  view->canvas ()->send_mouse_release_event (pt, buttons);
}

static void
send_wheel_event (lay::LayoutViewBase *view, int delta, bool horizontal, const db::DPoint &pt, unsigned int buttons)
{
  view->canvas ()->send_wheel_event (delta, horizontal, pt, buttons);
}

namespace {

  /**
   *  @brief A wrapper class around LayerPropertiesConstIterator that adapts this iterator to GSI requirements
   *
   *  These requirements are basically a proper declaration of the value type.
   *  TODO: once the LayerPropertiesConstIterator is modified to return
   */
  class LayerPropertiesConstIteratorWrapper
  {
  public:
    typedef lay::LayerPropertiesNodeRef value_type;
    typedef lay::LayerPropertiesNodeRef reference;
    //  Dummy declarations required for std::iterator_traits
    typedef std::forward_iterator_tag iterator_category;
    typedef void difference_type;
    typedef void pointer;

    LayerPropertiesConstIteratorWrapper (const lay::LayerPropertiesConstIterator &iter)
      : m_iter (iter)
    {
      //  .. nothing yet ..
    }

    reference operator* () const
    {
      return lay::LayerPropertiesNodeRef (m_iter);
    }

    bool at_end () const
    {
      return m_iter.at_end ();
    }

    LayerPropertiesConstIteratorWrapper &operator++()
    {
      ++m_iter;
      return *this;
    }

  private:
    lay::LayerPropertiesConstIterator m_iter;
  };

}

static LayerPropertiesConstIteratorWrapper each_layer (lay::LayoutViewBase *view)
{
  return LayerPropertiesConstIteratorWrapper (view->begin_layers ());
}

static LayerPropertiesConstIteratorWrapper each_layer2 (lay::LayoutViewBase *view, unsigned int list_index)
{
  return LayerPropertiesConstIteratorWrapper (view->begin_layers (list_index));
}

static lay::AbstractMenu *menu (lay::LayoutViewBase *view)
{
  return view->menu ();
}

LAYBASIC_PUBLIC Class<lay::LayoutViewBase> decl_LayoutViewBase ("lay", "LayoutViewBase",
  gsi::constant ("LV_NoLayers", (unsigned int) lay::LayoutViewBase::LV_NoLayers,
    "@brief With this option, no layers view will be provided (see \\layer_control_frame)\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoHierarchyPanel", (unsigned int) lay::LayoutViewBase::LV_NoHierarchyPanel,
    "@brief With this option, no cell hierarchy view will be provided (see \\hierarchy_control_frame)\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoLibrariesView", (unsigned int) lay::LayoutViewBase::LV_NoLibrariesView,
    "@brief With this option, no library view will be provided (see \\libraries_frame)\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoEditorOptionsPanel", (unsigned int) lay::LayoutViewBase::LV_NoEditorOptionsPanel,
    "@brief With this option, no editor options panel will be provided (see \\editor_options_frame)\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoBookmarksView", (unsigned int) lay::LayoutViewBase::LV_NoBookmarksView,
    "@brief With this option, no bookmarks view will be provided (see \\bookmarks_frame)\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_Naked", (unsigned int) lay::LayoutViewBase::LV_Naked,
    "@brief With this option, no separate views will be provided\n"
    "Use this value with the constructor's 'options' argument.\n"
    "This option is basically equivalent to using \\LV_NoLayers+\\LV_NoHierarchyPanel+\\LV_NoLibrariesView+\\LV_NoBookmarksView\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoZoom", (unsigned int) lay::LayoutViewBase::LV_NoZoom,
    "@brief With this option, zooming is disabled\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoGrid", (unsigned int) lay::LayoutViewBase::LV_NoGrid,
    "@brief With this option, the grid background is not shown\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoMove", (unsigned int) lay::LayoutViewBase::LV_NoMove,
    "@brief With this option, move operations are not supported\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoTracker", (unsigned int) lay::LayoutViewBase::LV_NoTracker,
    "@brief With this option, mouse position tracking is not supported\n"
    "Use this value with the constructor's 'options' argument.\n"
    "This option is not useful currently as no mouse tracking support is provided.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoSelection", (unsigned int) lay::LayoutViewBase::LV_NoSelection,
    "@brief With this option, objects cannot be selected\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoPlugins", (unsigned int) lay::LayoutViewBase::LV_NoPlugins,
    "@brief With this option, all plugins are disabled\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::constant ("LV_NoPropertiesPopup", (unsigned int) lay::LayoutViewBase::LV_NoPropertiesPopup,
    "@brief This option disables the properties popup on double click\n"
    "Use this value with the constructor's 'options' argument.\n"
    "\n"
    "This constant has been introduced in version 0.28.\n"
  ) +
  gsi::constant ("LV_NoServices", (unsigned int) lay::LayoutViewBase::LV_NoServices,
    "@brief This option disables all services except the ones for pure viewing\n"
    "Use this value with the constructor's 'options' argument.\n"
    "With this option, all manipulation features are disabled, except zooming.\n"
    "It is equivalent to \\LV_NoMove + \\LV_NoTracker + \\LV_NoSelection + \\LV_NoPlugins.\n"
    "\n"
    "This constant has been introduced in version 0.27.\n"
  ) +
  gsi::method ("stop_redraw", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::stop_redraw),
    "@brief Stops the redraw thread\n"
    "\n"
    "It is very important to stop the redraw thread before applying changes to the "
    "layout or the cell views and the LayoutView configuration. This is usually done automatically. "
    "For rare cases, where this is not the case, this method is provided.\n"
  ) +
  gsi::method ("title=|#set_title", static_cast<void (lay::LayoutViewBase::*) (const std::string &)> (&lay::LayoutViewBase::set_title), gsi::arg ("title"),
    "@brief Sets the title of the view\n"
    "\n"
    "@param title The title string to use\n"
    "\n"
    "Override the standard title of the view indicating the file names loaded by "
    "the specified title string. The title string can be reset with \\reset_title to "
    "the standard title again."
  ) +
  gsi::method ("reset_title", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::reset_title),
    "@brief Resets the title to the standard title\n"
    "\n"
    "See \\set_title and \\title for a description about how titles are handled."
  ) +
  gsi::method ("title", static_cast<std::string (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::title),
    "@brief Returns the view's title string\n"
    "\n"
    "@return The title string\n"
    "\n"
    "The title string is either a string composed of the file names loaded (in some "
    "\"readable\" manner) or a customized title string set by \\set_title."
  ) +
  gsi::method ("save_layer_props", static_cast<void (lay::LayoutViewBase::*) (const std::string &)> (&lay::LayoutViewBase::save_layer_props), gsi::arg ("fn"),
    "@brief Saves the layer properties\n"
    "\n"
    "Save the layer properties to the file given in \"fn\""
  ) +
  gsi::method ("load_layer_props", static_cast <void (lay::LayoutViewBase::*)(const std::string &)> (&lay::LayoutViewBase::load_layer_props), gsi::arg ("fn"),
    "@brief Loads the layer properties\n"
    "\n"
    "@param fn The file name of the .lyp file to load\n"
    "\n"
    "Load the layer properties from the file given in \"fn\""
  ) +
  gsi::method ("load_layer_props", static_cast <void (lay::LayoutViewBase::*)(const std::string &, bool)> (&lay::LayoutViewBase::load_layer_props), gsi::arg ("fn"), gsi::arg ("add_default"),
    "@brief Loads the layer properties with options\n"
    "\n"
    "@param fn The file name of the .lyp file to load\n"
    "@param add_default If true, default layers will be added for each other layer in the layout\n"
    "\n"
    "Load the layer properties from the file given in \"fn\".\n"
    "This version allows one to specify whether defaults should be used for all other layers by "
    "setting \"add_default\" to true.\n"
    "\n"
    "This variant has been added on version 0.21."
  ) +
  gsi::method ("load_layer_props", static_cast <void (lay::LayoutViewBase::*)(const std::string &, int, bool)> (&lay::LayoutViewBase::load_layer_props), gsi::arg ("fn"), gsi::arg ("cv_index"), gsi::arg ("add_default"),
    "@brief Loads the layer properties with options\n"
    "\n"
    "@param fn The file name of the .lyp file to load\n"
    "@param cv_index See description text\n"
    "@param add_default If true, default layers will be added for each other layer in the layout\n"
    "\n"
    "Load the layer properties from the file given in \"fn\".\n"
    "This version allows one to specify whether defaults should be used for all other layers by "
    "setting \"add_default\" to true. It can be used to load the layer properties for a specific "
    "cellview by setting \"cv_index\" to the index for which the layer properties file should be applied. "
    "All present definitions for this layout will be removed before the properties file is loaded. \"cv_index\" can "
    "be set to -1. In that case, the layer properties file is applied to each of the layouts individually.\n"
    "\n"
    "Note that this version will override all cellview index definitions in the layer properties file.\n"
    "\n"
    "This variant has been added on version 0.21."
  ) +
  gsi::method ("min_hier_levels=", static_cast<void (lay::LayoutViewBase::*) (int)> (&lay::LayoutViewBase::set_min_hier_levels), gsi::arg ("level"),
    "@brief Sets the minimum hierarchy level at which to display geometries\n"
    "\n"
    "@param level The minimum level above which to display something\n"
    "\n"
    "This methods allows setting the minimum hierarchy level above which to display geometries."
    "This method may cause a redraw if required."
  ) +
  gsi::method ("min_hier_levels", static_cast<int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::get_min_hier_levels),
    "@brief Returns the minimum hierarchy level at which to display geometries\n"
    "\n"
    "@return The minimum level at which to display geometries"
  ) +
  gsi::method ("max_hier_levels=", static_cast<void (lay::LayoutViewBase::*) (int)> (&lay::LayoutViewBase::set_max_hier_levels), gsi::arg ("level"),
    "@brief Sets the maximum hierarchy level up to which to display geometries\n"
    "\n"
    "@param level The maximum level below which to display something\n"
    "\n"
    "This methods allows setting the maximum hierarchy below which to display geometries."
    "This method may cause a redraw if required."
  ) +
  gsi::method ("max_hier_levels", static_cast<int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::get_max_hier_levels),
    "@brief Returns the maximum hierarchy level up to which to display geometries\n"
    "\n"
    "@return The maximum level up to which to display geometries"
  ) +
  gsi::method ("enable_edits", static_cast<void (lay::LayoutViewBase::*) (bool)> (&lay::LayoutViewBase::enable_edits), gsi::arg ("enable"),
    "@brief Enables or disables edits\n"
    "\n"
    "@param enable Enable edits if set to true\n"
    "\n"
    "This method allows putting the view into read-only mode by disabling all edit "
    "functions. For doing so, this method has to be called with a 'false' argument. Calling it "
    "with a 'true' parameter enables all edits again. This method must not be confused with the "
    "edit/viewer mode. The LayoutView's enable_edits method is intended to temporarily disable "
    "all menu entries and functions which could allow the user to alter the database."
    "\n"
    "In 0.25, this method has been moved from MainWindow to LayoutView.\n"
  ) +
  gsi::method ("is_editable?", static_cast<bool (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::is_editable),
    "@brief Returns true if the view is in editable mode\n"
    "\n"
    "This read-only attribute has been added in version 0.27.5.\n"
  ) +
  gsi::method ("reload_layout", static_cast<void (lay::LayoutViewBase::*) (unsigned int)> (&lay::LayoutViewBase::reload_layout), gsi::arg ("cv"),
    "@brief Reloads the given cellview\n"
    "\n"
    "@param cv The index of the cellview to reload"
  ) + 
  gsi::method ("create_layout", static_cast<unsigned int (lay::LayoutViewBase::*) (bool)> (&lay::LayoutViewBase::create_layout), gsi::arg ("add_cellview"),
    "@brief Creates a new, empty layout\n"
    "\n"
    "The add_cellview parameter controls whether to create a new cellview (true)\n"
    "or clear all cellviews before (false).\n"
    "\n"
    "This version will associate the new layout with the default technology.\n"
    "\n"
    "@return The index of the cellview created.\n"
  ) +
  gsi::method ("create_layout", static_cast<unsigned int (lay::LayoutViewBase::*) (const std::string &, bool)> (&lay::LayoutViewBase::create_layout), gsi::arg ("tech"), gsi::arg ("add_cellview"),
    "@brief Create a new, empty layout and associate it with the given technology\n"
    "\n"
    "The add_cellview parameter controls whether to create a new cellview (true)\n"
    "or clear all cellviews before (false).\n"
    "\n"
    "@return The index of the cellview created.\n"
    "\n"
    "This variant has been introduced in version 0.22.\n"
  ) +
  gsi::method ("create_layout", static_cast<unsigned int (lay::LayoutViewBase::*) (const std::string &, bool, bool)> (&lay::LayoutViewBase::create_layout), gsi::arg ("tech"), gsi::arg ("add_cellview"), gsi::arg ("init_layers"),
    "@brief Create a new, empty layout and associate it with the given technology\n"
    "\n"
    "The add_cellview parameter controls whether to create a new cellview (true)\n"
    "or clear all cellviews before (false). This variant also allows one to control whether the layer properties are\n"
    "initialized (init_layers = true) or not (init_layers = false).\n"
    "\n"
    "@return The index of the cellview created.\n"
    "\n"
    "This variant has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("show_layout", &show_layout1, gsi::arg ("layout"), gsi::arg ("add_cellview"),
    "@brief Shows an existing layout in the view\n"
    "\n"
    "Shows the given layout in the view. If add_cellview is true, the new layout is added to the list of "
    "cellviews in the view.\n"
    "\n"
    "Note: once a layout is passed to the view with show_layout, it is owned by the view and must not be "
    "destroyed with the 'destroy' method.\n"
    "\n"
    "@return The index of the cellview created.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("show_layout", &show_layout2, gsi::arg ("layout"), gsi::arg ("tech"), gsi::arg ("add_cellview"),
    "@brief Shows an existing layout in the view\n"
    "\n"
    "Shows the given layout in the view. If add_cellview is true, the new layout is added to the list of "
    "cellviews in the view.\n"
    "The technology to use for that layout can be specified as well with the 'tech' parameter. Depending "
    "on the definition of the technology, layer properties may be loaded for example.\n"
    "The technology string can be empty for the default technology.\n"
    "\n"
    "Note: once a layout is passed to the view with show_layout, it is owned by the view and must not be "
    "destroyed with the 'destroy' method.\n"
    "\n"
    "@return The index of the cellview created.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("show_layout", &show_layout3, gsi::arg ("layout"), gsi::arg ("tech"), gsi::arg ("add_cellview"), gsi::arg ("init_layers"),
    "@brief Shows an existing layout in the view\n"
    "\n"
    "Shows the given layout in the view. If add_cellview is true, the new layout is added to the list of "
    "cellviews in the view.\n"
    "The technology to use for that layout can be specified as well with the 'tech' parameter. Depending "
    "on the definition of the technology, layer properties may be loaded for example.\n"
    "The technology string can be empty for the default technology.\n"
    "This variant also allows one to control whether the layer properties are\n"
    "initialized (init_layers = true) or not (init_layers = false).\n"
    "\n"
    "Note: once a layout is passed to the view with show_layout, it is owned by the view and must not be "
    "destroyed with the 'destroy' method.\n"
    "\n"
    "@return The index of the cellview created.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method ("erase_cellview", static_cast<void (lay::LayoutViewBase::*) (unsigned int)> (&lay::LayoutViewBase::erase_cellview), gsi::arg ("index"),
    "@brief Erases the cellview with the given index\n"
    "\n"
    "This closes the given cellview and unloads the layout associated with it, unless referred to by another cellview."
  ) +
  gsi::method ("rename_cellview", static_cast<void (lay::LayoutViewBase::*) (const std::string &, int)> (&lay::LayoutViewBase::rename_cellview), gsi::arg ("name"), gsi::arg ("index"),
    "@brief Renames the cellview with the given index\n"
    "\n"
    "If the name is not unique, a unique name will be constructed from the name given.\n"
    "The name may be different from the filename but is associated with the layout object.\n"
    "If a layout is shared between multiple cellviews (which may happen due to a clone of the layout view\n"
    "for example), all cellviews are renamed.\n"
  ) +
  gsi::method ("load_layout", static_cast<unsigned int (lay::LayoutViewBase::*) (const std::string &, const db::LoadLayoutOptions &, const std::string &, bool)> (&lay::LayoutViewBase::load_layout), gsi::arg ("filename"), gsi::arg ("options"), gsi::arg ("technology"), gsi::arg ("add_cellview", true),
    "@brief Loads a (new) file into the layout view with the given technology\n"
    "\n"
    "Loads the file given by the \"filename\" parameter and associates it with the given technology.\n"
    "The options specify various options for reading the file.\n"
    "The add_cellview param controls whether to create a new cellview (true)\n"
    "or clear all cellviews before (false).\n"
    "\n"
    "@return The index of the cellview loaded.\n"
    "\n"
    "This version has been introduced in version 0.22. The 'add_cellview' argument has been made optional in version 0.28.\n"
  ) +
  gsi::method ("load_layout", static_cast<unsigned int (lay::LayoutViewBase::*) (const std::string &, const db::LoadLayoutOptions &, bool)> (&lay::LayoutViewBase::load_layout), gsi::arg ("filename"), gsi::arg ("options"), gsi::arg ("add_cellview", true),
    "@brief Loads a (new) file into the layout view\n"
    "\n"
    "Loads the file given by the \"filename\" parameter.\n"
    "The options specify various options for reading the file.\n"
    "The add_cellview param controls whether to create a new cellview (true)\n"
    "or clear all cellviews before (false).\n"
    "\n"
    "@return The index of the cellview loaded.\n"
    "\n"
    "This method has been introduced in version 0.18. The 'add_cellview' argument has been made optional in version 0.28.\n"
  ) +
  gsi::method ("load_layout", static_cast<unsigned int (lay::LayoutViewBase::*) (const std::string &, const std::string &, bool)> (&lay::LayoutViewBase::load_layout), gsi::arg ("filename"), gsi::arg ("technology"), gsi::arg ("add_cellview", true),
    "@brief Loads a (new) file into the layout view with the given technology\n"
    "\n"
    "Loads the file given by the \"filename\" parameter and associates it with the given technology.\n"
    "The add_cellview param controls whether to create a new cellview (true)\n"
    "or clear all cellviews before (false).\n"
    "\n"
    "@return The index of the cellview loaded.\n"
    "\n"
    "This version has been introduced in version 0.22. The 'add_cellview' argument has been made optional in version 0.28.\n"
  ) +
  gsi::method ("load_layout", static_cast<unsigned int (lay::LayoutViewBase::*) (const std::string &filename, bool add_cellview)> (&lay::LayoutViewBase::load_layout), gsi::arg ("filename"), gsi::arg ("add_cellview", true),
    "@brief Loads a (new) file into the layout view\n"
    "\n"
    "Loads the file given by the \"filename\" parameter.\n"
    "The add_cellview param controls whether to create a new cellview (true)\n"
    "or clear all cellviews before (false).\n"
    "\n"
    "@return The index of the cellview loaded. The 'add_cellview' argument has been made optional in version 0.28.\n"
  ) +
  gsi::method ("active_cellview", static_cast<lay::CellViewRef (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::active_cellview_ref),
    "@brief Gets the active cellview (shown in hierarchy browser)\n"
    "\n"
    "This is a convenience method which is equivalent to cellview(active_cellview_index()).\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
    "Starting from version 0.25, the returned object can be manipulated which will have an immediate effect "
    "on the display."
  ) +
  gsi::method ("active_cellview_index", static_cast<int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::active_cellview_index),
    "@brief Gets the index of the active cellview (shown in hierarchy browser)\n"
  ) +
  gsi::method ("active_setview_index=|#set_active_cellview_index", &lay::LayoutViewBase::set_active_cellview_index, gsi::arg ("index"),
    "@brief Makes the cellview with the given index the active one (shown in hierarchy browser)\n"
    "See \\active_cellview_index.\n"
    "\n"
    "This method has been renamed from set_active_cellview_index to active_cellview_index= in version 0.25. "
    "The original name is still available, but is deprecated."
  ) +
  gsi::method_ext ("selected_cells_paths", &selected_cells_paths, gsi::arg ("cv_index"),
    "@brief Gets the paths of the selected cells\n"
    "\n"
    "Gets a list of cell paths to the cells selected in the cellview given by \\cv_index. "
    "The \"selected cells\" are the ones selected in the cell list or cell tree. This is not the \"current cell\" "
    "which is the one that is shown in the layout window.\n"
    "\n"
    "The cell paths are arrays of cell indexes where the last element is the actual cell selected.\n"
    "\n"
    "This method has be introduced in version 0.25.\n"
  ) +
  gsi::method ("#get_current_cell_path", static_cast<lay::LayoutViewBase::cell_path_type (lay::LayoutViewBase::*) (int) const> (&lay::LayoutViewBase::get_current_cell_path), gsi::arg ("cv_index"),
    "@brief Gets the cell path of the current cell\n"
    "\n"
    "The current cell is the one highlighted in the browser with the focus rectangle. The \n"
    "current path is returned for the cellview given by cv_index.\n"
    "The cell path is a list of cell indices from the top cell to the current cell.\n"
    "\n"
    "@param cv_index The cellview index for which to get the current path from (usually this will be the active cellview index)"
    "\n"
    "This method is was deprecated in version 0.25 since from then, the \\CellView object can be used to obtain an manipulate the selected cell."
  ) +
  gsi::method ("#set_current_cell_path", static_cast<void (lay::LayoutViewBase::*) (int, const lay::LayoutViewBase::cell_path_type &)> (&lay::LayoutViewBase::set_current_cell_path), gsi::arg ("cv_index"), gsi::arg ("cell_path"),
    "@brief Sets the path to the current cell\n"
    "\n"
    "The current cell is the one highlighted in the browser with the focus rectangle. The\n"
    "cell given by the path is highlighted and scrolled into view.\n"
    "To select the cell to be drawn, use the \\select_cell or \\select_cell_path method.\n"
    "\n"
    "@param cv_index The cellview index for which to set the current path for (usually this will be the active cellview index)\n"
    "@param path The path to the current cell\n"
    "\n"
    "This method is was deprecated in version 0.25 since from then, the \\CellView object can be used to obtain an manipulate the selected cell."
  ) +
  gsi::method ("cellviews", static_cast<unsigned int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::cellviews),
    "@brief Gets the number of cellviews\n"
  ) + 
  gsi::method ("cellview", static_cast<lay::CellViewRef (lay::LayoutViewBase::*) (unsigned int)> (&lay::LayoutViewBase::cellview_ref), gsi::arg ("cv_index"),
    "@brief Gets the cellview object for a given index\n"
    "\n"
    "@param cv_index The cellview index for which to get the object for\n"
    "\n"
    "Starting with version 0.25, this method returns a \\CellView object that can be manipulated to directly reflect "
    "any changes in the display."
  ) + 
  gsi::method ("zoom_fit", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::zoom_fit),
    "@brief Fits the contents of the current view into the window"
  ) +
  gsi::method ("zoom_fit_sel", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::zoom_fit_sel),
    "@brief Fits the contents of the current selection into the window\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("zoom_box", static_cast<void (lay::LayoutViewBase::*) (const db::DBox &)> (&lay::LayoutViewBase::zoom_box), gsi::arg ("box"),
    "@brief Sets the viewport to the given box\n"
    "\n"
    "@param box The box to which to set the view in micron coordinates\n"
  ) +
  gsi::method ("zoom_in", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::zoom_in),
    "@brief Zooms in somewhat"
  ) +
  gsi::method ("zoom_out", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::zoom_out),
    "@brief Zooms out somewhat"
  ) +
  gsi::method ("pan_up", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::pan_up),
    "@brief Pans upward"
  ) +
  gsi::method ("pan_down", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::pan_down),
    "@brief Pans down"
  ) +
  gsi::method ("pan_left", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::pan_left),
    "@brief Pans to the left"
  ) +
  gsi::method ("pan_right", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::pan_right),
    "@brief Pans to the right"
  ) +
  gsi::method ("pan_center", static_cast<void (lay::LayoutViewBase::*) (const db::DPoint &)> (&lay::LayoutViewBase::pan_center), gsi::arg ("p"),
    "@brief Pans to the given point\n"
    "\n"
    "The window is positioned such that \"p\" becomes the new center"
  ) +
  gsi::method ("box", static_cast<db::DBox (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::box),
    "@brief Returns the displayed box in micron space"
  ) +
  gsi::method_ext ("viewport_trans", &viewport_trans,
    "@brief Returns the transformation that converts micron coordinates to pixels\n"
    "Hint: the transformation returned will convert any point in micron coordinate space into "
    "a pixel coordinate. Contrary to usual convention, the y pixel coordinate is given in a mathematically "
    "oriented space - which means the bottom coordinate is 0.\n"
    "This method was introduced in version 0.18.\n"
  ) +
  gsi::method_ext ("viewport_width", &viewport_width,
    "@brief Returns the viewport width in pixels\n"
    "This method was introduced in version 0.18.\n"
  ) +
  gsi::method_ext ("viewport_height", &viewport_height,
    "@brief Return the viewport height in pixels\n"
    "This method was introduced in version 0.18.\n"
  ) +
  gsi::method ("add_missing_layers", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::add_missing_layers),
    "@brief Adds new layers to layer list\n"
    "This method was introduced in version 0.19.\n"
  ) +
  gsi::method ("remove_unused_layers", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::remove_unused_layers),
    "@brief Removes unused layers from layer list\n"
    "This method was introduced in version 0.19.\n"
  ) +
  gsi::method ("init_layer_properties", (void (lay::LayoutViewBase::*) (lay::LayerProperties &) const) &lay::LayoutViewBase::init_layer_properties, gsi::arg ("props"),
    "@brief Fills the layer properties for a new layer\n"
    "\n"
    "This method initializes a layer properties object's color and stipples according to "
    "the defaults for the given layer source specification. The layer's source must be set already "
    "on the layer properties object.\n"
    "\n"
    "This method was introduced in version 0.19.\n"
    "\n"
    "@param props The layer properties object to initialize."
  ) +
  gsi::method ("switch_mode", static_cast<void (lay::LayoutViewBase::*) (const std::string &)> (&lay::LayoutViewBase::switch_mode),
    "@brief Switches the mode.\n"
    "\n"
    "See \\mode_name about a method to get the name of the current mode and \\mode_names for a method "
    "to retrieve all available mode names.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method ("mode_name", &lay::LayoutViewBase::mode_name,
    "@brief Gets the name of the current mode.\n"
    "\n"
    "See \\switch_mode about a method to change the mode and \\mode_names for a method "
    "to retrieve all available mode names.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method ("mode_names", &lay::LayoutViewBase::mode_names,
    "@brief Gets the names of the available modes.\n"
    "\n"
    "This method allows asking the view for the available mode names for \\switch_mode and "
    "for the value returned by \\mode.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method_ext ("menu", &menu,
    "@brief Gets the \\AbstractMenu associated with this view.\n"
    "\n"
    "In normal UI application mode this is the main window's view. For a detached view or in non-UI "
    "applications this is the view's private menu.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method ("call_menu", &lay::LayoutViewBase::menu_activated, gsi::arg ("symbol"),
    "@brief Calls the menu item with the provided symbol.\n"
    "To obtain all symbols, use \\menu_symbols.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method ("menu_symbols", &lay::LayoutViewBase::menu_symbols,
    "@brief Gets all available menu symbols (see \\call_menu).\n"
    "NOTE: currently this method delivers a superset of all available symbols. Depending on the context, no all symbols may trigger actual functionality.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method ("cancel", &lay::LayoutViewBase::cancel,
    "@brief Cancels all edit operations\n"
    "\n"
    "This method will stop all pending edit operations (i.e. drag and drop) and cancel the current "
    "selection. Calling this method is useful to ensure there are no potential interactions with the script's "
    "functionality.\n"
  ) +
  gsi::method ("clear_selection", (void (lay::LayoutViewBase::*) ()) &lay::LayoutViewBase::clear_selection,
    "@brief Clears the selection of all objects (shapes, annotations, images ...)\n"
    "\n"
    "This method has been introduced in version 0.26.2\n"
  ) +
  gsi::method ("select_all", (void (lay::LayoutViewBase::*) ()) &lay::LayoutViewBase::select,
    "@brief Selects all objects from the view\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("select_from", (void (lay::LayoutViewBase::*) (const db::DPoint &, lay::Editable::SelectionMode)) &lay::LayoutViewBase::select, gsi::arg ("point"), gsi::arg ("mode", lay::Editable::SelectionMode::Replace, "Replace"),
    "@brief Selects the objects from a given point\n"
    "\n"
    "The mode indicates whether to add to the selection, replace the selection, remove from selection or invert the selected status of the objects "
    "found around the given point.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("select_from", (void (lay::LayoutViewBase::*) (const db::DBox &, lay::Editable::SelectionMode)) &lay::LayoutViewBase::select, gsi::arg ("box"), gsi::arg ("mode", lay::Editable::SelectionMode::Replace, "Replace"),
    "@brief Selects the objects from a given box\n"
    "\n"
    "The mode indicates whether to add to the selection, replace the selection, remove from selection or invert the selected status of the objects "
    "found inside the given box.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("clear_transient_selection", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::clear_transient_selection),
    "@brief Clears the transient selection (mouse-over hightlights) of all objects (shapes, annotations, images ...)\n"
    "\n"
    "This method has been introduced in version 0.26.2\n"
  ) +
  gsi::method ("transient_to_selection", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::transient_to_selection),
    "@brief Turns the transient selection into the actual selection\n"
    "\n"
    "The current selection is cleared before. All highlighted objects under the mouse will become selected. "
    "This applies to all types of objects (rulers, shapes, images ...).\n"
    "\n"
    "This method has been introduced in version 0.26.2\n"
  ) +
  gsi::method ("selection_bbox", static_cast<db::DBox (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::selection_bbox),
    "@brief Returns the bounding box of the current selection\n"
    "\n"
    "This method has been introduced in version 0.26.2\n"
  ) +
  gsi::method ("selection_size", (size_t (lay::LayoutViewBase::*) ()) &lay::LayoutViewBase::selection_size,
    "@brief Returns the number of selected objects\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("has_selection?", (bool (lay::LayoutViewBase::*) ()) &lay::LayoutViewBase::has_selection,
    "@brief Indicates whether any objects are selected\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("stop", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::stop),
    "@brief Stops redraw thread and close any browsers\n"
    "This method usually does not need to be called explicitly. The redraw thread is stopped automatically."
  ) +
  gsi::method ("#select_cell_path", (void (lay::LayoutViewBase::*) (const lay::LayoutViewBase::cell_path_type &, int)) &lay::LayoutViewBase::select_cell, gsi::arg ("cell_index"), gsi::arg ("cv_index"),
    "@brief Selects a cell by cell index for a certain cell view\n"
    "\n"
    "Select the current (top) cell by specifying a cell indexand the cellview index for which this cell should become the currently shown one. The path to the cell is constructed by "
    "selecting one that leads to a top cell.\n"
    "This method selects the cell to be drawn. In constrast, the \\set_current_cell_path method selects "
    "the cell that is highlighted in the cell tree (but not necessarily drawn)."
    "\n"
    "This method is was deprecated in version 0.25 since from then, the \\CellView object can be used to obtain an manipulate the selected cell."
  ) +
  gsi::method ("#select_cell", (void (lay::LayoutViewBase::*) (lay::LayoutViewBase::cell_index_type, int)) &lay::LayoutViewBase::select_cell, gsi::arg ("cell_index"), gsi::arg ("cv_index"),
    "@brief Selects a cell by index for a certain cell view\n"
    "\n"
    "Select the current (top) cell by specifying a path (a list of cell indices from top to "
    "the actual cell) and the cellview index for which this cell should become the currently "
    "shown one.\n"
    "This method selects the cell to be drawn. In constrast, the \\set_current_cell_path method selects "
    "the cell that is highlighted in the cell tree (but not necessarily drawn)."
    "\n"
    "This method is was deprecated in version 0.25 since from then, the \\CellView object can be used to obtain an manipulate the selected cell."
  ) +
  gsi::method ("descend", static_cast<void (lay::LayoutViewBase::*) (const std::vector<db::InstElement> &, int)> (&lay::LayoutViewBase::descend), gsi::arg ("path"), gsi::arg ("index"),
    "@brief Descends further into the hierarchy.\n"
    "\n"
    "Adds the given path (given as an array of InstElement objects) to the specific path of the "
    "cellview with the given index. In effect, the cell addressed by the terminal of the new path "
    "components can be shown in the context of the upper cells, if the minimum hierarchy level is "
    "set to a negative value.\n"
    "The path is assumed to originate from the current cell and contain specific instances sorted from "
    "top to bottom."
  ) +
  gsi::method ("ascend", static_cast<db::InstElement (lay::LayoutViewBase::*) (int)> (&lay::LayoutViewBase::ascend), gsi::arg ("index"),
    "@brief Ascends upwards in the hierarchy.\n"
    "\n"
    "Removes one element from the specific path of the cellview with the given index. Returns the element "
    "removed."
  ) +
  gsi::method ("is_cell_hidden?", static_cast<bool (lay::LayoutViewBase::*) (db::cell_index_type, int) const> (&lay::LayoutViewBase::is_cell_hidden), gsi::arg ("cell_index"), gsi::arg ("cv_index"),
    "@brief Returns true, if the cell is hidden\n"
    "\n"
    "@return True, if the cell with \"cell_index\" is hidden for the cellview \"cv_index\""
  ) +
  gsi::method ("hide_cell", static_cast<void (lay::LayoutViewBase::*) (db::cell_index_type, int)> (&lay::LayoutViewBase::hide_cell), gsi::arg ("cell_index"), gsi::arg ("cv_index"),
    "@brief Hides the given cell for the given cellview\n"
  ) +
  gsi::method ("show_cell", static_cast<void (lay::LayoutViewBase::*) (db::cell_index_type, int)> (&lay::LayoutViewBase::show_cell), gsi::arg ("cell_index"), gsi::arg ("cv_index"),
    "@brief Shows the given cell for the given cellview (cancel effect of \\hide_cell)\n"
  ) +
  gsi::method ("show_all_cells", (void (lay::LayoutViewBase::*) ()) &lay::LayoutViewBase::show_all_cells,
    "@brief Makes all cells shown (cancel effects of \\hide_cell)"
  ) +
  gsi::method ("show_all_cells", (void (lay::LayoutViewBase::*) (int)) &lay::LayoutViewBase::show_all_cells, gsi::arg ("cv_index"),
    "@brief Makes all cells shown (cancel effects of \\hide_cell) for the specified cell view\n"
    "Unlike \\show_all_cells, this method will only clear the hidden flag on the cell view selected by \\cv_index.\n"
    "\n"
    "This variant has been added in version 0.25."
  ) +
  gsi::method ("update_content", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::force_update_content),
    "@brief Updates the layout view to the current state\n"
    "\n"
    "This method triggers an update of the hierarchy tree and layer view tree. Usually, this "
    "method does not need to be called. The widgets are updated automatically in most cases.\n"
    "\n"
    "Currently, this method should be called however, after the layer view tree has been changed by "
    "the \\insert_layer, \\replace_layer_node or \\delete_layer methods.\n" 
  ) +
  gsi::method ("max_hier", static_cast<void (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::max_hier),
    "@brief Selects all hierarchy levels available\n"
    "\n"
    "Show the layout in full depth down to the deepest level of hierarchy. "
    "This method may cause a redraw."
  ) +
  gsi::method ("resize", static_cast<void (lay::LayoutViewBase::*) (unsigned int, unsigned int)> (&lay::LayoutViewBase::resize),
    "@brief Resizes the layout view to the given dimension\n"
    "\n"
    "This method has been made available in all builds in 0.28.\n"
  ) +
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  gsi::method ("get_screenshot", static_cast<QImage (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::get_screenshot),
    "@brief Gets a screenshot as a \\QImage\n"
    "\n"
    "Getting the image requires the drawing to be complete. Ideally, synchronous mode is switched on "
    "for the application to guarantee this condition. The image will have the size of the viewport "
    "showing the current layout."
  ) +
  gsi::method ("get_image", static_cast<QImage (lay::LayoutViewBase::*) (unsigned int, unsigned int)> (&lay::LayoutViewBase::get_image), gsi::arg ("width"), gsi::arg ("height"),
    "@brief Gets the layout image as a \\QImage\n"
    "\n"
    "@param width The width of the image to render in pixel.\n"
    "@param height The height of the image to render in pixel.\n"
    "\n"
    "The image contains the current scene (layout, annotations etc.).\n"
    "The image is drawn synchronously with the given width and height. Drawing may take some time. "
  ) +
  gsi::method_ext ("get_image_with_options", &get_image_with_options, gsi::arg ("width"), gsi::arg ("height"), gsi::arg ("linewidth", 0), gsi::arg ("oversampling", 0), gsi::arg ("resolution", 0.0), gsi::arg ("target", db::DBox (), "current"), gsi::arg ("monochrome", false),
    "@brief Gets the layout image as a \\QImage (with options)\n"
    "\n"
    "@param width The width of the image to render in pixel.\n"
    "@param height The height of the image to render in pixel.\n"
    "@param linewidth The width of a line in pixels (usually 1) or 0 for default.\n"
    "@param oversampling The oversampling factor (1..3) or 0 for default.\n"
    "@param resolution The resolution (pixel size compared to a screen pixel size, i.e 1/oversampling) or 0 for default.\n"
    "@param target_box The box to draw or an empty box for default.\n"
    "@param monochrome If true, monochrome images will be produced.\n"
    "\n"
    "The image contains the current scene (layout, annotations etc.).\n"
    "The image is drawn synchronously with the given width and height. Drawing may take some time. "
    "Monochrome images don't have background or annotation objects currently.\n"
    "\n"
    "This method has been introduced in version 0.23.10.\n"
  ) +
  gsi::method_ext ("widget", &widget,
    "@brief Gets the QWidget object of the view\n"
    "\n"
    "This method has been introduced in version 0.28.7.\n"
  ) +
#endif
  gsi::method ("get_screenshot_pixels", static_cast<tl::PixelBuffer (lay::LayoutViewBase::*) ()> (&lay::LayoutViewBase::get_screenshot_pb),
    "@brief Gets a screenshot as a \\PixelBuffer\n"
    "\n"
    "Getting the image requires the drawing to be complete. Ideally, synchronous mode is switched on "
    "for the application to guarantee this condition. The image will have the size of the viewport "
    "showing the current layout."
    "\n"
    "This method has been introduced in 0.28.\n"
  ) +
  gsi::method ("get_pixels", static_cast<tl::PixelBuffer (lay::LayoutViewBase::*) (unsigned int, unsigned int)> (&lay::LayoutViewBase::get_pixels), gsi::arg ("width"), gsi::arg ("height"),
    "@brief Gets the layout image as a \\PixelBuffer\n"
    "\n"
    "@param width The width of the image to render in pixel.\n"
    "@param height The height of the image to render in pixel.\n"
    "\n"
    "The image contains the current scene (layout, annotations etc.).\n"
    "The image is drawn synchronously with the given width and height. Drawing may take some time. "
    "\n"
    "This method has been introduced in 0.28.\n"
  ) +
  gsi::method_ext ("get_pixels_with_options", &get_pixels_with_options, gsi::arg ("width"), gsi::arg ("height"), gsi::arg ("linewidth", 0), gsi::arg ("oversampling", 0), gsi::arg ("resolution", 0.0), gsi::arg ("target", db::DBox (), "current"),
    "@brief Gets the layout image as a \\PixelBuffer (with options)\n"
    "\n"
    "@param width The width of the image to render in pixel.\n"
    "@param height The height of the image to render in pixel.\n"
    "@param linewidth The width of a line in pixels (usually 1) or 0 for default.\n"
    "@param oversampling The oversampling factor (1..3) or 0 for default.\n"
    "@param resolution The resolution (pixel size compared to a screen pixel size, i.e 1/oversampling) or 0 for default.\n"
    "@param target_box The box to draw or an empty box for default.\n"
    "\n"
    "The image contains the current scene (layout, annotations etc.).\n"
    "The image is drawn synchronously with the given width and height. Drawing may take some time. "
    "\n"
    "This method has been introduced in 0.28.\n"
  ) +
  gsi::method_ext ("get_pixels_with_options_mono", &get_pixels_with_options_mono, gsi::arg ("width"), gsi::arg ("height"), gsi::arg ("linewidth", 0), gsi::arg ("target", db::DBox (), "current"),
    "@brief Gets the layout image as a \\PixelBuffer (with options)\n"
    "\n"
    "@param width The width of the image to render in pixel.\n"
    "@param height The height of the image to render in pixel.\n"
    "@param linewidth The width of a line in pixels (usually 1) or 0 for default.\n"
    "@param target_box The box to draw or an empty box for default.\n"
    "\n"
    "The image contains the current scene (layout, annotations etc.).\n"
    "The image is drawn synchronously with the given width and height. Drawing may take some time. "
    "Monochrome images don't have background or annotation objects currently.\n"
    "\n"
    "This method has been introduced in 0.28.\n"
  ) +
  gsi::method ("save_screenshot", static_cast<void (lay::LayoutViewBase::*) (const std::string &)> (&lay::LayoutViewBase::save_screenshot), gsi::arg ("filename"),
    "@brief Saves a screenshot to the given file\n"
    "\n"
    "@param filename The file to which to write the screenshot to.\n"
    "\n"
    "The screenshot is written as a PNG file to the given file. "
    "This requires the drawing to be complete. Ideally, synchronous mode is switched on "
    "for the application to guarantee this condition. The image will have the size of the viewport "
    "showing the current layout."
  ) +
  gsi::method ("save_image", static_cast<void (lay::LayoutViewBase::*) (const std::string &, unsigned int, unsigned int)> (&lay::LayoutViewBase::save_image), gsi::arg ("filename"), gsi::arg ("width"), gsi::arg ("height"),
    "@brief Saves the layout as an image to the given file\n"
    "\n"
    "@param filename The file to which to write the screenshot to.\n"
    "@param width The width of the image to render in pixel.\n"
    "@param height The height of the image to render in pixel.\n"
    "\n"
    "The image contains the current scene (layout, annotations etc.).\n"
    "The image is written as a PNG file to the given file. "
    "The image is drawn synchronously with the given width and height. Drawing may take some time. "
  ) +
  gsi::method_ext ("save_image_with_options", &save_image_with_options, gsi::arg ("filename"), gsi::arg ("width"), gsi::arg ("height"), gsi::arg ("linewidth", 0), gsi::arg ("oversampling", 0), gsi::arg ("resolution", 0.0), gsi::arg ("target", db::DBox (), "current"), gsi::arg ("monochrome", false),
    "@brief Saves the layout as an image to the given file (with options)\n"
    "\n"
    "@param filename The file to which to write the screenshot to.\n"
    "@param width The width of the image to render in pixel.\n"
    "@param height The height of the image to render in pixel.\n"
    "@param linewidth The line width scale factor (usually 1) or 0 for 1/resolution.\n"
    "@param oversampling The oversampling factor (1..3) or 0 for the oversampling the view was configured with.\n"
    "@param resolution The resolution (pixel size compared to a screen pixel) or 0 for 1/oversampling.\n"
    "@param target_box The box to draw or an empty box for default.\n"
    "@param monochrome If true, monochrome images will be produced.\n"
    "\n"
    "The image contains the current scene (layout, annotations etc.).\n"
    "The image is written as a PNG file to the given file. "
    "The image is drawn synchronously with the given width and height. Drawing may take some time. "
    "Monochrome images don't have background or annotation objects currently.\n"
    "\n"
    "The 'linewidth' factor scales the layout style line widths.\n"
    "\n"
    "The 'oversampling' factor will use multiple passes passes to create a single image pixels. An "
    "oversampling factor of 2 uses 2x2 virtual pixels to generate an output pixel. This results in a "
    "smoother image. This however comes with a corresponding memory and run time penalty. "
    "When using oversampling, you can set linewidth and resolution to 0. This way, line widths and stipple "
    "pattern are scaled such that the resulting image is equivalent to the standard image.\n"
    "\n"
    "The 'resolution' is the pixel size used to translate font sizes and stipple pattern. A resolution of 0.5 "
    "renders twice as large fonts and stipple pattern. When combining this value with an oversampling factor of 2 "
    "and a line width factor of 2, the resulting image is an oversampled version of the standard image.\n"
    "\n"
    "Examples:\n"
    "\n"
    "@code\n"
    "# standard image 500x500 pixels (oversampling as configured in the view)\n"
    "layout_view.save_image_with_options(\"image.png\", 500, 500)\n"
    "\n"
    "# 2x oversampled image with 500x500 pixels\n"
    "layout_view.save_image_with_options(\"image.png\", 500, 500, 0, 2, 0)\n"
    "\n"
    "# 2x scaled image with 1000x1000 pixels\n"
    "layout_view.save_image_with_options(\"image.png\", 1000, 1000, 2, 1, 0.5)\n"
    "@/code\n"
    "\n"
    "This method has been introduced in 0.23.10.\n"
  ) +
  gsi::method_ext ("#save_as", &save_as2, gsi::arg ("index"), gsi::arg ("filename"), gsi::arg ("gzip"), gsi::arg ("options"),
    "@brief Saves a layout to the given stream file\n"
    "\n"
    "@param index The cellview index of the layout to save.\n"
    "@param filename The file to write.\n"
    "@param gzip Ignored.\n"
    "@param options Writer options.\n"
    "\n"
    "The layout with the given index is written to the stream file with the given options. "
    "'options' is a \\SaveLayoutOptions object that specifies which format to write and further options such "
    "as scaling factor etc.\n"
    "Calling this method is equivalent to calling 'write' on the respective layout object.\n"
    "\n"
    "This method is deprecated starting from version 0.23. The compression mode is "
    "determined from the file name automatically and the \\gzip parameter is ignored.\n"
  ) +
  gsi::method_ext ("save_as", &save_as1, gsi::arg ("index"), gsi::arg ("filename"), gsi::arg ("options"),
    "@brief Saves a layout to the given stream file\n"
    "\n"
    "@param index The cellview index of the layout to save.\n"
    "@param filename The file to write.\n"
    "@param options Writer options.\n"
    "\n"
    "The layout with the given index is written to the stream file with the given options. "
    "'options' is a \\SaveLayoutOptions object that specifies which format to write and further options such "
    "as scaling factor etc.\n"
    "Calling this method is equivalent to calling 'write' on the respective layout object.\n"
    "\n"
    "If the file name ends with a suffix \".gz\" or \".gzip\", the file is compressed with the zlib "
    "algorithm.\n"
  ) +
  gsi::method ("set_layer_properties", static_cast<void (lay::LayoutViewBase::*) (const lay::LayerPropertiesConstIterator &, const lay::LayerProperties &)> (&lay::LayoutViewBase::set_properties), gsi::arg ("iter"), gsi::arg ("props"),
    "@brief Sets the layer properties of the layer pointed to by the iterator\n"
    "\n"
    "This method replaces the layer properties of the element pointed to by \"iter\" by the properties "
    "given by \"props\". It will not change the hierarchy but just the properties of the given node."
  ) +
  gsi::method ("set_layer_properties", static_cast<void (lay::LayoutViewBase::*) (unsigned int index, const lay::LayerPropertiesConstIterator &, const lay::LayerProperties &)> (&lay::LayoutViewBase::set_properties), gsi::arg ("index"), gsi::arg ("iter"), gsi::arg ("props"),
    "@brief Sets the layer properties of the layer pointed to by the iterator\n"
    "\n"
    "This method replaces the layer properties of the element pointed to by \"iter\" by the properties "
    "given by \"props\" in the tab given by \"index\". It will not change the hierarchy but just the properties of the given node."
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("expand_layer_properties", (void (lay::LayoutViewBase::*) ()) &lay::LayoutViewBase::expand_properties,
    "@brief Expands the layer properties for all tabs\n"
    "\n"
    "This method will expand all wildcard specifications in the layer properties by iterating over the specified objects (i.e. layers, cellviews) and "
    "by replacing default colors and stipples by the ones specified with the palettes.\n"
    "\n"
    "This method was introduced in version 0.21.\n"
  ) +
  gsi::method ("expand_layer_properties", (void (lay::LayoutViewBase::*) (unsigned int)) &lay::LayoutViewBase::expand_properties, gsi::arg ("index"),
    "@brief Expands the layer properties for the given tab\n"
    "\n"
    "This method will expand all wildcard specifications in the layer properties by iterating over the specified objects (i.e. layers, cellviews) and "
    "by replacing default colors and stipples by the ones specified with the palettes.\n"
    "\n"
    "This method was introduced in version 0.21.\n"
  ) +
  gsi::method_ext ("replace_layer_node", &replace_layer_node1, gsi::arg ("iter"), gsi::arg ("node"),
    "@brief Replaces the layer node at the position given by \"iter\" with a new one\n"
    "\n"
    "Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode "
    "object can contain a hierarchy of further nodes."
  ) +
  gsi::method_ext ("replace_layer_node", &replace_layer_node2, gsi::arg ("index"), gsi::arg ("iter"), gsi::arg ("node"),
    "@brief Replaces the layer node at the position given by \"iter\" with a new one\n"
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "\n"
    "This method has been introduced in version 0.21.\n"
    "Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode "
    "object can contain a hierarchy of further nodes."
  ) +
  gsi::method_ext ("insert_layer", &insert_layer1, gsi::arg ("iter"), gsi::arg ("node", lay::LayerProperties (), "LayerProperties()"),
    "@brief Inserts the given layer properties node into the list before the given position\n"
    "\n"
    "This method inserts the new properties node before the position given by \"iter\" and returns "
    "a const reference to the element created. The iterator that specified the position will remain valid "
    "after the node was inserted and will point to the newly created node. It can be used to add further nodes. "
    "To add children to the node inserted, use iter.last_child as insertion point for the next insert operations.\n"
    "\n"
    "Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode "
    "object can contain a hierarchy of further nodes.\n"
    "Since version 0.26 the node parameter is optional and the "
    "reference returned by this method can be used to set the properties of the new node."
  ) +
  gsi::method_ext ("insert_layer", &insert_layer2, gsi::arg ("index"), gsi::arg ("iter"), gsi::arg ("node", lay::LayerProperties (), "LayerProperties()"),
    "@brief Inserts the given layer properties node into the list before the given position\n"
    "\n"
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "This method inserts the new properties node before the position given by \"iter\" and returns "
    "a const reference to the element created. The iterator that specified the position will remain valid "
    "after the node was inserted and will point to the newly created node. It can be used to add further nodes. "
    "\n"
    "This method has been introduced in version 0.21.\n"
    "Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode "
    "object can contain a hierarchy of further nodes.\n"
    "Since version 0.26 the node parameter is optional and the "
    "reference returned by this method can be used to set the properties of the new node."
  ) +
  gsi::method_ext ("delete_layers", &delete_layers1, gsi::arg ("iterators"),
    "@brief Deletes the layer properties nodes specified by the iterator\n"
    "\n"
    "This method deletes the nodes specifies by the iterators. This method is the most convenient way to "
    "delete multiple entries.\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  gsi::method_ext ("delete_layers", &delete_layers2, gsi::arg ("index"), gsi::arg ("iterators"),
    "@brief Deletes the layer properties nodes specified by the iterator\n"
    "\n"
    "This method deletes the nodes specifies by the iterators. This method is the most convenient way to "
    "delete multiple entries.\n"
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method ("delete_layer", (void (lay::LayoutViewBase::*) (lay::LayerPropertiesConstIterator &iter)) &lay::LayoutViewBase::delete_layer, gsi::arg ("iter"),
    "@brief Deletes the layer properties node specified by the iterator\n"
    "\n"
    "This method deletes the object that the iterator points to and invalidates\n"
    "the iterator since the object that the iterator points to is no longer valid.\n"
  ) +
  gsi::method ("delete_layer", (void (lay::LayoutViewBase::*) (unsigned int index, lay::LayerPropertiesConstIterator &iter)) &lay::LayoutViewBase::delete_layer, gsi::arg ("index"), gsi::arg ("iter"),
    "@brief Deletes the layer properties node specified by the iterator\n"
    "\n"
    "This method deletes the object that the iterator points to and invalidates\n"
    "the iterator since the object that the iterator points to is no longer valid.\n"
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::iterator_ext ("each_layer", &each_layer,
    "@brief Hierarchically iterates over the layers in the first layer list\n"
    "\n"
    "This iterator will recursively deliver the layers in the first layer list of the view. "
    "The objects presented by the iterator are \\LayerPropertiesNodeRef objects. They can be manipulated to "
    "apply changes to the layer settings or even the hierarchy of layers:\n"
    "\n"
    "@code\n"
    "RBA::LayoutViewBase::current.each_layer do |lref|\n"
    "  # lref is a RBA::LayerPropertiesNodeRef object\n"
    "  lref.visible = false\n"
    "end\n"
    "@/code\n"
    "\n"
    "This method was introduced in version 0.25."
  ) +
  gsi::iterator_ext ("each_layer", &each_layer2, gsi::arg ("layer_list"),
    "@brief Hierarchically iterates over the layers in the given layer list\n"
    "\n"
    "This version of this method allows specification of the layer list to be iterated over. "
    "The layer list is specified by its index which is a value between 0 and \\num_layer_lists-1."
    "For details see the parameter-less version of this method.\n"
    "\n"
    "This method was introduced in version 0.25."
  ) +
  gsi::method ("begin_layers", (lay::LayerPropertiesConstIterator (lay::LayoutViewBase::*) () const) &lay::LayoutViewBase::begin_layers,
    "@brief Begin iterator for the layers\n"
    "\n"
    "This iterator delivers the layers of this view, either in a recursive or non-recursive\n"
    "fashion, depending which iterator increment methods are used.\n"
    "The iterator delivered by \\end_layers is the past-the-end iterator. It can be compared\n"
    "against a current iterator to check, if there are no further elements.\n"
    "\n"
    "Starting from version 0.25, an alternative solution is provided with 'each_layer' which is based on the "
    "\\LayerPropertiesNodeRef class."
  ) +
  gsi::method ("end_layers", (lay::LayerPropertiesConstIterator (lay::LayoutViewBase::*) () const) &lay::LayoutViewBase::end_layers,
    "@brief End iterator for the layers\n"
    "See \\begin_layers for a description about this iterator\n"
  ) +
  gsi::method ("begin_layers", (lay::LayerPropertiesConstIterator (lay::LayoutViewBase::*) (unsigned int index) const) &lay::LayoutViewBase::begin_layers, gsi::arg ("index"),
    "@brief Begin iterator for the layers\n"
    "\n"
    "This iterator delivers the layers of this view, either in a recursive or non-recursive\n"
    "fashion, depending which iterator increment methods are used.\n"
    "The iterator delivered by \\end_layers is the past-the-end iterator. It can be compared\n"
    "against a current iterator to check, if there are no further elements.\n"
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("end_layers", (lay::LayerPropertiesConstIterator (lay::LayoutViewBase::*) (unsigned int index) const) &lay::LayoutViewBase::end_layers, gsi::arg ("index"),
    "@brief End iterator for the layers\n"
    "See \\begin_layers for a description about this iterator\n"
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("clear_layers", (void (lay::LayoutViewBase::*) ()) &lay::LayoutViewBase::clear_layers,
    "@brief Clears all layers\n"
  ) +
  gsi::method ("clear_layers", (void (lay::LayoutViewBase::*) (unsigned int index)) &lay::LayoutViewBase::clear_layers, gsi::arg ("index"),
    "@brief Clears all layers for the given layer properties list\n"
    "This version addresses a specific list in a multi-tab layer properties arrangement with the \"index\" parameter. "
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("delete_layer_list", (void (lay::LayoutViewBase::*) (unsigned int index)) &lay::LayoutViewBase::delete_layer_list, gsi::arg ("index"),
    "@brief Deletes the given properties list\n"
    "At least one layer properties list must remain. This method may change the current properties list.\n"
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("insert_layer_list", (void (lay::LayoutViewBase::*) (unsigned int index)) &lay::LayoutViewBase::insert_layer_list, gsi::arg ("index"),
    "@brief Inserts a new layer properties list at the given index\n"
    "This method inserts a new tab at the given position. The current layer properties list will be changed to "
    "the new list.\n"
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("num_layer_lists", static_cast<unsigned int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::layer_lists),
    "@brief Gets the number of layer properties tabs present\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("current_layer_list", static_cast<unsigned int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::current_layer_list),
    "@brief Gets the index of the currently selected layer properties tab\n"
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("current_layer_list=|#set_current_layer_list", static_cast<void (lay::LayoutViewBase::*) (unsigned int)> (&lay::LayoutViewBase::set_current_layer_list), gsi::arg ("index"),
    "@brief Sets the index of the currently selected layer properties tab\n"
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method ("rename_layer_list", static_cast<void (lay::LayoutViewBase::*) (unsigned int, const std::string &)> (&lay::LayoutViewBase::rename_properties), gsi::arg ("index"), gsi::arg ("name"),
    "@brief Sets the title of the given layer properties tab\n"
    "This method has been introduced in version 0.21.\n"
  ) +
  gsi::method_ext ("remove_stipple", &remove_stipple, gsi::arg ("index"),
    "@brief Removes the stipple pattern with the given index\n"
    "The pattern with an index less than the first custom pattern cannot be removed. "
    "If a stipple pattern is removed that is still used, the results are undefined. "
  ) +
  gsi::method_ext ("clear_stipples", &clear_stipples,
    "@brief Removes all custom line styles\n"
    "All stipple pattern except the fixed ones are removed. If any of the custom stipple pattern is "
    "still used by the layers displayed, the results will be undefined."
  ) +
  gsi::method_ext ("add_stipple", &add_stipple1, gsi::arg ("name"), gsi::arg ("data"), gsi::arg ("bits"),
    "@brief Adds a stipple pattern\n"
    "\n"
    "'data' is an array of unsigned integers describing the bits that make up the stipple "
    "pattern. If the array has less than 32 entries, the pattern will be repeated vertically. "
    "The number of bits used can be less than 32 bit which can be specified by the 'bits' parameter. "
    "Logically, the pattern will be put at the end of the list.\n"
    "\n"
    "@param name The name under which this pattern will appear in the stipple editor\n"
    "@param data See above\n"
    "@param bits See above\n"
    "@return The index of the newly created stipple pattern, which can be used as the dither pattern index of \\LayerProperties."
  ) +
  gsi::method_ext ("add_stipple", &add_stipple2, gsi::arg ("name"), gsi::arg ("string"),
    "@brief Adds a stipple pattern given by a string\n"
    "\n"
    "'string' is a string describing the pattern. It consists of one or more lines composed of '.' or '*' characters and "
    "separated by newline characters. A '.' is for a missing pixel and '*' for a set pixel. The length of each line must be "
    "the same. Blanks before or after each line are ignored.\n"
    "\n"
    "@param name The name under which this pattern will appear in the stipple editor\n"
    "@param string See above\n"
    "@return The index of the newly created stipple pattern, which can be used as the dither pattern index of \\LayerProperties."
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("get_stipple", &get_stipple, gsi::arg ("index"),
    "@brief Gets the stipple pattern string for the pattern with the given index\n"
    "\n"
    "This method will return the stipple pattern string for the pattern with the given index.\n"
    "The format of the string is the same than the string accepted by \\add_stipple.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("remove_line_style", &remove_line_style, gsi::arg ("index"),
    "@brief Removes the line style with the given index\n"
    "The line styles with an index less than the first custom style. "
    "If a style is removed that is still used, the results are undefined.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("clear_line_styles", &clear_line_styles,
    "@brief Removes all custom line styles\n"
    "All line styles except the fixed ones are removed. If any of the custom styles is "
    "still used by the layers displayed, the results will be undefined."
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("add_line_style", &add_line_style1, gsi::arg ("name"), gsi::arg ("data"), gsi::arg ("bits"),
    "@brief Adds a custom line style\n"
    "\n"
    "@param name The name under which this pattern will appear in the style editor\n"
    "@param data A bit set with the new line style pattern (bit 0 is the leftmost pixel)\n"
    "@param bits The number of bits to be used\n"
    "@return The index of the newly created style, which can be used as the line style index of \\LayerProperties."
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("add_line_style", &add_line_style2, gsi::arg ("name"), gsi::arg ("string"),
    "@brief Adds a custom line style from a string\n"
    "\n"
    "@param name The name under which this pattern will appear in the style editor\n"
    "@param string A string describing the bits of the pattern ('.' for missing pixel, '*' for a set pixel)\n"
    "@return The index of the newly created style, which can be used as the line style index of \\LayerProperties."
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("get_line_style", &get_line_style, gsi::arg ("index"),
    "@brief Gets the line style string for the style with the given index\n"
    "\n"
    "This method will return the line style string for the style with the given index.\n"
    "The format of the string is the same than the string accepted by \\add_line_style.\n"
    "An empty string corresponds to 'solid line'.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("current_layer", static_cast<lay::LayerPropertiesConstIterator (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::current_layer),
    "@brief Gets the current layer view\n"
    "\n"
    "Returns the \\LayerPropertiesIterator pointing to the current layer view (the one that has the focus). "
    "If no layer view is active currently, a null iterator is returned.\n"
  ) +
  gsi::method ("current_layer=", static_cast<void (lay::LayoutViewBase::*) (const lay::LayerPropertiesConstIterator &l)> (&lay::LayoutViewBase::set_current_layer), gsi::arg ("iter"),
    "@brief Sets the current layer view\n"
    "\n"
    "Specifies an \\LayerPropertiesIterator pointing to the new current layer view.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("selected_layers", static_cast<std::vector<lay::LayerPropertiesConstIterator> (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::selected_layers),
    "@brief Gets the selected layers\n"
    "\n"
    "Returns an array of \\LayerPropertiesIterator objects pointing to the currently selected layers. "
    "If no layer view is selected currently, an empty array is returned.\n"
  ) +
  gsi::method ("icon_for_layer", &lay::LayoutViewBase::icon_for_layer, gsi::arg ("iter"), gsi::arg ("w"), gsi::arg ("h"), gsi::arg ("dpr"), gsi::arg ("di_off", 0), gsi::arg ("no_state", false),
    "@brief Creates an icon pixmap for the given layer.\n"
    "\n"
    "The icon will have size w times h pixels multiplied by the device pixel ratio (dpr). The dpr is "
    "The number of physical pixels per logical pixels on high-DPI displays.\n"
    "\n"
    "'di_off' will shift the dither pattern by the given number of (physical) pixels. "
    "If 'no_state' is true, the icon will not reflect visibility or validity states but rather the display style.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::event ("on_active_cellview_changed", static_cast<tl::Event (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::active_cellview_changed_event),
    "@brief An event indicating that the active cellview has changed\n"
    "\n"
    "If the active cellview is changed by selecting a new one from the drop-down list, this event is triggered.\n"
    "When this event is triggered, the cellview has already been changed."
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_active_cellview_changed/remove_active_cellview_changed) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_cellviews_changed", static_cast<tl::Event (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::cellviews_changed_event),
    "@brief An event indicating that the cellview collection has changed\n"
    "\n"
    "If new cellviews are added or cellviews are removed, this event is triggered.\n"
    "When this event is triggered, the cellviews have already been changed."
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_cellview_list_observer/remove_cellview_list_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_cellview_changed", static_cast<tl::event<int> (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::cellview_changed_event), gsi::arg ("cellview_index"),
    "@brief An event indicating that a cellview has changed\n"
    "\n"
    "If a cellview is modified, this event is triggered.\n"
    "When this event is triggered, the cellview have already been changed.\n"
    "The integer parameter of this event will indicate the cellview that has changed.\n"
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_cellview_observer/remove_cellview_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_apply_technology", static_cast<tl::event<int> (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::apply_technology_event), gsi::arg ("cellview_index"),
    "@brief An event indicating that a cellview has requested a new technology\n"
    "\n"
    "If the technology of a cellview is changed, this event is triggered.\n"
    "The integer parameter of this event will indicate the cellview that has changed.\n"
    "\n"
    "This event has been introduced in version 0.28.\n"
  ) +
  gsi::event ("on_file_open", static_cast<tl::Event (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::file_open_event),
    "@brief An event indicating that a file was opened\n"
    "\n"
    "If a file is loaded, this event is triggered.\n"
    "When this event is triggered, the file was already loaded and the new file is the new active cellview.\n"
    "Despite its name, this event is also triggered if a layout object is loaded into the view.\n"
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_file_open_observer/remove_file_open_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_viewport_changed", static_cast<tl::Event (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::viewport_changed_event),
    "@brief An event indicating that the viewport (the visible rectangle) has changed\n"
    "\n"
    "This event is triggered after a new display rectangle was chosen - for example, because the user "
    "zoomed into the layout.\n"
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_viewport_changed_observer/remove_viewport_changed_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_layer_list_changed", static_cast<tl::event<int> (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::layer_list_changed_event), gsi::arg ("flags"),
    "@brief An event indicating that the layer list has changed\n"
    "\n"
    "This event is triggered after the layer list has changed its configuration.\n"
    "The integer argument gives a hint about the nature of the changed:\n"
    "Bit 0 is set, if the properties (visibility, color etc.) of one or more layers have changed. Bit 1 is\n"
    "set if the hierarchy has changed. Bit 2 is set, if layer names have changed."
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_layer_list_observer/remove_layer_list_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_layer_list_inserted", static_cast<tl::event<int> (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::layer_list_inserted_event), gsi::arg ("index"),
    "@brief An event indicating that a layer list (a tab) has been inserted\n"
    "@param index The index of the layer list that was inserted\n"
    "\n"
    "This event is triggered after the layer list has been inserted - i.e. a new tab was created.\n"
    "\n"
    "This event was introduced in version 0.25.\n"
  ) +
  gsi::event ("on_layer_list_deleted", static_cast<tl::event<int> (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::layer_list_deleted_event), gsi::arg ("index"),
    "@brief An event indicating that a layer list (a tab) has been removed\n"
    "@param index The index of the layer list that was removed\n"
    "\n"
    "This event is triggered after the layer list has been removed - i.e. a tab was deleted.\n"
    "\n"
    "This event was introduced in version 0.25.\n"
  ) +
  gsi::event ("on_current_layer_list_changed", static_cast<tl::event<int> (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::current_layer_list_changed_event), gsi::arg ("index"),
    "@brief An event indicating the current layer list (the selected tab) has changed\n"
    "@param index The index of the new current layer list\n"
    "\n"
    "This event is triggered after the current layer list was changed - i.e. a new tab was selected.\n"
    "\n"
    "This event was introduced in version 0.25.\n"
  ) +
  gsi::event ("on_cell_visibility_changed", static_cast<tl::Event (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::cell_visibility_changed_event),
    "@brief An event indicating that the visibility of one or more cells has changed\n"
    "\n"
    "This event is triggered after the visibility of one or more cells has changed.\n"
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_cell_visibility_observer/remove_cell_visibility_observer) have been removed in 0.25.\n"
  ) +
  //  HINT: the cast is important to direct GSI to the LayoutView member rather than the
  //  Editables member (in which case we get a segmentation violation ..)
  gsi::event ("on_transient_selection_changed", (tl::Event (lay::LayoutViewBase::*)) &lay::LayoutViewBase::transient_selection_changed_event,
    "@brief An event that is triggered if the transient selection is changed\n"
    "\n"
    "If the transient selection is changed, this event is triggered.\n"
    "The transient selection is the highlighted selection when the mouse hovers over some object(s)."
    "\n"
    "This event was translated from the Observer pattern to an event in version 0.25."
  ) +
  //  HINT: the cast is important to direct GSI to the LayoutView method rather than the
  //  Editables method (in which case we get a segmentation violation ..)
  gsi::event ("on_selection_changed", (tl::Event (lay::LayoutViewBase::*)) &lay::LayoutViewBase::selection_changed_event,
    "@brief An event that is triggered if the selection is changed\n"
    "\n"
    "If the selection changed, this event is triggered.\n"
    "\n"
    "This event was translated from the Observer pattern to an event in version 0.25."
  ) +
  gsi::event ("on_rdb_list_changed", static_cast<tl::Event (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::rdb_list_changed_event),
    "@brief An event that is triggered the list of report databases is changed\n"
    "\n"
    "If a report database is added or removed, this event is triggered.\n"
    "\n"
    "This event was translated from the Observer pattern to an event in version 0.25."
  ) +
  gsi::method ("num_rdbs", static_cast<unsigned int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::num_rdbs),
    "@brief Gets the number of report databases loaded into this view\n"
    "@return The number of \\ReportDatabase objects present in this view\n"
  ) +
  gsi::method ("remove_rdb", static_cast<void (lay::LayoutViewBase::*) (unsigned int)> (&lay::LayoutViewBase::remove_rdb), gsi::arg ("index"),
    "@brief Removes a report database with the given index\n"
    "@param The index of the report database to remove from this view"
  ) +
  gsi::method ("rdb", static_cast <rdb::Database *(lay::LayoutViewBase::*) (int index)> (&lay::LayoutViewBase::get_rdb), gsi::arg ("index"),
    "@brief Gets the report database with the given index\n"
    "@return The \\ReportDatabase object or nil if the index is not valid"
  ) +
  gsi::method ("add_rdb", static_cast<unsigned int (lay::LayoutViewBase::*) (rdb::Database *)> (&lay::LayoutViewBase::add_rdb), gsi::arg ("db"),
    "@brief Adds the given report database to the view\n"
    "\n"
    "This method will add an existing database to the view. It will then appear in the marker database browser.\n"
    "A similar method is \\create_rdb which will create a new database within the view.\n"
    "\n"
    "@return The index of the database within the view (see \\rdb)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("replace_rdb", static_cast<unsigned int (lay::LayoutViewBase::*) (unsigned int, rdb::Database *)> (&lay::LayoutViewBase::replace_rdb), gsi::arg ("db_index"), gsi::arg ("db"),
    "@brief Replaces the report database with the given index\n"
    "\n"
    "If the index is not valid, the database will be added to the view (see \\add_rdb).\n"
    "\n"
    "@return The index of the database within the view (see \\rdb)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method_ext ("create_rdb", &create_rdb, gsi::arg ("name"),
    "@brief Creates a new report database and returns the index of the new database\n"
    "@param name The name of the new report database\n"
    "@return The index of the new database\n"
    "This method returns an index of the new report database. Use \\rdb to get the actual object. "
    "If a report database with the given name already exists, a unique name will be created.\n"
    "The name will be replaced by the file name when a file is loaded into the report database.\n"
  ) +
  gsi::event ("on_l2ndb_list_changed", static_cast<tl::Event (lay::LayoutViewBase::*)> (&lay::LayoutViewBase::l2ndb_list_changed_event),
    "@brief An event that is triggered the list of netlist databases is changed\n"
    "\n"
    "If a netlist database is added or removed, this event is triggered.\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("num_l2ndbs", static_cast<unsigned int (lay::LayoutViewBase::*) () const> (&lay::LayoutViewBase::num_l2ndbs),
    "@brief Gets the number of netlist databases loaded into this view\n"
    "@return The number of \\LayoutToNetlist objects present in this view\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("remove_l2ndb", static_cast<void (lay::LayoutViewBase::*) (unsigned int)> (&lay::LayoutViewBase::remove_l2ndb), gsi::arg ("index"),
    "@brief Removes a netlist database with the given index\n"
    "@param The index of the netlist database to remove from this view"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("l2ndb", static_cast <db::LayoutToNetlist *(lay::LayoutViewBase::*) (int index)> (&lay::LayoutViewBase::get_l2ndb), gsi::arg ("index"),
    "@brief Gets the netlist database with the given index\n"
    "@return The \\LayoutToNetlist object or nil if the index is not valid"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("add_l2ndb", static_cast<unsigned int (lay::LayoutViewBase::*) (db::LayoutToNetlist *)> (&lay::LayoutViewBase::add_l2ndb), gsi::arg ("db"),
    "@brief Adds the given netlist database to the view\n"
    "\n"
    "This method will add an existing database to the view. It will then appear in the netlist database browser.\n"
    "A similar method is \\create_l2ndb which will create a new database within the view.\n"
    "\n"
    "@return The index of the database within the view (see \\l2ndb)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("replace_l2ndb", static_cast<unsigned int (lay::LayoutViewBase::*) (unsigned int, db::LayoutToNetlist *)> (&lay::LayoutViewBase::replace_l2ndb), gsi::arg ("db_index"), gsi::arg ("db"),
    "@brief Replaces the netlist database with the given index\n"
    "\n"
    "If the index is not valid, the database will be added to the view (see \\add_lvsdb).\n"
    "\n"
    "@return The index of the database within the view (see \\lvsdb)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method_ext ("create_l2ndb", &create_l2ndb, gsi::arg ("name"),
    "@brief Creates a new netlist database and returns the index of the new database\n"
    "@param name The name of the new netlist database\n"
    "@return The index of the new database\n"
    "This method returns an index of the new netlist database. Use \\l2ndb to get the actual object. "
    "If a netlist database with the given name already exists, a unique name will be created.\n"
    "The name will be replaced by the file name when a file is loaded into the netlist database.\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method_ext ("lvsdb", &get_lvsdb, gsi::arg ("index"),
    "@brief Gets the netlist database with the given index\n"
    "@return The \\LayoutVsSchematic object or nil if the index is not valid"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method_ext ("add_lvsdb", &add_lvsdb, gsi::arg ("db"),
    "@brief Adds the given database to the view\n"
    "\n"
    "This method will add an existing database to the view. It will then appear in the netlist database browser.\n"
    "A similar method is \\create_lvsdb which will create a new database within the view.\n"
    "\n"
    "@return The index of the database within the view (see \\lvsdb)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method_ext ("replace_lvsdb", &replace_lvsdb, gsi::arg ("db_index"), gsi::arg ("db"),
    "@brief Replaces the database with the given index\n"
    "\n"
    "If the index is not valid, the database will be added to the view (see \\add_lvsdb).\n"
    "\n"
    "@return The index of the database within the view (see \\lvsdb)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method_ext ("create_lvsdb", &create_lvsdb, gsi::arg ("name"),
    "@brief Creates a new netlist database and returns the index of the new database\n"
    "@param name The name of the new netlist database\n"
    "@return The index of the new database\n"
    "This method returns an index of the new netlist database. Use \\lvsdb to get the actual object. "
    "If a netlist database with the given name already exists, a unique name will be created.\n"
    "The name will be replaced by the file name when a file is loaded into the netlist database.\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  //  HINT: the cast is important to direct GSI to the LayoutView method rather than the
  //  Plugin method (in which case we get a segmentation violation ..)
  //  TODO: this method belongs to the Plugin interface and should be located there.
  //  Change this once there is a mixin concept available and the Plugin interface can 
  //  be mixed into LayoutView.
  gsi::method ("clear_config", (void (lay::LayoutViewBase::*)()) &lay::LayoutViewBase::clear_config,
    "@brief Clears the local configuration parameters\n"
    "\n"
    "See \\set_config for a description of the local configuration parameters."
  ) +
  //  TODO: this method belongs to the Plugin interface and should be located there.
  //  Change this once there is a mixin concept available and the Plugin interface can 
  //  be mixed into LayoutView.
  gsi::method_ext ("get_config_names", &get_config_names,
    "@brief Gets the configuration parameter names\n"
    "\n"
    "@return A list of configuration parameter names\n"
    "\n"
    "This method returns the names of all known configuration parameters. These names can be used to "
    "get and set configuration parameter values.\n"
    "\n"
    "This method was introduced in version 0.25.\n"
  ) +
  //  TODO: this method belongs to the Plugin interface and should be located there.
  //  Change this once there is a mixin concept available and the Plugin interface can 
  //  be mixed into LayoutView.
  gsi::method ("get_config", (std::string (lay::LayoutViewBase::*)(const std::string &name) const) &lay::LayoutViewBase::config_get, gsi::arg ("name"),
    "@brief Gets the value of a local configuration parameter\n"
    "\n"
    "@param name The name of the configuration parameter whose value shall be obtained (a string)\n"
    "\n"
    "@return The value of the parameter\n"
    "\n"
    "See \\set_config for a description of the local configuration parameters."
  ) +
  //  TODO: this method belongs to the Plugin interface and should be located there.
  //  Change this once there is a mixin concept available and the Plugin interface can 
  //  be mixed into LayoutView.
  gsi::method ("set_config", (void (lay::LayoutViewBase::*)(const std::string &name, const std::string &value)) &lay::LayoutViewBase::config_set, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Sets a local configuration parameter with the given name to the given value\n"
    "\n"
    "@param name The name of the configuration parameter to set\n"
    "@param value The value to which to set the configuration parameter\n"
    "\n"
    "This method sets a local configuration parameter with the given name to the given value. "
    "Values can only be strings. Numerical values have to be converted into strings first. "
    "Local configuration parameters override global configurations for this specific view. "
    "This allows for example to override global settings of background colors. "
    "Any local settings are not written to the configuration file. "
  ) +
  //  TODO: this method belongs to the Plugin interface and should be located there.
  //  Change this once there is a mixin concept available and the Plugin interface can 
  //  be mixed into LayoutView.
  gsi::method ("commit_config", (void (lay::LayoutViewBase::*)()) &lay::LayoutViewBase::config_end,
    "@brief Commits the configuration settings\n"
    "\n"
    "Some configuration options are queued for performance reasons and become active only after 'commit_config' has been called. "
    "After a sequence of \\set_config calls, this method should be called to activate the "
    "settings made by these calls.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) + 
  gsi::method_ext ("transaction", &gsi::transaction, gsi::arg ("description"),
    "@brief Begins a transaction\n"
    "\n"
    "@param description A text that appears in the 'undo' description\n"
    "\n"
    "A transaction brackets a sequence of database modifications that appear as a single "
    "undo action. Only modifications that are wrapped inside a transaction..commit call pair "
    "can be undone.\n"
    "Each transaction must be terminated with a \\commit method call, even if some error occurred. "
    "It is advisable therefore to catch errors and issue a commit call in this case.\n"
    "\n"
    "This method was introduced in version 0.16."
  ) +
  gsi::method_ext ("commit", &gsi::commit,
    "@brief Ends a transaction\n"
    "\n"
    "See \\transaction for a detailed description of transactions. "
    "\n"
    "This method was introduced in version 0.16."
  ) +
  gsi::method_ext ("is_transacting?", &gsi::transacting,
    "@brief Indicates if a transaction is ongoing\n"
    "\n"
    "See \\transaction for a detailed description of transactions. "
    "\n"
    "This method was introduced in version 0.16."
  ) +
  gsi::method_ext ("clear_transactions", &gsi::clear_transactions,
    "@brief Clears all transactions\n"
    "\n"
    "Discard all actions in the undo buffer. After clearing that buffer, no undo is available. "
    "It is important to clear the buffer when making database modifications outside transactions, i.e "
    "after that modifications have been done. If failing to do so, 'undo' operations are likely to produce "
    "invalid results."
    "\n"
    "This method was introduced in version 0.16."
  ) +
  gsi::method_ext ("send_key_press_event", &send_key_press_event, gsi::arg ("key"), gsi::arg ("buttons"),
    "@brief Sends a key press event\n"
    "\n"
    "This method is intended to emulate the key press events sent by Qt normally in environments where Qt is not present. "
    "The arguments follow the conventions used within \\Plugin#key_event for example.\n"
    "\n"
    "This method was introduced in version 0.28."
  ) +
  gsi::method_ext ("send_mouse_move_event", &send_mouse_move_event, gsi::arg ("pt"), gsi::arg ("buttons"),
    "@brief Sends a mouse move event\n"
    "\n"
    "This method is intended to emulate the mouse move events sent by Qt normally in environments where Qt is not present. "
    "The arguments follow the conventions used within \\Plugin#mouse_moved_event for example.\n"
    "\n"
    "This method was introduced in version 0.28."
  ) +
  gsi::method_ext ("send_mouse_press_event", &send_mouse_press_event, gsi::arg ("pt"), gsi::arg ("buttons"),
    "@brief Sends a mouse button press event\n"
    "\n"
    "This method is intended to emulate the mouse button press events sent by Qt normally in environments where Qt is not present. "
    "The arguments follow the conventions used within \\Plugin#mouse_moved_event for example.\n"
    "\n"
    "This method was introduced in version 0.28."
  ) +
  gsi::method_ext ("send_mouse_double_clicked_event", &send_mouse_double_clicked_event, gsi::arg ("pt"), gsi::arg ("buttons"),
    "@brief Sends a mouse button double-click event\n"
    "\n"
    "This method is intended to emulate the mouse button double-click events sent by Qt normally in environments where Qt is not present. "
    "The arguments follow the conventions used within \\Plugin#mouse_moved_event for example.\n"
    "\n"
    "This method was introduced in version 0.28."
  ) +
  gsi::method_ext ("send_mouse_release_event", &send_mouse_release_event, gsi::arg ("pt"), gsi::arg ("buttons"),
    "@brief Sends a mouse button release event\n"
    "\n"
    "This method is intended to emulate the mouse button release events sent by Qt normally in environments where Qt is not present. "
    "The arguments follow the conventions used within \\Plugin#mouse_moved_event for example.\n"
    "\n"
    "This method was introduced in version 0.28."
  ) +
  gsi::method_ext ("send_leave_event", &send_leave_event,
    "@brief Sends a mouse window leave event\n"
    "\n"
    "This method is intended to emulate the mouse mouse window leave events sent by Qt normally in environments where Qt is not present. "
    "\n"
    "This method was introduced in version 0.28."
  ) +
  gsi::method_ext ("send_enter_event", &send_enter_event,
    "@brief Sends a mouse window leave event\n"
    "\n"
    "This method is intended to emulate the mouse mouse window leave events sent by Qt normally in environments where Qt is not present. "
    "\n"
    "This method was introduced in version 0.28."
  ) +
  gsi::method_ext ("send_wheel_event", &send_wheel_event, gsi::arg ("delta"), gsi::arg ("horizontal"), gsi::arg ("pt"), gsi::arg ("buttons"),
    "@brief Sends a mouse wheel event\n"
    "\n"
    "This method is intended to emulate the mouse wheel events sent by Qt normally in environments where Qt is not present. "
    "The arguments follow the conventions used within \\Plugin#wheel_event for example.\n"
    "\n"
    "This method was introduced in version 0.28."
  ),
  "@hide\n"
  "@alias LayoutView\n"
);

gsi::EnumIn<lay::LayoutViewBase, lay::Editable::SelectionMode> decl_layLayoutView_SelectionMode ("lay", "SelectionMode",
  gsi::enum_const ("Add", lay::Editable::SelectionMode::Add,
    "@brief Adds to any existing selection\n"
  ) +
  gsi::enum_const ("Reset", lay::Editable::SelectionMode::Reset,
    "@brief Removes from any existing selection\n"
  ) +
  gsi::enum_const ("Replace", lay::Editable::SelectionMode::Replace,
    "@brief Replaces the existing selection\n"
  ) +
  gsi::enum_const ("Invert", lay::Editable::SelectionMode::Invert,
    "@brief Adds to any existing selection, if it's not there yet or removes it from the selection if it's already selected\n"
  ),
  "@brief Specifies how selected objects interact with already selected ones.\n"
  "\n"
  "This enum was introduced in version 0.27.\n"
);

//  Inject the NetlistCrossReference::Status declarations into NetlistCrossReference:
gsi::ClassExt<lay::LayoutViewBase> inject_SelectionMode_in_parent (decl_layLayoutView_SelectionMode.defs ());

static db::Layout *get_layout (const lay::CellViewRef *cv)
{
  if ((*cv).operator-> ()) {
    return &(*cv)->layout ();
  } else {
    return 0;
  }
}

static std::string name (const lay::CellViewRef *cv)
{
  if ((*cv).operator-> ()) {
    return (*cv)->name ();
  } else {
    return std::string ();
  }
}

static void set_name (lay::CellViewRef *cv, const std::string &name)
{
  cv->set_name (name);
}

static std::string filename (const lay::CellViewRef *cv)
{
  if ((*cv).operator-> ()) {
    return (*cv)->filename ();
  } else {
    return std::string ();
  }
}

static bool is_dirty (const lay::CellViewRef *cv)
{
  if ((*cv).operator-> ()) {
    return (*cv)->is_dirty ();
  } else {
    return false;
  }
}

static void apply_technology (const lay::CellViewRef *cv, const std::string &tech)
{
  if ((*cv).operator-> ()) {
    (*cv)->apply_technology (tech);
  }
}

static std::string get_technology (const lay::CellViewRef *cv)
{
  if ((*cv).operator-> ()) {
    return (*cv)->tech_name ();
  } else {
    return std::string ();
  }
}

static tl::Event &get_technology_changed_event (lay::CellViewRef *cv)
{
  if (! cv->is_valid ()) {
    throw tl::Exception (tl::to_string (tr ("Not a valid cellview")));
  }
  return (*cv)->technology_changed_event;
}

static void set_cell (lay::CellViewRef *cv, db::Cell *cell)
{
  if (! cell) {
    cv->reset_cell ();
  } else {
    cv->set_cell (cell->cell_index ());
  }
}

static void close_cellview (lay::CellViewRef *cv)
{
  if (cv->is_valid ()) {
    cv->view ()->erase_cellview (cv->index ());
  }
}

static std::string get_cell_name (const lay::CellViewRef *cv)
{
  if (cv->cell () == 0) {
    return std::string ();
  } else {
    return (*cv)->layout ().cell_name (cv->cell_index ());
  }
}

static void cv_descend (lay::CellViewRef *cv, const std::vector<db::InstElement> &path)
{
  if (cv->is_valid ()) {
    cv->view ()->descend (path, cv->index ());
  }
}

static void cv_ascend (lay::CellViewRef *cv)
{
  if (cv->is_valid ()) {
    cv->view ()->ascend (cv->index ());
  }
}

static bool cv_is_cell_hidden (lay::CellViewRef *cv, const db::Cell *cell)
{
  if (cv->is_valid () && cell) {
    if (cell->layout () != &(*cv)->layout ()) {
      throw tl::Exception (tl::to_string (tr ("The cell is not a cell of the view's layout")));
    }
    return cv->view ()->is_cell_hidden (cell->cell_index (), cv->index ());
  } else {
    return false;
  }
}

static void cv_hide_cell (lay::CellViewRef *cv, const db::Cell *cell)
{
  if (cv->is_valid () && cell) {
    if (cell->layout () != &(*cv)->layout ()) {
      throw tl::Exception (tl::to_string (tr ("The cell is not a cell of the view's layout")));
    }
    cv->view ()->hide_cell (cell->cell_index (), cv->index ());
  }
}

static void cv_show_cell (lay::CellViewRef *cv, const db::Cell *cell)
{
  if (cv->is_valid () && cell) {
    if (cell->layout () != &(*cv)->layout ()) {
      throw tl::Exception (tl::to_string (tr ("The cell is not a cell of the view's layout")));
    }
    cv->view ()->show_cell (cell->cell_index (), cv->index ());
  }
}

static void cv_show_all_cells (lay::CellViewRef *cv)
{
  if (cv->is_valid ()) {
    cv->view ()->show_all_cells (cv->index ());
  }
}

Class<lay::CellViewRef> decl_CellView ("lay", "CellView",
  method ("==", static_cast<bool (lay::CellViewRef::*) (const lay::CellViewRef &) const> (&lay::CellViewRef::operator==), gsi::arg ("other"),
    "@brief Equality: indicates whether the cellviews refer to the same one\n"
    "In version 0.25, the definition of the equality operator has been changed to reflect identity of the "
    "cellview. Before that version, identity of the cell shown was implied."
  ) +
  method ("index", &lay::CellViewRef::index,
    "@brief Gets the index of this cellview in the layout view\n"
    "The index will be negative if the cellview is not a valid one.\n"
    "This method has been added in version 0.25.\n"
  ) +
  method ("is_valid?", &lay::CellViewRef::is_valid,
    "@brief Returns true, if the cellview is valid\n"
    "A cellview may become invalid if the corresponding tab is closed for example."
  ) +
  method ("path=|set_path", &lay::CellViewRef::set_unspecific_path, gsi::arg ("path"),
    "@brief Sets the unspecific part of the path explicitly\n"
    "\n"
    "Setting the unspecific part of the path will clear the context path component and\n"
    "update the context and target cell.\n"
  ) +
  method ("context_path=|set_context_path", &lay::CellViewRef::set_specific_path, gsi::arg ("path"),
    "@brief Sets the context path explicitly\n"
    "\n"
    "This method assumes that the unspecific part of the path \n"
    "is established already and that the context path starts\n"
    "from the context cell.\n"
  ) +
  method ("cell_index=|set_cell", (void (lay::CellViewRef::*) (lay::CellViewRef::cell_index_type)) &lay::CellViewRef::set_cell, gsi::arg ("cell_index"),
    "@brief Sets the path to the given cell\n"
    "\n"
    "This method will construct any path to this cell, not a \n"
    "particular one. It will clear the context path\n"
    "and update the context and target cell. Note that the cell is specified by its index.\n"
  ) +
  method ("cell_name=|set_cell_name", (void (lay::CellViewRef::*) (const std::string &)) &lay::CellViewRef::set_cell, gsi::arg ("cell_name"),
    "@brief Sets the cell by name\n"
    "\n"
    "If the name is not a valid one, the cellview will become\n"
    "invalid.\n"
    "This method will construct any path to this cell, not a \n"
    "particular one. It will clear the context path\n"
    "and update the context and target cell.\n"
  ) +
  method_ext ("cell=", set_cell, gsi::arg ("cell"),
    "@brief Sets the cell by reference to a Cell object\n"
    "Setting the cell reference to nil invalidates the cellview. "
    "This method will construct any path to this cell, not a \n"
    "particular one. It will clear the context path\n"
    "and update the context and target cell.\n"
  ) + 
  method ("reset_cell", &lay::CellViewRef::reset_cell,
    "@brief Resets the cell \n"
    "\n"
    "The cellview will become invalid. The layout object will\n"
    "still be attached to the cellview, but no cell will be selected.\n"
  ) +
  method ("ctx_cell_index", &lay::CellViewRef::ctx_cell_index,
    "@brief Gets the context cell's index\n"
  ) +
  method ("ctx_cell", &lay::CellViewRef::ctx_cell,
    "@brief Gets the reference to the context cell currently addressed\n"
  ) +
  method ("cell_index", &lay::CellViewRef::cell_index,
    "@brief Gets the target cell's index\n"
  ) +
  method ("cell", &lay::CellViewRef::cell,
    "@brief Gets the reference to the target cell currently addressed\n"
  ) +
  method_ext ("cell_name", &get_cell_name,
    "@brief Gets the name of the target cell currently addressed\n"
  ) +
  method_ext ("filename", &gsi::filename,
    "@brief Gets filename associated with the layout behind the cellview\n"
  ) +
  method_ext ("is_dirty?", &gsi::is_dirty,
    "@brief Gets a flag indicating whether the layout needs saving\n"
    "A layout is 'dirty' if it is modified and needs saving. This method returns "
    "true in this case.\n"
    "\n"
    "This method has been introduced in version 0.24.10.\n"
  ) +
  method_ext ("name", &gsi::name,
    "@brief Gets the unique name associated with the layout behind the cellview\n"
  ) +
  method_ext ("name=", &gsi::set_name, gsi::arg("name"),
    "@brief sets the unique name associated with the layout behind the cellview\n"
    "\n"
    "this method has been introduced in version 0.25."
  ) +
  method ("path", &lay::CellViewRef::unspecific_path,
    "@brief Gets the cell's unspecific part of the path leading to the context cell\n"
  ) +
  method ("context_path", &lay::CellViewRef::specific_path,
    "@brief Gets the cell's context path\n"
    "The context path leads from the context cell to the target cell in a specific "
    "fashion, i.e. describing each instance in detail, not just by cell indexes. If "
    "the context and target cell are identical, the context path is empty."
  ) +
  method ("context_trans", &lay::CellViewRef::context_trans,
    "@brief Gets the accumulated transformation of the context path\n"
    "This is the transformation applied to the target cell before it is shown in the context cell\n"
    "Technically this is the product of all transformations over the context path.\n"
    "See \\context_dtrans for a version delivering a micron-unit space transformation.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  method ("context_dtrans", &lay::CellViewRef::context_dtrans,
    "@brief Gets the accumulated transformation of the context path in micron unit space\n"
    "This is the transformation applied to the target cell before it is shown in the context cell\n"
    "Technically this is the product of all transformations over the context path.\n"
    "See \\context_trans for a version delivering an integer-unit space transformation.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  event_ext ("on_technology_changed", &get_technology_changed_event,
    "@brief An event indicating that the technology has changed\n"
    "This event is triggered when the CellView is attached to a different technology.\n"
    "\n"
    "This event has been introduced in version 0.27.\n"
  ) +
  method_ext ("technology", &get_technology,
    "@brief Returns the technology name for the layout behind the given cell view\n"
    "This method has been added in version 0.23.\n"
  ) + 
  method_ext ("technology=", &apply_technology, gsi::arg ("tech_name"),
    "@brief Sets the technology for the layout behind the given cell view\n"
    "According to the specification of the technology, new layer properties may be loaded "
    "or the net tracer may be reconfigured. If the layout is shown in multiple views, the "
    "technology is updated for all views.\n"
    "This method has been added in version 0.22.\n"
  ) + 
  method_ext ("layout", &get_layout,
    "@brief Gets the reference to the layout object addressed by this view\n"
  ) +
  method_ext ("descend", &cv_descend, gsi::arg ("path"),
    "@brief Descends further into the hierarchy.\n"
    "Adds the given path (given as an array of InstElement objects) to the specific path of the "
    "cellview with the given index. In effect, the cell addressed by the terminal of the new path "
    "components can be shown in the context of the upper cells, if the minimum hierarchy level is "
    "set to a negative value.\n"
    "The path is assumed to originate from the current cell and contain specific instances sorted from "
    "top to bottom."
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("ascend", &cv_ascend,
    "@brief Ascends upwards in the hierarchy.\n"
    "Removes one element from the specific path of the cellview with the given index. Returns the element "
    "removed."
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("is_cell_hidden?", &cv_is_cell_hidden, gsi::arg ("cell"),
    "@brief Returns true, if the given cell is hidden\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("hide_cell", &cv_hide_cell, gsi::arg ("cell"),
    "@brief Hides the given cell\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("show_cell", &cv_show_cell, gsi::arg ("cell"),
    "@brief Shows the given cell (cancels the effect of \\hide_cell)\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("show_all_cells", &cv_show_all_cells,
    "@brief Makes all cells shown (cancel effects of \\hide_cell) for the specified cell view\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("close", &close_cellview,
    "@brief Closes this cell view\n"
    "\n"
    "This method will close the cellview - remove it from the layout view. After this method was called, the "
    "cellview will become invalid (see \\is_valid?).\n"
    "\n"
    "This method was introduced in version 0.25."
  ),
  "@brief A class describing what is shown inside a layout view\n"
  "\n"
  "The cell view points to a specific cell within a certain layout and a hierarchical context.\n"
  "For that, first of all a layout pointer is provided. The cell itself\n"
  "is addressed by an cell_index or a cell object reference.\n"
  "The layout pointer can be nil, indicating that the cell view is invalid.\n"
  "\n"
  "The cell is not only identified by its index or object but also \n"
  "by the path leading to that cell. This path indicates how to find the\n"
  "cell in the hierarchical context of its parent cells. \n"
  "\n"
  "The path is in fact composed of two parts: first in an unspecific fashion,\n"
  "just describing which parent cells are used. The target of this path\n"
  "is called the \"context cell\". It is accessible by the \\ctx_cell_index\n"
  "or \\ctx_cell methods. In the viewer, the unspecific part of the path is\n"
  "the location of the cell in the cell tree.\n"
  "\n"
  "Additionally the path's second part may further identify a specific instance of a certain\n"
  "subcell in the context cell. This is done through a set of \\InstElement\n"
  "objects. The target of this specific path is the actual cell addressed by the\n"
  "cellview. This target cell is accessible by the \\cell_index or \\cell methods.\n"
  "In the viewer, the target cell is shown in the context of the context cell.\n"
  "The hierarchy levels are counted from the context cell, which is on level 0.\n"
  "If the context path is empty, the context cell is identical with the target cell.\n"
  "\n"
  "Starting with version 0.25, the cellview can be modified directly. This will have an immediate "
  "effect on the display. For example, the following code will select a different cell:\n"
  "\n"
  "@code\n"
  "cv = RBA::CellView::active\n"
  "cv.cell_name = \"TOP2\"\n"
  "@/code\n"
  "\n"
  "See @<a href=\"/programming/application_api.xml\">The Application API@</a> for more details about the "
  "cellview objects."
); 

}

