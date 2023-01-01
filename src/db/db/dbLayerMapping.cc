
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


#include "dbLayout.h"
#include "dbLayerMapping.h"

namespace db
{

// -------------------------------------------------------------------------------------
//  LayerMapping implementation

LayerMapping::LayerMapping ()
{
  // .. nothing yet ..
}

void LayerMapping::clear ()
{
  m_b2a_mapping.clear ();
}

void 
LayerMapping::create (const db::Layout &layout_a, const db::Layout &layout_b)
{
  clear ();

  if (&layout_a == &layout_b) {

    for (db::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      m_b2a_mapping.insert (std::make_pair ((*la).first, (*la).first));
    }

  } else {

    std::map<db::LayerProperties, unsigned int, db::LPLogicalLessFunc> layers;

    for (db::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      if (! (*la).second->is_null ()) {
        layers.insert (std::make_pair (*(*la).second, (*la).first));
      }
    }

    for (db::Layout::layer_iterator lb = layout_b.begin_layers (); lb != layout_b.end_layers (); ++lb) {
      if (! (*lb).second->is_null ()) {
        std::map<db::LayerProperties, unsigned int, db::LPLogicalLessFunc>::const_iterator l = layers.find (*(*lb).second);
        if (l != layers.end ()) {
          m_b2a_mapping.insert (std::make_pair ((*lb).first, l->second));
        }
      }
    }

  }
}

std::vector<unsigned int> 
LayerMapping::create_full (db::Layout &layout_a, const db::Layout &layout_b)
{
  clear ();

  std::vector<unsigned int> new_layers;

  if (&layout_a == &layout_b) {

    for (db::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      m_b2a_mapping.insert (std::make_pair ((*la).first, (*la).first));
    }

  } else {

    std::map<db::LayerProperties, unsigned int, db::LPLogicalLessFunc> layers;

    for (db::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      if (! (*la).second->is_null ()) {
        layers.insert (std::make_pair (*(*la).second, (*la).first));
      }
    }

    for (db::Layout::layer_iterator lb = layout_b.begin_layers (); lb != layout_b.end_layers (); ++lb) {
      if (! (*lb).second->is_null ()) {
        std::map<db::LayerProperties, unsigned int, db::LPLogicalLessFunc>::const_iterator l = layers.find (*(*lb).second);
        if (l != layers.end ()) {
          m_b2a_mapping.insert (std::make_pair ((*lb).first, l->second));
        } else {
          unsigned int nl = layout_a.insert_layer (*(*lb).second);
          new_layers.push_back (nl);
          m_b2a_mapping.insert (std::make_pair ((*lb).first, nl));
        }
      }
    }

  }

  return new_layers;
}

std::pair<bool, unsigned int> 
LayerMapping::layer_mapping_pair (unsigned int layer_b) const
{
  std::map <unsigned int, unsigned int>::const_iterator m = m_b2a_mapping.find (layer_b);
  if (m == m_b2a_mapping.end ()) {
    return std::make_pair (false, 0);
  } else {
    return std::make_pair (true, m->second);
  }
}

bool 
LayerMapping::has_mapping (unsigned int layer_b) const
{
  std::map <unsigned int, unsigned int>::const_iterator m = m_b2a_mapping.find (layer_b);
  return (m != m_b2a_mapping.end ());
}

unsigned int
LayerMapping::layer_mapping (unsigned int layer_b) const
{
  std::map <unsigned int, unsigned int>::const_iterator m = m_b2a_mapping.find (layer_b);
  tl_assert (m != m_b2a_mapping.end ());
  return m->second;
}

}

