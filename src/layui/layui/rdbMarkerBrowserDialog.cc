
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

#include "rdbMarkerBrowserDialog.h"
#include "rdb.h"
#include "rdbReader.h"
#include "rdbUtils.h"
#include "tlProgress.h"
#include "layLayoutViewBase.h"
#include "tlExceptions.h"
#include "layFileDialog.h"
#include "layConverters.h"
#include "layQtTools.h"
#include "layConfigurationDialog.h"
#include "dbLayoutUtils.h"
#include "dbRecursiveShapeIterator.h"

#include "ui_MarkerBrowserDialog.h"

#include <QMessageBox>
#include <QInputDialog>

#include <memory>

namespace rdb
{

extern std::string cfg_rdb_context_mode;
extern std::string cfg_rdb_show_all;
extern std::string cfg_rdb_list_shapes;
extern std::string cfg_rdb_window_state;
extern std::string cfg_rdb_window_mode;
extern std::string cfg_rdb_window_dim;
extern std::string cfg_rdb_max_marker_count;
extern std::string cfg_rdb_marker_color;
extern std::string cfg_rdb_marker_line_width;
extern std::string cfg_rdb_marker_vertex_size;
extern std::string cfg_rdb_marker_halo;
extern std::string cfg_rdb_marker_dither_pattern;

MarkerBrowserDialog::MarkerBrowserDialog (lay::Dispatcher *root, lay::LayoutViewBase *vw)
  : lay::Browser (root, vw),
    m_context (rdb::AnyCell),
    m_window (rdb::FitMarker),
    m_window_dim (0.0),
    m_max_marker_count (0),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    m_cv_index (-1),
    m_rdb_index (-1)
{
  mp_ui = new Ui::MarkerBrowserDialog ();
  mp_ui->setupUi (this);

  mp_ui->browser_frame->set_dispatcher (root);

  if (view ()) {
    view ()->cellviews_changed_event.add (this, &MarkerBrowserDialog::cellviews_changed);
    view ()->cellview_changed_event.add (this, &MarkerBrowserDialog::cellview_changed);
    view ()->rdb_list_changed_event.add (this, &MarkerBrowserDialog::rdbs_changed);
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
  connect (mp_ui->rdb_cb, SIGNAL (activated (int)), this, SLOT (rdb_index_changed (int)));
  connect (mp_ui->configure_pb, SIGNAL (clicked ()), this, SLOT (configure_clicked ()));

  cellviews_changed ();
}

MarkerBrowserDialog::~MarkerBrowserDialog ()
{
  tl::Object::detach_from_all_events ();

  delete mp_ui;
  mp_ui = 0;
}

void
MarkerBrowserDialog::configure_clicked ()
{
  lay::ConfigurationDialog config_dialog (this, lay::Dispatcher::instance (), "MarkerBrowserPlugin");
  config_dialog.exec ();
}

void
MarkerBrowserDialog::unload_all_clicked ()
{
BEGIN_PROTECTED

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

  rdb_index_changed (-1);      

END_PROTECTED
}

void
MarkerBrowserDialog::unload_clicked ()
{
BEGIN_PROTECTED

  if (m_rdb_index < int (view ()->num_rdbs ()) && m_rdb_index >= 0) {

    rdb::Database *rdb = view ()->get_rdb (m_rdb_index);
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

    int new_rdb_index = m_rdb_index;

    view ()->remove_rdb (m_rdb_index);

    // try to use another rbd ...
    if (new_rdb_index >= int (view ()->num_rdbs ())) {
      --new_rdb_index;
    }
    if (new_rdb_index < int (view ()->num_rdbs ()) && new_rdb_index >= 0) {
      rdb_index_changed (new_rdb_index);      
    }

  }

END_PROTECTED
}

static void
collect_categories (const Category *cat, std::vector <const Category *> &categories)
{
  if (cat->sub_categories ().begin () == cat->sub_categories ().end ()) {
    if (cat->num_items () > 0) {
      categories.push_back (cat);
    }
  } else {
    for (Categories::const_iterator subcat = cat->sub_categories ().begin (); subcat != cat->sub_categories ().end (); ++subcat) {
      collect_categories (&*subcat, categories);
    }
  }
}

void
MarkerBrowserDialog::export_clicked ()
{
BEGIN_PROTECTED

  if (m_rdb_index >= int (view ()->num_rdbs ()) || m_rdb_index < 0) {
    return;
  }

  const rdb::Database *rdb = view ()->get_rdb (m_rdb_index);
  if (! rdb) {
    return;
  }

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  bool ok;
  QString text = QInputDialog::getText(this, QObject::tr ("Layer Offset"),
                                             QObject::tr ("Enter the first GDS layer that is produced.\nLeave empty for not producing GDS layer numbers at all:"),
                                             QLineEdit::Normal,
                                             QString (), &ok);
  if (! ok) {
    return;
  }

  text = text.simplified ();

  int lnum = -1;
  ok = false;
  if (! text.isEmpty ()) {
    lnum = text.toInt (&ok);
    if (! ok) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid layer number: ")) + tl::to_string (text));
    }
  }

  try {

    db::Transaction transaction (view ()->is_editable () ? view ()->manager () : 0, tl::to_string (QObject::tr ("Export Markers")));

    std::vector <const Category *> categories;
    for (rdb::Categories::const_iterator cat = rdb->categories ().begin (); cat != rdb->categories ().end (); ++cat) {
      collect_categories (&*cat, categories);
    }

    for (std::vector <const Category *>::const_iterator cat = categories.begin (); cat != categories.end (); ++cat) {

      db::LayerProperties lp;
      if (lnum >= 0) {
        lp.layer = lnum++;
        lp.datatype = 0;
      }
      lp.name = (*cat)->name ();

      unsigned int layer = cv->layout ().insert_layer (lp);

      lay::LayerProperties props;
      props.set_source (lay::ParsedLayerSource (lp, m_cv_index));

      view ()->init_layer_properties (props);
      view ()->insert_layer (view ()->end_layers (), props);

      for (Cells::const_iterator cell = rdb->cells ().begin (); cell != rdb->cells ().end (); ++cell) {

        std::pair<Database::const_item_ref_iterator, Database::const_item_ref_iterator> be = rdb->items_by_cell_and_category (cell->id (), (*cat)->id ());
        if (be.first == be.second) {
          continue;
        }

        db::cell_index_type target_cell = cv.cell_index ();
        db::DCplxTrans trans;

        // TODO: be more verbose if that fails:
        std::pair<bool, db::cell_index_type> cc = cv->layout ().cell_by_name (cell->name ().c_str ());
        if (cc.first) {
          target_cell = cc.second;
        } else {
          const rdb::Cell *top_cell = rdb->cell_by_qname (rdb->top_cell_name ());
          if (top_cell) {
            std::pair <bool, db::DCplxTrans> context = cell->path_to (top_cell->id (), rdb);
            if (context.first) {
              trans = context.second;
            }
          }
        }

        for (Database::const_item_ref_iterator item = be.first; item != be.second; ++item) {

          //  Produce the shapes ...
          for (rdb::Values::const_iterator v = (*item)->values ().begin (); v != (*item)->values ().end (); ++v) {

            const rdb::Value<db::DPolygon> *polygon_value = dynamic_cast <const rdb::Value<db::DPolygon> *> (v->get ());
            const rdb::Value<db::DBox> *box_value = dynamic_cast <const rdb::Value<db::DBox> *> (v->get ());
            const rdb::Value<db::DEdge> *edge_value = dynamic_cast <const rdb::Value<db::DEdge> *> (v->get ());
            const rdb::Value<db::DEdgePair> *edge_pair_value = dynamic_cast <const rdb::Value<db::DEdgePair> *> (v->get ());

            if (polygon_value) {

              db::Polygon polygon = db::Polygon (db::DCplxTrans (1.0 / cv->layout ().dbu ()) * trans * polygon_value->value ());
              cv->layout ().cell (target_cell).shapes (layer).insert (polygon);

            } else if (edge_value) {

              db::Edge edge = db::Edge (db::DCplxTrans (1.0 / cv->layout ().dbu ()) * trans * edge_value->value ());
              cv->layout ().cell (target_cell).shapes (layer).insert (edge);

            } else if (edge_pair_value) {

              //  Note: there is no edge pair inside the database currently. Hence we convert it to a polygon
              db::EdgePair edge_pair = db::EdgePair (db::DCplxTrans (1.0 / cv->layout ().dbu ()) * trans * edge_pair_value->value ());
              cv->layout ().cell (target_cell).shapes (layer).insert (edge_pair.to_polygon (1));

            } else if (box_value) {

              db::Polygon polygon = db::Polygon (db::DCplxTrans (1.0 / cv->layout ().dbu ()) * trans * db::DPolygon (box_value->value ()));
              cv->layout ().cell (target_cell).shapes (layer).insert (polygon);

            }

          }

        }

      }

    }

    view ()->update_content ();

  } catch (...) {
    view ()->update_content ();
    throw;
  }

END_PROTECTED
}

void
MarkerBrowserDialog::saveas_clicked ()
{
BEGIN_PROTECTED

  if (m_rdb_index < int (view ()->num_rdbs ()) && m_rdb_index >= 0) {

    rdb::Database *rdb = view ()->get_rdb (m_rdb_index);
    if (rdb) {

      //  prepare and open the file dialog
      lay::FileDialog save_dialog (this, tl::to_string (QObject::tr ("Save Marker Database File")), "KLayout RDB files (*.lyrdb)");
      std::string fn (rdb->filename ());
      if (save_dialog.get_save (fn)) {

        rdb->save (fn);
        rdb->reset_modified ();

      }

    }

  }

END_PROTECTED
}

void
MarkerBrowserDialog::reload_clicked ()
{
BEGIN_PROTECTED
  if (m_rdb_index < int (view ()->num_rdbs ()) && m_rdb_index >= 0) {

    rdb::Database *rdb = view ()->get_rdb (m_rdb_index);
    if (rdb && ! rdb->filename ().empty ()) {

      mp_ui->browser_frame->set_rdb (0);
      rdb->load (rdb->filename ());
      mp_ui->browser_frame->set_rdb (rdb);

    }

  }
END_PROTECTED
}

void
MarkerBrowserDialog::open_clicked ()
{
BEGIN_PROTECTED

  //  collect the formats available ...
  std::string fmts = tl::to_string (QObject::tr ("All files (*)"));
  for (tl::Registrar<rdb::FormatDeclaration>::iterator rdr = tl::Registrar<rdb::FormatDeclaration>::begin (); rdr != tl::Registrar<rdb::FormatDeclaration>::end (); ++rdr) {
    fmts += ";;" + rdr->file_format ();
  }

  //  prepare and open the file dialog
  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Load Marker Database File")), fmts);
  if (open_dialog.get_open (m_open_filename)) {

    std::unique_ptr <rdb::Database> db (new rdb::Database ());
    db->load (m_open_filename);

    int rdb_index = view ()->add_rdb (db.release ());
    mp_ui->rdb_cb->setCurrentIndex (rdb_index);
    //  it looks like the setCurrentIndex does not issue this signal:
    rdb_index_changed (rdb_index);

  }

END_PROTECTED
}

bool 
MarkerBrowserDialog::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;
  bool show_all = mp_ui->browser_frame->show_all ();
  bool list_shapes = mp_ui->browser_frame->list_shapes ();

