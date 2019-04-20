
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "layNetlistBrowserDialog.h"
#include "tlProgress.h"
#include "layLayoutView.h"
#include "tlExceptions.h"
#include "layFileDialog.h"
#include "layConverters.h"
#include "layQtTools.h"
#include "layConfigurationDialog.h"
#include "dbLayoutUtils.h"
#include "dbRecursiveShapeIterator.h"

#include <QMessageBox>
#include <QInputDialog>

#include <memory>

namespace lay
{

extern std::string cfg_l2n_context_mode;
extern std::string cfg_l2n_show_all;
extern std::string cfg_l2n_window_state;
extern std::string cfg_l2n_window_mode;
extern std::string cfg_l2n_window_dim;
extern std::string cfg_l2n_max_marker_count;
extern std::string cfg_l2n_highlight_color;
extern std::string cfg_l2n_highlight_line_width;
extern std::string cfg_l2n_highlight_vertex_size;
extern std::string cfg_l2n_highlight_halo;
extern std::string cfg_l2n_highlight_dither_pattern;

NetlistBrowserDialog::NetlistBrowserDialog (lay::PluginRoot *root, lay::LayoutView *vw)
  : lay::Browser (root, vw),
    Ui::NetlistBrowserDialog (),
    m_context (lay::NetlistBrowserConfig::AnyCell),
    m_window (lay::NetlistBrowserConfig::FitNet),
    m_window_dim (0.0),
    m_max_shape_count (0),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    m_cv_index (-1),
    m_l2n_index (-1)
{
  Ui::NetlistBrowserDialog::setupUi (this);

  browser_frame->set_plugin_root (root);

  if (view ()) {
    view ()->cellviews_changed_event.add (this, &NetlistBrowserDialog::cellviews_changed);
    view ()->cellview_changed_event.add (this, &NetlistBrowserDialog::cellview_changed);
    // @@@view ()->l2n_list_changed_event.add (this, &NetlistBrowserDialog::l2ndbs_changed);
  }

  m_open_action = new QAction (QObject::tr ("Open"), file_menu);
  m_saveas_action = new QAction (QObject::tr ("Save As"), file_menu);
  m_export_action = new QAction (QObject::tr ("Export To Layout"), file_menu);
  m_reload_action = new QAction (QObject::tr ("Reload"), file_menu);
  m_unload_action = new QAction (QObject::tr ("Unload"), file_menu);
  m_unload_all_action = new QAction (QObject::tr ("Unload All"), file_menu);

  connect (m_open_action, SIGNAL (triggered ()), this, SLOT (open_clicked ()));
  connect (m_saveas_action, SIGNAL (triggered ()), this, SLOT (saveas_clicked ()));
  connect (m_export_action, SIGNAL (triggered ()), this, SLOT (export_clicked ()));
  connect (m_reload_action, SIGNAL (triggered ()), this, SLOT (reload_clicked ()));
  connect (m_unload_action, SIGNAL (triggered ()), this, SLOT (unload_clicked ()));
  connect (m_unload_all_action, SIGNAL (triggered ()), this, SLOT (unload_all_clicked ()));

  file_menu->addAction (m_open_action);
  file_menu->addAction (m_saveas_action);
  QAction *sep0 = new QAction (file_menu);
  sep0->setSeparator (true);
  file_menu->addAction (m_export_action);
  QAction *sep1 = new QAction (file_menu);
  sep1->setSeparator (true);
  file_menu->addAction (sep1);
  file_menu->addAction (m_reload_action);
  QAction *sep2 = new QAction (file_menu);
  sep2->setSeparator (true);
  file_menu->addAction (sep2);
  file_menu->addAction (m_unload_action);
  file_menu->addAction (m_unload_all_action);

  connect (layout_cb, SIGNAL (activated (int)), this, SLOT (cv_index_changed (int)));
  connect (l2ndb_cb, SIGNAL (activated (int)), this, SLOT (l2ndb_index_changed (int)));
  connect (configure_pb, SIGNAL (clicked ()), this, SLOT (configure_clicked ()));

  cellviews_changed ();
}

NetlistBrowserDialog::~NetlistBrowserDialog ()
{
  tl::Object::detach_from_all_events ();
}

void
NetlistBrowserDialog::configure_clicked ()
{
  lay::ConfigurationDialog config_dialog (this, lay::PluginRoot::instance (), "NetlistBrowserPlugin");
  config_dialog.exec ();
}

void
NetlistBrowserDialog::unload_all_clicked ()
{
BEGIN_PROTECTED

#if 0 //  @@@
  bool modified = false;
  for (int i = 0; i < int (view ()->num_rdbs ()); ++i) {
    rdb::Database *rdb = view ()->get_rdb (i);
    if (rdb && rdb->is_modified ()) {
      modified = true;
      break;
    }
  }

  if (modified) {

    QMessageBox msgbox (QMessageBox::Question, QObject::tr ("Unload Without Saving"),
                                               QObject::tr ("At least one database was not saved.\nPress 'Continue' to continue anyway or 'Cancel' for not unloading the database."));
    QPushButton *ok = msgbox.addButton (QObject::tr ("Continue"), QMessageBox::AcceptRole);
    msgbox.setDefaultButton (msgbox.addButton (QMessageBox::Cancel));

    msgbox.exec ();

    if (msgbox.clickedButton () != ok) {
      return;
    }

  }

  while (view ()->num_rdbs () > 0) {
    view ()->remove_rdb (0);
  }

  l2ndb_index_changed (-1);
#endif

END_PROTECTED
}

void
NetlistBrowserDialog::unload_clicked ()
{
BEGIN_PROTECTED

#if 0 //  @@@
  if (m_l2n_index < int (view ()->num_rdbs ()) && m_l2n_index >= 0) {

    rdb::Database *rdb = view ()->get_rdb (m_l2n_index);
    if (rdb && rdb->is_modified ()) {

      QMessageBox msgbox (QMessageBox::Question, QObject::tr ("Unload Without Saving"),
                                                 QObject::tr ("The database was not saved.\nPress 'Continue' to continue anyway or 'Cancel' for not unloading the database."));
      QPushButton *ok = msgbox.addButton (QObject::tr ("Continue"), QMessageBox::AcceptRole);
      msgbox.setDefaultButton (msgbox.addButton (QMessageBox::Cancel));

      msgbox.exec ();

      if (msgbox.clickedButton () != ok) {
        return;
      }

    }

    int new_l2n_index = m_l2n_index;

    view ()->remove_rdb (m_l2n_index);

    // try to use another rbd ...
    if (new_l2n_index >= int (view ()->num_rdbs ())) {
      --new_l2n_index;
    }
    if (new_l2n_index < int (view ()->num_rdbs ()) && new_l2n_index >= 0) {
      l2ndb_index_changed (new_l2n_index);
    }

  }
#endif

END_PROTECTED
}

void
NetlistBrowserDialog::export_clicked ()
{
BEGIN_PROTECTED

#if 0 //  @@@
  if (m_l2n_index >= int (view ()->num_rdbs ()) || m_l2n_index < 0) {
    return;
  }

  const rdb::Database *rdb = view ()->get_rdb (m_l2n_index);
  if (! rdb) {
    return;
  }

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  try {

    view ()->manager ()->transaction (tl::to_string (QObject::tr ("Export Net")));

    // ....

    view ()->manager ()->commit ();
    view ()->update_content ();

  } catch (...) {
    view ()->manager ()->commit ();
    view ()->update_content ();
    throw;
  }
#endif

END_PROTECTED
}

void
NetlistBrowserDialog::saveas_clicked ()
{
BEGIN_PROTECTED

#if 0 //  @@@
  if (m_l2n_index < int (view ()->num_rdbs ()) && m_l2n_index >= 0) {

    rdb::Database *rdb = view ()->get_rdb (m_l2n_index);
    if (rdb) {

      //  prepare and open the file dialog
      lay::FileDialog save_dialog (this, tl::to_string (QObject::tr ("Save Net Database")), "KLayout RDB files (*.lyrdb)");
      std::string fn (rdb->filename ());
      if (save_dialog.get_save (fn)) {

        rdb->save (fn);
        rdb->reset_modified ();

      }

    }

  }
#endif

END_PROTECTED
}

void
NetlistBrowserDialog::reload_clicked ()
{
BEGIN_PROTECTED

#if 0 //  @@@
  if (m_l2n_index < int (view ()->num_rdbs ()) && m_l2n_index >= 0) {

    rdb::Database *rdb = view ()->get_rdb (m_l2n_index);
    if (rdb && ! rdb->filename ().empty ()) {

      browser_frame->set_rdb (0);
      rdb->load (rdb->filename ());
      browser_frame->set_rdb (rdb);

    }

  }
#endif

END_PROTECTED
}

void
NetlistBrowserDialog::open_clicked ()
{
BEGIN_PROTECTED

#if 0 //  @@@
  //  collect the formats available ...
  std::string fmts = tl::to_string (QObject::tr ("All files (*)"));
  for (tl::Registrar<rdb::FormatDeclaration>::iterator rdr = tl::Registrar<rdb::FormatDeclaration>::begin (); rdr != tl::Registrar<rdb::FormatDeclaration>::end (); ++rdr) {
    fmts += ";;" + rdr->file_format ();
  }

  //  prepare and open the file dialog
  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Marker Database File")), fmts);
  if (open_dialog.get_open (m_open_filename)) {

    std::auto_ptr <rdb::Database> db (new rdb::Database ());
    db->load (m_open_filename);

    int l2n_index = view ()->add_rdb (db.release ());
    l2n_cb->setCurrentIndex (l2n_index);
    //  it looks like the setCurrentIndex does not issue this signal:
    l2ndb_index_changed (l2n_index);

  }
#endif

END_PROTECTED
}

bool
NetlistBrowserDialog::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;
  bool show_all = browser_frame->show_all ();

