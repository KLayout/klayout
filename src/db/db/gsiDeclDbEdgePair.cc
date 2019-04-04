
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbEdgePair.h"
#include "dbHash.h"

namespace gsi
{

template <class C>
struct edge_pair_defs
{
  typedef typename C::coord_type coord_type;
  typedef typename C::box_type box_type;
  typedef typename C::point_type point_type;
  typedef typename C::distance_type distance_type;
  typedef typename C::area_type area_type;
  typedef db::edge<coord_type> edge_type;
  typedef db::simple_trans<coord_type> simple_trans_type;
  typedef db::complex_trans<coord_type, double> complex_trans_type;

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

  static C *new_ee (const edge_type &first, const edge_type &second) 
  {
    return new C (first, second);
  }

  static size_t hash_value (const C *ep)
  {
    return std::hfunc (*ep);
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v, 
      "@brief Default constructor\n"
      "\n"
      "This constructor creates an default edge pair.\n"
    ) + 
    constructor ("new", &new_ee, 
      "@brief Constructor from two edges\n"
      "\n"
      "This constructor creates an edge pair from the two edges given.\n"
    ) + 
    method ("first", (const edge_type &(C::*) () const) &C::first, 
      "@brief Gets the first edge\n"
    ) + 
    method ("first=", &C::set_first, 
      "@brief Sets the first edge\n"
      "@args edge\n"
    ) + 
    method ("second", (const edge_type &(C::*) () const) &C::second, 
      "@brief Gets the second edge\n"
    ) + 
    method ("second=", &C::set_second, 
      "@brief Sets the second edge\n"
      "@args edge\n"
    ) + 
    method ("normalized", &C::normalized, 
      "@brief Normalizes the edge pair\n"
      "This method normalized the edge pair such that when connecting the edges at their \n"
      "start and end points a closed loop is formed which is oriented clockwise. To "
      "achieve this, the points of the first and/or first and second edge are swapped. "
      "Normalization is a first step recommended before converting an edge pair to a polygon, "
      "because that way the polygons won't be self-overlapping and the enlargement parameter "
      "is applied properly."
    ) + 
    method ("polygon", &C::to_polygon, 
      "@brief Convert an edge pair to a polygon\n"
      "@args e The enlargement (set to zero for exact representation)\n"
      "The polygon is formed by connecting the end and start points of the edges. It is recommended to "
      "use \\normalized before converting the edge pair to a polygon.\n"
      "\n"
      "The enlargement parameter applies the specified enlargement parallel and perpendicular to the "
      "edges. Basically this introduces a bias which blows up edge pairs by the specified amount. That parameter "
      "is useful to convert degenerated edge pairs to valid polygons, i.e. edge pairs with coincident edges and "
      "edge pairs consisting of two point-like edges.\n"
      "\n"
      "Another version for converting edge pairs to simple polygons is \\simple_polygon which renders a \\SimplePolygon object."
    ) + 
    method ("simple_polygon", &C::to_simple_polygon, 
      "@brief Convert an edge pair to a simple polygon\n"
      "@args e The enlargement (set to zero for exact representation)\n"
      "The polygon is formed by connecting the end and start points of the edges. It is recommended to "
      "use \\normalized before converting the edge pair to a polygon.\n"
      "\n"
      "The enlargement parameter applies the specified enlargement parallel and perpendicular to the "
      "edges. Basically this introduces a bias which blows up edge pairs by the specified amount. That parameter "
      "is useful to convert degenerated edge pairs to valid polygons, i.e. edge pairs with coincident edges and "
      "edge pairs consisting of two point-like edges.\n"
      "\n"
      "Another version for converting edge pairs to polygons is \\polygon which renders a \\Polygon object."
    ) + 
    constructor ("from_s", &from_string,
      "@brief Creates an object from a string\n"
      "@args s\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
      "\n"
      "This method has been added in version 0.23.\n"
    ) +
    method ("to_s", (std::string (C::*) () const) &C::to_string, 
      "@brief Returns a string representing the edge pair\n"
    ) +
    method ("bbox", &C::bbox, 
      "@brief Gets the bounding box of the edge pair\n"
    ) +
    method ("<", &C::less,
      "@brief Less operator\n"
      "@args box\n"
      "Returns true, if this edge pair is 'less' with respect to first and second edge\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    method ("==", &C::equal,
      "@brief Equality\n"
      "@args box\n"
      "Returns true, if this edge pair and the given one are equal\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    method ("!=", &C::not_equal,
      "@brief Inequality\n"
      "@args box\n"
      "Returns true, if this edge pair and the given one are not equal\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given edge pair. This method enables edge pairs as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    method ("transformed", &C::template transformed<simple_trans_type>,
      "@brief Returns the transformed pair\n"
      "@args t\n"
      "\n"
      "Transforms the edge pair with the given transformation.\n"
      "Does not modify the edge pair but returns the transformed edge.\n"
      "\n"
      "@param t The transformation to apply.\n"
      "\n"
      "@return The transformed edge pair\n"
    ) +
    method ("transformed", &C::template transformed<complex_trans_type>,
      "@brief Returns the transformed edge pair\n"
      "@args t\n"
      "\n"
      "Transforms the edge pair with the given complex transformation.\n"
      "Does not modify the edge pair but returns the transformed edge.\n"
      "\n"
      "@param t The transformation to apply.\n"
      "\n"
      "@return The transformed edge pair\n"
    );
  }
};

static db::EdgePair *edge_pair_from_dedge_pair (const db::DEdgePair &e)
{
  return new db::EdgePair (e);
}

static db::DEdgePair edge_pair_to_dedge_pair (const db::EdgePair *e, double dbu)
{
  return db::DEdgePair (*e * dbu);
}

Class<db::EdgePair> decl_EdgePair ("db", "EdgePair",
  constructor ("new", &edge_pair_from_dedge_pair, gsi::arg ("dedge_pair"),
    "@brief Creates an integer coordinate edge pair from a floating-point coordinate edge pair\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dedge_pair'."
  ) +
  method_ext ("to_dtype", &edge_pair_to_dedge_pair, gsi::arg ("dbu", 1.0),
    "@brief Converts the edge pair to a floating-point coordinate edge pair\n"
    "\n"
    "The database unit can be specified to translate the integer-coordinate edge pair into a floating-point coordinate "
    "edge pair in micron units. The database unit is basically a scaling factor.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::EdgePair::transformed<db::ICplxTrans>,
    "@brief Returns the transformed edge pair\n"
    "@args t\n"
    "\n"
    "Transforms the edge pair with the given complex transformation.\n"
    "Does not modify the edge pair but returns the transformed edge.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pair (in this case an integer coordinate edge pair).\n"
  ) +
  edge_pair_defs<db::EdgePair>::methods (),
  "@brief An edge pair (a pair of two edges)\n"
  "Edge pairs are objects representing two edges or parts of edges. They play a role mainly in the context "
  "of DRC functions, where they specify a DRC violation by connecting two edges which violate the condition checked. "
  "Within the framework of polygon and edge collections which provide DRC functionality, edges pairs are used in the form "
  "of edge pair collections (\\EdgePairs).\n"
  "\n"
  "Edge pairs basically consist of two edges, called first and second. If created by a two-layer DRC "
  "function, the first edge will correspond to edges from the first layer and the second to edges from the "
  "second layer.\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

static db::DEdgePair *dedge_pair_from_iedge_pair (const db::EdgePair &e)
{
  return new db::DEdgePair (e);
}

static db::EdgePair dedge_pair_to_edge_pair (const db::DEdgePair *e, double dbu)
{
  return db::EdgePair (*e * (1.0 / dbu));
}

Class<db::DEdgePair> decl_DEdgePair ("db", "DEdgePair",
  constructor ("new", &dedge_pair_from_iedge_pair, gsi::arg ("edge_pair"),
    "@brief Creates a floating-point coordinate edge pair from an integer coordinate edge\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_iedge_pair'."
  ) +
  method_ext ("to_itype", &dedge_pair_to_edge_pair, gsi::arg ("dbu", 1.0),
    "@brief Converts the edge pair to an integer coordinate edge pair\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "edge pair in micron units to an integer-coordinate edge pair in database units. The edge pair's' "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::DEdgePair::transformed<db::VCplxTrans>,
    "@brief Transforms the edge pair with the given complex transformation\n"
    "\n"
    "@args t\n"
    "\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed edge pair (in this case an integer coordinate edge pair)\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  edge_pair_defs<db::DEdgePair>::methods (),
  "@brief An edge pair (a pair of two edges)\n"
  "Edge pairs are objects representing two edges or parts of edges. They play a role mainly in the context "
  "of DRC functions, where they specify a DRC violation by connecting two edges which violate the condition checked. "
  "Within the framework of polygon and edge collections which provide DRC functionality, edges pairs with integer coordinates (\\EdgePair type) are used in the form "
  "of edge pair collections (\\EdgePairs).\n"
  "\n"
  "Edge pairs basically consist of two edges, called first and second. If created by a two-layer DRC "
  "function, the first edge will correspond to edges from the first layer and the second to edges from the "
  "second layer.\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

}

