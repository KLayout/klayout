
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


#include <iostream>
#include <fstream>
#include <vector>

#include "laybasicConfig.h"

#include "tlInternational.h"
#include "tlExpression.h"
#include "tlTimer.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "tlExceptions.h"
#include "tlDeferredExecution.h"
#include "layLayoutViewBase.h"
#include "layBitmapsToImage.h"
#include "layViewOp.h"
#include "layViewObject.h"
#include "layConverters.h"
#include "layMove.h"
#include "layZoomBox.h"
#include "layMouseTracker.h"
#include "layEditable.h"
#include "layFixedFont.h"
#include "laySelector.h"
#include "layLayoutCanvas.h"
#include "layRedrawThread.h"
#include "layRedrawThreadWorker.h"
#include "layParsedLayerSource.h"
#include "dbClipboard.h"
#include "dbLayout.h"
#include "dbLayoutUtils.h"
#include "dbManager.h"
#include "dbLibrary.h"
#include "rdb.h"
#include "dbLayoutToNetlist.h"
#include "dbTechnology.h"
#include "tlXMLParser.h"
#include "gsi.h"

#if defined(HAVE_QT)
#  include <QImageWriter>
#endif

// Enable this if you have both Qt and libpng and want to use libpng for saving images:
//  #define PREFER_LIBPNG_FOR_SAVE 1

#include <limits>

namespace lay
{

//  factor for "zoom in & out"
const double zoom_factor = 0.7;

//  factor by which panning is faster in "fast" (+Shift) mode
const double fast_factor = 3.0;

// -------------------------------------------------------------

struct OpHideShowCell 
  : public db::Op
{
  OpHideShowCell (lay::CellView::cell_index_type ci, int cv_index, bool show)
    : m_cell_index (ci), m_cellview_index (cv_index), m_show (show)
  { }

  lay::CellView::cell_index_type m_cell_index;
  int m_cellview_index;
  bool m_show;
};

struct OpSetDitherPattern
  : public db::Op 
{
  OpSetDitherPattern (const lay::DitherPattern &o, const lay::DitherPattern &n)
    : db::Op (), m_old (o), m_new (n)
  { 
    //  nothing yet.
  }

  lay::DitherPattern m_old, m_new;
};

struct OpSetLineStyles
  : public db::Op
{
  OpSetLineStyles (const lay::LineStyles &o, const lay::LineStyles &n)
    : db::Op (), m_old (o), m_new (n)
  {
    //  nothing yet.
  }

  lay::LineStyles m_old, m_new;
};

struct OpSetLayerProps
  : public db::Op
{
  OpSetLayerProps (unsigned int li, unsigned int i, const lay::LayerProperties &o, const lay::LayerProperties &n)
    : m_list_index (li), m_index (i), m_old (o), m_new (n)
  { 
    //  .. nothing yet ..
  }
  
  unsigned int m_list_index;
  size_t m_index;
  lay::LayerProperties m_old, m_new;
};

struct OpSetLayerPropsNode 
  : public db::Op
{
  OpSetLayerPropsNode (unsigned int li, unsigned int i, const lay::LayerPropertiesNode &o, const lay::LayerPropertiesNode &n)
    : m_list_index (li), m_index (i), m_old (o), m_new (n)
  { 
    //  .. nothing yet ..
  }
  
  unsigned int m_list_index;
  size_t m_index;
  lay::LayerPropertiesNode m_old, m_new;
};

struct OpDeleteLayerList 
  : public db::Op
{
  OpDeleteLayerList (unsigned int li, const lay::LayerPropertiesList &o)
    : m_list_index (li), m_old (o)
  { 
    //  .. nothing yet ..
  }
  
  unsigned int m_list_index;
  lay::LayerPropertiesList m_old;
};

struct OpInsertLayerList 
  : public db::Op
{
  OpInsertLayerList (unsigned int li, const lay::LayerPropertiesList &n)
    : m_list_index (li), m_new (n)
  { 
    //  .. nothing yet ..
  }
  
  unsigned int m_list_index;
  lay::LayerPropertiesList m_new;
};

struct OpRenameProps 
  : public db::Op
{
  OpRenameProps (unsigned int li, const std::string &old_name, const std::string &new_name)
    : m_list_index (li), m_old (old_name), m_new (new_name)
  { 
    //  .. nothing yet ..
  }
  
  unsigned int m_list_index;
  std::string m_old, m_new;
};

struct OpSetAllProps 
  : public db::Op
{
  OpSetAllProps (unsigned int li, const lay::LayerPropertiesList &o, const lay::LayerPropertiesList &n)
    : m_list_index (li), m_old (o), m_new (n)
  { 
    //  .. nothing yet ..
  }
  
  unsigned int m_list_index;
  lay::LayerPropertiesList m_old, m_new;
};

struct OpLayerList
  : public db::Op
{
  enum Mode { Delete, Insert };

  OpLayerList (unsigned int li, unsigned int i, const lay::LayerPropertiesNode &n, Mode m)
    : m_list_index (li), m_index (i), m_mode (m), m_node (n)
  { }

  unsigned int m_list_index;
  size_t m_index;
  Mode m_mode;
  lay::LayerPropertiesNode m_node;
};

struct OpInsertLayerProps 
  : public OpLayerList
{
  OpInsertLayerProps (unsigned int li, unsigned int i, const lay::LayerPropertiesNode &n)
    : OpLayerList (li, i, n, Insert)
  { 
    // .. nothing yet ..
  }
};

struct OpDeleteLayerProps 
  : public OpLayerList
{
  OpDeleteLayerProps (unsigned int li, unsigned int i, const lay::LayerPropertiesNode &n)
    : OpLayerList (li, i, n, Delete)
  { 
    // .. nothing yet ..
  }
};

// -------------------------------------------------------------

const double animation_interval = 0.5;

LayoutViewBase::LayoutViewBase (db::Manager *manager, bool editable, lay::Plugin *plugin_parent, unsigned int options)
  : lay::Dispatcher (plugin_parent, false /*not standalone*/),
    mp_ui (0),
    dm_redraw (this, &LayoutViewBase::redraw),
    m_editable (editable),
    m_options (options),
    m_annotation_shapes (manager)
{
  //  either it's us or the parent has a dispatcher
  tl_assert (dispatcher () != 0);

  init (manager);
}

LayoutViewBase::LayoutViewBase (lay::LayoutView *ui, db::Manager *manager, bool editable, lay::Plugin *plugin_parent, unsigned int options)
  : lay::Dispatcher (plugin_parent, false /*not standalone*/),
    mp_ui (ui),
    dm_redraw (this, &LayoutViewBase::redraw),
    m_editable (editable),
    m_options (options),
    m_annotation_shapes (manager)
{
  //  either it's us or the parent has a dispatcher
  tl_assert (dispatcher () != 0);
}

void
LayoutViewBase::copy_from (lay::LayoutViewBase *source)
{
  m_annotation_shapes = source->m_annotation_shapes;

  //  set the handle reference and clear all cell related stuff 
  m_cellviews = source->cellview_list ();
  m_hidden_cells = source->m_hidden_cells;

  //  clear the history, store path and zoom box
  m_display_states.clear ();
  m_display_state_ptr = 0;
  m_synchronous = source->synchronous ();
  m_drawing_workers = source->drawing_workers ();

  begin_layer_updates ();

  //  duplicate the layer properties
  for (size_t i = 0; i < source->m_layer_properties_lists.size (); ++i) {
    if (i >= m_layer_properties_lists.size ()) {
      m_layer_properties_lists.push_back (new lay::LayerPropertiesList (*source->m_layer_properties_lists [i]));
    } else {
      *m_layer_properties_lists [i] = *source->m_layer_properties_lists [i];
    }
    m_layer_properties_lists [i]->attach_view (this, (unsigned int) i);
  }

  end_layer_updates ();

  if (! m_layer_properties_lists.empty ()) {
    mp_canvas->set_dither_pattern (m_layer_properties_lists [0]->dither_pattern ());
    mp_canvas->set_line_styles (m_layer_properties_lists [0]->line_styles ());
  }

  //  copy the title
  m_title = source->m_title;

  layer_list_changed_event (3);

  finish_cellviews_changed ();
}

void
LayoutViewBase::init (db::Manager *mgr)
{
  manager (mgr);

  m_active_cellview_index = -1;
  m_active_cellview_changed_event_enabled = true;

  m_annotation_shapes.manager (mgr);

  m_visibility_changed = false;
  m_disabled_edits = 0;
  m_synchronous = false;
  m_drawing_workers = 1;
  m_from_level = 0;
  m_pan_distance = 0.15;
  m_wheel_mode = 0;
  m_paste_display_mode = 2;
  m_guiding_shape_visible = true;
  m_guiding_shape_line_width = 1;
  m_guiding_shape_color = tl::Color ();
  m_guiding_shape_vertex_size = 5;
  m_to_level = 0;
  m_ctx_dimming = 50;
  m_ctx_hollow = false;
  m_child_ctx_dimming = 50;
  m_child_ctx_hollow = false;
  m_child_ctx_enabled = false;
  m_abstract_mode_width = 10.0;
  m_abstract_mode_enabled = false;
  m_box_text_transform = true;
  m_box_font = 0;
  m_min_size_for_label = 16;
  m_cell_box_visible = true;
  m_text_visible = true;
  m_default_font_size = lay::FixedFont::default_font_size ();
  m_text_lazy_rendering = true;
  m_bitmap_caching = true;
  m_show_properties = false;
  m_apply_text_trans = true;
  m_default_text_size = 0.1;
  m_text_point_mode = false;
  m_text_font = 0;
  m_show_markers = true;
  m_no_stipples = false;
  m_stipple_offset = true;
  m_fit_new_cell = true;
  m_full_hier_new_cell = false;
  m_clear_ruler_new_cell = false;
  m_dbu_coordinates = false;
  m_absolute_coordinates = false;
  m_drop_small_cells = false;
  m_drop_small_cells_value = 10;
  m_drop_small_cells_cond = DSC_Max;
  m_draw_array_border_instances = false;
  m_dirty = false;
  m_prop_changed = false;
  m_animated = false;
  m_phase = 0;
  m_palette = lay::ColorPalette::default_palette ();
  m_stipple_palette = lay::StipplePalette::default_palette ();
  m_display_state_ptr = 0;
  m_mode = std::numeric_limits<int>::min (); // nothing selected yet.
  mp_tracker = 0;
  mp_zoom_service = 0;
  mp_selection_service = 0;
  mp_move_service = 0;
  m_marker_line_width = 0;
  m_marker_vertex_size = 0;
  m_marker_dither_pattern = 1;
  m_marker_line_style = 0;
  m_marker_halo = true;
  m_transient_selection_mode = true;
  m_sel_inside_pcells = false;
  m_add_other_layers = false;
  m_search_range = 5;
  m_search_range_box = 0;

  m_layer_properties_lists.push_back (new LayerPropertiesList ());
  m_layer_properties_lists.back ()->attach_view (this, (unsigned int) (m_layer_properties_lists.size () - 1));
  m_current_layer_list = 0;

  mp_canvas = new lay::LayoutCanvas (this);

  //  occupy services and editables:
  //  these services get deleted by the canvas destructor automatically:
  if ((m_options & LV_NoTracker) == 0) {
    mp_tracker = new lay::MouseTracker (this);
  }
  if ((m_options & LV_NoZoom) == 0) {
    mp_zoom_service = new lay::ZoomService (this);
  }
  if ((m_options & LV_NoSelection) == 0) {
    mp_selection_service = new lay::SelectionService (this);
  }
  if ((m_options & LV_NoMove) == 0) {
    mp_move_service = new lay::MoveService (this);
  }

  create_plugins ();
}

void
LayoutViewBase::finish ()
{
  //  if we're the root dispatcher initialize the menu and build the context menus. No other menus are built so far.
  if (dispatcher () == this) {
    init_menu ();
  }
}

void
LayoutViewBase::init_menu ()
{
  make_menu ();

  //  make the plugins create their menu items
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    //  TODO: get rid of the const_cast hack
    const_cast <lay::PluginDeclaration *> (&*cls)->init_menu (dispatcher ());
  }

  //  if not in editable mode, hide all entries from "edit_mode" group and show all from the "view_mode" group and vice versa
  std::vector<std::string> edit_mode_grp = menu ()->group ("edit_mode");
  for (std::vector<std::string>::const_iterator g = edit_mode_grp.begin (); g != edit_mode_grp.end (); ++g) {
    menu ()->action (*g)->set_visible (is_editable ());
  }
  std::vector<std::string> view_mode_grp = menu ()->group ("view_mode");
  for (std::vector<std::string>::const_iterator g = view_mode_grp.begin (); g != view_mode_grp.end (); ++g) {
    menu ()->action (*g)->set_visible (! is_editable ());
  }
}

void
LayoutViewBase::shutdown ()
{
  //  detach all observers
  //  This is to prevent signals to partially destroyed observers that own a LayoutViewBase
  layer_list_changed_event.clear ();
  layer_list_deleted_event.clear ();
  layer_list_inserted_event.clear ();
  current_layer_list_changed_event.clear ();
  cell_visibility_changed_event.clear ();
  cellviews_about_to_change_event.clear ();
  cellview_about_to_change_event.clear ();
  cellviews_changed_event.clear ();
  cellview_changed_event.clear ();
  rdb_list_changed_event.clear ();
  l2ndb_list_changed_event.clear ();
  file_open_event.clear ();
  hier_changed_event.clear ();
  geom_changed_event.clear ();
  annotations_changed_event.clear ();

  //  detach ourselves from any observed objects to prevent signals while destroying
  tl::Object::detach_from_all_events ();

  //  remove all rdb's
  while (num_rdbs () > 0) {
    remove_rdb (0);
  }

  //  remove all L2N DB's
  while (num_l2ndbs () > 0) {
    remove_l2ndb (0);
  }

  //  delete layer lists
  std::vector<LayerPropertiesList *> layer_properties_lists;
  layer_properties_lists.swap (m_layer_properties_lists);
  for (std::vector<LayerPropertiesList *>::iterator l = layer_properties_lists.begin (); l !=  layer_properties_lists.end (); ++l) {
    if (*l) {
      delete *l;
    }
  }

  //  delete all plugins
  std::vector<lay::Plugin *> plugins;
  plugins.swap (mp_plugins);
  for (std::vector<lay::Plugin *>::iterator p = plugins.begin (); p != plugins.end (); ++p) {
    delete *p;
  }

  //  detach from the manager, so we can safely delete the manager
  manager (0);

  stop ();
}

LayoutViewBase::~LayoutViewBase ()
{
  shutdown ();

  //  because LayoutViewBase and LayoutCanvas both control lifetimes of
  //  ruler objects for example, it is safer to explicitly delete the
  //  LayoutCanvas object here:
  delete mp_canvas;
  mp_canvas = 0;
}

void LayoutViewBase::unregister_plugin (lay::Plugin *pi)
{
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if (pi == *p) {
      mp_plugins.erase (p);
      break;
    }
  }
}

void LayoutViewBase::resize (unsigned int width, unsigned int height)
{
  mp_canvas->resize (width, height);
}

void LayoutViewBase::update_event_handlers ()
{
  tl::Object::detach_from_all_events ();

  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    //  TODO: get rid of the const_cast hack
    const_cast<lay::PluginDeclaration *> ((*p)->plugin_declaration ())->editable_enabled_changed_event.add (this, &LayoutViewBase::signal_plugin_enabled_changed);
  }

  for (unsigned int i = 0; i < cellviews (); ++i) {
    cellview (i)->layout ().hier_changed_event.add (this, &LayoutViewBase::signal_hier_changed);
    cellview (i)->layout ().bboxes_changed_event.add (this, &LayoutViewBase::signal_bboxes_from_layer_changed, i);
    cellview (i)->layout ().dbu_changed_event.add (this, &LayoutViewBase::signal_bboxes_changed);
    cellview (i)->layout ().prop_ids_changed_event.add (this, &LayoutViewBase::signal_prop_ids_changed);
    cellview (i)->layout ().layer_properties_changed_event.add (this, &LayoutViewBase::signal_layer_properties_changed);
    cellview (i)->layout ().cell_name_changed_event.add (this, &LayoutViewBase::signal_cell_name_changed);
    cellview (i)->apply_technology_with_sender_event.add (this, &LayoutViewBase::signal_apply_technology);
  }

  annotation_shapes ().bboxes_changed_any_event.add (this, &LayoutViewBase::signal_annotations_changed);

  mp_canvas->viewport_changed_event.add (this, &LayoutViewBase::viewport_changed);
  mp_canvas->left_arrow_key_pressed.add (this, &LayoutViewBase::pan_left);
  mp_canvas->up_arrow_key_pressed.add (this, &LayoutViewBase::pan_up);
  mp_canvas->right_arrow_key_pressed.add (this, &LayoutViewBase::pan_right);
  mp_canvas->down_arrow_key_pressed.add (this, &LayoutViewBase::pan_down);
  mp_canvas->left_arrow_key_pressed_with_shift.add (this, &LayoutViewBase::pan_left_fast);
  mp_canvas->up_arrow_key_pressed_with_shift.add (this, &LayoutViewBase::pan_up_fast);
  mp_canvas->right_arrow_key_pressed_with_shift.add (this, &LayoutViewBase::pan_right_fast);
  mp_canvas->down_arrow_key_pressed_with_shift.add (this, &LayoutViewBase::pan_down_fast);
}

void LayoutViewBase::viewport_changed ()
{
  viewport_changed_event ();
}

bool LayoutViewBase::accepts_drop (const std::string &path_or_url) const
{
  for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->accepts_drop (path_or_url)) {
      return true;
    }
  }
  return false;
}

void LayoutViewBase::drop_url (const std::string &path_or_url)
{
  for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->accepts_drop (path_or_url)) {
      (*p)->drop_url (path_or_url);
      break;
    }
  }
}

void LayoutViewBase::clear_plugins ()
{
  std::vector<lay::Plugin *> plugins;
  mp_plugins.swap (plugins);
  for (std::vector<lay::Plugin *>::iterator p = plugins.begin (); p != plugins.end (); ++p) {
    delete *p;
  }
  mp_active_plugin = 0;
}

void LayoutViewBase::create_plugins (const lay::PluginDeclaration *except_this)
{
  clear_plugins ();

  //  create the plugins
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {

    if (&*cls != except_this) {

      //  TODO: clean solution. The following is a HACK:
      if (cls.current_name () == "ant::Plugin" || cls.current_name () == "img::Plugin") {
        //  ant and img are created always
        create_plugin (&*cls);
      } else if ((options () & LV_NoPlugins) == 0) {
        //  others: only create unless LV_NoPlugins is set
        create_plugin (&*cls);
      } else if ((options () & LV_NoGrid) == 0 && cls.current_name () == "GridNetPlugin") {
        //  except grid net plugin which is created on request
        create_plugin (&*cls);
      }

    }

  }

  mode (default_mode ());
}

lay::Plugin *LayoutViewBase::create_plugin (const lay::PluginDeclaration *cls)
{
  lay::Plugin *p = cls->create_plugin (manager (), dispatcher (), this);
  if (p) {

    //  unhook the plugin from the script side if created there (prevent GC from destroying it)
    p->gsi::ObjectBase::keep ();

    mp_plugins.push_back (p);
    p->set_plugin_declaration (cls);
  
    //  enable editable functionality
    if (p->editable_interface ()) {
      enable (p->editable_interface (), cls->editable_enabled ());
    }

    update_event_handlers ();

  }
  return p;
}

Plugin *LayoutViewBase::get_plugin_by_name (const std::string &name) const
{
  lay::PluginDeclaration *decl = 0;
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); !decl && cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    if (cls.current_name () == name) {
      decl = cls.operator-> ();
    }
  }

  if (decl) {
    for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
      if ((*p)->plugin_declaration () == decl) {
        return *p;
      }
    }
  }

  return 0;
}

void
LayoutViewBase::set_drawing_workers (int workers)
{
  m_drawing_workers = std::max (0, std::min (100, workers));
}

void
LayoutViewBase::set_synchronous (bool s)
{
  m_synchronous = s;
}

void
LayoutViewBase::message (const std::string & /*s*/, int /*timeout*/)
{
  //  .. nothing yet ..
}

bool
LayoutViewBase::is_dirty () const
{
  return m_dirty;
}

std::string
LayoutViewBase::title () const
{
  if (! m_title.empty ()) {
    return m_title;
  } else if (cellviews () == 0) {
    return tl::to_string (tr ("<empty>"));
  } else {

    int cv_index = active_cellview_index ();
    if (cv_index < 0 || cv_index >= int (cellviews ())) {
      cv_index = 0;
    }

    const lay::CellView &cv0 = cellview (cv_index);

    std::string t;

    t += cv0->name ();
    if (cv0->layout ().is_valid_cell_index (cv0.cell_index ())) {
      t += " [";
      t += cv0->layout ().cell_name (cv0.cell_index ());
      t += "]";
    }

    if (cellviews () > 1) {
      t += " ...";
    }

    return t;

  }
}

