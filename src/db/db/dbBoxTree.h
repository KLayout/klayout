
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


#ifndef HDR_dbBoxTree
#define HDR_dbBoxTree

#include "tlVector.h"
#include "tlReuseVector.h"
#include "dbBox.h"
#include "dbMemStatistics.h"

#include <limits>
#include <vector>

namespace db
{

//  forward definitions for db::BoxConvert
struct simple_bbox_tag;
struct complex_bbox_tag;

/// @brief a helper class required for the box_tree implementation

template <class Box, class Obj, class BoxConv, class Vector>
class box_tree_picker 
{
public:
  typedef Box box_type;
  typedef Vector object_vector_type;
  typedef typename object_vector_type::const_iterator const_iterator;

  box_tree_picker ()
  { }

  box_tree_picker (const BoxConv &box_conv)
    : m_box_conv (box_conv)
  { }

  box_type operator() (const Obj *o) const 
  {
    return m_box_conv (*o);
  }

  void rotate_boxes (int /*q*/, const_iterator /*e*/, const_iterator /*o0*/, const_iterator /*o1*/, const_iterator /*o2*/, const_iterator /*o3*/, const_iterator /*o4*/) 
  {
    //  .. nothing yet ..
  }

private:
  BoxConv m_box_conv;
};


/// @brief a helper class providing a linear-time iterator difference which is not necessarily 
///        the actual difference but monotonous

template <class X, bool R>
size_t box_tree_lt_difference (const tl::reuse_vector_const_iterator<X, R> &a, const tl::reuse_vector_const_iterator<X, R> &b)
{
  return a.index () - b.index ();
}

template <class X, bool R>
size_t box_tree_lt_difference_ptr (const X *a, const tl::reuse_vector_const_iterator<X, R> &b)
{
  return a - b.unsafe_target_addr ();
}

template <class Iter>
size_t box_tree_lt_difference (const Iter &a, const Iter &b)
{
  return a - b;
}

template <class X, class Iter>
size_t box_tree_lt_difference_ptr (const X *a, const Iter &b)
{
  return a - &*b;
}

/// @brief a helper class required for the box_tree implementation

template <class Obj, class Box, class BoxConv, class Vector>
class box_tree_cached_picker 
{
public:
  typedef Box box_type;
  typedef Vector object_vector_type;
  typedef typename object_vector_type::const_iterator const_iterator;

  box_tree_cached_picker ()
  { }

  box_tree_cached_picker (const BoxConv &box_conv, const_iterator from, const_iterator to)
    : m_from (from)
  { 
    m_boxes.resize (box_tree_lt_difference (to, from), box_type ());

    for (const_iterator o = from; o != to; ++o) {
      box_type b = box_conv (*o);
      m_boxes [box_tree_lt_difference (o, from)] = b;
      m_bbox += b;
    }
  }

  const box_type &operator() (const Obj *o) const 
  {
    return m_boxes [box_tree_lt_difference_ptr (o, m_from)];
  }

  const box_type &bbox () const
  {
    return m_bbox;
  }

  void rotate_boxes (int q, const_iterator e, const_iterator o0, const_iterator o1, const_iterator o2, const_iterator o3, const_iterator o4) 
  {
    size_t qi [5] = {
      box_tree_lt_difference (o0, m_from), 
      box_tree_lt_difference (o1, m_from), 
      box_tree_lt_difference (o2, m_from), 
      box_tree_lt_difference (o3, m_from), 
      box_tree_lt_difference (o4, m_from)
    };

    box_type bx = m_boxes [box_tree_lt_difference (e, m_from)];
    for (int i = 4; i > q; --i) {
      m_boxes [qi [i]] = m_boxes [qi [i - 1]];
    }
    m_boxes [qi [q]] = bx;
  }

private:
  const_iterator m_from;
  box_type m_bbox;
  tl::vector<box_type> m_boxes;
};

/**
 *  @brief The node object
 */

template <class Tree>
class box_tree_node
{
public:
  typedef typename Tree::point_type point_type;
  typedef typename Tree::coord_type coord_type;
  typedef typename Tree::box_type box_type;

  box_tree_node (box_tree_node *parent, const point_type &center, const box_type &qbox, unsigned int quad)
  {
    point_type corner;
    if (quad == 0) {
      corner = qbox.upper_right ();
    } else if (quad == 1) {
      corner = qbox.upper_left ();
    } else if (quad == 2) {
      corner = qbox.lower_left ();
    } else if (quad == 3) {
      corner = qbox.lower_right ();
    }

    init (parent, center, corner, quad);
  }

  ~box_tree_node ()
  {
    for (int i = 0; i < 4; ++i) {
      box_tree_node *c = child (i);
      if (c) {
        delete c;
      }
    }
  }

  box_tree_node *clone (box_tree_node *parent = 0, unsigned int quad = 0) const
  {
    box_tree_node *n = new box_tree_node (parent, m_center, m_corner, quad);
    n->m_lenq = m_lenq;
    n->m_len = m_len;
    for (unsigned int i = 0; i < 4; ++i) {
      box_tree_node *c = child (i);
      if (c) {
        c->clone (n, i);
      } else {
        n->m_childrefs [i] = m_childrefs [i];
      }
    }
    return n;
  }

  box_tree_node *child (int i) const
  {
    if ((m_childrefs [i] & 1) == 0) {
      return reinterpret_cast<box_tree_node *> (m_childrefs [i]);
    } else {
      return 0;
    }
  }

  void lenq (int i, size_t l) 
  {
    if (i < 0) {
      m_lenq = l;
    } else {
      box_tree_node *c = child (i);
      if (c) {
        c->m_len = l;
      } else {
        m_childrefs [i] = l * 2 + 1;
      }
    }
  }

  size_t lenq (int i) const
  {
    if (i < 0) {
      return m_lenq;
    } else {
      box_tree_node *c = child (i);
      if (c) {
        return c->m_len;
      } else {
        return m_childrefs [i] >> 1;
      }
    }
  }

  box_tree_node *parent () const
  {
    return (box_tree_node *)((char *) mp_parent - (size_t (mp_parent) & 3));
  }

  int quad () const
  {
    return (int)(size_t (mp_parent) & 3);
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent)
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    for (int i = 0; i < 4; ++i) {
      if (child (i)) {
        child (i)->mem_stat (stat, purpose, cat, no_self, parent);
      }
    }
  }

  const point_type &center () const
  {
    return m_center;
  }

  box_type quad_box (int quad) const
  {
    box_type qb = box_type::world ();
    if (parent ()) {
      qb = box_type (m_corner, parent ()->center ());
    }

    switch (quad) {
    case 0: return box_type (m_center, qb.upper_right ());
    case 1: return box_type (m_center, qb.upper_left ());
    case 2: return box_type (m_center, qb.lower_left ());
    case 3: return box_type (m_center, qb.lower_right ());
    default: return qb;
    }
  }

private:
  box_tree_node *mp_parent;
  size_t m_lenq, m_len;
  size_t m_childrefs [4];
  point_type m_center, m_corner;

  box_tree_node (const box_tree_node &d);
  box_tree_node &operator= (const box_tree_node &d);

  box_tree_node (box_tree_node *parent, const point_type &center, const point_type &corner, unsigned int quad)
  {
    init (parent, center, corner, quad);
  }

