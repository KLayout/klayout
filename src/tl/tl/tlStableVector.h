
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



#ifndef HDR_tlStableVector
#define HDR_tlStableVector

#include <stddef.h>
#include <vector>
#include <iterator>

namespace tl 
{

/**
 *  @brief A stable vector class
 *
 *  Unlike the normal vector, the stable vector guarantees that the  
 *  objects stored within are not relocated to other memory locations
 *  even if the vector reallocates memory.
 *  The stable vector does this by maintaining pointers rather than
 *  objects itself.
 *  This way, this container is able to store objects that do not have
 *  a copy semantics.
 *  
 *  The stable vector offers two iterators: a usual one and a stable one.
 *  While the usual iterator is invalidated if the vector reallocates
 *  (the iterator gets invalid, not the location of the objects them-
 *  selves), the stable iterator stays valid even if the container 
 *  reallocates. The stable iterator is a pointer to the container plus
 *  an index.
 */

template <class X>
class stable_vector
{
public:
  typedef X value_type;

  class iterator;

  class const_iterator 
  {
  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef X value_type;
    typedef const X &reference;
    typedef const X *pointer;

    const_iterator ()
      : m_iter ()
    {
      //  .. nothing yet ..
    }

    const_iterator (const const_iterator &d)
      : m_iter (d.m_iter)
    {
      //  .. nothing yet ..
    }

    const_iterator (const iterator &d)
      : m_iter (d.m_iter)
    {
      //  .. nothing yet ..
    }

    const_iterator (typename std::vector<X *>::const_iterator d)
      : m_iter (d)
    {
      //  .. nothing yet ..
    }

    const_iterator &operator= (const_iterator d) 
    {
      m_iter = d.m_iter;
      return *this;
    }

    const iterator &operator= (iterator d) 
    {
      m_iter = d.m_iter;
      return *this;
    }

    bool operator== (const_iterator d) const
    {
      return m_iter == d.m_iter;
    }

    bool operator!= (const_iterator d) const
    {
      return m_iter != d.m_iter;
    }

    bool operator< (const_iterator d) const
    {
      return m_iter < d.m_iter;
    }

    ptrdiff_t operator- (const_iterator d) const
    {
      return m_iter - d.m_iter;
    }

    const_iterator operator- (ptrdiff_t n) const
    {
      return m_iter - n;
    }

    const_iterator operator+ (ptrdiff_t n) const
    {
      return m_iter + n;
    }

    const_iterator &operator+= (ptrdiff_t n) 
    {
      m_iter += n;
      return *this;
    }

    const_iterator &operator-= (ptrdiff_t n)
    {
      m_iter -= n;
      return *this;
    }

    const_iterator &operator++ ()
    {
      ++m_iter;
      return *this;
    }

    const_iterator operator++ (int)
    {
      const_iterator i = *this;
      ++m_iter;
      return i;
    }

    const_iterator &operator-- ()
    {
      --m_iter;
      return *this;
    }

    const_iterator operator-- (int)
    {
      const_iterator i = *this;
      --m_iter;
      return i;
    }

    const X &operator[] (ptrdiff_t n) const
    {
      return *m_iter [n];
    }

    const X &operator* () const
    {
      return **m_iter;
    }

    const X *operator-> () const
    {
      return *m_iter;
    }

  private:
    typename std::vector<X *>::const_iterator m_iter;
  };

  class iterator 
  {
  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef X value_type;
    typedef X &reference;
    typedef X *pointer;

    iterator ()
      : m_iter ()
    {
      //  .. nothing yet ..
    }

    iterator (const iterator &d)
      : m_iter (d.m_iter)
    {
      //  .. nothing yet ..
    }

    iterator (typename std::vector<X *>::iterator d)
      : m_iter (d)
    {
      //  .. nothing yet ..
    }

    iterator &operator= (iterator d) 
    {
      m_iter = d.m_iter;
      return *this;
    }

    bool operator== (iterator d) const
    {
      return m_iter == d.m_iter;
    }

