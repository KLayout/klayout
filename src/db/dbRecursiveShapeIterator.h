
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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

#include <map>
#include <set>
#include <vector>

namespace db
{

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
  typedef db::Cell cell_type;
  typedef db::Cell::touching_iterator inst_iterator;
  typedef db::CellInstArray::iterator inst_array_iterator;
  typedef db::ShapeIterator shape_iterator;
  typedef db::Shape shape_type;
  typedef db::Shapes shapes_type;
  typedef db::ICplxTrans cplx_trans_type;

  /**
   *  @brief Default constructor
   */
  RecursiveShapeIterator ();

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
   *  @brief Standard constructor for "world" iteration
   *
   *  This iterator delivers all shapes recursively. The same effect can be acchieved by using a "world" region.
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
   *  @param region The region from which to select the shapes
   *  @param overlapping Specify overlapping mode
   *
   *  By default the iterator operates in touching mode - i.e. shapes that touch the given region
   *  are returned. By specifying the "overlapping" flag with a true value, the iterator delivers shapes that
   *  overlap the given region by at least one database unit.
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const box_type &region, bool overlapping = false);

  /**
   *  @brief Standard constructor for "world" iteration with a layer set
   *
   *  This iterator delivers all shapes recursively. The same effect can be acchieved by using a "world" region.
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers);

  /**
   *  @brief Standard constructor for "world" iteration with a layer set
   *
   *  This iterator delivers all shapes recursively. The same effect can be acchieved by using a "world" region.
   *
   *  @param layout The layout from which to get the cell hierarchy
   *  @param cell The starting cell
   *  @param layers The layers from which to deliver the shapes
   */
  RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers);

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
    return mp_layout;
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
   *  @brief Gets the region the iterator is using (will be world if none is set)
   */
  const box_type &region () const
  {
    return m_region;
  }

  /**
   *  @brief Sets the region 
   */
  void set_region (const box_type &region)
  {
    if (m_region != region) {
      m_region = region;
      m_needs_reinit = true;
    }
  }

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
   *  @brief Get the layer of the current shape
   */
  unsigned int layer () const
  {
    validate ();
    return m_layer;
  }

  /**
   *  @brief Gets the layers from which the shapes are taken from
   *
   *  The returned layers are useful only if \multiple_layers is
   *  true.
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
    validate ();
    return m_trans;
  }

  /**
   *  @brief Get the current depth
   *
   *  Returns the number of hierarchy levels we are below top level currently.
   */
  unsigned int depth () const
  {
    validate ();
    return (unsigned int) m_trans_stack.size ();
  }

  /**
   *  @brief Get the current shape
   *
   *  Returns the shape currently referred to by the recursive iterator. 
   *  This shape is not transformed yet and is located in the current cell.
   */
  shape_type shape () const
  {
    validate ();
    return *m_shape;
  }

  /**
   *  @brief Access operator
   *
   *  The access operator is identical to the shape method.
   */
  shape_type operator* () const
  {
    validate ();
    return *m_shape;
  }

  /**
   *  @brief Access (arrow) operator
   *
   *  The access operator is identical to the shape method.
   */
  const shape_type *operator-> () const
  {
    validate ();
    return m_shape.operator-> ();
  }

  /**
   *  @brief End of iterator predicate
   *
   *  Returns true, if the iterator is at the end of the sequence
   */
  bool at_end () const;

  /**
   *  @brief Get the current cell's index 
   */
  db::cell_index_type cell_index () const
  {
    validate ();
    size_t c = reinterpret_cast<size_t> (mp_cell);
    return reinterpret_cast<const cell_type *> (c - (c & size_t (1)))->cell_index ();
  }

  /**
   *  @brief Get the current cell's reference 
   */
  const cell_type *cell () const
  {
    validate ();
    size_t c = reinterpret_cast<size_t> (mp_cell);
    return reinterpret_cast<const cell_type *> (c - (c & size_t (1)));
  }

  /**
   *  @brief Increment the iterator (operator version)
   */
  RecursiveShapeIterator &operator++() 
  {
    next ();
    return *this;
  }

  /**
   *  @brief Increment the iterator
   */
  void next ()
  {
    if (! at_end ()) {
      ++m_shape;
      if (! mp_shapes) {
        next_shape ();
      }
    }
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

  const layout_type *mp_layout;
  const cell_type *mp_top_cell;
  const shapes_type *mp_shapes;

  box_type m_region;
  db::box_convert<db::CellInst> m_box_convert;

  mutable box_type m_local_region;
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
  mutable bool m_needs_reinit;

  void init ();
  void validate () const;
  void start_shapes () const;
  void next_shape () const;
  void new_inst () const;
  void new_cell () const;
  void new_layer () const;
  void up () const;
  void down () const;

  bool is_inactive () const
  {
    return (reinterpret_cast<size_t> (mp_cell) & size_t (1)) != 0;
  }

  void set_inactive (bool a) const 
  {
    size_t c = reinterpret_cast<size_t> (mp_cell);
    c -= (c & size_t (1));
    mp_cell = reinterpret_cast<const db::Cell *> (c + a);
  }
};

}  // namespace db

#endif


