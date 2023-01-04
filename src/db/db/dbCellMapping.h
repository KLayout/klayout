
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



#ifndef HDR_dbCellMapping
#define HDR_dbCellMapping

#include "dbCommon.h"

#include "dbTypes.h"

#include <map>
#include <vector>
#include <set>

namespace db
{

class Layout;

/**
 *  @brief A cell mapping
 *
 *  The cell mapping is basically a table that lists corresponding cells in a layout "A"
 *  for cells in layout "B". "B" is considered the source layout while "A" is the target layout.
 *
 *  Cell mappings can be created using a strict geometrical mapping algorithm or 
 *  using name equivalence.
 *
 *  Cell mappings play a role in copy and compare operations.
 */
class DB_PUBLIC CellMapping
{
public:
  typedef std::map <db::cell_index_type, db::cell_index_type>::const_iterator iterator;

  /**
   *  @brief Constructor - creates an empty mapping
   */
  CellMapping ();

  /**
   *  @brief Clear the mapping
   */
  void clear ();

  /**
   *  @brief Swaps the cell mapping with another one
   */
  void swap (CellMapping &other);

  /**
   *  @brief Create a single cell mapping 
   *
   *  A single cell mapping will not map the child cells. When used for hierarchical copy this will basically 
   *  flatten the cell on copy.
   */
  void create_single_mapping (const db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b);

  /**
   *  @brief Create a single cell full mapping
   *
   *  This will map the given cell plus create new cells for the child cells of cell_b in layout_a. The
   *  instance tree will be copied over from layout_b to layout_a.
   */
  std::vector<db::cell_index_type> create_single_mapping_full (db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b)
  {
    create_single_mapping (layout_a, cell_index_a, layout_b, cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cell_index_b);
  }

  /**
   *  @brief Create a multi-cell mapping
   *
   *  The mapping created by this function will map the cells of cell_indexes_b to cell_indexes_a. Both arrays need to have
   *  the same length.
   *  This cell mapping will not map the child cells. When used for hierarchical copy this will basically
   *  flatten the cells on copy.
   */
  void create_multi_mapping (const db::Layout &layout_a, const std::vector<db::cell_index_type> &cell_indexes_a, const db::Layout &layout_b, const std::vector<db::cell_index_type> &cell_indexes_b);

  /**
   *  @brief Create a single cell full mapping
   *
   *  The mapping created by this function will map the cells of cell_indexes_a to cell_indexes_b. Both arrays need to have
   *  the same length. In addition, new cells for the child cells of all cell_indexes_b cells are created in layout_a and
   *  included in the mapping.
   */
  std::vector<db::cell_index_type> create_multi_mapping_full (db::Layout &layout_a, const std::vector<db::cell_index_type> &cell_indexes_a, const db::Layout &layout_b, const std::vector<db::cell_index_type> &cell_indexes_b)
  {
    create_multi_mapping (layout_a, cell_indexes_a, layout_b, cell_indexes_b);
    return create_missing_mapping (layout_a, layout_b, cell_indexes_b);
  }

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   */
  void create_from_geometry (const db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b);

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   *
   *  This version creates a full mapping, i.e. cells with no direct corresponding cell in layout A are created there plus
   *  their instances are copied over.
   *
   *  @return A list of cell indexes of the newly created cells.
   */
  std::vector<db::cell_index_type> create_from_geometry_full (db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b)
  {
    create_from_geometry (layout_a, cell_index_a, layout_b, cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cell_index_b);
  }

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   */
  void create_from_names (const db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b);

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   *
   *  This version creates a full mapping, i.e. cells with no direct corresponding cell in layout A are created there plus
   *  their instances are copied over.
   *
   *  @return A list of cell indexes of the newly created cells.
   */
  std::vector<db::cell_index_type> create_from_names_full (db::Layout &layout_a, db::cell_index_type cell_index_a, const db::Layout &layout_b, db::cell_index_type cell_index_b)
  {
    create_from_names (layout_a, cell_index_a, layout_b, cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cell_index_b);
  }

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
   *  @brief Add a cell mapping
   *
   *  @param cell_index_b The index of the cell in layout_a (the source of the mapping)
   *  @param cell_index_a The index of the cell in layout_a (the target of the mapping)
   */
  void map (db::cell_index_type cell_index_b, db::cell_index_type cell_index_a)
  {
    m_b2a_mapping.insert (std::make_pair (cell_index_b, 0)).first->second = cell_index_a;
  }

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

