
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
  return tl::testsrc () + "/testdata/algo/" + fn;
}

enum TestMode
{
  TMAnd = 0,
  TMNot = 1,
  TMAndSwapped = 2,
  TMNotSwapped = 3,
  TMSelfOverlap = 4
};

/**
 *  @brief A new processor class which and/nots with a sized version of the intruder
 */
class BoolAndOrNotWithSizedLocalOperation
  : public db::BoolAndOrNotLocalOperation
{
public:
  BoolAndOrNotWithSizedLocalOperation (bool is_and, db::Coord dist)
    : BoolAndOrNotLocalOperation (is_and), m_dist (dist)
  {
    //  .. nothing yet ..
  }

  virtual void compute_local (db::Layout *layout, const db::shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::unordered_set<db::PolygonRef> &result, size_t max_vertex_count, double area_ratio) const
  {
    db::shape_interactions<db::PolygonRef, db::PolygonRef> sized_interactions = interactions;
    for (db::shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = sized_interactions.begin (); i != sized_interactions.end (); ++i) {
      for (db::shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        const db::PolygonRef &ref = interactions.intruder_shape (*j);
        db::Polygon poly = ref.obj ().transformed (ref.trans ());
        poly.size (m_dist, m_dist);
        sized_interactions.add_intruder_shape (*j, db::PolygonRef (poly, layout->shape_repository ()));
      }
    }
    BoolAndOrNotLocalOperation::compute_local (layout, sized_interactions, result, max_vertex_count, area_ratio);
  }

  db::Coord dist () const
  {
    return m_dist;
  }

private:
  db::Coord m_dist;
};

/**
 *  @brief A new processor class which and/nots with a sized version of the intruder
 */
class SelfOverlapWithSizedLocalOperation
  : public db::SelfOverlapMergeLocalOperation
{
public:
  SelfOverlapWithSizedLocalOperation (unsigned int wc, db::Coord dist)
    : SelfOverlapMergeLocalOperation (wc), m_dist (dist)
  {
    //  .. nothing yet ..
  }

  virtual void compute_local (db::Layout *layout, const db::shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::unordered_set<db::PolygonRef> &result, size_t max_vertex_count, double area_ratio) const
  {
    db::shape_interactions<db::PolygonRef, db::PolygonRef> sized_interactions = interactions;
    for (db::shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = sized_interactions.begin (); i != sized_interactions.end (); ++i) {

      const db::PolygonRef &ref = interactions.subject_shape (i->first);
      db::Polygon poly = ref.obj ().transformed (ref.trans ());
      poly.size (m_dist / 2, m_dist / 2);
      sized_interactions.add_subject_shape (i->first, db::PolygonRef (poly, layout->shape_repository ()));

      for (db::shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        const db::PolygonRef &ref = interactions.intruder_shape (*j);
        db::Polygon poly = ref.obj ().transformed (ref.trans ());
        poly.size (m_dist / 2, m_dist / 2);
        sized_interactions.add_intruder_shape (*j, db::PolygonRef (poly, layout->shape_repository ()));
      }

    }

    SelfOverlapMergeLocalOperation::compute_local (layout, sized_interactions, result, max_vertex_count, area_ratio);
  }

  db::Coord dist () const
  {
    return m_dist;
  }

private:
  db::Coord m_dist;
};

/**
 *  @brief Turns a layer into polygons and polygon references
 *  The hierarchical processor needs polygon references and can't work on polygons directly.
 */
static void normalize_layer (db::Layout &layout, unsigned int layer)
{
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes s (layout.is_editable ());
    s.swap (c->shapes (layer));
    for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes); !i.at_end (); ++i) {
      db::Polygon poly;
      i->polygon (poly);
      c->shapes (layer).insert (db::PolygonRef (poly, layout.shape_repository ()));
    }
  }
}


