
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_dbQuadTree
#define HDR_dbQuadTree

#include "dbBox.h"
#include "tlLog.h"
#include <vector>

namespace db
{

/**
 *  @brief The quad tree node implementation class
 */
template <class T, class BC, size_t thr, class CMP>
class quad_tree_node
{
public:
  typedef typename BC::box_type box_type;
  typedef typename box_type::point_type point_type;
  typedef typename box_type::vector_type vector_type;
  typedef std::vector<T> objects_vector;
  typedef db::coord_traits<typename box_type::coord_type> coord_traits;

  quad_tree_node (const point_type &center)
    : m_center (center)
  {
    init (true);
  }

  ~quad_tree_node ()
  {
    clear ();
  }

  quad_tree_node (const quad_tree_node &other)
    : m_center (center)
  {
    init (true);
    operator= (other);
  }

  quad_tree_node &operator= (const quad_tree_node &other)
  {
    if (this != &other) {
      clear ();
      m_center = other.m_center;
      m_objects = other.m_objects;
      if (! other.is_leaf ()) {
        init (false);
        for (unsigned int i = 0; i < 4; ++i) {
          if (other.m_q[i]) {
            m_q[i] = other.m_q[i]->clone ();
          }
        }
      }
    }
    return *this;
  }

  quad_tree_node (quad_tree_node &&other)
  {
    init (true);
    swap (other);
  }

  quad_tree_node &operator= (quad_tree_node &&other)
  {
    swap (other);
    return *this;
  }

  void swap (quad_tree_node &other)
  {
    if (this != &other) {
      std::swap (m_center, other.m_center);
      m_objects.swap (other.m_objects);
      for (unsigned int i = 0; i < 4; ++i) {
        std::swap (m_q[i], other.m_q[i]);
      }
    }
  }

  quad_tree_node *clone () const
  {
    quad_tree_node *node = new quad_tree_node (m_center);
    *node = *this;
    return node;
  }

  void clear ()
  {
    m_objects.clear ();
    if (! is_leaf ()) {
      for (unsigned int i = 0; i < 4; ++i) {
        if (m_q[i]) {
          delete m_q[i];
        }
        m_q[i] = 0;
      }
      init (true);
    }
  }

  const point_type &center () const
  {
    return m_center;
  }

  void insert_top (const T &value, const box_type &total_box, const box_type &b)
  {
    insert (value, propose_ucenter (total_box), b);
  }

  bool erase (const T &value, const box_type &b)
  {
    int n = quad_for (b);

    if (is_leaf () || n < 0) {

      for (auto i = m_objects.begin (); i != m_objects.end (); ++i) {
        if (CMP () (*i, value)) {
          m_objects.erase (i);
          return true;
        }
      }

    } else if (m_q[n]) {

      if (m_q[n]->erase (value, b)) {
        if (m_q[n]->empty ()) {
          delete m_q[n];
          m_q[n] = 0;
        }
        return true;
      }

    }

    return false;
  }

  const objects_vector &objects () const
  {
    return m_objects;
  }

  box_type q_box (unsigned int n) const
  {
    if (! is_leaf () && m_q[n]) {
      return m_q[n]->box (m_center);
    } else {
      return box_type ();
    }
  }

  quad_tree_node *node (unsigned int n) const
  {
    tl_assert (! is_leaf ());
    return m_q [n];
  }

  bool empty () const
  {
    if (m_objects.empty ()) {
      if (! is_leaf ()) {
        for (unsigned int n = 0; n < 4; ++n) {
          if (m_q[n] && ! m_q[n]->empty ()) {
            return false;
          }
        }
      }
      return true;
    } else {
      return false;
    }
  }

  size_t size () const
  {
    size_t count = m_objects.size ();
    if (! is_leaf ()) {
      for (unsigned int n = 0; n < 4; ++n) {
        if (m_q[n]) {
          count += m_q[n]->size ();
        }
      }
    }
    return count;
  }

