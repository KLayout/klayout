
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


#include <iostream>
#include <fstream>
#include <vector>

#include <QTimer>
#include <QSpinBox>
#include <QPainter>
#include <QPaintEvent>
#include <QComboBox>
#include <QDialog>
#include <QImageWriter>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>

#include "tlInternational.h"
#include "tlExpression.h"
#include "tlTimer.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "tlExceptions.h"
#include "layLayoutView.h"
#include "layViewOp.h"
#include "layViewObject.h"
#include "layLayoutViewConfigPages.h"
#include "laybasicConfig.h"
#include "layConverters.h"
#include "layGridNet.h"
#include "layMove.h"
#include "layDialogs.h"
#include "layZoomBox.h"
#include "layMouseTracker.h"
#include "layTipDialog.h"
#include "layEditable.h"
#include "layFixedFont.h"
#include "laySelector.h"
#include "layLayoutCanvas.h"
#include "layLayerControlPanel.h"
#include "layHierarchyControlPanel.h"
#include "layLibrariesView.h"
#include "layBrowser.h"
#include "layRedrawThread.h"
#include "layRedrawThreadWorker.h"
#include "layParsedLayerSource.h"
#include "layBookmarkManagementForm.h"
#include "layNetlistBrowserDialog.h"
#include "layBookmarksView.h"
#include "dbLayout.h"
#include "dbLayoutUtils.h"
#include "dbRecursiveShapeIterator.h"
#include "dbManager.h"
#include "dbEdgeProcessor.h"
#include "dbLibrary.h"
#include "rdb.h"
#include "rdbMarkerBrowserDialog.h"
#include "dbLayoutToNetlist.h"
#include "tlXMLParser.h"
#include "gsi.h"
#include "gtf.h"

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

const int timer_interval = 500;

static LayoutView *ms_current = 0;

LayoutView::LayoutView (db::Manager *manager, bool editable, lay::Plugin *plugin_parent, QWidget *parent, const char *name, unsigned int options)
  : QFrame (parent), 
    lay::Plugin (plugin_parent),
    m_editable (editable),
    m_options (options),
    m_annotation_shapes (manager),
    dm_prop_changed (this, &LayoutView::do_prop_changed) 
{
  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  setObjectName (QString::fromUtf8 (name));
  init (manager, plugin_root_maybe_null (), parent);
}

LayoutView::LayoutView (lay::LayoutView *source, db::Manager *manager, bool editable, lay::PluginRoot *root, QWidget *parent, const char *name, unsigned int options)
  : QFrame (parent), 
    lay::Plugin (root),
    m_editable (editable),
    m_options (options),
    m_annotation_shapes (manager),
    dm_prop_changed (this, &LayoutView::do_prop_changed)
{
  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  setObjectName (QString::fromUtf8 (name));

  m_annotation_shapes = source->m_annotation_shapes;

  init (manager, root, parent);

  //  set the handle reference and clear all cell related stuff 
  m_cellviews = source->cellview_list ();
  m_hidden_cells = source->m_hidden_cells;

  //  clear the history, store path and zoom box
  m_display_states.clear ();
  m_display_state_ptr = 0;
  m_synchronous = source->synchronous ();
  m_drawing_workers = source->drawing_workers ();

  //  duplicate the layer properties
  for (size_t i = 0; i < source->m_layer_properties_lists.size (); ++i) {
    if (i >= m_layer_properties_lists.size ()) {
      m_layer_properties_lists.push_back (new lay::LayerPropertiesList (*source->m_layer_properties_lists [i]));
    } else {
      *m_layer_properties_lists [i] = *source->m_layer_properties_lists [i];
    }
    m_layer_properties_lists [i]->attach_view (this, (unsigned int) i);
  }

  if (! m_layer_properties_lists.empty ()) {
    mp_canvas->set_dither_pattern (m_layer_properties_lists [0]->dither_pattern ()); 
  }

  bookmarks (source->bookmarks ());

  set_active_cellview_index (source->active_cellview_index ());

  //  copy the title
  m_title = source->m_title;

  layer_list_changed_event (3);

  finish_cellviews_changed ();
}

bool 
LayoutView::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == mp_min_hier_spbx || obj == mp_max_hier_spbx) {

    //  Makes the min/max spin boxes accept only numeric and some control keys ..
    QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);
    if (keyEvent && 
        keyEvent->key () != Qt::Key_Home &&
        keyEvent->key () != Qt::Key_End &&
        keyEvent->key () != Qt::Key_Delete &&
        keyEvent->key () != Qt::Key_Backspace &&
        (keyEvent->key () < Qt::Key_0 || keyEvent->key () > Qt::Key_9)) {
      return true;
    } else {
      return false;
    }

  } else {
    return QFrame::eventFilter (obj, event);
  }
}

void
LayoutView::init (db::Manager *mgr, lay::PluginRoot *root, QWidget * /*parent*/)
{
  manager (mgr);

  m_annotation_shapes.manager (mgr);

  m_visibility_changed = false;
  m_active_cellview_changed_event_enabled = true;
  m_disabled_edits = 0;
  m_synchronous = false;
  m_drawing_workers = 1;
  mp_control_panel = 0;
  mp_control_frame = 0;
  mp_hierarchy_panel = 0;
  mp_hierarchy_frame = 0;
  mp_libraries_view = 0;
  mp_bookmarks_view = 0;
  mp_libraries_frame = 0;
  mp_bookmarks_frame = 0;
  mp_min_hier_spbx = 0;
  mp_max_hier_spbx = 0;
  m_from_level = 0;
  m_pan_distance = 0.15;
  m_wheel_mode = 0;
  m_paste_display_mode = 2;
  m_guiding_shape_visible = true;
  m_guiding_shape_line_width = 1;
  m_guiding_shape_color = QColor ();
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
  m_text_font = 0;
  m_show_markers = true;
  m_no_stipples = false;
  m_stipple_offset = true;
  m_fit_new_cell = true;
  m_full_hier_new_cell = true;
  m_clear_ruler_new_cell = false;
  m_dbu_coordinates = false;
  m_absolute_coordinates = false;
  m_drop_small_cells = false;
  m_drop_small_cells_value = 10;
  m_drop_small_cells_cond = DSC_Max;
  m_draw_array_border_instances = false;
  m_dirty = false;
  m_activated = true;
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
  m_move_to_origin_mode_x = 0;
  m_move_to_origin_mode_y = 0;
  m_align_cell_origin_mode_x = -1;
  m_align_cell_origin_mode_y = -1;
  m_align_cell_origin_visible_layers = false;
  m_align_cell_adjust_parents = true;
  m_del_cell_mode = 0;
  m_layer_hier_mode = 0;
  m_add_other_layers = false;
  m_always_show_source = false;
  m_always_show_ld = true;
  m_always_show_layout_index = false;
  m_duplicate_hier_mode = 2;
  m_clear_before = true;
  m_copy_cva = -1;
  m_copy_cvr = -1;
  m_copy_layera = -1;
  m_copy_layerr = -1;
  m_search_range = 5;

  m_layer_properties_lists.push_back (new LayerPropertiesList ());
  m_layer_properties_lists.back ()->attach_view (this, (unsigned int) (m_layer_properties_lists.size () - 1));
  m_current_layer_list = 0;

  QVBoxLayout *vbl = new QVBoxLayout (this);
  vbl->setMargin (0);
  vbl->setSpacing (0);

  mp_canvas = new lay::LayoutCanvas (this, this);
  vbl->addWidget (mp_canvas);
  connect (mp_canvas, SIGNAL (left_arrow_key_pressed ()), this, SLOT (pan_left ()));
  connect (mp_canvas, SIGNAL (up_arrow_key_pressed ()), this, SLOT (pan_up ()));
  connect (mp_canvas, SIGNAL (right_arrow_key_pressed ()), this, SLOT (pan_right ()));
  connect (mp_canvas, SIGNAL (down_arrow_key_pressed ()), this, SLOT (pan_down ()));
  connect (mp_canvas, SIGNAL (left_arrow_key_pressed_with_shift ()), this, SLOT (pan_left_fast ()));
  connect (mp_canvas, SIGNAL (up_arrow_key_pressed_with_shift ()), this, SLOT (pan_up_fast ()));
  connect (mp_canvas, SIGNAL (right_arrow_key_pressed_with_shift ()), this, SLOT (pan_right_fast ()));
  connect (mp_canvas, SIGNAL (down_arrow_key_pressed_with_shift ()), this, SLOT (pan_down_fast ()));

  if ((m_options & LV_NoHierarchyPanel) == 0 && (m_options & LV_Naked) == 0) {

    QFrame *hierarchy_frame = new QFrame (0);
    hierarchy_frame->setObjectName (QString::fromUtf8 ("left"));
    mp_hierarchy_frame = hierarchy_frame;
    QVBoxLayout *left_frame_ly = new QVBoxLayout (hierarchy_frame);
    left_frame_ly->setMargin (0);
    left_frame_ly->setSpacing (0);

    mp_hierarchy_panel = new lay::HierarchyControlPanel (this, hierarchy_frame, "hcp");
    left_frame_ly->addWidget (mp_hierarchy_panel, 1 /*stretch*/);

    connect (mp_hierarchy_panel, SIGNAL (cell_selected (cell_path_type, int)), this, SLOT (select_cell_dispatch (cell_path_type, int)));
    connect (mp_hierarchy_panel, SIGNAL (active_cellview_changed (int)), this, SLOT (active_cellview_changed (int)));

    QFrame *levels_frame = new QFrame (hierarchy_frame);
    levels_frame->setObjectName (QString::fromUtf8 ("lvl_frame"));
    left_frame_ly->addWidget (levels_frame);
    QHBoxLayout *levels_frame_ly = new QHBoxLayout (levels_frame);
    levels_frame_ly->setMargin (1);
    QLabel *level_l1 = new QLabel (tl::to_qstring (" " + tl::to_string (QObject::tr ("Levels"))), levels_frame);
    levels_frame_ly->addWidget (level_l1);
    mp_min_hier_spbx = new QSpinBox (levels_frame);
    mp_min_hier_spbx->setObjectName (QString::fromUtf8 ("min_lvl"));
    levels_frame_ly->addWidget (mp_min_hier_spbx);
    QLabel *level_l2 = new QLabel (QString::fromUtf8 (".."), levels_frame);
    levels_frame_ly->addWidget (level_l2);
    mp_max_hier_spbx = new QSpinBox (levels_frame);
    mp_max_hier_spbx->setObjectName (QString::fromUtf8 ("max_lvl"));
    levels_frame_ly->addWidget (mp_max_hier_spbx);

    mp_min_hier_spbx->installEventFilter (this);
    mp_max_hier_spbx->installEventFilter (this);

    mp_min_hier_spbx->setMaximum (0);
    mp_min_hier_spbx->setMinimum (-1000);
    mp_min_hier_spbx->setValue (0);
    mp_max_hier_spbx->setMaximum (999);
    mp_max_hier_spbx->setValue (0);
    mp_max_hier_spbx->setMinimum (-1000);

    connect (mp_min_hier_spbx, SIGNAL (valueChanged (int)), this, SLOT (min_hier_changed (int)));
    connect (mp_max_hier_spbx, SIGNAL (valueChanged (int)), this, SLOT (max_hier_changed (int)));

  }

  if ((m_options & LV_NoBookmarksView) == 0 && (m_options & LV_Naked) == 0) {

    QFrame *bookmarks_frame = new QFrame (0);
    bookmarks_frame->setObjectName (QString::fromUtf8 ("bookmarks_frame"));
    mp_bookmarks_frame = bookmarks_frame;
    QVBoxLayout *left_frame_ly = new QVBoxLayout (bookmarks_frame);
    left_frame_ly->setMargin (0);
    left_frame_ly->setSpacing (0);

    mp_bookmarks_view = new lay::BookmarksView (this, bookmarks_frame, "bookmarks");
    left_frame_ly->addWidget (mp_bookmarks_view, 1 /*stretch*/);

  }

  if ((m_options & LV_NoLibrariesView) == 0 && (m_options & LV_Naked) == 0) {

    QFrame *libraries_frame = new QFrame (0);
    libraries_frame->setObjectName (QString::fromUtf8 ("libs_frame"));
    mp_libraries_frame = libraries_frame;
    QVBoxLayout *left_frame_ly = new QVBoxLayout (libraries_frame);
    left_frame_ly->setMargin (0);
    left_frame_ly->setSpacing (0);

    mp_libraries_view = new lay::LibrariesView (this, libraries_frame, "libs");
    left_frame_ly->addWidget (mp_libraries_view, 1 /*stretch*/);

    connect (mp_libraries_view, SIGNAL (active_library_changed (int)), this, SLOT (active_library_changed (int)));

  }

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

  if ((m_options & LV_NoLayers) == 0 && (m_options & LV_Naked) == 0) {

    mp_control_panel = new lay::LayerControlPanel (this, manager (), 0, "lcp");
    mp_control_frame = mp_control_panel;

    connect (mp_control_panel, SIGNAL (tab_changed ()), this, SLOT (layer_tab_changed ()));
    connect (mp_control_panel, SIGNAL (order_changed ()), this, SLOT (layer_order_changed ()));
    /*
    connect (mp_control_panel, SIGNAL (marked_changed ()), this, SLOT (prop_changed ()));
    connect (mp_control_panel, SIGNAL (width_changed ()), this, SLOT (prop_changed ()));
    connect (mp_control_panel, SIGNAL (animation_changed ()), this, SLOT (prop_changed ()));
    connect (mp_control_panel, SIGNAL (visibility_changed ()), this, SLOT (visibility_changed ()));
    connect (mp_control_panel, SIGNAL (transparency_changed ()), this, SLOT (prop_changed ()));
    connect (mp_control_panel, SIGNAL (stipple_changed ()), this, SLOT (prop_changed ()));
    connect (mp_control_panel, SIGNAL (color_changed ()), this, SLOT (prop_changed ()));
    */

  }
  
  mp_timer = new QTimer (this);
  connect (mp_timer, SIGNAL (timeout ()), this, SLOT (timer ()));
  mp_timer->start (timer_interval);

  if (root) {
    create_plugins (root);
  }

  m_new_layer_props.layer = 1;
  m_new_layer_props.datatype = 0;

  config_setup ();
}

LayoutView::~LayoutView ()
{
  close_event ();

  if (ms_current == this) {
    ms_current = 0;
  }

  //  detach all observers
  //  This is to prevent signals to partially destroyed observers that own a LayoutView
  close_event.clear ();
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

  //  because LayoutView and LayoutCanvas both control lifetimes of
  //  ruler objects for example, it is safer to explictly delete the
  //  LayoutCanvas object here:
  delete mp_canvas;
  mp_canvas = 0;

  if (mp_control_frame) {
    delete mp_control_frame;
  }
  mp_control_panel = 0;
  mp_control_frame = 0;

  if (mp_hierarchy_frame) {
    delete mp_hierarchy_frame;
  }
  mp_hierarchy_frame = 0;
  mp_hierarchy_panel = 0;

  if (mp_libraries_frame) {
    delete mp_libraries_frame;
  }
  mp_libraries_frame = 0;
  mp_libraries_view = 0;

  if (mp_bookmarks_frame) {
    delete mp_bookmarks_frame;
  }
  mp_bookmarks_frame = 0;
  mp_bookmarks_view = 0;
}

void LayoutView::hideEvent (QHideEvent *)
{
  hide_event ();
}

void LayoutView::showEvent (QShowEvent *)
{
  show_event ();
}

void LayoutView::set_current ()
{
  set_current (this);
}

void LayoutView::set_current (lay::LayoutView *view)
{
  if (ms_current != view) {
    if (ms_current) {
      ms_current->deactivate ();
    }
    ms_current = view;
    if (ms_current) {
      ms_current->activate ();
    }
  }
}

