
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


#include "tlUnitTest.h"

#include "dbTexts.h"
#include "dbTextsUtils.h"
#include "dbEdges.h"
#include "dbRegion.h"
#include "dbTestSupport.h"

TEST(1) 
{
  db::Texts texts;
  EXPECT_EQ (texts.empty (), true);
  EXPECT_EQ (texts.bbox ().to_string (), "()");
  EXPECT_EQ (texts == db::Texts (), true);
  EXPECT_EQ (texts < db::Texts (), false);
  EXPECT_EQ (texts != db::Texts (), false);
  texts.insert (db::Text ("abc", db::Trans (db::Vector (100, -200))));
  EXPECT_EQ (texts.empty (), false);
  EXPECT_EQ (texts.count (), size_t (1));
  EXPECT_EQ (texts.hier_count (), size_t (1));
  EXPECT_EQ (texts.bbox ().to_string (), "(100,-200;100,-200)");
  EXPECT_EQ (texts.to_string (), "('abc',r0 100,-200)");

  texts.clear ();
  EXPECT_EQ (texts.empty (), true);
  EXPECT_EQ (texts.count (), size_t (0));
  EXPECT_EQ (texts.hier_count (), size_t (0));
  EXPECT_EQ (texts.bbox ().to_string (), "()");
  texts.insert (db::Text ("uvw", db::Trans (db::Vector (110, 210))));
  EXPECT_EQ (texts == db::Texts (), false);
  EXPECT_EQ (texts < db::Texts (), true);
  EXPECT_EQ (texts != db::Texts (), true);
  EXPECT_EQ (texts != texts, false);
  EXPECT_EQ (texts == texts, true);
  EXPECT_EQ (texts < texts, false);
  EXPECT_EQ (texts.empty (), false);
  EXPECT_EQ (texts.bbox ().to_string (), "(110,210;110,210)");
  EXPECT_EQ (texts.to_string (), "('uvw',r0 110,210)");

  EXPECT_EQ (texts.transformed (db::ICplxTrans (2.0, 0.0, false, db::Vector ())).to_string (), "('uvw',r0 220,420)");
  EXPECT_EQ (texts.to_string (), "('uvw',r0 110,210)");
  texts.transform (db::ICplxTrans (3));
  EXPECT_EQ (texts.empty (), false);
  EXPECT_EQ (texts.bbox ().to_string (), "(210,-110;210,-110)");
  EXPECT_EQ (texts.to_string (), "('uvw',r270 210,-110)");

  db::Texts texts2;
  EXPECT_EQ (texts2.empty (), true);
  EXPECT_EQ (texts2.count (), size_t (0));
  EXPECT_EQ (texts2.hier_count (), size_t (0));
  EXPECT_EQ (texts2.bbox ().to_string (), "()");
  texts2.swap (texts);
  EXPECT_EQ (texts.empty (), true);
  EXPECT_EQ (texts.count (), size_t (0));
  EXPECT_EQ (texts.hier_count (), size_t (0));
  EXPECT_EQ (texts.bbox ().to_string (), "()");
  EXPECT_EQ (texts2.empty (), false);
  EXPECT_EQ (texts2.count (), size_t (1));
  EXPECT_EQ (texts2.hier_count (), size_t (1));
  EXPECT_EQ (texts2.bbox ().to_string (), "(210,-110;210,-110)");
}

TEST(2) 
{
  db::Texts texts;
  texts.insert (db::Text ("abc", db::Trans (db::Vector (100, -200))));
  texts.insert (db::Text ("uvw", db::Trans (db::Vector (110, 210))));

  EXPECT_EQ (db::compare (texts, "('abc',r0 100,-200);('uvw',r0 110,210)"), true);

  db::Texts ee;
  std::string s = texts.to_string ();
  tl::Extractor ex (s.c_str ());
  EXPECT_EQ (ex.try_read (ee), true);
  EXPECT_EQ (db::compare (ee, "('abc',r0 100,-200);('uvw',r0 110,210)"), true);

  db::Edges e;
  texts.edges (e);
  EXPECT_EQ (db::compare (e, "(100,-200;100,-200);(110,210;110,210)"), true);

  db::Region r;
  texts.polygons (r);
  EXPECT_EQ (db::compare (r, "(99,-201;99,-199;101,-199;101,-201);(109,209;109,211;111,211;111,209)"), true);
}

