
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include "dbShapeProcessor.h"
#include "dbLayout.h"

namespace gsi
{

// -------------------------------------------------------------------
//  ShapeProcessor declarations

static std::vector <db::Edge>
merge1 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, const std::vector<db::CplxTrans> &trans, unsigned int min_wc)
{
  std::vector <db::Edge> out;
  processor->merge (in, trans, out, min_wc);
  return out;
}

static std::vector <db::Polygon>
merge_to_polygon1 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, const std::vector<db::CplxTrans> &trans, unsigned int min_wc, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->merge (in, trans, out, min_wc, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
merge2 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, unsigned int min_wc)
{
  std::vector <db::Edge> out;
  processor->merge (in, out, min_wc);
  return out;
}

static std::vector <db::Polygon>
merge_to_polygon2 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, unsigned int min_wc, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->merge (in, out, min_wc, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
boolean1 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in_a, const std::vector<db::CplxTrans> &trans_a,
                                         const std::vector<db::Shape> &in_b, const std::vector<db::CplxTrans> &trans_b,
                                         int mode)
{
  std::vector <db::Edge> out;
  processor->boolean (in_a, trans_a, in_b, trans_b, mode, out);
  return out;
}

static std::vector <db::Polygon>
boolean_to_polygon1 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in_a, const std::vector<db::CplxTrans> &trans_a,
                                                    const std::vector<db::Shape> &in_b, const std::vector<db::CplxTrans> &trans_b,
                                                    int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->boolean (in_a, trans_a, in_b, trans_b, mode, out, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
boolean2 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in_a, const std::vector<db::Shape> &in_b, 
                                         int mode)
{
  std::vector <db::Edge> out;
  processor->boolean (in_a, in_b, mode, out);
  return out;
}

static std::vector <db::Polygon>
boolean_to_polygon2 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in_a, const std::vector<db::Shape> &in_b, 
                                                    int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->boolean (in_a, in_b, mode, out, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
size1 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, const std::vector<db::CplxTrans> &trans,
       db::Coord d, unsigned int mode)
{
  std::vector <db::Edge> out;
  processor->size (in, trans, d, out, mode);
  return out;
}

static std::vector <db::Polygon>
size_to_polygon1 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, const std::vector<db::CplxTrans> &trans,
                  db::Coord d, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->size (in, trans, d, out, mode, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
size2 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, const std::vector<db::CplxTrans> &trans,
       db::Coord dx, db::Coord dy, unsigned int mode)
{
  std::vector <db::Edge> out;
  processor->size (in, trans, dx, dy, out, mode);
  return out;
}

