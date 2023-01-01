
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


#ifndef HDR_dbRecursiveInstanceIterator
#define HDR_dbRecursiveInstanceIterator

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbInstElement.h"
#include "tlAssert.h"
#include "tlObject.h"

#include <map>
#include <set>
#include <vector>

namespace db
{

class Region;
class RecursiveInstanceReceiver;

/**
 *  @brief An iterator delivering shapes that touch or overlap the given region recursively
 *
 *  The iterator can be constructed from a layout, a cell and a region.
 *  It simplifies retrieval of instances from a geometrical region while considering
 *  subcells as well.
 *  Some options can be specified, i.e. the level to which to look into or which cells
 *  to select.
 *
 *  The general iteration scheme is iterating is depth-first and child instances before parent instances.
 */
class DB_PUBLIC RecursiveInstanceIterator
{
public:
  typedef db::Layout layout_type;
  typedef db::Box box_type;
  typedef db::Region region_type;
  typedef db::Cell cell_type;
  typedef db::Cell::touching_iterator inst_iterator;
  typedef db::CellInstArray::iterator inst_array_iterator;
  typedef db::Instances::overlapping_iterator overlapping_instance_iterator;
  typedef db::Instances::touching_iterator touching_instance_iterator;
  typedef db::Instance instance_type;
  typedef db::InstElement instance_element_type;
  typedef db::ICplxTrans cplx_trans_type;
  typedef instance_element_type value_type;
  typedef db::box_tree<db::Box, db::Box, db::box_convert<db::Box>, 20, 20> box_tree_type;

  /**
   *  @brief Default constructor
   */
  RecursiveInstanceIterator ();

  /**
   *  @brief Copy constructor
   */
  RecursiveInstanceIterator (const RecursiveInstanceIterator &d);

  /**
   *  @brief Assignment
   */
  RecursiveInstanceIterator &operator= (const RecursiveInstanceIterator &d);