  void init (box_tree_node *parent, const point_type &center, const point_type &corner, unsigned int quad)
  {
    m_center = center;
    m_corner = corner;

    m_lenq = m_len = 0;
    for (int i = 0; i < 4; ++i) {
      m_childrefs [i] = 0;
    }

    mp_parent = (box_tree_node *)((char *) parent + quad);
    if (parent) {
      m_len = (parent->m_childrefs [quad] >> 1);
      parent->m_childrefs [quad] = size_t (this);
    }
  }
};

/**
 *  @brief The flat iterator class
 *
 *  The iterator provides a flat, all-elements iterator for the box tree.
 *  Unlike the primitive iterator, which is based on the container inside
 *  the box tree, this iterator iterates "registered" elements, i.e. such that
 *  are within the sorted list. Since this list is maintained even if elements 
 *  are inserted, this iterator is less susceptible to changes in the container, 
 *  i.e. by inserting or deleting elements while iterating.
 *  This iterator is therefore recommended for being used in interleaved access/change
 *  operations.
 */

template <class Tree>
class box_tree_flat_it
{
public:
  typedef typename Tree::object_type object_type;
  typedef typename Tree::point_type point_type;
  typedef typename Tree::coord_type coord_type;
  typedef typename Tree::box_type box_type;
  typedef typename Tree::const_iterator const_iterator;
  typedef typename Tree::box_tree_picker_type value_picker_type;
  typedef typename Tree::size_type size_type;
  typedef object_type value_type;

  box_tree_flat_it ()
    : mp_tree (0)
  { 
    m_index = 0;
  }

  box_tree_flat_it (const Tree &t)
    : mp_tree (&t)
  { 
    m_index = 0;
  }

  box_tree_flat_it<Tree> &operator++ () 
  {
    ++m_index;
    return *this;
  }

  const object_type &operator* () const 
  {
    return mp_tree->objects ().item (mp_tree->elements () [m_index]);
  }
  
  const object_type *operator-> () const 
  {
    return &mp_tree->objects ().item (mp_tree->elements () [m_index]);
  }
  
  size_t position () const 
  {
    return m_index;
  }
  
  bool operator== (const box_tree_flat_it<Tree> &i) const
  {
    return m_index == i.m_index;
  }

  bool operator!= (const box_tree_flat_it<Tree> &i) const
  {
    return m_index != i.m_index;
  }

  /**
   *  @brief returns true if at end
   */
  bool at_end () const
  {
    return mp_tree == 0 || m_index == mp_tree->elements ().size ();
  }

private:
  size_t m_index;
  const Tree *mp_tree;
};

/**
 *  @brief The iterator class
 *
 *  The iterator provides a region-select iterator for the box tree.
 *  It requires a compare operator (type Cmp) that provides coordinate-wise
 *  and box-wise comparison.
 */

template <class Tree, class Cmp>
class box_tree_it
{
public:
  typedef typename Tree::object_type object_type;
  typedef typename Tree::point_type point_type;
  typedef typename Tree::coord_type coord_type;
  typedef typename Tree::box_type box_type;
  typedef typename Tree::element element;
  typedef typename Tree::const_iterator const_iterator;
  typedef typename Tree::box_tree_picker_type value_picker_type;
  typedef typename Tree::size_type size_type;
  typedef object_type value_type;
  typedef db::box_tree_node<Tree> box_tree_node;

  box_tree_it ()
    : mp_tree (0), m_picker (), m_compare () 
  { 
    mp_node = 0;
    m_index = 0;
    m_offset = 0;
    m_quad = -1;
  }

  box_tree_it (const Tree &t, value_picker_type p, const Cmp &c) 
    : mp_tree (&t), m_picker (p), m_compare (c) 
  { 
    mp_node = t.root ();
    m_index = 0;
    m_offset = 0;
    m_quad = -1;

    while (mp_node && mp_node->lenq (m_quad) == 0) {
      if (! next ()) {
        mp_node = 0;
      } else {
        down ();
      }
    }
    while (! at_end ()) {
      if (check ()) {
        break;
      }
      inc ();
    }
  }

  box_tree_it<Tree, Cmp> &operator++ () 
  {
    while (true) {
      inc ();
      if (at_end () || check ()) {
        break;
      }
    }
    return *this;
  }

  const object_type &operator* () const 
  {
    return mp_tree->objects ().item (mp_tree->elements () [m_index + m_offset]);
  }
  
  const object_type *operator-> () const 
  {
    return &mp_tree->objects ().item (mp_tree->elements () [m_index + m_offset]);
  }
  
  size_t position () const 
  {
    return m_index + m_offset;
  }
  
  bool operator== (const box_tree_it<Tree, Cmp> &i) const
  {
    return mp_node == i.mp_node;
  }

  bool operator!= (const box_tree_it<Tree, Cmp> &i) const
  {
    return mp_node != i.mp_node;
  }

  /**
   *  @brief returns true if at end
   */
  bool at_end () const
  {
    return mp_tree == 0 || m_index + m_offset == mp_tree->elements ().size ();
  }

  /**
   *  @brief Returns the quad ID
   *
   *  The quad ID is a value that can be used to determine whether the iterator entered the next quad.
   *  It is possible to optimize the search if there is no need to look into that quad and skip it.
   */
  size_t quad_id () const 
  {
    return mp_node ? size_t (mp_node) + size_t (m_quad + 1) : size_t (mp_tree);
  }

  /**
   *  @brief Returns the current's quad box
   *
   *  This method will return the world box if the root node is selected
   */
  box_type quad_box () const
  {
    if (! mp_node) {
      return box_type::world ();
    } else {
      return mp_node->quad_box (m_quad);
    }
  }

  /**
   *  @brief Skips the quad and move to the next one
   */
  void skip_quad ()
  {
    if (! mp_node) {
      m_offset = mp_tree->elements ().size ();
    } else {
      m_offset = 0;
      while (! next ()) {
        if (! up ()) {
          return;
        }
      }
      down ();
    }
  }

private:
  box_tree_node *mp_node;
  size_t m_index;
  size_t m_offset;
  int m_quad;
  const Tree *mp_tree;
  value_picker_type m_picker;
  Cmp m_compare;

  //  return true if the node is within the search range
  bool check () const
  {
    bool ret = m_compare.matches_obj (**this);
    return ret;
  }
  
  //  check if the current quad needs visit
  bool need_visit () const
  {
    if (mp_node->lenq (m_quad) == 0) {
      return false;
    }
    if (m_quad < 0) {
      return true;
    } else {
      //  TODO: simplify this, i.e. by providing a special method on the compare function ..
      coord_type m = std::numeric_limits<coord_type>::max ();
      point_type c = mp_node->center ();
      if (m_quad == 0) {
        return m_compare.matches_box (box_type (c, point_type (m, m)));
      } else if (m_quad == 1) {
        return m_compare.matches_box (box_type (-m, c.y (), c.x (), m));
      } else if (m_quad == 2) {
        return m_compare.matches_box (box_type (point_type (-m, -m), c));
      } else {
        return m_compare.matches_box (box_type (c.x (), -m, m, c.y ()));
      }
    }
  }

