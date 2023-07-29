
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

#include "edtPropertiesPages.h"
#include "edtPropertiesPageUtils.h"
#include "edtDialogs.h"
#include "edtPropertiesPageUtils.h"
#include "layDialogs.h"
#include "layObjectInstPath.h"
#include "layLayoutViewBase.h"
#include "layQtTools.h"
#include "tlExceptions.h"
#include "tlString.h"

#include <string>

#include <QMessageBox>

namespace edt
{

// -------------------------------------------------------------------------
//  ShapePropertiesPage implementation

ShapePropertiesPage::ShapePropertiesPage (const std::string &description, edt::Service *service, db::Manager *manager, QWidget *parent)
  : lay::PropertiesPage (parent, manager, service),
    m_description (description), mp_service (service), m_enable_cb_callback (true)
{
  m_selection_ptrs.reserve (service->selection ().size ());
  for (edt::Service::obj_iterator s = service->selection ().begin (); s != service->selection ().end (); ++s) {
    m_selection_ptrs.push_back (s);
  }
  m_prop_id = 0;
  mp_service->clear_highlights ();
}

ShapePropertiesPage::~ShapePropertiesPage ()
{
  mp_service->restore_highlights ();
}

void 
ShapePropertiesPage::setup ()
{
  connect (dbu_checkbox (), SIGNAL (toggled (bool)), this, SLOT (display_mode_changed (bool)));
  connect (abs_checkbox (), SIGNAL (toggled (bool)), this, SLOT (display_mode_changed (bool)));

  m_enable_cb_callback = false;
  dbu_checkbox ()->setChecked (mp_service->view ()->dbu_coordinates ());
  abs_checkbox ()->setChecked (mp_service->view ()->absolute_coordinates ());
  m_enable_cb_callback = true;
}

size_t
ShapePropertiesPage::count () const
{
  return m_selection_ptrs.size ();
}

void
ShapePropertiesPage::select_entries (const std::vector<size_t> &entries)
{
  m_indexes = entries;
}

lay::LayoutViewBase *
ShapePropertiesPage::view () const
{
  return mp_service->view ();
}

const db::Shape &
ShapePropertiesPage::shape (size_t entry) const
{
  return m_selection_ptrs [entry]->shape ();
}

double
ShapePropertiesPage::dbu (size_t entry) const
{
  unsigned int cv_index = m_selection_ptrs [entry]->cv_index ();
  return view ()->cellview (cv_index)->layout ().dbu ();
}

std::string
ShapePropertiesPage::description (size_t entry) const
{
  unsigned int cv_index = m_selection_ptrs [entry]->cv_index ();
  unsigned int layer = m_selection_ptrs [entry]->layer ();

  if (view ()->cellview (cv_index).is_valid ()) {
    const db::LayerProperties &lp = view ()->cellview (cv_index)->layout ().get_properties (layer);
    if (view ()->cellviews () > 1) {
      return lp.to_string () + "@" + tl::to_string (cv_index + 1);
    } else {
      return lp.to_string ();
    }
  }

  return std::string ();
}

QIcon
ShapePropertiesPage::icon (size_t entry, int w, int h) const
{
  int cv_index = m_selection_ptrs [entry]->cv_index ();
  int layer = m_selection_ptrs [entry]->layer ();

  auto *view = mp_service->view ();
  for (auto lp = view->begin_layers (view->current_layer_list ()); ! lp.at_end (); ++lp) {
    const lay::LayerPropertiesNode *ln = lp.operator-> ();
    if (ln->cellview_index () == cv_index && ln->layer_index () == layer) {
      return QIcon (QPixmap::fromImage (view->icon_for_layer (lp, w, h).to_image ()));
    }
  }

  return QIcon ();
}
std::string
ShapePropertiesPage::description () const
{
  return m_description;
}

void
ShapePropertiesPage::leave ()
{
  mp_service->clear_highlights ();
}

bool 
ShapePropertiesPage::dbu_units () const
{
  return dbu_checkbox ()->isChecked ();
}

bool 
ShapePropertiesPage::abs_trans () const
{
  return abs_checkbox ()->isChecked ();
}

db::ICplxTrans
ShapePropertiesPage::trans () const
{
  if (abs_trans () && ! m_indexes.empty ()) {
    return m_selection_ptrs[m_indexes.front ()]->trans ();
  } else {
    return db::ICplxTrans ();
  }
}

void 
ShapePropertiesPage::display_mode_changed (bool)
{
BEGIN_PROTECTED

  if (! m_enable_cb_callback) {
    return;
  }

  update_shape ();

END_PROTECTED
}

void 
ShapePropertiesPage::update ()
{
  mp_service->highlight (m_indexes);

  update_shape ();
}

void 
ShapePropertiesPage::recompute_selection_ptrs (const std::vector<lay::ObjectInstPath> &new_sel)
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
ShapePropertiesPage::do_apply (bool current_only, bool relative)
{
  if (m_indexes.empty ()) {
    return;
  }

  std::unique_ptr<ChangeApplicator> applicator;

  unsigned int cv_index = m_selection_ptrs [m_indexes.front ()]->cv_index ();

  {
    edt::Service::obj_iterator pos = m_selection_ptrs [m_indexes.front ()];
    tl_assert (! pos->is_cell_inst ());

    const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());

    db::Shapes &shapes = cv->layout ().cell (pos->cell_index ()).shapes (pos->layer ());
    double dbu = cv->layout ().dbu ();

    applicator.reset (create_applicator (shapes, pos->shape (), dbu));

    if (m_prop_id != pos->shape ().prop_id ()) {
      applicator.reset (new CombinedChangeApplicator (applicator.release (), new ChangePropertiesApplicator (m_prop_id)));
    }
  }

