
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
#include "rdb.h"
#include "rdbUtils.h"
#include "rdbTiledRdbOutputReceiver.h"

#include "dbPolygon.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbText.h"
#include "dbRecursiveShapeIterator.h"
#include "dbTilingProcessor.h"

namespace gsi
{

// ---------------------------------------------------------------
//  Utilities

class ItemRefUnwrappingIterator
{
public:
  typedef rdb::Database::const_item_ref_iterator::iterator_category iterator_category;
  typedef rdb::Database::const_item_ref_iterator::difference_type difference_type;
  typedef rdb::Item value_type;
  typedef const rdb::Item &reference;
  typedef const rdb::Item *pointer;

  ItemRefUnwrappingIterator (rdb::Database::const_item_ref_iterator i)
    : m_iter (i)
  { }

  bool operator== (const ItemRefUnwrappingIterator &d) const
  {
    return m_iter == d.m_iter;
  }

  bool operator!= (const ItemRefUnwrappingIterator &d) const
  {
    return m_iter != d.m_iter;
  }

  ItemRefUnwrappingIterator &operator++ () 
  {
    ++m_iter;
    return *this;
  }

  const rdb::Item &operator* () const
  {
    return (*m_iter).operator* ();
  }

  const rdb::Item *operator-> () const
  {
    return (*m_iter).operator-> ();
  }

private:
  rdb::Database::const_item_ref_iterator m_iter;
};

// ---------------------------------------------------------------
//  rdb::Reference binding

static rdb::Reference *new_ref_tp (const db::DCplxTrans &trans, rdb::id_type parent_cell_id)
{
  return new rdb::Reference (trans, parent_cell_id);
}

Class<rdb::Reference> decl_RdbReference ("rdb", "RdbReference",
  gsi::constructor ("new", &new_ref_tp, gsi::arg ("trans"), gsi::arg ("parent_cell_id"),
    "@brief Creates a reference with a given transformation and parent cell ID\n"
  ) +
  gsi::method ("database", (const rdb::Database *(rdb::Reference::*)() const) &rdb::Reference::database,
    "@brief Gets the database object that category is associated with\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("trans", &rdb::Reference::trans, 
    "@brief Gets the transformation for this reference\n"
    "The transformation describes the transformation of the child cell into the parent cell. In that sense that is the "
    "usual transformation of a cell reference.\n"
    "@return The transformation\n"
  ) +
  gsi::method ("trans=", &rdb::Reference::set_trans, gsi::arg ("trans"),
    "@brief Sets the transformation for this reference\n"
  ) +
  gsi::method ("parent_cell_id", &rdb::Reference::parent_cell_id, 
    "@brief Gets parent cell ID for this reference\n"
    "@return The parent cell ID\n"
  ) +
  gsi::method ("parent_cell_id=", &rdb::Reference::set_parent_cell_id, gsi::arg ("id"),
    "@brief Sets the parent cell ID for this reference\n"
  ),
  "@brief A cell reference inside the report database\n"
  "This class describes a cell reference. Such reference object can be attached to cells to describe instantiations of them "
  "in parent cells. Not necessarily all instantiations of a cell in the layout database are represented by references and "
  "in some cases there might even be no references at all. The references are merely a hint how a marker must be displayed "
  "in the context of any other, potentially parent, cell in the layout database."
);

// ---------------------------------------------------------------
//  rdb::Cell binding

static rdb::References::const_iterator begin_references (const rdb::Cell *cell)
{
  return cell->references ().begin ();
}

static rdb::References::const_iterator end_references (const rdb::Cell *cell)
{
  return cell->references ().end ();
}

static void add_reference (rdb::Cell *cell, const rdb::Reference &ref)
{
  cell->references ().insert (ref);
}

static void clear_references (rdb::Cell *cell)
{
  cell->references ().clear ();
}

ItemRefUnwrappingIterator cell_items_begin (const rdb::Cell *cell)
{
  tl_assert (cell->database ());
  return cell->database ()->items_by_cell (cell->id ()).first;
}

ItemRefUnwrappingIterator cell_items_end (const rdb::Cell *cell)
{
  tl_assert (cell->database ());
  return cell->database ()->items_by_cell (cell->id ()).second;
}

Class<rdb::Cell> decl_RdbCell ("rdb", "RdbCell",
  gsi::method ("rdb_id", &rdb::Cell::id, 
    "@brief Gets the cell ID\n"
    "The cell ID is an integer that uniquely identifies the cell. It is used for referring to a "
    "cell in \\RdbItem for example.\n"
    "@return The cell ID\n"
  ) +
  gsi::method ("database", (const rdb::Database *(rdb::Cell::*)() const) &rdb::Cell::database,
    "@brief Gets the database object that category is associated with\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::iterator_ext ("each_item", &cell_items_begin, &cell_items_end,
    "@brief Iterates over all items inside the database which are associated with this cell\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("name", &rdb::Cell::name, 
    "@brief Gets the cell name\n"
    "The cell name is an string that identifies the category in the database. "
    "Additionally, a cell may carry a variant identifier which is a string that uniquely identifies a cell "
    "in the context of its variants. The \"qualified name\" contains both the cell name and the variant name. "
    "Cell names are also used to identify report database cell's with layout cells. "
    "@return The cell name\n"
  ) +
  gsi::method ("variant", &rdb::Cell::variant, 
    "@brief Gets the cell variant name\n"
    "A variant name additionally identifies the cell when multiple cells with the same name are present. "
    "A variant name is either assigned automatically or set when creating a cell. "
    "@return The cell variant name\n"
  ) +
  gsi::method ("qname", &rdb::Cell::qname, 
    "@brief Gets the cell's qualified name\n"
    "The qualified name is a combination of the cell name and optionally the variant name. "
    "It is used to identify the cell by name in a unique way.\n"
    "@return The qualified name\n"
  ) +
  gsi::method ("num_items", &rdb::Cell::num_items, 
    "@brief Gets the number of items for this cell\n"
  ) +
  gsi::method ("num_items_visited", &rdb::Cell::num_items_visited, 
    "@brief Gets the number of visited items for this cell\n"
  ) +
  gsi::method_ext ("add_reference", &add_reference, gsi::arg ("ref"),
    "@brief Adds a reference to the references of this cell\n"
    "@param ref The reference to add.\n"
  ) +
  gsi::method_ext ("clear_references", &clear_references,
    "@brief Removes all references from this cell\n"
  ) +
  gsi::iterator_ext ("each_reference", &begin_references, &end_references,
    "@brief Iterates over all references\n"
  ),
  "@brief A cell inside the report database\n"
  "This class represents a cell in the report database. There is not necessarily a 1:1 correspondence of RDB cells "
  "and layout database cells. Cells have an ID, a name, optionally a variant name and a set of references which "
  "describe at least one example instantiation in some parent cell. The references do not necessarily map to "
  "references or cover all references in the layout database."
);

// ---------------------------------------------------------------
//  rdb::Category binding

static rdb::Categories::iterator begin_sub_categories (rdb::Category *cat)
{
  return cat->sub_categories ().begin ();
}

static rdb::Categories::iterator end_sub_categories (rdb::Category *cat)
{
  return cat->sub_categories ().end ();
}

ItemRefUnwrappingIterator category_items_begin (const rdb::Category *cat)
{
  tl_assert (cat->database ());
  return cat->database ()->items_by_category (cat->id ()).first;
}

ItemRefUnwrappingIterator category_items_end (const rdb::Category *cat)
{
  tl_assert (cat->database ());
  return cat->database ()->items_by_category (cat->id ()).second;
}

static void scan_layer (rdb::Category *cat, const db::Layout &layout, unsigned int layer, const db::Cell *from_cell, int levels, bool with_properties)
{
  rdb::scan_layer (cat, layout, layer, from_cell, levels, with_properties);
}

static void scan_shapes (rdb::Category *cat, const db::RecursiveShapeIterator &iter, bool flat, bool with_properties)
{
  rdb::scan_layer (cat, iter, flat, with_properties);
}

static void scan_region (rdb::Category *cat, rdb::Cell *cell, const db::CplxTrans &trans, const db::Region &region, bool flat, bool with_properties)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = region.begin_iter ();
  rdb::scan_layer (cat, cell, trans * it.second, it.first, flat, with_properties);
}

