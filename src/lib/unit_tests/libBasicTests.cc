
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#include "tlUnitTest.h"

#include "dbLibrary.h"
#include "dbLibraryManager.h"

#include "libBasicCircle.h"

TEST(1) 
{
  const size_t p_layer = 0;
  const size_t p_radius = 1;
  const size_t p_handle = 2;
  const size_t p_npoints = 3;
  const size_t p_actual_radius = 4;
  const size_t p_total = 5;

  db::Library *lib_basic = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  tl_assert (lib_basic);

  db::Layout ly;

  std::pair<bool, db::pcell_id_type> pc = lib_basic->layout ().pcell_by_name ("CIRCLE");
  tl_assert (pc.first);

  std::map<std::string, tl::Variant> params;
  params["layer"] = db::LayerProperties (1, 0);
  params["actual_radius"] = 10.0;

  db::cell_index_type lib_cell;
  db::Cell *circle;
  std::vector<tl::Variant> plist;

  lib_cell = lib_basic->layout ().get_pcell_variant_dict (pc.second, params);
  circle = &ly.cell (ly.get_lib_proxy (lib_basic, lib_cell));

  //  change radius explicitly

  //  has radius 10um
  EXPECT_EQ (circle->bbox ().to_string (), "(-10000,-10000;10000,10000)");
  EXPECT_EQ (circle->get_display_name (), "Basic.CIRCLE(l=1/0,r=10,n=64)");

  //  only after Library::refresh the parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_radius].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (circle->bbox ().to_string (), "(-10000,-10000;10000,10000)");
  EXPECT_EQ (circle->get_display_name (), "Basic.CIRCLE(l=1/0,r=10,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_radius].to_double (), 10.0);
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 10.0);

  //  change radius explicitly

  plist = ly.get_pcell_parameters (circle->cell_index ());
  plist[p_radius] = 9.0;
  circle = &ly.cell (ly.get_pcell_variant_cell (circle->cell_index (), plist));

  //  only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 10.0);

  lib_basic->refresh ();
  EXPECT_EQ (circle->bbox ().to_string (), "(-9000,-9000;9000,9000)");
  EXPECT_EQ (circle->get_display_name (), "Basic.CIRCLE(l=1/0,r=9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 9.0);

  //  change handle explicitly

  plist = ly.get_pcell_parameters (circle->cell_index ());
  plist[p_handle] = db::DPoint (0.0, 8.0);
  circle = &ly.cell (ly.get_pcell_variant_cell (circle->cell_index (), plist));

  //  only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 9.0);

  lib_basic->refresh ();
  EXPECT_EQ (circle->bbox ().to_string (), "(-8000,-8000;8000,8000)");
  EXPECT_EQ (circle->get_display_name (), "Basic.CIRCLE(l=1/0,r=8,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_radius].to_double (), 8.0);
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 8.0);
}

