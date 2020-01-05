
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


#include "dbLibrary.h"
#include "dbPCellHeader.h"
#include "edtInstPropertiesPage.h"
#include "edtPropertiesPageUtils.h"
#include "edtPCellParametersPage.h"
#include "edtDialogs.h"
#include "layDialogs.h"
#include "layObjectInstPath.h"
#include "layLayoutView.h"
#include "layCellSelectionForm.h"
#include "tlExceptions.h"
#include "tlString.h"

#include <string>

#include <QMessageBox>

namespace edt
{

// -------------------------------------------------------------------------
//  InstPropertiesPage implementation

InstPropertiesPage::InstPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent)
  : lay::PropertiesPage (parent, manager, service), mp_service (service), m_enable_cb_callback (true), mp_pcell_parameters (0)
{
  m_selection_ptrs.reserve (service->selection ().size ());
  for (edt::Service::obj_iterator s = service->selection ().begin (); s != service->selection ().end (); ++s) {
    m_selection_ptrs.push_back (s);
  }
  m_index = 0;
  m_prop_id = 0;
  mp_service->clear_highlights ();

  setupUi (this);
  connect (inst_pb, SIGNAL (clicked ()), this, SLOT (show_inst ()));
  connect (sel_pb, SIGNAL (clicked ()), this, SLOT (show_cell ()));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (show_props ()));
  connect (dbu_cb, SIGNAL (toggled (bool)), this, SLOT (display_mode_changed (bool)));
  connect (abs_cb, SIGNAL (toggled (bool)), this, SLOT (display_mode_changed (bool)));
  connect (browse_pb, SIGNAL (clicked ()), this, SLOT (browse_cell ()));
  connect (lib_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (library_changed (int)));
  connect (cell_name_le, SIGNAL (textChanged (const QString &)), this, SLOT (cell_name_changed (const QString &)));

  QHBoxLayout *layout = new QHBoxLayout (pcell_tab);
  layout->setMargin (0);
  pcell_tab->setLayout (layout);
}

InstPropertiesPage::~InstPropertiesPage ()
{
  mp_service->restore_highlights ();
}

void
InstPropertiesPage::library_changed (int)
{
BEGIN_PROTECTED
  update_pcell_parameters ();
END_PROTECTED
}

void
InstPropertiesPage::cell_name_changed (const QString &)
{
BEGIN_PROTECTED
  update_pcell_parameters ();
END_PROTECTED
}

void
InstPropertiesPage::browse_cell ()
{
BEGIN_PROTECTED

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
  //  the library selected
  db::Layout *layout = 0;
  db::Library *lib = 0;
  if (lib_cbx->current_library ()) {
    lib = lib_cbx->current_library ();
    layout = &lib->layout ();
  } else {
    edt::Service::obj_iterator pos = m_selection_ptrs [m_index];
    const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());
    layout = &cv->layout ();
  }

  lay::LibraryCellSelectionForm form (this, layout, "browse_lib_cell");
  if (lib) {
    form.setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Select Cell - Library: ")) + lib->get_description ()));
  }

  std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (cell_name_le->text ()).c_str ());
  if (pc.first) {
    form.set_selected_pcell_id (pc.second);
  } else {
    std::pair<bool, db::cell_index_type> c = layout->cell_by_name (tl::to_string (cell_name_le->text ()).c_str ());
    if (c.first) {
      form.set_selected_cell_index (c.second);
    }
  }

  if (form.exec ()) {
    if (form.selected_cell_is_pcell ()) {
      cell_name_le->setText (tl::to_qstring (layout->pcell_header (form.selected_pcell_id ())->get_name ()));
    } else if (layout->is_valid_cell_index (form.selected_cell_index ())) {
      cell_name_le->setText (tl::to_qstring (layout->cell_name (form.selected_cell_index ())));
    }
    update_pcell_parameters ();
  }

END_PROTECTED
}

void
InstPropertiesPage::show_props ()
{
  lay::UserPropertiesForm props_form (this);
  props_form.show (mp_service->view (), m_selection_ptrs [m_index]->cv_index (), m_prop_id);
}

