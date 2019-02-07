
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

#ifndef HDR_dbCellVariants
#define HDR_dbCellVariants

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbTrans.h"
#include "tlTypeTraits.h"

namespace db
{

/**
 *  @brief Tells some properties of a reduce function for cell_variants_builder
 */
template <class T>
struct DB_PUBLIC cell_variants_reduce_traits
{
  /**
   *  @brief Indicates whether the result of the compare function does not depend on translation
   */
  typedef typename T::is_translation_invariant is_translation_invariant;
};

/**
 *  @brief An orientation reducer
 *
 *  This reducer incarnation reduces the transformation to it's rotation/mirror part.
 */
struct DB_PUBLIC OrientationReducer
{
  typedef tl::true_tag is_translation_invariant;

  db::ICplxTrans operator () (const db::ICplxTrans &trans) const
  {
    db::ICplxTrans res (trans);
    res.disp (db::Vector ());
    res.mag (1.0);
    return res;
  }

  db::Trans operator () (const db::Trans &trans) const
  {
    return db::Trans (trans.fp_trans ());
  }
};

/**
 *  @brief A magnification reducer
 *
 *  This reducer incarnation reduces the transformation to it's scaling part.
 */
struct DB_PUBLIC MagnificationReducer
{
  typedef tl::true_tag is_translation_invariant;

  db::ICplxTrans operator () (const db::ICplxTrans &trans) const
  {
    return db::ICplxTrans (trans.mag ());
  }

  db::Trans operator () (const db::Trans &) const
  {
    return db::Trans ();
  }
};

/**
 *  @brief A grid reducer
 *
 *  This reducer incarnation reduces the transformation to it's displacement modulo a grid
 */
struct DB_PUBLIC GridReducer
{
  typedef tl::false_tag is_translation_invariant;

  GridReducer (db::Coord grid)
    : m_grid (grid)
  {
    //  .. nothing yet ..
  }

  db::ICplxTrans operator () (const db::ICplxTrans &trans) const
  {
    return db::ICplxTrans (db::Vector (trans.disp ().x () % m_grid, trans.disp ().y () % m_grid));
  }

  db::Trans operator () (const db::Trans &trans) const
  {
    return db::Trans (db::Vector (trans.disp ().x () % m_grid, trans.disp ().y () % m_grid));
  }

private:
  db::Coord m_grid;
};

/**
 *  @brief A class computing variants for cells according to a given criterion
 *
 *  The cell variants are build from the cell instances and are accumulated over
 *  the hierarchy path.
 *
 *  The criterion is a reduction function which defines the core information of a
 *  db::ICplxTrans or db::Trans object to be used for the comparison.
 *
 *  In any case, the compare function is expected to be distributive similar to
 *  the modulo function (A, B are transformations)
 *
 *    RED(A*B) = RED(RED(A)*RED(B))
 *
 */
template <class Reduce>
class DB_PUBLIC cell_variants_builder
{
public:
  typedef cell_variants_reduce_traits<Reduce> compare_traits;

  /**
   *  @brief Creates a variant extractor
   */
  cell_variants_builder (const Reduce &red)
    : m_red (red)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Builds cell variants for the given layout starting from the top cell
   */
  void build (db::Layout &layout, db::Cell &top_cell)
  {
    //  The top cell gets a single "variant" with unit transformation
    m_variants [top_cell.cell_index ()].insert (db::ICplxTrans ());

    std::set<db::cell_index_type> called;
    top_cell.collect_called_cells (called);

    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {

      if (called.find (*c) == called.end ()) {
        continue;
      }

      //  collect the parent variants per parent cell

      std::map<db::cell_index_type, std::set<db::ICplxTrans> > variants_per_parent_cell;
      for (db::Cell::parent_inst_iterator pi = layout.cell (*c).begin_parent_insts (); ! pi.at_end (); ++pi) {
        std::set<db::ICplxTrans> &variants = variants_per_parent_cell [pi->inst ().object ().cell_index ()];
        add_variant (variants, pi->child_inst ().cell_inst (), typename compare_traits::is_translation_invariant ());
      }

      //  compute the resulting variants

      std::set<db::ICplxTrans> &new_variants = m_variants [*c];

      for (std::map<db::cell_index_type, std::set<db::ICplxTrans> >::const_iterator pv = variants_per_parent_cell.begin (); pv != variants_per_parent_cell.end (); ++pv) {
        product (variants (pv->first), pv->second, new_variants);
      }

    }
  }

  /**
   *  @brief Gets the variants for a given cell
   */
  const std::set<db::ICplxTrans> &variants (db::cell_index_type ci) const
  {
    std::map<db::cell_index_type, std::set<db::ICplxTrans> >::const_iterator v = m_variants.find (ci);
    static std::set<db::ICplxTrans> empty_set;
    if (v == m_variants.end ()) {
      return empty_set;
    } else {
      return v->second;
    }
  }

private:
  std::map<db::cell_index_type, std::set<db::ICplxTrans> > m_variants;
  Reduce m_red;

  void add_variant (std::set<db::ICplxTrans> &variants, const db::CellInstArray &inst, tl::false_tag) const
  {
    if (inst.is_complex ()) {
      for (db::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
        variants.insert (m_red (inst.complex_trans (*i)));
      }
    } else {
      for (db::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
        variants.insert (db::ICplxTrans (m_red (*i)));
      }
    }
  }

  void add_variant (std::set<db::ICplxTrans> &variants, const db::CellInstArray &inst, tl::true_tag) const
  {
    if (inst.is_complex ()) {
      variants.insert (m_red (inst.complex_trans ()));
    } else {
      variants.insert (db::ICplxTrans (m_red (inst.front ())));
    }
  }

  void product (const std::set<db::ICplxTrans> &v1, const std::set<db::ICplxTrans> &v2, std::set<db::ICplxTrans> &prod) const
  {
    for (std::set<db::ICplxTrans>::const_iterator i = v1.begin (); i != v1.end (); ++i) {
      for (std::set<db::ICplxTrans>::const_iterator j = v2.begin (); j != v2.end (); ++j) {
        prod.insert (m_red (*i * *j));
      }
    }
  }
};

}  // namespace db

#endif

