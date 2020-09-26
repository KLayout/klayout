
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#ifndef HDR_dbGenericShapeIterator
#define HDR_dbGenericShapeIterator

#include "dbCommon.h"
#include "dbShapes.h"
#include "dbShapeFlags.h"

namespace db
{

template <class T>
class DB_PUBLIC generic_shape_iterator_delegate_base
{
public:
  typedef T value_type;

  generic_shape_iterator_delegate_base () { }
  virtual ~generic_shape_iterator_delegate_base () { }

  virtual void do_reset (const db::Box & /*region*/, bool /*overlapping*/) { }
  virtual db::Box bbox () const { return db::Box::world (); }
  virtual bool at_end () const = 0;
  virtual void increment () = 0;
  virtual const T *get () const = 0;
  virtual generic_shape_iterator_delegate_base<T> *clone () const = 0;
  virtual bool equals (const generic_shape_iterator_delegate_base<T> *other) const = 0;
};

template <class Iter>
class DB_PUBLIC generic_shape_iterator_delegate2
  : public generic_shape_iterator_delegate_base<typename Iter::value_type>
{
public:
  typedef typename Iter::value_type value_type;

  generic_shape_iterator_delegate2 (const Iter &from, const Iter &to)
    : m_iter (from), m_from (from), m_to (to)
  { }

  virtual void do_reset (const db::Box &, bool)
  {
    m_iter = m_from;
  }

  virtual bool at_end () const
  {
    return m_iter == m_to;
  }

  virtual void increment ()
  {
    ++m_iter;
  }

  virtual const value_type *get () const
  {
    return m_iter.operator-> ();
  }

  generic_shape_iterator_delegate_base<value_type> *clone () const
  {
    return new generic_shape_iterator_delegate2<Iter> (*this);
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
  {
    const generic_shape_iterator_delegate2<Iter> *o = dynamic_cast<const generic_shape_iterator_delegate2<Iter> *> (other);
    return o && o->m_iter == m_iter;
  }

private:
  Iter m_iter, m_from, m_to;
};

template <class Iter>
class DB_PUBLIC generic_shape_iterator_delegate1
  : public generic_shape_iterator_delegate_base<typename Iter::value_type>
{
public:
  typedef typename Iter::value_type value_type;

  generic_shape_iterator_delegate1 (const Iter &from)
    : m_iter (from), m_from (from)
  { }

  virtual void do_reset (const db::Box &, bool)
  {
    m_iter = m_from;
  }

  virtual bool at_end () const
  {
    return m_iter.at_end ();
  }

  virtual void increment ()
  {
    ++m_iter;
  }

  virtual const value_type *get () const
  {
    return m_iter.operator-> ();
  }

  generic_shape_iterator_delegate_base<value_type> *clone () const
  {
    return new generic_shape_iterator_delegate1<Iter> (*this);
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
  {
    const generic_shape_iterator_delegate1<Iter> *o = dynamic_cast<const generic_shape_iterator_delegate1<Iter> *> (other);
    return o && o->m_iter == m_iter;
  }

private:
  Iter m_iter, m_from;
};

template <class T>
class DB_PUBLIC generic_shapes_iterator_delegate
  : public generic_shape_iterator_delegate_base<T>
{
public:
  generic_shapes_iterator_delegate (const db::Shapes *shapes)
    : mp_shapes (shapes), m_iter (mp_shapes->begin (shape_flags<T> ()))
  {
    //  .. nothing yet ..
  }

  virtual void do_reset (const db::Box &box, bool overlapping)
  {
    if (box == db::Box::world ()) {
      m_iter = mp_shapes->begin (shape_flags<T> ());
    } else {
      if (mp_shapes->is_bbox_dirty ()) {
        const_cast<db::Shapes *> (mp_shapes)->update ();
      }
      if (overlapping) {
        m_iter = mp_shapes->begin_overlapping (box, shape_flags<T> ());
      } else {
        m_iter = mp_shapes->begin_touching (box, shape_flags<T> ());
      }
    }
  }

  virtual bool at_end () const
  {
    return m_iter.at_end ();
  }

  virtual void increment ()
  {
    ++m_iter;
  }

  virtual const T *get () const
  {
    return m_iter->basic_ptr (typename T::tag ());
  }

  generic_shape_iterator_delegate_base<T> *clone () const
  {
    return new generic_shapes_iterator_delegate<T> (*this);
  }

  virtual db::Box bbox () const
  {
    if (mp_shapes->is_bbox_dirty ()) {
      const_cast<db::Shapes *> (mp_shapes)->update ();
    }
    return mp_shapes->bbox ();
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<T> *other) const
  {
    const generic_shapes_iterator_delegate<T> *o = dynamic_cast<const generic_shapes_iterator_delegate<T> *> (other);
    return o && o->mp_shapes == mp_shapes && o->m_iter.at_end () == m_iter.at_end () && (m_iter.at_end () || o->m_iter->basic_ptr (typename T::tag ()) == m_iter->basic_ptr (typename T::tag ()));
  }

private:
  const db::Shapes *mp_shapes;
  db::Shapes::shape_iterator m_iter;

  generic_shapes_iterator_delegate (const generic_shapes_iterator_delegate &other)
    : mp_shapes (other.mp_shapes), m_iter (other.m_iter)
  {
    //  .. nothing yet ..
  }
};

template <class T>
class DB_PUBLIC generic_shape_iterator
{
public:
  typedef T value_type;
  typedef const value_type &reference;
  typedef const value_type *pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  template <class Iter>
  generic_shape_iterator (const Iter &from, const Iter &to)
    : mp_delegate (new generic_shape_iterator_delegate2<Iter> (from, to))
  { }

  template <class Iter>
  generic_shape_iterator (const Iter &from)
    : mp_delegate (new generic_shape_iterator_delegate1<Iter> (from))
  { }

  generic_shape_iterator ()
    : mp_delegate (0)
  { }

  generic_shape_iterator (generic_shape_iterator_delegate_base<T> *delegate)
    : mp_delegate (delegate)
  { }

  generic_shape_iterator (const db::Shapes *shapes)
    : mp_delegate (new generic_shapes_iterator_delegate<T> (shapes))
  { }

  generic_shape_iterator (const generic_shape_iterator &other)
    : mp_delegate (other.mp_delegate ? other.mp_delegate->clone () : 0)
  { }

  ~generic_shape_iterator ()
  {
    delete mp_delegate;
  }

  generic_shape_iterator &operator= (const generic_shape_iterator &other)
  {
    if (this != &other) {
      delete mp_delegate;
      mp_delegate = other.mp_delegate ? other.mp_delegate->clone () : 0;
    }
    return *this;
  }

  bool operator== (const generic_shape_iterator<T> &other) const
  {
    return mp_delegate && other.mp_delegate && mp_delegate->equals (other.mp_delegate);
  }

  template <class TO>
  bool operator== (const generic_shape_iterator<TO> &) const
  {
    return false;
  }

  reference operator* () const
  {
    return *mp_delegate->get ();
  }

  pointer operator-> () const
  {
    return mp_delegate->get ();
  }

  generic_shape_iterator &operator++ ()
  {
    mp_delegate->increment ();
    return *this;
  }

  bool at_end () const
  {
    return !mp_delegate || mp_delegate->at_end ();
  }

  void reset ()
  {
    if (mp_delegate) {
      mp_delegate->do_reset (db::Box::world (), false);
    }
  }

  void reset (const db::Box &box, bool overlapping)
  {
    if (mp_delegate) {
      mp_delegate->do_reset (box, overlapping);
    }
  }

  db::Box bbox () const
  {
    return mp_delegate ? mp_delegate->bbox () : db::Box ();
  }

public:
  generic_shape_iterator_delegate_base<T> *mp_delegate;
};

}

#endif