void
InstPropertiesPage::display_mode_changed (bool)
{
  if (! m_enable_cb_callback) {
    return;
  }

  mp_service->view ()->dbu_coordinates (dbu_cb->isChecked ());
  mp_service->view ()->absolute_coordinates (abs_cb->isChecked ());

  update ();
}

void 
InstPropertiesPage::back ()
{
  m_index = (unsigned int) m_selection_ptrs.size ();
}

void 
InstPropertiesPage::front ()
{
  m_index = 0;
}

bool 
InstPropertiesPage::at_begin () const
{
  return (m_index == 0);
}

bool 
InstPropertiesPage::at_end () const
{
  return (m_index == m_selection_ptrs.size ());
}

void 
InstPropertiesPage::operator-- ()
{
  --m_index;
}

void 
InstPropertiesPage::operator++ ()
{
  ++m_index;
}

void 
InstPropertiesPage::leave ()
{
  mp_service->clear_highlights ();
}

void 
InstPropertiesPage::update ()
{
  edt::Service::obj_iterator pos = m_selection_ptrs [m_index];
  tl_assert (pos->is_cell_inst ());

  mp_service->highlight (m_index);

  m_enable_cb_callback = false;
  dbu_cb->setChecked (mp_service->view ()->dbu_coordinates ());
  abs_cb->setChecked (mp_service->view ()->absolute_coordinates ());
  m_enable_cb_callback = true;

  const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());
  double dbu = cv->layout ().dbu ();

  std::string info (tl::to_string (QObject::tr ("Cell ")));
  info += cv->layout ().cell_name (pos->cell_index ());
  info_lbl->setText (tl::to_qstring (info));

  db::Layout *def_layout = &cv->layout ();
  db::cell_index_type def_cell_index = pos->back ().inst_ptr.cell_index ();
  const db::Cell &def_cell = def_layout->cell (def_cell_index);

  std::pair<db::Library *, db::cell_index_type> dl = def_layout->defining_library (def_cell_index);
  lib_cbx->set_technology_filter (cv->tech_name (), true);
  lib_cbx->set_current_library (dl.first);
  if (dl.first) {
    def_layout = &dl.first->layout ();
    def_cell_index = dl.second;
  }

  std::pair<bool, db::pcell_id_type> pci = def_layout->is_pcell_instance (def_cell_index);
  if (pci.first && def_layout->pcell_declaration (pci.second)) {
    cell_name_le->setText (tl::to_qstring (def_layout->pcell_header (pci.second)->get_name ()));
  } else {
    cell_name_le->setText (tl::to_qstring (def_layout->cell_name (def_cell_index)));
  }

  db::Vector rowv, columnv;
  unsigned long rows, columns;

  db::ICplxTrans gt;
  if (abs_cb->isChecked ()) {
    gt = pos->trans ();
  }

  bool du = dbu_cb->isChecked ();

  db::Box cell_bbox = def_cell.bbox ();
  cw_le->setText (tl::to_qstring (coord_to_string (cell_bbox.width (), dbu, du)));
  ch_le->setText (tl::to_qstring (coord_to_string (cell_bbox.height (), dbu, du)));

  db::Trans t (pos->back ().inst_ptr.front ());

  if (pos->back ().inst_ptr.is_regular_array (rowv, columnv, rows, columns)) {

    array_grp->setChecked (true);
    rows_le->setText (tl::to_qstring (tl::to_string (rows)));
    columns_le->setText (tl::to_qstring (tl::to_string (columns)));
    row_x_le->setText (tl::to_qstring (coord_to_string ((gt * rowv).x (), dbu, du)));
    row_y_le->setText (tl::to_qstring (coord_to_string ((gt * rowv).y (), dbu, du)));
    column_x_le->setText (tl::to_qstring (coord_to_string ((gt * columnv).x (), dbu, du)));
    column_y_le->setText (tl::to_qstring (coord_to_string ((gt * columnv).y (), dbu, du)));

    if (! pos->back ().array_inst.at_end ()) {

      //  show the array indices
      long row = pos->back ().array_inst.index_a ();
      long column = pos->back ().array_inst.index_b ();
      inst_lbl->setText (tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("This is instance [%ld,%ld] of array with")), row, column)));

    }

  } else {

    array_grp->setChecked (false);
    rows_le->setText (QString ());
    columns_le->setText (QString ());
    row_x_le->setText (QString ());
    row_y_le->setText (QString ());
    column_x_le->setText (QString ());
    column_y_le->setText (QString ());
    inst_lbl->setText (QString ());

  }

  pos_x_le->setText (tl::to_qstring (coord_to_string ((gt * db::ICplxTrans (t)).disp ().x (), dbu, du)));
  pos_y_le->setText (tl::to_qstring (coord_to_string ((gt * db::ICplxTrans (t)).disp ().y (), dbu, du)));
  angle_le->setText (tl::to_qstring (tl::to_string (pos->back ().inst_ptr.complex_trans ().angle ())));
  mirror_cbx->setChecked (t.is_mirror ());
  mag_le->setText (tl::to_qstring (tl::to_string (pos->back ().inst_ptr.complex_trans ().mag ())));

  m_prop_id = pos->back ().inst_ptr.prop_id ();

  update_pcell_parameters ();
}

