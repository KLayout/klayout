
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

#ifndef HDR_dbCell
#define HDR_dbCell

#include "dbCommon.h"

#include "dbInstances.h"
#include "dbObject.h"
#include "dbMemStatistics.h"
#include "dbBox.h"
#include "dbBoxTree.h"
#include "dbBoxConvert.h"
#include "dbShape.h"
#include "dbShapes.h"
#include "dbCellInst.h"
#include "dbArray.h"
#include "tlTypeTraits.h"
#include "tlVector.h"
#include "tlAlgorithm.h"
#include "gsi.h"

#include <map>
#include <set>

namespace db
{

template <class C> class cell_list;
template <class C> class cell_list_const_iterator;
template <class C> class cell_list_iterator;

template <class Coord> class generic_repository;

class Layout;
class Library;
class ImportLayerMapping;
class CellMapping;
class LayerMapping;

/**
 *  @brief The cell object
 * 
 *  A cell object consists of a set of shape containers (called layers),
 *  a set of child cell instances and auxiliary information such as
 *  the parent instance list.
 *  A cell is identified through an index given to the cell upon instantiation.
 *  The cell index is valid in the context of a layout object which
 *  must issue the cell index.
 */

class DB_PUBLIC Cell
  : public db::Object,
    public gsi::ObjectBase
{
public:
  typedef db::Box box_type;
  typedef std::map<unsigned int, box_type> box_map;
  typedef box_type::coord_type coord_type;
  typedef db::CellInst cell_inst_type;
  typedef db::simple_trans<coord_type> array_trans;
  typedef db::array<cell_inst_type, array_trans> cell_inst_array_type;
  typedef db::Shapes shapes_type;
  typedef db::Shapes::shape_iterator shape_iterator;
  typedef std::map<unsigned int, shapes_type> shapes_map;
  typedef db::Instances instances_type;
  typedef db::Instance instance_type;
  typedef instances_type::touching_iterator touching_iterator;
  typedef instances_type::overlapping_iterator overlapping_iterator;
  typedef instances_type::const_iterator const_iterator;
  typedef instances_type::parent_inst_type parent_inst_type;
  typedef instances_type::parent_inst_iterator parent_inst_iterator;
  typedef instances_type::parent_cell_iterator parent_cell_iterator;
  typedef instances_type::child_cell_iterator child_cell_iterator;
  typedef instances_type::basic_inst_type basic_inst_type;
  typedef instances_type::sorted_inst_iterator sorted_inst_iterator;

  friend class db::Layout;
  friend class db::Library;
  friend class db::cell_list<Cell>;
  friend class db::cell_list_iterator<Cell>;
  friend class db::cell_list_const_iterator<Cell>;
  friend class db::Instances;

  /**
   *  @brief The destructor
   */
  virtual ~Cell ();

  /**
   *  @brief The assignment operator 
   *
   *  The assignment operator does not change the association of the cell
   *  with a layout. It copies the "content" of the cell, where the instances
   *  refer to the same cell index than in the source cell.
   *
   *  HINT: this implementation can copy cell contents from one layout to 
   *  another. However, currently property id are not translated which limits
   *  the use of this method in this case to 1-to-1 copies of layouts.
   */
  Cell &operator= (const Cell &d);

  /**
   *  @brief Returns the number of layers stored in that cell
   */
  unsigned int layers () const;

  /**
   *  @brief Return the const shapes list of the given layer
   *
   *  This method allows one to access the shapes list on a certain layer.
   *  If the layer does not exist yet, a reference to an empty list is
   *  returned.
   *
   *  @param index The layer index of the shapes list to retrieve
   *
   *  @return A const reference to the shapes list
   */
  shapes_type &shapes (unsigned int index);

  /**
   *  @brief Return the shapes list of the given layer
   *
   *  This method allows one to access the shapes list on a certain layer.
   *  If the layer does not exist yet, it is created.
   *
   *  @param index The layer index of the shapes list to retrieve
   *
   *  @return A reference to the shapes list
   */
  const shapes_type &shapes (unsigned int index) const;

  /**
   *  @brief Gets the index of a given shapes array
   *
   *  If the shapes container is not part of the cell, std::numeric_limits<unsigned int>::max ()
   *  is returned.
   *
   *  This is not a cheap operation.
   */
  unsigned int index_of_shapes (const shapes_type *shapes) const;

  /**
   *  @brief Clear all shapes in the cell
   */
  void clear_shapes ();

  /**
   *  @brief Clear the instance list
   */
  void clear_insts ()
  {
    if (! m_instances.empty ()) {
      m_instances.clear_insts ();
    }
  }

  /**
   *  @brief Copy the shapes from layer src to dest
   *
   *  The target layer is not overwritten. Instead, the shapes are added to the target layer's shapes.
   */
  void copy (unsigned int src, unsigned int dest);

  /**
   *  @brief Copy the shapes from layer src to dest (only shapes from given classes)
   *
   *  The target layer is not overwritten. Instead, the shapes are added to the target layer's shapes.
   */
  void copy (unsigned int src, unsigned int dest, unsigned int types);

  /**
   *  @brief Move the shapes from layer src to dest
   *
   *  The target layer is not overwritten. Instead, the shapes are added to the target layer's shapes.
   */
  void move (unsigned int src, unsigned int dest);

  /**
   *  @brief Move the shapes from layer src to dest (only shapes from given classes)
   *
   *  The target layer is not overwritten. Instead, the shapes are added to the target layer's shapes.
   */
  void move (unsigned int src, unsigned int dest, unsigned int types);

  /**
   *  @brief Swap the layers given
   */
  void swap (unsigned int i1, unsigned int i2);

  /**
   *  @brief Clear the shapes on the given layer
   */
  void clear (unsigned int index);

  /**
   *  @brief Clear the shapes on the given layer (only the shapes from the given classes)
   */
  void clear (unsigned int index, unsigned int types);

  /**
   *  @brief Erase a cell instance given by a instance proxy
   *
   *  Erasing a cell instance will destroy the sorting order and invalidate
   *  other instance proxies.
   *  sort() must be called before a region query can be done
   *  on the cell instances. 
   */
  void erase (const instance_type &ref)
  {
    m_instances.erase (ref);
  }

  /**
   *  @brief Erase a cell instance
   *  
   *  Erasing a cell instance will destroy the sorting order.
   *  sort() must be called before a region query can be done
   *  on the cell instances. update_bbox() must be called 
   *  explicitly before the bounding box can be updated.
   */
  void erase (const_iterator e)
  {
    m_instances.erase (e);
  }

  /**
   *  @brief Erasing of multiple instances
   *
   *  Erase a set of positions given by an iterator I: *(from,to).
   *  *I must render an "iterator" object.
   *  The iterators in the sequence from, to must be sorted in
   *  "operator<" order.
   *
   *  @param first The start of the sequence of iterators
   *  @param last The end of the sequence of iterators
   */
  void erase_insts (const std::vector<instance_type> &instances)
  {
    m_instances.erase_insts (instances);
  }

  /**
   *  @brief Insert a generic cell instance 
   *  
   *  Inserting a cell instance will destroy the sorting order.
   *  sort() must be called before a region query can be done
   *  on the cell instances. update_bbox() must be called 
   *  explicitly before the bounding box can be updated.
   */
  template <class Inst>
  Instance insert (const Inst &inst)
  {
    return m_instances.insert (inst);
  }

  /**
   *  @brief Insert a sequence [from,to) of cell instances
   *  
   *  @param from The start of the sequence
   *  @param to The past-the-end pointer of the sequence
   */
  template <class I>
  void insert (I from, I to)
  {
    m_instances.insert (from, to);
  }

  /**
   *  @brief Insert an instance given by a instance reference 
   *
   *  This member may be used to copy an instance from one cell to another.
   *  Because of the inherent instability of the instance pointer it must not be 
   *  used to copy instances within one cell.
   *
   *  @param instance The instance reference of which to insert a copy
   */
  instance_type insert (const instance_type &instance)
  {
    return (m_instances.insert (instance));
  }

  /**
   *  @brief Transform the instance pointed to by the instance reference
   *
   *  @param t The transformation to apply
   */
  template <class Trans>
  instance_type transform (const instance_type &ref, const Trans &t)
  {
    return m_instances.transform (ref, t);
  }

  /**
   *  @brief Transform the instance pointed to by the instance reference into a new coordinate system
   *
   *  @param t The transformation to apply
   */
  template <class Trans>
  instance_type transform_into (const instance_type &ref, const Trans &t)
  {
    return m_instances.transform_into (ref, t);
  }

  /**
   *  @brief Transforms the cell by the given transformation.
   *
   *  The transformation is applied to all instances and shapes. Magnified transformations will
   *  render magnified instances. See \transform_into for a version which avoids this.
   *
   *  @param t The transformation to apply
   */
  template <class Trans>
  void transform (const Trans &t)
  {
    m_instances.transform (t);
    for (typename shapes_map::iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {
      if (! s->second.empty ()) {
        //  Note: don't use the copy ctor here - it will copy the attachment to the manager
        //  and create problems when destroyed. Plus: swap would be more efficient. But by using
        //  assign_transformed we get undo support for free.
        shapes_type d;
        d = s->second;
        s->second.assign_transformed (d, t);
      }
    }
  }

  /**
   *  @brief Transforms the cell into a new coordinate system.
   *
   *  The transformation is not applied to the children, but this method enables propagation 
   *  because the transformation is only applied to the instances insofar that it can be
   *  applied to the children as well.
   *
   *  @param t The transformation to apply
   */
  template <class Trans>
  void transform_into (const Trans &t)
  {
    m_instances.transform_into (t);
    for (typename shapes_map::iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {
      if (! s->second.empty ()) {
        //  Note: don't use the copy ctor here - it will copy the attachment to the manager 
        //  and create problems when destroyed. Plus: swap would be more efficient. But by using
        //  assign_transformed we get undo support for free.
        shapes_type d;
        d = s->second;
        s->second.assign_transformed (d, t);
      }
    }
  }

  /**
   *  @brief Insert an instance given by a instance reference with a different cell index and property ID
   *
   *  This member may be used to map an instance to another layout object. 
   *
   *  @param ref The instance reference of which to insert a copy
   *  @param im The mapper to new cell index to use (for mapping to a different layout for example)
   *  @param pm The mapper to new cell property ID use (for mapping to a different layout for example)
   */
  template <class IndexMap, class PropIdMap>
  instance_type insert (const instance_type &ref, IndexMap &im, PropIdMap &pm)
  {
    return (m_instances.insert (ref, im, pm));
  }

  /**
   *  @brief Test if the given reference is valid
   *  
   *  Returns true, if the given instance is valid. It is not valid, if it has been 
   *  deleted already. However, it may happen, that the instance memory has been 
   *  reused already. Therefore this method can safely be used only if nothing 
   *  has been inserted into this container in between.
   *  This method can only be used in editable mode.
   *
   *  @param instance The reference to the instance
   */
  bool is_valid (const instance_type &ref) const
  {
    return m_instances.is_valid (ref);
  }

  /** 
   *  @brief Replace the properties ID of an element (pointed to by the iterator) with the given one
   *
   *  The iterator must point to a instance already having a property ID. 
   *  If this is not the case, an exception is thrown
   */
  instance_type replace_prop_id (const instance_type &ref, db::properties_id_type prop_id)
  {
    return m_instances.replace_prop_id (ref, prop_id);
  }

  /**
   *  @brief Replace the instance pointed to by the iterator with the given instance
   *
   *  This method will delete the former object if it is not of the correct kind - i.e. it will 
   *  delete an object without a property and insert a new one with a property.
   *  This may invalidate other references in non-editable mode.
   *  If the inserted instance is one without a property and the existing instance has a property, 
   *  the latter is not touched.
   */
  template <class InstArray>
  instance_type replace (const instance_type &ref, const InstArray &inst)
  {
    return m_instances.replace (ref, inst);
  }

  /**
   *  @brief Test, if the given instance is a PCell instance
   *
   *  The return value is a pair of a boolean which is true when the instance is a PCell
   *  instance and the PCell id of the instance if this is the case.
   *
   *  @param ref The instance to test
   *  @return See above
   */
  std::pair<bool, db::pcell_id_type> is_pcell_instance (const instance_type &ref) const;

  /**
   *  @brief Gets the PCell parameters of a PCell instance
   *
   *  For the order of the parameters, ask the PCell declaration (available trough Layout::pcell_declaration
   *  from the PCell id.
   *
   *  @return A list of parameters in the order they are declared.
   */
  const std::vector<tl::Variant> &get_pcell_parameters (const instance_type &ref) const;

  /**
   *  @brief Gets a PCell parameter of a PCell instance
   *
   *  @return The value of the PCell parameter with the given name or a nil variant if there is no such parameter
   */
  tl::Variant get_pcell_parameter (const instance_type &ref, const std::string &name) const;

  /**
   *  @brief Gets the PCell parameters of a PCell instance as a name to value map
   *
   *  @return The PCell parameter values by name 
   */
  std::map<std::string, tl::Variant> get_named_pcell_parameters (const instance_type &ref) const;

  /**
   *  @brief Changes the PCell parameters of a PCell instance
   *
   *  For the order of the parameters, ask the PCell declaration (available trough Layout::pcell_declaration
   *  from the PCell id.
   *  This method is only allowed in editable mode.
   *
   *  @return A reference to the new instance. The original reference may be invalid.
   */
  instance_type change_pcell_parameters (const instance_type &ref, const std::vector<tl::Variant> &new_parameters);

  /**
   *  @brief The cell index accessor method
   *
   *  @return The cell index of the cell
   */
  const cell_index_type &cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Update the child-parent relationships
   * 
   *  This will update the child-parent relationships. Basically
   *  this means entering the cell as a parent into all it's child
   *  cells.
   */
  void update_relations ();

  /** 
   *  @brief Check if the bbox needs update because the shapes have changed
   *
   *  Returns true if the bounding box needs update because the
   *  shape bounding box (not one of the child bounding boxes) has changed.
   */
  bool is_shape_bbox_dirty () const;

  /** 
   *  @brief Updates the bbox
   *
   *  This will update the bbox from the shapes and instances.
   *  This requires the bboxes of the child cells to be computed
   *  before. Practically this will be done by computing the
   *  bboxes bottom-up in the hierarchy.
   *  In addition, the number of hierarchy levels below is also
   *  updated.
   *
   *  @param layers The max. number of layers in the child cells
   *  @return true, if the bounding box has changed.
   */
  bool update_bbox (unsigned int layers);

  /**
   *  @brief Sorts the shapes lists
   *
   *  This will sort the shapes lists for query of regions 
   *  on a per-shape basis. Since sorting of the shapes is
   *  guarded against redundant sorting (db::Layer::sort),
   *  we can safely call the sort method in any case.
   */
  void sort_shapes ();

  /**
   *  @brief Retrieve the bounding box of the cell
   *
   *  Before the bounding box can be retrieved, it must have
   *  been computed using update_bbox. This is performed by 
   *  requesting an update from the layout.
   *
   *  @return The bounding box that was computed by update_bbox
   */
  const box_type &bbox () const;

  /**
   *  @brief Retrieve the per-layer bounding box of the cell
   *
   *  Before the bounding box can be retrieved, it must have
   *  been computed using update_bbox. This is performed by 
   *  requesting an update from the layout.
   *
   *  @return The bounding box that was computed by update_bbox
   */
  const box_type &bbox (unsigned int l) const;

  /**
   *  @brief Region query for the instances in "overlapping" mode
   *
   *  This will return the region query iterator for the child cell
   *  instances overlapping with the given region b. 
   *
   *  @param b The region to query
   *
   *  @return The region iterator for the child cell instances
   */
  overlapping_iterator begin_overlapping (const box_type &b) const;

  /**
   *  @brief Region query for the instances in "touching" mode
   *
   *  This will return the region query iterator for the child cell
   *  instances touching the given region b. 
   *
   *  @param b The region to query
   *
   *  @return The region iterator for the child cell instances
   */
  touching_iterator begin_touching (const box_type &b) const;

  /**
   *  @brief The child cell iterator
   *
   *  This iterator will report the child cell indices, not every instance.
   *  Basically this is a filter which makes it slow.
   */
  child_cell_iterator begin_child_cells () const;

  /** 
   *  @brief Report the number of child cells
   *
   *  Just the number of child cells with distinct cell indices are
   *  reported.
   *  CAUTION: this method is SLOW!
   */
  size_t child_cells () const;

  /**
   *  @brief The number of cell instances 
   */
  size_t cell_instances () const
  {
    return m_instances.cell_instances ();
  }

  /**
   *  @brief The cell instance access begin iterator
   *
   *  The iterator follows the "at_end" semantics.
   */
  const_iterator begin () const;

  /**
   *  @brief The cell instance access by index in list of sorted instances
   *
   *  The iterator returned will point to the i-th element, where i is the index in the list
   *  of instances sorted by cell index.
   *  The iterator follows the "at_end" semantics.
   */
  instance_type sorted_inst_ptr (size_t i) const
  {
    return m_instances.instance_from_pointer (m_instances.begin_sorted_insts () [i]);
  }

  /**
   *  @brief The cell instance access by index in list of sorted instances
   *
   *  The pointer returned will point to the i-th element, where i is the index in the list
   *  of instances sorted by cell index. This is just a "basic_inst_type" pointer without any further
   *  hint about what object is being hidden behind it.
   */
  const basic_inst_type *basic_sorted_inst_ptr (size_t i) const
  {
    return m_instances.begin_sorted_insts () [i];
  }

  /**
   *  @brief The iterator for the instance list sorted by cell index
   */
  const sorted_inst_iterator begin_sorted_insts () const
  {
    return m_instances.begin_sorted_insts ();
  }

  /**
   *  @brief The end iterator for the instance list sorted by cell index
   */
  const sorted_inst_iterator end_sorted_insts () const
  {
    return m_instances.end_sorted_insts ();
  }

  /**
   *  @brief The parent instance list begin iterator
   *
   *  The begin_parent_insts() allows one to access to the parent instance list.
   */
  parent_inst_iterator begin_parent_insts () const;

  /**
   *  @brief Report the number of parent cells 
   *
   *  Since the m_parent_insts vector just stores references to those cells
   *  that have distinct cell indices, we can simply report their length.
   */
  size_t parent_cells () const;

  /**
   *  @brief The parent cell iterator
   * 
   *  This iterator will iterate over the parent cells, just returning their
   *  cell index.
   */
  parent_cell_iterator begin_parent_cells () const;

  /**
   *  @brief The parent cell end iterator
   */
  parent_cell_iterator end_parent_cells () const;

  /**
   *  @brief Tell if the cell is a top-level cell
   *
   *  A cell is a top-level cell if there are no parent instantiations.
   */
  bool is_top () const;

  /**
   *  @brief Tell if the cell is a leaf cell
   *
   *  A cell is a leaf cell if there are no child instantiations.
   */
  bool is_leaf () const;

  /**
   *  @brief Return the number of hierarchy levels below (expensive)
   */
  unsigned int hierarchy_levels () const;

  /**
   *  @brief begin iterator of all shapes 
   *
   *  @param layer The layer from which to query the shapes
   *  @param flags The flags of shapes to query (see db::shape documentation)
   *  @param prop_sel A property selector (a set of property set ids)
   *  @param inv_prop_sel true, if prop_sel should select all ids that should not be iterated
   *  
   *  @return The iterator delivering all these shapes
   */
  shape_iterator begin (unsigned int layer, unsigned int flags, const shape_iterator::property_selector *prop_sel = 0, bool inv_prop_sel = false) const
  {
    return shapes (layer).begin (flags, prop_sel, inv_prop_sel);
  }

  /**
   *  @brief begin iterator of all shapes with a overlapping mode region query
   *
   *  @param box The region to query the shapes from
   *  @param layer The layer from which to query the shapes
   *  @param flags The flags of shapes to query (see db::shape documentation)
   *  @param prop_sel A property selector (a set of property set ids)
   *  @param inv_prop_sel true, if prop_sel should select all ids that should not be iterated
   *  
   *  @return The iterator delivering all these shapes
   */
  shape_iterator begin_overlapping (unsigned int layer, const box_type &box, unsigned int flags, const shape_iterator::property_selector *prop_sel = 0, bool inv_prop_sel = false) const
  {
    return shapes (layer).begin_overlapping (box, flags, prop_sel, inv_prop_sel);
  }

  /**
   *  @brief begin iterator of all shapes with a touching mode region query
   *
   *  This method ensures that the box tress are updated before the query is done.
   *  The db::Shape-specific iterator does not do that.
   *
   *  @param box The region to query the shapes from
   *  @param layer The layer from which to query the shapes
   *  @param flags The flags of shapes to query (see db::shape documentation)
   *  @param prop_sel A property selector (a set of property set ids)
   *  @param inv_prop_sel true, if prop_sel should select all ids that should not be iterated
   *  
   *  @return The iterator delivering all these shapes
   */
  shape_iterator begin_touching (unsigned int layer, const box_type &box, unsigned int flags, const shape_iterator::property_selector *prop_sel = 0, bool inv_prop_sel = false) const
  {
    return shapes (layer).begin_touching (box, flags, prop_sel, inv_prop_sel);
  }

  /**
   *  @brief A quick, recursive test whether the cell has shapes touching the given box on the given layer
   */
  bool has_shapes_touching (unsigned int layer, const db::Box &box) const;

  /**
   *  @brief Collect all calling cells (either calling this cell directly or indirectly)
   *
   *  This method adds all cell indices of all cells that either directly or indirectly 
   *  call this cell to the set given. It is assumed that the if the set contains a cell, it
   *  will also contain all called cells, so it may act as a cache.
   *  
   *  @param callers The set of called cells (used as cache) 
   */
  void collect_caller_cells (std::set<cell_index_type> &callers) const;

  /**
   *  @brief Collect all calling cells (either calling this cell directly or indirectly) up to a certain level of hierarchy
   *
   *  This method adds all cell indices of all cells that either directly or indirectly 
   *  call this cell to the set given. It is assumed that the if the set contains a cell, it
   *  will also contain all called cells, so it may act as a cache.
   *  
   *  @param callers The set of called cells (used as cache) 
   *  @param levels The number of levels to descend (0: none, 1: direct callers ...)
   */
  void collect_caller_cells (std::set<cell_index_type> &callers, int levels) const;

  /**
   *  @brief Collect all calling cells (either calling this cell directly or indirectly) up to a certain level of hierarchy
   *
   *  This method adds all cell indices of all cells that either directly or indirectly 
   *  call this cell to the set given. It is assumed that the if the set contains a cell, it
   *  will also contain all called cells, so it may act as a cache.
   *
   *  This version allows one to restrict the search to a cone of the hierarchy tree, that is
   *  a set of cells which are collected from another call of "collect_called_cells" with another initial cell.
   *  
   *  @param callers The set of called cells (used as cache) 
   *  @param cone The selection cone 
   *  @param levels The number of levels to descend (0: none, 1: direct callers ..., -1: all)
   */
  void collect_caller_cells (std::set<cell_index_type> &callers, const std::set<cell_index_type> &cone, int levels) const;

  /**
   *  @brief Collect all cells called either directly or indirectly by this cell
   *
   *  This method adds all cell indices of all cells called either directly or indirectly 
   *  by this cell to the set given. It is assumed that the if the set contains a cell, it
   *  will also contain all called cells, so it may act as a cache.
   *  
   *  @param called The set of called cells (used as cache) 
   */
  void collect_called_cells (std::set<cell_index_type> &called) const;

  /**
   *  @brief Collect all cells called either directly or indirectly by this cell  down to a certain level of hierarchy
   *
   *  This method adds all cell indices of all cells called either directly or indirectly 
   *  by this cell to the set given. It is assumed that the if the set contains a cell, it
   *  will also contain all called cells, so it may act as a cache.
   *  
   *  @param called The set of called cells (used as cache) 
   *  @param levels The number of levels to descend (0: none, 1: direct callers ...)
   */
  void collect_called_cells (std::set<cell_index_type> &called, int level) const;

  /**
   *  @brief Unregister a cell inside it's context.
   *
   *  This method is called when a cell es taken out of a layout.
   *  It can for example, unregister a PCell variant from a PCell header.
   */
  virtual void unregister () { }

  /**
   *  @brief Reregister a cell from it's context.
   *
   *  This method is called when a cell es inserted again into a layout.
   *  It can for example, register a PCell variant at a PCell header.
   */
  virtual void reregister () { }

  /**
   *  @brief Update the layout
   *
   *  This method does not have any effect except for derived classes where it updates the layout
   *  with the current state, i.e. if a PCell declaration has changed.
   */
  virtual void update (ImportLayerMapping * /*layer_mapping*/ = 0) { }

  /**
   *  @brief Tell, if this cell is a proxy cell
   *
   *  Proxy cells are such whose layout represents a snapshot of another entity.
   *  Such cells can be PCell variants or library references for example.
   */
  virtual bool is_proxy () const 
  { 
    return false; 
  }

  /**
   *  @brief Sets the cell name
   *
   *  If a cell with that name already exists, a new name is created by appending a suffix.
   */
  void set_name (const std::string &name);

  /**
   *  @brief Get the basic name
   *
   *  The basic name of the cell is either the cell name or the cell name in the
   *  target library (for library proxies) or the PCell name (for PCell proxies).
   *  The actual name may be different by a extension to make it unique.
   */
  virtual std::string get_basic_name () const;

  /**
   *  @brief Gets the display name
   *
   *  The display name is some "nice" descriptive name of the cell (variant)
   *  For normal cells this name is equivalent to the normal cell name.
   *
   *  PCells will indicate the parameters in the display name. Library cells
   *  will indicate the library name the same way than the qualified name does.
   *  Ghost cells will be indicated by enclosing the name in brackets.
   */
  virtual std::string get_display_name () const;

  /**
   *  @brief Gets the qualified name
   *
   *  The qualified name contains a reference to the library, if the cell
   *  is a library reference. Otherwise it is identical to the basic name.
   */
  virtual std::string get_qualified_name () const;

  /**
   *  @brief Returns a value indicating whether the cell is a "ghost cell"
   *
   *  The ghost cell flag is used by the GDS reader for example to indicate that
   *  the cell is not located inside the file. Upon writing the reader can determine
   *  whether to write the cell or not.
   *  To satisfy the references inside the layout, a dummy cell is created in this case
   *  which has the "ghost cell" flag set to true.
   */
  bool is_ghost_cell () const
  {
    return m_ghost_cell;
  }

  /**
   *  @brief Sets the "ghost cell" flag
   *
   *  See "is_ghost_cell" for a description of this property.
   */
  void set_ghost_cell (bool g)
  {
    m_ghost_cell = g;
  }

  /**
   *  @brief Returns a value indicating whether the cell is empty
   *
   *  An empty cell is a cell not containing instances nor shapes.
   */
  bool empty () const;

  /**
   *  @brief Invalidate the instances bounding box (to be called by the instances container)
   */
  void invalidate_insts ();

  /**
   *  @brief Invalidate the hierarchical structure (to be called by the instances container)
   */
  void invalidate_hier ();

  /** 
   *  @brief Implementation of the redo method
   */
  void redo (db::Op *op);

  /** 
   *  @brief Implementation of the undo method
   */
  void undo (db::Op *op);

  /**
   *  @brief Collect memory usage statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const;

  /**
   *  @brief Sets the properties ID
   */
  void prop_id (db::properties_id_type id);

  /**
   *  @brief Gets the properties ID
   */
  db::properties_id_type prop_id () const
  {
    return m_prop_id;
  }

  /**
   *  @brief Get a reference to the layout object
   */
  db::Layout *layout () 
  {
    return mp_layout;
  }

  /**
   *  @brief Get a reference to the layout object (const version)
   */
  const db::Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Copies the shapes from the source cell's tree to this cell
   *
   *  This variant uses the given cell mapping and layer mapping.
   */
  void copy_tree_shapes (const db::Cell &source_cell, const db::CellMapping &cm);

  /**
   *  @brief Copies the shapes from the source cell's tree to this cell
   *
   *  This variant uses the given cell mapping and layer mapping.
   */
  void copy_tree_shapes (const db::Cell &source_cell, const db::CellMapping &cm, const db::LayerMapping &lm);

  /**
   *  @brief Copies instances and shapes from the source cell to this cell
   *
   *  If the source and target layout are different ones, the whole cell tree of the source cell
   *  will be copied.
   *  This will create new cells in the target layout to accommodate the source cell tree.
   *  Returns an array with the freshly created cells which accommodate the cell tree.
   */
  std::vector<db::cell_index_type> copy_tree (const db::Cell &source_cell);

  /**
   *  @brief Copies the instances from the source to this cell.
   */
  void copy_instances (const db::Cell &source_cell);

  /**
   *  @brief Copies all shapes from the source cell to this cell
   */
  void copy_shapes (const db::Cell &source_cell);

  /**
   *  @brief Copies all shapes from the source cell to this cell using the given layer mapping
   */
  void copy_shapes (const db::Cell &source_cell, const db::LayerMapping &layer_mapping);

  /**
   *  @brief Moves the shapes from the source cell's tree to this cell
   *
   *  This variant uses the given cell mapping and layer mapping.
   */
  void move_tree_shapes (db::Cell &source_cell, const db::CellMapping &cm);

  /**
   *  @brief Moves the shapes from the source cell's tree to this cell
   *
   *  This variant uses the given cell mapping and layer mapping.
   */
  void move_tree_shapes (db::Cell &source_cell, const db::CellMapping &cm, const db::LayerMapping &lm);

  /**
   *  @brief Moves instances and shapes from the source cell to this cell
   *
   *  If the source and target layout are different ones, the whole cell tree of the source cell
   *  will be copied.
   *  This will create new cells in the target layout to accommodate the source cell tree.
   *  Returns an array with the freshly created cells which accommodate the cell tree.
   */
  std::vector<db::cell_index_type> move_tree (db::Cell &source_cell);

  /**
   *  @brief Moves the instances from the source to this cell.
   */
  void move_instances (db::Cell &source_cell);

  /**
   *  @brief Moves all shapes from the source cell to this cell
   */
  void move_shapes (db::Cell &source_cell);

  /**
   *  @brief Moves all shapes from the source cell to this cell using the given layer mapping
   */
  void move_shapes (db::Cell &source_cell, const db::LayerMapping &layer_mapping);

protected:
  /**
   *  @brief Standard constructor: create an empty cell object
   * 
   *  Takes the manager object from the layout object.
   *
   *  @param ci The index of the cell
   *  @param g A reference to the layout object that owns the cell
   */
  Cell (cell_index_type ci, db::Layout &g);

  /**
   *  @brief Copy constructor
   *
   *  The copy constructor copies the contents of the cell and
   *  associates the new cell with the same layout than the source cell.
   */
  Cell (const Cell &d);

  /**
   *  @brief Clone the cell and attach to a different layout
   */
  virtual Cell *clone (db::Layout &layout) const;

private:
  cell_index_type m_cell_index;
  mutable db::Layout *mp_layout;
  shapes_map m_shapes_map;
  instances_type m_instances;
  box_type m_bbox;
  box_map m_bboxes;
  db::properties_id_type m_prop_id;

  // packed fields
  unsigned int m_hier_levels : 30;
  bool m_bbox_needs_update : 1;
  bool m_ghost_cell : 1;

  static box_type ms_empty_box;

  //  linked list, used by Layout
  Cell *mp_last, *mp_next;

  //  clear the shapes without telling the layout
  void clear_shapes_no_invalidate ();

  //  helper function for computing the number of hierarchy levels
  //  must be called bottom-up
  unsigned int count_hier_levels () const;

  /**
   *  @brief Set the cell index
   */
  void set_cell_index (db::cell_index_type ci)
  {
    m_cell_index = ci;
  }

  /**
   *  @brief Return a reference to the instances object 
   */
  instances_type &instances ()
  {
    return m_instances;
  }

  /**
   *  @brief Return a reference to the instances object 
   */
  const instances_type &instances () const
  {
    return m_instances;
  }

  /**
   *  @brief Count the number of parent instances
   * 
   *  This will accumulate the number of parent instances in the vector
   *  provided. Later, this count is used to resize the parent instance
   *  vector.
   */
  void count_parent_insts (std::vector <size_t> &count) const;

  /**
   *  @brief Clear the parent instance list
   *
   *  Reserve the given number of entries.
   */
  void clear_parent_insts (size_t sz = 0);

  /**
   *  @brief Sort the child instance list
   * 
   *  The child instances are first sorted by cell index, then by raw transformation
   *  (excluding displacement). This allows one to simplify the bbox computation by
   *  convolution of the displacements bboxes with the object bboxes.
   */
  void sort_child_insts ();

  /**
   *  @brief Sort the cell instance list
   *
   *  This will sort the cell instance list. As a prerequesite
   *  the cell's bounding boxes must have been computed.
   *
   *  @param force Force sorting, even if not strictly needed
   */
  void sort_inst_tree (bool force);
};

/**
 *  @brief Collect memory statistics
 */
inline void
mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const db::Cell &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

} // namespace db

#endif