LayoutView *LayoutView::current ()
{
  return ms_current;
}

void LayoutView::update_event_handlers ()
{
  tl::Object::detach_from_all_events ();

  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    //  TODO: get rid of the const_cast hack
    const_cast<lay::PluginDeclaration *> ((*p)->plugin_declaration ())->editable_enabled_changed_event.add (this, &LayoutView::signal_plugin_enabled_changed);
  }

  for (unsigned int i = 0; i < cellviews (); ++i) {
    cellview (i)->layout ().hier_changed_event.add (this, &LayoutView::signal_hier_changed);
    cellview (i)->layout ().bboxes_changed_event.add (this, &LayoutView::signal_bboxes_from_layer_changed, i);
    cellview (i)->layout ().dbu_changed_event.add (this, &LayoutView::signal_bboxes_changed);
    cellview (i)->layout ().prop_ids_changed_event.add (this, &LayoutView::signal_prop_ids_changed);
    cellview (i)->layout ().layer_properties_changed_event.add (this, &LayoutView::signal_layer_properties_changed);
    cellview (i)->layout ().cell_name_changed_event.add (this, &LayoutView::signal_cell_name_changed);
    cellview (i)->apply_technology_with_sender_event.add (this, &LayoutView::signal_apply_technology);
  }

  annotation_shapes ().bboxes_changed_any_event.add (this, &LayoutView::signal_annotations_changed);

  mp_canvas->viewport_changed_event.add (this, &LayoutView::viewport_changed);
}

void LayoutView::viewport_changed ()
{
  viewport_changed_event ();
}

bool LayoutView::accepts_drop (const std::string &path_or_url) const
{
  for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->accepts_drop (path_or_url)) {
      return true;
    }
  }
  return false;
}

void LayoutView::drop_url (const std::string &path_or_url)
{
  for (std::vector<lay::Plugin *>::const_iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->accepts_drop (path_or_url)) {
      (*p)->drop_url (path_or_url);
      break;
    }
  }
}

lay::Plugin *LayoutView::create_plugin (lay::PluginRoot *root, const lay::PluginDeclaration *cls)
{
  lay::Plugin *p = cls->create_plugin (manager (), root, this);
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

void LayoutView::create_plugins (lay::PluginRoot *root, const lay::PluginDeclaration *except_this)
{
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    delete *p;
  }
  mp_plugins.clear ();

  //  create the plugins
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {

    if (&*cls != except_this) {

      //  TODO: clean solution. The following is a HACK:
      if (cls.current_name () == "ant::Plugin" || cls.current_name () == "img::Plugin") {
        //  ant and img are created always
        create_plugin (root, &*cls);
      } else if ((m_options & LV_NoPlugins) == 0) {
        //  others: only create unless LV_NoPlugins is set
        create_plugin (root, &*cls);
      } else if ((m_options & LV_NoGrid) == 0 && cls.current_name () == "GridNetPlugin") {
        //  except grid net plugin which is created on request
        create_plugin (root, &*cls);
      }

    }

  }

  mode (default_mode ());
}

Plugin *LayoutView::get_plugin_by_name (const std::string &name) const
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
LayoutView::init_menu (lay::AbstractMenu &menu)
{
  lay::LayerControlPanel::init_menu (menu);
  lay::HierarchyControlPanel::init_menu (menu);
  lay::LibrariesView::init_menu (menu);
  lay::BookmarksView::init_menu (menu);
}

void
LayoutView::update_menu (lay::LayoutView *view, lay::AbstractMenu &menu)
{
  std::string bm_menu = "bookmark_menu.goto_bookmark_menu";

  if (menu.is_valid (bm_menu)) {

    menu.clear_menu (bm_menu);

    Action goto_bookmark_action = menu.action (bm_menu);

    if (view && view->bookmarks ().size () > 0) {

      goto_bookmark_action.set_enabled (true);

      const lay::BookmarkList &bookmarks = view->bookmarks ();
      for (size_t i = 0; i < bookmarks.size (); ++i) {
        Action action;
        gtf::action_connect (action.qaction (), SIGNAL (triggered ()), view, SLOT (goto_bookmark ()));
        action.set_title (bookmarks.name (i));
        action.qaction ()->setData (QVariant (int (i)));
        menu.insert_item (bm_menu + ".end", tl::sprintf ("bookmark_%d", i + 1), action);
      }

    } else {
      goto_bookmark_action.set_enabled (false);
    }

  }
}

void
LayoutView::set_drawing_workers (int workers)
{
  m_drawing_workers = std::max (0, std::min (100, workers));
}

void
LayoutView::set_synchronous (bool s)
{
  m_synchronous = s;
}

bool
LayoutView::is_dirty () const
{
  return m_dirty;
}

std::string
LayoutView::title () const
{
  if (! m_title.empty ()) {
    return m_title;
  } else if (cellviews () == 0) {
    return tl::to_string (QObject::tr ("<empty>"));
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
LayoutView::set_title (const std::string &t)
{
  if (m_title != t) {
    m_title = t;
    emit title_changed ();
  }
}

void 
LayoutView::reset_title ()
{
  if (! m_title.empty ()) {
    m_title = "";
    emit title_changed ();
  }
}

bool 
LayoutView::configure (const std::string &name, const std::string &value)
{
  if (mp_move_service && mp_move_service->configure (name, value)) {
    return true;
  }

  if (name == cfg_default_lyp_file) {

    m_def_lyp_file = value;
    return false; // not taken - let others set it too.

  } else if (name == cfg_default_add_other_layers) {

    tl::from_string (value, m_add_other_layers);
    return false; // not taken - let others set it too.

  } else if (name == cfg_layers_always_show_source) {

    bool a = false;
    tl::from_string (value, a);
    if (a != m_always_show_source) {
      m_always_show_source = a;
      layer_list_changed_event (4);
    }

    return true;

  } else if (name == cfg_layers_always_show_ld) {

    tl::from_string (value, m_always_show_ld);
    update_content ();
    return true;

  } else if (name == cfg_layers_always_show_layout_index) {

    tl::from_string (value, m_always_show_layout_index);
    update_content ();
    return true;

  } else if (name == cfg_flat_cell_list) {

    bool f;
    tl::from_string (value, f);
    if (mp_hierarchy_panel) {
      mp_hierarchy_panel->set_flat (f);
    }
    return true;

  } else if (name == cfg_split_cell_list) {

    bool f;
    tl::from_string (value, f);
    if (mp_hierarchy_panel) {
      mp_hierarchy_panel->set_split_mode (f);
    }
    return true;

  } else if (name == cfg_split_lib_views) {

    bool f;
    tl::from_string (value, f);
    if (mp_libraries_view) {
      mp_libraries_view->set_split_mode (f);
    }
    return true;

  } else if (name == cfg_bookmarks_follow_selection) {

    bool f;
    tl::from_string (value, f);
    if (mp_bookmarks_view) {
      mp_bookmarks_view->follow_selection (f);
    }
    return true;

  } else if (name == cfg_current_lib_view) {

    if (mp_libraries_view) {
      mp_libraries_view->select_active_lib_by_name (value);
    }
    return true;

  } else if (name == cfg_cell_list_sorting) {

    if (mp_hierarchy_panel) {
      if (value == "by-name") {
        mp_hierarchy_panel->set_sorting (CellTreeModel::ByName);
      } else if (value == "by-area") {
        mp_hierarchy_panel->set_sorting (CellTreeModel::ByArea);
      } else if (value == "by-area-reverse") {
        mp_hierarchy_panel->set_sorting (CellTreeModel::ByAreaReverse);
      }
    }
    return true;

  } else if (name == cfg_hide_empty_layers) {

    bool f;
    tl::from_string (value, f);
    if (mp_control_panel) {
      mp_control_panel->set_hide_empty_layers (f);
    }
    return true;

  } else if (name == cfg_test_shapes_in_view) {

    bool f;
    tl::from_string (value, f);
    if (mp_control_panel) {
      mp_control_panel->set_test_shapes_in_view (f);
    }
    return true;

  } else if (name == cfg_background_color) {

    QColor color;
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
      redraw ();
    }
    //  do not take - let others have the event for the redraw call
    return false;

  } else if (name == cfg_bitmap_oversampling) {

    int os = 1;
    tl::from_string (value, os);
    mp_canvas->set_oversampling (os);
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

    QColor color;
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

    QColor color;
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

    QColor color;
    ColorConverter ().from_string (value, color);
    cell_box_color (color);
    return true;

  } else if (name == cfg_text_color) {

    QColor color;
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

    QColor color;
    ColorConverter ().from_string (value, color);
    guiding_shapes_color (color);
    return true;

  } else if (name == cfg_guiding_shape_color) {

    QColor color;
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

    QColor color;
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
LayoutView::enable_edits (bool enable)
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
    enable = (m_disabled_edits == 0);
  } else {
    ++m_disabled_edits;
  }

  if (edits_enabled () != is_enabled) {
    emit edits_enabled_changed ();
  }
}

void 
LayoutView::set_current_layer (unsigned int cv_index, const db::LayerProperties &lp) 
{
  //  rename the ones that got shifted.
  lay::LayerPropertiesConstIterator l = begin_layers ();
  while (! l.at_end ()) {
    if (l->source (true).cv_index () == int (cv_index) && l->source (true).layer_props ().log_equal (lp)) {
      set_current_layer (l);
      break;
    }
    ++l;
  }
}

void 
LayoutView::set_current_layer (const lay::LayerPropertiesConstIterator &l) 
{
  if (mp_control_panel) {
    mp_control_panel->set_current_layer (l);
  }
}

lay::LayerPropertiesConstIterator 
LayoutView::current_layer () const
{
  if (mp_control_panel) {
    return mp_control_panel->current_layer ();
  } else {
    return lay::LayerPropertiesConstIterator ();
  }
}

std::vector<lay::LayerPropertiesConstIterator> 
LayoutView::selected_layers () const
{
  if (mp_control_panel) {
    return mp_control_panel->selected_layers ();
  } else {
    return std::vector<lay::LayerPropertiesConstIterator> ();
  }
}

void 
LayoutView::set_selected_layers (const std::vector<lay::LayerPropertiesConstIterator> &sel) 
{
  if (mp_control_panel) {
    mp_control_panel->set_selection (sel);
  }
}

void
LayoutView::set_line_styles (const lay::LineStyles &styles)
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
LayoutView::set_dither_pattern (const lay::DitherPattern &pattern)
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
LayoutView::get_properties (unsigned int index) const
{
  if (index >= layer_lists ()) {
    static lay::LayerPropertiesList empty;
    return empty;
  } else {
    return *m_layer_properties_lists [index];
  }
}

void
LayoutView::set_current_layer_list (unsigned int index)
{
  if (index != m_current_layer_list && index < layer_lists ()) {
    m_current_layer_list = index;
    current_layer_list_changed_event (index);
    redraw ();
  }
}

void 
LayoutView::insert_layer_list (unsigned index, const LayerPropertiesList &props)
{
  if (index > layer_lists ()) {
    return;
  }

  if (transacting ()) {
    manager ()->queue (this, new OpInsertLayerList (index, props));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }

  m_layer_properties_lists.insert (m_layer_properties_lists.begin () + index, new LayerPropertiesList (props));
  m_layer_properties_lists [index]->attach_view (this, index);
  merge_dither_pattern (*m_layer_properties_lists [index]);

  m_current_layer_list = index;
  current_layer_list_changed_event (index);

  layer_list_inserted_event (index);

  redraw ();

  dm_prop_changed ();
}

void 
LayoutView::delete_layer_list (unsigned index)
{
  if (index >= layer_lists ()) {
    return;
  }

  if (transacting ()) {
    manager ()->queue (this, new OpDeleteLayerList (index, *m_layer_properties_lists [index]));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }

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
  dm_prop_changed ();
}

void 
LayoutView::rename_properties (unsigned int index, const std::string &new_name)
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

void
LayoutView::merge_dither_pattern (lay::LayerPropertiesList &props)
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

void 
LayoutView::set_properties (unsigned int index, const LayerPropertiesList &props)
{
  //  If index is not a valid tab index, don't do anythign except for the case of
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

  if (mp_control_panel && index == current_layer_list ()) {
    mp_control_panel->begin_updates ();
  }

  *m_layer_properties_lists [index] = props;
  m_layer_properties_lists [index]->attach_view (this, index);

  merge_dither_pattern (*m_layer_properties_lists [index]);

  if (index == current_layer_list ()) {

    layer_list_changed_event (3);

    redraw ();

    dm_prop_changed ();

  }
}

void 
LayoutView::expand_properties ()
{
  expand_properties (std::map<int, int> (), false);
}
  
void 
LayoutView::expand_properties (unsigned int index)
{
  expand_properties (index, std::map<int, int> (), false);
}

void 
LayoutView::expand_properties (const std::map<int, int> &map_cv_index, bool add_default)
{
  for (unsigned int i = 0; i < cellviews (); ++i) {
    expand_properties (i, map_cv_index, add_default);
  }
}

void 
LayoutView::expand_properties (unsigned int index, const std::map<int, int> &map_cv_index, bool add_default)
{
  if (index < m_layer_properties_lists.size ()) {
    m_layer_properties_lists [index]->expand (map_cv_index, add_default);
  }
}

void
LayoutView::replace_layer_node (unsigned int index, const LayerPropertiesConstIterator &iter, const LayerPropertiesNode &node)
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

    if (mp_control_panel && index == current_layer_list ()) {
      mp_control_panel->begin_updates ();
    }

    LayerPropertiesIterator non_const_iter (get_properties (index), iter.uint ());
    *non_const_iter = node;
    non_const_iter->attach_view (this, index);

    if (index == current_layer_list ()) {

      layer_list_changed_event (2);

      //  TODO: check, if redraw is actually necessary (this is complex!)
      redraw ();

      dm_prop_changed ();

    }
  }
}

void 
LayoutView::set_properties (unsigned int index, const LayerPropertiesConstIterator &iter, const LayerProperties &props)
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
        redraw ();
      } 

      if (visible_changed) {
        m_visibility_changed = true;
      }

      //  perform the callbacks asynchronously to collect the necessary calls instead
      //  of executing them immediately.
      dm_prop_changed ();

    }
  }
}

const LayerPropertiesNode &
LayoutView::insert_layer (unsigned int index, const LayerPropertiesConstIterator &before, const LayerPropertiesNode &node)
{
  tl_assert (index < layer_lists ());

  if (transacting ()) {
    manager ()->queue (this, new OpInsertLayerProps (index, (unsigned int) before.uint (), node));
  } else if (manager () && ! replaying ()) {
    manager ()->clear ();
  }
  
  if (mp_control_panel && index == current_layer_list ()) {
    mp_control_panel->begin_updates ();
  }

  const LayerPropertiesNode &ret = m_layer_properties_lists [index]->insert (LayerPropertiesIterator (*m_layer_properties_lists [index], before.uint ()), node);

  //  signal to the observers that something has changed
  if (index == current_layer_list ()) {
    layer_list_changed_event (2);
    redraw ();
    dm_prop_changed ();
  }

  return ret;
}

void 
LayoutView::delete_layer (unsigned int index, LayerPropertiesConstIterator &iter)
{
  if (index >= layer_lists ()) {
    return;
  }

  lay::LayerPropertiesNode orig = *iter;

  if (mp_control_panel && index == current_layer_list ()) {
    mp_control_panel->begin_updates ();
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
    layer_list_changed_event (2);
    redraw ();
    dm_prop_changed ();
  }

  //  invalidate the iterator so it can be used to refer to the next element
  iter.invalidate ();
}

void
LayoutView::signal_selection_changed ()
{
  if (selection_size () > 1) {
    message (tl::sprintf (tl::to_string (QObject::tr ("selected: %ld objects")), selection_size ()));
  }

  lay::Editables::signal_selection_changed ();
}

