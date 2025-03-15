
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
#include <vector>

namespace db
{

template <class T, class BC, size_t thr>
class quad_tree_node
{
public:
  typedef typename T::coord_type coord_type;
  typedef db::box<coord_type> box_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef std::vector<T> objects_vector;

  quad_tree_node (const point_type &center)
    : m_split (false), m_center (center)
  {
    for (unsigned int i = 0; i < 4; ++i) {
      m_q [i] = 0;
    }
  }

  ~quad_tree_node ()
  {
    for (unsigned int i = 0; i < 4; ++i) {
      delete m_q [i];
      m_q [i] = 0;
    }
  }

  const point_type &center () const
  {
    return m_center;
  }

  void insert_top (const T &value, const box_type &total_box)
  {
    insert (value, propose_ucenter (total_box));
  }

  bool remove (const T &value)
  {
    box_type b = BC () (value);

    if (! m_split || b.contains (m_center)) {
      for (auto i = m_objects.begin (); i != m_objects.end (); ++i) {
        if (*i == value) {
          m_objects.erase (i);
          return true;
        }
      }
    }

    for (unsigned int i = 0; i < 4; ++i) {
      if (m_q[i] && b.inside (m_q[i]->box (m_center))) {
        return b.remove (value);
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
    if (m_q[n]) {
      return m_q[n]->box (m_center);
    } else {
      return box_type ();
    }
  }

  quad_tree_node *node (unsigned int n) const
  {
    return m_q [n];
  }

  bool empty () const
  {
    if (m_objects.empty ()) {
      for (unsigned int n = 0; n < 4; ++n) {
        if (m_q[n] && ! m_q[n]->empty ()) {
          return false;
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
    for (unsigned int n = 0; n < 4; ++n) {
      if (m_q[n]) {
        count += m_q[n]->size ();
      }
    }
    return count;
  }

private:
  bool m_split;
  point_type m_center;
  quad_tree_node *m_q [4];
  objects_vector m_objects;

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
    m_split = true;

    objects_vector ov;
    ov.swap (m_objects);

    for (auto o = ov.begin (); o != ov.end (); ++o) {
      insert (*o, ucenter);
    }
  }

  void insert (const T &value, const point_type &ucenter)
  {
    if (! m_split && m_objects.size () + 1 < thr) {

      m_objects.push_back (value);

    } else {

      if (! m_split) {
        split (ucenter);
      }

      box_type b = BC () (value);
      //  @@@ should exclude m_center on box
      if (b.contains (m_center)) {
        m_objects.push_back (value);
        return;
      }

      for (unsigned int i = 0; i < 4; ++i) {
        box_type bq = q (i, ucenter);
        if (b.inside (bq)) {
          if (! m_q[i]) {
            m_q[i] = new quad_tree_node (bq.center ());
          }
          m_q[i]->insert (value, m_center);
          return;
        }
      }

      for (unsigned int i = 0; i < 4; ++i) {
        if (m_q[i]) {
          grow (m_center - (m_center - m_q[i]->center ()) * 2.0);
          insert (value, ucenter);
          return;
        }
      }

      tl_assert (false);

    }
  }

  void grow (const point_type &ucenter)
  {
    for (unsigned int i = 0; i < 4; ++i) {
      if (m_q[i]) {
        quad_tree_node *n = m_q[i];
        m_q[i] = new quad_tree_node (q (i, ucenter).center ());
        m_q[i]->m_q[3 - i] = n;
      }
    }
  }

  point_type propose_ucenter (const box_type &total_box) const
  {
    for (unsigned int i = 0; i < 4; ++i) {
      if (m_q[i]) {
        return m_center - (m_center - m_q[i]->center ()) * 2.0;
      }
    }

    coord_type dx = std::max (std::abs (total_box.left () - m_center.x ()), std::abs (total_box.right () - m_center.y ()));
    coord_type dy = std::max (std::abs (total_box.bottom () - m_center.y ()), std::abs (total_box.top () - m_center.y ()));
    return m_center - vector_type (dx, dy);
  }
};

template <class T, class BC, size_t thr, class S>
class quad_tree_iterator
{
public:
  typedef quad_tree_node<T, BC, thr> quad_tree_node_type;
  typedef typename T::coord_type coord_type;
  typedef db::box<coord_type> box_type;

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

    while (! m_stack.empty ()) {

      m_stack.pop_back ();

      int &n = m_stack.back ().second;
      while (++n < 4) {
        box_type bq = qn->q_box (n);
        if (! bq.empty () && m_s.select_quad (bq)) {
          m_stack.push_back (std::make_pair (qn->node (n), -1));
          validate ();
          return;
        }
      }

    }
  }
};

template <class T, class BC>
class quad_tree_always_sel
{
public:
  typedef typename T::coord_type coord_type;
  typedef db::box<coord_type> box_type;

  bool select (const T &) const
  {
    return true;
  }

  bool select_quad (const box_type &) const
  {
    return true;
  }
};

template <class T, class BC>
class quad_tree_touching_sel
{
public:
  typedef typename T::coord_type coord_type;
  typedef db::box<coord_type> box_type;

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

template <class T, class BC>
class quad_tree_overlapping_sel
{
public:
  typedef typename T::coord_type coord_type;
  typedef db::box<coord_type> box_type;

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

template <class T, class BC, size_t thr>
class quad_tree
{
public:
  typedef quad_tree_node<T, BC, thr> quad_tree_node_type;
  typedef quad_tree_iterator<T, BC, thr, quad_tree_always_sel<T, BC> > quad_tree_flat_iterator;
  typedef quad_tree_iterator<T, BC, thr, quad_tree_touching_sel<T, BC> > quad_tree_touching_iterator;
  typedef quad_tree_iterator<T, BC, thr, quad_tree_overlapping_sel<T, BC> > quad_tree_overlapping_iterator;
  typedef typename T::coord_type coord_type;
  typedef db::box<coord_type> box_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef std::vector<T> objects_vector;

  quad_tree ()
    : m_root (point_type ())
  {
    //  .. nothing yet ..
  }

  bool empty () const
  {
    return m_root.empty ();
  }

  size_t size () const
  {
    return m_root.size ();
  }

  void insert (const T &value)
  {
    box_type b = BC () (value);
    if (b.empty ()) {
      return;
    }

    m_total_box += b;
    m_root.insert_top (value, m_total_box);
  }

  void erase (const T &value)
  {
    m_root.remove (value);
  }

  quad_tree_flat_iterator begin () const
  {
    return quad_tree_flat_iterator (&m_root, quad_tree_always_sel<T, BC> ());
  }

  quad_tree_overlapping_iterator begin_overlapping (const box_type &box) const
  {
    if (m_total_box.overlaps (box)) {
      return quad_tree_overlapping_iterator (&m_root, quad_tree_overlapping_sel<T, BC> (box));
    } else {
      return quad_tree_overlapping_iterator ();
    }
  }

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
