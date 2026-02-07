
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

#ifndef HDR_dbPLCConvexDecomposition
#define HDR_dbPLCConvexDecomposition

#include "dbCommon.h"
#include "dbPLC.h"
#include "dbPLCTriangulation.h"

#include <limits>
#include <list>
#include <vector>
#include <algorithm>

namespace db
{

namespace plc
{

struct DB_PUBLIC ConvexDecompositionParameters
{
  ConvexDecompositionParameters ()
    : with_segments (false),
      split_edges (false),
      base_verbosity (30)
  {
    tri_param.max_area = 0.0;
    tri_param.min_b = 0.0;

    //  Needed for the algorithm - don't change this
    tri_param.remove_outside_triangles = false;
  }

  /**
   *  @brief The parameters used for the triangulation
   */
  TriangulationParameters tri_param;

  /**
   *  @brief Introduce new segments
   *
   *  If true, new segments will be introduced.
   *  New segments are constructed perpendicular to the edges forming
   *  a concave corner.
   */
  bool with_segments;

  /**
   *  @brief Split edges
   *
   *  If true, edges in the resulting polygons may be split.
   *  This will produce edge sections that correlate with
   *  other polygon edges, but may be collinear with neighbor
   *  edges.
   */
  double split_edges;

  /**
   *  @brief The verbosity level above which triangulation reports details
   */
  int base_verbosity;
};

/**
 *  @brief A convex decomposition algorithm
 *
 *  This class implements a variant of the Hertel-Mehlhorn decomposition.
 */
class DB_PUBLIC ConvexDecomposition
{
public:
  /**
   *  @brief The constructor
   *
   *  The graph will be one filled by the decomposition.
   */
  ConvexDecomposition (Graph *graph);

  /**
   *  @brief Clears the triangulation
   */
  void clear ();

  /**
   *  @brief Creates a decomposition for the given region
   *
   *  The database unit should be chosen in a way that target area values are "in the order of 1".
   *  For inputs featuring acute angles (angles < ~25 degree), the parameters should defined a min
   *  edge length ("min_length").
   *  "min_length" should be at least 1e-4. If a min edge length is given, the max area constaints
   *  may not be satisfied.
   *
   *  Edges in the input should not be shorter than 1e-4.
   */
  void decompose (const db::Region &region, const ConvexDecompositionParameters &parameters, double dbu = 1.0);

  //  more versions
  void decompose (const db::Region &region, const ConvexDecompositionParameters &parameters, const db::CplxTrans &trans = db::CplxTrans ());
  void decompose (const db::Polygon &poly, const ConvexDecompositionParameters &parameters, double dbu = 1.0);
  void decompose (const db::Polygon &poly, const ConvexDecompositionParameters &parameters, const db::CplxTrans &trans = db::CplxTrans ());

  /**
   *  @brief Decomposes a floating-point polygon
   */
  void decompose (const db::DPolygon &poly, const ConvexDecompositionParameters &parameters, const db::DCplxTrans &trans = db::DCplxTrans ());

private:
  Graph *mp_graph;

  struct ConcaveCorner
  {
    ConcaveCorner ()
      : corner (0), incoming (0), outgoing (0)
    {
      //  .. nothing yet ..
    }

    ConcaveCorner (const Vertex *_corner, const Edge *_incoming, const Edge *_outgoing)
      : corner (_corner), incoming (_incoming), outgoing (_outgoing)
    {
      //  .. nothing yet ..
    }

    const Vertex *corner;
    const Edge *incoming, *outgoing;
  };

  void hertel_mehlhorn_decomposition (Triangulation &tris, const ConvexDecompositionParameters &param);
  void collect_concave_vertexes (std::vector<ConcaveCorner> &concave_vertexes);
  std::pair<bool, db::DPoint> search_crossing_with_next_segment (const Vertex *v0, const db::DVector &direction);
};

} //  namespace plc

} //  namespace db

#endif

