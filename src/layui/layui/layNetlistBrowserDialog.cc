
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "layNetlistBrowserDialog.h"
#include "tlProgress.h"
#include "tlExceptions.h"
#include "layLayoutViewBase.h"
#include "layFinder.h"
#include "layFileDialog.h"
#include "layConverters.h"
#include "layQtTools.h"
#include "layConfigurationDialog.h"
#include "dbLayoutToNetlist.h"
#include "dbRecursiveShapeIterator.h"

#include "ui_NetlistBrowserDialog.h"

#include <QMessageBox>
#include <QInputDialog>

#include <memory>

namespace lay
{

extern const std::string cfg_l2ndb_marker_color;
extern const std::string cfg_l2ndb_marker_cycle_colors;
extern const std::string cfg_l2ndb_marker_cycle_colors_enabled;
extern const std::string cfg_l2ndb_marker_dither_pattern;
extern const std::string cfg_l2ndb_marker_line_width;
extern const std::string cfg_l2ndb_marker_vertex_size;
extern const std::string cfg_l2ndb_marker_halo;
extern const std::string cfg_l2ndb_marker_intensity;
extern const std::string cfg_l2ndb_marker_use_original_colors;
extern const std::string cfg_l2ndb_window_mode;
extern const std::string cfg_l2ndb_window_dim;
extern const std::string cfg_l2ndb_max_shapes_highlighted;
extern const std::string cfg_l2ndb_show_all;
extern const std::string cfg_l2ndb_window_state;

NetlistBrowserDialog::NetlistBrowserDialog (lay::Dispatcher *root, LayoutViewBase *vw)
  : lay::Browser (root, vw),
    lay::ViewService (vw->canvas ()),
    m_window (lay::NetlistBrowserConfig::FitNet),
    m_window_dim (0.0),
    m_max_shape_count (0),
    m_auto_color_enabled (false),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    m_marker_intensity (0),
    m_use_original_colors (false),
    m_cv_index (-1),
    m_l2n_index (-1),
    m_mouse_state (0)
{
  mp_ui = new Ui::NetlistBrowserDialog ();
  mp_ui->setupUi (this);

  mp_ui->browser_page->set_dispatcher (root);

  if (view ()) {
    view ()->cellviews_changed_event.add (this, &NetlistBrowserDialog::cellviews_changed);
    view ()->cellview_changed_event.add (this, &NetlistBrowserDialog::cellview_changed);
    view ()->l2ndb_list_changed_event.add (this, &NetlistBrowserDialog::l2ndbs_changed);
  }

  m_open_action = new QAction (QObject::tr ("Open"), mp_ui->file_menu);
  m_saveas_action = new QAction (QObject::tr ("Save As"), mp_ui->file_menu);
  m_export_action = new QAction (QObject::tr ("Export To Layout"), mp_ui->file_menu);
  m_reload_action = new QAction (QObject::tr ("Reload"), mp_ui->file_menu);
  m_unload_action = new QAction (QObject::tr ("Unload"), mp_ui->file_menu);
  m_unload_all_action = new QAction (QObject::tr ("Unload All"), mp_ui->file_menu);

  connect (m_open_action, SIGNAL (triggered ()), this, SLOT (open_clicked ()));
  connect (m_saveas_action, SIGNAL (triggered ()), this, SLOT (saveas_clicked ()));
  connect (m_export_action, SIGNAL (triggered ()), this, SLOT (export_clicked ()));
  connect (m_reload_action, SIGNAL (triggered ()), this, SLOT (reload_clicked ()));
  connect (m_unload_action, SIGNAL (triggered ()), this, SLOT (unload_clicked ()));
  connect (m_unload_all_action, SIGNAL (triggered ()), this, SLOT (unload_all_clicked ()));

  mp_ui->file_menu->addAction (m_open_action);
  mp_ui->file_menu->addAction (m_saveas_action);
  QAction *sep0 = new QAction (mp_ui->file_menu);
  sep0->setSeparator (true);
  mp_ui->file_menu->addAction (m_export_action);
  QAction *sep1 = new QAction (mp_ui->file_menu);
  sep1->setSeparator (true);
  mp_ui->file_menu->addAction (sep1);
  mp_ui->file_menu->addAction (m_reload_action);
  QAction *sep2 = new QAction (mp_ui->file_menu);
  sep2->setSeparator (true);
  mp_ui->file_menu->addAction (sep2);
  mp_ui->file_menu->addAction (m_unload_action);
  mp_ui->file_menu->addAction (m_unload_all_action);

  connect (mp_ui->layout_cb, SIGNAL (activated (int)), this, SLOT (cv_index_changed (int)));
  connect (mp_ui->l2ndb_cb, SIGNAL (activated (int)), this, SLOT (l2ndb_index_changed (int)));
  connect (mp_ui->configure_pb, SIGNAL (clicked ()), this, SLOT (configure_clicked ()));
  connect (mp_ui->probe_pb, SIGNAL (clicked ()), this, SLOT (probe_button_pressed ()));
  connect (mp_ui->sticky_cbx, SIGNAL (clicked ()), this, SLOT (sticky_mode_clicked ()));

  cellviews_changed ();

  mp_ui->browser_page->selection_changed_event.add (this, &NetlistBrowserDialog::selection_changed);
}

NetlistBrowserDialog::~NetlistBrowserDialog ()
{
  tl::Object::detach_from_all_events ();

  delete mp_ui;
  mp_ui = 0;
}

db::LayoutToNetlist *
NetlistBrowserDialog::db ()
{
  return mp_ui->browser_page->db ();
}

const lay::NetlistObjectsPath &
NetlistBrowserDialog::current_path () const
{
  if (mp_ui->browser_page) {
    return mp_ui->browser_page->current_path ();
  } else {
    static lay::NetlistObjectsPath empty;
    return empty;
  }
}

const std::vector<lay::NetlistObjectsPath> &
NetlistBrowserDialog::selected_paths () const
{
  if (mp_ui->browser_page) {
    return mp_ui->browser_page->selected_paths ();
  } else {
    static std::vector<lay::NetlistObjectsPath> empty;
    return empty;
  }
}

void
NetlistBrowserDialog::configure_clicked ()
{
  release_mouse ();

  lay::ConfigurationDialog config_dialog (this, lay::Dispatcher::instance (), "NetlistBrowserPlugin");
  config_dialog.exec ();
}

bool
NetlistBrowserDialog::mouse_move_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool prio)
{
  if (prio && m_mouse_state != 0) {
    set_cursor (lay::Cursor::cross);
  }

  return false;
}