  if (name == cfg_l2n_context_mode) {

    NetlistBrowserConfig::net_context_mode_type context = m_context;
    NetlistBrowserContextModeConverter ().from_string (value, context);
    need_update = lay::test_and_set (m_context, context);

  } else if (name == cfg_l2n_show_all) {

    tl::from_string (value, show_all);

  } else if (name == cfg_l2n_window_mode) {

    NetlistBrowserConfig::net_window_type window = m_window;
    NetlistBrowserWindowModeConverter ().from_string (value, window);
    need_update = lay::test_and_set (m_window, window);

  } else if (name == cfg_l2n_window_dim) {

    double wdim = m_window_dim;
    tl::from_string (value, wdim);
    if (fabs (wdim - m_window_dim) > 1e-6) {
      m_window_dim = wdim;
      need_update = true;
    }

  } else if (name == cfg_l2n_max_marker_count) {

    unsigned int mc = 0;
    tl::from_string (value, mc);
    need_update = lay::test_and_set (m_max_shape_count, mc);

  } else if (name == cfg_l2n_highlight_color) {

    QColor color;
    if (! value.empty ()) {
      lay::ColorConverter ().from_string (value, color);
    }

    if (color != m_marker_color) {
      m_marker_color = color;
      need_update = true;
    }

  } else if (name == cfg_l2n_highlight_line_width) {

    int lw = 0;
    tl::from_string (value, lw);

    if (lw != m_marker_line_width) {
      m_marker_line_width = lw;
      need_update = true;
    }

  } else if (name == cfg_l2n_highlight_vertex_size) {

    int vs = 0;
    tl::from_string (value, vs);

    if (vs != m_marker_vertex_size) {
      m_marker_vertex_size = vs;
      need_update = true;
    }

  } else if (name == cfg_l2n_highlight_halo) {

    int halo = 0;
    tl::from_string (value, halo);

    if (halo != m_marker_halo) {
      m_marker_halo = halo;
      need_update = true;
    }

  } else if (name == cfg_l2n_highlight_dither_pattern) {

    int dp = 0;
    tl::from_string (value, dp);

    if (dp != m_marker_dither_pattern) {
      m_marker_dither_pattern = dp;
      need_update = true;
    }

  } else {
    taken = false;
  }

