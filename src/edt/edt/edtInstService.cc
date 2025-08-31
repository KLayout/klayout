
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


#include "edtInstService.h"

#include "layLayoutViewBase.h"
#include "layDragDropData.h"
#include "dbLibraryManager.h"

#if defined(HAVE_QT)
#  include "edtInstPropertiesPage.h"
#  include "layBusy.h"
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  InstService implementation

InstService::InstService (db::Manager *manager, lay::LayoutViewBase *view)
  : edt::Service (manager, view),
    m_angle (0.0), m_scale (1.0),
    m_mirror (false), m_is_pcell (false),
    m_array (false), m_rows (1), m_columns (1), 
    m_row_x (0.0), m_row_y (0.0), m_column_x (0.0), m_column_y (0.0),
    m_place_origin (false), m_reference_transaction_id (0),
    m_needs_update (true), m_parameters_changed (false), m_has_valid_cell (false), m_in_drag_drop (false),
    m_current_cell (0), mp_current_layout (0), mp_pcell_decl (0), m_cv_index (-1)
{ 
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
InstService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new edt::InstPropertiesPage (this, manager, parent));
  return pages;
}
#endif

bool
InstService::do_activated ()
{
  m_cv_index = view ()->active_cellview_index ();
  m_has_valid_cell = false;

  return true;  //  start editing immediately
}

tl::Variant
InstService::get_default_layer_for_pcell ()
{
  lay::LayerPropertiesConstIterator cl = view ()->current_layer ();
  if (! cl.is_null () && ! cl->has_children () && (cl->source (true).cv_index() < 0 || cl->source (true).cv_index () == view ()->active_cellview_index ())) {
    db::LayerProperties lp = cl->source (true).layer_props ();
    if (! lp.is_null ()) {
      return tl::Variant (lp);
    }
  }

  return tl::Variant ();
}

#if defined(HAVE_QT)
bool
InstService::drag_enter_event (const db::DPoint &p, const lay::DragDropDataBase *data)
{
  const lay::CellDragDropData *cd = dynamic_cast <const lay::CellDragDropData *> (data);
  if (view ()->is_editable () && cd && (cd->layout () == & view ()->active_cellview ()->layout () || cd->library ())) {

    view ()->cancel ();
    set_edit_marker (0);

    bool switch_parameters = true;

    //  configure from the drag/drop data
    if (cd->library ()) {

      //  Reject drag & drop if the target technology does not match
      if (cd->library ()->for_technologies () && view ()->cellview (view ()->active_cellview_index ()).is_valid ()) {
        if (! cd->library ()->is_for_technology (view ()->cellview (view ()->active_cellview_index ())->tech_name ())) {
          return false;
        }
      }

      if (m_lib_name != cd->library ()->get_name ()) {
        m_lib_name = cd->library ()->get_name ();
      }

    } else {
      m_lib_name.clear ();
    }

    if (cd->is_pcell ()) {

      const db::PCellDeclaration *pcell_decl = cd->layout ()->pcell_declaration (cd->cell_index ());
      if (! pcell_decl) {
        return false;
      }

      if (m_cell_or_pcell_name != pcell_decl->name ()) {
        m_cell_or_pcell_name = pcell_decl->name ();
      }

      if (! cd->pcell_params ().empty ()) {
        m_pcell_parameters = pcell_decl->named_parameters (cd->pcell_params ());
        switch_parameters = false;
      }

    } else if (cd->layout ()->is_valid_cell_index (cd->cell_index ())) {

      m_cell_or_pcell_name = cd->layout ()->cell_name (cd->cell_index ());

    } else {
      return false;
    }

    switch_cell_or_pcell (switch_parameters);

    sync_to_config ();
    m_in_drag_drop = true;

    view ()->switch_mode (plugin_declaration ()->id ());

    do_begin_edit (p);

    //  action taken.
    return true;

  }

  return false;
}

bool
InstService::drag_move_event (const db::DPoint &p, const lay::DragDropDataBase * /*data*/)
{ 
  if (m_in_drag_drop) {
    do_mouse_move (p);
    return true;
  } else {
    return false; 
  }
}

void
InstService::drag_leave_event () 
{ 
  if (m_in_drag_drop) {
    set_edit_marker (0);
    do_cancel_edit ();
  }
}

bool
InstService::drop_event (const db::DPoint & /*p*/, const lay::DragDropDataBase * /*data*/)
{
  m_in_drag_drop = false;
  return false;
}
#endif