  if (name == cfg_rdb_context_mode) {

    context_mode_type context = m_context;
    MarkerBrowserContextModeConverter ().from_string (value, context);
    need_update = lay::test_and_set (m_context, context);

  } else if (name == cfg_rdb_list_shapes) {

    tl::from_string (value, list_shapes);

  } else if (name == cfg_rdb_show_all) {

    tl::from_string (value, show_all);

  } else if (name == cfg_rdb_window_mode) {

    window_type window = m_window;
    MarkerBrowserWindowModeConverter ().from_string (value, window);
    need_update = lay::test_and_set (m_window, window);

  } else if (name == cfg_rdb_window_dim) {

    lay::Margin wdim = lay::Margin::from_string (value);
    if (wdim != m_window_dim) {
      m_window_dim = wdim;
      need_update = true;
    }

  } else if (name == cfg_rdb_max_marker_count) {

    unsigned int mc = 0;
    tl::from_string (value, mc);
    need_update = lay::test_and_set (m_max_marker_count, mc);

  } else if (name == cfg_rdb_marker_color) {

    tl::Color color;
    if (! value.empty ()) {
      lay::ColorConverter ().from_string (value, color);
    }

    if (color != m_marker_color) {
      m_marker_color = color;
      need_update = true;
    }

  } else if (name == cfg_rdb_marker_line_width) {

    int lw = 0;
    tl::from_string (value, lw);

    if (lw != m_marker_line_width) {
      m_marker_line_width = lw;
      need_update = true;
    }

  } else if (name == cfg_rdb_marker_vertex_size) {

    int vs = 0;
    tl::from_string (value, vs);

    if (vs != m_marker_vertex_size) {
      m_marker_vertex_size = vs;
      need_update = true;
    }

  } else if (name == cfg_rdb_marker_halo) {

    int halo = 0;
    tl::from_string (value, halo);

    if (halo != m_marker_halo) {
      m_marker_halo = halo;
      need_update = true;
    }

  } else if (name == cfg_rdb_marker_dither_pattern) {

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
    mp_ui->browser_frame->set_max_marker_count (m_max_marker_count);
    mp_ui->browser_frame->set_window (m_window, m_window_dim, m_context);
    mp_ui->browser_frame->set_marker_style (m_marker_color, m_marker_line_width, m_marker_vertex_size, m_marker_halo, m_marker_dither_pattern);
  }