void
LayoutViewBase::set_title (const std::string &t)
{
  if (m_title != t) {
    m_title = t;
    emit_title_changed ();
  }
}

void
LayoutViewBase::reset_title ()
{
  if (! m_title.empty ()) {
    m_title = "";
    emit_title_changed ();
  }
}

bool 
LayoutViewBase::configure (const std::string &name, const std::string &value)
{
  lay::Dispatcher::configure (name, value);

  if (mp_move_service && mp_move_service->configure (name, value)) {
    return true;
  }

  if (mp_tracker && mp_tracker->configure (name, value)) {
    return true;
  }

  if (name == cfg_default_lyp_file) {

    m_def_lyp_file = value;
    return false; // not taken - let others set it too.

  } else if (name == cfg_default_add_other_layers) {

    tl::from_string (value, m_add_other_layers);
    return false; // not taken - let others set it too.

  } else if (name == cfg_background_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    background_color (color);
    //  do not take - let others receive the background color events as well
    return false;

  } else if (name == cfg_default_font_size) {

    int df = 0;
    tl::from_string (value, df);
    if (m_default_font_size != df) {
      //  keep a shadow state to correctly issue the redraw call
      m_default_font_size = df;
      lay::FixedFont::set_default_font_size (df);
      redraw_later ();
    }
    //  do not take - let others have the event for the redraw call
    return false;

  } else if (name == cfg_bitmap_oversampling) {

    int os = 1;
    tl::from_string (value, os);
    mp_canvas->set_oversampling (os);
    return true;

  } else if (name == cfg_highres_mode) {

    bool hrm = false;
    tl::from_string (value, hrm);
    mp_canvas->set_highres_mode (hrm);
    return true;

  } else if (name == cfg_image_cache_size) {

    int sz = 0;
    tl::from_string (value, sz);
    mp_canvas->set_image_cache_size (size_t (sz));
    return true;

  } else if (name == cfg_global_trans) {

    tl::Extractor ex (value.c_str ());
    try {
      db::DCplxTrans t;
      ex.read (t);
      set_global_trans (t);
    } catch (...) { }
    return true;

  } else if (name == cfg_ctx_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    ctx_color (color);
    return true;

  } else if (name == cfg_ctx_dimming) {

    int n;
    tl::from_string (value, n);
    ctx_dimming (n);
    return true;

  } else if (name == cfg_ctx_hollow) {

    bool h;
    tl::from_string (value, h);
    ctx_hollow (h);
    return true;

  } else if (name == cfg_child_ctx_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    child_ctx_color (color);
    return true;

  } else if (name == cfg_child_ctx_dimming) {

    int n;
    tl::from_string (value, n);
    child_ctx_dimming (n);
    return true;

  } else if (name == cfg_child_ctx_hollow) {

    bool h;
    tl::from_string (value, h);
    child_ctx_hollow (h);
    return true;

  } else if (name == cfg_child_ctx_enabled) {

    bool h;
    tl::from_string (value, h);
    child_ctx_enabled (h);
    return true;

  } else if (name == cfg_search_range) {

    unsigned int n;
    tl::from_string (value, n);
    set_search_range (n);
    return true;

  } else if (name == cfg_search_range_box) {

    unsigned int n;
    tl::from_string (value, n);
    set_search_range_box (n);
    return true;

  } else if (name == cfg_abstract_mode_enabled) {

    bool e;
    tl::from_string (value, e);
    abstract_mode_enabled (e);
    return true;

  } else if (name == cfg_abstract_mode_width) {

    double w;
    tl::from_string (value, w);
    abstract_mode_width (w);
    return true;

  } else if (name == cfg_min_inst_label_size) {

    int n;
    tl::from_string (value, n);
    min_inst_label_size (n);
    return true;

  } else if (name == cfg_cell_box_text_font) {

    int n;
    tl::from_string (value, n);
    cell_box_text_font (n);
    return true;

  } else if (name == cfg_cell_box_text_transform) {

    bool flag;
    tl::from_string (value, flag);
    cell_box_text_transform (flag);
    return true;

  } else if (name == cfg_cell_box_visible) {

    bool flag;
    tl::from_string (value, flag);
    cell_box_visible (flag);
    return true;

  } else if (name == cfg_cell_box_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    cell_box_color (color);
    return true;

  } else if (name == cfg_text_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    text_color (color);
    return true;

  } else if (name == cfg_text_visible) {

    bool flag;
    tl::from_string (value, flag);
    text_visible (flag);
    return true;

  } else if (name == cfg_bitmap_caching) {

    bool flag;
    tl::from_string (value, flag);
    bitmap_caching (flag);
    return true;

  } else if (name == cfg_text_lazy_rendering) {

    bool flag;
    tl::from_string (value, flag);
    text_lazy_rendering (flag);
    return true;

  } else if (name == cfg_show_properties) {

    bool flag;
    tl::from_string (value, flag);
    show_properties_as_text (flag);
    return true;

  } else if (name == cfg_apply_text_trans) {

    bool flag;
    tl::from_string (value, flag);
    apply_text_trans (flag);
    return true;

  } else if (name == cfg_markers_visible) {

    bool flag;
    tl::from_string (value, flag);
    mp_canvas->set_dismiss_view_objects (! flag);
    return true;

  } else if (name == cfg_no_stipple) {

    bool flag;
    tl::from_string (value, flag);
    no_stipples (flag);
    return true;

  } else if (name == cfg_stipple_offset) {

    bool flag;
    tl::from_string (value, flag);
    offset_stipples (flag);
    return true;

  } else if (name == cfg_default_text_size) {

    double sz;
    tl::from_string (value, sz);
    default_text_size (sz);
    return true;

  } else if (name == cfg_text_point_mode) {

    bool flag;
    tl::from_string (value, flag);
    text_point_mode (flag);
    return true;

  } else if (name == cfg_text_font) {

    int n;
    tl::from_string (value, n);
    text_font (n);
    return true;

  } else if (name == cfg_full_hier_new_cell) {

    bool flag;
    tl::from_string (value, flag);
    full_hier_new_cell (flag);
    return true;

  } else if (name == cfg_fit_new_cell) {

    bool flag;
    tl::from_string (value, flag);
    fit_new_cell (flag);
    return true;

  } else if (name == cfg_clear_ruler_new_cell) {

    bool flag;
    tl::from_string (value, flag);
    clear_ruler_new_cell (flag);
    return true;

  } else if (name == cfg_abs_units) {

    bool flag;
    tl::from_string (value, flag);
    absolute_coordinates (flag);
    return true;

  } else if (name == cfg_guiding_shape_visible) {

    bool v = false;
    tl::from_string (value, v);
    guiding_shapes_visible (v);
    return true;

  } else if (name == cfg_guiding_shape_line_width) {

    int v = 0;
    tl::from_string (value, v);
    guiding_shapes_line_width (v);
    return true;

  } else if (name == cfg_guiding_shape_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    guiding_shapes_color (color);
    return true;

  } else if (name == cfg_guiding_shape_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    guiding_shapes_color (color);
    return true;

  } else if (name == cfg_guiding_shape_vertex_size) {

    int v = 0;
    tl::from_string (value, v);
    guiding_shapes_vertex_size (v);
    return true;

  } else if (name == cfg_paste_display_mode) {

    tl::from_string (value, m_paste_display_mode);
    return true;

  } else if (name == cfg_mouse_wheel_mode) {

    tl::from_string (value, m_wheel_mode);
    return true;

  } else if (name == cfg_pan_distance) {

    double pd;
    tl::from_string (value, pd);
    pan_distance (pd);
    return true;

  } else if (name == cfg_drawing_workers) {

    int workers;
    tl::from_string (value, workers);
    set_drawing_workers (workers);
    return true;

  } else if (name == cfg_drop_small_cells) {

    bool flag;
    tl::from_string (value, flag);
    drop_small_cells (flag);
    return true;

  } else if (name == cfg_drop_small_cells_cond) {

    unsigned int n;
    tl::from_string (value, n);
    drop_small_cells_cond (drop_small_cells_cond_type (n));
    return true;

  } else if (name == cfg_drop_small_cells_value) {

    unsigned int n;
    tl::from_string (value, n);
    drop_small_cells_value (n);
    return true;

  } else if (name == cfg_array_border_instances) {

    bool f;
    tl::from_string (value, f);
    draw_array_border_instances (f);
    return true;

  } else if (name == cfg_dbu_units) {

    bool flag;
    tl::from_string (value, flag);
    dbu_coordinates (flag);
    return true;

  } else if (name == cfg_stipple_palette) {

    lay::StipplePalette palette = lay::StipplePalette::default_palette ();

    try {
      //  empty string means: default palette
      if (! value.empty ()) {
        palette.from_string (value);
      }
    } catch (...) {
      //  ignore errors: just reset the palette 
      palette = lay::StipplePalette::default_palette ();
    }

    set_palette (palette);

    // others need this property too ..
    return false;

  } else if (name == cfg_line_style_palette) {

    lay::LineStylePalette palette = lay::LineStylePalette::default_palette ();

    try {
      //  empty string means: default palette
      if (! value.empty ()) {
        palette.from_string (value);
      }
    } catch (...) {
      //  ignore errors: just reset the palette
      palette = lay::LineStylePalette::default_palette ();
    }

    set_palette (palette);

    // others need this property too ..
    return false;

  } else if (name == cfg_color_palette) {

    lay::ColorPalette palette = lay::ColorPalette::default_palette ();

    try {
      //  empty string means: default palette
      if (! value.empty ()) {
        palette.from_string (value);
      }
    } catch (...) {
      //  ignore errors: just reset the palette 
      palette = lay::ColorPalette::default_palette ();
    }

    set_palette (palette);

    // others need this property too ..
    return false;

  } else if (name == cfg_sel_inside_pcells_mode) {

    bool flag;
    tl::from_string (value, flag);

    if (m_sel_inside_pcells != flag) {
      m_sel_inside_pcells = flag;
      clear_selection ();
    }

    return true;

  } else if (name == cfg_sel_transient_mode) {

    bool flag;
    tl::from_string (value, flag);
    m_transient_selection_mode = flag;

    if (! m_transient_selection_mode) {
      clear_transient_selection ();
    }

    //  do not take - let others receive this configuration as well
    return false;

  } else if (name == cfg_sel_color) {

    tl::Color color;
    lay::ColorConverter ().from_string (value, color);

    //  Change the color
    if (lay::test_and_set (m_marker_color, color)) {
      mp_canvas->update_image ();
    }

    //  do not take - let others receive this configuration as well
    return false;

  } else if (name == cfg_sel_line_width) {

    int lw = 0;
    tl::from_string (value, lw);

    //  Change the line width
    if (lay::test_and_set (m_marker_line_width, lw)) {
      mp_canvas->update_image ();
    }

    //  do not take - let others receive this configuration as well
    return false;

  } else if (name == cfg_sel_dither_pattern) {

    int dp = 0;
    tl::from_string (value, dp);

    //  Change the vertex_size
    if (lay::test_and_set (m_marker_dither_pattern, dp)) {
      mp_canvas->update_image ();
    }

    //  do not take - let others receive this configuration as well
    return false;

  } else if (name == cfg_sel_line_style) {

    int dp = 0;
    tl::from_string (value, dp);

    //  Change the vertex_size
    if (lay::test_and_set (m_marker_line_style, dp)) {
      mp_canvas->update_image ();
    }

    //  do not take - let others receive this configuration as well
    return false;

  } else if (name == cfg_sel_vertex_size) {

    int vs = 0;
    tl::from_string (value, vs);

    //  Change the vertex_size
    if (lay::test_and_set (m_marker_vertex_size, vs)) {
      mp_canvas->update_image ();
    }

    //  do not take - let others receive this configuration as well
    return false;

  } else if (name == cfg_sel_halo) {

    bool halo = 0;
    tl::from_string (value, halo);

    //  Change the vertex_size
    if (lay::test_and_set (m_marker_halo, halo)) {
      mp_canvas->update_image ();
    }

    //  do not take - let others receive this configuration as well
    return false;

  } else {
    return false;
  }
}

void
LayoutViewBase::config_finalize ()
{
  //  .. nothing yet ..
}

void 
LayoutViewBase::enable_edits (bool enable)
{
  //  enable or disable these services:
  if (mp_selection_service) {
    mp_selection_service->enable (enable);
  }
  if (mp_move_service) {
    mp_move_service->enable (enable);
  }

  //  enable or disable the services that implement "lay::ViewService"
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    lay::ViewService *svc = (*p)->view_service_interface ();
    if (svc) {
      svc->enable (enable);
    }
  }

  bool is_enabled = edits_enabled ();

  if (enable) {
    if (m_disabled_edits > 0) {
      --m_disabled_edits;
    }
  } else {
    ++m_disabled_edits;
  }

  if (edits_enabled () != is_enabled) {
    emit_edits_enabled_changed ();
  }
}

void
LayoutViewBase::set_line_styles (const lay::LineStyles &styles)
{
  if (mp_canvas->line_styles () != styles) {

    if (transacting ()) {
      manager ()->queue (this, new OpSetLineStyles (mp_canvas->line_styles (), styles));
    } else if (manager () && ! replaying ()) {
      manager ()->clear ();
    }
    mp_canvas->set_line_styles (styles);

    for (unsigned int i = 0; i < layer_lists (); ++i) {
      m_layer_properties_lists [i]->set_line_styles (styles);
    }

    layer_list_changed_event (1);

  }
}

void
LayoutViewBase::set_dither_pattern (const lay::DitherPattern &pattern)
{
  if (mp_canvas->dither_pattern () != pattern) {

    if (transacting ()) {
      manager ()->queue (this, new OpSetDitherPattern (mp_canvas->dither_pattern (), pattern));
    } else if (manager () && ! replaying ()) {
      manager ()->clear ();
    }
    mp_canvas->set_dither_pattern (pattern); 

    for (unsigned int i = 0; i < layer_lists (); ++i) {
      m_layer_properties_lists [i]->set_dither_pattern (pattern);
    }

    layer_list_changed_event (1);

  }
}

const LayerPropertiesList &
LayoutViewBase::get_properties (unsigned int index) const
{
  if (index >= layer_lists ()) {
    static lay::LayerPropertiesList empty;
    return empty;
  } else {
    return *m_layer_properties_lists [index];
  }
}

void
LayoutViewBase::set_current_layer_list (unsigned int index)
{
  if (index != m_current_layer_list && index < layer_lists ()) {
    m_current_layer_list = index;
    current_layer_list_changed_event (index);
    redraw ();
  }
}

void 
LayoutViewBase::insert_layer_list (unsigned index, const LayerPropertiesList &props)
{
  if (index > layer_lists ()) {
    return;
  }

  if (transacting ()) {
    manager ()->queue (this, new OpInsertLayerList (index, props));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }

  clear_layer_selection ();

  m_layer_properties_lists.insert (m_layer_properties_lists.begin () + index, new LayerPropertiesList (props));
  m_layer_properties_lists [index]->attach_view (this, index);
  merge_dither_pattern (*m_layer_properties_lists [index]);

  m_current_layer_list = index;
  current_layer_list_changed_event (index);

  layer_list_inserted_event (index);

  redraw ();

  m_prop_changed = true;
}

void 
LayoutViewBase::delete_layer_list (unsigned index)
{
  if (index >= layer_lists ()) {
    return;
  }

  if (transacting ()) {
    manager ()->queue (this, new OpDeleteLayerList (index, *m_layer_properties_lists [index]));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }

  clear_layer_selection ();

  delete m_layer_properties_lists [index];
  m_layer_properties_lists.erase (m_layer_properties_lists.begin () + index);

  if (m_current_layer_list > index) {

    --m_current_layer_list;
    current_layer_list_changed_event (m_current_layer_list);

    //  don't tell the other observers because effectively nothing has changed.

  } else if (m_current_layer_list == index) {

    if (m_current_layer_list > 0) {
      --m_current_layer_list;
    }

    current_layer_list_changed_event (m_current_layer_list);

    //  the current list has been deleted.
    layer_list_changed_event (3);

    redraw ();

  }

  layer_list_deleted_event (index);
  m_prop_changed = true;
}

void 
LayoutViewBase::rename_properties (unsigned int index, const std::string &new_name)
{
  if (index >= layer_lists ()) {
    return;
  }

  if (transacting ()) {
    manager ()->queue (this, new OpRenameProps (index, m_layer_properties_lists [index]->name (), new_name));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }

  m_layer_properties_lists [index]->set_name (new_name);

  layer_list_changed_event (4);
}

bool
LayoutViewBase::set_current_layer (unsigned int cv_index, const db::LayerProperties &lp)
{
  //  rename the ones that got shifted.
  lay::LayerPropertiesConstIterator l = begin_layers ();
  while (! l.at_end ()) {
    if (l->source (true).cv_index () == int (cv_index) && l->source (true).layer_props ().log_equal (lp)) {
      set_current_layer (l);
      return true;
    }
    ++l;
  }
  return false;
}

void
LayoutViewBase::clear_layer_selection ()
{
  m_current_layer = lay::LayerPropertiesConstIterator ();
  m_selected_layers.clear ();
}

void
LayoutViewBase::set_current_layer (const lay::LayerPropertiesConstIterator &l)
{
  m_current_layer = l;
  m_selected_layers.clear ();
  m_selected_layers.push_back (l);
}

lay::LayerPropertiesConstIterator
LayoutViewBase::current_layer () const
{
  return m_current_layer;
}

std::vector<lay::LayerPropertiesConstIterator>
LayoutViewBase::selected_layers () const
{
  return m_selected_layers;
}

void
LayoutViewBase::set_selected_layers (const std::vector<lay::LayerPropertiesConstIterator> &sel)
{
  m_selected_layers = sel;
  if (sel.empty ()) {
    m_current_layer = lay::LayerPropertiesConstIterator ();
  } else {
    m_current_layer = sel.front ();
  }
}

/**
 *  @brief A helper function to create an image from a single bitmap
 */
static void
single_bitmap_to_image (const lay::ViewOp &view_op, lay::Bitmap &bitmap,
                        tl::PixelBuffer *pimage, const lay::DitherPattern &dither_pattern, const lay::LineStyles &line_styles,
                        double dpr, unsigned int width, unsigned int height)
{
  std::vector <lay::ViewOp> view_ops;
  view_ops.push_back (view_op);

  std::vector <lay::Bitmap *> pbitmaps;
  pbitmaps.push_back (&bitmap);

  lay::bitmaps_to_image (view_ops, pbitmaps, dither_pattern, line_styles, dpr, pimage, width, height, false, 0);
}