  if (! applicator.get ()) {
    return;
  }

  //  Ask whether to use relative or absolute mode
  bool relative_mode = false;
  if (! current_only && applicator->supports_relative_mode ()) {
    relative_mode = relative;
  }

  //  Note: using the apply-all scheme for applying a single change may look like overhead.
  //  But it avoids issues with duplicate selections of the same shape which may happen when
  //  a shape is selected multiple times through different hierarchy branches.

  db::Shape current = m_selection_ptrs [m_indexes.front ()]->shape ();

  std::vector<lay::ObjectInstPath> new_sel;
  new_sel.reserve (m_selection_ptrs.size ());
  for (std::vector<edt::Service::obj_iterator>::const_iterator p = m_selection_ptrs.begin (); p != m_selection_ptrs.end (); ++p) {
    new_sel.push_back (**p);
  }

  std::map<db::Shape, db::Shape> shapes_seen;

  bool update_required = false;

  try {

    for (auto i = m_indexes.begin (); i != m_indexes.end (); ++i) {

      size_t index = *i;
      edt::Service::obj_iterator pos = m_selection_ptrs [*i];

      //  only update objects from the same layout - this is not practical limitation but saves a lot of effort for
      //  managing different property id's etc.
      if (pos->cv_index () != cv_index) {
        continue;
      }

      const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());
      db::Layout &layout = cv->layout ();

      tl_assert (! pos->is_cell_inst ());

      if (pos->shape ().is_array_member ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Shape array members cannot be changed")));
      }

      db::Shape new_shape = pos->shape ();

      //  Don't apply the same change twice
      std::map<db::Shape, db::Shape>::const_iterator s = shapes_seen.find (pos->shape ());
      if (s == shapes_seen.end ()) {

        db::Shapes &shapes = layout.cell (pos->cell_index ()).shapes (pos->layer ());
        double dbu = layout.dbu ();

        if (!current_only || pos->shape () == current) {
          new_shape = applicator->do_apply (shapes, pos->shape (), dbu, relative_mode);
        }

        shapes_seen.insert (std::make_pair (pos->shape (), new_shape));

      } else {
        new_shape = s->second;
      }

      if (new_shape != pos->shape ()) {

        //  change selection to new shape
        new_sel[index].set_shape (new_shape);

        mp_service->select (*pos, lay::Editable::Reset);
        mp_service->select (new_sel [index], lay::Editable::Add);

        update_required = true;

      }

      //  handle the case of guiding shape updates
      std::pair<bool, lay::ObjectInstPath> gs = mp_service->handle_guiding_shape_changes (new_sel[index]);
      if (gs.first) {

        new_sel[index] = gs.second;

        mp_service->select (*pos, lay::Editable::Reset);
        mp_service->select (new_sel [index], lay::Editable::Add);

        update_required = true;

      }

    }

    if (update_required) {
      mp_service->view ()->cellview (cv_index)->layout ().cleanup ();
      recompute_selection_ptrs (new_sel);
    }

  } catch (...) {
    mp_service->view ()->cellview (cv_index)->layout ().cleanup ();
    recompute_selection_ptrs (new_sel);
    throw;
  }

  update ();
}

void 
ShapePropertiesPage::apply ()
{
  do_apply (true, false);
}

bool
ShapePropertiesPage::can_apply_to_all () const
{
  return m_selection_ptrs.size () > 1;
}

void 
ShapePropertiesPage::apply_to_all (bool relative)
{
  do_apply (false, relative);
}

void 
ShapePropertiesPage::update_shape ()
{
  if (m_indexes.empty ()) {
    return;
  }

  edt::Service::obj_iterator pos = m_selection_ptrs [m_indexes.front ()];

  const lay::CellView &cv = mp_service->view ()->cellview (pos->cv_index ());
  double dbu = cv->layout ().dbu ();

  tl_assert (! pos->is_cell_inst ());

  std::string layer (tl::to_string (QObject::tr ("Layer ")));

  std::string ln = cv->layout ().get_properties (pos->layer ()).to_string ();
  for (lay::LayerPropertiesConstIterator lp = mp_service->view ()->begin_layers (); ! lp.at_end (); ++lp) {
    if (lp->cellview_index () == int (pos->cv_index ()) && lp->layer_index () == int (pos->layer ())) {
      ln = lp->display_string (mp_service->view (), true, true);
      break;
    }
  }
  layer += ln;

  layer += ", ";
  layer += tl::to_string (QObject::tr ("Cell "));
  layer += cv->layout ().cell_name (pos->cell_index ());

  mp_service->view ()->set_current_layer (pos->cv_index (), cv->layout ().get_properties (pos->layer ()));

  m_prop_id = pos->shape ().prop_id ();

  do_update (pos->shape (), dbu, layer);
}

void
ShapePropertiesPage::show_inst ()
{
  if (m_indexes.empty ()) {
    return;
  }

  InstantiationForm inst_form (this);
  inst_form.show (mp_service->view (), *m_selection_ptrs [m_indexes.front ()]);
}