  mp_ui->browser_frame->show_all (show_all);
  mp_ui->browser_frame->list_shapes (list_shapes);

  return taken;
}

void
MarkerBrowserDialog::load (int rdb_index, int cv_index)
{
  if (! view ()->get_rdb (rdb_index)) {
    return;
  }

  if (! view ()->cellview (cv_index).is_valid ()) {
    m_layout_name = std::string ();
  } else {
    m_layout_name = view ()->cellview (cv_index)->name ();
  }

  //  set the new references (by name)
  m_rdb_name = view ()->get_rdb (rdb_index)->name ();

  //  force an update 
  rdbs_changed ();
  cellviews_changed ();

  activate ();
}

void 
MarkerBrowserDialog::rdbs_changed ()
{
  int rdb_index = -1;

  mp_ui->rdb_cb->clear ();

  for (unsigned int i = 0; i < view ()->num_rdbs (); ++i) {
    const rdb::Database *rdb = view ()->get_rdb (i);
    std::string text = rdb->name ();
    if (! rdb->description ().empty ()) {
      text += " (";
      text += rdb->description ();
      text += ")";
    }
    mp_ui->rdb_cb->addItem (tl::to_qstring (text));
    if (rdb->name () == m_rdb_name) {
      rdb_index = i;
    }
  }

  //  force an update
  m_rdb_index = rdb_index;
  mp_ui->rdb_cb->setCurrentIndex (rdb_index);
  if (active ()) {
    update_content ();
  }
}

