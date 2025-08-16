
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


#include "dbAsIfFlatEdgePairs.h"
#include "dbFlatEdgePairs.h"
#include "dbFlatRegion.h"
#include "dbFlatEdges.h"
#include "dbEmptyEdgePairs.h"
#include "dbEmptyEdges.h"
#include "dbEmptyRegion.h"
#include "dbEdgePairs.h"
#include "dbEdgePairsLocalOperations.h"
#include "dbBoxConvert.h"
#include "dbRegion.h"
#include "dbHierProcessor.h"

#include <sstream>

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  AsIfFlagEdgePairs implementation

AsIfFlatEdgePairs::AsIfFlatEdgePairs ()
  : EdgePairsDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatEdgePairs::AsIfFlatEdgePairs (const AsIfFlatEdgePairs &other)
  : EdgePairsDelegate (other), m_bbox_valid (other.m_bbox_valid), m_bbox (other.m_bbox)
{
  operator= (other);
}

AsIfFlatEdgePairs &
AsIfFlatEdgePairs::operator= (const AsIfFlatEdgePairs &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }
  return *this;
}

AsIfFlatEdgePairs::~AsIfFlatEdgePairs ()
{
  //  .. nothing yet ..
}

std::string
AsIfFlatEdgePairs::to_string (size_t nmax) const
{
  std::ostringstream os;
  EdgePairsIterator p (begin ());
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

EdgePairsDelegate *
AsIfFlatEdgePairs::in (const EdgePairs &other, bool invert) const
{
  std::set <db::EdgePair> op;
  for (EdgePairsIterator o (other.begin ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());

  for (EdgePairsIterator o (begin ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_edge_pairs->insert (*o);
    }
  }

  return new_edge_pairs.release ();
}

size_t
AsIfFlatEdgePairs::count () const
{
  size_t n = 0;
  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    ++n;
  }
  return n;
}

size_t
AsIfFlatEdgePairs::hier_count () const
{
  return count ();
}

Box AsIfFlatEdgePairs::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatEdgePairs::compute_bbox () const
{
  db::Box b;
  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    b += e->bbox ();
  }
  return b;
}

void AsIfFlatEdgePairs::update_bbox (const db::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatEdgePairs::invalidate_bbox ()
{
  m_bbox_valid = false;
}

EdgePairsDelegate *
AsIfFlatEdgePairs::processed (const EdgePairProcessorBase &filter) const
{
  std::unique_ptr<FlatEdgePairs> edge_pairs (new FlatEdgePairs ());

  std::vector<db::EdgePairWithProperties> res_edge_pairs;

  for (EdgePairsIterator e = begin (); ! e.at_end (); ++e) {
    res_edge_pairs.clear ();
    filter.process (e.wp (), res_edge_pairs);
    for (auto er = res_edge_pairs.begin (); er != res_edge_pairs.end (); ++er) {
      if (er->properties_id () != 0) {
        edge_pairs->insert (*er);
      } else {
        edge_pairs->insert (er->base ());
      }
    }
  }

  return edge_pairs.release ();
}

RegionDelegate *
AsIfFlatEdgePairs::processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const
{
  std::unique_ptr<FlatRegion> region (new FlatRegion ());

  if (filter.result_must_not_be_merged ()) {
    region->set_merged_semantics (false);
  }

  std::vector<db::PolygonWithProperties> res_polygons;

  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    res_polygons.clear ();
    filter.process (e.wp (), res_polygons);
    for (auto pr = res_polygons.begin (); pr != res_polygons.end (); ++pr) {
      if (pr->properties_id () != 0) {
        region->insert (*pr);
      } else {
        region->insert (pr->base ());
      }
    }
  }

  return region.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const
{
  std::unique_ptr<FlatEdges> edges (new FlatEdges ());

  if (filter.result_must_not_be_merged ()) {
    edges->set_merged_semantics (false);
  }

  std::vector<db::EdgeWithProperties> res_edges;

  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    res_edges.clear ();
    filter.process (e.wp (), res_edges);
    for (auto pr = res_edges.begin (); pr != res_edges.end (); ++pr) {
      if (pr->properties_id () != 0) {
        edges->insert (*pr);
      } else {
        edges->insert (pr->base ());
      }
    }
  }

  return edges.release ();
}