  //  one level up. 
  bool up () 
  {
    box_tree_node *p = mp_node->parent ();
    if (p) {

      //  move the position to the beginning of the child node
      for (int q = -1; q < m_quad; ++q) {
        m_index -= mp_node->lenq (q);
      }

      m_quad = mp_node->quad ();
      mp_node = p;

      return true;

    } else {
      mp_node = 0;
      return false;
    }
  }

  //  move to next quad
  //  returns true if this is possible
  bool next () 
  {
    m_index += mp_node->lenq (m_quad);
    ++m_quad;
    while (m_quad < 4 && ! need_visit ()) {
      m_index += mp_node->lenq (m_quad);
      ++m_quad;
    }
    return m_quad < 4;
  }

  //  down as many levels as required for the next non-empty quad
  //  returns true if this is possible
  bool down ()
  {
    while (true) {

      box_tree_node *c = mp_node->child (m_quad);
      if (! c) {
        return false;
      }

      mp_node = c;
      m_quad = -1;
      while (m_quad < 4 && ! need_visit ()) {
        m_index += mp_node->lenq (m_quad);
        ++m_quad;
      }

      if (m_quad == 4) {
        //  nothing to visit: up again
        up ();
        return false;
      } else if (m_quad < 0) {
        //  stay in main chunk
        return true;
      }

    }
  }

  //  increment iterator
  void inc ()
  {
    if (! mp_node || m_offset + 1 < mp_node->lenq (m_quad)) {
      ++m_offset;
    } else {
      m_offset = 0;
      while (! next ()) {
        if (! up ()) {
          return;
        }
      }
      down ();
    }
  }
};

/// @brief a helper class required for the box_tree implementation

template <class Box, class Obj, class BoxConv, class BoxPred>
class box_tree_sel
{
public:
  typedef typename Box::coord_type coord_type;

  box_tree_sel ()
    : m_b (), m_bpred (), m_conv ()
  { 
    //  .. nothing yet ..
  }

  box_tree_sel (const Box &b, const BoxConv &conv) 
    : m_b (b), m_bpred (), m_conv (conv)
  { 
    //  .. nothing yet ..
  }

  bool matches_obj (const Obj &o) const
  {
    return m_bpred (m_conv (o), m_b);
  }

  bool matches_box (const Box &b) const
  {
    return m_bpred (b, m_b);
  }

private:
  Box m_b;
  BoxPred m_bpred;
  BoxConv m_conv;
};

/** 
 *  @brief box tree object
 *
 *  A box tree is a vector with special sorting and
 *  query capabilities. It contains objects of type
 *  Obj that can be converted to db::box<C> objects with
 *  the BoxConv function. 
 *  A box tree can be in state "inserting", in which the
 *  insertion of new objects is supported, or can be 
 *  sorted, after which it can be queried for objects 
 *  whose box overlaps or touches a specified test box.
 */

template <class Box, class Obj, class BoxConv, size_t min_bin = 100, size_t min_quads = 100, unsigned int thin_aspect = 4>
class box_tree 
{
public:
  typedef Box box_type;
  typedef BoxConv box_conv_type;
  typedef typename Box::coord_type coord_type;
  typedef Obj object_type;
  typedef db::point<coord_type> point_type;
  typedef tl::reuse_vector<object_type> obj_vector_type;
  typedef size_t size_type;
  typedef typename obj_vector_type::const_iterator const_iterator;
  typedef typename obj_vector_type::iterator iterator;
  typedef size_type element;
  typedef tl::vector<element> element_vector_type;
  typedef typename element_vector_type::iterator element_iterator;
  typedef box_tree<box_type, object_type, box_conv_type, min_bin, min_quads> box_tree_type;
  typedef db::box_tree_node<box_tree_type> box_tree_node;
  typedef box_tree_sel<box_type, object_type, box_conv_type, db::boxes_overlap<box_type> > box_tree_sel_overlap_type;
  typedef box_tree_sel<box_type, object_type, box_conv_type, db::boxes_touch<box_type> > box_tree_sel_touch_type;
  typedef box_tree_it<box_tree_type, box_tree_sel_touch_type> touching_iterator;
  typedef box_tree_it<box_tree_type, box_tree_sel_overlap_type> overlapping_iterator;
  typedef box_tree_flat_it<box_tree_type> flat_iterator;
  typedef box_tree_picker<Box, Obj, BoxConv, obj_vector_type> box_tree_picker_type;

