
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


std::string contexts_to_s (db::Layout *layout, db::LocalProcessorContexts &contexts)
{
  std::string res;

  for (db::Layout::top_down_const_iterator i = layout->begin_top_down (); i != layout->end_top_down(); ++i) {
    db::LocalProcessorContexts::iterator cc = contexts.context_map ().find (&layout->cell (*i));
    if (cc != contexts.context_map ().end ()) {
      int index = 1;
      for (db::LocalProcessorCellContexts::iterator j = cc->second.begin (); j != cc->second.end (); ++j) {
        res += tl::sprintf ("%s[%d] %d insts, %d shapes (%d times)\n", layout->cell_name (*i), index, int (j->first.first.size ()), int (j->first.second.size ()), int (j->second.size ()));
        index += 1;
      }
    }
  }

  return res;
}

void run_test_bool (tl::TestBase *_this, const char *file, TestMode mode, int out_layer_num, db::Coord enl = 0, std::string *context_doc = 0)
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
  db::LocalProcessor proc (&layout_org, &layout_org.cell (*layout_org.begin_top_down ()));

  if (! context_doc) {
    proc.run (&op, l1, l2, lout);
  } else {
    db::LocalProcessorContexts contexts;
    proc.compute_contexts (contexts, &op, l1, l2, enl);
    *context_doc = contexts_to_s (&layout_org, contexts);
    proc.compute_results (contexts, &op, lout);
  }

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

TEST(BasicAnd6)
{
  //  Extreme variants (copy, vanishing), AND
  run_test_bool (_this, "hlp6.oas", TMAnd, 100);
}

TEST(BasicNot6)
{
  //  Extreme variants (copy, vanishing), NOT
  run_test_bool (_this, "hlp6.oas", TMNot, 101);
}

TEST(BasicAnd7)
{
  //  Context replication - direct and indirect, AND
  run_test_bool (_this, "hlp7.oas", TMAnd, 100);
}

TEST(BasicNot7)
{
  //  Context replication - direct and indirect, NOT
  run_test_bool (_this, "hlp7.oas", TMNot, 101);
}

TEST(BasicAnd8)
{
  //  Mixed sibling-parent contexts, AND
  run_test_bool (_this, "hlp8.oas", TMAnd, 100);
}

TEST(BasicNot8)
{
  //  Mixed sibling-parent contexts, NOT
  run_test_bool (_this, "hlp8.oas", TMNot, 101);
}

TEST(BasicAnd9)
{
  //  Top-level ring structure, AND
  std::string doc;
  run_test_bool (_this, "hlp9.oas", TMAnd, 100, 0, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 0 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 4 shapes (2 times)\n"
  );
}

TEST(BasicNot9)
{
  //  Top-level ring structure, NOT
  std::string doc;
  run_test_bool (_this, "hlp9.oas", TMNot, 101, 0, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 0 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 4 shapes (2 times)\n"
  );
}

TEST(BasicAnd10)
{
  //  Array instances, AND
  run_test_bool (_this, "hlp10.oas", TMAnd, 100);
}

TEST(BasicNot10)
{
  //  Array instances, NOT
  run_test_bool (_this, "hlp10.oas", TMNot, 101);
}

