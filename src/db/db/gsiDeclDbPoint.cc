
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
//  point binding

template <class C>
struct point_defs
{
  typedef typename C::coord_type coord_type;

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

  static C *new_vec (const db::vector<coord_type> &v)
  {
    return new C (C () + v);
  }

  static C *new_xy (coord_type x, coord_type y)
  {
    return new C (x, y);
  }

  static db::vector<coord_type> to_vector (const C *p)
  {
    return *p - C ();
  }

  static C scale (const C *p, double s)
  {
    return C (*p * s);
  }

  static C divide (const C *p, double s)
  {
    return C (*p / s);
  }

  static C iscale (C *p, double s)
  {
    *p *= s;
    return *p;
  }

  static C idiv (C *p, double s)
  {
    *p /= s;
    return *p;
  }

  static C negate (const C *p)
  {
    return -*p;
  }

  static size_t hash_value (const C *pt)
  {
    return std::hfunc (*pt);
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v,
      "@brief Default constructor: creates a point at 0,0"
    ) +
    constructor ("new", &new_vec, gsi::arg ("v"),
      "@brief Default constructor: creates a point at from an vector\n"
      "This constructor is equivalent to computing point(0,0)+v.\n"
      "This method has been introduced in version 0.25."
    ) +
    constructor ("new", &new_xy, gsi::arg ("x"), gsi::arg ("y"),
      "@brief Constructor for a point from two coordinate values\n"
      "\n"
    ) +
    method_ext ("to_v", &to_vector,
      "@brief Turns the point into a vector\n"
      "This method returns a vector representing the distance from (0,0) to the point."
      "This method has been introduced in version 0.25."
    ) +
    method_ext ("-@", &negate,
      "@brief Compute the negative of a point\n"
      "\n"
      "\n"
      "Returns a new point with -x, -y.\n"
      "\n"
      "This method has been added in version 0.23."
    ) +
    method ("+", (C (C::*) (const db::vector<coord_type> &) const) &C::add, gsi::arg ("v"),
      "@brief Adds a vector to a point\n"
      "\n"
      "\n"
      "Adds vector v to self by adding the coordinates.\n"
      "\n"
      "Starting with version 0.25, this method expects a vector argument."
    ) +
    method ("-", (db::vector<coord_type> (C::*) (const C &) const) &C::subtract, gsi::arg ("p"),
      "@brief Subtract one point from another\n"
      "\n"
      "\n"
      "Subtract point p from self by subtracting the coordinates. This renders a vector.\n"
      "\n"
      "Starting with version 0.25, this method renders a vector."
    ) +
    method ("-", (C (C::*) (const db::vector<coord_type> &) const) &C::subtract, gsi::arg ("v"),
      "@brief Subtract one vector from a point\n"
      "\n"
      "\n"
      "Subtract vector v from from self by subtracting the coordinates. This renders a point.\n"
      "\n"
      "This method has been added in version 0.27."
    ) +
    method ("<", &C::less, gsi::arg ("p"),
      "@brief \"less\" comparison operator\n"
      "\n"
      "\n"
      "This operator is provided to establish a sorting\n"
      "order\n"
    ) +
    method ("==", &C::equal, gsi::arg ("p"),
      "@brief Equality test operator\n"
      "\n"
    ) +
    method ("!=", &C::not_equal, gsi::arg ("p"),
      "@brief Inequality test operator\n"
      "\n"
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given point. This method enables points as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    method ("x", &C::x,
      "@brief Accessor to the x coordinate\n"
    ) +
    method ("y", &C::y,
      "@brief Accessor to the y coordinate\n"
    ) +
    method ("x=", &C::set_x, gsi::arg ("coord"),
      "@brief Write accessor to the x coordinate\n"
    ) +
    method ("y=", &C::set_y, gsi::arg ("coord"),
      "@brief Write accessor to the y coordinate\n"
    ) +
    method_ext ("*", &scale, gsi::arg ("f"),
      "@brief Scaling by some factor\n"
      "\n"
      "\n"
      "Returns the scaled object. All coordinates are multiplied with the given factor and if "
      "necessary rounded."
    ) +
    method_ext ("*=", &iscale, gsi::arg ("f"),
      "@brief Scaling by some factor\n"
      "\n"
      "\n"
      "Scales object in place. All coordinates are multiplied with the given factor and if "
      "necessary rounded."
    ) +
    method_ext ("/", &divide, gsi::arg ("d"),
      "@brief Division by some divisor\n"
      "\n"
      "\n"
      "Returns the scaled object. All coordinates are divided with the given divisor and if "
      "necessary rounded."
    ) +
    method_ext ("/=", &idiv, gsi::arg ("d"),
      "@brief Division by some divisor\n"
      "\n"
      "\n"
      "Divides the object in place. All coordinates are divided with the given divisor and if "
      "necessary rounded."
    ) +
    method ("distance", (double (C::*) (const C &) const) &C::double_distance, gsi::arg ("d"),
      "@brief The Euclidian distance to another point\n"
      "\n"
      "\n"
      "@param d The other point to compute the distance to.\n"
    ) +
    method ("sq_distance", (double (C::*) (const C &) const) &C::sq_double_distance, gsi::arg ("d"),
      "@brief The square Euclidian distance to another point\n"
      "\n"
      "\n"
      "@param d The other point to compute the distance to.\n"
    ) +
    method ("abs", (double (C::*) () const) &C::double_distance,
      "@brief The absolute value of the point (Euclidian distance to 0,0)\n"
      "\n"
      "The returned value is 'sqrt(x*x+y*y)'.\n"
      "\n"
      "This method has been introduced in version 0.23."
    ) +
    method ("sq_abs", (double (C::*) () const) &C::sq_double_distance,
      "@brief The square of the absolute value of the point (Euclidian distance to 0,0)\n"
      "\n"
      "The returned value is 'x*x+y*y'.\n"
      "\n"
      "This method has been introduced in version 0.23."
    ) +
    constructor ("from_s", &from_string, gsi::arg ("s"),
      "@brief Creates an object from a string\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
      "\n"
      "This method has been added in version 0.23.\n"
    ) +
    method ("to_s", &C::to_string, gsi::arg ("dbu", 0.0),
      "@brief String conversion.\n"
      "If a DBU is given, the output units will be micrometers.\n"
      "\n"
      "The DBU argument has been added in version 0.27.6.\n"
    );
  }

};

static db::DPoint *dpoint_from_ipoint (const db::Point &p)
{
  return new db::DPoint (p);
}

static db::Point dpoint_to_point (const db::DPoint *p, double dbu)
{
  return db::Point (*p * (1.0 / dbu));
}

Class<db::DPoint> decl_DPoint ("db", "DPoint",
  constructor ("new|#from_ipoint", &dpoint_from_ipoint, gsi::arg ("point"),
    "@brief Creates a floating-point coordinate point from an integer coordinate point\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_ipoint'."
  ) +
  method_ext ("to_itype", &dpoint_to_point, gsi::arg ("dbu", 1.0),
    "@brief Converts the point to an integer coordinate point\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "point in micron units to an integer-coordinate point in database units. The point's' "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  point_defs<db::DPoint>::methods (),
  "@brief A point class with double (floating-point) coordinates\n"
  "Points represent a coordinate in the two-dimensional coordinate space of layout. "
  "They are not geometrical objects by itself. But they are frequently used in the database API "
  "for various purposes. Other than the integer variant (\\Point), points with floating-point coordinates can represent fractions of "
  "a database unit.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

static db::Point *point_from_dpoint (const db::DPoint &p)
{
  return new db::Point (p);
}

static db::DPoint point_to_dpoint (const db::Point *p, double dbu)
{
  return db::DPoint (*p * dbu);
}

Class<db::Point> decl_Point ("db", "Point",
  constructor ("new|#from_dpoint", &point_from_dpoint, gsi::arg ("dpoint"),
    "@brief Creates an integer coordinate point from a floating-point coordinate point\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dpoint'."
  ) +
  method_ext ("to_dtype", &point_to_dpoint, gsi::arg ("dbu", 1.0),
    "@brief Converts the point to a floating-point coordinate point\n"
    "\n"
    "The database unit can be specified to translate the integer-coordinate point into a floating-point coordinate "
    "point in micron units. The database unit is basically a scaling factor.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  point_defs<db::Point>::methods (),
  "@brief An integer point class\n"
  "Points represent a coordinate in the two-dimensional coordinate space of layout. "
  "They are not geometrical objects by itself. But they are frequently used in the database API "
  "for various purposes.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

}
