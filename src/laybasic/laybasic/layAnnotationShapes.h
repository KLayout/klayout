
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


#ifndef HDR_layAnnotationShapes
#define HDR_layAnnotationShapes

#include "laybasicCommon.h"

#include "dbUserObject.h"
#include "dbObject.h"
#include "dbLayer.h"
#include "dbLayoutStateModel.h"

#include <vector>
#include <limits>

namespace lay
{

class AnnotationShapes;

/**
 *  @brief A undo/redo queue object for the layer
 *
 *  This class is used internally to queue an insert or erase operation
 *  into the db::Object manager's undo/redo queue.
 */
class LAYBASIC_PUBLIC AnnotationLayerOp
  : public db::Op
{
public:
  typedef db::DUserObject shape_type;

  AnnotationLayerOp (bool insert, const shape_type &sh)
    : m_insert (insert)
  {
    m_shapes.push_back (sh);
  }
  
  template <class Iter>
  AnnotationLayerOp (bool insert, Iter from, Iter to)
    : m_insert (insert)
  {
    m_shapes.insert (m_shapes.end (), from, to);
  }

  template <class Iter>
  AnnotationLayerOp (bool insert, Iter from, Iter to, bool /*dummy*/)
    : m_insert (insert)
  {
    m_shapes.reserve (std::distance (from, to));
    for (Iter i = from; i != to; ++i) {
      m_shapes.push_back (**i);
    }
  }

  virtual void undo (AnnotationShapes *shapes)
  {
    if (m_insert) {
      erase (shapes);
    } else {
      insert (shapes);
    }
  }

  virtual void redo (AnnotationShapes *shapes)
  {
    if (m_insert) {
      insert (shapes);
    } else {
      erase (shapes);
    }
  }

private:
  bool m_insert;
  std::vector<shape_type> m_shapes;

  void insert (AnnotationShapes *shapes);
  void erase (AnnotationShapes *shapes);
};

/**
 *  @brief A collection of DUserObject objects that serves as a container for annotation shapes
 */

class LAYBASIC_PUBLIC AnnotationShapes 
  : public db::LayoutStateModel,
    public db::Object
{
public:
  typedef db::DCoord coord_type;
  typedef db::box<coord_type> box_type;
  typedef db::DUserObject shape_type;
  typedef db::layer<shape_type, db::stable_layer_tag> layer_type;
  typedef layer_type::overlapping_iterator overlapping_iterator;
  typedef layer_type::touching_iterator touching_iterator;
  typedef layer_type::iterator iterator;

  /**
   *  @brief Standard ctor: create an empty collection referencing a graph
   *
   *  The graph reference is used to invalid the bbox flag of the graph
   *  whenever something changes on the shapes list.
   */
  AnnotationShapes (db::Manager *manager = 0);

  /**
   *  @brief Dtor: clear all ..
   */
  ~AnnotationShapes (); 

  /**
   *  @brief Copy ctor
   */
  AnnotationShapes (const AnnotationShapes &d);

  /**
   *  @brief Copy ctor
   */
  AnnotationShapes (const AnnotationShapes &&d);

  /**
   *  @brief Assignment operator
   */
  AnnotationShapes &operator= (const AnnotationShapes &d);

  /**
   *  @brief Assignment operator (move)
   */
  AnnotationShapes &operator= (const AnnotationShapes &&d);

  /**
   *  @brief Insert a shape_type
   */
  const shape_type &insert (const shape_type &sh);

  /**
   *  @brief Insert a sequence of DUserObject shapes
   *
   *  Inserts a sequence of shapes [from,to)
   */

  /**
   *  @brief Insert a shape_type (move semantics)
   */
  const shape_type &insert (const shape_type &&sh);

  /**
   *  @brief Insert a sequence of DUserObject shapes
   *
   *  Inserts a sequence of shapes [from,to)
   */

  template <class Iter>
  void insert (Iter from, Iter to)
  {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new AnnotationLayerOp (true /*insert*/, from, to));
    }
    invalidate_state ();
    m_layer.insert (from, to);
  }

  /**
   *  @brief Reserve the number of elements for a shape type
   *
   *  @param n The number of elements to reserve
   */
  void reserve (size_t n);

  /**
   *  @brief Erase an element 
   *
   *  Erases a shape at the given position
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  @param pos The position of the shape to erase
   */
  void erase (layer_type::iterator pos);

  /**
   *  @brief Erasing of multiple elements
   *
   *  Erase a set of positions given by an iterator I: *(from,to).
   *  *I must render an "iterator" object.
   *  The iterators in the sequence from, to must be sorted in
   *  "later" order.
   *
   *  @param first The start of the sequence of iterators
   *  @param last The end of the sequence of iterators
   */
  template <class I>
  void erase_positions (I first, I last)
  {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new AnnotationLayerOp (false /*not insert*/, first, last, true /*dummy*/));
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    m_layer.erase_positions (first, last);
  }

  /**
   *  @brief Replace an element at the given position with another shape 
   *
   *  Replaces the element at the position pos with the
   *  new element.
   *
   *  @param pos The position at which to replace the shape
   *  @param sh The shape to replace 
   *  
   *  @return A reference to the object created
   */
  const shape_type &replace (iterator pos, const shape_type &sh);

  /**
   *  @brief Replace an element at the given position with another shape (move semantics)
   */
  const shape_type &replace (iterator pos, const shape_type &&sh);

  /**
   *  @brief updates the bbox 
   *
   *  Updating the bbox is required after insert operations
   *  and is performed only as far as necessary.
   */
  void update_bbox ()
  {
    m_layer.update_bbox ();
  }

  /**
   *  @brief check if the bounding box needs update
   *
   *  Returns true if the bounding box of the shapes has changed and 
   *  requires an update.
   */
  bool is_bbox_dirty () const
  {
    return m_layer.is_bbox_dirty ();
  }

  /**
   *  @brief Retrieve the bbox 
   *
   *  Retrieving the bbox might required an update_bbox
   *  before the bbox is valid. It will assert if the bbox
   *  was not valid.
   *  Doing a update_bbox before does not imply a performance penalty
   *  since the state is cached.
   */
  box_type bbox () const
  {
    return m_layer.bbox ();
  }

  /**
   *  @brief Clears the collection
   */
  void clear ();

  /**
   *  @brief Do a region search in "touching" mode
   *
   *  @return The region iterator 
   */
  layer_type::touching_iterator begin_touching (const box_type &b) const 
  {
    const_cast<layer_type &> (m_layer).sort (); // ensure the box tree is made
    return m_layer.begin_touching (b);
  }

  /**
   *  @brief The region iterator end token
   */
  size_t end_touching () const 
  {
    return 0; // KLUDGE: some shortcut but basically an implementation detail ..
  }

  /**
   *  @brief Do a region search in "overlapping" mode
   *
   *  @return The region iterator 
   */
  layer_type::overlapping_iterator begin_overlapping (const box_type &b) const 
  {
    const_cast<layer_type &> (m_layer).sort (); // ensure the box tree is made
    return m_layer.begin_overlapping (b);
  }

  /**
   *  @brief The region iterator end token
   */
  size_t end_overlapping () const 
  {
    return 0; // KLUDGE: some shortcut but basically an implementation detail ..
  }

  /**
   *  @brief begin iterator of all elements
   *
   *  @return The first position of the shapes
   */
  layer_type::iterator begin () const
  {
    return m_layer.begin ();
  }

  /**
   *  @brief end iterator of all elements
   *
   *  @return The post-end position of the shapes
   */
  layer_type::iterator end () const
  {
    return m_layer.end ();
  }

  /**
   *  @brief find a given shape (exactly)
   *
   *  @param s The shape to find
   *  
   *  @return end(Sh::tag) if the shape was not found, the position otherwise
   */
  layer_type::iterator find (const shape_type &s) const
  {
    return m_layer.find (s);
  }

  /**
   *  @brief get the iterator from a pointer
   *
   *  @param p The pointer to the element to retrieve
   *  @return The iterator pointing to that element
   */
  layer_type::iterator iterator_from_pointer (const shape_type *p) const
  {
    return m_layer.iterator_from_pointer (p);
  }

  /** 
   *  @brief Implementation of the redo method
   */
  void redo (db::Op *op);

  /** 
   *  @brief Implementation of the undo method
   */
  void undo (db::Op *op);

  /**
   *  @brief Collect memory usage
   */
  void mem_stat (db::MemStatistics *stat, db::MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;

private:
  void invalidate_state ()
  {
    invalidate_bboxes (std::numeric_limits<unsigned int>::max ());
  }

  virtual void do_update ();

  layer_type m_layer;
};

/**
 *  @brief Collect memory usage
 */
inline void mem_stat (db::MemStatistics *stat, db::MemStatistics::purpose_t purpose, int cat, const AnnotationShapes &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}


}

#endif


