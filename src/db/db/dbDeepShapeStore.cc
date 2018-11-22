
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
  if (mp_store.get ()) {
    mp_store->add_ref (m_layout, m_layer);
  }
}

DeepLayer::DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer)
  : mp_store (store), m_layout (layout), m_layer (layer)
{
  if (store) {
    store->add_ref (layout, layer);
  }
}

DeepLayer &DeepLayer::operator= (const DeepLayer &other)
{
  if (this != &other) {
    if (mp_store.get ()) {
      mp_store->remove_ref (m_layout, m_layer);
    }
    mp_store = other.mp_store;
    m_layout = other.m_layout;
    m_layer = other.m_layer;
    if (mp_store.get ()) {
      mp_store->add_ref (m_layout, m_layer);
    }
  }

  return *this;
}


DeepLayer::~DeepLayer ()
{
  if (mp_store.get ()) {
    mp_store->remove_ref (m_layout, m_layer);
  }
}

DeepLayer
DeepLayer::derived () const
{
  return DeepLayer (const_cast <db::DeepShapeStore *> (mp_store.get ()), m_layout, const_cast <db::Layout *> (layout ())->insert_layer ());
}

DeepLayer
DeepLayer::copy () const
{
  DeepLayer new_layer (derived ());

  db::DeepShapeStore *non_const_store = const_cast<db::DeepShapeStore *> (mp_store.get ());
  if (non_const_store->layout (m_layout)) {
    non_const_store->layout (m_layout)->copy_layer (m_layer, new_layer.layer ());
  }

  return new_layer;
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

db::Cell *
DeepLayer::initial_cell ()
{
  db::Layout *ly = layout ();
  tl_assert (ly->begin_top_down () != ly->end_top_down ());
  return &ly->cell (*ly->begin_top_down ());
}

const db::Cell *
DeepLayer::initial_cell () const
{
  const db::Layout *ly = layout ();
  tl_assert (ly->begin_top_down () != ly->end_top_down ());
  return &ly->cell (*ly->begin_top_down ());
}

void
DeepLayer::check_dss () const
{
  if (mp_store.get () == 0) {
    throw tl::Exception (tl::to_string (tr ("Heap lost: the DeepShapeStore container no longer exists")));
  }
}

// ----------------------------------------------------------------------------------

struct DeepShapeStore::LayoutHolder
{
  LayoutHolder ()
    : refs (0), layout (), builder (&layout)
  {
    //  .. nothing yet ..
  }

  void add_layer_ref (unsigned int layer)
  {
    layer_refs[layer] += 1;
  }

  void remove_layer_ref (unsigned int layer)
  {
    if ((layer_refs[layer] -= 1) <= 0) {
      layout.clear_layer (layer);
      layer_refs.erase (layer);
    }
  }

  int refs;
  db::Layout layout;
  db::HierarchyBuilder builder;
  std::map<unsigned int, int> layer_refs;
};

// ----------------------------------------------------------------------------------

static size_t s_instance_count = 0;

DeepShapeStore::DeepShapeStore ()
  : m_threads (1), m_max_area_ratio (3.0), m_max_vertex_count (16)
{
  ++s_instance_count;
}

DeepShapeStore::~DeepShapeStore ()
{
  --s_instance_count;

  for (std::vector<LayoutHolder *>::iterator h = m_layouts.begin (); h != m_layouts.end (); ++h) {
    delete *h;
  }
  m_layouts.clear ();
}

bool DeepShapeStore::is_valid_layout_index (unsigned int n) const
{
  return (n < (unsigned int) m_layouts.size () && m_layouts[n] != 0);
}

const db::Layout *DeepShapeStore::const_layout (unsigned int n) const
{
  tl_assert (is_valid_layout_index (n));
  return &(m_layouts [n]->layout);
}

db::Layout *DeepShapeStore::layout (unsigned int n)
{
  tl_assert (is_valid_layout_index (n));
  return &(m_layouts [n]->layout);
}

size_t DeepShapeStore::instance_count ()
{
  return s_instance_count;
}

void DeepShapeStore::set_threads (int n)
{
  m_threads = n;
}

void DeepShapeStore::set_max_area_ratio (double ar)
{
  m_max_area_ratio = ar;
}

void DeepShapeStore::set_max_vertex_count (size_t n)
{
  m_max_vertex_count = n;
}

void DeepShapeStore::add_ref (unsigned int layout, unsigned int layer)
{
  tl::MutexLocker locker (&m_lock);

  tl_assert (layout < (unsigned int) m_layouts.size () && m_layouts[layout] != 0);

  m_layouts[layout]->refs += 1;
  m_layouts[layout]->add_layer_ref (layer);
}

void DeepShapeStore::remove_ref (unsigned int layout, unsigned int layer)
{
  tl::MutexLocker locker (&m_lock);

  tl_assert (layout < (unsigned int) m_layouts.size () && m_layouts[layout] != 0);

  m_layouts[layout]->remove_layer_ref (layer);

  if ((m_layouts[layout]->refs -= 1) <= 0) {
    delete m_layouts[layout];
    m_layouts[layout] = 0;
  }
}

DeepLayer DeepShapeStore::create_polygon_layer (const db::RecursiveShapeIterator &si, double max_area_ratio, size_t max_vertex_count)
{
  if (max_area_ratio == 0.0) {
    max_area_ratio = m_max_area_ratio;
  }
  if (max_vertex_count == 0) {
    max_vertex_count = m_max_vertex_count;
  }

  unsigned int layout_index = 0;

  layout_map_type::iterator l = m_layout_map.find (si);
  if (l == m_layout_map.end ()) {

    layout_index = (unsigned int) m_layouts.size ();

    m_layouts.push_back (new LayoutHolder ());
    m_layouts.back ()->layout.dbu (si.layout ()->dbu ());

    m_layout_map[si] = layout_index;

  } else {

    layout_index = l->second;

  }

  unsigned int layer_index = m_layouts[layout_index]->layout.insert_layer ();
  m_layouts[layout_index]->builder.set_target_layer (layer_index);

  //  The chain of operators for producing clipped and reduced polygon references
  db::PolygonReferenceHierarchyBuilderShapeReceiver refs (& m_layouts[layout_index]->layout);
  db::ReducingHierarchyBuilderShapeReceiver red (&refs, max_area_ratio, max_vertex_count);
  db::ClippingHierarchyBuilderShapeReceiver clip (&red);

  //  Build the working hierarchy from the recursive shape iterator
  try {

    tl::SelfTimer timer (tl::to_string (tr ("Building working hierarchy")));

    m_layouts[layout_index]->builder.set_shape_receiver (&clip);
    db::RecursiveShapeIterator (si).push (& m_layouts[layout_index]->builder);
    m_layouts[layout_index]->builder.set_shape_receiver (0);

  } catch (...) {
    m_layouts[layout_index]->builder.set_shape_receiver (0);
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

  db::HierarchyBuilder &original_builder = m_layouts [deep_layer.layout_index ()]->builder;

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