  /**
   *  @brief Gets the source cells
   */
  std::vector<db::cell_index_type> source_cells () const;

  /**
   *  @brief Access to the mapping table
   */
  const std::map <db::cell_index_type, db::cell_index_type> &table () const 
  {
    return m_b2a_mapping;
  }

  /**
   *  @brief Creates mappings for all cells not mapped yet
   *
   *  When constructing a cell mapping by explicit mapping (map (a, b)), some child cells of the
   *  mapped other cells may be left unmapped.
   *  This method allows creating mappings for these missing cells by adding
   *  new cells and the corresponding instances into the target layout_a.
   *
   *  If given, "exclude_cells" can specify a list of cells not to map.
   *
   *  If given, "include_cells" can specify a list of cells which are included in the
   *  cell tree to create. Cells not in the "include_cells" list are ignored.
   *
   *  The returned vector lists the new cells.
   */
  std::vector<db::cell_index_type> create_missing_mapping (db::Layout &layout_a, const db::Layout &layout_b, const std::vector<db::cell_index_type> &cell_index_b, const std::set<db::cell_index_type> *exclude_cells = 0, const std::set<db::cell_index_type> *include_cells = 0);

  /**
   *  @brief A convenience version of create_missing_mappings which takes a single cell for cell_index_a and cell_index_b.
   */
  std::vector<db::cell_index_type> create_missing_mapping (db::Layout &layout_a, const db::Layout &layout_b, db::cell_index_type cell_index_b, const std::set<db::cell_index_type> *exclude_cells = 0, const std::set<db::cell_index_type> *include_cells = 0)
  {
    std::vector<db::cell_index_type> cib;
    cib.reserve (1);
    cib.push_back (cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cib, exclude_cells, include_cells);
  }

  /**
   *  @brief Like create_missing_mapping, but returns the newly mapped pairs
   *
   *  The first cell index of the pair is the old cell in layout_a, the second cell index
   *  is the new cell in layout_b.
   */
  std::vector<std::pair<db::cell_index_type, db::cell_index_type> > create_missing_mapping2 (db::Layout &layout_a, const db::Layout &layout_b, const std::vector<db::cell_index_type> &cell_index_b, const std::set<db::cell_index_type> *exclude_cells = 0, const std::set<db::cell_index_type> *include_cells = 0);

  /**
   *  @brief Like create_missing_mapping, but returns the newly mapped pairs (single-cell version)
   *
   *  The first cell index of the pair is the old cell in layout_a, the second cell index
   *  is the new cell in layout_b.
   */
  std::vector<std::pair<db::cell_index_type, db::cell_index_type> > create_missing_mapping2 (db::Layout &layout_a, const db::Layout &layout_b, db::cell_index_type cell_index_b, const std::set<db::cell_index_type> *exclude_cells = 0, const std::set<db::cell_index_type> *include_cells = 0)
  {
    std::vector<db::cell_index_type> cib;
    cib.reserve (1);
    cib.push_back (cell_index_b);
    return create_missing_mapping2 (layout_a, layout_b, cib, exclude_cells, include_cells);
  }

private:
  void extract_unique (std::map <db::cell_index_type, std::vector<db::cell_index_type> >::const_iterator cand, 
                       std::map<db::cell_index_type, db::cell_index_type> &unique_mapping,
                       const db::Layout &layout_a, const db::Layout &layout_b);

  void dump_mapping (const std::map <db::cell_index_type, std::vector<db::cell_index_type> > &candidates, 
                     const db::Layout &layout_a, const db::Layout &layout_b);

  void do_create_missing_mapping (db::Layout &layout_a, const db::Layout &layout_b, const std::vector<db::cell_index_type> &cell_index_b, const std::set<db::cell_index_type> *exclude_cells, const std::set<db::cell_index_type> *include_cells, std::vector<db::cell_index_type> *new_cells, std::vector<std::pair<db::cell_index_type, db::cell_index_type> > *mapped_pairs);

  std::map <db::cell_index_type, db::cell_index_type> m_b2a_mapping;
};

}

#endif


