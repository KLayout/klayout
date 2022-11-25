
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#include "dbCellVariants.h"
#include "dbPolygonTools.h"
#include "tlUtils.h"

namespace db
{

// ------------------------------------------------------------------------------------------

db::ICplxTrans OrientationReducer::reduce (const db::ICplxTrans &trans) const
{
  db::ICplxTrans res (trans);
  res.disp (db::Vector ());
  res.mag (1.0);
  return res;
}

db::Trans OrientationReducer::reduce (const db::Trans &trans) const
{
  return db::Trans (trans.fp_trans ());
}

// ------------------------------------------------------------------------------------------

db::ICplxTrans OrthogonalTransformationReducer::reduce (const db::ICplxTrans &trans) const
{
  if (trans.is_ortho ()) {
    return db::ICplxTrans ();
  } else {
    db::ICplxTrans res;
    double a = trans.angle ();
    double a90 = floor (a / 90.0 + 0.5 + db::epsilon) * 90.0;
    res.angle (a - a90);
    return res;
  }
}

db::Trans OrthogonalTransformationReducer::reduce (const db::Trans &) const
{
  return db::Trans ();
}

// ------------------------------------------------------------------------------------------

db::ICplxTrans MagnificationReducer::reduce (const db::ICplxTrans &trans) const
{
  return db::ICplxTrans (trans.mag ());
}

db::Trans MagnificationReducer::reduce (const db::Trans &) const
{
  return db::Trans ();
}

// ------------------------------------------------------------------------------------------

db::ICplxTrans XYAnisotropyAndMagnificationReducer::reduce (const db::ICplxTrans &trans) const
{
  double a = trans.angle ();
  if (a > 180.0 - db::epsilon) {
    a -= 180.0;
  }
  return db::ICplxTrans (trans.mag (), a, false, db::Vector ());
}

db::Trans XYAnisotropyAndMagnificationReducer::reduce (const db::Trans &trans) const
{
  return db::Trans (trans.angle () % 2, false, db::Vector ());
}

// ------------------------------------------------------------------------------------------

db::ICplxTrans MagnificationAndOrientationReducer::reduce (const db::ICplxTrans &trans) const
{
  db::ICplxTrans res (trans);
  res.disp (db::Vector ());
  return res;
}

db::Trans MagnificationAndOrientationReducer::reduce (const db::Trans &trans) const
{
  return db::Trans (trans.fp_trans ());
}

// ------------------------------------------------------------------------------------------

GridReducer::GridReducer (db::Coord grid)
  : m_grid (grid)
{
  //  .. nothing yet ..
}

db::ICplxTrans GridReducer::reduce (const db::ICplxTrans &trans) const
{
  //  NOTE: we need to keep magnification, angle and mirror so when combining the
  //  reduced transformations, the result will be equivalent to reducing the combined
  //  transformation.
  db::ICplxTrans res (trans);
  res.disp (db::Vector (trans.disp ().x () - snap_to_grid (trans.disp ().x (), m_grid), trans.disp ().y () - snap_to_grid (trans.disp ().y (), m_grid)));
  return res;
}

db::Trans GridReducer::reduce (const db::Trans &trans) const
{
  db::Trans res (trans);
  res.disp (db::Vector (trans.disp ().x () - snap_to_grid (trans.disp ().x (), m_grid), trans.disp ().y () - snap_to_grid (trans.disp ().y (), m_grid)));
  return res;
}

// ------------------------------------------------------------------------------------------

ScaleAndGridReducer::ScaleAndGridReducer (db::Coord grid, db::Coord mult, db::Coord div)
  : m_mult (mult), m_grid (int64_t (grid) * int64_t (div))
{
  //  .. nothing yet ..
}

db::ICplxTrans ScaleAndGridReducer::reduce_trans (const db::ICplxTrans &trans) const
{
  db::ICplxTrans res (trans);
  int64_t dx = int64_t (trans.disp ().x ()) * m_mult;
  int64_t dy = int64_t (trans.disp ().y ()) * m_mult;
  res.disp (db::Vector (db::Coord (dx - snap_to_grid (dx, m_grid)), db::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

db::Trans ScaleAndGridReducer::reduce_trans (const db::Trans &trans) const
{
  db::Trans res (trans);
  int64_t dx = int64_t (trans.disp ().x ()) * m_mult;
  int64_t dy = int64_t (trans.disp ().y ()) * m_mult;
  res.disp (db::Vector (db::Coord (dx - snap_to_grid (dx, m_grid)), db::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

db::ICplxTrans ScaleAndGridReducer::reduce (const db::ICplxTrans &trans) const
{
  db::ICplxTrans res (trans);
  int64_t dx = int64_t (trans.disp ().x ());
  int64_t dy = int64_t (trans.disp ().y ());
  res.disp (db::Vector (db::Coord (dx - snap_to_grid (dx, m_grid)), db::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

db::Trans ScaleAndGridReducer::reduce (const db::Trans &trans) const
{
  db::Trans res (trans);
  int64_t dx = int64_t (trans.disp ().x ());
  int64_t dy = int64_t (trans.disp ().y ());
  res.disp (db::Vector (db::Coord (dx - snap_to_grid (dx, m_grid)), db::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

// ------------------------------------------------------------------------------------------

VariantsCollectorBase::VariantsCollectorBase ()
  : mp_red ()
{
  //  .. nothing yet ..
}

VariantsCollectorBase::VariantsCollectorBase (const TransformationReducer *red)
  : mp_red (red)
{
  //  .. nothing yet ..
}

void
VariantsCollectorBase::collect (const db::Layout &layout, const db::Cell &top_cell)
{
  tl_assert (mp_red != 0);

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
      add_variant (variants, pi->child_inst ().cell_inst (), mp_red->is_translation_invariant ());
    }

    //  compute the resulting variants

    std::map<db::ICplxTrans, size_t> &new_variants = m_variants [*c];

    for (std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> >::const_iterator pv = variants_per_parent_cell.begin (); pv != variants_per_parent_cell.end (); ++pv) {
      product (variants (pv->first), pv->second, new_variants);
    }

  }
}

void
VariantsCollectorBase::separate_variants (db::Layout &layout, db::Cell &top_cell, std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > *var_table)
{
  tl_assert (mp_red != 0);

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
        create_var_instances (layout.cell (ci_var), inst, v->first, *var_table, mp_red->is_translation_invariant ());

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
        create_var_instances (cell, inst, vv.begin ()->first, *var_table, mp_red->is_translation_invariant ());

      }

    }

  }
}

void
VariantsCollectorBase::commit_shapes (db::Layout &layout, db::Cell &top_cell, unsigned int layer, std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > &to_commit)
{
  tl_assert (mp_red != 0);

  if (to_commit.empty ()) {
    return;
  }

  //  NOTE: this implementation suffers from accumulation of propagated shapes: we add more levels of propagated
  //  shapes if required. We don't clean up, because we do not know when a shape collection stops being required.

  db::LayoutLocker locker (&layout);

  std::set<db::cell_index_type> called;
  top_cell.collect_called_cells (called);
  called.insert (top_cell.cell_index ());

  for (db::Layout::bottom_up_const_iterator c = layout.begin_bottom_up (); c != layout.end_bottom_up (); ++c) {

    if (called.find (*c) == called.end ()) {
      continue;
    }

    db::Cell &cell = layout.cell (*c);

    std::map<db::ICplxTrans, size_t> &vvc = m_variants [*c];
    if (vvc.size () > 1) {

      for (std::map<db::ICplxTrans, size_t>::const_iterator vc = vvc.begin (); vc != vvc.end (); ++vc) {

        for (db::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {

          std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> >::const_iterator tc = to_commit.find (i->cell_index ());
          if (tc != to_commit.end ()) {

            const std::map<db::ICplxTrans, db::Shapes> &vt = tc->second;

            //  NOTE: this will add one more commit slot for propagation ... but we don't clean up.
            //  When would a cleanup happen?
            std::map<db::ICplxTrans, db::Shapes> &propagated = to_commit [*c];

            for (db::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {

              db::ICplxTrans t = i->complex_trans (*ia);
              db::ICplxTrans rt = mp_red->reduce (vc->first * mp_red->reduce_trans (t));
              std::map<db::ICplxTrans, db::Shapes>::const_iterator v = vt.find (rt);
              if (v != vt.end ()) {

                db::Shapes &ps = propagated [vc->first];
                tl::ident_map<db::Layout::properties_id_type> pm;

                for (db::Shapes::shape_iterator si = v->second.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
                  ps.insert (*si, t, pm);
                }

              }

            }

          }

        }

      }

    } else {

      //  single variant -> we can commit any shapes we have kept for this cell directly to the cell

      std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> >::iterator l = to_commit.find (*c);
      if (l != to_commit.end ()) {
        tl_assert (l->second.size () == 1);
        cell.shapes (layer).insert (l->second.begin ()->second);
        to_commit.erase (l);
      }

      //  for child cells, pull everything that needs to be committed to the parent

      for (db::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {

        std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> >::const_iterator tc = to_commit.find (i->cell_index ());
        if (tc != to_commit.end ()) {

          const std::map<db::ICplxTrans, db::Shapes> &vt = tc->second;

          for (db::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {

            db::ICplxTrans t = i->complex_trans (*ia);
            db::ICplxTrans rt = mp_red->reduce (vvc.begin ()->first * mp_red->reduce_trans (t));
            std::map<db::ICplxTrans, db::Shapes>::const_iterator v = vt.find (rt);

            if (v != vt.end ()) {

              tl::ident_map<db::Layout::properties_id_type> pm;

              for (db::Shapes::shape_iterator si = v->second.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
                cell.shapes (layer).insert (*si, t, pm);
              }

            }

          }

        }

      }

    }

  }
}

const std::map<db::ICplxTrans, size_t> &
VariantsCollectorBase::variants (db::cell_index_type ci) const
{
  std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> >::const_iterator v = m_variants.find (ci);
  static std::map<db::ICplxTrans, size_t> empty_set;
  if (v == m_variants.end ()) {
    return empty_set;
  } else {
    return v->second;
  }
}

bool
VariantsCollectorBase::has_variants () const
{
  for (std::map<db::cell_index_type, std::map<db::ICplxTrans, size_t> >::const_iterator i = m_variants.begin (); i != m_variants.end (); ++i) {
    if (i->second.size () > 1) {
      return true;
    }
  }
  return false;
}

void
VariantsCollectorBase::add_variant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst, bool tl_invariant) const
{
  if (tl_invariant) {
    add_variant_tl_invariant (variants, inst);
  } else {
    add_variant_non_tl_invariant (variants, inst);
  }
}

void
VariantsCollectorBase::add_variant_non_tl_invariant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst) const
{
  if (inst.is_complex ()) {
    for (db::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
      variants [mp_red->reduce_trans (inst.complex_trans (*i))] += 1;
    }
  } else {
    for (db::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
      variants [db::ICplxTrans (mp_red->reduce_trans (*i))] += 1;
    }
  }
}

void
VariantsCollectorBase::add_variant_tl_invariant (std::map<db::ICplxTrans, size_t> &variants, const db::CellInstArray &inst) const
{
  if (inst.is_complex ()) {
    variants [mp_red->reduce_trans (inst.complex_trans ())] += inst.size ();
  } else {
    variants [db::ICplxTrans (mp_red->reduce_trans (inst.front ()))] += inst.size ();
  }
}

void
VariantsCollectorBase::product (const std::map<db::ICplxTrans, size_t> &v1, const std::map<db::ICplxTrans, size_t> &v2, std::map<db::ICplxTrans, size_t> &prod) const
{
  for (std::map<db::ICplxTrans, size_t>::const_iterator i = v1.begin (); i != v1.end (); ++i) {
    for (std::map<db::ICplxTrans, size_t>::const_iterator j = v2.begin (); j != v2.end (); ++j) {
      prod [mp_red->reduce (i->first * j->first)] += i->second * j->second;
    }
  }
}

void
VariantsCollectorBase::copy_shapes (db::Layout &layout, db::cell_index_type ci_to, db::cell_index_type ci_from)
{
  db::Cell &to = layout.cell (ci_to);
  const db::Cell &from = layout.cell (ci_from);
  for (db::Layout::layer_iterator li = layout.begin_layers (); li != layout.end_layers (); ++li) {
    to.shapes ((*li).first) = from.shapes ((*li).first);
  }
}

void
VariantsCollectorBase::create_var_instances (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table, bool tl_invariant) const
{
  if (tl_invariant) {
    create_var_instances_tl_invariant (in_cell, inst, for_var, var_table);
  } else {
    create_var_instances_non_tl_invariant (in_cell, inst, for_var, var_table);
  }
}

void
VariantsCollectorBase::create_var_instances_non_tl_invariant (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table) const
{
  for (std::vector<db::CellInstArrayWithProperties>::const_iterator i = inst.begin (); i != inst.end (); ++i) {

    std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> >::const_iterator f = var_table.find (i->object ().cell_index ());
    if (f == var_table.end ()) {

      in_cell.insert (*i);

    } else {

      const std::map<db::ICplxTrans, db::cell_index_type> &vt = f->second;

      bool need_explode = false;
      bool first = true;
      db::cell_index_type ci = 0;

      for (db::CellInstArray::iterator ia = i->begin (); ! ia.at_end () && ! need_explode; ++ia) {

        db::ICplxTrans rt = mp_red->reduce (for_var * mp_red->reduce_trans (i->complex_trans (*ia)));
        std::map<db::ICplxTrans, db::cell_index_type>::const_iterator v = vt.find (rt);
        tl_assert (v != vt.end ());

        if (first) {
          ci = v->second;
          first = false;
        } else {
          need_explode = (ci != v->second);
        }

      }

      if (need_explode) {

        for (db::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {

          db::ICplxTrans rt = mp_red->reduce (for_var * mp_red->reduce_trans (i->complex_trans (*ia)));
          std::map<db::ICplxTrans, db::cell_index_type>::const_iterator v = vt.find (rt);
          tl_assert (v != vt.end ());

          in_cell.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (v->second), i->complex_trans (*ia)), i->properties_id ()));

        }

      } else if (ci != i->object ().cell_index ()) {

        db::CellInstArray new_array = *i;
        new_array.object () = db::CellInst (ci);
        in_cell.insert (db::CellInstArrayWithProperties (new_array, i->properties_id ()));

      } else {

        in_cell.insert (*i);

      }

    }

  }
}

void
VariantsCollectorBase::create_var_instances_tl_invariant (db::Cell &in_cell, std::vector<db::CellInstArrayWithProperties> &inst, const db::ICplxTrans &for_var, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_table) const
{
  for (std::vector<db::CellInstArrayWithProperties>::const_iterator i = inst.begin (); i != inst.end (); ++i) {

    std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> >::const_iterator f = var_table.find (i->object ().cell_index ());
    if (f == var_table.end ()) {

       in_cell.insert (*i);

    } else {

      const std::map<db::ICplxTrans, db::cell_index_type> &vt = f->second;

      std::map<db::ICplxTrans, db::cell_index_type>::const_iterator v;

      db::ICplxTrans rt = mp_red->reduce (for_var * mp_red->reduce_trans (i->complex_trans ()));
      v = vt.find (rt);
      tl_assert (v != vt.end ());

      db::CellInstArrayWithProperties new_inst = *i;
      new_inst.object ().cell_index (v->second);
      in_cell.insert (new_inst);

    }

  }
}

}