static std::string contexts_to_s (db::Layout *layout, db::local_processor_contexts<db::PolygonRef, db::PolygonRef, db::PolygonRef> &contexts)
{
  std::string res;

  for (db::Layout::top_down_const_iterator i = layout->begin_top_down (); i != layout->end_top_down(); ++i) {
    db::local_processor_contexts<db::PolygonRef, db::PolygonRef, db::PolygonRef>::iterator cc = contexts.context_map ().find (&layout->cell (*i));
    if (cc != contexts.context_map ().end ()) {
      int index = 1;
      for (db::local_processor_cell_contexts<db::PolygonRef, db::PolygonRef, db::PolygonRef>::iterator j = cc->second.begin (); j != cc->second.end (); ++j) {
        res += tl::sprintf ("%s[%d] %d insts, %d shapes (%d times)\n", layout->cell_name (*i), index, int (j->first.first.size ()), int (j->first.second.size ()), int (j->second.size ()));
        index += 1;
      }
    }
  }

  return res;
}

static void run_test_bool_gen (tl::TestBase *_this, const char *file, TestMode mode, int out_layer_num, std::string *context_doc, bool single, db::Coord dist, unsigned int nthreads = 0)
{
  db::Layout layout_org;

  unsigned int l1 = 0, l2 = 0, lout = 0;
  db::LayerMap lmap;
  bool swap = (mode == TMAndSwapped || mode == TMNotSwapped);

  {
    tl::InputStream stream (testdata (file));
    db::Reader reader (stream);

    db::LayerProperties p;

    p.layer = swap ? 2 : 1;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l1 = layout_org.insert_layer ());
    layout_org.set_properties (l1, p);

    p.layer = swap ? 1 : 2;
    p.datatype = 0;
    if (mode == TMSelfOverlap) {
      lmap.map (db::LDPair (p.layer, p.datatype), l2 = l1);
    } else {
      lmap.map (db::LDPair (p.layer, p.datatype), l2 = layout_org.insert_layer ());
      layout_org.set_properties (l2, p);
    }

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
  if (l1 != l2) {
    normalize_layer (layout_org, l2);
  }

  db::local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> *lop = 0;
  db::BoolAndOrNotLocalOperation bool_op (mode == TMAnd || mode == TMAndSwapped);
  db::SelfOverlapMergeLocalOperation self_intersect_op (2);
  BoolAndOrNotWithSizedLocalOperation sized_bool_op (mode == TMAnd || mode == TMAndSwapped, dist);
  SelfOverlapWithSizedLocalOperation sized_self_intersect_op (2, dist);
  if (mode == TMSelfOverlap) {
    if (dist > 0) {
      lop = &sized_self_intersect_op;
    } else {
      lop = &self_intersect_op;
    }
  } else {
    if (dist > 0) {
      lop = &sized_bool_op;
    } else {
      lop = &bool_op;
    }
  }

  if (single) {

    db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (&layout_org, &layout_org.cell (*layout_org.begin_top_down ()));
    proc.set_threads (nthreads);
    proc.set_area_ratio (3.0);
    proc.set_max_vertex_count (16);

    if (! context_doc) {
      proc.run (lop, l1, l2, lout);
    } else {
      db::local_processor_contexts<db::PolygonRef, db::PolygonRef, db::PolygonRef> contexts;
      proc.compute_contexts (contexts, lop, l1, l2);
      *context_doc = contexts_to_s (&layout_org, contexts);
      proc.compute_results (contexts, lop, lout);
    }

  } else {

    db::Layout layout_org2 = layout_org;

    db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (&layout_org, &layout_org.cell (*layout_org.begin_top_down ()), &layout_org2, &layout_org2.cell (*layout_org2.begin_top_down ()));
    proc.set_threads (nthreads);
    proc.set_area_ratio (3.0);
    proc.set_max_vertex_count (16);

    if (! context_doc) {
      proc.run (lop, l1, l2, lout);
    } else {
      db::local_processor_contexts<db::PolygonRef, db::PolygonRef, db::PolygonRef> contexts;
      proc.compute_contexts (contexts, lop, l1, l2);
      *context_doc = contexts_to_s (&layout_org, contexts);
      proc.compute_results (contexts, lop, lout);
    }

  }

  db::compare_layouts (_this, layout_org, testdata (file), lmap, false /*skip other layers*/, db::AsPolygons);
}