    bool operator!= (iterator d) const
    {
      return m_iter != d.m_iter;
    }

    bool operator< (iterator d) const
    {
      return m_iter < d.m_iter;
    }

    ptrdiff_t operator- (iterator d) const
    {
      return m_iter - d.m_iter;
    }

    iterator operator- (ptrdiff_t n) const
    {
      return m_iter - n;
    }

    iterator operator+ (ptrdiff_t n) const
    {
      return m_iter + n;
    }

    iterator &operator+= (ptrdiff_t n) 
    {
      m_iter += n;
      return *this;
    }

    iterator &operator-= (ptrdiff_t n)
    {
      m_iter -= n;
      return *this;
    }

    iterator &operator++ ()
    {
      ++m_iter;
      return *this;
    }

    iterator operator++ (int)
    {
      iterator i = *this;
      ++m_iter;
      return i;
    }

    iterator &operator-- ()
    {
      --m_iter;
      return *this;
    }

    iterator operator-- (int)
    {
      iterator i = *this;
      --m_iter;
      return i;
    }

    X &operator[] (ptrdiff_t n) const
    {
      return *m_iter [n];
    }

    X &operator* () const
    {
      return **m_iter;
    }

    X *operator-> () const
    {
      return *m_iter;
    }

  private:
    typename std::vector<X *>::iterator m_iter;
    friend class stable_vector::const_iterator;
  };

  class stable_iterator;

  class stable_const_iterator 
  {
  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef X value_type;
    typedef const X &reference;
    typedef const X *pointer;

    stable_const_iterator ()
      : mp_vector (0), m_index (0)
    {
      //  .. nothing yet ..
    }

    stable_const_iterator (const stable_const_iterator &d)
      : mp_vector (d.mp_vector), m_index (d.m_index)
    {
      //  .. nothing yet ..
    }

    stable_const_iterator (const stable_iterator &d)
      : mp_vector (d.mp_vector), m_index (d.m_index)
    {
      //  .. nothing yet ..
    }

    stable_const_iterator (const std::vector<X *> &v, size_t index)
      : mp_vector (&v), m_index (index)
    {
      //  .. nothing yet ..
    }

    stable_const_iterator &operator= (stable_const_iterator d) 
    {
      mp_vector = d.mp_vector;
      m_index = d.m_index;
      return *this;
    }

    stable_const_iterator &operator= (stable_iterator d) 
    {
      mp_vector = d.mp_vector;
      m_index = d.m_index;
      return *this;
    }

    bool operator== (stable_const_iterator d) const
    {
      return mp_vector == d.mp_vector && m_index == d.m_index;
    }

    bool operator!= (stable_const_iterator d) const
    {
      return !operator== (d);
    }

    bool operator< (stable_const_iterator d) const
    {
      return m_index < d.m_index;
    }

    ptrdiff_t operator- (stable_const_iterator d) const
    {
      return m_index - d.m_index;
    }

    stable_const_iterator operator+ (ptrdiff_t n) const
    {
      return stable_const_iterator (*mp_vector, m_index + n);
    }

    stable_const_iterator operator- (ptrdiff_t n) const
    {
      return stable_const_iterator (*mp_vector, m_index - n);
    }

    stable_const_iterator &operator+= (ptrdiff_t n) 
    {
      m_index += n;
      return *this;
    }

    stable_const_iterator &operator-= (ptrdiff_t n)
    {
      m_index -= n;
      return *this;
    }

    stable_const_iterator &operator++ ()
    {
      ++m_index;
      return *this;
    }

    stable_const_iterator operator++ (int)
    {
      stable_const_iterator i = *this;
      ++m_index;
      return i;
    }

    stable_const_iterator &operator-- ()
    {
      --m_index;
      return *this;
    }

    stable_const_iterator operator-- (int)
    {
      stable_const_iterator i = *this;
      --m_index;
      return i;
    }

    const X &operator[] (ptrdiff_t n) const
    {
      return *(*mp_vector) [m_index + n];
    }

    const X &operator* () const
    {
      return *(*mp_vector) [m_index];
    }

