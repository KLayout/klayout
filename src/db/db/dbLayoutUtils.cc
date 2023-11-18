
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


#include "dbLayoutUtils.h"
#include "dbCellVariants.h"
#include "dbPolygonTools.h"
#include "tlProgress.h"
#include "tlTimer.h"
#include "tlThreads.h"

namespace db
{

// ------------------------------------------------------------------------------------
//  DirectLayerMapping implementation

DirectLayerMapping::DirectLayerMapping (db::Layout *target_layout)
  : ImportLayerMapping (), mp_layout (target_layout), m_initialized (false)
{
  // .. nothing yet ..
}

std::pair <bool, unsigned int> 
DirectLayerMapping::map_layer (const LayerProperties &lprops)
{
  if (! m_initialized) {
    for (db::Layout::layer_iterator l = mp_layout->begin_layers (); l != mp_layout->end_layers (); ++l) {
      m_lmap.insert (std::make_pair (*(*l).second, (*l).first));
    }
    m_initialized = true;
  }

  std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc>::const_iterator lm = m_lmap.find (lprops);
  if (lm != m_lmap.end ()) {
    return std::make_pair (true, lm->second);
  } else {
    return std::make_pair (true, m_lmap.insert (std::make_pair (lprops, mp_layout->insert_layer (lprops))).first->second);
  }
}

// ------------------------------------------------------------------------------------
//  PropertyMapper implementation

PropertyMapper::PropertyMapper (db::Layout *target, const db::Layout *source)
  : mp_target (target ? &target->properties_repository () : 0), mp_source (source ? &source->properties_repository () : 0)
{
  //  .. nothing yet ..
}

PropertyMapper::PropertyMapper (db::PropertiesRepository *target, const db::PropertiesRepository *source)
  : mp_target (target), mp_source (source)
{
  //  .. nothing yet ..
}

/**
 *  @brief Instantiate a property mapper for mapping of property ids from the source to the target layout
 *
 *  This version does not specify a certain source or target layout. These must be set with the
 *  set_source or set_target methods.
 */
PropertyMapper::PropertyMapper ()
  : mp_target (0), mp_source (0)
{
  //  .. nothing yet ..
}

/**
 *  @brief Specify the source layout
 */
void 
PropertyMapper::set_source (const db::Layout *source)
{
  const db::PropertiesRepository *pr = source ? &source->properties_repository () : 0;
  if (pr != mp_source) {
    m_prop_id_map.clear ();
    mp_source = pr;
  }
}

/**
 *  @brief Specify the source property repository
 */
void
PropertyMapper::set_source (const db::PropertiesRepository *source)
{
  if (source != mp_source) {
    m_prop_id_map.clear ();
    mp_source = source;
  }
}

/**
 *  @brief Specify the target layout
 */
void 
PropertyMapper::set_target (db::Layout *target)
{
  db::PropertiesRepository *pr = target ? &target->properties_repository () : 0;
  if (pr != mp_target) {
    m_prop_id_map.clear ();
    mp_target = pr;
  }
}

/**
 *  @brief Specify the target property repository
 */
void
PropertyMapper::set_target (db::PropertiesRepository *target)
{
  if (target != mp_target) {
    m_prop_id_map.clear ();
    mp_target = target;
  }
}

/**
 *  @brief The actual mapping function
 */
db::Layout::properties_id_type 
PropertyMapper::operator() (db::Layout::properties_id_type source_id)
{
  if (source_id == 0 || mp_source == mp_target || ! mp_source || ! mp_target) {
    return source_id;
  }

  tl_assert (mp_source != 0);
  tl_assert (mp_target != 0);

  static tl::Mutex s_mutex;
  tl::MutexLocker locker (&s_mutex);

  std::map <db::Layout::properties_id_type, db::Layout::properties_id_type>::const_iterator p = m_prop_id_map.find (source_id);

  if (p == m_prop_id_map.end ()) {
    db::Layout::properties_id_type new_id = mp_target->translate (*mp_source, source_id);
    m_prop_id_map.insert (std::make_pair (source_id, new_id));
    return new_id;
  } else {
    return p->second;
  }
}

// ------------------------------------------------------------------------------------
//  merge_layouts implementation

static void
collect_cells_to_copy (const db::Layout &source,
                       const std::vector<db::cell_index_type> &source_cells,
                       const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
                       std::set<db::cell_index_type> &all_top_level_cells,
                       std::set<db::cell_index_type> &all_cells_to_copy)
{
  std::vector<db::cell_index_type> dropped_cells;

  for (std::map<db::cell_index_type, db::cell_index_type>::const_iterator m = cell_mapping.begin (); m != cell_mapping.end (); ++m) {
    if (m->second == DropCell) {
      dropped_cells.push_back (m->first);
    }
  }

  for (std::vector<db::cell_index_type>::const_iterator src = source_cells.begin (); src != source_cells.end (); ++src) {

    all_cells_to_copy.insert (*src);
    all_top_level_cells.insert (*src);

    //  feed the excluded cells into the "all_cells_to_copy" cache. This will make "collect_called_cells" not
    //  dive into their hierarchy. We will later delete them there.
    all_cells_to_copy.insert (dropped_cells.begin (), dropped_cells.end ());

    source.cell (*src).collect_called_cells (all_cells_to_copy);

    for (std::vector<db::cell_index_type>::const_iterator i = dropped_cells.begin (); i != dropped_cells.end (); ++i) {
      all_cells_to_copy.erase (*i);
      all_top_level_cells.erase (*i);
    }

  }
}

void 
merge_layouts (db::Layout &target, 
               const db::Layout &source, 
               const db::ICplxTrans &trans,
               const std::vector<db::cell_index_type> &source_cells, 
               const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
               const std::map<unsigned int, unsigned int> &layer_mapping,
               std::map<db::cell_index_type, db::cell_index_type> *final_cell_mapping)
{
  //  collect all called cells and all top level cells
  std::set<db::cell_index_type> all_top_level_cells;
  std::set<db::cell_index_type> all_cells_to_copy;

  collect_cells_to_copy (source, source_cells, cell_mapping, all_top_level_cells, all_cells_to_copy);

  //  identify all new cells and create new ones
  std::map<db::cell_index_type, db::cell_index_type> new_cell_mapping;
  for (std::set<db::cell_index_type>::const_iterator c = all_cells_to_copy.begin (); c != all_cells_to_copy.end (); ++c) {
    if (cell_mapping.find (*c) == cell_mapping.end ()) {
      new_cell_mapping.insert (std::make_pair (*c, target.add_cell (source, *c)));
    }
  }

  if (final_cell_mapping) {
    for (std::map<db::cell_index_type, db::cell_index_type>::const_iterator m = cell_mapping.begin (); m != cell_mapping.end (); ++m) {
      if (m->second != DropCell) {
        final_cell_mapping->insert (*m);
      }
    }
    final_cell_mapping->insert (new_cell_mapping.begin (), new_cell_mapping.end ());
  }

  //  provide the property mapper
  db::PropertyMapper pm (&target, &source);

  tl::RelativeProgress progress (tl::to_string (tr ("Merge layouts")), all_cells_to_copy.size (), 1);

  //  actually to the mapping
  for (std::set<db::cell_index_type>::const_iterator c = all_cells_to_copy.begin (); c != all_cells_to_copy.end (); ++c) {

    ++progress;

    db::cell_index_type target_cell_index = 0;
    std::map<db::cell_index_type, db::cell_index_type>::const_iterator cm = cell_mapping.find (*c);
    if (cm == cell_mapping.end ()) {
      target_cell_index = new_cell_mapping[*c];
    } else {
      target_cell_index = cm->second;
    }

    const db::Cell &source_cell = source.cell (*c);
    db::Cell &target_cell = target.cell (target_cell_index);

    //  NOTE: this implementation employs the safe but cumbersome "local transformation" feature.
    //  This means, all cells are transformed according to the given transformation and their
    //  references are transformed to account for that effect. This will lead to somewhat strange
    //  local modifications.

    //  copy and transform the shapes
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      target_cell.shapes (lm->second).insert_transformed (source_cell.shapes (lm->first), trans, pm);
    }

    //  copy the instances
    for (db::Cell::const_iterator inst = source_cell.begin (); !inst.at_end (); ++inst) {

      //  only copy instances for new cells ..
      std::map<db::cell_index_type, db::cell_index_type>::const_iterator nc = new_cell_mapping.find (inst->cell_index ());
      if (nc != new_cell_mapping.end ()) {

        db::CellInstArray new_inst_array (inst->cell_inst ());
        new_inst_array.transform_into (trans, 0 /*no array repository*/);

        new_inst_array.object ().cell_index (nc->second);

        if (inst->has_prop_id ()) {
          target_cell.insert (db::object_with_properties<db::CellInstArray> (new_inst_array, pm (inst->prop_id ())));
        } else {
          target_cell.insert (new_inst_array);
        }

      }

    }

  }

}

