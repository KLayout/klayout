
/*

  KLayoutLayers LayoutLayers Viewer
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


#include "dbLayoutLayers.h"

namespace db
{

// -----------------------------------------------------------------
//  Implementation of the LayerIterator class

LayerIterator::LayerIterator (unsigned int layer_index, const db::LayoutLayers &layout)
  : m_layer_index (layer_index), m_layout (layout)
{
  while (m_layer_index < m_layout.layers () && m_layout.layer_state (m_layer_index) != db::LayoutLayers::Normal) {
    ++m_layer_index;
  }
}

LayerIterator &
LayerIterator::operator++() 
{
  do {
    ++m_layer_index;
  } while (m_layer_index < m_layout.layers () && m_layout.layer_state (m_layer_index) != db::LayoutLayers::Normal);

  return *this;
}

std::pair<unsigned int, const db::LayerProperties *> 
LayerIterator::operator*() const
{
  return std::pair<unsigned int, const db::LayerProperties *> (m_layer_index, &m_layout.get_properties (m_layer_index));
}

// -----------------------------------------------------------------
//  Implementation of the LayoutLayers class

LayoutLayers::LayoutLayers ()
  : m_guiding_shape_layer (-1),
    m_waste_layer (-1),
    m_error_layer (-1)
{
  // .. nothing yet ..
}

LayoutLayers::LayoutLayers (const db::LayoutLayers &layout)
  : m_guiding_shape_layer (-1),
    m_waste_layer (-1),
    m_error_layer (-1)
{
  *this = layout;
}

LayoutLayers::~LayoutLayers ()
{
  //  .. nothing yet ..
}

void
LayoutLayers::clear ()
{
  m_free_indices.clear ();
  m_layer_states.clear ();
  m_layer_props.clear ();
  m_layers_by_props.clear ();

  m_guiding_shape_layer = -1;
  m_waste_layer = -1;
  m_error_layer = -1;
}

LayoutLayers &
LayoutLayers::operator= (const LayoutLayers &d)
{
  if (&d != this) {

    m_guiding_shape_layer = d.m_guiding_shape_layer;
    m_waste_layer = d.m_waste_layer;
    m_error_layer = d.m_error_layer;

    m_free_indices = d.m_free_indices;
    m_layer_states = d.m_layer_states;
    m_layer_props = d.m_layer_props;
    m_layers_by_props = d.m_layers_by_props;

  }
  return *this;
}

void
LayoutLayers::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (!no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  db::mem_stat (stat, purpose, cat, m_free_indices, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_layer_states, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_layer_props, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_layers_by_props, true, (void *) this);
}

void 
LayoutLayers::delete_layer (unsigned int n)
{
  const db::LayerProperties &lp = m_layer_props [n];
  if (! lp.is_null ()) {
    for (auto i = m_layers_by_props.find (lp); i != m_layers_by_props.end () && i->first.log_equal (lp); ++i) {
      if (i->second == n) {
        m_layers_by_props.erase (i);
        break;
      }
    }
  }

  m_free_indices.push_back (n);
  m_layer_props [n] = db::LayerProperties ();
  m_layer_states [n] = Free;
}

unsigned int 
LayoutLayers::insert_layer (const LayerProperties &props)
{
  unsigned int i = do_insert_layer ();
  set_properties (i, props);
  return i;
}

void 
LayoutLayers::insert_layer (unsigned int index, const LayerProperties &props)
{
  if (layer_state (index) == Normal) {
    delete_layer (index);
  }
  do_insert_layer (index);
  set_properties (index, props);
}

int
LayoutLayers::get_layer_maybe (const db::LayerProperties &lp) const
{
  if (lp.is_null ()) {
    return -1;
  } else {
    auto i = m_layers_by_props.find (lp);
    if (i != m_layers_by_props.end () && i->first.log_equal (lp)) {
      return int (i->second);
    } else {
      return -1;
    }
  }
}

unsigned int
LayoutLayers::error_layer () const
{
  if (m_error_layer < 0) {
    //  create the error layer (since that layer is cached we can do
    //  this in a "const" fashion.
    db::LayoutLayers *self = const_cast<db::LayoutLayers *> (this);
    self->m_error_layer = (int) self->insert_special_layer (db::LayerProperties ("ERROR"));
  }

  return (unsigned int) m_error_layer;
}

unsigned int
LayoutLayers::waste_layer () const
{
  if (m_waste_layer < 0) {
    //  create the waste layer (since that layer is cached we can do
    //  this in a "const" fashion.
    db::LayoutLayers *self = const_cast<db::LayoutLayers *> (this);
    self->m_waste_layer = (int) self->insert_special_layer (db::LayerProperties ("WASTE"));
  }

  return (unsigned int) m_waste_layer;
}

unsigned int
LayoutLayers::guiding_shape_layer () const
{
  if (m_guiding_shape_layer < 0) {
    //  create the guiding shape layer (since that layer is cached we can do
    //  this in a "const" fashion.
    db::LayoutLayers *self = const_cast<db::LayoutLayers *> (this);
    self->m_guiding_shape_layer = (int) self->insert_special_layer (db::LayerProperties ("GUIDING_SHAPES"));
  }

  return (unsigned int) m_guiding_shape_layer;
}

unsigned int 
LayoutLayers::insert_special_layer (const LayerProperties &props)
{
  unsigned int i = do_insert_layer (true /*special*/);
  set_properties (i, props);
  return i;
}

void 
LayoutLayers::insert_special_layer (unsigned int index, const LayerProperties &props)
{
  if (layer_state (index) == Normal) {
    delete_layer (index);
  }

  do_insert_layer (index, true /*special*/);
  set_properties (index, props);
}

unsigned int 
LayoutLayers::do_insert_layer (bool special) 
{
  if (m_free_indices.size () > 0) {
    unsigned int i = m_free_indices.back ();
    m_free_indices.pop_back ();
    m_layer_states [i] = special ? Special : Normal;
    return i;
  } else {
    m_layer_states.push_back (special ? Special : Normal);
    unsigned int i = layers () - 1;
    return i;
  }
}

void 
LayoutLayers::do_insert_layer (unsigned int index, bool special) 
{
  if (index >= layers ()) {

    //  add layer to the end of the list.
    //  add as may freelist entries as required.
    while (index > layers ()) {
      m_free_indices.push_back (layers ());
      m_layer_states.push_back (Free);
    }
    m_layer_states.push_back (special ? Special : Normal);

  } else {

    tl_assert (m_layer_states [index] == Free);
    m_layer_states [index] = special ? Special : Normal;
  
  }

}

void 
LayoutLayers::reserve_layers (unsigned int n)
{
  m_layer_states.reserve (n);
}

void 
LayoutLayers::set_properties (unsigned int n, const LayerProperties &props)
{
  while (m_layer_props.size () <= n) {
    m_layer_props.push_back (LayerProperties ());
  }

  const db::LayerProperties &lp = m_layer_props [n];
  if (! lp.is_null ()) {
    for (auto i = m_layers_by_props.find (lp); i != m_layers_by_props.end () && i->first.log_equal (lp); ++i) {
      if (i->second == n) {
        m_layers_by_props.erase (i);
        break;
      }
    }
  }

  m_layer_props [n] = props;

  if (! props.is_null ()) {
    m_layers_by_props.insert (std::make_pair (props, n));
  }
}

}

