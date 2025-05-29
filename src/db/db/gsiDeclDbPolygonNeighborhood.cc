
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


#include "gsiDecl.h"
#include "gsiEnums.h"

#include "tlThreads.h"
#include "dbPolygonNeighborhood.h"

namespace gsi
{

// ---------------------------------------------------------------------------------
//  EdgeFilter binding

class PolygonNeighborhoodVisitorImpl
  : public db::PolygonNeighborhoodVisitor
{
public:
  PolygonNeighborhoodVisitorImpl () { }

  void issue_neighbors (const db::Layout *, const db::Cell *, const db::PolygonWithProperties &, const db::PolygonNeighborhoodVisitor::neighbors_type &)
  {
    //  just for signature
  }

  void neighbors (const db::Layout *layout, const db::Cell *cell, const db::PolygonWithProperties &polygon, const db::PolygonNeighborhoodVisitor::neighbors_type &neighbors)
  {
    if (f_neighbors.can_issue ()) {

      //  NOTE: as scripts are potentially thread unsafe, we lock here
      tl::MutexLocker locker (&m_lock);
      return f_neighbors.issue<PolygonNeighborhoodVisitorImpl, const db::Layout *, const db::Cell *, const db::PolygonWithProperties &, const db::PolygonNeighborhoodVisitor::neighbors_type &> (&PolygonNeighborhoodVisitorImpl::issue_neighbors, layout, cell, polygon, neighbors);

    }
  }

  gsi::Callback f_neighbors;

private:
  //  No copying
  PolygonNeighborhoodVisitorImpl &operator= (const PolygonNeighborhoodVisitorImpl &);
  PolygonNeighborhoodVisitorImpl (const PolygonNeighborhoodVisitorImpl &);

  tl::Mutex m_lock;
};

Class<db::PolygonNeighborhoodVisitor> decl_PolygonNeighborhoodVisitor ("db", "PolygonNeighborhoodVisitorBase",
  "@hide"
);

Class<gsi::PolygonNeighborhoodVisitorImpl> decl_PolygonNeighborhoodVisitorImpl (decl_PolygonNeighborhoodVisitor, "db", "PolygonNeighborhoodVisitor",
  gsi::callback ("neighbors", &PolygonNeighborhoodVisitorImpl::issue_neighbors, &PolygonNeighborhoodVisitorImpl::f_neighbors, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("polygon"), gsi::arg ("neighborhood"),
    "@brief Is called for each polygon with the neighbors\n"
    "This method is called for every (merged) polygon on the input region. It delivers the polygon and the neighborhood. "
    "The neighborhood is a collection of polygons (with properties) vs. input index.\n"
    "It contains all polygons 'close to' the current polygon given by 'polygon'. 'Close to' does not necessarily refer to "
    "being exactly in the vicinity, but may include other polygons just entering the bounding box of the current polygon."
  ) +
  gsi::method ("output", &PolygonNeighborhoodVisitorImpl::output_polygon, gsi::arg ("polygon"),
    "@brief Outputs a polygon\n"
    "Use this method from one of the callbacks (\\on_edge, \\begin_polygon, \\end_polygon) to deliver a polygon. "
    "Note that you have to configure the result type as 'Region' on construction of the visitor before being able to do so.\n"
    "\n"
    "'output' expects an object in original space - i.e. of the input edge. \\to_original_trans gives you a suitable "
    "transformation to bring objects from 'edge is horizontal' space into the original space."
  ) +
  gsi::method ("output", &PolygonNeighborhoodVisitorImpl::output_edge, gsi::arg ("edge"),
    "@brief Outputs an edge\n"
    "Use this method from one of the callbacks (\\on_edge, \\begin_polygon, \\end_polygon) to deliver a polygon. "
    "Note that you have to configure the result type as 'Edges' on construction of the visitor before being able to do so."
    "\n"
    "'output' expects an object in original space - i.e. of the input edge. \\to_original_trans gives you a suitable "
    "transformation to bring objects from 'edge is horizontal' space into the original space."
  ) +
  gsi::method ("output", &PolygonNeighborhoodVisitorImpl::output_edge_pair, gsi::arg ("edge_pair"),
    "@brief Outputs an edge pair\n"
    "Use this method from one of the callbacks (\\on_edge, \\begin_polygon, \\end_polygon) to deliver a polygon. "
    "Note that you have to configure the result type as 'EdgePairs' on construction of the visitor before being able to do so."
    "\n"
    "'output' expects an object in original space - i.e. of the input edge. \\to_original_trans gives you a suitable "
    "transformation to bring objects from 'edge is horizontal' space into the original space."
  ) +
  gsi::method ("result_type=", &PolygonNeighborhoodVisitorImpl::set_result_type, gsi::arg ("result_type"),
    "@brief Configures the result type\n"
    "Use this method to indicate what type of result you want to deliver. You can use the corresponding 'output' method then to "
    "deliver result shapes from one the callbacks (\\on_edge, \\begin_polygon, \\end_polygon). Set this attribute when you create "
    "the visitor object. This attribute does not need to be set if no output is indended to be delivered."
  ) +
  gsi::method ("result_type", &PolygonNeighborhoodVisitorImpl::result_type,
    "@brief Gets the result type\n"
  ) +
  gsi::method ("variant_type=", &PolygonNeighborhoodVisitorImpl::set_variant_type, gsi::arg ("variant_type"),
    "@brief Configures the variant type\n"
    "The variant type configures transformation variant formation. The polygons presented to the visitor are "
    "normalized to the given variant type. For example, specify \\VariantType#Orientation to force orientation variants "
    "in the cell tree. Polygons presented to the visitor are normalized to 'as if top' orientation with this variant type.\n"
    "\n"
    "This property was introduced in version 0.30.2."
  ) +
  gsi::method ("variant_type", &PolygonNeighborhoodVisitorImpl::variant_type,
    "@brief Gets the variant type\n"
    "See \\variant_type= for a description of this property.\n"
    "\n"
    "This property was introduced in version 0.30.2."
  ),
  "@brief A visitor for the neighborhood of polygons in the input\n"
  "\n"
  "Objects of this class are passed to \\PolygonNeighborhoodCompoundOperationNode constructor to handle "
  "events on each edge of the primary input along with the neighborhood taken from the additional inputs.\n"
  "\n"
  "See \\neighbors for the description of the events delivered."
  "\n"
  "This class has been introduced in version 0.30.0.\n"
);

// ---------------------------------------------------------------------------------
//  PolygonNeighborhoodCompoundOperationNode binding

static db::CompoundRegionOperationNode *new_polygon_neighborhood (const std::vector<db::CompoundRegionOperationNode *> &children, db::PolygonNeighborhoodVisitor *visitor, const db::Coord dist)
{
  return new db::PolygonNeighborhoodCompoundOperationNode (children, visitor, dist);
}

gsi::ClassExt<db::CompoundRegionOperationNode> decl_CompoundRegionOperationNode_ext_PolygonNeighborhood (
  gsi::constructor ("new_polygon_neighborhood", &new_polygon_neighborhood, gsi::arg ("children"), gsi::arg ("visitor"), gsi::arg ("dist", 0),
    "@brief Creates a new polygon neighborhood collector\n"
    "\n"
    "@param children The inputs to use. The inputs are enumerated by base zero indexes in the visitor callback.\n"
    "@param visitor The visitor object (see \\PolygonNeighborhoodVisitor) receiving the polygon events.\n"
    "@param dist The search distance in which to look up neighbors.\n"
    "\n"
    "This constructor has been introduced in version 0.30.0.\n"
  )
);

}