void
ShapePropertiesPage::show_props ()
{
  if (m_indexes.empty ()) {
    return;
  }

  lay::UserPropertiesForm props_form (this);
  if (props_form.show (mp_service->view (), m_selection_ptrs [m_indexes.front ()]->cv_index (), m_prop_id)) {
    emit edited ();
  }
}

bool 
ShapePropertiesPage::readonly ()
{
  return ! mp_service->view ()->is_editable ();
}

// -------------------------------------------------------------------------
//  PolygonPropertiesPage implementation

PolygonPropertiesPage::PolygonPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent)
  : ShapePropertiesPage (tl::to_string (tr ("Polygons")), service, manager, parent), m_in_text_changed (false)
{
  setupUi (this);
  setup ();

#if QT_VERSION >= 0x60000
  pointListEdit->setTabStopDistance (100);
#else
  pointListEdit->setTabStopWidth (100);
#endif

  connect (inst_pb, SIGNAL (clicked ()), this, SLOT (show_inst ()));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (show_props ()));

  if (! readonly ()) {
    connect (pointListEdit, SIGNAL (textChanged ()), this, SLOT (text_changed ()));
  } else {
    pointListEdit->setReadOnly (true);
  }
}

static size_t count_polygon_points (const db::Shape &sh)
{
  size_t n = 0;
  for (auto pt = sh.begin_hull (); pt != sh.end_hull (); ++pt) {
    ++n;
  }
  return n;
}

std::string
PolygonPropertiesPage::description (size_t entry) const
{
  const db::Shape &sh = shape (entry);

  size_t npts = count_polygon_points (sh);
  if (sh.holes () == 0 && npts > 4) {
    return ShapePropertiesPage::description (entry) + " - " + tl::sprintf (tl::to_string (tr ("Polygon(%d points)")), npts);
  } else if (sh.holes () > 0) {
    return ShapePropertiesPage::description (entry) + " - " + tl::sprintf (tl::to_string (tr ("Polygon(%d points, %d holes)")), npts, sh.holes ());
  } else {
    db::Polygon poly;
    sh.polygon (poly);
    db::CplxTrans dbu_trans (dbu (entry));
    return ShapePropertiesPage::description (entry) + " - " + tl::sprintf (tl::to_string (tr ("Polygon%s")), (dbu_trans * poly).to_string ());
  }
}

void
PolygonPropertiesPage::do_update (const db::Shape &shape, double dbu, const std::string &lname)
{
  layer_lbl->setText (tl::to_qstring (lname));

  db::Polygon poly;
  shape.polygon (poly);

  std::string ptlist;
  ptlist.reserve (4096);

  db::CplxTrans t = db::CplxTrans (trans ());
  bool du = dbu_units ();

  bool first = true;
  for (db::Polygon::polygon_contour_iterator pt = poly.begin_hull (); pt != poly.end_hull (); ++pt) {
    if (! first) {
      ptlist += "\n";
    } else {
      first = false;
    }
    ptlist += coords_to_string (t * *pt, dbu, du);
  }

  for (unsigned int h = 0; h < poly.holes (); ++h) {

    ptlist += "\n/";

    for (db::Polygon::polygon_contour_iterator pt = poly.begin_hole (h); pt != poly.end_hole (h); ++pt) {
      ptlist += "\n";
      ptlist += coords_to_string (t * *pt, dbu, du);
    }

  }

  if (! m_in_text_changed) {
    pointListEdit->blockSignals (true);
    pointListEdit->setText (tl::to_qstring (ptlist));
    pointListEdit->blockSignals (false);
  }

  pointCountLabel->setText (tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("(%lu points)")), poly.vertices ())));
}

void
PolygonPropertiesPage::text_changed ()
{
  m_in_text_changed = true;
  try {
    emit edited ();
  } catch (tl::Exception &) {
    //  ignore exceptions
  }
  m_in_text_changed = false;
}

ChangeApplicator *
PolygonPropertiesPage::create_applicator (db::Shapes & /*shapes*/, const db::Shape &shape, double dbu)
{
  db::Polygon poly;

  try {

    std::string text (tl::to_string (pointListEdit->toPlainText ()));
    tl::Extractor ex (text.c_str ());

    db::VCplxTrans t = db::CplxTrans (trans ()).inverted ();
    bool du = dbu_units ();

    if (*ex.skip () == '(') {

      db::DPolygon dp;
      ex.read (dp);

      poly = db::Polygon (dp.transformed (db::DCplxTrans (t) * db::DCplxTrans (du ? 1.0 : 1.0 / dbu)));

    } else {

      unsigned int h = 0;
      while (! ex.at_end ()) {

        std::vector <db::Point> points;

        while (! ex.at_end () && ! ex.test ("/")) {

          double dx = 0.0, dy = 0.0;
          ex.read (dx);
          ex.test (",");
          ex.read (dy);
          ex.test (";");

          points.push_back (point_from_dpoint (db::DPoint (dx, dy), dbu, du, t));

        }

        if (points.size () < 3) {
          throw tl::Exception (tl::to_string (QObject::tr ("Polygon must have at least three points")));
        }

        if (h == 0) {
          poly.assign_hull (points.begin (), points.end (), false /*not compressed*/);
        } else {
          poly.insert_hole (points.begin (), points.end (), false /*not compressed*/);
        }

        ++h;

      }

    }

    lay::indicate_error (pointListEdit, (tl::Exception *) 0);

  } catch (tl::Exception &ex) {
    lay::indicate_error (pointListEdit, &ex);
    throw;
  }

  db::Polygon org_poly;
  shape.polygon (org_poly);

  //  shape changed - replace the old by the new one
  return new PolygonChangeApplicator (poly, org_poly);
}