tl::PixelBuffer
LayoutViewBase::icon_for_layer (const LayerPropertiesConstIterator &iter, unsigned int w, unsigned int h, double dpr, unsigned int di_off, bool no_state)
{
  if (dpr < 0.0) {
    dpr = canvas ()->dpr ();
  }

  int oversampling = canvas () ? canvas ()->oversampling () : 1;
  double gamma = 2.0;

  bool hrm = canvas () ? canvas ()->highres_mode () : false;
  double dpr_drawing = oversampling * (hrm ? 1.0 : dpr);

  h = std::max ((unsigned int) 16, h) * oversampling * dpr + 0.5;
  w = std::max ((unsigned int) 16, w) * oversampling * dpr + 0.5;

  tl::color_t def_color   = 0x808080;
  tl::color_t fill_color  = iter->has_fill_color (true)  ? iter->eff_fill_color (true)  : def_color;
  tl::color_t frame_color = iter->has_frame_color (true) ? iter->eff_frame_color (true) : def_color;

  tl::PixelBuffer image (w, h);
  image.set_transparent (true);
  image.fill (background_color ().rgb ());

  //  upper scanline is a dummy one
  tl::color_t *sl0 = (uint32_t *) image.scan_line (0);
  for (size_t i = 0; i < w; ++i) {
    *sl0++ = 0;
  }

  lay::Bitmap fill (w, h, 1.0);
  lay::Bitmap frame (w, h, 1.0);
  lay::Bitmap text (w, h, 1.0);
  lay::Bitmap vertex (w, h, 1.0);

  unsigned int wp = w - 1;

  if (! no_state && ! iter->visible (true)) {

    wp = w / 4;

    //  Show the arrow if it is invisible also locally.
    if (! iter->visible (false)) {

      unsigned int aw = h / 4;
      unsigned int ap = w / 2 - 1;
      for (unsigned int i = 0; i <= aw; ++i) {
        text.fill (h / 2 - 1 - i, ap, ap + aw - i + 1);
        text.fill (h / 2 - 1 + i, ap, ap + aw - i + 1);
      }

    }

  }

  if (! no_state && no_stipples ()) {
    //  Show a partial stipple pattern only for "no stipple" mode
    for (unsigned int i = 1; i < h - 2; ++i) {
      fill.fill (i, w - 1 - w / 4, w);
    }
  } else {
    for (unsigned int i = 1; i < h - 2; ++i) {
      fill.fill (i, w - 1 - wp, w);
    }
  }

  int lw = iter->width (true);
  if (lw < 0) {
    //  default line width is 0 for parents and 1 for leafs
    lw = iter->has_children () ? 0 : 1;
  }
  lw = lw * dpr_drawing + 0.5;

  int p0 = lw / 2;
  p0 = std::max (0, std::min (int (w / 4 - 1), p0));

  int p1 = (lw - 1) / 2;
  p1 = std::max (0, std::min (int (w / 4 - 1), p1));

  int p0x = p0, p1x = p1;
  unsigned int ddx = 0;
  unsigned int ddy = h - 2 - p1 - p0;
  if (iter->xfill (true)) {
    ddx = wp - p0 - p1 - 1;
  }
  unsigned int d = ddx / 2;

  frame.fill (p0, w - 1 - (wp - p1), w);
  frame.fill (h - 2 - p1, w - 1 - (wp - p1), w);

  for (unsigned int i = p0; i < h - 2; ++i) {

    frame.fill (i, w - 1 - p0, w - p0);
    frame.fill (i, w - 1 - (wp - p1), w - (wp - p1));
    frame.fill (i, w - 1 - p0x, w - p0x);
    frame.fill (i, w - 1 - (wp - p1x), w - (wp - p1x));

    while (d < ddx) {
      d += ddy;
      frame.fill (i, w - 1 - p0x, w - p0x);
      frame.fill (i, w - 1 - (wp - p1x), w - (wp - p1x));
      ++p0x;
      ++p1x;
    }

    if (d >= ddx) {
      d -= ddx;
    }

  }

  if (! no_state && ! iter->valid (true)) {

    unsigned int bp = w - 1 - ((w * 7) / 8 - 1);
    unsigned int be = bp + h / 2;
    unsigned int bw = h / 4 - 1;
    unsigned int by = h / 2 - 1;

    for (unsigned int i = 0; i < bw + 2; ++i) {
      fill.clear (by - i, bp - 1, be);
      fill.clear (by + i, bp - 1, be);
    }

    for (unsigned int i = 0; i < bw; ++i) {
      text.fill (by - i, bp + bw - i - 1, bp + bw - i + 1);
      text.fill (by - i - 1, bp + bw - i - 1, bp + bw - i + 1);
      text.fill (by - i, bp + bw + i, bp + bw + i + 2);
      text.fill (by - i - 1, bp + bw + i, bp + bw + i + 2);
      text.fill (by + i, bp + bw - i - 1, bp + bw - i + 1);
      text.fill (by + i + 1, bp + bw - i - 1, bp + bw - i + 1);
      text.fill (by + i, bp + bw + i, bp + bw + i + 2);
      text.fill (by + i + 1, bp + bw + i, bp + bw + i + 2);
    }

  }

  vertex.fill (h / 2 - 1, w - 1 - wp / 2, w - wp / 2);

  lay::ViewOp::Mode mode = lay::ViewOp::Copy;

  //  create fill
  single_bitmap_to_image (lay::ViewOp (fill_color, mode, 0, iter->eff_dither_pattern (true), di_off), fill, &image, dither_pattern (), line_styles (), dpr_drawing, w, h);
  //  create frame
  if (lw == 0) {
    single_bitmap_to_image (lay::ViewOp (frame_color, mode, 0 /*solid line*/, 2 /*dotted*/, 0), frame, &image, dither_pattern (), line_styles (), dpr_drawing, w, h);
  } else {
    single_bitmap_to_image (lay::ViewOp (frame_color, mode, iter->eff_line_style (true), 0, 0, lay::ViewOp::Rect, lw), frame, &image, dither_pattern (), line_styles (), dpr_drawing, w, h);
  }
  //  create text
  single_bitmap_to_image (lay::ViewOp (frame_color, mode, 0, 0, 0), text, &image, dither_pattern (), line_styles (), dpr_drawing, w, h);
  //  create vertex
  single_bitmap_to_image (lay::ViewOp (frame_color, mode, 0, 0, 0, lay::ViewOp::Cross, iter->marked (true) ? int (9 * dpr_drawing + 0.5) : 0), vertex, &image, dither_pattern (), line_styles (), dpr_drawing, w, h);

  if (oversampling > 1) {
    tl::PixelBuffer subsampled (image.width () / oversampling, image.height () / oversampling);
    image.subsample (subsampled, oversampling, gamma);
    return subsampled;
  } else {
    return image;
  }
}

void
LayoutViewBase::merge_dither_pattern (lay::LayerPropertiesList &props)
{
  {
    lay::DitherPattern dp (dither_pattern ());

    std::map <unsigned int, unsigned int> index_map;
    dp.merge (props.dither_pattern (), index_map);

    //  remap the dither pattern index
    for (lay::LayerPropertiesIterator l = props.begin_recursive (); l != props.end_recursive (); ++l) {
      int dpi = l->dither_pattern (false /*local*/);
      std::map <unsigned int, unsigned int>::iterator m = index_map.find ((unsigned int) dpi);
      if (m != index_map.end ()) {
        l->set_dither_pattern (int (m->second));
      }
    }

    //  install the new custom pattern table
    if (mp_canvas->dither_pattern () != dp) {
      mp_canvas->set_dither_pattern (dp);
      for (unsigned int i = 0; i < layer_lists (); ++i) {
        m_layer_properties_lists [i]->set_dither_pattern (dp);
      }
    }
  }

  {
    lay::LineStyles ls (line_styles ());

    std::map <unsigned int, unsigned int> index_map;
    ls.merge (props.line_styles (), index_map);

    //  remap the dither pattern index
    for (lay::LayerPropertiesIterator l = props.begin_recursive (); l != props.end_recursive (); ++l) {
      int lsi = l->line_style (false /*local*/);
      std::map <unsigned int, unsigned int>::iterator m = index_map.find ((unsigned int) lsi);
      if (m != index_map.end ()) {
        l->set_line_style (int (m->second));
      }
    }

    //  install the new custom pattern table
    if (mp_canvas->line_styles () != ls) {
      mp_canvas->set_line_styles (ls);
      for (unsigned int i = 0; i < layer_lists (); ++i) {
        m_layer_properties_lists [i]->set_line_styles (ls);
      }
    }
  }
}

bool
LayoutViewBase::always_show_source () const
{
  return false;
}

bool
LayoutViewBase::always_show_ld () const
{
  return true;
}

bool
LayoutViewBase::always_show_layout_index () const
{
  return false;
}

void 
LayoutViewBase::set_properties (unsigned int index, const LayerPropertiesList &props)
{
  //  If index is not a valid tab index, don't do anything except for the case of
  //  index 0 in which the first entry is created (this can happen as a result of
  //  delete_properties).
  if (index >= layer_lists ()) {
    if (index > 0) {
      return;
    } else {
      m_layer_properties_lists.push_back (new LayerPropertiesList ());
      m_layer_properties_lists.back ()->attach_view (this, (unsigned int) (m_layer_properties_lists.size () - 1));
    }
  }

  //  HINT: this method is quite frequently used in an imperative way. 
  //  Since it has some desired side effects such as forcing a recomputation of the internals, 
  //  it should be executed in any case, even if props == get_properties ().

  if (transacting ()) {
    manager ()->queue (this, new OpSetAllProps (index, get_properties (), props));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }

  if (index == current_layer_list ()) {
    begin_layer_updates ();
  }

  *m_layer_properties_lists [index] = props;
  m_layer_properties_lists [index]->attach_view (this, index);

  merge_dither_pattern (*m_layer_properties_lists [index]);

  if (index == current_layer_list ()) {
    end_layer_updates ();
    layer_list_changed_event (3);
    redraw_later ();
    m_prop_changed = true;
  }
}

void 
LayoutViewBase::expand_properties ()
{
  expand_properties (std::map<int, int> (), false);
}
  
void 
LayoutViewBase::expand_properties (unsigned int index)
{
  expand_properties (index, std::map<int, int> (), false);
}

void 
LayoutViewBase::expand_properties (const std::map<int, int> &map_cv_index, bool add_default)
{
  for (unsigned int i = 0; i < cellviews (); ++i) {
    expand_properties (i, map_cv_index, add_default);
  }
}

void 
LayoutViewBase::expand_properties (unsigned int index, const std::map<int, int> &map_cv_index, bool add_default)
{
  if (index < m_layer_properties_lists.size ()) {
    m_layer_properties_lists [index]->expand (map_cv_index, add_default);
  }
}

void
LayoutViewBase::replace_layer_node (unsigned int index, const LayerPropertiesConstIterator &iter, const LayerPropertiesNode &node)
{
  if (index >= layer_lists ()) {
    return;
  }

  //  if the source specification changed, a redraw is required
  if (*iter != node) {

    if (transacting ()) {
      manager ()->queue (this, new OpSetLayerPropsNode (index, (unsigned int) iter.uint (), *iter, node));
    } else if (manager () && ! replaying ()) {
      manager ()->clear ();
    }

  if (index == current_layer_list ()) {
    begin_layer_updates ();
  }

    LayerPropertiesIterator non_const_iter (get_properties (index), iter.uint ());
    *non_const_iter = node;
    non_const_iter->attach_view (this, index);

    if (index == current_layer_list ()) {
      end_layer_updates ();
      layer_list_changed_event (2);
      //  TODO: check, if redraw is actually necessary (this is complex!)
      redraw_later ();
      m_prop_changed = true;
    }
  }
}

void
LayoutViewBase::set_layer_node_expanded (unsigned int index, const LayerPropertiesConstIterator &iter, bool ex)
{
  if (ex != iter->expanded ()) {

    LayerPropertiesIterator non_const_iter (get_properties (index), iter.uint ());
    non_const_iter->set_expanded (ex);

    if (index == current_layer_list ()) {
      layer_list_changed_event (8 /*expanded state needs update*/);
    }

  }
}

void 
LayoutViewBase::set_properties (unsigned int index, const LayerPropertiesConstIterator &iter, const LayerProperties &props)
{
  if (index >= layer_lists ()) {
    return;
  }

  //  if the source specification changed, a redraw is required
  const LayerProperties &l = *iter;
  if (l != props) {

    if (transacting ()) {
      manager ()->queue (this, new OpSetLayerProps (index, (unsigned int) iter.uint (), l, props));
    } else if (manager () && ! replaying ()) {
      manager ()->clear ();
    }

    bool need_redraw = (l.source (false /*local*/) != props.source (false /*local*/) || l.xfill (false /*local*/) != props.xfill (false /*local*/));
    bool visible_changed = (l.visible (true /*real*/) != props.visible (true /*real*/));

    LayerPropertiesIterator non_const_iter (get_properties (index), iter.uint ());
    *non_const_iter = props;

    if (index == current_layer_list ()) {

      layer_list_changed_event (1);

      if (need_redraw) {
        redraw_later ();
      } 

      if (visible_changed) {
        m_visibility_changed = true;
      }

      //  perform the callbacks asynchronously to collect the necessary calls instead
      //  of executing them immediately.
      m_prop_changed = true;

    }
  }
}

const LayerPropertiesNode &
LayoutViewBase::insert_layer (unsigned int index, const LayerPropertiesConstIterator &before, const LayerPropertiesNode &node)
{
  tl_assert (index < layer_lists ());

  if (transacting ()) {
    manager ()->queue (this, new OpInsertLayerProps (index, (unsigned int) before.uint (), node));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }
  
  if (index == current_layer_list ()) {
    begin_layer_updates ();
  }

  const LayerPropertiesNode &ret = m_layer_properties_lists [index]->insert (LayerPropertiesIterator (*m_layer_properties_lists [index], before.uint ()), node);

  //  signal to the observers that something has changed
  if (index == current_layer_list ()) {
    end_layer_updates ();
    layer_list_changed_event (2);
    redraw_later ();
    m_prop_changed = true;
  }

  return ret;
}

void 
LayoutViewBase::delete_layer (unsigned int index, LayerPropertiesConstIterator &iter)
{
  if (index >= layer_lists ()) {
    return;
  }

  lay::LayerPropertiesNode orig = *iter;

  if (index == current_layer_list ()) {
    begin_layer_updates ();
  }

  //  delete the element
  m_layer_properties_lists [index]->erase (LayerPropertiesIterator (*m_layer_properties_lists [index], iter.uint ()));

  if (transacting ()) {
    manager ()->queue (this, new OpDeleteLayerProps (index, (unsigned int) iter.uint (), orig));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }

  //  signal to the observers that something has changed
  if (index == current_layer_list ()) {
    end_layer_updates ();
    layer_list_changed_event (2);
    redraw_later ();
    m_prop_changed = true;
  }

  //  invalidate the iterator so it can be used to refer to the next element
  iter.invalidate ();
}

void 
LayoutViewBase::save_as (unsigned int index, const std::string &filename, tl::OutputStream::OutputStreamMode om, const db::SaveLayoutOptions &options, bool update, int keep_backups)
{
  tl_assert (index < cellviews ());

  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Saving")));
  cellview (index)->save_as (filename, om, options, update, keep_backups);

  cellview_changed (index);
}

void
LayoutViewBase::redo (db::Op *op)
{
  tl_assert (! transacting ());

  OpSetLayerProps *sop = dynamic_cast <OpSetLayerProps *> (op);
  if (sop) {
    if (sop->m_list_index < m_layer_properties_lists.size ()) {
      set_properties (sop->m_list_index, lay::LayerPropertiesConstIterator (*m_layer_properties_lists [sop->m_list_index], sop->m_index), sop->m_new);
    }
    return;
  }

  OpSetLayerPropsNode *snop = dynamic_cast <OpSetLayerPropsNode *> (op);
  if (snop) {
    if (snop->m_list_index < m_layer_properties_lists.size ()) {
      replace_layer_node (snop->m_list_index, lay::LayerPropertiesConstIterator (*m_layer_properties_lists [snop->m_list_index], snop->m_index), snop->m_new);
    }
    return;
  }

  OpInsertLayerList *ilop = dynamic_cast <OpInsertLayerList *> (op);
  if (ilop) {
    if (ilop->m_list_index <= m_layer_properties_lists.size ()) {
      insert_layer_list (ilop->m_list_index, ilop->m_new);
    }
    return;
  }

  OpDeleteLayerList *dlop = dynamic_cast <OpDeleteLayerList *> (op);
  if (dlop) {
    if (dlop->m_list_index < m_layer_properties_lists.size ()) {
      delete_layer_list (dlop->m_list_index);
    }
    return;
  }

  OpSetAllProps *saop = dynamic_cast <OpSetAllProps *> (op);
  if (saop) {
    if (saop->m_list_index < m_layer_properties_lists.size ()) {
      set_properties (saop->m_list_index, saop->m_new);
    }
    return;
  }

  OpRenameProps *rnop = dynamic_cast <OpRenameProps *> (op);
  if (rnop) {
    if (rnop->m_list_index < m_layer_properties_lists.size ()) {
      rename_properties (rnop->m_list_index, rnop->m_new);
    }
    return;
  }

  OpLayerList *lop = dynamic_cast <OpLayerList *> (op);
  if (lop) {
    if (lop->m_list_index < m_layer_properties_lists.size ()) {
      if (lop->m_mode == OpLayerList::Insert) {
        insert_layer (lop->m_list_index, lay::LayerPropertiesConstIterator (*m_layer_properties_lists [lop->m_list_index], lop->m_index), lop->m_node);
      } else {
        lay::LayerPropertiesConstIterator iter (*m_layer_properties_lists [lop->m_list_index], lop->m_index);
        delete_layer (lop->m_list_index, iter);
      }
    }
    return;
  } 

  OpSetDitherPattern *stpop = dynamic_cast <OpSetDitherPattern *> (op);
  if (stpop) {
    set_dither_pattern (stpop->m_new);
    return;
  }

  OpHideShowCell *hscop = dynamic_cast <OpHideShowCell *> (op);
  if (hscop) {
    if (hscop->m_show) {
      show_cell (hscop->m_cell_index, hscop->m_cellview_index);
    } else {
      hide_cell (hscop->m_cell_index, hscop->m_cellview_index);
    }
    return;
  }

  db::Object::redo (op);
}

void 
LayoutViewBase::undo (db::Op *op)
{
  tl_assert (! transacting ());

  OpSetLayerProps *sop = dynamic_cast <OpSetLayerProps *> (op);
  if (sop) {
    if (sop->m_list_index < m_layer_properties_lists.size ()) {
      set_properties (sop->m_list_index, lay::LayerPropertiesConstIterator (*m_layer_properties_lists [sop->m_list_index], sop->m_index), sop->m_old);
    }
    return;
  }

  OpSetLayerPropsNode *snop = dynamic_cast <OpSetLayerPropsNode *> (op);
  if (snop) {
    if (snop->m_list_index < m_layer_properties_lists.size ()) {
      replace_layer_node (snop->m_list_index, lay::LayerPropertiesConstIterator (*m_layer_properties_lists [snop->m_list_index], snop->m_index), snop->m_old);
    }
    return;
  }

  OpInsertLayerList *ilop = dynamic_cast <OpInsertLayerList *> (op);
  if (ilop) {
    if (ilop->m_list_index <= m_layer_properties_lists.size ()) {
      delete_layer_list (ilop->m_list_index);
    }
    return;
  }

  OpDeleteLayerList *dlop = dynamic_cast <OpDeleteLayerList *> (op);
  if (dlop) {
    if (dlop->m_list_index < m_layer_properties_lists.size ()) {
      insert_layer_list (dlop->m_list_index, dlop->m_old);
    }
    return;
  }

  OpSetAllProps *saop = dynamic_cast <OpSetAllProps *> (op);
  if (saop) {
    if (saop->m_list_index < m_layer_properties_lists.size ()) {
      set_properties (saop->m_list_index, saop->m_old);
    }
    return;
  }

  OpRenameProps *rnop = dynamic_cast <OpRenameProps *> (op);
  if (rnop) {
    if (rnop->m_list_index < m_layer_properties_lists.size ()) {
      rename_properties (rnop->m_list_index, rnop->m_old);
    }
    return;
  }

  OpLayerList *lop = dynamic_cast <OpLayerList *> (op);
  if (lop) {
    if (lop->m_list_index < m_layer_properties_lists.size ()) {
      if (lop->m_mode == OpLayerList::Insert) {
        lay::LayerPropertiesConstIterator iter (*m_layer_properties_lists [lop->m_list_index], lop->m_index);
        delete_layer (lop->m_list_index, iter);
      } else {
        insert_layer (lop->m_list_index, lay::LayerPropertiesConstIterator (*m_layer_properties_lists [lop->m_list_index], lop->m_index), lop->m_node);
      }
    }
    return;
  } 

  OpHideShowCell *hscop = dynamic_cast <OpHideShowCell *> (op);
  if (hscop) {
    
    if (hscop->m_show) {
      hide_cell (hscop->m_cell_index, hscop->m_cellview_index);
    } else {
      show_cell (hscop->m_cell_index, hscop->m_cellview_index);
    }

    return;

  }

  OpSetDitherPattern *stpop = dynamic_cast <OpSetDitherPattern *> (op);
  if (stpop) {
    set_dither_pattern (stpop->m_old);
    return;
  }

  db::Object::undo (op);
}

void
LayoutViewBase::signal_hier_changed ()
{
  //  schedule a redraw request for all layers
  redraw_later ();
  //  forward this event to our observers
  hier_changed_event ();
}

void
LayoutViewBase::signal_bboxes_from_layer_changed (unsigned int cv_index, unsigned int layer_index)
{
  if (layer_index == std::numeric_limits<unsigned int>::max ()) {

    //  redraw all
    signal_bboxes_changed ();

  } else {

    //  redraw only the layers required for redrawing
    for (std::vector<lay::RedrawLayerInfo>::const_iterator l = mp_canvas->get_redraw_layers ().begin (); l != mp_canvas->get_redraw_layers ().end (); ++l) {
      if (l->cellview_index == int (cv_index) && l->layer_index == int (layer_index)) {
        redraw_layer ((unsigned int) (l - mp_canvas->get_redraw_layers ().begin ()));
      }
    }

    //  forward this event to our observers
    geom_changed_event ();

  }
}

void
LayoutViewBase::signal_bboxes_changed ()
{
  //  schedule a redraw request for all layers
  redraw_later ();

  //  forward this event to our observers
  geom_changed_event ();
}

void
LayoutViewBase::signal_cell_name_changed ()
{
  cell_visibility_changed_event (); // HINT: that is not what actually is intended, but it serves the function ...
  redraw_later ();  //  needs redraw
}

void
LayoutViewBase::signal_layer_properties_changed ()
{
  //  recompute the source 
  //  TODO: this is a side effect of this method - provide a special method for this purpose
  for (unsigned int i = 0; i < layer_lists (); ++i) {
    m_layer_properties_lists [i]->attach_view (this, i);
  }

  //  schedule a redraw request - since the layer views might not have changed, this is necessary
  redraw_later ();
}