static std::vector <db::Polygon>
size_to_polygon2 (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, const std::vector<db::CplxTrans> &trans,
                  db::Coord dx, db::Coord dy, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->size (in, trans, dx, dy, out, mode, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
size1n (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, 
        db::Coord d, unsigned int mode)
{
  std::vector <db::Edge> out;
  processor->size (in, d, out, mode);
  return out;
}

static std::vector <db::Polygon>
size_to_polygon1n (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, 
                   db::Coord d, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->size (in, d, out, mode, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
size2n (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, 
        db::Coord dx, db::Coord dy, unsigned int mode)
{
  std::vector <db::Edge> out;
  processor->size (in, dx, dy, out, mode);
  return out;
}

static std::vector <db::Polygon>
size_to_polygon2n (db::ShapeProcessor *processor, const std::vector<db::Shape> &in, 
                   db::Coord dx, db::Coord dy, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->size (in, dx, dy, out, mode, resolve_holes, min_coherence);
  return out;
}


Class<db::ShapeProcessor> decl_ShapeProcessor ("db", "ShapeProcessor",
  method ("merge", (void (db::ShapeProcessor::*) (const db::Layout &, const db::Cell &, unsigned int, db::Shapes &, bool, unsigned int, bool, bool)) &db::ShapeProcessor::merge, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("out"), gsi::arg ("hierarchical"), gsi::arg ("min_wc"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Merge the given shapes from a layout into a shapes container\n"
    "\n"
    "See the \\EdgeProcessor for a description of the merge method. This implementation takes shapes\n"
    "from a layout cell (optionally all in hierarchy) and produces new shapes in a shapes container. "
    "\n"
    "@param layout The layout from which to take the shapes\n"
    "@param cell The cell (in 'layout') from which to take the shapes\n"
    "@param layer The cell (in 'layout') from which to take the shapes\n"
    "@param out The shapes container where to put the shapes into (is cleared before)\n"
    "@param hierarchical Collect shapes from sub cells as well\n"
    "@param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method ("boolean", (void (db::ShapeProcessor::*) (const db::Layout &, const db::Cell &, unsigned int, const db::Layout &, const db::Cell &, unsigned int, db::Shapes &, int, bool, bool, bool)) &db::ShapeProcessor::boolean, gsi::arg ("layout_a"), gsi::arg ("cell_a"), gsi::arg ("layer_a"), gsi::arg ("layout_b"), gsi::arg ("cell_b"), gsi::arg ("layer_b"), gsi::arg ("out"), gsi::arg ("mode"), gsi::arg ("hierarchical"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Boolean operation on shapes from layouts\n"
    "\n"
    "See the \\EdgeProcessor for a description of the boolean operations. This implementation takes shapes\n"
    "from layout cells (optionally all in hierarchy) and produces new shapes in a shapes container. "
    "\n"
    "@param layout_a The layout from which to take the shapes for input A\n"
    "@param cell_a The cell (in 'layout') from which to take the shapes for input A\n"
    "@param layer_a The cell (in 'layout') from which to take the shapes for input A\n"
    "@param layout_b The layout from which to take the shapes for input B\n"
    "@param cell_b The cell (in 'layout') from which to take the shapes for input B\n"
    "@param layer_b The cell (in 'layout') from which to take the shapes for input B\n"
    "@param out The shapes container where to put the shapes into (is cleared before)\n"
    "@param mode The boolean operation (see \\EdgeProcessor)\n"
    "@param hierarchical Collect shapes from sub cells as well\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method ("size", (void (db::ShapeProcessor::*) (const db::Layout &, const db::Cell &, unsigned int, db::Shapes &, db::Coord, db::Coord, unsigned int, bool, bool, bool)) &db::ShapeProcessor::size, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("out"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"), gsi::arg ("hierarchical"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Sizing operation on shapes from layouts\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing operation. This implementation takes shapes\n"
    "from a layout cell (optionally all in hierarchy) and produces new shapes in a shapes container. "
    "\n"
    "@param layout The layout from which to take the shapes\n"
    "@param cell The cell (in 'layout') from which to take the shapes\n"
    "@param layer The cell (in 'layout') from which to take the shapes\n"
    "@param out The shapes container where to put the shapes into (is cleared before)\n"
    "@param dx The sizing value in x-direction (see \\EdgeProcessor)\n"
    "@param dy The sizing value in y-direction (see \\EdgeProcessor)\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
    "@param hierarchical Collect shapes from sub cells as well\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method ("size", (void (db::ShapeProcessor::*) (const db::Layout &, const db::Cell &, unsigned int, db::Shapes &, db::Coord, unsigned int, bool, bool, bool)) &db::ShapeProcessor::size, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("out"), gsi::arg ("d"), gsi::arg ("mode"), gsi::arg ("hierarchical"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Sizing operation on shapes from layouts\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing operation. This implementation takes shapes\n"
    "from a layout cell (optionally all in hierarchy) and produces new shapes in a shapes container. "
    "This is the isotropic version which does not allow specification of different sizing values in x and y-direction. "
    "\n"
    "@param layout The layout from which to take the shapes\n"
    "@param cell The cell (in 'layout') from which to take the shapes\n"
    "@param layer The cell (in 'layout') from which to take the shapes\n"
    "@param out The shapes container where to put the shapes into (is cleared before)\n"
    "@param d The sizing value (see \\EdgeProcessor)\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
    "@param hierarchical Collect shapes from sub cells as well\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("merge", &gsi::merge1, gsi::arg ("in"), gsi::arg ("trans"), gsi::arg ("min_wc"),
    "@brief Merge the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the merge method. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set.\n"
    "\n"
    "@param in The set of shapes to merge\n"
    "@param trans A corresponding set of transformations to apply on the shapes\n"
    "@param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)\n"
  ) +
  method_ext ("merge_to_polygon", &gsi::merge_to_polygon1, gsi::arg ("in"), gsi::arg ("trans"), gsi::arg ("min_wc"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Merge the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the merge method. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set.\n"
    "\n"
    "@param in The set of shapes to merge\n"
    "@param trans A corresponding set of transformations to apply on the shapes\n"
    "@param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("merge", &gsi::merge2, gsi::arg ("in"), gsi::arg ("min_wc"),
    "@brief Merge the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the merge method. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set.\n"
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in The set of shapes to merge\n"
    "@param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)\n"
  ) +
  method_ext ("merge_to_polygon", &gsi::merge_to_polygon2, gsi::arg ("in"), gsi::arg ("min_wc"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Merge the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the merge method. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set.\n"
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in The set of shapes to merge\n"
    "@param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("boolean", &gsi::boolean1, gsi::arg ("in_a"), gsi::arg ("trans_a"), gsi::arg ("in_b"), gsi::arg ("trans_b"), gsi::arg ("mode"),
    "@brief Boolean operation on two given shape sets into an edge set\n"
    "\n"
    "See the \\EdgeProcessor for a description of the boolean operations. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set.\n"
    "\n"
    "@param in_a The set of shapes to use for input A\n"
    "@param trans_a A set of transformations to apply before the shapes are used\n"
    "@param in_b The set of shapes to use for input A\n"
    "@param trans_b A set of transformations to apply before the shapes are used\n"
    "@param mode The boolean operation (see \\EdgeProcessor)\n"
  ) +
  method_ext ("boolean_to_polygon", &gsi::boolean_to_polygon1, gsi::arg ("in_a"), gsi::arg ("trans_a"), gsi::arg ("in_b"), gsi::arg ("trans_b"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Boolean operation on two given shape sets into a polygon set\n"
    "\n"
    "See the \\EdgeProcessor for a description of the boolean operations. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set.\n"
    "\n"
    "@param in_a The set of shapes to use for input A\n"
    "@param trans_a A set of transformations to apply before the shapes are used\n"
    "@param in_b The set of shapes to use for input A\n"
    "@param trans_b A set of transformations to apply before the shapes are used\n"
    "@param mode The boolean operation (see \\EdgeProcessor)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("boolean", &gsi::boolean2, gsi::arg ("in_a"), gsi::arg ("in_b"), gsi::arg ("mode"),
    "@brief Boolean operation on two given shape sets into an edge set\n"
    "\n"
    "See the \\EdgeProcessor for a description of the boolean operations. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set.\n"
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in_a The set of shapes to use for input A\n"
    "@param in_b The set of shapes to use for input A\n"
    "@param mode The boolean operation (see \\EdgeProcessor)\n"
  ) +
  method_ext ("boolean_to_polygon", &gsi::boolean_to_polygon2, gsi::arg ("in_a"), gsi::arg ("in_b"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Boolean operation on two given shape sets into a polygon set\n"
    "\n"
    "See the \\EdgeProcessor for a description of the boolean operations. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set.\n"
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in_a The set of shapes to use for input A\n"
    "@param in_b The set of shapes to use for input A\n"
    "@param mode The boolean operation (see \\EdgeProcessor)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("size", &gsi::size1, gsi::arg ("in"), gsi::arg ("trans"), gsi::arg ("d"), gsi::arg ("mode"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set. This is isotropic version that does not allow\n"
    "to specify different values in x and y direction. "
    "\n"
    "@param in The set of shapes to size\n"
    "@param trans A corresponding set of transformations to apply on the shapes\n"
    "@param d The sizing value\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
  ) +
  method_ext ("size", &gsi::size2, gsi::arg ("in"), gsi::arg ("trans"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set.\n"
    "\n"
    "@param in The set of shapes to size\n"
    "@param trans A corresponding set of transformations to apply on the shapes\n"
    "@param dx The sizing value in x-direction\n"
    "@param dy The sizing value in y-direction\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
  ) +
  method_ext ("size_to_polygon", &gsi::size_to_polygon1, gsi::arg ("in"), gsi::arg ("trans"), gsi::arg ("d"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set. This is isotropic version that does not allow\n"
    "to specify different values in x and y direction. "
    "\n"
    "@param in The set of shapes to size\n"
    "@param trans A corresponding set of transformations to apply on the shapes\n"
    "@param d The sizing value\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("size_to_polygon", &gsi::size_to_polygon2, gsi::arg ("in"), gsi::arg ("trans"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set.\n"
    "\n"
    "@param in The set of shapes to size\n"
    "@param trans A corresponding set of transformations to apply on the shapes\n"
    "@param dx The sizing value in x-direction\n"
    "@param dy The sizing value in y-direction\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("size", &gsi::size1n, gsi::arg ("in"), gsi::arg ("d"), gsi::arg ("mode"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set. This is isotropic version that does not allow\n"
    "to specify different values in x and y direction. "
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in The set of shapes to size\n"
    "@param d The sizing value\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
  ) +
  method_ext ("size", &gsi::size2n, gsi::arg ("in"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces an edge set.\n"
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in The set of shapes to size\n"
    "@param dx The sizing value in x-direction\n"
    "@param dy The sizing value in y-direction\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
  ) +
  method_ext ("size_to_polygon", &gsi::size_to_polygon1n, gsi::arg ("in"), gsi::arg ("d"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set. This is isotropic version that does not allow\n"
    "to specify different values in x and y direction. "
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in The set of shapes to size\n"
    "@param d The sizing value\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ) +
  method_ext ("size_to_polygon", &gsi::size_to_polygon2n, gsi::arg ("in"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Size the given shapes\n"
    "\n"
    "See the \\EdgeProcessor for a description of the sizing method. This implementation takes shapes\n"
    "rather than polygons for input and produces a polygon set.\n"
    "\n"
    "This version does not feature a transformation for each shape (unity is assumed).\n"
    "\n"
    "@param in The set of shapes to size\n"
    "@param dx The sizing value in x-direction\n"
    "@param dy The sizing value in y-direction\n"
    "@param mode The sizing mode (see \\EdgeProcessor)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if minimum polygons should be created for touching corners\n"
  ),
  "@brief The shape processor (boolean, sizing, merge on shapes)\n"
  "\n"
  "The shape processor implements the boolean and edge set operations (size, merge). Because the shape processor "
  "might allocate resources which can be reused in later operations, it is implemented as an object that can be used several times. "
  "The shape processor is similar to the \\EdgeProcessor. The latter is specialized on handling polygons and edges directly. "
);

}