static void run_test_bool (tl::TestBase *_this, const char *file, TestMode mode, int out_layer_num, std::string *context_doc = 0, unsigned int nthreads = 0)
{
  run_test_bool_gen (_this, file, mode, out_layer_num, context_doc, true, 0, nthreads);
}

static void run_test_bool2 (tl::TestBase *_this, const char *file, TestMode mode, int out_layer_num, std::string *context_doc = 0, unsigned int nthreads = 0)
{
  run_test_bool_gen (_this, file, mode, out_layer_num, context_doc, false, 0, nthreads);
}

static void run_test_bool_with_size (tl::TestBase *_this, const char *file, TestMode mode, db::Coord dist, int out_layer_num, std::string *context_doc = 0, unsigned int nthreads = 0)
{
  run_test_bool_gen (_this, file, mode, out_layer_num, context_doc, true, dist, nthreads);
}

static void run_test_bool2_with_size (tl::TestBase *_this, const char *file, TestMode mode, db::Coord dist, int out_layer_num, std::string *context_doc = 0, unsigned int nthreads = 0)
{
  run_test_bool_gen (_this, file, mode, out_layer_num, context_doc, false, dist, nthreads);
}

TEST(BasicAnd1)
{
  //  Simple flat AND
  run_test_bool (_this, "hlp1.oas", TMAnd, 100);
}

TEST(BasicAnd1SingleThread)
{
  //  Simple flat AND
  run_test_bool (_this, "hlp1.oas", TMAnd, 100, 0, 1);
}

TEST(BasicAnd1FourThreads)
{
  //  Simple flat AND
  run_test_bool (_this, "hlp1.oas", TMAnd, 100, 0, 4);
}

TEST(BasicNot1)
{
  //  Simple flat AND
  run_test_bool (_this, "hlp1.oas", TMNot, 101);
}

TEST(BasicNot1SingleThread)
{
  //  Simple flat NOT
  run_test_bool (_this, "hlp1.oas", TMNot, 101, 0, 1);
}

TEST(BasicNot1FourThreads)
{
  //  Simple flat NOT
  run_test_bool (_this, "hlp1.oas", TMNot, 101, 0, 4);
}

TEST(BasicAnd2)
{
  //  Up/down and down/up interactions, AND
  run_test_bool (_this, "hlp2.oas", TMAnd, 100);
}

TEST(BasicAnd2SingleThread)
{
  //  Up/down and down/up interactions, AND
  run_test_bool (_this, "hlp2.oas", TMAnd, 100, 0, 1);
}

TEST(BasicAnd2FourThreads)
{
  //  Up/down and down/up interactions, AND
  run_test_bool (_this, "hlp2.oas", TMAnd, 100, 0, 4);
}

TEST(BasicNot2)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool (_this, "hlp2.oas", TMNot, 101);
}

TEST(BasicNot2SingleThread)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool (_this, "hlp2.oas", TMNot, 101, 0, 1);
}

TEST(BasicNot2FourThreads)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool (_this, "hlp2.oas", TMNot, 101, 0, 4);
}

TEST(BasicAnd3)
{
  //  Variant building, AND
  run_test_bool (_this, "hlp3.oas", TMAnd, 100);
}

TEST(BasicAnd3SingleThread)
{
  //  Variant building, AND
  run_test_bool (_this, "hlp3.oas", TMAnd, 100, 0, 1);
}

TEST(BasicAnd3FourThreads)
{
  //  Variant building, AND
  run_test_bool (_this, "hlp3.oas", TMAnd, 100, 0, 4);
}

TEST(BasicNot3)
{
  //  Variant building, NOT
  run_test_bool (_this, "hlp3.oas", TMNot, 101);
}

TEST(BasicNot3SingleThread)
{
  //  Variant building, NOT
  run_test_bool (_this, "hlp3.oas", TMNot, 101, 0, 1);
}