  if (active () && need_update) {
    browser_frame->set_max_shape_count (m_max_shape_count);
    browser_frame->set_window (m_window, m_window_dim, m_context);
    browser_frame->set_highlight_style (m_marker_color, m_marker_line_width, m_marker_vertex_size, m_marker_halo, m_marker_dither_pattern);
  }

  browser_frame->show_all (show_all);

  return taken;
}

void
NetlistBrowserDialog::load (int l2n_index, int cv_index)
{
#if 0 // @@@ TODO: implement
  if (! view ()->get_rdb (l2n_index)) {
    return;
  }

  if (! view ()->cellview (cv_index).is_valid ()) {
    m_layout_name = std::string ();
  } else {
    m_layout_name = view ()->cellview (cv_index)->name ();
  }

  //  set the new references (by name)
  m_l2n_name = view ()->get_rdb (l2n_index)->name ();

  //  force an update
  rdbs_changed ();
  cellviews_changed ();

  activate ();
#endif
}

void
NetlistBrowserDialog::l2ndbs_changed ()
{
#if 0 // @@@ TODO: implement
  int l2n_index = -1;

  l2n_cb->clear ();

  for (unsigned int i = 0; i < view ()->num_rdbs (); ++i) {
    const rdb::Database *rdb = view ()->get_rdb (i);
    l2n_cb->addItem (tl::to_qstring (rdb->name ()));
    if (rdb->name () == m_l2n_name) {
      l2n_index = i;
    }
  }

  //  force an update
  m_l2n_index = l2n_index;
  l2n_cb->setCurrentIndex (l2n_index);
  if (active ()) {
    update_content ();
  }
#endif
}

