
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


#include "dbNetTracerIO.h"

#include "layNetTracerDialog.h"
#include "layNetTracerConfig.h"
#include "layConfigurationDialog.h"
#include "laybasicConfig.h"
#include "layConverters.h"
#include "layFinder.h"
#include "layLayoutView.h"
#include "layTechSetupDialog.h"
#include "layFileDialog.h"
#include "layQtTools.h"
#include "tlExceptions.h"
#include "tlXMLWriter.h"
#include "tlUtils.h"
#include "gsiDecl.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QItemDelegate>
#include <QHeaderView>
#include <QPainter>

#include <fstream>
#include <sstream>

namespace lay
{

// -----------------------------------------------------------------------------------
//  NetTracerDialog implementation

NetTracerDialog::NetTracerDialog (lay::Dispatcher *root, LayoutViewBase *view)
  : lay::Browser (root, view, "net_tracer_dialog"),
    lay::ViewService (view->canvas ()), 
    m_cv_index (0), 
    m_net_index (1),
    m_window (lay::NTFitNet),
    m_window_dim (0.0),
    m_max_marker_count (0),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    m_marker_intensity (0),
    m_auto_color_enabled (false),
    m_auto_color_index (0),
    m_mouse_state (0),
    mp_view (view)
{
  mp_export_file_dialog = new lay::FileDialog (this, tl::to_string (QObject::tr ("Export Net")), tl::to_string (QObject::tr ("KLayout net files (*.lyn);;All files (*)")));

  Ui::NetTracerDialog::setupUi (this);

  connect (add_pb, SIGNAL (clicked ()), this, SLOT (trace_net_button_clicked ()));
  connect (add2_pb, SIGNAL (clicked ()), this, SLOT (trace_path_button_clicked ()));
  connect (del_pb, SIGNAL (clicked ()), this, SLOT (delete_button_clicked ()));
  connect (clear_all_pb, SIGNAL (clicked ()), this, SLOT (clear_all_button_clicked ()));
  connect (detailed_cb, SIGNAL (clicked ()), this, SLOT (detailed_mode_clicked ()));
  connect (export_pb, SIGNAL (clicked ()), this, SLOT (export_clicked ()));
  connect (redo_pb, SIGNAL (clicked ()), this, SLOT (redo_trace_clicked ()));
  connect (export_text_pb, SIGNAL (clicked ()), this, SLOT (export_text_clicked ()));
  connect (configure_pb, SIGNAL (clicked ()), this, SLOT (configure_clicked ()));
  connect (stack_pb, SIGNAL (clicked ()), this, SLOT (layer_stack_clicked ()));
  connect (net_list, SIGNAL (itemSelectionChanged ()), this, SLOT (item_selection_changed ()));
  connect (net_color, SIGNAL (color_changed (QColor)), this, SLOT (net_color_changed (QColor)));
  connect (net_list, SIGNAL (itemDoubleClicked (QListWidgetItem *)), this, SLOT (item_double_clicked (QListWidgetItem *)));
  connect (sticky_cbx, SIGNAL (clicked ()), this, SLOT (sticky_mode_clicked ()));

  view->layer_list_changed_event.add (this, &NetTracerDialog::layer_list_changed);

  attach_events ();
  update_info ();
  update_list_of_stacks ();
}

NetTracerDialog::~NetTracerDialog ()
{
  clear_markers ();
  clear_nets ();
}

void
NetTracerDialog::attach_events ()
{
  detach_from_all_events ();

  mp_view->layer_list_changed_event.add (this, &NetTracerDialog::layer_list_changed);

  db::Technologies::instance ()->technology_changed_event.add (this, &NetTracerDialog::update_list_of_stacks_with_technology);
  db::Technologies::instance ()->technologies_changed_event.add (this, &NetTracerDialog::update_list_of_stacks);

  mp_view->cellviews_changed_event.add (this, &NetTracerDialog::update_list_of_stacks);
  mp_view->apply_technology_event.add (this, &NetTracerDialog::update_list_of_stacks_with_cellview);
}

void
NetTracerDialog::update_list_of_stacks_with_technology (db::Technology *)
{
  update_list_of_stacks ();
}

void
NetTracerDialog::update_list_of_stacks_with_cellview (int)
{
  update_list_of_stacks ();
}

void
NetTracerDialog::update_list_of_stacks ()
{
  QString current_name = stack_selector->currentText ();

  std::set<QString> names;
  for (unsigned int cvi = 0; cvi < mp_view->cellviews (); ++cvi) {
    const db::Technology *tech = mp_view->cellview (cvi)->technology ();
    if (tech) {
      const db::NetTracerTechnologyComponent *tech_component = dynamic_cast <const db::NetTracerTechnologyComponent *> (tech->component_by_name (db::net_tracer_component_name ()));
      if (tech_component) {
        for (auto d = tech_component->begin (); d != tech_component->end (); ++d) {
          names.insert (tl::to_qstring (d->name ()));
        }
      }
    }
  }

  stack_selector->clear ();

  int current_index = 0;
  int i = 0;
  for (auto n = names.begin (); n != names.end (); ++n, ++i) {
    if (n->isEmpty ()) {
      stack_selector->addItem (tr ("(default)"), QVariant (*n));
    } else {
      stack_selector->addItem (*n, QVariant (*n));
    }
    if (*n == current_name) {
      current_index = i;
    }
  }

  stack_selector->setVisible (stack_selector->count () >= 2);
  stack_selector->setCurrentIndex (current_index);
}

void
NetTracerDialog::clear_nets ()
{
  for (std::vector <db::NetTracerNet *>::iterator n = mp_nets.begin (); n != mp_nets.end (); ++n) {
    delete *n;
  }
  mp_nets.clear ();
}

void 
NetTracerDialog::item_double_clicked (QListWidgetItem *item)
{
  int item_index = net_list->row (item);
  if (item_index >= 0 && item_index < int (mp_nets.size ())) {

    QString name = tl::to_qstring (mp_nets [item_index]->name ());

    bool ok = false;
    name = QInputDialog::getText (this,
                                  QObject::tr ("Net Name"),
                                  QObject::tr ("Enter new net name"),
                                  QLineEdit::Normal, name,
                                  &ok);

    if (ok) {

      mp_nets [item_index]->set_name (tl::to_string (name));

      update_list ();
      item_selection_changed ();

    }

  }
}

void
NetTracerDialog::drag_cancel ()
{
  if (m_mouse_state > 0) {

    view ()->message ();
    ui ()->ungrab_mouse (this);
    set_cursor (lay::Cursor::none);

    m_mouse_state = 0;

  }
}

bool
NetTracerDialog::claims_message_bar () const
{
  return true;
}

bool 
NetTracerDialog::mouse_move_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool prio) 
{ 
  if (prio && m_mouse_state != 0) {
    set_cursor (lay::Cursor::cross);
  }
  return false;
}