TEST(BasicNot3FourThreads)
{
  //  Variant building, NOT
  run_test_bool (_this, "hlp3.oas", TMNot, 101, 0, 4);
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
  run_test_bool (_this, "hlp9.oas", TMAnd, 100, &doc);
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
  run_test_bool (_this, "hlp9.oas", TMNot, 101, &doc);
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

TEST(BasicAndWithSize1)
{
  //  Simple flat AND
  run_test_bool_with_size (_this, "hlp1.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize1)
{
  //  Simple flat NOT
  run_test_bool_with_size (_this, "hlp1.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize2)
{
  //  Up/down and down/up interactions, AND
  run_test_bool_with_size (_this, "hlp2.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize2)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool_with_size (_this, "hlp2.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize3)
{
  //  Variant building, AND
  run_test_bool_with_size (_this, "hlp3.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize3)
{
  //  Variant building, NOT
  run_test_bool_with_size (_this, "hlp3.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize4)
{
  //  Sibling interactions, variant building, AND
  run_test_bool_with_size (_this, "hlp4.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize4)
{
  //  Sibling interactions, variant building, NOT
  run_test_bool_with_size (_this, "hlp4.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize5)
{
  //  Variant building with intermediate hierarchy, AND
  run_test_bool_with_size (_this, "hlp5.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize5)
{
  //  Variant building with intermediate hierarchy, NOT
  run_test_bool_with_size (_this, "hlp5.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize6)
{
  //  Extreme variants (copy, vanishing), AND
  run_test_bool_with_size (_this, "hlp6.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize6)
{
  //  Extreme variants (copy, vanishing), NOT
  run_test_bool_with_size (_this, "hlp6.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize7)
{
  //  Context replication - direct and indirect, AND
  run_test_bool_with_size (_this, "hlp7.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize7)
{
  //  Context replication - direct and indirect, NOT
  run_test_bool_with_size (_this, "hlp7.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize8)
{
  //  Mixed sibling-parent contexts, AND
  run_test_bool_with_size (_this, "hlp8.oas", TMAnd, 1500, 102);
}

TEST(BasicNotWithSize8)
{
  //  Mixed sibling-parent contexts, NOT
  run_test_bool_with_size (_this, "hlp8.oas", TMNot, 1500, 103);
}

TEST(BasicAndWithSize9)
{
  //  Top-level ring structure, AND
  std::string doc;
  run_test_bool_with_size (_this, "hlp9.oas", TMAnd, 1500, 102, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 0 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 6 shapes (2 times)\n"
  );
}

TEST(BasicNotWithSize9)
{
  //  Top-level ring structure, NOT
  std::string doc;
  run_test_bool_with_size (_this, "hlp9.oas", TMNot, 1500, 103, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 0 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 6 shapes (2 times)\n"
  );
}

TEST(BasicAndWithSize10)
{
  //  Array instances, AND
  run_test_bool_with_size (_this, "hlp10.oas", TMAnd, 150, 102);
}

TEST(BasicNotWithSize10)
{
  //  Array instances, NOT
  run_test_bool_with_size (_this, "hlp10.oas", TMNot, 150, 103);
}

TEST(BasicNotWithSize11)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool_with_size (_this, "hlp11.oas", TMNot, 1500, 103);
}

TEST(BasicNotWithSizeSwappedLayers11)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool_with_size (_this, "hlp11.oas", TMNotSwapped, 1500, 104);
}

TEST(TwoInputsAnd1)
{
  //  Simple flat AND
  run_test_bool2 (_this, "hlp1.oas", TMAnd, 100);
}

TEST(TwoInputsNot1)
{
  //  Simple flat NOT
  run_test_bool2 (_this, "hlp1.oas", TMNot, 101);
}

TEST(TwoInputsAnd2)
{
  //  Up/down and down/up interactions, AND
  run_test_bool2 (_this, "hlp2.oas", TMAnd, 100);
}

TEST(TwoInputsNot2)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool2 (_this, "hlp2.oas", TMNot, 101);
}

TEST(TwoInputsAnd3)
{
  //  Variant building, AND
  run_test_bool2 (_this, "hlp3.oas", TMAnd, 100);
}

TEST(TwoInputsNot3)
{
  //  Variant building, NOT
  run_test_bool2 (_this, "hlp3.oas", TMNot, 101);
}

TEST(TwoInputsAnd4)
{
  //  Sibling interactions, variant building, AND
  run_test_bool2 (_this, "hlp4.oas", TMAnd, 100);
}

TEST(TwoInputsNot4)
{
  //  Sibling interactions, variant building, NOT
  run_test_bool2 (_this, "hlp4.oas", TMNot, 101);
}

TEST(TwoInputsAnd5)
{
  //  Variant building with intermediate hierarchy, AND
  run_test_bool2 (_this, "hlp5.oas", TMAnd, 100);
}

TEST(TwoInputsNot5)
{
  //  Variant building with intermediate hierarchy, NOT
  run_test_bool2 (_this, "hlp5.oas", TMNot, 101);
}

TEST(TwoInputsAnd6)
{
  //  Extreme variants (copy, vanishing), AND
  run_test_bool2 (_this, "hlp6.oas", TMAnd, 120);
}

TEST(TwoInputsNot6)
{
  //  Extreme variants (copy, vanishing), NOT
  run_test_bool2 (_this, "hlp6.oas", TMNot, 121);
}

TEST(TwoInputsAnd7)
{
  //  Context replication - direct and indirect, AND
  run_test_bool2 (_this, "hlp7.oas", TMAnd, 100);
}

TEST(TwoInputsNot7)
{
  //  Context replication - direct and indirect, NOT
  run_test_bool2 (_this, "hlp7.oas", TMNot, 101);
}

TEST(TwoInputsAnd8)
{
  //  Mixed sibling-parent contexts, AND
  run_test_bool2 (_this, "hlp8.oas", TMAnd, 100);
}

TEST(TwoInputsNot8)
{
  //  Mixed sibling-parent contexts, NOT
  run_test_bool2 (_this, "hlp8.oas", TMNot, 101);
}

TEST(TwoInputsAnd9)
{
  //  Top-level ring structure, AND
  std::string doc;
  run_test_bool2 (_this, "hlp9.oas", TMAnd, 100, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 1 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 4 shapes (2 times)\n"
  );
}

TEST(TwoInputsNot9)
{
  //  Top-level ring structure, NOT
  std::string doc;
  run_test_bool2 (_this, "hlp9.oas", TMNot, 101, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 1 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 4 shapes (2 times)\n"
  );
}

TEST(TwoInputsAnd10)
{
  //  Array instances, AND
  run_test_bool2 (_this, "hlp10.oas", TMAnd, 100);
}

TEST(TwoInputsNot10)
{
  //  Array instances, NOT
  run_test_bool2 (_this, "hlp10.oas", TMNot, 101);
}

TEST(TwoInputsAndWithSize1)
{
  //  Simple flat AND
  run_test_bool2_with_size (_this, "hlp1.oas", TMAnd, 1500, 102);
}

TEST(TwoInputsNotWithSize1)
{
  //  Simple flat NOT
  run_test_bool2_with_size (_this, "hlp1.oas", TMNot, 1500, 103);
}

TEST(TwoInputsAndWithSize2)
{
  //  Up/down and down/up interactions, AND
  run_test_bool2_with_size (_this, "hlp2.oas", TMAnd, 1500, 102);
}

TEST(TwoInputsNotWithSize2)
{
  //  Up/down and down/up interactions, NOT
  run_test_bool2_with_size (_this, "hlp2.oas", TMNot, 1500, 103);
}

TEST(TwoInputsAndWithSize3)
{
  //  Variant building, AND
  run_test_bool2_with_size (_this, "hlp3.oas", TMAnd, 1500, 102);
}

TEST(TwoInputsNotWithSize3)
{
  //  Variant building, NOT
  run_test_bool2_with_size (_this, "hlp3.oas", TMNot, 1500, 103);
}

TEST(TwoInputsAndWithSize4)
{
  //  Sibling interactions, variant building, AND
  run_test_bool2_with_size (_this, "hlp4.oas", TMAnd, 1500, 102);
}

TEST(TwoInputsNotWithSize4)
{
  //  Sibling interactions, variant building, NOT
  run_test_bool2_with_size (_this, "hlp4.oas", TMNot, 1500, 103);
}

TEST(TwoInputsAndWithSize5)
{
  //  Variant building with intermediate hierarchy, AND
  run_test_bool2_with_size (_this, "hlp5.oas", TMAnd, 1500, 102);
}

TEST(TwoInputsNotWithSize5)
{
  //  Variant building with intermediate hierarchy, NOT
  run_test_bool2_with_size (_this, "hlp5.oas", TMNot, 1500, 103);
}

TEST(TwoInputsAndWithSize6)
{
  //  Extreme variants (copy, vanishing), AND
  run_test_bool2_with_size (_this, "hlp6.oas", TMAnd, 1500, 122);
}

TEST(TwoInputsNotWithSize6)
{
  //  Extreme variants (copy, vanishing), NOT
  run_test_bool2_with_size (_this, "hlp6.oas", TMNot, 1500, 123);
}

TEST(TwoInputsAndWithSize7)
{
  //  Context replication - direct and indirect, AND
  run_test_bool2_with_size (_this, "hlp7.oas", TMAnd, 1500, 102);
}

TEST(TwoInputsNotWithSize7)
{
  //  Context replication - direct and indirect, NOT
  run_test_bool2_with_size (_this, "hlp7.oas", TMNot, 1500, 103);
}

TEST(TwoInputsAndWithSize8)
{
  //  Mixed sibling-parent contexts, AND
  run_test_bool2_with_size (_this, "hlp8.oas", TMAnd, 1500, 102);
}

TEST(TwoInputsNotWithSize8)
{
  //  Mixed sibling-parent contexts, NOT
  run_test_bool2_with_size (_this, "hlp8.oas", TMNot, 1500, 103);
}

TEST(TwoInputsAndWithSize9)
{
  //  Top-level ring structure, AND
  std::string doc;
  run_test_bool2_with_size (_this, "hlp9.oas", TMAnd, 1500, 102, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 1 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 6 shapes (2 times)\n"
  );
}

TEST(TwoInputsNotWithSize9)
{
  //  Top-level ring structure, NOT
  std::string doc;
  run_test_bool2_with_size (_this, "hlp9.oas", TMNot, 1500, 103, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 1 insts, 0 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 6 shapes (2 times)\n"
  );
}

TEST(TwoInputsAndWithSize10)
{
  //  Array instances, AND
  run_test_bool2_with_size (_this, "hlp10.oas", TMAnd, 150, 102);
}

TEST(TwoInputsNotWithSize10)
{
  //  Array instances, NOT
  run_test_bool2_with_size (_this, "hlp10.oas", TMNot, 150, 103);
}

TEST(BasicSelfOverlap1)
{
  //  Simple flat Self overlap
  run_test_bool (_this, "hlp1.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap2)
{
  //  Up/down and down/up interactions, Self overlap
  run_test_bool (_this, "hlp2.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap3)
{
  //  Variant building, Self overlap
  run_test_bool (_this, "hlp3.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap4)
{
  //  Sibling interactions, variant building, Self overlap
  run_test_bool (_this, "hlp4.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap5)
{
  //  Variant building with intermediate hierarchy, Self overlap
  run_test_bool (_this, "hlp5.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap6)
{
  //  Extreme variants (copy, vanishing), Self overlap
  run_test_bool (_this, "hlp6.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap7)
{
  //  Context replication - direct and indirect, Self overlap
  run_test_bool (_this, "hlp7.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap8)
{
  //  Mixed sibling-parent contexts, Self overlap
  run_test_bool (_this, "hlp8.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlap9)
{
  //  Top-level ring structure, Self overlap
  std::string doc;
  run_test_bool (_this, "hlp9.oas", TMSelfOverlap, 110, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 0 insts, 1 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 4 shapes (2 times)\n"
  );
}

TEST(BasicSelfOverlap10)
{
  //  Array instances, Self overlap
  run_test_bool (_this, "hlp10.oas", TMSelfOverlap, 110);
}

TEST(BasicSelfOverlapWithSize1)
{
  //  Simple flat Self overlap
  run_test_bool_with_size (_this, "hlp1.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize2)
{
  //  Up/down and down/up interactions, Self overlap
  run_test_bool_with_size (_this, "hlp2.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize3)
{
  //  Variant building, Self overlap
  run_test_bool_with_size (_this, "hlp3.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize4)
{
  //  Sibling interactions, variant building, Self overlap
  run_test_bool_with_size (_this, "hlp4.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize5)
{
  //  Variant building with intermediate hierarchy, Self overlap
  run_test_bool_with_size (_this, "hlp5.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize6)
{
  //  Extreme variants (copy, vanishing), Self overlap
  run_test_bool_with_size (_this, "hlp6.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize7)
{
  //  Context replication - direct and indirect, Self overlap
  run_test_bool_with_size (_this, "hlp7.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize8)
{
  //  Mixed sibling-parent contexts, Self overlap
  run_test_bool_with_size (_this, "hlp8.oas", TMSelfOverlap, 1500, 111);
}

TEST(BasicSelfOverlapWithSize9)
{
  //  Top-level ring structure, Self overlap
  std::string doc;
  run_test_bool_with_size (_this, "hlp9.oas", TMSelfOverlap, 1500, 111, &doc);
  EXPECT_EQ (doc,
    //  This means: the interaction test is strong enough, so it does not see interactions between the
    //  ring and the cells embedded inside the ring. So there is only one cell context. Some shapes
    //  from atop the CHILD cell don't interact with shapes inside CHILD, so there are 4 shapes rather than
    //  6. And the shapes from top inside the ring are not seen by the RING's subject shapes.
    "TOP[1] 0 insts, 0 shapes (1 times)\n"
    "RING[1] 0 insts, 1 shapes (1 times)\n"
    "CHILD1[1] 0 insts, 6 shapes (2 times)\n"
  );
}

TEST(BasicSelfOverlapWithSize10)
{
  //  Array instances, Self overlap
  run_test_bool_with_size (_this, "hlp10.oas", TMSelfOverlap, 150, 111);
}

TEST(TopWithBelow1)
{
  run_test_bool (_this, "hlp12.oas", TMNot, 100);
}

TEST(TopWithBelow2)
{
  run_test_bool (_this, "hlp12.oas", TMNotSwapped, 101);
}

TEST(BasicHierarchyVariantsAnd)
{
  run_test_bool (_this, "hlp13.oas", TMAnd, 100);
}

TEST(BasicHierarchyVariantsNot)
{
  run_test_bool (_this, "hlp13.oas", TMNot, 101);
}

TEST(BasicHierarchyVariantsAnd2)
{
  run_test_bool (_this, "hlp14.oas", TMAnd, 100);
}

TEST(BasicHierarchyVariantsNot2)
{
  run_test_bool (_this, "hlp14.oas", TMNot, 101);
}

TEST(RedundantHierarchyAnd1)
{
  //  Redundant hierarchy, NOT
  run_test_bool2 (_this, "hlp15.oas", TMAnd, 100);
}

TEST(RedundantHierarchyNot1)
{
  //  Redundant hierarchy, NOT
  run_test_bool2 (_this, "hlp15.oas", TMNot, 101);
}

TEST(RedundantHierarchyAnd2)
{
  //  Redundant hierarchy, NOT
  run_test_bool2 (_this, "hlp16.gds", TMAnd, 100);
}

TEST(RedundantHierarchyNot2)
{
  //  Redundant hierarchy, NOT
  run_test_bool2 (_this, "hlp16.gds", TMNot, 101);
}