void
LayoutViewBase::signal_prop_ids_changed ()
{
  //  inform the layer list observers that they need to recompute the property selectors
  layer_list_changed_event (1);

  //  recompute the source 
  //  TODO: this is a side effect of this method - provide a special method for this purpose
  for (unsigned int i = 0; i < layer_lists (); ++i) {
    m_layer_properties_lists [i]->attach_view (this, i);
  }
}

void
LayoutViewBase::signal_plugin_enabled_changed ()
{
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->editable_interface ()) {
      enable ((*p)->editable_interface (), (*p)->plugin_declaration ()->editable_enabled ());
    }
  }
}

void
LayoutViewBase::signal_annotations_changed ()
{
  //  schedule a redraw request for the annotation shapes
  redraw_deco_layer ();
  //  forward this event to our observers
  annotations_changed_event ();
}

void 
LayoutViewBase::finish_cellviews_changed ()
{
  update_event_handlers ();

  cellviews_changed_event ();

  redraw_later ();
}

std::list<lay::CellView>::iterator
LayoutViewBase::cellview_iter (int cv_index)
{
  std::list<lay::CellView>::iterator i = m_cellviews.begin ();
  while (cv_index > 0 && i != m_cellviews.end ()) {
    ++i;
    --cv_index;
  }
  tl_assert (i != m_cellviews.end ());
  return i;
}

std::list<lay::CellView>::const_iterator
LayoutViewBase::cellview_iter (int cv_index) const
{
  std::list<lay::CellView>::const_iterator i = m_cellviews.begin ();
  while (cv_index > 0 && i != m_cellviews.end ()) {
    ++i;
    --cv_index;
  }
  tl_assert (i != m_cellviews.end ());
  return i;
}

void
LayoutViewBase::erase_cellview (unsigned int index)
{
  if (index >= m_cellviews.size ()) {
    return;
  }

  cancel_esc ();

  //  issue to event that signals a change in the cellviews
  cellviews_about_to_change_event ();

  //  no undo available - clear all transactions
  if (manager ()) {
    manager ()->clear ();
  }

  begin_layer_updates ();

  m_cellviews.erase (cellview_iter (int (index)));

  if (m_hidden_cells.size () > index) {
    m_hidden_cells.erase (m_hidden_cells.begin () + index);
  }

  if (m_current_cell_per_cellview.size () > index) {
    m_current_cell_per_cellview.erase (m_current_cell_per_cellview.begin () + index);
  }

  for (unsigned int lindex = 0; lindex < layer_lists (); ++lindex) {

    //  remove all references to the cellview
    m_layer_properties_lists [lindex]->remove_cv_references (index);

    //  rename the ones that got shifted.
    lay::LayerPropertiesConstIterator l = begin_layers (lindex);
    while (! l.at_end ()) {
      lay::ParsedLayerSource source (l->source (false));
      if (source.cv_index () >= int (index)) {
        lay::LayerProperties new_props (*l);
        source.cv_index (source.cv_index () == int (index) ? -1 : source.cv_index () - 1);
        new_props.set_source (source);
        LayerPropertiesIterator non_const_iter (*m_layer_properties_lists [lindex], l.uint ());
        *non_const_iter = new_props;
      }
      ++l;
    }

  }

  //  clear the history
  m_display_states.clear ();
  m_display_state_ptr = 0;

  end_layer_updates ();

  //  signal to the observers that something has changed
  layer_list_changed_event (3);

  finish_cellviews_changed ();

  update_content ();

  if (m_title.empty ()) {
    emit_title_changed ();
  }
}

void
LayoutViewBase::clear_cellviews ()
{
  //  issue to event that signals a change in the cellviews
  cellviews_about_to_change_event ();

  //  no undo available - clear all transactions
  if (manager ()) {
    manager ()->clear ();
  }

  //  clear the layer lists and cellviews 
  while (layer_lists () > 0) {
    delete_layer_list (layer_lists () - 1);
  }
  set_properties (lay::LayerPropertiesList ());
  m_cellviews.clear ();

  m_hidden_cells.clear ();
  m_current_cell_per_cellview.clear ();

  //  clear the history, store path and zoom box
  m_display_states.clear ();
  m_display_state_ptr = 0;

  finish_cellviews_changed ();

  if (m_title.empty ()) {
    emit_title_changed ();
  }
}

const CellView &
LayoutViewBase::cellview (unsigned int index) const
{
  static const CellView empty;
  if (index >= m_cellviews.size ()) {
    return empty;
  } else {
    return *cellview_iter (int (index));
  }
}

CellViewRef
LayoutViewBase::cellview_ref (unsigned int index)
{
  if (index >= m_cellviews.size ()) {
    return CellViewRef ();
  } else {
    return CellViewRef (cellview_iter (index).operator-> (), this);
  }
}

int
LayoutViewBase::index_of_cellview (const lay::CellView *cv) const
{
  int index = 0;
  for (std::list<CellView>::const_iterator i = m_cellviews.begin (); i != m_cellviews.end (); ++i, ++index) {
    if (cv == i.operator-> ()) {
      return index;
    }
  }
  return -1;
}

void
LayoutViewBase::set_layout (const lay::CellView &cv, unsigned int cvindex)
{
  //  issue to event that signals a change in the cellviews
  cellviews_about_to_change_event ();

  //  no undo available - clear all transactions
  if (manager ()) {
    manager ()->clear ();
  }

  //  signal the change of layer properties to the observer
  layer_list_changed_event (3);

  //  create a new cellview if required
  while (m_cellviews.size () <= cvindex) {
    m_cellviews.push_back (lay::CellView ());
  }

  //  set the handle reference and clear all cell related stuff 
  *cellview_iter (cvindex) = cv;

  //  clear the history, store path and zoom box
  clear_states ();

  finish_cellviews_changed ();

  //  since the hierarchy panel may hold cellviews, we explicitly request an initialization
  //  of the tree. This will release such references. This way, set_layout guarantees that
  //  the layouts are released as far as possible. This is important for reload () for example.
  update_content_for_cv (cvindex);

  if (m_title.empty ()) {
    emit_title_changed ();
  }
}

void
LayoutViewBase::signal_apply_technology (lay::LayoutHandle *layout_handle)
{
  //  find the cellview which issued the event
  for (unsigned int i = 0; i < cellviews (); ++i) {

    if (cellview (i).handle () == layout_handle) {

      cancel_esc ();

      std::string lyp_file;
      const db::Technology *tech = db::Technologies::instance ()->technology_by_name (cellview (i)->tech_name ());
      if (tech && ! tech->eff_layer_properties_file ().empty ()) {
        lyp_file = tech->eff_layer_properties_file ();
      }

      if (! lyp_file.empty ()) {

        //  interpolate the layout properties file name
        tl::Eval expr;
        expr.set_var ("layoutfile", cellview (i)->filename ());
        lyp_file = expr.interpolate (lyp_file);

        //  remove all references to the cellview in the layer properties
        for (unsigned int lindex = 0; lindex < layer_lists (); ++lindex) {
          m_layer_properties_lists [lindex]->remove_cv_references (i);
        }

        //  if a layer properties file is set, create the layer properties now
        create_initial_layer_props (i, lyp_file, tech->add_other_layers ());

      }

      apply_technology_event (int (i));

    }

  }
}

void
LayoutViewBase::bookmarks (const BookmarkList &b)
{
  m_bookmarks = b;
  bookmarks_changed ();
}

void
LayoutViewBase::bookmark_view (const std::string &name)
{
  DisplayState state (box (), get_min_hier_levels (), get_max_hier_levels (), cellview_list ());
  m_bookmarks.add (name, state);
  bookmarks_changed ();
}

bool
LayoutViewBase::is_single_cv_layer_properties_file (const std::string &fn)
{
  //  If the file contains information for a single layout but we have multiple ones,
  //  show the dialog to determine what layout to apply the information to.
  std::vector<lay::LayerPropertiesList> props;
  try {
    tl::XMLFileSource in (fn);
    props.push_back (lay::LayerPropertiesList ());
    props.back ().load (in);
  } catch (...) {
    props.clear ();
    tl::XMLFileSource in (fn);
    lay::LayerPropertiesList::load (in, props);
  }

  //  Collect all cv indices in the layer properties
  std::set <int> cv;
  for (std::vector<lay::LayerPropertiesList>::const_iterator p = props.begin (); p != props.end (); ++p) {
    for (lay::LayerPropertiesConstIterator lp = p->begin_const_recursive (); ! lp.at_end (); ++lp) {
      if (! lp->has_children ()) {
        cv.insert (lp->source (true).cv_index ());
        if (cv.size () >= 2) {
          break;
        }
      }
    }
  }

  return (cv.size () == 1);
}

void 
LayoutViewBase::load_layer_props (const std::string &fn)
{
  do_load_layer_props (fn, false, -1, false);
}

void 
LayoutViewBase::load_layer_props (const std::string &fn, bool add_default)
{
  do_load_layer_props (fn, false, -1, add_default);
}

void 
LayoutViewBase::load_layer_props (const std::string &fn, int cv_index, bool add_default)
{
  do_load_layer_props (fn, true, cv_index, add_default);
}

void 
LayoutViewBase::do_load_layer_props (const std::string &fn, bool map_cv, int cv_index, bool add_default)
{
  std::vector<lay::LayerPropertiesList> props;
  bool single_list = false;

  //  read the layer properties from the file
  try {
    tl::XMLFileSource in (fn);
    props.push_back (lay::LayerPropertiesList ());
    props.back ().load (in);
    single_list = true;
  } catch (...) {
    props.clear ();
    tl::XMLFileSource in (fn);
    lay::LayerPropertiesList::load (in, props);
  }

  //  expand the wildcards and map to the target cv.
  for (std::vector<lay::LayerPropertiesList>::iterator p = props.begin (); p != props.end (); ++p) {
    std::map <int, int> cv_map;
    if (map_cv) {
      cv_map.insert (std::make_pair (-1, cv_index));
    }
    p->attach_view (this, p - props.begin ());
    p->expand (cv_map, add_default);
  }

  transaction (tl::to_string (tr ("Load layer properties")));

  if (single_list) {

    //  a single list will only replace the current tab
    if (map_cv && cv_index >= 0) {
      lay::LayerPropertiesList new_props (get_properties ());
      new_props.remove_cv_references (cv_index);
      new_props.append (props [0]);
      set_properties (new_props);
    } else {
      set_properties (props [0]);
    }

  } else {

    for (unsigned int i = 0; i < props.size (); ++i) {

      if (i < layer_lists ()) {

        if (map_cv && cv_index >= 0) {
          lay::LayerPropertiesList new_props (get_properties (i));
          new_props.remove_cv_references (cv_index);
          new_props.append (props [i]);
          set_properties (i, new_props);
        } else {
          set_properties (i, props [i]);
        }

      } else {
        insert_layer_list (i, props [i]);
      }

    }

    while (layer_lists () > props.size () && layer_lists () > 1) {
      delete_layer_list (layer_lists () - 1);
    }

  }

  commit ();

  update_content ();

  tl::log << "Loaded layer properties from " << fn;
}

void 
LayoutViewBase::save_layer_props (const std::string &fn)
{
  tl::OutputStream os (fn, tl::OutputStream::OM_Plain);

  if (layer_lists () == 1) {

    //  a single list is written in the traditional format
    get_properties ().save (os);

  } else {

    //  multiple tabs are written in the multi-tab format
    std::vector<lay::LayerPropertiesList> props;
    for (unsigned int i = 0; i < layer_lists (); ++i) {
      props.push_back (get_properties (i));
    }

    lay::LayerPropertiesList::save (os, props); 

  }

  tl::log << "Saved layer properties to " << fn;
}

void 
LayoutViewBase::add_new_layers (const std::vector <unsigned int> &layer_ids, int cv_index)
{
  if (cv_index >= 0 && cv_index < int (cellviews ())) {

    const lay::CellView &cv = cellview (cv_index);

    //  create the layers and do a basic recoloring ..
    lay::LayerPropertiesList new_props (get_properties ());

    bool was_empty = new_props.begin_const_recursive ().at_end ();

    //  don't create new layers for those, for which there are layers already: compute a 
    //  set of layers already present
    std::set <db::LayerProperties, db::LPLogicalLessFunc> present_layers;
    for (LayerPropertiesConstIterator lay_iter = begin_layers (); ! lay_iter.at_end (); ++lay_iter) {
      if (! lay_iter->has_children () && lay_iter->cellview_index () == cv_index) {
        present_layers.insert (lay_iter->source (true /*real*/).layer_props ());
      }
    }

    //  determine layers which are new and need to be created
    std::vector <db::LayerProperties> new_layers;
    for (std::vector <unsigned int>::const_iterator l = layer_ids.begin (); l != layer_ids.end (); ++l) {
      const db::LayerProperties &lp = cv->layout ().get_properties (*l);
      if (present_layers.find (lp) == present_layers.end ()) {
        new_layers.push_back (lp);
      }
    }

    //  create them in the sorting order provided by db::LayerProperties
    std::sort (new_layers.begin (), new_layers.end (), db::LPLogicalLessFunc ());

    //  and actually create them
    for (std::vector <db::LayerProperties>::const_iterator l = new_layers.begin (); l != new_layers.end (); ++l) {
      lay::LayerProperties p;
      p.set_source (lay::ParsedLayerSource (*l, cv_index));
      init_layer_properties (p, new_props);
      new_props.push_back (p);
    }

    set_properties (new_props);

    if (was_empty) {
      set_current_layer (new_props.begin_const_recursive ());
    }

  }
}

void 
LayoutViewBase::init_layer_properties (LayerProperties &p) const
{
  init_layer_properties (p, get_properties ());
}

void 
LayoutViewBase::init_layer_properties (LayerProperties &p, const LayerPropertiesList &lp_list) const
{
  tl::color_t c = 0;
  if (m_palette.luminous_colors () > 0) {
    c = m_palette.luminous_color_by_index (p.source (true /*real*/).color_index ());
  }

  p.set_dither_pattern (m_stipple_palette.standard_stipple_by_index (lp_list.end_const () - lp_list.begin_const ()));
  p.set_fill_color (c);
  p.set_frame_color (c);
  p.set_fill_brightness (0);
  p.set_frame_brightness (0);
  p.set_frame_brightness (0);
  p.set_transparent (false);  // :TODO: make variable
  p.set_visible (true);
  p.set_width (1); 
  p.set_animation (0);
  p.set_marked (false);
}

#if defined(HAVE_QT)
QImage 
LayoutViewBase::get_screenshot ()
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save screenshot")));

  refresh ();
  
  return mp_canvas->screenshot ().to_image_copy ();
}
#endif

tl::PixelBuffer
LayoutViewBase::get_screenshot_pb ()
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save screenshot")));

  refresh ();

  return mp_canvas->screenshot ();
}

static std::vector<std::pair<std::string, std::string> >
png_texts (const lay::LayoutViewBase *view, const db::DBox &box)
{
  std::vector<std::pair<std::string, std::string> > texts;

  //  Unfortunately the PNG writer does not allow writing of long strings.
  //  We separate the description into a set of keys:

  for (unsigned int i = 0; i < view->cellviews (); ++i) {
    if (view->cellview (i).is_valid ()) {
      std::string name = view->cellview (i)->layout ().cell_name (view->cellview (i).cell_index ());
      texts.push_back (std::make_pair (std::string ("Cell") + tl::to_string (int (i) + 1), name));
    }
  }

  texts.push_back (std::make_pair (std::string ("Rect"), box.to_string ()));

  return texts;
}

#if defined(HAVE_QT) && !defined(PREFER_LIBPNG_FOR_SAVE)
void
LayoutViewBase::save_screenshot (const std::string &fn)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save screenshot")));

  QImageWriter writer (tl::to_qstring (fn), QByteArray ("PNG"));

  std::vector<std::pair<std::string, std::string> > texts = png_texts (this, box ());
  for (auto i = texts.begin (); i != texts.end (); ++i) {
    writer.setText (tl::to_qstring (i->first), tl::to_qstring (i->second));
  }

  refresh ();

  if (! writer.write (mp_canvas->screenshot ().to_image ())) {
    throw tl::Exception (tl::to_string (tr ("Unable to write screenshot to file: %s (%s)")), fn, tl::to_string (writer.errorString ()));
  }

  tl::log << "Saved screen shot to " << fn;
}
#elif defined(HAVE_PNG)
void
LayoutViewBase::save_screenshot (const std::string &fn)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save screenshot")));

  refresh ();

  tl::OutputStream stream (fn);
  tl::PixelBuffer img = mp_canvas->screenshot ();
  img.set_texts (png_texts (this, box ()));
  img.write_png (stream);

  tl::log << "Saved screen shot to " << fn;
}
#else
void
LayoutViewBase::save_screenshot (const std::string &)
{
  throw tl::Exception (tl::to_string (tr ("Unable to write screenshot - PNG library not compiled in")));
}
#endif

#if defined(HAVE_QT)
QImage
LayoutViewBase::get_image (unsigned int width, unsigned int height)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Get image")));

  refresh ();

  return mp_canvas->image (width, height).to_image_copy ();
}
#endif

tl::PixelBuffer
LayoutViewBase::get_pixels (unsigned int width, unsigned int height)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Get image")));

  refresh ();

  return mp_canvas->image (width, height);
}

#if defined(HAVE_QT)
QImage
LayoutViewBase::get_image_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution,
                                        tl::Color background, tl::Color foreground, tl::Color active, const db::DBox &target_box, bool monochrome)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Get image")));

  refresh ();

  if (monochrome) {
    return mp_canvas->image_with_options_mono (width, height, linewidth, background, foreground, active, target_box).to_image_copy ();
  } else {
    return mp_canvas->image_with_options (width, height, linewidth, oversampling, resolution, background, foreground, active, target_box).to_image_copy ();
  }
}
#endif

tl::PixelBuffer
LayoutViewBase::get_pixels_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution,
                                         tl::Color background, tl::Color foreground, tl::Color active, const db::DBox &target_box)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Get image")));

  refresh ();

  return mp_canvas->image_with_options (width, height, linewidth, oversampling, resolution, background, foreground, active, target_box);
}

tl::BitmapBuffer
LayoutViewBase::get_pixels_with_options_mono (unsigned int width, unsigned int height, int linewidth,
                                              tl::Color background, tl::Color foreground, tl::Color active, const db::DBox &target_box)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Get image")));

  refresh ();

  return mp_canvas->image_with_options_mono (width, height, linewidth, background, foreground, active, target_box);
}

#if defined(HAVE_QT) && !defined(PREFER_LIBPNG_FOR_SAVE)
void
LayoutViewBase::save_image (const std::string &fn, unsigned int width, unsigned int height)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save image")));

  QImageWriter writer (tl::to_qstring (fn), QByteArray ("PNG"));

  lay::Viewport vp (width, height, mp_canvas->viewport ().target_box ());
  std::vector<std::pair<std::string, std::string> > texts = png_texts (this, vp.box ());
  for (auto i = texts.begin (); i != texts.end (); ++i) {
    writer.setText (tl::to_qstring (i->first), tl::to_qstring (i->second));
  }

  refresh ();

  if (! writer.write (mp_canvas->image (width, height).to_image ())) {
    throw tl::Exception (tl::to_string (tr ("Unable to write screenshot to file: %s (%s)")), fn, tl::to_string (writer.errorString ()));
  }

  tl::log << "Saved image to " << fn;
}
#elif defined(HAVE_PNG)
void
LayoutViewBase::save_image (const std::string &fn, unsigned int width, unsigned int height)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save image")));

  lay::Viewport vp (width, height, mp_canvas->viewport ().target_box ());

  refresh ();

  tl::OutputStream stream (fn);
  tl::PixelBuffer img = mp_canvas->image (width, height);
  std::vector<std::pair<std::string, std::string> > texts = png_texts (this, vp.box ());
  img.set_texts (texts);
  img.write_png (stream);

  tl::log << "Saved image to " << fn;
}
#else
void
LayoutViewBase::save_image (const std::string &, unsigned int, unsigned int)
{
  throw tl::Exception (tl::to_string (tr ("Unable to save image - PNG library not compiled in")));
}
#endif

