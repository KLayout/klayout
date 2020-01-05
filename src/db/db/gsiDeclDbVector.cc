
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "dbVector.h"
#include "dbPoint.h"
#include "dbHash.h"

namespace gsi
{

// ---------------------------------------------------------------
//  vector binding

template <class C>
struct vector_defs
{
  typedef typename C::coord_type coord_type;

  static C *from_string (const char *s)
  {
    tl::Extractor ex (s);
    std::auto_ptr<C> c (new C ());
    ex.read (*c.get ());
    return c.release ();
  }

  static C *new_v ()
  {
    return new C ();
  }

  static C *new_point (const db::point<coord_type> &p)
  {
    return new C (p - db::point<coord_type> ());
  }

  static C *new_xy (coord_type x, coord_type y)
  {
    return new C (x, y);
  }

  static db::point<coord_type> to_point (const C *p)
  {
    return db::point<coord_type> () + *p;
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

  static typename C::area_type vprod (const C *p, const C &q)
  {
    return db::vprod (*p, q);
  }

  static int vprod_sign (const C *p, const C &q)
  {
    return db::vprod_sign (*p, q);
  }

  static typename C::area_type sprod (const C *p, const C &q)
  {
    return db::sprod (*p, q);
  }

  static int sprod_sign (const C *p, const C &q)
  {
    return db::sprod_sign (*p, q);
  }

  static size_t hash_value (const C *v)
  {
    return std::hfunc (*v);
  }

  static db::point<coord_type> add_with_point (const C *v, const db::point<coord_type> &p)
  {
    return p + *v;
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v,
      "@brief Default constructor: creates a null vector with coordinates (0,0)"
    ) +
    constructor ("new", &new_point,
      "@brief Default constructor: creates a vector from a point\n"
      "@args p\n"
      "This constructor is equivalent to computing p-point(0,0).\n"
      "This method has been introduced in version 0.25."
    ) +
    constructor ("new", &new_xy,
      "@brief Constructor for a vector from two coordinate values\n"
      "\n"
      "@args x, y\n"
    ) +
    method_ext ("to_p", &to_point,
      "@brief Turns the vector into a point\n"
      "This method returns the point resulting from adding the vector to (0,0)."
      "\n"
      "This method has been introduced in version 0.25."
    ) +
    method_ext ("-@", &negate,
      "@brief Compute the negative of a vector\n"
      "\n"
      "@args p\n"
      "\n"
      "Returns a new vector with -x,-y.\n"
    ) +
    method ("+", (C (C::*) (const C &) const) &C::add,
      "@brief Adds two vectors\n"
      "\n"
      "@args v\n"
      "\n"
      "Adds vector v to self by adding the coordinates.\n"
    ) +
    method_ext ("+", &add_with_point,
      "@brief Adds a vector and a point\n"
      "\n"
      "@args p\n"
      "\n"
      "Returns the point p shifted by the vector.\n"
    ) +
    method ("-", (C (C::*) (const C &) const) &C::subtract,
      "@brief Subtract two vectors\n"
      "\n"
      "@args v\n"
      "\n"
      "Subtract vector v from self by subtracting the coordinates.\n"
    ) +
    method ("<", &C::less,
      "@brief \"less\" comparison operator\n"
      "\n"
      "@args v\n"
      "\n"
      "This operator is provided to establish a sorting\n"
      "order\n"
    ) +
    method ("==", &C::equal,
      "@brief Equality test operator\n"
      "\n"
      "@args v\n"
    ) +
    method ("!=", &C::not_equal,
      "@brief Inequality test operator\n"
      "\n"
      "@args v\n"
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given vector. This method enables vectors as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    method ("x", &C::x,
      "@brief Accessor to the x coordinate\n"
    ) +
    method ("y", &C::y,
      "@brief Accessor to the y coordinate\n"
    ) +
    method ("x=", &C::set_x,
      "@brief Write accessor to the x coordinate\n"
      "@args coord\n"
    ) +
    method ("y=", &C::set_y,
      "@brief Write accessor to the y coordinate\n"
      "@args coord\n"
    ) +
    method_ext ("*", &scale,
      "@brief Scaling by some factor\n"
      "\n"
      "@args f\n"
      "\n"
      "Returns the scaled object. All coordinates are multiplied with the given factor and if "
      "necessary rounded."
    ) +
    method_ext ("*=", &iscale,
      "@brief Scaling by some factor\n"
      "\n"
      "@args f\n"
      "\n"
      "Scales object in place. All coordinates are multiplied with the given factor and if "
      "necessary rounded."
    ) +
    method_ext ("/", &divide,
      "@brief Division by some divisor\n"
      "\n"
      "@args d\n"
      "\n"
      "Returns the scaled object. All coordinates are divided with the given divisor and if "
      "necessary rounded."
    ) +
    method_ext ("/=", &idiv,
      "@brief Division by some divisor\n"
      "\n"
      "@args d\n"
      "\n"
      "Divides the object in place. All coordinates are divided with the given divisor and if "
      "necessary rounded."
    ) +
    method_ext ("vprod", &vprod,
      "@brief Computes the vector product between self and the given vector\n"
      "\n"
      "@args v\n"
      "\n"
      "The vector product of a and b is defined as: vp = ax*by-ay*bx.\n"
    ) +
    method_ext ("vprod_sign", &vprod_sign,
      "@brief Computes the vector product between self and the given vector and returns a value indicating the sign of the product\n"
      "\n"
      "@args v\n"
      "\n"
      "@return 1 if the vector product is positive, 0 if it is zero and -1 if it is negative.\n"
    ) +
    method_ext ("sprod", &sprod,
      "@brief Computes the scalar product between self and the given vector\n"
      "\n"
      "@args v\n"
      "\n"
      "The scalar product of a and b is defined as: vp = ax*bx+ay*by.\n"
    ) +
    method_ext ("sprod_sign", &sprod_sign,
      "@brief Computes the scalar product between self and the given vector and returns a value indicating the sign of the product\n"
      "\n"
      "@args v\n"
      "\n"
      "@return 1 if the scalar product is positive, 0 if it is zero and -1 if it is negative.\n"
    ) +
    method ("length|abs", (double (C::*) () const) &C::double_length,
      "@brief Returns the length of the vector\n"
      "'abs' is an alias provided for compatibility with the former point type."
    ) +
    method ("sq_length|sq_abs", (double (C::*) () const) &C::sq_double_length,
      "@brief The square length of the vector\n"
      "'sq_abs' is an alias provided for compatibility with the former point type."
    ) +
    constructor ("from_s", &from_string,
      "@brief Creates an object from a string\n"
      "@args s\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
    ) +
    method ("to_s", (std::string (C::*) () const) &C::to_string,
      "@brief String conversion\n"
    );
  }

};

static db::DVector *dvector_from_ivector (const db::Vector &v)
{
  return new db::DVector (v);
}

static db::Vector dvector_to_vector (const db::DVector *v, double dbu)
{
  return db::Vector (*v * (1.0 / dbu));
}

Class<db::DVector> decl_DVector ("db", "DVector",
  constructor ("new", &dvector_from_ivector, gsi::arg ("vector"),
    "@brief Creates a floating-point coordinate vector from an integer coordinate vector\n"
  ) +
  method_ext ("to_itype", &dvector_to_vector, gsi::arg ("dbu", 1.0),
    "@brief Converts the point to an integer coordinate point\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "vector in micron units to an integer-coordinate vector in database units. The vector's' "
    "coordinates will be divided by the database unit.\n"
  ) +
  vector_defs<db::DVector>::methods (),
  "@brief A vector class with double (floating-point) coordinates\n"
  "A vector is a distance in cartesian, 2 dimensional space. A vector is given by two coordinates (x and y) and represents "
  "the distance between two points. Being the distance, transformations act differently on vectors: the displacement is not applied. "
  "\n"
  "Vectors are not geometrical objects by itself. But they are frequently used in the database API "
  "for various purposes. Other than the integer variant (\\Vector), points with floating-point coordinates can represent fractions of "
  "a database unit or vectors in physical (micron) units.\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

static db::Vector *vector_from_dvector (const db::DVector &v)
{
  return new db::Vector (v);
}

static db::DVector vector_to_dvector (const db::Vector *v, double dbu)
{
  return db::DVector (*v * dbu);
}

Class<db::Vector> decl_Vector ("db", "Vector",
  constructor ("new", &vector_from_dvector, gsi::arg ("dvector"),
    "@brief Creates an integer coordinate vector from a floating-point coordinate vector\n"
  ) +
  method_ext ("to_dtype", &vector_to_dvector, gsi::arg ("dbu", 1.0),
    "@brief Converts the vector to a floating-point coordinate vector"
    "\n"
    "The database unit can be specified to translate the integer-coordinate vector into a floating-point coordinate "
    "vector in micron units. The database unit is basically a scaling factor.\n"
  ) +
  vector_defs<db::Vector>::methods (),
  "@brief A integer vector class\n"
  "A vector is a distance in cartesian, 2 dimensional space. A vector is given by two coordinates (x and y) and represents "
  "the distance between two points. Being the distance, transformations act differently on vectors: the displacement is not applied. "
  "\n"
  "Vectors are not geometrical objects by itself. But they are frequently used in the database API "
  "for various purposes.\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

}