void
NetlistBrowserDialog::sticky_mode_clicked ()
{
BEGIN_PROTECTED
  if (! mp_ui->sticky_cbx->isChecked ()) {
    release_mouse ();
  } else {
    probe_button_pressed ();
  }
END_PROTECTED
}

bool
NetlistBrowserDialog::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (prio && (buttons & lay::LeftButton) != 0 && m_mouse_state != 0) {

    //  TODO: not used yet, borrowed from net tracer ... TODO: implement short locator!
    if (m_mouse_state == 2) {

      m_mouse_first_point = p;
      m_mouse_state = 3;

      view ()->message (tl::to_string (QObject::tr ("Click on the second point in the net")));

    } else {

      bool trace_path = (m_mouse_state == 3);

      if (trace_path || ! mp_ui->sticky_cbx->isChecked ()) {
        release_mouse ();
      }

      probe_net (p, trace_path);

    }

  }

  return true;
}

void
NetlistBrowserDialog::probe_net (const db::DPoint &p, bool trace_path)
{
  //  prepare for the net tracing
  double l = double (view ()->search_range ()) / ui ()->mouse_event_trans ().mag ();

  db::DBox start_search_box = db::DBox (p, p).enlarged (db::DVector (l, l));

  //  TODO: not used yet ..
  db::DBox stop_search_box;
  if (trace_path) {
    stop_search_box = db::DBox (m_mouse_first_point, m_mouse_first_point).enlarged (db::DVector (l, l));
  }

  unsigned int start_layer = 0;
  db::Point start_point;
  unsigned int cv_index;

  //  locate the seed shape to figure out the cv index and layer
  {

    lay::ShapeFinder finder (true /*point mode*/, false /*all levels*/, db::ShapeIterator::All);

    //  go through all visible layers of all cellviews and find a seed shape
    for (lay::LayerPropertiesConstIterator lprop = view ()->begin_layers (); ! lprop.at_end (); ++lprop) {
      if (lprop->is_visual ()) {
        finder.find (view (), *lprop, start_search_box);
      }
    }

    //  return, if no shape was found
    lay::ShapeFinder::iterator r = finder.begin ();
    if (r == finder.end ()) {
      return;
    }

    cv_index = r->cv_index ();
    start_layer = r->layer ();

  }

  //  if the cv index is not corresponding to the one of the current netlist, ignore this event
  if (int (cv_index) != m_cv_index) {
    return;
  }

  //  determine the cellview
  lay::CellView cv = view ()->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  //  determine the start point
  {

    std::vector<db::DCplxTrans> tv = view ()->cv_transform_variants (m_cv_index, start_layer);
    if (tv.empty ()) {
      return;
    }

    db::CplxTrans tt = tv.front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();

    start_point = tt.inverted ().trans (start_search_box.center ());

  }

  db::Net *net = 0;
  db::Circuit *root = 0;
  std::vector<db::SubCircuit *> sc_path;

  db::LayoutToNetlist *l2ndb = view ()->get_l2ndb (m_l2n_index);
  if (l2ndb) {

    root = l2ndb->netlist ()->circuit_by_name (cv->layout ().cell_name (cv.cell_index ()));
    if (root) {

      //  determines the corresponding layer inside the database and probe the net from this region and the
      //  start point.

      std::vector<db::Region *> regions;

      const db::Connectivity &conn = l2ndb->connectivity ();
      for (db::Connectivity::all_layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {
        db::LayerProperties lp = l2ndb->internal_layout ()->get_properties (*layer);
        if (! lp.is_null ()) {
          db::Region *region = l2ndb->layer_by_index (*layer);
          if (lp == cv->layout ().get_properties (start_layer)) {
            //  a matching original layer is looked up with higher prio
            regions.insert (regions.begin (), region);
          } else {
            regions.push_back (region);
          }
        }
      }

      //  probe the net

      for (std::vector<db::Region *>::const_iterator r = regions.begin (); r != regions.end () && !net; ++r) {
        sc_path.clear ();
        net = l2ndb->probe_net (**r, start_point, &sc_path, root);
      }

    }

  }

  //  select the net if one was found
  lay::NetlistObjectPath path;
  if (net) {
    path.root = root;
    path.net = net;
    path.path = lay::NetlistObjectPath::path_type (sc_path.begin (), sc_path.end ());
  }

  mp_ui->browser_page->select_path (path);

  //  emits the probe event
  //  NOTE: browser_page->current_path () will hold the paired path with the schematic side being
  //  expanded.
  probe_event (mp_ui->browser_page->current_path ().first (), mp_ui->browser_page->current_path ().second ());
}

void
NetlistBrowserDialog::release_mouse ()
{
  m_mouse_state = 0;
  view ()->message ();
  ui ()->ungrab_mouse (this);
}

lay::ViewService *
NetlistBrowserDialog::view_service_interface ()
{
  return this;
}

void
NetlistBrowserDialog::probe_button_pressed ()
{
BEGIN_PROTECTED

  m_mouse_state = 1;

  view ()->message (tl::to_string (QObject::tr ("Click on a point in the net")));
  ui ()->grab_mouse (this, false);

END_PROTECTED
}

void
NetlistBrowserDialog::unload_all_clicked ()
{
BEGIN_PROTECTED

  while (view ()->num_l2ndbs () > 0) {
    view ()->remove_l2ndb (0);
  }

  l2ndb_index_changed (-1);

END_PROTECTED
}

void
NetlistBrowserDialog::unload_clicked ()
{
BEGIN_PROTECTED

  if (m_l2n_index < int (view ()->num_l2ndbs ()) && m_l2n_index >= 0) {

    int new_l2n_index = m_l2n_index;

    view ()->remove_l2ndb (m_l2n_index);

    // try to use another rbd ...
    if (new_l2n_index >= int (view ()->num_l2ndbs ())) {
      --new_l2n_index;
    }
    if (new_l2n_index < int (view ()->num_l2ndbs ()) && new_l2n_index >= 0) {
      l2ndb_index_changed (new_l2n_index);
    }

  }

END_PROTECTED
}

void
NetlistBrowserDialog::export_clicked ()
{
  if (m_l2n_index < int (view ()->num_l2ndbs ()) && m_l2n_index >= 0) {
    mp_ui->browser_page->export_all ();
  }
}

void
NetlistBrowserDialog::saveas_clicked ()
{
BEGIN_PROTECTED

  if (m_l2n_index < int (view ()->num_l2ndbs ()) && m_l2n_index >= 0) {

    db::LayoutToNetlist *l2ndb = view ()->get_l2ndb (m_l2n_index);
    db::LayoutVsSchematic *lvsdb = dynamic_cast<db::LayoutVsSchematic *> (l2ndb);

    if (lvsdb && ! mp_ui->browser_page->is_netlist_mode ()) {

      //  prepare and open the file dialog
      lay::FileDialog save_dialog (this, tl::to_string (QObject::tr ("Save LVS Database")), "KLayout LVS DB files (*.lvsdb)");
      std::string fn (lvsdb->filename ());
      if (save_dialog.get_save (fn)) {

        tl::log << tl::to_string (QObject::tr ("Saving file: ")) << fn;
        tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Saving")));

        lvsdb->save (fn, true);

      }

    } else if (l2ndb) {

      //  prepare and open the file dialog
      lay::FileDialog save_dialog (this, tl::to_string (QObject::tr ("Save Netlist Database")), "KLayout L2N DB files (*.l2n)");
      std::string fn (l2ndb->filename ());
      if (save_dialog.get_save (fn)) {

        tl::log << tl::to_string (QObject::tr ("Saving file: ")) << fn;
        tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Saving")));

        l2ndb->save (fn, true);

      }

    }

  }

END_PROTECTED
}

void
NetlistBrowserDialog::reload_clicked ()
{
BEGIN_PROTECTED

  if (m_l2n_index < int (view ()->num_l2ndbs ()) && m_l2n_index >= 0) {

    db::LayoutToNetlist *l2ndb = view ()->get_l2ndb (m_l2n_index);
    if (l2ndb && ! l2ndb->filename ().empty ()) {

      tl::log << tl::to_string (QObject::tr ("Loading file: ")) << l2ndb->filename ();
      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Loading")));

      mp_ui->browser_page->set_db (0);
      try {

        m_l2ndb_name = l2ndb->name ();
        db::LayoutToNetlist *new_l2ndb = db::LayoutToNetlist::create_from_file (l2ndb->filename ());

        view ()->replace_l2ndb (m_l2n_index, new_l2ndb);
        mp_ui->browser_page->set_db (new_l2ndb);

        current_db_changed_event ();

      } catch (...) {
        current_db_changed_event ();
        throw;
      }

    }

  }

END_PROTECTED
}

void
NetlistBrowserDialog::open_clicked ()
{
BEGIN_PROTECTED

  std::string fmts = tl::to_string (QObject::tr ("All files (*)"));
#if 0 //  TODO: would be good to have this:
  //  collect the formats available ...
  for (tl::Registrar<db::NetlistFormatDeclaration>::iterator rdr = tl::Registrar<db::NetlistFormatDeclaration>::begin (); rdr != tl::Registrar<db::NetlistFormatDeclaration>::end (); ++rdr) {
    fmts += ";;" + rdr->file_format ();
  }
#else
  fmts += ";;L2N DB files (*.l2n);;LVS DB files (*.lvsdb)";
  //  TODO: add plain spice
#endif

  //  prepare and open the file dialog
  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Load Netlist/LVS Database File")), fmts);
  if (open_dialog.get_open (m_open_filename)) {

    tl::log << tl::to_string (QObject::tr ("Loading file: ")) << m_open_filename;
    tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Loading")));

    int l2n_index = view ()->add_l2ndb (db::LayoutToNetlist::create_from_file (m_open_filename));
    mp_ui->l2ndb_cb->setCurrentIndex (l2n_index);
    //  it looks like the setCurrentIndex does not issue this signal:
    l2ndb_index_changed (l2n_index);

  }

END_PROTECTED
}

bool
NetlistBrowserDialog::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;
  bool show_all = mp_ui->browser_page->show_all ();

  if (name == cfg_l2ndb_show_all) {

    tl::from_string (value, show_all);

  } else if (name == cfg_l2ndb_window_mode) {

    NetlistBrowserConfig::net_window_type window = m_window;
    NetlistBrowserWindowModeConverter ().from_string (value, window);
    need_update = lay::test_and_set (m_window, window);

  } else if (name == cfg_l2ndb_window_dim) {

    double wdim = m_window_dim;
    tl::from_string (value, wdim);
    if (fabs (wdim - m_window_dim) > 1e-6) {
      m_window_dim = wdim;
      need_update = true;
    }

  } else if (name == cfg_l2ndb_max_shapes_highlighted) {

    unsigned int mc = 0;
    tl::from_string (value, mc);
    need_update = lay::test_and_set (m_max_shape_count, mc);

  } else if (name == cfg_l2ndb_marker_color) {

    tl::Color color;
    if (! value.empty ()) {
      lay::ColorConverter ().from_string (value, color);
    }

    need_update = lay::test_and_set (m_marker_color, color);

  } else if (name == cfg_l2ndb_marker_cycle_colors) {

    lay::ColorPalette colors;
    colors.from_string (value, true);

    need_update = lay::test_and_set (m_auto_colors, colors);

  } else if (name == cfg_l2ndb_marker_cycle_colors_enabled) {

    bool f = false;
    tl::from_string (value, f);

    need_update = lay::test_and_set (m_auto_color_enabled, f);

  } else if (name == cfg_l2ndb_marker_line_width) {

    int lw = 0;
    tl::from_string (value, lw);

    need_update = lay::test_and_set (m_marker_line_width, lw);

  } else if (name == cfg_l2ndb_marker_vertex_size) {

    int vs = 0;
    tl::from_string (value, vs);

    need_update = lay::test_and_set (m_marker_vertex_size, vs);

  } else if (name == cfg_l2ndb_marker_halo) {

    int halo = 0;
    tl::from_string (value, halo);

    need_update = lay::test_and_set (m_marker_halo, halo);

  } else if (name == cfg_l2ndb_marker_dither_pattern) {

    int dp = 0;
    tl::from_string (value, dp);

    need_update = lay::test_and_set (m_marker_dither_pattern, dp);

  } else if (name == cfg_l2ndb_marker_intensity) {

    int bo = 0;
    tl::from_string (value, bo);

    need_update = lay::test_and_set (m_marker_intensity, bo);

  } else if (name == cfg_l2ndb_marker_use_original_colors) {

    bool oc = false;
    tl::from_string (value, oc);

    need_update = lay::test_and_set (m_use_original_colors, oc);

  } else {
    taken = false;
  }

  if (active () && need_update) {
    mp_ui->browser_page->set_max_shape_count (m_max_shape_count);
    mp_ui->browser_page->set_window (m_window, m_window_dim);
    mp_ui->browser_page->set_highlight_style (m_marker_color, m_marker_line_width, m_marker_vertex_size, m_marker_halo, m_marker_dither_pattern, m_marker_intensity, m_use_original_colors, m_auto_color_enabled ? &m_auto_colors : 0);
  }

  mp_ui->browser_page->show_all (show_all);

  return taken;
}

