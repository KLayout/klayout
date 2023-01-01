
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



#ifndef HDR_dbLayer
#define HDR_dbLayer

#include "dbBoxTree.h"
#include "dbBoxConvert.h"
#include "tlVector.h"

#include <iterator>

namespace db 
{

template <class Coord> class generic_repository;
class ArrayRepository;

struct stable_layer_tag { };
struct unstable_layer_tag { };

template <class Box, class Sh, class BoxConvert, class StableTag>
struct box_tree_typedef { };

template <class Box, class Sh, class BoxConvert>
struct box_tree_typedef<Box, Sh, BoxConvert, stable_layer_tag> 
{ 
  typedef db::box_tree<Box, Sh, BoxConvert> box_tree_type;  
};

template <class Box, class Sh, class BoxConvert>
struct box_tree_typedef<Box, Sh, BoxConvert, unstable_layer_tag> 
{ 
  typedef db::unstable_box_tree<Box, Sh, BoxConvert> box_tree_type;  
};

template <class ConstIter, class NonConstIter>
void to_non_const_box_tree_iter (const ConstIter &ci, NonConstIter &nci, stable_layer_tag)
{
  nci = ci.to_non_const ();
}

template <class ConstIter, class NonConstIter>
void to_non_const_box_tree_iter (const ConstIter &ci, NonConstIter &nci, unstable_layer_tag)
{
  //  HACK: this assumes non-const and const iterators have the same memory layout ...
  nci = *reinterpret_cast<NonConstIter *> ((void *) &ci);
}


/**
 *  @brief A layer object
 *
 *  A layer is basically a collection of shape objects 
 *  with a bounding box and the capability to do region queries
 *  with a test box.
 */

template <class Sh, class StableTag>
struct layer 
{
  typedef db::box_convert<Sh> box_convert;
  typedef typename Sh::coord_type coord_type;
  typedef typename db::box<coord_type> box_type;
  typedef typename box_tree_typedef<box_type, Sh, box_convert, StableTag>::box_tree_type box_tree_type;
  typedef typename box_tree_type::flat_iterator flat_iterator;
  typedef typename box_tree_type::const_iterator iterator;
  typedef typename box_tree_type::iterator non_const_iterator;
  typedef typename box_tree_type::touching_iterator touching_iterator;
  typedef typename box_tree_type::overlapping_iterator overlapping_iterator;

  /**
   *  @brief Default ctor: creates an empty layer object
   */
  layer ()
    : m_bbox_dirty (false), m_tree_dirty (false) 
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The copy constructor
   */
  layer (const layer &d)
  {
    operator= (d);
  }

  /**
   *  @brief The move constructor
   */
  layer (const layer &&d)
  {
    operator= (d);
  }

  /**
   *  @brief The assignment operator
   *
   *  The manager attachment is not copied.
   */
  layer &operator= (const layer &d)
  {
    if (&d != this) {
      m_box_tree = d.m_box_tree;
      m_bbox = d.m_bbox;
      m_bbox_dirty = d.m_bbox_dirty;
      m_tree_dirty = d.m_tree_dirty;
    }
    return *this;
  }

  /**
   *  @brief The assignment operator (move semantics)
   */
  layer &operator= (const layer &&d)
  {
    if (&d != this) {
      m_box_tree = d.m_box_tree;
      m_bbox = d.m_bbox;
      m_bbox_dirty = d.m_bbox_dirty;
      m_tree_dirty = d.m_tree_dirty;
    }
    return *this;
  }

  /**
   *  @brief Get the iterator for an object given by a pointer 
   */
  iterator iterator_from_pointer (const Sh *p) const
  {
    return m_box_tree.iterator_from_pointer (p);
  }
  
  /**
   *  @brief The translation operator
   *
   *  This operator is used to copy one layer to another repository space.
   *  The current layer will be overwritten. 
   *
   *  @param src The source layer
   *  @param rep The repository that is associated with *this and into which the
   *             shapes will be copied
   */
  void translate (const layer<Sh, StableTag> &d, db::generic_repository<coord_type> &rep, db::ArrayRepository &array_rep)
  {
    tl_assert (&d != this);

    clear ();
    reserve (d.size ());

    for (typename layer<Sh, StableTag>::iterator s = d.begin (); s != d.end (); ++s) {
      m_box_tree.insert (Sh ())->translate (*s, rep, array_rep);
    }

    m_bbox = d.m_bbox;
    m_bbox_dirty = d.m_bbox_dirty;
    m_tree_dirty = true;
  }

