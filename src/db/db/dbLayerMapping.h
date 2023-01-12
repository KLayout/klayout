
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



#ifndef HDR_dbLayerMapping
#define HDR_dbLayerMapping

#include "dbCommon.h"

#include <map>

namespace db
{

class Layout;

/**
 *  @brief A layer mapping
 *
 *  The layer mapping represents corresponding layers between two layouts, layer A and B.
 *  Layer mappings can be generated from the layer properties (layer, datatype, name) or
 *  a custom layer mapping can be created using the "map" method. The purpose of the layer
 *  mapping is to find a layer in layout A for a corresponding layer in Layout B.
 *
 *  Layer mappings play a role in copy and compare operations.
 */
class DB_PUBLIC LayerMapping
{
public:
  typedef std::map <unsigned int, unsigned int>::const_iterator iterator;

  /**
   *  @brief Constructor - creates an empty mapping
   */
  LayerMapping ();

  /**
   *  @brief Clear the mapping
   */
  void clear ();

  /**
   *  @brief Create a mapping for layout_b to layout_a employing the layer properties
   *
   *  Layers with null properties (temporary layers) are not mapped.
   */
  void create (const db::Layout &layout_a, const db::Layout &layout_b);

  /**
   *  @brief Create a full mapping for layout_b to layout_a employing the layer properties
   *
   *  A full mapping means that all layers of layout_b are mapped. Layers missing in layout A
   *  are created. 
   *
   *  Layers with null properties (temporary layers) are not mapped and not created.
   *
   *  @return A list of newly created layers.
   */
  std::vector<unsigned int> create_full (db::Layout &layout_a, const db::Layout &layout_b);

  /**
   *  @brief Determine layer mapping to a layout_b layer to the corresponding layout_a layer.
   *
   *  @param layer_b The index of the layer in layout_b whose mapping is requested.
   *  @return First: true, if a unique mapping is given, Second: the layer index in layout_a.
   */
  std::pair<bool, unsigned int> layer_mapping_pair (unsigned int layer_b) const;

  /**
   *  @brief Determine if a layer has a mapping to a layout_a layer.
   *
   *  @param layer_b The index of the layer in layout_b whose mapping is requested.
   *  @return true, if the layer has a mapping
   */
  bool has_mapping (unsigned int layer_b) const;

  /**
   *  @brief Add a layer mapping
   *
   *  @param layer_b The index of the layer in layout_a (the source of the mapping)
   *  @param layer_a The index of the layer in layout_a (the target of the mapping)
   */
  void map (unsigned int layer_b, unsigned int layer_a)
  {
    m_b2a_mapping.insert (std::make_pair (layer_b, 0)).first->second = layer_a;
  }

  /**
   *  @brief Determine layer mapping to a layout_b layer to the corresponding layout_a layer.
   *
   *  @param layer_b The index of the layer in layout_b whose mapping is requested.
   *  @return the layer in layout_a.
   */
  unsigned int layer_mapping (unsigned int layer_b) const;

  /**
   *  @brief Begin iterator for the b to a layer mapping
   */
  iterator begin () const
  {
    return m_b2a_mapping.begin ();
  }

  /**
   *  @brief End iterator for the b to a layer mapping
   */
  iterator end () const
  {
    return m_b2a_mapping.end ();
  }

  /**
   *  @brief Access to the mapping table
   */
  const std::map <unsigned int, unsigned int> &table () const 
  {
    return m_b2a_mapping;
  }

private:
  std::map <unsigned int, unsigned int> m_b2a_mapping;
};

}

#endif