void 
LayoutView::save_as (unsigned int index, const std::string &filename, tl::OutputStream::OutputStreamMode om, const db::SaveLayoutOptions &options, bool update)
{
  tl_assert (index < cellviews ());

  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Saving")));
  cellview (index)->save_as (filename, om, options, update);

  cellview_changed (index);
}

void
LayoutView::redo (db::Op *op)
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
LayoutView::undo (db::Op *op)
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
LayoutView::signal_hier_changed ()
{
  //  schedule a redraw request for all layers
  redraw ();
  //  forward this event to our observers
  hier_changed_event ();
}

void
LayoutView::signal_bboxes_from_layer_changed (unsigned int cv_index, unsigned int layer_index)
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
LayoutView::signal_bboxes_changed ()
{
  //  schedule a redraw request for all layers
  redraw ();

  //  forward this event to our observers
  geom_changed_event ();
}

void
LayoutView::signal_cell_name_changed ()
{
  cell_visibility_changed_event (); // HINT: that is not what actually is intended, but it serves the function ...
  redraw ();  //  needs redraw
}

void
LayoutView::signal_layer_properties_changed ()
{
  //  recompute the source 
  //  TODO: this is a side effect of this method - provide a special method for this purpose
  for (unsigned int i = 0; i < layer_lists (); ++i) {
    m_layer_properties_lists [i]->attach_view (this, i);
  }

  //  schedule a redraw request - since the layer views might not have changed, this is necessary
  redraw ();
}

void
LayoutView::signal_prop_ids_changed ()
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
LayoutView::signal_plugin_enabled_changed ()
{
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->editable_interface ()) {
      enable ((*p)->editable_interface (), (*p)->plugin_declaration ()->editable_enabled ());
    }
  }
}

void
LayoutView::signal_annotations_changed ()
{
  //  schedule a redraw request for the annotation shapes
  redraw_deco_layer ();
  //  forward this event to our observers
  annotations_changed_event ();
}

void 
LayoutView::finish_cellviews_changed ()
{
  update_event_handlers ();

  cellviews_changed_event ();

  redraw ();
}

std::list<lay::CellView>::iterator
LayoutView::cellview_iter (int cv_index)
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
LayoutView::cellview_iter (int cv_index) const
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
LayoutView::erase_cellview (unsigned int index)
{
  if (index >= m_cellviews.size ()) {
    return;
  }

  cancel ();

  //  issue to event that signals a change in the cellviews
  cellviews_about_to_change_event ();

  //  no undo available - clear all transactions
  if (manager ()) {
    manager ()->clear ();
  }

  if (mp_control_panel) {
    mp_control_panel->begin_updates ();
  }

  m_cellviews.erase (cellview_iter (int (index)));

  if (m_hidden_cells.size () > index) {
    m_hidden_cells.erase (m_hidden_cells.begin () + index);
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

  //  signal to the observers that something has changed
  layer_list_changed_event (3);

  finish_cellviews_changed ();

  update_content ();

  if (m_title.empty ()) {
    emit title_changed ();
  }
}

void
LayoutView::clear_cellviews ()
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

  //  clear the history, store path and zoom box
  m_display_states.clear ();
  m_display_state_ptr = 0;

  finish_cellviews_changed ();

  if (m_title.empty ()) {
    emit title_changed ();
  }
}

const CellView &
LayoutView::cellview (unsigned int index) const
{
  static const CellView empty;
  if (index >= m_cellviews.size ()) {
    return empty;
  } else {
    return *cellview_iter (int (index));
  }
}

CellViewRef
LayoutView::cellview_ref (unsigned int index)
{
  if (index >= m_cellviews.size ()) {
    return CellViewRef ();
  } else {
    return CellViewRef (cellview_iter (index).operator-> (), this);
  }
}

int
LayoutView::index_of_cellview (const lay::CellView *cv) const
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
LayoutView::set_layout (const lay::CellView &cv, unsigned int cvindex) 
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
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->do_update_content (cvindex);
  }

  if (m_title.empty ()) {
    emit title_changed ();
  }
}

void
LayoutView::signal_apply_technology (lay::LayoutHandle *layout_handle)
{
  //  find the cellview which issued the event
  for (unsigned int i = 0; i < cellviews (); ++i) {

    if (cellview (i).handle () == layout_handle) {

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

    }

  }
}

void 
LayoutView::load_layer_props (const std::string &fn)
{
  do_load_layer_props (fn, false, -1, false);
}

void 
LayoutView::load_layer_props (const std::string &fn, bool add_default)
{
  do_load_layer_props (fn, false, -1, add_default);
}

void 
LayoutView::load_layer_props (const std::string &fn, int cv_index, bool add_default)
{
  do_load_layer_props (fn, true, cv_index, add_default);
}

void 
LayoutView::do_load_layer_props (const std::string &fn, bool map_cv, int cv_index, bool add_default)
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

  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Load layer properties"))); 
  }

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

  if (manager ()) {
    manager ()->commit ();
  }

  update_content ();

  tl::log << "Loaded layer properties from " << fn;
}

void 
LayoutView::save_layer_props (const std::string &fn)
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
LayoutView::add_new_layers (const std::vector <unsigned int> &layer_ids, int cv_index)
{
  if (cv_index >= 0 && cv_index < int (cellviews ())) {

    const lay::CellView &cv = cellview (cv_index);

    //  create the layers and do a basic recoloring ..
    lay::LayerPropertiesList new_props (get_properties ());

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

  }
}

void 
LayoutView::init_layer_properties (LayerProperties &p) const
{
  init_layer_properties (p, get_properties ());
}

void 
LayoutView::init_layer_properties (LayerProperties &p, const LayerPropertiesList &lp_list) const
{
  lay::color_t c = 0;
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

QImage 
LayoutView::get_screenshot ()
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Save screenshot")));

  //  Execute all deferred methods - ensure there are no pending tasks
  tl::DeferredMethodScheduler::execute ();
  
  return mp_canvas->screenshot ();
}

void 
LayoutView::save_screenshot (const std::string &fn)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Save screenshot")));

  QImageWriter writer (tl::to_qstring (fn), QByteArray ("PNG"));

  //  Unfortunately the PNG writer does not allow writing of long strings.
  //  We separate the description into a set of keys:

  for (unsigned int i = 0; i < cellviews (); ++i) {
    if (cellview (i).is_valid ()) {
      std::string name = cellview (i)->layout ().cell_name (cellview (i).cell_index ());
      writer.setText (tl::to_qstring ("Cell" + tl::to_string (int (i) + 1)), tl::to_qstring (name));
    }
  }

  db::DBox b (box ());
  std::string desc;
  desc += tl::micron_to_string (b.left ()) + "," + tl::micron_to_string (b.bottom ());
  desc += "/";
  desc += tl::micron_to_string (b.right ()) + "," + tl::micron_to_string (b.top ());
  writer.setText (QString::fromUtf8 ("Rect"), tl::to_qstring (desc));

  //  Execute all deferred methods - ensure there are no pending tasks
  tl::DeferredMethodScheduler::execute ();
  
  if (! writer.write (mp_canvas->screenshot ())) {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to write screenshot to file: %s (%s)")), fn, tl::to_string (writer.errorString ()));
  }

  tl::log << "Saved screen shot to " << fn;
}

QImage 
LayoutView::get_image (unsigned int width, unsigned int height)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Save image")));

  //  Execute all deferred methods - ensure there are no pending tasks
  tl::DeferredMethodScheduler::execute ();
  
  return mp_canvas->image (width, height);
}

QImage 
LayoutView::get_image_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, 
                                    QColor background, QColor foreground, QColor active, const db::DBox &target_box, bool monochrome)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Save image")));

  //  Execute all deferred methods - ensure there are no pending tasks
  tl::DeferredMethodScheduler::execute ();
  
  return mp_canvas->image_with_options (width, height, linewidth, oversampling, resolution, background, foreground, active, target_box, monochrome);
}

void 
LayoutView::save_image (const std::string &fn, unsigned int width, unsigned int height)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Save image")));

  QImageWriter writer (tl::to_qstring (fn), QByteArray ("PNG"));

  //  Unfortunately the PNG writer does not allow writing of long strings.
  //  We separate the description into a set of keys:

  for (unsigned int i = 0; i < cellviews (); ++i) {
    if (cellview (i).is_valid ()) {
      std::string name = cellview (i)->layout ().cell_name (cellview (i).cell_index ());
      writer.setText (tl::to_qstring ("Cell" + tl::to_string (int (i) + 1)), tl::to_qstring (name));
    }
  }

  lay::Viewport vp (width, height, mp_canvas->viewport ().target_box ());
  writer.setText (QString::fromUtf8 ("Rect"), tl::to_qstring (vp.box ().to_string ()));
  
  //  Execute all deferred methods - ensure there are no pending tasks
  tl::DeferredMethodScheduler::execute ();
  
  if (! writer.write (mp_canvas->image (width, height))) {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to write screenshot to file: %s (%s)")), fn, tl::to_string (writer.errorString ()));
  }

  tl::log << "Saved screen shot to " << fn;
}

void 
LayoutView::save_image_with_options (const std::string &fn, 
                                     unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, 
                                     QColor background, QColor foreground, QColor active, const db::DBox &target_box, bool monochrome)
{
  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Save image")));

  QImageWriter writer (tl::to_qstring (fn), QByteArray ("PNG"));

  //  Unfortunately the PNG writer does not allow writing of long strings.
  //  We separate the description into a set of keys:

  for (unsigned int i = 0; i < cellviews (); ++i) {
    if (cellview (i).is_valid ()) {
      std::string name = cellview (i)->layout ().cell_name (cellview (i).cell_index ());
      writer.setText (tl::to_qstring ("Cell" + tl::to_string (int (i) + 1)), tl::to_qstring (name));
    }
  }

  lay::Viewport vp (width, height, mp_canvas->viewport ().target_box ());
  writer.setText (QString::fromUtf8 ("Rect"), tl::to_qstring (vp.box ().to_string ()));
  
  //  Execute all deferred methods - ensure there are no pending tasks
  tl::DeferredMethodScheduler::execute ();
  
  if (! writer.write (mp_canvas->image_with_options (width, height, linewidth, oversampling, resolution, background, foreground, active, target_box, monochrome))) {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to write screenshot to file: %s (%s)")), fn, tl::to_string (writer.errorString ()));
  }

  tl::log << "Saved screen shot to " << fn;
}

