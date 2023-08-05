
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


#ifndef _HDR_gsiIterators
#define _HDR_gsiIterators

#include "tlException.h"
#include "tlTypeTraits.h"
#include "gsiSerialisation.h"

#include <iterator>

//  For a comprehensive documentation see gsi.h

namespace gsi
{

template <class X, class Y> struct address_of;

template<class X> 
struct address_of<X, X> 
{
  address_of<X, X> () : b () { }
  const void *operator() (const X &x) const { b = x; return &b; }
  mutable X b;
};

template<class X> 
struct address_of<X &, X> 
{
  const void *operator() (X &x) const { return &x; }
};

template<class X> 
struct address_of<const X &, X> 
{
  const void *operator() (const X &x) const { return &x; }
};

template<class X> 
struct address_of<X *, X *> 
{
  const void *operator() (X *x) const { return x; }
};

template<class X> 
struct address_of<const X *, const X *> 
{
  const void *operator() (const X *x) const { return x; }
};

template<class X> 
struct address_of<X * const &, X *> 
{
  const void *operator() (X *x) const { return x; }
};

template<class X> 
struct address_of<const X * const &, const X *> 
{
  const void *operator() (const X *x) const { return x; }
};

template<class X> 
struct address_of<X * &, X *> 
{
  const void *operator() (X *x) const { return x; }
};

template<class X> 
struct address_of<const X * &, const X *> 
{
  const void *operator() (const X *x) const { return x; }
};

/**
 *  @brief The basic iterator abstraction 
 */
class IterAdaptorAbstractBase
{
public:
  virtual ~IterAdaptorAbstractBase () { }

  virtual void get (SerialArgs &w) const = 0;
  virtual size_t serial_size () const = 0;
  virtual bool at_end () const = 0;
  virtual void inc () = 0;
};

/**
 *  @brief The template providing a binding of a specific C++ iterator to the abstraction
 */
template <class V>
class IterPtrAdaptor 
  : public IterAdaptorAbstractBase
{
public:
  IterPtrAdaptor (V *b, V *e) 
    : m_b (b), m_e (e)
  {
    //  .. nothing yet ..
  }

  virtual void get (SerialArgs &w) const
  {
    w.write<V &> (*m_b);
  }
  
  virtual size_t serial_size () const 
  {
    return gsi::type_traits<V &>::serial_size ();
  }

  virtual bool at_end () const 
  {
    return m_b == m_e;
  }

  virtual void inc () 
  {
    ++m_b;
  }

private:
  V *m_b, *m_e;
};

/**
 *  @brief The template providing a binding of a specific C++ iterator to the abstraction
 */
template <class V>
class ConstIterPtrAdaptor 
  : public IterAdaptorAbstractBase
{
public:
  ConstIterPtrAdaptor (const V *b, const V *e) 
    : m_b (b), m_e (e)
  {
    //  .. nothing yet ..
  }

  virtual void get (SerialArgs &w) const
  {
    w.write<const V &> (*m_b);
  }

  virtual size_t serial_size () const 
  {
    return gsi::type_traits<const V &>::serial_size ();
  }

  virtual bool at_end () const 
  {
    return m_b == m_e;
  }

  virtual void inc () 
  {
    ++m_b;
  }

private:
  const V *m_b, *m_e;
};

/**
 *  @brief The template providing a binding of a specific C++ iterator to the abstraction
 */
template <class I>
class IterAdaptor 
  : public IterAdaptorAbstractBase
{
public:
  typedef std::iterator_traits<I> it;
  typedef typename it::reference reference;
  
  IterAdaptor (const I &b, const I &e) 
    : m_b (b), m_e (e)
  {
    //  .. nothing yet ..
  }

  virtual void get (SerialArgs &w) const
  {
    w.write<reference> (*m_b);
  }

  virtual size_t serial_size () const 
  {
    return gsi::type_traits<reference>::serial_size ();
  }

  virtual bool at_end () const 
  {
    return m_b == m_e;
  }

  virtual void inc () 
  {
    ++m_b;
  }

private:
  I m_b, m_e;
};

/**
 *  @brief The template providing a binding of a "free iterator" (one that provides its own at_end method)
 */
template <class I>
class FreeIterAdaptor 
  : public IterAdaptorAbstractBase
{
public:
  typedef std::iterator_traits<I> it;
  typedef typename it::reference reference;
  
  FreeIterAdaptor (const I &i) 
    : m_i (i)
  {
    //  .. nothing yet ..
  }

  virtual void get (SerialArgs &w) const
  {
    w.write<reference> (*m_i);
  }

  virtual size_t serial_size () const 
  {
    return gsi::type_traits<reference>::serial_size ();
  }

  virtual bool at_end () const 
  {
    return m_i.at_end ();
  }

  virtual void inc () 
  {
    ++m_i;
  }

private:
  I m_i;
};

}

#endif

