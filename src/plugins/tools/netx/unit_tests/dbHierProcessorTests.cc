
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
#include "tlStream.h"
#include "dbHierProcessor.h"
#include "dbTestSupport.h"
#include "dbReader.h"
#include "dbCommonReader.h"

static std::string testdata (const std::string &fn)
{
  return tl::testsrc () + "/src/plugins/tools/netx/testdata/" + fn;
}

enum TestMode
{
  TMAnd = 0,
  TMNot = 1
};

/**
 *  @brief Turns a layer into polygons and polygon references
 *  The hierarchical processor needs polygon references and can't work on polygons directly.
 */
void normalize_layer (db::Layout &layout, unsigned int layer)
{
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes s;
    s.swap (c->shapes (layer));
    for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes); !i.at_end (); ++i) {
      db::Polygon poly;
      i->polygon (poly);
      c->shapes (layer).insert (db::PolygonRef (poly, layout.shape_repository ()));
    }
  }
}

void run_test_bool (tl::TestBase *_this, const char *file, TestMode mode, int out_layer_num)
{
  db::Layout layout_org;

  unsigned int l1 = 0, l2 = 0, lout = 0;
  db::LayerMap lmap;

  {
    tl::InputStream stream (testdata (file));
    db::Reader reader (stream);

    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (1, 0), l1 = layout_org.insert_layer ());
    layout_org.set_properties (l1, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (2, 0), l2 = layout_org.insert_layer ());
    layout_org.set_properties (l2, p);

    p.layer = out_layer_num;
    p.datatype = 0;
    lmap.map (db::LDPair (out_layer_num, 0), lout = layout_org.insert_layer ());
    layout_org.set_properties (lout, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;
    reader.read (layout_org, options);
  }

  layout_org.clear_layer (lout);
  normalize_layer (layout_org, l1);
  normalize_layer (layout_org, l2);

  db::BoolAndOrNotLocalOperation op (mode == TMAnd);
  db::LocalProcessor proc (&layout_org, &layout_org.cell (*layout_org.begin_top_down ()), &op, l1, l2, lout);
  proc.run ();

  db::compare_layouts (_this, layout_org, testdata (file), lmap, false /*skip other layers*/, db::AsPolygons);
}

TEST(BasicAnd1)
{
  //  Simple flat AND
  run_test_bool (_this, "hlp1.oas", TMAnd, 100);
}

TEST(BasicNot1)
{
  //  Simple flat NOT
  run_test_bool (_this, "hlp1.oas", TMNot, 101);
}

TEST(BasicAnd2)
{
  //  Up/down and down/up interactions, AND
  run_test_bool (_this, "hlp2.oas", TMAnd, 100);
}

TEST(BasicNot2)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool (_this, "hlp2.oas", TMNot, 101);
}

TEST(BasicAnd3)
{
  //  Variant building, AND
  run_test_bool (_this, "hlp3.oas", TMAnd, 100);
}

TEST(BasicNot3)
{
  //  Variant building, NOT
  run_test_bool (_this, "hlp3.oas", TMNot, 101);
}

TEST(BasicAnd4)
{
  //  Sibling interactions, variant building, AND
  run_test_bool (_this, "hlp4.oas", TMAnd, 100);
}

TEST(BasicNot4)
{
  //  Sibling interactions, variant building, NOT
  run_test_bool (_this, "hlp4.oas", TMNot, 101);
}

TEST(BasicAnd5)
{
  //  Variant building with intermediate hierarchy, AND
  run_test_bool (_this, "hlp5.oas", TMAnd, 100);
}

TEST(BasicNot5)
{
  //  Variant building with intermediate hierarchy, NOT
  run_test_bool (_this, "hlp5.oas", TMNot, 101);
}
