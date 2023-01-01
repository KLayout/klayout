
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

#include "dbEdgeProcessor.h"
#include "dbLayout.h"

namespace gsi
{

// -------------------------------------------------------------------
//  EdgeProcessor declarations

static std::vector <db::Edge>
simple_merge1 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in)
{
  std::vector <db::Edge> out;
  processor->simple_merge (in, out);
  return out;
}

static std::vector <db::Polygon>
simple_merge_to_polygon1 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->simple_merge (in, out, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
simple_merge2 (db::EdgeProcessor *processor, const std::vector<db::Edge> &in)
{
  std::vector <db::Edge> out;
  processor->simple_merge (in, out);
  return out;
}

static std::vector <db::Polygon>
simple_merge_to_polygon2 (db::EdgeProcessor *processor, const std::vector<db::Edge> &in, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->simple_merge (in, out, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge>
simple_merge1m (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, int mode)
{
  std::vector <db::Edge> out;
  processor->simple_merge (in, out, mode);
  return out;
}

static std::vector <db::Polygon>
simple_merge_to_polygon1m (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, bool resolve_holes, bool min_coherence, int mode)
{
  std::vector <db::Polygon> out;
  processor->simple_merge (in, out, resolve_holes, min_coherence, mode);
  return out;
}

static std::vector <db::Edge>
simple_merge2m (db::EdgeProcessor *processor, const std::vector<db::Edge> &in, int mode)
{
  std::vector <db::Edge> out;
  processor->simple_merge (in, out, mode);
  return out;
}

static std::vector <db::Polygon>
simple_merge_to_polygon2m (db::EdgeProcessor *processor, const std::vector<db::Edge> &in, bool resolve_holes, bool min_coherence, int mode)
{
  std::vector <db::Polygon> out;
  processor->simple_merge (in, out, resolve_holes, min_coherence, mode);
  return out;
}

static std::vector <db::Polygon> 
boolean_to_polygon1 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &a, const std::vector<db::Polygon> &b, int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->boolean (a, b, out, mode, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge> 
boolean1 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &a, const std::vector<db::Polygon> &b, int mode)
{
  std::vector <db::Edge> out;
  processor->boolean (a, b, out, mode);
  return out;
}

static std::vector <db::Polygon> 
boolean_to_polygon2 (db::EdgeProcessor *processor, const std::vector<db::Edge> &a, const std::vector<db::Edge> &b, int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->boolean (a, b, out, mode, resolve_holes, min_coherence);
  return out;
}

static std::vector <db::Edge> 
boolean2 (db::EdgeProcessor *processor, const std::vector<db::Edge> &a, const std::vector<db::Edge> &b, int mode)
{
  std::vector <db::Edge> out;
  processor->boolean (a, b, out, mode);
  return out;
}

std::vector <db::Edge> 
merge (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, unsigned int min_wc)
{
  std::vector <db::Edge> out;
  processor->merge (in, out, min_wc);
  return out;
}

std::vector <db::Polygon> 
merge_to_polygon (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, unsigned int min_wc, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->merge (in, out, min_wc, resolve_holes, min_coherence);
  return out;
}

std::vector <db::Edge> 
size1 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, db::Coord dx, db::Coord dy, unsigned int mode)
{
  std::vector <db::Edge> out;
  processor->size (in, dx, dy, out, mode);
  return out;
}

std::vector <db::Polygon> 
size_to_polygon1 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, db::Coord dx, db::Coord dy, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->size (in, dx, dy, out, mode, resolve_holes, min_coherence);
  return out;
}

std::vector <db::Edge> 
size2 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, db::Coord d, unsigned int mode)
{
  std::vector <db::Edge> out;
  processor->size (in, d, out, mode);
  return out;
}

std::vector <db::Polygon> 
size_to_polygon2 (db::EdgeProcessor *processor, const std::vector<db::Polygon> &in, db::Coord d, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  std::vector <db::Polygon> out;
  processor->size (in, d, out, mode, resolve_holes, min_coherence);
  return out;
}

static int mode_and () { return int (db::BooleanOp::And); }
static int mode_or () { return int (db::BooleanOp::Or); }
static int mode_xor () { return int (db::BooleanOp::Xor); }
static int mode_anotb () { return int (db::BooleanOp::ANotB); }
static int mode_bnota () { return int (db::BooleanOp::BNotA); }

