
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
EdgeNeighborhoodVisitor::connect_output (Layout * /*layout*/, std::unordered_set<db::PolygonWithProperties> *polygons) const
{
  disconnect_outputs ();
  mp_polygons = polygons;
}

void
EdgeNeighborhoodVisitor::connect_output (db::Layout *layout, std::unordered_set<db::PolygonRefWithProperties> *polygons) const
{
  disconnect_outputs ();
  mp_layout = layout;
  mp_polygon_refs = polygons;
}

void
EdgeNeighborhoodVisitor::connect_output (db::Layout * /*layout*/, std::unordered_set<db::EdgeWithProperties> *edges) const
{
  disconnect_outputs ();
  mp_edges = edges;
}

void
EdgeNeighborhoodVisitor::connect_output (Layout * /*layout*/, std::unordered_set<db::EdgePairWithProperties> *edge_pairs) const
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
EdgeNeighborhoodVisitor::output_polygon (const db::PolygonWithProperties &poly)
{
  if (mp_polygons) {
    mp_polygons->insert (poly);
  } else if (mp_polygon_refs) {
    tl_assert (mp_layout != 0);
    mp_polygon_refs->insert (db::PolygonRefWithProperties (db::PolygonRef (poly, mp_layout->shape_repository ()), poly.properties_id ()));
  } else {
    throw tl::Exception (tl::to_string (tr ("EdgeNeighborhoodVisitor is not configured for edge output (use 'result_type=Edges')")));
  }
}

void
EdgeNeighborhoodVisitor::output_edge (const db::EdgeWithProperties &edge)
{
  if (mp_edges == 0) {
    throw tl::Exception (tl::to_string (tr ("EdgeNeighborhoodVisitor is not configured for edge output (use 'result_type=Edges')")));
  }
  mp_edges->insert (edge);
}

