
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


#include "pexSquareCountingRExtractor.h"
#include "dbBoxScanner.h"
#include "dbPolygonTools.h"
#include "tlIntervalMap.h"

namespace pex
{

//  Value used for number of squares for width 0 (should not happen)
const double infinite_squares = 1e10;

namespace
{

class PortInteractionReceiverBase
{
public:
  const std::set<size_t> &interactions (size_t index) const
  {
    static std::set<size_t> empty;
    auto i = m_interactions.find (index);
    if (i == m_interactions.end ()) {
      return empty;
    } else {
      return i->second;
    }
  }

protected:
  void insert (size_t index1, size_t index2)
  {
    m_interactions[index1].insert (index2);
  }

private:
  std::map<size_t, std::set<size_t> > m_interactions;
};

class PolygonPortInteractionReceiver
  : public db::box_scanner_receiver2<const db::Polygon, size_t, const db::Polygon, size_t>,
    public PortInteractionReceiverBase
{
public:
  void add (const db::Polygon *obj1, const size_t &index1, const db::Polygon *obj2, const size_t &index2)
  {
    if (db::interact_pp (*obj1, *obj2)) {
      insert (index1, index2);
    }
  }
};

class VertexPortInteractionReceiver
  : public db::box_scanner_receiver2<const db::Polygon, size_t, const db::Point, size_t>,
    public PortInteractionReceiverBase
{
public:
  void add (const db::Polygon *obj1, const size_t &index1, const db::Point *obj2, const size_t &index2)
  {
    if (obj1->box ().contains (*obj2) && db::inside_poly (obj1->begin_edge (), *obj2) >= 0) {
      insert (index1, index2);
    }
  }
};

struct JoinEdgeSets
{
  void operator() (std::set<db::Edge> &a, const std::set<db::Edge> &b) const
  {
    a.insert (b.begin (), b.end ());
  }
};

}

SquareCountingRExtractor::SquareCountingRExtractor (double dbu)
{
  m_dbu = dbu;
  m_skip_simplify = false;

  m_decomp_param.split_edges = true;
  m_decomp_param.with_segments = false;
}

static
double yatx (const db::Edge &e, int x)
{
  db::Point p1 = e.p1 (), p2 = e.p2 ();
  if (p1.x () > p2.x ()) {
    std::swap (p1, p2);
  }

  return p1.y () + double (p2.y () - p1.y ()) * double (x - p1.x ()) / double (p2.x () - p1.x ());
}

static
double calculate_squares (db::Coord x1, db::Coord x2, const std::set<db::Edge> &edges)
{
  tl_assert (edges.size () == 2);

  auto i = edges.begin ();
  db::Edge e1 = *i++;
  db::Edge e2 = *i;

  double w1 = fabs (yatx (e1, x1) - yatx (e2, x1));
  double w2 = fabs (yatx (e1, x2) - yatx (e2, x2));

  //  integrate the resistance along the axis x1->x2 with w=w1->w2

  if (w1 < db::epsilon) {
    return infinite_squares;
  } else if (fabs (w1 - w2) < db::epsilon) {
    return (x2 - x1) / w1;
  } else {
    return (x2 - x1) / (w2 - w1) * log (w2 / w1);
  }
}

void
SquareCountingRExtractor::do_extract (const db::Polygon &db_poly, const std::vector<std::pair<PortDefinition, pex::RNode *> > &ports, pex::RNetwork &rnetwork)
{
  //  "trans" will orient the polygon to be flat rather than tall
  db::Trans trans;
  if (db_poly.box ().width () < db_poly.box ().height ()) {
    trans = db::Trans (db::Trans::r90);
  }

  //  sort the edges into an interval map - as the polygons are convex, there
  //  can only be two edges in each interval.

  tl::interval_map<db::Coord, std::set<db::Edge> > edges;
  for (auto e = db_poly.begin_edge (); ! e.at_end (); ++e) {
    db::Edge et = trans * *e;
    if (et.x1 () != et.x2 ()) {
      std::set<db::Edge> es;
      es.insert (et);
      JoinEdgeSets jes;
      edges.add (std::min (et.p1 ().x (), et.p2 ().x ()), std::max (et.p1 ().x (), et.p2 ().x ()), es, jes);
    }
  }

  //  sort the port locations - note that we take the port box centers for the location!

  std::multimap<db::Coord, pex::RNode *> port_locations;
  for (auto p = ports.begin (); p != ports.end (); ++p) {
    db::Coord c = (trans * p->first.location).center ().x ();
    port_locations.insert (std::make_pair (c, p->second));
  }

  //  walk along the long axis of the polygon and compute the square count between the port locations

  for (auto pl = port_locations.begin (); pl != port_locations.end (); ++pl) {

    auto pl_next = pl;
    ++pl_next;
    if (pl_next == port_locations.end ()) {
      break;
    }

    db::Coord c = pl->first;
    db::Coord cc = pl_next->first;

    double r = 0.0;

    auto em = edges.find (c);
    while (em != edges.end () && em->first.first < cc) {
      r += calculate_squares (std::max (c, em->first.first), std::min (cc, em->first.second), em->second);
      ++em;
    }

    //  TODO: width dependency?
    if (r == 0) {
      rnetwork.create_element (pex::RElement::short_value (), pl->second, pl_next->second);
    } else {
      rnetwork.create_element (1.0 / r, pl->second, pl_next->second);
    }

  }
}

void
SquareCountingRExtractor::extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, pex::RNetwork &rnetwork)
{
  rnetwork.clear ();

  db::CplxTrans to_um (m_dbu);
  db::CplxTrans trans = to_um * db::ICplxTrans (db::Trans (db::Point () - polygon.box ().center ()));
  auto inv_trans = trans.inverted ();

  db::plc::Graph plc;

  db::plc::ConvexDecomposition decomp (&plc);
  decomp.decompose (polygon, m_decomp_param, trans);

  //  create a heap for the scanners

  std::vector<std::pair<db::Polygon, const db::plc::Polygon *> > decomp_polygons;
  for (auto p = plc.begin (); p != plc.end (); ++p) {
    decomp_polygons.push_back (std::make_pair (db::Polygon (), p.operator-> ()));
    decomp_polygons.back ().first = inv_trans * p->polygon ();
  }

  //  Set up a scanner to detect interactions between polygon ports
  //  and decomposed polygons

  PolygonPortInteractionReceiver interactions_pp;

  if (! decomp_polygons.empty () && ! polygon_ports.empty ()) {

    db::box_scanner2<const db::Polygon, size_t, const db::Polygon, size_t> scanner;

    for (auto i = decomp_polygons.begin (); i != decomp_polygons.end (); ++i) {
      scanner.insert1 (&i->first, i - decomp_polygons.begin ());
    }

    for (auto i = polygon_ports.begin (); i != polygon_ports.end (); ++i) {
      scanner.insert2 (i.operator-> (), i - polygon_ports.begin ());
    }

    db::box_convert<db::Polygon> bc;
    scanner.process (interactions_pp, 1, bc, bc);

  }

  //  Set up a scanner to detect interactions between vertex ports
  //  and decomposed polygons

  VertexPortInteractionReceiver interactions_vp;

  if (! decomp_polygons.empty () && ! vertex_ports.empty ()) {

    db::box_scanner2<const db::Polygon, size_t, const db::Point, size_t> scanner;

    for (auto i = decomp_polygons.begin (); i != decomp_polygons.end (); ++i) {
      scanner.insert1 (&i->first, i - decomp_polygons.begin ());
    }

    for (auto i = vertex_ports.begin (); i != vertex_ports.end (); ++i) {
      scanner.insert2 (i.operator-> (), i - vertex_ports.begin ());
    }

    db::box_convert<db::Polygon> bc1;
    db::box_convert<db::Point> bc2;
    scanner.process (interactions_vp, 1, bc1, bc2);

  }

  //  Generate the internal ports: those are defined by edges connecting two polygons

  std::vector<const db::plc::Edge *> internal_port_edges;
  std::map<const db::plc::Edge *, size_t> internal_ports;
  std::vector<std::vector<size_t> > internal_port_indexes;

  for (auto i = decomp_polygons.begin (); i != decomp_polygons.end (); ++i) {

    internal_port_indexes.push_back (std::vector<size_t> ());
    auto p = i->second;

    for (size_t j = 0; j < p->size (); ++j) {

      const db::plc::Edge *e = p->edge (int (j));
      if (e->left () && e->right ()) {

        auto ip = internal_ports.find (e);
        if (ip == internal_ports.end ()) {
          size_t n = internal_port_edges.size ();
          internal_port_edges.push_back (e);
          ip = internal_ports.insert (std::make_pair (e, n)).first;
        }
        internal_port_indexes.back ().push_back (ip->second);

      }

    }

  }

  //  Now we can extract the resistors

  std::vector<std::pair<PortDefinition, pex::RNode *> > ports;
  std::map<PortDefinition, pex::RNode *> nodes_for_ports;

  for (auto p = decomp_polygons.begin (); p != decomp_polygons.end (); ++p) {

    ports.clear ();

    const db::Polygon &db_poly = p->first;
    const std::set<size_t> &pp_indexes = interactions_pp.interactions (p - decomp_polygons.begin ());
    const std::set<size_t> &vp_indexes = interactions_vp.interactions (p - decomp_polygons.begin ());
    const std::vector<size_t> &ip_indexes = internal_port_indexes [p - decomp_polygons.begin ()];

    //  set up the ports:

    //  1. internal ports
    for (auto i = ip_indexes.begin (); i != ip_indexes.end (); ++i) {
      db::Box loc = (inv_trans * internal_port_edges [*i]->edge ()).bbox ();
      ports.push_back (std::make_pair (PortDefinition (pex::RNode::Internal, loc, (unsigned int) *i), (pex::RNode *) 0));
    }

    //  2. vertex ports
    for (auto i = vp_indexes.begin (); i != vp_indexes.end (); ++i) {
      db::Point loc = vertex_ports [*i];
      ports.push_back (std::make_pair (PortDefinition (pex::RNode::VertexPort, db::Box (loc, loc), (unsigned int) *i), (pex::RNode *) 0));
    }

    //  3. polygon ports
    for (auto i = pp_indexes.begin (); i != pp_indexes.end (); ++i) {
      db::Box loc = polygon_ports [*i].box ();
      ports.push_back (std::make_pair (PortDefinition (pex::RNode::PolygonPort, loc, (unsigned int) *i), (pex::RNode *) 0));
    }

    //  create nodes for the ports
    //  (we reuse nodes for existing ports in "nodes_for_ports", hence to establish the connection)

    for (auto p = ports.begin (); p != ports.end (); ++p) {
      auto n4p = nodes_for_ports.find (p->first);
      if (n4p == nodes_for_ports.end ()) {
        pex::RNode *node = rnetwork.create_node (p->first.type, p->first.port_index, 0);
        node->location = to_um * p->first.location;
        n4p = nodes_for_ports.insert (std::make_pair (p->first, node)).first;
      }
      p->second = n4p->second;
    }

    do_extract (db_poly, ports, rnetwork);

  }

  if (! m_skip_simplify) {
    rnetwork.simplify ();
  }
}

}