bool 
NetTracerDialog::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio) 
{ 
  if (prio && (buttons & lay::LeftButton) != 0 && m_mouse_state != 0) {

    if (m_mouse_state == 2) {

      m_mouse_first_point = p;
      m_mouse_state = 3;

      view ()->message (tl::to_string (QObject::tr ("Click on the second point in the net")));

    } else {

      bool trace_path = (m_mouse_state == 3);

      if (trace_path || ! sticky_cbx->isChecked ()) {
        release_mouse ();
      }

      //  prepare for the net tracing 
      clear_markers ();

      double l = double (view ()->search_range ()) / ui ()->mouse_event_trans ().mag ();

      db::DBox start_search_box = db::DBox (p, p).enlarged (db::DVector (l, l));

      db::DBox stop_search_box;
      if (trace_path) {
        stop_search_box = db::DBox (m_mouse_first_point, m_mouse_first_point).enlarged (db::DVector (l, l));
      }

      db::NetTracerNet *net = do_trace (start_search_box, stop_search_box, trace_path);
      if (net) {

        //  create a new net taking the shapes from the tracer
        mp_nets.push_back (net);

        //  do auto coloring
        if (m_auto_color_enabled) {
          if (m_auto_color_index < int (m_auto_colors.colors ())) {
            mp_nets.back ()->set_color (m_auto_colors.color_by_index (m_auto_color_index));
          }
          ++m_auto_color_index;
          if (m_auto_color_index >= int (m_auto_colors.colors ())) {
            m_auto_color_index = 0;
          }
        }

        std::string n = mp_nets.back ()->name ();
        if (n.empty ()) {
          mp_nets.back ()->set_name (tl::sprintf (tl::to_string (QObject::tr ("Net%d")), m_net_index++));
        }

        update_list ();
        item_selection_changed ();
        net_list->setCurrentItem (net_list->item (int (mp_nets.size () - 1)));

      }

    }

    return true;

  } else {
    return false; 
  }
}

void 
NetTracerDialog::redo_trace_clicked ()
{
BEGIN_PROTECTED

  std::set <db::NetTracerNet *> selected_nets;

  QList<QListWidgetItem *> selected_items = net_list->selectedItems ();
  for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {
    int item_index = net_list->row (*item);
    if (item_index >= 0 && item_index < int (mp_nets.size ())) {
      selected_nets.insert (mp_nets [item_index]);
    }
  }

  std::vector <db::NetTracerNet *> nets;
  nets.swap (mp_nets);

  m_net_index = 1;

  std::vector <size_t> new_selection;

  for (std::vector <db::NetTracerNet *>::const_iterator n = nets.begin (); n != nets.end (); ++n) {

    try {

      db::NetTracerNet *net = do_trace ((*n)->start_search_box (), (*n)->stop_search_box (), (*n)->trace_path_flag ());
      if (net) {

        //  create a new net taking the shapes from the tracer
        mp_nets.push_back (net);

        net->set_color ((*n)->color ());

        std::string name = mp_nets.back ()->name ();
        if (name.empty ()) {
          mp_nets.back ()->set_name (tl::sprintf (tl::to_string (QObject::tr ("Net%d")), m_net_index++));
        }

        if (selected_nets.find (*n) != selected_nets.end ()) {
          new_selection.push_back (mp_nets.size () - 1);
        }

      }

    } catch (...) {
      //  ignore errors on redo
    }

    delete *n;

  }

  //  re-establish the selection
  net_list->blockSignals (true);
  update_list ();
  for (std::vector <size_t>::const_iterator i = new_selection.begin (); i != new_selection.end (); ++i) {
    net_list->item (int (*i))->setSelected (true);
  }
  net_list->blockSignals (false);

  item_selection_changed ();

END_PROTECTED
}

bool
NetTracerDialog::get_net_tracer_setup_from_tech (const std::string &tech_name, const std::string &stack_name, const db::Layout &layout, db::NetTracerData &data)
{
  //  fetch the net tracer data from the technology and apply to the current layout
  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (tech_name);
  if (! tech) {
    return false;
  }

  const db::NetTracerTechnologyComponent *tech_component = dynamic_cast <const db::NetTracerTechnologyComponent *> (tech->component_by_name (db::net_tracer_component_name ()));
  if (! tech_component) {
    return false;
  }

  const db::NetTracerConnectivity *connectivity = 0;
  for (auto d = tech_component->begin (); d != tech_component->end () && ! connectivity; ++d) {
    if (d->name () == stack_name) {
      connectivity = d.operator-> ();
    }
  }

  if (! connectivity) {
    return false;
  }

  //  Set up the net tracer environment
  data = connectivity->get_tracer_data (layout);
  return true;
}

bool
NetTracerDialog::get_net_tracer_setup (const lay::CellView &cv, db::NetTracerData &data)
{
  //  fetch the net tracer data from the technology and apply to the current layout
  const db::Technology *tech = cv->technology ();
  if (! tech) {
    return false;
  }

  const std::string &tech_name = tech->name ();
  std::string stack_name = tl::to_string (stack_selector->itemData (stack_selector->currentIndex ()).toString ());

  return get_net_tracer_setup_from_tech (tech_name, stack_name, cv->layout (), data);
}

