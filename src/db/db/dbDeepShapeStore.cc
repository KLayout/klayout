
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

// ----------------------------------------------------------------------------------

DeepShapeStore::DeepShapeStore ()
{
  //  .. nothing yet ..
}

DeepShapeStore::~DeepShapeStore ()
{
  //  .. nothing yet ..
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

}