static void scan_edges (rdb::Category *cat, rdb::Cell *cell, const db::CplxTrans &trans, const db::Edges &edges, bool flat, bool with_properties)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = edges.begin_iter ();
  rdb::scan_layer (cat, cell, trans * it.second, it.first, flat, with_properties);
}

static void scan_edge_pairs (rdb::Category *cat, rdb::Cell *cell, const db::CplxTrans &trans, const db::EdgePairs &edge_pairs, bool flat, bool with_properties)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = edge_pairs.begin_iter ();
  rdb::scan_layer (cat, cell, trans * it.second, it.first, flat, with_properties);
}

static void scan_texts (rdb::Category *cat, rdb::Cell *cell, const db::CplxTrans &trans, const db::Texts &texts, bool flat, bool with_properties)
{
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> it = texts.begin_iter ();
  rdb::scan_layer (cat, cell, trans * it.second, it.first, flat, with_properties);
}

Class<rdb::Category> decl_RdbCategory ("rdb", "RdbCategory",
  gsi::method ("rdb_id", &rdb::Category::id, 
    "@brief Gets the category ID\n"
    "The category ID is an integer that uniquely identifies the category. It is used for referring to a "
    "category in \\RdbItem for example.\n"
    "@return The category ID\n"
  ) +
  gsi::method ("database", (const rdb::Database *(rdb::Category::*)() const) &rdb::Category::database,
    "@brief Gets the database object that category is associated with\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::iterator_ext ("each_item", &category_items_begin, &category_items_end,
    "@brief Iterates over all items inside the database which are associated with this category\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("scan_shapes", &scan_shapes, gsi::arg ("iter"), gsi::arg ("flat", false), gsi::arg ("with_properties", true),
    "@brief Scans the polygon or edge shapes from the shape iterator into the category\n"
    "Creates RDB items for each polygon or edge shape read from the iterator and puts them into this category.\n"
    "A similar, but lower-level method is \\ReportDatabase#create_items with a \\RecursiveShapeIterator argument.\n"
    "In contrast to \\ReportDatabase#create_items, 'scan_shapes' can also produce hierarchical databases "
    "if the \\flat argument is false. In this case, the hierarchy the recursive shape iterator traverses is "
    "copied into the report database using sample references.\n"
    "\n"
    "If 'with_properties' is true, user properties will be turned into tagged values as well.\n"
    "\n"
    "This method has been introduced in version 0.23. The flat mode argument has been added in version 0.26. The 'with_properties' argument has been added in version 0.28.\n"
  ) +
  gsi::method_ext ("scan_collection", &scan_region, gsi::arg ("cell"), gsi::arg ("trans"), gsi::arg ("region"), gsi::arg ("flat", false), gsi::arg ("with_properties", true),
    "@brief Turns the given region into a hierarchical or flat report database\n"
    "The exact behavior depends on the nature of the region. If the region is a hierarchical (original or deep) region "
    "and the 'flat' argument is false, this method will produce a hierarchical report database in the given category. "
    "The 'cell_id' parameter is ignored in this case. Sample references will be produced to supply "
    "minimal instantiation information.\n"
    "\n"
    "If the region is a flat one or the 'flat' argument is true, the region's polygons will be produced as "
    "report database items in this category and in the cell given by 'cell_id'.\n"
    "\n"
    "The transformation argument needs to supply the dbu-to-micron transformation.\n"
    "\n"
    "If 'with_properties' is true, user properties will be turned into tagged values as well.\n"
    "\n"
    "This method has been introduced in version 0.26. The 'with_properties' argument has been added in version 0.28.\n"
  ) +
  gsi::method_ext ("scan_collection", &scan_edges, gsi::arg ("cell"), gsi::arg ("trans"), gsi::arg ("edges"), gsi::arg ("flat", false), gsi::arg ("with_properties", true),
    "@brief Turns the given edge collection into a hierarchical or flat report database\n"
    "This a another flavour of \\scan_collection accepting an edge collection.\n"
    "\n"
    "This method has been introduced in version 0.26. The 'with_properties' argument has been added in version 0.28.\n"
  ) +
  gsi::method_ext ("scan_collection", &scan_edge_pairs, gsi::arg ("cell"), gsi::arg ("trans"), gsi::arg ("edge_pairs"), gsi::arg ("flat", false), gsi::arg ("with_properties", true),
    "@brief Turns the given edge pair collection into a hierarchical or flat report database\n"
    "This a another flavour of \\scan_collection accepting an edge pair collection.\n"
    "\n"
    "This method has been introduced in version 0.26. The 'with_properties' argument has been added in version 0.28.\n"
  ) +
  gsi::method_ext ("scan_collection", &scan_texts, gsi::arg ("cell"), gsi::arg ("trans"), gsi::arg ("texts"), gsi::arg ("flat", false), gsi::arg ("with_properties", true),
    "@brief Turns the given edge pair collection into a hierarchical or flat report database\n"
    "This a another flavour of \\scan_collection accepting a text collection.\n"
    "\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  gsi::method_ext ("scan_layer", &scan_layer, gsi::arg ("layout"), gsi::arg ("layer"), gsi::arg ("cell", (const db::Cell *) 0, "nil"), gsi::arg ("levels", -1), gsi::arg ("with_properties", true),
    "@brief Scans a layer from a layout into this category, starting with a given cell and a depth specification\n"
    "Creates RDB items for each polygon or edge shape read from the cell and its children in the layout on the given layer and puts them into this category.\n"
    "New cells will be generated when required.\n"
    "\"levels\" is the number of hierarchy levels to take the child cells from. 0 means to use only \"cell\" and don't descend, -1 means \"all levels\".\n"  
    "Other settings like database unit, description, top cell etc. are not made in the RDB.\n"
    "\n"
    "If 'with_properties' is true, user properties will be turned into tagged values as well.\n"
    "\n"
    "This method has been introduced in version 0.23. The 'with_properties' argument has been added in version 0.28.\n"
  ) +
  gsi::method ("name", &rdb::Category::name, 
    "@brief Gets the category name\n"
    "The category name is an string that identifies the category in the context of a parent category or "
    "inside the database when it is a top level category. The name is not the path name which is a path "
    "to a child category and incorporates all names of parent categories.\n"
    "@return The category name\n"
  ) +
  gsi::method ("path", &rdb::Category::path, 
    "@brief Gets the category path\n"
    "The category path is the category name for top level categories. For child categories, the path "
    "contains the names of all parent categories separated by a dot.\n"
    "@return The path for this category\n"
  ) +
  gsi::method ("description", &rdb::Category::description, 
    "@brief Gets the category description\n"
    "@return The description string\n"
  ) +
  gsi::method ("description=", &rdb::Category::set_description, gsi::arg ("description"),
    "@brief Sets the category description\n"
    "@param description The description string\n"
  ) +
  gsi::iterator_ext ("each_sub_category", &begin_sub_categories, &end_sub_categories,
    "@brief Iterates over all sub-categories\n"
  ) +
  gsi::method ("parent", (rdb::Category *(rdb::Category::*) ()) &rdb::Category::parent, 
    "@brief Gets the parent category of this category\n"
    "@return The parent category or nil if this category is a top-level category\n"
  ) +
  gsi::method ("num_items", &rdb::Category::num_items, 
    "@brief Gets the number of items in this category\n"
    "The number of items includes the items in sub-categories of this category.\n"
  ) +
  gsi::method ("num_items_visited", &rdb::Category::num_items_visited, 
    "@brief Gets the number of visited items in this category\n"
    "The number of items includes the items in sub-categories of this category.\n"
  ),
  "@brief A category inside the report database\n"
  "Every item in the report database is assigned to a category. A category is a DRC rule check for example. "
  "Categories can be organized hierarchically, i.e. a category may have sub-categories. Item counts are summarized "
  "for categories and items belonging to sub-categories of one category can be browsed together for example. "
  "As a general rule, categories not being leaf categories (having child categories) may not have items. "
);
  