  /**
   *  @brief Creates a empty box tree object 
   */
  box_tree ()
    : mp_root (0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Copy constructor
   */
  box_tree (const box_tree &b)
    : m_objects (b.m_objects), m_elements (b.m_elements), mp_root (b.mp_root ? b.mp_root->clone () : 0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Move constructor
   */
  box_tree (box_tree &&b)
    : m_objects (b.m_objects), m_elements (b.m_elements), mp_root (b.mp_root)
  {
    b.mp_root = 0;
  }

  /**
   *  @brief Assignment 
   */
  box_tree &operator= (const box_tree &b)
  {
    clear ();
    m_objects = b.m_objects;
    m_elements = b.m_elements;
    if (b.mp_root) {
      mp_root = b.mp_root->clone ();
    }
    return *this;
  }

  /**
   *  @brief Assignment (move)
   */
  box_tree &operator= (box_tree &&b)
  {
    clear ();
    m_objects = b.m_objects;
    m_elements = b.m_elements;
    if (b.mp_root) {
      mp_root = b.mp_root;
      b.mp_root = 0;
    }
    return *this;
  }

  /**
   *  @brief The destructor
   */
  ~box_tree ()
  {
    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;
  }

  /**
   *  @brief Insert a new object into the box tree
   *
   *  Inserts a new object into the box tree. The object
   *  is appended at the end of the vector. Sorting is 
   *  invalidated through this operation.
   *
   *  @param o The object to insert
   */
  iterator insert (const Obj &o)
  {
    return m_objects.insert (o);
  }

  /**
   *  @brief reserve space for a given number of elements
   */
  void reserve (size_t n)
  {
    m_objects.reserve (n);
  }

  /**
   *  @brief resize to a given number of elements
   */
  void resize (size_t n)
  {
    m_objects.resize (n);
  }

  /** 
   *  @brief Insert a range of objects
   *
   *  Analogous to the other insert method, but accepting
   *  and range of objects [start,end).
   */
  template <class I> 
  void insert (I from, I to)
  {
    m_objects.reserve (m_objects.size () + std::distance (from, to));
    for (I i = from; i != to; ++i) {
      m_objects.insert (*i);
    }
  }

  /**
   *  @brief Replace an object
   *  
   *  Replace the object at the given (const) iterator position
   *  with the object given. Replacing an object invalidates
   *  the sorting state.
   *
   *  @param pos The position of the object to replace
   *  @param obj The object to replace the current one with
   */
  void replace (const_iterator pos, const Obj &obj)
  {
    m_objects [std::distance (((const box_tree_type *) this)->begin (), pos)] = obj;
  }

  /**
   *  @brief Map a const iterator to a non-const one
   */
  iterator nc_iter (const_iterator pos)
  {
    return begin () + (pos - ((const box_tree *) this)->begin ());
  }

  /**
   *  @brief Erase an object
   *  
   *  Erase the object pointed to by the given iterator.
   *  Erasing an object will invalidate sorting. Erasing is
   *  not a cheap operation and its complexity is O(n).
   *
   *  @param pos The position of the object to erase
   */
  void erase (iterator pos)
  {
    m_objects.erase (pos);
  }

  /**
   *  @brief Erase an object sequence
   *  
   *  Erase the objects [from,to) to by the given iterator.
   *  Erasing an object will invalidate sorting. Erasing is
   *  not a cheap operation and its complexity is O(n).
   *
   *  @param pos The position of the object to erase
   */
  void erase (iterator from, iterator to)
  {
    m_objects.erase (from, to);
  }

  /**
   *  @brief Erase several objects
   *  
   *  Erase the objects given by a set of iterators (given by a sequence [from,to)).
   *  The iterators given must be sorted in ascending order.
   *  Erasing an object will invalidate sorting. Erasing is
   *  not a cheap operation and its complexity is O(n).
   *
   *  @param first The iterator delivering the iterators
   *  @param last The end iterator for the list of iterators
   */
  template <class I>
  void erase_positions (I first, I last)
  {
    iterator t = begin ();
    for (iterator i = begin (); i != end (); ++i) {
      if (first == last || i != *first) {
        if (&(*t) != &(*i)) {
          *t = *i;
        }
        ++t;
      } else {
        ++first;
      }
    }

    if (t != end ()) {
      erase (t, end ());
    }
  }

  /**
   *  @brief Returns the size of the vector.
   *
   *  @return The number of elements in the vector.
   */
  size_type size () const
  {
    return m_objects.size ();
  }

  /** 
   *  @brief Empty the vector.
   */
  void clear ()
  {
    m_objects.clear ();
    m_elements.clear ();
    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;
  }

  /**
   *  @brief Make the index 
   *
   *  Only after making the index, the flat iterators are available.
   *  Complexity is O(N).
   */
  void make_index ()
  {
    m_elements.clear ();
    m_elements.reserve (m_objects.size ());

    for (typename obj_vector_type::const_iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
      m_elements.push_back (o.index ());
    }
  }

  /**
   *  @brief Sort the vector
   *
   *  Only after sorting the query iterators are available.
   *  Sorting complexity is approx O(N*log(N)).
   */
  void sort (const BoxConv &conv)
  {
    typename BoxConv::complexity complexity_tag;
    sort (conv, complexity_tag);
  }

  /**
   *  @brief Direct access to the underlying vector
   *
   *  @return A reference to the underlying object vector
   */
  const obj_vector_type &objects () const
  {
    return m_objects;
  }
  
  /**
   *  @brief Direct access to the underlying element vector
   *
   *  @return A reference to the underlying element vector
   */
  const element_vector_type &elements () const
  {
    return m_elements;
  }
  
  /**
   *  @brief Get the iterator for an object given by a pointer (non-const version)
   */
  iterator iterator_from_pointer (object_type *p) 
  {
    return m_objects.iterator_from_pointer (p);
  }
  
  /**
   *  @brief Get the iterator for an object given by a pointer
   */
  const_iterator iterator_from_pointer (const object_type *p) const
  {
    return m_objects.iterator_from_pointer (p);
  }
  
  /**
   *  @brief Test, if an object is member of this tree
   */
  template <class V>
  bool is_member_of (const V *p) const
  {
    return m_objects.is_member_of (p);
  }
  
  /**
   *  @brief Test, if the box tree is empty
   */
  bool empty () const
  {
    return m_objects.empty ();
  }
  
  /**
   *  @brief Sequential access begin iterator
   *
   *  The normal sequential iterator delivers object by
   *  object in the sequence the objects have been stored
   *  in the vector. Using the sequential access iterator
   *  does not require a sorted vector.
   *
   *  @return A const iterator pointing to the first
   *  element in the vector
   */
  const_iterator begin () const
  {
    return m_objects.begin ();
  }

  /**
   *  @brief Sequential access end iterator
   *
   *  See %begin for a detailed description
   *
   *  @return A const iterator pointing past the end
   *  of the vector
   */
  const_iterator end () const
  {
    return m_objects.end ();
  }
  
  /**
   *  @brief Sequential access begin iterator
   *
   *  The normal sequential iterator delivers object by
   *  object in the sequence the objects have been stored
   *  in the vector. Using the sequential access iterator
   *  does not require a sorted vector.
   *
   *  @return A non-const iterator pointing to the first
   *  element in the vector
   */
  iterator begin () 
  {
    return m_objects.begin ();
  }

  /**
   *  @brief Sequential access end iterator
   *
   *  See %begin for a detailed description
   *
   *  @return A non-const iterator pointing past the end
   *  of the vector
   */
  iterator end () 
  {
    return m_objects.end ();
  }

  /**
   *  @brief flat iterator
   *
   *  This delivers the flat (non-constrained) iterator. 
   *  Unlike the primitive iterator, the flat iterator delivers
   *  elements that are registered upon sort() - i.e. the sequence
   *  does not change if elements are inserted or deleted unless
   *  sort() is being called.
   */
  flat_iterator begin_flat () const 
  {
    return box_tree_flat_it<box_tree_type> (*this);
  }
  
  /** 
   *  @brief selection of objects touching the test box
   *
   *  The const iterator returned by this method will
   *  iterator over all objects touching the test box
   *  with their own box (as converted by BoxConv).
   *  The sequence the objects are delivered is arbitrary.
   *
   *  @param b The test box
   *  @param conv The box converter instance
   *
   *  @return The const touching iterator pointing to the first Object.
   */
  touching_iterator begin_touching (const Box &b, BoxConv conv) const 
  {
    box_tree_picker_type p (conv);
    box_tree_sel_touch_type s (b, conv);
    return box_tree_it<box_tree_type, box_tree_sel_touch_type> (*this, p, s);
  }
  
  /** 
   *  @brief selection of objects overlapping the test box
   *
   *  The const iterator returned by this method will
   *  iterator over all objects touching the test box
   *  with their own box (as converted by BoxConv).
   *  The sequence the objects are delivered is arbitrary.
   *
   *  @param b The test box
   *  @param conv The box converter instance
   *
   *  @return The const overlapping iterator pointing to the first Object.
   */
  overlapping_iterator begin_overlapping (const Box &b, BoxConv conv) const 
  {
    box_tree_picker_type p (conv);
    box_tree_sel_overlap_type s (b, conv);
    return box_tree_it<box_tree_type, box_tree_sel_overlap_type> (*this, p, s);
  }
  
  /**
   *  @brief Access to the root node object
   *
   *  This is mainly used by the iterator implementation
   */
  box_tree_node *root () const
  {
    return mp_root;
  }

  /**
   *  @brief Swaps the box tree with another one
   */
  void swap (box_tree &other)
  {
    m_objects.swap (other.m_objects);
    m_elements.swap (other.m_elements);
    std::swap (mp_root, other.mp_root);
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_objects, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_elements, true, (void *) this);
  }

private:

  /// The basic object and element vector
  obj_vector_type m_objects;
  element_vector_type m_elements;
  box_tree_node *mp_root;

  /// Sort implementation for simple bboxes - no caching
  void sort (const BoxConv &conv, const db::simple_bbox_tag &/*complexity*/)
  {
    m_elements.clear ();
    m_elements.reserve (m_objects.size ());

    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;

    if (! m_objects.empty ()) {

      box_tree_picker_type picker (conv);

      box_type bbox;
      for (typename obj_vector_type::const_iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
        box_type b = conv (*o);
        m_elements.push_back (o.index ());
        bbox += b;
      }

      //  TODO: resize m_elements to actual size ?

      tree_sort (0, m_elements.begin (), m_elements.end (), picker, bbox, 0);

    }
  }

  /// Sort implementation for complex bboxes - with caching
  void sort (const box_conv_type &conv, const db::complex_bbox_tag &/*complexity*/)
  {
    m_elements.clear ();
    m_elements.reserve (m_objects.size ());

    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;

    if (! m_objects.empty ()) {

      box_tree_cached_picker<object_type, box_type, box_conv_type, obj_vector_type> picker (conv, m_objects.begin (), m_objects.end ());

      for (typename obj_vector_type::const_iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
        m_elements.push_back (o.index ());
      }

      //  TODO: resize m_elements to actual size ?

      tree_sort (0, m_elements.begin (), m_elements.end (), picker, picker.bbox (), 0);

    }
  }

  template <class CoordPicker>
  void tree_sort (box_tree_node *parent, element_iterator from, element_iterator to, const CoordPicker &picker, const box_type &bbox, int quad)
  {
    size_t ntot = size_t (to - from);
    if (ntot <= min_bin || (bbox.width () < 2 && bbox.height () < 2)) {
      return; //  not worth splitting
    } 

    //  the bins are: overall, ur, ul, ll, lr, empty
    element_iterator qloc [6] = { from, from, from, from, from, from };
    point_type center;
    if (bbox.width () < bbox.height () / thin_aspect) {
      //  separate by height only
      center = point_type (bbox.left (), bbox.bottom () + bbox.height () / 2);
    } else if (bbox.height () < bbox.width () / thin_aspect) {
      //  separate by width only
      center = point_type (bbox.left () + bbox.width () / 2, bbox.bottom ());
    } else {
      center = bbox.center ();
    }

    for (element_iterator e = from; e != to; ++e) {

      box_type b = picker (&m_objects.item (*e));

      int q = 0;
      if (b.empty ()) {
        q = 5;
      } else if (b.right () <= center.x ()) {
        if (b.top () <= center.y ()) {
          q = 3;
        } else if (b.bottom () >= center.y ()) {
          q = 2;
        }
      } else if (b.left () >= center.x ()) {
        if (b.top () <= center.y ()) {
          q = 4;
        } else if (b.bottom () >= center.y ()) {
          q = 1;
        }
      }

      //  make space for the element and swap the new element into position 
      if (q < 5) {
        element el = *e;
        for (int i = 5; i > q; --i) {
          *qloc [i] = *qloc [i - 1];
          ++qloc [i];
        }
        *qloc [q] = el;
      }
      ++qloc [q];
      
    }

    //  compute sizes of quad fields
    size_t nx, n[4];
    size_t nn = 0;
    nx = std::distance (from, qloc[0]);
    for (int i = 0; i < 4; ++i) {
      n[i] = std::distance (qloc [i], qloc[i + 1]);
      nn += n[i];
    }

    //  is it worth to split into sub-quads?
    if (nn >= min_quads) {

      //  create a new node representing this tree
      box_tree_node *node = new box_tree_node (parent, center, bbox, quad);
      if (parent == 0) {
        mp_root = node;
      }

      //  tell the parent the length of the "overall" bin
      node->lenq (-1, nx);

      //  yes: create sub-quads
      box_type qboxes [4];
      qboxes [0] = box_type (center, bbox.p2 ()); 
      qboxes [1] = box_type (bbox.left (), center.y (), center.x (), bbox.top ());
      qboxes [2] = box_type (bbox.p1 (), center);
      qboxes [3] = box_type (center.x (), bbox.bottom (), bbox.right (), center.y ());
      for (unsigned int q = 0; q < 4; ++q) {
        if (n[q] > 0) {
          node->lenq (q, n[q]);
          tree_sort (node, qloc[q], qloc[q + 1], picker, qboxes [q], int (q));
        }
      }

    } 

  }

};

/**
 *  @brief Collect memory statistics
 */
template <class Box, class Obj, class BoxConv, size_t min_bin, size_t min_quads>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const box_tree<Box, Obj, BoxConv, min_bin, min_quads> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief The flat iterator class
 *
 *  The iterator provides a flat, all-elements iterator for the box tree.
 *  Unlike the primitive iterator, which is based on the container inside
 *  the box tree, this iterator iterates "registered" elements, i.e. such that
 *  are within the sorted list. Since this list is maintained even if elements 
 *  are inserted, this iterator is less susceptible to changes in the container, 
 *  i.e. by inserting or deleting elements while iterating.
 *  This iterator is therefore recommended for being used in interleaved access/change
 *  operations.
 */

template <class Tree>
class unstable_box_tree_flat_it
{
public:
  typedef typename Tree::object_type object_type;
  typedef typename Tree::point_type point_type;
  typedef typename Tree::coord_type coord_type;
  typedef typename Tree::box_type box_type;
  typedef typename Tree::const_iterator const_iterator;
  typedef typename Tree::box_tree_picker_type value_picker_type;
  typedef typename Tree::size_type size_type;