void
EdgeNeighborhoodVisitor::output_edge_pair (const db::EdgePairWithProperties &edge_pair)
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
  : public db::box_scanner_receiver2<db::EdgeWithProperties, unsigned int, db::PolygonWithProperties, unsigned int>
{
public:
  EdgeCollectorReceiver (db::EdgeNeighborhoodVisitor *visitor, const db::Layout *layout, const db::Cell *cell,
                         db::Coord bext, db::Coord eext, db::Coord din, db::Coord dout)
    : mp_visitor (visitor), mp_layout (layout), mp_cell (cell),
      m_bext (bext), m_eext (eext), m_din (din), m_dout (dout)
  {
    //  .. nothing yet ..
  }

  void add (const db::EdgeWithProperties *o1, const unsigned int &p1, const db::PolygonWithProperties *o2, const unsigned int &p2)
  {
    m_edge_neighbors[p1][p2].push_back (o2);
    enter_edge (o1, p1);
  }

  void finish1 (const db::EdgeWithProperties *o1, const unsigned int &p1)
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
  std::map<unsigned int, std::map<unsigned int, std::vector<const db::PolygonWithProperties *> > > m_edge_neighbors;
  std::vector<db::EdgeWithProperties> m_edges;
  db::EdgeNeighborhoodVisitor *mp_visitor;
  const db::Layout *mp_layout;
  const db::Cell *mp_cell;
  db::Coord m_bext, m_eext, m_din, m_dout;

  /**
   *  @brief A compare function that compares a pair of layer and properties ID using the properties values
   */
  struct CompareLayerAndPropertiesId
  {
    bool operator() (const std::pair<unsigned int, db::properties_id_type> &a, const std::pair<unsigned int, db::properties_id_type> &b) const
    {
      if (a.first != b.first) {
        return a.first < b.first;
      }
      return db::properties_id_less (a.second, b.second);
    }
  };

  void enter_edge (const db::EdgeWithProperties *o1, const unsigned int &p1)
  {
    while (size_t (p1) >= m_edges.size ()) {
      m_edges.push_back (db::EdgeWithProperties ());
    }
    m_edges[p1] = *o1;
  }

  void commit_edge (const db::EdgeWithProperties &edge, const std::map<unsigned int, std::vector<const db::PolygonWithProperties *> > &neighbors) const
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
    //  NOTE: we first separate by layer and properties ID before we merge. Hence
    //  shapes with different properties IDs are kept separate.

    std::map<unsigned int, std::vector<db::PolygonWithProperties> > merged_neighbors;

    //  NOTE: using a by-value compare for the properties ID makes the result order predictable
    std::map<std::pair<unsigned int, db::properties_id_type>, std::vector<const db::Polygon *>, CompareLayerAndPropertiesId> neighbors_by_prop_ids;
    for (auto n = neighbors.begin (); n != neighbors.end (); ++n) {
      for (auto p = n->second.begin (); p != n->second.end (); ++p) {
        neighbors_by_prop_ids [std::make_pair (n->first, (*p)->properties_id ())].push_back (*p);
      }
    }

    db::EdgeProcessor ep;
    for (auto n = neighbors_by_prop_ids.begin (); n != neighbors_by_prop_ids.end (); ++n) {

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
      db::PolygonContainerWithProperties pc (merged_neighbors [n->first.first], n->first.second);
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
EdgeNeighborhoodCompoundOperationNode::do_collect_neighbors (db::box_scanner2<db::EdgeWithProperties, unsigned int, db::PolygonWithProperties, unsigned int> &scanner, const db::Layout *layout, const db::Cell *cell) const
{
  EdgeCollectorReceiver rec (const_cast<db::EdgeNeighborhoodVisitor *> (mp_visitor.get ()), layout, cell, m_bext, m_eext, m_din, m_dout);
  scanner.process (rec, computed_dist (), db::box_convert<db::EdgeWithProperties> (), db::box_convert<db::PolygonWithProperties> ());
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::EdgeWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRefWithProperties, db::EdgeWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> &interactions, std::vector<std::unordered_set<db::EdgeWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonWithProperties, db::EdgeWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> &interactions, std::vector<std::unordered_set<db::PolygonWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonWithProperties, db::PolygonWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> &interactions, std::vector<std::unordered_set<db::EdgePairWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonWithProperties, db::EdgePairWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::PolygonRefWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRefWithProperties, db::PolygonRefWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::EdgePairWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRefWithProperties, db::EdgePairWithProperties> (cache, layout, cell, interactions, results, proc);
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

    db::box_scanner2<db::EdgeWithProperties, unsigned int, db::PolygonWithProperties, unsigned int> scanner;

    std::list<db::EdgeWithProperties> edges;
    std::list<db::PolygonWithProperties> polygons;

    for (unsigned int i = 0; i < children (); ++i) {

      std::vector<std::unordered_set<T> > others;
      others.push_back (std::unordered_set<T> ());

      shape_interactions<T, T> computed_interactions;
      child (i)->compute_local (cache, layout, cell, interactions_for_child (interactions, i, computed_interactions), others, proc);

      for (auto p = others.front ().begin (); p != others.front ().end (); ++p) {
        polygons.push_back (db::PolygonWithProperties (p->instantiate (), p->properties_id ()));
        scanner.insert2 (&polygons.back (), i);
      }

    }

    const T &pr = interactions.begin_subjects ()->second;
    unsigned int ie = 0;
    for (auto e = pr.begin_edge (); ! e.at_end (); ++e, ++ie) {
      edges.push_back (db::EdgeWithProperties (*e, pr.properties_id ()));
      scanner.insert1 (&edges.back (), ie);
    }

    const_cast<db::EdgeNeighborhoodVisitor *> (mp_visitor.get ())->begin_polygon (layout, cell, db::PolygonWithProperties (pr.instantiate (), pr.properties_id ()));
    do_collect_neighbors (scanner, layout, cell);
    const_cast<db::EdgeNeighborhoodVisitor *> (mp_visitor.get ())->end_polygon ();

    mp_visitor->disconnect_outputs ();

  } catch (...) {
    mp_visitor->disconnect_outputs ();
    throw;
  }
}

}

