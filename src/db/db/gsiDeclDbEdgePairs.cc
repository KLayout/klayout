
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

#include "dbEdgePairs.h"
#include "dbEdges.h"
#include "dbRegion.h"
#include "dbDeepEdgePairs.h"

namespace gsi
{

static db::EdgePairs *new_v () 
{
  return new db::EdgePairs ();
}

static db::EdgePairs *new_a (const std::vector<db::EdgePair> &pairs)
{
  return new db::EdgePairs (pairs.begin (), pairs.end ());
}

static db::EdgePairs *new_ep (const db::EdgePair &pair)
{
  return new db::EdgePairs (pair);
}

static db::EdgePairs *new_shapes (const db::Shapes &s)
{
  db::EdgePairs *r = new db::EdgePairs ();
  for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::EdgePairs); !i.at_end (); ++i) {
    r->insert (*i);
  }
  return r;
}

static db::EdgePairs *new_si (const db::RecursiveShapeIterator &si)
{
  return new db::EdgePairs (si);
}

static db::EdgePairs *new_si2 (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  return new db::EdgePairs (si, trans);
}

static db::EdgePairs *new_sid (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss)
{
  return new db::EdgePairs (si, dss);
}

static db::EdgePairs *new_si2d (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const db::ICplxTrans &trans)
{
  return new db::EdgePairs (si, dss, trans);
}

static std::string to_string0 (const db::EdgePairs *r)
{
  return r->to_string ();
}

static std::string to_string1 (const db::EdgePairs *r, size_t n)
{
  return r->to_string (n);
}

static db::EdgePairs &move_p (db::EdgePairs *r, const db::Vector &p)
{
  r->transform (db::Disp (p));
  return *r;
}

static db::EdgePairs &move_xy (db::EdgePairs *r, db::Coord x, db::Coord y)
{
  r->transform (db::Disp (db::Vector (x, y)));
  return *r;
}

static db::EdgePairs moved_p (const db::EdgePairs *r, const db::Vector &p)
{
  return r->transformed (db::Disp (p));
}

static db::EdgePairs moved_xy (const db::EdgePairs *r, db::Coord x, db::Coord y)
{
  return r->transformed (db::Disp (db::Vector (x, y)));
}

static db::Region polygons1 (const db::EdgePairs *e)
{
  db::Region r;
  e->polygons (r);
  return r;
}

static db::Region polygons2 (const db::EdgePairs *e, db::Coord d)
{
  db::Region r;
  e->polygons (r, d);
  return r;
}

static db::Region extents2 (const db::EdgePairs *r, db::Coord dx, db::Coord dy)
{
  db::Region e;
  e.reserve (r->size ());
  for (db::EdgePairs::const_iterator i = r->begin (); ! i.at_end (); ++i) {
    e.insert (i->bbox ().enlarged (db::Vector (dx, dy)));
  }
  return e;
}

static db::Region extents1 (const db::EdgePairs *r, db::Coord d)
{
  return extents2 (r, d, d);
}

static db::Region extents0 (const db::EdgePairs *r)
{
  return extents2 (r, 0, 0);
}

static db::Edges edges (const db::EdgePairs *ep)
{
  db::Edges e;
  ep->edges (e);
  return e;
}

static db::Edges first_edges (const db::EdgePairs *ep)
{
  db::Edges e;
  ep->first_edges (e);
  return e;
}

static db::Edges second_edges (const db::EdgePairs *ep)
{
  db::Edges e;
  ep->second_edges (e);
  return e;
}

static void insert_e (db::EdgePairs *e, const db::EdgePairs &a)
{
  for (db::EdgePairs::const_iterator p = a.begin (); ! p.at_end (); ++p) {
    e->insert (*p);
  }
}

static bool is_deep (const db::EdgePairs *ep)
{
  return dynamic_cast<const db::DeepEdgePairs *> (ep->delegate ()) != 0;
}

static size_t id (const db::EdgePairs *ep)
{
  return tl::id_of (ep->delegate ());
}

