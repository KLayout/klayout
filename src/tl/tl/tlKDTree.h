
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


#ifndef HDR_tlKDTree
#define HDR_tlKDTree

#include <vector>
#include <algorithm>
#include <functional>

#include "tlAlgorithm.h"

namespace tl
{

template <class Tree, class Cmp>
class kd_tree_it
{
public:
  typedef typename Tree::value_type value_type;
  typedef typename Tree::object_type object_type;
  typedef typename Tree::value_picker_type value_picker_type;
  typedef typename Tree::difference_type difference_type;
  typedef typename Tree::size_type size_type;

  kd_tree_it (const Tree &t, value_picker_type p, const Cmp &c) 
    : m_j (1), m_n (t.size ()), m_l (0), 
      m_tree (t), m_picker (p), m_compare (c) 
  { 
    if (m_n > 0 && need_visit ()) {
      traverse ();
      while (! at_end ()) {
        if (check ()) {
          break;
        }
        inc ();
      }
    } else {
      finish ();
    }
  }

  kd_tree_it<Tree, Cmp> &operator++ () 
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
    return m_tree.objects () [m_j - 1];
  }
  
  operator difference_type () const
  {
    return index ();
  }

  /**
   *  @brief Return the index of the position indicated by this iterator
   */
  difference_type index () const
  {
    return m_j - 1;
  }

  bool operator== (const kd_tree_it<Tree, Cmp> &i) 
  {
    return m_j == i.m_j;
  }

  bool operator!= (const kd_tree_it<Tree, Cmp> &i) 
  {
    return m_j != i.m_j;
  }

  bool operator== (size_type j)
  {
    return m_j == j;
  }

  bool operator!= (size_type j)
  {
    return m_j != j;
  }

  /**
   *  @brief returns true if at end
   */
  bool at_end () const
  {
    return m_j == 0;
  }

private:
  size_type m_j;
  size_type m_n;
  int m_l; 
  const Tree &m_tree;
  value_picker_type m_picker;
  Cmp m_compare;

  //  return true if the node is within the search range
  bool check () const
  {
    return m_compare (m_tree.objects () [m_j - 1]);
  }
  
  //  one level up. Returns true if from right
  bool up () 
  {
    size_type m = size_type (1) << m_l;
    size_type mm = m / 2;
    --m_l;
    bool right = ((m_j & mm) != 0);
    m_j = (m_j & ~m) | (m >> 1);
    return right;
  }

  //  one level down (right). Returns false if not possible
  bool down_right ()
  {
    size_type m = 2 << m_l;
    m_j += m;
    if (m_j > m_n) { 
      m_j -= m;
      return false;
    } else {
      ++m_l;
      return true;
    }  
  }

  //  one level down (left). Returns false if not possible
  bool down_left ()
  {
    size_type m = size_type (1) << m_l;
    m_j += m;
    if (m_j > m_n) { 
      m_j -= m;
      return false;
    } else {
      ++m_l;
      return true;
    }  
  }

  //  place at end
  void finish () 
  {
    m_j = 0;
    m_l = -1;
  }
    
  //  return true if the current node needs a visit
  bool need_visit () const
  {
    return m_compare ((unsigned int) m_l, m_tree.bounds () [m_j - 1]);
  }

  //  traverse as far as right down possible
  void traverse ()
  {
    bool next;

    do {
      next = false;
      if (down_right ()) {
        if (need_visit ()) {
          next = true;
        } else {
          up ();
        }
      } 
      if (! next && down_left ()) {
        if (need_visit ()) {
          next = true;
        } else {
          up ();
        }
      }
    } while (next);  
  }

  //  increment iterator
  void inc ()
  {
    if (! at_end ()) {
      if (up ()) { 
        if (! down_left ()) {
          // no left node. just stay here.
        } else if (! need_visit ()) {
          up ();
        } else {
          traverse ();
        }
      }
    }
  }
};

template <class Obj, class Val, class ValPicker, class Cmp>
class kd_comp_f
{
public:
  kd_comp_f (unsigned int l, ValPicker picker, Cmp comp) 
    : m_level (l), m_picker (picker), m_comp (comp)
  { }

  bool operator() (const Obj &o1, const Obj &o2) const
  {
    return m_comp (m_level, m_picker (m_level, o1), m_picker (m_level, o2));
  }

private:
  unsigned int m_level;
  ValPicker m_picker;
  Cmp m_comp;
};

template <class Cont>
class kd_n_it
{
public:
  typedef typename Cont::iterator it_type;
  typedef typename Cont::difference_type difference_type;
  typedef typename Cont::size_type size_type;
  typedef typename std::iterator_traits<it_type> it_traits;
  typedef typename it_traits::value_type value_type;
  typedef typename it_traits::iterator_category iterator_category;
  typedef typename it_traits::pointer pointer;
  typedef typename it_traits::reference reference;

  kd_n_it (size_type step, const it_type &begin, difference_type index)
    : m_begin (begin), m_step (step), m_index (index)
  { }