static void 
copy_or_propagate_shapes (db::Layout &target, 
                          const db::Layout &source, 
                          const db::ICplxTrans &trans,
                          const db::ICplxTrans &propagate_trans,
                          db::PropertyMapper &pm,
                          db::cell_index_type source_cell_index,
                          db::cell_index_type source_parent_cell_index,
                          unsigned int target_layer, unsigned int source_layer,
                          const std::set<db::cell_index_type> &all_cells_to_copy,
                          const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
                          const ShapesTransformer *transformer)
{
  const db::Cell &source_cell = source.cell (source_cell_index);
  const db::Cell &source_parent_cell = source.cell (source_parent_cell_index);

  std::map<db::cell_index_type, db::cell_index_type>::const_iterator cm = cell_mapping.find (source_parent_cell_index);
  if (cm == cell_mapping.end ()) {

    for (db::Cell::parent_inst_iterator p = source_parent_cell.begin_parent_insts (); ! p.at_end (); ++p) {

      if (all_cells_to_copy.find (p->parent_cell_index ()) != all_cells_to_copy.end ()) {
        const db::CellInstArray &cell_inst = p->child_inst ().cell_inst ();
        for (db::CellInstArray::iterator a = cell_inst.begin (); ! a.at_end (); ++a) {
          db::ICplxTrans t = db::ICplxTrans (cell_inst.complex_trans (*a)) * propagate_trans;
          copy_or_propagate_shapes (target, source, trans, t, pm, source_cell_index, p->parent_cell_index (), target_layer, source_layer, all_cells_to_copy, cell_mapping, transformer);
        }
      }

    }

  } else if (cm->second != DropCell) {

    db::Cell &target_cell = target.cell (cm->second);
    transformer->insert_transformed (target_cell.shapes (target_layer), source_cell.shapes (source_layer), trans * propagate_trans, pm);
  }
}