  size_t levels () const
  {
    size_t l = 1;
    if (! is_leaf ()) {
      for (unsigned int n = 0; n < 4; ++n) {
        if (m_q[n]) {
          l = std::max (l, m_q[n]->levels () + 1);
        }
      }
    }
    return l;
  }

  bool check_top (const box_type &total_box) const
  {
    return check (propose_ucenter (total_box));
  }

private:
  point_type m_center;
  quad_tree_node *m_q [4];
  objects_vector m_objects;

  void init (bool is_leaf)
  {
    for (unsigned int i = 0; i < 4; ++i) {
      m_q[i] = 0;
    }
    if (is_leaf) {
      m_q[0] = reinterpret_cast<quad_tree_node *> (1);
    }
  }

  bool is_leaf () const
  {
    return m_q[0] == reinterpret_cast<quad_tree_node *> (1);
  }

  int quad_for (const box_type &box) const
  {
    int sx = coord_traits::less (box.right (), m_center.x ()) ? 0 : (coord_traits::less (m_center.x (), box.left ()) ? 1 : -1);
    int sy = coord_traits::less (box.top (), m_center.y ()) ? 0 : (coord_traits::less (m_center.y (), box.bottom ()) ? 2 : -1);
    if (sx < 0 || sy < 0) {
      return -1;
    } else {
      return sx + sy;
    }
  }

  box_type box (const point_type &ucenter) const
  {
    return box_type (ucenter, ucenter - (ucenter - m_center) * 2.0);
  }

  box_type q (unsigned int n, const point_type &ucenter) const
  {
    //  NOTE: with this definition the opposite quad index is 3 - n

    vector_type vx (std::abs (ucenter.x () - m_center.x ()), 0);
    vector_type vy (0, std::abs (ucenter.y () - m_center.y ()));
    switch (n) {
    case 0:
      return box_type (m_center - vx - vy, m_center);
    case 1:
      return box_type (m_center - vy, m_center + vx);
    case 2:
      return box_type (m_center - vx, m_center + vy);
    default:
      return box_type (m_center, m_center + vx + vy);
    }
  }

  void split (const point_type &ucenter)
  {
    init (false);

    objects_vector ov;
    ov.swap (m_objects);

    for (auto o = ov.begin (); o != ov.end (); ++o) {
      insert (*o, ucenter, BC () (*o));
    }
  }

  void insert (const T &value, const point_type &ucenter, const box_type &b)
  {
    if (is_leaf () && m_objects.size () + 1 < thr) {

      m_objects.push_back (value);

    } else {

      if (is_leaf ()) {
        split (ucenter);
      }

      if (inside (b, box (ucenter))) {

        int n = quad_for (b);
        if (n < 0) {
          m_objects.push_back (value);
        } else {
          if (! m_q[n]) {
            box_type bq = q (n, ucenter);
            m_q[n] = new quad_tree_node (bq.center ());
          }
          m_q[n]->insert (value, m_center, b);
        }

      } else {

        tl_assert (m_q[0] || m_q[1] || m_q[2] || m_q[3]);
        point_type new_ucenter = m_center - (m_center - ucenter) * 2.0;
        grow (new_ucenter);
        insert (value, new_ucenter, b);

      }

    }
  }

  void grow (const point_type &ucenter)
  {
    for (unsigned int i = 0; i < 4; ++i) {
      if (m_q[i]) {
        quad_tree_node *n = m_q[i];
        m_q[i] = new quad_tree_node (q (i, ucenter).center ());
        m_q[i]->init (false);
        m_q[i]->m_q[3 - i] = n;
      }
    }
  }

