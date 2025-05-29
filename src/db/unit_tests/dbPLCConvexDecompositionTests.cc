
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


#include "dbPLCConvexDecomposition.h"
#include "dbWriter.h"
#include "dbRegionProcessors.h"
#include "dbTestSupport.h"
#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

#include <set>
#include <vector>
#include <cstdlib>
#include <cmath>

class TestableConvexDecomposition
  : public db::plc::ConvexDecomposition
{
public:
  using db::plc::ConvexDecomposition::ConvexDecomposition;
};

TEST(basic)
{
  db::plc::Graph plc;
  TestableConvexDecomposition decomp (&plc);

  db::Point contour[] = {
    db::Point (0, 0),
    db::Point (0, 100),
    db::Point (1000, 100),
    db::Point (1000, 500),
    db::Point (1100, 500),
    db::Point (1100, 100),
    db::Point (2100, 100),
    db::Point (2100, 0)
  };

  db::Point contour2[] = {
    db::Point (4000, 0),
    db::Point (4000, 100),
    db::Point (5000, 100),
    db::Point (5000, 500),
    db::Point (5100, 500),
    db::Point (5100, 100),
    db::Point (6100, 100),
    db::Point (6100, -1000),
    db::Point (4150, -1000),
    db::Point (4150, 0)
  };

  db::Region region;

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));
  region.insert (poly);

  poly.clear ();
  poly.assign_hull (contour2 + 0, contour2 + sizeof (contour2) / sizeof (contour2[0]));
  region.insert (poly);

  double dbu = 0.001;

  db::plc::ConvexDecompositionParameters param;
  decomp.decompose (region, param, dbu);

  std::unique_ptr<db::Layout> ly (plc.to_layout ());
  db::compare_layouts (_this, *ly, tl::testdata () + "/algo/hm_decomposition_au1.gds");

  param.with_segments = true;
  param.split_edges = false;
  decomp.decompose (region, param, dbu);

  ly.reset (plc.to_layout ());
  db::compare_layouts (_this, *ly, tl::testdata () + "/algo/hm_decomposition_au2.gds");

  param.with_segments = false;
  param.split_edges = true;
  decomp.decompose (region, param, dbu);

  ly.reset (plc.to_layout ());
  db::compare_layouts (_this, *ly, tl::testdata () + "/algo/hm_decomposition_au3.gds");

  param.with_segments = true;
  param.split_edges = true;
  decomp.decompose (region, param, dbu);

  ly.reset (plc.to_layout ());
  db::compare_layouts (_this, *ly, tl::testdata () + "/algo/hm_decomposition_au4.gds");
}

TEST(internal_vertex)
{
  db::plc::Graph plc;
  TestableConvexDecomposition decomp (&plc);

  db::Point contour[] = {
    db::Point (0, 0),
    db::Point (0, 100),
    db::Point (1000, 100),
    db::Point (1000, 0)
  };

  std::vector<db::Point> vertexes;
  vertexes.push_back (db::Point (0, 50));  // on edge
  vertexes.push_back (db::Point (200, 70));
  vertexes.push_back (db::Point (0, 0));  //  on vertex

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  db::plc::ConvexDecompositionParameters param;
  decomp.decompose (poly, vertexes, param, dbu);

  EXPECT_EQ (plc.begin () == plc.end (), false);
  if (plc.begin () == plc.end ()) {
    return;
  }

  auto p = plc.begin ();
  EXPECT_EQ (p->polygon ().to_string (), "(0,0;0,0.05;0,0.1;1,0.1;1,0)");

  std::vector<std::string> ip;
  for (size_t i = 0; i < p->internal_vertexes (); ++i) {
    ip.push_back (p->internal_vertex (i)->to_string () + "#" + tl::join (p->internal_vertex (i)->ids ().begin (), p->internal_vertex (i)->ids ().end (), ","));
  }
  std::sort (ip.begin (), ip.end ());
  EXPECT_EQ (tl::join (ip, "/"), "(0, 0)#2/(0, 0.05)#0/(0.2, 0.07)#1");

  EXPECT_EQ (++p == plc.end (), true);
}

TEST(problematic_polygon)
{
  db::Point contour[] = {
    db::Point (14590, 990),
    db::Point (6100, 990),
    db::Point (7360, 4450),
    db::Point (2280, 4450),
    db::Point (2280, 6120),
    db::Point (7360, 6120),
    db::Point (8760, 7490),
    db::Point (13590, 17100),
    db::Point (10280, 6120),
    db::Point (26790, 13060),
    db::Point (41270, 970)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  db::plc::ConvexDecompositionParameters param;
  param.with_segments = true;
  param.split_edges = false;

  db::plc::Graph plc;
  TestableConvexDecomposition decomp (&plc);

  decomp.decompose (poly, param, dbu);

  std::unique_ptr<db::Layout> ly (plc.to_layout ());
  db::compare_layouts (_this, *ly, tl::testdata () + "/algo/hm_decomposition_au5.gds");
}

