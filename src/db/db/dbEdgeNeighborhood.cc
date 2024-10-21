
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "dbEdgeNeighborhood.h"
#include "dbBoxScanner.h"
#include "dbClip.h"

namespace db
{

EdgeNeighborhoodCompoundOperationNode::EdgeNeighborhoodCompoundOperationNode (const std::vector<CompoundRegionOperationNode *> &children, EdgeNeighborhoodVisitor *visitor, db::Coord bext, db::Coord eext, db::Coord din, db::Coord dout)
  : CompoundRegionMultiInputOperationNode (children), m_bext (bext), m_eext (eext), m_din (din), m_dout (dout), mp_visitor (visitor)
{
  tl_assert (visitor != 0);
  visitor->keep ();
}

db::Coord
EdgeNeighborhoodCompoundOperationNode::computed_dist () const
{
  return std::max (std::max (m_bext, m_eext), std::max (m_din, m_dout));
}

std::string
EdgeNeighborhoodCompoundOperationNode::generated_description () const
{
  return tl::to_string (tr ("Neighborhood collector"));
}

namespace {

class EdgeCollectorReceiver
  : public db::box_scanner_receiver2<db::Edge, unsigned int, db::Polygon, unsigned int>
{
public:
  EdgeCollectorReceiver (db::EdgeNeighborhoodVisitor *visitor, const db::Layout *layout, const db::Cell *cell,
                         db::Coord bext, db::Coord eext, db::Coord din, db::Coord dout)
    : mp_visitor (visitor), mp_layout (layout), mp_cell (cell),
      m_bext (bext), m_eext (eext), m_din (din), m_dout (dout)
  {
    //  .. nothing yet ..
  }

  void add (const db::Edge *o1, const unsigned int & /*p1*/, const db::Polygon *o2, const unsigned int &p2)
  {
    m_edge_neighbors[o1][p2].push_back (o2);
  }

  void finalize (bool)
  {
    for (auto en = m_edge_neighbors.begin (); en != m_edge_neighbors.end (); ++en) {
      commit_edge (*en->first, en->second);
    }
  }

private:
  std::map<const db::Edge *, std::map<unsigned int, std::vector<const db::Polygon *> > > m_edge_neighbors;
  db::EdgeNeighborhoodVisitor *mp_visitor;
  const db::Layout *mp_layout;
  const db::Cell *mp_cell;
  db::Coord m_bext, m_eext, m_din, m_dout;

  void commit_edge (const db::Edge &edge, const std::map<unsigned int, std::vector<const db::Polygon *> > &neighbors) const
  {
    if (edge.is_degenerate ()) {
      return;
    }

    db::DVector e = db::DVector (edge.d ());
    e = e * (1.0 / e.double_length ());

    db::DVector ne (-e.y (), e.x ());

    //  transform on the edge
    db::IMatrix2d trans (e.x (), ne.x (), e.y (), ne.y ());
    db::IMatrix2d itrans = trans.inverted ();

    db::Edge ref_edge = itrans * edge;
    tl_assert (ref_edge.dy () == 0);
    tl_assert (ref_edge.dx () > 0);

    std::set<db::Coord> xpos;
    db::Coord xmin = -m_bext - 1;
    db::Coord xmax = m_eext + 1;

    for (auto n = neighbors.begin (); n != neighbors.end (); ++n) {
      for (auto p = n->second.begin (); p != n->second.end (); ++p) {
        db::Polygon poly = itrans * **p;
        for (auto p = poly.begin_edge (); ! p.at_end (); ++p) {
          db::Edge e = *p;
          xpos.insert (std::max (xmin, std::min (xmax, e.p1 ().x ())));
          xpos.insert (std::max (xmin, std::min (xmax, e.p2 ().x ())));
        }
      }
    }

    EdgeNeighborhoodVisitor::neighbors_type binned_neighbors;

    for (auto i = xpos.begin (); i != xpos.end (); ) {

      db::Coord xfrom = *i;
      ++i;
      if (i == xpos.end ()) {
        break;
      }
      db::Coord xto = *i;

      binned_neighbors.push_back (EdgeNeighborhoodVisitor::neighbors_type::value_type ());
      binned_neighbors.back ().first.first = xfrom;
      binned_neighbors.back ().first.second = xto;

      db::Box clip_box (xfrom, -m_din - 1, xto, m_dout + 1);

      EdgeNeighborhoodVisitor::neighbors_per_interval_type &i2n = binned_neighbors.back ().second;

      //  NOTE: this could be more efficient if we had a multi-layer capable trapezoid decomposition tool
      for (auto n = neighbors.begin (); n != neighbors.end (); ++n) {
        for (auto p = n->second.begin (); p != n->second.end (); ++p) {
          db::Polygon poly = itrans * **p;
          db::clip_poly (poly, clip_box, i2n[n->first], false);
        }
      }

    }

    mp_visitor->on_edge (mp_layout, mp_cell, edge, binned_neighbors);
  }
};

}

void
EdgeNeighborhoodCompoundOperationNode::do_collect_neighbors (db::box_scanner2<db::Edge, unsigned int, db::Polygon, unsigned int> &scanner, const db::Layout *layout, const db::Cell *cell) const
{
  EdgeCollectorReceiver rec (const_cast<db::EdgeNeighborhoodVisitor *> (mp_visitor.get ()), layout, cell, m_bext, m_eext, m_din, m_dout);
  scanner.process (rec, computed_dist (), db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > & /*results*/, const db::LocalProcessorBase * /*proc*/) const
{
  if (! mp_visitor) {
    return;
  }
  tl_assert (interactions.num_subjects () == 1);

  db::box_scanner2<db::Edge, unsigned int, db::Polygon, unsigned int> scanner;

  std::list<db::Edge> edges;
  std::list<db::Polygon> polygons;

  for (auto i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    db::PolygonRef pr (i->second.second);
    polygons.push_back (pr.instantiate ());
    scanner.insert2 (&polygons.back (), i->second.first);
  }

  const db::PolygonRef &pr = interactions.begin_subjects ()->second;
  unsigned int ie = 0;
  for (auto e = pr.begin_edge (); ! e.at_end (); ++e, ++ie) {
    edges.push_back (*e);
    scanner.insert1 (&edges.back (), ie);
  }

  do_collect_neighbors (scanner, layout, cell);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > & /*results*/, const db::LocalProcessorBase * /*proc*/) const
{
  if (! mp_visitor) {
    return;
  }
  tl_assert (interactions.num_subjects () == 1);

  db::box_scanner2<db::Edge, unsigned int, db::Polygon, unsigned int> scanner;

  std::list<db::Edge> edges;
  std::list<db::Polygon> polygons;

  for (auto i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    polygons.push_back (i->second.second);
    scanner.insert2 (&polygons.back (), i->second.first);
  }

  const db::Polygon &pr = interactions.begin_subjects ()->second;
  unsigned int ie = 0;
  for (auto e = pr.begin_edge (); ! e.at_end (); ++e, ++ie) {
    edges.push_back (*e);
    scanner.insert1 (&edges.back (), ie);
  }

  do_collect_neighbors (scanner, layout, cell);
}

}