  point_type propose_ucenter (const box_type &total_box) const
  {
    if (! is_leaf ()) {
      for (unsigned int i = 0; i < 4; ++i) {
        if (m_q[i]) {
          return m_center - (m_center - m_q[i]->center ()) * 2.0;
        }
      }
    }

    typename box_type::coord_type dx = std::max (std::abs (total_box.left () - m_center.x ()), std::abs (total_box.right () - m_center.y ()));
    typename box_type::coord_type dy = std::max (std::abs (total_box.bottom () - m_center.y ()), std::abs (total_box.top () - m_center.y ()));
    return m_center - vector_type (dx * 2, dy * 2);
  }

  bool check (const point_type &ucenter) const
  {
    bool result = true;

    box_type bq = box (ucenter);

    for (auto i = m_objects.begin (); i != m_objects.end (); ++i) {
      box_type b = BC () (*i);
      if (! inside (b, bq)) {
        tl::error << "Box " << b.to_string () << " not inside quad box " << bq.to_string ();
        result = false;
      }
      point_type ucenter_opp = m_center + (m_center - ucenter);
      if (coord_traits::equal (b.left (), ucenter.x ()) || coord_traits::equal (b.right (), ucenter.x ()) ||
          coord_traits::equal (b.bottom (), ucenter.y ()) || coord_traits::equal (b.top (), ucenter.y ()) ||
          coord_traits::equal (b.left (), ucenter_opp.x ()) || coord_traits::equal (b.right (), ucenter_opp.x ()) ||
          coord_traits::equal (b.bottom (), ucenter_opp.y ()) || coord_traits::equal (b.top (), ucenter_opp.y ())) {
        tl::error << "Box " << b.to_string () << " touches quad boundary " << ucenter.to_string () << " .. " << ucenter_opp.to_string ();
        result = false;
      }
    }

    if (! is_leaf ()) {

      for (auto i = m_objects.begin (); i != m_objects.end (); ++i) {
        box_type b = BC () (*i);
        int n = quad_for (b);
        if (n >= 0) {
          tl::error << "Box " << b.to_string () << " on quad level not overlapping multiple quads";
          result = false;
        }
      }

      for (unsigned int n = 0; n < 4; ++n) {
        if (m_q[n]) {
          if (! m_q[n]->check (m_center)) {
            result = false;
          }
          box_type bbq = m_q[n]->box (m_center);
          if (! bbq.equal (q (n, ucenter))) {
            tl::error << "Quad not centered (quad box is " << bbq.to_string () << ", should be " << q (n, ucenter).to_string ();
            result = false;
          }
        }
      }

    } else {

      if (m_objects.size () > thr) {
        tl::error << "Non-split object count exceeds threshold " << m_objects.size () << " > " << thr;
        result = false;
      }

    }

    return result;
  }

  static bool inside (const box_type &box, const box_type &in)
  {
    if (box.empty () || in.empty ()) {
      return false;
    } else {
      return coord_traits::less (in.left (), box.left ()) && coord_traits::less (box.right (), in.right ()) &&
             coord_traits::less (in.bottom (), box.bottom ()) && coord_traits::less (box.top (), in.top ());
    }
  }
};

/**
 *  @brief The iterator implementation class
 */
template <class T, class BC, size_t thr, class CMP, class S>
class quad_tree_iterator
{
public:
  typedef quad_tree_node<T, BC, thr, CMP> quad_tree_node_type;
  typedef typename BC::box_type box_type;

  quad_tree_iterator ()
    : m_s (), m_i (0)
  {
    //  .. nothing yet ..
  }

  quad_tree_iterator (const quad_tree_node_type *root, const S &s)
    : m_s (s), m_i (0)
  {
    m_stack.push_back (std::make_pair (root, -1));
    validate ();
  }

  bool at_end () const
  {
    return m_stack.empty ();
  }

  quad_tree_iterator &operator++ ()
  {
    ++m_i;
    validate ();
    return *this;
  }

  const T &operator* () const
  {
    return m_stack.back ().first->objects () [m_i];
  }