void 
InstPropertiesPage::show_cell ()
{
  edt::Service::obj_iterator pos = m_selection_ptrs [m_index];

  lay::CellView::unspecific_cell_path_type path (mp_service->view ()->cellview (pos->cv_index ()).combined_unspecific_path ());
  for (lay::ObjectInstPath::iterator p = pos->begin (); p != pos->end (); ++p) {
    path.push_back (p->inst_ptr.cell_index ());
  }

  mp_service->view ()->set_current_cell_path (pos->cv_index (), path);
}

void
InstPropertiesPage::show_inst ()
{
  InstantiationForm inst_form (this);
  inst_form.show (mp_service->view (), *m_selection_ptrs [m_index]);
}

bool 
InstPropertiesPage::readonly ()
{
  return ! mp_service->view ()->is_editable ();
}

ChangeApplicator *
InstPropertiesPage::create_applicator (db::Cell & /*cell*/, const db::Instance & /*inst*/, double dbu)
{
  std::auto_ptr<CombinedChangeApplicator> appl (new CombinedChangeApplicator ());

  edt::Service::obj_iterator pos = m_selection_ptrs [m_index];
  const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());

  bool du = dbu_cb->isChecked ();

  db::Layout *layout;
  db::Library *lib = lib_cbx->current_library ();

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
  //  the library selected
  if (lib) {
    layout = &lib->layout ();
  } else {
    layout = &cv->layout ();
  }

  std::pair<bool, db::cell_index_type> ci = layout->cell_by_name (tl::to_string (cell_name_le->text ()).c_str ());
  std::pair<bool, db::pcell_id_type> pci = layout->pcell_by_name (tl::to_string (cell_name_le->text ()).c_str ());
  if (! ci.first && ! pci.first) {
    throw tl::Exception (tl::to_string (QObject::tr ("Not a valid cell name: %s")).c_str (), tl::to_string (cell_name_le->text ()).c_str ());
  }

  db::cell_index_type inst_cell_index = ci.second;

  //  instantiate the PCell
  if (pci.first) {
    tl_assert (mp_pcell_parameters != 0);
    tl_assert (layout->pcell_declaration (pci.second) == mp_pcell_parameters->pcell_decl ());
    inst_cell_index = layout->get_pcell_variant (pci.second, mp_pcell_parameters->get_parameters ());
  }

  //  reference the library
  if (lib) {
    layout = & cv->layout ();
    inst_cell_index = layout->get_lib_proxy (lib, inst_cell_index);
  }

  if (inst_cell_index != pos->back ().inst_ptr.cell_index ()) {
    appl->add (new ChangeTargetCellApplicator (inst_cell_index));
  }

  double x = 0.0, y = 0.0;
  tl::from_string (tl::to_string (pos_x_le->text ()), x);
  tl::from_string (tl::to_string (pos_y_le->text ()), y);

  db::DCplxTrans t;
  if (abs_cb->isChecked ()) {
    t = db::DCplxTrans (pos->trans ().inverted ());
  }

  db::DVector disp = db::DVector (dpoint_from_dpoint (db::DPoint (x, y), dbu, du, t));

  bool mirror = mirror_cbx->isChecked ();
  double angle = 0.0;
  tl::from_string (tl::to_string (angle_le->text ()), angle);

  double mag = 0.0;
  tl::from_string (tl::to_string (mag_le->text ()), mag);

  angle -= (floor (angle / 360.0) + 1.0) * 360.0;
  while (angle < -1e-6) {
    angle += 360.0;
  }

  db::CellInstArray::complex_trans_type tr = pos->back ().inst_ptr.complex_trans ();

  if (fabs (angle - tr.angle ()) > 1e-6 || mirror != tr.is_mirror () || fabs (mag - tr.mag ()) > 1e-6 || ! disp.equal (tr.disp () * dbu)) {
    appl->add (new ChangeInstanceTransApplicator (angle, tr.angle (), mirror, tr.is_mirror (), mag, tr.mag (), disp, tr.disp () * dbu));
  }

  db::CellInstArray::vector_type a_org, b_org;
  unsigned long na_org = 0, nb_org = 0;
  bool is_array_org = pos->back ().inst_ptr.is_regular_array (a_org, b_org, na_org, nb_org);

  if (array_grp->isChecked ()) {

    double cx = 0.0, cy = 0.0;
    double rx = 0.0, ry = 0.0;
    unsigned long rows = 0, cols = 0;

    tl::from_string (tl::to_string (column_x_le->text ()), cx);
    tl::from_string (tl::to_string (column_y_le->text ()), cy);
    tl::from_string (tl::to_string (row_x_le->text ()), rx);
    tl::from_string (tl::to_string (row_y_le->text ()), ry);
    tl::from_string (tl::to_string (rows_le->text ()), rows);
    tl::from_string (tl::to_string (columns_le->text ()), cols);

    db::DVector rv = db::DVector (dpoint_from_dpoint (db::DPoint (rx, ry), dbu, du, t));
    db::DVector cv = db::DVector (dpoint_from_dpoint (db::DPoint (cx, cy), dbu, du, t));

    bool set_a = (! rv.equal (a_org * dbu) || ! is_array_org);
    bool set_na = (rows != na_org || ! is_array_org);
    bool set_b = (! cv.equal (b_org * dbu) || ! is_array_org);
    bool set_nb = (cols != nb_org || ! is_array_org);

    if (set_a || set_b || set_na || set_nb) {
      appl->add (new ChangeInstanceArrayApplicator (rv, set_a, cv, set_b, rows, set_na, cols, set_nb));
    }

  } else if (is_array_org) {

    appl->add (new InstanceRemoveArrayApplicator ());

  }

  return appl.release ();
}