#if defined(HAVE_QT) && !defined(PREFER_LIBPNG_FOR_SAVE)
void
LayoutViewBase::save_image_with_options (const std::string &fn,
                                         unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution,
                                         tl::Color background, tl::Color foreground, tl::Color active, const db::DBox &target_box, bool monochrome)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save image")));

  QImageWriter writer (tl::to_qstring (fn), QByteArray ("PNG"));

  lay::Viewport vp (width, height, mp_canvas->viewport ().target_box ());
  std::vector<std::pair<std::string, std::string> > texts = png_texts (this, vp.box ());
  for (auto i = texts.begin (); i != texts.end (); ++i) {
    writer.setText (tl::to_qstring (i->first), tl::to_qstring (i->second));
  }

  refresh ();

  if (monochrome) {
    if (! writer.write (mp_canvas->image_with_options_mono (width, height, linewidth, background, foreground, active, target_box).to_image ())) {
      throw tl::Exception (tl::to_string (tr ("Unable to write screenshot to file: %s (%s)")), fn, tl::to_string (writer.errorString ()));
    }
  } else {
    if (! writer.write (mp_canvas->image_with_options (width, height, linewidth, oversampling, resolution, background, foreground, active, target_box).to_image ())) {
      throw tl::Exception (tl::to_string (tr ("Unable to write screenshot to file: %s (%s)")), fn, tl::to_string (writer.errorString ()));
    }
  }

  tl::log << "Saved image to " << fn;
}
#elif defined(HAVE_PNG)
void
LayoutViewBase::save_image_with_options (const std::string &fn,
                                         unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution,
                                         tl::Color background, tl::Color foreground, tl::Color active, const db::DBox &target_box, bool monochrome)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Save image")));

  lay::Viewport vp (width, height, mp_canvas->viewport ().target_box ());
  std::vector<std::pair<std::string, std::string> > texts = png_texts (this, vp.box ());

  refresh ();

  tl::OutputStream stream (fn);
  if (monochrome) {

    tl::BitmapBuffer img = mp_canvas->image_with_options_mono (width, height, linewidth, background, foreground, active, target_box);
    img.set_texts (texts);
    img.write_png (stream);

  } else {

    tl::PixelBuffer img = mp_canvas->image_with_options (width, height, linewidth, oversampling, resolution, background, foreground, active, target_box);
    img.set_texts (texts);
    img.write_png (stream);

  }

  tl::log << "Saved image to " << fn;
}
#else
void
LayoutViewBase::save_image_with_options (const std::string &,
                                         unsigned int, unsigned int, int, int, double,
                                         tl::Color, tl::Color, tl::Color, const db::DBox &, bool)
{
  throw tl::Exception (tl::to_string (tr ("Unable to save image - PNG library not compiled in")));
}
#endif

void
LayoutViewBase::reload_layout (unsigned int cv_index)
{
  stop ();
  cancel_esc ();

  //  save the current view state
  lay::DisplayState state;
  save_view (state);

  //  this is the cellview at the given index (use a copy since the original is overwritten)
  CellView cvorg = cellview (cv_index);

  //  obtain the original filename  
  std::string filename = cvorg->filename ();
  std::string technology = cvorg->tech_name ();
  std::string name = cvorg->name ();

  //  recreate hidden cells by doing a name referencing
  std::vector <std::string> hidden_cells;
  if (m_hidden_cells.size () > cv_index) {
    hidden_cells.reserve (m_hidden_cells [cv_index].size ());
    for (std::set <cell_index_type>::const_iterator ci = m_hidden_cells [cv_index].begin (); ci != m_hidden_cells [cv_index].end (); ++ci) {
      hidden_cells.push_back (std::string (cvorg->layout ().cell_name (*ci)));
    }
  }

  //  Set up a list of present layers
  std::set <db::LayerProperties, db::LPLogicalLessFunc> present_layers;
  for (LayerPropertiesConstIterator lay_iter = begin_layers (); ! lay_iter.at_end (); ++lay_iter) {
    if (! lay_iter->has_children ()) {
      present_layers.insert (lay_iter->source (true /*real*/).layer_props ());
    }
  }

  std::map <unsigned int, db::LayerProperties> org_layers;

  for (unsigned int i = 0; i < cvorg->layout ().layers (); ++i) {
    if (cvorg->layout ().is_valid_layer (i)) {
      const db::LayerProperties &p = cvorg->layout ().get_properties (i);
      if (! p.log_equal (db::LayerProperties ())) {
        org_layers.insert (std::make_pair (i, p));
      }
    }
  }

  lay::LayoutHandle *handle;

  //  reset the layout: create a dummy handle and install this in between
  //  this will clear the original layout if not further referenced.
  //  Since the dummy layout will act as a placeholder if something goes wrong
  //  when reading the file, it must have the layers created as well
  lay::CellView cv_empty;

  handle = new lay::LayoutHandle (new db::Layout (is_editable (), manager ()), filename);
  handle->set_tech_name (technology);
  cv_empty.set (handle);

  for (std::map <unsigned int, db::LayerProperties>::const_iterator ol = org_layers.begin (); ol != org_layers.end (); ++ol) {
    cv_empty->layout ().insert_layer (ol->first, ol->second);
  }
  cv_empty->rename (name, true);

  set_layout (cv_empty, cv_index);

  //  create a new handle
  lay::CellView cv;
  handle = new lay::LayoutHandle (new db::Layout (is_editable (), manager ()), filename);
  cv.set (handle);

  try {

    //  re-create the layers required
    for (std::map <unsigned int, db::LayerProperties>::const_iterator ol = org_layers.begin (); ol != org_layers.end (); ++ol) {
      cv->layout ().insert_layer (ol->first, ol->second);
    }
    
    {
      tl::log << tl::to_string (tr ("Loading file: ")) << filename;
      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Loading")));

      //  Load with the previous options again.
      db::LoadLayoutOptions options (cvorg->load_options ());
      cv->load (cvorg->load_options (), technology);
    }

    //  sort the layout explicitly here. Otherwise it would be done
    //  implicitly at some other time. This may throw an exception
    //  if the operation was cancelled.
    {
      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Sorting")));
      cv->layout ().update ();
    }

    //  print the memory statistics now.
    if (tl::verbosity () >= 31) {
      db::MemStatisticsCollector m (false);
      cv->layout ().mem_stat (&m, db::MemStatistics::LayoutInfo, 0);
      m.print ();
    }

    //  this is required to release every reference to the cv_empty layout
    cv_empty = lay::CellView ();

    //  install the new layout
    cv->rename (name, true);
    set_layout (cv, cv_index);

  } catch (...) {
    update_content ();
    throw;
  }

  //  recreate the hidden cell indices from the names
  if (m_hidden_cells.size () > cv_index) {
    m_hidden_cells [cv_index].clear ();
    for (std::vector <std::string>::const_iterator cn = hidden_cells.begin (); cn != hidden_cells.end (); ++cn) {
      std::pair<bool, cell_index_type> cid = cv->layout ().cell_by_name (cn->c_str ());
      if (cid.first) {
        m_hidden_cells [cv_index].insert (cid.second);
      }
    }
  }

  //  clear the current cell (NOTE: this is for providing a cell target for some UI functions only)
  if (m_current_cell_per_cellview.size () > cv_index) {
    m_current_cell_per_cellview [cv_index] = cell_path_type ();
  }

  //  Determine which layers to create as new layers. New layer need to be created
  //  if these have not been present in the original layout and there are no layer views
  //  referring to them.
  std::vector <db::LayerProperties> new_layers;
  for (unsigned int i = 0; i < cv->layout ().layers (); ++i) {
    if (cv->layout ().is_valid_layer (i)) {
      std::map <unsigned int, db::LayerProperties>::iterator ol = org_layers.find (i);
      if (ol == org_layers.end () && present_layers.find (cv->layout ().get_properties (i)) == present_layers.end ()) {
        new_layers.push_back (cv->layout ().get_properties (i));
      }
    }
  }

  std::sort (new_layers.begin (), new_layers.end (), db::LPLogicalLessFunc ());

  //  create the layers and do a basic recoloring ..
  lay::LayerPropertiesList new_props (get_properties ());

  for (std::vector <db::LayerProperties>::const_iterator l = new_layers.begin (); l != new_layers.end (); ++l) {
    lay::LayerProperties p;
    p.set_source (lay::ParsedLayerSource (*l, int (cv_index)));
    init_layer_properties (p, new_props);
    new_props.push_back (p);
  }

  set_properties (new_props);

  goto_view (state);
}

static void
get_lyp_from_meta_info (const db::Layout &layout, std::string &lyp_file, bool &add_other_layers)
{
  db::Layout::meta_info_name_id_type layer_properties_file_name_id = layout.meta_info_name_id ("layer-properties-file");
  db::Layout::meta_info_name_id_type layer_properties_add_other_layers_name_id = layout.meta_info_name_id ("layer-properties-add-other-layers");

  for (db::Layout::meta_info_iterator meta = layout.begin_meta (); meta != layout.end_meta (); ++meta) {
    if (meta->first == layer_properties_file_name_id) {
      lyp_file = meta->second.value.to_string ();
    }
    if (meta->first == layer_properties_add_other_layers_name_id) {
      try {
        add_other_layers = meta->second.value.to_bool ();
      } catch (...) {
      }
    }
  }
}

unsigned int 
LayoutViewBase::add_layout (lay::LayoutHandle *layout_handle, bool add_cellview, bool initialize_layers)
{
  unsigned int cv_index = 0;

  try {

    enable_active_cellview_changed_event (false);

    stop_redraw ();

    bool set_max_hier = (m_full_hier_new_cell || has_max_hier ());

    lay::CellView cv;

    if (! add_cellview) {
      clear_cellviews ();
    }

    cv.set (layout_handle);

    cv->layout ().update ();

    //  select the cell with the largest area as the first top cell
    db::Layout::top_down_const_iterator top = cv->layout ().begin_top_down ();
    for (db::Layout::top_down_const_iterator t = cv->layout ().begin_top_down (); t != cv->layout ().end_top_cells (); ++t) {
      if (cv->layout ().cell (*t).bbox ().area () > cv->layout ().cell (*top).bbox ().area ()) {
        top = t;
      }
    }

    if (top != cv->layout ().end_top_down ()) {
      std::vector <db::cell_index_type> p;
      p.push_back (*top);
      cv.set_unspecific_path (p);
    }

    cv_index = cellviews ();
    set_layout (cv, cv_index);

    if (top != cv->layout ().end_top_cells ()) {
      std::vector <db::cell_index_type> p;
      p.push_back (*top);
      select_cell (p, cv_index);
    }

    //  even if there is no cell, select the cellview item
    //  to support applications with an active cellview (that is however invalid)
    set_active_cellview_index (cv_index);

    if (initialize_layers) {

      bool add_other_layers = m_add_other_layers;

      //  Use the "layer-properties-file" meta info from the handle to get the layer properties file.
      //  If no such file is present, use the default file or the technology specific file.
      std::string lyp_file = m_def_lyp_file;
      const db::Technology *tech = db::Technologies::instance ()->technology_by_name (layout_handle->tech_name ());
      if (tech && ! tech->eff_layer_properties_file ().empty ()) {
        lyp_file = tech->eff_layer_properties_file ();
        add_other_layers = tech->add_other_layers ();
      }

      //  Give the layout object a chance to specify a certain layer property file
      get_lyp_from_meta_info (cv->layout (), lyp_file, add_other_layers);

      //  interpolate the layout properties file name
      tl::Eval expr;
      expr.set_var ("layoutfile", layout_handle->filename ());
      lyp_file = expr.interpolate (lyp_file);

      //  create the initial layer properties
      create_initial_layer_props (cv_index, lyp_file, add_other_layers);

    }

    //  select the first layer if nothing else is selected
    if (cv_index == 0) {
      ensure_layer_selected ();
    }

    //  signal to any observers
    file_open_event ();

    if (cv->layout ().begin_top_down () != cv->layout ().end_top_down ()) {

      //  do a fit and update layer lists etc.
      zoom_fit ();
      if (set_max_hier) {
        max_hier ();
      }
      update_content ();

    } else {
      //  even if there is no cell, select the cellview item
      //  to support applications with an active cellview (that is however invalid)
      set_active_cellview_index (cv_index);
    }

    enable_active_cellview_changed_event (true);

  } catch (...) {

    update_content ();

    enable_active_cellview_changed_event (true, true);
    throw;

  }

  return cv_index;
}

unsigned int 
LayoutViewBase::create_layout (const std::string &technology, bool add_cellview, bool initialize_layers)
{
  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (technology);

  db::Layout *layout = new db::Layout (is_editable (), manager ());
  if (tech) {
    layout->dbu (tech->dbu ());
  }

  lay::LayoutHandle *handle = new lay::LayoutHandle (layout, "");
  handle->set_tech_name (technology);
  return add_layout (handle, add_cellview, initialize_layers);
}

unsigned int 
LayoutViewBase::load_layout (const std::string &filename, const std::string &technology, bool add_cellview)
{
  return load_layout (filename, db::LoadLayoutOptions (), technology, add_cellview);
}

unsigned int 
LayoutViewBase::load_layout (const std::string &filename, const db::LoadLayoutOptions &options, const std::string &technology, bool add_cellview)
{
  stop ();
  
  bool set_max_hier = (m_full_hier_new_cell || has_max_hier ());

  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (technology);

  //  create a new layout handle 
  lay::CellView cv;
  lay::LayoutHandle *handle = new lay::LayoutHandle (new db::Layout (is_editable (), manager ()), filename);
  cv.set (handle);

  unsigned int cv_index;
  db::LayerMap lmap;

  try {

    tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Loading")));

    //  load the file
    {
      tl::log << tl::to_string (tr ("Loading file: ")) << filename << tl::to_string (tr (" with technology: ")) << technology;
      lmap = cv->load (options, technology);
    }

    //  sort the layout explicitly here. Otherwise it would be done
    //  implicitly at some other time. This may throw an exception
    //  if the operation was cancelled.
    {
      cv->layout ().update ();
    }

    //  print the memory statistics now.
    if (tl::verbosity () >= 31) {
      db::MemStatisticsCollector m (false);
      cv->layout ().mem_stat (&m, db::MemStatistics::LayoutInfo, 0);
      m.print ();
    }

    //  clear the cellviews if required
    if (! add_cellview) {
      clear_cellviews ();
    }

    //  set the new layout as the layout for the last cellview
    cv_index = cellviews ();
    set_layout (cv, cv_index);

  } catch (...) {

    update_content ();
    throw;

  }

  try {

    enable_active_cellview_changed_event (false);

    //  select the cell with the largest area as the first top cell
    db::Layout::top_down_const_iterator top = cv->layout ().begin_top_down ();
    for (db::Layout::top_down_const_iterator t = cv->layout ().begin_top_down (); t != cv->layout ().end_top_cells (); ++t) {
      if (cv->layout ().cell (*t).bbox ().area () > cv->layout ().cell (*top).bbox ().area ()) {
        top = t;
      }
    }
    if (top != cv->layout ().end_top_cells ()) {
      std::vector <db::cell_index_type> p;
      p.push_back (*top);
      select_cell (p, cv_index);
    }

    //  force "active_cellview_changed" event
    m_active_cellview_index = -1;

    //  even if there is no cell, select the cellview item
    //  to support applications with an active cellview (that is however invalid)
    set_active_cellview_index (cv_index);

    bool add_other_layers = m_add_other_layers;

    //  Use the "layer-properties-file" meta info from the handle to get the layer properties file.
    //  If no such file is present, use the default file or the technology specific file.
    std::string lyp_file = m_def_lyp_file;
    if (tech && ! tech->eff_layer_properties_file ().empty ()) {
      lyp_file = tech->eff_layer_properties_file ();
      add_other_layers = tech->add_other_layers ();
    }

    //  Give the layout object a chance to specify a certain layer property file
    get_lyp_from_meta_info (cv->layout (), lyp_file, add_other_layers);

    //  interpolate the layout properties file name
    tl::Eval expr;
    expr.set_var ("layoutfile", filename);
    lyp_file = expr.interpolate (lyp_file);

    //  create the initial layer properties
    create_initial_layer_props (cv_index, lyp_file, add_other_layers);

    //  select the first layer if nothing else is selected
    if (cv_index == 0) {
      ensure_layer_selected ();
    }

    //  signal to any observers
    file_open_event ();

    //  do a fit and update layer lists etc.
    zoom_fit ();
    if (set_max_hier) {
      max_hier ();
    }
    update_content ();

    enable_active_cellview_changed_event (true);

  } catch (...) {

    update_content ();

    enable_active_cellview_changed_event (true, true /*silent*/);
    throw;

  }

  return cv_index;
}

void 
LayoutViewBase::create_initial_layer_props (int cv_index, const std::string &lyp_file, bool add_missing)
{
  std::vector<lay::LayerPropertiesList> props;
  bool loaded = false;

  if (! lyp_file.empty ()) {

    //  read the layer properties from the file
    try { 

      try {
        tl::XMLFileSource in (lyp_file);
        props.push_back (lay::LayerPropertiesList ());
        props.back ().load (in);
        loaded = true;
      } catch (...) {
        props.clear ();
        tl::XMLFileSource in (lyp_file);
        tl::log << tl::to_string (tr ("Loading layer properties file: ")) << lyp_file;
        lay::LayerPropertiesList::load (in, props);
        loaded = true;
      }

    } catch (tl::Exception &ex) {
      tl::warn << tl::to_string (tr ("Initialization of layers failed: ")) << ex.msg ();
    } catch (...) {
      tl::warn << tl::to_string (tr ("Initialization of layers failed: unspecific error"));
    }

  }

  std::map <int, int> cv_map;
  cv_map.insert (std::make_pair (-1, cv_index));

  if (! loaded) {

    props.clear ();
    props.push_back (lay::LayerPropertiesList ());

  } else {

    //  do't map cv's if the input file is a multi-cv one.
    std::set <int> cv;
    for (std::vector<lay::LayerPropertiesList>::const_iterator p = props.begin (); p != props.end (); ++p) {
      for (lay::LayerPropertiesConstIterator lp = p->begin_const_recursive (); ! lp.at_end (); ++lp) {
        if (! lp->has_children ()) {
          cv.insert (lp->source (true).cv_index ());
          if (cv.size () >= 2) {
            cv_map.clear ();
            cv_map.insert (std::make_pair (cv_index, cv_index));
            //  erase the others:
            cv_map.insert (std::make_pair (-1, -2));
            break;
          }
        }
      }
    }

  }

  //  expand the wildcards and map to the target cv.
  for (std::vector<lay::LayerPropertiesList>::iterator p = props.begin (); p != props.end (); ++p) {
    p->attach_view (this, p - props.begin ());
    p->expand (cv_map, add_missing || !loaded);
  }

  merge_layer_props (props);
}

void 
LayoutViewBase::merge_layer_props (const std::vector<lay::LayerPropertiesList> &props)
{
  lay::LayerPropertiesList p0;
  if (layer_lists () > 0) {
    p0 = get_properties (0);
  }

  //  merge the new layer views into the present ones
  //  If the specific list is a single list (no tabs), it is merged into every tab present. 
  if (props.size () == 1) {

    for (size_t n = 0; n < layer_lists () || n == 0; ++n) {

      std::vector<lay::LayerPropertiesList>::const_iterator p = props.begin ();

      if (n < layer_lists ()) {
        lay::LayerPropertiesList new_props (get_properties ((unsigned int) n));
        new_props.append (*p);
        if (! p->name ().empty ()) {
          new_props.set_name (p->name ());
        }
        set_properties ((unsigned int) n, new_props);
      } else {

        lay::LayerPropertiesList new_props = p0;
        new_props.append (*p);
        if (! p->name ().empty ()) {
          new_props.set_name (p->name ());
        }
        insert_layer_list ((unsigned int) n, new_props);
      }

    }

  } else {

    size_t n = 0;
    for (std::vector<lay::LayerPropertiesList>::const_iterator p = props.begin (); p != props.end (); ++p, ++n) {

      if (n < layer_lists ()) {
        lay::LayerPropertiesList new_props (get_properties ((unsigned int) n));
        new_props.append (*p);
        if (! p->name ().empty ()) {
          new_props.set_name (p->name ());
        }
        set_properties ((unsigned int) n, new_props);
      } else {
        lay::LayerPropertiesList new_props = p0;
        new_props.append (*p);
        if (! p->name ().empty ()) {
          new_props.set_name (p->name ());
        }
        insert_layer_list ((unsigned int) n, new_props);
      }

    }

  }
}

void
LayoutViewBase::pop_state ()
{
  if (m_display_state_ptr > 0) {
    m_display_states.erase (m_display_states.begin () + m_display_state_ptr, m_display_states.end ());
    --m_display_state_ptr;
  }
}

void
LayoutViewBase::clear_states ()
{
  m_display_states.clear ();
  m_display_state_ptr = 0;
}

void
LayoutViewBase::store_state ()
{
  //  erase all states after the current position
  if (m_display_state_ptr + 1 < m_display_states.size ()) {
    m_display_states.erase (m_display_states.begin () + m_display_state_ptr + 1, m_display_states.end ());
  }

  //  save the state
  DisplayState state (box (), get_min_hier_levels (), get_max_hier_levels (), m_cellviews);
  m_display_states.push_back (state);

  m_display_state_ptr = (unsigned int) (m_display_states.size () - 1);
}

