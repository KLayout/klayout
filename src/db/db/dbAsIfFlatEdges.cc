
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


#include "dbAsIfFlatEdges.h"
#include "dbFlatEdges.h"
#include "dbFlatEdgePairs.h"
#include "dbFlatRegion.h"
#include "dbEmptyEdges.h"
#include "dbEdges.h"
#include "dbEdgesUtils.h"
#include "dbEdgeBoolean.h"
#include "dbBoxConvert.h"
#include "dbRegion.h"
#include "dbFlatRegion.h"
#include "dbPolygonTools.h"
#include "dbShapeProcessor.h"
#include "dbEdgeProcessor.h"
#include "dbPolygonGenerators.h"
#include "dbPolygon.h"
#include "dbPath.h"

#include <sstream>

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  AsIfFlagEdges implementation

AsIfFlatEdges::AsIfFlatEdges ()
  : EdgesDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatEdges::~AsIfFlatEdges ()
{
  //  .. nothing yet ..
}

std::string
AsIfFlatEdges::to_string (size_t nmax) const
{
  std::ostringstream os;
  EdgesIterator p (begin ());
  bool first = true;
  for ( ; ! p.at_end () && nmax != 0; ++p, --nmax) {
    if (! first) {
      os << ";";
    }
    first = false;
    os << p->to_string ();
  }
  if (! p.at_end ()) {
    os << "...";
  }
  return os.str ();
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting_generic (const Region &other, bool inverse) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyEdges ();
  }

  db::box_scanner2<db::Edge, size_t, db::Polygon, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin_merged (), has_valid_merged_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressablePolygonDelivery p = other.addressable_polygons ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::auto_ptr<FlatEdges> output (new FlatEdges (true));

  if (! inverse) {

    edge_to_region_interaction_filter<FlatEdges> filter (*output);
    scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());

  } else {

    std::set<db::Edge> interacting;
    edge_to_region_interaction_filter<std::set<db::Edge> > filter (interacting);
    scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());

    for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
      if (interacting.find (*o) == interacting.end ()) {
        output->insert (*o);
      }
    }

  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting (const Region &other) const
{
  return selected_interacting_generic (other, false);
}

EdgesDelegate *
AsIfFlatEdges::selected_not_interacting (const Region &other) const
{
  return selected_interacting_generic (other, true);
}

namespace
{

struct JoinEdgesClusterCollector
  : public db::cluster_collector<db::Edge, size_t, JoinEdgesCluster>
{
  typedef db::Edge::coord_type coord_type;

  JoinEdgesClusterCollector (db::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
    : db::cluster_collector<db::Edge, size_t, JoinEdgesCluster> (JoinEdgesCluster (output, ext_b, ext_e, ext_o, ext_i), true)
  {
    //  .. nothing yet ..
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  {
    if (o1->p2 () == o2->p1 () || o1->p1 () == o2->p2 ()) {
      db::cluster_collector<db::Edge, size_t, JoinEdgesCluster>::add (o1, p1, o2, p2);
    }
  }
};

}

RegionDelegate *
AsIfFlatEdges::extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const
{
  if (join) {

    std::auto_ptr<FlatRegion> output (new FlatRegion ());
    db::ShapeGenerator sg (output->raw_polygons (), false);
    JoinEdgesClusterCollector cluster_collector (&sg, ext_b, ext_e, ext_o, ext_i);

    db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
    scanner.reserve (size ());

    AddressableEdgeDelivery e (begin (), has_valid_edges ());

    size_t n = 0;
    for ( ; ! e.at_end (); ++e) {
      scanner.insert (e.operator-> (), n);
      ++n;
    }

    scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    return output.release ();

  } else {

    std::auto_ptr<FlatRegion> output (new FlatRegion ());
    for (EdgesIterator e (begin_merged ()); ! e.at_end (); ++e) {
      output->insert (extended_edge (*e, ext_b, ext_e, ext_o, ext_i));
    }

    return output.release ();

  }
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting_generic (const Edges &edges, bool inverse) const
{
  db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin_merged (), has_valid_merged_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), 0);
  }

  AddressableEdgeDelivery ee = edges.addressable_edges ();

  for ( ; ! ee.at_end (); ++ee) {
    scanner.insert (ee.operator-> (), 1);
  }

  std::auto_ptr<FlatEdges> output (new FlatEdges (true));