db::NetTracerNet *
NetTracerDialog::do_trace (const db::DBox &start_search_box, const db::DBox &stop_search_box, bool trace_path)
{
  unsigned int start_layer = 0;
  db::Point start_point;
  db::Shape start_shape;

  //  locate the seed
  {

    lay::ShapeFinder finder (true /*point mode*/, false /*all levels*/, db::ShapeIterator::All);
    finder.set_consider_viewport (false);

    //  go through all visible layers of all cellviews and find a seed shape
    for (lay::LayerPropertiesConstIterator lprop = view ()->begin_layers (); ! lprop.at_end (); ++lprop) {
      if (lprop->is_visual ()) {
        finder.find (view (), *lprop, start_search_box);
      }
    }

    //  return, if no shape was found
    lay::ShapeFinder::iterator r = finder.begin ();
    if (r == finder.end ()) {
      return 0;
    }

    m_cv_index = r->cv_index ();
    start_shape = r->shape ();
    start_layer = r->layer ();

  }

  //  determine the cellview
  lay::CellView cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return 0;
  }

  //  determine the start point
  {

    std::vector<db::DCplxTrans> tv = view ()->cv_transform_variants (m_cv_index, start_layer);
    if (tv.empty ()) {
      return 0;
    }

    db::CplxTrans tt = tv.front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();

    start_point = tt.inverted ().trans (start_search_box.center ());

    //  stop if the center start point is not inside the start polygon
    db::Polygon poly;
    if (start_shape.polygon (poly) && db::inside_poly (poly.begin_edge (), start_point) < 0) {
      return 0;
    }

  }

  //  Set up the net tracer environment
  db::NetTracerData tracer_data;
  if (! get_net_tracer_setup (cv, tracer_data)) {
    return 0;
  }

  unsigned int stop_layer = 0;
  db::Point stop_point;

  //  locate the stop shape (the second mouse click)
  if (trace_path) {

    lay::ShapeFinder finder (true /*point mode*/, false /*all levels*/, db::ShapeIterator::All);
    finder.set_consider_viewport (false);

    //  go through all visible layers of all cellviews and find a seed shape
    for (lay::LayerPropertiesConstIterator lprop = view ()->begin_layers (); ! lprop.at_end (); ++lprop) {
      if (lprop->is_visual ()) {
        finder.find (view (), *lprop, stop_search_box);
      }
    }

    //  return, if no shape was found
    lay::ShapeFinder::iterator r = finder.begin ();
    if (r == finder.end ()) {
      return 0;
    }

    if (r->cv_index () != m_cv_index) {
      throw tl::Exception (tl::to_string (QObject::tr ("Both shapes for path tracing must come from the same layout")));
    }

    std::vector<db::DCplxTrans> tv = view ()->cv_transform_variants (m_cv_index, r->layer ());
    if (tv.empty ()) {
      return 0;
    }

    db::CplxTrans tt = tv.front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();

    stop_point = tt.inverted ().trans (stop_search_box.center ());
    stop_layer = r->layer ();

    //  stop if the center stop point is not inside the stop polygon
    db::Polygon poly;
    if (r->shape ().polygon (poly) && db::inside_poly (poly.begin_edge (), stop_point) < 0) {
      return 0;
    }

  }

  db::NetTracer net_tracer;
  net_tracer.set_trace_depth (get_trace_depth ());

  //  and trace
  if (trace_path) {
    net_tracer.trace (cv->layout (), *cv.cell (), start_point, start_layer, stop_point, stop_layer, tracer_data);
  } else {
    net_tracer.trace (cv->layout (), *cv.cell (), start_point, start_layer, tracer_data);
  }

  if (net_tracer.begin () == net_tracer.end ()) {

    return 0;

  } else {

    //  create a new net taking the shapes from the tracer
    db::NetTracerNet *net = new db::NetTracerNet (net_tracer, db::ICplxTrans (cv.context_trans ()), cv->layout (), cv.cell_index (), cv->filename (), cv->name (), tracer_data);
    net->set_start_search_box (start_search_box);
    net->set_stop_search_box (stop_search_box);
    net->set_trace_path_flag (trace_path);

    return net;

  }
}

bool 
NetTracerDialog::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;

  if (name == lay::cfg_background_color) {

    need_update = true;

  } else if (name == cfg_nt_trace_depth) {
      
    unsigned int n = 0;
    tl::from_string (value, n);
    if (n > 0) {
      depth_le->setText (tl::to_qstring (tl::to_string (n)));
    } else {
      depth_le->setText (QString ());
    }

  } else if (name == cfg_nt_marker_cycle_colors) {

    m_auto_colors.from_string (value, true);

  } else if (name == cfg_nt_marker_cycle_colors_enabled) {

    bool en = false;
    tl::from_string (value, en);
    if (en != m_auto_color_enabled) {
      m_auto_color_index = 0;
      m_auto_color_enabled = en;
    }

  } else if (name == cfg_nt_window_mode) {

    nt_window_type window = m_window;
    NetTracerWindowModeConverter ().from_string (value, window);
    need_update = lay::test_and_set (m_window, window);

  } else if (name == cfg_nt_window_dim) {

    double wdim = m_window_dim;
    tl::from_string (value, wdim);
    if (fabs (wdim - m_window_dim) > 1e-6) {
      m_window_dim = wdim;
      need_update = true;
    }

  } else if (name == cfg_nt_max_shapes_highlighted) {

    unsigned int mc = 0;
    tl::from_string (value, mc);
    need_update = lay::test_and_set (m_max_marker_count, mc);

  } else if (name == cfg_nt_marker_color) {

    tl::Color color;
    if (! value.empty ()) {
      lay::ColorConverter ().from_string (value, color);
    }

    if (color != m_marker_color) {
      m_marker_color = color;
      need_update = true;
    }

  } else if (name == cfg_nt_marker_line_width) {

    int lw = 0;
    tl::from_string (value, lw);

    if (lw != m_marker_line_width) {
      m_marker_line_width = lw;
      need_update = true;
    }

  } else if (name == cfg_nt_marker_vertex_size) {

    int vs = 0;
    tl::from_string (value, vs);

    if (vs != m_marker_vertex_size) {
      m_marker_vertex_size = vs;
      need_update = true;
    }

  } else if (name == cfg_nt_marker_halo) {

    int halo = 0;
    tl::from_string (value, halo);

    if (halo != m_marker_halo) {
      m_marker_halo = halo;
      need_update = true;
    }

  } else if (name == cfg_nt_marker_dither_pattern) {

    int dp = 0;
    tl::from_string (value, dp);

    if (dp != m_marker_dither_pattern) {
      m_marker_dither_pattern = dp;
      need_update = true;
    }

  } else if (name == cfg_nt_marker_intensity) {

    int bo = 0;
    tl::from_string (value, bo);

    if (bo != m_marker_intensity) {
      m_marker_intensity = bo;
      need_update = true;
    }

  } else {
    taken = false;
  }

  if (active () && need_update) {
    update_highlights ();
    adjust_view ();
    update_info ();
    update_list_of_stacks ();
  }

  return taken;
}

void  
NetTracerDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "lay::net_trace") {

    const lay::CellView &cv = view ()->cellview (view ()->active_cellview_index ());
    if (cv.is_valid ()) {
      show ();
      activateWindow ();
      raise ();
      activate ();
    }

  } else if (symbol == "lay::edit_layer_stack") {

    layer_stack_clicked ();

  } else if (symbol == "lay::trace_all_nets" || symbol == "lay::trace_all_nets_flat") {

    bool flat = symbol == "lay::trace_all_nets_flat";

    const lay::CellView &cv = view ()->cellview (view ()->active_cellview_index ());
    if (cv.is_valid ()) {

      db::RecursiveShapeIterator si (cv->layout (), *cv.cell (), std::vector<unsigned int> ());
      std::unique_ptr <db::LayoutToNetlist> l2ndb (new db::LayoutToNetlist (si));
      trace_all_nets (l2ndb.get (), cv, flat);

      if (l2ndb->netlist ()) {
        unsigned int l2ndb_index = view ()->add_l2ndb (l2ndb.release ());
        view ()->open_l2ndb_browser (l2ndb_index, view ()->index_of_cellview (&cv));
      }

    }

  } else {
    lay::Browser::menu_activated (symbol);
  }
}

void 
NetTracerDialog::net_color_changed (QColor qc)
{
  bool changed = false;
  tl::Color color (qc);

  QList<QListWidgetItem *> selected_items = net_list->selectedItems ();
  for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {
    int item_index = net_list->row (*item);
    if (item_index >= 0 && item_index < int (mp_nets.size ())) {
      if (color != mp_nets [item_index]->color ()) {
        mp_nets [item_index]->set_color (color);
        changed = true;
      }
    }
  }

  if (changed) {
    update_highlights ();
    adjust_view ();
    update_list ();
  }
}

void 
NetTracerDialog::item_selection_changed ()
{
  if (active ()) {
    update_highlights ();
    adjust_view ();
    update_info ();
  }
}

void
NetTracerDialog::detailed_mode_clicked ()
{
  update_info ();
}

void
NetTracerDialog::update_info ()
{
  bool detailed = detailed_cb->isChecked ();

  std::ostringstream info_stream;
  info_stream.imbue (std::locale ("C"));

  tl::XMLWriter info (info_stream); 
  
  info.start_document ("");
  info.start_element ("html");
  info.start_element ("body");

  QList<QListWidgetItem *> selected_items = net_list->selectedItems ();

  if (selected_items.size () == 0) {

    info.start_element ("p");
    info.cdata (tl::to_string (QObject::tr ("No net selected")));
    info.end_element ("p");

  } else {

    size_t ntot = 0;

    info.start_element ("p");
    bool first = true;

    for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {

      int item_index = net_list->row (*item);
      if (item_index >= 0 && item_index < int (mp_nets.size ())) {

        if (! first) {
          info.start_element ("br");
          info.end_element ("br");
        }
        first = false;

        info.cdata (tl::sprintf (tl::to_string (QObject::tr ("%ld Shapes")), mp_nets[item_index]->size ()));
        if (selected_items.size () > 1) {
          info.cdata (" (" + mp_nets[item_index]->name () + ")");
        }
        if (mp_nets[item_index]->incomplete ()) {
          info.start_element ("span");
          info.write_attribute ("style", "color:red; font-weight: bold");
          info.cdata(" (" + tl::to_string (QObject::tr ("Net is incomplete")) + ") ");
          info.end_element ("span");
        }

        ntot += mp_nets[item_index]->size ();

      }
    }

    info.end_element ("p");

    if (selected_items.size () > 1) {
      info.start_element ("p");
      info.cdata (tl::sprintf (tl::to_string (QObject::tr ("%ld Shapes (total)")), ntot));
      info.end_element ("p");
    }

    if (ntot > m_max_marker_count) {
      info.start_element ("p");
      info.write_attribute ("style", "color:red; font-weight: bold");
      info.cdata (tl::to_string (QObject::tr ("Not all shapes are highlighted")));
      info.end_element ("p");
    }

    if (selected_items.size () == 1) {

      int item_index = net_list->row (selected_items [0]);
      if (item_index >= 0 && item_index < int (mp_nets.size ())) {

        db::CplxTrans dbu (mp_nets [item_index]->dbu ());
        db::VCplxTrans dbuinv (1.0 / mp_nets [item_index]->dbu ());
        double dbu_unidir = mp_nets [item_index]->dbu ();

        size_t max_labels = 1000;
        size_t max_cells = 1000;
        size_t max_shapes = 2000;

        if (detailed) {

          info.start_element ("h3");
          info.cdata (tl::to_string (QObject::tr ("General:")));
          info.end_element ("h3");

          info.start_element ("p");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Net name: ")));
          info.end_element ("b");
          info.cdata (mp_nets[item_index]->name ());
          info.start_element ("br");
          info.end_element ("br");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Top cell: ")));
          info.end_element ("b");
          info.cdata (mp_nets[item_index]->top_cell_name ());
          info.start_element ("br");
          info.end_element ("br");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Layout: ")));
          info.end_element ("b");
          info.cdata (mp_nets[item_index]->layout_name ());
          info.start_element ("br");
          info.end_element ("br");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Layout file: ")));
          info.end_element ("b");
          info.cdata (mp_nets[item_index]->layout_filename ());
          info.end_element ("p");

          bool incomplete = false;
          std::map<std::string, std::set<std::string> > shapes;

          //  map as (layernumber, group of shapes by layer): 
          std::map<unsigned int, std::vector<db::Polygon> > shapes_by_layer;
          std::map<unsigned int, std::string> layer_names;
          std::map<unsigned int, db::coord_traits<db::Coord>::area_type> statinfo_area;
          std::map<unsigned int, db::coord_traits<db::Coord>::perimeter_type> statinfo_perimeter;

          size_t tot_shapes = 0;
          for (db::NetTracerNet::iterator net_shape = mp_nets [item_index]->begin (); net_shape != mp_nets [item_index]->end (); ++net_shape) {

            if (tot_shapes++ >= max_shapes) {
              incomplete = true;
              break;
            } 

            std::string l (mp_nets [item_index]->layer_for (net_shape->layer ()).to_string ());
            if (l.empty ()) {
              l = "<anonymous>";
            }

            //  Get layer number, to be used as key for map of merged_shapes 
            unsigned int lay_num = net_shape->layer ();

            //  Check if layer is already detected, otherwise create vector-of-Shape object to hold shapes 
            //  plus initialize the perimeter and area sums
            std::map<unsigned int, std::vector<db::Polygon> >::iterator s = shapes_by_layer.find (lay_num); 
            if (s == shapes_by_layer.end ()) {
              s = shapes_by_layer.insert (std::make_pair (lay_num, std::vector<db::Polygon> ())).first;
              layer_names.insert (std::make_pair (lay_num, l));
              statinfo_perimeter.insert (std::make_pair (lay_num, db::coord_traits<db::Coord>::perimeter_type (0)));
              statinfo_area.insert (std::make_pair (lay_num, db::coord_traits<db::Coord>::area_type (0)));
            }

            //  As layer now certainly exists, insert the shape 
            if (net_shape->shape ().is_box () || net_shape->shape ().is_path () || net_shape->shape ().is_polygon ()) {
              s->second.push_back (db::Polygon ());
              net_shape->shape ().polygon (s->second.back ());
              s->second.back ().transform (net_shape->trans ());
            }

            std::string c (std::string (mp_nets [item_index]->cell_name (net_shape->cell_index ())));
            c += " (with ";
            c += (dbu * db::CplxTrans (net_shape->trans ()) * dbuinv).to_string ();
            c += ")";

            std::string t;

            if (net_shape->shape ().is_text ()) {
              db::Text text;
              net_shape->shape ().text (text);
              t = tl::to_string (QObject::tr ("text on ")) + l + ": " + (dbu * text).to_string ();
            } else if (net_shape->shape ().is_box ()) {
              db::Box box;
              net_shape->shape ().box (box);
              t = tl::to_string (QObject::tr ("box on ")) + l + ": " + (dbu * box).to_string ();
            } else if (net_shape->shape ().is_path ()) {
              db::Path path;
              net_shape->shape ().path (path);
              t = tl::to_string (QObject::tr ("path on ")) + l + ": " + (dbu * path).to_string ();
            } else if (net_shape->shape ().is_polygon ()) {
              db::Polygon polygon;
              net_shape->shape ().polygon (polygon);
              t = tl::to_string (QObject::tr ("polygon on ")) + l + ": " + (dbu * polygon).to_string ();
            }

            if (! t.empty ()) {
              shapes.insert (std::make_pair (c, std::set<std::string> ())).first->second.insert (t);
            }

          }

          //  Try to merge all shaped to polygons, use Map of (layernumber, group of polygons by layer) 
          std::map<unsigned int, std::vector<db::Polygon> > polygons_by_layer;
          for (std::map<unsigned int, std::vector<db::Polygon> >::iterator i = shapes_by_layer.begin(); i != shapes_by_layer.end (); ++i) { 

            unsigned int l = i->first;

            db::EdgeProcessor ep;
            std::vector <db::Polygon> &merged = polygons_by_layer.insert (std::make_pair (l, std::vector <db::Polygon> ())).first->second;
            ep.merge(i->second, merged, 0, true, true);

            db::coord_traits<db::Coord>::area_type area = 0;
            db::coord_traits<db::Coord>::perimeter_type perimeter = 0;

            //  Despite merging, a multitude of separate non-touching polygons can exist.
            for (std::vector <db::Polygon>::iterator j = merged.begin (); j != merged.end (); ++j) {
              //  Sum area 
              area += j->area ();
              //  Sum perimeter for the merged polygon 
              perimeter += j->perimeter ();
            }

            statinfo_area [l] += area;
            statinfo_perimeter [l] += perimeter;

          }

          if (! shapes.empty ()) {

            if (! incomplete) {

              info.start_element ("h3");
              info.cdata (tl::to_string (QObject::tr ("Statistics:")));
              info.end_element ("h3");
              
              db::coord_traits<db::Coord>::area_type total_area = 0;
              db::coord_traits<db::Coord>::perimeter_type total_perimeter = 0;

              //  Print perimeter and area and sum up total
              info.start_element ("table");

              info.start_element ("tr");
              info.start_element ("td");
              info.start_element ("b");
              info.cdata (tl::to_string (QObject::tr ("Layer")));
              info.end_element ("b");
              info.end_element ("td");
              info.start_element ("td");
              info.start_element ("b");
              info.cdata (tl::to_string (QObject::tr ("Perimeter")));
              info.start_element ("br");
              info.end_element ("br");
              info.cdata (tl::to_string (QObject::tr ("(micron)")));
              info.end_element ("b");
              info.end_element ("td");
              info.start_element ("td");
              info.start_element ("b");
              info.cdata (tl::to_string (QObject::tr ("Area")));
              info.start_element ("br");
              info.end_element ("br");
              info.cdata (tl::to_string (QObject::tr ("(square micron)")));
              info.end_element ("b");
              info.end_element ("td");
              info.end_element ("tr");

              for (std::map<unsigned int, db::coord_traits<db::Coord>::area_type>::iterator i = statinfo_area.begin (); i != statinfo_area.end(); ++i) {

                unsigned int l = i->first;

                info.start_element ("tr");
                info.start_element ("td");
                info.cdata (layer_names [l]);
                info.end_element ("td");
                info.start_element ("td");
                total_perimeter += statinfo_perimeter [l];
                info.cdata (tl::micron_to_string (statinfo_perimeter [l] * dbu_unidir));
                info.end_element ("td");
                info.start_element ("td");
                total_area += statinfo_area[l];
                info.cdata (tl::to_string (statinfo_area [l] * dbu_unidir * dbu_unidir));
                info.end_element ("td");
                info.end_element ("tr");

              }

              //  Only if more than one layer is involved, print summed values
              if (statinfo_area.size () != 1) {

                info.start_element ("tr");
                info.start_element ("td");
                info.cdata (tl::to_string (QObject::tr ("Total")));
                info.end_element ("td");
                info.start_element ("td");
                info.cdata (tl::micron_to_string (total_perimeter * dbu_unidir));
                info.end_element ("td");
                info.start_element ("td");
                info.cdata (tl::to_string (total_area * dbu_unidir * dbu_unidir));
                info.end_element ("td");
                info.end_element ("tr");

              }

              info.end_element ("table");

            }

            info.start_element ("h3");
            info.cdata (tl::to_string (QObject::tr ("Shapes:")));
            info.end_element ("h3");

            for (std::map<std::string, std::set<std::string> >::const_iterator s = shapes.begin (); s != shapes.end (); ++s) {

              info.start_element ("p");

              info.start_element ("b");
              info.cdata (tl::to_string (QObject::tr ("Cell ")));
              info.cdata (s->first);
              info.cdata (":");
              info.end_element ("b");

              for (std::set <std::string>::const_iterator l = s->second.begin (); l != s->second.end (); ++l) {
                info.start_element ("br");
                info.end_element ("br");
                info.cdata (*l);
              }

              info.end_element ("p");

            }

            if (incomplete) {
              info.start_element ("p");
              info.cdata ("...");
              info.end_element ("p");
            }

          }

        } else {

          bool incomplete = false;
          std::set<std::string> labels;

          for (db::NetTracerNet::iterator net_shape = mp_nets [item_index]->begin (); net_shape != mp_nets [item_index]->end (); ++net_shape) {

            if (net_shape->shape ().is_text ()) {

              if (labels.size () >= max_labels) {
                incomplete = true;
                break;
              }

              std::string t (std::string (mp_nets [item_index]->cell_name (net_shape->cell_index ())) + "." + net_shape->shape ().text_string ());
              labels.insert (t);

            }

          }

          if (! labels.empty ()) {

            info.start_element ("h3");
            info.cdata (tl::to_string (QObject::tr ("Labels:")));
            info.end_element ("h3");

            info.start_element ("p");

            for (std::set <std::string>::const_iterator l = labels.begin (); l != labels.end (); ++l) {
              if (l != labels.begin ()) {
                info.start_element ("br");
                info.end_element ("br");
              }
              info.cdata (*l);
            }

            if (incomplete) {
              info.start_element ("br");
              info.end_element ("br");
              info.cdata ("...");
            }

            info.end_element ("p");

          }

          incomplete = false;
          std::set<std::string> cells;

          for (db::NetTracerNet::iterator net_shape = mp_nets [item_index]->begin (); net_shape != mp_nets [item_index]->end (); ++net_shape) {

            if (cells.size () >= max_cells) {
              incomplete = true;
              break;
            }

            std::string t (mp_nets [item_index]->cell_name (net_shape->cell_index ()));
            cells.insert (t);
          }

          if (! cells.empty ()) {

            info.start_element ("h3");
            info.cdata (tl::to_string (QObject::tr ("Cells on net:")));
            info.end_element ("h3");

            info.start_element ("p");

            for (std::set <std::string>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
              if (c != cells.begin ()) {
                info.start_element ("br");
                info.end_element ("br");
              }
              info.cdata (*c);
            }

            if (incomplete) {
              info.start_element ("br");
              info.end_element ("br");
              info.cdata ("...");
            }

            info.end_element ("p");

          }

        }

      }

    }

  }

  info.end_element ("body");
  info.end_element ("html");

  net_info_text->setHtml (tl::to_qstring (info_stream.str ()));

  //  determine and set the common net color

  if (selected_items.size () != 1) {

    net_color->set_color (QColor ());
    net_color->setEnabled (false);

  } else {

    QColor nc;

    int item_index = net_list->row (*selected_items.begin ());
    if (item_index >= 0 && item_index < int (mp_nets.size ())) {
      nc = mp_nets [item_index]->color ().to_qc ();
    }

    net_color->set_color (nc);
    net_color->setEnabled (true);

  }
}

void
NetTracerDialog::update_list ()
{
  QSize icon_size (12, 12);
  net_list->setIconSize (icon_size);

  QPixmap empty_pxmp (icon_size);
  empty_pxmp.fill (QColor (0, 0, 0, 0));

  QColor text_color = palette ().color (QPalette::Active, QPalette::Text);

  for (size_t i = 0; i < mp_nets.size (); ++i) {

    QListWidgetItem *item = 0;

    if (net_list->count () > int (i)) {
      item = net_list->item (int (i));
    } else {
      item = new QListWidgetItem (net_list);
      net_list->addItem (item);
    }

    item->setData (Qt::DisplayRole, tl::to_qstring (mp_nets [i]->name ()));

    if (mp_nets [i]->color ().is_valid ()) {

      QPixmap pxmp (icon_size);
      QPainter pxpainter (&pxmp);
      pxpainter.setPen (QPen (text_color));
      pxpainter.setBrush (QBrush (mp_nets [i]->color ().to_qc ()));
      QRect r (0, 0, pxmp.width () - 1, pxmp.height () - 1);
      pxpainter.drawRect (r);

      item->setIcon (QIcon (pxmp));

    } else {

      item->setIcon (QIcon (empty_pxmp));

    }

  }

  while (net_list->count () > int (mp_nets.size ())) {
    delete net_list->item (int (mp_nets.size ()));
  }
}

void 
NetTracerDialog::trace_path_button_clicked ()
{
BEGIN_PROTECTED
  commit ();
  net_list->setCurrentItem (0);
  m_mouse_state = 2;
  view ()->message (tl::to_string (QObject::tr ("Click on the first point in the net")));
  ui ()->grab_mouse (this, false);
END_PROTECTED
}

void 
NetTracerDialog::trace_net_button_clicked ()
{
BEGIN_PROTECTED
  commit ();
  net_list->setCurrentItem (0);
  m_mouse_state = 1;
  view ()->message (tl::to_string (QObject::tr ("Click on a point in the net")));
  ui ()->grab_mouse (this, false);
END_PROTECTED
}

void
NetTracerDialog::sticky_mode_clicked ()
{
BEGIN_PROTECTED
  if (! sticky_cbx->isChecked ()) {
    release_mouse ();
  } else {
    trace_net_button_clicked ();
  }
END_PROTECTED
}

void 
NetTracerDialog::release_mouse ()
{
  add_pb->setChecked (false);
  add2_pb->setChecked (false);
  m_mouse_state = 0;
  view ()->message ();
  ui ()->ungrab_mouse (this);
  set_cursor (lay::Cursor::none);
}

void
NetTracerDialog::clear_all_button_clicked ()
{
BEGIN_PROTECTED
  release_mouse ();

  if (QMessageBox::question (this, QObject::tr ("Clear All Nets"),
                             QObject::tr ("Are you sure to delete all nets?\nThis operation cannot be undone."),
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

    clear_markers ();
    clear_nets ();
    update_list ();
    item_selection_changed ();
    
  }
END_PROTECTED
}

void 
NetTracerDialog::delete_button_clicked ()
{
BEGIN_PROTECTED
  release_mouse ();

  std::vector<int> to_delete;

  QList<QListWidgetItem *> selected_items = net_list->selectedItems ();
  for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {
    int item_index = net_list->row (*item);
    if (item_index >= 0 && item_index < int (mp_nets.size ())) {
      to_delete.push_back (item_index);
    }
  }

  std::sort (to_delete.begin (), to_delete.end ());

  for (std::vector<int>::const_iterator d = to_delete.end (); d != to_delete.begin (); ) {
    --d;
    delete mp_nets [*d];
    mp_nets.erase (mp_nets.begin () + *d);
  }

  clear_markers ();
  update_list ();
  item_selection_changed ();
    
END_PROTECTED
}

void 
NetTracerDialog::layer_stack_clicked ()
{
BEGIN_PROTECTED

  release_mouse ();

  std::string tech_name;

  //  use actual technology name of the active cellview
  int cv_index = view ()->active_cellview_index ();
  lay::CellView cv = view ()->cellview (cv_index);
  if (cv.is_valid ()) {
    tech_name = cv->tech_name ();
  }

  if (! db::Technologies::instance ()->has_technology (tech_name)) {
    throw std::runtime_error (tl::to_string (QObject::tr ("Invalid technology attached to layout: ")) + tech_name);
  }

  //  create a temporary copy
  db::Technology tech = *db::Technologies::instance ()->technology_by_name (tech_name);

  //  call the dialog and if successful, install the new technology
  lay::TechComponentSetupDialog dialog (isVisible () ? this : parentWidget (), &tech, db::net_tracer_component_name ());
  if (dialog.exec ()) {
    *db::Technologies::instance ()->technology_by_name (tech.name ()) = tech;
    update_list_of_stacks ();
  }

END_PROTECTED
}

void 
NetTracerDialog::export_text_clicked ()
{
BEGIN_PROTECTED

  release_mouse ();

  int cv_index = view ()->active_cellview_index ();
  lay::CellView cv = view ()->cellview (cv_index);
  if (cv.is_valid ()) {

    QList<QListWidgetItem *> selected_items = net_list->selectedItems ();
    if (selected_items.size () == 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("No net selected to export")));
    }

    if (mp_export_file_dialog->get_save (m_export_file_name)) {

      std::ofstream os (m_export_file_name.c_str ());
      tl::XMLWriter w (os);

      w.start_document ();

      w.start_element ("nets");

      for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {

        int item_index = net_list->row (*item);
        if (item_index >= 0 && item_index < int (mp_nets.size ())) {

          w.start_element ("net");

          const db::NetTracerNet *net = mp_nets[item_index];

          w.start_element ("name");
          w.cdata (net->name ());
          w.end_element ("name");

          w.start_element ("top_cell");
          w.cdata (net->top_cell_name ());
          w.end_element ("top_cell");

          w.start_element ("layout");
          w.cdata (net->layout_filename ());
          w.end_element ("layout");

          w.start_element ("dbu");
          w.cdata (tl::to_string (net->dbu ()));
          w.end_element ("dbu");

          w.start_element ("complete");
          w.cdata (tl::to_string (! net->incomplete ()));
          w.end_element ("complete");

          w.start_element ("shapes");

          for (db::NetTracerNet::iterator net_shape = net->begin (); net_shape != net->end (); ++net_shape) {

            w.start_element ("element");

            w.start_element ("layer");
            std::string l (mp_nets [item_index]->layer_for (net_shape->layer ()).to_string ());
            if (l.empty ()) {
              l = "<anonymous>";
            }
            w.cdata (l);
            w.end_element ("layer");

            w.start_element ("cell");
            w.cdata (net->cell_name (net_shape->cell_index ()));
            w.end_element ("cell");

            w.start_element ("trans");
            w.cdata (net_shape->trans ().to_string ());
            w.end_element ("trans");

            w.start_element ("shape");
            w.cdata (net_shape->shape ().to_string ());
            w.end_element ("shape");

            w.end_element ("element");

          }

          w.end_element ("shapes");

          w.end_element ("net");

        }

      }

      w.end_element ("nets");

      w.end_document ();

    }

  }

