
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


#ifndef HDR_dbPCellVariant
#define HDR_dbPCellVariant

#include "dbCommon.h"

#include "dbLayout.h"
#include "tlVariant.h"
#include "dbPCellHeader.h"

#include <map>
#include <vector>
#include <string>

namespace db
{

/**
 *  @brief A PCell variant
 */
class DB_PUBLIC PCellVariant 
  : public db::Cell
{
public:
  /** 
   *  @brief The constructor
   *
   *  The constructor gets the parameters that are unique for this variant.
   */
  PCellVariant (db::cell_index_type ci, db::Layout &layout, db::pcell_id_type pcell_id, const pcell_parameters_type &parameters);

  /**
   *  @brief The destructor
   */
  ~PCellVariant ();

  /**
   *  @brief Cloning 
   */
  virtual Cell *clone (Layout &layout) const;

  /**
   *  @brief Gets the parameter name map for a parameter list
   */
  std::map<std::string, tl::Variant> parameters_by_name_from_list (const pcell_parameters_type &list) const;

  /**
   *  @brief Gets the parameter name map for this variant
   */
  std::map<std::string, tl::Variant> parameters_by_name () const;

  /**
   *  @brief Gets the parameter by name for this variant
   *  Returns a nil variant if there is no parameter with that name
   */
  tl::Variant parameter_by_name (const std::string &name) const;

  /**
   *  @brief Gets the parameters for this variant
   */
  const std::vector<tl::Variant> &parameters () const
  {
    return m_parameters;
  }

  /**
   *  @brief Get the PCell Id for this variant
   */
  db::pcell_id_type pcell_id () const
  {
    return m_pcell_id;
  }

  /**
   *  @brief Get the basic name
   *
   *  The basic name of the cell is either the cell name or the cell name in the
   *  target library (for library proxies) or the PCell name (for PCell proxies).
   *  The actual name may be different by a extension to make it unique.
   */
  virtual std::string get_basic_name () const;

  /**
   *  @brief Get the display name
   *
   *  The display name is some "nice" descriptive name of the cell (variant)
   *  For normal cells this name is equivalent to the normal cell name.
   */
  virtual std::string get_display_name () const;

  /**
   *  @brief Unregister a cell from it's context.
   */
  virtual void unregister ();

  /**
   *  @brief Reregister a cell inside it's context.
   */
  virtual void reregister ();

  /**
   *  @brief Update the layout
   */
  virtual void update (ImportLayerMapping *layer_mapping = 0);

  /**
   *  @brief Tell, if this cell is a proxy cell
   *
   *  Proxy cells are such whose layout represents a snapshot of another entity.
   *  Such cells can be PCell variants or library references for example.
   */
  virtual bool is_proxy () const 
  { 
    return true; 
  }

protected:
  /**
   *  @brief Get the PCell header for this variant
   */
  PCellHeader *pcell_header () 
  {
    return layout ()->pcell_header (m_pcell_id);
  }

  /**
   *  @brief Get the PCell header for this variant
   */
  const PCellHeader *pcell_header () const
  {
    return layout ()->pcell_header (m_pcell_id);
  }

private:
  pcell_parameters_type m_parameters;
  mutable std::string m_display_name;
  db::pcell_id_type m_pcell_id;
  bool m_registered;
};
  
}

#endif