  /**
   *  @brief Standard constructor
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param region The region from which to select the instances
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. instances that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers instances that
   *  overlap the given region by at least one database unit.
   *  The cell instances are selected according to their overall bounding box.
   */
  RecursiveInstanceIterator (const layout_type &layout, const cell_type &cell, const box_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param region The complex region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. instances that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers instances that
   *  overlap the given region by at least one database unit.
   *  The cell instances are selected according to their overall bounding box.
   *  This version offers a complex search region instead of a simple box.
   */
  RecursiveInstanceIterator (const layout_type &layout, const cell_type &cell, const region_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor for "overall" iteration
   *
   *  This iterator delivers all instances recursively.
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   */
  RecursiveInstanceIterator (const layout_type &layout, const cell_type &cell);

  /**
   *  @brief Destructor
   */
  ~RecursiveInstanceIterator ();

  /**
   *  @brief Specify the maximum hierarchy depth to look into
   *
   *  A depth of 0 instructs the iterator to deliver only instances from the initial cell.
   *  A higher depth instructs the iterator to look deeper.
   *  The depth must be specified before the instances are being retrieved.
   */
  void max_depth (int depth) 
  { 
    if (m_max_depth != depth) {
      m_max_depth = depth; 
      m_needs_reinit = true;
    }
  }

  /**
   *  @brief Gets the maximum hierarchy depth to search for
   */
  int max_depth () const
  {
    return m_max_depth;
  }

  /**
   *  @brief Specify the minimum hierarchy depth to look into
   *
   *  A depth of 0 instructs the iterator to deliver instance from the top level and below.
   *  1 instructs to deliver instance from the first child level.
   *  The minimum depth must be specified before the instances are being retrieved.
   */
  void min_depth (int depth)
  {
    if (m_min_depth != depth) {
      m_min_depth = depth;
      m_needs_reinit = true;
    }
  }

  /**
   *  @brief Gets the minimum hierarchy depth to search for
   */
  int min_depth () const
  {
    return m_min_depth;
  }

  /**
   *  @brief Gets the layout
   */
  const layout_type *layout () const
  {
    return mp_layout.get ();
  }

  /**
   *  @brief Gets the top cell
   *
   *  The top cell is the cell with which the iterator was started
   */
  const cell_type *top_cell () const
  {
    return mp_top_cell;
  }

  /**
   *  @brief Gets the basic region the iterator is using (will be world if none is set)
   *  In addition to the basic region, a complex region may be defined that is further confining the
   *  search to a subregion of the basic region.
   */
  const box_type &region () const
  {
    return m_region;
  }

  /**
   *  @brief Returns true if a complex region is given
   */
  bool has_complex_region () const
  {
    return mp_complex_region.get () != 0;
  }

  /**
   *  @brief Gets the complex region the iterator is using
   */
  const region_type &complex_region () const
  {
    tl_assert (mp_complex_region.get ());
    return *mp_complex_region;
  }

  /**
   *  @brief Sets the region to a basic rectangle
   *  This will reset the iterator.
   */
  void set_region (const box_type &region);

  /**
   *  @brief Sets a complex search region
   *  This will reset the iterator to the beginning.
   */
  void set_region (const region_type &region);

  /**
   *  @brief Confines the search further to the given rectangle.
   *  This will reset the iterator and confine the search to the given rectangle
   *  in addition to any region or complex region already defined.
   */
  void confine_region (const box_type &region);

  /**
   *  @brief Confines the search further to the given complex region.
   *  This will reset the iterator and confine the search to the given region
   *  in addition to any simple region or complex region already defined.
   */
  void confine_region (const region_type &region);

  /**
   *  @brief Gets a flag indicating whether overlapping instances are selected when a region is used
   */
  bool overlapping () const
  {
    return m_overlapping;
  }

  /**
   *  @brief Sets a flag indicating whether overlapping instances are selected when a region is used
   */
  void set_overlapping (bool f)
  {
    if (m_overlapping != f) {
      m_overlapping = f;
      m_needs_reinit = true;
    }
  }

  /**
   *  @brief Reset the iterator
   */
  void reset () 
  {
    m_needs_reinit = true;
  }

  /**
   *  @brief Gets the selected target cells
   *
   *  Only instances of cells in the targets set are reported.
   *  By default the iterator is configured to deliver all instances.
   *  By using "set_targets" with a set of cell indexes, the reporting
   *  can be confined to certain cells only. To enable all-cell reporting
   *  use "enable_all_targets".
   *
   *  "all_targets_enabled" can be used to check which mode is used.
   */
  const std::set<db::cell_index_type> &targets () const
  {
    return m_targets;
  }

  /**
   *  @brief Gets a flags indicating whether all targets are selected
   *  See \targets for more details.
   */
  bool all_targets_enabled () const
  {
    return m_all_targets;
  }

  /**
   *  @brief Selects all target cells
   *  See \targets for more details.
   */
  void enable_all_targets ();

  /**
   *  @brief Selects the given targets
   *
   *  This will reset the "all_targets" flag to false.
   *  See \targets for more details.
   */
  void set_targets (const std::set<db::cell_index_type> &set_targets);

  /**
   *  @brief Select cells 
   *
   *  Cell selection allows confining the hierarchy traversal to subtrees of the
   *  hierarchy tree. This happens by "selecting" and "unselecting" cells in the traversal path.
   *  "selected" cells will make iterator traverse the tree below this cell while
   *  "unselected" cells make the iterator ignore this cell.
   *  Cells which are neither selected nor unselected will be traversed depending
   *  on their parent's state. They are traversed if their parents are and are not traversed
   *  if their parents are not.
   *
   *  If no specific cells have been selected before, this method will confine the selection 
   *  to the given cells (plus their sub-hierarchy).
   *  If cells have been selected before, this will add the given cells to the selection.
   */
  void select_cells (const std::set<db::cell_index_type> &cells);

  /**
   *  @brief Select all cells
   *
   *  Makes all cells selected. After doing so, all unselect_cells calls 
   *  will unselect only that specific cell without children.
   *
   *  See \select_cells for more details.
   */
  void select_all_cells ();

  /**
   *  @brief Unselect cells 
   *
   *  This method will remove the given cells (plus their sub-hierarchy) from the selection.
   *
   *  See \select_cells for more details.
   */
  void unselect_cells (const std::set<db::cell_index_type> &cells);

  /**
   *  @brief Unselect all cells
   *
   *  Makes all cells unselected. After doing so, select_cells calls 
   *  will select only that specific cell without children.
   *
   *  See \select_cells for more details.
   */
  void unselect_all_cells ();

  /**
   *  @brief Resets the selection
   *
   *  This will reset all selections and unselections. 
   *  After calling this methods, all select_cells will again select the cells
   *  including their children.
   *
   *  See \select_cells for more details.
   */
  void reset_selection ();

  /**
   *  @brief Returns the cells in the "enable" selection
   *
   *  Cells in this set make the iterator become active, while cells in the
   *  disable selection make the iterator inactive. Only when active, the
   *  iterator will deliver shapes.
   */
  const std::set<db::cell_index_type> &enables () const
  {
    return m_start;
  }

  /**
   *  @brief Returns the cells in the "disable" selection
   */
  const std::set<db::cell_index_type> &disables () const
  {
    return m_stop;
  }

  /**
   *  @brief Get the current transformation by which the instances must be transformed into the initial cell
   *
   *  The instances delivered are not transformed. Instead, this transformation must be applied to
   *  get the instance in the coordinate system of the top cell.
   */
  const cplx_trans_type &trans () const
  {
    validate (0);
    return m_trans;
  }

  /**
   *  @brief Gets the current depth
   *
   *  Returns the number of hierarchy levels we are below top level currently.
   */
  unsigned int depth () const
  {
    validate (0);
    return (unsigned int) m_trans_stack.size ();
  }

  /**
   *  @brief Gets the current instance
   *
   *  Returns the instance currently referred to by the recursive iterator.
   *  This instance is not transformed yet and is located in the current cell.
   */
  instance_element_type instance () const
  {
    return *operator-> ();
  }

  /**
   *  @brief Access operator
   *
   *  The access operator is identical to the instance method.
   */
  instance_element_type operator* () const
  {
    return *operator-> ();
  }

  /**
   *  @brief Access (arrow) operator
   *
   *  The access operator is identical to the instance method.
   */
  const instance_element_type *operator-> () const;

  /**
   *  @brief End of iterator predicate
   *
   *  Returns true, if the iterator is at the end of the sequence
   */
  bool at_end () const;

  /**
   *  @brief Gets the current cell's index
   */
  db::cell_index_type cell_index () const
  {
    return cell ()->cell_index ();
  }

  /**
   *  @brief Gets the current cell's reference
   */
  const cell_type *cell () const
  {
    validate (0);
    size_t c = reinterpret_cast<size_t> (mp_cell);
    return reinterpret_cast<const cell_type *> (c - (c & size_t (3)));
  }

  /**
   *  @brief Increments the iterator (operator version)
   */
  RecursiveInstanceIterator &operator++()
  {
    next (0);
    return *this;
  }

  /**
   *  @brief Increments the iterator
   */
  void next ()
  {
    next (0);
  }

  /**
   *  @brief Comparison of iterators - equality
   */
  bool operator==(const RecursiveInstanceIterator &d) const
  {
    if (at_end () != d.at_end ()) {
      return false;
    } else if (at_end ()) {
      return true;
    } else {
      return (*m_inst == *d.m_inst);
    }
  }

  /**
   *  @brief Comparison of iterators - inequality
   */
  bool operator!=(const RecursiveInstanceIterator &d) const
  {
    return !operator==(d);
  }

  /**
   *  @brief The instance path
   */
  std::vector<instance_element_type> path () const;

  /**
   *  @brief Push-mode delivery
   *
   *  This method will deliver all instances to the given receiver.
   *  In contrast to pull mode, this method allows tailoring the
   *  traversal of the hierarchy tree during iteration.
   *  For this purpose, the receiver has methods that receive
   *  events and to some extend may modify the traversal (e.g.
   *  return value of enter_cell).
   *
   *  See RecursiveInstanceReceiver class for more details.
   */
  void push (RecursiveInstanceReceiver *receiver);

  /**
   *  @brief Returns a value indicating whether the current cell is inactive (disabled)
   */
  bool is_inactive () const
  {
    return (reinterpret_cast<size_t> (mp_cell) & size_t (1)) != 0;
  }

  /**
   *  @brief Returns a value indicating whether a new child cell of the current cell will be inactive
   */
  bool is_child_inactive (db::cell_index_type new_child) const;

private:
  int m_max_depth;
  int m_min_depth;
  bool m_overlapping;
  std::set<db::cell_index_type> m_start, m_stop;
  std::set<db::cell_index_type> m_targets;
  bool m_all_targets;

  tl::weak_ptr<layout_type> mp_layout;
  const cell_type *mp_top_cell;

  box_type m_region;
  std::unique_ptr<region_type> mp_complex_region;
  db::box_convert<db::CellInst> m_box_convert;

  mutable inst_iterator m_inst;
  mutable inst_array_iterator m_inst_array;
  mutable instance_element_type m_combined_instance;
  mutable std::map<db::cell_index_type, bool> m_empty_cells_cache;
  mutable const cell_type *mp_cell;
  mutable cplx_trans_type m_trans;
  mutable std::vector<cplx_trans_type> m_trans_stack;
  mutable std::vector<inst_iterator> m_inst_iterators;
  mutable std::vector<inst_array_iterator> m_inst_array_iterators;
  mutable std::vector<const cell_type *> m_cells;
  mutable std::vector<box_tree_type> m_local_complex_region_stack;
  mutable std::vector<box_type> m_local_region_stack;
  mutable bool m_needs_reinit;
  mutable size_t m_inst_quad_id;
  mutable std::vector<size_t> m_inst_quad_id_stack;
  mutable std::set<db::cell_index_type> m_target_tree;

  void init ();
  void init_region (const region_type &region);
  void init_region (const box_type &region);
  void skip_inst_iter_for_complex_region () const;
  void validate (RecursiveInstanceReceiver *receiver) const;
  void next (RecursiveInstanceReceiver *receiver);
  bool needs_visit () const;
  void next_instance (RecursiveInstanceReceiver *receiver) const;
  void new_inst (RecursiveInstanceReceiver *receiver) const;
  void new_inst_member (RecursiveInstanceReceiver *receiver) const;
  void new_cell (RecursiveInstanceReceiver *receiver) const;
  void up (RecursiveInstanceReceiver *receiver) const;
  void down (RecursiveInstanceReceiver *receiver) const;

  bool is_outside_complex_region (const box_type &box) const;
  box_type correct_box_overlapping (const box_type &box) const;

  void set_inactive (bool a) const 
  {
    size_t c = reinterpret_cast<size_t> (mp_cell);
    c -= (c & size_t (1));
    mp_cell = reinterpret_cast<const db::Cell *> (c + (a ? 1 : 0));
  }

  bool is_all_of_instance () const
  {
    return (reinterpret_cast<size_t> (mp_cell) & size_t (2)) != 0;
  }

  void set_all_of_instance (bool a) const
  {
    size_t c = reinterpret_cast<size_t> (mp_cell);
    c -= (c & size_t (2));
    mp_cell = reinterpret_cast<const db::Cell *> (c + (a ? 2 : 0));
  }
};

/**
 *  @brief A receiver interface for "push" mode
 *
 *  In push mode, the iterator will deliver the instances and hierarchy transitions
 *  to this interface. See "RecursiveInstanceIterator::push" for details about this
 *  mode.
 *
 *  The receiver receives events for the start of the delivery, on each cell
 *  entry and on each instance (followed by a cell entry).
 */
class DB_PUBLIC RecursiveInstanceReceiver
{
public:
  typedef RecursiveInstanceIterator::box_tree_type box_tree_type;

  /**
   *  @brief See new_inst for details.
   */
  enum  new_inst_mode { NI_all = 0, NI_single = 1, NI_skip = 2 };

  /**
   *  @brief Constructor
   */
  RecursiveInstanceReceiver () { }

  /**
   *  @brief Destructor
   */
  virtual ~RecursiveInstanceReceiver () { }

  /**
   *  @brief Called once when the iterator begins pushing
   */
  virtual void begin (const RecursiveInstanceIterator * /*iter*/) { }

  /**
   *  @brief Called once after the iterator pushed everything
   */
  virtual void end (const RecursiveInstanceIterator * /*iter*/) { }

  /**
   *  @brief Enters a cell
   *
   *  This method is called when the recursive shape iterator
   *  enters a new cell. It is not called for the top cell. When it is called, "iter->trans()"
   *  will already be updated.
   *
   *  @param iter The iterator
   *  @param cell The cell which is entered
   *  @param region The clip box as seen from "cell" or db::Box::world if there is no clip box
   *  @param complex_region A complex clip region if one is supplied together with "region"
   */
  virtual void enter_cell (const RecursiveInstanceIterator * /*iter*/, const db::Cell * /*cell*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/) { }

  /**
   *  @brief Leaves the current cell
   *
   *  This method is the counterpart for "enter_cell". It is called when traversal of "cell" ended.
   */
  virtual void leave_cell (const RecursiveInstanceIterator * /*iter*/, const db::Cell * /*cell*/) { }

  /**
   *  @brief Enters a new instance
   *
   *  This method is called before "enter_cell" and "new_inst_member" is called and will indicate the instance to follow.
   *  The sequence of events is
   *
   *    new_inst(A)
   *    new_inst_member(A[0,0])
   *    enter_cell(A)
   *    ...
   *    leave_cell(A)
   *    new_inst_member(A[1,0])
   *    enter_cell(A)
   *    ...
   *    leave_cell(A)
   *    ...
   *    new_inst(B)
   *    ...
   *
   *  The "all" parameter is true, if all instances of the array will be addressed.
   *
   *  This method can return the following values:
   *   - NI_all: iterate all members through "new_inst_member"
   *   - NI_single: iterate a single member (the first one)
   *   - NI_skip: skips the whole array (not a single instance is iterated)
   */
  virtual new_inst_mode new_inst (const RecursiveInstanceIterator * /*iter*/, const db::CellInstArray & /*inst*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool /*all*/) { return NI_all; }

  /**
   *  @brief Enters a new array member of the instance
   *
   *  See "new_inst" for a description. This method adds the "trans" parameter
   *  which holds the complex transformation for this particular instance of
   *  the array.
   *
   *  "all" is true, if an instance array is iterated in "all" mode (see new_inst).
   *
   *  If this method returns false, this array instance (but not the whole array) is skipped and the cell is not entered.
   */
  virtual bool new_inst_member (const RecursiveInstanceIterator * /*iter*/, const db::CellInstArray & /*inst*/, const db::ICplxTrans & /*trans*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool /*all*/) { return true; }
};

}  // namespace db

#endif