// -------------------------------------------------------------------------
//  BoxPropertiesPage implementation

static bool s_coordinateMode = true;

BoxPropertiesPage::BoxPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent)
  : ShapePropertiesPage (tl::to_string (tr ("Boxes")), service, manager, parent),
    m_recursion_sentinel (false), m_tab_index (0), m_dbu (1.0), m_lr_swapped (false), m_tb_swapped (false)
{
  setupUi (this);
  setup ();

  mode_tab->setCurrentIndex (s_coordinateMode ? 0 : 1);

  if (! readonly ()) {

    connect (mode_tab, SIGNAL (currentChanged (int)), this, SLOT (changed ()));
    connect (x1_le_1, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (y1_le_1, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (x2_le_1, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (y2_le_1, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (w_le_2, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (h_le_2, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (cx_le_2, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (cy_le_2, SIGNAL (editingFinished ()), this, SLOT (changed ()));

  } else {

    x1_le_1->setReadOnly (true);
    y1_le_1->setReadOnly (true);
    x2_le_1->setReadOnly (true);
    y2_le_1->setReadOnly (true);
    w_le_2->setReadOnly (true);
    h_le_2->setReadOnly (true);
    cx_le_2->setReadOnly (true);
    cy_le_2->setReadOnly (true);

  }

  connect (inst_pb, SIGNAL (clicked ()), this, SLOT (show_inst ()));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (show_props ()));
}

std::string
BoxPropertiesPage::description (size_t entry) const
{
  const db::Shape &sh = shape (entry);
  db::CplxTrans dbu_trans (dbu (entry));
  return ShapePropertiesPage::description (entry) + " - " + tl::sprintf (tl::to_string (tr ("Box%s")), (dbu_trans * sh.box ()).to_string ());
}

void 
BoxPropertiesPage::do_update (const db::Shape &shape, double dbu, const std::string &lname)
{
  m_dbu = dbu;
  m_lr_swapped = false;
  m_tb_swapped = false;

  layer_lbl->setText (tl::to_qstring (lname));

  db::Box box;
  shape.box (box);
  set_box (box);
}

ChangeApplicator *
BoxPropertiesPage::create_applicator (db::Shapes & /*shapes*/, const db::Shape &shape, double dbu)
{
  m_dbu = dbu;

  db::Box box = get_box (mode_tab->currentIndex ());

  db::Box org_box;
  shape.box (org_box);

  if (box != org_box) {
    return new BoxDimensionsChangeApplicator (box.left () - org_box.left (), box.bottom () - org_box.bottom (),
                                              box.right () - org_box.right (), box.top () - org_box.top (),
                                              box.left (), box.bottom (), box.right (), box.top ());
  } else {
    return 0;
  }
}

db::Box 
BoxPropertiesPage::get_box (int mode) const
{
  if (mode == 0) {

    bool has_error = false;
    double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

    try {
      tl::from_string_ext (tl::to_string (x1_le_1->text ()), x1);
      lay::indicate_error (x1_le_1, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (x1_le_1, &ex);
      has_error = true;
    }

    try {
      tl::from_string_ext (tl::to_string (y1_le_1->text ()), y1);
      lay::indicate_error (y1_le_1, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (y1_le_1, &ex);
      has_error = true;
    }

    try {
      tl::from_string_ext (tl::to_string (x2_le_1->text ()), x2);
      lay::indicate_error (x2_le_1, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (x2_le_1, &ex);
      has_error = true;
    }

    try {
      tl::from_string_ext (tl::to_string (y2_le_1->text ()), y2);
      lay::indicate_error (y2_le_1, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (y2_le_1, &ex);
      has_error = true;
    }

    if (has_error) {
      throw tl::Exception (tl::to_string (tr ("Invalid values - see highlighted entry boxes")));
    }

    if (m_lr_swapped) {
      std::swap (x1, x2);
    }
    if (m_tb_swapped) {
      std::swap (y1, y2);
    }

    if (x1 > x2 + 1e-6) {
      m_lr_swapped = !m_lr_swapped;
    }
    if (y1 > y2 + 1e-6) {
      m_tb_swapped = !m_tb_swapped;
    }

    db::VCplxTrans t = db::VCplxTrans (trans ().inverted ());
    bool du = dbu_units ();

    return db::Box (point_from_dpoint (db::DPoint (x1, y1), m_dbu, du, t), point_from_dpoint (db::DPoint (x2, y2), m_dbu, du, t));

  } else {

    bool has_error = false;
    double cx = 0.0, cy = 0.0, w = 0.0, h = 0.0;

    try {
      tl::from_string_ext (tl::to_string (cx_le_2->text ()), cx);
      lay::indicate_error (cx_le_2, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (cx_le_2, &ex);
      has_error = true;
    }

    try {
      tl::from_string_ext (tl::to_string (cy_le_2->text ()), cy);
      lay::indicate_error (cy_le_2, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (cy_le_2, &ex);
      has_error = true;
    }

    try {
      tl::from_string_ext (tl::to_string (w_le_2->text ()), w);
      lay::indicate_error (w_le_2, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (w_le_2, &ex);
      has_error = true;
    }

    try {
      tl::from_string_ext (tl::to_string (h_le_2->text ()), h);
      lay::indicate_error (h_le_2, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (h_le_2, &ex);
      has_error = true;
    }

    if (has_error) {
      throw tl::Exception (tl::to_string (tr ("Invalid values - see highlighted entry boxes")));
    }

    db::VCplxTrans t = db::VCplxTrans (trans ().inverted ());
    bool du = dbu_units ();

    return db::Box (point_from_dpoint (db::DPoint (cx - w * 0.5, cy - h * 0.5), m_dbu, du, t), point_from_dpoint (db::DPoint (cx + w * 0.5, cy + h * 0.5), m_dbu, du, t));

  }
}

void
BoxPropertiesPage::set_box (const db::Box &box)
{
  if (m_recursion_sentinel) {
    return;
  }
  m_recursion_sentinel = true;

  m_tab_index = mode_tab->currentIndex ();

  db::CplxTrans t = db::CplxTrans (trans ());
  db::DBox bt = db::DBox (t (box.lower_left ()), t (box.upper_right ()));

  bool du = dbu_units ();

  (m_lr_swapped ? x2_le_1 : x1_le_1)->setText (tl::to_qstring (coord_to_string (bt.lower_left ().x (), m_dbu, du)));
  (m_tb_swapped ? y2_le_1 : y1_le_1)->setText (tl::to_qstring (coord_to_string (bt.lower_left ().y (), m_dbu, du)));
  (m_lr_swapped ? x1_le_1 : x2_le_1)->setText (tl::to_qstring (coord_to_string (bt.upper_right ().x (), m_dbu, du)));
  (m_tb_swapped ? y1_le_1 : y2_le_1)->setText (tl::to_qstring (coord_to_string (bt.upper_right ().y (), m_dbu, du)));

  cx_le_1->setText (tl::to_qstring (coord_to_string (bt.center ().x (), m_dbu, du)));
  cy_le_1->setText (tl::to_qstring (coord_to_string (bt.center ().y (), m_dbu, du)));
  w_le_1->setText (tl::to_qstring (coord_to_string (bt.width (), m_dbu, du)));
  h_le_1->setText (tl::to_qstring (coord_to_string (bt.height (), m_dbu, du)));

  (m_lr_swapped ? x2_le_2 : x1_le_2)->setText (tl::to_qstring (coord_to_string (bt.lower_left ().x (), m_dbu, du)));
  (m_tb_swapped ? y2_le_2 : y1_le_2)->setText (tl::to_qstring (coord_to_string (bt.lower_left ().y (), m_dbu, du)));
  (m_lr_swapped ? x1_le_2 : x2_le_2)->setText (tl::to_qstring (coord_to_string (bt.upper_right ().x (), m_dbu, du)));
  (m_tb_swapped ? y1_le_2 : y2_le_2)->setText (tl::to_qstring (coord_to_string (bt.upper_right ().y (), m_dbu, du)));

  cx_le_2->setText (tl::to_qstring (coord_to_string (bt.center ().x (), m_dbu, du)));
  cy_le_2->setText (tl::to_qstring (coord_to_string (bt.center ().y (), m_dbu, du)));
  w_le_2->setText (tl::to_qstring (coord_to_string (bt.width (), m_dbu, du)));
  h_le_2->setText (tl::to_qstring (coord_to_string (bt.height (), m_dbu, du)));

  m_recursion_sentinel = false;
}

void 
BoxPropertiesPage::changed ()
{
  s_coordinateMode = (mode_tab->currentIndex () == 0);

  try {
    set_box (get_box (m_tab_index));
  } catch (...) {
  }

  emit edited ();
}

// -------------------------------------------------------------------------
//  PointPropertiesPage implementation

PointPropertiesPage::PointPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent)
  : ShapePropertiesPage (tl::to_string (tr ("Points")), service, manager, parent),
    m_dbu (1.0)
{
  setupUi (this);
  setup ();

  if (! readonly ()) {

    connect (x_le, SIGNAL (editingFinished ()), this, SLOT (changed ()));
    connect (y_le, SIGNAL (editingFinished ()), this, SLOT (changed ()));

  } else {

    x_le->setReadOnly (true);
    y_le->setReadOnly (true);

  }

  connect (inst_pb, SIGNAL (clicked ()), this, SLOT (show_inst ()));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (show_props ()));
}

std::string
PointPropertiesPage::description (size_t entry) const
{
  const db::Shape &sh = shape (entry);
  db::CplxTrans dbu_trans (dbu (entry));
  return ShapePropertiesPage::description (entry) + " - " + tl::sprintf (tl::to_string (tr ("Point%s")), (dbu_trans * sh.point ()).to_string ());
}

void
PointPropertiesPage::do_update (const db::Shape &shape, double dbu, const std::string &lname)
{
  m_dbu = dbu;

  layer_lbl->setText (tl::to_qstring (lname));

  db::Point point;
  shape.point (point);
  set_point (point);
}

ChangeApplicator *
PointPropertiesPage::create_applicator (db::Shapes & /*shapes*/, const db::Shape &shape, double dbu)
{
  m_dbu = dbu;

  db::Point point = get_point ();

  db::Point org_point;
  shape.point (org_point);

  if (point != org_point) {
    return new PointDimensionsChangeApplicator (point, org_point);
  } else {
    return 0;
  }
}

db::Point
PointPropertiesPage::get_point () const
{
  bool has_error = false;
  double x = 0.0, y = 0.0;

  try {
    tl::from_string_ext (tl::to_string (x_le->text ()), x);
    lay::indicate_error (x_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (x_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (y_le->text ()), y);
    lay::indicate_error (y_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (y_le, &ex);
    has_error = true;
  }

  if (has_error) {
    throw tl::Exception (tl::to_string (tr ("Invalid values - see highlighted entry boxes")));
  }

  db::VCplxTrans t = db::VCplxTrans (trans ().inverted ());
  bool du = dbu_units ();

  return point_from_dpoint (db::DPoint (x, y), m_dbu, du, t);
}

void
PointPropertiesPage::set_point (const db::Point &point)
{
  db::CplxTrans t = db::CplxTrans (trans ());
  db::DPoint pt = db::DPoint (t (point));

  bool du = dbu_units ();

  x_le->setText (tl::to_qstring (coord_to_string (pt.x (), m_dbu, du)));
  y_le->setText (tl::to_qstring (coord_to_string (pt.y (), m_dbu, du)));
}

void
PointPropertiesPage::changed ()
{
  try {
    set_point (get_point ());
  } catch (...) {
  }

  emit edited ();
}

// -------------------------------------------------------------------------
//  TextPropertiesPage implementation

TextPropertiesPage::TextPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent)
  : ShapePropertiesPage (tl::to_string (tr ("Texts")), service, manager, parent)
{
  setupUi (this);
  setup ();

  connect (inst_pb, SIGNAL (clicked ()), this, SLOT (show_inst ()));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (show_props ()));

  if (! readonly ()) {

    connect (text_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
    connect (x_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
    connect (y_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
    connect (size_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
    connect (orient_cbx, SIGNAL (activated (int)), this, SIGNAL (edited ()));
    connect (halign_cbx, SIGNAL (activated (int)), this, SIGNAL (edited ()));
    connect (valign_cbx, SIGNAL (activated (int)), this, SIGNAL (edited ()));

  } else {

    text_le->setReadOnly (true);
    x_le->setReadOnly (true);
    y_le->setReadOnly (true);
    size_le->setReadOnly (true);
    orient_cbx->setEnabled (false);
    halign_cbx->setEnabled (false);
    valign_cbx->setEnabled (false);

  }
}

std::string
TextPropertiesPage::description (size_t entry) const
{
  const db::Shape &sh = shape (entry);
  db::Text text;
  sh.text (text);
  db::CplxTrans dbu_trans (dbu (entry));
  return ShapePropertiesPage::description (entry) + " - " + tl::sprintf (tl::to_string (tr ("Text%s")), (dbu_trans * text).to_string ());
}

void
TextPropertiesPage::do_update (const db::Shape &shape, double dbu, const std::string &lname)
{
  layer_lbl->setText (tl::to_qstring (lname));

  db::Text text;
  shape.text (text);

  db::CplxTrans t = db::CplxTrans (trans ());
  bool du = dbu_units ();
  db::DPoint dp = t * (db::Point () + text.trans ().disp ());

  text_le->setText (tl::to_qstring (tl::escape_string (text.string ())));
  x_le->setText (tl::to_qstring (coord_to_string (dp.x (), dbu, du)));
  y_le->setText (tl::to_qstring (coord_to_string (dp.y (), dbu, du)));
  if (text.size () != 0) {
    size_le->setText (tl::to_qstring (coord_to_string (t.ctrans (text.size ()), dbu, du)));
  } else {
    size_le->setText (QString ());
  }
  orient_cbx->setCurrentIndex (text.trans ().rot ());
  halign_cbx->setCurrentIndex (int (text.halign ()) + 1);
  valign_cbx->setCurrentIndex (int (text.valign ()) + 1);
}

ChangeApplicator *
TextPropertiesPage::create_applicator (db::Shapes & /*shapes*/, const db::Shape &shape, double dbu)
{
  bool has_error = false;

  db::VCplxTrans t = db::CplxTrans (trans ()).inverted ();
  bool du = dbu_units ();

  double x = 0.0, y = 0.0;

  try {
    tl::from_string_ext (tl::to_string (x_le->text ()), x);
    lay::indicate_error (x_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (x_le, &ex);
    has_error = true;
  }

  try {
    tl::from_string_ext (tl::to_string (y_le->text ()), y);
    lay::indicate_error (y_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (y_le, &ex);
    has_error = true;
  }

  db::Vector tp = db::Vector (point_from_dpoint (db::DPoint (x, y), dbu, du, t));
  db::Trans tt (orient_cbx->currentIndex (), tp);

  std::string str = tl::unescape_string (tl::to_string (text_le->text ()));

  db::Text org_text;
  shape.text (org_text);

  std::unique_ptr<CombinedChangeApplicator> appl (new CombinedChangeApplicator ());

  if (db::FTrans (tt) != db::FTrans (org_text.trans ())) {
    appl->add (new TextOrientationChangeApplicator (db::FTrans (tt)));
  }
  if (tt.disp () != org_text.trans ().disp ()) {
    appl->add (new TextPositionChangeApplicator (tt.disp (), org_text.trans ().disp ()));
  }
  if (db::HAlign (halign_cbx->currentIndex () - 1) != org_text.halign ()) {
    appl->add (new TextHAlignChangeApplicator (db::HAlign (halign_cbx->currentIndex () - 1)));
  }
  if (db::VAlign (valign_cbx->currentIndex () - 1) != org_text.valign ()) {
    appl->add (new TextVAlignChangeApplicator (db::VAlign (valign_cbx->currentIndex () - 1)));
  }

  db::Coord size = 0;
  if (! size_le->text ().isEmpty ()) {
    try {
      size = coord_from_string (tl::to_string (size_le->text ()).c_str (), dbu, du, t);
      lay::indicate_error (size_le, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (size_le, &ex);
      has_error = true;
    }
  }
  if (size != org_text.size ()) {
    appl->add (new TextSizeChangeApplicator (size));
  }

  if (str != org_text.string ()) {
    appl->add (new TextStringChangeApplicator (str));
  }

  if (has_error) {
    throw tl::Exception (tl::to_string (tr ("Invalid values - see highlighted entry boxes")));
  }

  return appl.release ();
}

// -------------------------------------------------------------------------
//  PathPropertiesPage implementation

PathPropertiesPage::PathPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent)
  : ShapePropertiesPage (tl::to_string (tr ("Paths")), service, manager, parent), m_in_text_changed (false)
{
  setupUi (this);
  setup ();

#if QT_VERSION >= 0x60000
  ptlist_le->setTabStopDistance (100);
#else
  ptlist_le->setTabStopWidth (100);
#endif

  connect (inst_pb, SIGNAL (clicked ()), this, SLOT (show_inst ()));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (show_props ()));

  ptlist_le->setReadOnly (true);
  width_le->setReadOnly (true);
  start_ext_le->setReadOnly (true);
  end_ext_le->setReadOnly (true);
  round_cb->setEnabled (false);
}

static size_t count_path_points (const db::Shape &sh)
{
  size_t n = 0;
  for (auto pt = sh.begin_point (); pt != sh.end_point (); ++pt) {
    ++n;
  }
  return n;
}

static std::string path_description (const db::Shape &sh, double dbu)
{
  size_t npts = count_path_points (sh);
  if (npts > 4) {
    return tl::sprintf (tl::to_string (tr ("Path(%d points, w=%.12g)")), npts, sh.path_width () * dbu);
  } else {
    db::CplxTrans dbu_trans (dbu);
    db::Path path;
    sh.path (path);
    return tl::sprintf (tl::to_string (tr ("Path%s")), (dbu_trans * path).to_string ());
  }
}

std::string
PathPropertiesPage::description (size_t entry) const
{
  return ShapePropertiesPage::description (entry) + " - " + path_description (shape (entry), dbu (entry));
}

void
PathPropertiesPage::do_update (const db::Shape &shape, double dbu, const std::string &lname)
{
  layer_lbl->setText (tl::to_qstring (lname));

  db::Path path;
  shape.path (path);

  std::string ptlist;
  ptlist.reserve (4096);

  db::CplxTrans t = db::CplxTrans (trans ());
  bool du = dbu_units ();

  bool first = true;
  for (db::Path::iterator pt = path.begin (); pt != path.end (); ++pt) {
    if (! first) {
      ptlist += "\n";
    } else {
      first = false;
    }
    ptlist += coords_to_string (t * *pt, dbu, du);
  }

  if (! m_in_text_changed) {
    ptlist_le->blockSignals (true);
    ptlist_le->setText (tl::to_qstring (ptlist));
    ptlist_le->blockSignals (false);
  }

  width_le->setText (tl::to_qstring (coord_to_string (t.ctrans (path.width ()), dbu, du)));
  start_ext_le->setText (tl::to_qstring (coord_to_string (t.mag () * path.extensions ().first, dbu, du)));
  end_ext_le->setText (tl::to_qstring (coord_to_string (t.mag () * path.extensions ().second, dbu, du)));
  round_cb->setChecked (path.round ());
}

ChangeApplicator *
PathPropertiesPage::create_applicator (db::Shapes & /*shapes*/, const db::Shape & /*shape*/, double /*dbu*/)
{
  return 0;
}

// -------------------------------------------------------------------------
//  EditablePathPropertiesPage implementation

EditablePathPropertiesPage::EditablePathPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent)
  : ShapePropertiesPage (tl::to_string (tr ("Paths")), service, manager, parent), m_in_text_changed (false)
{
  setupUi (this);
  setup ();

#if QT_VERSION >= 0x60000
  ptlist_le->setTabStopDistance (100);
#else
  ptlist_le->setTabStopWidth (100);
#endif

  connect (inst_pb, SIGNAL (clicked ()), this, SLOT (show_inst ()));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (show_props ()));
  connect (type_cb, SIGNAL (currentIndexChanged (int)), this, SLOT (type_selected (int)));

  connect (ptlist_le, SIGNAL (textChanged ()), this, SLOT (text_changed ()));
  connect (width_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (start_ext_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (end_ext_le, SIGNAL (editingFinished ()), this, SIGNAL (edited ()));
  connect (type_cb, SIGNAL (activated (int)), this, SIGNAL (edited ()));
}

static int 
path_type_choice (const db::Path &path)
{
  db::Coord w = path.width ();
  db::Coord se = path.extensions ().first;
  db::Coord ee = path.extensions ().second;

  if (se == 0 && ee == 0) {
    return 0; // flush
  } else if (se == w / 2 && ee == w / 2 && path.round ()) {
    return 3; // round
  } else if (se == w / 2 && ee == w / 2) {
    return 1; // square
  } else {
    return 2; // variable
  }
}

void
EditablePathPropertiesPage::text_changed ()
{
  m_in_text_changed = true;
  try {
    emit edited ();
  } catch (tl::Exception &) {
    //  ignore exceptions
  }
  m_in_text_changed = false;
}

std::string
EditablePathPropertiesPage::description (size_t entry) const
{
  return ShapePropertiesPage::description (entry) + " - " + path_description (shape (entry), dbu (entry));
}

void
EditablePathPropertiesPage::do_update (const db::Shape &shape, double dbu, const std::string &lname)
{
  layer_lbl->setText (tl::to_qstring (lname));

  db::Path path;
  shape.path (path);

  std::string ptlist;
  ptlist.reserve (4096);

  db::CplxTrans t = db::CplxTrans (trans ());
  bool du = dbu_units ();

  bool first = true;
  for (db::Path::iterator pt = path.begin (); pt != path.end (); ++pt) {
    if (! first) {
      ptlist += "\n";
    } else {
      first = false;
    }
    ptlist += coords_to_string (t * *pt, dbu, du);
  }

  if (! m_in_text_changed) {
    ptlist_le->blockSignals (true);
    ptlist_le->setText (tl::to_qstring (ptlist));
    ptlist_le->blockSignals (false);
  }

  db::Coord w = path.width ();
  db::Coord se = path.extensions ().first;
  db::Coord ee = path.extensions ().second;

  width_le->setText (tl::to_qstring (coord_to_string (t.ctrans (w), dbu, du)));

  start_ext_le->setText (tl::to_qstring (coord_to_string (t.mag () * se, dbu, du)));
  end_ext_le->setText (tl::to_qstring (coord_to_string (t.mag () * ee, dbu, du)));

  int type_choice = path_type_choice (path);
  if (type_cb->currentIndex () == 2) {
    //  keep "variable" mode, otherwise if's difficult to switch to it
    type_choice = 2;
  }
  type_cb->setCurrentIndex (type_choice);
  type_selected (type_choice);
}

ChangeApplicator *
EditablePathPropertiesPage::create_applicator (db::Shapes & /*shapes*/, const db::Shape &shape, double dbu)
{
  bool has_error = false;

  db::VCplxTrans t = db::CplxTrans (trans ()).inverted ();
  bool du = dbu_units ();

  std::string text (tl::to_string (ptlist_le->toPlainText ()));
  tl::Extractor ex (text.c_str ());

  std::vector <db::Point> points;

  try {

    while (! ex.at_end ()) {

      double dx = 0.0, dy = 0.0;
      ex.read (dx);
      ex.read (dy);

      points.push_back (point_from_dpoint (db::DPoint (dx, dy), dbu, du, t));

    }

    if (points.size () < 1) {
      throw tl::Exception (tl::to_string (QObject::tr ("The path must have at least one point")));
    }

    lay::indicate_error (ptlist_le, (tl::Exception *) 0);

  } catch (tl::Exception &ex) {
    lay::indicate_error (ptlist_le, &ex);
    has_error = true;
  }

  db::Coord w = 0;
  try {
    w = coord_from_string (tl::to_string (width_le->text ()).c_str (), dbu, du, t);
    lay::indicate_error (width_le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (width_le, &ex);
    has_error = true;
  }

  db::Coord se = 0, ee = 0;
  switch (type_cb->currentIndex ()) {
  case 0: // flush
    break;
  case 1: // square
  case 3: // round
    se = ee = std::numeric_limits <db::Coord>::min ();  //  force to half width
    break;
  case 2: // variable
    try {
      se = coord_from_string (tl::to_string (start_ext_le->text ()).c_str (), dbu, du, t);
      lay::indicate_error (start_ext_le, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (start_ext_le, &ex);
      has_error = true;
    }
    try {
      ee = coord_from_string (tl::to_string (end_ext_le->text ()).c_str (), dbu, du, t);
      lay::indicate_error (end_ext_le, (tl::Exception *) 0);
    } catch (tl::Exception &ex) {
      lay::indicate_error (end_ext_le, &ex);
      has_error = true;
    }
    break;
  } 

  std::unique_ptr<CombinedChangeApplicator> appl (new CombinedChangeApplicator ());

  db::Path org_path;
  shape.path (org_path);
  std::vector <db::Point> org_points;
  for (db::Path::iterator p = org_path.begin (); p != org_path.end (); ++p) {
    org_points.push_back (*p);
  }

  if (org_points != points) {
    appl->add (new PathPointsChangeApplicator (points, org_points));
  }
  if (w != org_path.width ()) {
    appl->add (new PathWidthChangeApplicator (w, org_path.width ()));
  }

  if (type_cb->currentIndex () != path_type_choice (org_path) || 
      (type_cb->currentIndex () == 2 && (se != org_path.extensions ().first || ee != org_path.extensions ().second))) {
    appl->add (new PathStartExtensionChangeApplicator (se));
    appl->add (new PathEndExtensionChangeApplicator (ee));
    appl->add (new PathRoundEndChangeApplicator (type_cb->currentIndex () == 3));
  }

  if (has_error) {
    throw tl::Exception (tl::to_string (tr ("Invalid values - see highlighted entry boxes")));
  }

  return appl.release ();
}

void
EditablePathPropertiesPage::type_selected (int t)
{
  start_ext_le->setEnabled (t == 2);
  end_ext_le->setEnabled (t == 2);
}

}

#endif
