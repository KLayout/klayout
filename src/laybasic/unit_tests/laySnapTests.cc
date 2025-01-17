
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "laySnap.h"
#include "layLayoutViewBase.h"

#include "tlUnitTest.h"

TEST(1)
{
  db::Manager mgr (true);
  lay::LayoutViewBase view (&mgr, is_editable (), 0);

  int cv1 = view.create_layout ("", true, false);
  db::Layout &ly1 = view.cellview (cv1)->layout ();
  db::Cell &top = ly1.cell (ly1.add_cell ("TOP"));
  unsigned int l1 = ly1.insert_layer (db::LayerProperties (1, 0));
  view.select_cell (0, top.cell_index ());

  lay::LayerPropertiesNode lp;
  lp.set_source ("1/0@1");
  view.insert_layer (view.begin_layers (), lp);

  db::Polygon poly;
  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (1000, 0),
    db::Point (0, 1000)
  };
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  top.shapes (l1).insert (poly);

  view.set_max_hier_levels (1);

  lay::PointSnapToObjectResult res;

  //  not hit
  res = lay::obj_snap (&view, db::DPoint (1.505, 1.505), db::DVector (), 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::NoObject);
  EXPECT_EQ (res.snapped_point.to_string (), "1.505,1.505");

  res = lay::obj_snap (&view, db::DPoint (0.505, 0.505), db::DVector (), 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::ObjectEdge);
  EXPECT_EQ (res.snapped_point.to_string (), "0.5,0.5");

  res = lay::obj_snap (&view, db::DPoint (0.485, 0.505), db::DVector (0.01, 0.01), 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::ObjectEdge);
  EXPECT_EQ (res.snapped_point.to_string (), "0.49,0.51");
  EXPECT_EQ (res.object_ref.to_string (), "(0,1;1,0)");

  res = lay::obj_snap (&view, db::DPoint (0.205, 0.215), db::DVector (0.01, 0.025), 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::NoObject);
  EXPECT_EQ (res.snapped_point.to_string (), "0.21,0.225");

  res = lay::obj_snap (&view, db::DPoint (0.505, 1.005), db::DVector (), 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::NoObject);
  EXPECT_EQ (res.snapped_point.to_string (), "0.505,1.005");

  res = lay::obj_snap (&view, db::DPoint (0.005, 1.005), db::DVector (), 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::ObjectVertex);
  EXPECT_EQ (res.snapped_point.to_string (), "0,1");

  res = lay::obj_snap (&view, db::DPoint (0.0, 1.005), db::DVector (), 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::ObjectVertex);
  EXPECT_EQ (res.snapped_point.to_string (), "0,1");

  res = lay::obj_snap (&view, db::DPoint (1.000, 0.505), db::DPoint (0.505, 0.500), db::DVector (), lay::AC_Horizontal, 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::ObjectEdge);
  EXPECT_EQ (res.snapped_point.to_string (), "0.495,0.505");

  //  projected snapping
  res = lay::obj_snap (&view, db::DPoint (1.000, 0.505), db::DPoint (0.005, 1.005), db::DVector (), lay::AC_Horizontal, 0.1);
  EXPECT_EQ (res.object_snap, lay::PointSnapToObjectResult::ObjectUnspecific);
  EXPECT_EQ (res.snapped_point.to_string (), "0,0.505");
  EXPECT_EQ (res.object_ref.to_string (), "(0,1;0,1)");

  lay::TwoPointSnapToObjectResult res2;

  res2 = lay::obj_snap2 (&view, db::DPoint (1.5, 1.5), db::DVector (), 0.005, 1.0);
  EXPECT_EQ (res2.any, false);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0;0,0)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.5), db::DVector (), 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0.3525,0.6475;0,0.295)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.5), db::DVector (), lay::AC_Horizontal, 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0.5;0.5,0.5)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.5), db::DVector (0.03, 0.03), lay::AC_Horizontal, 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0.51;0.49,0.51)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.5), db::DVector (), lay::AC_Vertical, 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0.205,0.795;0.205,0)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.5), db::DVector (), lay::AC_Diagonal, 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0.3525,0.6475;0,0.295)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.505), db::DVector (), lay::AC_Ortho, 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0.505;0.495,0.505)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.5), db::DVector (), lay::AC_Any, 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0.3525,0.6475;0,0.295)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.205, 0.495), db::DVector (0.01, 0.01), 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0.355,0.645;0,0.29)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.5, 0.5), db::DVector (), 0.005, 1.0);
  EXPECT_EQ (res2.any, false);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0;0,0)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.005, 0.5), db::DVector (), 0.005, 1.0);
  EXPECT_EQ (res2.any, true);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0.5;0.5,0.5)");

  res2 = lay::obj_snap2 (&view, db::DPoint (0.0, 0.5), db::DVector (), 0.005, 1.0);
  EXPECT_EQ (res2.any, false);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0;0,0)");

  res2 = lay::obj_snap2 (&view, db::DPoint (-0.2, 0.5), db::DVector (), 0.005, 1.0);
  EXPECT_EQ (res2.any, false);
  EXPECT_EQ (db::DEdge (res2.first, res2.second).to_string (), "(0,0;0,0)");

}

// .. TODO: implement ..
