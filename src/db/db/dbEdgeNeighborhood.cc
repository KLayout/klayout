
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

#include "dbEdgeNeighborhood.h"
#include "dbBoxScanner.h"
#include "dbClip.h"

namespace db
{

static db::IMatrix3d to_original_trans (const db::Edge &edge)
{
  //  compute normal and unit vector along edge
  db::DVector e = db::DVector (edge.d ());
  e = e * (1.0 / e.double_length ());
  db::DVector ne (-e.y (), e.x ());

  //  transform on the edge
  return db::IMatrix3d (e.x (), ne.x (), e.y (), ne.y (), edge.p1 ().x (), edge.p1 ().y (), 0.0, 0.0);
}

// --------------------------------------------------------------------------------------------------

EdgeNeighborhoodVisitor::EdgeNeighborhoodVisitor ()
  : m_result_type (db::CompoundRegionOperationNode::ResultType::Edges)
{
  disconnect_outputs ();
}

void
EdgeNeighborhoodVisitor::connect_output (Layout * /*layout*/, std::unordered_set<db::Polygon> *polygons) const
{
  disconnect_outputs ();
  mp_polygons = polygons;
}

void
EdgeNeighborhoodVisitor::connect_output (db::Layout *layout, std::unordered_set<db::PolygonRef> *polygons) const
{
  disconnect_outputs ();
  mp_layout = layout;
  mp_polygon_refs = polygons;
}

void
EdgeNeighborhoodVisitor::connect_output (db::Layout * /*layout*/, std::unordered_set<db::Edge> *edges) const
{
  disconnect_outputs ();
  mp_edges = edges;
}

void
EdgeNeighborhoodVisitor::connect_output (Layout * /*layout*/, std::unordered_set<db::EdgePair> *edge_pairs) const
{
  disconnect_outputs ();
  mp_edge_pairs = edge_pairs;
}

void
EdgeNeighborhoodVisitor::disconnect_outputs () const
{
  mp_layout = 0;
  mp_polygons = 0;
  mp_polygon_refs = 0;
  mp_edges = 0;
  mp_edge_pairs = 0;
}

void
EdgeNeighborhoodVisitor::output_polygon (const db::Polygon &poly)
{
  if (mp_polygons) {
    mp_polygons->insert (poly);
  } else if (mp_polygon_refs) {
    tl_assert (mp_layout != 0);
    mp_polygon_refs->insert (db::PolygonRef (poly, mp_layout->shape_repository ()));
  } else {
    throw tl::Exception (tl::to_string (tr ("EdgeNeighborhoodVisitor is not configured for edge output (use 'result_type=Edges')")));
  }
}

void
EdgeNeighborhoodVisitor::output_edge (const db::Edge &edge)
{
  if (mp_edges == 0) {
    throw tl::Exception (tl::to_string (tr ("EdgeNeighborhoodVisitor is not configured for edge output (use 'result_type=Edges')")));
  }
  mp_edges->insert (edge);
}

void
EdgeNeighborhoodVisitor::output_edge_pair (const db::EdgePair &edge_pair)
{
  if (mp_edge_pairs == 0) {
    throw tl::Exception (tl::to_string (tr ("EdgeNeighborhoodVisitor is not configured for edge pair output (use 'result_type=EdgePairs')")));
  }
  mp_edge_pairs->insert (edge_pair);
}

db::IMatrix3d
EdgeNeighborhoodVisitor::to_original_trans (const db::Edge &edge)
{
  return db::to_original_trans (edge);
}

db::IMatrix3d
EdgeNeighborhoodVisitor::to_edge_local_trans (const db::Edge &edge)
{
  return db::to_original_trans (edge).inverted ();
}

// --------------------------------------------------------------------------------------------------

EdgeNeighborhoodCompoundOperationNode::EdgeNeighborhoodCompoundOperationNode (const std::vector<CompoundRegionOperationNode *> &children, EdgeNeighborhoodVisitor *visitor, db::Coord bext, db::Coord eext, db::Coord din, db::Coord dout)
  : CompoundRegionMultiInputOperationNode (children), m_bext (bext), m_eext (eext), m_din (din), m_dout (dout), mp_visitor (visitor)
{
  tl_assert (visitor != 0);
  visitor->keep ();
}

db::Coord
EdgeNeighborhoodCompoundOperationNode::computed_dist () const
{
  return std::max (std::max (m_bext, m_eext), std::max (m_din, m_dout)) + 1;
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

  void add (const db::Edge *o1, const unsigned int &p1, const db::Polygon *o2, const unsigned int &p2)
  {
    m_edge_neighbors[p1][p2].push_back (o2);
    enter_edge (o1, p1);
  }

  void finish1 (const db::Edge *o1, const unsigned int &p1)
  {
    m_edge_neighbors[p1];
    enter_edge (o1, p1);
  }

  void finalize (bool)
  {
    for (auto en = m_edge_neighbors.begin (); en != m_edge_neighbors.end (); ++en) {
      commit_edge (m_edges[en->first], en->second);
    }
  }

private:
  std::map<unsigned int, std::map<unsigned int, std::vector<const db::Polygon *> > > m_edge_neighbors;
  std::vector<db::Edge> m_edges;
  db::EdgeNeighborhoodVisitor *mp_visitor;
  const db::Layout *mp_layout;
  const db::Cell *mp_cell;
  db::Coord m_bext, m_eext, m_din, m_dout;

  void enter_edge (const db::Edge *o1, const unsigned int &p1)
  {
    while (size_t (p1) >= m_edges.size ()) {
      m_edges.push_back (db::Edge ());
    }
    m_edges[p1] = *o1;
  }

  void commit_edge (const db::Edge &edge, const std::map<unsigned int, std::vector<const db::Polygon *> > &neighbors) const
  {
    if (edge.is_degenerate ()) {
      return;
    }

    db::IMatrix3d from_original_trans = db::to_original_trans (edge).inverted ();

    db::Edge ref_edge = from_original_trans * edge;
    tl_assert (ref_edge.dy () == 0);
    tl_assert (ref_edge.dx () > 0);

    std::set<db::Coord> xpos;
    db::Coord xmin = -m_bext - 1;
    db::Coord xmax = ref_edge.dx () + m_eext + 1;

    db::SimplePolygon per_edge_clip_box (db::Box (xmin, -m_din - 1, xmax, m_dout + 1));

    //  compute the merged neighbors
    std::map<unsigned int, std::vector<db::Polygon> > merged_neighbors;

    db::EdgeProcessor ep;
    for (auto n = neighbors.begin (); n != neighbors.end (); ++n) {

      ep.clear ();

      size_t id = 0;
      for (auto nn = n->second.begin (); nn != n->second.end (); ++nn) {
        for (auto e = (*nn)->begin_edge (); ! e.at_end (); ++e) {
          ep.insert (from_original_trans * *e, id);
        }
        id += 2;
      }

      ep.insert (per_edge_clip_box, size_t (1));

      db::BooleanOp and_op (db::BooleanOp::And);
      db::PolygonContainer pc (merged_neighbors [n->first]);
      db::PolygonGenerator pg (pc, false);
      ep.process (pg, and_op);

    }

    for (auto n = merged_neighbors.begin (); n != merged_neighbors.end (); ++n) {
      for (auto p = n->second.begin (); p != n->second.end (); ++p) {
        for (auto pe = p->begin_edge (); ! pe.at_end (); ++pe) {
          db::Edge e = *pe;
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

      bool first_per_interval = true;

      db::Box clip_box (xfrom, -m_din - 1, xto, m_dout + 1);

      //  NOTE: this could be more efficient if we had a multi-layer capable trapezoid decomposition tool
      for (auto n = merged_neighbors.begin (); n != merged_neighbors.end (); ++n) {

        EdgeNeighborhoodVisitor::neighbor_shapes_type polygons;

        for (auto p = n->second.begin (); p != n->second.end (); ++p) {
          db::clip_poly (*p, clip_box, polygons, false);
        }

        if (!polygons.empty ()) {

          if (first_per_interval) {
            first_per_interval = false;
            binned_neighbors.push_back (EdgeNeighborhoodVisitor::neighbors_type::value_type ());
            binned_neighbors.back ().first.first = xfrom;
            binned_neighbors.back ().first.second = xto;
          }

          EdgeNeighborhoodVisitor::neighbors_per_interval_type &i2n = binned_neighbors.back ().second;
          i2n[n->first].swap (polygons);

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
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRef, db::Edge> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::Polygon, db::Edge> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::Polygon, db::Polygon> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::Polygon, db::EdgePair> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRef, db::PolygonRef> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRef, db::EdgePair> (cache, layout, cell, interactions, results, proc);
}

template <class T, class TR>
void
EdgeNeighborhoodCompoundOperationNode::compute_local_impl (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
{
  if (! mp_visitor) {
    return;
  }
  tl_assert (interactions.num_subjects () == 1);
  tl_assert (! results.empty ());

  try {

    mp_visitor->connect_output (layout, &results.front ());

    db::box_scanner2<db::Edge, unsigned int, db::Polygon, unsigned int> scanner;

    std::list<db::Edge> edges;
    std::list<db::Polygon> polygons;

    for (unsigned int i = 0; i < children (); ++i) {

      std::vector<std::unordered_set<T> > others;
      others.push_back (std::unordered_set<T> ());

      shape_interactions<T, T> computed_interactions;
      child (i)->compute_local (cache, layout, cell, interactions_for_child (interactions, i, computed_interactions), others, proc);

      for (auto p = others.front ().begin (); p != others.front ().end (); ++p) {
        polygons.push_back (p->instantiate ());
        scanner.insert2 (&polygons.back (), i);
      }

    }

    const T &pr = interactions.begin_subjects ()->second;
    unsigned int ie = 0;
    for (auto e = pr.begin_edge (); ! e.at_end (); ++e, ++ie) {
      edges.push_back (*e);
      scanner.insert1 (&edges.back (), ie);
    }

    const_cast<db::EdgeNeighborhoodVisitor *> (mp_visitor.get ())->begin_polygon (layout, cell, pr.instantiate ());
    do_collect_neighbors (scanner, layout, cell);
    const_cast<db::EdgeNeighborhoodVisitor *> (mp_visitor.get ())->end_polygon ();

    mp_visitor->disconnect_outputs ();

  } catch (...) {
    mp_visitor->disconnect_outputs ();
    throw;
  }
}

}