void
InstPropertiesPage::recompute_selection_ptrs (const std::vector<lay::ObjectInstPath> &new_sel)
{
  std::map<lay::ObjectInstPath, edt::Service::obj_iterator> ptrs;

  for (edt::Service::obj_iterator pos = mp_service->selection ().begin (); pos != mp_service->selection ().end (); ++pos) {
    ptrs.insert (std::make_pair (*pos, pos));
  }

  m_selection_ptrs.clear ();
  for (std::vector<lay::ObjectInstPath>::const_iterator s = new_sel.begin (); s != new_sel.end (); ++s) {
    std::map<lay::ObjectInstPath, edt::Service::obj_iterator>::const_iterator pm = ptrs.find (*s);
    tl_assert (pm != ptrs.end ());
    m_selection_ptrs.push_back (pm->second);
  }
}

void 
InstPropertiesPage::do_apply (bool current_only)
{
  lay::LayerState layer_state = mp_service->view ()->layer_snapshot ();
  unsigned int cv_index = m_selection_ptrs [m_index]->cv_index ();

  std::auto_ptr<ChangeApplicator> applicator;

  {
    edt::Service::obj_iterator pos = m_selection_ptrs [m_index];
    tl_assert (pos->is_cell_inst ());

    const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());

    db::Cell &cell = cv->layout ().cell (pos->cell_index ());
    double dbu = cv->layout ().dbu ();

    applicator.reset (create_applicator (cell, pos->back ().inst_ptr, dbu));

    if (m_prop_id != pos->back ().inst_ptr.prop_id ()) {
      applicator.reset (new CombinedChangeApplicator (applicator.release (), new ChangePropertiesApplicator (m_prop_id)));
    }
  }

  if (! applicator.get ()) {
    return;
  }

  bool relative_mode = false;
  if (! current_only && applicator->supports_relative_mode ()) {

    static bool s_relative_mode = true;

    QMessageBox mb (QMessageBox::Question, 
                    tr ("Apply Changes To All"), 
                    tr ("For this operation absolute or relative mode is available which affects the way parameters of the selected objects are changed:\n\n"
                        "In absolute mode, they will be set to the given value. In relative mode, they will be adjusted by the same amount.\n"),
                    QMessageBox::NoButton, this);

    mb.addButton (tr ("Cancel"), QMessageBox::RejectRole);
    QPushButton *absolute = mb.addButton (tr ("Absolute"), QMessageBox::NoRole);
    QPushButton *relative = mb.addButton (tr ("Relative"), QMessageBox::YesRole);
    
    mb.setDefaultButton (s_relative_mode ? relative : absolute);

    mb.exec ();

    if (mb.clickedButton () == absolute) {
      s_relative_mode = relative_mode = false;
    } else if (mb.clickedButton () == relative) {
      s_relative_mode = relative_mode = true;
    } else {
      //  Cancel pressed
      return;
    }

  }

  //  Note: using the apply-all scheme for applying a single change may look like overhead.
  //  But it avoids issues with duplicate selections of the same instance which may happen when
  //  an instance is selected multiple times through different hierarchy branches.

  db::Instance current = m_selection_ptrs [m_index]->back ().inst_ptr;

  std::vector<lay::ObjectInstPath> new_sel;
  new_sel.reserve (m_selection_ptrs.size ());
  for (std::vector<edt::Service::obj_iterator>::const_iterator p = m_selection_ptrs.begin (); p != m_selection_ptrs.end (); ++p) {
    new_sel.push_back (**p);
  }

  std::map<db::Instance, db::Instance> insts_seen;

  bool update_required = false;

  try {

    for (std::vector<edt::Service::obj_iterator>::const_iterator p = m_selection_ptrs.begin (); p != m_selection_ptrs.end (); ++p) {

      edt::Service::obj_iterator pos = *p;

      //  only update objects from the same layout - this is not practical limitation but saves a lot of effort for
      //  managing different property id's etc.
      if (pos->cv_index () != cv_index) {
        continue;
      }

      tl_assert (pos->is_cell_inst ());

      db::Instance new_inst = pos->back ().inst_ptr;

      //  Don't apply the same change twice
      std::map<db::Instance, db::Instance>::const_iterator i = insts_seen.find (pos->back ().inst_ptr);
      if (i == insts_seen.end ()) {

        const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());

        db::Cell &cell = cv->layout ().cell (pos->cell_index ());
        double dbu = cv->layout ().dbu ();

        if (!current_only || pos->back ().inst_ptr == current) {
          new_inst = applicator->do_apply_inst (cell, pos->back ().inst_ptr, dbu, relative_mode);
        }

        insts_seen.insert (std::make_pair (pos->back ().inst_ptr, new_inst));

      } else {
        new_inst = i->second;
      }

      if (new_inst != pos->back ().inst_ptr) {

        size_t index = p - m_selection_ptrs.begin ();

        //  change selection to new instance
        new_sel[index].back ().inst_ptr = new_inst;

        mp_service->select (*pos, lay::Editable::Reset);
        mp_service->select (new_sel[index], lay::Editable::Add);

        update_required = true;

      } 

    }

    if (update_required) {
      recompute_selection_ptrs (new_sel);
    }

  } catch (...) {
    recompute_selection_ptrs (new_sel);
    throw;
  }

  mp_service->view ()->add_new_layers (layer_state);

  //  remove superfluous proxies
  for (unsigned int i = 0; i < mp_service->view ()->cellviews (); ++i) {
    mp_service->view ()->cellview (i)->layout ().cleanup ();
  }

  update ();
}

