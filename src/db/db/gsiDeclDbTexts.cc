
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

#include "dbTexts.h"
#include "dbRegion.h"
#include "dbDeepTexts.h"
#include "dbTextsUtils.h"

#include "gsiDeclDbContainerHelpers.h"

namespace gsi
{

static db::Texts *new_v ()
{
  return new db::Texts ();
}

static db::Texts *new_a (const std::vector<db::Text> &t)
{
  return new db::Texts (t.begin (), t.end ());
}

static db::Texts *new_text (const db::Text &t)
{
  return new db::Texts (t);
}

static db::Texts *new_shapes (const db::Shapes &s)
{
  db::Texts *r = new db::Texts ();
  for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::Texts); !i.at_end (); ++i) {
    r->insert (*i);
  }
  return r;
}

static db::Texts *new_si (const db::RecursiveShapeIterator &si)
{
  return new db::Texts (si);
}

static db::Texts *new_si2 (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  return new db::Texts (si, trans);
}

static db::Texts *new_sid (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss)
{
  return new db::Texts (si, dss);
}

static db::Texts *new_si2d (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const db::ICplxTrans &trans)
{
  return new db::Texts (si, dss, trans);
}

static std::string to_string0 (const db::Texts *r)
{
  return r->to_string ();
}

static std::string to_string1 (const db::Texts *r, size_t n)
{
  return r->to_string (n);
}

static db::Texts &move_p (db::Texts *r, const db::Vector &p)
{
  r->transform (db::Disp (p));
  return *r;
}

static db::Texts &move_xy (db::Texts *r, db::Coord x, db::Coord y)
{
  r->transform (db::Disp (db::Vector (x, y)));
  return *r;
}

static db::Texts moved_p (const db::Texts *r, const db::Vector &p)
{
  return r->transformed (db::Disp (p));
}

static db::Texts moved_xy (const db::Texts *r, db::Coord x, db::Coord y)
{
  return r->transformed (db::Disp (db::Vector (x, y)));
}

static db::Region polygons0 (const db::Texts *e, db::Coord d)
{
  db::Region r;
  e->polygons (r, d);
  return r;
}

static db::Region extents1 (const db::Texts *r, db::Coord dx, db::Coord dy)
{
  db::Region output;
  r->processed (output, db::extents_processor<db::Text> (dx, dy));
  return output;
}

static db::Region extents0 (const db::Texts *r, db::Coord d)
{
  return extents1 (r, d, d);
}

static db::Edges edges (const db::Texts *ep)
{
  db::Edges e;
  ep->edges (e);
  return e;
}

static void insert_t (db::Texts *t, const db::Texts &a)
{
  for (db::Texts::const_iterator p = a.begin (); ! p.at_end (); ++p) {
    t->insert (*p);
  }
}

static bool is_deep (const db::Texts *t)
{
  return dynamic_cast<const db::DeepTexts *> (t->delegate ()) != 0;
}

static size_t id (const db::Texts *t)
{
  return tl::id_of (t->delegate ());
}

static db::Texts with_text (const db::Texts *r, const std::string &text, bool inverse)
{
  db::TextStringFilter f (text, inverse);
  return r->filtered (f);
}

static db::Texts with_match (const db::Texts *r, const std::string &pattern, bool inverse)
{
  db::TextPatternFilter f (pattern, inverse);
  return r->filtered (f);
}

static db::Region pull_interacting (const db::Texts *r, const db::Region &other)
{
  db::Region out;
  r->pull_interacting (out, other);
  return out;
}

extern Class<db::ShapeCollection> decl_dbShapeCollection;