static void
copy_or_move_shapes (db::Layout &target,
                     db::Layout &source,
                     const db::ICplxTrans &trans,
                     const std::vector<db::cell_index_type> &source_cells,
                     const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
                     const std::map<unsigned int, unsigned int> &layer_mapping,
                     const ShapesTransformer *transformer,
                     bool move
                    )
{
  //  collect all called cells and all top level cells
  std::set<db::cell_index_type> all_top_level_cells;
  std::set<db::cell_index_type> all_cells_to_copy;

  collect_cells_to_copy (source, source_cells, cell_mapping, all_top_level_cells, all_cells_to_copy);

  //  provide the property mapper
  db::PropertyMapper pm (&target, &source);

  tl::RelativeProgress progress (tl::to_string (tr ("Copy shapes")), all_cells_to_copy.size () * layer_mapping.size (), 1);

  //  and copy
  for (std::set<db::cell_index_type>::const_iterator c = all_cells_to_copy.begin (); c != all_cells_to_copy.end (); ++c) {
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      ++progress;
      copy_or_propagate_shapes (target, source, trans, db::ICplxTrans (), pm, *c, *c, lm->second, lm->first, all_cells_to_copy, cell_mapping, transformer);
      if (move) {
        source.cell (*c).shapes (lm->first).clear ();
      }
    }
  }
}