  if (! inverse) {

    edge_interaction_filter<FlatEdges> filter (*output);
    scanner.process (filter, 1, db::box_convert<db::Edge> ());

  } else {

    std::set<db::Edge> interacting;
    edge_interaction_filter<std::set<db::Edge> > filter (interacting);
    scanner.process (filter, 1, db::box_convert<db::Edge> ());

    for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
      if (interacting.find (*o) == interacting.end ()) {
        output->insert (*o);
      }
    }

  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting (const Edges &other) const
{
  return selected_interacting_generic (other, false);
}

EdgesDelegate *
AsIfFlatEdges::selected_not_interacting (const Edges &other) const
{
  return selected_interacting_generic (other, true);
}

EdgesDelegate *
AsIfFlatEdges::in (const Edges &other, bool invert) const
{
  std::set <db::Edge> op;
  for (EdgesIterator o (other.begin_merged ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::auto_ptr<FlatEdges> new_region (new FlatEdges (false));

  for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_region->insert (*o);
    }
  }

  return new_region.release ();
}

size_t
AsIfFlatEdges::size () const
{
  size_t n = 0;
  for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
    ++n;
  }
  return n;
}

AsIfFlatEdges::length_type
AsIfFlatEdges::length (const db::Box &box) const
{
  distance_type l = 0;

  for (EdgesIterator e (begin_merged ()); ! e.at_end (); ++e) {

    if (box.empty () || (box.contains (e->p1 ()) && box.contains (e->p2 ()))) {
      l += e->length ();
    } else {

      std::pair<bool, db::Edge> ce = e->clipped (box);
      if (ce.first) {

        db::Coord dx = ce.second.dx ();
        db::Coord dy = ce.second.dy ();
        db::Coord x = ce.second.p1 ().x ();
        db::Coord y = ce.second.p1 ().y ();
        if ((dx == 0 && x == box.left ()   && dy < 0) ||
            (dx == 0 && x == box.right ()  && dy > 0) ||
            (dy == 0 && y == box.top ()    && dx < 0) ||
            (dy == 0 && y == box.bottom () && dx > 0)) {
          //  not counted -> box is at outside side of the edge
        } else {
          l += ce.second.length ();
        }

      }

    }

  }

  return l;
}

Box AsIfFlatEdges::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatEdges::compute_bbox () const
{
  db::Box b;
  for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
    b += e->bbox ();
  }
  return b;
}

void AsIfFlatEdges::update_bbox (const db::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatEdges::invalidate_bbox ()
{
  m_bbox_valid = false;
}

EdgesDelegate *
AsIfFlatEdges::processed (const EdgeProcessorBase &filter) const
{
  std::auto_ptr<FlatEdges> edges (new FlatEdges ());

  if (filter.result_must_not_be_merged ()) {
    edges->set_merged_semantics (false);
  }

  std::vector<db::Edge> res_edges;

  for (EdgesIterator e (filter.requires_raw_input () ? begin () : begin_merged ()); ! e.at_end (); ++e) {
    res_edges.clear ();
    filter.process (*e, res_edges);
    for (std::vector<db::Edge>::const_iterator er = res_edges.begin (); er != res_edges.end (); ++er) {
      edges->insert (*er);
    }
  }

  return edges.release ();
}

EdgePairsDelegate *
AsIfFlatEdges::processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &filter) const
{
  std::auto_ptr<FlatEdgePairs> edge_pairs (new FlatEdgePairs ());

  if (filter.result_must_not_be_merged ()) {
    edge_pairs->set_merged_semantics (false);
  }

  std::vector<db::EdgePair> res_edge_pairs;

  for (EdgesIterator e (filter.requires_raw_input () ? begin () : begin_merged ()); ! e.at_end (); ++e) {
    res_edge_pairs.clear ();
    filter.process (*e, res_edge_pairs);
    for (std::vector<db::EdgePair>::const_iterator epr = res_edge_pairs.begin (); epr != res_edge_pairs.end (); ++epr) {
      edge_pairs->insert (*epr);
    }
  }

  return edge_pairs.release ();
}

RegionDelegate *
AsIfFlatEdges::processed_to_polygons (const EdgeToPolygonProcessorBase &filter) const
{
  std::auto_ptr<FlatRegion> region (new FlatRegion ());

  if (filter.result_must_not_be_merged ()) {
    region->set_merged_semantics (false);
  }

  std::vector<db::Polygon> res_polygons;

  for (EdgesIterator e (filter.requires_raw_input () ? begin () : begin_merged ()); ! e.at_end (); ++e) {
    res_polygons.clear ();
    filter.process (*e, res_polygons);
    for (std::vector<db::Polygon>::const_iterator pr = res_polygons.begin (); pr != res_polygons.end (); ++pr) {
      region->insert (*pr);
    }
  }

  return region.release ();
}

