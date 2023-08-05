
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
#include "dbEdge.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbText.h"
#include "dbHash.h"

namespace gsi
{

// ---------------------------------------------------------------
//  simple_trans binding

template <class C>
struct trans_defs 
{
  typedef typename C::coord_type coord_type;
  typedef typename C::displacement_type displacement_type;
  typedef typename db::point<coord_type> point_type;
  typedef typename db::vector<coord_type> vector_type;

  static C trans_r0 ()   { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r0));   }
  static C trans_r90 ()  { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r90));  }
  static C trans_r180 () { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r180)); }
  static C trans_r270 () { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r270)); }
  static C trans_m0 ()   { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m0));   }
  static C trans_m45 ()  { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m45));  }
  static C trans_m90 ()  { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m90));  }
  static C trans_m135 () { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m135)); }

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

  static C *new_cu (const C &c, const displacement_type &u)
  {
    return new C (C (u) * c);
  }

  static C *new_cxy (const C &c, coord_type x, coord_type y)
  {
    return new C (C (displacement_type (x, y)) * c);
  }

  static C *new_xy (coord_type x, coord_type y)
  {
    return new C (displacement_type (x, y));
  }

  static C *new_rmxy (int r, bool m, coord_type x, coord_type y)
  {
    return new C (r, m, displacement_type (x, y));
  }

  static C *new_u (const displacement_type &u)
  {
    return new C (u);
  }

  static C *new_rmu (int r, bool m, const displacement_type &u)
  {
    return new C (r, m, u);
  }

  static void set_angle (C *trans, int angle)
  {
    *trans = C (angle, trans->is_mirror (), trans->disp ());
  }

  static void set_rot (C *trans, int rot)
  {
    *trans = C (rot, trans->disp ());
  }

  static void set_mirror (C *trans, bool mirror)
  {
    *trans = C (trans->angle (), mirror, trans->disp ());
  }

  static db::edge<coord_type> trans_edge (const C *t, const db::edge<coord_type> &edge)
  {
    return edge.transformed (*t);
  }

  static db::box<coord_type> trans_box (const C *t, const db::box<coord_type> &box)
  {
    return box.transformed (*t);
  }

  static db::polygon<coord_type> trans_polygon (const C *t, const db::polygon<coord_type> &polygon)
  {
    return polygon.transformed (*t);
  }

  static db::path<coord_type> trans_path (const C *t, const db::path<coord_type> &path)
  {
    return path.transformed (*t);
  }

  static db::text<coord_type> trans_text (const C *t, const db::text<coord_type> &text)
  {
    return text.transformed (*t);
  }

  static size_t hash_value (const C *t)
  {
    return std::hfunc (*t);
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v, 
      "@brief Creates a unit transformation\n"
    ) +
    constructor ("new", &new_cu, arg ("c"), arg ("u", displacement_type ()),
      "@brief Creates a transformation from another transformation plus a displacement\n"
      "\n"
      "Creates a new transformation from a existing transformation. This constructor is provided for creating duplicates "
      "and backward compatibility since the constants are transformations now. It will copy the original transformation "
      "and add the given displacement.\n"
      "\n"
      "This variant has been introduced in version 0.25.\n"
      "\n"
      "@param c The original transformation\n"
      "@param u The Additional displacement\n"
    ) +
    constructor ("new", &new_cxy, arg ("c"), arg ("x"), arg ("y"),
      "@brief Creates a transformation from another transformation plus a displacement\n"
      "\n"
      "Creates a new transformation from a existing transformation. This constructor is provided for creating duplicates "
      "and backward compatibility since the constants are transformations now. It will copy the original transformation "
      "and add the given displacement.\n"
      "\n"
      "This variant has been introduced in version 0.25.\n"
      "\n"
      "@param c The original transformation\n"
      "@param x The Additional displacement (x)\n"
      "@param y The Additional displacement (y)\n"
    ) +
    constructor ("new", &new_rmu, arg ("rot"), arg ("mirr", false), arg ("u", displacement_type ()),
      "@brief Creates a transformation using angle and mirror flag\n"
      "\n"
      "The sequence of operations is: mirroring at x axis,\n"
      "rotation, application of displacement.\n"
      "\n"
      "@param rot The rotation in units of 90 degree\n"
      "@param mirrx True, if mirrored at x axis\n"
      "@param u The displacement\n"
    ) +
    constructor ("new", &new_rmxy, arg ("rot"), arg ("mirr"), arg ("x"), arg ("y"),
      "@brief Creates a transformation using angle and mirror flag and two coordinate values for displacement\n"
      "\n"
      "The sequence of operations is: mirroring at x axis,\n"
      "rotation, application of displacement.\n"
      "\n"
      "@param rot The rotation in units of 90 degree\n"
      "@param mirrx True, if mirrored at x axis\n"
      "@param x The horizontal displacement\n"
      "@param y The vertical displacement\n"
    ) +
    constructor ("new", &new_u, arg ("u"),
      "@brief Creates a transformation using a displacement only\n"
      "\n"
      "@param u The displacement\n"
    ) +
    constructor ("new", &new_xy, arg ("x"), arg ("y"),
      "@brief Creates a transformation using a displacement given as two coordinates\n"
      "\n"
      "@param x The horizontal displacement\n"
      "@param y The vertical displacement\n"
    ) +
    method ("inverted", &C::inverted, 
      "@brief Returns the inverted transformation"
      "\n"
      "Returns the inverted transformation\n"
      "\n"
      "@return The inverted transformation\n"
    ) +
    method ("invert", &C::invert,
      "@brief Inverts the transformation (in place)\n"
      "\n"
      "Inverts the transformation and replaces this object by the\n"
      "inverted one.\n"
      "\n"
      "@return The inverted transformation\n"
    ) +
    method ("ctrans|*", &C::ctrans, arg ("d"),
      "@brief Transforms a distance\n"
      "\n"
      "The \"ctrans\" method transforms the given distance.\n"
      "e = t(d). For the simple transformations, there\n"
      "is no magnification and no modification of the distance\n"
      "therefore.\n"
      "\n"
      "@param d The distance to transform\n"
      "@return The transformed distance\n"
      "\n"
      "The product '*' has been added as a synonym in version 0.28."
    ) +
    method ("trans|*", (point_type (C::*) (const point_type &) const) &C::trans, arg ("p"),
      "@brief Transforms a point\n"
      "\n"
      "The \"trans\" method or the * operator transforms the given point.\n"
      "q = t(p)\n"
      "\n"
      "The * operator has been introduced in version 0.25.\n"
      "\n"
      "@param p The point to transform\n"
      "@return The transformed point\n"
    ) +
    method ("trans|*", (vector_type (C::*) (const vector_type &) const) &C::trans, arg ("v"),
      "@brief Transforms a vector\n"
      "\n"
      "The \"trans\" method or the * operator transforms the given vector.\n"
      "w = t(v)\n"
      "\n"
      "Vector transformation has been introduced in version 0.25.\n"
      "\n"
      "@param v The vector to transform\n"
      "@return The transformed vector\n"
    ) +
    method_ext ("trans|*", &trans_box, arg ("box"),
      "@brief Transforms a box\n"
      "\n"
      "'t*box' or 't.trans(box)' is equivalent to box.transformed(t).\n"
      "\n"
      "@param box The box to transform\n"
      "@return The transformed box\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_edge, arg ("edge"),
      "@brief Transforms an edge\n"
      "\n"
      "'t*edge' or 't.trans(edge)' is equivalent to edge.transformed(t).\n"
      "\n"
      "@param edge The edge to transform\n"
      "@return The transformed edge\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_polygon, arg ("polygon"),
      "@brief Transforms a polygon\n"
      "\n"
      "'t*polygon' or 't.trans(polygon)' is equivalent to polygon.transformed(t).\n"
      "\n"
      "@param polygon The polygon to transform\n"
      "@return The transformed polygon\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_path, arg ("path"),
      "@brief Transforms a path\n"
      "\n"
      "'t*path' or 't.trans(path)' is equivalent to path.transformed(t).\n"
      "\n"
      "@param path The path to transform\n"
      "@return The transformed path\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_text, arg ("text"),
      "@brief Transforms a text\n"
      "\n"
      "'t*text' or 't.trans(text)' is equivalent to text.transformed(t).\n"
      "\n"
      "@param text The text to transform\n"
      "@return The transformed text\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method ("*!", &C::concat, arg ("t"),
      "@brief Returns the concatenated transformation\n"
      "\n"
      "The * operator returns self*t (\"t is applied before this transformation\").\n"
      "\n"
      "@param t The transformation to apply before\n"
      "@return The modified transformation\n"
    ) +
    method ("<", &C::less, arg ("other"),
      "@brief Provides a 'less' criterion for sorting\n"
      "This method is provided to implement a sorting order. The definition of 'less' is opaque and might change in "
      "future versions."
    ) +
    method ("==", &C::equal, arg ("other"),
      "@brief Tests for equality\n"
    ) +
    method ("!=", &C::not_equal, arg ("other"),
      "@brief Tests for inequality\n"
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given transformation. This method enables transformations as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    constructor ("from_s", &from_string, arg ("s"),
      "@brief Creates a transformation from a string\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
      "\n"
      "This method has been added in version 0.23.\n"
    ) +
    method ("to_s", &C::to_string, gsi::arg ("dbu", 0.0),
      "@brief String conversion\n"
      "If a DBU is given, the output units will be micrometers.\n"
      "\n"
      "The DBU argument has been added in version 0.27.6.\n"
    ) +
    method ("disp", (const vector_type &(C::*) () const) &C::disp,
      "@brief Gets to the displacement vector\n"
      "\n"
      "Staring with version 0.25 the displacement type is a vector."
    ) +
    method ("rot", &C::rot,
      "@brief Gets the angle/mirror code\n"
      "\n"
      "The angle/mirror code is one of the constants R0, R90, R180, R270, M0, M45, M90 and M135. "
      "rx is the rotation by an angle of x counter clockwise. mx is the mirroring at the axis given "
      "by the angle x (to the x-axis). "
    ) +
    method ("is_mirror?", &C::is_mirror,
      "@brief Gets the mirror flag\n"
      "\n"
      "If this property is true, the transformation is composed of a mirroring at the x-axis followed by a rotation "
      "by the angle given by the \\angle property. "
    ) +
    method ("angle", &C::angle,
      "@brief Gets the angle in units of 90 degree\n"
      "\n"
      "This value delivers the rotation component. In addition, a mirroring at the x axis may be applied before "
      "if the \\is_mirror? property is true. "
    ) +
    method_ext ("angle=", &set_angle, arg ("a"),
      "@brief Sets the angle in units of 90 degree\n"
      "@param a The new angle\n"
      "\n"
      "This method was introduced in version 0.20.\n"
    ) +
    method ("disp=", (void (C::*) (const vector_type &)) &C::disp, arg ("u"),
      "@brief Sets the displacement\n"
      "@param u The new displacement\n"
      "\n"
      "This method was introduced in version 0.20.\n"
      "Staring with version 0.25 the displacement type is a vector."
    ) +
    method_ext ("mirror=", &set_mirror, arg ("m"),
      "@brief Sets the mirror flag\n"
      "\"mirroring\" describes a reflection at the x-axis which is included in the transformation prior to rotation."
      "@param m The new mirror flag\n"
      "\n"
      "This method was introduced in version 0.20.\n"
    ) +
    method_ext ("rot=", &set_rot, arg ("r"),
      "@brief Sets the angle/mirror code\n"
      "@param r The new angle/rotation code (see \\rot property)\n"
      "\n"
      "This method was introduced in version 0.20.\n"
    ) +
    method ("R0", &trans_r0,
      "@brief A constant giving \"unrotated\" (unit) transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("R90", &trans_r90,
      "@brief A constant giving \"rotated by 90 degree counterclockwise\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("R180", &trans_r180,
      "@brief A constant giving \"rotated by 180 degree counterclockwise\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("R270", &trans_r270,
      "@brief A constant giving \"rotated by 270 degree counterclockwise\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M0", &trans_m0,
      "@brief A constant giving \"mirrored at the x-axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M45", &trans_m45,
      "@brief A constant giving \"mirrored at the 45 degree axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M90", &trans_m90,
      "@brief A constant giving \"mirrored at the y (90 degree) axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M135", &trans_m135,
      "@brief A constant giving \"mirrored at the 135 degree axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    );
  }
};

static db::Trans *trans_from_dtrans (const db::DTrans &t)
{
  return new db::Trans (t);
}

static db::DTrans trans_to_dtrans (const db::Trans *t, double dbu)
{
  db::DTrans f (*t);
  f.disp (f.disp () * dbu);
  return f;
}

Class<db::Trans> decl_Trans ("db", "Trans",
  constructor ("new|#from_dtrans", &trans_from_dtrans, gsi::arg ("dtrans"),
    "@brief Creates an integer coordinate transformation from a floating-point coordinate transformation\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dtrans'."
  ) +
  method_ext ("to_dtype", &trans_to_dtrans, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to a floating-point coordinate transformation\n"
    "\n"
    "The database unit can be specified to translate the integer-coordinate transformation into a floating-point coordinate "
    "transformation in micron units. The database unit is basically a scaling factor.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  trans_defs<db::Trans>::methods (),
  "@brief A simple transformation\n"
  "\n"
  "Simple transformations only provide rotations about angles which a multiples of 90 degree.\n"
  "Together with the mirror options, this results in 8 distinct orientations (fixpoint transformations).\n"
  "These can be combined with a displacement which is applied after the rotation/mirror.\n"
  "This version acts on integer coordinates. A version for floating-point coordinates is \\DTrans.\n"
  "\n"
  "Here are some examples for using the Trans class:\n"
  "\n"
  "@code\n"
  "t = RBA::Trans::new(0, 100)  # displacement by 100 DBU in y direction\n"
  "# the inverse: -> \"r0 0,-100\"\n"
  "t.inverted.to_s\n"
  "# concatenation: -> \"r90 -100,0\"\n"
  "(RBA::Trans::R90 * t).to_s\n"
  "# apply to a point: -> \"0,100\"\n"
  "RBA::Trans::R90.trans(RBA::Point::new(100, 0))\n"
  "@/code\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

static db::DTrans *dtrans_from_itrans (const db::Trans &t)
{
  return new db::DTrans (t);
}

static db::Trans dtrans_to_trans (const db::DTrans *t, double dbu)
{
  db::Trans f (*t);
  f.disp (db::Trans::displacement_type (t->disp () * (1.0 / dbu)));
  return f;
}

Class<db::DTrans> decl_DTrans ("db", "DTrans",
  constructor ("new|#from_itrans", &dtrans_from_itrans, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from an integer coordinate transformation\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_itrans'."
  ) +
  method_ext ("to_itype", &dtrans_to_trans, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to an integer coordinate transformation\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "transformation in micron units to an integer-coordinate transformation in database units. The transformation's' "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  trans_defs<db::DTrans>::methods (),
  "@brief A simple transformation\n"
  "\n"
  "Simple transformations only provide rotations about angles which a multiples of 90 degree.\n"
  "Together with the mirror options, this results in 8 distinct orientations (fixpoint transformations).\n"
  "These can be combined with a displacement which is applied after the rotation/mirror.\n"
  "This version acts on floating-point coordinates. A version for integer coordinates is \\Trans.\n"
  "\n"
  "Here are some examples for using the DTrans class:\n"
  "\n"
  "@code\n"
  "t = RBA::DTrans::new(0, 100)  # displacement by 100 DBU in y direction\n"
  "# the inverse: -> \"r0 0,-100\"\n"
  "t.inverted.to_s\n"
  "# concatenation: -> \"r90 -100,0\"\n"
  "(RBA::DTrans::new(RBA::DTrans::R90) * t).to_s\n"
  "# apply to a point: -> \"0,100\"\n"
  "RBA::DTrans::new(RBA::DTrans::R90).trans(RBA::DPoint::new(100, 0))\n"
  "@/code\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

// ---------------------------------------------------------------
//  complex_trans binding

template <class C>
struct cplx_trans_defs 
{
  typedef typename C::coord_type coord_type;
  typedef typename C::target_coord_type target_coord_type;
  typedef typename C::displacement_type displacement_type;
  typedef db::simple_trans<coord_type> simple_trans_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef db::point<target_coord_type> target_point_type;
  typedef db::vector<target_coord_type> target_vector_type;

  static C trans_r0 ()   { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r0));   }
  static C trans_r90 ()  { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r90));  }
  static C trans_r180 () { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r180)); }
  static C trans_r270 () { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::r270)); }
  static C trans_m0 ()   { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m0));   }
  static C trans_m45 ()  { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m45));  }
  static C trans_m90 ()  { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m90));  }
  static C trans_m135 () { return C (db::fixpoint_trans<coord_type> (db::fixpoint_trans<coord_type>::m135)); }

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

  static C *new_cmu (const C &c, double mag, const displacement_type &u)
  {
    return new C (C (u) * C (mag) * c);
  }

  static C *new_cmxy (const C &c, double mag, coord_type x, coord_type y)
  {
    return new C (C (displacement_type (x, y)) * C (mag) * c);
  }

  static C *new_xy (target_coord_type x, target_coord_type y)
  {
    return new C (displacement_type (x, y));
  }

  static C *new_u (const displacement_type &u)
  {
    return new C (u);
  }

  static C *new_t (const simple_trans_type &t)
  {
    return new C (t, 1.0, 1.0);
  }

  static C *new_tm (const simple_trans_type &t, double m)
  {
    return new C (t, 1.0, m);
  }

  static C *new_m (double m)
  {
    return new C (m);
  }

  static C *new_mrmu (double mag, double r, bool m, const displacement_type &u)
  {
    return new C (mag, r, m, u);
  }

  static C *new_mrmxy (double mag, double r, bool m, target_coord_type x, target_coord_type y)
  {
    return new C (mag, r, m, displacement_type (x, y));
  }

  static simple_trans_type s_trans (const C *cplx_trans)
  {
    return simple_trans_type (db::complex_trans<coord_type, coord_type> (*cplx_trans));
  }

  static db::edge<target_coord_type> trans_edge (const C *t, const db::edge<coord_type> &edge)
  {
    return edge.transformed (*t);
  }

  static db::box<target_coord_type> trans_box (const C *t, const db::box<coord_type> &box)
  {
    return box.transformed (*t);
  }

  static db::polygon<target_coord_type> trans_polygon (const C *t, const db::polygon<coord_type> &polygon)
  {
    return polygon.transformed (*t);
  }

  static db::path<target_coord_type> trans_path (const C *t, const db::path<coord_type> &path)
  {
    return path.transformed (*t);
  }

  static db::text<target_coord_type> trans_text (const C *t, const db::text<coord_type> &text)
  {
    return text.transformed (*t);
  }

  static size_t hash_value (const C *t)
  {
    return std::hfunc (*t);
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v, 
      "@brief Creates a unit transformation\n"
    ) +
    constructor ("new", &new_cmu, arg ("c"), arg ("m", 1.0), arg ("u", displacement_type ()),
      "@brief Creates a transformation from another transformation plus a magnification and displacement\n"
      "\n"
      "Creates a new transformation from a existing transformation. This constructor is provided for creating duplicates "
      "and backward compatibility since the constants are transformations now. It will copy the original transformation "
      "and add the given displacement.\n"
      "\n"
      "This variant has been introduced in version 0.25.\n"
      "\n"
      "@param c The original transformation\n"
      "@param u The Additional displacement\n"
    ) +
    constructor ("new", &new_cmxy, arg ("c"), arg ("m"), arg ("x"), arg ("y"),
      "@brief Creates a transformation from another transformation plus a magnification and displacement\n"
      "\n"
      "Creates a new transformation from a existing transformation. This constructor is provided for creating duplicates "
      "and backward compatibility since the constants are transformations now. It will copy the original transformation "
      "and add the given displacement.\n"
      "\n"
      "This variant has been introduced in version 0.25.\n"
      "\n"
      "@param c The original transformation\n"
      "@param x The Additional displacement (x)\n"
      "@param y The Additional displacement (y)\n"
    ) +
    constructor ("new", &new_xy, arg ("x"), arg ("y"),
      "@brief Creates a transformation from a x and y displacement\n"
      "\n"
      "This constructor will create a transformation with the specified displacement\n"
      "but no rotation.\n"
      "\n"
      "@param x The x displacement\n"
      "@param y The y displacement\n"
    ) +
    constructor ("new", &new_m, arg ("m"),
      "@brief Creates a transformation from a magnification\n"
      "\n"
      "Creates a magnifying transformation without displacement and rotation given the magnification m."
    ) +
    constructor ("new", &new_tm, arg ("t"), arg ("m"),
      "@brief Creates a transformation from a simple transformation and a magnification\n"
      "\n"
      "Creates a magnifying transformation from a simple transformation and a magnification."
    ) +
    constructor ("new", &new_t, arg ("t"),
      "@brief Creates a transformation from a simple transformation alone\n"
      "\n"
      "Creates a magnifying transformation from a simple transformation and a magnification of 1.0."
    ) +
    constructor ("new", &new_u, arg ("u"),
      "@brief Creates a transformation from a displacement\n"
      "\n"
      "Creates a transformation with a displacement only.\n"
      "\n"
      "This method has been added in version 0.25."
    ) +
    constructor ("new", &new_mrmu, arg ("mag"), arg ("rot"), arg ("mirrx"), arg ("u"),
      "@brief Creates a transformation using magnification, angle, mirror flag and displacement\n"
      "\n"
      "The sequence of operations is: magnification, mirroring at x axis,\n"
      "rotation, application of displacement.\n"
      "\n"
      "@param mag The magnification\n"
      "@param rot The rotation angle in units of degree\n"
      "@param mirrx True, if mirrored at x axis\n"
      "@param u The displacement\n"
    ) +
    constructor ("new", &new_mrmxy, arg ("mag"), arg ("rot"), arg ("mirrx"), arg ("x"), arg ("y"),
      "@brief Creates a transformation using magnification, angle, mirror flag and displacement\n"
      "\n"
      "The sequence of operations is: magnification, mirroring at x axis,\n"
      "rotation, application of displacement.\n"
      "\n"
      "@param mag The magnification\n"
      "@param rot The rotation angle in units of degree\n"
      "@param mirrx True, if mirrored at x axis\n"
      "@param x The x displacement\n"
      "@param y The y displacement\n"
    ) +
    method ("inverted", &C::inverted, 
      "@brief Returns the inverted transformation\n"
      "\n"
      "Returns the inverted transformation. This method does not modify the transformation.\n"
      "\n"
      "@return The inverted transformation\n"
    ) +
    method ("invert", &C::invert,
      "@brief Inverts the transformation (in place)\n"
      "\n"
      "Inverts the transformation and replaces this transformation by its\n"
      "inverted one.\n"
      "\n"
      "@return The inverted transformation\n"
    ) +
    method ("ctrans|*", &C::ctrans, arg ("d"),
      "@brief Transforms a distance\n"
      "\n"
      "The \"ctrans\" method transforms the given distance.\n"
      "e = t(d). For the simple transformations, there\n"
      "is no magnification and no modification of the distance\n"
      "therefore.\n"
      "\n"
      "@param d The distance to transform\n"
      "@return The transformed distance\n"
      "\n"
      "The product '*' has been added as a synonym in version 0.28."
    ) +
    method ("trans|*", (target_point_type (C::*) (const point_type &) const) &C::trans, arg ("p"),
      "@brief Transforms a point\n"
      "\n"
      "The \"trans\" method or the * operator transforms the given point.\n"
      "q = t(p)\n"
      "\n"
      "The * operator has been introduced in version 0.25.\n"
      "\n"
      "@param p The point to transform\n"
      "@return The transformed point\n"
    ) +
    method ("trans|*", (target_vector_type (C::*) (const vector_type &) const) &C::trans, arg ("p"),
      "@brief Transforms a vector\n"
      "\n"
      "The \"trans\" method or the * operator transforms the given vector.\n"
      "w = t(v)\n"
      "\n"
      "Vector transformation has been introduced in version 0.25.\n"
      "\n"
      "@param v The vector to transform\n"
      "@return The transformed vector\n"
    ) +
    method_ext ("trans|*", &trans_box, arg ("box"),
      "@brief Transforms a box\n"
      "\n"
      "'t*box' or 't.trans(box)' is equivalent to box.transformed(t).\n"
      "\n"
      "@param box The box to transform\n"
      "@return The transformed box\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_edge, arg ("edge"),
      "@brief Transforms an edge\n"
      "\n"
      "'t*edge' or 't.trans(edge)' is equivalent to edge.transformed(t).\n"
      "\n"
      "@param edge The edge to transform\n"
      "@return The transformed edge\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_polygon, arg ("polygon"),
      "@brief Transforms a polygon\n"
      "\n"
      "'t*polygon' or 't.trans(polygon)' is equivalent to polygon.transformed(t).\n"
      "\n"
      "@param polygon The polygon to transform\n"
      "@return The transformed polygon\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_path, arg ("path"),
      "@brief Transforms a path\n"
      "\n"
      "'t*path' or 't.trans(path)' is equivalent to path.transformed(t).\n"
      "\n"
      "@param path The path to transform\n"
      "@return The transformed path\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method_ext ("trans|*", &trans_text, arg ("text"),
      "@brief Transforms a text\n"
      "\n"
      "'t*text' or 't.trans(text)' is equivalent to text.transformed(t).\n"
      "\n"
      "@param text The text to transform\n"
      "@return The transformed text\n"
      "\n"
      "This convenience method has been introduced in version 0.25."
    ) +
    method ("*!", (C (C::*) (const C &c) const) &C::concat_same, arg ("t"),
      "@brief Returns the concatenated transformation\n"
      "\n"
      "The * operator returns self*t (\"t is applied before this transformation\").\n"
      "\n"
      "@param t The transformation to apply before\n"
      "@return The modified transformation\n"
    ) +
    method ("<", &C::less, arg ("other"),
      "@brief Provides a 'less' criterion for sorting\n"
      "This method is provided to implement a sorting order. The definition of 'less' is opaque and might change in "
      "future versions."
    ) +
    method ("==", &C::equal, arg ("other"),
      "@brief Tests for equality\n"
    ) +
    method ("!=", &C::not_equal, arg ("other"),
      "@brief Tests for inequality\n"
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given transformation. This method enables transformations as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    constructor ("from_s", &from_string, arg ("s"),
      "@brief Creates an object from a string\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
      "\n"
      "This method has been added in version 0.23.\n"
    ) +
    method ("to_s", &C::to_string, gsi::arg ("lazy", false), gsi::arg ("dbu", 0.0),
      "@brief String conversion\n"
      "If 'lazy' is true, some parts are omitted when not required.\n"
      "If a DBU is given, the output units will be micrometers.\n"
      "\n"
      "The lazy and DBU arguments have been added in version 0.27.6.\n"
    ) +
    method ("disp", (displacement_type (C::*)() const) &C::disp,
      "@brief Gets the displacement\n"
    ) +
    method ("disp=", (void (C::*) (const displacement_type &)) &C::disp, arg ("u"),
      "@brief Sets the displacement\n"
      "@param u The new displacement"
    ) +
    method ("rot", &C::rot,
      "@brief Returns the respective simple transformation equivalent rotation code if possible\n"
      "\n"
      "If this transformation is orthogonal (is_ortho () == true), then this method\n"
      "will return the corresponding fixpoint transformation, not taking into account\n"
      "magnification and displacement. If the transformation is not orthogonal, the result\n"
      "reflects the quadrant the rotation goes into.\n"
    ) +
    method ("is_mirror?", &C::is_mirror,
      "@brief Gets the mirror flag\n"
      "\n"
      "If this property is true, the transformation is composed of a mirroring at the x-axis followed by a rotation "
      "by the angle given by the \\angle property. "
    ) +
    method ("mirror=", &C::mirror, arg ("m"),
      "@brief Sets the mirror flag\n"
      "\"mirroring\" describes a reflection at the x-axis which is included in the transformation prior to rotation."
      "@param m The new mirror flag"
    ) +
    method ("is_unity?", &C::is_unity,
      "@brief Tests, whether this is a unit transformation\n"
    ) +
    method ("is_ortho?", &C::is_ortho,
      "@brief Tests, if the transformation is an orthogonal transformation\n"
      "\n"
      "If the rotation is by a multiple of 90 degree, this method will return true.\n"
    ) +
    method_ext ("s_trans", &s_trans,
      "@brief Extracts the simple transformation part\n"
      "\n"
      "The simple transformation part does not reflect magnification or arbitrary angles.\n"
      "Rotation angles are rounded down to multiples of 90 degree. Magnification is fixed to 1.0.\n"
    ) +
    method ("angle", (double (C::*) () const) &C::angle,
      "@brief Gets the angle\n"
      "\n"
      "Note that the simple transformation returns the angle in units of 90 degree. Hence for "
      "a simple trans (i.e. \\Trans), a rotation angle of 180 degree delivers a value of 2 for "
      "the angle attribute. The complex transformation, supporting any rotation angle returns "
      "the angle in degree.\n"
      "\n"
      "@return The rotation angle this transformation provides in degree units (0..360 deg).\n"
    ) +
    method ("angle=", (void (C::*) (double)) &C::angle, arg ("a"),
      "@brief Sets the angle\n"
      "@param a The new angle"
      "\n"
      "See \\angle for a description of that attribute.\n"
    ) +
    method ("mag", (double (C::*) () const) &C::mag,
      "@brief Gets the magnification\n"
    ) +
    method ("is_mag?", &C::is_mag,
      "@brief Tests, if the transformation is a magnifying one\n"
      "\n"
      "This is the recommended test for checking if the transformation represents\n"
      "a magnification.\n"
    ) +
    method ("mag=", (void (C::*) (double)) &C::mag, arg ("m"),
      "@brief Sets the magnification\n"
      "@param m The new magnification"
    ) +
    method ("is_complex?", &C::is_complex,
      "@brief Returns true if the transformation is a complex one\n"
      "\n"
      "If this predicate is false, the transformation can safely be converted to a simple transformation.\n"
      "Otherwise, this conversion will be lossy.\n"
      "The predicate value is equivalent to 'is_mag || !is_ortho'.\n"
      "\n"
      "This method has been introduced in version 0.27.5."
    ) +
    method ("R0", &trans_r0,
      "@brief A constant giving \"unrotated\" (unit) transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("R90", &trans_r90,
      "@brief A constant giving \"rotated by 90 degree counterclockwise\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("R180", &trans_r180,
      "@brief A constant giving \"rotated by 180 degree counterclockwise\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("R270", &trans_r270,
      "@brief A constant giving \"rotated by 270 degree counterclockwise\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M0", &trans_m0,
      "@brief A constant giving \"mirrored at the x-axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M45", &trans_m45,
      "@brief A constant giving \"mirrored at the 45 degree axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M90", &trans_m90,
      "@brief A constant giving \"mirrored at the y (90 degree) axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    ) +
    method ("M135", &trans_m135,
      "@brief A constant giving \"mirrored at the 135 degree axis\" transformation\n"
      "The previous integer constant has been turned into a transformation in version 0.25."
    );
  }
};

template <class F, class I>
static F *cplxtrans_from_cplxtrans (const I &t)
{
  return new F (t);
}

template <class F, class I>
static F cplxtrans_to_cplxtrans (const I *t)
{
  return F (*t);
}

template <class F, class I>
static F cplxtrans_to_icplxtrans (const I *t, double dbu)
{
  F f = F (*t);
  f.disp (typename F::displacement_type (f.disp () * (1.0 / dbu)));
  return f;
}

template <class F, class I>
static F cplxtrans_to_dcplxtrans (const I *t, double dbu)
{
  F f = F (*t);
  f.disp (f.disp () * dbu);
  return f;
}

Class<db::DCplxTrans> decl_DCplxTrans ("db", "DCplxTrans",
  constructor ("new|#from_itrans", &cplxtrans_from_cplxtrans<db::DCplxTrans, db::CplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_itrans'."
  ) +
  constructor ("new", &cplxtrans_from_cplxtrans<db::DCplxTrans, db::ICplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25."
  ) +
  constructor ("new", &cplxtrans_from_cplxtrans<db::DCplxTrans, db::VCplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25."
  ) +
  method_ext ("to_itrans", &cplxtrans_to_icplxtrans<db::ICplxTrans, db::DCplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with integer input and output coordinates\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "displacement in micron units to an integer-coordinate displacement in database units. The displacement's' "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_vtrans", &cplxtrans_to_icplxtrans<db::VCplxTrans, db::DCplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with integer output coordinates\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "displacement in micron units to an integer-coordinate displacement in database units. The displacement's' "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_trans", &cplxtrans_to_cplxtrans<db::CplxTrans, db::DCplxTrans>,
    "@brief Converts the transformation to another transformation with integer input coordinates\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("*!", (db::CplxTrans (db::DCplxTrans::*) (const db::CplxTrans &) const) &db::DCplxTrans::concat, gsi::arg ("t"),
    "@brief Multiplication (concatenation) of transformations\n"
    "\n"
    "The * operator returns self*t (\"t is applied before this transformation\").\n"
    "\n"
    "@param t The transformation to apply before\n"
    "@return The modified transformation\n"
  ) +
  cplx_trans_defs<db::DCplxTrans>::methods (),
  "@brief A complex transformation\n"
  "\n"
  "A complex transformation provides magnification, mirroring at the x-axis, rotation by an arbitrary\n"
  "angle and a displacement. This is also the order, the operations are applied.\n"
  "\n"
  "A complex transformation provides a superset of the simple transformation.\n"
  "In many applications, a complex transformation computes floating-point coordinates to minimize rounding effects.\n"
  "This version can transform floating-point coordinate objects.\n"
  "\n"
  "Complex transformations are extensions of the simple transformation classes (\\DTrans in that case) and behave similar.\n"
  "\n"
  "Transformations can be used to transform points or other objects. Transformations can be combined with the '*' operator "
  "to form the transformation which is equivalent to applying the second and then the first. Here is some code:\n"
  "\n"
  "@code\n"
  "# Create a transformation that applies a magnification of 1.5, a rotation by 90 degree\n"
  "# and displacement of 10 in x and 20 units in y direction:\n"
  "t = RBA::CplxTrans::new(1.5, 90, false, 10.0, 20.0)\n"
  "t.to_s            # r90 *1.5 10,20\n"
  "# compute the inverse:\n"
  "t.inverted.to_s   # r270 *0.666666667 -13,7\n"
  "# Combine with another displacement (applied after that):\n"
  "(RBA::CplxTrans::new(5, 5) * t).to_s    # r90 *1.5 15,25\n"
  "# Transform a point:\n"
  "t.trans(RBA::Point::new(100, 200)).to_s # -290,170\n"
  "@/code\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

Class<db::CplxTrans> decl_CplxTrans ("db", "CplxTrans",
  constructor ("new|#from_dtrans", &cplxtrans_from_cplxtrans<db::CplxTrans, db::DCplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dtrans'."
  ) +
  constructor ("new", &cplxtrans_from_cplxtrans<db::CplxTrans, db::ICplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25."
  ) +
  constructor ("new", &cplxtrans_from_cplxtrans<db::CplxTrans, db::VCplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25."
  ) +
  method_ext ("to_itrans", &cplxtrans_to_icplxtrans<db::ICplxTrans, db::CplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with integer input and output coordinates\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "displacement in micron units to an integer-coordinate displacement in database units. The displacement's' "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_vtrans", &cplxtrans_to_icplxtrans<db::VCplxTrans, db::CplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with integer output and floating-point input coordinates\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "displacement in micron units to an integer-coordinate displacement in database units. The displacement's' "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_trans", &cplxtrans_to_cplxtrans<db::DCplxTrans, db::CplxTrans>,
    "@brief Converts the transformation to another transformation with floating-point input coordinates\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("*!", (db::DCplxTrans (db::CplxTrans::*) (const db::VCplxTrans &) const) &db::CplxTrans::concat, gsi::arg ("t"),
    "@brief Multiplication (concatenation) of transformations\n"
    "\n"
    "The * operator returns self*t (\"t is applied before this transformation\").\n"
    "\n"
    "@param t The transformation to apply before\n"
    "@return The modified transformation\n"
  ) +
  method ("*!", (db::CplxTrans (db::CplxTrans::*) (const db::ICplxTrans &) const) &db::CplxTrans::concat, gsi::arg ("t"),
    "@brief Multiplication (concatenation) of transformations\n"
    "\n"
    "The * operator returns self*t (\"t is applied before this transformation\").\n"
    "\n"
    "@param t The transformation to apply before\n"
    "@return The modified transformation\n"
  ) +
  cplx_trans_defs<db::CplxTrans>::methods (),
  "@brief A complex transformation\n"
  "\n"
  "A complex transformation provides magnification, mirroring at the x-axis, rotation by an arbitrary\n"
  "angle and a displacement. This is also the order, the operations are applied.\n"
  "This version can transform integer-coordinate objects into floating-point coordinate objects. "
  "This is the generic and exact case, for example for non-integer magnifications.\n"
  "\n"
  "Complex transformations are extensions of the simple transformation classes (\\Trans or \\DTrans in that case) and behave similar.\n"
  "\n"
  "Transformations can be used to transform points or other objects. Transformations can be combined with the '*' operator "
  "to form the transformation which is equivalent to applying the second and then the first. Here is some code:\n"
  "\n"
  "@code\n"
  "# Create a transformation that applies a magnification of 1.5, a rotation by 90 degree\n"
  "# and displacement of 10 in x and 20 units in y direction:\n"
  "t = RBA::DCplxTrans::new(1.5, 90, false, 10.0, 20.0)\n"
  "t.to_s            # r90 *1.5 10,20\n"
  "# compute the inverse:\n"
  "t.inverted.to_s   # r270 *0.666666667 -13,7\n"
  "# Combine with another displacement (applied after that):\n"
  "(RBA::DCplxTrans::new(5, 5) * t).to_s    # r90 *1.5 15,25\n"
  "# Transform a point:\n"
  "t.trans(RBA::DPoint::new(100, 200)).to_s # -290,170\n"
  "@/code\n"
  "\n"
  "The inverse type of the CplxTrans type is VCplxTrans which will transform floating-point to integer coordinate objects. "
  "Transformations of CplxTrans type can be concatenated (operator *) with either itself or with transformations of compatible input or output type. "
  "This means, the operator CplxTrans * ICplxTrans is allowed (output types of ICplxTrans and input of CplxTrans are identical) while "
  "CplxTrans * DCplxTrans is not."
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

Class<db::ICplxTrans> decl_ICplxTrans ("db", "ICplxTrans",
  constructor ("new|#from_dtrans", &cplxtrans_from_cplxtrans<db::ICplxTrans, db::DCplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dtrans'."
  ) +
  constructor ("new|#from_trans", &cplxtrans_from_cplxtrans<db::ICplxTrans, db::CplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_trans'."
  ) +
  constructor ("new", &cplxtrans_from_cplxtrans<db::ICplxTrans, db::VCplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
    "\n"
    "This constructor has been introduced in version 0.25."
  ) +
  method_ext ("to_itrans", &cplxtrans_to_dcplxtrans<db::DCplxTrans, db::ICplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with floating-point input and output coordinates\n"
    "\n"
    "The database unit can be specified to translate the integer coordinate "
    "displacement in database units to a floating-point displacement in micron units. The displacement's' "
    "coordinates will be multiplied with the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_vtrans", &cplxtrans_to_dcplxtrans<db::CplxTrans, db::ICplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with floating-point output coordinates\n"
    "\n"
    "The database unit can be specified to translate the integer coordinate "
    "displacement in database units to a floating-point displacement in micron units. The displacement's' "
    "coordinates will be multiplied with the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_trans", &cplxtrans_to_cplxtrans<db::VCplxTrans, db::ICplxTrans>,
    "@brief Converts the transformation to another transformation with floating-point input coordinates\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("*!", (db::VCplxTrans (db::ICplxTrans::*) (const db::VCplxTrans &) const) &db::ICplxTrans::concat, gsi::arg ("t"),
    "@brief Multiplication (concatenation) of transformations\n"
    "\n"
    "The * operator returns self*t (\"t is applied before this transformation\").\n"
    "\n"
    "@param t The transformation to apply before\n"
    "@return The modified transformation\n"
  ) +
  cplx_trans_defs<db::ICplxTrans>::methods (),
  "@brief A complex transformation\n"
  "\n"
  "A complex transformation provides magnification, mirroring at the x-axis, rotation by an arbitrary\n"
  "angle and a displacement. This is also the order, the operations are applied.\n"
  "This version can transform integer-coordinate objects into the same, which may involve rounding and can be inexact.\n"
  "\n"
  "Complex transformations are extensions of the simple transformation classes (\\Trans in that case) and behave similar.\n"
  "\n"
  "Transformations can be used to transform points or other objects. Transformations can be combined with the '*' operator "
  "to form the transformation which is equivalent to applying the second and then the first. Here is some code:\n"
  "\n"
  "@code\n"
  "# Create a transformation that applies a magnification of 1.5, a rotation by 90 degree\n"
  "# and displacement of 10 in x and 20 units in y direction:\n"
  "t = RBA::ICplxTrans::new(1.5, 90, false, 10.0, 20.0)\n"
  "t.to_s            # r90 *1.5 10,20\n"
  "# compute the inverse:\n"
  "t.inverted.to_s   # r270 *0.666666667 -13,7\n"
  "# Combine with another displacement (applied after that):\n"
  "(RBA::ICplxTrans::new(5, 5) * t).to_s    # r90 *1.5 15,25\n"
  "# Transform a point:\n"
  "t.trans(RBA::Point::new(100, 200)).to_s  # -290,170\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.18.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

Class<db::VCplxTrans> decl_VCplxTrans ("db", "VCplxTrans",
  constructor ("new", &cplxtrans_from_cplxtrans<db::VCplxTrans, db::DCplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
  ) +
  constructor ("new", &cplxtrans_from_cplxtrans<db::VCplxTrans, db::CplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
  ) +
  constructor ("new", &cplxtrans_from_cplxtrans<db::VCplxTrans, db::ICplxTrans>, gsi::arg ("trans"),
    "@brief Creates a floating-point coordinate transformation from another coordinate flavour\n"
  ) +
  method_ext ("to_itrans", &cplxtrans_to_dcplxtrans<db::DCplxTrans, db::VCplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with floating-point output coordinates\n"
    "\n"
    "The database unit can be specified to translate the integer coordinate "
    "displacement in database units to a floating-point displacement in micron units. The displacement's' "
    "coordinates will be multiplied with the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_vtrans", &cplxtrans_to_dcplxtrans<db::CplxTrans, db::VCplxTrans>, gsi::arg ("dbu", 1.0),
    "@brief Converts the transformation to another transformation with integer input and floating-point output coordinates\n"
    "\n"
    "The database unit can be specified to translate the integer coordinate "
    "displacement in database units to an floating-point displacement in micron units. The displacement's' "
    "coordinates will be multiplied with the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("to_trans", &cplxtrans_to_cplxtrans<db::ICplxTrans, db::VCplxTrans>,
    "@brief Converts the transformation to another transformation with integer input coordinates\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("*!", (db::VCplxTrans (db::VCplxTrans::*) (const db::DCplxTrans &) const) &db::VCplxTrans::concat, gsi::arg ("t"),
    "@brief Multiplication (concatenation) of transformations\n"
    "\n"
    "The * operator returns self*t (\"t is applied before this transformation\").\n"
    "\n"
    "@param t The transformation to apply before\n"
    "@return The modified transformation\n"
  ) +
  method ("*!", (db::ICplxTrans (db::VCplxTrans::*) (const db::CplxTrans &) const) &db::VCplxTrans::concat, gsi::arg ("t"),
    "@brief Multiplication (concatenation) of transformations\n"
    "\n"
    "The * operator returns self*t (\"t is applied before this transformation\").\n"
    "\n"
    "@param t The transformation to apply before\n"
    "@return The modified transformation\n"
  ) +
  cplx_trans_defs<db::VCplxTrans>::methods (),
  "@brief A complex transformation\n"
  "\n"
  "A complex transformation provides magnification, mirroring at the x-axis, rotation by an arbitrary\n"
  "angle and a displacement. This is also the order, the operations are applied.\n"
  "This version can transform floating point coordinate objects into integer coordinate objects, which may involve rounding and can be inexact.\n"
  "\n"
  "Complex transformations are extensions of the simple transformation classes (\\Trans in that case) and behave similar.\n"
  "\n"
  "Transformations can be used to transform points or other objects. Transformations can be combined with the '*' operator "
  "to form the transformation which is equivalent to applying the second and then the first. Here is some code:\n"
  "\n"
  "@code\n"
  "# Create a transformation that applies a magnification of 1.5, a rotation by 90 degree\n"
  "# and displacement of 10 in x and 20 units in y direction:\n"
  "t = RBA::VCplxTrans::new(1.5, 90, false, 10, 20)\n"
  "t.to_s            # r90 *1.5 10,20\n"
  "# compute the inverse:\n"
  "t.inverted.to_s   # r270 *0.666666667 -13,7\n"
  "# Combine with another displacement (applied after that):\n"
  "(RBA::VCplxTrans::new(5, 5) * t).to_s     # r90 *1.5 15,25\n"
  "# Transform a point:\n"
  "t.trans(RBA::DPoint::new(100, 200)).to_s  # -290,170\n"
  "@/code\n"
  "\n"
  "The VCplxTrans type is the inverse transformation of the CplxTrans transformation and vice versa."
  "Transformations of VCplxTrans type can be concatenated (operator *) with either itself or with transformations of compatible input or output type. "
  "This means, the operator VCplxTrans * CplxTrans is allowed (output types of CplxTrans and input of VCplxTrans are identical) while "
  "VCplxTrans * ICplxTrans is not."
  "\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

}