void
LayoutView::reload_layout (unsigned int cv_index)
{
  stop ();
  cancel (); 

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

  handle = new lay::LayoutHandle (new db::Layout (manager ()), filename);
  handle->set_tech_name (technology);
  cv_empty.set (handle);

  for (std::map <unsigned int, db::LayerProperties>::const_iterator ol = org_layers.begin (); ol != org_layers.end (); ++ol) {
    cv_empty->layout ().insert_layer (ol->first, ol->second);
  }
  cv_empty->rename (name, true);

  set_layout (cv_empty, cv_index);

  //  create a new handle
  lay::CellView cv;
  handle = new lay::LayoutHandle (new db::Layout (manager ()), filename);
  cv.set (handle);

  try {

    //  re-create the layers required
    for (std::map <unsigned int, db::LayerProperties>::const_iterator ol = org_layers.begin (); ol != org_layers.end (); ++ol) {
      cv->layout ().insert_layer (ol->first, ol->second);
    }
    
    {
      tl::log << tl::to_string (QObject::tr ("Loading file: ")) << filename;
      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Loading")));

      //  Load with the previous options again.
      db::LoadLayoutOptions options (cvorg->load_options ());
      cv->load (cvorg->load_options (), technology);
    }

    //  sort the layout explicitly here. Otherwise it would be done
    //  implicitly at some other time. This may throw an exception
    //  if the operation was cancelled.
    {
      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Sorting")));
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

unsigned int 
LayoutView::add_layout (lay::LayoutHandle *layout_handle, bool add_cellview, bool initialize_layers)
{
  unsigned int cv_index = 0;

  try {

    m_active_cellview_changed_event_enabled = false;

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
    } else {
      //  even if there is no cell, select the cellview item
      //  to support applications with an active cellview (that is however invalid)
      set_active_cellview_index (cv_index);
    }

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
      for (db::Layout::meta_info_iterator meta = cv->layout ().begin_meta (); meta != cv->layout ().end_meta (); ++meta) {
        if (meta->name == "layer-properties-file") {
          lyp_file = meta->value;
        }
        if (meta->name == "layer-properties-add-other-layers") {
          try {
            tl::from_string (meta->value, add_other_layers);
          } catch (...) {
          }
        }
      }

      //  interpolate the layout properties file name
      tl::Eval expr;
      expr.set_var ("layoutfile", layout_handle->filename ());
      lyp_file = expr.interpolate (lyp_file);

      //  create the initial layer properties
      create_initial_layer_props (cv_index, lyp_file, add_other_layers);

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

    m_active_cellview_changed_event_enabled = true;

  } catch (...) {

    update_content ();

    m_active_cellview_changed_event_enabled = true;
    throw;

  }

  //  this event may not be generated otherwise, hence force it now.
  active_cellview_changed (cv_index);

  return cv_index;
}

unsigned int 
LayoutView::create_layout (const std::string &technology, bool add_cellview, bool initialize_layers)
{
  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (technology);

  db::Layout *layout = new db::Layout (manager ());
  if (tech) {
    layout->dbu (tech->dbu ());
  }

  lay::LayoutHandle *handle = new lay::LayoutHandle (layout, "");
  handle->set_tech_name (technology);
  return add_layout (handle, add_cellview, initialize_layers);
}

unsigned int 
LayoutView::load_layout (const std::string &filename, const std::string &technology, bool add_cellview)
{
  return load_layout (filename, db::LoadLayoutOptions (), technology, add_cellview);
}

unsigned int 
LayoutView::load_layout (const std::string &filename, const db::LoadLayoutOptions &options, const std::string &technology, bool add_cellview)
{
  stop ();
  
  bool set_max_hier = (m_full_hier_new_cell || has_max_hier ());

  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (technology);

  //  create a new layout handle 
  lay::CellView cv;
  lay::LayoutHandle *handle = new lay::LayoutHandle (new db::Layout (manager ()), filename);
  cv.set (handle);

  unsigned int cv_index;
  db::LayerMap lmap;

  try {

    tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Loading")));

    //  load the file
    {
      tl::log << tl::to_string (QObject::tr ("Loading file: ")) << filename << tl::to_string (QObject::tr (" with technology: ")) << technology;
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

    m_active_cellview_changed_event_enabled = false;

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
    } else {
      //  even if there is no cell, select the cellview item
      //  to support applications with an active cellview (that is however invalid)
      set_active_cellview_index (cv_index);
    }

    bool add_other_layers = m_add_other_layers;

    //  Use the "layer-properties-file" meta info from the handle to get the layer properties file.
    //  If no such file is present, use the default file or the technology specific file.
    std::string lyp_file = m_def_lyp_file;
    if (tech && ! tech->eff_layer_properties_file ().empty ()) {
      lyp_file = tech->eff_layer_properties_file ();
      add_other_layers = tech->add_other_layers ();
    }

    //  Give the layout object a chance to specify a certain layer property file
    for (db::Layout::meta_info_iterator meta = cv->layout().begin_meta (); meta != cv->layout().end_meta (); ++meta) {
      if (meta->name == "layer-properties-file") {
        lyp_file = meta->value;
      }
      if (meta->name == "layer-properties-add-other-layers") {
        try {
          tl::from_string (meta->value, add_other_layers);
        } catch (...) {
        }
      }
    }

    //  interpolate the layout properties file name
    tl::Eval expr;
    expr.set_var ("layoutfile", filename);
    lyp_file = expr.interpolate (lyp_file);

    //  create the initial layer properties
    create_initial_layer_props (cv_index, lyp_file, add_other_layers);

    //  signal to any observers
    file_open_event ();

    //  do a fit and update layer lists etc.
    zoom_fit ();
    if (set_max_hier) {
      max_hier ();
    }
    update_content ();

    m_active_cellview_changed_event_enabled = true;

  } catch (...) {

    update_content ();

    m_active_cellview_changed_event_enabled = true;
    throw;

  }

  //  this event may not be generated otherwise, hence force it now.
  active_cellview_changed (cv_index);

  return cv_index;
}

void 
LayoutView::create_initial_layer_props (int cv_index, const std::string &lyp_file, bool add_missing)
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
        tl::log << tl::to_string (QObject::tr ("Loading layer properties file: ")) << lyp_file;
        lay::LayerPropertiesList::load (in, props);
        loaded = true;
      }

    } catch (tl::Exception &ex) {
      tl::warn << tl::to_string (QObject::tr ("Initialization of layers failed: ")) << ex.msg ();
    } catch (...) {
      tl::warn << tl::to_string (QObject::tr ("Initialization of layers failed: unspecific error"));
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
LayoutView::merge_layer_props (const std::vector<lay::LayerPropertiesList> &props)
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
LayoutView::pop_state ()
{
  if (m_display_state_ptr > 0) {
    m_display_states.erase (m_display_states.begin () + m_display_state_ptr, m_display_states.end ());
    --m_display_state_ptr;
  }
}

void
LayoutView::clear_states ()
{
  m_display_states.clear ();
  m_display_state_ptr = 0;
}

void
LayoutView::store_state ()
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
LayoutView::box () const
{
  return mp_canvas->viewport ().box ();
}

void
LayoutView::timer ()
{
  bool dirty = false;
  for (std::list<lay::CellView>::const_iterator i = m_cellviews.begin (); i != m_cellviews.end () && ! dirty; ++i) {
    dirty = (*i)->layout ().is_editable () && (*i)->is_dirty ();
  }

  if (dirty != m_dirty) {
    m_dirty = dirty;
    emit dirty_changed ();
  }

  if (m_animated) {
    set_view_ops ();
    if (mp_control_panel) {
      mp_control_panel->set_phase (int (m_phase));
    }
    if (m_animated) {
      ++m_phase;
    }
  }
}

bool
LayoutView::layer_model_updated ()
{
  //  because check_updated is called in the initialization phase, we check if the pointers
  //  to the widgets are non-null:
  if (mp_control_panel) {
    return mp_control_panel->model_updated ();
  } else {
    return false;
  }
}

void
LayoutView::force_update_content ()
{
  set_view_ops ();
}

void
LayoutView::update_content ()
{
  if (m_activated) {
    set_view_ops ();
  }
}

void
LayoutView::zoom_fit_sel ()
{
  db::DBox bbox = selection_bbox ();
  if (! bbox.empty ()) {
    bbox = db::DBox (bbox.left () - 0.025 * bbox.width (), bbox.bottom () - 0.025 * bbox.height (),
                     bbox.right () + 0.025 * bbox.width (), bbox.top () + 0.025 * bbox.height ());
    zoom_box (bbox);
  }
}

db::DBox
LayoutView::full_box () const
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
LayoutView::zoom_fit ()
{
  mp_canvas->zoom_box (full_box (), true /*precious*/);
  store_state ();
}

void
LayoutView::ensure_selection_visible ()
{
  ensure_visible (selection_bbox ());
}

void
LayoutView::ensure_visible (const db::DBox &bbox)
{
  db::DBox new_box = bbox + viewport ().box ();
  mp_canvas->zoom_box (new_box);
  store_state ();
}

void
LayoutView::zoom_box_and_set_hier_levels (const db::DBox &bbox, const std::pair<int, int> &levels)
{
  mp_canvas->zoom_box (bbox);
  set_hier_levels_basic (levels);
  store_state ();
}

void
LayoutView::zoom_box (const db::DBox &bbox)
{
  mp_canvas->zoom_box (bbox);
  store_state ();
}

void
LayoutView::set_global_trans (const db::DCplxTrans &trans)
{
  mp_canvas->set_global_trans (trans);
  store_state ();
}

void 
LayoutView::zoom_trans (const db::DCplxTrans &trans)
{
  mp_canvas->zoom_trans (trans);
  store_state ();
}

void
LayoutView::pan_left ()
{
  shift_window (1.0, -m_pan_distance, 0.0);
}

void
LayoutView::pan_right ()
{
  shift_window (1.0, m_pan_distance, 0.0);
}

void
LayoutView::pan_up ()
{
  shift_window (1.0, 0.0, m_pan_distance);
}

void
LayoutView::pan_down ()
{
  shift_window (1.0, 0.0, -m_pan_distance);
}

void
LayoutView::pan_left_fast ()
{
  shift_window (1.0, -m_pan_distance * fast_factor, 0.0);
}

void
LayoutView::pan_right_fast ()
{
  shift_window (1.0, m_pan_distance * fast_factor, 0.0);
}

void
LayoutView::pan_up_fast ()
{
  shift_window (1.0, 0.0, m_pan_distance * fast_factor);
}

void
LayoutView::pan_down_fast ()
{
  shift_window (1.0, 0.0, -m_pan_distance * fast_factor);
}

void
LayoutView::pan_center (const db::DPoint &p)
{
  db::DBox b = mp_canvas->viewport ().box ();
  db::DVector d (b.width () * 0.5, b.height () * 0.5);
  zoom_box (db::DBox (p - d, p + d));
}

void
LayoutView::zoom_in ()
{
  shift_window (zoom_factor, 0.0, 0.0);
}

void
LayoutView::zoom_out ()
{
  shift_window (1.0 / zoom_factor, 0.0, 0.0);
}

void
LayoutView::shift_window (double f, double dx, double dy)
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
LayoutView::goto_window (const db::DPoint &p, double s)
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
LayoutView::redraw_layer (unsigned int index)
{
  do_redraw (index);
}

void
LayoutView::redraw_cell_boxes ()
{
  do_redraw (lay::draw_boxes_queue_entry);
}

void
LayoutView::redraw_deco_layer ()
{
  //  redraw background annotations (images etc.)
  mp_canvas->touch_bg ();

  //  redraw other annotations:
  do_redraw (lay::draw_custom_queue_entry);
}

void
LayoutView::redraw ()
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
LayoutView::cancel_edits ()
{
  //  cancel all drag and pending edit operations such as move operations.
  mp_canvas->drag_cancel ();
  lay::Editables::cancel_edits ();
}

void
LayoutView::cancel ()
{
  //  cancel all drags and pending edit operations such as move operations.
  cancel_edits ();
  //  and clear the selection
  clear_selection ();
}

void
LayoutView::bookmark_current_view ()
{
  QString proposed_name = tl::to_qstring (m_bookmarks.propose_new_bookmark_name ());

  while (true) {
    bool ok = false;
    QString text = QInputDialog::getText (this, QObject::tr ("Enter Bookmark Name"), QObject::tr ("Bookmark name"),
                                          QLineEdit::Normal, proposed_name, &ok);
    if (! ok) {
      break;
    } else if (text.isEmpty ()) {
      QMessageBox::critical (this, QObject::tr ("Error"), QObject::tr ("Enter a name for the bookmark"));
    } else {
      bookmark_view (tl::to_string (text));
      break;
    }
  }
}

void
LayoutView::manage_bookmarks ()
{
  std::set<size_t> selected_bm;
  if (mp_bookmarks_frame->isVisible ()) {
    selected_bm = mp_bookmarks_view->selected_bookmarks ();
  }

  BookmarkManagementForm dialog (this, "bookmark_form", bookmarks (), selected_bm);
  if (dialog.exec ()) {
    bookmarks (dialog.bookmarks ());
  }
}

void 
LayoutView::bookmarks (const BookmarkList &b)
{
  m_bookmarks = b;
  mp_bookmarks_view->refresh ();
  emit menu_needs_update ();
}

void
LayoutView::bookmark_view (const std::string &name)
{
  DisplayState state (box (), get_min_hier_levels (), get_max_hier_levels (), m_cellviews);
  m_bookmarks.add (name, state);
  mp_bookmarks_view->refresh ();
  emit menu_needs_update ();
}

void
LayoutView::goto_bookmark ()
{
  BEGIN_PROTECTED

  QAction *action = dynamic_cast <QAction *> (sender ());
  tl_assert (action);
  size_t id = size_t (action->data ().toInt ());
  if (bookmarks ().size () > id) {
    goto_view (bookmarks ().state (id));
  }

  END_PROTECTED
}

void
LayoutView::goto_view (const DisplayState &state)
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
LayoutView::save_view (DisplayState &state) const
{
  state = DisplayState (box (), get_min_hier_levels (), get_max_hier_levels (), m_cellviews);
}

void
LayoutView::do_redraw (int layer)
{
  std::vector<int> layers;
  layers.push_back (layer);

  mp_canvas->redraw_selected (layers);
}

void 
LayoutView::do_prop_changed ()
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
LayoutView::layer_tab_changed ()
{
  update_content ();
}

void 
LayoutView::layer_order_changed ()
{
  update_content ();
}

void
LayoutView::set_view_ops ()
{
  bool bright_background = (mp_canvas->background_color ().green () > 128);
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

  lay::color_t box_color;
  if (! m_box_color.isValid ()) {
    box_color = mp_canvas->foreground_color ().rgb ();
  } else {
    box_color = m_box_color.rgb ();
  }

  //  cell boxes
  if (m_cell_box_visible) {

    lay::ViewOp vop;

    //  context level
    if (m_ctx_color.isValid ()) {
      vop = lay::ViewOp (m_ctx_color.rgb (), lay::ViewOp::Copy, 0, 0, 0);
    } else {
      vop = lay::ViewOp (lay::LayerProperties::brighter (box_color, brightness_for_context), lay::ViewOp::Copy, 0, 0, 0);
    }

    //  fill, frame, text, vertex
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
    view_ops.push_back (vop);
    view_ops.push_back (vop);
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));

    //  child level
    if (m_child_ctx_color.isValid ()) {
      vop = lay::ViewOp (m_child_ctx_color.rgb (), lay::ViewOp::Copy, 0, 0, 0);
    } else {
      vop = lay::ViewOp (lay::LayerProperties::brighter (box_color, brightness_for_context), lay::ViewOp::Copy, 0, 0, 0);
    }

    //  fill, frame, text, vertex
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));
    view_ops.push_back (vop);
    view_ops.push_back (vop);
    view_ops.push_back (lay::ViewOp (0, lay::ViewOp::Or, 0, 0, 0));

    //  current level
    vop = lay::ViewOp (box_color, lay::ViewOp::Copy, 0, 0, 0);

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

  color_t gs_color = box_color;
  if (m_guiding_shape_color.isValid ()) {
    gs_color = m_guiding_shape_color.rgb ();
  }

  for (int ctx = 0; ctx < 3; ++ctx) { // 0 (context), 1 (child), 2 (current)

    lay::ViewOp::Mode mode = lay::ViewOp::Copy;

    color_t fill_color, frame_color, text_color;
    int dp = 1; // no stipples for guiding shapes 

    if (ctx == 0) {

      //  context planes
      if (m_ctx_color.isValid ()) {
        frame_color = text_color = fill_color = m_ctx_color.rgb ();
      } else {
        frame_color = text_color = fill_color = lay::LayerProperties::brighter (gs_color, brightness_for_context);
      }

      if (m_ctx_hollow) {
        dp = 1;
      }

    } else if (ctx == 1) {

      //  child level planes (if used)
      if (m_child_ctx_color.isValid ()) {
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
          // inversly blinking
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

        color_t fill_color, frame_color, text_color;
        int dp = m_no_stipples ? 1 : l->dither_pattern (true /*real*/);
        int ls = l->line_style (true /*real*/);

        if (ctx == 0) {

          //  context planes
          if (m_ctx_color.isValid ()) {
            frame_color = text_color = fill_color = m_ctx_color.rgb ();
          } else {
            fill_color = l->eff_fill_color_brighter (true /*real*/, brightness_for_context);
            frame_color = l->eff_frame_color_brighter (true /*real*/, brightness_for_context);
            if (m_text_color.isValid ()) {
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
          if (m_child_ctx_color.isValid ()) {
            frame_color = text_color = fill_color = m_child_ctx_color.rgb ();
          } else {
            fill_color = l->eff_fill_color_brighter (true /*real*/, brightness_for_child_context);
            frame_color = l->eff_frame_color_brighter (true /*real*/, brightness_for_child_context);
            if (m_text_color.isValid ()) {
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
          if (m_text_color.isValid ()) {
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
LayoutView::guiding_shapes_visible (bool v)
{
  if (v != m_guiding_shape_visible) {
    m_guiding_shape_visible = v;
    update_content ();
  }
}

void
LayoutView::guiding_shapes_color (QColor c)
{
  if (c != m_guiding_shape_color) {
    m_guiding_shape_color = c;
    update_content ();
  }
}

void
LayoutView::guiding_shapes_line_width (int v)
{
  if (v != m_guiding_shape_line_width) {
    m_guiding_shape_line_width = v;
    update_content ();
  }
}

void
LayoutView::guiding_shapes_vertex_size (int v)
{
  if (v != m_guiding_shape_vertex_size) {
    m_guiding_shape_vertex_size = v;
    update_content ();
  }
}

void 
LayoutView::draw_array_border_instances (bool m)
{
  if (m != m_draw_array_border_instances) {
    m_draw_array_border_instances = m;
    redraw ();
  }
}

void 
LayoutView::drop_small_cells (bool m)
{
  if (m != m_drop_small_cells) {
    m_drop_small_cells = m;
    redraw ();
  }
}

void 
LayoutView::drop_small_cells_value (unsigned int s)
{
  if (s != m_drop_small_cells_value) {
    m_drop_small_cells_value = s;
    redraw ();
  }
}

void 
LayoutView::drop_small_cells_cond (drop_small_cells_cond_type t)
{
  if (t != m_drop_small_cells_cond) {
    m_drop_small_cells_cond = t;
    redraw ();
  }
}

void 
LayoutView::cell_box_color (QColor c)
{
  if (c != m_box_color) {
    m_box_color = c;
    update_content ();
  }
}

void 
LayoutView::cell_box_text_transform (bool xform)
{
  if (xform != m_box_text_transform) {
    m_box_text_transform = xform;
    redraw ();
  } 
}

void 
LayoutView::cell_box_text_font (unsigned int f)
{
  if (f != m_box_font) {
    m_box_font = f;
    redraw ();
  } 
}

bool
LayoutView::set_hier_levels_basic (std::pair<int, int> l)
{
  if (l != get_hier_levels ()) {

    if (mp_min_hier_spbx) {
      mp_min_hier_spbx->blockSignals (true);
      mp_min_hier_spbx->setValue (l.first);
      mp_min_hier_spbx->setMaximum (l.second);
      mp_min_hier_spbx->blockSignals (false);
    }

    if (mp_max_hier_spbx) {
      mp_max_hier_spbx->blockSignals (true);
      mp_max_hier_spbx->setValue (l.second);
      mp_max_hier_spbx->setMinimum (l.first);
      mp_max_hier_spbx->blockSignals (false);
    }

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
LayoutView::set_hier_levels (std::pair<int, int> l)
{
  if (set_hier_levels_basic (l)) {
    store_state ();
  } 
}

std::pair<int, int> 
LayoutView::get_hier_levels () const
{
  return std::make_pair (m_from_level, m_to_level);
}

void 
LayoutView::min_hier_changed (int i)
{
  mp_max_hier_spbx->setMinimum (i);
  set_hier_levels (std::make_pair (i, m_to_level));
}

void 
LayoutView::max_hier_changed (int i)
{
  mp_min_hier_spbx->setMaximum (i);
  set_hier_levels (std::make_pair (m_from_level, i));
}

/**
 *  @brief set the maximum hierarchy level to the number of levels available
 */
void 
LayoutView::max_hier ()
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
LayoutView::max_hier_level () const
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
LayoutView::has_max_hier () const
{
  int ml = max_hier_level ();
  return ml > 0 && m_to_level >= ml;
}

void
LayoutView::set_palette (const lay::ColorPalette &p)
{
  m_palette = p;
}

void
LayoutView::set_palette (const lay::StipplePalette &p)
{
  m_stipple_palette = p;
}

void
LayoutView::set_palette (const lay::LineStylePalette &p)
{
  m_line_style_palette = p;
}

void
LayoutView::ctx_color (QColor c)
{
  if (c != m_ctx_color) {
    m_ctx_color = c;
    update_content ();
  }
}

void
LayoutView::ctx_dimming (int d)
{
  if (d != m_ctx_dimming) {
    m_ctx_dimming = d;
    update_content ();
  }
}

void
LayoutView::ctx_hollow (bool h)
{
  if (h != m_ctx_hollow) {
    m_ctx_hollow = h;
    update_content ();
  }
}

void
LayoutView::child_ctx_color (QColor c)
{
  if (c != m_child_ctx_color) {
    m_child_ctx_color = c;
    update_content ();
  }
}

void
LayoutView::child_ctx_dimming (int d)
{
  if (d != m_child_ctx_dimming) {
    m_child_ctx_dimming = d;
    update_content ();
  }
}

void
LayoutView::child_ctx_hollow (bool h)
{
  if (h != m_child_ctx_hollow) {
    m_child_ctx_hollow = h;
    update_content ();
  }
}

void
LayoutView::child_ctx_enabled (bool f)
{
  if (f != m_child_ctx_enabled) {
    m_child_ctx_enabled = f;
    update_content ();
    redraw ();
  }
}

void
LayoutView::abstract_mode_width (double w)
{
  if (fabs (w - m_abstract_mode_width) > 1e-6) {
    m_abstract_mode_width = w;
    if (m_abstract_mode_enabled) {
      redraw ();
    }
  }
}

void
LayoutView::abstract_mode_enabled (bool e)
{
  if (e != m_abstract_mode_enabled) {
    m_abstract_mode_enabled = e;
    redraw ();
  }
}

void 
LayoutView::background_color (QColor c)
{
  if (c == mp_canvas->background_color ()) {
    return;
  }

  //  replace by "real" background color if required
  if (! c.isValid ()) {
    c = palette ().color (QPalette::Normal, QPalette::Base);
  }

  QColor contrast;
  if (c.green () > 128) {
    contrast = QColor (0, 0, 0);
  } else {
    contrast = QColor (255, 255, 255);
  }

  if (mp_control_panel) {
    mp_control_panel->set_background_color (c);
    mp_control_panel->set_text_color (contrast);
  }

  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->set_background_color (c);
    mp_hierarchy_panel->set_text_color (contrast);
  }

  if (mp_libraries_view) {
    mp_libraries_view->set_background_color (c);
    mp_libraries_view->set_text_color (contrast);
  }

  if (mp_bookmarks_view) {
    mp_bookmarks_view->set_background_color (c);
    mp_bookmarks_view->set_text_color (contrast);
  }

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
LayoutView::dbu_coordinates (bool f) 
{
  m_dbu_coordinates = f;
}

void 
LayoutView::absolute_coordinates (bool f) 
{
  m_absolute_coordinates = f;
}

void 
LayoutView::select_cellviews_fit (const std::list <CellView> &cvs)
{
  if (m_cellviews != cvs) {

    for (int index = 0; index < int (m_cellviews.size ()); ++index) {
      cellview_about_to_change_event (index);
    }

    cellviews_about_to_change_event ();

    set_min_hier_levels (0);
    cancel (); 
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
LayoutView::active_cellview_changed (int index)
{
  if (m_active_cellview_changed_event_enabled) {

    active_cellview_changed_event ();
    active_cellview_changed_with_index_event (index);

    //  Because the title reflects the active one, emit a title changed event
    if (m_title.empty ()) {
      emit title_changed ();
    }

  }
}

void
LayoutView::active_library_changed (int /*index*/)
{
  std::string lib_name;
  if (mp_libraries_view->active_lib ()) {
    lib_name = mp_libraries_view->active_lib ()->get_name ();
  }

  //  commit the new active library to the other views and persist this state
  //  TODO: could be passed through the LibraryController (like through some LibraryController::active_library)
  plugin_root ()->config_set (cfg_current_lib_view, lib_name);
}

void
LayoutView::cellview_changed (unsigned int index)
{
  mp_hierarchy_panel->do_update_content (index);

  cellview_changed_event (index);

  if (m_title.empty ()) {
    emit title_changed ();
  }
}

void 
LayoutView::select_cell_dispatch (const cell_path_type &path, int cellview_index)
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
LayoutView::select_cell_fit (const cell_path_type &path, int index)
{ 
  if (index >= 0 && int (m_cellviews.size ()) > index && (cellview_iter (index)->specific_path ().size () > 0 || cellview_iter (index)->unspecific_path () != path)) {

    cellview_about_to_change_event (index);

    set_min_hier_levels (0);
    cancel (); 
    cellview_iter (index)->set_unspecific_path (path);
    set_active_cellview_index (index);
    redraw ();
    zoom_fit ();

    cellview_changed (index);

    update_content ();

  }
}

void 
LayoutView::select_cell_fit (cell_index_type cell_index, int index)
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
LayoutView::select_cellviews (const std::list <CellView> &cvs)
{
  if (m_cellviews != cvs) {

    for (int index = 0; index < int (m_cellviews.size ()); ++index) {
      cellview_about_to_change_event (index);
    }
    cellviews_about_to_change_event ();

    set_min_hier_levels (0);
    cancel (); 
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
LayoutView::select_cellview (int index, const CellView &cv)
{
  if (index < 0 || index >= int (m_cellviews.size ())) {
    return;
  }

  if (*cellview_iter (index) != cv) {

    cellview_about_to_change_event (index);

    cancel ();
    *cellview_iter (index) = cv;
    redraw ();

    cellview_changed (index);

    update_content ();

  }
}

void
LayoutView::select_cell (const cell_path_type &path, int index)
{ 
  if (index >= 0 && int (m_cellviews.size ()) > index && (cellview_iter (index)->specific_path ().size () > 0 || cellview_iter (index)->unspecific_path () != path)) {

    cellview_about_to_change_event (index);

    set_min_hier_levels (0);
    cancel (); 
    cellview_iter (index)->set_unspecific_path (path);
    set_active_cellview_index (index);
    redraw ();

    cellview_changed (index);

    update_content ();

  }
}

void 
LayoutView::select_cell (cell_index_type cell_index, int index)
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
LayoutView::is_cell_hidden (cell_index_type ci, int cellview_index) const
{
  if (int (m_hidden_cells.size ()) > cellview_index && cellview_index >= 0) {
    return m_hidden_cells [cellview_index].find (ci) != m_hidden_cells [cellview_index].end ();
  } else {
    return false;
  }
}

const std::set<LayoutView::cell_index_type> &
LayoutView::hidden_cells (int cellview_index) const
{
  if (int (m_hidden_cells.size ()) > cellview_index && cellview_index >= 0) {
    return m_hidden_cells[cellview_index];
  } else {
    static std::set<cell_index_type> empty_set;
    return empty_set;
  }
}

void 
LayoutView::hide_cell (cell_index_type ci, int cellview_index)
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
LayoutView::show_cell (cell_index_type ci, int cellview_index)
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
LayoutView::show_all_cells (int cv_index)
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
LayoutView::show_all_cells ()
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
LayoutView::min_inst_label_size (int px)
{
  if (m_min_size_for_label != px) {
    m_min_size_for_label = px;
    redraw ();
  }
}

void 
LayoutView::text_visible (bool vis)
{
  if (m_text_visible != vis) {
    m_text_visible = vis;
    update_content ();
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutView::show_properties_as_text (bool sp)
{
  if (m_show_properties != sp) {
    m_show_properties = sp;
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutView::bitmap_caching (bool l)
{
  if (m_bitmap_caching != l) {
    m_bitmap_caching = l;
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutView::text_lazy_rendering (bool l)
{
  if (m_text_lazy_rendering != l) {
    m_text_lazy_rendering = l;
    redraw (); //  required because we do some optimizations is text is not visible ..
  }
}

void 
LayoutView::cell_box_visible (bool vis)
{
  if (m_cell_box_visible != vis) {
    m_cell_box_visible = vis;
    update_content ();
  }
}

void 
LayoutView::text_font (unsigned int f)
{
  if (m_text_font != f) {
    m_text_font = f;
    redraw ();
  }
}

void 
LayoutView::default_text_size (double fs)
{
  if (m_default_text_size != fs) {
    m_default_text_size = fs;
    redraw ();
  }
}

void 
LayoutView::clear_ruler_new_cell (bool f)
{
  m_clear_ruler_new_cell = f;
}
  
void 
LayoutView::full_hier_new_cell (bool f)
{
  m_full_hier_new_cell = f;
}

double
LayoutView::pan_distance () const
{
  return m_pan_distance;
}

void
LayoutView::pan_distance (double pd) 
{
  m_pan_distance = pd;
}

void 
LayoutView::fit_new_cell (bool f)
{
  m_fit_new_cell = f;
}
  
void 
LayoutView::apply_text_trans (bool f)
{
  if (m_apply_text_trans != f) {
    m_apply_text_trans = f;
    redraw ();
  }
}
  
void 
LayoutView::offset_stipples (bool f)
{
  if (m_stipple_offset != f) {
    m_stipple_offset = f;
    update_content ();
  }
}
  
void 
LayoutView::no_stipples (bool f)
{
  if (m_no_stipples != f) {
    m_no_stipples = f;
    if (mp_control_panel) {
      mp_control_panel->set_no_stipples (m_no_stipples);
    }
    update_content ();
  }
}
  
void
LayoutView::show_markers (bool f)
{
  if (m_show_markers != f) {
    m_show_markers = f;
    mp_canvas->update_image ();
  }
}

void
LayoutView::text_color (QColor c)
{
  if (m_text_color != c) {
    m_text_color = c;
    update_content ();
  }
}

bool
LayoutView::has_selection ()
{
  if (mp_control_panel && mp_control_panel->has_focus ()) {
    return mp_control_panel->has_selection ();
  } else if (mp_hierarchy_panel && mp_hierarchy_panel->has_focus ()) {
    return mp_hierarchy_panel->has_selection ();
  } else {
    return lay::Editables::selection_size () > 0;
  }
}

void
LayoutView::paste ()
{
  clear_selection ();

  {
    db::Transaction trans (manager (), tl::to_string (QObject::tr ("Paste")));

    //  let the receivers sort out who is pasting what ..
    if (mp_hierarchy_panel) {
      mp_hierarchy_panel->paste ();
    }
    if (mp_control_panel) {
      mp_control_panel->paste ();
    }
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
LayoutView::paste_interactive ()
{
  clear_selection ();

  std::auto_ptr<db::Transaction> trans (new db::Transaction (manager (), tl::to_string (QObject::tr ("Paste and move"))));

  {
    //  let the receivers sort out who is pasting what ..
    if (mp_hierarchy_panel) {
      mp_hierarchy_panel->paste ();
    }
    if (mp_control_panel) {
      mp_control_panel->paste ();
    }
    lay::Editables::paste ();
  }

  //  temporarily close the transaction and pass to the move service for appending it's own
  //  operations.
  trans->close ();

  if (mp_move_service->begin_move (trans.release (), false)) {
    switch_mode (-1);  //  move mode
  }
}

void
LayoutView::copy ()
{
  if (mp_hierarchy_panel && mp_hierarchy_panel->has_focus ()) {
    mp_hierarchy_panel->copy ();
  } else if (mp_control_panel && mp_control_panel->has_focus ()) {
    mp_control_panel->copy ();
  } else {

    if (lay::Editables::selection_size () == 0) {
      //  try to use the transient selection for the real one
      lay::Editables::transient_to_selection ();
    }

    lay::Editables::copy ();

  }
}

void
LayoutView::cut ()
{
  if (mp_hierarchy_panel && mp_hierarchy_panel->has_focus ()) {
    //  TODO: currently the hierarchy panel's cut function does it's own transaction handling.
    //  Otherwise the cut function is not working propertly.
    mp_hierarchy_panel->cut ();
  } else if (mp_control_panel && mp_control_panel->has_focus ()) {
    db::Transaction trans (manager (), tl::to_string (QObject::tr ("Cut Layers")));
    mp_control_panel->cut ();
  } else {

    if (lay::Editables::selection_size () == 0) {
      //  try to use the transient selection for the real one
      lay::Editables::transient_to_selection ();
    }

    db::Transaction trans (manager (), tl::to_string (QObject::tr ("Cut")));
    lay::Editables::cut ();

  }
}

void 
LayoutView::cm_align_cell_origin ()
{
  int cv_index = active_cellview_index ();
  if (cv_index >= 0) {

    const db::Cell *cell = cellview (cv_index).cell ();
    if (! cell) {
      return;
    }
    if (cell->is_proxy ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Cannot use this function on a PCell or library cell")));
    }

    lay::AlignCellOptionsDialog dialog (this);
    if (dialog.exec_dialog (m_align_cell_origin_mode_x, m_align_cell_origin_mode_y, m_align_cell_origin_visible_layers, m_align_cell_adjust_parents)) {

      clear_selection ();

      if (manager ()) {
        manager ()->transaction (tl::to_string (QObject::tr ("Align cell origin"))); 
      }

      db::Box bbox; 

      if (m_align_cell_origin_visible_layers) {
        for (lay::LayerPropertiesConstIterator l = begin_layers (); !l.at_end (); ++l) {
          if (! l->has_children () && l->layer_index () >= 0 && l->cellview_index () == cv_index && l->visible (true /*real*/)) {
            bbox += cell->bbox (l->layer_index ());
          }
        }
      } else {
        bbox = cell->bbox ();
      }

      db::Coord refx, refy;
      switch (m_align_cell_origin_mode_x) {
      case -1:
        refx = bbox.left ();
        break;
      case 1:
        refx = bbox.right ();
        break;
      default:
        refx = bbox.center ().x ();
        break;
      }
      switch (m_align_cell_origin_mode_y) {
      case -1:
        refy = bbox.bottom ();
        break;
      case 1:
        refy = bbox.top ();
        break;
      default:
        refy = bbox.center ().y ();
        break;
      }

      db::Trans t (db::Vector (-refx, -refy));
      db::Layout &layout = cellview (cv_index)->layout ();
      db::Cell &nc_cell = layout.cell (cell->cell_index ());

      for (unsigned int i = 0; i < layout.layers (); ++i) {
        if (layout.is_valid_layer (i)) {
          db::Shapes &shapes = nc_cell.shapes (i);
          for (db::Shapes::shape_iterator s = shapes.begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
            shapes.transform (*s, t);
          }
        }
      }

      for (db::Cell::const_iterator inst = nc_cell.begin (); ! inst.at_end (); ++inst) {
        nc_cell.transform (*inst, t);
      }

      if (m_align_cell_adjust_parents) {

        std::vector<std::pair<db::Cell *, db::Instance> > insts_to_modify;
        for (db::Cell::parent_inst_iterator pi = nc_cell.begin_parent_insts (); ! pi.at_end (); ++pi) {
          insts_to_modify.push_back (std::make_pair (& layout.cell (pi->parent_cell_index ()), pi->child_inst ()));
        }

        db::Trans ti (db::Vector (refx, refy));
        for (std::vector<std::pair<db::Cell *, db::Instance> >::const_iterator im = insts_to_modify.begin (); im != insts_to_modify.end (); ++im) {
          im->first->transform (im->second, db::Trans (db::Vector (im->second.complex_trans ().trans (db::Vector (refx, refy)))));
        }

      }

      if (manager ()) {
        manager ()->commit ();
      }

    }

  }
}

void
LayoutView::cm_cell_user_properties ()
{
  if (! mp_hierarchy_panel) {
    return;
  }

  int cv_index = active_cellview_index ();
  cell_path_type path;
  mp_hierarchy_panel->current_cell (cv_index, path);

  if (cv_index >= 0 && path.size () > 0) {

    db::Layout &layout = cellview (cv_index)->layout ();
    db::Cell &cell = layout.cell (path.back ());
    db::properties_id_type prop_id = cell.prop_id ();

    lay::UserPropertiesForm props_form (this);
    if (props_form.show (this, cv_index, prop_id)) {

      if (manager ()) {
        manager ()->transaction (tl::to_string (QObject::tr ("Edit cell's user propertes"))); 
        cell.prop_id (prop_id);
        manager ()->commit ();
      } else {
        cell.prop_id (prop_id);
      }

    }

  }
}

void
LayoutView::cm_cell_replace ()
{
  if (! mp_hierarchy_panel) {
    return;
  }

  int cv_index = active_cellview_index ();
  std::vector<cell_path_type> paths;
  mp_hierarchy_panel->selected_cells (cv_index, paths);

  if (cv_index >= 0 && paths.size () > 0) {

    if (paths.size () > 1) {
      throw tl::Exception (tl::to_string (QObject::tr ("Replace cell cannot be used when multiple cells are selected")));
    }

    db::Layout &layout = cellview (cv_index)->layout ();

    bool needs_to_ask = false;
    for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end () && ! needs_to_ask; ++p) {
      if (layout.is_valid_cell_index (p->back ()) && ! layout.cell (p->back ()).is_leaf ()) {
        needs_to_ask = true;
      }
    }


    lay::ReplaceCellOptionsDialog mode_dialog (this);

    db::cell_index_type with_cell = paths.front ().back ();
    int mode = needs_to_ask ? m_del_cell_mode : 0;

    if (mode_dialog.exec_dialog (cellview (cv_index), mode, with_cell)) {

      if (needs_to_ask) {
        m_del_cell_mode = mode;
      }

      if (with_cell != paths.front ().back ()) {

        //  remember the current path
        cell_path_type cell_path (cellview (cv_index).combined_unspecific_path ());

        clear_selection ();

        manager ()->transaction (tl::to_string (QObject::tr ("Replace cells"))); 

        //  replace instances of the target cell with the new cell 

        db::Cell &target_cell = layout.cell (paths.front ().back ());

        std::vector<std::pair<db::cell_index_type, db::Instance> > parents;
        for (db::Cell::parent_inst_iterator pi = target_cell.begin_parent_insts (); ! pi.at_end (); ++pi) {
          parents.push_back (std::make_pair (pi->parent_cell_index (), pi->child_inst ()));
        }

        for (std::vector<std::pair<db::cell_index_type, db::Instance> >::const_iterator p = parents.begin (); p != parents.end (); ++p) {
          db::CellInstArray ia = p->second.cell_inst ();
          ia.object ().cell_index (with_cell);
          layout.cell (p->first).replace (p->second, ia);
        }

        std::set<db::cell_index_type> cells_to_delete;
        for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
          if (! p->empty () && layout.is_valid_cell_index (p->back ())) {
            cells_to_delete.insert (p->back ());
            if (mode == 2) {
              layout.cell (p->back ()).collect_called_cells (cells_to_delete);
            }
          }
        }

        //  support a propagation use case:
        std::set<db::cell_index_type> cells_below_replacement_cell;
        cells_below_replacement_cell.insert (with_cell);
        layout.cell (with_cell).collect_called_cells (cells_below_replacement_cell);
        for (std::set<db::cell_index_type>::const_iterator c = cells_below_replacement_cell.begin (); c != cells_below_replacement_cell.end (); ++c) {
          cells_to_delete.erase (*c);
        }

        if (mode == 0 || mode == 2) {
          layout.delete_cells (cells_to_delete);
        } else if (mode == 1) {
          layout.prune_cells (cells_to_delete);
        }

        layout.cleanup ();

        manager ()->commit ();

        //  If one of the cells in the path was deleted, establish a valid path

        bool needs_update = false;
        for (size_t i = cell_path.size (); i > 0; ) {
          --i;
          if (! layout.is_valid_cell_index (cell_path [i])) {
            cell_path.erase (cell_path.begin () + i, cell_path.end ());
            needs_update = true;
          }
        }

        if (needs_update) {
          select_cell (cell_path, cv_index);
        }

      }

    }

  }
}

void
LayoutView::cm_lay_convert_to_static ()
{
  //  end move operations, cancel edit operations
  cancel_edits ();
  clear_selection ();

  int cv_index = active_cellview_index ();
  if (cv_index >= 0) {

    db::Layout &layout = cellview (cv_index)->layout ();

    manager ()->transaction (tl::to_string (QObject::tr ("Convert all cells to static"))); 

    std::vector<db::cell_index_type> cells;
    for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {
      cells.push_back (c->cell_index ());
    }

    std::map<db::cell_index_type, db::cell_index_type> cell_map;
    for (std::vector<db::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
      if (layout.is_valid_cell_index (*c)) {
        db::cell_index_type new_cell = layout.convert_cell_to_static (*c);
        if (new_cell != *c) {
          cell_map.insert (std::make_pair (*c, new_cell));
        }
      }
    }

    //  rewrite instances
    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
      for (db::Cell::const_iterator i = c->begin (); ! i.at_end (); ++i) {
        std::map<db::cell_index_type, db::cell_index_type>::const_iterator cm = cell_map.find (i->cell_index ());
        if (cm != cell_map.end ()) {
          db::CellInstArray ci = i->cell_inst ();
          ci.object ().cell_index (cm->second);
          c->replace (*i, ci);
        }
      }
    }

    layout.cleanup ();

    manager ()->commit ();

  }
}

void
LayoutView::cm_cell_convert_to_static ()
{
  if (! mp_hierarchy_panel) {
    return;
  }

  int cv_index = active_cellview_index ();
  std::vector<cell_path_type> paths;
  mp_hierarchy_panel->selected_cells (cv_index, paths);

  if (cv_index >= 0 && paths.size () > 0) {

    db::Layout &layout = cellview (cv_index)->layout ();

    //  remember the current path
    cell_path_type cell_path (cellview (cv_index).combined_unspecific_path ());

    clear_selection ();

    manager ()->transaction (tl::to_string (QObject::tr ("Convert cells to static"))); 

    std::map<db::cell_index_type, db::cell_index_type> cell_map;

    for (std::vector<cell_path_type>::iterator p = paths.begin (); p != paths.end (); ++p) {
      if (! p->empty () && layout.is_valid_cell_index (p->back ())) {
        db::cell_index_type new_cell = layout.convert_cell_to_static (p->back ());
        if (new_cell != p->back ()) {
          cell_map.insert (std::make_pair (p->back (), new_cell));
          p->back () = new_cell;
        }
      }
    }

    //  rewrite instances
    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
      for (db::Cell::const_iterator i = c->begin (); ! i.at_end (); ++i) {
        std::map<db::cell_index_type, db::cell_index_type>::const_iterator cm = cell_map.find (i->cell_index ());
        if (cm != cell_map.end ()) {
          db::CellInstArray ci = i->cell_inst ();
          ci.object ().cell_index (cm->second);
          c->replace (*i, ci);
        }
      }
    }

    layout.cleanup ();

    manager ()->commit ();

    //  If one of the cells in the path was deleted, establish a valid path

    bool needs_update = false;
    for (size_t i = cell_path.size (); i > 0; ) {
      --i;
      if (! layout.is_valid_cell_index (cell_path [i])) {
        cell_path.erase (cell_path.begin () + i, cell_path.end ());
        needs_update = true;
      }
    }

    if (needs_update) {
      select_cell (cell_path, cv_index);
    }

  }
}

static void
collect_cells_to_delete (const db::Layout &layout, const db::Cell &cell, std::set<db::cell_index_type> &called)
{
  //  don't delete proxies - they are deleted later when the layout is cleaned
  for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); ! cc.at_end (); ++cc) {
    if (called.find (*cc) == called.end () && !layout.cell (*cc).is_proxy ()) {
      called.insert (*cc);
      collect_cells_to_delete (layout, layout.cell (*cc), called);
    }
  }
}

void
LayoutView::cm_cell_delete ()
{
  if (! mp_hierarchy_panel) {
    return;
  }

  int cv_index = active_cellview_index ();
  std::vector<cell_path_type> paths;
  mp_hierarchy_panel->selected_cells (cv_index, paths);

  if (cv_index >= 0 && paths.size () > 0) {

    db::Layout &layout = cellview (cv_index)->layout ();

    bool needs_to_ask = false;
    for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end () && ! needs_to_ask; ++p) {
      if (layout.is_valid_cell_index (p->back ()) && ! layout.cell (p->back ()).is_leaf ()) {
        needs_to_ask = true;
      }
    }

    int mode = m_del_cell_mode;
    if (! needs_to_ask) {
      mode = 0;
    }

    lay::DeleteCellModeDialog mode_dialog (this);
    if (! needs_to_ask || mode_dialog.exec_dialog (mode)) {

      if (needs_to_ask) {
        m_del_cell_mode = mode;
      }

      //  remember the current path
      cell_path_type cell_path (cellview (cv_index).combined_unspecific_path ());

      clear_selection ();

      std::set<db::cell_index_type> cells_to_delete;
      for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
        if (! p->empty () && layout.is_valid_cell_index (p->back ())) {
          cells_to_delete.insert (p->back ());
          if (mode == 2) {
            collect_cells_to_delete (layout, layout.cell (p->back ()), cells_to_delete);
          }
        }
      }

      manager ()->transaction (tl::to_string (QObject::tr ("Delete cells"))); 

      if (mode == 0 || mode == 2) {
        layout.delete_cells (cells_to_delete);
      } else if (mode == 1) {
        layout.prune_cells (cells_to_delete);
      }

      layout.cleanup ();

      manager ()->commit ();

      //  If one of the cells in the path was deleted, establish a valid path

      bool needs_update = false;
      for (size_t i = cell_path.size (); i > 0; ) {
        --i;
        if (! layout.is_valid_cell_index (cell_path [i])) {
          cell_path.erase (cell_path.begin () + i, cell_path.end ());
          needs_update = true;
        }
      }

      if (needs_update) {
        select_cell (cell_path, cv_index);
      }

    }

  }
}

void 
LayoutView::cm_layer_copy ()
{
  if (mp_control_panel) {
    mp_control_panel->copy ();
  }
}

void 
LayoutView::cm_layer_cut ()
{
  if (mp_control_panel) {
    db::Transaction trans (manager (), tl::to_string (QObject::tr ("Cut Layers"))); 
    mp_control_panel->cut ();
  }
}

void 
LayoutView::cm_layer_paste ()
{
  if (mp_control_panel) {
    db::Transaction trans (manager (), tl::to_string (QObject::tr ("Paste Layers"))); 
    mp_control_panel->paste ();
  }
}

void
LayoutView::cm_cell_cut ()
{
  if (mp_hierarchy_panel) {
    //  TODO: currently the hierarchy panel's cut function does it's own transaction handling.
    //  Otherwise the cut function is not working propertly.
    mp_hierarchy_panel->cut ();
  }
}

void
LayoutView::cm_cell_paste ()
{
  if (mp_hierarchy_panel) {
    db::Transaction trans (manager (), tl::to_string (QObject::tr ("Paste Cells"))); 
    mp_hierarchy_panel->paste ();
  }
}

void
LayoutView::cm_cell_copy ()
{
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->copy ();
  }
}

void
LayoutView::cm_cell_flatten ()
{
  if (! mp_hierarchy_panel) {
    return;
  }

  tl_assert (is_editable ());

  int cv_index = active_cellview_index ();
  if (cv_index >= 0) {

    const lay::CellView &cv = cellview (cv_index);
    if (cv.is_valid ()) {

      std::vector<HierarchyControlPanel::cell_path_type> paths;
      mp_hierarchy_panel->selected_cells (cv_index, paths);
      if (paths.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("No cells selected for flattening")));
      }

      for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
        if (p->size () > 0 && cv->layout ().cell (p->back ()).is_proxy ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("Cannot use this function on a PCell or library cell")));
        }
      }

      FlattenInstOptionsDialog options_dialog (this);

      int flatten_insts_levels = -1;
      bool prune = true;
      if (options_dialog.exec_dialog (flatten_insts_levels, prune) && flatten_insts_levels != 0) {

        bool supports_undo = true;

        if (db::transactions_enabled ()) {

          lay::TipDialog td (QApplication::activeWindow (), 
                             tl::to_string (QObject::tr ("Undo buffering for the following operation can be memory and time consuming.\nChoose \"Yes\" to use undo buffering or \"No\" for no undo buffering. Warning: in the latter case, the undo history will be lost.\n\nChoose undo buffering?")), 
                             "flatten-undo-buffering",
                             lay::TipDialog::yesnocancel_buttons);

          lay::TipDialog::button_type button = lay::TipDialog::null_button;
          td.exec_dialog (button);
          if (button == lay::TipDialog::cancel_button) {
            return;
          }

          supports_undo = (button == lay::TipDialog::yes_button);

        } else {
          supports_undo = false;
        }

        cancel_edits ();
        clear_selection ();

        if (manager ()) {
          if (! supports_undo) {
            manager ()->clear ();
          } else {
            manager ()->transaction (tl::to_string (QObject::tr ("Flatten cell")));
          }
        }

        db::Layout &layout = cv->layout ();

        std::set<db::cell_index_type> child_cells;
        for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
          if (p->size () > 0) {
            layout.cell (p->back ()).collect_called_cells (child_cells);
          }
        }

        //  don't flatten cells which are child cells of the cells to flatten
        std::set<db::cell_index_type> cells_to_flatten;
        for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
          if (p->size () > 0 && child_cells.find (p->back ()) == child_cells.end ()) {
            cells_to_flatten.insert (p->back ());
          }
        }

        for (std::set<db::cell_index_type>::const_iterator c = cells_to_flatten.begin (); c != cells_to_flatten.end (); ++c) {
          db::Cell &target_cell = layout.cell (*c);
          layout.flatten (target_cell, flatten_insts_levels, prune);
        }

        layout.cleanup ();

        if (supports_undo && manager ()) {
          manager ()->commit ();
        }

      }

    }

  }
}

void
LayoutView::cm_cell_rename ()
{
  if (! mp_hierarchy_panel) {
    return;
  }

  int cv_index = active_cellview_index ();
  cell_path_type path;
  mp_hierarchy_panel->current_cell (cv_index, path);

  if (cv_index >= 0 && path.size () > 0) {

    lay::RenameCellDialog name_dialog (this);

    db::Layout &layout = cellview (cv_index)->layout ();
    std::string name (layout.cell_name (path.back ()));
    if (name_dialog.exec_dialog (layout, name)) {

      if (manager ()) {
        manager ()->transaction (tl::to_string (QObject::tr ("Rename cell"))); 
      }

      layout.rename_cell (path.back (), name.c_str ());

      if (manager ()) {
        manager ()->commit ();
      }

    }

  }
}

void
LayoutView::cm_cell_select ()
{
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->cm_cell_select ();
  }
}

void
LayoutView::cm_open_current_cell ()
{
  set_current_cell_path (active_cellview_index (), cellview (active_cellview_index ()).combined_unspecific_path ());
}

void
LayoutView::cm_cell_hide ()
{
  if (mp_hierarchy_panel) {

    std::vector<HierarchyControlPanel::cell_path_type> paths;
    mp_hierarchy_panel->selected_cells (active_cellview_index (), paths);

    manager ()->transaction (tl::to_string (QObject::tr ("Hide cell"))); 

    for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
      if (! p->empty ()) {
        hide_cell (p->back (), active_cellview_index ());
      }
    }

    manager ()->commit ();

  }
}

void
LayoutView::cm_cell_show ()
{
  if (mp_hierarchy_panel) {

    std::vector<HierarchyControlPanel::cell_path_type> paths;
    mp_hierarchy_panel->selected_cells (active_cellview_index (), paths);

    manager ()->transaction (tl::to_string (QObject::tr ("Show cell"))); 

    for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
      if (! p->empty ()) {
        show_cell (p->back (), active_cellview_index ());
      }
    }

    manager ()->commit ();

  }
}

void
LayoutView::cm_cell_show_all ()
{
  if (mp_hierarchy_panel) {
    manager ()->transaction (tl::to_string (QObject::tr ("Show all cells"))); 
    show_all_cells ();
    manager ()->commit ();
  }
}

void
LayoutView::cm_select_all ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_select_all ();
  }
}

void
LayoutView::cm_new_tab ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_new_tab ();
  }
}

