
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


#ifndef HDR_dbRecursiveShapeIterator
#define HDR_dbRecursiveShapeIterator

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
class RecursiveShapeReceiver;

/**
 *  @brief An iterator delivering shapes that touch or overlap the given region recursively
 *
 *  The iterator can be constructed from a layout, a cell and a region.
 *  It simplifies retrieval of shapes from a geometrical region while considering
 *  subcells as well.
 *  Some options can be specified, i.e. the level to which to look into or
 *  shape classes and shape properties. 
 */
class DB_PUBLIC RecursiveShapeIterator
{
public:
  typedef db::Layout layout_type;
  typedef db::Box box_type;
  typedef db::Region region_type;
  typedef db::Cell cell_type;
  typedef db::Cell::touching_iterator inst_iterator;
  typedef db::CellInstArray::iterator inst_array_iterator;
  typedef db::ShapeIterator shape_iterator;
  typedef db::Shape shape_type;
  typedef db::Shapes shapes_type;
  typedef db::ICplxTrans cplx_trans_type;
  typedef db::box_tree<db::Box, db::Box, db::box_convert<db::Box>, 20, 20> box_tree_type;

  /**
   *  @brief Default constructor
   */
  RecursiveShapeIterator ();

  /**
   *  @brief Copy constructor
   */
  RecursiveShapeIterator (const RecursiveShapeIterator &d);

  /**
   *  @brief Assignment
   */
  RecursiveShapeIterator &operator= (const RecursiveShapeIterator &d);

  /**
   *  @brief Standard constructor iterating a single shape container
   *
   *  @param shapes The shape container to iterate
   *
   *  This iterator will iterate the shapes from the given shapes container.
   */
  RecursiveShapeIterator (const shapes_type &shapes);

