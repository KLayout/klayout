
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


#include "dbAsIfFlatEdges.h"
#include "dbFlatEdges.h"
#include "dbFlatEdgePairs.h"
#include "dbFlatRegion.h"
#include "dbEmptyEdges.h"
#include "dbEdges.h"
#include "dbEdgesUtils.h"
#include "dbEdgesLocalOperations.h"
#include "dbEdgeBoolean.h"
#include "dbBoxConvert.h"
#include "dbRegion.h"
#include "dbFlatRegion.h"
#include "dbEmptyRegion.h"
#include "dbPolygonTools.h"
#include "dbShapeProcessor.h"
#include "dbEdgeProcessor.h"
#include "dbPolygonGenerators.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbHierProcessor.h"

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

AsIfFlatEdges::AsIfFlatEdges (const AsIfFlatEdges &other)
  : EdgesDelegate (other), m_bbox_valid (false)
{
  operator= (other);
}

AsIfFlatEdges &
AsIfFlatEdges::operator= (const AsIfFlatEdges &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }
  return *this;
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
    if (p.prop_id () != 0) {
      os << db::properties (p.prop_id ()).to_dict_var ().to_string ();
    }
  }
  if (! p.at_end ()) {
    os << "...";
  }
  return os.str ();
}

namespace {

class OutputPairHolder
{
public:
  OutputPairHolder (int inverse, bool merged_semantics)
  {
    m_e1.reset (new FlatEdges (merged_semantics));
    m_results.push_back (& m_e1->raw_edges ());

    if (inverse == 0) {
      m_e2.reset (new FlatEdges (merged_semantics));
      m_results.push_back (& m_e2->raw_edges ());
    }
  }

  std::pair<EdgesDelegate *, EdgesDelegate *> region_pair ()
  {
    return std::make_pair (m_e1.release (), m_e2.release ());
  }

  const std::vector<db::Shapes *> &results () { return m_results; }

private:
  std::unique_ptr<FlatEdges> m_e1, m_e2;
  std::vector<db::Shapes *> m_results;
};

}

EdgesDelegate *
AsIfFlatEdges::selected_interacting_generic (const Region &other, EdgeInteractionMode mode, bool inverse, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    return ((mode == EdgesOutside) == inverse) ? new EmptyEdges () : clone ();
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (inverse ? 1 : -1, merged_semantics () || is_merged ());

  db::EdgesIterator edges (begin_merged ());

  db::edge_to_polygon_interacting_local_operation<db::Polygon> op (mode, inverse ? db::edge_to_polygon_interacting_local_operation<db::Polygon>::Inverse : db::edge_to_polygon_interacting_local_operation<db::Polygon>::Normal, min_count, max_count);

  db::local_processor<db::Edge, db::Polygon, db::Edge> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Polygon> > others;
  //  NOTE: with counting the other region needs to be merged
  others.push_back (counting || mode != EdgesInteract ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ().first;
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting_generic (const Edges &other, EdgeInteractionMode mode, bool inverse, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    return ((mode == EdgesOutside) == inverse) ? new EmptyEdges () : clone ();
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (inverse ? 1 : -1, merged_semantics () || is_merged ());

  db::EdgesIterator edges (begin_merged ());

  db::Edge2EdgeInteractingLocalOperation op (mode, inverse ? db::Edge2EdgeInteractingLocalOperation::Inverse : db::Edge2EdgeInteractingLocalOperation::Normal, min_count, max_count);

  db::local_processor<db::Edge, db::Edge, db::Edge> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Edge> > others;
  //  NOTE: with counting the other edge collection needs to be merged
  others.push_back (counting || mode != EdgesInteract ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ().first;
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_interacting_pair_generic (const Region &other, EdgeInteractionMode mode, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    if (mode != EdgesOutside) {
      return std::make_pair (new EmptyEdges (), clone ());
    } else {
      return std::make_pair (clone (), new EmptyEdges ());
    }
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (0, merged_semantics () || is_merged ());

  db::EdgesIterator edges (begin_merged ());

  db::edge_to_polygon_interacting_local_operation<db::Polygon> op (mode, db::edge_to_polygon_interacting_local_operation<db::Polygon>::Both, min_count, max_count);

  db::local_processor<db::Edge, db::Polygon, db::Edge> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Polygon> > others;
  //  NOTE: with counting the other region needs to be merged
  others.push_back (counting || mode != EdgesInteract ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_interacting_pair_generic (const Edges &other, EdgeInteractionMode mode, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    if (mode != EdgesOutside) {
      return std::make_pair (new EmptyEdges (), clone ());
    } else {
      return std::make_pair (clone (), new EmptyEdges ());
    }
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (0, merged_semantics () || is_merged ());

  db::EdgesIterator edges (begin_merged ());

  db::Edge2EdgeInteractingLocalOperation op (mode, db::Edge2EdgeInteractingLocalOperation::Both, min_count, max_count);

  db::local_processor<db::Edge, db::Edge, db::Edge> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Edge> > others;
  //  NOTE: with counting the other edge collection needs to be merged
  others.push_back (counting || mode != EdgesInteract ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}


EdgesDelegate *
AsIfFlatEdges::pull_generic (const Edges &edges) const
{
  db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), 1);
  }

  AddressableEdgeDelivery ee = edges.addressable_merged_edges ();

  for ( ; ! ee.at_end (); ++ee) {
    scanner.insert (ee.operator-> (), 0);
  }

  std::unique_ptr<FlatEdges> output (new FlatEdges (true));
  edge_interaction_filter<FlatEdges> filter (*output, EdgesInteract, size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::Edge> ());

  return output.release ();
}

RegionDelegate *
AsIfFlatEdges::pull_generic (const Region &other) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyRegion ();
  }

  db::box_scanner2<db::Edge, size_t, db::Polygon, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressablePolygonDelivery p = other.addressable_merged_polygons ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::unique_ptr<FlatRegion> output (new FlatRegion (true));

  edge_to_polygon_interaction_filter<FlatRegion> filter (output.get (), EdgesInteract, size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::pull_interacting (const Edges &other) const
{
  return pull_generic (other);
}

RegionDelegate *
AsIfFlatEdges::pull_interacting (const Region &other) const
{
  return pull_generic (other);
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting (const Edges &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, EdgesInteract, false, min_count, max_count);
}

EdgesDelegate *
AsIfFlatEdges::selected_not_interacting (const Edges &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, EdgesInteract, true, min_count, max_count);
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting (const Region &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, EdgesInteract, false, min_count, max_count);
}

EdgesDelegate *
AsIfFlatEdges::selected_not_interacting (const Region &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, EdgesInteract, true, min_count, max_count);
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_interacting_pair (const Region &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_pair_generic (other, EdgesInteract, min_count, max_count);
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_interacting_pair (const Edges &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_pair_generic (other, EdgesInteract, min_count, max_count);
}

EdgesDelegate *
AsIfFlatEdges::selected_outside (const Region &other) const
{
  return selected_interacting_generic (other, EdgesOutside, false, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgesDelegate *
AsIfFlatEdges::selected_not_outside (const Region &other) const
{
  return selected_interacting_generic (other, EdgesOutside, true, size_t (1), std::numeric_limits<size_t>::max ());
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_outside_pair (const Region &other) const
{
  return selected_interacting_pair_generic (other, EdgesOutside, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgesDelegate *
AsIfFlatEdges::selected_inside (const Region &other) const
{
  return selected_interacting_generic (other, EdgesInside, false, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgesDelegate *
AsIfFlatEdges::selected_not_inside (const Region &other) const
{
  return selected_interacting_generic (other, EdgesInside, true, size_t (1), std::numeric_limits<size_t>::max ());
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_inside_pair (const Region &other) const
{
  return selected_interacting_pair_generic (other, EdgesInside, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgesDelegate *
AsIfFlatEdges::selected_outside (const Edges &other) const
{
  return selected_interacting_generic (other, EdgesOutside, false, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgesDelegate *
AsIfFlatEdges::selected_not_outside (const Edges &other) const
{
  return selected_interacting_generic (other, EdgesOutside, true, size_t (1), std::numeric_limits<size_t>::max ());
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_outside_pair (const Edges &other) const
{
  return selected_interacting_pair_generic (other, EdgesOutside, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgesDelegate *
AsIfFlatEdges::selected_inside (const Edges &other) const
{
  return selected_interacting_generic (other, EdgesInside, false, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgesDelegate *
AsIfFlatEdges::selected_not_inside (const Edges &other) const
{
  return selected_interacting_generic (other, EdgesInside, true, size_t (1), std::numeric_limits<size_t>::max ());
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::selected_inside_pair (const Edges &other) const
{
  return selected_interacting_pair_generic (other, EdgesInside, size_t (1), std::numeric_limits<size_t>::max ());
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

    //  TODO: property support?

    std::unique_ptr<FlatRegion> output (new FlatRegion ());
    db::ShapeGenerator sg (output->raw_polygons (), false);
    JoinEdgesClusterCollector cluster_collector (&sg, ext_b, ext_e, ext_o, ext_i);

    db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
    scanner.reserve (count ());

    AddressableEdgeDelivery e (begin ());

    size_t n = 0;
    for ( ; ! e.at_end (); ++e) {
      scanner.insert (e.operator-> (), n);
      ++n;
    }

    scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    return output.release ();

  } else {

    std::unique_ptr<FlatRegion> output (new FlatRegion ());

    for (EdgesIterator e (begin_merged ()); ! e.at_end (); ++e) {
      db::properties_id_type prop_id = e.prop_id ();
      if (prop_id != 0) {
        output->insert (db::PolygonWithProperties (extended_edge (*e, ext_b, ext_e, ext_o, ext_i), prop_id));
      } else {
        output->insert (extended_edge (*e, ext_b, ext_e, ext_o, ext_i));
      }
    }

    return output.release ();

  }
}

EdgesDelegate *
AsIfFlatEdges::in (const Edges &other, bool invert) const
{
  if (other.empty ()) {
    return invert ? clone () : new EmptyEdges ();
  } else if (empty ()) {
    return new EmptyEdges ();
  }

  std::set <db::Edge> op;
  for (EdgesIterator o (other.begin_merged ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatEdges> new_region (new FlatEdges (false));

  for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_region->insert (*o);
    }
  }

  return new_region.release ();
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::in_and_out (const Edges &other) const
{
  if (other.empty ()) {
    return std::make_pair (new EmptyEdges (), clone ());
  } else if (empty ()) {
    return std::make_pair (new EmptyEdges (), new EmptyEdges ());
  }

  std::set <db::Edge> op;
  for (EdgesIterator o (other.begin_merged ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatEdges> in (new FlatEdges (false));
  std::unique_ptr<FlatEdges> out (new FlatEdges (false));

  for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
    if (op.find (*o) != op.end ()) {
      in->insert (*o);
    } else {
      out->insert (*o);
    }
  }

  return std::make_pair (in.release (), out.release ());
}

size_t
AsIfFlatEdges::count () const
{
  size_t n = 0;
  for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
    ++n;
  }
  return n;
}

size_t
AsIfFlatEdges::hier_count () const
{
  return count ();
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
  std::unique_ptr<FlatEdges> edges (new FlatEdges ());

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
  std::unique_ptr<FlatEdgePairs> edge_pairs (new FlatEdgePairs ());

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
  std::unique_ptr<FlatRegion> region (new FlatRegion ());

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
  std::unique_ptr<FlatEdges> new_region (new FlatEdges ());

  for (EdgesIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p, p.prop_id ())) {
      if (p.prop_id () != 0) {
        new_region->insert (db::EdgeWithProperties (*p, p.prop_id ()));
      } else {
        new_region->insert (*p);
      }
    }
  }

  return new_region.release ();
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::filtered_pair (const EdgeFilterBase &filter) const
{
  std::unique_ptr<FlatEdges> new_region_true (new FlatEdges ());
  std::unique_ptr<FlatEdges> new_region_false (new FlatEdges ());

  for (EdgesIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p, p.prop_id ())) {
      new_region_true->insert (*p);
    } else {
      new_region_false->insert (*p);
    }
  }

  return std::make_pair (new_region_true.release (), new_region_false.release ());
}

EdgePairsDelegate *
AsIfFlatEdges::run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, const db::EdgesCheckOptions &options) const
{
  std::unique_ptr<FlatEdgePairs> result (new FlatEdgePairs ());

  db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (count () + (other ? other->count () : 0));

  AddressableEdgeDelivery e (begin_merged ());

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

  EdgeRelationFilter check (rel, d, options);

  edge2edge_check_for_edges<db::FlatEdgePairs> edge_check (check, *result, other != 0);
  scanner.process (edge_check, d, db::box_convert<db::Edge> ());

  return result.release ();
}

EdgesDelegate *
AsIfFlatEdges::merged () const
{
  if (empty ()) {
    return new db::EmptyEdges ();
  } else {
    return boolean (0, EdgeOr);
  }
}

EdgesDelegate *
AsIfFlatEdges::and_with (const Edges &other) const
{
  if (empty () || other.empty ()) {
    return new db::EmptyEdges ();
  } else {
    return boolean (&other, EdgeAnd);
  }
}

EdgesDelegate *
AsIfFlatEdges::not_with (const Edges &other) const
{
  if (empty ()) {
    return new db::EmptyEdges ();
  } else if (other.empty ()) {
    return clone ();
  } else {
    return boolean (&other, EdgeNot);
  }
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::andnot_with (const Edges &other) const
{
  if (empty ()) {
    return std::make_pair (new db::EmptyEdges (), new db::EmptyEdges ());
  } else if (other.empty ()) {
    return std::make_pair (new db::EmptyEdges (), clone ());
  } else {
    return boolean_andnot (&other);
  }
}

EdgesDelegate *
AsIfFlatEdges::and_with (const Region &other) const
{
  if (empty () || other.empty ()) {
    return new db::EmptyEdges ();
  } else {
    return edge_region_op (other, db::EdgePolygonOp::Inside, true /*include borders*/).first;
  }
}

EdgesDelegate *
AsIfFlatEdges::not_with (const Region &other) const
{
  if (empty ()) {
    return new db::EmptyEdges ();
  } else if (other.empty ()) {
    return clone ();
  } else {
    return edge_region_op (other, db::EdgePolygonOp::Outside, true /*include borders*/).first;
  }
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::andnot_with (const Region &other) const
{
  if (empty ()) {
    return std::make_pair (new db::EmptyEdges (), new db::EmptyEdges ());
  } else if (other.empty ()) {
    return std::make_pair (new db::EmptyEdges (), clone ());
  } else {
    return edge_region_op (other, db::EdgePolygonOp::Both, true /*include borders*/);
  }
}

EdgesDelegate *
AsIfFlatEdges::xor_with (const Edges &other) const
{
  if (empty ()) {
    return other.delegate ()->clone ();
  } else if (other.empty ()) {
    return clone ();
  } else {
    return boolean (&other, EdgeXor);
  }
}

EdgesDelegate *
AsIfFlatEdges::or_with (const Edges &other) const
{
  if (empty ()) {
    return other.delegate ()->clone ();
  } else if (other.empty ()) {
    return clone ();
  } else {
    return boolean (&other, EdgeOr);
  }
}

EdgesDelegate *
AsIfFlatEdges::intersections (const Edges &other) const
{
  if (empty () || other.empty ()) {
    return new db::EmptyEdges ();
  } else {
    return boolean (&other, EdgeIntersections);
  }
}

EdgesDelegate * 
AsIfFlatEdges::boolean (const Edges *other, EdgeBoolOp op) const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges (true));
  EdgeBooleanClusterCollectorToShapes cluster_collector (&output->raw_edges (), op);

  db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (count () + (other ? other->count () : 0));

  AddressableEdgeDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), 0);
  }

  AddressableEdgeDelivery ee;

  if (other) {
    ee = other->addressable_edges ();
    for ( ; ! ee.at_end (); ++ee) {
      scanner.insert (ee.operator-> (), 1);
    }
  }

  scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

  return output.release ();
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::boolean_andnot (const Edges *other) const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges (true));
  std::unique_ptr<FlatEdges> output2 (new FlatEdges (true));
  EdgeBooleanClusterCollectorToShapes cluster_collector (&output->raw_edges (), EdgeAndNot, &output2->raw_edges ());

  db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (count () + (other ? other->count () : 0));

  AddressableEdgeDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), 0);
  }

  AddressableEdgeDelivery ee;

  if (other) {
    ee = other->addressable_edges ();
    for ( ; ! ee.at_end (); ++ee) {
      scanner.insert (ee.operator-> (), 1);
    }
  }

  scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

  return std::make_pair (output.release (), output2.release ());
}

std::pair<EdgesDelegate *, EdgesDelegate *>
AsIfFlatEdges::edge_region_op (const Region &other, db::EdgePolygonOp::mode_t mode, bool include_borders) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    if (mode == db::EdgePolygonOp::Both) {
      return std::make_pair (new EmptyEdges (), clone ());
    } else if (mode == db::EdgePolygonOp::Inside) {
      return std::make_pair (new EmptyEdges (), (EdgesDelegate *) 0);
    } else {
      return std::make_pair (clone (), (EdgesDelegate *) 0);
    }
  }

  db::EdgeProcessor ep (report_progress (), progress_desc ());

  bool has_dots = false;

  for (db::Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, 0);
    }
  }

  for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
    if (e->is_degenerate ()) {
      has_dots = true;
    } else {
      ep.insert (*e, 1);
    }
  }

  std::unique_ptr<FlatEdges> output_second;
  std::unique_ptr<db::EdgeShapeGenerator> cc_second;
  if (mode == db::EdgePolygonOp::Both) {
    output_second.reset (new FlatEdges (false));
    cc_second.reset (new db::EdgeShapeGenerator (output_second->raw_edges (), true /*clear*/, 2 /*second tag*/));
  }

  std::unique_ptr<FlatEdges> output (new FlatEdges (false));
  db::EdgeShapeGenerator cc (output->raw_edges (), true /*clear*/, 1 /*tag*/, cc_second.get ());
  db::EdgePolygonOp op (mode, include_borders);
  ep.process (cc, op);

  //  process dots which are not captured by the booleans using the interaction function

  if (has_dots) {

    std::unique_ptr<FlatEdges> dots (new FlatEdges (false));
    for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
      if (e->is_degenerate ()) {
        dots->insert (*e);
      }
    }

    std::pair<EdgesDelegate *, EdgesDelegate *> res (0, 0);

    if (mode == EdgePolygonOp::Both) {
      res = dots->selected_interacting_pair_generic (other, include_borders ? EdgesInteract : EdgesInside, size_t (1), std::numeric_limits<size_t>::max ());
    } else if (mode == EdgePolygonOp::Inside) {
      res.first = dots->selected_interacting_generic (other, include_borders ? EdgesInteract : EdgesInside, false, size_t (1), std::numeric_limits<size_t>::max ());
    } else if (mode == EdgePolygonOp::Outside) {
      res.first = dots->selected_interacting_generic (other, include_borders ? EdgesInteract : EdgesOutside, include_borders, size_t (1), std::numeric_limits<size_t>::max ());
    }

    if (res.first) {
      output->add_in_place (db::Edges (res.first));
    }
    if (res.second) {
      output_second->add_in_place (db::Edges (res.second));
    }

  }

  return std::make_pair (output.release (), output_second.release ());
}

EdgesDelegate *
AsIfFlatEdges::add (const Edges &other) const
{
  const FlatEdges *other_flat = dynamic_cast<const FlatEdges *> (other.delegate ());
  if (other_flat) {

    std::unique_ptr<FlatEdges> new_edges (new FlatEdges (*other_flat));
    new_edges->set_is_merged (false);
    new_edges->invalidate_cache ();

    for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
      if (p.prop_id () == 0) {
        new_edges->raw_edges ().insert (*p);
      } else {
        new_edges->raw_edges ().insert (db::EdgeWithProperties (*p, p.prop_id ()));
      }
    }

    return new_edges.release ();

  } else {

    std::unique_ptr<FlatEdges> new_edges (new FlatEdges (false /*not merged*/));

    for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
      if (p.prop_id () == 0) {
        new_edges->raw_edges ().insert (*p);
      } else {
        new_edges->raw_edges ().insert (db::EdgeWithProperties (*p, p.prop_id ()));
      }
    }
    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      if (p.prop_id () == 0) {
        new_edges->raw_edges ().insert (*p);
      } else {
        new_edges->raw_edges ().insert (db::EdgeWithProperties (*p, p.prop_id ()));
      }
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
  if (count () != other.count ()) {
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
  if (count () != other.count ()) {
    return (count () < other.count ());
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
    if (e.prop_id () != 0) {
      shapes.insert (db::EdgeWithProperties (*e, e.prop_id ()));
    } else {
      shapes.insert (*e);
    }
  }
}

}