  kd_n_it (size_type step, const kd_n_it &i)
    : m_begin (i.m_begin), m_step (step), m_index (i.m_index)
  { }

  size_type step () const 
  { return m_step; }

  kd_n_it &operator+= (difference_type n) 
  {
    m_index += (n * m_step);
    return *this;
  }

  kd_n_it &operator-= (difference_type n) 
  {
    m_index -= (n * m_step);
    return *this;
  }

  kd_n_it operator+ (difference_type n) const
  {
    return kd_n_it (m_step, m_begin, m_index + n * m_step);
  }

  kd_n_it operator- (difference_type n) const
  {
    return kd_n_it (m_step, m_begin, m_index - n * m_step);
  }

  value_type &operator* () const
  {
    return m_begin [m_index];
  }

  kd_n_it &operator++ () 
  {
    m_index += m_step;
    return *this;
  }

  kd_n_it &operator-- () 
  {
    m_index -= m_step;
    return *this;
  }

  value_type &operator[] (difference_type n) const 
  {
    return m_begin [m_index + n * m_step];
  }

  bool operator== (const kd_n_it &i) const
  {
    return m_index == i.m_index;
  }

  bool operator!= (const kd_n_it &i) const
  {
    return !operator== (i);
  }

  bool operator< (const kd_n_it &i) const
  {
    return m_index < i.m_index;
  }

  bool operator<= (const kd_n_it &i) const
  {
    return m_index <= i.m_index;
  }

  difference_type operator- (const kd_n_it &i) const
  {
    return (m_index - i.m_index) / m_step;
  }

private:
  it_type m_begin;
  size_type m_step;
  difference_type m_index;
};

/**
 *  @brief A generic KD tree objects
 * 
 *  A KD tree is a tree in which each level describes
 *  a different dimension of values with repeating 
 *  permutations of dimensions. This way a n-dimensional
 *  space can be mapped and efficiently searched.
 *  This implementation is based on a special sorting
 *  scheme of a simple vector of elements with a single
 *  "boundary value" vector accompanying it.
 *  It takes four template arguments. 
 *  "Obj" is the type of the object, "Val" is the type
 *  of the value (a single dimension), "ValPicker" is 
 *  a converter class that gets a certain value of a 
 *  given dimension from the object, "Cmp" is the compare
 *  function used to compare values in a special flavour
 *  that allows making the comparison dependent on the
 *  dimension. "ObjV" is the container used for storing the
 *  objects, "ValV" is the container used for storing values.
 */
 
template <class Obj, class Val, class ValPicker, class Cmp, class ObjV = std::vector<Obj>, class ValV = std::vector<Val> >
class kd_tree 
{
public:
  typedef tl::kd_tree<Obj, Val, ValPicker, Cmp, ObjV, ValV> tree_type;
  typedef Val value_type;
  typedef Obj object_type;
  typedef ValPicker value_picker_type;
  typedef Cmp compare_type;
  typedef ObjV obj_vector_type;
  typedef ValV bound_vector_type;
  typedef tl::kd_n_it<obj_vector_type> kd_n_it;
  typedef typename obj_vector_type::const_iterator const_iterator;
  typedef typename obj_vector_type::iterator iterator;
  typedef typename obj_vector_type::difference_type difference_type;
  typedef typename obj_vector_type::size_type size_type;

  /**
   *  @brief Constructs an empty KD tree object
   */
  kd_tree () { }

  /**
   *  @brief Returns the number of elements in the KD tree
   */
  size_type size () const
  {
    return m_objs.size ();
  }

  /**
   *  @brief Reserves space for a given number of elements
   */
  void reserve (size_type n) 
  {
    m_objs.reserve (n);
  }

  /**
   *  @brief Resize the tree to a given number of elements inserting default elements
   */
  void resize (size_type n) 
  {
    m_objs.resize (n);
  }

  /**
   *  @brief Clear the KD tree
   */
  void clear ()
  {
    m_objs.clear ();
    m_bounds.clear ();
  }

  /**
   *  @brief Insert a given object into the KD tree
   *
   *  Inserts the given object. Inserting will destroy the
   *  "sorted" state of the tree. It will require resorting
   *  once an element is inserted.
   */
  Obj &insert (const Obj &obj)
  {
    m_objs.push_back (obj);
    return m_objs.back ();
  }

  /**
   *  @brief Erase a given object from the KD tree at the given position
   *    
   *  Erasing objects will required resorting.
   */
  void erase (iterator pos)
  {
    m_objs.erase (pos);
  }

  /**
   *  @brief Erase a sequence of objects from the KD tree at the given position
   *    
   *  Erasing objects will required resorting.
   */
  void erase (iterator from, iterator to)
  {
    m_objs.erase (from, to);
  }

  /**
   *  @brief Insert a range of objects into the KD tree
   *
   *  Inserts the objects [from,to) into the KD tree.
   *  Inserting will destroy the "sorted" state of the tree. 
   *  It will require resorting once an element is inserted.
   */
  template <class I>
  void insert (I from, I to)
  {
    m_objs.insert (m_objs.end (), from, to);
  }