void 
InstPropertiesPage::apply ()
{
  do_apply (true);
}

bool
InstPropertiesPage::can_apply_to_all () const
{
  return m_selection_ptrs.size () > 1;
}

void 
InstPropertiesPage::apply_to_all ()
{
  do_apply (false);
}

void
InstPropertiesPage::update_pcell_parameters ()
{
  db::Layout *layout;

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
  //  the library selected
  if (lib_cbx->current_library ()) {

    layout = &lib_cbx->current_library ()->layout ();

  } else {

    edt::Service::obj_iterator pos = m_selection_ptrs [m_index];
    const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());
    layout = &cv->layout ();

  }

  std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (cell_name_le->text ()).c_str ());
  std::pair<bool, db::cell_index_type> cc = layout->cell_by_name (tl::to_string (cell_name_le->text ()).c_str ());

  //  by the way, update the foreground color of the cell edit box as well (red, if not valid)
  QPalette pl = cell_name_le->palette ();
  if (! pc.first && ! cc.first) {
    pl.setColor (QPalette::Text, Qt::red);
    pl.setColor (QPalette::Base, QColor (Qt::red).lighter (180));
  } else {
    pl.setColor (QPalette::Text, palette ().color (QPalette::Text));
    pl.setColor (QPalette::Base, palette ().color (QPalette::Base));
  }
  cell_name_le->setPalette (pl);

  if (pc.first && layout->pcell_declaration (pc.second)) {

    const db::PCellDeclaration *pcell_decl = layout->pcell_declaration (pc.second);

    std::vector<tl::Variant> parameters;

    edt::Service::obj_iterator pos = m_selection_ptrs [m_index];
    const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());
    db::Cell &cell = cv->layout ().cell (pos->cell_index ());
    std::pair<bool, db::pcell_id_type> pci = cell.is_pcell_instance (pos->back ().inst_ptr);
    const db::Library *pci_lib = cv->layout ().defining_library (pos->back ().inst_ptr.cell_index ()).first;

    //  fetch the parameters of the current instance if it matches the selected PCell and in that
    //  case use the current parameters
    if (pci.first) {
      if (pci.second == pc.second && pci_lib == lib_cbx->current_library ()) {
        //  exact match: take the parameters
        parameters = cell.get_pcell_parameters (pos->back ().inst_ptr);
      } else {
        //  otherwise: update the parameter whose name matches, use default for others
        parameters = pcell_decl->map_parameters (cell.get_named_pcell_parameters (pos->back ().inst_ptr));
      }
    }

    if (mp_pcell_parameters && mp_pcell_parameters->pcell_decl () == pcell_decl) {

      //  For identical parameter declarations just set the new values
      //  TODO: formally the declaration could change - compare current declarations vs. new ones?
      //  Better: provide a way to install custom parameter pages ...
      mp_pcell_parameters->set_parameters (parameters);

    } else {

      //  Hint: we shall not delete the page immediately. This gives a segmentation fault in some cases.
      if (mp_pcell_parameters) {
        mp_pcell_parameters->hide ();
        mp_pcell_parameters->deleteLater ();
      }

      mp_pcell_parameters = new PCellParametersPage (pcell_tab, &cv->layout (), mp_service->view (), pos->cv_index (), layout->pcell_declaration (pc.second), parameters);
      pcell_tab->layout ()->addWidget (mp_pcell_parameters);

    }

    param_tab_widget->setTabEnabled (1, true);
    param_tab_widget->setCurrentIndex (1);

  } else {

    //  Hint: we shall not delete the page immediately. This gives a segmentation fault in some cases.
    if (mp_pcell_parameters) {
      mp_pcell_parameters->hide ();
      mp_pcell_parameters->deleteLater ();
    }

    mp_pcell_parameters = 0;

    param_tab_widget->setCurrentIndex (0);
    param_tab_widget->setTabEnabled (1, false);
  }

}

}

