
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

#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbFileBasedLibrary.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"

//  map library to different file (aka reload/refresh)
TEST(1) 
{
  std::string pa = tl::testsrc () + "/testdata/gds/lib_a.gds";
  std::string pb = tl::testsrc () + "/testdata/gds/lib_b.gds";

  db::FileBasedLibrary *lib = new db::FileBasedLibrary (pa);
  lib->load ();
  lib->set_name ("LIB");
  db::LibraryManager::instance ().register_lib (lib);

  db::Layout l;
  auto top_cell_index = l.add_cell ("TOP");
  auto lib_proxy = l.get_lib_proxy (lib, lib->layout ().cell_by_name ("A").second);

  l.cell (top_cell_index).insert (db::CellInstArray (db::CellInst (lib_proxy), db::Trans ()));

  EXPECT_EQ (l.cell (top_cell_index).bbox ().to_string (), "(0,0;2000,2000)");

  //  switch to a different file and refresh
  lib->set_paths (pb);
  lib->refresh ();

  EXPECT_EQ (l.cell (top_cell_index).bbox ().to_string (), "(-1000,-1000;1000,1000)");

  db::LibraryManager::instance ().delete_lib (lib);
}

//  merge multiple files into one library
TEST(2)
{
  std::string pa = tl::testsrc () + "/testdata/gds/lib_a.gds";
  std::string pb = tl::testsrc () + "/testdata/gds/lib_b.gds";

  db::FileBasedLibrary *lib = new db::FileBasedLibrary (pa);
  lib->merge_with_other_layout (pb);
  lib->load ();
  lib->set_name ("LIB");
  db::LibraryManager::instance ().register_lib (lib);

  {
    db::Layout l;
    auto top_cell_index = l.add_cell ("TOP");
    auto lib_proxy = l.get_lib_proxy (lib, lib->layout ().cell_by_name ("A").second);

    l.cell (top_cell_index).insert (db::CellInstArray (db::CellInst (lib_proxy), db::Trans ()));

    //  A comes from the first layout
    EXPECT_EQ (l.cell (top_cell_index).bbox ().to_string (), "(0,0;2000,2000)");
  }

  {
    db::Layout l;
    auto top_cell_index = l.add_cell ("TOP");
    auto lib_proxy = l.get_lib_proxy (lib, lib->layout ().cell_by_name ("Z").second);

    l.cell (top_cell_index).insert (db::CellInstArray (db::CellInst (lib_proxy), db::Trans ()));

    //  Z comes from the second layout
    EXPECT_EQ (l.cell (top_cell_index).bbox ().to_string (), "(0,0;100,100)");
  }

  db::LibraryManager::instance ().delete_lib (lib);
}