void
NetlistBrowserDialog::load (int l2ndb_index, int cv_index)
{
  if (! view ()->get_l2ndb (l2ndb_index)) {
    return;
  }

  if (! view ()->cellview (cv_index).is_valid ()) {
    m_layout_name = std::string ();
  } else {
    m_layout_name = view ()->cellview (cv_index)->name ();
  }

  //  set the new references (by name)
  m_l2ndb_name = view ()->get_l2ndb (l2ndb_index)->name ();

  //  force an update
  l2ndbs_changed ();
  cellviews_changed ();

  activate ();
}

void
NetlistBrowserDialog::l2ndbs_changed ()
{
  int l2n_index = -1;

  mp_ui->l2ndb_cb->clear ();

  for (unsigned int i = 0; i < view ()->num_l2ndbs (); ++i) {
    const db::LayoutToNetlist *l2ndb = view ()->get_l2ndb (i);
    std::string text = l2ndb->name ();
    if (! l2ndb->description ().empty ()) {
      text += " (";
      text += l2ndb->description ();
      text += ")";
    }
    mp_ui->l2ndb_cb->addItem (tl::to_qstring (text));
    if (l2ndb->name () == m_l2ndb_name) {
      l2n_index = i;
    }
  }

  //  force an update
  m_l2n_index = l2n_index;
  mp_ui->l2ndb_cb->setCurrentIndex (l2n_index);
  if (active ()) {
    update_content ();
  }
}

