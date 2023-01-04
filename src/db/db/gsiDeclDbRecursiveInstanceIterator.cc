
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
#include "dbRecursiveInstanceIterator.h"
#include "dbRegion.h"

#include "tlGlobPattern.h"

namespace gsi
{

namespace {

/**
 *  @brief A wrapper that allows using "each" on the iterator
 */
class IteratorIterator
{
public:
  typedef db::RecursiveInstanceIterator value_type;
  typedef db::RecursiveInstanceIterator &reference;
  typedef db::RecursiveInstanceIterator *pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  IteratorIterator (db::RecursiveInstanceIterator *iter) : mp_iter (iter) { }
  bool at_end () const { return mp_iter->at_end (); }
  reference operator* () const { return *mp_iter; }
  void operator++ () { ++*mp_iter; }

private:
  db::RecursiveInstanceIterator *mp_iter;
};

}

// ---------------------------------------------------------------
//  db::RecursiveInstanceIterator binding

static db::RecursiveInstanceIterator *new_si1 (const db::Layout &layout, const db::Cell &cell)
{
  return new db::RecursiveInstanceIterator (layout, cell);
}

static db::RecursiveInstanceIterator *new_si2 (const db::Layout &layout, const db::Cell &cell, const db::Box &box, bool overlapping)
{
  return new db::RecursiveInstanceIterator (layout, cell, box, overlapping);
}

static db::RecursiveInstanceIterator *new_si2a (const db::Layout &layout, const db::Cell &cell, const db::Region &region, bool overlapping)
{
  return new db::RecursiveInstanceIterator (layout, cell, region, overlapping);
}

static IteratorIterator each (db::RecursiveInstanceIterator *r)
{
  return IteratorIterator (r);
}

static db::DCplxTrans si_dtrans (const db::RecursiveInstanceIterator *r)
{
  const db::Layout *ly = r->layout ();
  tl_assert (ly != 0);
  return db::CplxTrans (ly->dbu ()) * r->trans () * db::VCplxTrans (1.0 / ly->dbu ());
}

static void set_targets1 (db::RecursiveInstanceIterator *r, const std::vector<db::cell_index_type> &cells)
{
  std::set<db::cell_index_type> cc;
  cc.insert (cells.begin (), cells.end ());
  r->set_targets (cc);
}

static db::DCplxTrans inst_dtrans (const db::RecursiveInstanceIterator *r)
{
  const db::Layout *ly = r->layout ();
  tl_assert (ly != 0);
  return db::CplxTrans (ly->dbu ()) * (*r)->complex_trans () * db::VCplxTrans (1.0 / ly->dbu ());
}

static db::ICplxTrans inst_trans (const db::RecursiveInstanceIterator *r)
{
  return (*r)->complex_trans ();
}

static db::Cell *inst_cell (const db::RecursiveInstanceIterator *r)
{
  const db::Layout *ly = r->layout ();
  tl_assert (ly != 0);
  return const_cast<db::Cell *> (&ly->cell ((*r)->inst_ptr.cell_index ()));
}

static void set_targets2 (db::RecursiveInstanceIterator *r, const std::string &pattern)
{
  tl::GlobPattern p (pattern);
  std::set<db::cell_index_type> cc;
  for (db::Layout::const_iterator ci = r->layout ()->begin (); ci != r->layout ()->end (); ++ci) {
    if (p.match (r->layout ()->cell_name (ci->cell_index ()))) {
      cc.insert (ci->cell_index ());
    }
  }

  r->set_targets (cc);
}

static void select_cells1 (db::RecursiveInstanceIterator *r, const std::vector<db::cell_index_type> &cells)
{
  std::set<db::cell_index_type> cc;
  cc.insert (cells.begin (), cells.end ());
  r->select_cells (cc);
}

static void select_cells2 (db::RecursiveInstanceIterator *r, const std::string &pattern)
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

static void unselect_cells1 (db::RecursiveInstanceIterator *r, const std::vector<db::cell_index_type> &cells)
{
  std::set<db::cell_index_type> cc;
  cc.insert (cells.begin (), cells.end ());
  r->unselect_cells (cc);
}

static void unselect_cells2 (db::RecursiveInstanceIterator *r, const std::string &pattern)
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

static db::Region complex_region (const db::RecursiveInstanceIterator *iter)
{
  if (iter->has_complex_region ()) {
    return iter->complex_region ();
  } else {
    return db::Region (iter->region ());
  }
}

Class<db::RecursiveInstanceIterator> decl_RecursiveInstanceIterator ("db", "RecursiveInstanceIterator",
  gsi::constructor ("new", &new_si1, gsi::arg ("layout"), gsi::arg ("cell"),
    "@brief Creates a recursive instance iterator.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param layer The layer (index) from which the shapes are taken\n"
    "\n"
    "This constructor creates a new recursive instance iterator which delivers the instances of "
    "the given cell plus its children.\n"
  ) +
  gsi::constructor ("new", &new_si2, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("box"), gsi::arg ("overlapping", false),
    "@brief Creates a recursive instance iterator with a search region.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param box The search region\n"
    "@param overlapping If set to true, instances overlapping the search region are reported, otherwise touching is sufficient\n"
    "\n"
    "This constructor creates a new recursive instance iterator which delivers the instances of "
    "the given cell plus its children.\n"
    "\n"
    "The search is confined to the region given by the \"box\" parameter. If \"overlapping\" is true, instances whose "
    "bounding box is overlapping the search region are reported. If \"overlapping\" is false, instances whose "
    "bounding box touches the search region are reported. The bounding box of instances is measured taking all layers "
    "of the target cell into account.\n"
  ) +
  gsi::constructor ("new", &new_si2a, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("region"), gsi::arg ("overlapping"),
    "@brief Creates a recursive instance iterator with a search region.\n"
    "@param layout The layout which shall be iterated\n"
    "@param cell The initial cell which shall be iterated (including its children)\n"
    "@param region The search region\n"
    "@param overlapping If set to true, instances overlapping the search region are reported, otherwise touching is sufficient\n"
    "\n"
    "This constructor creates a new recursive instance iterator which delivers the instances of "
    "the given cell plus its children.\n"
    "\n"
    "The search is confined to the region given by the \"region\" parameter. The region needs to be a rectilinear region.\n"
    "If \"overlapping\" is true, instances whose "
    "bounding box is overlapping the search region are reported. If \"overlapping\" is false, instances whose "
    "bounding box touches the search region are reported. The bounding box of instances is measured taking all layers "
    "of the target cell into account.\n"
  ) +
  gsi::iterator_ext ("each", &each,
    "@brief Native iteration\n"
    "This method enables native iteration, e.g.\n"
    "\n"
    "@code\n"
    "  iter = ... # RecursiveInstanceIterator\n"
    "  iter.each do |i|\n"
    "     ... i is the iterator itself\n"
    "  end\n"
    "@/code\n"
    "\n"
    "This is slightly more convenient than the 'at_end' .. 'next' loop.\n"
    "\n"
    "This feature has been introduced in version 0.28.\n"
  ) +
  gsi::method ("max_depth=", (void (db::RecursiveInstanceIterator::*) (int)) &db::RecursiveInstanceIterator::max_depth, gsi::arg ("depth"),
    "@brief Specifies the maximum hierarchy depth to look into\n"
    "\n"
    "A depth of 0 instructs the iterator to deliver only instances from the initial cell.\n"
    "A higher depth instructs the iterator to look deeper.\n"
    "The depth must be specified before the instances are being retrieved.\n"
  ) +
  gsi::method ("max_depth", (int (db::RecursiveInstanceIterator::*) () const) &db::RecursiveInstanceIterator::max_depth,
    "@brief Gets the maximum hierarchy depth\n"
    "\n"
    "See \\max_depth= for a description of that attribute.\n"
  ) +
  gsi::method ("min_depth=", (void (db::RecursiveInstanceIterator::*) (int)) &db::RecursiveInstanceIterator::min_depth, gsi::arg ("depth"),
    "@brief Specifies the minimum hierarchy depth to look into\n"
    "\n"
    "A depth of 0 instructs the iterator to deliver instances from the top level.\n"
    "1 instructs to deliver instances from the first child level.\n"
    "The minimum depth must be specified before the instances are being retrieved.\n"
  ) +
  gsi::method ("min_depth", (int (db::RecursiveInstanceIterator::*) () const) &db::RecursiveInstanceIterator::min_depth,
    "@brief Gets the minimum hierarchy depth\n"
    "\n"
    "See \\min_depth= for a description of that attribute.\n"
  ) +
  gsi::method ("reset", &db::RecursiveInstanceIterator::reset,
    "@brief Resets the iterator to the initial state\n"
  ) +
  gsi::method ("reset_selection", &db::RecursiveInstanceIterator::reset_selection,
    "@brief Resets the selection to the default state\n"
    "\n"
    "In the initial state, the top cell and its children are selected. Child cells can be switched on and off "
    "together with their sub-hierarchy using \\select_cells and \\unselect_cells.\n"
    "\n"
    "This method will also reset the iterator.\n"
  ) +
  gsi::method ("layout", &db::RecursiveInstanceIterator::layout,
    "@brief Gets the layout this iterator is connected to\n"
  ) +
  gsi::method ("top_cell", &db::RecursiveInstanceIterator::top_cell,
    "@brief Gets the top cell this iterator is connected to\n"
  ) +
  gsi::method ("region", &db::RecursiveInstanceIterator::region,
    "@brief Gets the basic region that this iterator is using\n"
    "The basic region is the overall box the region iterator iterates over. "
    "There may be an additional complex region that confines the region iterator. "
    "See \\complex_region for this attribute.\n"
  ) +
  gsi::method_ext ("complex_region", &complex_region,
    "@brief Gets the complex region that this iterator is using\n"
    "The complex region is the effective region (a \\Region object) that the "
    "iterator is selecting from the layout. This region can be a single box "
    "or a complex region.\n"
  ) +
  gsi::method ("region=", (void (db::RecursiveInstanceIterator::*)(const db::RecursiveInstanceIterator::box_type &)) &db::RecursiveInstanceIterator::set_region, gsi::arg ("box_region"),
    "@brief Sets the rectangular region that this iterator is iterating over\n"
    "See \\region for a description of this attribute.\n"
    "Setting a simple region will reset the complex region to a rectangle and reset the iterator to "
    "the beginning of the sequence."
  ) +
  gsi::method ("region=", (void (db::RecursiveInstanceIterator::*)(const db::RecursiveInstanceIterator::region_type &)) &db::RecursiveInstanceIterator::set_region, gsi::arg ("complex_region"),
    "@brief Sets the complex region that this iterator is using\n"
    "See \\complex_region for a description of this attribute. Setting the complex region will "
    "reset the basic region (see \\region) to the bounding box of the complex region and "
    "reset the iterator to the beginning of the sequence.\n"
  ) +
  gsi::method ("confine_region", (void (db::RecursiveInstanceIterator::*)(const db::RecursiveInstanceIterator::box_type &)) &db::RecursiveInstanceIterator::confine_region, gsi::arg ("box_region"),
    "@brief Confines the region that this iterator is iterating over\n"
    "This method is similar to setting the region (see \\region=), but will confine any region (complex or simple) already set. "
    "Essentially it does a logical AND operation between the existing and given region. "
    "Hence this method can only reduce a region, not extend it.\n"
  ) +
  gsi::method ("confine_region", (void (db::RecursiveInstanceIterator::*)(const db::RecursiveInstanceIterator::region_type &)) &db::RecursiveInstanceIterator::confine_region, gsi::arg ("complex_region"),
    "@brief Confines the region that this iterator is iterating over\n"
    "This method is similar to setting the region (see \\region=), but will confine any region (complex or simple) already set. "
    "Essentially it does a logical AND operation between the existing and given region. "
    "Hence this method can only reduce a region, not extend it.\n"
  ) +
  gsi::method ("overlapping?", &db::RecursiveInstanceIterator::overlapping,
    "@brief Gets a flag indicating whether overlapping instances are selected when a region is used\n"
  ) +
  gsi::method ("overlapping=", &db::RecursiveInstanceIterator::set_overlapping, gsi::arg ("region"),
    "@brief Sets a flag indicating whether overlapping instances are selected when a region is used\n"
    "\n"
    "If this flag is false, instances touching the search region are returned.\n"
  ) +
  gsi::method ("unselect_all_cells", &db::RecursiveInstanceIterator::unselect_all_cells,
    "@brief Unselects all cells.\n"
    "\n"
    "This method will set the \"unselected\" mark on all cells. The effect is "
    "that subsequent calls of \\select_cells will select only the specified cells, not "
    "their children, because they are still unselected.\n"
    "\n"
    "This method will also reset the iterator.\n"
  ) +
  gsi::method ("select_all_cells", &db::RecursiveInstanceIterator::select_all_cells,
    "@brief Selects all cells.\n"
    "\n"
    "This method will set the \"selected\" mark on all cells. The effect is "
    "that subsequent calls of \\unselect_cells will unselect only the specified cells, not "
    "their children, because they are still unselected.\n"
    "\n"
    "This method will also reset the iterator.\n"
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
  ) +
  gsi::method_ext ("targets=", &set_targets1, gsi::arg ("cells"),
    "@brief Specifies the target cells.\n"
    "\n"
    "If target cells are specified, only instances of these cells are delivered. "
    "This version takes a list of cell indexes for the targets. "
    "By default, no target cell list is present and the instances of all cells "
    "are delivered by the iterator. See \\all_targets_enabled? and \\enable_all_targets for "
    "a description of this mode. Once a target list is specified, the iteration is "
    "confined to the cells from this list."
    "\n"
    "The cells are given as a list of cell indexes.\n"
    "\n"
    "This method will also reset the iterator.\n"
  ) +
  gsi::method_ext ("targets=", &set_targets2, gsi::arg ("cells"),
    "@brief Specifies the target cells.\n"
    "\n"
    "If target cells are specified, only instances of these cells are delivered. "
    "This version takes a cell list as a glob pattern. "
    "A glob pattern follows the syntax of "
    "file names on the shell (i.e. \"A*\" are all cells starting with a letter \"A\").\n"
    "Use the curly-bracket notation to list different cells, e.g \"{A,B,C}\" for cells A, B and C.\n"
    "\n"
    "By default, no target cell list is present and the instances of all cells "
    "are delivered by the iterator. See \\all_targets_enabled? and \\enable_all_targets for "
    "a description of this mode. Once a target list is specified, the iteration is "
    "confined to the cells from this list."
    "\n"
    "The cells are given as a list of cell indexes.\n"
    "\n"
    "This method will also reset the iterator.\n"
  ) +
  gsi::method ("targets", &db::RecursiveInstanceIterator::targets,
    "@brief Gets the list of target cells\n"
    "See \\targets= for a description of the target cell concept. "
    "This method returns a list of cell indexes of the selected target cells."
  ) +
  gsi::method ("all_targets_enabled?", &db::RecursiveInstanceIterator::all_targets_enabled,
    "@brief Gets a value indicating whether instances of all cells are reported\n"
    "See \\targets= for a description of the target cell concept. "
  ) +
  gsi::method ("enable_all_targets", &db::RecursiveInstanceIterator::enable_all_targets,
    "@brief Enables 'all targets' mode in which instances of all cells are reported\n"
    "See \\targets= for a description of the target cell concept. "
  ) +
  gsi::method ("trans", &db::RecursiveInstanceIterator::trans,
    "@brief Gets the accumulated transformation of the current instance parent cell to the top cell\n"
    "\n"
    "This transformation represents how the current instance is seen in the top cell.\n"
  ) +
  gsi::method_ext ("dtrans", &gsi::si_dtrans,
    "@brief Gets the accumulated transformation of the current instance parent cell to the top cell\n"
    "\n"
    "This transformation represents how the current instance is seen in the top cell.\n"
    "This version returns the micron-unit transformation.\n"
  ) +
  gsi::method ("at_end?", &db::RecursiveInstanceIterator::at_end,
    "@brief End of iterator predicate\n"
    "\n"
    "Returns true, if the iterator is at the end of the sequence\n"
  ) +
  gsi::method ("cell", &db::RecursiveInstanceIterator::cell,
    "@brief Gets the cell the current instance sits in\n"
  ) +
  gsi::method ("cell_index", &db::RecursiveInstanceIterator::cell_index,
    "@brief Gets the index of the cell the current instance sits in\n"
    "This is equivalent to 'cell.cell_index'."
  ) +
  gsi::method_ext ("inst_trans", &inst_trans,
    "@brief Gets the integer-unit transformation of the current instance\n"
    "This is the transformation of the current instance inside its parent.\n"
    "'trans * inst_trans' gives the full transformation how the current cell is seen in the top cell.\n"
    "See also \\inst_dtrans and \\inst_cell.\n"
  ) +
  gsi::method_ext ("inst_dtrans", &inst_dtrans,
    "@brief Gets the micron-unit transformation of the current instance\n"
    "This is the transformation of the current instance inside its parent.\n"
    "'dtrans * inst_dtrans' gives the full micron-unit transformation how the current cell is seen in the top cell.\n"
    "See also \\inst_trans and \\inst_cell.\n"
  ) +
  gsi::method_ext ("inst_cell", &inst_cell,
    "@brief Gets the target cell of the current instance\n"
    "This is the cell the current instance refers to. It is one of the \\targets if a target list is given.\n"
  ) +
  gsi::method ("current_inst_element", &db::RecursiveInstanceIterator::instance,
    "@brief Gets the current instance\n"
    "\n"
    "This is the instance/array element the iterator currently refers to.\n"
    "This is a \\InstElement object representing the current instance and the array element the iterator currently points at.\n"
    "\n"
    "See \\inst_trans, \\inst_dtrans and \\inst_cell for convenience methods to access the details of the current element.\n"
  ) +
  gsi::method ("next", (void (db::RecursiveInstanceIterator::*) ()) &db::RecursiveInstanceIterator::next,
    "@brief Increments the iterator\n"
    "This moves the iterator to the next instance inside the search scope."
  ) +
  gsi::method ("path", &db::RecursiveInstanceIterator::path,
    "@brief Gets the instantiation path of the instance addressed currently\n"
    "\n"
    "This attribute is a sequence of \\InstElement objects describing the cell instance path from the initial "
    "cell to the current instance. The path is empty if the current instance is in the top cell.\n"
  ) +
  gsi::method ("==", &db::RecursiveInstanceIterator::operator==, gsi::arg ("other"),
    "@brief Comparison of iterators - equality\n"
    "\n"
    "Two iterators are equal if they point to the same instance.\n"
  ) +
  gsi::method ("!=", &db::RecursiveInstanceIterator::operator!=, gsi::arg ("other"),
    "@brief Comparison of iterators - inequality\n"
    "\n"
    "Two iterators are not equal if they do not point to the same instance.\n"
  ),
  "@brief An iterator delivering instances recursively\n"
  "\n"
  "The iterator can be obtained from a cell and optionally a region.\n"
  "It simplifies retrieval of instances while considering\n"
  "subcells as well.\n"
  "Some options can be specified in addition, i.e. the hierarchy level to which to look into.\n"
  "The search can be confined to instances of certain cells (see \\targets=) or to certain regions. "
  "Subtrees can be selected for traversal or excluded from it (see \\select_cells).\n"
  "\n"
  "This is some sample code:\n"
  "\n"
  "@code\n"
  "# prints the effective instances of cell \"A\" as seen from the initial cell \"cell\"\n"
  "iter = cell.begin_instances_rec\n"
  "iter.targets = \"A\"\n"
  "while !iter.at_end?\n"
  "  puts \"Instance of #{iter.inst_cell.name} in #{cell.name}: \" + (iter.dtrans * iter.inst_dtrans).to_s\n"
  "  iter.next\n"
  "end\n"
  "\n"
  "# or shorter:\n"
  "cell.begin_instances_rec.each do |iter|\n"
  "  puts \"Instance of #{iter.inst_cell.name} in #{cell.name}: \" + (iter.dtrans * iter.inst_dtrans).to_s\n"
  "end\n"
  "@/code\n"
  "\n"
  "Here, a target cell is specified which confines the search to instances of this particular cell.\n"
  "'iter.dtrans' gives us the accumulated transformation of all parents up to the top cell. "
  "'iter.inst_dtrans' gives us the transformation from the current instance. "
  "'iter.inst_cell' finally gives us the target cell of the current instance (which is always 'A' in our case).\n"
  "\n"
  "\\Cell offers three methods to get these iterators: begin_instances_rec, begin_instances_rec_touching and begin_instances_rec_overlapping.\n"
  "\\Cell#begin_instances_rec will deliver a standard recursive instance iterator which starts from the given cell and iterates "
  "over all child cells. \\Cell#begin_instances_rec_touching creates a RecursiveInstanceIterator which delivers the instances "
  "whose bounding boxed touch the given search box. \\Cell#begin_instances_rec_overlapping gives an iterator which delivers all instances whose bounding box "
  "overlaps the search box.\n"
  "\n"
  "A RecursiveInstanceIterator object can also be created directly, like this:\n"
  "\n"
  "@code\n"
  "iter = RBA::RecursiveInstanceIterator::new(layout, cell [, options ])\n"
  "@/code\n"
  "\n"
  "\"layout\" is the layout object, \"cell\" the \\Cell object of the initial cell.\n"
  "\n"
  "The recursive instance iterator can be confined to a maximum hierarchy depth. By using \\max_depth=, the "
  "iterator will restrict the search depth to the given depth in the cell tree.\n"
  "In the same way, the iterator can be configured to start from a certain hierarchy depth using \\min_depth=. "
  "The hierarchy depth always applies to the parent of the instances iterated.\n"
  "\n"
  "In addition, the recursive instance iterator supports selection and exclusion of subtrees. For that purpose "
  "it keeps flags per cell telling it for which cells to turn instance delivery on and off. The \\select_cells method "
  "sets the \"start delivery\" flag while \\unselect_cells sets the \"stop delivery\" flag. In effect, using "
  "\\unselect_cells will exclude that cell plus the subtree from delivery. Parts of that subtree can be "
  "turned on again using \\select_cells. For the cells selected that way, the instances of these cells and their "
  "child cells are delivered, even if their parent was unselected.\n"
  "\n"
  "To get instances from a specific cell, i.e. \"MACRO\" plus its child cells, unselect the top cell first "
  "and the select the desired cell again:\n"
  "\n"
  "@code\n"
  "# deliver all instances inside \"MACRO\" and the sub-hierarchy:\n"
  "iter = RBA::RecursiveInstanceIterator::new(layout, cell)\n"
  "iter.unselect_cells(cell.cell_index)\n"
  "iter.select_cells(\"MACRO\")\n"
  "...\n"
  "@/code\n"
  "\n"
  "The \\unselect_all_cells and \\select_all_cells methods turn on the \"stop\" and \"start\" flag "
  "for all cells respectively. If you use \\unselect_all_cells and use \\select_cells for a specific cell, "
  "the iterator will deliver only the instances of the selected cell, not its children. Those are still "
  "unselected by \\unselect_all_cells:\n"
  "\n"
  "@code\n"
  "# deliver all instance inside \"MACRO\" but not of child cells:\n"
  "iter = RBA::RecursiveInstanceIterator::new(layout, cell)\n"
  "iter.unselect_all_cells\n"
  "iter.select_cells(\"MACRO\")\n"
  "...\n"
  "@/code\n"
  "\n"
  "Cell selection is done using cell indexes or glob pattern. Glob pattern are equivalent to the usual "
  "file name wildcards used on various command line shells. For example \"A*\" matches all cells starting with "
  "an \"A\". The curly brace notation and character classes are supported as well. For example \"C{125,512}\" matches "
  "\"C125\" and \"C512\" and \"[ABC]*\" matches all cells starting with an \"A\", a \"B\" or \"C\". \"[^ABC]*\" matches "
  "all cells not starting with one of that letters.\n"
  "\n"
  "To confine instance iteration to instances of certain cells, use the \\targets feature:\n"
  "\n"
  "@code\n"
  "# deliver all instance of \"INV1\":\n"
  "iter = RBA::RecursiveInstanceIterator::new(layout, cell)\n"
  "iter.targets = \"INV1\"\n"
  "...\n"
  "@/code\n"
  "\n"
  "Targets can be specified either as lists of cell indexes or through a glob pattern.\n"
  "\n"
  "Instances are always delivered depth-first with child instances before their parents. A default recursive instance "
  "iterator will first deliver leaf cells, followed by the parent of these cells.\n"
  "\n"
  "When a search region is used, instances whose bounding box touch or overlap (depending on 'overlapping' flag) will "
  "be reported. The instance bounding box taken as reference is computed using all layers of the layout.\n"
  "\n"
  "The iterator will deliver the individual elements of instance arrays, confined to the search region if one is given. "
  "Consequently the return value (\\current_inst_element) is an \\InstElement "
  "object which is basically a combination of an \\Instance object and information about the current array element.\n"
  "\\inst_cell, \\inst_trans and \\inst_dtrans are methods provided for convenience to access the current array member's transformation "
  "and the target cell of the current instance.\n"
  "\n"
  "The RecursiveInstanceIterator class has been introduced in version 0.27.\n"
);

}