db::DBox 
LayoutViewBase::box () const
{
  return mp_canvas->viewport ().box ();
}

LayoutView *
LayoutViewBase::get_ui ()
{
  return mp_ui;
}

//  NOTE: this methods needs to be called "frequently"
void
LayoutViewBase::timer ()
{
  bool dirty = false;
  for (std::list<lay::CellView>::const_iterator i = m_cellviews.begin (); i != m_cellviews.end () && ! dirty; ++i) {
    dirty = (*i).is_valid () && (*i)->layout ().is_editable () && (*i)->is_dirty ();
  }

  if (dirty != m_dirty) {
    m_dirty = dirty;
    emit_dirty_changed ();
  }

  if (m_prop_changed) {
    do_prop_changed ();
    m_prop_changed = false;
  }

  tl::Clock current_time = tl::Clock::current ();
  if ((current_time - m_last_checked).seconds () > animation_interval) {
    m_last_checked = current_time;
    if (m_animated) {
      set_view_ops ();
      do_set_phase (int (m_phase));
      if (m_animated) {
        ++m_phase;
      }
    }
  }
}

void
LayoutViewBase::refresh ()
{
  //  Execute all deferred methods - ensure there are no pending tasks
  tl::DeferredMethodScheduler::execute ();

  //  Issue a "tick" to execute all other pending tasks
  timer ();

  //  Update the view ops as this is not always guaranteed (issue #1512)
  set_view_ops ();
}

void
LayoutViewBase::force_update_content ()
{
  set_view_ops ();
}

void
LayoutViewBase::update_content ()
{
  if (is_activated ()) {
    set_view_ops ();
  }
}

void
LayoutViewBase::zoom_fit_sel ()
{
  db::DBox bbox = selection_bbox ();
  if (! bbox.empty ()) {
    bbox = db::DBox (bbox.left () - 0.025 * bbox.width (), bbox.bottom () - 0.025 * bbox.height (),
                     bbox.right () + 0.025 * bbox.width (), bbox.top () + 0.025 * bbox.height ());
    zoom_box (bbox);
  }
}

db::DBox
LayoutViewBase::full_box () const
{
  //  compute the bounding box over all layers
  //  this will trigger the update procedures of the layout objects if not done yet ..

  db::DBox bbox;

  for (LayerPropertiesConstIterator l = get_properties ().begin_const_recursive (); ! l.at_end (); ++l) {
    bbox += l->bbox ();
  }

  for (lay::AnnotationShapes::iterator a = annotation_shapes ().begin (); ! a.at_end (); ++a) {
    bbox += a->box ();
  }

  if (bbox.empty ()) {
    bbox = db::DBox (0, 0, 0, 0); // default box
  } else {
    bbox = db::DBox (bbox.left () - 0.025 * bbox.width (), bbox.bottom () - 0.025 * bbox.height (),
                     bbox.right () + 0.025 * bbox.width (), bbox.top () + 0.025 * bbox.height ());
  }

  return bbox;
}

void
LayoutViewBase::zoom_fit ()
{
  mp_canvas->zoom_box (full_box (), true /*precious*/);
  store_state ();
}

void
LayoutViewBase::ensure_selection_visible ()
{
  ensure_visible (selection_bbox ());
}

void
LayoutViewBase::ensure_visible (const db::DBox &bbox)
{
  db::DBox new_box = bbox + viewport ().box ();
  mp_canvas->zoom_box (new_box);
  store_state ();
}

void
LayoutViewBase::zoom_box_and_set_hier_levels (const db::DBox &bbox, const std::pair<int, int> &levels)
{
  mp_canvas->zoom_box (bbox);
  set_hier_levels_basic (levels);
  store_state ();
}

void
LayoutViewBase::zoom_box (const db::DBox &bbox)
{
  mp_canvas->zoom_box (bbox);
  store_state ();
}

void
LayoutViewBase::set_global_trans (const db::DCplxTrans &trans)
{
  mp_canvas->set_global_trans (trans);
  store_state ();
}

void 
LayoutViewBase::zoom_trans (const db::DCplxTrans &trans)
{
  mp_canvas->zoom_trans (trans);
  store_state ();
}

void
LayoutViewBase::pan_left ()
{
  shift_window (1.0, -m_pan_distance, 0.0);
}

void
LayoutViewBase::pan_right ()
{
  shift_window (1.0, m_pan_distance, 0.0);
}

void
LayoutViewBase::pan_up ()
{
  shift_window (1.0, 0.0, m_pan_distance);
}

void
LayoutViewBase::pan_down ()
{
  shift_window (1.0, 0.0, -m_pan_distance);
}

void
LayoutViewBase::pan_left_fast ()
{
  shift_window (1.0, -m_pan_distance * fast_factor, 0.0);
}

void
LayoutViewBase::pan_right_fast ()
{
  shift_window (1.0, m_pan_distance * fast_factor, 0.0);
}

void
LayoutViewBase::pan_up_fast ()
{
  shift_window (1.0, 0.0, m_pan_distance * fast_factor);
}

void
LayoutViewBase::pan_down_fast ()
{
  shift_window (1.0, 0.0, -m_pan_distance * fast_factor);
}

void
LayoutViewBase::pan_center (const db::DPoint &p)
{
  db::DBox b = mp_canvas->viewport ().box ();
  db::DVector d (b.width () * 0.5, b.height () * 0.5);
  zoom_box (db::DBox (p - d, p + d));
}

void
LayoutViewBase::zoom_in ()
{
  zoom_by (zoom_factor);
}

void
LayoutViewBase::zoom_out ()
{
  zoom_by (1.0 / zoom_factor);
}

void
LayoutViewBase::zoom_by (double f)
{
  db::DBox b = mp_canvas->viewport ().box ();

  db::DPoint c = b.center ();
  if (mp_canvas->mouse_in_window ()) {
    c = mp_canvas->mouse_position_um ();
  }

  zoom_box ((b.moved (db::DPoint () - c) * f).moved (c - db::DPoint ()));
}

void
LayoutViewBase::shift_window (double f, double dx, double dy)
{
  db::DBox b = mp_canvas->viewport ().box ();

  db::DPoint s = mp_canvas->viewport ().global_trans ().inverted () * db::DPoint (dx, dy);
  db::DPoint c = b.center () + db::DVector (b.width () * s.x (), b.height () * s.y ());

  double w = b.width () * f;
  double h = b.height () * f;

  db::DVector d (w * 0.5, h * 0.5);

  zoom_box (db::DBox (c - d, c + d));
}

void
LayoutViewBase::goto_window (const db::DPoint &p, double s)
{
  if (s > 1e-6) {
    db::DBox b (p.x () - s * 0.5, p.y () - s * 0.5, p.x () + s * 0.5, p.y () + s * 0.5);
    zoom_box (b);
  } else {
    db::DBox b (box ());
    b.move (p - b.center ());
    zoom_box (b);
  }
}

void 
LayoutViewBase::redraw_layer (unsigned int index)
{
  do_redraw (index);
}

void
LayoutViewBase::redraw_cell_boxes ()
{
  do_redraw (lay::draw_boxes_queue_entry);
}

void
LayoutViewBase::redraw_deco_layer ()
{
  //  redraw background annotations (images etc.)
  mp_canvas->touch_bg ();

  //  redraw other annotations:
  do_redraw (lay::draw_custom_queue_entry);
}

void
LayoutViewBase::redraw_later ()
{
  dm_redraw ();
}

void
LayoutViewBase::redraw ()
{
  std::vector <lay::RedrawLayerInfo> layers;

  size_t nlayers = 0;
  for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
    if (! l->has_children ()) {
      ++nlayers;
    }
  }
  layers.reserve (nlayers);

  for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
    if (! l->has_children ()) {
      layers.push_back (RedrawLayerInfo (*l));
    }
  }

  mp_canvas->redraw_new (layers);
}

void
LayoutViewBase::cancel_edits ()
{
  //  cancel all drag and pending edit operations such as move operations.
  mp_canvas->drag_cancel ();
  lay::Editables::cancel_edits ();
}

void
LayoutViewBase::cancel ()
{
  //  cancel all drags and pending edit operations such as move operations.
  cancel_edits ();
  //  re-enable edit mode
  enable_edits (true);
  //  and clear the selection
  clear_selection ();
}

void
LayoutViewBase::cancel_esc ()
{
  cancel ();
  switch_mode (default_mode ());
}

void
LayoutViewBase::goto_view (const DisplayState &state)
{
  mp_canvas->zoom_box (state.box ());

  std::list <lay::CellView> cellviews;
  for (unsigned int i = 0; i < m_cellviews.size (); ++i) {
    cellviews.push_back (state.cellview (i, cellview_iter (i)->operator-> ()));
  }

  select_cellviews (cellviews);

  if (state.min_hier () <= state.max_hier ()) {
    set_hier_levels_basic (std::make_pair (state.min_hier (), state.max_hier ()));
  }

  update_content ();
}

void
LayoutViewBase::save_view (DisplayState &state) const
{
  state = DisplayState (box (), get_min_hier_levels (), get_max_hier_levels (), m_cellviews);
}

void
LayoutViewBase::do_redraw (int layer)
{
  std::vector<int> layers;
  layers.push_back (layer);

  mp_canvas->redraw_selected (layers);
}

void 
LayoutViewBase::do_prop_changed ()
{
  if (m_visibility_changed) {

    // change visibility and redraw exposed layers
    std::vector<bool> visibility; 
    for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
      if (! l->has_children ()) {
        visibility.push_back (l->visible (true /*real*/));
      }
    }
    mp_canvas->change_visibility (visibility);

    m_visibility_changed = false;

  }

  update_content ();
}

void
LayoutViewBase::set_view_ops ()
{
  bool bright_background = (mp_canvas->background_color ().to_mono ());
  int brightness_for_context = ((bright_background ? m_ctx_dimming : -m_ctx_dimming) * 256) / 100;
  int brightness_for_child_context = ((bright_background ? m_child_ctx_dimming : -m_child_ctx_dimming) * 256) / 100;

  //  count the layers to be able to reserve the number of view_ops
  size_t nlayers = 0;
  for (LayerPropertiesConstIterator lp = get_properties ().begin_const_recursive (); !lp.at_end (); ++lp) {
    if (! lp->has_children ()) {
      ++nlayers;
    }
  }

  std::vector <lay::ViewOp> view_ops;
  view_ops.reserve (nlayers * planes_per_layer + special_planes_before + special_planes_after);

  tl::Color box_color;
  if (! m_box_color.is_valid ()) {
    box_color = mp_canvas->foreground_color ();
  } else {
    box_color = m_box_color;
  }

  //  cell boxes
  if (m_cell_box_visible) {

    lay::ViewOp vop;

    //  context level
    if (m_ctx_color.is_valid ()) {
      vop = lay::ViewOp (m_ctx_color.rgb (), lay::ViewOp::Copy, 0, 0, 0);
    } else {
      vop = lay::ViewOp (lay::LayerProperties::brighter (box_color.rgb (), brightness_for_context), lay::ViewOp::Copy, 0, 0, 0);
    }

    //  fill, frame, text, vertex
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
    view_ops.push_back (vop);
    view_ops.push_back (vop);
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));

    //  child level
    if (m_child_ctx_color.is_valid ()) {
      vop = lay::ViewOp (m_child_ctx_color.rgb (), lay::ViewOp::Copy, 0, 0, 0);
    } else {
      vop = lay::ViewOp (lay::LayerProperties::brighter (box_color.rgb (), brightness_for_context), lay::ViewOp::Copy, 0, 0, 0);
    }

    //  fill, frame, text, vertex
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
    view_ops.push_back (vop);
    view_ops.push_back (vop);
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));

    //  current level
    vop = lay::ViewOp (box_color.rgb (), lay::ViewOp::Copy, 0, 0, 0);

    //  fill, frame, text, vertex
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
    view_ops.push_back (vop);
    view_ops.push_back (vop);
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));

  } else {
    //  invisible
    for (unsigned int i = 0; i < (unsigned int) planes_per_layer; ++i) {  //  frame, fill, vertex, text
      view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
    }
  }

  //  sanity check: number of planes defined in layRedrawThreadWorker must match to view_ops layout
  tl_assert (view_ops.size () == (size_t)cell_box_planes);

  //  produce the ViewOps for the guiding shapes

  tl::color_t gs_color = box_color.rgb ();
  if (m_guiding_shape_color.is_valid ()) {
    gs_color = m_guiding_shape_color.rgb ();
  }

  for (int ctx = 0; ctx < 3; ++ctx) { // 0 (context), 1 (child), 2 (current)

    lay::ViewOp::Mode mode = lay::ViewOp::Copy;

    tl::color_t fill_color, frame_color, text_color;
    int dp = 1; // no stipples for guiding shapes 

    if (ctx == 0) {

      //  context planes
      if (m_ctx_color.is_valid ()) {
        frame_color = text_color = fill_color = m_ctx_color.rgb ();
      } else {
        frame_color = text_color = fill_color = lay::LayerProperties::brighter (gs_color, brightness_for_context);
      }

      if (m_ctx_hollow) {
        dp = 1;
      }

    } else if (ctx == 1) {

      //  child level planes (if used)
      if (m_child_ctx_color.is_valid ()) {
        frame_color = text_color = fill_color = m_child_ctx_color.rgb ();
      } else {
        frame_color = text_color = fill_color = lay::LayerProperties::brighter (gs_color, brightness_for_child_context);
      }

      if (m_child_ctx_hollow) {
        dp = 1;
      }

    } else {

      //  current level planes
      frame_color = text_color = fill_color = gs_color;

    }

    if (m_guiding_shape_visible) {

      //  fill 
      view_ops.push_back (lay::ViewOp (fill_color, mode, 0, dp, 0)); // fill

      //  frame 
      view_ops.push_back (lay::ViewOp (frame_color, mode, 0, 0, 0, lay::ViewOp::Rect, m_guiding_shape_line_width));

      //  text 
      if (m_text_visible) {
        view_ops.push_back (lay::ViewOp (text_color, mode, 0, 0, 0));
      } else {
        view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
      }

      // vertex 
      view_ops.push_back (lay::ViewOp (frame_color, mode, 0, 0, 0, lay::ViewOp::Rect, m_guiding_shape_vertex_size /*mark size*/)); // vertex

    } else {
      view_ops.push_back (lay::ViewOp ());
      view_ops.push_back (lay::ViewOp ());
      view_ops.push_back (lay::ViewOp ());
      view_ops.push_back (lay::ViewOp ());
    }

  }

  //  sanity check: number of planes defined in layRedrawThreadWorker must match to view_ops layout
  tl_assert (view_ops.size () == (size_t)special_planes_before);

  bool animated = false;

  for (int ctx = 0; ctx < 3; ++ctx) { // 0 (context), 1 (child), 2 (current)

    unsigned int ilayer = 0;
    for (LayerPropertiesConstIterator lp = get_properties ().begin_const_recursive (); !lp.at_end (); ++lp, ++ilayer) {

      //  because accessing the LayerPropertiesNode with lp->... is not quite efficient, we get the pointer here:
      const lay::LayerPropertiesNode *l = &*lp;

      if (l->has_children ()) {
        continue;
      }

      bool animate_visible = true;
      unsigned int di_off = m_stipple_offset ? ilayer : 0;

      if (l->animation (true /*real*/)) {

        animated = true;
        if (! m_animated) {
          m_animated = true;
          m_phase = 0;
        }

        if (l->animation (true /*real*/) == 1) {
          // scrolling 
          di_off += m_phase;
        } else if (l->animation (true /*real*/) == 2) {
          // blinking
          animate_visible = ((m_phase & 1) == 0);
        } else {
          // inversely blinking
          animate_visible = ((m_phase & 1) != 0);
        }

      }

      if (l->visible (true /*real*/) && animate_visible) {

        lay::ViewOp::Mode mode = lay::ViewOp::Copy;
        if (l->transparent (true /*real*/)) {
          if (bright_background) {
            mode = lay::ViewOp::And;
          } else {
            mode = lay::ViewOp::Or;
          }
        }

        tl::color_t fill_color, frame_color, text_color;
        int dp = m_no_stipples ? 1 : l->dither_pattern (true /*real*/);
        int ls = l->line_style (true /*real*/);

        if (ctx == 0) {

          //  context planes
          if (m_ctx_color.is_valid ()) {
            frame_color = text_color = fill_color = m_ctx_color.rgb ();
          } else {
            fill_color = l->eff_fill_color_brighter (true /*real*/, brightness_for_context);
            frame_color = l->eff_frame_color_brighter (true /*real*/, brightness_for_context);
            if (m_text_color.is_valid ()) {
              text_color = lay::LayerProperties::brighter (m_text_color.rgb (), brightness_for_context);
            } else {
              text_color = frame_color;
            }
          }

          if (m_ctx_hollow) {
            dp = 1;
          }

        } else if (ctx == 1) {

          //  child level planes (if used)
          if (m_child_ctx_color.is_valid ()) {
            frame_color = text_color = fill_color = m_child_ctx_color.rgb ();
          } else {
            fill_color = l->eff_fill_color_brighter (true /*real*/, brightness_for_child_context);
            frame_color = l->eff_frame_color_brighter (true /*real*/, brightness_for_child_context);
            if (m_text_color.is_valid ()) {
              text_color = lay::LayerProperties::brighter (m_text_color.rgb (), brightness_for_child_context);
            } else {
              text_color = frame_color;
            }
          }

          if (m_child_ctx_hollow) {
            dp = 1;
          }

        } else {

          //  current level planes
          fill_color = l->eff_fill_color (true /*real*/);
          frame_color = l->eff_frame_color (true /*real*/);
          if (m_text_color.is_valid ()) {
            text_color = m_text_color.rgb ();
          } else {
            text_color = frame_color;
          }

        }

        //  fill 
        view_ops.push_back (lay::ViewOp (fill_color, mode, 0, dp, di_off)); // fill

        //  frame 
        int lw = l->width (true /*real*/);
        if (lw < 0) {
          //  default line width is 0 for parents and 1 for leafs
          lw = l->has_children () ? 0 : 1;
        }
        view_ops.push_back (lay::ViewOp (frame_color, mode, ls, 0, 0, lay::ViewOp::Rect, lw));

        //  text 
        if (m_text_visible) {
          view_ops.push_back (lay::ViewOp (text_color, mode, 0, 0, 0));
        } else {
          view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
        }
        // vertex 
        view_ops.push_back (lay::ViewOp (frame_color, mode, 0, 0, 0, lay::ViewOp::Cross, l->marked (true /*real*/) ? 9/*mark size*/ : 0)); // vertex

      } else {
        for (unsigned int i = 0; i < (unsigned int) planes_per_layer / 3; ++i) {
          view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
        }
      }

    }

  }

  if (! animated) {
    m_animated = false;
    m_phase = 0;
  }

  mp_canvas->set_view_ops (view_ops);
}

void
LayoutViewBase::guiding_shapes_visible (bool v)
{
  if (v != m_guiding_shape_visible) {
    m_guiding_shape_visible = v;
    update_content ();
  }
}

void
LayoutViewBase::guiding_shapes_color (tl::Color c)
{
  if (c != m_guiding_shape_color) {
    m_guiding_shape_color = c;
    update_content ();
  }
}

void
LayoutViewBase::guiding_shapes_line_width (int v)
{
  if (v != m_guiding_shape_line_width) {
    m_guiding_shape_line_width = v;
    update_content ();
  }
}

void
LayoutViewBase::guiding_shapes_vertex_size (int v)
{
  if (v != m_guiding_shape_vertex_size) {
    m_guiding_shape_vertex_size = v;
    update_content ();
  }
}

void 
LayoutViewBase::draw_array_border_instances (bool m)
{
  if (m != m_draw_array_border_instances) {
    m_draw_array_border_instances = m;
    redraw ();
  }
}

void 
LayoutViewBase::drop_small_cells (bool m)
{
  if (m != m_drop_small_cells) {
    m_drop_small_cells = m;
    redraw ();
  }
}

void 
LayoutViewBase::drop_small_cells_value (unsigned int s)
{
  if (s != m_drop_small_cells_value) {
    m_drop_small_cells_value = s;
    redraw ();
  }
}

void 
LayoutViewBase::drop_small_cells_cond (drop_small_cells_cond_type t)
{
  if (t != m_drop_small_cells_cond) {
    m_drop_small_cells_cond = t;
    redraw ();
  }
}

void 
LayoutViewBase::cell_box_color (tl::Color c)
{
  if (c != m_box_color) {
    m_box_color = c;
    update_content ();
  }
}

void 
LayoutViewBase::cell_box_text_transform (bool xform)
{
  if (xform != m_box_text_transform) {
    m_box_text_transform = xform;
    redraw ();
  } 
}

void 
LayoutViewBase::cell_box_text_font (unsigned int f)
{
  if (f != m_box_font) {
    m_box_font = f;
    redraw ();
  } 
}