bool
InstService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return sel.is_cell_inst ();
}

void
InstService::sync_to_config ()
{
  //  push the current setup to configuration so the instance dialog will take these as default
  //  and "apply" of these instance properties doesn't fail because of insistency.
  dispatcher ()->config_set (cfg_edit_inst_lib_name, m_lib_name);
  dispatcher ()->config_set (cfg_edit_inst_cell_name, m_cell_or_pcell_name);
  if (m_is_pcell) {
    dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, pcell_parameters_to_string (m_pcell_parameters));
  } else {
    dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, std::string ());
  }
  dispatcher ()->config_end ();
}

void 
InstService::do_begin_edit (const db::DPoint &p)
{
  m_has_valid_cell = false;
  m_disp = snap (p);

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  if (cv.cell ()->is_proxy ()) {
    throw tl::Exception (tl::to_string (tr ("Cannot put an instance into a PCell or library cell")));
  }

  m_trans = cv.context_trans ();

  std::pair<bool, db::cell_index_type> ci = make_cell (cv);
  if (ci.first) {
    // use the snapped lower left corner of the bbox unless the origin is inside the bbox
    db::Box cell_bbox = cv->layout ().cell (ci.second).bbox_with_empty ();
    if (! m_place_origin && ! cell_bbox.contains (db::Point ())) {
      db::CplxTrans ct (1.0, m_angle, m_mirror, db::DVector ());
      m_disp = db::DPoint () + (m_disp - snap (cell_bbox.transformed (ct).lower_left () * cv->layout ().dbu ()));
    }
  }

  //  compute the transformation variants
  //  TODO: this is duplicated code
  //  TODO: from this computed vector we take just the first one!
  std::vector<db::DCplxTrans> tv;
  for (lay::LayerPropertiesConstIterator l = view ()->begin_layers (); !l.at_end (); ++l) {
    if (! l->has_children ()) {
      int cvi = (l->cellview_index () >= 0) ? l->cellview_index () : 0;
      if (cvi == m_cv_index) {
        tv.insert (tv.end (), l->trans ().begin (), l->trans ().end ());
      }
    }
  }
  std::sort (tv.begin (), tv.end ());
  tv.erase (std::unique (tv.begin (), tv.end ()), tv.end ());
  if (! tv.empty ()) {
    m_trans = db::VCplxTrans (1.0 / cv->layout ().dbu ()) * tv [0] * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();
  }

  open_editor_hooks ();

  update_marker ();
}

std::pair<bool, db::cell_index_type> 
InstService::make_cell (const lay::CellView &cv)
{
  if (m_has_valid_cell) {
    return std::make_pair (true, m_current_cell);
  }

#if defined(HAVE_QT)
  //  prevents recursion
  lay::BusySection busy;
#endif

  //  NOTE: do this at the beginning: creating a transaction might delete transactions behind the
  //  head transaction, hence releasing (thus: deleting) cells. To prevert interference, create
  //  the transaction at the beginning.
  db::Transaction transaction (manager (), tl::to_string (tr ("Create reference cell")), m_reference_transaction_id);

  lay::LayerState layer_state = view ()->layer_snapshot ();

  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name, cv->tech_name ());

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
  //  the library selected
  if (lib) {
    mp_current_layout = &lib->layout ();
  } else {
    mp_current_layout = &cv->layout ();
  }

  std::pair<bool, db::cell_index_type> ci (false, db::cell_index_type (0));
  std::pair<bool, db::pcell_id_type> pci (false, db::pcell_id_type (0));

  if (! m_is_pcell) {
    ci = mp_current_layout->cell_by_name (m_cell_or_pcell_name.c_str ());
  } else {
    pci = mp_current_layout->pcell_by_name (m_cell_or_pcell_name.c_str ());
  }

  if (! ci.first && ! pci.first) {
    return std::pair<bool, db::cell_index_type> (false, 0);
  }

  db::cell_index_type inst_cell_index = ci.second;

  mp_pcell_decl = 0;

  //  instantiate the PCell
  if (pci.first) {

    std::vector<tl::Variant> pv;

    mp_pcell_decl = mp_current_layout->pcell_declaration (pci.second);
    if (mp_pcell_decl) {

      pv = mp_pcell_decl->map_parameters (m_pcell_parameters);

      //  make the parameters fit (i.e. PCells may not define consistent default parameters)
      mp_pcell_decl->coerce_parameters (*mp_current_layout, pv);

    }

    inst_cell_index = mp_current_layout->get_pcell_variant (pci.second, pv);

  }

  //  reference the library
  if (lib) {

    mp_current_layout = & cv->layout ();
    inst_cell_index = mp_current_layout->get_lib_proxy (lib, inst_cell_index);

    //  remove unused references
    std::set<db::cell_index_type> keep;
    keep.insert (inst_cell_index);
    mp_current_layout->cleanup (keep);

  }

  view ()->add_new_layers (layer_state);

  m_has_valid_cell = true;
  m_current_cell = inst_cell_index;

  if (! transaction.is_empty ()) {
    m_reference_transaction_id = transaction.id ();
  }

  return std::pair<bool, db::cell_index_type> (true, inst_cell_index);
}