void
LayoutView::cm_remove_tab ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_remove_tab ();
  }
}

void
LayoutView::cm_rename_tab ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_rename_tab ();
  }
}

void
LayoutView::cm_make_invalid ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_make_invalid ();
  }
}

void
LayoutView::cm_make_valid ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_make_valid ();
  }
}

void
LayoutView::cm_hide ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_hide ();
  }
}

void
LayoutView::cm_hide_all ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_hide_all ();
  }
}

void
LayoutView::cm_show_only ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_show_only ();
  }
}

void
LayoutView::cm_show_all ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_show_all ();
  }
}

void
LayoutView::cm_show ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_show ();
  }
}

void
LayoutView::cm_rename ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_rename ();
  }
}

void
LayoutView::cm_delete ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_delete ();
  }
}

void
LayoutView::cm_insert ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_insert ();
  }
}

void
LayoutView::cm_group ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_group ();
  }
}

void
LayoutView::cm_ungroup ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_ungroup ();
  }
}

void
LayoutView::cm_source ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_source ();
  }
}

void
LayoutView::cm_sort_by_name ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_sort_by_name ();
  }
}

void
LayoutView::cm_sort_by_ild ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_sort_by_ild ();
  }
}

void
LayoutView::cm_sort_by_idl ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_sort_by_idl ();
  }
}