// ---------------------------------------------------------------
//  rdb::Value binding

rdb::ValueWrapper *value_from_string (const std::string &s)
{
  if (s.empty ()) {
    return new rdb::ValueWrapper ();
  } else {
    return new rdb::ValueWrapper (rdb::ValueBase::create_from_string (s));
  }
}

rdb::ValueWrapper *new_value_f (double f)
{
  return new rdb::ValueWrapper (new rdb::Value<double> (f));
}

rdb::ValueWrapper *new_value_s (const std::string &s)
{
  return new rdb::ValueWrapper (new rdb::Value<std::string> (s));
}

rdb::ValueWrapper *new_value_p (const db::DPolygon &p)
{
  return new rdb::ValueWrapper (new rdb::Value<db::DPolygon> (p));
}

rdb::ValueWrapper *new_value_text (const db::DText &t)
{
  return new rdb::ValueWrapper (new rdb::Value<db::DText> (t));
}

rdb::ValueWrapper *new_value_path (const db::DPath &t)
{
  return new rdb::ValueWrapper (new rdb::Value<db::DPath> (t));
}

rdb::ValueWrapper *new_value_ep (const db::DEdgePair &e)
{
  return new rdb::ValueWrapper (new rdb::Value<db::DEdgePair> (e));
}

rdb::ValueWrapper *new_value_e (const db::DEdge &e)
{
  return new rdb::ValueWrapper (new rdb::Value<db::DEdge> (e));
}

rdb::ValueWrapper *new_value_b (const db::DBox &b)
{
  return new rdb::ValueWrapper (new rdb::Value<db::DBox> (b));
}

std::string value_to_string (const rdb::ValueWrapper *v)
{
  if (v->get () == 0) {
    return std::string ();
  } else {
    return v->get ()->to_string ();
  }
}

bool value_is_polygon (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<db::DPolygon> *> (v->get ()) != 0;
}

db::DPolygon value_get_polygon (const rdb::ValueWrapper *v)
{
  const rdb::Value<db::DPolygon> *g = dynamic_cast <const rdb::Value<db::DPolygon> *> (v->get ());
  if (g) {
    return g->value ();
  } else {
    return db::DPolygon ();
  }
}

bool value_is_path (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<db::DPath> *> (v->get ()) != 0;
}

db::DPath value_get_path (const rdb::ValueWrapper *v)
{
  const rdb::Value<db::DPath> *g = dynamic_cast <const rdb::Value<db::DPath> *> (v->get ());
  if (g) {
    return g->value ();
  } else {
    return db::DPath ();
  }
}

bool value_is_text (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<db::DText> *> (v->get ()) != 0;
}

db::DText value_get_text (const rdb::ValueWrapper *v)
{
  const rdb::Value<db::DText> *g = dynamic_cast <const rdb::Value<db::DText> *> (v->get ());
  if (g) {
    return g->value ();
  } else {
    return db::DText ();
  }
}

bool value_is_edge_pair (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<db::DEdgePair> *> (v->get ()) != 0;
}

db::DEdgePair value_get_edge_pair (const rdb::ValueWrapper *v)
{
  const rdb::Value<db::DEdgePair> *g = dynamic_cast <const rdb::Value<db::DEdgePair> *> (v->get ());
  if (g) {
    return g->value ();
  } else {
    return db::DEdgePair ();
  }
}

bool value_is_edge (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<db::DEdge> *> (v->get ()) != 0;
}

db::DEdge value_get_edge (const rdb::ValueWrapper *v)
{
  const rdb::Value<db::DEdge> *g = dynamic_cast <const rdb::Value<db::DEdge> *> (v->get ());
  if (g) {
    return g->value ();
  } else {
    return db::DEdge ();
  }
}

bool value_is_box (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<db::DBox> *> (v->get ()) != 0;
}

db::DBox value_get_box (const rdb::ValueWrapper *v)
{
  const rdb::Value<db::DBox> *g = dynamic_cast <const rdb::Value<db::DBox> *> (v->get ());
  if (g) {
    return g->value ();
  } else {
    return db::DBox ();
  }
}

bool value_is_string (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<std::string> *> (v->get ()) != 0;
}

std::string value_get_string (const rdb::ValueWrapper *v)
{
  if (v->get ()) {
    return v->get ()->to_display_string ();
  } else {
    return std::string ();
  }
}

bool value_is_float (const rdb::ValueWrapper *v)
{
  return dynamic_cast <const rdb::Value<double> *> (v->get ()) != 0;
}

double value_get_float (const rdb::ValueWrapper *v)
{
  const rdb::Value<double> *g = dynamic_cast <const rdb::Value<double> *> (v->get ());
  if (g) {
    return g->value ();
  } else {
    return 0.0;
  }
}

rdb::id_type value_get_tag_id (const rdb::ValueWrapper *v)
{
  return v->tag_id ();
}

void value_set_tag_id (rdb::ValueWrapper *v, rdb::id_type id)
{
  v->set_tag_id (id);
}

