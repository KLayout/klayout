
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
#include "dbReader.h"
#include "dbLayout.h"
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

TEST(problematic_polygon2)
{
  db::Point contour[] = {
    db::Point (-2100, 200),
    db::Point (-2100, 2000),
    db::Point (-500, 2000),
    db::Point (-500, 1700),
    db::Point (-849, 1700),
    db::Point (-947, 1690),
    db::Point (-1043, 1671),
    db::Point (-1137, 1643),
    db::Point (-1228, 1605),
    db::Point (-1315, 1559),
    db::Point (-1396, 1504),
    db::Point (-1472, 1442),
    db::Point (-1542, 1372),
    db::Point (-1604, 1296),
    db::Point (-1659, 1215),
    db::Point (-1705, 1128),
    db::Point (-1743, 1037),
    db::Point (-1771, 943),
    db::Point (-1790, 847),
    db::Point (-1800, 749),
    db::Point (-1800, 200)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  db::plc::ConvexDecompositionParameters param;
  param.with_segments = false;
  param.split_edges = false;
  param.tri_param.max_area = 1000000;
  param.tri_param.min_b = 0.5;

  db::plc::Graph plc;
  TestableConvexDecomposition decomp (&plc);

  decomp.decompose (poly, param, dbu);

  std::unique_ptr<db::Layout> ly (plc.to_layout ());
  db::compare_layouts (_this, *ly, tl::testdata () + "/algo/hm_decomposition_au6.gds");
}

TEST(polygon_with_holes)
{
  db::Layout ly;
  tl::InputStream s (tl::testdata () + "/algo/hm_decomposition_7.gds");
  db::Reader reader (s);
  reader.read (ly);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  const db::Cell &top = ly.cell (*ly.begin_top_down ());

  db::Region r (db::RecursiveShapeIterator (ly, top, l1));
  r.merge ();
  db::Polygon poly = *r.begin ();

  double dbu = 0.001;

  db::plc::ConvexDecompositionParameters param;
  param.with_segments = false;
  param.split_edges = false;
  param.tri_param.max_area = 1000000;
  param.tri_param.min_b = 0.5;

  db::plc::Graph plc;
  TestableConvexDecomposition decomp (&plc);

  decomp.decompose (poly, param, dbu);

  std::unique_ptr<db::Layout> ly_out (plc.to_layout ());
  db::compare_layouts (_this, *ly_out, tl::testdata () + "/algo/hm_decomposition_au7.gds");
}