bool
LayoutViewBase::set_hier_levels_basic (std::pair<int, int> l)
{
  if (l != get_hier_levels ()) {

    m_from_level = l.first;
    m_to_level = l.second;

    //  notify all connected observers
    hier_levels_changed_event ();

    redraw ();

    return true;

  } else {
    return false;
  }
}

void 
LayoutViewBase::set_hier_levels (std::pair<int, int> l)
{
  if (set_hier_levels_basic (l)) {
    store_state ();
  } 
}

std::pair<int, int> 
LayoutViewBase::get_hier_levels () const
{
  return std::make_pair (m_from_level, m_to_level);
}

/**
 *  @brief set the maximum hierarchy level to the number of levels available
 */
void 
LayoutViewBase::max_hier ()
{
  //  determine the maximum level of hierarchies
  int max_level = max_hier_level ();

  //  and set the levels
  if (max_level > 0) {
    set_hier_levels (std::make_pair (std::min (m_from_level, max_level), max_level));
  }
}

/**
 *  @brief determine the maximum hierarchy level
 */
int
LayoutViewBase::max_hier_level () const
{
  int max_level = 0;
  for (std::list <CellView>::const_iterator cv = m_cellviews.begin (); cv != m_cellviews.end (); ++cv) {
    if (cv->is_valid ()) {
      int nl = cv->ctx_cell ()->hierarchy_levels () + 1;
      if (nl > max_level) {
        max_level = nl;
      }
    }
  }
  return max_level;
}

/**
 *  @brief Returns a value indicating whether the maximum level is shown
 */
bool 
LayoutViewBase::has_max_hier () const
{
  int ml = max_hier_level ();
  return ml > 0 && m_to_level >= ml;
}

void
LayoutViewBase::set_palette (const lay::ColorPalette &p)
{
  m_palette = p;
}

void
LayoutViewBase::set_palette (const lay::StipplePalette &p)
{
  m_stipple_palette = p;
}

void
LayoutViewBase::set_palette (const lay::LineStylePalette &p)
{
  m_line_style_palette = p;
}

void
LayoutViewBase::ctx_color (tl::Color c)
{
  if (c != m_ctx_color) {
    m_ctx_color = c;
    update_content ();
  }
}

void
LayoutViewBase::ctx_dimming (int d)
{
  if (d != m_ctx_dimming) {
    m_ctx_dimming = d;
    update_content ();
  }
}

void
LayoutViewBase::ctx_hollow (bool h)
{
  if (h != m_ctx_hollow) {
    m_ctx_hollow = h;
    update_content ();
  }
}

void
LayoutViewBase::child_ctx_color (tl::Color c)
{
  if (c != m_child_ctx_color) {
    m_child_ctx_color = c;
    update_content ();
  }
}

void
LayoutViewBase::child_ctx_dimming (int d)
{
  if (d != m_child_ctx_dimming) {
    m_child_ctx_dimming = d;
    update_content ();
  }
}

void
LayoutViewBase::child_ctx_hollow (bool h)
{
  if (h != m_child_ctx_hollow) {
    m_child_ctx_hollow = h;
    update_content ();
  }
}

void
LayoutViewBase::child_ctx_enabled (bool f)
{
  if (f != m_child_ctx_enabled) {
    m_child_ctx_enabled = f;
    update_content ();
    redraw ();
  }
}

void
LayoutViewBase::abstract_mode_width (double w)
{
  if (fabs (w - m_abstract_mode_width) > 1e-6) {
    m_abstract_mode_width = w;
    if (m_abstract_mode_enabled) {
      redraw ();
    }
  }
}

void
LayoutViewBase::abstract_mode_enabled (bool e)
{
  if (e != m_abstract_mode_enabled) {
    m_abstract_mode_enabled = e;
    redraw ();
  }
}

tl::Color
LayoutViewBase::default_background_color ()
{
  return tl::Color (0, 0, 0);  // black.
}

void
LayoutViewBase::do_set_background_color (tl::Color /*color*/, tl::Color /*contrast*/)
{
  //  .. nothing yet ..
}

void 
LayoutViewBase::background_color (tl::Color c)
{
  if (c == mp_canvas->background_color ()) {
    return;
  }

  //  replace by "real" background color if required
  if (! c.is_valid ()) {
    c = default_background_color ();
  }

  tl::Color contrast;
  if (c.to_mono ()) {
    contrast = tl::Color (0, 0, 0);
  } else {
    contrast = tl::Color (255, 255, 255);
  }

  do_set_background_color (c, contrast);

  if (mp_selection_service) {
    mp_selection_service->set_colors (c, contrast);
  }
  if (mp_zoom_service) {
    mp_zoom_service->set_colors (c, contrast);
  }

  //  Set the color for all ViewService interfaces
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    lay::ViewService *svc = (*p)->view_service_interface ();
    if (svc) {
      svc->set_colors (c, contrast);
    }
  }

  mp_canvas->set_colors (c, contrast, mp_canvas->active_color ());

  update_content ();

  background_color_changed_event ();
}

void 
LayoutViewBase::dbu_coordinates (bool f)
{
  m_dbu_coordinates = f;
}

void 
LayoutViewBase::absolute_coordinates (bool f)
{
  m_absolute_coordinates = f;
}

void 
LayoutViewBase::select_cellviews_fit (const std::list <CellView> &cvs)
{
  if (m_cellviews != cvs) {

    for (int index = 0; index < int (m_cellviews.size ()); ++index) {
      cellview_about_to_change_event (index);
    }

    cellviews_about_to_change_event ();

    set_min_hier_levels (0);
    cancel_esc ();
    m_cellviews = cvs;
    zoom_fit ();
    finish_cellviews_changed ();

    for (int index = 0; index < int (m_cellviews.size ()); ++index) {
      cellview_changed (index);
    }

    update_content ();

  } else {
    zoom_fit ();
  }
}

void
LayoutViewBase::cellview_changed (unsigned int index)
{
  update_content_for_cv (index);

  cellview_changed_event (index);

  if (m_title.empty ()) {
    emit_title_changed ();
  }
}

const lay::CellView &
LayoutViewBase::active_cellview () const
{
  return cellview ((unsigned int) active_cellview_index ());
}

lay::CellViewRef
LayoutViewBase::active_cellview_ref ()
{
  return cellview_ref ((unsigned int) active_cellview_index ());
}

int
LayoutViewBase::active_cellview_index () const
{
  return m_active_cellview_index;
}

void
LayoutViewBase::set_active_cellview_index (int index)
{
  if (index >= 0 && index < int (cellviews ())) {
    if (m_active_cellview_index != index) {
      m_active_cellview_index = index;
      active_cellview_changed (index);
    }
  } else {
    m_active_cellview_index = -1;
  }
}

void
LayoutViewBase::selected_cells_paths (int /*cv_index*/, std::vector<cell_path_type> & /*paths*/) const
{
  //  TODO: not implemented yet as there is no setter so far.
  //  (but it is implemented in the UI version where it is bound to the hierarchy control panel)
}

void
LayoutViewBase::current_cell_path (int cv_index, cell_path_type &path) const
{
  if (cv_index >= 0 && cv_index < int (m_current_cell_per_cellview.size ())) {
    path = m_current_cell_per_cellview [cv_index];
  } else {
    path = cell_path_type ();
  }
}

void
LayoutViewBase::set_current_cell_path (int cv_index, const cell_path_type &path)
{
  if (cv_index >= 0) {
    while (cv_index <= int (m_current_cell_per_cellview.size ())) {
      m_current_cell_per_cellview.push_back (cell_path_type ());
    }
    m_current_cell_per_cellview [cv_index] = path;
  }
}

void
LayoutViewBase::do_change_active_cellview ()
{
  //  .. nothing yet ..
}

void
LayoutViewBase::enable_active_cellview_changed_event (bool enable, bool silent)
{
  if (m_active_cellview_changed_event_enabled == enable) {
    return;
  }

  m_active_cellview_changed_event_enabled = enable;
  if (enable) {

    if (!silent && ! m_active_cellview_changed_events.empty ()) {

      //  deliver stored events

      //  we need to cancel pending drawing or dragging operations to reflect the new cellview (different target, may have different technology etc.)
      cancel_esc ();

      //  we need to setup the editor option pages because the technology may have changed
      do_change_active_cellview ();

      active_cellview_changed_event ();
      for (std::set<int>::const_iterator i = m_active_cellview_changed_events.begin (); i != m_active_cellview_changed_events.end (); ++i) {
        active_cellview_changed_with_index_event (*i);
      }

      //  Because the title reflects the active one, emit a title changed event
      if (title_string ().empty ()) {
        emit_title_changed ();
      }

    }

  }

  m_active_cellview_changed_events.clear ();
}

void
LayoutViewBase::active_cellview_changed (int index)
{
  if (m_active_cellview_changed_event_enabled) {

    //  we need to cancel pending drawing or dragging operations to reflect the new cellview (different target, may have different technology etc.)
    cancel_esc ();

    //  we need to setup the editor option pages because the technology may have changed
    do_change_active_cellview ();

    active_cellview_changed_event ();
    active_cellview_changed_with_index_event (index);

    //  Because the title reflects the active one, emit a title changed event
    if (title_string ().empty ()) {
      emit_title_changed ();
    }

  } else {
    m_active_cellview_changed_events.insert (index);
  }
}

void
LayoutViewBase::select_cell_dispatch (const cell_path_type &path, int cellview_index)
{
  bool set_max_hier = (m_full_hier_new_cell || has_max_hier ());
  if (m_clear_ruler_new_cell) {

    //  This is a HACK, but the clean solution would be to provide a new editable 
    //  method like "clear_annotations":
    lay::Plugin *antPlugin = get_plugin_by_name ("ant::Plugin");
    if (antPlugin) {
      antPlugin->menu_activated ("ant::clear_all_rulers_internal");
    }

  }

  if (m_fit_new_cell) {
    select_cell_fit (path, cellview_index);
  } else {
    select_cell (path, cellview_index);
  }
  set_current_cell_path (cellview_index, path);
  if (set_max_hier) {
    max_hier ();
  }
}

void 
LayoutViewBase::select_cell_fit (const cell_path_type &path, int index)
{ 
  if (index >= 0 && int (m_cellviews.size ()) > index && (cellview_iter (index)->specific_path ().size () > 0 || cellview_iter (index)->unspecific_path () != path)) {

    cellview_about_to_change_event (index);

    set_min_hier_levels (0);
    cancel (); 
    cellview_iter (index)->set_specific_path (lay::CellView::specific_cell_path_type ());
    cellview_iter (index)->set_unspecific_path (path);
    set_active_cellview_index (index);
    redraw ();
    zoom_fit ();

    cellview_changed (index);

    update_content ();

  }
}

void 
LayoutViewBase::select_cell_fit (cell_index_type cell_index, int index)
{ 
  if (index >= 0 && int (m_cellviews.size ()) > index && cellview_iter (index)->cell_index () != cell_index) {

    cellview_about_to_change_event (index);

    set_min_hier_levels (0);
    cancel (); 
    cellview_iter (index)->set_cell (cell_index);
    set_active_cellview_index (index);
    redraw ();
    zoom_fit ();

    cellview_changed (index);

    update_content ();

  }
}

void 
LayoutViewBase::select_cellviews (const std::list <CellView> &cvs)
{
  if (m_cellviews != cvs) {

    for (int index = 0; index < int (m_cellviews.size ()); ++index) {
      cellview_about_to_change_event (index);
    }
    cellviews_about_to_change_event ();

    set_min_hier_levels (0);
    cancel_esc ();
    m_cellviews = cvs;
    redraw ();

    cellviews_changed_event ();
    for (int index = 0; index < int (m_cellviews.size ()); ++index) {
      cellview_changed (index);
    }

    update_content ();

  }
}

void
LayoutViewBase::select_cellview (int index, const CellView &cv)
{
  if (index < 0 || index >= int (m_cellviews.size ())) {
    return;
  }

  if (*cellview_iter (index) != cv) {

    cellview_about_to_change_event (index);

    cancel_esc ();
    *cellview_iter (index) = cv;
    redraw ();

    cellview_changed (index);

    update_content ();

  }
}

void
LayoutViewBase::select_cell (const cell_path_type &path, int index)
{ 
  if (index >= 0 && int (m_cellviews.size ()) > index && (cellview_iter (index)->specific_path ().size () > 0 || cellview_iter (index)->unspecific_path () != path)) {

    cellview_about_to_change_event (index);

    set_min_hier_levels (0);
    cancel (); 
    cellview_iter (index)->set_specific_path (lay::CellView::specific_cell_path_type ());
    cellview_iter (index)->set_unspecific_path (path);
    set_active_cellview_index (index);
    redraw ();

    cellview_changed (index);

    update_content ();

  }
}

void 
LayoutViewBase::select_cell (cell_index_type cell_index, int index)
{ 
  if (index >= 0 && int (m_cellviews.size ()) > index && (! cellview_iter (index)->is_valid () || cellview_iter (index)->cell_index () != cell_index)) {

    cellview_about_to_change_event (index);

    set_min_hier_levels (0);
    cancel (); 
    cellview_iter (index)->set_cell (cell_index);
    set_active_cellview_index (index);
    redraw ();

    cellview_changed (index);

    update_content ();

  }
}

bool
LayoutViewBase::is_cell_hidden (cell_index_type ci, int cellview_index) const
{
  if (int (m_hidden_cells.size ()) > cellview_index && cellview_index >= 0) {
    return m_hidden_cells [cellview_index].find (ci) != m_hidden_cells [cellview_index].end ();
  } else {
    return false;
  }
}

const std::set<LayoutViewBase::cell_index_type> &
LayoutViewBase::hidden_cells (int cellview_index) const
{
  if (int (m_hidden_cells.size ()) > cellview_index && cellview_index >= 0) {
    return m_hidden_cells[cellview_index];
  } else {
    static std::set<cell_index_type> empty_set;
    return empty_set;
  }
}

void 
LayoutViewBase::hide_cell (cell_index_type ci, int cellview_index)
{
  if (cellview_index < 0) {
    return;
  }
  while (int (m_hidden_cells.size ()) <= cellview_index) {
    m_hidden_cells.push_back (std::set <cell_index_type> ());
  }
  if (m_hidden_cells [cellview_index].insert (ci).second) {
    if (transacting ()) {
      manager ()->queue (this, new OpHideShowCell (ci, cellview_index, false /*=hide*/));
    } else if (manager () && ! replaying ()) {
      manager ()->clear ();
    }
    cell_visibility_changed_event ();
    redraw ();  //  needs redraw
  }
}

void 
LayoutViewBase::show_cell (cell_index_type ci, int cellview_index)
{
  if (cellview_index < 0) {
    return;
  }
  if (int (m_hidden_cells.size ()) > cellview_index) {
    if (m_hidden_cells [cellview_index].erase (ci) > 0) {
      if (transacting ()) {
        manager ()->queue (this, new OpHideShowCell (ci, cellview_index, true /*=show*/));
      } else if (manager () && ! replaying ()) {
        manager ()->clear ();
      }
      cell_visibility_changed_event ();
      redraw ();  //  needs redraw
    }
  }
}

void
LayoutViewBase::show_all_cells (int cv_index)
{
  if (cv_index < 0 || cv_index >= int (m_hidden_cells.size ())) {
    return;
  }

  if (! m_hidden_cells [cv_index].empty ()) {
    if (transacting ()) {
      for (std::set<cell_index_type>::const_iterator ci = m_hidden_cells [cv_index].begin (); ci != m_hidden_cells [cv_index].end (); ++ci) {
        manager ()->queue (this, new OpHideShowCell (*ci, cv_index, true /*=show*/));
      }
    } else if (manager () && ! replaying ()) {
      manager ()->clear ();
    }
    m_hidden_cells [cv_index].clear ();
    cell_visibility_changed_event ();
    redraw ();  //  needs redraw
  }
}

void
LayoutViewBase::show_all_cells ()
{
  bool any = false;

  for (unsigned int i = 0; i < m_hidden_cells.size (); ++i) {
    if (! m_hidden_cells [i].empty ()) {
      if (transacting ()) {
        for (std::set<cell_index_type>::const_iterator ci = m_hidden_cells [i].begin (); ci != m_hidden_cells [i].end (); ++ci) {
          manager ()->queue (this, new OpHideShowCell (*ci, i, true /*=show*/));
        }
      } else if (manager () && ! replaying ()) {
        manager ()->clear ();
      }
      m_hidden_cells [i].clear ();
      any = true;
    }
  }

  if (any) {
    cell_visibility_changed_event ();
    redraw ();  //  needs redraw
    return;
  }
}

void 
LayoutViewBase::min_inst_label_size (int px)
{
  if (m_min_size_for_label != px) {
    m_min_size_for_label = px;
    redraw ();
  }
}