  /**
   *  @brief The translation operator
   *
   *  This operator is used to copy one layer to another repository space with a transformation.
   *  The current layer will be overwritten. 
   *
   *  @param src The source layer
   *  @param trans The transformation to apply
   *  @param rep The repository that is associated with *this and into which the
   *             shapes will be copied
   */
  template <class T>
  void translate (const layer<Sh, StableTag> &d, const T &trans, db::generic_repository<coord_type> &rep, db::ArrayRepository &array_rep)
  {
    tl_assert (&d != this);

    clear ();
    reserve (d.size ());

    for (typename layer<Sh, StableTag>::iterator s = d.begin (); s != d.end (); ++s) {
      m_box_tree.insert (Sh ())->translate (*s, trans, rep, array_rep);
    }

    m_bbox = d.m_bbox;
    m_bbox_dirty = d.m_bbox_dirty;
    m_tree_dirty = true;
  }

  /**
   *  @brief Insert a new shape object
   *
   *  Insert a new shape object. This will invalidate the sorted
   *  state and the bounding box. It will require a "update_bbox"
   *  and a "sort" call to restore these states.
   *
   *  @param sh The object (copy) to insert
   *  
   *  @return A reference to the object created. This reference
   *          is only guaranteed to be valid until the next insert
   *          or sort call.
   */
  iterator insert (const Sh &sh)
  {
    //  inserting will make the bbox and the tree "dirty" - i.e.
    //  it will need to be updated.
    m_bbox_dirty = true;
    m_tree_dirty = true;
    return m_box_tree.insert (sh);
  }

  /**
   *  @brief Insert a new shape object (move semantics)
   */
  iterator insert (const Sh &&sh)
  {
    //  inserting will make the bbox and the tree "dirty" - i.e.
    //  it will need to be updated.
    m_bbox_dirty = true;
    m_tree_dirty = true;
    return m_box_tree.insert (sh);
  }

  /**
   *  @brief Replace the given element with a new one
   *
   *  Replace the element at the position "pos" with the new
   *  element "sh".
   *  
   *  @param pos The position at which to replace the element
   *  @param sh The element to replace *pos
   * 
   *  @return A reference to the new element
   */
  Sh &replace (iterator pos, const Sh &sh)
  {
    m_bbox_dirty = true;
    m_tree_dirty = true;
    non_const_iterator ncpos;
    to_non_const_box_tree_iter (pos, ncpos, StableTag ());
    *ncpos = sh;
    return *ncpos;
  }

  /**
   *  @brief Replace the given element with a new one (move semantics)
   */
  Sh &replace (iterator pos, const Sh &&sh)
  {
    m_bbox_dirty = true;
    m_tree_dirty = true;
    non_const_iterator ncpos;
    to_non_const_box_tree_iter (pos, ncpos, StableTag ());
    *ncpos = sh;
    return *ncpos;
  }

  /**
   *  @brief Erasing of an element
   *
   *  Erase the element at the given position. Invalidates sorting
   *  and the bbox.
   */
  void erase (iterator pos)
  {
    m_bbox_dirty = true;
    m_tree_dirty = true;
    non_const_iterator ncpos;
    to_non_const_box_tree_iter (pos, ncpos, StableTag ());
    m_box_tree.erase (ncpos);
  }

  /**
   *  @brief Erasing of elements
   *
   *  Erase the elements at the given positions [from,to). 
   *  Invalidates sorting and the bbox.
   */
  void erase (iterator from, iterator to)
  {
    m_bbox_dirty = true;
    m_tree_dirty = true;
    non_const_iterator ncfrom, ncto;
    to_non_const_box_tree_iter (from, ncfrom, StableTag ());
    to_non_const_box_tree_iter (to, ncto, StableTag ());
    m_box_tree.erase (ncfrom, ncto);
  }

  /**
   *  @brief Erasing of multiple elements
   *
   *  Erase a set of positions given by an iterator I: *(from,to).
   *  *I must render an "iterator" object.
   *  The iterators in the sequence from, to must be sorted in
   *  "later" order.
   */
  template <class I>
  void erase_positions (I first, I last)
  {
    if (first != last) {
      m_bbox_dirty = true;
      m_tree_dirty = true;
      m_box_tree.erase_positions (first, last);
    }
  }

  /**
   *  @brief Insertion of a range [from,to)
   */
  template <class I>
  void insert (I from, I to)
  {
    //  inserting will make the bbox and the tree "dirty" - i.e.
    //  it will need to be updated.
    m_bbox_dirty = true;
    m_tree_dirty = true;
    m_box_tree.insert (from, to);
  }