END_PROTECTED
}

void 
NetTracerDialog::export_clicked ()
{
BEGIN_PROTECTED

  release_mouse ();

  int cv_index = view ()->active_cellview_index ();
  lay::CellView cv = view ()->cellview (cv_index);
  if (cv.is_valid ()) {

    QList<QListWidgetItem *> selected_items = net_list->selectedItems ();
    if (selected_items.size () == 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("No net selected to export")));
    }

    bool ok;
    QString cell_name = tl::to_qstring (m_export_cell_name);
    cell_name = QInputDialog::getText (this, QObject::tr ("Export Net"), QObject::tr ("Export net to cell named"), QLineEdit::Normal, cell_name, &ok);
    if (ok) {

      if (cell_name.isEmpty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("No cell was specified")));
      }

      //  Clear undo buffers if layout is created.
      view ()->manager ()->clear ();

      m_export_cell_name = tl::to_string (cell_name);
      std::pair<bool, db::cell_index_type> cbn = cv->layout ().cell_by_name (m_export_cell_name.c_str ());
      if (! cbn.first) {
        cbn.second = cv->layout ().add_cell (m_export_cell_name.c_str ());
      } 

      db::Cell &export_cell = cv->layout ().cell (cbn.second);

      for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {

        int item_index = net_list->row (*item);
        if (item_index >= 0 && item_index < int (mp_nets.size ())) {

          std::vector<unsigned int> new_layers = mp_nets[item_index]->export_net (cv->layout (), export_cell);

          //  Add a new entries in the layer list
          for (std::vector<unsigned int>::const_iterator l = new_layers.begin (); l != new_layers.end (); ++l) {
            lay::LayerProperties props;
            props.set_source (lay::ParsedLayerSource (cv->layout ().get_properties (*l), cv_index));
            view ()->init_layer_properties (props);
            view ()->insert_layer (view ()->end_layers (), props);
          }

        }

      }

      view ()->select_cell (export_cell.cell_index (), view ()->active_cellview_index ());

    }

  }

END_PROTECTED
}

void 
NetTracerDialog::configure_clicked ()
{
BEGIN_PROTECTED
  lay::ConfigurationDialog config_dialog (this, root (), "NetTracerPlugin");
  config_dialog.exec ();
END_PROTECTED
}

size_t
NetTracerDialog::get_trace_depth()
{
  double n = 0.0;
  try {
    QString depth = depth_le->text ().trimmed ();
    if (! depth.isEmpty ()) {
      tl::from_string_ext (tl::to_string (depth), n);
      if (n < 0 || n > double (std::numeric_limits<size_t>::max ())) {
        n = 0.0;
      }
    }
  } catch (...) {
    //  .. nothing yet ..
  }

  return (size_t) n;
}

void
NetTracerDialog::commit ()
{
  root ()->config_set (cfg_nt_trace_depth, tl::to_string (get_trace_depth ()));
}

void  
NetTracerDialog::deactivated ()
{
  commit ();
  clear_markers ();
  release_mouse ();
}

void  
NetTracerDialog::activated ()
{
  // .. nothing yet ..
}

lay::ViewService * 
NetTracerDialog::view_service_interface ()
{
  return this;
}

void
NetTracerDialog::layer_list_changed (int)
{
  if (active ()) {
    update_highlights ();
  }
}

void
NetTracerDialog::adjust_view ()
{
  int cv_index = view ()->active_cellview_index ();
  const lay::CellView &cv = view ()->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  if (m_window != NTFitNet && m_window != NTCenter && m_window != NTCenterSize) {
    return;
  }

  db::DBox bbox;

  QList<QListWidgetItem *> selected_items = net_list->selectedItems ();
  for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {

    int item_index = net_list->row (*item);

    if (item_index >= 0 && item_index < int (mp_nets.size ())) {

      std::map<unsigned int, std::vector<db::DCplxTrans> > tv_by_layer = view ()->cv_transform_variants_by_layer (cv_index);

      std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc> lm;
      for (db::Layout::layer_iterator l = cv->layout ().begin_layers (); l != cv->layout ().end_layers (); ++l) {
        lm.insert (std::make_pair (*(*l).second, (*l).first));
      }

      std::map <unsigned int, unsigned int> llmap;

      db::DBox cv_bbox;

      //  Create markers for the shapes 
      for (db::NetTracerNet::iterator net_shape = mp_nets [item_index]->begin (); net_shape != mp_nets [item_index]->end (); ++net_shape) {

        //  Find the actual layer by looking up the layer properties ..
        std::map <unsigned int, unsigned int>::const_iterator ll = llmap.find (net_shape->layer ());
        if (ll == llmap.end ()) {
          std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc>::const_iterator i = lm.find (mp_nets [item_index]->representative_layer_for (net_shape->layer ()));
          if (i != lm.end ()) {
            ll = llmap.insert (std::make_pair (net_shape->layer (), i->second)).first;
          }
        }

        unsigned int ly = 0;
        if (ll != llmap.end ()) {
          ly = ll->second;
        }

        std::map<unsigned int, std::vector<db::DCplxTrans> >::const_iterator tv = tv_by_layer.find (ly);
        if (tv != tv_by_layer.end ()) {

          db::Box shape_box = net_shape->shape ().bbox ();
          for (std::vector<db::DCplxTrans>::const_iterator t = tv->second.begin (); t != tv->second.end (); ++t) {
            cv_bbox += *t * db::CplxTrans (cv->layout ().dbu ()) * net_shape->trans () * shape_box;
          }

        }

      }

      bbox += cv_bbox;

    }

  }

  if (! bbox.empty ()) {

    if (m_window == NTFitNet) {

      view ()->zoom_box (bbox.enlarged (db::DVector (m_window_dim, m_window_dim)));

    } else if (m_window == NTCenter) {

      view ()->pan_center (bbox.p1 () + (bbox.p2 () - bbox.p1 ()) * 0.5);

    } else if (m_window == NTCenterSize) {

      double w = std::max (bbox.width (), m_window_dim);
      double h = std::max (bbox.height (), m_window_dim);
      db::DPoint center (bbox.p1() + (bbox.p2 () - bbox.p1 ()) * 0.5);
      db::DVector d (w * 0.5, h * 0.5);
      view ()->zoom_box (db::DBox (center - d, center + d));

    }

  }

}

