
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


#ifndef HDR_dbLibraryProxy
#define HDR_dbLibraryProxy

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbCell.h"

namespace db
{

/**
 *  @brief A cell specialization: a proxy for a library cell.
 *
 *  This cell serves as a proxy (or copy) of a cell contained in another library.
 *  This cell thus is a reference and a cached copy. 
 *  The library reference is through the library id and the cell index inside this
 *  library.
 */
class DB_PUBLIC LibraryProxy 
  : public Cell
{
public:
  /** 
   *  @brief The constructor
   *
   *  The constructor gets the parameters that are unique for this variant.
   */
  LibraryProxy (db::cell_index_type ci, db::Layout &layout, lib_id_type lib_id, cell_index_type cell_index);

  /**
   *  @brief The destructor
   */
  ~LibraryProxy ();

  /**
   *  @brief Cloning 
   */
  virtual Cell *clone (Layout &layout) const;

  /**
   *  @brief Get the library id 
   */
  lib_id_type lib_id () const
  {
    return m_lib_id;
  }

  /**
   *  @brief Get the cell index inside the library
   */
  cell_index_type library_cell_index () const
  {
    return m_library_cell_index;
  }

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

  /**
   *  @brief Gets the basic name
   *
   *  The basic name of the cell is either the cell name or the cell name in the
   *  target library (for library proxies) or the PCell name (for PCell proxies).
   *  The actual name may be different by a extension to make it unique.
   */
  virtual std::string get_basic_name () const;

  /**
   *  @brief Gets the display name
   *
   *  The display name is some "nice" descriptive name of the cell (variant)
   *  For normal cells this name is equivalent to the normal cell name.
   */
  virtual std::string get_display_name () const;

  /**
   *  @brief Gets the qualified name
   *
   *  The qualified name for a library proxy is made from the library name, a
   *  dot and the cell's name.
   */
  virtual std::string get_qualified_name () const;

  /**
   *  @brief Resets the binding of this proxy
   */
  void remap (lib_id_type lib_id, cell_index_type cell_index);

  /**
   *  @brief Reimplemented from Cell: unregisters the proxy at the layout
   */
  void unregister ();

  /**
   *  @brief Reimplemented from Cell: reregisters the proxy at the layout
   */
  void reregister ();

private:
  lib_id_type m_lib_id;
  cell_index_type m_library_cell_index;

  std::vector<int> get_layer_indices (db::Layout &layout, db::ImportLayerMapping *layer_mapping);
};

}

#endif


