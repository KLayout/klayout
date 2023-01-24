#
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
#include "dbRecursiveShapeIterator.h"
#include "dbRegion.h"

#include "tlGlobPattern.h"

#include <iterator>

namespace gsi
{

// ---------------------------------------------------------------
//  db::RecursiveShapeIterator binding

namespace {

/**
 *  @brief A wrapper that allows using "each" on the iterator
 */
class IteratorIterator
{
public:
  typedef db::RecursiveShapeIterator value_type;
  typedef db::RecursiveShapeIterator &reference;
  typedef db::RecursiveShapeIterator *pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  IteratorIterator (db::RecursiveShapeIterator *iter) : mp_iter (iter) { }
  bool at_end () const { return mp_iter->at_end (); }
  reference operator* () const { return *mp_iter; }
  void operator++ () { ++*mp_iter; }

private:
  db::RecursiveShapeIterator *mp_iter;
};

}

static db::RecursiveShapeIterator *new_si1 (const db::Layout &layout, const db::Cell &cell, unsigned int layer)
{
  return new db::RecursiveShapeIterator (layout, cell, layer);
}

static db::RecursiveShapeIterator *new_si2 (const db::Layout &layout, const db::Cell &cell, const std::vector<unsigned int> &layers)
{
  return new db::RecursiveShapeIterator (layout, cell, layers);
}

static db::RecursiveShapeIterator *new_si3 (const db::Layout &layout, const db::Cell &cell, unsigned int layer, const db::Box &box, bool overlapping)
{
  return new db::RecursiveShapeIterator (layout, cell, layer, box, overlapping);
}

static db::RecursiveShapeIterator *new_si3a (const db::Layout &layout, const db::Cell &cell, unsigned int layer, const db::Region &region, bool overlapping)
{
  return new db::RecursiveShapeIterator (layout, cell, layer, region, overlapping);
}

static db::RecursiveShapeIterator *new_si4 (const db::Layout &layout, const db::Cell &cell, const std::vector<unsigned int> &layers, const db::Box &box, bool overlapping)
{
  return new db::RecursiveShapeIterator (layout, cell, layers, box, overlapping);
}

static db::RecursiveShapeIterator *new_si4a (const db::Layout &layout, const db::Cell &cell, const std::vector<unsigned int> &layers, const db::Region &region, bool overlapping)
{
  return new db::RecursiveShapeIterator (layout, cell, layers, region, overlapping);
}

static IteratorIterator each (db::RecursiveShapeIterator *r)
{
  return IteratorIterator (r);
}

static db::DCplxTrans si_dtrans (const db::RecursiveShapeIterator *r)
{
  const db::Layout *ly = r->layout ();
  tl_assert (ly != 0);
  return db::CplxTrans (ly->dbu ()) * r->trans () * db::VCplxTrans (1.0 / ly->dbu ());
}

static db::DCplxTrans si_global_dtrans (const db::RecursiveShapeIterator *r)
{
  const db::Layout *ly = r->layout ();
  tl_assert (ly != 0);
  return db::CplxTrans (ly->dbu ()) * r->global_trans () * db::VCplxTrans (1.0 / ly->dbu ());
}

static db::DCplxTrans si_always_apply_dtrans (const db::RecursiveShapeIterator *r)
{
  const db::Layout *ly = r->layout ();
  tl_assert (ly != 0);
  return db::CplxTrans (ly->dbu ()) * r->always_apply () * db::VCplxTrans (1.0 / ly->dbu ());
}

static void si_set_global_dtrans (db::RecursiveShapeIterator *r, const db::DCplxTrans &gt)
{
  const db::Layout *ly = r->layout ();
  tl_assert (ly != 0);
  r->set_global_trans (db::VCplxTrans (1.0 / ly->dbu ()) * gt * db::CplxTrans (ly->dbu ()));
}

static void select_cells1 (db::RecursiveShapeIterator *r, const std::vector<db::cell_index_type> &cells)
{
  std::set<db::cell_index_type> cc;
  cc.insert (cells.begin (), cells.end ());
  r->select_cells (cc);
}

static void select_cells2 (db::RecursiveShapeIterator *r, const std::string &pattern)
{
  tl::GlobPattern p (pattern);
  std::set<db::cell_index_type> cc;
  for (db::Layout::const_iterator ci = r->layout ()->begin (); ci != r->layout ()->end (); ++ci) {
    if (p.match (r->layout ()->cell_name (ci->cell_index ()))) {
      cc.insert (ci->cell_index ());
    }
  }

  r->select_cells (cc);
}

static void unselect_cells1 (db::RecursiveShapeIterator *r, const std::vector<db::cell_index_type> &cells) 
{
  std::set<db::cell_index_type> cc;
  cc.insert (cells.begin (), cells.end ());
  r->unselect_cells (cc);
}

static void unselect_cells2 (db::RecursiveShapeIterator *r, const std::string &pattern)
{
  tl::GlobPattern p (pattern);
  std::set<db::cell_index_type> cc;
  for (db::Layout::const_iterator ci = r->layout ()->begin (); ci != r->layout ()->end (); ++ci) {
    if (p.match (r->layout ()->cell_name (ci->cell_index ()))) {
      cc.insert (ci->cell_index ());
    }
  }

  r->unselect_cells (cc);
}

static db::Region complex_region (const db::RecursiveShapeIterator *iter)
{
  if (iter->has_complex_region ()) {
    return iter->complex_region ();
  } else {
    return db::Region (iter->region ());
  }
}

static void enable_properties (db::RecursiveShapeIterator *c)
{
  c->apply_property_translator (db::PropertiesTranslator::make_pass_all ());
}

static void remove_properties (db::RecursiveShapeIterator *c)
{
  c->apply_property_translator (db::PropertiesTranslator::make_remove_all ());
}

static void filter_properties (db::RecursiveShapeIterator *c, const std::vector<tl::Variant> &keys)
{
  if (c->layout ()) {
    std::set<tl::Variant> kf;
    kf.insert (keys.begin (), keys.end ());
    c->apply_property_translator (db::PropertiesTranslator::make_filter (const_cast<db::Layout *> (c->layout ())->properties_repository (), kf));
  }
}

static void map_properties (db::RecursiveShapeIterator *c, const std::map<tl::Variant, tl::Variant> &map)
{
  if (c->layout ()) {
    c->apply_property_translator (db::PropertiesTranslator::make_key_mapper (const_cast<db::Layout *> (c->layout ())->properties_repository (), map));
  }
}

Class<db::RecursiveShapeIterator> decl_RecursiveShapeIterator ("db", "RecursiveShapeIterator",
  gsi::constructor ("new", &new_si1, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layer"),
    "@brief Creates a recursive, single-layer shape iterator.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param layer The layer (index) from which the shapes are taken\n"
    "\n"
    "This constructor creates a new recursive shape iterator which delivers the shapes of "
    "the given cell plus its children from the layer given by the layer index in the \"layer\" parameter.\n"
    "\n"
    "This constructor has been introduced in version 0.23.\n"
  ) +
  gsi::constructor ("new", &new_si2, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layers"),
    "@brief Creates a recursive, multi-layer shape iterator.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param layers The layer indexes from which the shapes are taken\n"
    "\n"
    "This constructor creates a new recursive shape iterator which delivers the shapes of "
    "the given cell plus its children from the layers given by the layer indexes in the \"layers\" parameter.\n"
    "While iterating use the \\layer method to retrieve the layer of the current shape.\n"
    "\n"
    "This constructor has been introduced in version 0.23.\n"
  ) +
  gsi::constructor ("new", &new_si3, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("box"), gsi::arg ("overlapping", false),
    "@brief Creates a recursive, single-layer shape iterator with a region.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param layer The layer (index) from which the shapes are taken\n"
    "@param box The search region\n"
    "@param overlapping If set to true, shapes overlapping the search region are reported, otherwise touching is sufficient\n"
    "\n"
    "This constructor creates a new recursive shape iterator which delivers the shapes of "
    "the given cell plus its children from the layer given by the layer index in the \"layer\" parameter.\n"
    "\n"
    "The search is confined to the region given by the \"box\" parameter. If \"overlapping\" is true, shapes whose "
    "bounding box is overlapping the search region are reported. If \"overlapping\" is false, shapes whose "
    "bounding box touches the search region are reported.\n"
    "\n"
    "This constructor has been introduced in version 0.23. The 'overlapping' parameter has been made optional in version 0.27.\n"
  ) +
  gsi::constructor ("new", &new_si3a, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("region"), gsi::arg ("overlapping", false),
    "@brief Creates a recursive, single-layer shape iterator with a region.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param layer The layer (index) from which the shapes are taken\n"
    "@param region The search region\n"
    "@param overlapping If set to true, shapes overlapping the search region are reported, otherwise touching is sufficient\n"
    "\n"
    "This constructor creates a new recursive shape iterator which delivers the shapes of "
    "the given cell plus its children from the layer given by the layer index in the \"layer\" parameter.\n"
    "\n"
    "The search is confined to the region given by the \"region\" parameter. The region needs to be a rectilinear region.\n"
    "If \"overlapping\" is true, shapes whose "
    "bounding box is overlapping the search region are reported. If \"overlapping\" is false, shapes whose "
    "bounding box touches the search region are reported.\n"
    "\n"
    "This constructor has been introduced in version 0.25. The 'overlapping' parameter has been made optional in version 0.27.\n"
  ) +
  gsi::constructor ("new", &new_si4, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layers"), gsi::arg ("box"), gsi::arg ("overlapping", false),
    "@brief Creates a recursive, multi-layer shape iterator with a region.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param layers The layer indexes from which the shapes are taken\n"
    "@param box The search region\n"
    "@param overlapping If set to true, shapes overlapping the search region are reported, otherwise touching is sufficient\n"
    "\n"
    "This constructor creates a new recursive shape iterator which delivers the shapes of "
    "the given cell plus its children from the layers given by the layer indexes in the \"layers\" parameter.\n"
    "While iterating use the \\layer method to retrieve the layer of the current shape.\n"
    "\n"
    "The search is confined to the region given by the \"box\" parameter. If \"overlapping\" is true, shapes whose "
    "bounding box is overlapping the search region are reported. If \"overlapping\" is false, shapes whose "
    "bounding box touches the search region are reported.\n"
    "\n"
    "This constructor has been introduced in version 0.23. The 'overlapping' parameter has been made optional in version 0.27.\n"
  ) +
  gsi::constructor ("new", &new_si4a, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("layers"), gsi::arg ("region"), gsi::arg ("overlapping", false),
    "@brief Creates a recursive, multi-layer shape iterator with a region.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param layers The layer indexes from which the shapes are taken\n"
    "@param region The search region\n"
    "@param overlapping If set to true, shapes overlapping the search region are reported, otherwise touching is sufficient\n"
    "\n"
    "This constructor creates a new recursive shape iterator which delivers the shapes of "
    "the given cell plus its children from the layers given by the layer indexes in the \"layers\" parameter.\n"
    "While iterating use the \\layer method to retrieve the layer of the current shape.\n"
    "\n"
    "The search is confined to the region given by the \"region\" parameter. The region needs to be a rectilinear region.\n"
    "If \"overlapping\" is true, shapes whose "
    "bounding box is overlapping the search region are reported. If \"overlapping\" is false, shapes whose "
    "bounding box touches the search region are reported.\n"
    "\n"
    "This constructor has been introduced in version 0.23. The 'overlapping' parameter has been made optional in version 0.27.\n"
  ) +
  gsi::iterator_ext ("each", &each,
    "@brief Native iteration\n"
    "This method enables native iteration, e.g.\n"
    "\n"
    "@code\n"
    "  iter = ... # RecursiveShapeIterator\n"
    "  iter.each do |i|\n"
    "     ... i is the iterator itself\n"
    "  end\n"
    "@/code\n"
    "\n"
    "This is slightly more convenient than the 'at_end' .. 'next' loop.\n"
    "\n"
    "This feature has been introduced in version 0.28.\n"
  ) +
  gsi::method ("max_depth=", (void (db::RecursiveShapeIterator::*) (int)) &db::RecursiveShapeIterator::max_depth, gsi::arg ("depth"),
    "@brief Specifies the maximum hierarchy depth to look into\n"
    "\n"
    "A depth of 0 instructs the iterator to deliver only shapes from the initial cell.\n"
    "The depth must be specified before the shapes are being retrieved.\n"
    "Setting the depth resets the iterator.\n"
  ) +
  gsi::method ("max_depth", (int (db::RecursiveShapeIterator::*) () const) &db::RecursiveShapeIterator::max_depth, 
    "@brief Gets the maximum hierarchy depth\n"
    "\n"
    "See \\max_depth= for a description of that attribute.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("min_depth=", (void (db::RecursiveShapeIterator::*) (int)) &db::RecursiveShapeIterator::min_depth, gsi::arg ("depth"),
    "@brief Specifies the minimum hierarchy depth to look into\n"
    "\n"
    "A depth of 0 instructs the iterator to deliver shapes from the top level.\n"
    "1 instructs to deliver shapes from the first child level.\n"
    "The minimum depth must be specified before the shapes are being retrieved.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("min_depth", (int (db::RecursiveShapeIterator::*) () const) &db::RecursiveShapeIterator::min_depth,
    "@brief Gets the minimum hierarchy depth\n"
    "\n"
    "See \\min_depth= for a description of that attribute.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("reset", &db::RecursiveShapeIterator::reset,
    "@brief Resets the iterator to the initial state\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("reset_selection", &db::RecursiveShapeIterator::reset_selection, 
    "@brief Resets the selection to the default state\n"
    "\n"
    "In the initial state, the top cell and its children are selected. Child cells can be switched on and off "
    "together with their sub-hierarchy using \\select_cells and \\unselect_cells.\n"
    "\n"
    "This method will also reset the iterator.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("layout", &db::RecursiveShapeIterator::layout, 
    "@brief Gets the layout this iterator is connected to\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("top_cell", &db::RecursiveShapeIterator::top_cell, 
    "@brief Gets the top cell this iterator is connected to\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("global_trans=", &db::RecursiveShapeIterator::set_global_trans, gsi::arg ("t"),
    "@brief Sets the global transformation to apply to all shapes delivered\n"
    "The global transformation will be applied to all shapes delivered by biasing the \"trans\" attribute.\n"
    "The search regions apply to the coordinate space after global transformation.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("global_trans", &db::RecursiveShapeIterator::global_trans,
    "@brief Gets the global transformation to apply to all shapes delivered\n"
    "See also \\global_trans=.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("global_dtrans=", &si_set_global_dtrans,
    "@brief Sets the global transformation to apply to all shapes delivered (transformation in micrometer units)\n"
    "The global transformation will be applied to all shapes delivered by biasing the \"trans\" attribute.\n"
    "The search regions apply to the coordinate space after global transformation.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("global_dtrans", &si_global_dtrans,
    "@brief Gets the global transformation to apply to all shapes delivered (in micrometer units)\n"
    "See also \\global_dtrans=.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("always_apply_trans", &db::RecursiveShapeIterator::always_apply,
    "@brief Gets the global transformation if at top level, unity otherwise\n"
    "As the global transformation is only applicable on top level, use this method to transform shapes and instances into their local (cell-level) version "
    "while considering the global transformation properly.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("always_apply_dtrans", &si_always_apply_dtrans,
    "@brief Gets the global transformation if at top level, unity otherwise (micrometer-unit version)\n"
    "As the global transformation is only applicable on top level, use this method to transform shapes and instances into their local (cell-level) version "
    "while considering the global transformation properly.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("region", &db::RecursiveShapeIterator::region,
    "@brief Gets the basic region that this iterator is using\n"
    "The basic region is the overall box the region iterator iterates over. "
    "There may be an additional complex region that confines the region iterator. "
    "See \\complex_region for this attribute.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("complex_region", &complex_region,
    "@brief Gets the complex region that this iterator is using\n"
    "The complex region is the effective region (a \\Region object) that the "
    "iterator is selecting from the layout layers. This region can be a single box "
    "or a complex region.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("region=", (void (db::RecursiveShapeIterator::*)(const db::RecursiveShapeIterator::box_type &)) &db::RecursiveShapeIterator::set_region, gsi::arg ("box_region"),
    "@brief Sets the rectangular region that this iterator is iterating over\n"
    "See \\region for a description of this attribute.\n"
    "Setting a simple region will reset the complex region to a rectangle and reset the iterator to "
    "the beginning of the sequence."
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("region=", (void (db::RecursiveShapeIterator::*)(const db::RecursiveShapeIterator::region_type &)) &db::RecursiveShapeIterator::set_region, gsi::arg ("complex_region"),
    "@brief Sets the complex region that this iterator is using\n"
    "See \\complex_region for a description of this attribute. Setting the complex region will "
    "reset the basic region (see \\region) to the bounding box of the complex region and "
    "reset the iterator to the beginning of the sequence.\n"
    "\n"
    "This method overload has been introduced in version 0.25.\n"
  ) +
  gsi::method ("confine_region", (void (db::RecursiveShapeIterator::*)(const db::RecursiveShapeIterator::box_type &)) &db::RecursiveShapeIterator::confine_region, gsi::arg ("box_region"),
    "@brief Confines the region that this iterator is iterating over\n"
    "This method is similar to setting the region (see \\region=), but will confine any region (complex or simple) already set. "
    "Essentially it does a logical AND operation between the existing and given region. "
    "Hence this method can only reduce a region, not extend it.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("confine_region", (void (db::RecursiveShapeIterator::*)(const db::RecursiveShapeIterator::region_type &)) &db::RecursiveShapeIterator::confine_region, gsi::arg ("complex_region"),
    "@brief Confines the region that this iterator is iterating over\n"
    "This method is similar to setting the region (see \\region=), but will confine any region (complex or simple) already set. "
    "Essentially it does a logical AND operation between the existing and given region. "
    "Hence this method can only reduce a region, not extend it.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("overlapping?", &db::RecursiveShapeIterator::overlapping,
    "@brief Gets a flag indicating whether overlapping shapes are selected when a region is used\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("overlapping=", &db::RecursiveShapeIterator::set_overlapping, gsi::arg ("region"),
    "@brief Sets a flag indicating whether overlapping shapes are selected when a region is used\n"
    "\n"
    "If this flag is false, shapes touching the search region are returned.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("unselect_all_cells", &db::RecursiveShapeIterator::unselect_all_cells,
    "@brief Unselects all cells.\n"
    "\n"
    "This method will set the \"unselected\" mark on all cells. The effect is "
    "that subsequent calls of \\select_cells will select only the specified cells, not "
    "their children, because they are still unselected.\n"
    "\n"
    "This method will also reset the iterator.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("select_all_cells", &db::RecursiveShapeIterator::select_all_cells,
    "@brief Selects all cells.\n"
    "\n"
    "This method will set the \"selected\" mark on all cells. The effect is "
    "that subsequent calls of \\unselect_cells will unselect only the specified cells, not "
    "their children, because they are still unselected.\n"
    "\n"
    "This method will also reset the iterator.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("unselect_cells", &unselect_cells1, gsi::arg ("cells"),
    "@brief Unselects the given cells.\n"
    "\n"
    "This method will sets the \"unselected\" mark on the given cells. "
    "That means that these cells or their child cells will not be visited, unless "
    "they are marked as \"selected\" again with the \\select_cells method.\n"
    "\n"
    "The cells are given as a list of cell indexes.\n"
    "\n"
    "This method will also reset the iterator.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("unselect_cells", &unselect_cells2, gsi::arg ("cells"),
    "@brief Unselects the given cells.\n"
    "\n"
    "This method will sets the \"unselected\" mark on the given cells. "
    "That means that these cells or their child cells will not be visited, unless "
    "they are marked as \"selected\" again with the \\select_cells method.\n"
    "\n"
    "The cells are given as a glob pattern.\n"
    "A glob pattern follows the syntax of "
    "file names on the shell (i.e. \"A*\" are all cells starting with a letter \"A\").\n"
    "\n"
    "This method will also reset the iterator.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("select_cells", &select_cells1, gsi::arg ("cells"),
    "@brief Unselects the given cells.\n"
    "\n"
    "This method will sets the \"selected\" mark on the given cells. "
    "That means that these cells or their child cells are visited, unless "
    "they are marked as \"unselected\" again with the \\unselect_cells method.\n"
    "\n"
    "The cells are given as a list of cell indexes.\n"
    "\n"
    "This method will also reset the iterator.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("select_cells", &select_cells2, gsi::arg ("cells"),
    "@brief Unselects the given cells.\n"
    "\n"
    "This method will sets the \"selected\" mark on the given cells. "
    "That means that these cells or their child cells are visited, unless "
    "they are marked as \"unselected\" again with the \\unselect_cells method.\n"
    "\n"
    "The cells are given as a glob pattern.\n"
    "A glob pattern follows the syntax of "
    "file names on the shell (i.e. \"A*\" are all cells starting with a letter \"A\").\n"
    "\n"
    "This method will also reset the iterator.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("shape_flags=", (void (db::RecursiveShapeIterator::*)(unsigned int)) &db::RecursiveShapeIterator::shape_flags, gsi::arg ("flags"),
    "@brief Specifies the shape selection flags\n"
    "\n"
    "The flags are the same then being defined in \\Shapes (the default is RBA::Shapes::SAll).\n"
    "The flags must be specified before the shapes are being retrieved.\n"
    "Settings the shapes flags will reset the iterator.\n"
  ) +
  gsi::method ("shape_flags", (unsigned int (db::RecursiveShapeIterator::*)() const) &db::RecursiveShapeIterator::shape_flags,
    "@brief Gets the shape selection flags\n"
    "\n"
    "See \\shape_flags= for a description of that property.\n"
    "\n"
    "This getter has been introduced in version 0.28.\n"
  ) +
  gsi::method ("trans|#itrans", &db::RecursiveShapeIterator::trans,
    "@brief Gets the current transformation by which the shapes must be transformed into the initial cell\n"
    "\n"
    "The shapes delivered are not transformed. Instead, this transformation must be applied to \n"
    "get the shape in the coordinate system of the top cell.\n"
    "\n"
    "Starting with version 0.25, this transformation is a int-to-int transformation the 'itrans' method "
    "which was providing this transformation before is deprecated."
  ) +
  gsi::method_ext ("dtrans", &gsi::si_dtrans,
    "@brief Gets the transformation into the initial cell applicable for floating point types\n"
    "\n"
    "This transformation corresponds to the one delivered by \\trans, but is applicable for "
    "the floating-point shape types in micron unit space.\n"
    "\n"
    "This method has been introduced in version 0.25.3."
  ) +
  gsi::method ("prop_id", &db::RecursiveShapeIterator::prop_id,
    "@brief Gets the effective properties ID\n"
    "The shape iterator supports property filtering and translation. This method will deliver "
    "the effective property ID after translation. The original property ID can be obtained from "
    "'shape.prop_id' and is not changed by installing filters or mappers.\n"
    "\n"
    "\\prop_id is evaluated by \\Region objects for example, when they are created "
    "from a shape iterator.\n"
    "\n"
    "See \\enable_properties, \\filter_properties, \\remove_properties and \\map_properties for "
    "details on this feature.\n"
    "\n"
    "This attribute has been introduced in version 0.28.4."
  ) +
  gsi::method ("shape", &db::RecursiveShapeIterator::shape,
    "@brief Gets the current shape\n"
    "\n"
    "Returns the shape currently referred to by the recursive iterator. \n"
    "This shape is not transformed yet and is located in the current cell.\n"
  ) +
  gsi::method ("at_end?", &db::RecursiveShapeIterator::at_end, 
    "@brief End of iterator predicate\n"
    "\n"
    "Returns true, if the iterator is at the end of the sequence\n"
  ) +
  gsi::method ("cell", &db::RecursiveShapeIterator::cell, 
    "@brief Gets the current cell's object \n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("cell_index", &db::RecursiveShapeIterator::cell_index, 
    "@brief Gets the current cell's index \n"
  ) +
  gsi::method ("next", (void (db::RecursiveShapeIterator::*) ()) &db::RecursiveShapeIterator::next,
    "@brief Increments the iterator\n"
    "This moves the iterator to the next shape inside the search scope."
  ) +
  gsi::method ("layer", &db::RecursiveShapeIterator::layer,
    "@brief Returns the layer index where the current shape is coming from.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("path", &db::RecursiveShapeIterator::path,
    "@brief Gets the instantiation path of the shape addressed currently\n"
    "\n"
    "This attribute is a sequence of \\InstElement objects describing the cell instance path from the initial "
    "cell to the current cell containing the current shape.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("==", &db::RecursiveShapeIterator::operator==, gsi::arg ("other"),
    "@brief Comparison of iterators - equality\n"
    "\n"
    "Two iterators are equal if they point to the same shape.\n"
  ) +
  gsi::method ("!=", &db::RecursiveShapeIterator::operator!=, gsi::arg ("other"),
    "@brief Comparison of iterators - inequality\n"
    "\n"
    "Two iterators are not equal if they do not point to the same shape.\n"
  ) +
  gsi::method_ext ("enable_properties", &enable_properties,
    "@brief Enables properties for the given iterator.\n"
    "Afer enabling properties, \\prop_id will deliver the effective properties ID for the current shape. "
    "By default, properties are not enabled and \\prop_id will always return 0 (no properties attached). "
    "Alternatively you can apply \\filter_properties "
    "or \\map_properties to enable properties with a specific name key.\n"
    "\n"
    "Note that property filters/mappers are additive and act in addition (after) the currently installed filter.\n"
    "\n"
    "This feature has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("remove_properties", &remove_properties,
    "@brief Removes properties for the given container.\n"
    "This will remove all properties and \\prop_id will deliver 0 always (no properties attached).\n"
    "Alternatively you can apply \\filter_properties "
    "or \\map_properties to enable properties with a specific name key.\n"
    "\n"
    "Note that property filters/mappers are additive and act in addition (after) the currently installed filter.\n"
    "So effectively after 'remove_properties' you cannot get them back.\n"
    "\n"
    "This feature has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("filter_properties", &filter_properties, gsi::arg ("keys"),
    "@brief Filters properties by certain keys.\n"
    "Calling this method will reduce the properties to values with name keys from the 'keys' list.\n"
    "As a side effect, this method enables properties.\n"
    "As with \\enable_properties or \\remove_properties, this filter has an effect on the value returned "
    "by \\prop_id, not on the properties ID attached to the shape directly.\n"
    "\n"
    "Note that property filters/mappers are additive and act in addition (after) the currently installed filter.\n"
    "\n"
    "This feature has been introduced in version 0.28.4."
  ) +
  gsi::method_ext ("map_properties", &map_properties, gsi::arg ("key_map"),
    "@brief Maps properties by name key.\n"
    "Calling this method will reduce the properties to values with name keys from the 'keys' hash and "
    "renames the properties. Property values with keys not listed in the key map will be removed.\n"
    "As a side effect, this method enables properties.\n"
    "As with \\enable_properties or \\remove_properties, this filter has an effect on the value returned "
    "by \\prop_id, not on the properties ID attached to the shape directly.\n"
    "\n"
    "Note that property filters/mappers are additive and act in addition (after) the currently installed filter.\n"
    "\n"
    "This feature has been introduced in version 0.28.4."
  ),
  "@brief An iterator delivering shapes recursively\n"
  "\n"
  "The iterator can be obtained from a cell, a layer and optionally a region.\n"
  "It simplifies retrieval of shapes from a geometrical region while considering\n"
  "subcells as well.\n"
  "Some options can be specified in addition, i.e. the level to which to look into or\n"
  "shape classes and shape properties. The shapes are retrieved by using the \\shape method,\n"
  "\\next moves to the next shape and \\at_end tells, if the iterator has move shapes to deliver.\n"
  "\n"
  "This is some sample code:\n"
  "\n"
  "@code\n"
  "# print the polygon-like objects as seen from the initial cell \"cell\"\n"
  "iter = cell.begin_shapes_rec(layer)\n"
  "while !iter.at_end?\n"
  "  if iter.shape.renders_polygon?\n"
  "    polygon = iter.shape.polygon.transformed(iter.itrans)\n"
  "    puts \"In cell #{iter.cell.name}: \" + polygon.to_s\n"
  "  end\n"
  "  iter.next\n"
  "end\n"
  "\n"
  "# or shorter:\n"
  "cell.begin_shapes_rec(layer).each do |iter|\n"
  "  if iter.shape.renders_polygon?\n"
  "    polygon = iter.shape.polygon.transformed(iter.itrans)\n"
  "    puts \"In cell #{iter.cell.name}: \" + polygon.to_s\n"
  "  end\n"
  "end\n"
  "@/code\n"
  "\n"
  "\\Cell offers three methods to get these iterators: begin_shapes_rec, begin_shapes_rec_touching and begin_shapes_rec_overlapping.\n"
  "\\Cell#begin_shapes_rec will deliver a standard recursive shape iterator which starts from the given cell and iterates "
  "over all child cells. \\Cell#begin_shapes_rec_touching delivers a RecursiveShapeIterator which delivers the shapes "
  "whose bounding boxed touch the given search box. \\Cell#begin_shapes_rec_overlapping delivers all shapes whose bounding box "
  "overlaps the search box.\n"
  "\n"
  "A RecursiveShapeIterator object can also be created explicitly. This allows some more options, i.e. using "
  "multiple layers. A multi-layer recursive shape iterator can be created like this:\n"
  "\n"
  "@code\n"
  "iter = RBA::RecursiveShapeIterator::new(layout, cell, [ layer_index1, layer_index2 .. ])\n"
  "@/code\n"
  "\n"
  "\"layout\" is the layout object, \"cell\" the RBA::Cell object of the initial cell. layer_index1 etc. are the "
  "layer indexes of the layers to get the shapes from. While iterating, \\RecursiveShapeIterator#layer delivers "
  "the layer index of the current shape.\n"
  "\n"
  "The recursive shape iterator can be confined to a maximum hierarchy depth. By using \\max_depth=, the "
  "iterator will restrict the search depth to the given depth in the cell tree.\n"
  "\n"
  "In addition, the recursive shape iterator supports selection and exclusion of subtrees. For that purpose "
  "it keeps flags per cell telling it for which cells to turn shape delivery on and off. The \\select_cells method "
  "sets the \"start delivery\" flag while \\unselect_cells sets the \"stop delivery\" flag. In effect, using "
  "\\unselect_cells will exclude that cell plus the subtree from delivery. Parts of that subtree can be "
  "turned on again using \\select_cells. For the cells selected that way, the shapes of these cells and their "
  "child cells are delivered, even if their parent was unselected.\n"
  "\n"
  "To get shapes from a specific cell, i.e. \"MACRO\" plus its child cells, unselect the top cell first "
  "and the select the desired cell again:\n"
  "\n"
  "@code\n"
  "# deliver all shapes inside \"MACRO\" and the sub-hierarchy:\n"
  "iter = RBA::RecursiveShapeIterator::new(layout, cell, layer)\n"
  "iter.unselect_cells(cell.cell_index)\n"
  "iter.select_cells(\"MACRO\")\n"
  "@/code\n"
  "\n"
  "Note that if \"MACRO\" uses library cells for example which are used otherwise as well, the "
  "iterator will only deliver the shapes for those instances belonging to \"MACRO\" (directly or indirectly), "
  "not those for other instances of these library cells.\n"
  "\n"
  "The \\unselect_all_cells and \\select_all_cells methods turn on the \"stop\" and \"start\" flag "
  "for all cells respectively. If you use \\unselect_all_cells and use \\select_cells for a specific cell, "
  "the iterator will deliver only the shapes of the selected cell, not its children. Those are still "
  "unselected by \\unselect_all_cells:\n"
  "\n"
  "@code\n"
  "# deliver all shapes of \"MACRO\" but not of child cells:\n"
  "iter = RBA::RecursiveShapeIterator::new(layout, cell, layer)\n"
  "iter.unselect_all_cells\n"
  "iter.select_cells(\"MACRO\")\n"
  "@/code\n"
  "\n"
  "Cell selection is done using cell indexes or glob pattern. Glob pattern are equivalent to the usual "
  "file name wildcards used on various command line shells. For example \"A*\" matches all cells starting with "
  "an \"A\". The curly brace notation and character classes are supported as well. For example \"C{125,512}\" matches "
  "\"C125\" and \"C512\" and \"[ABC]*\" matches all cells starting with an \"A\", a \"B\" or \"C\". \"[^ABC]*\" matches "
  "all cells not starting with one of that letters.\n"
  "\n"
  "The RecursiveShapeIterator class has been introduced in version 0.18 and has been extended substantially in 0.23.\n"
);

}