void
LayoutView::cm_sort_by_ldi ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_sort_by_ldi ();
  }
}

void
LayoutView::cm_sort_by_dli ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_sort_by_dli ();
  }
}

void
LayoutView::cm_regroup_by_index ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_regroup_by_index ();
  }
}

void
LayoutView::cm_regroup_by_datatype ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_regroup_by_datatype ();
  }
}

void
LayoutView::cm_regroup_by_layer ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_regroup_by_layer ();
  }
}

void
LayoutView::cm_regroup_flatten ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_regroup_flatten ();
  }
}

void
LayoutView::cm_expand_all ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_expand_all ();
  }
}

void
LayoutView::cm_add_missing ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_add_missing ();
  }
}

void 
LayoutView::add_missing_layers ()
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

  emit layer_order_changed ();
}

LayerState 
LayoutView::layer_snapshot () const
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
LayoutView::add_new_layers (const LayerState &state)
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
    emit layer_order_changed ();
  }
}

void
LayoutView::cm_remove_unused ()
{
  remove_unused_layers ();
}

void 
LayoutView::remove_unused_layers ()
{
  if (mp_control_panel) {
    mp_control_panel->cm_remove_unused ();
  }
}

void 
LayoutView::prev_display_state ()
{
  if (m_display_state_ptr > 0) {
    m_display_state_ptr--;
    goto_view (m_display_states [m_display_state_ptr]);
  }
}