  /**
   *  @brief Restore the tree's sorted state.
   *  
   *  "sorting" the tree will make the tree accessible for 
   *  search accesses. Inserting destroys the sorted state.
   *  Complexity of the sorting is O(n*log(n)).
   */
  void sort (ValPicker picker, Cmp comp)
  {
    m_bounds.clear ();
    m_bounds.resize (m_objs.size ());

    partial_sort (0, 0, kd_n_it (1, m_objs.begin (), 0), kd_n_it (1, m_objs.begin (), m_objs.size ()), picker, comp);
  }

  /**
   *  @brief Direct access to the objects vector
   */
  const obj_vector_type &objects () const
  {
    return m_objs;
  }

  /**
   *  @brief Sets the bounds vector
   *
   *  This method installs a new bounds vector. This method is not
   *  intended to be used publicly. It's main use is the special implementation
   *  of the caching BoxTree.
   */
  void bounds (const bound_vector_type &bounds)
  {
    m_bounds = bounds;
  }

  /**
   *  @brief Direct access to the bounds vector
   */
  const bound_vector_type &bounds () const
  {
    return m_bounds;
  }

  /** 
   *  @brief Direct iterator access to the element vector without search 
   */
  const_iterator begin () const
  {
    return m_objs.begin ();
  }

  /** 
   *  @brief Direct iterator access to the element vector without search 
   */
  const_iterator end () const
  {
    return m_objs.end ();
  }

  /** 
   *  @brief Direct iterator access to the element vector without search 
   */
  iterator begin () 
  {
    return m_objs.begin ();
  }

  /** 
   *  @brief Direct iterator access to the element vector without search 
   */
  iterator end () 
  {
    return m_objs.end ();
  }

  /**
   *  @brief search initiation
   *
   *  The model used in the search follows the usual begin..end 
   *  iterator semantics. However, begin and end are asymmetric:
   *  While "sel_begin" creates, initializes and delivers a iterator
   *  which walks through the tree skipping irrelevant items, the
   *  "sel_end" methods just delivers a "token" which is matched by
   *  the iterators comparison operator against the "done" state.
   *  
   *  @param picker The value picker object that is used to fetch
   *                a value of a given dimension of an object.
   *  @param cmp The comparison function used in the search.
   */
  template <class SelCmp>
  kd_tree_it<tree_type, SelCmp> sel_begin (value_picker_type picker, const SelCmp &cmp) const
  {
    return kd_tree_it<tree_type, SelCmp> (*this, picker, cmp);
  }

  size_type sel_end () const 
  {
    return 0;
  }

private:
  obj_vector_type m_objs;
  bound_vector_type m_bounds;

  void partial_sort (typename bound_vector_type::difference_type bi, 
                     unsigned int l, 
                     const kd_n_it &from, const kd_n_it &to, 
                     ValPicker picker, Cmp comp)
  {
    typename kd_n_it::difference_type n = to - from;
    if (n > 2) {

      typename kd_n_it::difference_type n1 = n / 2;
      kd_n_it mid = from + n1;

      kd_comp_f<Obj, Val, ValPicker, Cmp> kdcomp (l, picker, comp);

      tl::nth_element (from, mid, to, kdcomp);

      //  determine upper limit
      Val bound (picker (l, *mid));
      for (kd_n_it i = mid + 1; i != to; ++i) {
        Val v = picker (l, *i);
        if (comp (l, bound, v)) {
          bound = v;
        }
      }
      m_bounds [bi] = bound;
      
      bool even = ((n1 % 2) == 0);

      kd_n_it i1 (from);
      kd_n_it i2 (mid);
      for (typename kd_n_it::difference_type nn = 0; nn < n1; nn += 2) {
        std::swap (*i1, *i2);
        if (even) {
          std::swap (i2 [0], i2 [1]);
        }
        i1 += 2;
        i2 += 2;
      }

      kd_n_it s2 (from.step () * 2, from + 1);
      partial_sort (bi + from.step (), l + 1, s2, s2 + n1, picker, comp);
      
      kd_n_it s1 (from.step () * 2, from + 2);
      partial_sort (bi + from.step () * 2, l + 1, s1, s1 + (n - n1 - 1), picker, comp);

    } else if (n == 2) {

      //  some simple optimization for the trivial case 2
      Val v1 = picker (l, from [0]);
      Val v2 = picker (l, from [1]);
      if (comp (l, v2, v1)) {
        m_bounds [bi] = v1;
      } else {
        m_bounds [bi] = v2;
        std::swap (from [0], from [1]);
      }
      m_bounds [bi + from.step ()] = picker (l + 1, from [1]);

    } else if (n == 1) {

      //  some simple optimization for the trivial case 1
      m_bounds [bi] = picker (l, *from);

    }
  }
};

} // namespace tl

#endif

