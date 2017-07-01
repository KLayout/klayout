
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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
#include "layLayoutView.h"
#include "layApplication.h"
#include "layMainWindow.h"

#include "utHead.h"

TEST(1)
{
  db::Manager mgr;
  lay::LayoutView view (&mgr, lay::Application::instance ()->is_editable (), lay::MainWindow::instance ());

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

  std::pair<bool, db::DPoint> res;

  res = lay::obj_snap (&view, db::DPoint (0.505, 0.505), db::DVector (), 0.1);
  EXPECT_EQ (res.first, true);
  EXPECT_EQ (res.second.to_string (), "0.5,0.5");

  res = lay::obj_snap (&view, db::DPoint (0.505, 1.005), db::DVector (), 0.1);
  EXPECT_EQ (res.first, false);
  EXPECT_EQ (res.second.to_string (), "0.505,1.005");

  res = lay::obj_snap (&view, db::DPoint (0.005, 1.005), db::DVector (), 0.1);
  EXPECT_EQ (res.first, true);
  EXPECT_EQ (res.second.to_string (), "0,1");

  res = lay::obj_snap (&view, db::DPoint (1.000, 0.505), db::DPoint (0.505, 0.500), db::DVector (), lay::AC_Horizontal, 0.1);
  EXPECT_EQ (res.first, true);
  EXPECT_EQ (res.second.to_string (), "0.495,0.505");

}

// .. TODO: implement ..