bool 
LayoutView::has_prev_display_state ()
{
  return m_display_state_ptr > 0;
}

void 
LayoutView::next_display_state ()
{
  if (m_display_state_ptr + 1 < m_display_states.size ()) {
    m_display_state_ptr++;
    goto_view (m_display_states [m_display_state_ptr]);
  }
}

bool 
LayoutView::has_next_display_state ()
{
  return m_display_state_ptr + 1 < m_display_states.size ();
}

const lay::CellView &
LayoutView::active_cellview () const
{
  return cellview ((unsigned int) active_cellview_index ());
}

lay::CellViewRef
LayoutView::active_cellview_ref ()
{
  return cellview_ref ((unsigned int) active_cellview_index ());
}

int
LayoutView::active_cellview_index () const
{
  return mp_hierarchy_panel->active ();
}

void 
LayoutView::set_active_cellview_index (int index) 
{
  if (index >= 0 && index < int (m_cellviews.size ())) {
    mp_hierarchy_panel->select_active (index);
  }
}

void 
LayoutView::selected_cells_paths (int cv_index, std::vector<cell_path_type> &paths) const
{
  mp_hierarchy_panel->selected_cells (cv_index, paths);
}

void
LayoutView::current_cell_path (int cv_index, cell_path_type &path) const
{
  mp_hierarchy_panel->current_cell (cv_index, path);
}

void
LayoutView::set_current_cell_path (int cv_index, const cell_path_type &path) 
{
  mp_hierarchy_panel->set_current_cell (cv_index, path);
}

void
LayoutView::activate ()
{
  if (! m_activated) {
    for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
      if ((*p)->browser_interface () && (*p)->browser_interface ()->active ()) {
        (*p)->browser_interface ()->show ();
      }
    }
    mp_timer->start (timer_interval);
    m_activated = true;
    update_content ();
  }
}

void
LayoutView::deactivate ()
{
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->browser_interface ()) {
      (*p)->browser_interface ()->hide ();
    }
  }

  emit clear_current_pos ();
  mp_canvas->free_resources ();
  mp_timer->stop ();
  m_activated = false;
}

void
LayoutView::deactivate_all_browsers ()
{
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    if ((*p)->browser_interface ()) {
      (*p)->browser_interface ()->deactivate ();
    }
  }
}

void
LayoutView::current_pos (double x, double y)
{
  if (m_activated) {
    if (dbu_coordinates ()) {
      double dx = 0.0, dy = 0.0;
      if (active_cellview_index () >= 0) {
        double dbu = cellview (active_cellview_index ())->layout ().dbu ();
        dx = x / dbu;
        dy = y / dbu;
      }
      emit current_pos_changed (dx, dy, true);
    } else {
      emit current_pos_changed (x, y, false);
    }
  }
}

void
LayoutView::stop_redraw ()
{
  mp_canvas->stop_redraw ();
}

void
LayoutView::stop ()
{
  stop_redraw ();
  deactivate_all_browsers ();
}

void 
LayoutView::mode (int m)
{
  if (m != m_mode) {

    m_mode = m;

    if (m > 0) {

      for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
        if ((*p)->plugin_declaration ()->id () == m) {
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
LayoutView::is_move_mode () const
{
  return m_mode == -1;
}

bool 
LayoutView::is_selection_mode () const
{
  return m_mode == 0;
}
  
unsigned int 
LayoutView::intrinsic_mouse_modes (std::vector<std::string> *descriptions)
{
  if (descriptions) {
    descriptions->push_back ("select\t" + tl::to_string (QObject::tr ("Select")) + "<:select.png>");
    descriptions->push_back ("move\t" + tl::to_string (QObject::tr ("Move")) + "<:move.png>");
  }
  return 2;
}

int 
LayoutView::default_mode ()
{
  return 0; // TODO: any generic scheme? is select, should be ruler..
}

void 
LayoutView::menu_activated (const std::string &symbol)
{
  //  distribute the menu on the plugins - one should take it.
  for (std::vector<lay::Plugin *>::iterator p = mp_plugins.begin (); p != mp_plugins.end (); ++p) {
    (*p)->menu_activated (symbol);
  }
}

void 
LayoutView::rename_cellview (const std::string &name, int cellview_index)
{
  if (cellview_index >= 0 && cellview_index < int (m_cellviews.size ())) {
    if ((*cellview_iter (cellview_index))->name () != name) {
      (*cellview_iter (cellview_index))->rename (name);
      mp_hierarchy_panel->do_update_content (cellview_index);
      if (m_title.empty ()) {
        emit title_changed ();
      }
    }
  }
}

std::vector<db::DCplxTrans>
LayoutView::cv_transform_variants (int cv_index) const
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
LayoutView::cv_transform_variants (int cv_index, unsigned int layer) const
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
LayoutView::cv_transform_variants_by_layer (int cv_index) const
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
LayoutView::cv_transform_variants () const
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
LayoutView::ascend (int index)
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
LayoutView::descend (const std::vector<db::InstElement> &path, int index)
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
LayoutView::is_editable () const
{
  return m_editable;
}

unsigned int
LayoutView::search_range ()
{
  return m_search_range;
}

void
LayoutView::set_search_range (unsigned int sr)
{
  m_search_range = sr;
}

void
LayoutView::message (const std::string &s, int timeout)
{
  emit show_message (s, timeout * 1000);
}

db::cell_index_type
LayoutView::new_cell (int cv_index, const std::string &cell_name) 
{
  db::cell_index_type new_ci (0);

  if (cv_index >= 0 && int (m_cellviews.size ()) > cv_index) {

    db::Layout &layout = cellview (cv_index)->layout ();
    if (! cell_name.empty () && layout.cell_by_name (cell_name.c_str ()).first) {
      throw tl::Exception (tl::to_string (QObject::tr ("A cell with that name already exists: %s")), cell_name);
    }

    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("New cell"))); 
    }

    new_ci = layout.add_cell (cell_name.empty () ? 0 : cell_name.c_str ());

    if (manager ()) {
      manager ()->commit ();
    }

  }

  return new_ci;
}

void
LayoutView::do_transform (const db::DCplxTrans &tr)
{
  //  end move operations, cancel edit operations
  cancel_edits ();
  lay::Editables::transform (tr);
}

void
LayoutView::transform_layout (const db::DCplxTrans &tr_mic)
{
  //  end move operations, cancel edit operations
  cancel_edits ();
  clear_selection ();

  int cv_index = active_cellview_index ();
  if (cv_index >= 0) {

    db::Layout &layout = cellview (cv_index)->layout ();

    db::ICplxTrans tr (db::DCplxTrans (1.0 / layout.dbu ()) * tr_mic * db::DCplxTrans (layout.dbu ()));

    bool has_proxy = false;
    for (db::Layout::const_iterator c = layout.begin (); ! has_proxy && c != layout.end (); ++c) {
      has_proxy = c->is_proxy ();
    }

    if (has_proxy && 
        QMessageBox::question (this, 
                               QObject::tr ("Transforming PCells Or Library Cells"), 
                               QObject::tr ("The layout contains PCells or library cells or both.\n"
                                            "Any changes to such cells may be lost when their layout is refreshed later.\n"
                                            "Consider using 'Convert all cells to static' before transforming the layout.\n"
                                            "\n"
                                            "Would you like to continue?\n"
                                            "Choose 'Yes' to continue anyway. Choose 'No' to cancel."), 
                               QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
      return;
    }

    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("Transform layout"))); 
      layout.transform (tr);
      manager ()->commit ();
    } else {
      active_cellview ()->layout ().transform (tr);
    }

  }
}

void 
LayoutView::cm_lay_flip_x ()
{
  transform_layout (db::DCplxTrans (db::FTrans::m90));
}

void 
LayoutView::cm_lay_flip_y ()
{
  transform_layout (db::DCplxTrans (db::FTrans::m0));
}

void 
LayoutView::cm_lay_rot_ccw ()
{
  db::DCplxTrans tr (db::DFTrans::r90);
  transform_layout (db::DCplxTrans (db::FTrans::r90));
}

void 
LayoutView::cm_lay_rot_cw ()
{
  transform_layout (db::DCplxTrans (db::FTrans::r270));
}

void 
LayoutView::cm_lay_free_rot ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     QObject::tr ("Free rotation"), 
                                     QObject::tr ("Rotation angle in degree (counterclockwise)"), 
                                     QLineEdit::Normal, QString::fromUtf8 ("0.0"), 
                                     &ok);

  if (ok) {

    double angle = 0.0;
    tl::from_string (tl::to_string (s), angle);

    transform_layout (db::DCplxTrans (1.0, angle, false, db::DVector ()));

  }
}

void 
LayoutView::cm_lay_scale ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     QObject::tr ("Scaling"), 
                                     QObject::tr ("Scaling factor"), 
                                     QLineEdit::Normal, QString::fromUtf8 ("1.0"), 
                                     &ok);

  if (ok) {

    double scale = 0.0;
    tl::from_string (tl::to_string (s), scale);

    transform_layout (db::DCplxTrans (scale));

  }
}

void 
LayoutView::cm_lay_move ()
{
  lay::MoveOptionsDialog options (this);
  if (options.exec_dialog (m_move_dist)) {
    transform_layout (db::DCplxTrans (m_move_dist));
  }
}

