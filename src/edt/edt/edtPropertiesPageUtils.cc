
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

#include "edtPropertiesPageUtils.h"

#include "dbShapes.h"
#include "dbLayout.h"
#include "dbLibrary.h"
#include "dbPCellDeclaration.h"

#include <QLineEdit>

namespace edt
{

// -------------------------------------------------------------------------
//  CombinedChangeApplicator implementation

CombinedChangeApplicator::CombinedChangeApplicator ()
{
}
    
CombinedChangeApplicator::CombinedChangeApplicator (ChangeApplicator *a1)
{
  m_appl.push_back (a1);
}
    
CombinedChangeApplicator::CombinedChangeApplicator (ChangeApplicator *a1, ChangeApplicator *a2)
{
  m_appl.push_back (a1);
  m_appl.push_back (a2);
}
    
void CombinedChangeApplicator::add (ChangeApplicator *a)
{
  m_appl.push_back (a);
}
    
CombinedChangeApplicator::~CombinedChangeApplicator ()
{
  for (std::vector<ChangeApplicator *>::const_iterator a = m_appl.begin (); a != m_appl.end (); ++a) {
    delete *a;
  }
  m_appl.clear ();
}

bool CombinedChangeApplicator::supports_relative_mode () const 
{ 
  for (std::vector<ChangeApplicator *>::const_iterator a = m_appl.begin (); a != m_appl.end (); ++a) {
    if ((*a)->supports_relative_mode ()) {
      return true;
    }
  }
  return false; 
}

db::Shape CombinedChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const 
{
  db::Shape s = shape;
  for (std::vector<ChangeApplicator *>::const_iterator a = m_appl.begin (); a != m_appl.end (); ++a) {
    if (*a) {
      s = (*a)->do_apply (shapes, s, dbu, relative);
    }
  }
  return s;
}

db::Instance CombinedChangeApplicator::do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const 
{
  db::Instance i = instance;
  for (std::vector<ChangeApplicator *>::const_iterator a = m_appl.begin (); a != m_appl.end (); ++a) {
    if (*a) {
      i = (*a)->do_apply_inst (cell, i, dbu, relative);
    }
  }
  return i;
}

// -------------------------------------------------------------------------
//  ChangePropertiesApplicator implementation

ChangePropertiesApplicator::ChangePropertiesApplicator (db::properties_id_type prop_id) 
  : m_prop_id (prop_id)
{ 
  //  .. nothing yet ...
}

db::Shape ChangePropertiesApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const 
{
  return shapes.replace_prop_id (shape, m_prop_id);
}

db::Instance ChangePropertiesApplicator::do_apply_inst (db::Cell &cell, const db::Instance &instance, double /*dbu*/, bool /*relative*/) const 
{
  return cell.replace_prop_id (instance, m_prop_id);
}

// -------------------------------------------------------------------------
//  BoxDimensionsChangeApplicator implementation

BoxDimensionsChangeApplicator::BoxDimensionsChangeApplicator (db::Coord dl, db::Coord db, db::Coord dr, db::Coord dt, db::Coord l, db::Coord b, db::Coord r, db::Coord t)
  : m_dl (dl), m_db (db), m_dr (dr), m_dt (dt), m_l (l), m_b (b), m_r (r), m_t (t)
{
  //  .. nothing yet ..
}

db::Shape BoxDimensionsChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool relative) const
{
  db::Box org_box;
  shape.box (org_box);

  db::Box new_box;
  if (relative) {
    new_box = db::Box (org_box.left () + m_dl, 
                       org_box.bottom () + m_db,
                       org_box.right () + m_dr, 
                       org_box.top () + m_dt);
  } else {

    db::Coord l = org_box.left ();
    db::Coord r = org_box.right ();

    if (m_dl != 0 && m_dr == 0) {
      //  left side is fixed
      l = m_l;
    } else if (m_dl == 0 && m_dr != 0) {
      //  right side is fixed
      r = m_r;
    } else if (m_dl != 0 && m_dl == m_dr) {
      //  center is fixed
      l = (m_l + m_r) / 2 - org_box.width () / 2;
      r = (m_l + m_r) / 2 + org_box.width () / 2;
    } else if (m_dl != 0 && m_dl == -m_dr) {
      //  width is fixed
      l = org_box.center ().x () - (m_r - m_l) / 2;
      r = org_box.center ().x () + (m_r - m_l) / 2;
    } else if (m_dl != 0 && m_dr != 0) {
      //  both sides have changed
      l = m_l;
      r = m_r;
    }

    db::Coord b = org_box.bottom ();
    db::Coord t = org_box.top ();

    if (m_db != 0 && m_dt == 0) {
      //  left side is fixed
      b = m_b;
    } else if (m_db == 0 && m_dt != 0) {
      //  right side is fixed
      t = m_t;
    } else if (m_db != 0 && m_db == m_dt) {
      //  center is fixed
      b = (m_b + m_t) / 2 - org_box.height () / 2;
      t = (m_b + m_t) / 2 + org_box.height () / 2;
    } else if (m_db != 0 && m_db == -m_dt) {
      //  height is fixed
      b = org_box.center ().y () - (m_t - m_b) / 2;
      t = org_box.center ().y () + (m_t - m_b) / 2;
    } else if (m_db != 0 && m_dt != 0) {
      //  both sides have changed
      b = m_b;
      t = m_t;
    }

    new_box = db::Box (l, b, r, t);

  }

  if (new_box != org_box) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_box);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  PointDimensionsChangeApplicator implementation