void
InstService::do_mouse_move_inactive (const db::DPoint &p)
{
  clear_mouse_cursors ();
  add_mouse_cursor (snap (p));
}

void
InstService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  m_disp = snap (p);

  std::pair<bool, db::cell_index_type> ci = make_cell (cv);
  if (ci.first) {
    //  use the snapped lower left corner of the bbox unless the origin is inside the bbox
    db::Box cell_bbox = cv->layout ().cell (ci.second).bbox_with_empty ();
    if (! m_place_origin && ! cell_bbox.contains (db::Point ())) {
      db::CplxTrans ct (1.0, m_angle, m_mirror, db::DVector ());
      m_disp = db::DPoint () + (m_disp - snap (cell_bbox.transformed (ct).lower_left () * cv->layout ().dbu ()));
    }
  }

  update_marker ();
}

void 
InstService::do_mouse_transform (const db::DPoint &p, db::DFTrans trans)
{
  db::DCplxTrans ct (1.0, m_angle, m_mirror, db::DVector ());
  ct *= db::DCplxTrans (trans);

  m_angle = ct.angle ();
  m_mirror = ct.is_mirror ();

  db::DPoint r (m_row_x, m_row_y);
  r.transform (trans);
  m_row_x = r.x ();
  m_row_y = r.y ();

  db::DPoint c (m_column_x, m_column_y);
  c.transform (trans);
  m_column_x = c.x ();
  m_column_y = c.y ();

  dispatcher ()->config_set (cfg_edit_inst_angle, m_angle);
  dispatcher ()->config_set (cfg_edit_inst_mirror, m_mirror);
  dispatcher ()->config_set (cfg_edit_inst_row_x, m_row_x);
  dispatcher ()->config_set (cfg_edit_inst_row_y, m_row_y);
  dispatcher ()->config_set (cfg_edit_inst_column_x, m_column_x);
  dispatcher ()->config_set (cfg_edit_inst_column_y, m_column_y);
  dispatcher ()->config_end ();

  //  honour the new transformation
  do_mouse_move (p);
}

bool 
InstService::do_mouse_click (const db::DPoint &p)
{
  do_mouse_move (p);
  return true;
}

void 
InstService::do_finish_edit ()
{
  try {

    db::CellInstArray inst;
    if (get_inst (inst)) {

      //  check for recursive hierarchy
      const lay::CellView &cv = view ()->cellview (m_cv_index);
      std::set <db::cell_index_type> called, callers;

      cv->layout ().cell (inst.object ().cell_index ()).collect_called_cells (called);
      called.insert (inst.object ().cell_index ());
      cv.cell ()->collect_caller_cells (callers);
      callers.insert (cv.cell_index ());

      std::vector <db::cell_index_type> intersection;
      std::set_intersection (called.begin (), called.end (), callers.begin (), callers.end (), std::back_inserter (intersection));
      if (! intersection.empty ()) {
        throw tl::Exception (tl::to_string (tr ("Inserting this instance would create a recursive hierarchy")));
      }

      if (manager ()) {
        manager ()->transaction (tl::to_string (tr ("Create instance")), m_reference_transaction_id);
      }
      m_reference_transaction_id = 0;
      db::Instance i = cv.cell ()->insert (inst);
      cv->layout ().cleanup ();
      if (manager ()) {
        manager ()->commit ();
      }

      commit_recent ();

      if (m_in_drag_drop) {

        lay::ObjectInstPath sel;
        sel.set_cv_index (m_cv_index);
        sel.set_topcell (cv.cell_index ());
        sel.add_path (db::InstElement (i, db::CellInstArray::iterator ()));

        add_selection (sel);

      }

    }

    m_has_valid_cell = false;
    m_in_drag_drop = false;
    close_editor_hooks (true);

  } catch (...) {
    m_has_valid_cell = false;
    m_in_drag_drop = false;
    close_editor_hooks (false);
    throw;
  }
}

