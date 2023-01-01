
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



#ifndef HDR_dbCellInst
#define HDR_dbCellInst

#include "dbCommon.h"

#include "dbBox.h"

namespace db
{

class Layout;

/**
 *  @brief The cell instance class
 *  This class does not form the "real" instance. It just provides the link to the
 *  cell. The transformation is added through the "db::array" framework. A db::CellInst
 *  within a db::array forms a db::CellInstArray which is the actual cell instance.
 */
class DB_PUBLIC CellInst
{
public:
  typedef db::Layout layout_type;
  typedef db::Box box_type;
  typedef Box::coord_type coord_type;

  typedef db::object_tag<db::CellInst> tag;

  /**
   *  @brief Default ctor
   */
  CellInst ()
    : m_cell_index (0)
  { }

  /**
   *  @brief Create a cell instance from the given index
   *  
   *  @param ci The cell index
   */
  CellInst (cell_index_type ci)
    : m_cell_index (ci)
  { }

  /**
   *  @brief The cell index accessor
   */
  cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief The cell index setter
   */
  void cell_index (cell_index_type ci) 
  {
    m_cell_index = ci;
  }

  /**
   *  @brief Compute the bounding box
   *
   *  This method computes the bbox of the cell instance.
   *  As a requirement, the cell's bounding box must have been
   *  computed before.
   */
  box_type bbox (const Layout &g) const;

  /**
   *  @brief Compute the bounding box
   *
   *  This method computes the bbox of the cell instance
   *  given a certain layer.
   *  As a requirement, the cell's bounding boxes must have been
   *  computed before.
   */
  box_type bbox (const Layout &g, unsigned int l) const;

  /**
   *  @brief Comparison: comparison for equality 
   */
  bool operator== (const CellInst &d) const
  {
    return m_cell_index == d.m_cell_index;
  }

  /**
   *  @brief Comparison: compare by cell id 
   *
   *  This sorting order is used by the cell instances of the 
   *  cell.
   */
  bool operator< (const CellInst &d) const
  {
    return m_cell_index < d.m_cell_index;
  }

  /**
   *  @brief Convert to a string
   */
  std::string to_string () const;

private:
  cell_index_type m_cell_index;
};

} // namespace db

#endif