  /**
   *  @brief Standard constructor iterating a single shape container
   *
   *  @param shapes The shape container to iterate
   *  @param region The region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  This iterator will iterate the shapes from the given shapes container using 
   *  the given search region in overlapping or touching mode.
   */
  RecursiveShapeIterator (const shapes_type &shapes, const box_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor iterating a single shape container
   *
   *  @param shapes The shape container to iterate
   *  @param region The complex region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  This iterator will iterate the shapes from the given shapes container using
   *  the given search region in overlapping or touching mode. It allows specification of a complex
   *  search region.
   */
  RecursiveShapeIterator (const shapes_type &shapes, const region_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layer The layer from which to deliver the shapes
   *  @param region The region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. shapes that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers shapes that
   *  overlap the given region by at least one database unit.
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer, const box_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layer The layer from which to deliver the shapes
   *  @param region The complex region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. shapes that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers shapes that
   *  overlap the given region by at least one database unit. It allows specification of a complex
   *  search region.
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer, const region_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor for "world" iteration
   *
   *  This iterator delivers all shapes recursively. The same effect can be achieved by using a "world" region.
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layer The layer from which to deliver the shapes
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer);

  /**
   *  @brief Standard constructor with a layer selection
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   *  @param region The region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. shapes that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers shapes that
   *  overlap the given region by at least one database unit.
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers, const box_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor with a layer selection
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   *  @param region The complex region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. shapes that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers shapes that
   *  overlap the given region by at least one database unit. It allows specification of a complex
   *  search region.
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers, const region_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor with a layer selection
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   *  @param region The region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. shapes that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers shapes that
   *  overlap the given region by at least one database unit.
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const box_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor with a layer selection
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   *  @param region The complex region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. shapes that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers shapes that
   *  overlap the given region by at least one database unit. It allows specification of a complex
   *  search region.
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const region_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor for "world" iteration with a layer set
   *
   *  This iterator delivers all shapes recursively. The same effect can be achieved by using a "world" region.
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers);

  /**
   *  @brief Standard constructor for "world" iteration with a layer set
   *
   *  This iterator delivers all shapes recursively. The same effect can be achieved by using a "world" region.
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers);

  /**
   *  @brief Destructor
   */
  ~RecursiveShapeIterator ();

  /**
   *  @brief Specify the maximum hierarchy depth to look into
   *
   *  A depth of 0 instructs the iterator to deliver only shapes from the initial cell.
   *  The depth must be specified before the shapes are being retrieved.
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
   *  A depth of 0 instructs the iterator to deliver shapes from the top level.
   *  1 instructs to deliver shapes from the first child level.
   *  The minimum depth must be specified before the shapes are being retrieved.
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
   *  @brief Gets the iterated shapes
   *
   *  Alternatively to layout/cell, the shape iterator can iterate shapes which will 
   *  deliver the shapes of the shape container rather than the cell.
   *  Layers and hierarchy levels don't have a meaning in that case.
   */
  const shapes_type *shapes () const
  {
    return mp_shapes;
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
   *  @brief Gets the installed property translator
   *
   *  The property translator is not automatically applied, but available to consumers
   *  of shapes to perform property translation.
   */
  const db::PropertiesTranslator &property_translator () const
  {
    return m_property_translator;
  }

  /**
   *  @brief Applies a PropertyTranslator
   *
   *  The property translator is available for receivers of the recursive shape
   *  iterator items. This method will apply an additional property translator
   *  atop of existing ones.
   */
  void apply_property_translator (const db::PropertiesTranslator &pt)
  {
    m_property_translator = pt * m_property_translator;
  }

  /**
   *  @brief Sets a PropertyTranslator
   *
   *  The property translator is available for receivers of the recursive shape
   *  iterator items. This method will apply an additional property translator
   *  atop of existing ones.
   */
  void set_property_translator (const db::PropertiesTranslator &pt)
  {
    m_property_translator = pt;
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
   *  @brief Gets a flag indicating whether overlapping shapes are selected when a region is used
   */
  bool overlapping () const
  {
    return m_overlapping;
  }

  /**
   *  @brief Sets a flag indicating whether overlapping shapes are selected when a region is used
   */
  void set_overlapping (bool f)
  {
    if (m_overlapping != f) {
      m_overlapping = f;
      m_needs_reinit = true;
    }
  }

  /**
   *  @brief Sets a global transformation
   *
   *  The global transformation will be applied to all shapes delivered by biasing the "trans" attribute
   */
  void set_global_trans (const cplx_trans_type &tr);

  /**
   *  @brief Gets the global transformation
   */
  cplx_trans_type global_trans () const
  {
    return m_global_trans;
  }

  /**
   *  @brief Gets the transformation which is to be applied always in push mode
   *
   *  The reasoning behind this method is that in push mode and with the presence of a global transformation we need to
   *  somehow reflect the fact that the top-level is transformed. Instead of transforming every shape and instance we use
   *  this attribute. It is unity for all cells below top level and equal to the global transformation for the top cell.
   */
  const cplx_trans_type &always_apply () const;

  /**
   *  @brief Reset the iterator
   */
  void reset () 
  {
    m_needs_reinit = true;
  }

  /**
   *  @brief Select cells 
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
   */
  void select_all_cells ();

  /**
   *  @brief Unselect cells 
   *
   *  This method will remove the given cells (plus their sub-hierarchy) from the selection.
   */
  void unselect_cells (const std::set<db::cell_index_type> &cells);

  /**
   *  @brief Unselect all cells
   *
   *  Makes all cells unselected. After doing so, select_cells calls 
   *  will select only that specific cell without children.
   */
  void unselect_all_cells ();

  /**
   *  @brief Resets the selection
   *
   *  This will reset all selections and unselections. 
   *  After calling this methods, all select_cells will again select the cells
   *  including their children.
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
   *  @brief Specify the shape selection flags
   *
   *  The flags are the same then being defined in db::ShapeIterator.
   *  The flags must be specified before the shapes are being retrieved.
   */
  void shape_flags (unsigned int flags) 
  { 
    if (m_shape_flags != flags) {
      m_shape_flags = flags; 
      m_needs_reinit = true;
    }
  }

  /**
   *  @brief Gets the shape selection flags
   */
  unsigned int shape_flags () const
  {
    return m_shape_flags;
  }

  /**
   *  @brief Changes the layer to be traversed
   *
   *  This method must be used before shapes are being retrieved. Using this method may reset the iterator.
   */
  void set_layer (unsigned int layer);

  /**
   *  @brief Changes the layers to be traversed
   *
   *  This method must be used before shapes are being retrieved. Using this method may reset the iterator.
   */
  void set_layers (const std::vector<unsigned int> &layers);

  /**
   *  @brief Specify the property selector
   *
   *  The selector is the same as being used in db::ShapeIterator.
   *  It is not copied and the object must stay valid as long as the iterator is used.
   *  The selector must be specified before the shapes are being retrieved.
   */
  void shape_property_selector (const shape_iterator::property_selector *prop_sel) 
  { 
    if (mp_shape_prop_sel != prop_sel) {
      mp_shape_prop_sel = prop_sel; 
      m_needs_reinit = true;
    }
  }

  /**
   *  @brief Specify the inverse of the property selector
   *
   *  This flag is the same as being used in db::ShapeIterator.
   *  The flags must be specified before the shapes are being retrieved.
   */
  void inverse_shape_property_selection (bool inv)
  { 
    if (m_shape_inv_prop_sel != inv) {
      m_shape_inv_prop_sel = inv; 
      m_needs_reinit = true;
    }
  }

  /**
   *  @brief Gets the layer of the current shape
   */
  unsigned int layer () const
  {
    if (m_has_layers) {
      validate (0);
    }
    return m_layer;
  }

  /**
   *  @brief Gets the layers from which the shapes are taken from
   *
   *  The returned layers are useful only if \multiple_layers is
   *  true. Otherwise use \layer to get the iterated layer.
   */
  const std::vector<unsigned int> &layers () const
  {
    return m_layers;
  }

  /**
   *  @brief Gets a value indicating whether a multiple layers are used
   *
   *  If this value is true, multiple layers are searched for shapes. 
   *  Use the \layers method to retrieve them
   *  Otherwise a single layer is used. Use the \layer method to retrieve it.
   */
  bool multiple_layers () const
  {
    return m_has_layers;
  }

  /**
   *  @brief Get the current transformation by which the shapes must be transformed into the initial cell
   *
   *  The shapes delivered are not transformed. Instead, this transformation must be applied to 
   *  get the shape in the coordinate system of the top cell.
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
   *  @brief Gets the current shape
   *
   *  Returns the shape currently referred to by the recursive iterator. 
   *  This shape is not transformed yet and is located in the current cell.
   */
  shape_type shape () const
  {
    validate (0);
    return *m_shape;
  }

  /**
   *  @brief Access operator
   *
   *  The access operator is identical to the shape method.
   */
  shape_type operator* () const
  {
    validate (0);
    return *m_shape;
  }

  /**
   *  @brief Access (arrow) operator
   *
   *  The access operator is identical to the shape method.
   */
  const shape_type *operator-> () const
  {
    validate (0);
    return m_shape.operator-> ();
  }

  /**
   *  @brief End of iterator predicate
   *
   *  Returns true, if the iterator is at the end of the sequence
   */
  bool at_end () const;

  /**
   *  @brief Gets the translated property ID
   *
   *  This version employs the property translator to deliver the real property ID.
   */
  db::properties_id_type prop_id () const
  {
    if (m_property_translator.is_null ()) {
      return 0;
    } else {
      validate (0);
      return m_property_translator (m_shape->prop_id ());
    }
  }

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
   *  @brief Gets the current cell's bounding box
   *
   *  The returned box is limited to the selected layer if applicable.
   */
  box_type cell_bbox (db::cell_index_type cell_index) const
  {
    return m_box_convert (db::CellInst (cell_index));
  }

  /**
   *  @brief Increments the iterator (operator version)
   */
  RecursiveShapeIterator &operator++() 
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
  bool operator==(const RecursiveShapeIterator &d) const
  {
    if (at_end () != d.at_end ()) {
      return false;
    } else if (at_end ()) {
      return true;
    } else {
      return *m_shape == *d.m_shape;
    }
  }

  /**
   *  @brief Comparison of iterators - inequality
   */
  bool operator!=(const RecursiveShapeIterator &d) const
  {
    return !operator==(d);
  }

  /**
   *  @brief Returns the bounding box of the region that will be iterated
   *
   *  (that does not mean all shapes will be inside that region)
   */
  box_type bbox () const;

  /**
   *  @brief The instance path
   */
  std::vector<db::InstElement> path () const;

  /**
   *  @brief Push-mode delivery
   *
   *  This method will deliver all shapes to the given receiver.
   *  In contrast to pull mode, this method allows tailoring the
   *  traversal of the hierarchy tree during iteration.
   *  For this purpose, the receiver has methods that receive
   *  events and to some extend may modify the traversal (e.g.
   *  return value of enter_cell).
   *
   *  See RecursiveShapeReceiver class for more details.
   */
  void push (RecursiveShapeReceiver *receiver);

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
  std::vector<unsigned int> m_layers;
  bool m_has_layers;
  int m_max_depth;
  int m_min_depth;
  unsigned int m_shape_flags;
  const shape_iterator::property_selector *mp_shape_prop_sel;
  bool m_shape_inv_prop_sel;
  bool m_overlapping;
  std::set<db::cell_index_type> m_start, m_stop;
  cplx_trans_type m_global_trans;
  db::PropertiesTranslator m_property_translator;

  tl::weak_ptr<layout_type> mp_layout;
  const cell_type *mp_top_cell;
  const shapes_type *mp_shapes;

  box_type m_region;
  std::unique_ptr<region_type> mp_complex_region;
  db::box_convert<db::CellInst> m_box_convert;

  mutable inst_iterator m_inst;
  mutable inst_array_iterator m_inst_array;
  mutable std::map<db::cell_index_type, bool> m_empty_cells_cache;
  mutable unsigned int m_layer;
  mutable const cell_type *mp_cell;
  mutable size_t m_current_layer;
  mutable shape_iterator m_shape;
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
  mutable size_t m_shape_quad_id;

  void init ();
  void init_region (const region_type &region);
  void init_region (const box_type &region);
  void skip_shape_iter_for_complex_region () const;
  void skip_inst_iter_for_complex_region () const;
  void validate (RecursiveShapeReceiver *receiver) const;
  void start_shapes () const;
  void next (RecursiveShapeReceiver *receiver);
  void next_shape (RecursiveShapeReceiver *receiver) const;
  void new_inst (RecursiveShapeReceiver *receiver) const;
  void new_inst_member (RecursiveShapeReceiver *receiver) const;
  void new_cell (RecursiveShapeReceiver *receiver) const;
  void new_layer () const;
  void up (RecursiveShapeReceiver *receiver) const;
  void down (RecursiveShapeReceiver *receiver) const;

  bool is_outside_complex_region (const db::Box &box) const;

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
 *  In push mode, the iterator will deliver the shapes and hierarchy transitions
 *  to this interface. See "RecursiveShapeIterator::push" for details about this
 *  mode.
 *
 *  The receiver receives events for the start of the delivery, on each cell
 *  entry and on each instance (followed by a cell entry). It also receives
 *  the shapes.
 */
class DB_PUBLIC RecursiveShapeReceiver
{
public:
  typedef RecursiveShapeIterator::box_tree_type box_tree_type;

  /**
   *  @brief See new_inst for details.
   */
  enum  new_inst_mode { NI_all = 0, NI_single = 1, NI_skip = 2 };

  /**
   *  @brief Constructor
   */
  RecursiveShapeReceiver () { }

  /**
   *  @brief Destructor
   */
  virtual ~RecursiveShapeReceiver () { }

  /**
   *  @brief Returns true, if the receivers wants the full hierarchy and not just non-empty cells
   */
  virtual bool wants_all_cells () const { return false; }

  /**
   *  @brief Called once when the iterator begins pushing
   */
  virtual void begin (const RecursiveShapeIterator * /*iter*/) { }

  /**
   *  @brief Called once after the iterator pushed everything
   */
  virtual void end (const RecursiveShapeIterator * /*iter*/) { }

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
  virtual void enter_cell (const RecursiveShapeIterator * /*iter*/, const db::Cell * /*cell*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/) { }

  /**
   *  @brief Leaves the current cell
   *
   *  This method is the counterpart for "enter_cell". It is called when traversal of "cell" ended.
   */
  virtual void leave_cell (const RecursiveShapeIterator * /*iter*/, const db::Cell * /*cell*/) { }

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
  virtual new_inst_mode new_inst (const RecursiveShapeIterator * /*iter*/, const db::CellInstArray & /*inst*/, const db::ICplxTrans & /*always_apply*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool /*all*/) { return NI_all; }

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
  virtual bool new_inst_member (const RecursiveShapeIterator * /*iter*/, const db::CellInstArray & /*inst*/, const db::ICplxTrans & /*always_apply*/, const db::ICplxTrans & /*trans*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool /*all*/) { return true; }

  /**
   *  @brief Delivers a shape
   *
   *  @param trans The transformation which maps the shape to the top cell.
   */
  virtual void shape (const RecursiveShapeIterator * /*iter*/, const db::Shape & /*shape*/, const db::ICplxTrans & /*always_apply*/, const db::ICplxTrans & /*trans*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/) { }
};

}  // namespace db

#endif