Class<db::Texts> decl_Texts (decl_dbShapeCollection, "db", "Texts",
  constructor ("new", &new_v, 
    "@brief Default constructor\n"
    "\n"
    "This constructor creates an empty text collection.\n"
  ) + 
  constructor ("new", &new_a, gsi::arg ("array"),
    "@brief Constructor from an text array\n"
    "\n"
    "This constructor creates an text collection from an array of \\Text objects.\n"
  ) +
  constructor ("new", &new_text, gsi::arg ("text"),
    "@brief Constructor from a single edge pair object\n"
    "\n"
    "This constructor creates an text collection with a single text.\n"
  ) +
  constructor ("new", &new_shapes, gsi::arg ("shapes"),
    "@brief Shapes constructor\n"
    "\n"
    "This constructor creates an text collection from a \\Shapes collection.\n"
  ) +
  constructor ("new", &new_si, gsi::arg ("shape_iterator"),
    "@brief Constructor from a hierarchical shape set\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only texts are taken from the shape set and other shapes are ignored.\n"
    "This method allows feeding the text collection from a hierarchy of cells.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2, gsi::arg ("shape_iterator"), gsi::arg ("trans"),
    "@brief Constructor from a hierarchical shape set with a transformation\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "Only texts are taken from the shape set and other shapes are ignored.\n"
    "The given transformation is applied to each text taken.\n"
    "This method allows feeding the text collection from a hierarchy of cells.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_sid, gsi::arg ("shape_iterator"), gsi::arg ("dss"),
    "@brief Creates a hierarchical text collection from an original layer\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical text collection which supports hierarchical operations.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2d, gsi::arg ("shape_iterator"), gsi::arg ("dss"), gsi::arg ("trans"),
    "@brief Creates a hierarchical text collection from an original layer with a transformation\n"
    "\n"
    "This constructor creates a text collection from the shapes delivered by the given recursive shape iterator.\n"
    "This version will create a hierarchical text collection which supports hierarchical operations.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Texts::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
  ) +
  method ("insert_into", &db::Texts::insert_into, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Inserts this texts into the given layout, below the given cell and into the given layer.\n"
    "If the text collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
  ) +
  method ("insert_into_as_polygons", &db::Texts::insert_into_as_polygons, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("e"),
    "@brief Inserts this texts into the given layout, below the given cell and into the given layer.\n"
    "If the text collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "The texts will be converted to polygons with the enlargement value given be 'e'. See \\polygon or \\extents for details.\n"
  ) +
  method ("insert", (void (db::Texts::*) (const db::Text &)) &db::Texts::insert, gsi::arg ("text"),
    "@brief Inserts a text into the collection\n"
  ) +
  method_ext ("is_deep?", &is_deep,
    "@brief Returns true if the edge pair collection is a deep (hierarchical) one\n"
  ) +
  method_ext ("data_id", &id,
    "@brief Returns the data ID (a unique identifier for the underlying data storage)\n"
  ) +
  method ("+|join", &db::Texts::operator+, gsi::arg ("other"),
    "@brief Returns the combined text collection of self and the other one\n"
    "\n"
    "@return The resulting text collection\n"
    "\n"
    "This operator adds the texts of the other collection to self and returns a new combined set.\n"
    "\n"
    "The 'join' alias has been introduced in version 0.28.12."
  ) +
  method ("+=|join_with", &db::Texts::operator+=, gsi::arg ("other"),
    "@brief Adds the texts of the other text collection to self\n"
    "\n"
    "@return The text collection after modification (self)\n"
    "\n"
    "This operator adds the texts of the other collection to self.\n"
    "\n"
    "Note that in Ruby, the '+=' operator actually does not exist, but is emulated by '+' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'join_with' instead.\n"
    "\n"
    "The 'join_with' alias has been introduced in version 0.28.12."
  ) +
  method_ext ("move", &move_p, gsi::arg ("p"),
    "@brief Moves the text collection\n"
    "\n"
    "Moves the texts by the given offset and returns the \n"
    "moved text collection. The text collection is overwritten.\n"
    "\n"
    "@param p The distance to move the texts.\n"
    "\n"
    "@return The moved texts (self).\n"
  ) +
  method_ext ("move", &move_xy, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Moves the text collection\n"
    "\n"
    "Moves the edge pairs by the given offset and returns the \n"
    "moved texts. The edge pair collection is overwritten.\n"
    "\n"
    "@param x The x distance to move the texts.\n"
    "@param y The y distance to move the texts.\n"
    "\n"
    "@return The moved texts (self).\n"
  ) +
  method_ext ("moved", &moved_p, gsi::arg ("p"),
    "@brief Returns the moved text collection (does not modify self)\n"
    "\n"
    "Moves the texts by the given offset and returns the \n"
    "moved texts. The text collection is not modified.\n"
    "\n"
    "@param p The distance to move the texts.\n"
    "\n"
    "@return The moved texts.\n"
  ) +
  method_ext ("moved", &moved_xy, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Returns the moved edge pair collection (does not modify self)\n"
    "\n"
    "Moves the texts by the given offset and returns the \n"
    "moved texts. The text collection is not modified.\n"
    "\n"
    "@param x The x distance to move the texts.\n"
    "@param y The y distance to move the texts.\n"
    "\n"
    "@return The moved texts.\n"
  ) +
  method ("transformed", (db::Texts (db::Texts::*)(const db::Trans &) const) &db::Texts::transformed, gsi::arg ("t"),
    "@brief Transform the edge pair collection\n"
    "\n"
    "Transforms the texts with the given transformation.\n"
    "Does not modify the edge pair collection but returns the transformed texts.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed texts.\n"
  ) +
  method ("transformed|#transformed_icplx", (db::Texts (db::Texts::*)(const db::ICplxTrans &) const) &db::Texts::transformed, gsi::arg ("t"),
    "@brief Transform the text collection with a complex transformation\n"
    "\n"
    "Transforms the text with the given complex transformation.\n"
    "Does not modify the text collection but returns the transformed texts.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed texts.\n"
  ) +
  method ("transform", (db::Texts &(db::Texts::*)(const db::Trans &)) &db::Texts::transform, gsi::arg ("t"),
    "@brief Transform the text collection (modifies self)\n"
    "\n"
    "Transforms the text collection with the given transformation.\n"
    "This version modifies the text collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed text collection.\n"
  ) +
  method ("transform|#transform_icplx", (db::Texts &(db::Texts::*)(const db::ICplxTrans &)) &db::Texts::transform, gsi::arg ("t"),
    "@brief Transform the text collection with a complex transformation (modifies self)\n"
    "\n"
    "Transforms the text collection with the given transformation.\n"
    "This version modifies the text collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed text collection.\n"
  ) +
  method_ext ("insert", &insert_t, gsi::arg ("texts"),
    "@brief Inserts all texts from the other text collection into this collection\n"
  ) +
  method_ext ("edges", &edges,
    "@brief Returns dot-like edges for the texts\n"
    "@return An edge collection containing the individual, dot-like edges\n"
  ) +
  method_ext ("extents", &extents0, gsi::arg ("d", db::Coord (1)),
    "@brief Returns a region with the enlarged bounding boxes of the texts\n"
    "Text bounding boxes are point-like boxes which vanish unless an enlargement of >0 is specified.\n"
    "The bounding box is centered at the text's location.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents1, gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Returns a region with the enlarged bounding boxes of the texts\n"
    "This method acts like the other version of \\extents, but allows giving different enlargements for x and y direction.\n"
  ) + 
  method_ext ("polygons", &polygons0, gsi::arg ("e", db::Coord (1)),
    "@brief Converts the edge pairs to polygons\n"
    "This method creates polygons from the texts. This is equivalent to calling \\extents."
  ) +
  method_ext ("with_text", with_text, gsi::arg ("text"), gsi::arg ("inverse"),
    "@brief Filter the text by text string\n"
    "If \"inverse\" is false, this method returns the texts with the given string.\n"
    "If \"inverse\" is true, this method returns the texts not having the given string.\n"
  ) +
  method_ext ("with_match", with_match, gsi::arg ("pattern"), gsi::arg ("inverse"),
    "@brief Filter the text by glob pattern\n"
    "\"pattern\" is a glob-style pattern (e.g. \"A*\" will select all texts starting with a capital \"A\").\n"
    "If \"inverse\" is false, this method returns the texts matching the pattern.\n"
    "If \"inverse\" is true, this method returns the texts not matching the pattern.\n"
  ) +
  method ("interacting|&", (db::Texts (db::Texts::*) (const db::Region &) const)  &db::Texts::selected_interacting, gsi::arg ("other"),
    "@brief Returns the texts from this text collection which are inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A new text collection containing the texts inside or on the edge of polygons from the region\n"
  ) +
  method ("not_interacting|-", (db::Texts (db::Texts::*) (const db::Region &) const)  &db::Texts::selected_not_interacting, gsi::arg ("other"),
    "@brief Returns the texts from this text collection which are not inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A new text collection containing the texts not inside or on the edge of polygons from the region\n"
  ) +
  method ("select_interacting", (db::Texts &(db::Texts::*) (const db::Region &)) &db::Texts::select_interacting, gsi::arg ("other"),
    "@brief Selects the texts from this text collection which are inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A text collection after the texts have been selected (self)\n"
    "\n"
    "In contrast to \\interacting, this method will modify self.\n"
  ) +
  method ("select_not_interacting", (db::Texts &(db::Texts::*) (const db::Region &)) &db::Texts::select_not_interacting, gsi::arg ("other"),
    "@brief Selects the texts from this text collection which are not inside or on the edge of polygons from the given region\n"
    "\n"
    "@return A text collection after the texts have been selected (self)\n"
    "\n"
    "In contrast to \\interacting, this method will modify self.\n"
  ) +
  method_ext ("pull_interacting", &pull_interacting, gsi::arg ("other"),
    "@brief Returns all polygons of \"other\" which are including texts of this text set\n"
    "The \"pull_...\" method is similar to \"select_...\" but works the opposite way: it "
    "selects shapes from the argument region rather than self. In a deep (hierarchical) context "
    "the output region will be hierarchically aligned with self, so the \"pull_...\" method "
    "provide a way for re-hierarchization.\n"
    "\n"
    "@return The region after the polygons have been selected (from other)\n"
    "\n"
    "Merged semantics applies for the polygon region.\n"
  ) +
  method ("clear", &db::Texts::clear,
    "@brief Clears the text collection\n"
  ) +
  method ("swap", &db::Texts::swap, gsi::arg ("other"),
    "@brief Swap the contents of this collection with the contents of another collection\n"
    "This method is useful to avoid excessive memory allocation in some cases. "
    "For managed memory languages such as Ruby, those cases will be rare. " 
  ) +
  method ("bbox", &db::Texts::bbox,
    "@brief Return the bounding box of the text collection\n"
    "The bounding box is the box enclosing all origins of all texts.\n"
  ) +
  method ("is_empty?", &db::Texts::empty,
    "@brief Returns true if the collection is empty\n"
  ) +
  method ("count|#size", (size_t (db::Texts::*) () const) &db::Texts::count,
    "@brief Returns the (flat) number of texts in the text collection\n"
    "\n"
    "The count is computed 'as if flat', i.e. texts inside a cell are multiplied by the number of times a cell is instantiated.\n"
    "\n"
    "Starting with version 0.27, the method is called 'count' for consistency with \\Region. 'size' is still provided as an alias."
  ) +
  method ("hier_count", (size_t (db::Texts::*) () const) &db::Texts::hier_count,
    "@brief Returns the (hierarchical) number of texts in the text collection\n"
    "\n"
    "The count is computed 'hierarchical', i.e. texts inside a cell are counted once even if the cell is instantiated multiple times.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::iterator ("each", &db::Texts::begin,
    "@brief Returns each text of the text collection\n"
  ) +
  method ("[]", &db::Texts::nth, gsi::arg ("n"),
    "@brief Returns the nth text\n"
    "\n"
    "This method returns nil if the index is out of range. It is available for flat texts only - i.e. "
    "those for which \\has_valid_texts? is true. Use \\flatten to explicitly flatten an text collection.\n"
    "\n"
    "The \\each iterator is the more general approach to access the texts."
  ) +
  method ("flatten", &db::Texts::flatten,
    "@brief Explicitly flattens an text collection\n"
    "\n"
    "If the collection is already flat (i.e. \\has_valid_texts? returns true), this method will "
    "not change the collection.\n"
  ) +
  method ("has_valid_texts?", &db::Texts::has_valid_texts,
    "@brief Returns true if the text collection is flat and individual texts can be accessed randomly\n"
  ) +
  method ("enable_progress", &db::Texts::enable_progress, gsi::arg ("label"),
    "@brief Enable progress reporting\n"
    "After calling this method, the text collection will report the progress through a progress bar while "
    "expensive operations are running.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
  ) +
  method ("disable_progress", &db::Texts::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will disable progress reporting. See \\enable_progress.\n"
  ) +
  method_ext ("to_s", &to_string0,
    "@brief Converts the text collection to a string\n"
    "The length of the output is limited to 20 texts to avoid giant strings on large collections. "
    "For full output use \"to_s\" with a maximum count parameter.\n"
  ) +
  method_ext ("to_s", &to_string1, gsi::arg ("max_count"),
    "@brief Converts the text collection to a string\n"
    "This version allows specification of the maximum number of texts contained in the string."
  ) +
  gsi::make_property_methods<db::Texts> ()
  ,
  "@brief Texts (a collection of texts)\n"
  "\n"
  "Text objects are useful as labels for net names, to identify certain regions and to specify specific locations in general. "
  "Text collections provide a way to store - also in a hierarchical fashion - and manipulate a collection of text objects.\n"
  "\n"
  "Text objects can be turned into polygons by creating small boxes around the texts (\\polygons). Texts can also be turned into dot-like "
  "edges (\\edges). Texts can be filtered by string, either by matching against a fixed string (\\with_text) or a glob-style pattern (\\with_match).\n"
  "\n"
  "Text collections can be filtered geometrically against a polygon \\Region using \\interacting or \\non-interacting. "
  "Vice versa, texts can be used to select polygons from a \\Region using \\pull_interacting.\n"
  "\n"
  "Beside that, text collections can be transformed, flattened and combined, similar to \\EdgePairs.\n"
  "\n"
  "This class has been introduced in version 0.27.\n"
);

}
