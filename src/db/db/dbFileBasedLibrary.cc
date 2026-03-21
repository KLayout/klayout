
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


#include "dbFileBasedLibrary.h"
#include "dbReader.h"
#include "dbCellMapping.h"

#include "tlFileUtils.h"
#include "tlStream.h"

namespace db
{

// -------------------------------------------------------------------------------------------

FileBasedLibrary::FileBasedLibrary (const std::string &path, const std::string &name)
  : db::Library (), m_name (name), m_path (path), m_is_loaded (false)
{
  set_description (tl::filename (path));
}

void
FileBasedLibrary::merge_with_other_layout (const std::string &path)
{
  m_other_paths.push_back (path);
  if (m_is_loaded) {
    merge_impl (path);
  }
}

std::string
FileBasedLibrary::load ()
{
  if (! m_is_loaded) {
    return reload ();
  } else {
    return get_name ();
  }
}

std::string
FileBasedLibrary::reload ()
{
  std::string name = m_name.empty () ? tl::basename (m_path) : m_name;

  layout ().clear ();

  tl::InputStream stream (m_path);
  db::Reader reader (stream);
  reader.read (layout ());

  //  Use the libname if there is one
  if (m_name.empty ()) {
    db::Layout::meta_info_name_id_type libname_name_id = layout ().meta_info_name_id ("libname");
    for (db::Layout::meta_info_iterator m = layout ().begin_meta (); m != layout ().end_meta (); ++m) {
      if (m->first == libname_name_id && ! m->second.value.is_nil ()) {
        name = m->second.value.to_string ();
        break;
      }
    }
  }

  for (auto p = m_other_paths.begin (); p != m_other_paths.end (); ++p) {
    merge_impl (*p);
  }

  m_is_loaded = true;

  return name;
}

void
FileBasedLibrary::merge_impl (const std::string &path)
{
  db::Layout ly;

  tl::InputStream stream (path);
  db::Reader reader (stream);
  reader.read (ly);

  std::vector<db::cell_index_type> target_cells, source_cells;

  //  collect the cells to pull in (all top cells of the library layout)
  //  NOTE: cells are not overwritten - the first layout wins, in terms
  //  of cell names and also in terms of database unit.
  for (auto c = ly.begin_top_down (); c != ly.end_top_cells (); ++c) {
    std::string cn = ly.cell_name (*c);
    if (! layout ().has_cell (cn.c_str ())) {
      source_cells.push_back (*c);
      target_cells.push_back (layout ().add_cell (cn.c_str ()));
    }
  }

  db::CellMapping cm;
  cm.create_multi_mapping_full (layout (), target_cells, ly, source_cells);
  layout ().copy_tree_shapes (ly, cm);
}

}

