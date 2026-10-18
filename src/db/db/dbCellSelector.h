
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


#ifndef HDR_dbCellSelector
#define HDR_dbCellSelector

#include "dbCommon.h"
#include "dbTypes.h"
#include "tlString.h"

#include <set>
#include <string>
#include <limits>

namespace db
{

class Layout;

/**
 *  @brief A cell selector based on a on/off scheme during hierarchy traversal
 *
 *  The idea is to turn some "enabled" state on or off while traversing from
 *  parent to child cells. For this, the cell selector has two sets:
 *  on set of cells that become enabled (selected) and one set of cells that become
 *  disabled (unselected).
 *
 *  When a child cell is in the selected set, the enable flag is turned on and
 *  subcells of that cell are considered enabled too. When a child cell is in the
 *  unselected set, the enabled flag is turned off. When a child cell is
 *  none of these sets, the enable flag remains unchanged.
 *
 *  This class provides the two sets and
 */
class DB_PUBLIC CellSelector
{
public:
  /**
   *  @brief Default constructor
   */
  CellSelector ();

  /**
   *  @brief Copy constructor
   */
  CellSelector (const CellSelector &) = default;

  /**
   *  @brief Assignment
   */
  CellSelector &operator= (const CellSelector &) = default;

  /**
   *  @brief Returns a value indicating whether the cell selector does not select or unselect anything
   */
  bool empty () const;

  /**
   *  @brief Returns a value indicating whether the cell selector selects all cells
   */
  bool selects_all () const;

  /**
   *  @brief Returns a value indicating whether the cell selector unselects all cells
   */
  bool unselects_all () const;

  /**
   *  @brief Select cells
   *
   *  If no specific cells have been selected before, this method will confine the selection
   *  to the given cells (plus their sub-hierarchy).
   *  If cells have been selected before, this will add the given cells to the selection.
   */
  void select_cells (const std::set<db::cell_index_type> &cells);

  /**
   *  @brief Select a cell
   *
   *  If no specific cells have been selected before, this method will confine the selection
   *  to the given cell (plus their sub-hierarchy).
   *  If cells have been selected before, this will add this cell to the selection.
   */
  void select_cell (db::cell_index_type cell);

  /**
   *  @brief Select all cells
   *
   *  Makes all cells selected. After doing so, all unselect_cells calls
   *  will unselect only that specific cell without children.
   */
  void select_all_cells ();

  /**
   *  @brief Unselect cells
   *
   *  This method will remove the given cells (plus their sub-hierarchy) from the selection.
   */
  void unselect_cells (const std::set<db::cell_index_type> &cells);

  /**
   *  @brief Unselect a cell
   *
   *  This method will remove the given cell (plus their sub-hierarchy) from the selection.
   */
  void unselect_cell (db::cell_index_type cell);

  /**
   *  @brief Unselect all cells
   *
   *  Makes all cells unselected. After doing so, select_cells calls
   *  will select only that specific cell without children.
   */
  void unselect_all_cells ();

  /**
   *  @brief Resets the selection
   *
   *  This will reset all selections and unselections.
   *  After calling this methods, all select_cells will again select the cells
   *  including their children.
   */
  void reset_selection ();

  /**
   *  @brief Gets a value indicating if a cell is selected or unselected
   *
   *  This method will return 1, if the cell is in the "selected" set.
   *  It returns -1, if the cell is in the "unselected" set.
   *  If the cell is neither in the selected or unselected set, it
   *  will return 0.
   */
  int in_selection (db::cell_index_type cell_index) const;

  /**
   *  @brief Converts the selection to a string
   *
   *  If the layout is given, cell names will be used instead of
   *  cell indexes.
   */
  std::string to_string (const db::Layout *layout = 0) const;

  /**
   *  @brief Reads the selection from a string
   *
   *  Returns true, if the selection could be parsed successfully.
   *
   *  "top_cell" is the index of the cell taken for top cell (i.e. plain "-" or "+").
   *
   *  The format is a whitespace-separated sequence of:
   *    * +<filter>: selects cells
   *    * -<filter>: unselects cells
   *    * +-<filter>: removes cells from "all selected" (used internally)
   *    * --<filter>: removes cells from "all unselected" (used internally)
   *    * <filter> can be:
   *      - <glob> or "<glob>": a glob pattern
   *      - '<name>': a plain cell name
   *      - #<n>: a cell by cell index
   *      - @: the top cell, if one is given
   */
  bool parse (tl::Extractor &ex, const db::Layout *layout = 0, db::cell_index_type top_cell = std::numeric_limits<db::cell_index_type>::max ());

private:
  std::set<db::cell_index_type> m_start, m_stop;
  std::set<db::cell_index_type> m_start_all_except, m_stop_all_except;
  bool m_start_all, m_stop_all;

  void clear (bool start_all, bool stop_all);
  void select_or_unselect_cell_exclude (bool select, db::cell_index_type cell_index, bool exclude);
};

}  // namespace db

#endif