Class<db::EdgeProcessor> decl_EdgeProcessor ("db", "EdgeProcessor",
  method_ext ("simple_merge_p2e|#simple_merge", &gsi::simple_merge1, gsi::arg ("in"),
    "@brief Merge the given polygons in a simple \"non-zero wrapcount\" fashion\n"
    "\n"
    "The wrapcount is computed over all polygons, i.e. overlapping polygons may \"cancel\" if they\n"
    "have different orientation (since a polygon is oriented by construction that is not easy to achieve).\n"
    "The other merge operation provided for this purpose is \"merge\" which normalizes each polygon individually before\n"
    "merging them. \"simple_merge\" is somewhat faster and consumes less memory.\n"
    "\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "Prior to version 0.21 this method was called 'simple_merge'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@return The output edges\n"
  ) +
  method_ext ("simple_merge_p2e|#simple_merge", &gsi::simple_merge1m, gsi::arg ("in"), gsi::arg ("mode"),
    "@brief Merge the given polygons and specify the merge mode\n"
    "\n"
    "The wrapcount is computed over all polygons, i.e. overlapping polygons may \"cancel\" if they\n"
    "have different orientation (since a polygon is oriented by construction that is not easy to achieve).\n"
    "The other merge operation provided for this purpose is \"merge\" which normalizes each polygon individually before\n"
    "merging them. \"simple_merge\" is somewhat faster and consumes less memory.\n"
    "\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "This method has been added in version 0.22.\n"
    "\n"
    "The mode specifies the rule to use when producing output. A value of 0 specifies the even-odd rule. "
    "A positive value specifies the wrap count threshold (positive only). A negative value specifies the "
    "threshold of the absolute value of the wrap count (i.e. -1 is non-zero rule).\n"
    "\n"
    "@param mode See description\n"
    "@param in The input polygons\n"
    "@return The output edges\n"
  ) +
  method_ext ("simple_merge_p2p|#simple_merge_to_polygon", &gsi::simple_merge_to_polygon1, gsi::arg ("in"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Merge the given polygons in a simple \"non-zero wrapcount\" fashion into polygons\n"
    "\n"
    "The wrapcount is computed over all polygons, i.e. overlapping polygons may \"cancel\" if they\n"
    "have different orientation (since a polygon is oriented by construction that is not easy to achieve).\n"
    "The other merge operation provided for this purpose is \"merge\" which normalizes each polygon individually before\n"
    "merging them. \"simple_merge\" is somewhat faster and consumes less memory.\n"
    "\n"
    "This method produces polygons and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "Prior to version 0.21 this method was called 'simple_merge_to_polygon'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("simple_merge_p2p|#simple_merge_to_polygon", &gsi::simple_merge_to_polygon1m, gsi::arg ("in"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"), gsi::arg ("mode"),
    "@brief Merge the given polygons and specify the merge mode\n"
    "\n"
    "The wrapcount is computed over all polygons, i.e. overlapping polygons may \"cancel\" if they\n"
    "have different orientation (since a polygon is oriented by construction that is not easy to achieve).\n"
    "The other merge operation provided for this purpose is \"merge\" which normalizes each polygon individually before\n"
    "merging them. \"simple_merge\" is somewhat faster and consumes less memory.\n"
    "\n"
    "This method produces polygons and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "This method has been added in version 0.22.\n"
    "\n"
    "The mode specifies the rule to use when producing output. A value of 0 specifies the even-odd rule. "
    "A positive value specifies the wrap count threshold (positive only). A negative value specifies the "
    "threshold of the absolute value of the wrap count (i.e. -1 is non-zero rule).\n"
    "\n"
    "@param mode See description\n"
    "@param in The input polygons\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("simple_merge_e2e|#simple_merge", &gsi::simple_merge2, gsi::arg ("in"),
    "@brief Merge the given edges in a simple \"non-zero wrapcount\" fashion\n"
    "\n"
    "The edges provided must form valid closed contours. Contours oriented differently \"cancel\" each other. \n"
    "Overlapping contours are merged when the orientation is the same.\n"
    "\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "Prior to version 0.21 this method was called 'simple_merge'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input edges\n"
    "@return The output edges\n"
  ) +
  method_ext ("simple_merge_e2e|#simple_merge", &gsi::simple_merge2m, gsi::arg ("in"), gsi::arg ("mode"),
    "@brief Merge the given polygons and specify the merge mode\n"
    "\n"
    "The edges provided must form valid closed contours. Contours oriented differently \"cancel\" each other. \n"
    "Overlapping contours are merged when the orientation is the same.\n"
    "\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "This method has been added in version 0.22.\n"
    "\n"
    "The mode specifies the rule to use when producing output. A value of 0 specifies the even-odd rule. "
    "A positive value specifies the wrap count threshold (positive only). A negative value specifies the "
    "threshold of the absolute value of the wrap count (i.e. -1 is non-zero rule).\n"
    "\n"
    "@param mode See description\n"
    "@param in The input edges\n"
    "@return The output edges\n"
  ) +
  method_ext ("simple_merge_e2p|#simple_merge_to_polygon", &gsi::simple_merge_to_polygon2, gsi::arg ("in"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Merge the given edges in a simple \"non-zero wrapcount\" fashion into polygons\n"
    "\n"
    "The edges provided must form valid closed contours. Contours oriented differently \"cancel\" each other. \n"
    "Overlapping contours are merged when the orientation is the same.\n"
    "\n"
    "This method produces polygons and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "Prior to version 0.21 this method was called 'simple_merge_to_polygon'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input edges\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("simple_merge_e2p|#simple_merge_to_polygon", &gsi::simple_merge_to_polygon2m, gsi::arg ("in"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"), gsi::arg ("mode"),
    "@brief Merge the given polygons and specify the merge mode\n"
    "\n"
    "The edges provided must form valid closed contours. Contours oriented differently \"cancel\" each other. \n"
    "Overlapping contours are merged when the orientation is the same.\n"
    "\n"
    "This method produces polygons and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a SimpleMerge operator and puts the result into an output vector.\n"
    "\n"
    "This method has been added in version 0.22.\n"
    "\n"
    "The mode specifies the rule to use when producing output. A value of 0 specifies the even-odd rule. "
    "A positive value specifies the wrap count threshold (positive only). A negative value specifies the "
    "threshold of the absolute value of the wrap count (i.e. -1 is non-zero rule).\n"
    "\n"
    "@param mode See description\n"
    "@param in The input edges\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("merge_p2e|#merge", &gsi::merge, gsi::arg ("in"), gsi::arg ("min_wc"),
    "@brief Merge the given polygons \n"
    "\n"
    "In contrast to \"simple_merge\", this merge implementation considers each polygon individually before merging them.\n"
    "Thus self-overlaps are effectively removed before the output is computed and holes are correctly merged with the\n"
    "hull. In addition, this method allows selecting areas with a higher wrap count which in turn allows computing overlaps\n"
    "of polygons on the same layer. Because this method merges the polygons before the overlap is computed, self-overlapping\n"
    "polygons do not contribute to higher wrap count areas.\n"
    "\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "Prior to version 0.21 this method was called 'merge'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)\n"
    "@return The output edges\n"
  ) +
  method_ext ("merge_p2p|#merge_to_polygon", &gsi::merge_to_polygon, gsi::arg ("in"), gsi::arg ("min_wc"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Merge the given polygons \n"
    "\n"
    "In contrast to \"simple_merge\", this merge implementation considers each polygon individually before merging them.\n"
    "Thus self-overlaps are effectively removed before the output is computed and holes are correctly merged with the\n"
    "hull. In addition, this method allows selecting areas with a higher wrap count which in turn allows computing overlaps\n"
    "of polygons on the same layer. Because this method merges the polygons before the overlap is computed, self-overlapping\n"
    "polygons do not contribute to higher wrap count areas.\n"
    "\n"
    "This method produces polygons and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "Prior to version 0.21 this method was called 'merge_to_polygon'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("size_p2e|#size", &gsi::size1, gsi::arg ("in"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"),
    "@brief Size the given polygons \n"
    "\n"
    "This method sizes a set of polygons. Before the sizing is applied, the polygons are merged. After that, sizing is applied \n"
    "on the individual result polygons of the merge step. The result may contain overlapping contours, but no self-overlaps. \n"
    "\n"
    "dx and dy describe the sizing. A positive value indicates oversize (outwards) while a negative one describes undersize (inwards).\n"
    "The sizing applied can be chosen differently in x and y direction. In this case, the sign must be identical for both\n"
    "dx and dy.\n"
    "\n"
    "The 'mode' parameter describes the corner fill strategy. Mode 0 connects all corner segments directly. Mode 1 is the 'octagon' strategy in which "
    "square corners are interpolated with a partial octagon. Mode 2 is the standard mode in which corners are filled by expanding edges unless these "
    "edges form a sharp bend with an angle of more than 90 degree. In that case, the corners are cut off. In Mode 3, no cutoff occurs up to a bending angle of 135 degree. "
    "Mode 4 and 5 are even more aggressive and allow very sharp bends without cutoff. This strategy may produce long spikes on sharply bending corners. "
    "\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "Prior to version 0.21 this method was called 'size'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@param dx The sizing value in x direction\n"
    "@param dy The sizing value in y direction\n"
    "@param mode The sizing mode (standard is 2)\n"
    "@return The output edges\n"
  ) +
  method_ext ("size_p2p|#size_to_polygon", &gsi::size_to_polygon1, gsi::arg ("in"), gsi::arg ("dx"), gsi::arg ("dy"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Size the given polygons into polygons\n"
    "\n"
    "This method sizes a set of polygons. Before the sizing is applied, the polygons are merged. After that, sizing is applied \n"
    "on the individual result polygons of the merge step. The result may contain overlapping polygons, but no self-overlapping ones. \n"
    "Polygon overlap occurs if the polygons are close enough, so a positive sizing makes polygons overlap.\n"
    "\n"
    "dx and dy describe the sizing. A positive value indicates oversize (outwards) while a negative one describes undersize (inwards).\n"
    "The sizing applied can be chosen differently in x and y direction. In this case, the sign must be identical for both\n"
    "dx and dy.\n"
    "\n"
    "The 'mode' parameter describes the corner fill strategy. Mode 0 connects all corner segments directly. Mode 1 is the 'octagon' strategy in which "
    "square corners are interpolated with a partial octagon. Mode 2 is the standard mode in which corners are filled by expanding edges unless these "
    "edges form a sharp bend with an angle of more than 90 degree. In that case, the corners are cut off. In Mode 3, no cutoff occurs up to a bending angle of 135 degree. "
    "Mode 4 and 5 are even more aggressive and allow very sharp bends without cutoff. This strategy may produce long spikes on sharply bending corners. "
    "\n"
    "This method produces polygons and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "Prior to version 0.21 this method was called 'size_to_polygon'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@param dx The sizing value in x direction\n"
    "@param dy The sizing value in y direction\n"
    "@param mode The sizing mode (standard is 2)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("size_p2e|#size", &gsi::size2, gsi::arg ("in"), gsi::arg ("d"), gsi::arg ("mode"),
    "@brief Size the given polygons (isotropic)\n"
    "\n"
    "This method is equivalent to calling the anisotropic version with identical dx and dy.\n"
    "\n"
    "Prior to version 0.21 this method was called 'size'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@param d The sizing value in x direction\n"
    "@param mode The sizing mode\n"
    "@return The output edges\n"
  ) +
  method_ext ("size_p2p|#size_to_polygon", &gsi::size_to_polygon2, gsi::arg ("in"), gsi::arg ("d"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Size the given polygons into polygons (isotropic)\n"
    "\n"
    "This method is equivalent to calling the anisotropic version with identical dx and dy.\n"
    "\n"
    "Prior to version 0.21 this method was called 'size_to_polygon'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param in The input polygons\n"
    "@param d The sizing value in x direction\n"
    "@param mode The sizing mode\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("boolean_p2e|#boolean", &gsi::boolean1, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("mode"),
    "@brief Boolean operation for a set of given polygons, creating edges\n"
    "\n"
    "This method computes the result for the given boolean operation on two sets of polygons.\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a Boolean operator and puts the result into an output vector.\n"
    "\n"
    "Prior to version 0.21 this method was called 'boolean'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param a The input polygons (first operand)\n"
    "@param b The input polygons (second operand)\n"
    "@param mode The boolean mode\n"
    "@return The output edges\n"
  ) +
  method_ext ("boolean_p2p|#boolean_to_polygon", &gsi::boolean_to_polygon1, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Boolean operation for a set of given polygons, creating polygons\n"
    "\n"
    "This method computes the result for the given boolean operation on two sets of polygons.\n"
    "This method produces polygons on output and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "This is a convenience method that bundles filling of the edges, processing with\n"
    "a Boolean operator and puts the result into an output vector.\n"
    "\n"
    "Prior to version 0.21 this method was called 'boolean_to_polygon'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param a The input polygons (first operand)\n"
    "@param b The input polygons (second operand)\n"
    "@param mode The boolean mode (one of the Mode.. values)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method_ext ("boolean_e2e|#boolean", &gsi::boolean2, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("mode"),
    "@brief Boolean operation for a set of given edges, creating edges\n"
    "\n"
    "This method computes the result for the given boolean operation on two sets of edges.\n"
    "The input edges must form closed contours where holes and hulls must be oriented differently. \n"
    "The input edges are processed with a simple non-zero wrap count rule as a whole.\n"
    "\n"
    "The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while\n"
    "holes are oriented counter-clockwise.\n"
    "\n"
    "Prior to version 0.21 this method was called 'boolean'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param a The input edges (first operand)\n"
    "@param b The input edges (second operand)\n"
    "@param mode The boolean mode (one of the Mode.. values)\n"
    "@return The output edges\n"
  ) +
  method_ext ("boolean_e2p|#boolean_to_polygon", &gsi::boolean_to_polygon2, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("mode"), gsi::arg ("resolve_holes"), gsi::arg ("min_coherence"),
    "@brief Boolean operation for a set of given edges, creating polygons\n"
    "\n"
    "This method computes the result for the given boolean operation on two sets of edges.\n"
    "The input edges must form closed contours where holes and hulls must be oriented differently. \n"
    "The input edges are processed with a simple non-zero wrap count rule as a whole.\n"
    "\n"
    "This method produces polygons on output and allows fine-tuning of the parameters for that purpose.\n"
    "\n"
    "Prior to version 0.21 this method was called 'boolean_to_polygon'. Is was renamed to avoid ambiguities "
    "for empty input arrays. The old version is still available but deprecated.\n"
    "\n"
    "@param a The input polygons (first operand)\n"
    "@param b The input polygons (second operand)\n"
    "@param mode The boolean mode (one of the Mode.. values)\n"
    "@param resolve_holes true, if holes should be resolved into the hull\n"
    "@param min_coherence true, if touching corners should be resolved into less connected contours\n"
    "@return The output polygons\n"
  ) +
  method ("enable_progress", &db::EdgeProcessor::enable_progress, gsi::arg ("label"),
    "@brief Enable progress reporting\n"
    "After calling this method, the edge processor will report the progress through a progress bar.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  method ("disable_progress", &db::EdgeProcessor::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will stop the edge processor from showing a progress bar. See \\enable_progress.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  method ("ModeAnd|#mode_and", &gsi::mode_and, "@brief boolean method's mode value for AND operation") +
  method ("ModeOr|#mode_or", &gsi::mode_or, "@brief boolean method's mode value for OR operation") +
  method ("ModeXor|#mode_xor", &gsi::mode_xor, "@brief boolean method's mode value for XOR operation") +
  method ("ModeANotB|#mode_anotb", &gsi::mode_anotb, "@brief boolean method's mode value for A NOT B operation") +
  method ("ModeBNotA|#mode_bnota", &gsi::mode_bnota, "@brief boolean method's mode value for B NOT A operation"),
  "@brief The edge processor (boolean, sizing, merge)\n"
  "\n"
  "The edge processor implements the boolean and edge set operations (size, merge). Because the edge processor "
  "might allocate resources which can be reused in later operations, it is implemented as an object that can be used several times.\n"
  "\n"
  "Here is a simple example of how to use the edge processor:\n"
  "\n"
  "@code\n"
  "ep = RBA::EdgeProcessor::new\n"
  "# Prepare two boxes\n"
  "a = [ RBA::Polygon::new(RBA::Box::new(0, 0, 300, 300)) ]\n"
  "b = [ RBA::Polygon::new(RBA::Box::new(100, 100, 200, 200)) ]\n"
  "# Run an XOR -> creates a polygon with a hole, since the 'resolve_holes' parameter\n"
  "# is false:\n"
  "out = ep.boolean_p2p(a, b, RBA::EdgeProcessor::ModeXor, false, false)\n"
  "out.to_s    # -> [(0,0;0,300;300,300;300,0/100,100;200,100;200,200;100,200)]\n"
  "@/code\n"
);

} // namespace gsi