void 
MarkerBrowserDialog::cellview_changed (int)
{
  mp_ui->browser_frame->update_markers ();
}

void 
MarkerBrowserDialog::cellviews_changed ()
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
MarkerBrowserDialog::rdb_index_changed (int index)
{
  if (m_rdb_index != index) {
    m_rdb_index = index;
    if (active ()) {
      update_content ();
    }
  }
}

void 
MarkerBrowserDialog::cv_index_changed (int index)
{
  if (m_cv_index != index) {
    m_cv_index = index;
    if (active ()) {
      update_content ();
    }
  }
}

void 
MarkerBrowserDialog::activated ()
{
  std::string state;
  view ()->config_get (cfg_rdb_window_state, state);
  lay::restore_dialog_state (this, state);

  //  Switch to the active cellview index when no valid one is set.
  lay::CellView cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    m_cv_index = view ()->active_cellview_index ();
  }

  if (m_rdb_index < 0 && view ()->get_rdb (0) != 0) {

    m_rdb_name = view ()->get_rdb (0)->name ();
    rdbs_changed ();

  } else {
    update_content ();
  }
}

void
MarkerBrowserDialog::update_content ()
{
  rdb::Database *rdb = view ()->get_rdb (m_rdb_index);
  
  if (!rdb ) {
    mp_ui->central_stack->setCurrentIndex (1);
  }

  m_saveas_action->setEnabled (rdb != 0);
  m_export_action->setEnabled (rdb != 0);
  m_unload_action->setEnabled (rdb != 0);
  m_unload_all_action->setEnabled (rdb != 0);
  m_reload_action->setEnabled (rdb != 0);

  mp_ui->browser_frame->enable_updates (false);  //  Avoid building the internal lists several times ...
  mp_ui->browser_frame->set_rdb (0);    //  force update
  mp_ui->browser_frame->set_rdb (rdb);
  mp_ui->browser_frame->set_max_marker_count (m_max_marker_count);
  mp_ui->browser_frame->set_marker_style (m_marker_color, m_marker_line_width, m_marker_vertex_size, m_marker_halo, m_marker_dither_pattern);
  mp_ui->browser_frame->set_window (m_window, m_window_dim, m_context);
  mp_ui->browser_frame->set_view (view (), m_cv_index);
  mp_ui->browser_frame->enable_updates (true);

  if (rdb) {
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

  if (mp_ui->rdb_cb->currentIndex () != m_rdb_index) {
    mp_ui->rdb_cb->setCurrentIndex (m_rdb_index);
  }
}

void 
MarkerBrowserDialog::deactivated ()
{
  if (lay::Dispatcher::instance ()) {
    lay::Dispatcher::instance ()->config_set (cfg_rdb_window_state, lay::save_dialog_state (this).c_str ());
  }

  mp_ui->browser_frame->set_rdb (0);
  mp_ui->browser_frame->set_view (0, 0);
}

void 
MarkerBrowserDialog::scan_layer ()
{
  std::vector<lay::LayerPropertiesConstIterator> layers = view ()->selected_layers ();
  if (layers.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer selected to get shapes from")));
  }

  int cv_index = -1;
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    if (!(*l)->has_children ()) {
      if (cv_index < 0) {
        cv_index = (*l)->cellview_index ();
      } else if ((*l)->cellview_index () >= 0) {
        if (cv_index != (*l)->cellview_index ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("All layers must originate from the same layout")));
        }
      }
    }
  }

  if (cv_index < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected")));
  }

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Shapes To Markers")), 10000);
  progress.set_format (tl::to_string (QObject::tr ("%.0f0000 markers")));
  progress.set_unit (10000);

  const lay::CellView &cv = view ()->cellview (cv_index);
  const db::Layout &layout = cv->layout ();

  std::unique_ptr<rdb::Database> rdb (new rdb::Database ());
  rdb->set_name ("Shapes");
  rdb->set_top_cell_name (layout.cell_name (cv.cell_index ()));
  rdb::Cell *rdb_top_cell = rdb->create_cell (rdb->top_cell_name ());

  std::string desc;
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    if (!(*l)->has_children () && (*l)->cellview_index () == cv_index && layout.is_valid_layer ((*l)->layer_index ())) {
      if (! desc.empty ()) {
        desc += ", ";
      }
      desc += layout.get_properties ((*l)->layer_index ()).to_string ();
    }
  }
  desc = tl::to_string (tr ("Hierarchical shapes of layer(s) ")) + desc;
  desc += " ";
  desc += tl::to_string (tr ("from cell "));
  desc += cv->layout ().cell_name (cv.cell_index ());
  rdb->set_description (desc);

  std::set<db::cell_index_type> called_cells;
  called_cells.insert (cv.cell_index ());
  cv->layout ().cell (cv.cell_index ()).collect_called_cells (called_cells);

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = layers.begin (); l != layers.end (); ++l) {

    if (!(*l)->has_children () && (*l)->cellview_index () == cv_index && layout.is_valid_layer ((*l)->layer_index ())) {

      rdb::Category *cat = rdb->create_category (layout.get_properties ((*l)->layer_index ()).to_string ());

      for (db::Layout::const_iterator cid = layout.begin (); cid != layout.end (); ++cid) {

        if (called_cells.find (cid->cell_index ()) == called_cells.end ()) {
          continue;
        }

        const db::Cell &cell = *cid;
        if (cell.shapes ((*l)->layer_index ()).size () > 0) {

          std::string cn = layout.cell_name (cell.cell_index ());
          const rdb::Cell *rdb_cell = rdb->cell_by_qname (cn);
          if (! rdb_cell) {

            rdb::Cell *rdb_cell_nc = rdb->create_cell (cn);
            rdb_cell = rdb_cell_nc;

            std::pair<bool, db::ICplxTrans> ctx = db::find_layout_context (layout, cell.cell_index (), cv.cell_index ());
            if (ctx.first) {
              db::DCplxTrans t = db::DCplxTrans (layout.dbu ()) * db::DCplxTrans (ctx.second) * db::DCplxTrans (1.0 / layout.dbu ());
              rdb_cell_nc->references ().insert (Reference (t, rdb_top_cell->id ()));
            }

          }
          
          for (db::ShapeIterator shape = cell.shapes ((*l)->layer_index ()).begin (db::ShapeIterator::All); ! shape.at_end (); ++shape) {

            rdb::create_item_from_shape (rdb.get (), rdb_cell->id (), cat->id (), db::CplxTrans (layout.dbu ()), *shape);

            ++progress;

          }

        }

      }

    }

  }

  unsigned int rdb_index = view ()->add_rdb (rdb.release ());
  view ()->open_rdb_browser (rdb_index, cv_index);
}