EdgesDelegate *
AsIfFlatEdges::filtered (const EdgeFilterBase &filter) const
{
  std::auto_ptr<FlatEdges> new_region (new FlatEdges ());

  for (EdgesIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      new_region->insert (*p);
    }
  }

  return new_region.release ();
}

EdgePairsDelegate *
AsIfFlatEdges::run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  std::auto_ptr<FlatEdgePairs> result (new FlatEdgePairs ());

  db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (size () + (other ? other->size () : 0));

  AddressableEdgeDelivery e (begin_merged (), has_valid_edges ());

  size_t n = 0;
  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), n);
    n += 2;
  }

  AddressableEdgeDelivery ee;

  if (other) {
    ee = other->addressable_merged_edges ();
    n = 1;
    for ( ; ! ee.at_end (); ++ee) {
      scanner.insert (ee.operator-> (), n);
      n += 2;
    }
  }

  EdgeRelationFilter check (rel, d, metrics);
  check.set_include_zero (false);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  edge2edge_check_for_edges<db::FlatEdgePairs> edge_check (check, *result, other != 0);
  scanner.process (edge_check, d, db::box_convert<db::Edge> ());

  return result.release ();
}

EdgesDelegate * 
AsIfFlatEdges::boolean (const Edges *other, EdgeBoolOp op) const
{
  std::auto_ptr<FlatEdges> output (new FlatEdges (true));
  EdgeBooleanClusterCollector<db::Shapes> cluster_collector (&output->raw_edges (), op);

  db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (size () + (other ? other->size () : 0));

  AddressableEdgeDelivery e (begin (), has_valid_edges ());

  for ( ; ! e.at_end (); ++e) {
    if (! e->is_degenerate ()) {
      scanner.insert (e.operator-> (), 0);
    }
  }

  AddressableEdgeDelivery ee;

  if (other) {
    ee = other->addressable_edges ();
    for ( ; ! ee.at_end (); ++ee) {
      if (! ee->is_degenerate ()) {
        scanner.insert (ee.operator-> (), 1);
      }
    }
  }

  scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

  return output.release ();
}

EdgesDelegate * 
AsIfFlatEdges::edge_region_op (const Region &other, bool outside, bool include_borders) const
{
  //  shortcuts
  if (other.empty ()) {
    if (! outside) {
      return new EmptyEdges ();
    } else {
      return clone ();
    }
  } else if (empty ()) {
    return new EmptyEdges ();
  }

  db::EdgeProcessor ep (report_progress (), progress_desc ());

  for (db::Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, 0);
    }
  }

  for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
    ep.insert (*e, 1);
  }

  std::auto_ptr<FlatEdges> output (new FlatEdges (false));
  db::EdgeShapeGenerator cc (output->raw_edges (), true /*clear*/);
  db::EdgePolygonOp op (outside, include_borders);
  ep.process (cc, op);

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::add (const Edges &other) const
{
  FlatEdges *other_flat = dynamic_cast<FlatEdges *> (other.delegate ());
  if (other_flat) {

    std::auto_ptr<FlatEdges> new_edges (new FlatEdges (*other_flat));
    new_edges->set_is_merged (false);
    new_edges->invalidate_cache ();

    size_t n = new_edges->raw_edges ().size () + size ();

    new_edges->reserve (n);

    for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
      new_edges->raw_edges ().insert (*p);
    }

    return new_edges.release ();

  } else {

    std::auto_ptr<FlatEdges> new_edges (new FlatEdges (false /*not merged*/));

    size_t n = size () + other.size ();

    new_edges->reserve (n);

    for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
      new_edges->raw_edges ().insert (*p);
    }
    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_edges->raw_edges ().insert (*p);
    }

    return new_edges.release ();

  }
}

bool
AsIfFlatEdges::equals (const Edges &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (size () != other.size ()) {
    return false;
  }
  EdgesIterator o1 (begin ());
  EdgesIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return false;
    }
    ++o1;
    ++o2;
  }
  return true;
}

bool
AsIfFlatEdges::less (const Edges &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (size () != other.size ()) {
    return (size () < other.size ());
  }
  EdgesIterator o1 (begin ());
  EdgesIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

void
AsIfFlatEdges::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
    shapes.insert (*e);
  }
}

}