void 
InstService::do_cancel_edit ()
{
  //  Undo "create reference" transactions which basically unfinished "create instance" transactions
  if (m_reference_transaction_id > 0 && manager ()->transaction_id_for_undo () == m_reference_transaction_id) {
    manager ()->undo ();
  }

  m_reference_transaction_id = 0;
  m_has_valid_cell = false;
  m_in_drag_drop = false;

  set_edit_marker (0);

  //  clean up any proxy cells created so far 
  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (cv.is_valid ()) {
    cv->layout ().cleanup ();
  }

  close_editor_hooks (false);
}

void
InstService::service_configuration_changed ()
{
  m_needs_update = true;
}

bool 
InstService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_inst_cell_name) {

    if (value != m_cell_or_pcell_name) {
      m_cell_or_pcell_name = value;
      m_needs_update = true;
    }

    return true; // taken
  }

  if (name == cfg_edit_inst_lib_name) {

    if (value != m_lib_name) {
      m_lib_name_previous = m_lib_name;
      m_lib_name = value;
      m_needs_update = true;
    }

    return true; // taken
  }

  if (name == cfg_edit_inst_pcell_parameters) {

    std::map<std::string, tl::Variant> pcp = pcell_parameters_from_string (value);
    if (pcp != m_pcell_parameters) {

      m_pcell_parameters = pcp;
      m_is_pcell = ! value.empty ();

      m_needs_update = true;
      m_parameters_changed = true;

    }

    return true; // taken

  }

  if (name == cfg_edit_inst_place_origin) {

    bool f;
    tl::from_string (value, f);

    if (f != m_place_origin) {
      m_place_origin = f;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_scale) {

    double s;
    tl::from_string (value, s);

    if (fabs (s - m_scale) > 1e-10) {
      m_scale = s;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_angle) {

    double a;
    tl::from_string (value, a);

    if (fabs (a - m_angle) > 1e-10) {
      m_angle = a;
      m_needs_update = true;
    }

    return true; // taken
  }

  if (name == cfg_edit_inst_mirror) {

    bool f;
    tl::from_string (value, f);

    if (f != m_mirror) {
      m_mirror = f;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_array) {

    bool f;
    tl::from_string (value, f);

    if (f != m_array) {
      m_array = f;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_rows) {

    unsigned int v;
    tl::from_string (value, v);

    if (v != m_rows) {
      m_rows = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_row_x) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_row_x, v)) {
      m_row_x = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_row_y) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_row_y, v)) {
      m_row_y = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_columns) {

    unsigned int v;
    tl::from_string (value, v);

    if (v != m_columns) {
      m_columns = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_column_x) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_column_x, v)) {
      m_column_x = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_column_y) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_column_y, v)) {
      m_column_y = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  return edt::Service::configure (name, value);
}

void
InstService::switch_cell_or_pcell (bool switch_parameters)
{
  //  if the library or cell name has changed, store the current pcell parameters and try to reuse
  //  an existing parameter set
  if (! m_cell_or_pcell_name_previous.empty () && (m_cell_or_pcell_name_previous != m_cell_or_pcell_name || m_lib_name_previous != m_lib_name)) {

    m_stored_pcell_parameters[std::make_pair (m_cell_or_pcell_name_previous, m_lib_name_previous)] = m_pcell_parameters;

    if (switch_parameters) {

      std::map<std::pair<std::string, std::string>, std::map<std::string, tl::Variant> >::const_iterator p = m_stored_pcell_parameters.find (std::make_pair (m_cell_or_pcell_name, m_lib_name));
      if (p != m_stored_pcell_parameters.end ()) {
        m_pcell_parameters = p->second;
      } else {
        m_pcell_parameters.clear ();
      }

    }

  }

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  db::Library *lib = 0;
  if (cv.is_valid ()) {
    lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name, cv->tech_name ());
  } else {
    lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name);
  }

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or
  //  the library selected
  const db::Layout *layout = 0;
  if (lib) {
    layout = &lib->layout ();
  } else if (cv.is_valid ()) {
    layout = &cv->layout ();
  }

  if (layout) {
    m_is_pcell = layout->pcell_by_name (m_cell_or_pcell_name.c_str ()).first;
  } else {
    m_is_pcell = false;
  }

  //  remember the current cell or library name
  m_cell_or_pcell_name_previous = m_cell_or_pcell_name;
  m_lib_name_previous = m_lib_name;
}