TEST(3) 
{
  db::Texts texts;
  texts.insert (db::Text ("abc", db::Trans (db::Vector (100, -200))));
  texts.insert (db::Text ("uvw", db::Trans (db::Vector (110, 210))));

  db::Texts tcopy = texts;

  db::TextStringFilter f ("abc", false);
  EXPECT_EQ (texts.filtered (f).to_string (), "('abc',r0 100,-200)");
  texts.filter (f);
  EXPECT_EQ (texts.to_string (), "('abc',r0 100,-200)");

  texts = tcopy;

  db::TextStringFilter fi ("abc", true);
  EXPECT_EQ (texts.filtered (fi).to_string (), "('uvw',r0 110,210)");
  texts.filter (fi);
  EXPECT_EQ (texts.to_string (), "('uvw',r0 110,210)");
}

TEST(4)
{
  db::Texts texts;
  texts.insert (db::Text ("abc", db::Trans (db::Vector (100, -200))));
  texts.insert (db::Text ("uvw", db::Trans (db::Vector (110, 210))));

  db::Texts tcopy = texts;

  db::TextPatternFilter f ("*v*", false);
  EXPECT_EQ (texts.filtered (f).to_string (), "('uvw',r0 110,210)");
  texts.filter (f);
  EXPECT_EQ (texts.to_string (), "('uvw',r0 110,210)");

  texts = tcopy;

  db::TextPatternFilter fi ("*v*", true);
  EXPECT_EQ (texts.filtered (fi).to_string (), "('abc',r0 100,-200)");
  texts.filter (fi);
  EXPECT_EQ (texts.to_string (), "('abc',r0 100,-200)");
}

TEST(5)
{
  db::Texts texts;
  texts.insert (db::Text ("abc", db::Trans (db::Vector (100, -200))));
  texts.insert (db::Text ("uvw", db::Trans (db::Vector (110, 210))));

  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  db::cell_index_type top_cell = ly.add_cell ("TOP");

  texts.insert_into_as_polygons (&ly, top_cell, l1, 1);

  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (top_cell), l1));
  EXPECT_EQ (db::compare (r, "(99,-201;99,-199;101,-199;101,-201);(109,209;109,211;111,211;111,209)"), true);
}

TEST(6)
{
  db::Texts texts;
  texts.insert (db::Text ("abc", db::Trans (db::Vector (100, -200))));
  texts.insert (db::Text ("uvw", db::Trans (db::Vector (110, 210))));

  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  db::cell_index_type top_cell = ly.add_cell ("TOP");

  texts.insert_into (&ly, top_cell, l1);

  db::Texts r (db::RecursiveShapeIterator (ly, ly.cell (top_cell), l1));
  EXPECT_EQ (db::compare (r, "('abc',r0 100,-200);('uvw',r0 110,210)"), true);
}

TEST(7)
{
  db::Texts texts;
  texts.insert (db::Text ("abc", db::Trans (db::Vector (100, -200))));
  texts.insert (db::Text ("uvw", db::Trans (db::Vector (110, 210))));

  db::Region region;
  region.insert (db::Polygon (db::Box (50, -300, 150, -100)));

  EXPECT_EQ (texts.selected_interacting (region).to_string (), "('abc',r0 100,-200)");
  EXPECT_EQ (texts.selected_not_interacting (region).to_string (), "('uvw',r0 110,210)");

  {
    db::Texts tcopy = texts;
    tcopy.select_interacting (region);
    EXPECT_EQ (tcopy.to_string (), "('abc',r0 100,-200)");
  }

  {
    db::Texts tcopy = texts;
    tcopy.select_not_interacting (region);
    EXPECT_EQ (tcopy.to_string (), "('uvw',r0 110,210)");
  }

  db::Region region_out;
  texts.pull_interacting (region_out, region);
  EXPECT_EQ (region_out.to_string (), "(50,-300;50,-100;150,-100;150,-300)");
}