Class<db::EdgePairs> decl_EdgePairs ("db", "EdgePairs",
  constructor ("new", &new_v, 
    "@brief Default constructor\n"
    "\n"
    "This constructor creates an empty edge pair collection.\n"
  ) + 
  constructor ("new", &new_a,
    "@brief Constructor from an edge pair array\n"
    "@args array\n"
    "\n"
    "This constructor creates an edge pair collection from an array of \\EdgePair objects.\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_ep,
    "@brief Constructor from a single edge pair object\n"
    "@args edge_pair\n"
    "\n"
    "This constructor creates an edge pair collection with a single edge pair.\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_shapes,
    "@brief Shapes constructor\n"
    "@args shapes\n"
    "\n"
    "This constructor creates an edge pair collection from a \\Shapes collection.\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_si,
    "@brief Constructor from a hierarchical shape set\n"
    "@args shape_iterator\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only edge pairs are taken from the shape set and other shapes are ignored.\n"
    "This method allows feeding the edge pair collection from a hierarchy of cells.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_si2,
    "@brief Constructor from a hierarchical shape set with a transformation\n"
    "@args shape_iterator, trans\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only edge pairs are taken from the shape set and other shapes are ignored.\n"
    "The given transformation is applied to each edge pair taken.\n"
    "This method allows feeding the edge pair collection from a hierarchy of cells.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_sid, gsi::arg ("shape_iterator"), gsi::arg ("dss"),
    "@brief Creates a hierarchical edge pair collection from an original layer\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical edge pair collection which supports hierarchical operations.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  constructor ("new", &new_si2d, gsi::arg ("shape_iterator"), gsi::arg ("dss"), gsi::arg ("trans"),
    "@brief Creates a hierarchical edge pair collection from an original layer with a transformation\n"
    "\n"
    "This constructor creates an edge pair collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical edge pair collection which supports hierarchical operations.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "Edge pairs in layout objects are somewhat special as most formats don't support reading "
    "or writing of edge pairs. Still they are useful objects and can be created and manipulated inside layouts.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::EdgePairs::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
    "\n"
    "This constructor has been introduced in version 0.26."
  ) +
  method ("insert_into", &db::EdgePairs::insert_into, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Inserts this edge pairs into the given layout, below the given cell and into the given layer.\n"
    "If the edge pair collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("insert_into_as_polygons", &db::EdgePairs::insert_into_as_polygons, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("e"),
    "@brief Inserts this edge pairs into the given layout, below the given cell and into the given layer.\n"
    "If the edge pair collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "The edge pairs will be converted to polygons with the enlargement value given be 'e'.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("insert", (void (db::EdgePairs::*) (const db::Edge &, const db::Edge &)) &db::EdgePairs::insert,
    "@brief Inserts an edge pair into the collection\n"
    "@args first, second\n"
  ) +
  method ("insert", (void (db::EdgePairs::*) (const db::EdgePair &)) &db::EdgePairs::insert,
    "@brief Inserts an edge pair into the collection\n"
    "@args edge_pair\n"
  ) +
  method_ext ("is_deep?", &is_deep,
    "@brief Returns true if the edge pair collection is a deep (hierarchical) one\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method_ext ("data_id", &id,
    "@brief Returns the data ID (a unique identifier for the underlying data storage)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method ("+", &db::EdgePairs::operator+,
    "@brief Returns the combined edge pair collection of self and the other one\n"
    "\n"
    "@args other\n"
    "@return The resulting edge pair collection\n"
    "\n"
    "This operator adds the edge pairs of the other collection to self and returns a new combined set.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) + 
  method ("+=", &db::EdgePairs::operator+=,
    "@brief Adds the edge pairs of the other edge pair collection to self\n"
    "\n"
    "@args other\n"
    "@return The edge pair collection after modification (self)\n"
    "\n"
    "This operator adds the edge pairs of the other collection to self.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) + 
  method_ext ("move", &move_p,
    "@brief Moves the edge pair collection\n"
    "@args p\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pair collection. The edge pair collection is overwritten.\n"
    "\n"
    "@param p The distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs (self).\n"
    "\n"
    "Starting with version 0.25 the displacement is of vector type."
  ) +
  method_ext ("move", &move_xy,
    "@brief Moves the edge pair collection\n"
    "@args x,y\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pairs. The edge pair collection is overwritten.\n"
    "\n"
    "@param x The x distance to move the edge pairs.\n"
    "@param y The y distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs (self).\n"
  ) +
  method_ext ("moved", &moved_p,
    "@brief Returns the moved edge pair collection (does not modify self)\n"
    "@args p\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pairs. The edge pair collection is not modified.\n"
    "\n"
    "@param p The distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs.\n"
    "\n"
    "Starting with version 0.25 the displacement is of vector type."
  ) +
  method_ext ("moved", &moved_xy,
    "@brief Returns the moved edge pair collection (does not modify self)\n"
    "@args x,y\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved edge pairs. The edge pair collection is not modified.\n"
    "\n"
    "@param x The x distance to move the edge pairs.\n"
    "@param y The y distance to move the edge pairs.\n"
    "\n"
    "@return The moved edge pairs.\n"
  ) +
  method ("transformed", (db::EdgePairs (db::EdgePairs::*)(const db::Trans &) const) &db::EdgePairs::transformed,
    "@brief Transform the edge pair collection\n"
    "@args t\n"
    "\n"
    "Transforms the edge pairs with the given transformation.\n"
    "Does not modify the edge pair collection but returns the transformed edge pairs.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pairs.\n"
  ) +
  method ("transformed|#transformed_icplx", (db::EdgePairs (db::EdgePairs::*)(const db::ICplxTrans &) const) &db::EdgePairs::transformed,
    "@brief Transform the edge pair collection with a complex transformation\n"
    "@args t\n"
    "\n"
    "Transforms the edge pairs with the given complex transformation.\n"
    "Does not modify the edge pair collection but returns the transformed edge pairs.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pairs.\n"
  ) +
  method ("transform", (db::EdgePairs &(db::EdgePairs::*)(const db::Trans &)) &db::EdgePairs::transform,
    "@brief Transform the edge pair collection (modifies self)\n"
    "@args t\n"
    "\n"
    "Transforms the edge pair collection with the given transformation.\n"
    "This version modifies the edge pair collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pair collection.\n"
  ) +
  method ("transform|#transform_icplx", (db::EdgePairs &(db::EdgePairs::*)(const db::ICplxTrans &)) &db::EdgePairs::transform,
    "@brief Transform the edge pair collection with a complex transformation (modifies self)\n"
    "@args t\n"
    "\n"
    "Transforms the edge pair collection with the given transformation.\n"
    "This version modifies the edge pair collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge pair collection.\n"
  ) +
  method_ext ("insert", &insert_e,
    "@brief Inserts all edge pairs from the other edge pair collection into this edge pair collection\n"
    "@args edge_pairs\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("edges", &edges,
    "@brief Decomposes the edge pairs into single edges\n"
    "@return An edge collection containing the individual edges\n"
  ) +
  method_ext ("first_edges", &first_edges,
    "@brief Returns the first one of all edges\n"
    "@return An edge collection containing the first edges\n"
  ) +
  method_ext ("second_edges", &second_edges,
    "@brief Returns the second one of all edges\n"
    "@return An edge collection containing the second edges\n"
  ) +
  method_ext ("extents", &extents0,
    "@brief Returns a region with the bounding boxes of the edge pairs\n"
    "This method will return a region consisting of the bounding boxes of the edge pairs.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents1,
    "@brief Returns a region with the enlarged bounding boxes of the edge pairs\n"
    "@args d\n"
    "This method will return a region consisting of the bounding boxes of the edge pairs enlarged by the given distance d.\n"
    "The enlargement is specified per edge, i.e the width and height will be increased by 2*d.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents2,
    "@brief Returns a region with the enlarged bounding boxes of the edge pairs\n"
    "@args dx, dy\n"
    "This method will return a region consisting of the bounding boxes of the edge pairs enlarged by the given distance dx in x direction and dy in y direction.\n"
    "The enlargement is specified per edge, i.e the width will be increased by 2*dx.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("polygons", &polygons1,
    "@brief Converts the edge pairs to polygons\n"
    "This method creates polygons from the edge pairs. Each polygon will be a triangle or quadrangle "
    "which connects the start and end points of the edges forming the edge pair."
  ) +
  method_ext ("polygons", &polygons2,
    "@brief Converts the edge pairs to polygons\n"
    "@args e\n"
    "This method creates polygons from the edge pairs. Each polygon will be a triangle or quadrangle "
    "which connects the start and end points of the edges forming the edge pair. "
    "This version allows one to specify an enlargement which is applied to the edges. The length of the edges is "
    "modified by applying the enlargement and the edges are shifted by the enlargement. By specifying an "
    "enlargement it is possible to give edge pairs an area which otherwise would not have one (coincident edges, "
    "two point-like edges)."
  ) +
  method ("clear", &db::EdgePairs::clear,
    "@brief Clears the edge pair collection\n"
  ) +
  method ("swap", &db::EdgePairs::swap,
    "@brief Swap the contents of this collection with the contents of another collection\n"
    "@args other\n"
    "This method is useful to avoid excessive memory allocation in some cases. "
    "For managed memory languages such as Ruby, those cases will be rare. " 
  ) +
  method ("bbox", &db::EdgePairs::bbox,
    "@brief Return the bounding box of the edge pair collection\n"
    "The bounding box is the box enclosing all points of all edge pairs.\n"
  ) +
  method ("is_empty?", &db::EdgePairs::empty,
    "@brief Returns true if the collection is empty\n"
  ) +
  method ("size", &db::EdgePairs::size,
    "@brief Returns the number of edge pairs in this collection\n"
  ) +
  gsi::iterator ("each", &db::EdgePairs::begin,
    "@brief Returns each edge pair of the edge pair collection\n"
  ) +
  method ("[]", &db::EdgePairs::nth,
    "@brief Returns the nth edge pair\n"
    "@args n\n"
    "\n"
    "This method returns nil if the index is out of range. It is available for flat edge pairs only - i.e. "
    "those for which \\has_valid_edge_pairs? is true. Use \\flatten to explicitly flatten an edge pair collection.\n"
    "\n"
    "The \\each iterator is the more general approach to access the edge pairs."
  ) +
  method ("flatten", &db::EdgePairs::flatten,
    "@brief Explicitly flattens an edge pair collection\n"
    "\n"
    "If the collection is already flat (i.e. \\has_valid_edge_pairs? returns true), this method will "
    "not change the collection.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("has_valid_edge_pairs?", &db::EdgePairs::has_valid_edge_pairs,
    "@brief Returns true if the edge pair collection is flat and individual edge pairs can be accessed randomly\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("enable_progress", &db::EdgePairs::enable_progress,
    "@brief Enable progress reporting\n"
    "@args label\n"
    "After calling this method, the edge pair collection will report the progress through a progress bar while "
    "expensive operations are running.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
  ) +
  method ("disable_progress", &db::EdgePairs::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will disable progress reporting. See \\enable_progress.\n"
  ) +
  method_ext ("to_s", &to_string0,
    "@brief Converts the edge pair collection to a string\n"
    "The length of the output is limited to 20 edge pairs to avoid giant strings on large regions. "
    "For full output use \"to_s\" with a maximum count parameter.\n"
  ) +
  method_ext ("to_s", &to_string1,
    "@brief Converts the edge pair collection to a string\n"
    "@args max_count\n"
    "This version allows specification of the maximum number of edge pairs contained in the string."
  ),
  "@brief EdgePairs (a collection of edge pairs)\n"
  "\n"
  "Edge pairs are used mainly in the context of the DRC functions (width_check, space_check etc.) of \\Region and \\Edges. "
  "A single edge pair represents two edges participating in a DRC violation. In the two-layer checks (inside, overlap) "
  "The first edge represents an edge from the first layer and the second edge an edge from the second layer. "
  "For single-layer checks (width, space) the order of the edges is arbitrary.\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

}