void
NetlistBrowserDialog::cellview_changed (int)
{
  browser_frame->update_highlights ();
}

void
NetlistBrowserDialog::cellviews_changed ()
{
  int cv_index = -1;

  layout_cb->clear ();

  for (unsigned int i = 0; i < view ()->cellviews (); ++i) {
    const lay::CellView &cv = view ()->cellview (i);
    layout_cb->addItem (tl::to_qstring (cv->name ()));
    if (cv.is_valid () && cv->name () == m_layout_name) {
      cv_index = i;
    }
  }

  layout_cb->setCurrentIndex (cv_index);
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
#if 0 // @@@ TODO: implement
  std::string state;
  if (lay::PluginRoot::instance ()) {
    lay::PluginRoot::instance ()->config_get (cfg_l2n_window_state, state);
  }
  lay::restore_dialog_state (this, state);

  //  Switch to the active cellview index when no valid one is set.
  lay::CellView cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    m_cv_index = view ()->active_cellview_index ();
  }

  if (m_l2n_index < 0 && view ()->get_rdb (0) != 0) {

    m_l2n_name = view ()->get_rdb (0)->name ();
    rdbs_changed ();

  } else {
    update_content ();
  }
#endif
}

void
NetlistBrowserDialog::update_content ()
{
#if 0 // @@@ TODO: implement
  rdb::Database *rdb = view ()->get_rdb (m_l2n_index);

  if (!rdb ) {
    central_stack->setCurrentIndex (1);
  }

  m_saveas_action->setEnabled (rdb != 0);
  m_export_action->setEnabled (rdb != 0);
  m_unload_action->setEnabled (rdb != 0);
  m_unload_all_action->setEnabled (rdb != 0);
  m_reload_action->setEnabled (rdb != 0);

  browser_frame->enable_updates (false);  //  Avoid building the internal lists several times ...
  browser_frame->set_rdb (rdb);
  browser_frame->set_max_marker_count (m_max_marker_count);
  browser_frame->set_marker_style (m_marker_color, m_marker_line_width, m_marker_vertex_size, m_marker_halo, m_marker_dither_pattern);
  browser_frame->set_window (m_window, m_window_dim, m_context);
  browser_frame->set_view (view (), m_cv_index);
  browser_frame->enable_updates (true);

  if (rdb) {
    //  Note: it appears to be required to show the browser page after it has been configured.
    //  Otherwise the header gets messed up and the configuration is reset.
    central_stack->setCurrentIndex (0);
  }

  lay::CellView cv = view ()->cellview (m_cv_index);
  m_layout_name = std::string ();
  if (cv.is_valid ()) {
    m_layout_name = cv->name ();
  }

  if (layout_cb->currentIndex () != m_cv_index) {
    layout_cb->setCurrentIndex (m_cv_index);
  }

  if (l2n_cb->currentIndex () != m_l2n_index) {
    l2n_cb->setCurrentIndex (m_l2n_index);
  }
#endif
}

void
NetlistBrowserDialog::deactivated ()
{
  if (lay::PluginRoot::instance ()) {
    lay::PluginRoot::instance ()->config_set (cfg_l2n_window_state, lay::save_dialog_state (this).c_str ());
  }

  // @@@ browser_frame->set_rdb (0);
  browser_frame->set_view (0, 0);
}

void
NetlistBrowserDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "marker_browser::show") {
    view ()->deactivate_all_browsers ();
    activate ();
  } else {
    lay::Browser::menu_activated (symbol);
  }
}

}