void
NetTracerDialog::update_highlights ()
{
  clear_markers ();

  int cv_index = view ()->active_cellview_index ();
  const lay::CellView &cv = view ()->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  size_t n_marker = 0;

  QList<QListWidgetItem *> selected_items = net_list->selectedItems ();
  for (QList<QListWidgetItem *>::const_iterator item = selected_items.begin (); item != selected_items.end (); ++item) {

    int item_index = net_list->row (*item);

    if (item_index >= 0 && item_index < int (mp_nets.size ())) {

      std::map<unsigned int, std::vector<db::DCplxTrans> > tv_by_layer = view ()->cv_transform_variants_by_layer (cv_index);
      std::map<unsigned int, lay::LayerPropertiesConstIterator> layer_props;

      std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc> lm;
      for (db::Layout::layer_iterator l = cv->layout ().begin_layers (); l != cv->layout ().end_layers (); ++l) {
        lm.insert (std::make_pair (*(*l).second, (*l).first));
      }

      std::map <unsigned int, unsigned int> llmap;

      tl::Color net_color = mp_nets [item_index]->color ();

      //  Create markers for the shapes 
      for (db::NetTracerNet::iterator net_shape = mp_nets [item_index]->begin (); net_shape != mp_nets [item_index]->end () && n_marker < m_max_marker_count; ++net_shape) {

        //  Find the actual layer by looking up the layer properties ..
        std::map <unsigned int, unsigned int>::const_iterator ll = llmap.find (net_shape->layer ());
        if (ll == llmap.end ()) {
          std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc>::const_iterator i = lm.find (mp_nets [item_index]->representative_layer_for (net_shape->layer ()));
          if (i != lm.end ()) {
            ll = llmap.insert (std::make_pair (net_shape->layer (), i->second)).first;
          }
        }

        unsigned int ly = 0;
        if (ll != llmap.end ()) {
          ly = ll->second;
        }

        std::map<unsigned int, std::vector<db::DCplxTrans> >::const_iterator tv = tv_by_layer.find (ly);
        if (tv != tv_by_layer.end ()) {

          lay::LayerPropertiesConstIterator original;

          std::map<unsigned int, lay::LayerPropertiesConstIterator>::const_iterator lp_cache = layer_props.find (ly);
          if (lp_cache != layer_props.end ()) {
            original = lp_cache->second;
          } else {
            for (lay::LayerPropertiesConstIterator lp = view ()->begin_layers (); !lp.at_end (); ++lp) {
              if (!lp->has_children () && lp->cellview_index () == int (cv_index) && lp->layer_index () == int (ly)) {
                layer_props.insert (std::make_pair (ly, lp));
                original = lp;
                break;
              }
            }
          }

          mp_markers.push_back (new lay::ShapeMarker (view (), cv_index));
          mp_markers.back ()->set (net_shape->shape (), net_shape->trans (), tv->second);

          if (! original.at_end ()) {
            mp_markers.back ()->set_line_width (original->width (true));
            mp_markers.back ()->set_vertex_size (1);
            mp_markers.back ()->set_dither_pattern (original->dither_pattern (true));
            if (! view ()->background_color ().to_mono ()) {
              mp_markers.back ()->set_color (original->eff_fill_color_brighter (true, (m_marker_intensity * 255) / 100));
              mp_markers.back ()->set_frame_color (original->eff_frame_color_brighter (true, (m_marker_intensity * 255) / 100));
            } else {
              mp_markers.back ()->set_color (original->eff_fill_color_brighter (true, (-m_marker_intensity * 255) / 100));
              mp_markers.back ()->set_frame_color (original->eff_frame_color_brighter (true, (-m_marker_intensity * 255) / 100));
            }
          }

          if (net_color.is_valid ()) {
            mp_markers.back ()->set_color (net_color);
            mp_markers.back ()->set_frame_color (net_color);
          } else if (m_marker_color.is_valid ()) {
            mp_markers.back ()->set_color (m_marker_color);
            mp_markers.back ()->set_frame_color (m_marker_color);
          }

          if (m_marker_line_width >= 0) {
            mp_markers.back ()->set_line_width (m_marker_line_width);
          }

          if (m_marker_vertex_size >= 0) {
            mp_markers.back ()->set_vertex_size (m_marker_vertex_size);
          }

          if (m_marker_halo >= 0) {
            mp_markers.back ()->set_halo (m_marker_halo);
          }

          if (m_marker_dither_pattern >= 0) {
            mp_markers.back ()->set_dither_pattern (m_marker_dither_pattern);
          }

          ++n_marker;

        }

      }

    }

  }

}

void
NetTracerDialog::clear_markers ()
{
  for (std::vector <lay::ShapeMarker *>::iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
    delete *m;
  }

  mp_markers.clear ();
}

void
NetTracerDialog::trace_all_nets (db::LayoutToNetlist *l2ndb, const lay::CellView &cv, bool flat)
{
  const db::Technology *tech = cv->technology ();
  if (! tech) {
    return;
  }

  static std::string current_stack;

  QStringList stacks;
  std::vector<std::string> raw_stacks;
  int current = 0;

  const db::NetTracerTechnologyComponent *tech_component = dynamic_cast <const db::NetTracerTechnologyComponent *> (tech->component_by_name (db::net_tracer_component_name ()));
  if (tech_component) {
    for (auto d = tech_component->begin (); d != tech_component->end (); ++d) {
      raw_stacks.push_back (d->name ());
      if (d->name () == current_stack) {
        current = stacks.size ();
      }
      if (d->name ().empty ()) {
        stacks.push_back (tr ("(default)"));
      } else {
        stacks.push_back (tl::to_qstring (d->name ()));
      }
    }
  }

  if (raw_stacks.empty ()) {
    return;
  }

  current_stack = raw_stacks.front ();

  if (stacks.size () >= 2) {
    bool ok = true;
    QString sel = QInputDialog::getItem (parentWidget (), tr ("Select Stack for Net Tracing (All Nets)"), tr ("Stack"), stacks, current, false, &ok);
    if (! ok) {
      return;
    }
    current = stacks.indexOf (sel);
    if (current < 0) {
      return;
    }
    current_stack = raw_stacks [current];
  }

  db::NetTracerData tracer_data;
  if (! get_net_tracer_setup_from_tech (tech->name (), current_stack, cv->layout (), tracer_data)) {
    return;
  }

  tracer_data.configure_l2n (*l2ndb);

  std::string description = flat ? tl::to_string (tr ("Flat nets")) : tl::to_string (tr ("Hierarchical nets"));
  std::string name = flat ? "Flat_Nets" : "Hierarchical_Nets";
  if (! tech->name ().empty ()) {
    description += ", ";
    description += tl::to_string (tr ("Technology"));
    description += ": ";
    description += tech->name ();
    name += "_";
    name += tech->name ();
  }
  if (! current_stack.empty ()) {
    description += ", ";
    description += tl::to_string (tr ("Stack"));
    description += ": ";
    description += current_stack;
    name += "_";
    name += current_stack;
  }
  l2ndb->set_description (description);
  l2ndb->set_name (name);

  l2ndb->clear_join_nets ();
  l2ndb->clear_join_net_names ();

  l2ndb->set_include_floating_subcircuits (true);
  l2ndb->extract_netlist ();

  if (flat) {
    l2ndb->netlist ()->flatten ();
  }
}

}



