
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
    //  NOTE: we need to keep magnification, angle and mirror so when combining the
    //  reduced transformations, the result will be equivalent to reducing the combined
    //  transformation.
    db::ICplxTrans res (trans);
    res.disp (db::Vector (mod (trans.disp ().x ()), mod (trans.disp ().y ())));
    return res;
  }

  db::Trans operator () (const db::Trans &trans) const
  {
    db::Trans res (trans);
    res.disp (db::Vector (mod (trans.disp ().x ()), mod (trans.disp ().y ())));
    return res;
  }

private:
  db::Coord m_grid;

  inline db::Coord mod (db::Coord c) const
  {
    if (c < 0) {
      return m_grid - (-c) % m_grid;
    } else {
      return c % m_grid;
    }
  }
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
class DB_PUBLIC cell_variants_collector
{
public:
  typedef cell_variants_reduce_traits<Reduce> compare_traits;

  /**
   *  @brief Creates a variant extractor
   */
  cell_variants_collector (const Reduce &red)
    : m_red (red)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Collects cell variants for the given layout starting from the top cell
   */
  void collect (const db::Layout &layout, const db::Cell &top_cell)
  {
    //  The top cell gets a "variant" with unit transformation
    m_variants [top_cell.cell_index ()].insert (std::make_pair (db::ICplxTrans (), 1));

    std::set<db::cell_index_type> called;
    top_cell.collect_called_cells (called);

    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {

      if (called.find (*c) == called.end ()) {
        continue;
      }

      //  collect the parent variants per parent cell

      std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> > variants_per_parent_cell;
      for (db::Cell::parent_inst_iterator pi = layout.cell (*c).begin_parent_insts (); ! pi.at_end (); ++pi) {
        std::map<db::ICplxTrans, size_t> &variants = variants_per_parent_cell [pi->inst ().object ().cell_index ()];
        add_variant (variants, pi->child_inst ().cell_inst (), typename compare_traits::is_translation_invariant ());
      }

      //  compute the resulting variants

      std::map<db::ICplxTrans, size_t> &new_variants = m_variants [*c];

      for (std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> >::const_iterator pv = variants_per_parent_cell.begin (); pv != variants_per_parent_cell.end (); ++pv) {
        product (variants (pv->first), pv->second, new_variants);
      }

    }
  }

  /**
   *  @brief Creates cell variants for singularization of the different variants
   *
   *  After this method can been used, all cells with more than one variant are separated and
   *  the corresponding instances are updated.
   *
   *  If given, *var_table will be filled with a map giving the new cell and variant against
   *  the old cell for all cells with more than one variant.
   */
  void separate_variants (db::Layout &layout, db::Cell &top_cell, std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > *var_table = 0)
  {
    db::LayoutLocker locker (&layout);

    std::set<db::cell_index_type> called;
    top_cell.collect_called_cells (called);
    called.insert (top_cell.cell_index ());

    //  create new cells for the variants

    std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > var_table_intern;
    if (! var_table) {
      var_table = &var_table_intern;
    }

    for (db::Layout::bottom_up_const_iterator c = layout.begin_bottom_up (); c != layout.end_bottom_up (); ++c) {

      if (called.find (*c) == called.end ()) {
        continue;
      }

      db::Cell &cell = layout.cell (*c);

      std::map<db::ICplxTrans, size_t> &vv = m_variants [*c];
      if (vv.size () > 1) {

        std::map<db::ICplxTrans, db::cell_index_type> &vt = (*var_table) [*c];

        std::vector<db::CellInstArrayWithProperties> inst;
        inst.reserve (cell.cell_instances ());

        for (db::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
          inst.push_back (db::CellInstArrayWithProperties (i->cell_inst (), i->prop_id ()));
        }

        cell.clear_insts ();

        int index = 0;
        for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v, ++index) {

          db::cell_index_type ci_var;

          if (v != vv.begin ()) {

            std::string var_name = layout.cell_name (*c);
            var_name += "$VAR" + tl::to_string (index);

            ci_var = layout.add_cell (var_name.c_str ());
            copy_shapes (layout, ci_var, *c);

            //  a new entry for the variant
            m_variants [ci_var].insert (*v);

          } else {
            ci_var = *c;
          }

          vt.insert (std::make_pair (v->first, ci_var));
          create_var_instances (layout.cell (ci_var), inst, v->first, *var_table, typename compare_traits::is_translation_invariant ());

        }

        //  correct the first (remaining) entry
        std::pair<db::ICplxTrans, size_t> v1 = *vt.begin ();
        vv.clear ();
        vv.insert (v1);

      } else {

        //  if the children of this cell are separated, map the instances to the new variants
        bool needs_update = false;
        for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); ! cc.at_end () && ! needs_update; ++cc) {
          if (var_table->find (*cc) != var_table->end ()) {
            needs_update = true;
          }
        }