  unstable_box_tree_flat_it ()
    : mp_tree (0)
  { 
    m_index = 0;
  }

  unstable_box_tree_flat_it (const Tree &t)
    : mp_tree (&t)
  { 
    m_index = 0;
  }

  unstable_box_tree_flat_it<Tree> &operator++ () 
  {
    ++m_index;
    return *this;
  }

  const object_type &operator* () const 
  {
    return mp_tree->objects () [m_index];
  }
  
  const object_type *operator-> () const 
  {
    return &mp_tree->objects () [m_index];
  }
  
  bool operator== (const unstable_box_tree_flat_it<Tree> &i) const
  {
    return m_index == i.m_index;
  }

  bool operator!= (const unstable_box_tree_flat_it<Tree> &i) const
  {
    return m_index != i.m_index;
  }

  /**
   *  @brief returns true if at end
   */
  bool at_end () const
  {
    return mp_tree == 0 || m_index == mp_tree->size ();
  }

private:
  size_t m_index;
  const Tree *mp_tree;
};

/**
 *  @brief The iterator class for the "unstable" box tree (see below)
 *
 *  The iterator provides a region-select iterator for the box tree.
 *  It requires a compare operator (type Cmp) that provides coordinate-wise
 *  and box-wise comparison.
 */

template <class Tree, class Cmp>
class unstable_box_tree_it
{
public:
  typedef typename Tree::object_type object_type;
  typedef typename Tree::point_type point_type;
  typedef typename Tree::coord_type coord_type;
  typedef typename Tree::box_type box_type;
  typedef typename Tree::const_iterator const_iterator;
  typedef typename Tree::box_tree_picker_type value_picker_type;
  typedef typename Tree::size_type size_type;
  typedef db::box_tree_node<Tree> box_tree_node;

