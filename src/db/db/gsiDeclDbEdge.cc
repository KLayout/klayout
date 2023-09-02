
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
#include "dbPoint.h"
#include "dbEdge.h"
#include "dbHash.h"

namespace gsi
{

// ---------------------------------------------------------------
//  edge binding

template <class C>
struct edge_defs
{
  typedef typename C::coord_type coord_type;
  typedef typename C::box_type box_type;
  typedef typename C::point_type point_type;
  typedef typename C::vector_type vector_type;
  typedef typename C::distance_type distance_type;
  typedef typename C::area_type area_type;
  typedef db::simple_trans<coord_type> simple_trans_type;
  typedef db::complex_trans<coord_type, double> complex_trans_type;

  static C *from_string (const char *s)
  {
    tl::Extractor ex (s);
    std::unique_ptr<C> c (new C ());
    ex.read (*c.get ());
    return c.release ();
  }

  static C *new_v ()
  {
    return new C ();
  }

  static C *new_pp (const point_type &p1, const point_type &p2)
  {
    return new C (p1, p2);
  }

  static C *new_xyxy (coord_type x1, coord_type y1, coord_type x2, coord_type y2)
  {
    return new C (point_type (x1, y1), point_type (x2, y2));
  }

  static box_type bbox (const C *e)
  {
    return box_type (e->p1 (), e->p2 ());
  }

  static tl::Variant intersect_point (const C *e, const C &ee)
  {
    std::pair<bool, point_type> ip = e->intersect_point (ee);
    if (ip.first) {
      return tl::Variant (ip.second);
    } else {
      return tl::Variant ();
    }
  }

  static tl::Variant cut_point (const C *e, const C &ee)
  {
    std::pair<bool, point_type> ip = e->cut_point (ee);
    if (ip.first) {
      return tl::Variant (ip.second);
    } else {
      return tl::Variant ();
    }
  }

  static point_type crossing_point (const C *e, const C &ee)
  {
    return e->crossed_by_point (ee).second;
  }

  static tl::Variant clipped (const C *e, const box_type &bx)
  {
    std::pair<bool, C> c = e->clipped (bx);
    if (c.first) {
      return tl::Variant (c.second);
    } else {
      return tl::Variant ();
    }
  }

  static tl::Variant clipped_line (const C *e, const box_type &bx)
  {
    std::pair<bool, C> c = e->clipped_line (bx);
    if (c.first) {
      return tl::Variant (c.second);
    } else {
      return tl::Variant ();
    }
  }

  static C &move_xy (C *e, coord_type dx, coord_type dy)
  {
    return e->move (vector_type (dx, dy));
  }

  static C moved_xy (const C *e, coord_type dx, coord_type dy)
  {
    return e->moved (vector_type (dx, dy));
  }

  static void set_p1 (C *e, const point_type &p)
  {
    *e = C (p, e->p2 ());
  }

  static void set_p2 (C *e, const point_type &p)
  {
    *e = C (e->p1 (), p);
  }

  static void set_x1 (C *e, coord_type v)
  {
    *e = C (point_type (v, e->p1 ().y ()), e->p2 ());
  }

  static void set_y1 (C *e, coord_type v)
  {
    *e = C (point_type (e->p1 ().x (), v), e->p2 ());
  }

  static void set_x2 (C *e, coord_type v)
  {
    *e = C (e->p1 (), point_type (v, e->p2 ().y ()));
  }

  static void set_y2 (C *e, coord_type v)
  {
    *e = C (e->p1 (), point_type (e->p2 ().x (), v));
  }

#if defined(HAVE_64BIT_COORD)
  //  workaround for missing 128bit binding of GSI
  static double sq_length (const C *edge)
#else
  static area_type sq_length (const C *edge)
#endif
  { 
    return edge->sq_length ();
  }