  const T *operator-> () const
  {
    return (m_stack.back ().first->objects ().begin () + m_i).operator-> ();
  }

public:
  S m_s;
  std::vector<std::pair<const quad_tree_node_type *, int> > m_stack;
  size_t m_i;

  void validate ()
  {
    auto s = m_stack.end ();
    tl_assert (s != m_stack.begin ());
    --s;

    const quad_tree_node_type *qn = s->first;
    while (m_i < s->first->objects ().size () && ! m_s.select (s->first->objects () [m_i])) {
      ++m_i;
    }

    if (m_i < qn->objects ().size ()) {
      return;
    }

    m_i = 0;

    for (unsigned int n = 0; n < 4; ++n) {
      box_type bq = qn->q_box (n);
      if (! bq.empty () && m_s.select_quad (bq)) {
        m_stack.back ().second = n;
        m_stack.push_back (std::make_pair (qn->node (n), -1));
        validate ();
        return;
      }
    }

    m_stack.pop_back ();

    while (! m_stack.empty ()) {

      qn = m_stack.back ().first;

      int &n = m_stack.back ().second;
      while (++n < 4) {
        box_type bq = qn->q_box (n);
        if (! bq.empty () && m_s.select_quad (bq)) {
          m_stack.push_back (std::make_pair (qn->node (n), -1));
          validate ();
          return;
        }
      }

      m_stack.pop_back ();

    }
  }
};

/**
 *  @brief The selector for implementing the all-iterator
 */
template <class T, class BC>
class quad_tree_always_sel
{
public:
  typedef typename BC::box_type box_type;

  bool select (const T &) const
  {
    return true;
  }

  bool select_quad (const box_type &) const
  {
    return true;
  }
};

/**
 *  @brief The selector for implementing the touching iterator
 */
template <class T, class BC>
class quad_tree_touching_sel
{
public:
  typedef typename BC::box_type box_type;

  quad_tree_touching_sel ()
  {
    //  .. nothing yet ..
  }

  quad_tree_touching_sel (const box_type &box)
    : m_box (box)
  {
    //  .. nothing yet ..
  }

  bool select (const T &value) const
  {
    return select_quad (BC () (value));
  }

  bool select_quad (const box_type &box) const
  {
    return m_box.touches (box);
  }

private:
  box_type m_box;
};

/**
 *  @brief The selector for implementing the overlapping iterator
 */
template <class T, class BC>
class quad_tree_overlapping_sel
{
public:
  typedef typename BC::box_type box_type;

  quad_tree_overlapping_sel ()
  {
    //  .. nothing yet ..
  }

  quad_tree_overlapping_sel (const box_type &box)
    : m_box (box)
  {
    //  .. nothing yet ..
  }

  bool select (const T &value) const
  {
    return select_quad (BC () (value));
  }

  bool select_quad (const box_type &box) const
  {
    return m_box.overlaps (box);
  }

private:
  box_type m_box;
};

/**
 *  @brief The default compare function
 */
template <class T>
struct quad_tree_default_cmp
{
  bool operator() (const T &a, const T &b) const
  {
    return a == b;
  }
};

/**
 *  @brief A generic quad tree implementation
 *
 *  In contrast to the box_tree implementation, this is a self-sorting implementation
 *  which is more generic.
 *
 *  @param T The value to be stored
 *  @param BC The box converter
 *  @param thr The number of items per leaf node before splitting
 *  @param CMP The compare function (equality)
 *
 *  T needs to have a type member named "coord_type".
 *  BC is a function of T and delivers a db::box<coord_type> for T
 *  CMP is a function that delivers a bool value for a pair of T (equality)
 */
template <class T, class BC, size_t thr = 10, class CMP = quad_tree_default_cmp<T>>
class quad_tree
{
public:
  typedef quad_tree_node<T, BC, thr, CMP> quad_tree_node_type;
  typedef quad_tree_iterator<T, BC, thr, CMP, quad_tree_always_sel<T, BC> > quad_tree_flat_iterator;
  typedef quad_tree_iterator<T, BC, thr, CMP, quad_tree_touching_sel<T, BC> > quad_tree_touching_iterator;
  typedef quad_tree_iterator<T, BC, thr, CMP, quad_tree_overlapping_sel<T, BC> > quad_tree_overlapping_iterator;
  typedef typename BC::box_type box_type;
  typedef typename box_type::point_type point_type;
  typedef typename box_type::vector_type vector_type;
  typedef std::vector<T> objects_vector;

