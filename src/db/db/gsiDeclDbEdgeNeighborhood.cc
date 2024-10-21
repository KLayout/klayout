
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


#include "gsiDecl.h"
#include "gsiEnums.h"

#include "tlThreads.h"
#include "dbEdgeNeighborhood.h"

namespace gsi
{

// ---------------------------------------------------------------------------------
//  EdgeFilter binding

class EdgeNeighborhoodVisitorImpl
  : public db::EdgeNeighborhoodVisitor
{
public:
  EdgeNeighborhoodVisitorImpl () { }

  void issue_on_edge (const db::Layout *, const db::Cell *, const db::Edge &, const tl::Variant &)
  {
    //  just for signature
  }

  void on_edge (const db::Layout *layout, const db::Cell *cell, const db::Edge &edge, const db::EdgeNeighborhoodVisitor::neighbors_type &neighbors)
  {
    if (f_on_edge.can_issue ()) {

      tl::Variant neighborhood = build_neighbors (neighbors);

      //  NOTE: as scripts are potentially thread unsafe, we lock here
      tl::MutexLocker locker (&m_lock);
      return f_on_edge.issue<EdgeNeighborhoodVisitorImpl, const db::Layout *, const db::Cell *, const db::Edge &, const tl::Variant &> (&EdgeNeighborhoodVisitorImpl::issue_on_edge, layout, cell, edge, neighborhood);

    }
  }

  gsi::Callback f_on_edge;

  static tl::Variant build_neighbors (const db::EdgeNeighborhoodVisitor::neighbors_type &neighbors)
  {
    tl::Variant result;
    result.set_list ();

    for (auto n = neighbors.begin (); n != neighbors.end (); ++n) {

      tl::Variant row;
      row.set_list ();

      tl::Variant interval;
      interval.set_list ();
      interval.push (n->first.first);
      interval.push (n->first.second);
      row.push (interval);

      tl::Variant nmap;
      nmap.set_array ();

      for (auto nn = n->second.begin (); nn != n->second.end (); ++nn) {
        nmap.insert (tl::Variant (nn->first), tl::Variant (nn->second));
      }

      row.push (nmap);

      result.push (row);

    }

    return result;
  }

private:
  //  No copying
  EdgeNeighborhoodVisitorImpl &operator= (const EdgeNeighborhoodVisitorImpl &);
  EdgeNeighborhoodVisitorImpl (const EdgeNeighborhoodVisitorImpl &);

  tl::Mutex m_lock;
};

Class<gsi::EdgeNeighborhoodVisitorImpl> decl_EdgeNeighborhoodVisitorImpl ("db", "EdgeNeighborhoodVisitor",
  gsi::callback ("on_edge", &EdgeNeighborhoodVisitorImpl::issue_on_edge, &EdgeNeighborhoodVisitorImpl::f_on_edge, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("edge"), gsi::arg ("neighborhood"),
    "@brief Is called for each edge with the edge neighbors\n"
    "This method is called for every edge on the input region. It delivers the edge and the edge neighborhood. "
    "The edge neighborhood is classified in intervals along the edge. The intervals are given by a range of "
    "positions along the edge - 0.0 being the beginning of the edge and positive values towards the end of the edge. "
    "For 'bext' and 'eext' larger than zero (see "
    "\\EdgeNeighborhoodCompoundOperationNode), the position can be negative or larger than the edge length.\n"
    "\n"
    "The structure of the neighbors is:\n"
    "\n"
    "@code\n"
    "[\n"
    "  [ [ from, to ], { input_index => polygons }\n"
    "]\n"
    "@/code\n"
    "\n"
    "'from' and 'to' are the positions of the interval, 'input_index' is the index of the input the neighbors are on "
    "(see 'children' argument of \\EdgeNeighborhoodCompoundOperationNode constructor), 'prop_id' is the properties ID of "
    "the neighbors and 'polygons' is a list of polygons describing the neighborhood.\n"
    "The polygons are projected on the edge - i.e. they are in a coordinate system where the edge is horizonal and "
    "goes from (0,0) to (length,0).\n"
    "\n"
    "The polygons are boxes for manhattan input and trapezoids in the general case.\n"
  ),
  "@brief A visitor for the neighborhood of edges in the input\n"
  "\n"
  "Objects of this class are passed to \\EdgeNeighborhoodCompoundOperationNode constructor to handle "
  "events on each edge of the primary input along with the neighborhood taken from the additional inputs.\n"
  "\n"
  "See \\on_edge for the description of the events delivered."
  "\n"
  "This class has been introduced in version 0.xx.\n" // @@@
);

// ---------------------------------------------------------------------------------
//  EdgeProcessor binding

static db::CompoundRegionOperationNode *new_edge_neighborhood (const std::vector<db::CompoundRegionOperationNode *> &children, db::EdgeNeighborhoodVisitor *visitor, const db::Coord bext, db::Coord eext, db::Coord din, db::Coord dout)
{
  return new db::EdgeNeighborhoodCompoundOperationNode (children, visitor, bext, eext, din, dout);
}

gsi::ClassExt<db::CompoundRegionOperationNode> decl_CompoundRegionOperationNode_ext (
  gsi::constructor ("new_edge_neighborhood", &new_edge_neighborhood, gsi::arg ("children"), gsi::arg ("visitor"), gsi::arg ("bext", 0), gsi::arg ("eext", 0), gsi::arg ("din", 0), gsi::arg ("dout", 0),
    "@brief Creates a new edge neighborhood collector\n"
    "\n"
    "@param children The inputs to use. The first one in the primary input, the others are neighbors.\n"
    "@param visitor The visitor object (see \\EdgeNeighborhoodVisitor) receiving the edge events.\n"
    "@param bext The search window extension to use at the edge beginning.\n"
    "@param eext The search window extension to use at the edge beginning.\n"
    "@param din The search window extension to use at the edge beginning.\n"
    "@param dout The search window extension to use at the edge beginning.\n"
  )
);

}