  static size_t hash_value (const C *e)
  {
    return std::hfunc (*e);
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v, 
      "@brief Default constructor: creates a degenerated edge 0,0 to 0,0"
    ) +
    constructor ("new|#new_xyxy", &new_xyxy, gsi::arg ("x1"), gsi::arg ("y1"), gsi::arg ("x2"), gsi::arg ("y2"),
      "@brief Constructor with two coordinates given as single values\n"
      "\n"
      "Two points are given to create a new edge."
    ) +
    constructor ("new|#new_pp", &new_pp, gsi::arg ("p1"), gsi::arg ("p2"),
      "@brief Constructor with two points\n"
      "\n"
      "Two points are given to create a new edge."
    ) +
    method ("<", &C::less, gsi::arg ("e"),
      "@brief Less operator\n"
      "@param e The object to compare against\n"
      "@return True, if the edge is 'less' as the other edge with respect to first and second point"
    ) +
    method ("==", &C::equal, gsi::arg ("e"),
      "@brief Equality test\n"
      "@param e The object to compare against"
    ) +
    method ("!=", &C::not_equal, gsi::arg ("e"),
      "@brief Inequality test\n"
      "@param e The object to compare against"
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given edge. This method enables edges as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    method ("moved", &C::moved, gsi::arg ("p"),
      "@brief Returns the moved edge (does not modify self)\n"
      "\n"
      "Moves the edge by the given offset and returns the \n"
      "moved edge. The edge is not modified.\n"
      "\n"
      "@param p The distance to move the edge.\n"
      "\n"
      "@return The moved edge.\n"
    ) +
    method_ext ("moved", &moved_xy, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Returns the moved edge (does not modify self)\n"
      "\n"
      "Moves the edge by the given offset and returns the \n"
      "moved edge. The edge is not modified.\n"
      "\n"
      "@param dx The x distance to move the edge.\n"
      "@param dy The y distance to move the edge.\n"
      "\n"
      "@return The moved edge.\n"
      "\n"
      "This version has been added in version 0.23.\n"
    ) +
    method ("enlarged", &C::enlarged, gsi::arg ("p"),
      "@brief Returns the enlarged edge (does not modify self)\n"
      "\n"
      "Enlarges the edge by the given offset and returns the \n"
      "enlarged edge. The edge is not modified. Enlargement means\n"
      "that the first point is shifted by -p, the second by p.\n"
      "\n"
      "@param p The distance to move the edge points.\n"
      "\n"
      "@return The enlarged edge.\n"
    ) +
    method ("extended", &C::extended, gsi::arg ("d"),
      "@brief Returns the extended edge (does not modify self)\n"
      "\n"
      "Extends the edge by the given distance and returns the \n"
      "extended edge. The edge is not modified. Extending means\n"
      "that the first point is shifted by -d along the edge, the second by d.\n"
      "The length of the edge will increase by 2*d.\n"
      "\n"
      "\\extend is a version that modifies self (in-place).\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@param d The distance by which to shift the end points.\n"
      "\n"
      "@return The extended edge.\n"
    ) +
    method ("extend", &C::extend, gsi::arg ("d"),
      "@brief Extends the edge (modifies self)\n"
      "\n"
      "Extends the edge by the given distance and returns the \n"
      "extended edge. The edge is not modified. Extending means\n"
      "that the first point is shifted by -d along the edge, the second by d.\n"
      "The length of the edge will increase by 2*d.\n"
      "\n"
      "\\extended is a version that does not modify self but returns the extended edges.\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@param d The distance by which to shift the end points.\n"
      "\n"
      "@return The extended edge (self).\n"
    ) +
    method ("shifted", &C::shifted, gsi::arg ("d"),
      "@brief Returns the shifted edge (does not modify self)\n"
      "\n"
      "Shifts the edge by the given distance and returns the \n"
      "shifted edge. The edge is not modified. Shifting by a positive value "
      "will produce an edge which is shifted by d to the left. Shifting by a negative value "
      "will produce an edge which is shifted by d to the right.\n"
      "\n"
      "\\shift is a version that modifies self (in-place).\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@param d The distance by which to shift the edge.\n"
      "\n"
      "@return The shifted edge.\n"
    ) +
    method ("shift", &C::shift, gsi::arg ("d"),
      "@brief Shifts the edge (modifies self)\n"
      "\n"
      "Shifts the edge by the given distance and returns the \n"
      "shifted edge. The edge is not modified. Shifting by a positive value "
      "will produce an edge which is shifted by d to the left. Shifting by a negative value "
      "will produce an edge which is shifted by d to the right.\n"
      "\n"
      "\\shifted is a version that does not modify self but returns the extended edges.\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@param d The distance by which to shift the edge.\n"
      "\n"
      "@return The shifted edge (self).\n"
    ) +
    method ("transformed", &C::template transformed<simple_trans_type>, gsi::arg ("t"),
      "@brief Transform the edge.\n"
      "\n"
      "Transforms the edge with the given transformation.\n"
      "Does not modify the edge but returns the transformed edge.\n"
      "\n"
      "@param t The transformation to apply.\n"
      "\n"
      "@return The transformed edge.\n"
    ) +
    method ("transformed|#transformed_cplx", &C::template transformed<complex_trans_type>, gsi::arg ("t"),
      "@brief Transform the edge.\n"
      "\n"
      "Transforms the edge with the given complex transformation.\n"
      "Does not modify the edge but returns the transformed edge.\n"
      "\n"
      "@param t The transformation to apply.\n"
      "\n"
      "@return The transformed edge.\n"
    ) +
    method ("move", &C::move, gsi::arg ("p"),
      "@brief Moves the edge.\n"
      "\n"
      "Moves the edge by the given offset and returns the \n"
      "moved edge. The edge is overwritten.\n"
      "\n"
      "@param p The distance to move the edge.\n"
      "\n"
      "@return The moved edge.\n"
    ) +
    method_ext ("move", &move_xy, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Moves the edge.\n"
      "\n"
      "Moves the edge by the given offset and returns the \n"
      "moved edge. The edge is overwritten.\n"
      "\n"
      "@param dx The x distance to move the edge.\n"
      "@param dy The y distance to move the edge.\n"
      "\n"
      "@return The moved edge.\n"
      "\n"
      "This version has been added in version 0.23.\n"
    ) +
    method ("enlarge", &C::enlarge, gsi::arg ("p"),
      "@brief Enlarges the edge.\n"
      "\n"
      "Enlarges the edge by the given distance and returns the \n"
      "enlarged edge. The edge is overwritten.\n"
      "Enlargement means\n"
      "that the first point is shifted by -p, the second by p.\n"
      "\n"
      "@param p The distance to move the edge points.\n"
      "\n"
      "@return The enlarged edge.\n"
    ) +
    method ("p1", &C::p1,
      "@brief The first point.\n"
    ) +
    method_ext ("p1=", &set_p1, gsi::arg ("point"),
      "@brief Sets the first point.\n"
      "This method has been added in version 0.23."
    ) +
    method ("p2", &C::p2,
      "@brief The second point.\n"
    ) +
    method_ext ("p2=", &set_p2, gsi::arg ("point"),
      "@brief Sets the second point.\n"
      "This method has been added in version 0.23."
    ) +
    method ("dx", &C::dx,
      "@brief The horizontal extend of the edge.\n"
    ) +
    method ("dy", &C::dy,
      "@brief The vertical extend of the edge.\n"
    ) +
    method ("x1", &C::x1,
      "@brief Shortcut for p1.x\n"
    ) +
    method_ext ("x1=", &set_x1, gsi::arg ("coord"),
      "@brief Sets p1.x\n"
      "This method has been added in version 0.23."
    ) +
    method ("y1", &C::y1,
      "@brief Shortcut for p1.y\n"
    ) +
    method_ext ("y1=", &set_y1, gsi::arg ("coord"),
      "@brief Sets p1.y\n"
      "This method has been added in version 0.23."
    ) +
    method ("x2", &C::x2,
      "@brief Shortcut for p2.x\n"
    ) +
    method_ext ("x2=", &set_x2, gsi::arg ("coord"),
      "@brief Sets p2.x\n"
      "This method has been added in version 0.23."
    ) +
    method ("y2", &C::y2,
      "@brief Shortcut for p2.y\n"
    ) +
    method_ext ("y2=", &set_y2, gsi::arg ("coord"),
      "@brief Sets p2.y\n"
      "This method has been added in version 0.23."
    ) +
    method ("dx_abs", &C::dx_abs,
      "@brief The absolute value of the horizontal extend of the edge.\n"
    ) +
    method ("dy_abs", &C::dy_abs,
      "@brief The absolute value of the vertical extend of the edge.\n"
    ) +
    method_ext ("bbox", &bbox,
      "@brief Return the bounding box of the edge.\n"
    ) +
    method ("is_degenerate?", &C::is_degenerate,
      "@brief Test for degenerated edge\n"
      "\n"
      "An edge is degenerate, if both end and start point are identical."
    ) +
    method ("length", &C::length,
      "@brief The length of the edge\n"
    ) +
    method_ext ("sq_length", &sq_length,
      "@brief The square of the length of the edge\n"
    ) +
    method ("ortho_length", &C::ortho_length,
      "@brief The orthogonal length of the edge (\"manhattan-length\")\n"
      "\n"
      "@return The orthogonal length (abs(dx)+abs(dy))\n"
    ) +
    constructor ("from_s", &from_string, gsi::arg ("s"),
      "@brief Creates an object from a string\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
      "\n"
      "This method has been added in version 0.23.\n"
    ) +
    method ("to_s", &C::to_string, gsi::arg ("dbu", 0.0),
      "@brief Returns a string representing the edge\n "
      "If a DBU is given, the output units will be micrometers.\n"
      "\n"
      "The DBU argument has been added in version 0.27.6.\n"
    ) +
    method ("is_parallel?", &C::parallel, gsi::arg ("e"),
      "@brief Test for being parallel\n"
      "\n"
      "@param e The edge to test against\n"
      "\n"
      "@return True if both edges are parallel\n"
    ) +
    method ("*", &C::scaled, gsi::arg ("scale_factor"),
      "@brief Scale edge\n"
      "\n"
      "The * operator scales self with the given factor.\n"
      "\n"
      "This method has been introduced in version 0.22.\n"
      "\n"
      "@param scale_factor The scaling factor\n"
      "\n"
      "@return The scaled edge\n"
    ) +
    method ("contains?", &C::contains, gsi::arg ("p"),
      "@brief Tests whether a point is on an edge.\n"
      "\n"
      "A point is on a edge if it is on (or at least closer \n"
      "than a grid point to) the edge.\n"
      "\n"
      "@param p The point to test with the edge.\n"
      "\n"
      "@return True if the point is on the edge.\n"
    ) +
    method ("contains_excl?", &C::contains_excl, gsi::arg ("p"),
      "@brief Tests whether a point is on an edge excluding the endpoints.\n"
      "\n"
      "A point is on a edge if it is on (or at least closer \n"
      "than a grid point to) the edge.\n"
      "\n"
      "@param p The point to test with the edge.\n"
      "\n"
      "@return True if the point is on the edge but not equal p1 or p2.\n"
    ) +
    method ("coincident?", &C::coincident, gsi::arg ("e"),
      "@brief Coincidence check.\n"
      "\n"
      "Checks whether a edge is coincident with another edge. \n"
      "Coincidence is defined by being parallel and that \n"
      "at least one point of one edge is on the other edge.\n"
      "\n"
      "@param e the edge to test with\n"
      "\n"
      "@return True if the edges are coincident.\n"
    ) +
    method ("intersects?|#intersect?", &C::intersect, gsi::arg ("e"),
      "@brief Intersection test. \n"
      "\n"
      "Returns true if the edges intersect. Two edges intersect if they share at least one point. \n"
      "If the edges coincide, they also intersect.\n"
      "If one of the edges is degenerate (both points are identical), that point is "
      "required to sit exaclty on the other edge. If both edges are degenerate, their "
      "points are required to be identical.\n"
      "\n"
      "@param e The edge to test.\n"
      "\n"
      "The 'intersects' (with an 's') synonym has been introduced in version 0.28.12.\n"
    ) +
    method_ext ("intersection_point", &intersect_point, gsi::arg ("e"),
      "@brief Returns the intersection point of two edges. \n"
      "\n"
      "This method delivers the intersection point. If the edges do not intersect, the result will be nil.\n"
      "\n"
      "@param e The edge to test.\n"
      "@return The point where the edges intersect.\n"
      "\n"
      "This method has been introduced in version 0.19.\n"
      "From version 0.26.2, this method will return nil in case of non-intersection.\n"
    ) +
    method_ext ("cut_point", &cut_point, gsi::arg ("e"),
      "@brief Returns the intersection point of the lines through the two edges.\n"
      "\n"
      "This method delivers the intersection point between the lines through the two edges. If the lines are parallel and do not intersect, the result will be nil.\n"
      "In contrast to \\intersection_point, this method will regard the edges as infinitely extended and intersection is not confined to the edge span.\n"
      "\n"
      "@param e The edge to test.\n"
      "@return The point where the lines intersect.\n"
      "\n"
      "This method has been introduced in version 0.27.1.\n"
    ) +
    method_ext ("clipped", &clipped, gsi::arg ("box"),
      "@brief Returns the edge clipped at the given box\n"
      "\n"
      "@param box The clip box.\n"
      "@return The clipped edge or nil if the edge does not intersect with the box.\n"
      "\n"
      "This method has been introduced in version 0.26.2.\n"
    ) +
    method_ext ("clipped_line", &clipped_line, gsi::arg ("box"),
      "@brief Returns the line through the edge clipped at the given box\n"
      "\n"
      "@param box The clip box.\n"
      "@return The part of the line through the box or nil if the line does not intersect with the box.\n"
      "\n"
      "In contrast to \\clipped, this method will consider the edge extended infinitely (a \"line\"). "
      "The returned edge will be the part of this line going through the box.\n"
      "\n"
      "This method has been introduced in version 0.26.2.\n"
    ) +
    method ("d", &C::d,
      "@brief Gets the edge extension as a vector.\n"
      "This method is equivalent to p2 - p1."
      "\n"
      "This method has been introduced in version 0.26.2.\n"
    ) +
    method ("distance", &C::distance, gsi::arg ("p"),
      "@brief Distance between the edge and a point.\n"
      "\n"
      "Returns the distance between the edge and the point. The \n"
      "distance is signed which is negative if the point is to the\n"
      "\"right\" of the edge and positive if the point is to the \"left\".\n"
      "The distance is measured by projecting the point onto the\n"
      "line through the edge. If the edge is degenerated, the distance\n"
      "is not defined.\n"
      "\n"
      "@param p The point to test.\n"
      "\n"
      "@return The distance\n"
    ) +
    method ("side_of", &C::side_of, gsi::arg ("p"),
      "@brief Indicates at which side the point is located relative to the edge.\n"
      "\n"
      "Returns 1 if the point is \"left\" of the edge, 0 if on\n"
      "and -1 if the point is \"right\" of the edge.\n"
      "\n"
      "@param p The point to test.\n"
      "\n"
      "@return The side value\n"
    ) +
    method ("distance_abs", &C::distance_abs, gsi::arg ("p"),
      "@brief Absolute distance between the edge and a point.\n"
      "\n"
      "Returns the distance between the edge and the point. \n"
      "\n"
      "@param p The point to test.\n"
      "\n"
      "@return The distance\n"
    ) +
    method ("swap_points", &C::swap_points,
      "@brief Swap the points of the edge\n"
      "\n"
      "This version modifies self. A version that does not modify self is \\swapped_points. "
      "Swapping the points basically reverses the direction of the edge.\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ) +
    method ("swapped_points", &C::swapped_points,
      "@brief Returns an edge in which both points are swapped\n"
      "\n"
      "Swapping the points basically reverses the direction of the edge.\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ) +
    method ("crossed_by?", &C::crossed_by, gsi::arg ("e"),
      "@brief Checks, if the line given by self is crossed by the edge e\n"
      "\n"
      "self if considered an infinite line. This predicate renders true "
      "if the edge e is cut by this line. In other words: "
      "this method returns true if e.p1 is in one semispace of self \n"
      "while e.p2 is in the other or one of them is exactly on self.\n"
      "\n"
      "@param e The edge representing the line that the edge must be crossing.\n"
    ) +
    method_ext ("crossing_point", &crossing_point, gsi::arg ("e"),
      "@brief Returns the crossing point on two edges. \n"
      "\n"
      "This method delivers the point where the given line (self) crosses the edge given "
      "by the argument \"e\". self is considered infinitely long and is required to cut "
      "through the edge \"e\". If self does not cut this line, the result is undefined. "
      "See \\crossed_by? for a description of the crossing predicate.\n"
      "\n"
      "@param e The edge representing the line that self must be crossing.\n"
      "@return The point where self crosses the line given by \"e\".\n"
      "\n"
      "This method has been introduced in version 0.19.\n"
    );
  }
};

static db::Edge *edge_from_dedge (const db::DEdge &e)
{
  return new db::Edge (e);
}

static db::DEdge edge_to_dedge (const db::Edge *e, double dbu)
{
  return db::DEdge (*e * dbu);
}

Class<db::Edge> decl_Edge ("db", "Edge",
  constructor ("new|#from_dedge", &edge_from_dedge, gsi::arg ("dedge"),
    "@brief Creates an integer coordinate edge from a floating-point coordinate edge\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dedge'."
  ) +
  method_ext ("to_dtype", &edge_to_dedge, gsi::arg ("dbu", 1.0),
    "@brief Converts the edge to a floating-point coordinate edge\n"
    "\n"
    "The database unit can be specified to translate the integer-coordinate edge into a floating-point coordinate "
    "edge in micron units. The database unit is basically a scaling factor.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::Edge::transformed<db::ICplxTrans>, gsi::arg ("t"),
    "@brief Transform the edge.\n"
    "\n"
    "Transforms the edge with the given complex transformation.\n"
    "Does not modify the edge but returns the transformed edge.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge (in this case an integer coordinate edge).\n"
    "\n"
    "This method has been introduced in version 0.18.\n"
  ) +
  edge_defs<db::Edge>::methods (),
  "@brief An edge class\n"
  "\n"
  "An edge is a connection between points, usually participating in a larger context "
  "such as a polygon. An edge has a defined direction (from p1 to p2). "
  "Edges play a role in the database as parts of polygons and to describe a line through both points.\n"
  "Although supported, edges are rarely used as individual database objects.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects like the Edge class."
);

static db::DEdge *dedge_from_iedge (const db::Edge &e)
{
  return new db::DEdge (e);
}

static db::Edge dedge_to_edge (const db::DEdge *e, double dbu)
{
  return db::Edge (*e * (1.0 / dbu));
}

Class<db::DEdge> decl_DEdge ("db", "DEdge",
  constructor ("new|#from_iedge", &dedge_from_iedge, gsi::arg ("edge"),
    "@brief Creates a floating-point coordinate edge from an integer coordinate edge\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_iedge'."
  ) +
  method_ext ("to_itype", &dedge_to_edge, gsi::arg ("dbu", 1.0),
    "@brief Converts the edge to an integer coordinate edge\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "edge in micron units to an integer-coordinate edge in database units. The edges "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::DEdge::transformed<db::VCplxTrans>, gsi::arg ("t"),
    "@brief Transforms the edge with the given complex transformation\n"
    "\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed edge (in this case an integer coordinate edge)\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  edge_defs<db::DEdge>::methods (),
  "@brief An edge class\n"
  "\n"
  "An edge is a connection between points, usually participating in a larger context "
  "such as a polygon. An edge has a defined direction (from p1 to p2). "
  "Edges play a role in the database as parts of polygons and to describe a line through both points.\n"
  "The \\Edge object is also used inside the boolean processor (\\EdgeProcessor).\n"
  "Although supported, edges are rarely used as individual database objects.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects like the Edge class."
);

}