Class<rdb::ValueWrapper> decl_RdbItemValue ("rdb", "RdbItemValue",
  gsi::method ("from_s", &value_from_string, gsi::arg ("s"),
    "@brief Creates a value object from a string\n"
    "The string format is the same than obtained by the to_s method.\n"
  ) +
  gsi::constructor ("new", &new_value_f, gsi::arg ("f"),
    "@brief Creates a value representing a numeric value\n"
    "\n"
    "This variant has been introduced in version 0.24\n"
  ) +
  gsi::constructor ("new", &new_value_s, gsi::arg ("s"),
    "@brief Creates a value representing a string\n"
  ) +
  gsi::constructor ("new", &new_value_p, gsi::arg ("p"),
    "@brief Creates a value representing a DPolygon object\n"
  ) +
  gsi::constructor ("new", &new_value_path, gsi::arg ("p"),
    "@brief Creates a value representing a DPath object\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::constructor ("new", &new_value_text, gsi::arg ("t"),
    "@brief Creates a value representing a DText object\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::constructor ("new", &new_value_e, gsi::arg ("e"),
    "@brief Creates a value representing a DEdge object\n"
  ) +
  gsi::constructor ("new", &new_value_ep, gsi::arg ("ee"),
    "@brief Creates a value representing a DEdgePair object\n"
  ) +
  gsi::constructor ("new", &new_value_b, gsi::arg ("b"),
    "@brief Creates a value representing a DBox object\n"
  ) +
  gsi::method_ext ("to_s", &value_to_string, 
    "@brief Converts a value to a string\n"
    "The string can be used by the string constructor to create another object from it.\n"
    "@return The string\n"
  ) +
  gsi::method_ext ("is_float?", &value_is_float, 
    "@brief Returns true if the value object represents a numeric value\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("float", &value_get_float, 
    "@brief Gets the numeric value.\n"
    "@return The numeric value or 0\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("is_string?", &value_is_string, 
    "@brief Returns true if the object represents a string value\n"
  ) +
  gsi::method_ext ("string", &value_get_string, 
    "@brief Gets the string representation of the value.\n"
    "@return The string"
    "This method will always deliver a valid string, even if \\is_string? is false. "
    "The objects stored in the value are converted to a string accordingly.\n"
  ) +
  gsi::method_ext ("is_polygon?", &value_is_polygon, 
    "@brief Returns true if the value object represents a polygon\n"
  ) +
  gsi::method_ext ("polygon", &value_get_polygon, 
    "@brief Gets the polygon if the value represents one.\n"
    "@return The \\DPolygon object"
  ) +
  gsi::method_ext ("is_path?", &value_is_path, 
    "@brief Returns true if the value object represents a path\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("path", &value_get_path, 
    "@brief Gets the path if the value represents one.\n"
    "@return The \\DPath object"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("is_text?", &value_is_text, 
    "@brief Returns true if the value object represents a text\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("text", &value_get_text, 
    "@brief Gets the text if the value represents one.\n"
    "@return The \\DText object"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("is_edge_pair?", &value_is_edge_pair, 
    "@brief Returns true if the value object represents an edge pair\n"
  ) +
  gsi::method_ext ("edge_pair", &value_get_edge_pair, 
    "@brief Gets the edge pair if the value represents one.\n"
    "@return The \\DEdgePair object or nil"
  ) +
  gsi::method_ext ("is_edge?", &value_is_edge, 
    "@brief Returns true if the value object represents an edge\n"
  ) +
  gsi::method_ext ("edge", &value_get_edge, 
    "@brief Gets the edge if the value represents one.\n"
    "@return The \\DEdge object or nil"
  ) +
  gsi::method_ext ("is_box?", &value_is_box, 
    "@brief Returns true if the value object represents a box\n"
  ) +
  gsi::method_ext ("box", &value_get_box, 
    "@brief Gets the box if the value represents one.\n"
    "@return The \\DBox object or nil"
  ) + 
  gsi::method_ext ("tag_id=", &value_set_tag_id, gsi::arg ("id"),
    "@brief Sets the tag ID to make the value a tagged value or 0 to reset it\n"
    "@param id The tag ID\n"
    "To get a tag ID, use \\RdbDatabase#user_tag_id (preferred) or \\RdbDatabase#tag_id (for internal use).\n"
    "Tagged values have been added in version 0.24. Tags can be given to identify a value, for example "
    "to attache measurement values to an item. To attach a value for a specific measurement, a tagged value "
    "can be used where the tag ID describes the measurement made. In that way, multiple values for "
    "different measurements can be attached to an item.\n"
    "\n"
    "This variant has been introduced in version 0.24\n"
  ) +
  gsi::method_ext ("tag_id", &value_get_tag_id, 
    "@brief Gets the tag ID if the value is a tagged value or 0 if not\n"
    "@return The tag ID\n"
    "See \\tag_id= for details about tagged values.\n"
    "\n"
    "Tagged values have been added in version 0.24.\n"
  ),
  "@brief A value object inside the report database\n"
  "Value objects are attached to items to provide markers. An arbitrary number of such value objects can be attached to "
  "an item.\n"
  "Currently, a value can represent a box, a polygon or an edge. Geometrical objects are represented in micron units and are "
  "therefore \"D\" type objects (DPolygon, DEdge and DBox). "
);

// ---------------------------------------------------------------
//  rdb::Item binding

static rdb::Values::const_iterator begin_values (const rdb::Item *item)
{
  return item->values ().begin ();
}

static rdb::Values::const_iterator end_values (const rdb::Item *item)
{
  return item->values ().end ();
}

static void add_value_from_shape (rdb::Item *item, const db::Shape &shape, const db::CplxTrans &trans)
{
  rdb::ValueBase *value = rdb::ValueBase::create_from_shape (shape, trans);
  if (value) {
    item->values ().add (value);
  }
}

static void add_value (rdb::Item *item, const rdb::ValueWrapper &value)
{
  item->values ().add (value);
}

template <class T>
static void add_value_t (rdb::Item *item, const T &value)
{
  add_value (item, rdb::ValueWrapper (new rdb::Value<T> (value)));
}

static void clear_values (rdb::Item *item)
{
  item->set_values (rdb::Values ());
}

Class<rdb::Item> decl_RdbItem ("rdb", "RdbItem",
  gsi::method ("database", (const rdb::Database *(rdb::Item::*)() const) &rdb::Item::database,
    "@brief Gets the database object that item is associated with\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("cell_id", &rdb::Item::cell_id, 
    "@brief Gets the cell ID\n"
    "Returns the ID of the cell that this item is associated with.\n"
    "@return The cell ID\n"
  ) +
  gsi::method ("category_id", &rdb::Item::category_id, 
    "@brief Gets the category ID\n"
    "Returns the ID of the category that this item is associated with.\n"
    "@return The category ID\n"
  ) +
  gsi::method ("is_visited?", &rdb::Item::visited, 
    "@brief Gets a value indicating whether the item was already visited\n"
    "@return True, if the item has been visited already\n"
  ) +
  gsi::method ("add_tag", &rdb::Item::add_tag, gsi::arg ("tag_id"),
    "@brief Adds a tag with the given id to the item\n"
    "Each tag can be added once to the item. The tags of an item thus form a set. "
    "If a tag with that ID already exists, this method does nothing."
  ) +
  gsi::method ("remove_tag", &rdb::Item::remove_tag, gsi::arg ("tag_id"),
    "@brief Remove the tag with the given id from the item\n"
    "If a tag with that ID does not exists on this item, this method does nothing."
  ) +
  gsi::method ("has_tag?", &rdb::Item::has_tag, gsi::arg ("tag_id"),
    "@brief Returns a value indicating whether the item has a tag with the given ID\n"
    "@return True, if the item has a tag with the given ID\n"
  ) +
  gsi::method ("tags_str", &rdb::Item::tag_str, 
    "@brief Returns a string listing all tags of this item\n"
    "@return A comma-separated list of tags\n"
  ) +
  gsi::method ("tags_str=", &rdb::Item::set_tag_str, gsi::arg ("tags"),
    "@brief Sets the tags from a string\n"
    "@param tags A comma-separated list of tags\n"
  ) +
  gsi::method ("has_image?", &rdb::Item::has_image,
    "@brief Gets a value indicating that the item has an image attached\n"
    "See \\image_str how to obtain the image.\n\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  gsi::method ("image_str", &rdb::Item::image_str,
    "@brief Gets the image associated with this item as a string\n"
    "@return A base64-encoded image file (in PNG format)\n"
  ) +
  gsi::method ("image_str=", &rdb::Item::set_image_str, gsi::arg ("image"),
    "@brief Sets the image from a string\n"
    "@param image A base64-encoded image file (preferably in PNG format)\n"
  ) +
#if defined(HAVE_PNG)
  gsi::method ("image_pixels", &rdb::Item::image_pixels,
    "@brief Gets the attached image as a PixelBuffer object\n"
    "\n"
    "This method has been added in version 0.28."
  ) +
  gsi::method ("image=", static_cast<void (rdb::Item::*) (const tl::PixelBuffer &)> (&rdb::Item::set_image),
    "@brief Sets the attached image from a PixelBuffer object\n"
    "\n"
    "This method has been added in version 0.28."
  ) +
#endif
  /* Not supported yet:
  gsi::method ("multiplicity", &rdb::Item::multiplicity, 
    "@brief Gets the item's multiplicity\n"
    "The multiplicity of an item is the concept of giving an item a \"weight\", i.e. if an item represents an "
    "certain number of original shapes. The multiplicity is an integer that is added to the total number of items "
    "for this specific item instead of simply counting the items. "
    "@return The multiplicity\n"
  ) +
  gsi::method ("multiplicity=", &rdb::Item::set_multiplicity, gsi::arg ("multiplicity"),
    "@brief Sets the item's multiplicity\n"
  ) +
  */
  gsi::method_ext ("add_value", &add_value, gsi::arg ("value"),
    "@brief Adds a value object to the values of this item\n"
    "@param value The value to add.\n"
  ) +
  gsi::method_ext ("add_value", &add_value_t<db::DPolygon>, gsi::arg ("value"),
    "@brief Adds a polygon object to the values of this item\n"
    "@param value The polygon to add.\n"
    "This method has been introduced in version 0.25 as a convenience method."
  ) +
  gsi::method_ext ("add_value", &add_value_t<db::DBox>, gsi::arg ("value"),
    "@brief Adds a box object to the values of this item\n"
    "@param value The box to add.\n"
    "This method has been introduced in version 0.25 as a convenience method."
  ) +
  gsi::method_ext ("add_value", &add_value_t<db::DEdge>, gsi::arg ("value"),
    "@brief Adds an edge object to the values of this item\n"
    "@param value The edge to add.\n"
    "This method has been introduced in version 0.25 as a convenience method."
  ) +
  gsi::method_ext ("add_value", &add_value_t<db::DEdgePair>, gsi::arg ("value"),
    "@brief Adds an edge pair object to the values of this item\n"
    "@param value The edge pair to add.\n"
    "This method has been introduced in version 0.25 as a convenience method."
  ) +
  gsi::method_ext ("add_value", &add_value_t<std::string>, gsi::arg ("value"),
    "@brief Adds a string object to the values of this item\n"
    "@param value The string to add.\n"
    "This method has been introduced in version 0.25 as a convenience method."
  ) +
  gsi::method_ext ("add_value", &add_value_t<double>, gsi::arg ("value"),
    "@brief Adds a numeric value to the values of this item\n"
    "@param value The value to add.\n"
    "This method has been introduced in version 0.25 as a convenience method."
  ) +
  gsi::method_ext ("add_value", &add_value_from_shape, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Adds a geometrical value object from a shape\n"
    "@param value The shape object from which to take the geometrical object.\n"
    "@param trans The transformation to apply.\n"
    "\n"
    "The transformation can be used to convert database units to micron units.\n"
    "\n"
    "This method has been introduced in version 0.25.3."
  ) +
  gsi::method_ext ("clear_values", &clear_values,
    "@brief Removes all values from this item\n"
  ) +
  gsi::iterator_ext ("each_value", &begin_values, &end_values,
    "@brief Iterates over all values\n"
  ),
  "@brief An item inside the report database\n"
  "An item is the basic information entity in the RDB. It is associated with a cell and a category. It can be "
  "assigned values which encapsulate other objects such as strings and geometrical objects. In addition, items "
  "can be assigned an image (i.e. a screenshot image) and tags which are basically boolean flags that can be "
  "defined freely."
);

// ---------------------------------------------------------------
//  rdb::Database binding

rdb::Database *create_rdb (const std::string &name)
{
  rdb::Database *rdb = new rdb::Database ();
  rdb->set_name (name);
  return rdb;
}

rdb::id_type database_tag_id (const rdb::Database *db, const std::string &name)
{
  return db->tags ().tag (name, false).id ();
}

rdb::id_type database_user_tag_id (const rdb::Database *db, const std::string &name)
{
  return db->tags ().tag (name, true).id ();
}

rdb::Items::const_iterator database_items_begin (const rdb::Database *db)
{
  return db->items ().begin ();
}

rdb::Items::const_iterator database_items_end (const rdb::Database *db)
{
  return db->items ().end ();
}

ItemRefUnwrappingIterator database_items_begin_cell (const rdb::Database *db, rdb::id_type cell_id)
{
  return db->items_by_cell (cell_id).first;
}

ItemRefUnwrappingIterator database_items_end_cell (const rdb::Database *db, rdb::id_type cell_id)
{
  return db->items_by_cell (cell_id).second;
}

ItemRefUnwrappingIterator database_items_begin_cat (const rdb::Database *db, rdb::id_type cat_id)
{
  return db->items_by_category (cat_id).first;
}

ItemRefUnwrappingIterator database_items_end_cat (const rdb::Database *db, rdb::id_type cat_id)
{
  return db->items_by_category (cat_id).second;
}

ItemRefUnwrappingIterator database_items_begin_cc (const rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id)
{
  return db->items_by_cell_and_category (cell_id, cat_id).first;
}

ItemRefUnwrappingIterator database_items_end_cc (const rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id)
{
  return db->items_by_cell_and_category (cell_id, cat_id).second;
}

rdb::Categories::const_iterator database_begin_categories (const rdb::Database *db)
{
  return db->categories ().begin ();
}

rdb::Categories::const_iterator database_end_categories (const rdb::Database *db)
{
  return db->categories ().end ();
}

rdb::Cells::const_iterator database_begin_cells (const rdb::Database *db)
{
  return db->cells ().begin ();
}

rdb::Cells::const_iterator database_end_cells (const rdb::Database *db)
{
  return db->cells ().end ();
}

const std::string &database_tag_name (const rdb::Database *db, rdb::id_type tag)
{
  return db->tags ().tag (tag).name ();
}

const std::string &database_tag_description (const rdb::Database *db, rdb::id_type tag)
{
  return db->tags ().tag (tag).description ();
}

void database_set_tag_description (rdb::Database *db, rdb::id_type tag, const std::string &d)
{
  db->set_tag_description (tag, d);
}

void create_items_from_polygon_array (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const std::vector<db::Polygon> &collection)
{
  rdb::create_items_from_sequence (db, cell_id, cat_id, trans, collection.begin (), collection.end ());
}

void create_items_from_edge_array (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const std::vector<db::Edge> &collection)
{
  rdb::create_items_from_sequence (db, cell_id, cat_id, trans, collection.begin (), collection.end ());
}

void create_items_from_edge_pair_array (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const std::vector<db::EdgePair> &collection)
{
  rdb::create_items_from_sequence (db, cell_id, cat_id, trans, collection.begin (), collection.end ());
}

static rdb::Item *create_item (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id)
{
  if (! db->cell_by_id (cell_id)) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Not a valid cell ID: %d")), cell_id));
  }
  if (! db->category_by_id (cat_id)) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Not a valid category ID: %d")), cat_id));
  }
  return db->create_item (cell_id, cat_id);
}