    const X *operator-> () const
    {
      return (*mp_vector) [m_index];
    }

    size_t index () const
    {
      return m_index;
    }

  private:
    const std::vector<X *> *mp_vector;
    size_t m_index;
  };

  class stable_iterator 
  {
  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef X value_type;
    typedef X &reference;
    typedef X *pointer;

    stable_iterator ()
      : mp_vector (0), m_index (0)
    {
      //  .. nothing yet ..
    }

    stable_iterator (const stable_iterator &d)
      : mp_vector (d.mp_vector), m_index (d.m_index)
    {
      //  .. nothing yet ..
    }

    stable_iterator (std::vector<X *> &v, size_t index)
      : mp_vector (&v), m_index (index)
    {
      //  .. nothing yet ..
    }

    stable_iterator &operator= (stable_iterator d) 
    {
      mp_vector = d.mp_vector;
      m_index = d.m_index;
      return *this;
    }

    bool operator== (stable_iterator d) const
    {
      return mp_vector == d.mp_vector && m_index == d.m_index;
    }

    bool operator!= (stable_iterator d) const
    {
      return !operator== (d);
    }

    bool operator< (stable_iterator d) const
    {
      return m_index < d.m_index;
    }

    ptrdiff_t operator- (stable_iterator d) const
    {
      return m_index - d.m_index;
    }

    stable_iterator operator+ (ptrdiff_t n) const
    {
      return stable_iterator (*mp_vector, m_index + n);
    }

    stable_iterator operator- (ptrdiff_t n) const
    {
      return stable_iterator (*mp_vector, m_index - n);
    }

    stable_iterator &operator+= (ptrdiff_t n) 
    {
      m_index += n;
      return *this;
    }

    stable_iterator &operator-= (ptrdiff_t n)
    {
      m_index -= n;
      return *this;
    }

    stable_iterator &operator++ ()
    {
      ++m_index;
      return *this;
    }

    stable_iterator operator++ (int)
    {
      stable_iterator i = *this;
      ++m_index;
      return i;
    }

    stable_iterator &operator-- ()
    {
      --m_index;
      return *this;
    }

    stable_iterator operator-- (int)
    {
      stable_iterator i = *this;
      --m_index;
      return i;
    }

    X &operator[] (ptrdiff_t n) const
    {
      return *(*mp_vector) [m_index + n];
    }

    X &operator* () const
    {
      return *(*mp_vector) [m_index];
    }

    X *operator-> () const
    {
      return (*mp_vector) [m_index];
    }

    size_t index () const
    {
      return m_index;
    }

  private:
    std::vector<X *> *mp_vector;
    size_t m_index;
    friend class stable_vector::stable_const_iterator;
  };

  stable_vector () 
    : m_objects ()
  {
    //  .. nothing yet ..
  }

  stable_vector (const stable_vector<X> &d) 
    : m_objects ()
  {
    operator= (d);
  }

  ~stable_vector ()
  {
    delete_objects ();
  }

  void reserve (size_t n)
  {
    m_objects.reserve (n);
  }

  stable_vector &operator= (const stable_vector<X> &d)
  {
    if (&d != this) {
      delete_objects ();
      m_objects.reserve (d.size ());
      for (typename std::vector <X *>::const_iterator c = d.m_objects.begin (); c != d.m_objects.end (); ++c) {
        m_objects.push_back (new X (**c));
      }
    }
    return *this;
  }

  bool operator== (const stable_vector<X> &d) const
  {
    if (size () != d.size ()) {
      return false;
    }
    for (typename std::vector <X *>::const_iterator a = m_objects.begin (), b = d.m_objects.begin (); a != m_objects.end (); ++a, ++b) {
      if (**a != **b) {
        return false;
      }
    }
    return true;
  }

  bool operator!= (const stable_vector<X> &d) const
  {
    return !operator== (d);
  }

