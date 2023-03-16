
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
#include "dbBox.h"
#include "dbHash.h"

namespace gsi
{

// ---------------------------------------------------------------
//  box binding

template <class C>
struct box_defs 
{
  typedef typename C::coord_type coord_type;
  typedef typename C::point_type point_type;
  typedef typename C::vector_type vector_type;
  typedef db::simple_trans<coord_type> simple_trans_type;
  typedef db::complex_trans<coord_type, double> complex_trans_type;

  static C *from_string (const char *s)
  {
    tl::Extractor ex (s);
    std::unique_ptr<C> c (new C ());
    ex.read (*c.get ());
    return c.release ();
  }

  static C world ()
  {
    return C::world ();
  }

  static C *new_v ()
  {
    return new C ();
  }

  static C *new_sq (coord_type s)
  {
    return new C (-s / 2, -s / 2, s / 2, s / 2);
  }

  static C *new_wh (coord_type w, coord_type h)
  {
    return new C (-w / 2, -h / 2, w / 2, h / 2);
  }

  static C *new_lbrt (coord_type l, coord_type b, coord_type r, coord_type t)
  {
    return new C (l, b, r, t);
  }

  static C *new_pp (const point_type &ll, const point_type &ur)
  {
    return new C (ll, ur);
  }

  static C join_with_point (const C *box, const point_type &p)
  {
    C b (*box);
    b += p;
    return b;
  }

  static bool contains (const C *box, coord_type x, coord_type y)
  {
    return box->contains (point_type (x, y));
  }

  static C &enlarge (C *box, coord_type x, coord_type y)
  {
    return box->enlarge (vector_type (x, y));
  }

  static C &enlarge1 (C *box, coord_type s)
  {
    return box->enlarge (vector_type (s, s));
  }

  static C enlarged (const C *box, coord_type x, coord_type y)
  {
    return box->enlarged (vector_type (x, y));
  }

  static C enlarged1 (const C *box, coord_type s)
  {
    return box->enlarged (vector_type (s, s));
  }

  static C &move (C *box, coord_type x, coord_type y)
  {
    return box->move (vector_type (x, y));
  }

  static C moved (const C *box, coord_type x, coord_type y)
  {
    return box->moved (vector_type (x, y));
  }

  static size_t hash_value (const C *box)
  {
    return std::hfunc (*box);
  }