PointDimensionsChangeApplicator::PointDimensionsChangeApplicator (const db::Point &point, const db::Point &org_point)
  : m_point (point), m_org_point (org_point)
{
  //  .. nothing yet ..
}

db::Shape PointDimensionsChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool relative) const
{
  db::Point org_point;
  shape.point (org_point);

  db::Point new_point;
  if (relative) {
    new_point = org_point + (m_point - m_org_point);
  } else if (m_point != m_org_point) {
    new_point = org_point;
    if (m_point.x () != m_org_point.x ()) {
      new_point.set_x (m_point.x ());
    }
    if (m_point.y () != m_org_point.y ()) {
      new_point.set_y (m_point.y ());
    }
  }

  if (new_point != org_point) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_point);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  PolygonChangeApplicator implementation

PolygonChangeApplicator::PolygonChangeApplicator (const db::Polygon &poly, const db::Polygon &org_poly)
  : m_poly (poly), m_org_poly (org_poly)
{
  //  .. nothing yet ..
}

db::Shape PolygonChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool relative) const
{
  db::Polygon org_poly;
  shape.polygon (org_poly);

  if (relative) {

    db::Polygon new_poly = m_poly.moved (org_poly.box ().p1 () - m_poly.box ().p1 ());

    if (new_poly != org_poly) {
      //  shape changed - replace the old by the new one
      return shapes.replace (shape, new_poly);
    } else {
      //  shape did not change
      return shape;
    }

  } else if (m_poly != org_poly) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, m_poly);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  TextOrientationChangeApplicator implementation

TextOrientationChangeApplicator::TextOrientationChangeApplicator (const db::FTrans &trans)
  : ChangeApplicator (), m_trans (trans)
{
  //  .. nothing yet ..
}

db::Shape TextOrientationChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Text org_text;
  shape.text (org_text);

  db::Text new_text = org_text;
  new_text.trans (db::Trans (org_text.trans ().disp ()) * db::Trans (m_trans));

  if (new_text != org_text) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_text);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  TextPositionChangeApplicator implementation

TextPositionChangeApplicator::TextPositionChangeApplicator (const db::Vector &disp, const db::Vector &org_disp)
  : ChangeApplicator (), m_disp (disp), m_org_disp (org_disp)
{
  //  .. nothing yet ..
}

db::Shape TextPositionChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool relative) const
{
  db::Text org_text;
  shape.text (org_text);

  db::Text new_text = org_text;
  if (relative) {
    new_text.trans (db::Trans (m_disp - m_org_disp) * org_text.trans ());
  } else {
    db::Vector np = org_text.trans ().disp ();
    if (m_disp.x () != m_org_disp.x ()) {
      np.set_x (m_disp.x ());
    }
    if (m_disp.y () != m_org_disp.y ()) {
      np.set_y (m_disp.y ());
    }
    new_text.trans (db::Trans (np - org_text.trans ().disp ()) * org_text.trans ());
  }

  if (new_text != org_text) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_text);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  TextHAlignChangeApplicator implementation

