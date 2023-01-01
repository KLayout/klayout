
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


#ifndef HDR_dbFuzzyCellMapping
#define HDR_dbFuzzyCellMapping

#include "dbCommon.h"

#include "dbTypes.h"

#include <map>

namespace db
{

class Layout;

class DB_PUBLIC FuzzyCellMapping
{
public:
  typedef std::map <db::cell_index_type, db::cell_index_type>::const_iterator iterator;

  /**
   *  @brief Constructor - creates an empty mapping
   */
  FuzzyCellMapping ();

  /**
   *  @brief Clear the mapping
   */
  void clear ();

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   */
  void create (const db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b);

  /**
   *  @brief Determine cell mapping to a layout_b cell to the corresponding layout_a cell.
   *
   *  @param cell_index_b The index of the cell in layout_b whose mapping is requested.
   *  @return First: true, if a unique mapping is given, Second: the cell index in layout_a.
   */
  std::pair<bool, db::cell_index_type> cell_mapping_pair (db::cell_index_type cell_index_b) const;

  /**
   *  @brief Determine if a cell layout_b has a mapping to a layout_a cell.
   *
   *  @param cell_index_b The index of the cell in layout_b whose mapping is requested.
   *  @return true, if the cell has a mapping
   */
  bool has_mapping (db::cell_index_type cell_index_b) const;

  /**
   *  @brief Determine cell mapping to a layout_b cell to the corresponding layout_a cell.
   *
   *  @param cell_index_b The index of the cell in layout_b whose mapping is requested.
   *  @return the cell index in layout_a.
   */
  db::cell_index_type cell_mapping (db::cell_index_type cell_index_b) const;

  /**
   *  @brief Begin iterator for the b to a cell mapping
   */
  iterator begin () const
  {
    return m_b2a_mapping.begin ();
  }

  /**
   *  @brief End iterator for the b to a cell mapping
   */
  iterator end () const
  {
    return m_b2a_mapping.end ();
  }

private:
  void dump_mapping (const std::map <db::cell_index_type, std::vector<db::cell_index_type> > &candidates, 
                     const db::Layout &layout_a, const db::Layout &layout_b);

  std::map <db::cell_index_type, db::cell_index_type> m_b2a_mapping;
};

}

#endif