void 
LayoutView::cm_sel_flip_x ()
{
  db::DCplxTrans tr (db::DFTrans::m90);
  db::DBox sel_bbox (lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutView::cm_sel_flip_y ()
{
  db::DCplxTrans tr (db::DFTrans::m0);
  db::DBox sel_bbox (lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutView::cm_sel_rot_ccw ()
{
  db::DCplxTrans tr (db::DFTrans::r90);
  db::DBox sel_bbox (lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutView::cm_sel_rot_cw ()
{
  db::DCplxTrans tr (db::DFTrans::r270);
  db::DBox sel_bbox (lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutView::cm_sel_free_rot ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     QObject::tr ("Free rotation"), 
                                     QObject::tr ("Rotation angle in degree (counterclockwise)"), 
                                     QLineEdit::Normal, QString::fromUtf8 ("0.0"), 
                                     &ok);

  if (ok) {

    double angle = 0.0;
    tl::from_string (tl::to_string (s), angle);

    db::DCplxTrans tr = db::DCplxTrans (1.0, angle, false, db::DVector ());
    db::DBox sel_bbox (lay::Editables::selection_bbox ());
    if (! sel_bbox.empty ()) {
      tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
    }
    do_transform (tr);

  }
}

void 
LayoutView::cm_sel_scale ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     QObject::tr ("Scaling"), 
                                     QObject::tr ("Scaling factor"), 
                                     QLineEdit::Normal, QString::fromUtf8 ("1.0"), 
                                     &ok);

  if (ok) {

    double scale = 0.0;
    tl::from_string (tl::to_string (s), scale);

    db::DCplxTrans tr = db::DCplxTrans (scale);
    db::DBox sel_bbox (lay::Editables::selection_bbox ());
    if (! sel_bbox.empty ()) {
      tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
    }
    do_transform (tr);

  }
}

void
LayoutView::cm_sel_move_interactive ()
{
  if (mp_move_service->begin_move ()) {
    switch_mode (-1);  //  move mode
  }
}

void
LayoutView::switch_mode (int m)
{
  if (m_mode != m) {
    mode (m);
    emit mode_change (m);
  }
}

void 
LayoutView::cm_sel_move_to ()
{
  db::DBox sel_bbox (lay::Editables::selection_bbox ());
  if (sel_bbox.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Nothing selected to move")));
  }

  double x = sel_bbox.left () + (sel_bbox.width () * (1 + m_move_to_origin_mode_x) * 0.5);
  double y = sel_bbox.bottom () + (sel_bbox.height () * (1 + m_move_to_origin_mode_y) * 0.5);
  db::DPoint move_target (x, y);

  lay::MoveToOptionsDialog options (this);
  if (options.exec_dialog (m_move_to_origin_mode_x, m_move_to_origin_mode_y, move_target)) {

    x = sel_bbox.left () + (sel_bbox.width () * (1 + m_move_to_origin_mode_x) * 0.5);
    y = sel_bbox.bottom () + (sel_bbox.height () * (1 + m_move_to_origin_mode_y) * 0.5);

    do_transform (db::DCplxTrans (move_target - db::DPoint (x, y)));

  }
}

void 
LayoutView::cm_sel_move ()
{
  lay::MoveOptionsDialog options (this);
  if (options.exec_dialog (m_move_dist)) {
    do_transform (db::DCplxTrans (m_move_dist));
  }
}

void
LayoutView::cm_copy_layer () 
{
  struct { int *cv; int *layer; } specs [] = {
    { &m_copy_cva, &m_copy_layera },
    { &m_copy_cvr, &m_copy_layerr }
  };

  for (unsigned int i = 0; i < sizeof (specs) / sizeof (specs[0]); ++i) {

    int &cv = *(specs[i].cv);
    int &layer = *(specs[i].layer);

    if (cv >= int (m_cellviews.size ())) {
      cv = -1;
    }

    int index = active_cellview_index ();
    if (cv < 0) {
      cv = index;
    }

    if (cv < 0 || ! (*cellview_iter (cv))->layout ().is_valid_layer ((unsigned int) layer)) {
      layer = -1;
    }

  }

  lay::DuplicateLayerDialog dialog (this);
  if (dialog.exec_dialog (this, m_copy_cva, m_copy_layera, m_copy_cvr, m_copy_layerr, m_duplicate_hier_mode, m_clear_before)) {

    bool supports_undo = true;

    if (db::transactions_enabled ()) {

      lay::TipDialog td (QApplication::activeWindow (), 
                         tl::to_string (QObject::tr ("Undo buffering for the following operation can be memory and time consuming.\nChoose \"Yes\" to use undo buffering or \"No\" for no undo buffering. Warning: in the latter case, the undo history will be lost.\n\nChoose undo buffering?")), 
                         "copy-layer-undo-buffering",
                         lay::TipDialog::yesnocancel_buttons);

      lay::TipDialog::button_type button = lay::TipDialog::null_button;
      td.exec_dialog (button);
      if (button == lay::TipDialog::cancel_button) {
        return;
      }

      supports_undo = (button == lay::TipDialog::yes_button);

    } else {
      supports_undo = false;
    }

    cancel ();

    if (manager ()) {
      if (! supports_undo) {
        manager ()->clear ();
      } else {
        manager ()->transaction (tl::to_string (QObject::tr ("Duplicate layer"))); 
      }
    }

    try {

      bool same_layout = (&cellview (m_copy_cvr)->layout () == &cellview (m_copy_cva)->layout ());
      if (same_layout && m_copy_layera == m_copy_layerr) {
        throw tl::Exception (tl::to_string (QObject::tr ("Source and target layer must not be identical for duplicate operation")));
      }

      if (m_duplicate_hier_mode == 0) {

        //  clear the result layer for all called cells in flat mode
        if (m_clear_before) {
          std::set<db::cell_index_type> called_cells;
          called_cells.insert (cellview (m_copy_cvr).cell_index ());
          cellview (m_copy_cvr).cell ()->collect_called_cells (called_cells);
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            cellview (m_copy_cvr)->layout ().cell (*c).shapes (m_copy_layerr).clear ();
          }
        }

        db::Cell &target_cell = *cellview (m_copy_cvr).cell ();

        if (! same_layout) {

          //  flat mode (different layouts)
          db::PropertyMapper pm (cellview (m_copy_cvr)->layout (), cellview (m_copy_cva)->layout ());
          for (db::RecursiveShapeIterator si (cellview (m_copy_cva)->layout (), *cellview (m_copy_cva).cell (), m_copy_layera); ! si.at_end (); ++si) {
            target_cell.shapes (m_copy_layerr).insert (*si, si.trans (), pm);
          }

        } else {

          //  flat mode (same layouts)
          tl::ident_map<db::Layout::properties_id_type> pm1;
          db::Shapes &res = target_cell.shapes (m_copy_layerr);
          
          db::Layout &layout = cellview (m_copy_cvr)->layout ();
          try {

            //  using update/start_layout and end_changes improves the performance since changing the 
            //  shapes collection will invalidate the layout and cause updates inside the RecursiveShapeIerator
            layout.update ();
            layout.start_changes ();
            for (db::RecursiveShapeIterator si (cellview (m_copy_cva)->layout (), *cellview (m_copy_cva).cell (), m_copy_layera); ! si.at_end (); ++si) {
              res.insert (*si, si.trans (), pm1);
            }
            layout.end_changes ();

          } catch (...) {
            layout.end_changes ();
            throw;
          }

        }
        
      } else if (m_duplicate_hier_mode == 1) {

        db::Cell &cell = *cellview (m_copy_cva).cell ();
        db::Cell &target_cell = *cellview (m_copy_cvr).cell ();

        if (m_clear_before) {
          target_cell.clear (m_copy_layerr);
        } 

        if (m_copy_cvr == m_copy_cva) {

          //  current cell only mode: identical cell
          cell.copy (m_copy_layera, m_copy_layerr);

        } else if (! same_layout) {

          //  current cell only mode (different layouts)
          db::PropertyMapper pm (cellview (m_copy_cvr)->layout (), cellview (m_copy_cva)->layout ());
          for (db::Shapes::shape_iterator si = cellview (m_copy_cva).cell ()->shapes (m_copy_layera).begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
            target_cell.shapes (m_copy_layerr).insert (*si, pm);
          }

        } else {

          //  current cell only mode (same layouts, but different cells)
          for (db::Shapes::shape_iterator si = cellview (m_copy_cva).cell ()->shapes (m_copy_layera).begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
            target_cell.shapes (m_copy_layerr).insert (*si);
          }

        }
        
      } else if (m_duplicate_hier_mode == 2) {

        //  subcells cell by cell - source and target layout must be identical
        std::set<db::cell_index_type> called_cells;
        cellview (m_copy_cva).cell ()->collect_called_cells (called_cells);
        called_cells.insert (cellview (m_copy_cva).cell_index ());

        db::Layout &layout = cellview (m_copy_cva)->layout ();
        for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
          db::Cell &cell = layout.cell (*c);
          if (m_clear_before) {
            cell.clear (m_copy_layerr);
          } 
          cell.copy (m_copy_layera, m_copy_layerr);
        }

      }

      if (manager () && supports_undo) {
        manager ()->commit ();
      }

    } catch (...) {
      if (manager () && supports_undo) {
        manager ()->commit ();
      }
      throw;
    }

  }
}

void
LayoutView::cm_new_layer () 
{
  int index = active_cellview_index ();

  if (index >= 0 && int (m_cellviews.size ()) > index) {

    const lay::CellView &cv = cellview (index);

    lay::NewLayerPropertiesDialog prop_dia (this);
    if (prop_dia.exec_dialog (cv, m_new_layer_props)) {

      for (unsigned int l = 0; l < cv->layout ().layers (); ++l) {
        if (cv->layout ().is_valid_layer (l) && cv->layout ().get_properties (l).log_equal (m_new_layer_props)) {
          throw tl::Exception (tl::to_string (QObject::tr ("A layer with that signature already exists: ")) + m_new_layer_props.to_string ());
        }
      }

      if (manager ()) {
        manager ()->transaction (tl::to_string (QObject::tr ("New layer"))); 
      }

      unsigned int l = cv->layout ().insert_layer (m_new_layer_props);
      std::vector <unsigned int> nl;
      nl.push_back (l);
      add_new_layers (nl, index);
      update_content ();

      if (manager ()) {
        manager ()->commit ();
      }

    }

  }
}

void
LayoutView::cm_edit_layer () 
{
  lay::LayerPropertiesConstIterator sel = current_layer ();
  if (sel.is_null ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer selected for editing it's properties")));
  }

  int index = sel->cellview_index ();
  if (sel->has_children () || index < 0 || int (m_cellviews.size ()) <= index || sel->layer_index () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected for editing it's properties")));
  }

  const lay::CellView &cv = cellview (index);

  db::LayerProperties layer_props = cv->layout ().get_properties ((unsigned int) sel->layer_index ());

  lay::NewLayerPropertiesDialog prop_dia (this);
  if (prop_dia.exec_dialog (cv, layer_props)) {

    for (unsigned int l = 0; l < cv->layout ().layers (); ++l) {
      if (cv->layout ().is_valid_layer (l) && int (l) != sel->layer_index () && cv->layout ().get_properties (l).log_equal (layer_props)) {
        throw tl::Exception (tl::to_string (QObject::tr ("A layer with that signature already exists: ")) + layer_props.to_string ());
      }
    }

    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("Edit layer"))); 
    }

    cv->layout ().set_properties (sel->layer_index (), layer_props);

    lay::LayerProperties lp (*sel);
    lay::ParsedLayerSource s = lp.source (false);
    s.layer (layer_props.layer);
    s.datatype (layer_props.datatype);
    if (! layer_props.name.empty ()) {
      s.name (layer_props.name);
    } else {
      s.clear_name ();
    }
    lp.set_source (s);
    set_properties (sel, lp);

    update_content ();

    if (manager ()) {
      manager ()->commit ();
    }

  }
}

void
LayoutView::cm_delete_layer () 
{
  std::vector<lay::LayerPropertiesConstIterator> sel = selected_layers ();
  std::sort (sel.begin (), sel.end (), CompareLayerIteratorBottomUp ());

  //  collect valid layers
  std::vector<lay::LayerPropertiesConstIterator> valid_sel;
  std::set<std::pair<db::Layout *, unsigned int> > valid_layers;
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator si = sel.begin (); si != sel.end (); ++si) {
    int cv_index = (*si)->cellview_index ();
    const lay::CellView &cv = cellview (cv_index);
    if (!(*si)->has_children () && cv_index >= 0 && int (m_cellviews.size ()) > cv_index && (*si)->layer_index () >= 0 && cv.is_valid ()) {
      valid_sel.push_back (*si);
      valid_layers.insert (std::make_pair (&cv->layout (), (*si)->layer_index ()));
    }
  }

  if (valid_sel.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No or no valid layer selected for deleting them")));
  }

  cancel_edits ();
  clear_selection ();

  if (manager ()) {
    manager ()->transaction (tl::to_string (QObject::tr ("Delete layers"))); 
  }

  //  Hint: delete_layer must come before the layers are actually deleted in because
  //  for undo this must be the last thing to do (otherwise the layout is not propertly set up)

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator si = valid_sel.begin (); si != valid_sel.end (); ++si) {
    lay::LayerPropertiesConstIterator lp = *si;
    delete_layer (lp);
  }

  for (std::set<std::pair<db::Layout *, unsigned int> >::const_iterator li = valid_layers.begin (); li != valid_layers.end(); ++li) {

    unsigned int layer_index = li->second;
    db::Layout *layout = li->first;

    for (db::Layout::iterator c = layout->begin (); c != layout->end (); ++c) {
      c->shapes (layer_index).clear ();
    }

    layout->delete_layer (layer_index);

  }

  update_content ();

  if (manager ()) {
    manager ()->commit ();
  }
}

void
LayoutView::cm_clear_layer () 
{
  std::vector<lay::LayerPropertiesConstIterator> sel = selected_layers ();
  if (sel.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer selected for clearing")));
  }

  lay::ClearLayerModeDialog mode_dialog (this);
  if (mode_dialog.exec_dialog (m_layer_hier_mode)) {

    cancel_edits ();
    clear_selection ();

    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("Clear layer")));
    }

    for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator si = sel.begin (); si != sel.end (); ++si) {

      if (! (*si)->has_children () && (*si)->layer_index () >= 0 && cellview ((*si)->cellview_index ()).is_valid ()) {

        int layer_index = (*si)->layer_index ();
        const lay::CellView &cv = cellview ((*si)->cellview_index ());

        if (m_layer_hier_mode == 0) {
          cv.cell ()->clear ((unsigned int) layer_index);
        } else if (m_layer_hier_mode == 1) {

          cv.cell ()->clear ((unsigned int) layer_index);

          std::set <db::cell_index_type> called_cells;
          cv.cell ()->collect_called_cells (called_cells);
          for (std::set <db::cell_index_type>::const_iterator cc = called_cells.begin (); cc != called_cells.end (); ++cc) {
            cv->layout ().cell (*cc).clear ((unsigned int) layer_index);
          }

        } else {
          cv->layout ().clear_layer ((unsigned int) layer_index);
        }

      }

    }

    if (manager ()) {
      manager ()->commit ();
    }

  }
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
LayoutView::add_l2ndb (db::LayoutToNetlist *l2ndb)
{
  make_unique_name (l2ndb, m_l2ndbs.begin (), m_l2ndbs.end ());
  m_l2ndbs.push_back (l2ndb);

  //  Mark this object as owned by us (for GSI)
  l2ndb->keep ();

  l2ndb_list_changed_event ();

  return (unsigned int)(m_l2ndbs.size () - 1);
}

unsigned int
LayoutView::replace_l2ndb (unsigned int db_index, db::LayoutToNetlist *l2ndb)
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
LayoutView::get_l2ndb (int index)
{
  if (index >= 0 && index < int (m_l2ndbs.size ())) {
    return m_l2ndbs [index];
  } else {
    return 0;
  }
}

void 
LayoutView::open_l2ndb_browser (int l2ndb_index, int cv_index)
{
  lay::NetlistBrowserDialog *l2ndb_browser = get_plugin <lay::NetlistBrowserDialog> ();
  if (l2ndb_browser) {
    l2ndb_browser->load (l2ndb_index, cv_index);
  }
}

const db::LayoutToNetlist *
LayoutView::get_l2ndb (int index) const
{
  if (index >= 0 && index < int (m_l2ndbs.size ())) {
    return m_l2ndbs [index];
  } else {
    return 0;
  }
}

void 
LayoutView::remove_l2ndb (unsigned int index)
{
  if (index < (unsigned int) (m_l2ndbs.size ())) {
    delete m_l2ndbs [index];
    m_l2ndbs.erase (m_l2ndbs.begin () + index);
    l2ndb_list_changed_event ();
  }
}

unsigned int
LayoutView::add_rdb (rdb::Database *rdb)
{
  make_unique_name (rdb, m_rdbs.begin (), m_rdbs.end ());
  m_rdbs.push_back (rdb);

  //  Mark this object as owned by us (for GSI)
  rdb->keep ();

  rdb_list_changed_event ();

  return (unsigned int)(m_rdbs.size () - 1);
}

unsigned int
LayoutView::replace_rdb (unsigned int db_index, rdb::Database *rdb)
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
LayoutView::get_rdb (int index)
{
  if (index >= 0 && index < int (m_rdbs.size ())) {
    return m_rdbs [index];
  } else {
    return 0;
  }
}

void
LayoutView::open_rdb_browser (int rdb_index, int cv_index)
{
  rdb::MarkerBrowserDialog *rdb_browser = get_plugin <rdb::MarkerBrowserDialog> ();
  if (rdb_browser) {
    rdb_browser->load (rdb_index, cv_index);
  }
}

const rdb::Database *
LayoutView::get_rdb (int index) const
{
  if (index >= 0 && index < int (m_rdbs.size ())) {
    return m_rdbs [index];
  } else {
    return 0;
  }
}

void
LayoutView::remove_rdb (unsigned int index)
{
  if (index < (unsigned int) (m_rdbs.size ())) {
    delete m_rdbs [index];
    m_rdbs.erase (m_rdbs.begin () + index);
    rdb_list_changed_event ();
  }
}

QSize
LayoutView::sizeHint () const
{
  if ((m_options & LV_Naked) != 0) {
    return QSize (200, 200);
  } else if ((m_options & LV_NoLayers) != 0 || (m_options & LV_NoHierarchyPanel) != 0 || (m_options & LV_NoLibrariesView) != 0) {
    return QSize (400, 200);
  } else {
    return QSize (600, 200);
  }
}

} // namespace lay