namespace
{
  class StandardShapesTransformer
    : public ShapesTransformer
  {
  public:
    void insert_transformed (Shapes &into, const Shapes &from, const ICplxTrans &trans, PropertyMapper &pm) const
    {
      into.insert_transformed (from, trans, pm);
    }
  };
}

void
copy_shapes (db::Layout &target, 
             const db::Layout &source, 
             const db::ICplxTrans &trans,
             const std::vector<db::cell_index_type> &source_cells, 
             const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer)
{
  StandardShapesTransformer st;
  if (! transformer) {
    transformer = &st;
  }
  copy_or_move_shapes (target, const_cast<db::Layout &> (source), trans, source_cells, cell_mapping, layer_mapping, transformer, false);
}

void 
move_shapes (db::Layout &target, 
             db::Layout &source, 
             const db::ICplxTrans &trans,
             const std::vector<db::cell_index_type> &source_cells, 
             const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer)
{
  StandardShapesTransformer st;
  if (! transformer) {
    transformer = &st;
  }
  copy_or_move_shapes (target, source, trans, source_cells, cell_mapping, layer_mapping, transformer, true);
}

// ------------------------------------------------------------
//  Implementation of "find_layout_context"

static std::pair<bool, db::ICplxTrans> 
find_layout_context (const db::Layout &layout, db::cell_index_type from, db::cell_index_type to, std::set <db::cell_index_type> &visited, const db::ICplxTrans &trans) 
{
  const db::Cell &cell = layout.cell (from);
  for (db::Cell::parent_inst_iterator p = cell.begin_parent_insts (); ! p.at_end (); ++p) {

    if (p->parent_cell_index () == to) {

      return std::make_pair (true, db::ICplxTrans (p->child_inst ().complex_trans ()) * trans);

    } else if (visited.find (p->parent_cell_index ()) == visited.end ()) {

      visited.insert (p->parent_cell_index ());

      std::pair<bool, db::ICplxTrans> context = find_layout_context (layout, p->parent_cell_index (), to, visited, db::ICplxTrans (p->child_inst ().complex_trans ()) * trans);
      if (context.first) {
        return context;
      }

    }

  }

  return std::pair<bool, db::ICplxTrans> (false, db::ICplxTrans ());
}

std::pair<bool, db::ICplxTrans> 
find_layout_context (const db::Layout &layout, db::cell_index_type from, db::cell_index_type to)
{
  if (from == to) {
    return std::make_pair (true, db::ICplxTrans ());
  } else {
    std::set <db::cell_index_type> v;
    return find_layout_context (layout, from, to, v, db::ICplxTrans ());
  }
}

// ------------------------------------------------------------
//  Implementation of ContextCache

ContextCache::ContextCache (const db::Layout *layout)
  : mp_layout (layout)
{
  //  .. nothing yet ..
}

const std::pair<bool, db::ICplxTrans> &
ContextCache::find_layout_context (db::cell_index_type from, db::cell_index_type to)
{
  if (! mp_layout) {
    static std::pair<bool, db::ICplxTrans> nothing (false, db::ICplxTrans ());
    return nothing;
  }

  std::map<std::pair<db::cell_index_type, db::cell_index_type>, std::pair<bool, db::ICplxTrans> >::iterator c = m_cache.find (std::make_pair (from, to));
  if (c == m_cache.end ()) {
    c = m_cache.insert (std::make_pair (std::make_pair (from, to), std::make_pair (false, db::ICplxTrans ()))).first;
    c->second = db::find_layout_context (*mp_layout, from, to);
  }
  return c->second;
}

