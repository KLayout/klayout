
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


#include "dbPCellHeader.h"
#include "dbPCellVariant.h"
#include "dbLayoutUtils.h"

namespace db
{

// ----------------------------------------------------------------------------------------
//  PCellParametersCompareFunc implementation

bool 
PCellParametersCompareFunc::operator() (const pcell_parameters_type *a, const pcell_parameters_type *b) const
{
  if (a->size () != b->size ()) {
    return a->size () < b->size ();
  }

  for (size_t i = 0; i < a->size (); ++i) {
    if ((*a)[i] < (*b)[i]) {
      return true;
    } else if ((*b)[i] < (*a)[i]) {
      return false;
    }
  }

  return false;
}
  
// ----------------------------------------------------------------------------------------
//  PCellHeader implementation

PCellHeader::PCellHeader (size_t pcell_id, const std::string &name, PCellDeclaration *declaration)
  : mp_declaration (declaration), m_pcell_id (pcell_id), m_name (name)
{
  if (mp_declaration) {
    mp_declaration->add_ref ();
  }
}

PCellHeader::PCellHeader (const PCellHeader &d)
  : mp_declaration (d.mp_declaration), m_pcell_id (d.m_pcell_id), m_name (d.m_name)
{
  if (mp_declaration) {
    mp_declaration->add_ref ();
  }
}

PCellHeader::~PCellHeader ()
{
  if (mp_declaration) {
    mp_declaration->release_ref ();
  }
  mp_declaration = 0;
}

void 
PCellHeader::declaration (PCellDeclaration *declaration)
{
  if (mp_declaration) {
    mp_declaration->release_ref ();
  }
  mp_declaration = declaration;
  if (declaration) {
    mp_declaration->add_ref ();
  }
}

std::vector<unsigned int> 
PCellHeader::get_layer_indices (db::Layout &layout, const db::pcell_parameters_type &parameters, db::ImportLayerMapping *layer_mapping)
{
  if (! declaration ()) {
    return std::vector<unsigned int> ();
  }

  db::DirectLayerMapping direct_layer_mapping (&layout);
  if (! layer_mapping) {
    layer_mapping = &direct_layer_mapping;
  }

  std::vector<db::PCellLayerDeclaration> layer_declarations = mp_declaration->get_layer_declarations (parameters);

  std::vector<unsigned int> layer_indices;
  layer_indices.reserve (layer_declarations.size ());
  for (size_t i = 0; i < layer_declarations.size (); ++i) {
    std::pair<bool, unsigned int> lm (false, 0);
    if (layer_declarations[i] != db::LayerProperties ()) {
      lm = layer_mapping->map_layer (layer_declarations[i]);
    }
    if (lm.first) {
      layer_indices.push_back (lm.second);
    } else {
      layer_indices.push_back (layout.waste_layer ());
    }
  }

  return layer_indices;
}

PCellVariant *
PCellHeader::get_variant (db::Layout & /*layout*/, const pcell_parameters_type &parameters)
{
  variant_map_t::iterator v = m_variant_map.find (&parameters);
  if (v != m_variant_map.end ()) {
    return v->second;
  } else {
    return 0;
  }
}

void 
PCellHeader::unregister_variant (PCellVariant *variant)
{
  variant_map_t::iterator v = m_variant_map.find (&variant->parameters ());
  tl_assert (v != m_variant_map.end ());
  m_variant_map.erase (v);
}

void 
PCellHeader::register_variant (PCellVariant *variant)
{
  variant_map_t::iterator v = m_variant_map.find (&variant->parameters ());
  tl_assert (v == m_variant_map.end ());
  m_variant_map.insert (std::make_pair (&variant->parameters (), variant));
}
  
}

