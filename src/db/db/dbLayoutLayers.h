
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


#ifndef HDR_dbLayoutLayers
#define HDR_dbLayoutLayers

#include "dbCommon.h"

#include "dbLayerProperties.h"
#include "dbMemStatistics.h"

#include <vector>
#include <map>

namespace db
{

class MemStatistics;
class LayoutLayers;

/**
 *  @brief A layer iterator (for valid layers)
 *
 *  The layer iterator delivers layer indices and layer properties of layer layers.
 */
class DB_PUBLIC LayerIterator
{
public:
  /**
   *  @brief Constructor
   */
  LayerIterator (unsigned int layer_index, const db::LayoutLayers &layout);

  /**
   *  @brief Increment operator
   */
  LayerIterator &operator++();

  /**
   *  @brief Equality
   */
  bool operator== (const LayerIterator &i)
  {
    return i.m_layer_index == m_layer_index;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const LayerIterator &i)
  {
    return i.m_layer_index != m_layer_index;
  }

  /**
   *  @brief Access operator
   */
  std::pair<unsigned int, const db::LayerProperties *> operator*() const;

private:
  unsigned int m_layer_index;
  const db::LayoutLayers &m_layout;
};

/**
 *  @brief The layoutLayers object
 *
 *  This object wraps the layer list and manages layer properties,
 *  layer states and the free layer list.
 */

class DB_PUBLIC LayoutLayers
{
public:
  typedef LayerIterator layer_iterator;
  enum LayerState { Normal, Free, Special };

  /**
   *  @brief Standard constructor
   */
  LayoutLayers ();

  /**
   *  @brief The copy ctor
   */
  LayoutLayers (const LayoutLayers &d);

  /**
   *  @brief Destructor
   */
  ~LayoutLayers ();

  /**
   *  @brief Assignment operator
   */
  LayoutLayers &operator= (const LayoutLayers &d);

  /**
   *  @brief Clears the layout layers
   */
  void clear ();

  /**  
   *  @brief Deletes a layer
   */
  void delete_layer (unsigned int n);

  /**  
   *  @brief Gets the layer's state
   */
  LayerState layer_state (unsigned int l) const
  {
    return l < (unsigned int) m_layer_states.size () ? m_layer_states [l] : Free;
  }

  /**
   *  @brief Gets the number of layers defined so far
   *  
   *  TODO: the list of 0 to nlayers-1 also contains the free layers -
   *  we should get a vector containing the layers that are actually
   *  allocated.
   */
  unsigned int layers () const
  {
    return (unsigned int) m_layer_states.size ();
  }

  /**
   *  @brief The iterator of valid layers: begin 
   */
  layer_iterator begin_layers () const
  {
    return layer_iterator (0, *this);
  }

  /**
   *  @brief The iterator of valid layers: end 
   */
  layer_iterator end_layers () const
  {
    return layer_iterator (layers (), *this);
  }

  /**
   *  @brief Reserve space for n layers
   */
  void reserve_layers (unsigned int n);

  /**
   *  @brief Inserts a new layer with the given properties
   */
  unsigned int insert_layer (const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Inserts a new layer with the given properties at the given index
   */
  void insert_layer (unsigned int index, const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Gets or creates a layer with the given properties
   *
   *  If there already is a layer matching the given properties, it's index will be
   *  returned. Otherwise a new layer with these properties is created.
   */
  unsigned int get_layer (const db::LayerProperties &props);

  /**
   *  @brief Gets the layer with the given properties or -1 if such a layer does not exist.
   */
  int get_layer_maybe (const db::LayerProperties &props) const;

  /**
   *  @brief Insert a new special layer with the given properties
   *
   *  A special layers is used for example to represent rulers.
   */
  unsigned int insert_special_layer (const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Insert a new layer with the given properties at the given index
   *
   *  A special layers is used for example to represent rulers.
   */
  void insert_special_layer (unsigned int index, const LayerProperties &props = LayerProperties ());

  /**
   *  @brief Gets the guiding shape layer or -1 if none is set yet.
   */
  int guiding_shape_layer_maybe () const
  {
    return m_guiding_shape_layer;
  }

  /**
   *  @brief Gets the guiding shape layer
   *
   *  The guiding shape layer is used to store the guiding shapes of PCells
   */
  unsigned int guiding_shape_layer () const;

  /**
   *  @brief Gets the waste layer
   *
   *  The waste layer is used to store shapes that should not be visible and can be cleared at any time.
   */
  unsigned int waste_layer () const;

  /**
   *  @brief Gets the error layer
   *
   *  The error layer is used to display error messages.
   */
  unsigned int error_layer () const;

  /**
   *  @brief Sets the properties for a specified layer
   */
  void set_properties (unsigned int i, const LayerProperties &props);

  /**
   *  @brief Gets the properties for a specified layer
   */
  const LayerProperties &get_properties (unsigned int i) const
  {
    return m_layer_props [i];
  }

  /**
   *  @brief Collects memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const;

private:
  std::vector<unsigned int> m_free_indices;
  std::vector<LayerState> m_layer_states;
  std::vector<LayerProperties> m_layer_props;
  std::multimap<LayerProperties, unsigned int, db::LPLogicalLessFunc> m_layers_by_props;
  int m_guiding_shape_layer;
  int m_waste_layer;
  int m_error_layer;

  /**  
   *  @brief Insert a new layer
   *
   *  This creates a new index number, either from the free list
   *  of by creating a new one.
   */
  unsigned int do_insert_layer (bool special = false);

  /**  
   *  @brief Insert a new layer at the given index
   *
   *  If the index is unused, create a new layer there.
   */
  void do_insert_layer (unsigned int index, bool special = false);
};

}

#endif