void
NetlistBrowserDialog::cellview_changed (int)
{
  mp_ui->browser_page->update_highlights ();
}

void
NetlistBrowserDialog::cellviews_changed ()
{
  int cv_index = -1;

  mp_ui->layout_cb->clear ();

  for (unsigned int i = 0; i < view ()->cellviews (); ++i) {
    const lay::CellView &cv = view ()->cellview (i);
    mp_ui->layout_cb->addItem (tl::to_qstring (cv->name ()));
    if (cv.is_valid () && cv->name () == m_layout_name) {
      cv_index = i;
    }
  }

  mp_ui->layout_cb->setCurrentIndex (cv_index);
  cv_index_changed (cv_index);
}

void
NetlistBrowserDialog::l2ndb_index_changed (int index)
{
  if (m_l2n_index != index) {
    m_l2n_index = index;
    if (active ()) {
      update_content ();
    }
  }
}

void
NetlistBrowserDialog::cv_index_changed (int index)
{
  if (m_cv_index != index) {
    m_cv_index = index;
    if (active ()) {
      update_content ();
    }
  }
}

void
NetlistBrowserDialog::activated ()
{
  std::string state;
  view ()->config_get (cfg_l2ndb_window_state, state);
  lay::restore_dialog_state (this, state, false /*don't adjust the section sizes*/);

  //  Switch to the active cellview index when no valid one is set.
  lay::CellView cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    m_cv_index = view ()->active_cellview_index ();
  }

  if (m_l2n_index < 0 && view ()->get_l2ndb (0) != 0) {

    m_l2ndb_name = view ()->get_l2ndb (0)->name ();
    l2ndbs_changed ();

  } else {
    update_content ();
  }
}