// ------------------------------------------------------------
//  Scale and snap a layout

static void
scale_and_snap_cell_instance (db::CellInstArray &ci, const db::ICplxTrans &tr, const db::ICplxTrans &trinv, const db::Vector &delta, db::Coord g, db::Coord m, db::Coord d)
{
  db::Trans ti (ci.front ());

  db::Vector ti_disp = ti.disp ();
  ti_disp.transform (tr);
  ti_disp = scaled_and_snapped_vector (ti_disp, g, m, d, delta.x (), g, m, d, delta.y ());
  ti_disp.transform (trinv);

  ci.move (ti_disp - ti.disp ());
}

static db::Edge
scaled_and_snapped_edge (const db::Edge &e, db::Coord g, db::Coord m, db::Coord d, db::Coord ox, db::Coord oy)
{
  int64_t dg = int64_t (g) * int64_t (d);

  int64_t x1 = snap_to_grid (int64_t (e.p1 ().x ()) * m + int64_t (ox), dg) / int64_t (d);
  int64_t y1 = snap_to_grid (int64_t (e.p1 ().y ()) * m + int64_t (oy), dg) / int64_t (d);

  int64_t x2 = snap_to_grid (int64_t (e.p2 ().x ()) * m + int64_t (ox), dg) / int64_t (d);
  int64_t y2 = snap_to_grid (int64_t (e.p2 ().y ()) * m + int64_t (oy), dg) / int64_t (d);

  return db::Edge (db::Point (x1, y1), db::Point (x2, y2));
}

void
scale_and_snap (db::Layout &layout, db::Cell &cell, db::Coord g, db::Coord m, db::Coord d)
{
  tl::SelfTimer timer (tl::verbosity () >= 31, tl::to_string (tr ("scale_and_snap")));

  if (g < 0) {
    throw tl::Exception (tl::to_string (tr ("Snapping requires a positive grid value")));
  }

  if (m <= 0 || d <= 0) {
    throw tl::Exception (tl::to_string (tr ("Scale and snap requires positive and non-null magnification or divisor values")));
  }

  if (! g && m == d) {
    return;
  }

  db::cell_variants_collector<db::ScaleAndGridReducer> vars (db::ScaleAndGridReducer (g, m, d));

  {
    tl::SelfTimer timer1 (tl::verbosity () >= 41, tl::to_string (tr ("scale_and_snap: variant formation")));
    vars.collect (&layout, cell.cell_index ());
    vars.separate_variants ();
  }

  std::set<db::cell_index_type> called_cells;
  cell.collect_called_cells (called_cells);
  called_cells.insert (cell.cell_index ());

  db::LayoutLocker layout_locker (&layout);
  layout.update ();

  tl::SelfTimer timer2 (tl::verbosity () >= 41, tl::to_string (tr ("scale_and_snap: snapping and scaling")));

  std::vector<db::Point> heap;
  std::vector<db::Vector> iterated_array_vectors;

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    if (called_cells.find (c->cell_index ()) == called_cells.end ()) {
      continue;
    }

    db::ICplxTrans tr = vars.single_variant_transformation (c->cell_index ());

    //  NOTE: tr_disp is already multiplied with mag, so it can be an integer
    db::Vector tr_disp = tr.disp ();

    tr.disp (db::Vector ());
    db::ICplxTrans trinv = tr.inverted ();

    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {

      db::Shapes &s = c->shapes ((*l).first);
      db::Shapes new_shapes (layout.is_editable ());

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes); ! si.at_end (); ++si) {

        db::Polygon poly;
        si->polygon (poly);
        poly.transform (tr);
        poly = scaled_and_snapped_polygon (poly, g, m, d, tr_disp.x (), g, m, d, tr_disp.y (), heap);
        poly.transform (trinv);

        if (si->is_box () && poly.is_box ()) {
          if (si->has_prop_id ()) {
            new_shapes.insert (db::BoxWithProperties (poly.box (), si->prop_id ()));
          } else {
            new_shapes.insert (poly.box ());
          }
        } else {
          if (si->has_prop_id ()) {
            new_shapes.insert (db::PolygonWithProperties (poly, si->prop_id ()));
          } else {
            new_shapes.insert (poly);
          }
        }

      }

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Texts); ! si.at_end (); ++si) {

        db::Text text;
        si->text (text);
        text.transform (tr);
        text.trans (db::Trans (text.trans ().rot (), scaled_and_snapped_vector (text.trans ().disp (), g, m, d, tr_disp.x (), g, m, d, tr_disp.y ())));
        text.transform (trinv);

        if (si->has_prop_id ()) {
          new_shapes.insert (db::TextWithProperties (text, si->prop_id ()));
        } else {
          new_shapes.insert (text);
        }

      }

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Edges); ! si.at_end (); ++si) {

        db::Edge edge;
        si->edge (edge);
        edge.transform (tr);
        edge = scaled_and_snapped_edge (edge, g, m , d, tr_disp.x (), tr_disp.y ());
        edge.transform (trinv);

        if (si->has_prop_id ()) {
          new_shapes.insert (db::EdgeWithProperties (edge, si->prop_id ()));
        } else {
          new_shapes.insert (edge);
        }

      }

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::EdgePairs); ! si.at_end (); ++si) {

        db::EdgePair edge_pair;
        si->edge_pair (edge_pair);
        edge_pair.transform (tr);
        edge_pair = db::EdgePair (scaled_and_snapped_edge (edge_pair.first (), g, m , d, tr_disp.x (), tr_disp.y ()),
                                  scaled_and_snapped_edge (edge_pair.second (), g, m , d, tr_disp.x (), tr_disp.y ()));
        edge_pair.transform (trinv);

        if (si->has_prop_id ()) {
          new_shapes.insert (db::EdgePairWithProperties (edge_pair, si->prop_id ()));
        } else {
          new_shapes.insert (edge_pair);
        }

      }

      s.swap (new_shapes);

    }

    //  Snap instance placements to grid and magnify
    //  NOTE: we can modify the instances because the ScaleAndGridReducer marked every cell with children
    //  as a variant cell (an effect of ScaleAndGridReducer::want_variants(cell) == true where cells have children).
    //  The variant formation also made sure the iterated and regular arrays are exploded where required.

    for (db::Cell::const_iterator inst = c->begin (); ! inst.at_end (); ++inst) {

      const db::CellInstArray &ia = inst->cell_inst ();

      iterated_array_vectors.clear ();
      db::Vector a, b;
      unsigned long na, nb;

      db::CellInstArray new_array (ia);

      if (ia.is_iterated_array (&iterated_array_vectors)) {

        bool needs_update = false;
        for (std::vector<db::Vector>::iterator i = iterated_array_vectors.begin (); i != iterated_array_vectors.end (); ++i) {
          db::Vector v = scaled_and_snapped_vector (*i, g, m, d, tr_disp.x (), g, m, d, tr_disp.y ());
          if (v != *i) {
            needs_update = true;
            *i = v;
          }
        }

        if (needs_update) {
          new_array = db::CellInstArray (ia.object (), ia.complex_trans (ia.front ()), iterated_array_vectors.begin (), iterated_array_vectors.end ());
        }

      } else if (ia.is_regular_array (a, b, na, nb)) {

        a = scaled_and_snapped_vector (a, g, m, d, tr_disp.x (), g, m, d, tr_disp.y ());
        b = scaled_and_snapped_vector (b, g, m, d, tr_disp.x (), g, m, d, tr_disp.y ());

        new_array = db::CellInstArray (ia.object (), ia.complex_trans (ia.front ()), a, b, na, nb);

      }

      scale_and_snap_cell_instance (new_array, tr, trinv, tr_disp, g, m, d);
      c->replace (*inst, new_array);

    }

  }
}

}

