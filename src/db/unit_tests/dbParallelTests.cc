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
#include "tlException.h"
#include "dbHierProcessor.h"
#include "dbRegionLocalOperations.h"

#include <algorithm>
#include <mutex>
#include <set>
#include <thread>

namespace {

const int benchmark_cells = 64;

class ObservedAndOperation : public db::BoolAndOrNotLocalOperation
{
public:
  ObservedAndOperation () : db::BoolAndOrNotLocalOperation (true) { }

  void do_compute_local (db::Layout *layout, db::Cell *cell, const db::shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const override
  {
    // Keep the tally lock out of the geometry work.
    {
      std::lock_guard<std::mutex> lock (m_mutex);
      m_workers.insert (std::this_thread::get_id ());
    }
    db::BoolAndOrNotLocalOperation::do_compute_local (layout, cell, interactions, results, proc);
  }

  size_t workers () const
  {
    return m_workers.size ();
  }

private:
  mutable std::mutex m_mutex;
  mutable std::set<std::thread::id> m_workers;
};

struct ParallelResult
{
  std::vector<std::pair<std::string, std::string> > shapes;
  std::vector<std::pair<std::string, std::string> > expected;
  size_t workers;
};

ParallelResult run_hierarchical_and (unsigned int threads, int boxes_per_cell)
{
  db::Layout layout;
  const unsigned int subject = layout.insert_layer (db::LayerProperties (1, 0));
  const unsigned int intruder = layout.insert_layer (db::LayerProperties (2, 0));
  const unsigned int output = layout.insert_layer (db::LayerProperties (3, 0));
  db::Cell &top = layout.cell (layout.add_cell ("TOP"));
  ParallelResult result;

  for (int c = 0; c < benchmark_cells; ++c) {
    const std::string name = tl::sprintf ("LEAF_%d", c);
    db::Cell &leaf = layout.cell (layout.add_cell (name.c_str ()));
    top.insert (db::CellInstArray (db::CellInst (leaf.cell_index ()), db::Trans (0, false, db::Vector (c * 2000, 0))));
    for (int b = 0; b < boxes_per_cell; ++b) {
      const int x = (b % 10) * 20;
      const int y = (b / 10) * 20;
      db::Polygon expected (db::Box (x, y, x + 10, y + 10));
      db::PolygonRef polygon (expected, layout.shape_repository ());
      leaf.shapes (subject).insert (polygon);
      leaf.shapes (intruder).insert (polygon);
      result.expected.push_back (std::make_pair (name, expected.to_string ()));
    }
  }

  ObservedAndOperation op;
  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (&layout, &top);
  proc.set_threads (threads);
  proc.set_report_progress (false);
  std::vector<unsigned int> intruders (1, intruder);
  std::vector<unsigned int> outputs (1, output);

  proc.run (&op, subject, intruders, outputs);

  result.workers = op.workers ();
  for (db::Layout::iterator cell = layout.begin (); cell != layout.end (); ++cell) {
    for (db::Shapes::shape_iterator shape = cell->shapes (output).begin (db::ShapeIterator::Polygons); ! shape.at_end (); ++shape) {
      db::Polygon polygon;
      shape->polygon (polygon);
      result.shapes.push_back (std::make_pair (layout.cell_name (cell->cell_index ()), polygon.to_string ()));
    }
  }
  std::sort (result.shapes.begin (), result.shapes.end ());
  std::sort (result.expected.begin (), result.expected.end ());
  return result;
}

void check_hierarchical_and (tl::TestBase *_this, int boxes_per_cell)
{
  ParallelResult serial = run_hierarchical_and (0, boxes_per_cell);
  ParallelResult parallel = run_hierarchical_and (4, boxes_per_cell);

  EXPECT_EQ (serial.shapes.size (), size_t (benchmark_cells * boxes_per_cell));
  EXPECT (serial.shapes == serial.expected);
  EXPECT (serial.shapes == parallel.shapes);
  EXPECT_EQ (serial.workers, size_t (1));
  if (std::thread::hardware_concurrency () > 1) {
    EXPECT (parallel.workers > 1);
  }
}

}

TEST(ParallelHierarchicalAndCorrectness)
{
  check_hierarchical_and (_this, 800);
}

namespace {

const int deep_level_count = 5;   //  TOP -> LEVEL_0 -> ... -> LEVEL_4 -> leaves
const int deep_leaf_count = 64;

typedef std::vector<std::pair<std::string, std::string> > shape_list_type;

//  runs a hierarchical AND on a deep and unbalanced hierarchy: a chain of 5 levels
//  below TOP, a wide level of leaf cells with very different shape counts and some
//  leaves instantiated again at higher levels. Intruder shapes in LEVEL_4 and TOP
//  overlap leaf instance boxes, so leaf results depend on their ancestors' contexts
//  and have to propagate up the whole chain: the output is only correct if the
//  cells are computed strictly bottom-up.
shape_list_type run_deep_hierarchical_and (unsigned int threads)
{
  db::Layout layout;
  const unsigned int subject = layout.insert_layer (db::LayerProperties (1, 0));
  const unsigned int intruder = layout.insert_layer (db::LayerProperties (2, 0));
  const unsigned int output = layout.insert_layer (db::LayerProperties (3, 0));
  db::Cell &top = layout.cell (layout.add_cell ("TOP"));

  std::vector<db::cell_index_type> leaves;
  for (int c = 0; c < deep_leaf_count; ++c) {
    db::Cell &leaf = layout.cell (layout.add_cell (tl::sprintf ("LEAF_%d", c).c_str ()));
    leaves.push_back (leaf.cell_index ());
    const int boxes = (c % 8 + 1) * 25;   //  1:8 work ratio between the leaves
    for (int b = 0; b < boxes; ++b) {
      const int x = (b % 10) * 20;
      const int y = (b / 10) * 20;
      db::Polygon polygon (db::Box (x, y, x + 10, y + 10));
      db::PolygonRef ref (polygon, layout.shape_repository ());
      leaf.shapes (subject).insert (ref);
      //  odd boxes get no intruder copy of their own: their result comes from the
      //  parent intruders below, i.e. from the propagation through the hierarchy
      if (b % 2 == 0) {
        leaf.shapes (intruder).insert (ref);
      }
    }
  }

  std::vector<db::cell_index_type> levels;
  db::Cell *parent_cell = &top;
  for (int l = 0; l < deep_level_count; ++l) {
    db::Cell &level = layout.cell (layout.add_cell (tl::sprintf ("LEVEL_%d", l).c_str ()));
    parent_cell->insert (db::CellInstArray (db::CellInst (level.cell_index ()), db::Trans ()));
    levels.push_back (level.cell_index ());
    parent_cell = &level;
  }

  for (int c = 0; c < deep_leaf_count; ++c) {
    parent_cell->insert (db::CellInstArray (db::CellInst (leaves [c]), db::Trans (0, false, db::Vector (c * 2000, 0))));
  }

  //  instantiate some leaves again at other levels, so cells appear at several depths
  for (int c = 0; c < 8; ++c) {
    layout.cell (levels [0]).insert (db::CellInstArray (db::CellInst (leaves [c]), db::Trans (0, false, db::Vector (c * 2000, 1000000))));
    top.insert (db::CellInstArray (db::CellInst (leaves [8 + c]), db::Trans (0, false, db::Vector (c * 2000, 2000000))));
  }

  //  intruder shapes in the parent cells partially overlapping the leaf subject
  //  boxes: they force the propagation of leaf results into LEVEL_4 and TOP
  for (int c = 0; c < 16; ++c) {
    db::PolygonRef intruder_box (db::Polygon (db::Box (c * 2000 + 15, 15, c * 2000 + 35, 35)), layout.shape_repository ());
    layout.cell (levels [4]).shapes (intruder).insert (intruder_box);
  }
  for (int c = 16; c < 24; ++c) {
    db::PolygonRef intruder_box (db::Polygon (db::Box (c * 2000 + 15, 5, c * 2000 + 35, 15)), layout.shape_repository ());
    top.shapes (intruder).insert (intruder_box);
  }
  for (int c = 0; c < 8; ++c) {
    db::PolygonRef intruder_box (db::Polygon (db::Box (c * 2000 + 15, 2000015, c * 2000 + 35, 2000035)), layout.shape_repository ());
    top.shapes (intruder).insert (intruder_box);
  }

  db::BoolAndOrNotLocalOperation op (true);
  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (&layout, &top);
  proc.set_threads (threads);
  proc.set_report_progress (false);
  std::vector<unsigned int> intruders (1, intruder);
  std::vector<unsigned int> outputs (1, output);

  proc.run (&op, subject, intruders, outputs);

  shape_list_type result;
  for (db::Layout::iterator cell = layout.begin (); cell != layout.end (); ++cell) {
    for (db::Shapes::shape_iterator shape = cell->shapes (output).begin (db::ShapeIterator::Polygons); ! shape.at_end (); ++shape) {
      db::Polygon polygon;
      shape->polygon (polygon);
      result.push_back (std::make_pair (layout.cell_name (cell->cell_index ()), polygon.to_string ()));
    }
  }
  std::sort (result.begin (), result.end ());
  return result;
}

void check_deep_hierarchical_and (tl::TestBase *_this)
{
  //  the exact geometry is hard to predict with the parent intruders, so the serial
  //  run defines the reference result
  shape_list_type serial = run_deep_hierarchical_and (0);

  //  every even box ANDs with its identical intruder copy inside the leaf, so at
  //  least this many polygons must show up (the parent intruders add more)
  EXPECT (serial.size () >= size_t (3616));

  const unsigned int thread_counts[] = { 1, 2, 4, 8 };
  for (size_t i = 0; i < sizeof (thread_counts) / sizeof (thread_counts[0]); ++i) {
    shape_list_type parallel = run_deep_hierarchical_and (thread_counts [i]);
    EXPECT_EQ (parallel.size (), serial.size ());
    EXPECT (parallel == serial);
  }
}

class ThrowingAndOperation
  : public db::BoolAndOrNotLocalOperation
{
public:
  ThrowingAndOperation ()
    : db::BoolAndOrNotLocalOperation (true)
  {
    //  .. nothing yet ..
  }

  void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const db::shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/, const db::LocalProcessorBase * /*proc*/) const override
  {
    throw tl::Exception ("Simulated local operation failure");
  }
};

//  runs a hierarchical AND with an op that throws and expects the exception to surface
void run_failing_hierarchical_and (tl::TestBase *_this, unsigned int threads, const char *where)
{
  bool caught = false;
  try {

    db::Layout layout;
    const unsigned int subject = layout.insert_layer (db::LayerProperties (1, 0));
    const unsigned int intruder = layout.insert_layer (db::LayerProperties (2, 0));
    const unsigned int output = layout.insert_layer (db::LayerProperties (3, 0));
    db::Cell &top = layout.cell (layout.add_cell ("TOP"));
    db::Cell &mid = layout.cell (layout.add_cell ("MID"));
    top.insert (db::CellInstArray (db::CellInst (mid.cell_index ()), db::Trans ()));
    for (int c = 0; c < 8; ++c) {
      db::Cell &leaf = layout.cell (layout.add_cell (tl::sprintf ("LEAF_%d", c).c_str ()));
      mid.insert (db::CellInstArray (db::CellInst (leaf.cell_index ()), db::Trans (0, false, db::Vector (c * 2000, 0))));
      for (int b = 0; b < 10; ++b) {
        db::Polygon polygon (db::Box ((b % 10) * 20, 0, (b % 10) * 20 + 10, 10));
        db::PolygonRef ref (polygon, layout.shape_repository ());
        leaf.shapes (subject).insert (ref);
        leaf.shapes (intruder).insert (ref);
      }
    }

    ThrowingAndOperation op;
    db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (&layout, &top);
    proc.set_threads (threads);
    proc.set_report_progress (false);
    proc.run (&op, subject, std::vector<unsigned int> (1, intruder), std::vector<unsigned int> (1, output));

  } catch (const tl::Exception &) {
    caught = true;
  }

  EXPECT_EQ (caught, true);
  if (! caught) {
    _this->raise (__FILE__, __LINE__, std::string ("exception did not propagate (") + where + ")");
  }
}

}

TEST(ParallelHierarchicalAndDeepCorrectness)
{
  check_deep_hierarchical_and (_this);
}

TEST(ParallelHierarchicalExceptionPropagation)
{
  run_failing_hierarchical_and (_this, 0, "serial");

#if defined(_OPENMP)
  run_failing_hierarchical_and (_this, 2, "OpenMP, 2 threads");
  run_failing_hierarchical_and (_this, 4, "OpenMP, 4 threads");
#endif
}