  /**
   *  @brief Default constructor
   *
   *  This creates an empty tree.
   */
  quad_tree ()
    : m_root (point_type ())
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Copy constructor
   */
  quad_tree (const quad_tree &other)
    : m_root (point_type ())
  {
    operator= (other);
  }

  /**
   *  @brief Assignment
   */
  quad_tree &operator= (const quad_tree &other)
  {
    if (this != &other) {
      m_root = other.m_root;
      m_total_box = other.m_total_box;
    }
    return *this;
  }

  /**
   *  @brief Move constructor
   */
  quad_tree (quad_tree &&other)
    : m_root (point_type ())
  {
    swap (other);
  }

  /**
   *  @brief Move assignment
   */
  quad_tree &operator= (quad_tree &&other)
  {
    swap (other);
    return *this;
  }

  /**
   *  @brief Empties the tree
   */
  void clear ()
  {
    m_root.clear ();
    m_total_box = box_type ();
  }

  /**
   *  @brief Swaps the tree with another
   */
  void swap (quad_tree &other)
  {
    if (this != &other) {
      m_root.swap (other.m_root);
      std::swap (m_total_box, other.m_total_box);
    }
  }

  /**
   *  @brief Returns a value indicating whether the tree is empty
   */
  bool empty () const
  {
    return m_root.empty ();
  }

  /**
   *  @brief Returns the number of items stored in the tree
   */
  size_t size () const
  {
    return m_root.size ();
  }

  /**
   *  @brief Returns the number of quad levels (for testing)
   */
  size_t levels () const
  {
    return m_root.levels ();
  }

  /**
   *  @brief Checks the tree for consistency (for testing)
   */
  bool check () const
  {
    return m_root.check_top (m_total_box);
  }

  /**
   *  @brief Inserts an object into the tree
   */
  void insert (const T &value)
  {
    box_type b = BC () (value);
    if (b.empty ()) {
      return;
    }

    m_total_box += b;
    m_root.insert_top (value, m_total_box, b);
  }

  /**
   *  @brief Erases the given element from the tree
   *
   *  @return true, if the element was found and erased.
   *
   *  If multiple elements of the same kind are stored, the
   *  first one is erased.
   */
  bool erase (const T &value)
  {
    return m_root.erase (value, BC () (value));
  }

  /**
   *  @brief begin iterator for all elements
   */
  quad_tree_flat_iterator begin () const
  {
    return quad_tree_flat_iterator (&m_root, quad_tree_always_sel<T, BC> ());
  }

  /**
   *  @brief begin iterator for all elements overlapping the given box
   */
  quad_tree_overlapping_iterator begin_overlapping (const box_type &box) const
  {
    if (m_total_box.overlaps (box)) {
      return quad_tree_overlapping_iterator (&m_root, quad_tree_overlapping_sel<T, BC> (box));
    } else {
      return quad_tree_overlapping_iterator ();
    }
  }

  /**
   *  @brief begin iterator for all elements touching the given box
   */
  quad_tree_touching_iterator begin_touching (const box_type &box) const
  {
    if (m_total_box.touches (box)) {
      return quad_tree_touching_iterator (&m_root, quad_tree_touching_sel<T, BC> (box));
    } else {
      return quad_tree_touching_iterator ();
    }
  }

private:
  box_type m_total_box;
  quad_tree_node_type m_root;
};

}

#endif