  unstable_box_tree_it ()
    : mp_tree (0), m_picker (), m_compare () 
  { 
    mp_node = 0;
    m_index = 0;
    m_offset = 0;
    m_quad = -1;
  }

  unstable_box_tree_it (const Tree &t, value_picker_type p, const Cmp &c) 
    : mp_tree (&t), m_picker (p), m_compare (c) 
  { 
    mp_node = t.root ();
    m_index = 0;
    m_offset = 0;
    m_quad = -1;

    while (mp_node && mp_node->lenq (m_quad) == 0) {
      if (! next ()) {
        mp_node = 0;
      } else {
        down ();
      }
    }
    while (! at_end ()) {
      if (check ()) {
        break;
      }
      inc ();
    }
  }

  unstable_box_tree_it<Tree, Cmp> &operator++ () 
  {
    while (true) {
      inc ();
      if (at_end () || check ()) {
        break;
      }
    }
    return *this;
  }

  const object_type &operator* () const 
  {
    return mp_tree->objects () [m_index + m_offset];
  }
  
  const object_type *operator-> () const 
  {
    return &(mp_tree->objects () [m_index + m_offset]);
  }
  
  const_iterator position () const 
  {
    return mp_tree->objects ().begin () + (m_index + m_offset);
  }
  
  /**
   *  @brief Return the index of the position indicated by this iterator
   */
  size_t index () const
  {
    return size_t (std::distance (mp_tree->objects ().begin (), position ()));
  }

  bool operator== (const unstable_box_tree_it<Tree, Cmp> &i) const
  {
    return mp_node == i.mp_node;
  }

  bool operator!= (const unstable_box_tree_it<Tree, Cmp> &i) const
  {
    return mp_node != i.mp_node;
  }

  /**
   *  @brief returns true if at end
   */
  bool at_end () const
  {
    return m_index + m_offset == mp_tree->objects ().size ();
  }

  /**
   *  @brief Returns the quad ID
   *
   *  The quad ID is a value that can be used to determine whether the iterator entered the next quad.
   *  It is possible to optimize the search if there is no need to look into that quad and skip it.
   */
  size_t quad_id () const 
  {
    return mp_node ? size_t (mp_node) + size_t (m_quad + 1): size_t (mp_tree);
  }

  /**
   *  @brief Returns the current's quad box
   *
   *  This method will return the world box if the root node is selected
   */
  box_type quad_box () const
  {
    if (! mp_node) {
      return box_type::world ();
    } else {
      return mp_node->quad_box (m_quad);
    }
  }

  /**
   *  @brief Skips the quad and move to the next one
   */
  void skip_quad ()
  {
    if (! mp_node) {
      m_offset = mp_tree->objects ().size ();
    } else {
      m_offset = 0;
      while (! next ()) {
        if (! up ()) {
          return;
        }
      }
      down ();
    }
  }

private:
  box_tree_node *mp_node;
  size_t m_index;
  size_t m_offset;
  int m_quad;
  const Tree *mp_tree;
  value_picker_type m_picker;
  Cmp m_compare;

  //  return true if the node is within the search range
  bool check () const
  {
    bool ret = m_compare.matches_obj (mp_tree->objects () [m_index + m_offset]);
    return ret;
  }
  
  //  check if the current quad needs visit
  bool need_visit () const
  {
    if (mp_node->lenq (m_quad) == 0) {
      return false;
    }
    if (m_quad < 0) {
      return true;
    } else {
      //  TODO: simplify this, i.e. by providing a special method on the compare function ..
      coord_type m = std::numeric_limits<coord_type>::max ();
      point_type c = mp_node->center ();
      if (m_quad == 0) {
        return m_compare.matches_box (box_type (c, point_type (m, m)));
      } else if (m_quad == 1) {
        return m_compare.matches_box (box_type (-m, c.y (), c.x (), m));
      } else if (m_quad == 2) {
        return m_compare.matches_box (box_type (point_type (-m, -m), c));
      } else {
        return m_compare.matches_box (box_type (c.x (), -m, m, c.y ()));
      }
    }
  }

  //  one level up. 
  bool up () 
  {
    box_tree_node *p = mp_node->parent ();
    if (p) {

      //  move the position to the beginning of the child node
      for (int q = -1; q < m_quad; ++q) {
        m_index -= mp_node->lenq (q);
      }

      m_quad = mp_node->quad ();
      mp_node = p;

      return true;

    } else {
      mp_node = 0;
      return false;
    }
  }

  //  move to next quad
  //  returns true if this is possible
  bool next () 
  {
    m_index += mp_node->lenq (m_quad);
    ++m_quad;
    while (m_quad < 4 && ! need_visit ()) {
      m_index += mp_node->lenq (m_quad);
      ++m_quad;
    }
    return m_quad < 4;
  }

  //  down as many levels as required for the next non-empty quad
  //  returns true if this is possible
  bool down ()
  {
    while (true) {

      box_tree_node *c = mp_node->child (m_quad);
      if (! c) {
        return false;
      }

      mp_node = c;
      m_quad = -1;
      while (m_quad < 4 && ! need_visit ()) {
        m_index += mp_node->lenq (m_quad);
        ++m_quad;
      }

      if (m_quad == 4) {
        //  nothing to visit: up again
        up ();
        return false;
      } else if (m_quad < 0) {
        //  stay in main chunk
        return true;
      }

    }
  }

  //  increment iterator
  void inc ()
  {
    if (! mp_node || m_offset + 1 < mp_node->lenq (m_quad)) {
      ++m_offset;
    } else {
      m_offset = 0;
      while (! next ()) {
        if (! up ()) {
          return;
        }
      }
      down ();
    }
  }
};

/** 
 *  @brief "unstable" box tree object
 *
 *  A box tree is a vector with special sorting and
 *  query capabilities. It contains objects of type
 *  Obj that can be converted to db::box<C> objects with
 *  the BoxConv function. 
 *  A box tree can be in state "inserting", in which the
 *  insertion of new objects is supported, or can be 
 *  sorted, after which it can be queried for objects 
 *  whose box overlaps or touches a specified test box.
 *  This incarnation is "unstable". That means, the original
 *  order of the elements is not maintained when the tree
 *  is sorted.
 */

template <class Box, class Obj, class BoxConv, size_t min_bin = 100, size_t min_quads = 100, unsigned int thin_aspect = 4>
class unstable_box_tree 
{
public:
  typedef Box box_type;
  typedef BoxConv box_conv_type;
  typedef Obj object_type;
  typedef typename box_type::coord_type coord_type;
  typedef db::point<coord_type> point_type;
  typedef tl::vector<object_type> obj_vector_type;
  typedef typename obj_vector_type::iterator obj_iterator;
  typedef size_t size_type;
  typedef typename obj_vector_type::const_iterator const_iterator;
  typedef typename obj_vector_type::iterator iterator;
  typedef unstable_box_tree<box_type, object_type, box_conv_type, min_bin, min_quads> box_tree_type;
  typedef db::box_tree_node<box_tree_type> box_tree_node;
  typedef box_tree_sel<box_type, object_type, box_conv_type, db::boxes_overlap<box_type> > box_tree_sel_overlap_type;
  typedef box_tree_sel<box_type, object_type, box_conv_type, db::boxes_touch<box_type> > box_tree_sel_touch_type;
  typedef unstable_box_tree_flat_it<box_tree_type> flat_iterator;
  typedef unstable_box_tree_it<box_tree_type, box_tree_sel_touch_type> touching_iterator;
  typedef unstable_box_tree_it<box_tree_type, box_tree_sel_overlap_type> overlapping_iterator;
  typedef box_tree_picker<box_type, object_type, box_conv_type, obj_vector_type> box_tree_picker_type;