TextHAlignChangeApplicator::TextHAlignChangeApplicator (db::HAlign halign)
  : ChangeApplicator (), m_halign (halign)
{
  //  .. nothing yet ..
}

db::Shape TextHAlignChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Text org_text;
  shape.text (org_text);

  db::Text new_text = org_text;
  new_text.halign (m_halign);

  if (new_text != org_text) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_text);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  TextVAlignChangeApplicator implementation

TextVAlignChangeApplicator::TextVAlignChangeApplicator (db::VAlign valign)
  : ChangeApplicator (), m_valign (valign)
{
  //  .. nothing yet ..
}

db::Shape TextVAlignChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Text org_text;
  shape.text (org_text);

  db::Text new_text = org_text;
  new_text.valign (m_valign);

  if (new_text != org_text) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_text);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  TextSizeChangeApplicator implementation

TextSizeChangeApplicator::TextSizeChangeApplicator (db::Coord size)
  : ChangeApplicator (), m_size (size)
{
  //  .. nothing yet ..
}

db::Shape TextSizeChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Text org_text;
  shape.text (org_text);

  db::Text new_text = org_text;
  new_text.size (m_size);

  if (new_text != org_text) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_text);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  TextStringChangeApplicatorimplementation

TextStringChangeApplicator::TextStringChangeApplicator (const std::string &string)
  : ChangeApplicator (), m_string (string)
{
  //  .. nothing yet ..
}

db::Shape TextStringChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Text org_text;
  shape.text (org_text);

  db::Text new_text = org_text;
  new_text.string (m_string);

  if (new_text != org_text) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_text);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  PathPointsChangeApplicator implementation

PathPointsChangeApplicator::PathPointsChangeApplicator (const std::vector<db::Point> &points, const std::vector<db::Point> &org_points)
  : ChangeApplicator (), m_points (points), m_org_points (org_points)
{
  //  .. nothing yet ..
}

db::Shape PathPointsChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool relative) const
{
  db::Path org_path;
  shape.path (org_path);

  db::Path new_path = org_path;
  if (relative && ! m_org_points.empty () && new_path.begin () != new_path.end ()) {
    std::vector<db::Point> new_points = m_points;
    for (std::vector<db::Point>::iterator p = new_points.begin (); p != new_points.end (); ++p) {
      *p += *new_path.begin () - m_org_points.front ();
    }
    new_path.assign (new_points.begin (), new_points.end ());
  } else {
    new_path.assign (m_points.begin (), m_points.end ());
  }

  if (new_path != org_path) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_path);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  PathWidthChangeApplicator implementation

PathWidthChangeApplicator::PathWidthChangeApplicator (db::Coord w, db::Coord org_w)
  : ChangeApplicator (), m_width (w), m_org_width (org_w)
{
  //  .. nothing yet ..
}

db::Shape PathWidthChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool relative) const
{
  db::Path org_path;
  shape.path (org_path);

  db::Path new_path = org_path;
  if (relative) {
    new_path.width (new_path.width () + m_width - m_org_width);
  } else {
    new_path.width (m_width);
  }

  //  Adjust extensions if equal to half width if that was the case before
  if (org_path.bgn_ext () == org_path.width () / 2) {
    new_path.bgn_ext (new_path.width () / 2);
  }
  if (org_path.end_ext () == org_path.width () / 2) {
    new_path.end_ext (new_path.width () / 2);
  }

  if (new_path != org_path) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_path);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  PathStartExtensionChangeApplicator implementation

PathStartExtensionChangeApplicator::PathStartExtensionChangeApplicator (db::Coord e)
  : ChangeApplicator (), m_ext (e)
{
  //  .. nothing yet ..
}

db::Shape PathStartExtensionChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Path org_path;
  shape.path (org_path);

  db::Path new_path = org_path;
  if (m_ext == std::numeric_limits <db::Coord>::min ()) {
    new_path.bgn_ext (new_path.width () / 2);
  } else {
    new_path.bgn_ext (m_ext);
  }

  if (new_path != org_path) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_path);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  PathEndExtensionChangeApplicator implementation

