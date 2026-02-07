
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


#include "dbInstElement.h"

namespace db
{

// ------------------------------------------------------------
//  "to_string" implementation

std::string
InstElement::to_string (bool resolve_cell_name) const
{
  if (inst_ptr.is_null ()) {
    return std::string ();
  }

  db::cell_index_type ci = inst_ptr.cell_index ();

  std::string r;
  if (resolve_cell_name && inst_ptr.instances () && inst_ptr.instances ()->cell () && inst_ptr.instances ()->cell ()->layout ()) {
    r = inst_ptr.instances ()->cell ()->layout ()->cell_name (ci);
  } else {
    r = "cell_index=" + tl::to_string (ci);
  }

  r += " " + complex_trans ().to_string ();

  return r;
}

// ------------------------------------------------------------
//  Implementation of "find_path"

static bool
find_path (const db::Layout &layout, db::cell_index_type from, db::cell_index_type to, std::set <db::cell_index_type> &visited, std::vector<db::InstElement> &path)
{
  const db::Cell &cell = layout.cell (from);
  for (db::Cell::parent_inst_iterator p = cell.begin_parent_insts (); ! p.at_end (); ++p) {

    if (p->parent_cell_index () == to) {

      path.push_back (db::InstElement (p->child_inst ()));
      return true;

    } else if (visited.find (p->parent_cell_index ()) == visited.end ()) {

      visited.insert (p->parent_cell_index ());
      path.push_back (db::InstElement (p->child_inst ()));
      if (find_path (layout, p->parent_cell_index (), to, visited, path)) {
        return true;
      }
      path.pop_back ();

    }

  }

  return false;
}

bool
find_path (const db::Layout &layout, db::cell_index_type from, db::cell_index_type to, std::vector<db::InstElement> &path)
{
  path.clear ();
  if (from == to) {
    return true;
  } else {
    std::set <db::cell_index_type> v;
    if (find_path (layout, from, to, v, path)) {
      std::reverse (path.begin (), path.end ());
      return true;
    } else {
      return false;
    }
  }
}

}
