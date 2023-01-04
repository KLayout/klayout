
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


#ifndef HDR_dbPCellHeader
#define HDR_dbPCellHeader

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbPCellDeclaration.h"
#include "tlVariant.h"

#include <string.h>

namespace db
{

class PCellVariant;

struct DB_PUBLIC PCellParametersCompareFunc
{
  bool operator() (const pcell_parameters_type *a, const pcell_parameters_type *b) const;
};

/**
 *  @brief A PCell header
 *
 *  The PCell header manages the PCell variants along with the PCell declaration.
 */
class DB_PUBLIC PCellHeader
{
public:
  typedef std::map<const pcell_parameters_type *, db::PCellVariant *, PCellParametersCompareFunc> variant_map_t;
  typedef variant_map_t::const_iterator variant_iterator;

  /**
   *  @brief The default constructor
   */
  PCellHeader(size_t pcell_id, const std::string &name, PCellDeclaration *declaration);

  /**
   *  @brief The destructor
   */
  ~PCellHeader ();

  /**
   *  @brief Copy constructor
   */
  PCellHeader (const PCellHeader &d);

  /**
   *  @brief Get the name
   */
  const std::string &get_name () const
  {
    return m_name;
  }

  /**
   *  @brief Get the declaration
   */
  const PCellDeclaration *declaration () const
  {
    return mp_declaration;
  }

  /**
   *  @brief Set the declaration
   *
   *  The declaration can be 0, in which case the PCell cannot produce any code.
   */
  void declaration (PCellDeclaration *declaration);

  /**
   *  @brief Get the layer index list for this PCell declaration for the given parameter set
   */
  std::vector<unsigned int> get_layer_indices (db::Layout &layout, const pcell_parameters_type &parameters, db::ImportLayerMapping *layer_mapping = 0);

  /**
   *  @brief Get the variant for a given parameter set if it already exists.
   *
   *  If no variant is registered for this parameter set, 0 is returned.
   */
  PCellVariant *get_variant (db::Layout &layout, const pcell_parameters_type &parameters);

  /**
   *  @brief Register a variant 
   */
  void unregister_variant (PCellVariant *variant);

  /**
   *  @brief Unregister a variant 
   */
  void register_variant (PCellVariant *variant);

  /**
   *  @brief Iterates the variants (begin)
   */
  variant_iterator begin () const
  {
    return m_variant_map.begin ();
  }

  /**
   *  @brief Iterates the variants (end)
   */
  variant_iterator end () const
  {
    return m_variant_map.end ();
  }

  /**
   *  @brief Get the PCell Id for this variant
   */
  size_t pcell_id () const
  {
    return m_pcell_id;
  }

private:
  variant_map_t m_variant_map;
  db::PCellDeclaration *mp_declaration;
  size_t m_pcell_id;
  std::string m_name;
};
  
}

#endif

