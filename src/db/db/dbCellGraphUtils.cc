
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


#include "dbCellGraphUtils.h"

namespace db
{

// ---------------------------------------------------------------------
//  CellCounter implementation

CellCounter::CellCounter (const db::Layout *cell_graph)
  : m_cache (), mp_cell_graph (cell_graph)
{
  //  .. nothing yet ..
}

CellCounter::CellCounter (const db::Layout *cell_graph, db::cell_index_type starting_cell)
  : m_cache (), mp_cell_graph (cell_graph)
{
  cell_graph->cell (starting_cell).collect_called_cells (m_selection);
  m_selection.insert (starting_cell);
}

size_t 
CellCounter::weight (db::cell_index_type ci)
{
  cache_t::const_iterator c = m_cache.find (ci);

  if (c != m_cache.end ()) {
    return c->second;
  } else if (! m_selection.empty () && m_selection.find (ci) == m_selection.end ()) {
    return 0;
  } else {

    const db::Cell *cell = & mp_cell_graph->cell (ci);
    size_t count = 0;

    for (db::Cell::parent_inst_iterator p = cell->begin_parent_insts (); ! p.at_end (); ++p) {
      if (m_selection.empty () || m_selection.find (p->parent_cell_index ()) != m_selection.end ()) {
        count += weight (p->parent_cell_index ()) * p->child_inst ().size ();
      }
    }

    if (count == 0) {
      count = 1;  // top cells have multiplicity 1
    }

    m_cache.insert (std::make_pair (ci, count));
    return count;

  }
}

}

