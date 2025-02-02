
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

#include "dbPolygonNeighborhood.h"
#include "dbBoxScanner.h"
#include "dbClip.h"

namespace db
{

PolygonNeighborhoodVisitor::PolygonNeighborhoodVisitor ()
  : m_result_type (db::CompoundRegionOperationNode::ResultType::Edges)
{
  disconnect_outputs ();
}

void
PolygonNeighborhoodVisitor::connect_output (Layout * /*layout*/, std::unordered_set<db::PolygonWithProperties> *polygons) const
{
  disconnect_outputs ();
  mp_polygons = polygons;
}

void
PolygonNeighborhoodVisitor::connect_output (db::Layout *layout, std::unordered_set<db::PolygonRefWithProperties> *polygons) const
{
  disconnect_outputs ();
  mp_layout = layout;
  mp_polygon_refs = polygons;
}

void
PolygonNeighborhoodVisitor::connect_output (db::Layout * /*layout*/, std::unordered_set<db::EdgeWithProperties> *edges) const
{
  disconnect_outputs ();
  mp_edges = edges;
}

void
PolygonNeighborhoodVisitor::connect_output (Layout * /*layout*/, std::unordered_set<db::EdgePairWithProperties> *edge_pairs) const
{
  disconnect_outputs ();
  mp_edge_pairs = edge_pairs;
}

void
PolygonNeighborhoodVisitor::disconnect_outputs () const
{
  mp_layout = 0;
  mp_polygons = 0;
  mp_polygon_refs = 0;
  mp_edges = 0;
  mp_edge_pairs = 0;
}

void
PolygonNeighborhoodVisitor::output_polygon (const db::PolygonWithProperties &poly)
{
  if (mp_polygons) {
    mp_polygons->insert (poly);
  } else if (mp_polygon_refs) {
    tl_assert (mp_layout != 0);
    mp_polygon_refs->insert (db::PolygonRefWithProperties (db::PolygonRef (poly, mp_layout->shape_repository ()), poly.properties_id ()));
  } else {
    throw tl::Exception (tl::to_string (tr ("PolygonNeighborhoodVisitor is not configured for edge output (use 'result_type=Edges')")));
  }
}

void
PolygonNeighborhoodVisitor::output_edge (const db::EdgeWithProperties &edge)
{
  if (mp_edges == 0) {
    throw tl::Exception (tl::to_string (tr ("PolygonNeighborhoodVisitor is not configured for edge output (use 'result_type=Edges')")));
  }
  mp_edges->insert (edge);
}

void
PolygonNeighborhoodVisitor::output_edge_pair (const db::EdgePairWithProperties &edge_pair)
{
  if (mp_edge_pairs == 0) {
    throw tl::Exception (tl::to_string (tr ("PolygonNeighborhoodVisitor is not configured for edge pair output (use 'result_type=EdgePairs')")));
  }
  mp_edge_pairs->insert (edge_pair);
}

// --------------------------------------------------------------------------------------------------

PolygonNeighborhoodCompoundOperationNode::PolygonNeighborhoodCompoundOperationNode (const std::vector<CompoundRegionOperationNode *> &children, PolygonNeighborhoodVisitor *visitor, db::Coord dist)
  : CompoundRegionMultiInputOperationNode (children), m_dist (dist), mp_visitor (visitor)
{
  tl_assert (visitor != 0);
  visitor->keep ();
}

db::Coord
PolygonNeighborhoodCompoundOperationNode::computed_dist () const
{
  return m_dist;
}

std::string
PolygonNeighborhoodCompoundOperationNode::generated_description () const
{
  return tl::to_string (tr ("Polygon neighborhood collector"));
}

void
PolygonNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::EdgeWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRefWithProperties, db::EdgeWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
PolygonNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> &interactions, std::vector<std::unordered_set<db::EdgeWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonWithProperties, db::EdgeWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
PolygonNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> &interactions, std::vector<std::unordered_set<db::PolygonWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonWithProperties, db::PolygonWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
PolygonNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> &interactions, std::vector<std::unordered_set<db::EdgePairWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonWithProperties, db::EdgePairWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
PolygonNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::PolygonRefWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRefWithProperties, db::PolygonRefWithProperties> (cache, layout, cell, interactions, results, proc);
}

void
PolygonNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::EdgePairWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  compute_local_impl<db::PolygonRefWithProperties, db::EdgePairWithProperties> (cache, layout, cell, interactions, results, proc);
}

template <class T, class TR>
void
PolygonNeighborhoodCompoundOperationNode::compute_local_impl (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
{
  if (! mp_visitor) {
    return;
  }
  tl_assert (interactions.num_subjects () == 1);
  tl_assert (! results.empty ());

  try {

    mp_visitor->connect_output (layout, &results.front ());

    const T &pr = interactions.begin_subjects ()->second;
    db::PolygonWithProperties subject (pr.instantiate (), pr.properties_id ());

    PolygonNeighborhoodVisitor::neighbors_type neighbors;

    for (unsigned int i = 0; i < children (); ++i) {

      std::vector<db::PolygonWithProperties> &n = neighbors [i];

      std::vector<std::unordered_set<T> > others;
      others.push_back (std::unordered_set<T> ());

      shape_interactions<T, T> computed_interactions;
      child (i)->compute_local (cache, layout, cell, interactions_for_child (interactions, i, computed_interactions), others, proc);

      for (auto p = others.front ().begin (); p != others.front ().end (); ++p) {
        n.push_back (db::PolygonWithProperties (p->instantiate (), p->properties_id ()));
      }

    }

    const_cast<db::PolygonNeighborhoodVisitor *> (mp_visitor.get ())->neighbors (layout, cell, subject, neighbors);

    mp_visitor->disconnect_outputs ();

  } catch (...) {
    mp_visitor->disconnect_outputs ();
    throw;
  }
}

}

