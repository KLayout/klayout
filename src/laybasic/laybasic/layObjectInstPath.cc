
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


#include "dbBox.h"
#include "dbLayout.h"

#include "layObjectInstPath.h"
#include "layCellView.h"
#include "layLayoutViewBase.h"
#include "tlException.h"

namespace lay {

// ----------------------------------------------------------------------
//  ObjectInstPath implementation

ObjectInstPath::ObjectInstPath ()
  : m_cv_index (0), m_topcell (0), m_layer (-1), m_seq (0)
{
  //  .. nothing yet ..
}

bool
ObjectInstPath::is_valid (lay::LayoutViewBase *view) const
{
  const lay::CellView &cv = view->cellview (cv_index ());
  if (! cv.is_valid ()) {
    return false;
  }

  const db::Layout &ly = cv->layout ();
  db::cell_index_type ci = topcell ();
  if (! ly.is_valid_cell_index (ci)) {
    return false;
  }

  for (auto p = begin (); p != end (); ++p) {
    if (! ly.cell (ci).is_valid (p->inst_ptr)) {
      return false;
    }
    ci = p->inst_ptr.cell_index ();
    if (! ly.is_valid_cell_index (ci)) {
      return false;
    }
  }

  if (! is_cell_inst ()) {
    if (! ly.is_valid_layer (layer ()) && layer () != ly.guiding_shape_layer ()) {
      return false;
    }
    if (! ly.cell (ci).shapes (layer ()).is_valid (shape ())) {
      return false;
    }
  }

  return true;
}

db::cell_index_type 
ObjectInstPath::cell_index_tot () const
{
  if (m_path.empty ()) {
    return m_topcell;
  } else {
    return m_path.back ().inst_ptr.cell_index ();
  }
}

db::ICplxTrans
ObjectInstPath::trans_tot () const
{
  db::ICplxTrans t;
  iterator end = m_path.end ();
  for (iterator i = m_path.begin (); i != end; ++i) {
    t = t * i->complex_trans ();
  }
  return t;
}

db::cell_index_type 
ObjectInstPath::cell_index () const
{
  if (m_path.empty ()) {
    return m_topcell;
  } else if (! is_cell_inst ()) {
    return m_path.back ().inst_ptr.cell_index ();
  } else {
    iterator end = m_path.end ();
    --end;
    if (end == m_path.begin ()) {
      return m_topcell;
    } else {
      --end;
      return end->inst_ptr.cell_index ();
    }
  }
}

db::ICplxTrans
ObjectInstPath::trans () const
{
  db::ICplxTrans t;
  //  For instances, the last element is the instance itself - do not count that in the transformation.
  iterator end = m_path.end ();
  if (is_cell_inst () && end != m_path.begin ()) {
    --end;
  }
  for (iterator i = m_path.begin (); i != end; ++i) {
    t = t * i->complex_trans ();
  }
  return t;
}

void 
ObjectInstPath::remove_front (unsigned int n)
{
  while (n > 0) {
    --n;
    tl_assert (! m_path.empty ());
    if (n == 0) {
      m_topcell = m_path.front ().inst_ptr.cell_index ();
    }
    m_path.erase (m_path.begin ());
  }
}

void 
ObjectInstPath::insert_front (db::cell_index_type topcell, const db::InstElement &elem)
{
  tl_assert (m_topcell == elem.inst_ptr.cell_index ());
  m_topcell = topcell;
  m_path.insert (m_path.begin (), elem);
}

bool 
ObjectInstPath::operator< (const ObjectInstPath &d) const
{
  if (is_cell_inst () != d.is_cell_inst ()) {
    return is_cell_inst () < d.is_cell_inst ();
  }
  if (! is_cell_inst ()) {
    if (m_layer != d.m_layer) {
      return m_layer < d.m_layer;
    }
    if (m_shape != d.m_shape) {
      return m_shape < d.m_shape;
    }
  }
  if (m_cv_index != d.m_cv_index) {
    return m_cv_index < d.m_cv_index;
  }
  if (m_topcell != d.m_topcell) {
    return m_topcell < d.m_topcell;
  }
  return m_path < d.m_path;
}

bool 
ObjectInstPath::operator== (const ObjectInstPath &d) const
{
  if (is_cell_inst () != d.is_cell_inst ()) {
    return false;
  }
  if (! is_cell_inst ()) {
    if (m_layer != d.m_layer) {
      return false;
    } 
    if (m_shape != d.m_shape) {
      return false;
    }
  }
  if (m_cv_index != d.m_cv_index) {
    return false;
  }
  if (m_topcell != d.m_topcell) {
    return false;
  }
  return m_path == d.m_path;
}

}