PathEndExtensionChangeApplicator::PathEndExtensionChangeApplicator (db::Coord e)
  : ChangeApplicator (), m_ext (e)
{
  //  .. nothing yet ..
}

db::Shape PathEndExtensionChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Path org_path;
  shape.path (org_path);

  db::Path new_path = org_path;
  if (m_ext == std::numeric_limits <db::Coord>::min ()) {
    new_path.end_ext (new_path.width () / 2);
  } else {
    new_path.end_ext (m_ext);
  }

  if (new_path != org_path) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_path);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  PathRoundEndChangeApplicator implementation

PathRoundEndChangeApplicator::PathRoundEndChangeApplicator (bool r)
  : ChangeApplicator (), m_round (r)
{
  //  .. nothing yet ..
}

db::Shape PathRoundEndChangeApplicator::do_apply (db::Shapes &shapes, const db::Shape &shape, double /*dbu*/, bool /*relative*/) const
{
  db::Path org_path;
  shape.path (org_path);

  db::Path new_path = org_path;
  new_path.round (m_round);

  if (new_path != org_path) {
    //  shape changed - replace the old by the new one
    return shapes.replace (shape, new_path);
  } else {
    //  shape did not change
    return shape;
  }
}

// -------------------------------------------------------------------------
//  ChangeTargetCellApplicator implementation

ChangeTargetCellApplicator::ChangeTargetCellApplicator (db::cell_index_type cell_index)
  : m_cell_index (cell_index)
{
  //  .. nothing yet ..
}

db::Instance ChangeTargetCellApplicator::do_apply_inst (db::Cell &cell, const db::Instance &instance, double /*dbu*/, bool /*relative*/) const 
{
  tl_assert (cell.layout ());

  //  detect recursions in the hierarchy
  std::set<db::cell_index_type> called;
  cell.layout ()->cell (m_cell_index).collect_called_cells (called);
  if (m_cell_index == cell.cell_index () || called.find (cell.cell_index ()) != called.end ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Trying to build a recursive hierarchy")).c_str ());
  }

  db::CellInstArray arr = instance.cell_inst ();
  arr.object ().cell_index (m_cell_index);
  return cell.replace (instance, arr);
}

// -------------------------------------------------------------------------
//  ChangeTargetPCellApplicator implementation

ChangeTargetPCellApplicator::ChangeTargetPCellApplicator (db::pcell_id_type pcell_id, bool apply_new_id, db::Library *new_lib, bool apply_new_lib, const std::map<std::string, tl::Variant> &modified_parameters)
  : m_pcell_id (pcell_id), m_apply_new_id (apply_new_id), mp_new_lib (new_lib), m_apply_new_lib (apply_new_lib), m_modified_parameters (modified_parameters)
{
  //  .. nothing yet ..
}

db::Instance
ChangeTargetPCellApplicator::do_apply_inst (db::Cell &cell, const db::Instance &instance, double /*dbu*/, bool /*relative*/) const
{
  tl_assert (cell.layout ());

  db::Layout *layout = cell.layout ();

  std::pair<bool, db::pcell_id_type> pci = layout->is_pcell_instance (instance.cell_index ());
  std::pair<bool, db::cell_index_type> ci (false, 0);

  db::Library *lib = layout->defining_library (instance.cell_index ()).first;

  std::map<std::string, tl::Variant> named_parameters;
  if (pci.first) {
    named_parameters = layout->get_named_pcell_parameters (instance.cell_index ());
  }
  for (std::map<std::string, tl::Variant>::const_iterator p = m_modified_parameters.begin (); p != m_modified_parameters.end (); ++p) {
    named_parameters [p->first] = p->second;
  }

  if ((m_apply_new_lib && lib != mp_new_lib) || (m_apply_new_id && (lib != mp_new_lib || ! pci.first || pci.second != m_pcell_id))) {

    if (m_apply_new_id) {

      lib = mp_new_lib;
      pci.first = true;
      pci.second = m_pcell_id;

    } else if (m_apply_new_lib) {

      if (! pci.first) {
        std::string cell_name = (lib ? &lib->layout () : layout)->cell_name (instance.cell_index ());
        ci = (mp_new_lib ? &mp_new_lib->layout () : layout)->cell_by_name (cell_name.c_str ());
      } else {
        std::string pcell_name = (lib ? &lib->layout () : layout)->pcell_declaration (pci.second)->name ();
        pci = (mp_new_lib ? &mp_new_lib->layout () : layout)->pcell_by_name (pcell_name.c_str ());
      }

      lib = mp_new_lib;

    }

  }

  db::CellInstArray arr = instance.cell_inst ();
  db::cell_index_type inst_cell_index = arr.object ().cell_index ();

  if (ci.first || pci.first) {

    //  instantiates the PCell
    if (pci.first) {
      inst_cell_index = (lib ? &lib->layout () : layout)->get_pcell_variant_dict (pci.second, named_parameters);
    } else {
      inst_cell_index = ci.second;
    }

    //  references the library
    if (lib) {
      inst_cell_index = layout->get_lib_proxy (lib, inst_cell_index);
    }

  }

  if (arr.object ().cell_index () != inst_cell_index) {
    arr.object ().cell_index (inst_cell_index);
    return cell.replace (instance, arr);
  } else {
    return instance;
  }
}

// -------------------------------------------------------------------------
//  ChangeInstanceTransApplicator implementation

ChangeInstanceTransApplicator::ChangeInstanceTransApplicator (double a, double org_a, bool mirror, bool org_mirror, double m, double org_m, const db::DVector &disp, const db::DVector &org_disp)
  : m_angle (a), m_org_angle (org_a), m_mirror (mirror), m_org_mirror (org_mirror),
    m_mag (m), m_org_mag (org_m), m_disp (disp), m_org_disp (org_disp)
{
  //  .. nothing yet ..
}

db::Instance ChangeInstanceTransApplicator::do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const 
{
  db::CellInstArray::complex_trans_type tr = instance.complex_trans ();

  if (relative) {

    tr = db::CellInstArray::complex_trans_type (m_mag, m_angle, m_mirror, db::Vector (m_disp * (1.0 / dbu))) * db::CellInstArray::complex_trans_type (m_org_mag, m_org_angle, m_org_mirror, db::Vector (m_org_disp * (1.0 / dbu))).inverted () * tr;

  } else {

    db::Vector disp = tr.disp ();

    if (fabs (m_disp.x () - m_org_disp.x ()) > 1e-6) {
      disp.set_x (db::coord_traits<db::Coord>::rounded (m_disp.x () / dbu));
    }
    if (fabs (m_disp.y () - m_org_disp.y ()) > 1e-6) {
      disp.set_y (db::coord_traits<db::Coord>::rounded (m_disp.y () / dbu));
    }

    double mag = fabs (m_mag - m_org_mag) > 1e-6 ? m_mag : tr.mag ();
    double angle = fabs (m_angle - m_org_angle) > 1e-6 ? m_angle : tr.angle ();
    bool mirror = (m_mirror != m_org_mirror) ? m_mirror : tr.is_mirror ();

    tr = db::CellInstArray::complex_trans_type (mag, angle, mirror, disp);

  }

  bool is_complex = (tr.is_mag () || ! tr.is_ortho ());

  db::CellInstArray new_inst;

  db::CellInstArray::vector_type a, b;
  unsigned long na = 0, nb = 0;

  if (instance.is_regular_array (a, b, na, nb)) {

    if (is_complex) {
      new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), tr, a, b, na, nb);
    } else {
      new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), db::Trans (tr.rot (), tr.disp ()), a, b, na, nb);
    }

  } else {

    if (is_complex) {
      new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), tr);
    } else {
      new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), db::Trans (tr.rot (), tr.disp ()));
    }

  }

  if (new_inst != instance.cell_inst ()) {
    return cell.replace (instance, new_inst);
  } else {
    return instance;
  }
}

// -------------------------------------------------------------------------
//  ChangeInstanceArrayApplicator implementation

ChangeInstanceArrayApplicator::ChangeInstanceArrayApplicator (const db::DVector &a, bool set_a, const db::DVector &b, bool set_b, unsigned long na, bool set_na, unsigned long nb, bool set_nb)
  : m_a (a), m_set_a (set_a), m_b (b), m_set_b (set_b),
    m_na (na), m_set_na (set_na), m_nb (nb), m_set_nb (set_nb)
{
  //  .. nothing yet ..
}

db::Instance ChangeInstanceArrayApplicator::do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool /*relative*/) const 
{
  db::CellInstArray new_inst;

  db::CellInstArray::vector_type a, b;
  unsigned long na = 0, nb = 0;
  if (! instance.is_regular_array (a, b, na, nb)) {
    na = nb = 1;
    a = db::Vector (m_a * (1.0 / dbu));
    b = db::Vector (m_b * (1.0 / dbu));
  }

  if (m_set_a) {
    a = db::Vector (m_a * (1.0 / dbu));
  }
  if (m_set_na) {
    na = m_na;
  }
  if (m_set_b) {
    b = db::Vector (m_b * (1.0 / dbu));
  }
  if (m_set_nb) {
    nb = m_nb;
  }

  if (instance.is_complex ()) {
    new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), instance.complex_trans (), a, b, na, nb);
  } else {
    new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), instance.front (), a, b, na, nb);
  }

  if (new_inst != instance.cell_inst ()) {
    return cell.replace (instance, new_inst);
  } else {
    return instance;
  }
}

// -------------------------------------------------------------------------
//  InstanceRemoveArrayApplicator implementation

InstanceRemoveArrayApplicator::InstanceRemoveArrayApplicator ()
{
  //  .. nothing yet ..
}

db::Instance InstanceRemoveArrayApplicator::do_apply_inst (db::Cell &cell, const db::Instance &instance, double /*dbu*/, bool /*relative*/) const 
{
  db::CellInstArray new_inst;
  if (instance.is_complex ()) {
    new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), instance.complex_trans ());
  } else {
    new_inst = db::CellInstArray (db::CellInst (instance.cell_index ()), instance.front ());
  }

  if (new_inst != instance.cell_inst ()) {
    return cell.replace (instance, new_inst);
  } else {
    return instance;
  }
}

// -------------------------------------------------------------------------
//  helper functions to convert coordinates

std::string
coord_to_string (double dc, double dbu, bool du)
{
  if (du) {
    return tl::db_to_string (dc);
  } else {
    return tl::micron_to_string (dc * dbu);
  }
}

db::DCoord
dcoord_from_dcoord (double d, double dbu, bool du, const db::DCplxTrans &t)
{
  db::DCoord dc = t.ctrans (d * (du ? 1.0 : 1.0 / dbu));
  return db::Coord (floor (dc + 0.5)) * dbu;
}

db::Coord
coord_from_dcoord (double d, double dbu, bool du, const db::VCplxTrans &t)
{
  return t.ctrans (d * (du ? 1.0 : 1.0 / dbu));
}

db::DPoint
dpoint_from_dpoint (const db::DPoint &dp, double dbu, bool du, const db::DCplxTrans &t)
{
  return (t * (dp * (du ? 1.0 : 1.0 / dbu))) * dbu;
}

db::DVector
dvector_from_dvector (const db::DVector &dp, double dbu, bool du, const db::DCplxTrans &t)
{
  return (t * (dp * (du ? 1.0 : 1.0 / dbu))) * dbu;
}

db::Point
point_from_dpoint (const db::DPoint &dp, double dbu, bool du, const db::VCplxTrans &t)
{
  return t * (dp * (du ? 1.0 : 1.0 / dbu));
}

db::DCoord
dcoord_from_string (const char *txt, double dbu, bool du, const db::DCplxTrans &t)
{
  double d = 0.0;
  tl::from_string_ext (txt, d);
  return dcoord_from_dcoord (d, dbu, du, t);
}

db::Coord
coord_from_string (const char *txt, double dbu, bool du, const db::VCplxTrans &t)
{
  double d = 0.0;
  tl::from_string_ext (txt, d);
  return coord_from_dcoord (d, dbu, du, t);
}

std::string
coords_to_string (const db::DPoint &dp, double dbu, bool du, const char *sep)
{
  return coord_to_string (dp.x (), dbu, du) + sep + coord_to_string (dp.y (), dbu, du);
}

}

#endif
