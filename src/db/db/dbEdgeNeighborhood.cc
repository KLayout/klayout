
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

void
EdgeNeighborhoodCompoundOperationNode::do_collect_neighbors (db::box_scanner2<db::Edge, unsigned int, db::Polygon, unsigned int> &scanner, const db::Layout *layout, const db::Cell *cell) const
{


}

void
EdgeNeighborhoodCompoundOperationNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > & /*results*/, const db::LocalProcessorBase * /*proc*/) const
{
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

