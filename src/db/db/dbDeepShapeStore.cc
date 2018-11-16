
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "dbDeepShapeStore.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"

#include "tlTimer.h"

namespace db
{

// ----------------------------------------------------------------------------------

DeepLayer::DeepLayer ()
  : mp_store (), m_layout (0), m_layer (0)
{
  //  .. nothing yet ..
}

DeepLayer::DeepLayer (const DeepLayer &x)
  : mp_store (x.mp_store), m_layout (x.m_layout), m_layer (x.m_layer)
{
  //  .. nothing yet ..
}

DeepLayer::DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer)
  : mp_store (store), m_layout (layout), m_layer (layer)
{
  //  .. nothing yet ..
}

DeepLayer::~DeepLayer ()
{
  //  .. nothing yet ..
}

void
DeepLayer::insert_into (db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  check_dss ();
  const_cast<db::DeepShapeStore *> (mp_store.get ())->insert (*this, into_layout, into_cell, into_layer);
}

db::Layout *
DeepLayer::layout ()
{
  check_dss ();
  return mp_store->layout (m_layout);
}

const db::Layout *
DeepLayer::layout () const
{
  check_dss ();
  return const_cast<db::DeepShapeStore *> (mp_store.get ())->layout (m_layout);
}

void
DeepLayer::check_dss () const
{
  if (mp_store.get () == 0) {
    throw tl::Exception (tl::to_string (tr ("Heap lost: the DeepShapeStore container no longer exists")));
  }
}

// ----------------------------------------------------------------------------------

static size_t s_instance_count = 0;

DeepShapeStore::DeepShapeStore ()
{
  ++s_instance_count;
}

DeepShapeStore::~DeepShapeStore ()
{
  --s_instance_count;
}

size_t DeepShapeStore::instance_count ()
{
  return s_instance_count;
}

DeepLayer DeepShapeStore::create_polygon_layer (const db::RecursiveShapeIterator &si, double max_area_ratio, size_t max_vertex_count)
{
  unsigned int layout_index = 0;
  unsigned int layer_index = 0;

  layout_map_type::iterator l = m_layout_map.find (si);
  if (l == m_layout_map.end ()) {

    layout_index = (unsigned int) m_layouts.size ();

    m_layouts.push_back (new db::Layout ());
    m_layouts.back ().dbu (si.layout ()->dbu ());
    layer_index = m_layouts.back ().insert_layer ();

    m_builders.push_back (new db::HierarchyBuilder (&m_layouts.back (), layer_index));

    m_layout_map[si] = layout_index;

  } else {

    layout_index = l->second;
    layer_index = m_layouts[layout_index].insert_layer ();

    m_builders[layout_index].set_target_layer (layer_index);

  }

  //  The chain of operators for producing clipped and reduced polygon references
  db::PolygonReferenceHierarchyBuilderShapeReceiver refs (& m_layouts[layout_index]);
  db::ReducingHierarchyBuilderShapeReceiver red (&refs, max_area_ratio, max_vertex_count);
  db::ClippingHierarchyBuilderShapeReceiver clip (&red);

  //  Build the working hierarchy from the recursive shape iterator
  try {

    tl::SelfTimer timer (tl::to_string (tr ("Building working hierarchy")));

    m_builders[layout_index].set_shape_receiver (&clip);
    db::RecursiveShapeIterator (si).push (& m_builders[layout_index]);
    m_builders[layout_index].set_shape_receiver (0);

  } catch (...) {
    m_builders[layout_index].set_shape_receiver (0);
    throw;
  }

  return DeepLayer (this, layout_index, layer_index);
}

void
DeepShapeStore::insert (const DeepLayer &deep_layer, db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer)
{
  const db::Layout *source_layout = deep_layer.layout ();
  if (source_layout->begin_top_down () == source_layout->end_top_cells ()) {
    //  empty source - nothing to do.
    return;
  }

  db::cell_index_type source_top = *source_layout->begin_top_down();

  db::HierarchyBuilder &original_builder = m_builders [deep_layer.layout_index ()];

  //  derive a cell mapping for source to target. We employ a

  DeliveryMappingCacheKey key (deep_layer.layout_index (), into_layout, into_cell);

  std::map<DeliveryMappingCacheKey, db::CellMapping>::iterator cm = m_delivery_mapping_cache.find (key);
  if (cm == m_delivery_mapping_cache.end ()) {

    cm = m_delivery_mapping_cache.insert (std::make_pair (key, db::CellMapping ())).first;

    if (into_layout == original_builder.source ().layout () && &into_layout->cell (into_cell) == original_builder.source ().top_cell ()) {

      //  This is the case of mapping back to the original. In this case we can use the information
      //  provided inside the original hierarchy builders. They list the source cells and the target cells
      //  create from them. We need to consider however, that the hierarchy builder is allowed to create
      //  variants which we cannot map.

      bool any_skipped = false;

      for (HierarchyBuilder::cell_map_type::const_iterator m = original_builder.begin_cell_map (); m != original_builder.end_cell_map (); ++m) {

        HierarchyBuilder::cell_map_type::const_iterator mm = m;
        ++mm;
        bool skip = false;
        while (mm != original_builder.end_cell_map () && mm->first.first == m->first.first) {
          //  we have cell variants and cannot simply map
          ++mm;
          ++m;
          skip = true;
        }

        if (! skip) {
          cm->second.map (m->first.first, m->second);
        } else {
          any_skipped = true;
        }

      }

      if (any_skipped) {
        //  Add new cells for the variants
        cm->second.create_missing_mapping (*into_layout, into_cell, *source_layout, source_top);
      }

    } else if (into_layout->cells () == 1) {

      //  Another simple case is mapping into an empty (or single-top-cell-only) layout, where we can use "create_from_single_full".
      cm->second.create_single_mapping_full (*into_layout, into_cell, *source_layout, source_top);

    } else {

      cm->second.create_from_geometry_full (*into_layout, into_cell, *source_layout, source_top);

    }

  }

  //  Actually copy the shapes

  db::ICplxTrans trans (source_layout->dbu () / into_layout->dbu ());

  std::map<unsigned int, unsigned int> lm;
  lm.insert (std::make_pair (deep_layer.layer (), into_layer));

  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (source_top);
  db::copy_shapes (*into_layout, *source_layout, trans, source_cells, cm->second.table (), lm);
}

}