  static const C &bbox (const C *box)
  {
    return *box;
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v, 
      "@brief Creates an empty (invalid) box\n"
      "\n"
      "Empty boxes don't modify a box when joined with it. The intersection between an empty and any other "
      "box is also an empty box. The width, height, p1 and p2 attributes of an empty box are undefined. "
      "Use \\empty? to get a value indicating whether the box is empty.\n"
    ) +
    constructor ("new", &new_sq, gsi::arg ("w"),
      "@brief Creates a square with the given dimensions centered around the origin\n"
      "\n"
      "Note that for integer-unit boxes, the dimension has to be an even number to avoid rounding.\n"
      "\n"
      "This convenience constructor has been introduced in version 0.28."
    ) +
    constructor ("new", &new_wh, gsi::arg ("w"), gsi::arg ("h"),
      "@brief Creates a rectangle with given width and height, centered around the origin\n"
      "\n"
      "Note that for integer-unit boxes, the dimensions have to be an even number to avoid rounding.\n"
      "\n"
      "This convenience constructor has been introduced in version 0.28."
    ) +
    constructor ("new", &new_lbrt, gsi::arg ("left"), gsi::arg ("bottom"), gsi::arg ("right"), gsi::arg ("top"),
      "@brief Creates a box with four coordinates\n"
      "\n"
      "\n"
      "Four coordinates are given to create a new box. If the coordinates "
      "are not provided in the correct order (i.e. right < left), these are "
      "swapped."
    ) +
    constructor ("new", &new_pp, gsi::arg ("lower_left"), gsi::arg ("upper_right"),
      "@brief Creates a box from two points\n"
      "\n"
      "\n"
      "Two points are given to create a new box. If the coordinates "
      "are not provided in the correct order (i.e. right < left), these are "
      "swapped."
    ) +
    method ("world", &world,
      "@brief Gets the 'world' box\n"
      "The world box is the biggest box that can be represented. So it is basically 'all'. The "
      "world box behaves neutral on intersections for example. In other operations such as displacement or transformations, "
      "the world box may render unexpected results because of coordinate overflow.\n"
      "\n"
      "The world box can be used\n"
      "@ul\n"
      "  @li for comparison ('==', '!=', '<') @/li\n"
      "  @li in union and intersection ('+' and '&') @/li\n"
      "  @li in relations (\\contains?, \\overlaps?, \\touches?) @/li\n"
      "  @li as 'all' argument in region queries @/li\n"
      "@/ul\n"
      "\n"
      "This method has been introduced in version 0.28."
    ) +
    method ("p1", &C::p1,
      "@brief Gets the lower left point of the box\n"
    ) +
    method ("p2", &C::p2,
      "@brief Gets the upper right point of the box\n"
    ) +
    method ("center", &C::center,
      "@brief Gets the center of the box\n"
    ) +
    method ("left", &C::left,
      "@brief Gets the left coordinate of the box\n"
    ) +
    method ("right", &C::right,
      "@brief Gets the right coordinate of the box\n"
    ) +
    method ("bottom", &C::bottom,
      "@brief Gets the bottom coordinate of the box\n"
    ) +
    method ("top", &C::top,
      "@brief Gets the top coordinate of the box\n"
    ) +
    method ("width", &C::width,
      "@brief Gets the width of the box\n"
    ) +
    method ("height", &C::height,
      "@brief Gets the height of the box\n"
    ) +
    method ("left=", &C::set_left, gsi::arg ("c"),
      "@brief Sets the left coordinate of the box\n"
    ) +
    method ("right=", &C::set_right, gsi::arg ("c"),
      "@brief Sets the right coordinate of the box\n"
    ) +
    method ("bottom=", &C::set_bottom, gsi::arg ("c"),
      "@brief Sets the bottom coordinate of the box\n"
    ) +
    method ("top=", &C::set_top, gsi::arg ("c"),
      "@brief Sets the top coordinate of the box\n"
    ) +
    method ("p1=", &C::set_p1, gsi::arg ("p"),
      "@brief Sets the lower left point of the box\n"
    ) +
    method ("p2=", &C::set_p2, gsi::arg ("p"),
      "@brief Sets the upper right point of the box\n"
    ) +
    method_ext ("bbox", &bbox,
      "@brief Returns the bounding box\n"
      "This method is provided for consistency of the shape API is returns the box itself.\n"
      "\n"
      "This method has been introduced in version 0.27."
    ) +
    method_ext ("contains?", &box_defs<C>::contains, gsi::arg ("x"), gsi::arg ("y"),
      "@brief Returns true if the box contains the given point\n"
      "\n"
      "\n"
      "Tests whether a point (x, y) is inside the box.\n"
      "It also returns true if the point is exactly on the box contour.\n"
      "\n"
      "@return true if the point is inside the box.\n"
    ) +
    method ("contains?", &C::contains, gsi::arg ("point"),
      "@brief Returns true if the box contains the given point\n"
      "\n"
      "\n"
      "Tests whether a point is inside the box.\n"
      "It also returns true if the point is exactly on the box contour.\n"
      "\n"
      "@param p The point to test against.\n"
      "\n"
      "@return true if the point is inside the box.\n"
    ) +
    method ("empty?", &C::empty,
      "@brief Returns a value indicating whether the box is empty\n"
      "\n"
      "An empty box may be created with the default constructor for example. "
      "Such a box is neutral when combining it with other boxes and renders empty boxes "
      "if used in box intersections and false in geometrical relationship tests. "
    ) +
    method ("inside?", &C::inside, gsi::arg ("box"),
      "@brief Tests if this box is inside the argument box\n"
      "\n"
      "\n"
      "Returns true, if this box is inside the given box, i.e. the box intersection renders this box"
    ) +
    method ("touches?", &C::touches, gsi::arg ("box"),
      "@brief Tests if this box touches the argument box\n"
      "\n"
      "\n"
      "Two boxes touch if they overlap or their boundaries share at least one common point. "
      "Touching is equivalent to a non-empty intersection ('!(b1 & b2).empty?')."
    ) +
    method ("overlaps?", &C::overlaps, gsi::arg ("box"),
      "@brief Tests if this box overlaps the argument box\n"
      "\n"
      "\n"
      "Returns true, if the intersection box of this box with the argument box exists and has a non-vanishing area"
    ) +
    method ("area", &C::double_area,
      "@brief Computes the box area\n"
      "\n"
      "Returns the box area or 0 if the box is empty"
    ) +
    method ("is_point?", &C::is_point,
      "@brief Returns true, if the box is a single point\n"
    ) +
    method ("perimeter", &C::perimeter,
      "@brief Returns the perimeter of the box\n"
      "\n"
      "This method is equivalent to 2*(width+height). For empty boxes, this method returns 0.\n"
      "\n"
      "This method has been introduced in version 0.23."
    ) + 
    method_ext ("+", &box_defs<C>::join_with_point, gsi::arg ("point"),
      "@brief Joins box with a point\n"
      "\n"
      "\n"
      "The + operator joins a point with the box. The resulting box will enclose both the original "
      "box and the point.\n"
      "\n"
      "@param point The point to join with this box.\n"
      "\n"
      "@return The box joined with the point\n"
    ) +
    method ("+", &C::joined, gsi::arg ("box"),
      "@brief Joins two boxes\n"
      "\n"
      "\n"
      "The + operator joins the first box with the one given as \n"
      "the second argument. Joining constructs a box that encloses\n"
      "both boxes given. Empty boxes are neutral: they do not\n"
      "change another box when joining. Overwrites this box\n"
      "with the result.\n"
      "\n"
      "@param box The box to join with this box.\n"
      "\n"
      "@return The joined box\n"
    ) +
    method ("&", &C::intersection, gsi::arg ("box"),
      "@brief Returns the intersection of this box with another box\n"
      "\n"
      "\n"
      "The intersection of two boxes is the largest\n"
      "box common to both boxes. The intersection may be \n"
      "empty if both boxes to not touch. If the boxes do\n"
      "not overlap but touch the result may be a single\n"
      "line or point with an area of zero. Overwrites this box\n"
      "with the result.\n"
      "\n"
      "@param box The box to take the intersection with\n"
      "\n"
      "@return The intersection box\n"
    ) +
    method ("*", &C::convolved, gsi::arg ("box"),
      "@brief Returns the convolution product from this box with another box\n"
      "\n"
      "\n"
      "The * operator convolves the firstbox with the one given as \n"
      "the second argument. The box resulting from \"convolution\" is the\n"
      "outer boundary of the union set formed by placing \n"
      "the second box at every point of the first. In other words,\n"
      "the returned box of (p1,p2)*(q1,q2) is (p1+q1,p2+q2).\n"
      "\n"
      "@param box The box to convolve with this box.\n"
      "\n"
      "@return The convolved box\n"
    ) +
    method ("*", &C::scaled, gsi::arg ("scale_factor"),
      "@brief Returns the scaled box\n"
      "\n"
      "\n"
      "The * operator scales the box with the given factor and returns the result.\n"
      "\n"
      "This method has been introduced in version 0.22.\n"
      "\n"
      "@param scale_factor The scaling factor\n"
      "\n"
      "@return The scaled box\n"
    ) +
    method_ext ("move", &box_defs<C>::move, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Moves the box by a certain distance\n"
      "\n"
      "\n"
      "This is a convenience method which takes two values instead of a Point object.\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@return A reference to this box.\n"
    ) +
    method_ext ("moved", &box_defs<C>::moved, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Moves the box by a certain distance\n"
      "\n"
      "\n"
      "This is a convenience method which takes two values instead of a Point object.\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@return The moved box.\n"
    ) +
    method ("move", &C::move, gsi::arg ("distance"),
      "@brief Moves the box by a certain distance\n"
      "\n"
      "\n"
      "Moves the box by a given offset and returns the moved\n"
      "box. Does not check for coordinate overflows.\n"
      "\n"
      "@param distance The offset to move the box.\n"
      "\n"
      "@return A reference to this box.\n"
    ) +
    method ("moved", &C::moved, gsi::arg ("distance"),
      "@brief Returns the box moved by a certain distance\n"
      "\n"
      "\n"
      "Moves the box by a given offset and returns the moved\n"
      "box. Does not modify this box. Does not check for coordinate\n"
      "overflows.\n"
      "\n"
      "@param distance The offset to move the box.\n"
      "\n"
      "@return The moved box.\n"
    ) +
    method_ext ("enlarge", &box_defs<C>::enlarge, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Enlarges the box by a certain amount.\n"
      "\n"
      "\n"
      "This is a convenience method which takes two values instead of a Vector object.\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@return A reference to this box.\n"
    ) +
    method_ext ("enlarge", &box_defs<C>::enlarge1, gsi::arg ("d"),
      "@brief Enlarges the box by a certain amount on all sides.\n"
      "\n"
      "This is a convenience method which takes one values instead of two values. It will apply the given enlargement in both directions.\n"
      "This method has been introduced in version 0.28.\n"
      "\n"
      "@return A reference to this box.\n"
    ) +
    method_ext ("enlarged", &box_defs<C>::enlarged, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Enlarges the box by a certain amount.\n"
      "\n"
      "\n"
      "This is a convenience method which takes two values instead of a Vector object.\n"
      "This method has been introduced in version 0.23.\n"
      "\n"
      "@return The enlarged box.\n"
    ) +
    method_ext ("enlarged", &box_defs<C>::enlarged1, gsi::arg ("d"),
      "@brief Enlarges the box by a certain amount on all sides.\n"
      "\n"
      "This is a convenience method which takes one values instead of two values. It will apply the given enlargement in both directions.\n"
      "This method has been introduced in version 0.28.\n"
      "\n"
      "@return The enlarged box.\n"
    ) +
    method ("enlarge", &C::enlarge, gsi::arg ("enlargement"),
      "@brief Enlarges the box by a certain amount.\n"
      "\n"
      "\n"
      "Enlarges the box by x and y value specified in the vector\n"
      "passed. Positive values with grow the box, negative ones\n"
      "will shrink the box. The result may be an empty box if the\n"
      "box disappears. The amount specifies the grow or shrink\n"
      "per edge. The width and height will change by twice the\n"
      "amount.\n"
      "Does not check for coordinate\n"
      "overflows.\n"
      "\n"
      "@param enlargement The grow or shrink amount in x and y direction\n"
      "\n"
      "@return A reference to this box.\n"
    ) +
    method ("enlarged", &C::enlarged, gsi::arg ("enlargement"),
      "@brief Returns the enlarged box.\n"
      "\n"
      "\n"
      "Enlarges the box by x and y value specified in the vector\n"
      "passed. Positive values with grow the box, negative ones\n"
      "will shrink the box. The result may be an empty box if the\n"
      "box disappears. The amount specifies the grow or shrink\n"
      "per edge. The width and height will change by twice the\n"
      "amount.\n"
      "Does not modify this box. Does not check for coordinate\n"
      "overflows.\n"
      "\n"
      "@param enlargement The grow or shrink amount in x and y direction\n"
      "\n"
      "@return The enlarged box.\n"
    ) +
    method ("transformed", &C::template transformed<simple_trans_type>, gsi::arg ("t"),
      "@brief Returns the box transformed with the given simple transformation\n"
      "\n"
      "\n"
      "@param t The transformation to apply\n"
      "@return The transformed box\n"
    ) +
    method ("transformed", &C::template transformed<complex_trans_type>, gsi::arg ("t"),
      "@brief Returns the box transformed with the given complex transformation\n"
      "\n"
      "\n"
      "@param t The magnifying transformation to apply\n"
      "@return The transformed box (a DBox now)\n"
    ) +
    method ("<", &C::less, gsi::arg ("box"),
      "@brief Returns true if this box is 'less' than another box\n"
      "Returns true, if this box is 'less' with respect to first and second point (in this order)"
    ) +
    method ("==", &C::equal, gsi::arg ("box"),
      "@brief Returns true if this box is equal to the other box\n"
      "Returns true, if this box and the given box are equal "
    ) +
    method ("!=", &C::not_equal, gsi::arg ("box"),
      "@brief Returns true if this box is not equal to the other box\n"
      "Returns true, if this box and the given box are not equal "
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given box. This method enables boxes as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    constructor ("from_s", &from_string, gsi::arg ("s"),
      "@brief Creates a box object from a string\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
      "\n"
      "This method has been added in version 0.23.\n"
    ) +
    method ("to_s", &C::to_string, gsi::arg ("dbu", 0.0),
      "@brief Returns a string representing this box\n"
      "\n"
      "This string can be turned into a box again by using \\from_s\n. "
      "If a DBU is given, the output units will be micrometers.\n"
      "\n"
      "The DBU argument has been added in version 0.27.6.\n"
    );
  }
};

static db::Box *box_from_dbox (const db::DBox &b)
{
  return new db::Box (b);
}

static db::DBox box_to_dbox (const db::Box *b, double dbu)
{
  return db::DBox (*b) * dbu;
}

Class<db::Box> decl_Box ("db", "Box",
  constructor ("new|#from_dbox", &box_from_dbox, gsi::arg ("dbox"),
    "@brief Creates an integer coordinate box from a floating-point coordinate box\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dbox'."
  ) +
  method_ext ("to_dtype", &box_to_dbox, gsi::arg ("dbu", 1.0),
    "@brief Converts the box to a floating-point coordinate box\n"
    "\n"
    "The database unit can be specified to translate the integer-coordinate box into a floating-point coordinate "
    "box in micron units. The database unit is basically a scaling factor.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::Box::transformed<db::ICplxTrans>, gsi::arg ("t"),
    "@brief Transforms the box with the given complex transformation\n"
    "\n"
    "\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed box (in this case an integer coordinate box)\n"
    "\n"
    "This method has been introduced in version 0.18.\n"
  ) +
  box_defs<db::Box>::methods (),
  "@brief A box class with integer coordinates\n"
  "\n"
  "This object represents a box (a rectangular shape).\n"
  "\n"
  "The definition of the attributes is: p1 is the lower left point, p2 the \n"
  "upper right one. If a box is constructed from two points (or four coordinates), the \n"
  "coordinates are sorted accordingly.\n"
  "\n"
  "A box can be empty. An empty box represents no area\n"
  "(not even a point). Empty boxes behave neutral with respect to most operations. \n"
  "Empty boxes return true on \\empty?.\n"
  "\n"
  "A box can be a point or a single\n"
  "line. In this case, the area is zero but the box still\n"
  "can overlap other boxes for example and it is not empty. \n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

static db::DBox *dbox_from_ibox (const db::Box &b)
{
  return new db::DBox (b);
}

static db::Box dbox_to_box (const db::DBox *b, double dbu)
{
  return db::Box (*b * (1.0 / dbu));
}

Class<db::DBox> decl_DBox ("db", "DBox",
  constructor ("new|#from_ibox", &dbox_from_ibox, gsi::arg ("box"),
    "@brief Creates a floating-point coordinate box from an integer coordinate box\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_ibox'."
  ) +
  method_ext ("to_itype", &dbox_to_box, gsi::arg ("dbu", 1.0),
    "@brief Converts the box to an integer coordinate box\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "box in micron units to an integer-coordinate box in database units. The boxes "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::DBox::transformed<db::VCplxTrans>, gsi::arg ("t"),
    "@brief Transforms the box with the given complex transformation\n"
    "\n"
    "\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed box (in this case an integer coordinate box)\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  box_defs<db::DBox>::methods (),
  "@brief A box class with floating-point coordinates\n"
  "\n"
  "This object represents a box (a rectangular shape).\n"
  "\n"
  "The definition of the attributes is: p1 is the lower left point, p2 the \n"
  "upper right one. If a box is constructed from two points (or four coordinates), the \n"
  "coordinates are sorted accordingly.\n"
  "\n"
  "A box can be empty. An empty box represents no area\n"
  "(not even a point). Empty boxes behave neutral with respect to most operations. \n"
  "Empty boxes return true on \\empty?.\n"
  "\n"
  "A box can be a point or a single\n"
  "line. In this case, the area is zero but the box still\n"
  "can overlap other boxes for example and it is not empty.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

}