void 
MarkerBrowserDialog::scan_layer_flat ()
{
  std::vector<lay::LayerPropertiesConstIterator> layers = view ()->selected_layers ();
  if (layers.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer selected to get shapes from")));
  }

  int cv_index = -1;
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    if (!(*l)->has_children ()) {
      if (cv_index < 0) {
        cv_index = (*l)->cellview_index ();
      } else if ((*l)->cellview_index () >= 0) {
        if (cv_index != (*l)->cellview_index ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("All layers must originate from the same layout")));
        }
      }
    }
  }

  if (cv_index < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected")));
  }

  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Shapes To Markers")), 10000);
  progress.set_format (tl::to_string (QObject::tr ("%.0f0000 markers")));
  progress.set_unit (10000);

  const lay::CellView &cv = view ()->cellview (cv_index);
  const db::Layout &layout = cv->layout ();

  std::unique_ptr<rdb::Database> rdb (new rdb::Database ());
  rdb->set_name ("Shapes");
  rdb->set_top_cell_name (layout.cell_name (cv.cell_index ()));
  rdb::Cell *rdb_top_cell = rdb->create_cell (rdb->top_cell_name ());

  std::string desc;
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    if (!(*l)->has_children () && (*l)->cellview_index () == cv_index && layout.is_valid_layer ((*l)->layer_index ())) {
      if (! desc.empty ()) {
        desc += ", ";
      }
      desc += layout.get_properties ((*l)->layer_index ()).to_string ();
    }
  }
  desc = tl::to_string (tr ("Flat shapes of layer(s) ")) + desc;
  desc += " ";
  desc += tl::to_string (tr ("from cell "));
  desc += cv->layout ().cell_name (cv.cell_index ());
  rdb->set_description (desc);

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = layers.begin (); l != layers.end (); ++l) {

    if (!(*l)->has_children () && (*l)->cellview_index () == cv_index && layout.is_valid_layer ((*l)->layer_index ())) {

      rdb::Category *cat = rdb->create_category (layout.get_properties ((*l)->layer_index ()).to_string ()); 

      db::RecursiveShapeIterator shape (layout, *cv.cell (), (*l)->layer_index ());
      while (! shape.at_end ()) {

        rdb::create_item_from_shape (rdb.get (), rdb_top_cell->id (), cat->id (), db::CplxTrans (layout.dbu ()) * shape.trans (), *shape);

        ++progress;
        ++shape;

      }

    }

  }

  unsigned int rdb_index = view ()->add_rdb (rdb.release ());
  view ()->open_rdb_browser (rdb_index, cv_index);
}

void 
MarkerBrowserDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "marker_browser::show") {
    view ()->deactivate_all_browsers ();
    activate ();
  } else if (symbol == "marker_browser::scan_layers") {
    scan_layer ();
  } else if (symbol == "marker_browser::scan_layers_flat") {
    scan_layer_flat ();
  } else {
    lay::Browser::menu_activated (symbol);
  }
}

}

#endif