void 
LayoutViewBase::text_visible (bool vis)
{
  if (m_text_visible != vis) {
    m_text_visible = vis;
    update_content ();
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutViewBase::show_properties_as_text (bool sp)
{
  if (m_show_properties != sp) {
    m_show_properties = sp;
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutViewBase::bitmap_caching (bool l)
{
  if (m_bitmap_caching != l) {
    m_bitmap_caching = l;
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutViewBase::text_lazy_rendering (bool l)
{
  if (m_text_lazy_rendering != l) {
    m_text_lazy_rendering = l;
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutViewBase::cell_box_visible (bool vis)
{
  if (m_cell_box_visible != vis) {
    m_cell_box_visible = vis;
    update_content ();
  }
}

void 
LayoutViewBase::text_font (unsigned int f)
{
  if (m_text_font != f) {
    m_text_font = f;
    redraw ();
  }
}

void 
LayoutViewBase::default_text_size (double fs)
{
  if (m_default_text_size != fs) {
    m_default_text_size = fs;
    redraw ();
  }
}

void
LayoutViewBase::text_point_mode (bool pm)
{
  if (m_text_point_mode != pm) {
    m_text_point_mode = pm;
    redraw ();
  }
}

void
LayoutViewBase::clear_ruler_new_cell (bool f)
{
  m_clear_ruler_new_cell = f;
}
  
void 
LayoutViewBase::full_hier_new_cell (bool f)
{
  m_full_hier_new_cell = f;
}

double
LayoutViewBase::pan_distance () const
{
  return m_pan_distance;
}

void
LayoutViewBase::pan_distance (double pd)
{
  m_pan_distance = pd;
}

void 
LayoutViewBase::fit_new_cell (bool f)
{
  m_fit_new_cell = f;
}
  
void 
LayoutViewBase::apply_text_trans (bool f)
{
  if (m_apply_text_trans != f) {
    m_apply_text_trans = f;
    redraw ();
  }
}
  
void 
LayoutViewBase::offset_stipples (bool f)
{
  if (m_stipple_offset != f) {
    m_stipple_offset = f;
    update_content ();
  }
}
  
void 
LayoutViewBase::no_stipples (bool f)
{
  if (m_no_stipples != f) {
    m_no_stipples = f;
    do_set_no_stipples (f);
    update_content ();
  }
}
  
void
LayoutViewBase::show_markers (bool f)
{
  if (m_show_markers != f) {
    m_show_markers = f;
    mp_canvas->update_image ();
  }
}

void
LayoutViewBase::text_color (tl::Color c)
{
  if (m_text_color != c) {
    m_text_color = c;
    update_content ();
  }
}

bool
LayoutViewBase::has_selection ()
{
  return lay::Editables::has_selection ();
}

void
LayoutViewBase::do_paste ()
{
  //  .. nothing yet ..
}

void
LayoutViewBase::paste ()
{
  clear_selection ();

  {
    db::Transaction trans (manager (), tl::to_string (tr ("Paste")));

    //  let the receivers sort out who is pasting what ..
    do_paste ();
    lay::Editables::paste ();
  }

  //  if we change the state, save it before
  store_state ();

  db::DBox sel_bbox = selection_bbox ();
  if (! sel_bbox.empty ()) {
    if (m_paste_display_mode == 1) {
      // just make selection visible, i.e. shift window somewhat
      pan_center (sel_bbox.center ());
    } else if (m_paste_display_mode == 2) {
      // or: make selection fit into the screen
      zoom_fit_sel ();
    }
  }
}

void
LayoutViewBase::paste_interactive (bool transient_mode)
{
  clear_selection ();

  std::unique_ptr<db::Transaction> trans (new db::Transaction (manager (), tl::to_string (tr ("Paste and move"))));

  lay::Editables::paste ();

  //  temporarily close the transaction and pass to the move service for appending it's own
  //  operations.
  trans->close ();

  if (mp_move_service->begin_move (trans.release (), transient_mode)) {
    switch_mode (-1);  //  move mode
  }
}

void
LayoutViewBase::copy ()
{
  copy_view_objects ();
}

void
LayoutViewBase::copy_view_objects ()
{
  cancel_edits ();
  if (! lay::Editables::has_selection ()) {
    //  try to use the transient selection for the real one
    lay::Editables::transient_to_selection ();
  }

  lay::Editables::copy ();
}

void
LayoutViewBase::cut ()
{
  cancel_edits ();
  if (! lay::Editables::has_selection ()) {
    //  try to use the transient selection for the real one
    lay::Editables::transient_to_selection ();
  }

  db::Transaction trans (manager (), tl::to_string (tr ("Cut")));
  lay::Editables::cut ();
}

void
LayoutViewBase::remove_unused_layers ()
{
  bool any_deleted;
  do {

    std::vector <lay::LayerPropertiesConstIterator> sel;

    lay::LayerPropertiesConstIterator l = begin_layers ();
    while (! l.at_end ()) {
      if (! l->has_children () && l->bbox ().empty ()) {
        sel.push_back (l);
      }
      ++l;
    }

    std::sort (sel.begin (), sel.end (), CompareLayerIteratorBottomUp ());
    any_deleted = false;
    for (std::vector<lay::LayerPropertiesConstIterator>::iterator s = sel.begin (); s != sel.end (); ++s) {
      delete_layer (*s);
      any_deleted = true;
    }

  } while (any_deleted);

  emit_layer_order_changed ();
}

void 
LayoutViewBase::add_missing_layers ()
{
  std::set <lay::ParsedLayerSource> present;
  LayerPropertiesConstIterator l = begin_layers ();
  while (! l.at_end ()) {
    if (! l->has_children ()) {
      present.insert (l->source (true /*real*/));
    }
    ++l;
  }

  std::vector <lay::ParsedLayerSource> actual;
  for (unsigned int cv = 0; cv < cellviews (); ++cv) {
    const db::Layout &layout = cellview (cv)->layout ();
    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (layout.is_valid_layer (l)) {
        actual.push_back (lay::ParsedLayerSource (layout.get_properties (l), cv));
      }
    }
  }

  std::sort (actual.begin (), actual.end ());

  for (std::vector <lay::ParsedLayerSource>::const_iterator a = actual.begin (); a != actual.end (); ++a) {
    if (present.find (*a) == present.end ()) {
      lay::LayerPropertiesNode node;
      node.attach_view (this, current_layer_list ());
      node.set_source (*a);
      init_layer_properties (node);
      insert_layer (end_layers (), node);
    }
  }

  emit_layer_order_changed ();
}

LayerState 
LayoutViewBase::layer_snapshot () const
{
  LayerState state;
  LayerPropertiesConstIterator l = begin_layers ();
  while (! l.at_end ()) {
    if (! l->has_children ()) {
      state.present.insert (l->source (true /*real*/));
    }
    ++l;
  }
  return state;
}

void
LayoutViewBase::current_layer_changed_slot (const lay::LayerPropertiesConstIterator &iter)
{
  current_layer_changed_event (iter);
}

void 
LayoutViewBase::add_new_layers (const LayerState &state)
{
  std::vector <lay::ParsedLayerSource> actual;
  for (unsigned int cv = 0; cv < cellviews (); ++cv) {
    const db::Layout &layout = cellview (cv)->layout ();
    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (layout.is_valid_layer (l)) {
        actual.push_back (lay::ParsedLayerSource (layout.get_properties (l), cv));
      }
    }
  }

  std::sort (actual.begin (), actual.end ());

  bool needs_update = false;

  for (std::vector <lay::ParsedLayerSource>::const_iterator a = actual.begin (); a != actual.end (); ++a) {
    if (state.present.find (*a) == state.present.end ()) {
      needs_update = true;
      lay::LayerPropertiesNode node;
      node.attach_view (this, current_layer_list ());
      node.set_source (*a);
      //  HINT: in editable mode it is desireable to present all layers because otherwise they cannot be
      //  made visible to populate them.
      if (is_editable () || ! node.bbox ().empty ()) {
        init_layer_properties (node);
        insert_layer (end_layers (), node);
      }
    }
  }

  if (needs_update) {
    emit_layer_order_changed ();
  }
}

void 
LayoutViewBase::prev_display_state ()
{
  if (m_display_state_ptr > 0) {
    m_display_state_ptr--;
    goto_view (m_display_states [m_display_state_ptr]);
  }
}

bool 
LayoutViewBase::has_prev_display_state ()
{
  return m_display_state_ptr > 0;
}

void 
LayoutViewBase::next_display_state ()
{
  if (m_display_state_ptr + 1 < m_display_states.size ()) {
    m_display_state_ptr++;
    goto_view (m_display_states [m_display_state_ptr]);
  }
}

bool 
LayoutViewBase::has_next_display_state ()
{
  return m_display_state_ptr + 1 < m_display_states.size ();
}

void
LayoutViewBase::current_pos (double /*x*/, double /*y*/)
{
  //  .. nothing yet ..
}

void
LayoutViewBase::stop_redraw ()
{
  dm_redraw.cancel ();
  mp_canvas->stop_redraw ();
}

void
LayoutViewBase::free_resources ()
{
  mp_canvas->free_resources ();
}

void
LayoutViewBase::stop ()
{
  stop_redraw ();
  deactivate_all_browsers ();
}

void
LayoutViewBase::begin_layer_updates ()
{
  //  .. nothing yet ..
}

void
LayoutViewBase::end_layer_updates ()
{
  //  .. nothing yet ..
}

void
LayoutViewBase::ensure_layer_selected ()
{
  if (current_layer () == lay::LayerPropertiesConstIterator ()) {
    const lay::LayerPropertiesList &lp = get_properties ();
    lay::LayerPropertiesConstIterator li = lp.begin_const_recursive ();
    while (! li.at_end () && li->has_children ()) {
      ++li;
    }
    if (! li.at_end ()) {
      set_current_layer (li);
    }
  }
}

void
LayoutViewBase::do_set_no_stipples (bool /*no_stipples*/)
{
  //  .. nothing yet ..
}

void
LayoutViewBase::do_set_phase (int /*phase*/)
{
  //  .. nothing yet ..
}

void
LayoutViewBase::deactivate_all_browsers ()
{
  //  .. nothing yet ..
}

bool
LayoutViewBase::is_activated () const
{
  return true;
}

void
LayoutViewBase::switch_mode (int m)
{
  mode (m);
}

void 
LayoutViewBase::mode (int m)
{
  if (m != m_mode) {

    m_mode = m;
    mp_active_plugin = 0;

    if (m > 0) {

      for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
        if ((*p)->plugin_declaration ()->id () == m) {
          mp_active_plugin = *p;
          mp_canvas->activate ((*p)->view_service_interface ());
          break;
        }
      }

    } else if (m == 0 && mp_selection_service) {
      mp_canvas->activate (mp_selection_service);
    } else if (m == -1 && mp_move_service) {
      mp_canvas->activate (mp_move_service);
    }

  }
}

bool 
LayoutViewBase::is_move_mode () const
{
  return m_mode == -1;
}

bool 
LayoutViewBase::is_selection_mode () const
{
  return m_mode == 0;
}
  
unsigned int 
LayoutViewBase::intrinsic_mouse_modes (std::vector<std::string> *descriptions)
{
  if (descriptions) {
    descriptions->push_back ("select\t" + tl::to_string (tr ("Select")) + "<:select_24px.png>");
    descriptions->push_back ("move\t" + tl::to_string (tr ("Move")) + "<:move_24px.png>");
  }
  return 2;
}

int
LayoutViewBase::default_mode ()
{
  return 0; // TODO: any generic scheme? is select, should be ruler..
}

static std::string
name_from_title (const std::string &title)
{
  std::string s = title;
  std::string::size_type tab = s.find ('\t');
  if (tab != std::string::npos) {
    s = std::string (s, 0, tab);
  }
  std::string::size_type colon = s.find (':');
  if (colon != std::string::npos) {
    s = std::string (s, 0, colon);
  }
  return s;
}

static bool
edit_mode_from_title (const std::string &title)
{
  std::string s = title;
  std::string::size_type tab = s.find ('\t');
  if (tab != std::string::npos) {
    s = std::string (s, 0, tab);
  }
  std::vector<std::string> parts = tl::split (s, ":");
  for (auto i = parts.begin (); i != parts.end (); ++i) {
    if (*i == "edit_mode") {
      return true;
    }
  }
  return false;
}

std::vector<std::string>
LayoutViewBase::mode_names () const
{
  std::vector<std::string> names;

  std::vector<std::string> intrinsic_modes;
  intrinsic_mouse_modes (&intrinsic_modes);
  for (auto i = intrinsic_modes.begin (); i != intrinsic_modes.end (); ++i) {
    names.push_back (name_from_title (*i));
  }

  for (auto i = mp_plugins.begin (); i != mp_plugins.end (); ++i) {
    std::string title;
    if ((*i) && (*i)->plugin_declaration () && (*i)->plugin_declaration ()->implements_mouse_mode (title)) {
      if (is_editable () || !edit_mode_from_title (title)) {
        names.push_back (name_from_title (title));
      }
    }
  }

  return names;
}

std::string
LayoutViewBase::mode_name () const
{
  if (m_mode <= 0) {

    std::vector<std::string> intrinsic_modes;
    intrinsic_mouse_modes (&intrinsic_modes);

    if (int (intrinsic_modes.size ()) > -m_mode) {
      return name_from_title (intrinsic_modes [-m_mode]);
    }

  } else {

    for (auto i = mp_plugins.begin (); i != mp_plugins.end (); ++i) {
      std::string title;
      if ((*i) && (*i)->plugin_declaration () && (*i)->plugin_declaration ()->id () == m_mode && (*i)->plugin_declaration ()->implements_mouse_mode (title)) {
        return name_from_title (title);
      }
    }

  }

  return std::string ();
}

void
LayoutViewBase::switch_mode (const std::string &name)
{
  std::vector<std::string> intrinsic_modes;
  intrinsic_mouse_modes (&intrinsic_modes);
  for (auto i = intrinsic_modes.begin (); i != intrinsic_modes.end (); ++i) {
    if (name_from_title (*i) == name) {
      switch_mode (int (0 - int (i - intrinsic_modes.begin ())));
      return;
    }
  }

  for (auto i = mp_plugins.begin (); i != mp_plugins.end (); ++i) {
    std::string title;
    if ((*i) && (*i)->plugin_declaration () && (*i)->plugin_declaration ()->implements_mouse_mode (title)) {
      if (name_from_title (title) == name) {
        switch_mode ((*i)->plugin_declaration ()->id ());
        return;
      }
    }
  }
}

std::vector<std::string>
LayoutViewBase::menu_symbols ()
{
  //  TODO: currently these are all symbols from all plugins
  return lay::PluginDeclaration::menu_symbols ();
}

void
LayoutViewBase::menu_activated (const std::string &symbol)
{
  //  Try the plugin declarations if the view is the top-level dispatcher
  if (dispatcher () == this) {
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      if (cls->menu_activated (symbol)) {
        return;
      }
    }
  }

  //  distribute the menu item call on the plugins - one should take it.
  for (std::vector<lay::Plugin *>::const_iterator p = plugins ().begin (); p != plugins ().end (); ++p) {
    (*p)->menu_activated (symbol);
  }
}

void
LayoutViewBase::update_content_for_cv (int /*cellview_index*/)
{
  //  .. nothing yet ..
}

void
LayoutViewBase::rename_cellview (const std::string &name, int cellview_index)
{
  if (cellview_index >= 0 && cellview_index < int (m_cellviews.size ())) {
    if ((*cellview_iter (cellview_index))->name () != name) {
      (*cellview_iter (cellview_index))->rename (name);
      update_content_for_cv (cellview_index);
      if (m_title.empty ()) {
        emit_title_changed ();
      }
    }
  }
}

std::vector<db::DCplxTrans>
LayoutViewBase::cv_transform_variants (int cv_index) const
{
  std::set<db::DCplxTrans> trns_variants;
  for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
    if (! l->has_children ()) {
      int cvi = l->cellview_index () >= 0 ? l->cellview_index () : 0;
      if (cv_index < int (cellviews ()) && cvi == cv_index) {
        trns_variants.insert (l->trans ().begin (), l->trans ().end ());
      }
    }
  }
  return std::vector<db::DCplxTrans> (trns_variants.begin (), trns_variants.end ());
}

std::vector<db::DCplxTrans>
LayoutViewBase::cv_transform_variants (int cv_index, unsigned int layer) const
{
  if (cellview (cv_index)->layout ().is_valid_layer (layer)) {
    std::set<db::DCplxTrans> trns_variants;
    for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
      if (! l->has_children () && l->layer_index () == int (layer)) {
        int cvi = l->cellview_index () >= 0 ? l->cellview_index () : 0;
        if (cv_index < int (cellviews ()) && cvi == cv_index) {
          trns_variants.insert (l->trans ().begin (), l->trans ().end ());
        }
      }
    }
    return std::vector<db::DCplxTrans> (trns_variants.begin (), trns_variants.end ());
  } else {
    //  may happen if the layer is a guiding shape layer for example
    return cv_transform_variants (cv_index);
  }
}

std::map<unsigned int, std::vector<db::DCplxTrans> >
LayoutViewBase::cv_transform_variants_by_layer (int cv_index) const
{
  std::map<unsigned int, std::vector<db::DCplxTrans> > tv_map;

  for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
    if (! l->has_children () && l->layer_index () >= 0) {
      int cvi = l->cellview_index () >= 0 ? l->cellview_index () : 0;
      if (cv_index < int (cellviews ()) && cvi == cv_index) {
        std::vector<db::DCplxTrans> &v = tv_map.insert (std::make_pair (l->layer_index (), std::vector<db::DCplxTrans> ())).first->second;
        v.insert (v.end (), l->trans ().begin (), l->trans ().end ());
      }
    }
  }

  for (std::map<unsigned int, std::vector<db::DCplxTrans> >::iterator m = tv_map.begin (); m != tv_map.end (); ++m) {
    std::sort (m->second.begin (), m->second.end ());
    m->second.erase (std::unique (m->second.begin (), m->second.end ()), m->second.end ());
  }

  return tv_map;
}

std::set< std::pair<db::DCplxTrans, int> >
LayoutViewBase::cv_transform_variants () const
{
  std::set< std::pair<db::DCplxTrans, int> > box_variants;
  for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
    if (! l->has_children ()) {
      unsigned int cv_index = l->cellview_index () >= 0 ? (unsigned int) l->cellview_index () : 0;
      if (cv_index < cellviews ()) {
        for (std::vector<db::DCplxTrans>::const_iterator t = l->trans ().begin (); t != l->trans ().end (); ++t) {
          box_variants.insert (std::make_pair (*t, cv_index));
        }
      }
    }
  }
  return box_variants;
}

db::InstElement 
LayoutViewBase::ascend (int index)
{
  tl_assert (int (m_cellviews.size ()) > index && cellview_iter (index)->is_valid ());

  cellview_about_to_change_event (index);

  lay::CellView::specific_cell_path_type spath (cellview_iter (index)->specific_path ());
  if (spath.empty ()) {
    return db::InstElement ();
  } else {

    cancel (); 
    db::InstElement ret = spath.back ();
    spath.pop_back ();
    cellview_iter (index)->set_specific_path (spath);

    store_state ();
    redraw ();

    cellview_changed (index);

    update_content ();

    return ret;

  }
}

void 
LayoutViewBase::descend (const std::vector<db::InstElement> &path, int index)
{
  if (! path.empty () && index >= 0 && int (m_cellviews.size ()) > index && cellview_iter (index)->is_valid ()) {

    cellview_about_to_change_event (index);

    cancel (); 

    lay::CellView::specific_cell_path_type spath (cellview_iter (index)->specific_path ());
    spath.insert (spath.end (), path.begin (), path.end ());
    cellview_iter (index)->set_specific_path (spath);

    store_state ();
    redraw ();

    cellview_changed (index);

    update_content ();

  }
}

bool 
LayoutViewBase::is_editable () const
{
  return m_editable;
}

unsigned int
LayoutViewBase::search_range ()
{
  return m_search_range;
}

void
LayoutViewBase::set_search_range (unsigned int sr)
{
  m_search_range = sr;
}

unsigned int
LayoutViewBase::search_range_box ()
{
  return m_search_range_box;
}

void
LayoutViewBase::set_search_range_box (unsigned int sr)
{
  m_search_range_box = sr;
}

db::cell_index_type
LayoutViewBase::new_cell (int cv_index, const std::string &cell_name)
{
  db::cell_index_type new_ci (0);

  if (cv_index >= 0 && int (m_cellviews.size ()) > cv_index) {

    db::Layout &layout = cellview (cv_index)->layout ();
    if (! cell_name.empty () && layout.cell_by_name (cell_name.c_str ()).first) {
      throw tl::Exception (tl::to_string (tr ("A cell with that name already exists: %s")), cell_name);
    }

    transaction (tl::to_string (tr ("New cell")));
    new_ci = layout.add_cell (cell_name.empty () ? 0 : cell_name.c_str ());
    commit ();

  }

  return new_ci;
}

template <class T, class Iter>
static void make_unique_name (T *object, Iter from, Iter to)
{
  std::string n (object->name ());
  int nn = 0;

  do {

    bool found = n.empty ();
    for (Iter i = from; i != to && !found; ++i) {
      if ((*i)->name () == n) {
        found = true;
      }
    }

    if (! found) {
      break;
    }

    n = object->name () + tl::sprintf ("[%d]", ++nn);

  } while (1);

  object->set_name (n);
}

unsigned int 
LayoutViewBase::add_l2ndb (db::LayoutToNetlist *l2ndb)
{
  make_unique_name (l2ndb, m_l2ndbs.begin (), m_l2ndbs.end ());
  m_l2ndbs.push_back (l2ndb);

  //  Mark this object as owned by us (for GSI)
  l2ndb->keep ();

  l2ndb_list_changed_event ();

  return (unsigned int)(m_l2ndbs.size () - 1);
}

unsigned int
LayoutViewBase::replace_l2ndb (unsigned int db_index, db::LayoutToNetlist *l2ndb)
{
  tl_assert (l2ndb != 0);

  if (db_index < m_l2ndbs.size ()) {

    //  keep the name as it is used for reference in the browser for example
    std::string n = m_l2ndbs [db_index]->name ();
    l2ndb->set_name (n);

    delete m_l2ndbs [db_index];
    m_l2ndbs [db_index] = l2ndb;

    //  Mark this object as owned by us (for GSI)
    l2ndb->keep ();

    l2ndb_list_changed_event ();

    return db_index;

  } else {
    return add_l2ndb (l2ndb);
  }
}

db::LayoutToNetlist *
LayoutViewBase::get_l2ndb (int index)
{
  if (index >= 0 && index < int (m_l2ndbs.size ())) {
    return m_l2ndbs [index];
  } else {
    return 0;
  }
}

const db::LayoutToNetlist *
LayoutViewBase::get_l2ndb (int index) const
{
  if (index >= 0 && index < int (m_l2ndbs.size ())) {
    return m_l2ndbs [index];
  } else {
    return 0;
  }
}

void 
LayoutViewBase::remove_l2ndb (unsigned int index)
{
  if (index < (unsigned int) (m_l2ndbs.size ())) {
    delete m_l2ndbs [index];
    m_l2ndbs.erase (m_l2ndbs.begin () + index);
    l2ndb_list_changed_event ();
  }
}

unsigned int
LayoutViewBase::add_rdb (rdb::Database *rdb)
{
  make_unique_name (rdb, m_rdbs.begin (), m_rdbs.end ());
  m_rdbs.push_back (rdb);

  //  Mark this object as owned by us (for GSI)
  rdb->keep ();

  rdb_list_changed_event ();

  return (unsigned int)(m_rdbs.size () - 1);
}

unsigned int
LayoutViewBase::replace_rdb (unsigned int db_index, rdb::Database *rdb)
{
  tl_assert (rdb != 0);

  if (db_index < m_rdbs.size ()) {

    //  keep name because it's used for reference in the browser for example
    std::string n = m_rdbs [db_index]->name ();
    rdb->set_name (n);

    delete m_rdbs [db_index];
    m_rdbs [db_index] = rdb;

    //  Mark this object as owned by us (for GSI)
    rdb->keep ();

    rdb_list_changed_event ();

    return db_index;

  } else {
    return add_rdb (rdb);
  }
}

rdb::Database *
LayoutViewBase::get_rdb (int index)
{
  if (index >= 0 && index < int (m_rdbs.size ())) {
    return m_rdbs [index];
  } else {
    return 0;
  }
}

const rdb::Database *
LayoutViewBase::get_rdb (int index) const
{
  if (index >= 0 && index < int (m_rdbs.size ())) {
    return m_rdbs [index];
  } else {
    return 0;
  }
}

void
LayoutViewBase::remove_rdb (unsigned int index)
{
  if (index < (unsigned int) (m_rdbs.size ())) {
    delete m_rdbs [index];
    m_rdbs.erase (m_rdbs.begin () + index);
    rdb_list_changed_event ();
  }
}

} // namespace lay