        if (needs_update) {

          std::vector<db::CellInstArrayWithProperties> inst;
          inst.reserve (cell.cell_instances ());

          for (db::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
            inst.push_back (db::CellInstArrayWithProperties (i->cell_inst (), i->prop_id ()));
          }

          cell.clear_insts ();
          create_var_instances (cell, inst, vv.begin ()->first, *var_table, typename compare_traits::is_translation_invariant ());

        }

      }

    }
  }

  /**
   *  @brief Gets the variants for a given cell
   *
   *  The keys of the map are the variants, the values is the instance count of the variant
   *  (as seen from the top cell).
   */
  const std::map<db::ICplxTrans, size_t> &variants (db::cell_index_type ci) const
  {
    std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> >::const_iterator v = m_variants.find (ci);
    static std::map<db::ICplxTrans, size_t> empty_set;
    if (v == m_variants.end ()) {
      return empty_set;
    } else {
      return v->second;
    }
  }

private:
  std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> > m_variants;
  Reduce m_red;

  void add_variant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst, tl::false_tag) const
  {
    if (inst.is_complex ()) {
      for (db::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
        variants [m_red (inst.complex_trans (*i))] += 1;
      }
    } else {
      for (db::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
        variants [db::ICplxTrans (m_red (*i))] += 1;
      }
    }
  }

  void add_variant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst, tl::true_tag) const
  {
    if (inst.is_complex ()) {
      variants [m_red (inst.complex_trans ())] += inst.size ();
    } else {
      variants [db::ICplxTrans (m_red (inst.front ()))] += inst.size ();
    }
  }

  void product (const std::map<db::ICplxTrans, size_t> &v1, const std::map<db::ICplxTrans, size_t> &v2, std::map<db::ICplxTrans, size_t> &prod) const
  {
    for (std::map<db::ICplxTrans, size_t>::const_iterator i = v1.begin (); i != v1.end (); ++i) {
      for (std::map<db::ICplxTrans, size_t>::const_iterator j = v2.begin (); j != v2.end (); ++j) {
        prod [m_red (i->first * j->first)] += i->second * j->second;
      }
    }
  }

  void copy_shapes (db::Layout &layout, db::cell_index_type ci_to, db::cell_index_type ci_from) const
  {
    db::Cell &to = layout.cell (ci_to);
    const db::Cell &from = layout.cell (ci_from);
    for (db::Layout::layer_iterator li = layout.begin_layers (); li != layout.end_layers (); ++li) {
      to.shapes ((*li).first) = from.shapes ((*li).first);
    }
  }

  void create_var_instances (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table, tl::false_tag) const
  {
    for (std::vector<db::CellInstArrayWithProperties>::const_iterator i = inst.begin (); i != inst.end (); ++i) {

      std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> >::const_iterator f = var_table.find (i->object ().cell_index ());
      if (f == var_table.end ()) {

         in_cell.insert (*i);

      } else {

        const std::map<db::ICplxTrans, db::cell_index_type> &vt = f->second;

        for (db::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {

          db::ICplxTrans rt = m_red (for_var * i->complex_trans (*ia));
          std::map<db::ICplxTrans, db::cell_index_type>::const_iterator v = vt.find (rt);
          tl_assert (v != vt.end ());

          in_cell.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (v->second), i->complex_trans (*ia)), i->properties_id ()));

        }

      }

    }
  }

  void create_var_instances (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table, tl::true_tag) const
  {
    for (std::vector<db::CellInstArrayWithProperties>::const_iterator i = inst.begin (); i != inst.end (); ++i) {

      std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> >::const_iterator f = var_table.find (i->object ().cell_index ());
      if (f == var_table.end ()) {

         in_cell.insert (*i);

      } else {

        const std::map<db::ICplxTrans, db::cell_index_type> &vt = f->second;

        std::map<db::ICplxTrans, db::cell_index_type>::const_iterator v;

        db::ICplxTrans rt = m_red (for_var * i->complex_trans ());
        v = vt.find (rt);
        tl_assert (v != vt.end ());

        db::CellInstArrayWithProperties new_inst = *i;
        new_inst.object ().cell_index (v->second);
        in_cell.insert (new_inst);

      }

    }
  }
};

}  // namespace db

#endif

