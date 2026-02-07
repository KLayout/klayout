
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "dbCellInstanceSetHasher.h"
#include "dbHash.h"

namespace db
{

CellInstanceSetHasher::MatrixHash::MatrixHash (double s)
  : db::IMatrix3d (s, 0, 0, 0, s, 0, 0, 0, s)
{
  //  .. nothing yet ..
}

CellInstanceSetHasher::MatrixHash::MatrixHash (const db::ICplxTrans &trans)
  : db::IMatrix3d (trans)
{
  //  .. nothing yet ..
}

CellInstanceSetHasher::MatrixHash::MatrixHash (const db::CellInstArray &array)
  : db::IMatrix3d (array.complex_trans ())
{
  db::Vector a, b;
  unsigned long na = 0, nb = 0;

  if (array.is_regular_array (a, b, na, nb)) {

    na = std::max ((unsigned long) 1, na);
    nb = std::max ((unsigned long) 1, nb);

    //  compute the sum of all individual matrices
    *this *= double (na * nb);

    db::DVector dab = db::DVector (a) * double ((nb * (na - 1) * na) / 2) + db::DVector (b) * double ((na * (nb - 1) * nb) / 2);
    m() [0][2] += dab.x ();
    m() [1][2] += dab.y ();

  } else if (array.is_iterated_array ()) {

    db::DVector dab;
    double n = 0.0;

    tl_assert (! array.begin ().at_end ());
    db::DVector d0 = db::DVector ((*array.begin ()).disp ());
    for (auto i = array.begin (); ! i.at_end (); ++i) {
      n += 1.0;
      dab += db::DVector ((*i).disp ()) - d0;
    }

    *this *= n;

    m() [0][2] += dab.x ();
    m() [1][2] += dab.y ();

  }
}

static inline size_t d2h (double d)
{
  return d < 0 ? size_t (d - 0.5) : size_t (d + 0.5);
}

size_t
CellInstanceSetHasher::MatrixHash::hash_value () const
{
  //  The "close-to-unity" elements are scaled with this value, so
  //  after rounding to int for the hash value we are able to
  //  resolve a certain level of details. This applies to the
  //  rotation/shear/scale submatrix elements (m11, m12, m21, m22).
  const double res = 1024.0;

  size_t h =        d2h (m ()[0][0] * res);
  h = tl::hcombine (d2h (m ()[0][1] * res), h);
  h = tl::hcombine (d2h (m ()[0][2]), h);
  h = tl::hcombine (d2h (m ()[1][0] * res), h);
  h = tl::hcombine (d2h (m ()[1][1] * res), h);
  h = tl::hcombine (d2h (m ()[1][2]), h);
  //  m31 and m32 are always zero, so we don't count them here
  h = tl::hcombine (d2h (m ()[2][2]), h);
  return h;
}

CellInstanceSetHasher::CellInstanceSetHasher (const db::Layout *layout, db::cell_index_type top_cell, const std::set<db::cell_index_type> *selection)
  : mp_layout (layout), m_top_cell (top_cell), mp_selection (selection)
{
  //  .. nothing yet ..
}

size_t
CellInstanceSetHasher::instance_set_hash (db::cell_index_type for_cell)
{
  return get_hash (for_cell).hash_value ();
}

CellInstanceSetHasher::MatrixHash
CellInstanceSetHasher::get_hash (cell_index_type for_cell)
{
  auto c = m_cache.find (for_cell);
  if (c != m_cache.end ()) {
    return c->second;
  } else {
    MatrixHash hm = get_hash_uncached (for_cell);
    m_cache [for_cell] = hm;
    return hm;
  }
}

CellInstanceSetHasher::MatrixHash
CellInstanceSetHasher::get_hash_uncached (cell_index_type for_cell)
{
  if (for_cell == m_top_cell) {

    return MatrixHash ();

  } else {

    const db::Cell &fc = mp_layout->cell (for_cell);

    MatrixHash hm (0.0);
    for (auto pi = fc.begin_parent_insts (); ! pi.at_end (); ++pi) {
      auto pci = pi->parent_cell_index ();
      if (! mp_selection || mp_selection->find (pci) != mp_selection->end ()) {
        hm += get_hash (pci) * MatrixHash (pi->child_inst ().cell_inst ());
      }
    }

    return hm;

  }
}

}