static rdb::Item *create_item_from_objects (rdb::Database *db, rdb::Cell *cell, rdb::Category *cat)
{
  if (cell && cat) {
    return db->create_item (cell->id (), cat->id ());
  } else {
    return 0;
  }
}

Class<rdb::Database> decl_ReportDatabase ("rdb", "ReportDatabase",
  gsi::constructor ("new", &create_rdb, gsi::arg ("name"),
    "@brief Creates a report database\n"
    "@param name The name of the database\n"
    "The name of the database will be used in the user interface to refer to a certain database."
  ) + 
  gsi::method ("description", &rdb::Database::description, 
    "@brief Gets the databases description\n"
    "The description is a general purpose string that is supposed to further describe the database and its content "
    "in a human-readable form.\n"
    "@return The description string\n"
  ) +
  gsi::method ("description=", &rdb::Database::set_description, gsi::arg ("desc"),
    "@brief Sets the databases description\n"
    "@param desc The description string\n"
  ) + 
  gsi::method ("generator", &rdb::Database::generator, 
    "@brief Gets the databases generator\n"
    "The generator string describes how the database was created, i.e. DRC tool name and tool options.\n"
    "In a later version this will allow re-running the tool that created the report.\n"
    "@return The generator string\n"
  ) +
  gsi::method ("generator=", &rdb::Database::set_generator, gsi::arg ("generator"),
    "@brief Sets the generator string\n"
    "@param generator The generator string\n"
  ) + 
  gsi::method ("filename", &rdb::Database::filename, 
    "@brief Gets the file name and path where the report database is stored\n"
    "This property is set when a database is saved or loaded. It cannot be set manually.\n"
    "@return The file name and path\n"
  ) +
  gsi::method ("name", &rdb::Database::name, 
    "@brief Gets the database name\n"
    "The name of the database is supposed to identify the database within a layout view context. "
    "The name is modified to be unique when a database is entered into a layout view. "
    "@return The database name\n"
  ) +
  gsi::method ("top_cell_name", &rdb::Database::top_cell_name, 
    "@brief Gets the top cell name\n"
    "The top cell name identifies the top cell of the design for which the report was generated. "
    "This property must be set to establish a proper hierarchical context for a hierarchical report database. "
    "@return The top cell name\n"
  ) +
  gsi::method ("top_cell_name=", &rdb::Database::set_top_cell_name, gsi::arg ("cell_name"),
    "@brief Sets the top cell name string\n"
    "@param cell_name The top cell name\n"
  ) + 
  gsi::method ("original_file", &rdb::Database::original_file, 
    "@brief Gets the original file name and path\n"
    "The original file name is supposed to describe the file from which this report database was generated. "
    "@return The original file name and path\n"
  ) +
  gsi::method ("original_file=", &rdb::Database::set_original_file, gsi::arg ("path"),
    "@brief Sets the original file name and path\n"
    "@param path The path\n"
  ) +
  gsi::method_ext ("tag_id", &database_tag_id, gsi::arg ("name"),
    "@brief Gets the tag ID for a given tag name\n"
    "@param name The tag name\n"
    "@return The corresponding tag ID\n"
    "Tags are used to tag items in the database and to specify tagged (named) values. "
    "This method will always succeed and the tag will be created if it does not exist yet. "
    "Tags are basically names. There are user tags (for free assignment) and system tags "
    "which are used within the system. Both are separated to avoid name clashes.\n"
    "\n"
    "\\tag_id handles system tags while \\user_tag_id handles user tags.\n"
  ) +
  gsi::method_ext ("user_tag_id", &database_user_tag_id, gsi::arg ("name"),
    "@brief Gets the tag ID for a given user tag name\n"
    "@param name The user tag name\n"
    "@return The corresponding tag ID\n"
    "This method will always succeed and the tag will be created if it does not exist yet. "
    "See \\tag_id for a details about tags.\n"
    "\n"
    "This method has been added in version 0.24.\n"
  ) +
  gsi::method_ext ("set_tag_description", &database_set_tag_description, gsi::arg ("tag_id"), gsi::arg ("description"),
    "@brief Sets the tag description for the given tag ID\n"
    "@param tag_id The ID of the tag\n"
    "@param description The description string\n"
    "See \\tag_id for a details about tags.\n"
  ) +
  gsi::method_ext ("tag_description", &database_tag_description, gsi::arg ("tag_id"),
    "@brief Gets the tag description for the given tag ID\n"
    "@param tag_id The ID of the tag\n"
    "@return The description string\n"
    "See \\tag_id for a details about tags.\n"
  ) +
  gsi::method_ext ("tag_name", &database_tag_name, gsi::arg ("tag_id"),
    "@brief Gets the tag name for the given tag ID\n"
    "@param tag_id The ID of the tag\n"
    "@return The name of the tag\n"
    "See \\tag_id for a details about tags.\n\n"
    "This method has been introduced in version 0.24.10."
  ) +
  gsi::iterator_ext ("each_category", &database_begin_categories, &database_end_categories,
    "@brief Iterates over all top-level categories\n"
  ) +
  gsi::method ("create_category", (rdb::Category *(rdb::Database::*) (const std::string &)) &rdb::Database::create_category, gsi::arg ("name"),
    "@brief Creates a new top level category\n"
    "@param name The name of the category\n"
  ) +
  gsi::method ("create_category", (rdb::Category *(rdb::Database::*) (rdb::Category *, const std::string &)) &rdb::Database::create_category, gsi::arg ("parent"), gsi::arg ("name"),
    "@brief Creates a new sub-category\n"
    "@param parent The category under which the category should be created\n"
    "@param name The name of the category\n"
  ) +
  gsi::method ("category_by_path", &rdb::Database::category_by_name, gsi::arg ("path"),
    "@brief Gets a category by path\n"
    "@param path The full path to the category starting from the top level (subcategories separated by dots)\n"
    "@return The (const) category object or nil if the name is not valid\n"
  ) +
  gsi::method ("category_by_id", &rdb::Database::category_by_id, gsi::arg ("id"),
    "@brief Gets a category by ID\n"
    "@return The (const) category object or nil if the ID is not valid\n"
  ) +
  gsi::method ("create_cell", (rdb::Cell *(rdb::Database::*) (const std::string &)) &rdb::Database::create_cell, gsi::arg ("name"),
    "@brief Creates a new cell\n"
    "@param name The name of the cell\n"
  ) +
  gsi::method ("create_cell", (rdb::Cell *(rdb::Database::*) (const std::string &, const std::string &)) &rdb::Database::create_cell, gsi::arg ("name"), gsi::arg ("variant"),
    "@brief Creates a new cell, potentially as a variant for a cell with the same name\n"
    "@param name The name of the cell\n"
    "@param variant The variant name of the cell\n"
  ) +
  gsi::method ("variants", &rdb::Database::variants, gsi::arg ("name"),
    "@brief Gets the variants for a given cell name\n"
    "@param name The basic name of the cell\n"
    "@return An array of ID's representing cells that are variants for the given base name\n"
  ) +
  gsi::method ("cell_by_qname", &rdb::Database::cell_by_qname, gsi::arg ("qname"),
    "@brief Returns the cell for a given qualified name\n"
    "@param qname The qualified name of the cell (name plus variant name optionally)\n"
    "@return The cell object or nil if no such cell exists\n"
  ) +
  gsi::method ("cell_by_id", &rdb::Database::cell_by_id, gsi::arg ("id"),
    "@brief Returns the cell for a given ID\n"
    "@param id The ID of the cell\n"
    "@return The cell object or nil if no cell with that ID exists\n"
  ) +
  gsi::iterator_ext ("each_cell", &database_begin_cells, &database_end_cells,
    "@brief Iterates over all cells\n"
  ) +
  gsi::method ("num_items", (size_t (rdb::Database::*) () const) &rdb::Database::num_items,
    "@brief Returns the number of items inside the database\n"
    "@return The total number of items\n"
  ) +
  gsi::method ("num_items_visited", (size_t (rdb::Database::*) () const) &rdb::Database::num_items_visited,
    "@brief Returns the number of items already visited inside the database\n"
    "@return The total number of items already visited\n"
  ) +
  gsi::method ("num_items", (size_t (rdb::Database::*) (rdb::id_type, rdb::id_type) const) &rdb::Database::num_items, gsi::arg ("cell_id"), gsi::arg ("category_id"),
    "@brief Returns the number of items inside the database for a given cell/category combination\n"
    "@param cell_id The ID of the cell for which to retrieve the number\n"
    "@param category_id The ID of the category for which to retrieve the number\n"
    "@return The total number of items for the given cell and the given category\n"
  ) +
  gsi::method ("num_items_visited", (size_t (rdb::Database::*) (rdb::id_type, rdb::id_type) const) &rdb::Database::num_items_visited, gsi::arg ("cell_id"), gsi::arg ("category_id"),
    "@brief Returns the number of items visited already for a given cell/category combination\n"
    "@param cell_id The ID of the cell for which to retrieve the number\n"
    "@param category_id The ID of the category for which to retrieve the number\n"
    "@return The total number of items visited for the given cell and the given category\n"
  ) +
  gsi::method_ext ("create_item", &create_item, gsi::arg ("cell_id"), gsi::arg ("category_id"),
    "@brief Creates a new item for the given cell/category combination\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "\n"
    "A more convenient method that takes cell and category objects instead of ID's is the "
    "other version of \\create_item.\n"
  ) +
  gsi::method_ext ("create_item", &create_item_from_objects, gsi::arg ("cell"), gsi::arg ("category"),
    "@brief Creates a new item for the given cell/category combination\n"
    "@param cell The cell to which the item is associated\n"
    "@param category The category to which the item is associated\n"
    "\n"
    "This convenience method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("create_items", &rdb::create_items_from_iterator, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("iter"), gsi::arg ("with_properties", true),
    "@brief Creates new items from a shape iterator\n"
    "This method takes the shapes from the given iterator and produces items from them.\n"
    "It accepts various kind of shapes, such as texts, polygons, boxes and paths and "
    "converts them to corresponding items. This method will produce a flat version of the shapes iterated by the shape iterator. "
    "A similar method, which is intended for production of polygon or edge error layers and also provides hierarchical database "
    "construction is \\RdbCategory#scan_shapes.\n"
    "\n"
    "This method has been introduced in version 0.25.3. The 'with_properties' argument has been added in version 0.28.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param iter The iterator (a \\RecursiveShapeIterator object) from which to take the items\n"
    "@param with_properties If true, user properties will be turned into tagged values as well\n"
  ) +
  gsi::method_ext ("create_item", &rdb::create_item_from_shape, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("shape"), gsi::arg ("with_properties", true),
    "@brief Creates a new item from a single shape\n"
    "This method produces an item from the given shape.\n"
    "It accepts various kind of shapes, such as texts, polygons, boxes and paths and "
    "converts them to a corresponding item. The transformation argument can be used to "
    "supply the transformation that applies the database unit for example.\n"
    "\n"
    "This method has been introduced in version 0.25.3. The 'with_properties' argument has been added in version 0.28.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param shape The shape to take the geometrical object from\n"
    "@param trans The transformation to apply\n"
    "@param with_properties If true, user properties will be turned into tagged values as well\n"
  ) +
  gsi::method_ext ("create_items", &rdb::create_items_from_shapes, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("shapes"), gsi::arg ("with_properties", true),
    "@brief Creates new items from a shape container\n"
    "This method takes the shapes from the given container and produces items from them.\n"
    "It accepts various kind of shapes, such as texts, polygons, boxes and paths and "
    "converts them to corresponding items. The transformation argument can be used to "
    "supply the transformation that applies the database unit for example.\n"
    "\n"
    "This method has been introduced in version 0.25.3. The 'with_properties' argument has been added in version 0.28.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param shapes The shape container from which to take the items\n"
    "@param trans The transformation to apply\n"
    "@param with_properties If true, user properties will be turned into tagged values as well\n"
  ) +
  gsi::method_ext ("#create_items", &rdb::create_items_from_region, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("region"),
    "@brief Creates new polygon items for the given cell/category combination\n"
    "For each polygon in the region a single item will be created. The value of the item will be this "
    "polygon.\n"
    "A transformation can be supplied which can be used for example to convert the "
    "object's dimensions to micron units by scaling by the database unit.\n"
    "\n"
    "This method will also produce a flat version of the shapes inside the region. "
    "\\RdbCategory#scan_collection is a similar method which also supports construction of "
    "hierarchical databases from deep regions.\n"
    "\n"
    "This method has been introduced in version 0.23. It has been deprecated in favor of \\RdbCategory#scan_collection in version 0.28.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param trans The transformation to apply\n"
    "@param region The region (a \\Region object) containing the polygons for which to create items\n"
  ) +
  gsi::method_ext ("#create_items", &rdb::create_items_from_edges, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("edges"),
    "@brief Creates new edge items for the given cell/category combination\n"
    "For each edge a single item will be created. The value of the item will be this "
    "edge.\n"
    "A transformation can be supplied which can be used for example to convert the "
    "object's dimensions to micron units by scaling by the database unit.\n"
    "\n"
    "This method will also produce a flat version of the edges inside the edge collection. "
    "\\RdbCategory#scan_collection is a similar method which also supports construction of "
    "hierarchical databases from deep edge collections.\n"
    "\n"
    "This method has been introduced in version 0.23. It has been deprecated in favor of \\RdbCategory#scan_collection in version 0.28.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param trans The transformation to apply\n"
    "@param edges The list of edges (an \\Edges object) for which the items are created\n"
  ) +
  gsi::method_ext ("#create_items", &rdb::create_items_from_edge_pairs, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("edge_pairs"),
    "@brief Creates new edge pair items for the given cell/category combination\n"
    "For each edge pair a single item will be created. The value of the item will be this "
    "edge pair.\n"
    "A transformation can be supplied which can be used for example to convert the "
    "object's dimensions to micron units by scaling by the database unit.\n"
    "\n"
    "This method will also produce a flat version of the edge pairs inside the edge pair collection. "
    "\\RdbCategory#scan_collection is a similar method which also supports construction of "
    "hierarchical databases from deep edge pair collections.\n"
    "\n"
    "This method has been introduced in version 0.23. It has been deprecated in favor of \\RdbCategory#scan_collection in version 0.28.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param trans The transformation to apply\n"
    "@param edges The list of edge pairs (an \\EdgePairs object) for which the items are created\n"
  ) +
  gsi::method_ext ("create_items", &create_items_from_polygon_array, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("array"),
    "@brief Creates new polygon items for the given cell/category combination\n"
    "For each polygon a single item will be created. The value of the item will be this "
    "polygon.\n"
    "A transformation can be supplied which can be used for example to convert the "
    "object's dimensions to micron units by scaling by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param trans The transformation to apply\n"
    "@param polygons The list of polygons for which the items are created\n"
  ) +
  gsi::method_ext ("create_items", &create_items_from_edge_array, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("array"),
    "@brief Creates new edge items for the given cell/category combination\n"
    "For each edge a single item will be created. The value of the item will be this "
    "edge.\n"
    "A transformation can be supplied which can be used for example to convert the "
    "object's dimensions to micron units by scaling by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param trans The transformation to apply\n"
    "@param edges The list of edges for which the items are created\n"
  ) +
  gsi::method_ext ("create_items", &create_items_from_edge_pair_array, gsi::arg ("cell_id"), gsi::arg ("category_id"), gsi::arg ("trans"), gsi::arg ("array"),
    "@brief Creates new edge pair items for the given cell/category combination\n"
    "For each edge pair a single item will be created. The value of the item will be this "
    "edge pair.\n"
    "A transformation can be supplied which can be used for example to convert the "
    "object's dimensions to micron units by scaling by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
    "\n"
    "@param cell_id The ID of the cell to which the item is associated\n"
    "@param category_id The ID of the category to which the item is associated\n"
    "@param trans The transformation to apply\n"
    "@param edge_pairs The list of edge_pairs for which the items are created\n"
  ) +
  gsi::method ("is_modified?", &rdb::Database::is_modified,
    "@brief Returns a value indicating whether the database has been modified\n"
  ) +
  gsi::method ("reset_modified", &rdb::Database::reset_modified,
    "@brief Reset the modified flag\n"
  ) +
  gsi::iterator_ext ("each_item", &database_items_begin, &database_items_end,
    "@brief Iterates over all items inside the database\n"
  ) +
  gsi::iterator_ext ("each_item_per_cell", &database_items_begin_cell, &database_items_end_cell, gsi::arg ("cell_id"),
    "@brief Iterates over all items inside the database which are associated with the given cell\n"
    "@param cell_id The ID of the cell for which all associated items should be retrieved\n"
  ) +
  gsi::iterator_ext ("each_item_per_category", &database_items_begin_cat, &database_items_end_cat, gsi::arg ("category_id"),
    "@brief Iterates over all items inside the database which are associated with the given category\n"
    "@param category_id The ID of the category for which all associated items should be retrieved\n"
  ) +
  gsi::iterator_ext ("each_item_per_cell_and_category", &database_items_begin_cc, &database_items_end_cc, gsi::arg ("cell_id"), gsi::arg ("category_id"),
    "@brief Iterates over all items inside the database which are associated with the given cell and category\n"
    "@param cell_id The ID of the cell for which all associated items should be retrieved\n"
    "@param category_id The ID of the category for which all associated items should be retrieved\n"
  ) +
  gsi::method ("set_item_visited", &rdb::Database::set_item_visited, gsi::arg ("item"), gsi::arg ("visited"),
    "@brief Modifies the visited state of an item\n"
    "@param item The item to modify\n"
    "@param visited True to set the item to visited state, false otherwise\n"
  ) +
  gsi::method ("load", &rdb::Database::load, gsi::arg ("filename"),
    "@brief Loads the database from the given file\n"
    "@param filename The file from which to load the database\n"
    "The reader recognizes the format automatically and will choose the appropriate decoder. 'gzip' compressed files are uncompressed "
    "automatically.\n"
  ) + 
  gsi::method ("save", &rdb::Database::save, gsi::arg ("filename"),
    "@brief Saves the database to the given file\n"
    "@param filename The file to which to save the database\n"
    "The database is always saved in KLayout's XML-based format.\n"
  ),
  "@brief The report database object\n"
  "A report database is organized around a set of items which are associated with cells and categories. "
  "Categories can be organized hierarchically by created sub-categories of other categories. "
  "Cells are associated with layout database cells and can come with a example instantiation if the layout "
  "database does not allow a unique association of the cells.\n"
  "Items in the database can have a variety of attributes: values, tags and an image object. Values are "
  "geometrical objects for example. Tags are a set of boolean flags and an image can be attached to an item "
  "to provide a screenshot for visualization for example.\n"
  "This is the main report database object. The basic use case of this object is to create one inside a \\LayoutView and "
  "populate it with items, cell and categories or load it from a file. Another use case is to create a standalone "
  "ReportDatabase object and use the methods provided to perform queries or to populate it.\n"
);
  
static void tp_output_rdb (db::TilingProcessor *proc, const std::string &name, rdb::Database &rdb, rdb::id_type cell_id, rdb::id_type category_id)
{
  proc->output (name, 0, new rdb::TiledRdbOutputReceiver (&rdb, cell_id, category_id), db::ICplxTrans ());
}

//  extend the db::TilingProcessor with the ability to feed images
static
gsi::ClassExt<db::TilingProcessor> tiling_processor_ext (
  method_ext ("output", &tp_output_rdb, gsi::arg ("name"), gsi::arg ("rdb"), gsi::arg ("cell_id"), gsi::arg ("category_id"),
    "@brief Specifies output to a report database\n"
    "This method will establish an output channel for the processor. The output sent to that channel "
    "will be put into the report database given by the \"rdb\" parameter. \"cell_id\" specifies the "
    "cell and \"category_id\" the category to use.\n"
    "\n"
    "The name is the name which must be used in the _output function of the scripts in order to "
    "address that channel.\n"
  ),
  ""
);

}