  bool operator< (const stable_vector<X> &d) const
  {
    if (size () != d.size ()) {
      return size () < d.size ();
    }
    for (typename std::vector <X *>::const_iterator a = m_objects.begin (), b = d.m_objects.begin (); a != m_objects.end (); ++a, ++b) {
      if (**a != **b) {
        return **a < **b;
      }
    }
    return false;
  }

  void clear ()
  {
    delete_objects ();
  }

  bool empty () const
  {
    return m_objects.empty ();
  }

  void swap (stable_vector<X> &d)
  {
    m_objects.swap (d.m_objects);
  }

  void push_back (const X &o) 
  {
    m_objects.push_back (new X (o));
  }

  void push_back (X *o) 
  {
    m_objects.push_back (o);
  }

  const X &back () const
  {
    return *m_objects.back ();
  }

  const X &front () const
  {
    return *m_objects.front ();
  }

  X &back () 
  {
    return *m_objects.back ();
  }

  X &front () 
  {
    return *m_objects.front ();
  }

  void pop_back () 
  {
    delete m_objects.back ();
    m_objects.pop_back ();
  }

  stable_iterator insert (stable_iterator pos, const X &value)
  {
    m_objects.insert (m_objects.begin () + pos.index (), new X (value));
    return pos;
  }

  stable_iterator insert (stable_iterator pos, X *value)
  {
    m_objects.insert (m_objects.begin () + pos.index (), value);
    return pos;
  }

  void erase (stable_iterator pos)
  {
    typename std::vector <X *>::iterator p = m_objects.begin () + pos.index ();
    delete *p;
    m_objects.erase (p);
  }

  void erase (stable_iterator from, stable_iterator to)
  {
    typename std::vector <X *>::iterator p = m_objects.begin () + from.index ();
    typename std::vector <X *>::iterator q = m_objects.begin () + to.index ();
    for (typename std::vector <X *>::iterator i = p; i != q; ++i) {
      delete *i;
    }
    m_objects.erase (p, q);
  }

  iterator insert (iterator pos, const X &value)
  {
    size_t p = pos - begin ();
    m_objects.insert (m_objects.begin () + p, new X (value));
    return begin () + p;
  }

  iterator insert (iterator pos, X *value)
  {
    size_t p = pos - begin ();
    m_objects.insert (m_objects.begin () + p, value);
    return begin () + p;
  }

  void erase (iterator pos)
  {
    typename std::vector <X *>::iterator p = m_objects.begin () + (pos - begin ());
    delete *p;
    m_objects.erase (p);
  }

  void erase (iterator from, iterator to)
  {
    typename std::vector <X *>::iterator p = m_objects.begin () + (from - begin ());
    typename std::vector <X *>::iterator q = m_objects.begin () + (to - begin ());
    for (typename std::vector <X *>::iterator i = p; i != q; ++i) {
      delete *i;
    }
    m_objects.erase (p, q);
  }

  X &operator[] (size_t i)
  {
    return *m_objects [i];
  }

  const X &operator[] (size_t i) const
  {
    return *m_objects [i];
  }

  stable_iterator begin_stable () 
  {
    return stable_iterator (m_objects, 0);
  }

  stable_const_iterator begin_stable () const
  {
    return stable_const_iterator (m_objects, 0);
  }

  stable_iterator end_stable () 
  {
    return stable_iterator (m_objects, size ());
  }

  stable_const_iterator end_stable () const
  {
    return stable_const_iterator (m_objects, size ());
  }

  iterator begin () 
  {
    return iterator (m_objects.begin ());
  }

  const_iterator begin () const
  {
    return const_iterator (m_objects.begin ());
  }

  iterator end () 
  {
    return iterator (m_objects.end ());
  }

  const_iterator end () const
  {
    return const_iterator (m_objects.end ());
  }

  size_t size () const
  {
    return m_objects.size ();
  }

private:
  std::vector <X *> m_objects;

  void delete_objects () 
  {
    for (typename std::vector <X *>::iterator c = m_objects.begin (); c != m_objects.end (); ++c) {
      delete *c;
    }
    m_objects.clear ();
  }
};


} // namespace tl

#endif

