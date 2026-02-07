
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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



#ifndef HDR_dbCellInstanceSetHasher
#define HDR_dbCellInstanceSetHasher

#include "dbCommon.h"
#include "dbMatrix.h"
#include "dbLayout.h"

namespace db
{

/**
 *  @brief A hasher for a set of cell instances
 *
 *  The hasher starts with a layout, a top cell and optionally a
 *  set of cells selected. Only selected cells will be considered
 *  in the cell tree (the "cone").
 *
 *  The hasher allows to compute a hash value for a given cell,
 *  representative for the flat set of instances of that cell in
 *  the top cell.
 */
class DB_PUBLIC CellInstanceSetHasher
{
public:
  class DB_PUBLIC MatrixHash
    : public IMatrix3d
  {
  public:
    MatrixHash (double s = 1.0);
    MatrixHash (const db::ICplxTrans &trans);
    MatrixHash (const db::CellInstArray &array);

    size_t hash_value () const;
  };

  /**
   *  @brief Creates a new cell instance set hasher
   *
   *  @param layout The layout the hasher refers to
   *  @param top_cell The top cell the hasher starts with
   *  @param selection A set of selected cells or a null pointer if all cells should be considered
   *
   *  The hasher will not take ownership over the layout, nor
   *  the selected cell set.
   */
  CellInstanceSetHasher (const db::Layout *layout, db::cell_index_type top_cell, const std::set<db::cell_index_type> *selection = 0);

  /**
   *  @brief Computes the hash value representative for the flat instance set of the given cell in the top cell and the selection
   */
  size_t instance_set_hash (db::cell_index_type for_cell);

private:
  const db::Layout *mp_layout;
  db::cell_index_type m_top_cell;
  const std::set<db::cell_index_type> *mp_selection;
  std::map<db::cell_index_type, MatrixHash> m_cache;

  MatrixHash get_hash (db::cell_index_type for_cell);
  MatrixHash get_hash_uncached (db::cell_index_type for_cell);
};

}  // namespace db

#endif

