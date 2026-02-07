
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


#include "tlUnitTest.h"

#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbReader.h"
#include "dbTestSupport.h"

#include "libBasicCircle.h"

TEST(1_Circle)
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
  plist[p_actual_radius] = 9.0;
  circle = &ly.cell (ly.get_pcell_variant_cell (circle->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_radius].to_double (), 10.0);

  lib_basic->refresh ();
  EXPECT_EQ (circle->bbox ().to_string (), "(-9000,-9000;9000,9000)");
  EXPECT_EQ (circle->get_display_name (), "Basic.CIRCLE(l=1/0,r=9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 9.0);

  //  change handle explicitly

  plist = ly.get_pcell_parameters (circle->cell_index ());
  plist[p_handle] = db::DPoint (0.0, 8.0);
  circle = &ly.cell (ly.get_pcell_variant_cell (circle->cell_index (), plist));

  //  as the handle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 9.0);

  lib_basic->refresh ();
  EXPECT_EQ (circle->bbox ().to_string (), "(-8000,-8000;8000,8000)");
  EXPECT_EQ (circle->get_display_name (), "Basic.CIRCLE(l=1/0,r=8,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_radius].to_double (), 8.0);
  EXPECT_EQ (ly.get_pcell_parameters (circle->cell_index ()) [p_actual_radius].to_double (), 8.0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Shapes shapes;
  db::Shape s = shapes.insert (db::Box (1000, 2000, 4000, 5000));
  EXPECT_EQ (lib_basic->layout ().pcell_declaration (pc.second)->can_create_from_shape (ly, s, l1), true);
  EXPECT_EQ (lib_basic->layout ().pcell_declaration (pc.second)->transformation_from_shape (ly, s, l1).to_string (), "r0 2500,3500");
  plist = lib_basic->layout ().pcell_declaration (pc.second)->parameters_from_shape (ly, s, l1);
  EXPECT_EQ (plist [p_layer].to_string (), "1/0");
  EXPECT_EQ (plist [p_actual_radius].to_string (), "1.5");
}

TEST(2_Pie)
{
  static size_t p_layer = 0;
  static size_t p_radius = 1;
  static size_t p_start_angle = 2;
  static size_t p_end_angle = 3;
  static size_t p_handle1 = 4;
  static size_t p_handle2 = 5;
  static size_t p_npoints = 6;
  static size_t p_actual_radius = 7;
  static size_t p_actual_start_angle = 8;
  static size_t p_actual_end_angle = 9;
  static size_t p_actual_handle1 = 10;
  static size_t p_actual_handle2 = 11;
  static size_t p_total = 12;

  db::Library *lib_basic = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  tl_assert (lib_basic);

  db::Layout ly;

  std::pair<bool, db::pcell_id_type> pc = lib_basic->layout ().pcell_by_name ("PIE");
  tl_assert (pc.first);

  std::map<std::string, tl::Variant> params;
  params["layer"] = db::LayerProperties (1, 0);
  params["actual_radius"] = 10.0;
  params["actual_start_angle"] = -90.0;
  params["actual_end_angle"] = 0.0;

  db::cell_index_type lib_cell;
  db::Cell *pie;
  std::vector<tl::Variant> plist;

  lib_cell = lib_basic->layout ().get_pcell_variant_dict (pc.second, params);
  pie = &ly.cell (ly.get_lib_proxy (lib_basic, lib_cell));

  //  change radius explicitly

  //  has radius 10um, but bbox isn't correct for now (because handle was not updated)
  EXPECT_EQ (pie->bbox ().to_string (), "(-1000,-10000;10000,1000)");
  EXPECT_EQ (pie->get_display_name (), "Basic.PIE(l=1/0,r=10,a=-90..0,n=64)");

  //  only after Library::refresh the parameters get updated and bbox is correct
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (pie->bbox ().to_string (), "(0,-10000;10000,0)");
  EXPECT_EQ (pie->get_display_name (), "Basic.PIE(l=1/0,r=10,a=-90..0,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 10.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_radius].to_double (), 10.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_end_angle].to_double (), 0);

  //  change radius explicitly

  plist = ly.get_pcell_parameters (pie->cell_index ());
  plist[p_actual_radius] = 9.0;
  pie = &ly.cell (ly.get_pcell_variant_cell (pie->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 10.0);

  lib_basic->refresh ();
  EXPECT_EQ (pie->bbox ().to_string (), "(0,-9000;9000,0)");
  EXPECT_EQ (pie->get_display_name (), "Basic.PIE(l=1/0,r=9,a=-90..0,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle1].to_string (), "0,-9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle1].to_string (), "0,-9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_end_angle].to_double (), 0);

  //  change end angle explicitly

  plist = ly.get_pcell_parameters (pie->cell_index ());
  plist[p_actual_end_angle] = 90.0;
  pie = &ly.cell (ly.get_pcell_variant_cell (pie->cell_index (), plist));

  //  as the end angle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (pie->bbox ().to_string (), "(0,-9000;9000,9000)");
  EXPECT_EQ (pie->get_display_name (), "Basic.PIE(l=1/0,r=9,a=-90..90,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle1].to_string (), "0,-9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle1].to_string (), "0,-9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_end_angle].to_double (), 90);

  //  change start angle explicitly

  plist = ly.get_pcell_parameters (pie->cell_index ());
  plist[p_actual_start_angle] = 0.0;
  pie = &ly.cell (ly.get_pcell_variant_cell (pie->cell_index (), plist));

  //  as the end angle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), -90.0);

  lib_basic->refresh ();
  EXPECT_EQ (pie->bbox ().to_string (), "(0,0;9000,9000)");
  EXPECT_EQ (pie->get_display_name (), "Basic.PIE(l=1/0,r=9,a=0..90,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle1].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle1].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_radius].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_start_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_end_angle].to_double (), 90);

  //  change handle1 explicitly

  plist = ly.get_pcell_parameters (pie->cell_index ());
  plist[p_actual_handle1] = db::DPoint (0.0, -5.0);
  pie = &ly.cell (ly.get_pcell_variant_cell (pie->cell_index (), plist));

  //  as the handle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (pie->bbox ().to_string (), "(0,-9000;9000,9000)");
  EXPECT_EQ (pie->get_display_name (), "Basic.PIE(l=1/0,r=9,a=-90..90,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 9);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_radius].to_double (), 9);

  //  change handle2 explicitly

  plist = ly.get_pcell_parameters (pie->cell_index ());
  plist[p_actual_handle2] = db::DPoint (5.0, 0.0);
  pie = &ly.cell (ly.get_pcell_variant_cell (pie->cell_index (), plist));

  //  as the handle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 90.0);

  lib_basic->refresh ();
  EXPECT_EQ (pie->bbox ().to_string (), "(0,-5000;5000,0)");
  EXPECT_EQ (pie->get_display_name (), "Basic.PIE(l=1/0,r=5,a=-90..0,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_handle2].to_string (), "5,0");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_handle2].to_string (), "5,0");
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_radius].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (pie->cell_index ()) [p_actual_radius].to_double (), 5);
}

TEST(3_Arc)
{
  static size_t p_layer = 0;
  static size_t p_radius1 = 1;
  static size_t p_radius2 = 2;
  static size_t p_start_angle = 3;
  static size_t p_end_angle = 4;
  static size_t p_handle1 = 5;
  static size_t p_handle2 = 6;
  static size_t p_npoints = 7;
  static size_t p_actual_radius1 = 8;
  static size_t p_actual_radius2 = 9;
  static size_t p_actual_start_angle = 10;
  static size_t p_actual_end_angle = 11;
  static size_t p_actual_handle1 = 12;
  static size_t p_actual_handle2 = 13;
  static size_t p_total = 14;

  db::Library *lib_basic = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  tl_assert (lib_basic);

  db::Layout ly;

  std::pair<bool, db::pcell_id_type> pc = lib_basic->layout ().pcell_by_name ("ARC");
  tl_assert (pc.first);

  std::map<std::string, tl::Variant> params;
  params["layer"] = db::LayerProperties (1, 0);
  params["actual_radius1"] = 4.0;
  params["actual_radius2"] = 10.0;
  params["actual_start_angle"] = -90.0;
  params["actual_end_angle"] = 0.0;

  db::cell_index_type lib_cell;
  db::Cell *arc;
  std::vector<tl::Variant> plist;

  lib_cell = lib_basic->layout ().get_pcell_variant_dict (pc.second, params);
  arc = &ly.cell (ly.get_lib_proxy (lib_basic, lib_cell));

  //  change radius explicitly

  //  has radius 10um, but bbox isn't correct for now (because handle was not updated)
  EXPECT_EQ (arc->bbox ().to_string (), "(0,-10000;10000,1000)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=4..10,a=-90..0,n=64)");

  //  only after Library::refresh the parameters get updated and bbox is correct
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (arc->bbox ().to_string (), "(0,-10000;10000,0)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=4..10,a=-90..0,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 10.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius2].to_double (), 10.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_end_angle].to_double (), 0);

  //  change radius2 explicitly

  plist = ly.get_pcell_parameters (arc->cell_index ());
  plist[p_actual_radius2] = 9.0;
  arc = &ly.cell (ly.get_pcell_variant_cell (arc->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 10.0);

  lib_basic->refresh ();
  EXPECT_EQ (arc->bbox ().to_string (), "(0,-9000;9000,0)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=4..9,a=-90..0,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle1].to_string (), "0,-4");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle1].to_string (), "0,-4");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_end_angle].to_double (), 0);

  //  change radius1 explicitly

  plist = ly.get_pcell_parameters (arc->cell_index ());
  plist[p_actual_radius1] = 5.0;
  arc = &ly.cell (ly.get_pcell_variant_cell (arc->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 4.0);

  lib_basic->refresh ();
  EXPECT_EQ (arc->bbox ().to_string (), "(0,-9000;9000,0)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=5..9,a=-90..0,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_end_angle].to_double (), 0);

  //  change end angle explicitly

  plist = ly.get_pcell_parameters (arc->cell_index ());
  plist[p_actual_end_angle] = 90.0;
  arc = &ly.cell (ly.get_pcell_variant_cell (arc->cell_index (), plist));

  //  as the end angle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (arc->bbox ().to_string (), "(0,-9000;9000,9000)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=5..9,a=-90..90,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_start_angle].to_double (), -90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_end_angle].to_double (), 90);

  //  change start angle explicitly

  plist = ly.get_pcell_parameters (arc->cell_index ());
  plist[p_actual_start_angle] = 0.0;
  arc = &ly.cell (ly.get_pcell_variant_cell (arc->cell_index (), plist));

  //  as the end angle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), -90.0);

  lib_basic->refresh ();
  EXPECT_EQ (arc->bbox ().to_string (), "(0,0;9000,9000)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=5..9,a=0..90,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle1].to_string (), "5,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle1].to_string (), "5,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_start_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_end_angle].to_double (), 90);

  //  change handle1 explicitly

  plist = ly.get_pcell_parameters (arc->cell_index ());
  plist[p_actual_handle1] = db::DPoint (0.0, -5.0);
  arc = &ly.cell (ly.get_pcell_variant_cell (arc->cell_index (), plist));

  //  as the handle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (arc->bbox ().to_string (), "(0,-9000;9000,9000)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=5..9,a=-90..90,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle2].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_end_angle].to_double (), 90);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 9);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius2].to_double (), 9);

  //  change handle2 explicitly

  plist = ly.get_pcell_parameters (arc->cell_index ());
  plist[p_actual_handle2] = db::DPoint (9.0, 0.0);
  arc = &ly.cell (ly.get_pcell_variant_cell (arc->cell_index (), plist));

  //  as the handle is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 90.0);

  lib_basic->refresh ();
  EXPECT_EQ (arc->bbox ().to_string (), "(0,-9000;9000,0)");
  EXPECT_EQ (arc->get_display_name (), "Basic.ARC(l=1/0,r=5..9,a=-90..0,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_start_angle].to_double (), -90.0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_end_angle].to_double (), 0);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_radius2].to_double (), 9);
  EXPECT_EQ (ly.get_pcell_parameters (arc->cell_index ()) [p_actual_radius2].to_double (), 9);
}

TEST(4_Donut)
{
  static size_t p_layer = 0;
  static size_t p_radius1 = 1;
  static size_t p_radius2 = 2;
  static size_t p_handle1 = 3;
  static size_t p_handle2 = 4;
  static size_t p_npoints = 5;
  static size_t p_actual_radius1 = 6;
  static size_t p_actual_radius2 = 7;
  static size_t p_total = 8;

  db::Library *lib_basic = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  tl_assert (lib_basic);

  db::Layout ly;

  std::pair<bool, db::pcell_id_type> pc = lib_basic->layout ().pcell_by_name ("DONUT");
  tl_assert (pc.first);

  std::map<std::string, tl::Variant> params;
  params["layer"] = db::LayerProperties (1, 0);
  params["actual_radius1"] = 4.0;
  params["actual_radius2"] = 10.0;
  params["actual_start_angle"] = -90.0;
  params["actual_end_angle"] = 0.0;

  db::cell_index_type lib_cell;
  db::Cell *donut;
  std::vector<tl::Variant> plist;

  lib_cell = lib_basic->layout ().get_pcell_variant_dict (pc.second, params);
  donut = &ly.cell (ly.get_lib_proxy (lib_basic, lib_cell));

  //  change radius explicitly

  //  has radius 10um, but bbox isn't correct for now (because handle was not updated)
  EXPECT_EQ (donut->bbox ().to_string (), "(-10000,-10000;10000,10000)");
  EXPECT_EQ (donut->get_display_name (), "Basic.DONUT(l=1/0,r=4..10,n=64)");

  //  only after Library::refresh the parameters get updated and bbox is correct
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius1].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (donut->bbox ().to_string (), "(-10000,-10000;10000,10000)");
  EXPECT_EQ (donut->get_display_name (), "Basic.DONUT(l=1/0,r=4..10,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius2].to_double (), 10.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius2].to_double (), 10.0);

  //  change radius2 explicitly

  plist = ly.get_pcell_parameters (donut->cell_index ());
  plist[p_actual_radius2] = 9.0;
  donut = &ly.cell (ly.get_pcell_variant_cell (donut->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius2].to_double (), 10.0);

  lib_basic->refresh ();
  EXPECT_EQ (donut->bbox ().to_string (), "(-9000,-9000;9000,9000)");
  EXPECT_EQ (donut->get_display_name (), "Basic.DONUT(l=1/0,r=4..9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle1].to_string (), "-4,0");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle2].to_string (), "-9,0");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius1].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius2].to_double (), 9.0);

  //  change radius1 explicitly

  plist = ly.get_pcell_parameters (donut->cell_index ());
  plist[p_actual_radius1] = 5.0;
  donut = &ly.cell (ly.get_pcell_variant_cell (donut->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius1].to_double (), 4.0);

  lib_basic->refresh ();
  EXPECT_EQ (donut->bbox ().to_string (), "(-9000,-9000;9000,9000)");
  EXPECT_EQ (donut->get_display_name (), "Basic.DONUT(l=1/0,r=5..9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle1].to_string (), "-5,0");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle2].to_string (), "-9,0");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius1].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius2].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius2].to_double (), 9.0);

  //  change handle1 explicitly

  plist = ly.get_pcell_parameters (donut->cell_index ());
  plist[p_handle1] = db::DPoint (0.0, -5.0);
  donut = &ly.cell (ly.get_pcell_variant_cell (donut->cell_index (), plist));

  lib_basic->refresh ();
  EXPECT_EQ (donut->bbox ().to_string (), "(-9000,-9000;9000,9000)");
  EXPECT_EQ (donut->get_display_name (), "Basic.DONUT(l=1/0,r=5..9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle2].to_string (), "-9,0");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius2].to_double (), 9);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius2].to_double (), 9);

  //  change handle2 explicitly

  plist = ly.get_pcell_parameters (donut->cell_index ());
  plist[p_handle2] = db::DPoint (9.0, 0.0);
  donut = &ly.cell (ly.get_pcell_variant_cell (donut->cell_index (), plist));

  lib_basic->refresh ();
  EXPECT_EQ (donut->bbox ().to_string (), "(-9000,-9000;9000,9000)");
  EXPECT_EQ (donut->get_display_name (), "Basic.DONUT(l=1/0,r=5..9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle1].to_string (), "0,-5");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_handle2].to_string (), "9,0");
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius1].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_radius2].to_double (), 9);
  EXPECT_EQ (ly.get_pcell_parameters (donut->cell_index ()) [p_actual_radius2].to_double (), 9);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Shapes shapes;
  db::Shape s = shapes.insert (db::Box (1000, 2000, 3000, 5000));
  EXPECT_EQ (lib_basic->layout ().pcell_declaration (pc.second)->can_create_from_shape (ly, s, l1), true);
  EXPECT_EQ (lib_basic->layout ().pcell_declaration (pc.second)->transformation_from_shape (ly, s, l1).to_string (), "r0 2000,3500");
  plist = lib_basic->layout ().pcell_declaration (pc.second)->parameters_from_shape (ly, s, l1);
  EXPECT_EQ (plist [p_layer].to_string (), "1/0");
  EXPECT_EQ (plist [p_actual_radius1].to_string (), "1");
  EXPECT_EQ (plist [p_actual_radius2].to_string (), "0.5");
}

TEST(5_Ellipse)
{
  static size_t p_layer = 0;
  static size_t p_radius_x = 1;
  static size_t p_radius_y = 2;
  static size_t p_handle_x = 3;
  static size_t p_handle_y = 4;
  static size_t p_npoints = 5;
  static size_t p_actual_radius_x = 6;
  static size_t p_actual_radius_y = 7;
  static size_t p_total = 8;

  db::Library *lib_basic = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  tl_assert (lib_basic);

  db::Layout ly;

  std::pair<bool, db::pcell_id_type> pc = lib_basic->layout ().pcell_by_name ("ELLIPSE");
  tl_assert (pc.first);

  std::map<std::string, tl::Variant> params;
  params["layer"] = db::LayerProperties (1, 0);
  params["actual_radius_x"] = 4.0;
  params["actual_radius_y"] = 10.0;
  params["actual_start_angle"] = -90.0;
  params["actual_end_angle"] = 0.0;

  db::cell_index_type lib_cell;
  db::Cell *ellipse;
  std::vector<tl::Variant> plist;

  lib_cell = lib_basic->layout ().get_pcell_variant_dict (pc.second, params);
  ellipse = &ly.cell (ly.get_lib_proxy (lib_basic, lib_cell));

  //  change radius explicitly

  //  has radius 10um, but bbox isn't correct for now (because handle was not updated)
  EXPECT_EQ (ellipse->bbox ().to_string (), "(-4000,-10000;4000,10000)");
  EXPECT_EQ (ellipse->get_display_name (), "Basic.ELLIPSE(l=1/0,rx=4,ry=10,n=64)");

  //  only after Library::refresh the parameters get updated and bbox is correct
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_x].to_double (), 0.0);

  lib_basic->refresh ();
  EXPECT_EQ (ellipse->bbox ().to_string (), "(-4000,-10000;4000,10000)");
  EXPECT_EQ (ellipse->get_display_name (), "Basic.ELLIPSE(l=1/0,rx=4,ry=10,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_x].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_x].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_y].to_double (), 10.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_y].to_double (), 10.0);

  //  change radius2 explicitly

  plist = ly.get_pcell_parameters (ellipse->cell_index ());
  plist[p_actual_radius_y] = 9.0;
  ellipse = &ly.cell (ly.get_pcell_variant_cell (ellipse->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_y].to_double (), 10.0);

  lib_basic->refresh ();
  EXPECT_EQ (ellipse->bbox ().to_string (), "(-4000,-9000;4000,9000)");
  EXPECT_EQ (ellipse->get_display_name (), "Basic.ELLIPSE(l=1/0,rx=4,ry=9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_x].to_string (), "-4,0");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_y].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_x].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_x].to_double (), 4.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_y].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_y].to_double (), 9.0);

  //  change radius1 explicitly

  plist = ly.get_pcell_parameters (ellipse->cell_index ());
  plist[p_actual_radius_x] = 5.0;
  ellipse = &ly.cell (ly.get_pcell_variant_cell (ellipse->cell_index (), plist));

  //  as the radius is an input parameter, only after Library::refresh the other parameters get updated
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_x].to_double (), 4.0);

  lib_basic->refresh ();
  EXPECT_EQ (ellipse->bbox ().to_string (), "(-5000,-9000;5000,9000)");
  EXPECT_EQ (ellipse->get_display_name (), "Basic.ELLIPSE(l=1/0,rx=5,ry=9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_x].to_string (), "-5,0");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_y].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_x].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_x].to_double (), 5.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_y].to_double (), 9.0);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_y].to_double (), 9.0);

  //  change handle1 explicitly

  plist = ly.get_pcell_parameters (ellipse->cell_index ());
  plist[p_handle_x] = db::DPoint (-5.0, 0.0);
  ellipse = &ly.cell (ly.get_pcell_variant_cell (ellipse->cell_index (), plist));

  lib_basic->refresh ();
  EXPECT_EQ (ellipse->bbox ().to_string (), "(-5000,-9000;5000,9000)");
  EXPECT_EQ (ellipse->get_display_name (), "Basic.ELLIPSE(l=1/0,rx=5,ry=9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_x].to_string (), "-5,0");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_y].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_x].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_x].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_y].to_double (), 9);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_y].to_double (), 9);

  //  change handle2 explicitly

  plist = ly.get_pcell_parameters (ellipse->cell_index ());
  plist[p_handle_y] = db::DPoint (0.0, 9.0);
  ellipse = &ly.cell (ly.get_pcell_variant_cell (ellipse->cell_index (), plist));

  lib_basic->refresh ();
  EXPECT_EQ (ellipse->bbox ().to_string (), "(-5000,-9000;5000,9000)");
  EXPECT_EQ (ellipse->get_display_name (), "Basic.ELLIPSE(l=1/0,rx=5,ry=9,n=64)");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_x].to_string (), "-5,0");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_handle_y].to_string (), "0,9");
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_x].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_x].to_double (), 5);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_radius_y].to_double (), 9);
  EXPECT_EQ (ly.get_pcell_parameters (ellipse->cell_index ()) [p_actual_radius_y].to_double (), 9);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Shapes shapes;
  db::Shape s = shapes.insert (db::Box (1000, 2000, 3000, 5000));
  EXPECT_EQ (lib_basic->layout ().pcell_declaration (pc.second)->can_create_from_shape (ly, s, l1), true);
  EXPECT_EQ (lib_basic->layout ().pcell_declaration (pc.second)->transformation_from_shape (ly, s, l1).to_string (), "r0 2000,3500");
  plist = lib_basic->layout ().pcell_declaration (pc.second)->parameters_from_shape (ly, s, l1);
  EXPECT_EQ (plist [p_layer].to_string (), "1/0");
  EXPECT_EQ (plist [p_actual_radius_x].to_string (), "1");
  EXPECT_EQ (plist [p_actual_radius_y].to_string (), "1.5");
}

//  regeneration of PCells after reading
TEST(10)
{
  db::Layout ly;

  {
    std::string fn (tl::testdata ());
    fn += "/gds/basic_instances.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Library *lib_basic = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  tl_assert (lib_basic);
  lib_basic->refresh ();

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/gds/basic_instances_au.gds");
}