  /** 
   *  @brief update the bounding box if required
   */
  void update_bbox ()
  {
    //  Only do so, if the bbox is dirty (needs update)
    if (m_bbox_dirty) {

      //  determine the bounding box
      box_convert bc = box_convert ();
      m_bbox = box_type ();
      for (typename box_tree_type::const_iterator o = m_box_tree.begin (); o != m_box_tree.end (); ++o) {
        m_bbox += bc(*o);
      }

      m_bbox_dirty = false;
    }
  }

  /** 
   *  @brief Retrieve the bounding box 
   */
  const box_type &bbox () const
  {
    //  update the bbox if required
    tl_assert (! m_bbox_dirty);
    return m_bbox;
  }

  /**
   *  @brief Restore the sorted state
   */
  void sort () 
  {
    //  only sort if not done already
    if (m_tree_dirty) {
      //  and actually sort the tree
      box_convert bc = box_convert ();
      m_box_tree.sort (bc);
      m_tree_dirty = false;
    }
  }

  /**
   *  @brief Clear the layer
   */
  void clear ()
  {
    m_bbox = box_type ();
    m_box_tree.clear ();
    m_bbox_dirty = false;
    m_tree_dirty = false;
  }

  /**
   *  @brief A "flat" query (see box_tree::flat_iterator for a description)
   */
  flat_iterator begin_flat () const 
  {
    //  we do not assert !is_dirty here for two reasons: first, in unstable mode, this is not necessary
    //  and second, in stable mode, it might be by intention, if the shape iterator moves on to a 
    //  shape group that has been updated in between and should *not* iterate over the new set ...
    return m_box_tree.begin_flat ();
  }

  /**
   *  @brief A "touching" region query
   */
  touching_iterator begin_touching (const box_type &b) const 
  {
    //  sort the tree if required
    tl_assert (! m_tree_dirty);
    box_convert bc = box_convert ();
    return m_box_tree.begin_touching (b, bc);
  }

  /**
   *  @brief A "overlapping" region query
   */
  overlapping_iterator begin_overlapping (const box_type &b) const 
  {
    //  sort the tree if required
    tl_assert (! m_tree_dirty);
    box_convert bc = box_convert ();
    return m_box_tree.begin_overlapping (b, bc);
  }

  /**
   *  @brief Find a shape in the layer
   *
   *  This is a precise search. It returns end() if there is no
   *  shape exactly matching the one provided.
   */
  iterator find (const Sh &sh) const
  {
    //  TODO: this could be done more efficiently with an exact region search 
    //  if we had a converter of a touching iterator to a normal iterator
    for (iterator s = begin (); s != end (); ++s) {
      if (*s == sh) { 
        return s;
      }
    }
    return end ();
  }

  /**
   *  @brief The normal begin iterator returning the begin of all elements
   */
  iterator begin () const
  {
    return m_box_tree.begin ();
  }

  /**
   *  @brief The normal end iterator returning the past-end position of all elements
   */
  iterator end () const
  {
    return m_box_tree.end ();
  }

  /**
   *  @brief Return true if the bounding box needs update
   */
  bool is_bbox_dirty () const
  {
    return m_bbox_dirty;
  }

  /**
   *  @brief Return true if the tree needs update
   */
  bool is_tree_dirty () const
  {
    return m_tree_dirty;
  }

  /**
   *  @brief Reserve a certain number of elements
   */
  void reserve (size_t n)
  {
    m_box_tree.reserve (n);
  }

  /**
   *  @brief Reserve a certain number of elements
   */
  size_t size () const
  {
    return m_box_tree.size ();
  }

  /**
   *  @brief Reserve a certain number of elements
   */
  bool empty () const
  {
    return m_box_tree.empty ();
  }

  /**
   *  @brief Swaps the layer with another one
   */
  void swap (layer &other)
  {
    m_box_tree.swap (other.m_box_tree);
    std::swap (m_bbox, other.m_bbox);
    bool x;
    x = other.m_bbox_dirty; other.m_bbox_dirty = m_bbox_dirty; m_bbox_dirty = x;
    x = other.m_tree_dirty; other.m_tree_dirty = m_tree_dirty; m_tree_dirty = x;
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (! no_self) {
      stat->add (typeid (layer), (void *) this, sizeof (layer), sizeof (layer), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_box_tree, true, (void *) this);
  }

private:
  box_tree_type m_box_tree;
  box_type m_bbox;
  bool m_bbox_dirty : 8;
  bool m_tree_dirty : 8;
};

/**
 *  @brief Collect memory statistics
 */
template <class Sh, class StableTag>
inline void
mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const layer<Sh, StableTag> &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif

