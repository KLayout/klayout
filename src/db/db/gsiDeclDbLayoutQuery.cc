
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
#include "dbLayoutQuery.h"

namespace gsi
{

static db::LayoutQuery *new_query (const std::string &query)
{
  return new db::LayoutQuery (query);
}

static std::vector<std::string> query_prop_names (const db::LayoutQuery *q)
{
  size_t pcount = q->properties ();
  std::vector<std::string> pn;
  pn.reserve (pcount);
  for (size_t i = 0; i < pcount; ++i) {
    pn.push_back (q->property_name (int (i)));
  }
  return pn;
}

struct LayoutQueryIteratorWrapper
{
  typedef db::LayoutQueryIterator &reference;
  //  Dummy declarations
  typedef std::forward_iterator_tag iterator_category;
  typedef void value_type;
  typedef void difference_type;
  typedef void pointer;

  LayoutQueryIteratorWrapper (const db::LayoutQuery &q, const db::Layout *layout, tl::Eval *eval)
    : mp_iter (new db::LayoutQueryIterator (q, layout, eval))
  {
    //  .. nothing yet ..
  }

  reference operator* () const
  {
    return const_cast<db::LayoutQueryIterator &> (*this->mp_iter);
  }

  bool at_end () const
  {
    return mp_iter->at_end ();
  }

  void operator++ ()
  {
    ++*mp_iter;
  }

private:
  tl::shared_ptr<db::LayoutQueryIterator> mp_iter;
};

static LayoutQueryIteratorWrapper iterate (const db::LayoutQuery *q, const db::Layout *layout, tl::Eval *eval)
{
  return LayoutQueryIteratorWrapper (*q, layout, eval);
}

static tl::Variant iter_get (db::LayoutQueryIterator *iter, const std::string &name)
{
  tl::Variant v;
  if (iter->get (name, v)) {
    return v;
  } else {
    return tl::Variant ();
  }
}

template <const char *NAME> tl::Variant iter_get_named (db::LayoutQueryIterator *iter)
{
  return iter_get (iter, NAME);
}

template <const char *NAME>
gsi::Methods make_shortcut_method ()
{
  return gsi::method_ext (NAME, &iter_get_named<NAME>,
    std::string ("@brief A shortcut for 'get(\"") + NAME + std::string ("\")'\n")
  );
}

//  String literals cannot be used as template arguments but objects with external linkage can be:
char data_query_property_name[]               = "data";
char shape_query_property_name[]              = "shape";
char layer_index_query_property_name[]        = "layer_index";
char inst_query_property_name[]               = "inst";
char path_trans_query_property_name[]         = "path_trans";
char path_dtrans_query_property_name[]        = "path_dtrans";
char trans_query_property_name[]              = "trans";
char dtrans_query_property_name[]             = "dtrans";
char cell_index_query_property_name[]         = "cell_index";
char cell_query_property_name[]               = "cell";
char parent_cell_index_query_property_name[]  = "parent_cell_index";
char parent_cell_query_property_name[]        = "parent_cell";
char initial_cell_index_query_property_name[] = "initial_cell_index";
char initial_cell_query_property_name[]       = "initial_cell";

Class<db::LayoutQueryIterator> decl_LayoutQueryIterator ("db", "LayoutQueryIterator",
  gsi::method ("layout", &db::LayoutQueryIterator::layout,
    "@brief Gets the layout the query acts on\n"
  ) +
  gsi::method ("query", &db::LayoutQueryIterator::query,
    "@brief Gets the query the iterator follows on\n"
  ) +
  gsi::method_ext ("get", &iter_get, gsi::arg ("name"),
    "@brief Gets the query property with the given name\n"
    "The query properties available can be obtained from the query object using \\LayoutQuery#property_names.\n"
    "Some shortcut methods are available. For example, the \\data method provides a shortcut for 'get(\"data\")'.\n"
    "\n"
    "If a property with the given name is not available, nil will be returned."
  ) +
  make_shortcut_method<data_query_property_name>() +
  make_shortcut_method<shape_query_property_name>() +
  make_shortcut_method<layer_index_query_property_name>() +
  make_shortcut_method<inst_query_property_name>() +
  make_shortcut_method<path_trans_query_property_name>() +
  make_shortcut_method<path_dtrans_query_property_name>() +
  make_shortcut_method<trans_query_property_name>() +
  make_shortcut_method<dtrans_query_property_name>() +
  make_shortcut_method<cell_index_query_property_name>() +
  make_shortcut_method<cell_query_property_name>() +
  make_shortcut_method<parent_cell_index_query_property_name>() +
  make_shortcut_method<parent_cell_query_property_name>() +
  make_shortcut_method<initial_cell_index_query_property_name>() +
  make_shortcut_method<initial_cell_query_property_name>()
  ,
  "@brief Provides the results of the query\n"
  "\n"
  "This object is used by \\LayoutQuery#each to deliver the results of a query in an iterative fashion. "
  "See \\LayoutQuery for a detailed description of the query interface.\n"
  "\n"
  "The LayoutQueryIterator class has been introduced in version 0.25."
);

Class<db::LayoutQuery> decl_LayoutQuery ("db", "LayoutQuery",
  gsi::constructor ("new", &new_query, gsi::arg ("query"),
    "@brief Creates a new query object from the given query string\n"
  ) +
  gsi::method_ext ("property_names", &query_prop_names,
    "@brief Gets a list of property names available.\n"
    "The list of properties available from the query depends on the nature of the query. "
    "This method allows detection of the properties available. Within the query, all of these "
    "properties can be obtained from the query iterator using \\LayoutQueryIterator#get.\n"
  ) +
  gsi::method ("execute", &db::LayoutQuery::execute, gsi::arg("layout"), gsi::arg ("context", (tl::Eval *) 0, "nil"),
    "@brief Executes the query\n"
    "\n"
    "This method can be used to execute \"active\" queries such\n"
    "as \"delete\" or \"with ... do\".\n"
    "It is basically equivalent to iterating over the query until it is\n"
    "done.\n"
    "\n"
    "The context argument allows supplying an expression execution context. This context can be used for "
    "example to supply variables for the execution. It has been added in version 0.26.\n"
  ) +
  gsi::iterator_ext ("each", &iterate, gsi::arg ("layout"), gsi::arg ("context", (tl::Eval *) 0, "nil"),
    "@brief Executes the query and delivered the results iteratively.\n"
    "The argument to the block is a \\LayoutQueryIterator object which can be "
    "asked for specific results.\n"
    "\n"
    "The context argument allows supplying an expression execution context. This context can be used for "
    "example to supply variables for the execution. It has been added in version 0.26.\n"
  ),
  "@brief A layout query\n"
  "Layout queries are the backbone of the \"Search & replace\" feature. Layout queries allow retrieval of "
  "data from layouts and manipulation of layouts. This object provides script binding for this feature.\n"
  "Layout queries are used by first creating a query object. Depending on the nature of the query, either \\execute "
  "or \\each can be used to execute the query. \\execute will run the query and return once the query is finished. "
  "\\execute is useful for running queries that don't return results such as \"delete\" or \"with ... do\" queries.\n"
  "\\each can be used when the results of the query need to be retrieved.\n"
  "\n"
  "The \\each method will call a block a of code for every result available. It will provide a \\LayoutQueryIterator "
  "object that allows accessing the results of the query. Depending on the query, different attributes of the "
  "iterator object will be available. For example, \"select\" queries will fill the \"data\" attribute with an array of values "
  "corresponding to the columns of the selection.\n"
  "\n"
  "Here is some sample code:\n"
  "@code\n"
  "ly = RBA::CellView::active.layout\n"
  "q = RBA::LayoutQuery::new(\"select cell.name, cell.bbox from *\")\n"
  "q.each(ly) do |iter|\n"
  "  puts \"cell name: #{iter.data[0]}, bounding box: #{iter.data[1]}\"\n"
  "end\n"
  "@/code\n"
  "\n"
  "The LayoutQuery class has been introduced in version 0.25."
);

}