void
NetlistBrowserDialog::update_content ()
{
  release_mouse ();

  db::LayoutToNetlist *l2ndb = view ()->get_l2ndb (m_l2n_index);

  mp_ui->probe_pb->setEnabled (l2ndb != 0);
  release_mouse ();

  if (! l2ndb) {
    mp_ui->central_stack->setCurrentIndex (1);
  }

  bool db_changed = false;

  m_saveas_action->setEnabled (l2ndb != 0);
  m_export_action->setEnabled (l2ndb != 0);
  m_unload_action->setEnabled (l2ndb != 0);
  m_unload_all_action->setEnabled (l2ndb != 0);
  m_reload_action->setEnabled (l2ndb != 0);

  mp_ui->browser_page->enable_updates (false);  //  Avoid building the internal lists several times ...
  db_changed = mp_ui->browser_page->set_db (l2ndb);
  mp_ui->browser_page->set_max_shape_count (m_max_shape_count);
  mp_ui->browser_page->set_highlight_style (m_marker_color, m_marker_line_width, m_marker_vertex_size, m_marker_halo, m_marker_dither_pattern, m_marker_intensity, m_use_original_colors, m_auto_color_enabled ? &m_auto_colors : 0);
  mp_ui->browser_page->set_window (m_window, m_window_dim);
  mp_ui->browser_page->set_view (view (), m_cv_index);
  mp_ui->browser_page->enable_updates (true);

  if (l2ndb) {
    //  Note: it appears to be required to show the browser page after it has been configured.
    //  Otherwise the header gets messed up and the configuration is reset.
    mp_ui->central_stack->setCurrentIndex (0);
  }

  lay::CellView cv = view ()->cellview (m_cv_index);
  m_layout_name = std::string ();
  if (cv.is_valid ()) {
    m_layout_name = cv->name ();
  }

  if (mp_ui->layout_cb->currentIndex () != m_cv_index) {
    mp_ui->layout_cb->setCurrentIndex (m_cv_index);
  }

  if (mp_ui->l2ndb_cb->currentIndex () != m_l2n_index) {
    mp_ui->l2ndb_cb->setCurrentIndex (m_l2n_index);
  }

  if (db_changed) {
    current_db_changed_event ();
  }
}

void
NetlistBrowserDialog::deactivated ()
{
  release_mouse ();

  if (lay::Dispatcher::instance ()) {
    lay::Dispatcher::instance ()->config_set (cfg_l2ndb_window_state, lay::save_dialog_state (this, false /*don't store the section sizes*/).c_str ());
  }

  bool db_changed = false;
  if (mp_ui->browser_page->db () != 0) {
    db_changed = true;
    mp_ui->browser_page->set_db (0);
  }
  mp_ui->browser_page->set_view (0, 0);

  if (db_changed) {
    current_db_changed_event ();
  }
}

void
NetlistBrowserDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "netlist_browser::show") {
    view ()->deactivate_all_browsers ();
    activate ();
  } else {
    lay::Browser::menu_activated (symbol);
  }
}

}

#endif