void 
InstService::config_finalize ()
{
  if (m_needs_update) {

    //  don't switch parameters if they have been updated explicitly
    //  since the last "config_finalize". This means the sender of the configuration events
    //  wants the parameters to be set in a specific way. Don't interfere.
    bool switch_parameters = ! m_parameters_changed;

    switch_cell_or_pcell (switch_parameters);

    m_has_valid_cell = false;
    update_marker ();

    if (switch_parameters) {
      //  Reflects any changes in PCell parameters in the configuration
      //  TODO: it's somewhat questionable to do this inside "config_finalize" as this method is supposed
      //  to reflect changes rather than induce some.
      if (m_is_pcell) {
        dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, pcell_parameters_to_string (m_pcell_parameters));
      } else {
        dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, std::string ());
      }
    }

  }

  m_needs_update = false;
  m_parameters_changed = false;

  edt::Service::config_finalize ();
}

void
InstService::update_marker ()
{
  if (editing ()) {

    lay::Marker *marker = new lay::Marker (view (), m_cv_index, ! show_shapes_of_instances (), show_shapes_of_instances () ? max_shapes_of_instances () : 0);
    marker->set_vertex_shape (lay::ViewOp::Cross);
    marker->set_vertex_size (9 /*cross vertex size*/);
    set_edit_marker (marker);

    db::CellInstArray inst;
    if (get_inst (inst)) {
      marker->set (inst, m_trans);
    } else {
      marker->set ();
    }

  } else {
    set_edit_marker (0);
  }

  //  call hooks with new shape
  if (! editor_hooks ().empty ()) {

    call_editor_hooks (editor_hooks (), &edt::EditorHooks::begin_new_instances);

    try {

      const lay::CellView &cv = view ()->cellview (m_cv_index);

      db::CellInstArray inst;
      if (cv.is_valid () && get_inst (inst)) {

        //  Note: the instance collection is temporary
        db::Instances instances (cv.cell ());
        db::Instance i = instances.insert (inst);

        db::CplxTrans view_trans = db::CplxTrans (cv->layout ().dbu ()) * m_trans;
        call_editor_hooks<const db::Instance &, const db::CplxTrans &> (m_editor_hooks, &edt::EditorHooks::create_instance, i, view_trans);

      }

    } catch (...) {
      //  ignore exceptions
    }

    call_editor_hooks (editor_hooks (), &edt::EditorHooks::end_new_instances);

  }
}

bool
InstService::get_inst (db::CellInstArray &inst) 
{
  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (cv.is_valid ()) {

    std::pair<bool, db::cell_index_type> ci = make_cell (cv);
    if (ci.first) {

      //  compute the instance's transformation
      db::VCplxTrans pt = (db::CplxTrans (cv->layout ().dbu ()) * m_trans).inverted ();
      db::ICplxTrans trans = db::ICplxTrans (m_scale, m_angle, m_mirror, pt * m_disp - db::Point ());

      if (m_array && m_rows > 0 && m_columns > 0) {
        db::Vector row = db::Vector (pt * db::DVector (m_row_x, m_row_y));
        db::Vector column = db::Vector (pt * db::DVector (m_column_x, m_column_y));
        inst = db::CellInstArray (db::CellInst (ci.second), trans, row, column, m_rows, m_columns);
      } else {
        inst = db::CellInstArray (db::CellInst (ci.second), trans);
      }

      return true;

    }

  }
  return false;
}

void
InstService::open_editor_hooks ()
{
  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  std::string technology;
  if (cv->layout ().technology ()) {
    technology = cv->layout ().technology ()->name ();
  }

  m_editor_hooks = edt::EditorHooks::get_editor_hooks (technology);

  lay::CellViewRef cv_ref (view ()->cellview_ref (m_cv_index));
  call_editor_hooks<lay::CellViewRef &> (m_editor_hooks, &edt::EditorHooks::begin_create_instances, cv_ref);
}

void
InstService::close_editor_hooks (bool with_commit)
{
  if (with_commit) {
    call_editor_hooks (m_editor_hooks, &edt::EditorHooks::commit_instances);
  }
  call_editor_hooks (m_editor_hooks, &edt::EditorHooks::end_create_instances);

  m_editor_hooks.clear ();
}

} // namespace edt