static void
insert_ep (FlatEdgePairs *dest, const db::EdgePair &ep, db::properties_id_type prop_id)
{
  if (prop_id != 0) {
    dest->insert (db::EdgePairWithProperties (ep, prop_id));
  } else {
    dest->insert (ep);
  }
}

EdgePairsDelegate *
AsIfFlatEdgePairs::filtered (const EdgePairFilterBase &filter) const
{
  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());

  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    if (filter.selected (*p, p.prop_id ())) {
      insert_ep (new_edge_pairs.get (), *p, p.prop_id ());
    }
  }

  return new_edge_pairs.release ();
}

std::pair <EdgePairsDelegate *, EdgePairsDelegate *>
AsIfFlatEdgePairs::filtered_pair (const EdgePairFilterBase &filter) const
{
  std::unique_ptr<FlatEdgePairs> new_edge_pairs_true (new FlatEdgePairs ());
  std::unique_ptr<FlatEdgePairs> new_edge_pairs_false (new FlatEdgePairs ());

  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    FlatEdgePairs *dest = filter.selected (*p, p.prop_id ()) ? new_edge_pairs_true.get () : new_edge_pairs_false.get ();
    insert_ep (dest, *p, p.prop_id ());
  }

  return std::make_pair (new_edge_pairs_true.release (), new_edge_pairs_false.release ());
}

RegionDelegate *
AsIfFlatEdgePairs::pull_interacting (const Region &other) const
{
  return pull_generic (other);
}

EdgesDelegate *
AsIfFlatEdgePairs::pull_interacting (const Edges &other) const
{
  return pull_generic (other);
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_interacting (const Region &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, EdgePairsInteract, false, min_count, max_count);
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_not_interacting (const Region &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, EdgePairsInteract, true, min_count, max_count);
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_interacting (const Edges &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, false, min_count, max_count);
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_not_interacting (const Edges &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_generic (other, true, min_count, max_count);
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
AsIfFlatEdgePairs::selected_interacting_pair (const Region &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_pair_generic (other, EdgePairsInteract, min_count, max_count);
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
AsIfFlatEdgePairs::selected_interacting_pair (const Edges &other, size_t min_count, size_t max_count) const
{
  return selected_interacting_pair_generic (other, min_count, max_count);
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_outside (const Region &other) const
{
  return selected_interacting_generic (other, EdgePairsOutside, false, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_not_outside (const Region &other) const
{
  return selected_interacting_generic (other, EdgePairsOutside, true, size_t (1), std::numeric_limits<size_t>::max ());
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
AsIfFlatEdgePairs::selected_outside_pair (const Region &other) const
{
  return selected_interacting_pair_generic (other, EdgePairsOutside, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_inside (const Region &other) const
{
  return selected_interacting_generic (other, EdgePairsInside, false, size_t (1), std::numeric_limits<size_t>::max ());
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_not_inside (const Region &other) const
{
  return selected_interacting_generic (other, EdgePairsInside, true, size_t (1), std::numeric_limits<size_t>::max ());
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
AsIfFlatEdgePairs::selected_inside_pair (const Region &other) const
{
  return selected_interacting_pair_generic (other, EdgePairsInside, size_t (1), std::numeric_limits<size_t>::max ());
}

namespace {

class OutputPairHolder
{
public:
  OutputPairHolder (int inverse, bool merged_semantics)
  {
    m_e1.reset (new FlatEdgePairs (merged_semantics));
    m_results.push_back (& m_e1->raw_edge_pairs ());

    if (inverse == 0) {
      m_e2.reset (new FlatEdgePairs (merged_semantics));
      m_results.push_back (& m_e2->raw_edge_pairs ());
    }
  }

  std::pair<EdgePairsDelegate *, EdgePairsDelegate *> region_pair ()
  {
    return std::make_pair (m_e1.release (), m_e2.release ());
  }

  const std::vector<db::Shapes *> &results () { return m_results; }

private:
  std::unique_ptr<FlatEdgePairs> m_e1, m_e2;
  std::vector<db::Shapes *> m_results;
};

}

EdgesDelegate *
AsIfFlatEdgePairs::pull_generic (const Edges &other) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyEdges ();
  }

  db::box_scanner2<db::EdgePair, size_t, db::Edge, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgePairDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressableEdgeDelivery p = other.addressable_merged_edges ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::unique_ptr<FlatEdges> output (new FlatEdges (true));

  edge_pair_to_edge_interaction_filter<FlatEdges> filter (output.get (), size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Edge> ());

  return output.release ();
}

RegionDelegate *
AsIfFlatEdgePairs::pull_generic (const Region &other) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyRegion ();
  }

  db::box_scanner2<db::EdgePair, size_t, db::Polygon, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgePairDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressablePolygonDelivery p = other.addressable_merged_polygons ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::unique_ptr<FlatRegion> output (new FlatRegion (true));

  edge_pair_to_polygon_interaction_filter<FlatRegion> filter (output.get (), EdgePairsInteract, size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Polygon> ());

  return output.release ();
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_interacting_generic (const Edges &other, bool inverse, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    return inverse ? clone () : new EmptyEdgePairs ();
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (inverse ? 1 : -1, merged_semantics () || is_merged ());

  db::EdgePairsIterator edges (begin ());

  db::EdgePair2EdgeInteractingLocalOperation op (inverse ? db::EdgePair2EdgeInteractingLocalOperation::Inverse : db::EdgePair2EdgeInteractingLocalOperation::Normal, min_count, max_count);

  db::local_processor<db::EdgePair, db::Edge, db::EdgePair> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Edge> > others;
  //  NOTE: with counting the other edge collection needs to be merged
  others.push_back (counting ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ().first;
}

EdgePairsDelegate *
AsIfFlatEdgePairs::selected_interacting_generic (const Region &other, EdgePairInteractionMode mode, bool inverse, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    return ((mode == EdgePairsOutside) == inverse) ? new EmptyEdgePairs () : clone ();
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (inverse ? 1 : -1, merged_semantics () || is_merged ());

  db::EdgePairsIterator edges (begin ());

  db::edge_pair_to_polygon_interacting_local_operation<db::Polygon> op (mode, inverse ? db::edge_pair_to_polygon_interacting_local_operation<db::Polygon>::Inverse : db::edge_pair_to_polygon_interacting_local_operation<db::Polygon>::Normal, min_count, max_count);

  db::local_processor<db::EdgePair, db::Polygon, db::EdgePair> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Polygon> > others;
  //  NOTE: with counting the other region needs to be merged
  others.push_back (counting || mode != EdgePairsInteract ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ().first;
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
AsIfFlatEdgePairs::selected_interacting_pair_generic (const Edges &other, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    return std::make_pair (new EmptyEdgePairs (), clone ());
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (0, merged_semantics () || is_merged ());

  db::EdgePairsIterator edges (begin ());

  db::EdgePair2EdgeInteractingLocalOperation op (db::EdgePair2EdgeInteractingLocalOperation::Both, min_count, max_count);

  db::local_processor<db::EdgePair, db::Edge, db::EdgePair> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Edge> > others;
  //  NOTE: with counting the other region needs to be merged
  others.push_back (counting ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
AsIfFlatEdgePairs::selected_interacting_pair_generic (const Region &other, EdgePairInteractionMode mode, size_t min_count, size_t max_count) const
{
  min_count = std::max (size_t (1), min_count);

  //  shortcuts
  if (max_count < min_count || other.empty () || empty ()) {
    if (mode != EdgePairsOutside) {
      return std::make_pair (new EmptyEdgePairs (), clone ());
    } else {
      return std::make_pair (clone (), new EmptyEdgePairs ());
    }
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  OutputPairHolder oph (0, merged_semantics () || is_merged ());

  db::EdgePairsIterator edges (begin ());

  db::edge_pair_to_polygon_interacting_local_operation<db::Polygon> op (mode, db::edge_pair_to_polygon_interacting_local_operation<db::Polygon>::Both, min_count, max_count);

  db::local_processor<db::EdgePair, db::Polygon, db::EdgePair> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Polygon> > others;
  //  NOTE: with counting the other region needs to be merged
  others.push_back (counting || mode != EdgePairsInteract ? other.begin_merged () : other.begin ());

  proc.run_flat (edges, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}

RegionDelegate *
AsIfFlatEdgePairs::polygons (db::Coord e) const
{
  std::unique_ptr<FlatRegion> output (new FlatRegion ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::Polygon poly = ep->normalized ().to_polygon (e);
    if (poly.vertices () >= 3) {
      db::properties_id_type prop_id = ep.prop_id ();
      if (prop_id != 0) {
        output->insert (db::PolygonWithProperties (poly, prop_id));
      } else {
        output->insert (poly);
      }
    }
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::edges () const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::properties_id_type prop_id = ep.prop_id ();
    if (prop_id != 0) {
      output->insert (db::EdgeWithProperties (ep->first (), prop_id));
      output->insert (db::EdgeWithProperties (ep->second (), prop_id));
    } else {
      output->insert (ep->first ());
      output->insert (ep->second ());
    }
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::first_edges () const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::properties_id_type prop_id = ep.prop_id ();
    if (prop_id != 0) {
      output->insert (db::EdgeWithProperties (ep->first (), prop_id));
    } else {
      output->insert (ep->first ());
    }
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::second_edges () const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::properties_id_type prop_id = ep.prop_id ();
    if (prop_id != 0) {
      output->insert (db::EdgeWithProperties (ep->second (), prop_id));
    } else {
      output->insert (ep->second ());
    }
  }

  return output.release ();
}

EdgePairsDelegate *
AsIfFlatEdgePairs::add (const EdgePairs &other) const
{
  const FlatEdgePairs *other_flat = dynamic_cast<const FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs (*other_flat));
    new_edge_pairs->invalidate_cache ();

    for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = p.prop_id ();
      if (prop_id) {
        new_edge_pairs->raw_edge_pairs ().insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->raw_edge_pairs ().insert (*p);
      }
    }

    return new_edge_pairs.release ();

  } else {

    std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());

    for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = p.prop_id ();
      if (prop_id) {
        new_edge_pairs->raw_edge_pairs ().insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->raw_edge_pairs ().insert (*p);
      }
    }
    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = p.prop_id ();
      if (prop_id) {
        new_edge_pairs->raw_edge_pairs ().insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->raw_edge_pairs ().insert (*p);
      }
    }

    return new_edge_pairs.release ();

  }
}

bool
AsIfFlatEdgePairs::equals (const EdgePairs &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (count () != other.count ()) {
    return false;
  }
  EdgePairsIterator o1 (begin ());
  EdgePairsIterator o2 (other.begin ());
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
AsIfFlatEdgePairs::less (const EdgePairs &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (count () != other.count ()) {
    return (count () < other.count ());
  }
  EdgePairsIterator o1 (begin ());
  EdgePairsIterator o2 (other.begin ());
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
AsIfFlatEdgePairs::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    db::properties_id_type prop_id = e.prop_id ();
    if (prop_id) {
      shapes.insert (db::EdgePairWithProperties (*e, prop_id));
    } else {
      shapes.insert (*e);
    }
  }
}

void
AsIfFlatEdgePairs::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    db::properties_id_type prop_id = e.prop_id ();
    if (prop_id) {
      shapes.insert (db::SimplePolygonWithProperties (e->normalized ().to_simple_polygon (enl), prop_id));
    } else {
      shapes.insert (e->normalized ().to_simple_polygon (enl));
    }
  }
}

}

