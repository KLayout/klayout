
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
#include "dbEmptyEdges.h"
#include "dbEdges.h"
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
//  JoinEdgesCluster implementation

JoinEdgesCluster::JoinEdgesCluster (db::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
  : mp_output (output), m_ext_b (ext_b), m_ext_e (ext_e), m_ext_o (ext_o), m_ext_i (ext_i)
{
  //  .. nothing yet ..
}

void
JoinEdgesCluster::finish ()
{
  std::multimap<db::Point, iterator> objects_by_p1;
  std::multimap<db::Point, iterator> objects_by_p2;
  for (iterator o = begin (); o != end (); ++o) {
    if (o->first->p1 () != o->first->p2 ()) {
      objects_by_p1.insert (std::make_pair (o->first->p1 (), o));
      objects_by_p2.insert (std::make_pair (o->first->p2 (), o));
    }
  }

  while (! objects_by_p2.empty ()) {

    tl_assert (! objects_by_p1.empty ());

    //  Find the beginning of a new sequence
    std::multimap<db::Point, iterator>::iterator j0 = objects_by_p1.begin ();
    std::multimap<db::Point, iterator>::iterator j = j0;
    do {
      std::multimap<db::Point, iterator>::iterator jj = objects_by_p2.find (j->first);
      if (jj == objects_by_p2.end ()) {
        break;
      } else {
        j = objects_by_p1.find (jj->second->first->p1 ());
        tl_assert (j != objects_by_p1.end ());
      }
    } while (j != j0);

    iterator i = j->second;

    //  determine a sequence
    //  TODO: this chooses any solution in case of forks. Choose a specific one?
    std::vector<db::Point> pts;
    pts.push_back (i->first->p1 ());

    do {

      //  record the next point
      pts.push_back (i->first->p2 ());

      //  remove the edge as it's taken
      std::multimap<db::Point, iterator>::iterator jj;
      for (jj = objects_by_p2.find (i->first->p2 ()); jj != objects_by_p2.end () && jj->first == i->first->p2 (); ++jj) {
        if (jj->second == i) {
          break;
        }
      }
      tl_assert (jj != objects_by_p2.end () && jj->second == i);
      objects_by_p2.erase (jj);
      objects_by_p1.erase (j);

      //  process along the edge to the next one
      //  TODO: this chooses any solution in case of forks. Choose a specific one?
      j = objects_by_p1.find (i->first->p2 ());
      if (j != objects_by_p1.end ()) {
        i = j->second;
      } else {
        break;
      }

    } while (true);

    bool cyclic = (pts.back () == pts.front ());

    if (! cyclic) {

      //  non-cyclic sequence
      db::Path path (pts.begin (), pts.end (), 0, m_ext_b, m_ext_e, false);
      std::vector<db::Point> hull;
      path.hull (hull, m_ext_o, m_ext_i);
      db::Polygon poly;
      poly.assign_hull (hull.begin (), hull.end ());
      mp_output->put (poly);

    } else {

      //  we have a loop: form a contour by using the polygon size functions and a "Not" to form the hole
      db::Polygon poly;
      poly.assign_hull (pts.begin (), pts.end ());

      db::EdgeProcessor ep;
      db::PolygonGenerator pg (*mp_output, false, true);

      int mode_a = -1, mode_b = -1;

      if (m_ext_o == 0) {
        ep.insert (poly, 0);
      } else {
        db::Polygon sized_poly (poly);
        sized_poly.size (m_ext_o, m_ext_o, 2 /*sizing mode*/);
        ep.insert (sized_poly, 0);
        mode_a = 1;
      }

      if (m_ext_i == 0) {
        ep.insert (poly, 1);
      } else {
        db::Polygon sized_poly (poly);
        sized_poly.size (-m_ext_i, -m_ext_i, 2 /*sizing mode*/);
        ep.insert (sized_poly, 1);
        mode_b = 1;
      }

      db::BooleanOp2 op (db::BooleanOp::ANotB, mode_a, mode_b);
      ep.process (pg, op);

    }

  }
}

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

db::Polygon
AsIfFlatEdges::extended_edge (const db::Edge &edge, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
{
  db::DVector d;
  if (edge.is_degenerate ()) {
    d = db::DVector (1.0, 0.0);
  } else {
    d = db::DVector (edge.d ()) * (1.0 / edge.double_length ());
  }

  db::DVector n (-d.y (), d.x ());

  db::Point pts[4] = {
    db::Point (db::DPoint (edge.p1 ()) - d * double (ext_b) + n * double (ext_o)),
    db::Point (db::DPoint (edge.p2 ()) + d * double (ext_e) + n * double (ext_o)),
    db::Point (db::DPoint (edge.p2 ()) + d * double (ext_e) - n * double (ext_i)),
    db::Point (db::DPoint (edge.p1 ()) - d * double (ext_b) - n * double (ext_i)),
  };

  db::Polygon poly;
  poly.assign_hull (pts + 0, pts + 4);
  return poly;
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

db::Edge
AsIfFlatEdges::compute_partial (const db::Edge &edge, int mode, length_type length, double fraction)
{
  double l = std::max (edge.double_length () * fraction, double (length));

  if (mode < 0) {

    return db::Edge (edge.p1 (), db::Point (db::DPoint (edge.p1 ()) + db::DVector (edge.d ()) * (l / edge.double_length ())));

  } else if (mode > 0) {

    return db::Edge (db::Point (db::DPoint (edge.p2 ()) - db::DVector (edge.d ()) * (l / edge.double_length ())), edge.p2 ());

  } else {

    db::DVector dl = db::DVector (edge.d ()) * (0.5 * l / edge.double_length ());
    db::DPoint center = db::DPoint (edge.p1 ()) + db::DVector (edge.p2 () - edge.p1 ()) * 0.5;

    return db::Edge (db::Point (center - dl), db::Point (center + dl));

  }
}

EdgesDelegate *
AsIfFlatEdges::segments (int mode, length_type length, double fraction) const
{
  std::auto_ptr<FlatEdges> edges (new FlatEdges ());
  edges->reserve (size ());

  //  zero-length edges would vanish in merged sematics, so we don't set it now
  if (length == 0) {
    edges->set_merged_semantics (false);
  }

  for (EdgesIterator e (begin_merged ()); ! e.at_end (); ++e) {
    edges->insert (compute_partial (*e, mode, length, fraction));
  }

  return edges.release ();
}

EdgesDelegate *
AsIfFlatEdges::start_segments (length_type length, double fraction) const
{
  return segments (-1, length, fraction);
}

EdgesDelegate *
AsIfFlatEdges::end_segments (length_type length, double fraction) const
{
  return segments (1, length, fraction);
}

EdgesDelegate *
AsIfFlatEdges::centers (length_type length, double fraction) const
{
  return segments (0, length, fraction);
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

namespace
{

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 *
 *  If will perform a edge by edge check using the provided EdgeRelationFilter
 */
template <class Output>
class edge2edge_check_for_edges
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  edge2edge_check_for_edges (const EdgeRelationFilter &check, Output &output, bool requires_different_layers)
    : mp_check (&check), mp_output (&output)
  {
    m_requires_different_layers = requires_different_layers;
  }
  
  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  { 
    //  Overlap or inside checks require input from different layers
    if (! m_requires_different_layers || ((p1 ^ p2) & 1) != 0) {

      //  ensure that the first check argument is of layer 1 and the second of
      //  layer 2 (unless both are of the same layer)
      int l1 = int (p1 & size_t (1));
      int l2 = int (p2 & size_t (1));

      db::EdgePair ep;
      if (mp_check->check (l1 <= l2 ? *o1 : *o2, l1 <= l2 ? *o2 : *o1, &ep)) {
        mp_output->insert (ep);
      }

    }
  }

private:
  const EdgeRelationFilter *mp_check;
  Output *mp_output;
  bool m_requires_different_layers;
};

}

EdgePairs 
AsIfFlatEdges::run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  EdgePairs result;

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
  check.set_include_zero (other != 0);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  edge2edge_check_for_edges<db::EdgePairs> edge_check (check, result, other != 0);
  scanner.process (edge_check, d, db::box_convert<db::Edge> ());

  return result;
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