  /**
   *  @brief Creates a empty box tree object 
   */
  unstable_box_tree ()
    : mp_root (0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Copy constructor
   */
  unstable_box_tree (const unstable_box_tree &b)
    : m_objects (b.m_objects), mp_root (b.mp_root ? b.mp_root->clone () : 0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Move constructor
   */
  unstable_box_tree (unstable_box_tree &&b)
    : m_objects (b.m_objects), mp_root (b.mp_root)
  {
    b.mp_root = 0;
  }

  /**
   *  @brief Assignment 
   */
  unstable_box_tree &operator= (const unstable_box_tree &b)
  {
    clear ();
    m_objects = b.m_objects;
    if (b.mp_root) {
      mp_root = b.mp_root->clone ();
    }
    return *this;
  }

  /**
   *  @brief Assignment (move)
   */
  unstable_box_tree &operator= (unstable_box_tree &&b)
  {
    clear ();
    m_objects = b.m_objects;
    if (b.mp_root) {
      mp_root = b.mp_root;
      b.mp_root = 0;
    }
    return *this;
  }

  /**
   *  @brief The destructor
   */
  ~unstable_box_tree ()
  {
    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;
  }

  /**
   *  @brief Insert a new object into the box tree
   *
   *  Inserts a new object into the box tree. The object
   *  is appended at the end of the vector. Sorting is 
   *  invalidated through this operation.
   *
   *  @param o The object to insert
   */
  iterator insert (const Obj &o)
  {
    m_objects.push_back (o);
    return m_objects.end () - 1;
  }

  /**
   *  @brief reserve space for a given number of elements
   */
  void reserve (size_t n)
  {
    m_objects.reserve (n);
  }

  /**
   *  @brief resize to a given number of elements
   */
  void resize (size_t n)
  {
    m_objects.resize (n);
  }

  /** 
   *  @brief Insert a range of objects
   *
   *  Analogous to the other insert method, but accepting
   *  and range of objects [start,end).
   */
  template <class I> 
  void insert (I from, I to)
  {
    m_objects.insert (m_objects.end (), from, to);
  }

  /**
   *  @brief Replace an object
   *  
   *  Replace the object at the given (const) iterator position
   *  with the object given. Replacing an object invalidates
   *  the sorting state.
   *
   *  @param pos The position of the object to replace
   *  @param obj The object to replace the current one with
   */
  void replace (const_iterator pos, const Obj &obj)
  {
    m_objects [std::distance (((const box_tree_type *) this)->begin (), pos)] = obj;
  }

  /**
   *  @brief Erase an object
   *  
   *  Erase the object pointed to by the given iterator.
   *  Erasing an object will invalidate sorting. Erasing is
   *  not a cheap operation and its complexity is O(n).
   *
   *  @param pos The position of the object to erase
   */
  void erase (iterator pos)
  {
    m_objects.erase (pos);
  }

  /**
   *  @brief Erase multiple objects
   *  
   *  Erase the objects pointed to by the given iterators.
   *  Erasing an object will invalidate sorting. Erasing is
   *  not a cheap operation and its complexity is O(n).
   *
   *  @param pos The positions of the objects to erase. This vector must be sorted, otherwise this method will assert.
   */
  void erase (const std::vector<const_iterator> &pos)
  {
    obj_vector_type objects;
    objects.reserve (m_objects.size () - pos.size ());

    typename std::vector<const_iterator>::const_iterator pp = pos.begin ();
    for (iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
      if (pp == pos.end () || o != *pp) {
        objects.push_back (*o);
      } else {
        ++pp;
      }
    }

    tl_assert (pp == pos.end ()); //  not sorted or not inside the element list.

    m_objects.swap (objects);
  }

  /**
   *  @brief Erase an object sequence
   *  
   *  Erase the objects [from,to) to by the given iterator.
   *  Erasing an object will invalidate sorting. Erasing is
   *  not a cheap operation and its complexity is O(n).
   *
   *  @param pos The position of the object to erase
   */
  void erase (iterator from, iterator to)
  {
    m_objects.erase (from, to);
  }

  /**
   *  @brief Erase several objects
   *  
   *  Erase the objects given by a set of iterators (given by a sequence [from,to)).
   *  The iterators given must be sorted in ascending order.
   *  Erasing an object will invalidate sorting. Erasing is
   *  not a cheap operation and its complexity is O(n).
   *
   *  @param first The iterator delivering the iterators
   *  @param last The end iterator for the list of iterators
   */
  template <class I>
  void erase_positions (I first, I last)
  {
    iterator t = begin ();
    for (iterator i = begin (); i != end (); ++i) {
      if (first == last || i != *first) {
        if (&(*t) != &(*i)) {
          *t = *i;
        }
        ++t;
      } else {
        ++first;
      }
    }

    if (t != end ()) {
      erase (t, end ());
    }
  }

  /**
   *  @brief Get the iterator for an object given by a pointer (non-const version)
   */
  iterator iterator_from_pointer (object_type *p) 
  {
    return m_objects.begin () + (p - &m_objects.front ());
  }
  
  /**
   *  @brief Get the iterator for an object given by a pointer
   */
  const_iterator iterator_from_pointer (const object_type *p) const
  {
    return m_objects.begin () + (p - &m_objects.front ());
  }
  
  /**
   *  @brief Returns the size of the vector.
   *
   *  @return The number of elements in the vector.
   */
  size_type size () const
  {
    return m_objects.size ();
  }

  /**
   *  @brief Test, if the box tree is empty
   */
  bool empty () const
  {
    return m_objects.empty ();
  }
  
  /** 
   *  @brief Empty the vector.
   */
  void clear ()
  {
    m_objects.clear ();
    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;
  }

  /**
   *  @brief Sort the vector
   *
   *  Only after sorting the query iterators are available.
   *  Sorting complexity is approx O(N*log(N)).
   */
  void sort (const BoxConv &conv)
  {
    typename BoxConv::complexity complexity_tag;
    sort (conv, complexity_tag);
  }

  /**
   *  @brief Direct access to the underlying vector
   *
   *  @return A reference to the underlying object vector
   */
  const obj_vector_type &objects () const
  {
    return m_objects;
  }

  /**
   *  @brief Sequential access begin iterator
   *
   *  The normal sequential iterator delivers object by
   *  object in the sequence the objects have been stored
   *  in the vector. Using the sequential access iterator
   *  does not require a sorted vector.
   *
   *  @return A const iterator pointing to the first
   *  element in the vector
   */
  const_iterator begin () const
  {
    return m_objects.begin ();
  }

  /**
   *  @brief Sequential access end iterator
   *
   *  See %begin for a detailed description
   *
   *  @return A const iterator pointing past the end
   *  of the vector
   */
  const_iterator end () const
  {
    return m_objects.end ();
  }
  
  /**
   *  @brief Sequential access begin iterator
   *
   *  The normal sequential iterator delivers object by
   *  object in the sequence the objects have been stored
   *  in the vector. Using the sequential access iterator
   *  does not require a sorted vector.
   *
   *  @return A non-const iterator pointing to the first
   *  element in the vector
   */
  iterator begin () 
  {
    return m_objects.begin ();
  }

  /**
   *  @brief Sequential access end iterator
   *
   *  See %begin for a detailed description
   *
   *  @return A non-const iterator pointing past the end
   *  of the vector
   */
  iterator end () 
  {
    return m_objects.end ();
  }
  
  /**
   *  @brief flat iterator
   *
   *  This iterator is provided for compatibility between stable and unstable 
   *  box trees mainly. It delivers a iterator similar to the primitive one
   *  but with at_end semantics.
   */
  flat_iterator begin_flat () const 
  {
    return unstable_box_tree_flat_it<box_tree_type> (*this);
  }
  
  /** 
   *  @brief selection of objects touching the test box
   *
   *  The const iterator returned by this method will
   *  iterator over all objects touching the test box
   *  with their own box (as converted by BoxConv).
   *  The sequence the objects are delivered is arbitrary.
   *
   *  @param b The test box
   *  @param conv The box converter instance
   *
   *  @return The const touching iterator pointing to the first Object.
   */
  touching_iterator begin_touching (const Box &b, BoxConv conv) const 
  {
    box_tree_picker_type p (conv);
    box_tree_sel_touch_type s (b, conv);
    return unstable_box_tree_it<box_tree_type, box_tree_sel_touch_type> (*this, p, s);
  }
  
  /** 
   *  @brief selection of objects overlapping the test box
   *
   *  The const iterator returned by this method will
   *  iterator over all objects touching the test box
   *  with their own box (as converted by BoxConv).
   *  The sequence the objects are delivered is arbitrary.
   *
   *  @param b The test box
   *  @param conv The box converter instance
   *
   *  @return The const overlapping iterator pointing to the first Object.
   */
  overlapping_iterator begin_overlapping (const Box &b, BoxConv conv) const 
  {
    box_tree_picker_type p (conv);
    box_tree_sel_overlap_type s (b, conv);
    return unstable_box_tree_it<box_tree_type, box_tree_sel_overlap_type> (*this, p, s);
  }
  
  /**
   *  @brief Access to the root node object
   *
   *  This is mainly used by the iterator implementation
   */
  box_tree_node *root () const
  {
    return mp_root;
  }

  /**
   *  @brief Swaps the box tree with another one
   */
  void swap (unstable_box_tree &other)
  {
    m_objects.swap (other.m_objects);
    std::swap (mp_root, other.mp_root);
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_objects, true, (void *) this);
  }

private:
  /// The basic object and element vector
  obj_vector_type m_objects;
  box_tree_node *mp_root;

  /// Sort implementation for simple bboxes - no caching
  void sort (const BoxConv &conv, const db::simple_bbox_tag &/*complexity*/)
  {
    if (m_objects.empty ()) {
      return;
    }

    box_tree_picker_type picker (conv);

    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;

    box_type bbox;
    for (typename obj_vector_type::const_iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
      box_type b = conv (*o);
      if (! b.empty ()) {
        bbox += b;
      }
    }

    tree_sort (0, m_objects.begin (), m_objects.end (), picker, bbox, 0);
  }

  /// Sort implementation for complex bboxes - with caching
  void sort (const box_conv_type &conv, const db::complex_bbox_tag &/*complexity*/)
  {
    if (m_objects.empty ()) {
      return;
    }

    box_tree_cached_picker<object_type, box_type, box_conv_type, obj_vector_type> picker (conv, m_objects.begin (), m_objects.end ());

    if (mp_root) {
      delete mp_root;
    }
    mp_root = 0;

    tree_sort (0, m_objects.begin (), m_objects.end (), picker, picker.bbox (), 0);
  }

  template <class CoordPicker>
  void tree_sort (box_tree_node *parent, obj_iterator from, obj_iterator to, CoordPicker &picker, const box_type &bbox, int quad)
  {
    size_t ntot = size_t (to - from);
    if (ntot <= min_bin || (bbox.width () < 2 && bbox.height () < 2)) {
      return; //  not worth splitting
    } 

    obj_iterator qloc [5] = { from, from, from, from, from };
    point_type center;
    if (bbox.width () < bbox.height () / thin_aspect) {
      //  separate by height only
      center = point_type (bbox.left (), bbox.bottom () + bbox.height () / 2);
    } else if (bbox.height () < bbox.width () / thin_aspect) {
      //  separate by width only
      center = point_type (bbox.left () + bbox.width () / 2, bbox.bottom ());
    } else {
      center = bbox.center ();
    }

    for (obj_iterator e = from; e != to; ++e) {

      box_type b = picker (&*e);

      int q = 0;
      if (! b.empty ()) {
        if (b.right () <= center.x ()) {
          if (b.top () <= center.y ()) {
            q = 3;
          } else if (b.bottom () >= center.y ()) {
            q = 2;
          }
        } else if (b.left () >= center.x ()) {
          if (b.top () <= center.y ()) {
            q = 4;
          } else if (b.bottom () >= center.y ()) {
            q = 1;
          }
        }
      }

      //  make space for the element and swap the new element into position 
      if (q < 4) {

        //  since for the unstable tree there is no guarantee of stability of the 
        //  objects, we have to rotate the box cache of the caching picker as well:
        picker.rotate_boxes (q, e, qloc[0], qloc[1], qloc[2], qloc[3], qloc[4]);

        object_type el = *e;
        for (int i = 4; i > q; --i) {
          *qloc [i] = *qloc [i - 1];
          ++qloc [i];
        }
        *qloc [q] = el;

      }
      ++qloc [q];
      
    }

    //  compute sizes of quad fields
    size_t nx, n[4];
    size_t nn = 0;
    nx = std::distance (from, qloc[0]);
    for (int i = 0; i < 4; ++i) {
      n[i] = std::distance (qloc [i], qloc[i + 1]);
      nn += n[i];
    }

    //  is it worth to split into sub-quads?
    if (nn >= min_quads) {

      //  create a new node representing this tree
      box_tree_node *node = new box_tree_node (parent, center, bbox, quad);
      if (parent == 0) {
        mp_root = node;
      }

      //  tell the parent the length of the "overall" bin
      node->lenq (-1, nx);

      //  yes: create sub-quads
      box_type qboxes [4];
      qboxes [0] = box_type (center, bbox.p2 ()); 
      qboxes [1] = box_type (bbox.left (), center.y (), center.x (), bbox.top ());
      qboxes [2] = box_type (bbox.p1 (), center);
      qboxes [3] = box_type (center.x (), bbox.bottom (), bbox.right (), center.y ());
      for (unsigned int q = 0; q < 4; ++q) {
        if (n[q] > 0) {
          node->lenq (q, n[q]);
          tree_sort (node, qloc[q], qloc[q + 1], picker, qboxes [q], int (q));
        }
      }

    } 

  }

};

/**
 *  @brief Collect memory statistics
 */
template <class Box, class Obj, class BoxConv>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const db::unstable_box_tree<Box, Obj, BoxConv> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif

