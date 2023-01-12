
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


#ifndef _HDR_gsiSerialization
#define _HDR_gsiSerialization

#include "gsiTypes.h"
#include "tlHeap.h"
#include "tlUtils.h"

#include <list>
#include <memory>
#include <cstring>

namespace gsi
{

// ------------------------------------------------------------
//  SerialArgs definition

class GSI_PUBLIC AdaptorBase;

/**
 *  @brief Copy the contents of adaptor-provided data to the given type X
 *  X can be a valid target type for the adaptor's container scheme - for
 *  example a vector for a vector adaptor.
 *  This generic function will copy the elements of the vector one by one
 *  to the target.
 */
template <class X> void copy_to (AdaptorBase &a, X &x, tl::Heap &heap);

/**
 *  @brief Tie the contents of adaptor-provided data to the given type X
 *  This generic function will first copy the contents of the adaptor-provided
 *  container to the object x. It will also tie both containers, so when the
 *  adaptor is deleted, it will pull it's content from the former target x.
 *  This is used to implement "out" parameter schemes for adapted containers.
 *  This function will take over the ownership of the a adaptor.
 */
template <class X> void tie_copies (AdaptorBase *a, X &x, tl::Heap &heap);

/**
 *  @brief An adaptor factory for adaptors of type X
 */
template <class Tag, class X>
struct adaptor_factory
{
  /**
   *  @brief The factory method
   *  This method will return a new adaptor object providing access to the raw object v.
   */
  static AdaptorBase *get (const X &v);
};

/**
 *  @brief An exception thrown if there are not enough arguments on the serialization buffer
 */
struct GSI_PUBLIC ArglistUnderflowException
  : public tl::Exception
{
  ArglistUnderflowException ()
    : tl::Exception (tl::to_string (tr ("Too few arguments or no return value supplied")))
  { }
};

/**
 *  @brief An exception thrown if there are not enough arguments on the serialization buffer
 */
struct GSI_PUBLIC ArglistUnderflowExceptionWithType
  : public tl::Exception
{
  ArglistUnderflowExceptionWithType (const ArgSpecBase &as)
    : tl::Exception (tl::to_string (tr ("Too few arguments - missing '%s'")), as.name ())
  { }
};

/**
 *  @brief An exception thrown if a reference is null (nil)
 */
struct GSI_PUBLIC NilPointerToReference
  : public tl::Exception
{
  NilPointerToReference ()
    : tl::Exception (tl::to_string (tr ("nil object passed to a reference")))
  { }
};

/**
 *  @brief An exception thrown if a reference is null (nil)
 */
struct GSI_PUBLIC NilPointerToReferenceWithType
  : public tl::Exception
{
  NilPointerToReferenceWithType (const ArgSpecBase &as)
    : tl::Exception (tl::to_string (tr ("nil object passed to a reference for '%s'")), as.name ())
  { }
};

/**
 *  @brief This class provides the basic argument serialization mechanism for the C++/scripting interface
 */
class GSI_PUBLIC SerialArgs
{
public:
  /**
   *  @brief Default constructor
   *  Creates an empty list.
   */
  SerialArgs ()
    : mp_buffer (0)
  {
    mp_write = mp_read = mp_buffer;
  }

  /**
   *  @brief Constructor
   *  Creates a buffer with space for len bytes.
   */
  SerialArgs (size_t len)
    : mp_buffer (0)
  {
    //  use the internal buffer for small argument lists (which are quite common)
    if (len > sizeof (m_buffer)) {
      mp_buffer = new char [len];
    } else if (len > 0) {
      mp_buffer = m_buffer;
    }
    mp_write = mp_read = mp_buffer;
  }

  /**
   *  @brief Destructor
   */
  ~SerialArgs ()
  {
    if (mp_buffer && mp_buffer != m_buffer) {
      delete [] mp_buffer;
    }
    mp_buffer = 0;
  }

  /**
   *  @brief Resets the write and read pointer
   */
  void reset ()
  {
    mp_read = mp_write = mp_buffer;
  }

  /**
   *  @brief gets a pointer to the buffer
   */
  char *cptr ()
  {
    return mp_buffer;
  }

  /**
   *  @brief gets a const pointer to the buffer
   */
  const char *cptr () const
  {
    return mp_buffer;
  }

  /**
   *  @brief Gets a pointer to the current write position
   */
  char *wptr ()
  {
    return mp_write;
  }

  /**
   *  @brief Gets a const pointer to the current write position
   */
  const char *wptr () const
  {
    return mp_write;
  }

  /**
   *  @brief Gets a pointer to the current read position
   */
  char *rptr ()
  {
    return mp_read;
  }

  /**
   *  @brief Gets a const pointer to the current read position
   */
  const char *rptr () const
  {
    return mp_read;
  }

  /**
   *  @brief Writes a value into the buffer
   */
  template <class X>
  inline void write (const X &x)
  {
    this->write_impl (typename type_traits<X>::tag (), x);
  }

  /**
   *  @brief Reads a value from the buffer
   */
  template <class X>
  inline X read (tl::Heap &heap)
  {
    return this->read_impl<X> (typename type_traits<X>::tag (), heap, 0);
  }

  /**
   *  @brief Reads a value from the buffer
   */
  template <class X>
  inline X read (tl::Heap &heap, const ArgSpecBase *as)
  {
    return this->read_impl<X> (typename type_traits<X>::tag (), heap, as);
  }

  /**
   *  @brief returns true, if there is still data available for read
   */
  operator bool () const
  {
    return mp_read && mp_read < mp_write;
  }

private:
  char *mp_buffer;
  char *mp_read, *mp_write;
  char m_buffer[200];

  inline void check_data () const
  {
    if (! *this) {
      throw ArglistUnderflowException ();
    }
  }

  inline void check_data (const ArgSpecBase *as) const
  {
    if (! *this) {
      if (as) {
        throw ArglistUnderflowExceptionWithType (*as);
      } else {
        throw ArglistUnderflowException ();
      }
    }
  }

  inline void throw_nil_for_reference (const ArgSpecBase *as) const
  {
    if (as) {
      throw NilPointerToReferenceWithType (*as);
    } else {
      throw NilPointerToReference ();
    }
  }

  // -----------------------------------------------------------
  //  reader implementations

  template <class X>
  void write_impl (const pod_direct_tag &, const X &x)
  {
    *((X *)mp_write) = x;
    mp_write += item_size<X> ();
  }

  template <class X>
  void write_impl (const x_tag &, const X &x)
  {
    *((X **)mp_write) = new X (x);
    mp_write += item_size<X *> ();
  }

  template <class X>
  void write_impl (const ref_tag &, X &x)
  {
    *((X **)mp_write) = &x;
    mp_write += item_size<X *> ();
  }

  template <class X>
  void write_impl (const ptr_tag &, X *x)
  {
    *((X **)mp_write) = x;
    mp_write += item_size<X *> ();
  }

  template <class X>
  void write_impl (const pod_cref_tag &, const X &x)
  {
    X *r = (X *)mp_write;
    new (r) X(x);
    mp_write += item_size<X> ();
  }

  template <class X>
  void write_impl (const npod_cref_tag &, const X &x)
  {
    *((X const **)mp_write) = &x;
    mp_write += item_size<const X *> ();
  }

  template <class X>
  void write_impl (const x_cref_tag &, const X &x)
  {
    *((X const **)mp_write) = &x;
    mp_write += item_size<const X *> ();
  }

  template <class X>
  void write_impl (const pod_cptr_tag &, const X *x)
  {
    *(bool *)mp_write = (x != 0);
    mp_write += item_size<bool> ();
    if (x) {
      *(X *)mp_write = *x;
    }
    mp_write += item_size<X> ();
  }

  //  see notes on the serialization for this type:
  template <class X>
  void write_impl (const npod_cptr_tag &, const X *x)
  {
    *((X const **)mp_write) = x;
    mp_write += item_size<const X *> ();
  }

  template <class X>
  void write_impl (const x_cptr_tag &, const X *x)
  {
    *((X const **)mp_write) = x;
    mp_write += item_size<const X *> ();
  }

  void write_impl (const vptr_tag &, void *x)
  {
    *((void **)mp_write) = x;
    mp_write += item_size<void *> ();
  }

  void write_impl (const vptr_tag &, const void *x)
  {
    *((const void **)mp_write) = x;
    mp_write += item_size<const void *> ();
  }

  template <class X>
  void write_impl (const adaptor_direct_tag &, const X &x)
  {
    *((void **)mp_write) = adaptor_factory<typename gsi::type_traits<X>::tag, X>::get (x);
    mp_write += item_size<void *> ();
  }

  template <class X>
  void write_impl (const adaptor_ptr_tag &, X *x)
  {
    if (x) {
      *((void **)mp_write) = adaptor_factory<typename gsi::type_traits<X *>::tag, X *>::get (x);
    } else {
      *((void **)mp_write) = 0;
    }
    mp_write += item_size<void *> ();
  }

  template <class X>
  void write_impl (const adaptor_cptr_tag &, const X *x)
  {
    if (x) {
      *((void **)mp_write) = adaptor_factory<typename gsi::type_traits<const X *>::tag, const X *>::get (x);
    } else {
      *((void **)mp_write) = 0;
    }
    mp_write += item_size<void *> ();
  }

  template <class X>
  void write_impl (const adaptor_ref_tag &, X &x)
  {
    *((void **)mp_write) = adaptor_factory<typename gsi::type_traits<X &>::tag, X &>::get (x);
    mp_write += item_size<void *> ();
  }

  template <class X>
  void write_impl (const adaptor_cref_tag &, const X &x)
  {
    *((void **)mp_write) = adaptor_factory<typename gsi::type_traits<const X &>::tag, const X &>::get (x);
    mp_write += item_size<void *> ();
  }

  // -----------------------------------------------------------
  //  reader implementations

  template <class X>
  X read_impl (const pod_direct_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    check_data (as);
    X r = *((X *)mp_read);
    mp_read += item_size<X> ();
    return r;
  }

  template <class X>
  X read_impl (const x_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    check_data (as);
    X *xp = *(X **)mp_read;
    X x = *xp;
    delete xp;
    mp_read += item_size<void *> ();
    return x;
  }

  template <class X>
  X read_impl (const vptr_tag &, tl::Heap &, const ArgSpecBase *)
  {
    void *r = *((void **)mp_read);
    mp_read += item_size<void *> ();
    // Hint: X can be const unsigned char *, const signed char * as well.
    return (X)r;
  }

  template <class X>
  X read_impl (const ref_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    typedef typename type_traits<X>::value_type value_type;
    check_data (as);
    value_type *r = *((value_type **)mp_read);
    mp_read += item_size<value_type *> ();
    if (! r) {
      throw_nil_for_reference (as);
    }
    return *r;
  }

  template <class X>
  X read_impl (const pod_cref_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    //  X is actually an (const X &)
    typedef typename type_traits<X>::value_type value_type;
    check_data (as);
    const value_type *r = ((const value_type *)mp_read);
    mp_read += item_size<value_type> ();
    return *r;
  }

  template <class X>
  X read_impl (const npod_cref_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    //  X is actually an (const X &)
    typedef typename type_traits<X>::value_type value_type;
    check_data (as);
    const value_type *r = *((const value_type **)mp_read);
    mp_read += item_size<const value_type *> ();
    if (! r) {
      throw_nil_for_reference (as);
    }
    return *r;
  }

  template <class X>
  X read_impl (const x_cref_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    //  X is actually an (const X &)
    typedef typename type_traits<X>::value_type value_type;
    check_data (as);
    const value_type *r = *((const value_type **)mp_read);
    mp_read += item_size<const value_type *> ();
    if (! r) {
      throw_nil_for_reference (as);
    }
    return *r;
  }

  template <class X>
  X read_impl (const ptr_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    //  X is actually an (X *)
    typedef typename type_traits<X>::value_type value_type;
    check_data (as);
    value_type * const &r = *((value_type **)mp_read);
    mp_read += item_size<value_type *> ();
    return r;
  }

  template <class X>
  X read_impl (const pod_cptr_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    //  X is actually an (const X *)
    check_data (as);
    bool h = *(bool *)mp_read;
    mp_read += item_size<bool> ();
    X r = h ? (X)mp_read : (X)0;
    mp_read += item_size<X> ();
    return r;
  }

  //  see notes on the serialization for this type:
  template <class X>
  X read_impl (const npod_cptr_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    //  X is actually an (const X *)
    check_data (as);
    X r = *((X *)mp_read);
    mp_read += item_size<X> ();
    return r;
  }

  template <class X>
  X read_impl (const x_cptr_tag &, tl::Heap &, const ArgSpecBase *as)
  {
    //  X is actually an (const X *)
    check_data (as);
    X r = *((X *)mp_read);
    mp_read += item_size<X> ();
    return r;
  }

  template <class X>
  X read_impl (const adaptor_direct_tag &, tl::Heap &heap, const ArgSpecBase *as)
  {
    check_data (as);

    AdaptorBase *p = *(AdaptorBase **)mp_read;
    mp_read += item_size<AdaptorBase *> ();

    tl_assert (p != 0);
    //  late-destroy the adaptor since the new X object may still need data from there (e.g. QLatin1String)
    heap.push (p);

    X x = X ();
    copy_to<X> (*p, x, heap);
    return x;
  }

  template <class X>
  X read_impl (const adaptor_cref_tag &, tl::Heap &heap, const ArgSpecBase *as)
  {
    typedef typename tl::get_inner_type<X>::result x_type;

    check_data (as);

    AdaptorBase *p = *(AdaptorBase **)mp_read;
    mp_read += item_size<AdaptorBase *> ();

    tl_assert (p != 0);
    //  late-destroy the adaptor since the new X object may still need data from there (e.g. QLatin1String)
    heap.push (p);

    x_type *x = new x_type ();
    heap.push (x);
    copy_to<x_type> (*p, *x, heap);

    return *x;
  }

  template <class X>
  X read_impl (const adaptor_ref_tag &, tl::Heap &heap, const ArgSpecBase *as)
  {
    typedef typename tl::get_inner_type<X>::result x_type;

    check_data (as);

    AdaptorBase *p = *(AdaptorBase **)mp_read;
    mp_read += item_size<AdaptorBase *> ();
    tl_assert (p != 0);

    x_type *x = new x_type ();
    heap.push (x);
    tie_copies<x_type> (p, *x, heap);

    return *x;
  }

  template <class X>
  X read_impl (const adaptor_cptr_tag &, tl::Heap &heap, const ArgSpecBase *as)
  {
    typedef typename tl::get_inner_type<X>::result x_type;

    check_data (as);

    AdaptorBase *p = *(AdaptorBase **) mp_read;
    mp_read += item_size<AdaptorBase *> ();

    x_type *x = 0;
    if (p != 0) {

      //  late-destroy the adaptor since the new X object may still need data from there (e.g. QLatin1String)
      heap.push (p);

      x = new x_type ();
      heap.push (x);
      copy_to<x_type> (*p, *x, heap);

    }

    return x;
  }

  template <class X>
  X read_impl (const adaptor_ptr_tag &, tl::Heap &heap, const ArgSpecBase *as)
  {
    typedef typename tl::get_inner_type<X>::result x_type;

    check_data (as);

    AdaptorBase *p = *(AdaptorBase **)mp_read;
    mp_read += item_size<AdaptorBase *> ();

    x_type *x = 0;
    if (p != 0) {

      x = new x_type ();
      heap.push (x);
      tie_copies<x_type> (p, *x, heap);

    }

    return x;
  }
};

// ------------------------------------------------------------
//  A rebinding of SerialArgs::read to a functor

template <class X>
struct GSI_PUBLIC_TEMPLATE arg_reader
{
  inline X operator() (gsi::SerialArgs &args, tl::Heap &heap) { return args.read<X> (heap); }
};

// ------------------------------------------------------------
//  Provides a function analog to arg_reader, but for a static value
//  The reasoning behind this functor is to support default arguments.
//  Specifically in the case of (const X &) arguments, this must not
//  create references to temporaries.

template <class X>
struct GSI_PUBLIC_TEMPLATE arg_maker
{
  inline X operator() (const X &x, tl::Heap &) { return x; }
};

template <class X>
struct GSI_PUBLIC_TEMPLATE arg_maker<X &>
{
  inline X &operator() (X &x, tl::Heap &) { return x; }
};

template <class X>
struct GSI_PUBLIC_TEMPLATE arg_maker<const X &>
{
  inline const X &operator() (const X &x, tl::Heap &heap)
  { 
    //  avoid references to temp. With this copy we can create a const
    //  reference from a static value.
    X *copy = new X (x);
    heap.push (copy);
    return *copy; 
  }
};

// ------------------------------------------------------------
//  Basic adaptor 

class GSI_PUBLIC AdaptorBase
{
public:
  AdaptorBase ();
  virtual ~AdaptorBase ();

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const = 0;

  void tie_copies (AdaptorBase *target, tl::Heap &heap);
};

// ------------------------------------------------------------
//  String adaptor framework

/**
 *  @brief A generic adaptor for strings
 *  This is the base class for implementing generic access to strings
 */
class GSI_PUBLIC StringAdaptor
  : public AdaptorBase
{
public:
  /**
   *  @brief Default constructor
   */
  StringAdaptor () { }

  /**
   *  @brief Destructor
   */
  virtual ~StringAdaptor () { }

  /**
   *  @brief Returns the size of the string
   */
  virtual size_t size () const = 0;

  /**
   *  @brief Returns a pointer to a UTF8 encoded character array with size size()
   */
  virtual const char *c_str () const = 0;

  /**
   *  @brief Sets the string to the given UTF8 string with length s
   */
  virtual void set (const char *c_str, size_t s, tl::Heap &heap) = 0;

  /**
   *  @brief copy_to implementation
   */
  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const 
  {
    StringAdaptor *s = dynamic_cast<StringAdaptor *>(target);
    tl_assert (s);
    s->set (c_str (), size (), heap);
  }
};

/**
 *  @brief Generic string adaptor implementation
 */
template <class X> 
class GSI_PUBLIC_TEMPLATE StringAdaptorImpl
  : public StringAdaptor
{
};

#if defined(HAVE_QT)

/**
 *  @brief Specialization for QString
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<QString>
  : public StringAdaptor
{
public:
  StringAdaptorImpl (QString *s) 
    : mp_s (s), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QString *s) 
    : mp_s (const_cast<QString *> (s)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QString &s) 
    : m_is_const (false), m_s (s) 
  { 
    mp_s = &m_s; 
  }

  StringAdaptorImpl () 
    : m_is_const (false)
  { 
    mp_s = &m_s; 
  }

  virtual ~StringAdaptorImpl () 
  { 
    //  .. nothing yet ..
  }

  virtual size_t size () const 
  { 
    return mp_s->toUtf8 ().size (); 
  }

  virtual const char *c_str () const
  {
    m_s_utf8 = mp_s->toUtf8 ();
    return m_s_utf8.constData ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &) 
  {
    if (! m_is_const) {
      *mp_s = QString::fromUtf8 (c_str, int (s));
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    StringAdaptorImpl<QString> *s = dynamic_cast<StringAdaptorImpl<QString> *>(target);
    if (s) {
      *s->mp_s = *mp_s;
    } else {
      StringAdaptor::copy_to (target, heap);
    }
  }
   
private:
  QString *mp_s;
  bool m_is_const;
  QString m_s;
  mutable QByteArray m_s_utf8;
};

/**
 *  @brief Specialization for QStringRef
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<QStringRef>
  : public StringAdaptor
{
public:
  StringAdaptorImpl (QStringRef *s) 
    : mp_s (s), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QStringRef *s) 
    : mp_s (const_cast<QStringRef *> (s)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QStringRef &s) 
    : m_is_const (false), m_s (s) 
  { 
    mp_s = &m_s; 
  }

  StringAdaptorImpl () 
    : m_is_const (false)
  { 
    mp_s = &m_s; 
  }

  virtual ~StringAdaptorImpl () 
  { 
    //  .. nothing yet ..
  }

  virtual size_t size () const 
  { 
    return mp_s->toString ().toUtf8 ().size ();
  }

  virtual const char *c_str () const
  {
    m_s_utf8 = mp_s->toString ().toUtf8 ();
    return m_s_utf8.constData ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &heap) 
  {
    if (! m_is_const) {
      QString *qstr = new QString (QString::fromUtf8 (c_str, int (s)));
      heap.push (qstr);
      *mp_s = QStringRef (qstr);
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    StringAdaptorImpl<QStringRef> *s = dynamic_cast<StringAdaptorImpl<QStringRef> *>(target);
    if (s) {
      *s->mp_s = *mp_s;
    } else {
      StringAdaptor::copy_to (target, heap);
    }
  }
   
private:
  QStringRef *mp_s;
  bool m_is_const;
  QStringRef m_s;
  mutable QByteArray m_s_utf8;
};

#if QT_VERSION >= 0x60000

/**
 *  @brief Specialization for QString
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<QStringView>
  : public StringAdaptor
{
public:
  StringAdaptorImpl (QStringView *s)
    : mp_s (s), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QStringView *s)
    : mp_s (const_cast<QStringView *> (s)), m_is_const (true)
  {
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QStringView &s)
    : m_is_const (false), m_s (s)
  {
    mp_s = &m_s;
  }

  StringAdaptorImpl ()
    : m_is_const (false)
  {
    mp_s = &m_s;
  }

  virtual ~StringAdaptorImpl ()
  {
    //  .. nothing yet ..
  }

  virtual size_t size () const
  {
    return mp_s->toUtf8 ().size ();
  }

  virtual const char *c_str () const
  {
    m_s_utf8 = mp_s->toUtf8 ();
    return m_s_utf8.constData ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &heap)
  {
    if (! m_is_const) {
      QString *hstr = heap.create<QString> ();
      *hstr = QString::fromUtf8 (c_str, int (s));
      *mp_s = QStringView (hstr->constData (), hstr->size ());
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    StringAdaptorImpl<QStringView> *s = dynamic_cast<StringAdaptorImpl<QStringView> *>(target);
    if (s) {
      QString *hstr = heap.create<QString> ();
      *hstr = mp_s->toString ();
      *s->mp_s = QStringView (hstr->constData (), hstr->size ());
    } else {
      StringAdaptor::copy_to (target, heap);
    }
  }

private:
  QStringView *mp_s;
  bool m_is_const;
  QStringView m_s;
  mutable QByteArray m_s_utf8;
};

#endif

#if QT_VERSION >= 0x50000

/**
 *  @brief Specialization for QLatin1String
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<QLatin1String>
  : public StringAdaptor
{
public:
  StringAdaptorImpl (QLatin1String *s)
    : mp_s (s), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QLatin1String *s)
    : mp_s (const_cast<QLatin1String *> (s)), m_is_const (true)
  {
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const QLatin1String &s)
    : m_is_const (false), m_s (s)
  {
    mp_s = &m_s;
  }

  StringAdaptorImpl ()
    : m_is_const (false)
  {
    mp_s = &m_s;
  }

  virtual ~StringAdaptorImpl ()
  {
    //  .. nothing yet ..
  }

  virtual size_t size () const
  {
    return QString::fromLatin1 (mp_s->data (), mp_s->size ()).toUtf8 ().size ();
  }

  virtual const char *c_str () const
  {
    m_s_utf8 = QString::fromLatin1 (mp_s->data (), mp_s->size ()).toUtf8 ();
    return m_s_utf8.constData ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &heap)
  {
    if (! m_is_const) {
      QByteArray *latin1_holder = new QByteArray (QString::fromUtf8 (c_str, int (s)).toLatin1 ());
      heap.push (latin1_holder);
      *mp_s = QLatin1String (latin1_holder->constData (), latin1_holder->size ());
    }
  }

private:
  QLatin1String *mp_s;
  bool m_is_const;
  QLatin1String m_s;
  mutable QByteArray m_s_utf8;
};

#endif

#endif

/**
 *  @brief Specialization for std::string
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<std::string>
  : public StringAdaptor
{
public:
  StringAdaptorImpl (std::string *s) 
    : mp_s (s), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const std::string *s) 
    : mp_s (const_cast<std::string *> (s)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImpl (const std::string &s) 
    : m_is_const (false), m_s (s) 
  { 
    mp_s = &m_s; 
  }

  StringAdaptorImpl () 
    : m_is_const (false)
  { 
    mp_s = &m_s; 
  }

  virtual ~StringAdaptorImpl () 
  { 
    //  .. nothing yet ..
  }

  virtual size_t size () const 
  { 
    return mp_s->size (); 
  }

  virtual const char *c_str () const
  {
    return mp_s->c_str ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &) 
  {
    if (! m_is_const) {
      *mp_s = std::string (c_str, s);
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    StringAdaptorImpl<std::string> *s = dynamic_cast<StringAdaptorImpl<std::string> *>(target);
    if (s) {
      *s->mp_s = *mp_s;
    } else {
      StringAdaptor::copy_to (target, heap);
    }
  }
   
private:
  std::string *mp_s;
  bool m_is_const;
  std::string m_s;
};

/**
 *  @brief Specialization for const unsigned char *
 */
template <class CP>
class GSI_PUBLIC_TEMPLATE StringAdaptorImplCCP
  : public StringAdaptor
{
public:
  StringAdaptorImplCCP (CP *s) 
    : mp_s (s), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImplCCP (const CP *s) 
    : mp_s (const_cast<CP *> (s)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  StringAdaptorImplCCP (CP s) 
    : m_is_const (false), m_s ((const char *) s) 
  { 
    mp_s = 0; 
  }

  StringAdaptorImplCCP () 
    : m_is_const (false)
  { 
    mp_s = 0; 
  }

  virtual ~StringAdaptorImplCCP () 
  { 
    //  .. nothing yet ..
  }

  virtual size_t size () const 
  { 
    return mp_s ? strlen ((const char *) *mp_s) : m_s.size ();
  }

  virtual const char *c_str () const
  {
    return mp_s ? (const char *) *mp_s : m_s.c_str ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &heap) 
  {
    if (! m_is_const) {
      if (! mp_s) {
        //  This adaptor is not attached to an external "const char *" pointer, so we can 
        //  safely use the internal storage.
        m_s = std::string (c_str, s);
      } else {
        //  This means we assign a simple pointer some other pointer without 
        //  knowning how long the other points lives. To account for that we create 
        //  a temporary string on the heap and take the pointer from there.
        //  Since the target is a pointer outside this adaptor, we cannot use the
        //  adaptor's internal storage since the adaptor won't survive the outside
        //  use case.
        std::string *tmp_string = new std::string (c_str, s);
        heap.push (tmp_string);
        *mp_s = (CP) tmp_string->c_str ();
      }
    }
  }

private:
  CP *mp_s;
  bool m_is_const;
  std::string m_s;
};

/**
 *  @brief Specialization for const char *
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<const char *>
  : public StringAdaptorImplCCP<const char *>
{
public:
  typedef char char_type;
  StringAdaptorImpl (const char_type **s) : StringAdaptorImplCCP<const char_type *> (s) { }
  StringAdaptorImpl (const char_type * const *s) : StringAdaptorImplCCP<const char_type *> (s) { }
  StringAdaptorImpl (const char_type *s) : StringAdaptorImplCCP<const char_type *> (s) { }
};

/**
 *  @brief Specialization for const unsigned char *
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<const unsigned char *>
  : public StringAdaptorImplCCP<const unsigned char *>
{
public:
  typedef unsigned char char_type;
  StringAdaptorImpl (const char_type **s) : StringAdaptorImplCCP<const char_type *> (s) { }
  StringAdaptorImpl (const char_type * const *s) : StringAdaptorImplCCP<const char_type *> (s) { }
  StringAdaptorImpl (const char_type *s) : StringAdaptorImplCCP<const char_type *> (s) { }
};

/**
 *  @brief Specialization for const signed char *
 */
template <>
class GSI_PUBLIC StringAdaptorImpl<const signed char *>
  : public StringAdaptorImplCCP<const signed char *>
{
public:
  typedef signed char char_type;
  StringAdaptorImpl (const char_type **s) : StringAdaptorImplCCP<const char_type *> (s) { }
  StringAdaptorImpl (const char_type * const *s) : StringAdaptorImplCCP<const char_type *> (s) { }
  StringAdaptorImpl (const char_type *s) : StringAdaptorImplCCP<const char_type *> (s) { }
};

// ------------------------------------------------------------
//  ByteArray adaptor framework

/**
 *  @brief A generic adaptor for strings
 *  This is the base class for implementing generic access to strings
 */
class GSI_PUBLIC ByteArrayAdaptor
  : public AdaptorBase
{
public:
  /**
   *  @brief Default constructor
   */
  ByteArrayAdaptor () { }

  /**
   *  @brief Destructor
   */
  virtual ~ByteArrayAdaptor () { }

  /**
   *  @brief Returns the size of the string
   */
  virtual size_t size () const = 0;

  /**
   *  @brief Returns a pointer to a UTF8 encoded character array with size size()
   */
  virtual const char *c_str () const = 0;

  /**
   *  @brief Sets the string to the given UTF8 string with length s
   */
  virtual void set (const char *c_str, size_t s, tl::Heap &heap) = 0;

  /**
   *  @brief copy_to implementation
   */
  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    ByteArrayAdaptor *s = dynamic_cast<ByteArrayAdaptor *>(target);
    tl_assert (s);
    s->set (c_str (), size (), heap);
  }
};

/**
 *  @brief Generic string adaptor implementation
 */
template <class X>
class GSI_PUBLIC_TEMPLATE ByteArrayAdaptorImpl
  : public ByteArrayAdaptor
{
};

#if defined(HAVE_QT)

/**
 *  @brief Specialization for QByteArray
 */
template <>
class GSI_PUBLIC ByteArrayAdaptorImpl<QByteArray>
  : public ByteArrayAdaptor
{
public:
  ByteArrayAdaptorImpl (QByteArray *s)
    : mp_s (s), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  ByteArrayAdaptorImpl (const QByteArray *s)
    : mp_s (const_cast<QByteArray *> (s)), m_is_const (true)
  {
    //  .. nothing yet ..
  }

  ByteArrayAdaptorImpl (const QByteArray &s)
    : m_is_const (false), m_s (s)
  {
    mp_s = &m_s;
  }

  ByteArrayAdaptorImpl ()
    : m_is_const (false)
  {
    mp_s = &m_s;
  }

  virtual ~ByteArrayAdaptorImpl ()
  {
    //  .. nothing yet ..
  }

  virtual size_t size () const
  {
    return mp_s->size ();
  }

  virtual const char *c_str () const
  {
    return mp_s->constData ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &)
  {
    if (! m_is_const) {
      *mp_s = QByteArray (c_str, int (s));
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    ByteArrayAdaptorImpl<QByteArray> *s = dynamic_cast<ByteArrayAdaptorImpl<QByteArray> *>(target);
    if (s) {
      *s->mp_s = *mp_s;
    } else {
      ByteArrayAdaptor::copy_to (target, heap);
    }
  }

private:
  QByteArray *mp_s;
  bool m_is_const;
  QByteArray m_s;
};

#if QT_VERSION > 0x60000

/**
 *  @brief Specialization for QByteArray
 */
template <>
class GSI_PUBLIC ByteArrayAdaptorImpl<QByteArrayView>
  : public ByteArrayAdaptor
{
public:
  ByteArrayAdaptorImpl (QByteArrayView *s)
    : mp_s (s), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  ByteArrayAdaptorImpl (const QByteArrayView *s)
    : mp_s (const_cast<QByteArrayView *> (s)), m_is_const (true)
  {
    //  .. nothing yet ..
  }

  ByteArrayAdaptorImpl (const QByteArrayView &s)
    : m_is_const (false), m_s (s)
  {
    mp_s = &m_s;
  }

  ByteArrayAdaptorImpl ()
    : m_is_const (false)
  {
    mp_s = &m_s;
  }

  virtual ~ByteArrayAdaptorImpl ()
  {
    //  .. nothing yet ..
  }

  virtual size_t size () const
  {
    return mp_s->size ();
  }

  virtual const char *c_str () const
  {
    return mp_s->constData ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &heap)
  {
    if (! m_is_const) {
      QByteArray *str = heap.create<QByteArray> ();
      *str = QByteArray (c_str, s);
      *mp_s = QByteArrayView (str->constData (), str->size ());
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    ByteArrayAdaptorImpl<QByteArrayView> *s = dynamic_cast<ByteArrayAdaptorImpl<QByteArrayView> *>(target);
    if (s) {
      QByteArray *str = heap.create<QByteArray> ();
      *str = QByteArray (mp_s->constData (), mp_s->size ());
      *s->mp_s = *str;
    } else {
      ByteArrayAdaptor::copy_to (target, heap);
    }
  }

private:
  QByteArrayView *mp_s;
  bool m_is_const;
  QByteArrayView m_s;
};

#endif

#endif

/**
 *  @brief Specialization for std::string
 */
template <>
class GSI_PUBLIC ByteArrayAdaptorImpl<std::vector<char> >
  : public ByteArrayAdaptor
{
public:
  ByteArrayAdaptorImpl (std::vector<char> *s)
    : mp_s (s), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  ByteArrayAdaptorImpl (const std::vector<char> *s)
    : mp_s (const_cast<std::vector<char> *> (s)), m_is_const (true)
  {
    //  .. nothing yet ..
  }

  ByteArrayAdaptorImpl (const std::vector<char> &s)
    : m_is_const (false), m_s (s)
  {
    mp_s = &m_s;
  }

  ByteArrayAdaptorImpl ()
    : m_is_const (false)
  {
    mp_s = &m_s;
  }

  virtual ~ByteArrayAdaptorImpl ()
  {
    //  .. nothing yet ..
  }

  virtual size_t size () const
  {
    return mp_s->size ();
  }

  virtual const char *c_str () const
  {
    return &mp_s->front ();
  }

  virtual void set (const char *c_str, size_t s, tl::Heap &)
  {
    if (! m_is_const) {
      *mp_s = std::vector<char> (c_str, c_str + s);
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    ByteArrayAdaptorImpl<std::vector<char> > *s = dynamic_cast<ByteArrayAdaptorImpl<std::vector<char> > *>(target);
    if (s) {
      *s->mp_s = *mp_s;
    } else {
      ByteArrayAdaptor::copy_to (target, heap);
    }
  }

private:
  std::vector<char> *mp_s;
  bool m_is_const;
  std::vector<char> m_s;
};

// ------------------------------------------------------------
//  Variant adaptor framework

/**
 *  @brief A generic adaptor for variants
 *  This is the base class for implementing generic access to variants
 */
class GSI_PUBLIC VariantAdaptor
  : public AdaptorBase
{
public:
  /**
   *  @brief Default constructor
   */
  VariantAdaptor () { }

  /**
   *  @brief Destructor
   */
  virtual ~VariantAdaptor () { }

  /**
   *  @brief Gets the tl::Variant representing this variant
   */
  virtual tl::Variant var () const = 0;

  /**
   *  @brief Sets the variant's value
   */
  virtual void set (const tl::Variant &v, tl::Heap & /*heap*/) = 0;

  /**
   *  @brief Implementation of copy_to
   */
  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    VariantAdaptor *v = dynamic_cast<VariantAdaptor *>(target);
    tl_assert (v);
    v->set (var (), heap);
  }
};

/**
 *  @brief Generic variant adaptor implementation
 */
template <class X> 
class GSI_PUBLIC_TEMPLATE VariantAdaptorImpl
  : public VariantAdaptor
{
};

#if defined(HAVE_QT)

/**
 *  @brief Specialization for QVariant
 */
template <>
class GSI_PUBLIC VariantAdaptorImpl<QVariant>
  : public VariantAdaptor
{
public:
  VariantAdaptorImpl (QVariant *v) 
    : mp_v (v), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const QVariant *v) 
    : mp_v (const_cast<QVariant *> (v)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const QVariant &v) 
    : m_is_const (true), m_v (v)
  { 
    mp_v = &m_v; 
  }

  VariantAdaptorImpl () 
    : m_is_const (false)
  { 
    mp_v = &m_v; 
  }

  virtual ~VariantAdaptorImpl () 
  { 
    //  .. nothing yet ..
  }

  virtual tl::Variant var () const 
  { 
    return tl::Variant (*mp_v);
  }

  const QVariant &qvar () const 
  { 
    return *mp_v;
  }

  virtual void set (const tl::Variant &v, tl::Heap & /*heap*/)
  {
    if (! m_is_const) {
      *mp_v = v.to_qvariant ();
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    VariantAdaptorImpl<QVariant> *v = dynamic_cast<VariantAdaptorImpl<QVariant> *>(target);
    if (v) {
      *v->mp_v = qvar ();
    } else {
      VariantAdaptor::copy_to (target, heap);
    }
  }
   
private:
  QVariant *mp_v;
  bool m_is_const;
  QVariant m_v;
};

#if QT_VERSION >= 0x50000

/**
 *  @brief Specialization for QVariant
 */
template <typename T>
class GSI_PUBLIC_TEMPLATE VariantAdaptorImpl<QPointer<T> >
  : public VariantAdaptor
{
public:
  VariantAdaptorImpl (QPointer<T> *v)
    : mp_v (v), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const QPointer<T> *v)
    : mp_v (const_cast<QPointer<T> *> (v)), m_is_const (true)
  {
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const QPointer<T> &v)
    : m_is_const (true), m_v (v)
  {
    mp_v = &m_v;
  }

  VariantAdaptorImpl ()
    : m_is_const (false)
  {
    mp_v = &m_v;
  }

  virtual ~VariantAdaptorImpl ()
  {
    //  .. nothing yet ..
  }

  virtual tl::Variant var () const
  {
    return tl::Variant::make_variant_ref (mp_v->get ());
  }

  virtual void set (const tl::Variant &v, tl::Heap & /*heap*/)
  {
    if (m_is_const) {
      //  .. can't change
    } else if (v.is_nil ()) {
      mp_v->clear ();
    } else if (v.is_user<T> ()) {
      if (v.user_is_ref ()) {
        *mp_v = (T *) v.to_user ();
      } else {
        //  basically should not happen as the QPointer cannot hold anything other than a reference
        //  (a tl::Variant terminology)
        tl_assert (false);
      }
    } else {
      //  basically should not happen as the QPointer cannot hold anything other than a user object
      //  (a tl::Variant terminology)
      tl_assert (false);
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    VariantAdaptorImpl<QPointer<T>> *v = dynamic_cast<VariantAdaptorImpl<QPointer<T>> *>(target);
    if (v) {
      if (mp_v->isNull ()) {
        v->mp_v->clear ();
      } else {
        *v->mp_v = QPointer<T> (*mp_v);
      }
    } else {
      VariantAdaptor::copy_to (target, heap);
    }
  }

private:
  QPointer<T> *mp_v;
  bool m_is_const;
  QPointer<T> m_v;
};

#endif

#endif

/**
 *  @brief Specialization for tl::Variant
 */
template <>
class GSI_PUBLIC VariantAdaptorImpl<tl::Variant>
  : public VariantAdaptor
{
public:
  VariantAdaptorImpl (tl::Variant *v) 
    : mp_v (v), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const tl::Variant *v) 
    : mp_v (const_cast<tl::Variant *> (v)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const tl::Variant &v) 
    : m_is_const (true), m_v (v)
  { 
    mp_v = &m_v; 
  }

  VariantAdaptorImpl () 
    : m_is_const (false)
  { 
    mp_v = &m_v; 
  }

  virtual ~VariantAdaptorImpl () 
  { 
    //  .. nothing yet ..
  }

  virtual tl::Variant var () const 
  { 
    return *mp_v;
  }

  const tl::Variant &var_ref () const 
  { 
    return *mp_v;
  }

  tl::Variant &var_ref_nc ()
  {
    return *mp_v;
  }

  virtual void set (const tl::Variant &v, tl::Heap & /*heap*/)
  {
    if (! m_is_const) {
      *mp_v = v;
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    VariantAdaptorImpl<tl::Variant> *v = dynamic_cast<VariantAdaptorImpl<tl::Variant> *>(target);
    if (v) {
      *v->mp_v = var_ref ();
    } else {
      VariantAdaptor::copy_to (target, heap);
    }
  }
   
private:
  tl::Variant *mp_v;
  bool m_is_const;
  tl::Variant m_v;
};

#if __cplusplus >= 201703L

/**
 *  @brief Specialization for std::optional
 */
template <typename T>
class GSI_PUBLIC_TEMPLATE VariantAdaptorImpl<std::optional<T> >
  : public VariantAdaptor
{
public:
  VariantAdaptorImpl (std::optional<T> *v)
    : mp_v (v), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const std::optional<T> *v)
    : mp_v (const_cast<std::optional<T> *> (v)), m_is_const (true)
  {
    //  .. nothing yet ..
  }

  VariantAdaptorImpl (const std::optional<T> &v)
    : m_is_const (true), m_v (v)
  {
    mp_v = &m_v;
  }

  VariantAdaptorImpl ()
    : m_is_const (false)
  {
    mp_v = &m_v;
  }

  virtual ~VariantAdaptorImpl ()
  {
    //  .. nothing yet ..
  }

  virtual tl::Variant var () const
  {
    return *mp_v ? tl::Variant (mp_v->value ()) : tl::Variant ();
  }

  virtual void set (const tl::Variant &v, tl::Heap & /*heap*/)
  {
    if (m_is_const) {
      //  .. can't change
    } else if (v.is_nil ()) {
      mp_v->reset ();
    } else if (v.is_user<T> ()) {
      *mp_v = v.to_user<T> ();
    } else {
      *mp_v = v.to<T> ();
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    VariantAdaptorImpl<std::optional<T>> *v = dynamic_cast<VariantAdaptorImpl<std::optional<T>> *>(target);
    if (v) {
      if (*mp_v) {
        v->mp_v->reset ();
      } else {
        *v->mp_v = mp_v->value ();
      }
    } else {
      VariantAdaptor::copy_to (target, heap);
    }
  }

private:
  std::optional<T> *mp_v;
  bool m_is_const;
  std::optional<T> m_v;
};

#endif

// ------------------------------------------------------------
//  Vector adaptor framework

/**
 *  @brief A generalization to the vector (array) iterator
 *  This is the base class for the actual implementation
 */
class GSI_PUBLIC VectorAdaptorIterator
{
public:
  /**
   *  @brief Default constructor
   */
  VectorAdaptorIterator () { }

  /**
   *  @brief Destructor
   */
  virtual ~VectorAdaptorIterator () { }

  /**
   *  @brief Gets the currently pointed member
   *  The member is written to the serial buffer.
   *  The buffer is not cleared before.
   */
  virtual void get (SerialArgs &w, tl::Heap &) const = 0;

  /**
   *  @brief Returns true, if the iterator is at the end of the sequence
   */
  virtual bool at_end () const = 0;

  /**
   *  @brief Increments the iterator
   */
  virtual void inc () = 0;
};

/**
 *  @brief A generic adaptor for vectors (arrays in general)
 *  This is the base class for implementing generic access to vectors or other linear containers.
 */
class GSI_PUBLIC VectorAdaptor
  : public AdaptorBase
{
public:
  /**
   *  @brief Default constructor
   */
  VectorAdaptor () { }

  /**
   *  @brief Destructor
   */
  virtual ~VectorAdaptor () { }

  /**
   *  @brief Returns the size of the array
   */
  virtual size_t size () const = 0;

  /**
   *  @brief Creates a generic iterator object
   *  The object must be destroyed by the caller
   */
  virtual VectorAdaptorIterator *create_iterator () const = 0;

  /**
   *  @brief Adds a new member to the container
   *  The member is expected as a serialized object on the SerialArgs buffer.
   *  The heap must be provided to account for cases where temporary objects need to be created
   *  upon reading (for example a vector<vector *>).
   */
  virtual void push (SerialArgs &r, tl::Heap &heap) = 0;

  /**
   *  @brief Clears the vector
   */
  virtual void clear () = 0;

  /**
   *  @brief Gets the size the serial buffer requires to represent a value
   */
  virtual size_t serial_size () const = 0;

  /**
   *  @brief Implementation of copy_to
   */
  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    VectorAdaptor *v = dynamic_cast<VectorAdaptor *>(target);
    tl_assert (v);

    v->clear ();
    gsi::SerialArgs rr (serial_size ());
    tl_assert (v->serial_size () == serial_size ());

    std::unique_ptr<VectorAdaptorIterator> i (create_iterator ());
    while (! i->at_end ()) {
      rr.reset ();
      i->get (rr, heap);
      v->push (rr, heap);
      i->inc ();
    }
  }
};

/**
 *  @brief Implementation of the generic iterator adaptor for a specific container
 */
template <class Cont>
class GSI_PUBLIC_TEMPLATE VectorAdaptorIteratorImpl
  : public VectorAdaptorIterator
{
public:
  typedef typename Cont::value_type value_type;

  VectorAdaptorIteratorImpl (const Cont &v)
    : m_b (v.begin ()), m_e (v.end ())
  {
  }

  void get (SerialArgs &ww, tl::Heap &) const 
  {
    ww.write<value_type> (*m_b);
  }

  bool at_end () const 
  {
    return m_b == m_e;
  }

  void inc () 
  {
    ++m_b;
  }

private:
  typename Cont::const_iterator m_b, m_e;
};

template <class X>
void push_vector (std::vector<X> &v, const X &x)
{
  v.push_back (x);
}

template <class X>
void push_vector (std::list<X> &v, const X &x)
{
  v.push_back (x);
}

template <class X>
void push_vector (std::set<X> &v, const X &x)
{
  v.insert (x);
}

#if defined(HAVE_QT)

#if QT_VERSION < 0x60000
template <class X>
void push_vector (QVector<X> &v, const X &x)
{
  v.push_back (x);
}
#endif

inline void push_vector (QStringList &v, const QString &x)
{
  v.push_back (x);
}

template <class X>
void push_vector (QList<X> &v, const X &x)
{
  v.push_back (x);
}

template <class X>
void push_vector (QSet<X> &v, const X &x)
{
  v.insert (x);
}

#endif

/**
 *  @brief Implementation of the generic adaptor for a specific container
 */
template <class Cont>
class GSI_PUBLIC_TEMPLATE VectorAdaptorImpl
  : public VectorAdaptor
{
public:
  typedef typename Cont::value_type value_type;

  VectorAdaptorImpl (Cont *v) 
    : mp_v (v), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  VectorAdaptorImpl (const Cont *v) 
    : mp_v (const_cast<Cont *> (v)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  VectorAdaptorImpl () 
    : m_is_const (false)
  { 
    mp_v = &m_v; 
  }

  VectorAdaptorImpl (const Cont &v) 
    : m_is_const (false), m_v (v)
  { 
    mp_v = &m_v; 
  }

  virtual ~VectorAdaptorImpl () 
  { 
    //  .. nothing yet ..
  }

  virtual size_t size () const 
  { 
    return mp_v->size (); 
  }

  virtual VectorAdaptorIterator *create_iterator () const 
  {
    return new VectorAdaptorIteratorImpl<Cont> (*mp_v);
  }

  virtual void push (SerialArgs &r, tl::Heap &heap) 
  {
    if (! m_is_const) {
      push_vector<value_type> (*mp_v, r.read<value_type> (heap));
    }
  }

  virtual void clear () 
  {
    if (! m_is_const) {
      mp_v->clear ();
    }
  }

  virtual size_t serial_size () const 
  {
    return gsi::type_traits<value_type>::serial_size ();
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    //  simplify copies to identical kinds
    VectorAdaptorImpl<Cont> *t = dynamic_cast<VectorAdaptorImpl<Cont> *> (target);
    if (t) {
      if (! t->m_is_const) {
        *t->mp_v = *mp_v;
      }
    } else {
      VectorAdaptor::copy_to (target, heap);
    }
  }

private:
  Cont *mp_v;
  bool m_is_const;
  Cont m_v;
};

// ------------------------------------------------------------
//  Map adaptor framework

/**
 *  @brief A generalization to the map iterator
 *  This is the base class for the actual implementation
 */
class GSI_PUBLIC MapAdaptorIterator
{
public:
  /**
   *  @brief Default constructor
   */
  MapAdaptorIterator () { }

  /**
   *  @brief Destructor
   */
  virtual ~MapAdaptorIterator () { }

  /**
   *  @brief Gets the currently pointed key and value
   *  The key and value is written to the serial buffer in that order.
   *  The buffer is not cleared before.
   */
  virtual void get (SerialArgs &w, tl::Heap &) const = 0;

  /**
   *  @brief Returns true, if the iterator is at the end of the sequence
   */
  virtual bool at_end () const = 0;

  /**
   *  @brief Increments the iterator
   */
  virtual void inc () = 0;
};

/**
 *  @brief A generic adaptor for maps (associative containers in general)
 *  This is the base class for implementing generic access to map or other associative containers.
 */
class GSI_PUBLIC MapAdaptor
  : public AdaptorBase
{
public:
  /**
   *  @brief Default constructor
   */
  MapAdaptor () { }

  /**
   *  @brief Destructor
   */
  virtual ~MapAdaptor () { }

  /**
   *  @brief Returns the size of the map
   */
  virtual size_t size () const = 0;

  /**
   *  @brief Clears the map
   */
  virtual void clear () = 0;

  /**
   *  @brief Gets the size of the serial buffer for key and value
   */
  virtual size_t serial_size () const = 0;

  /**
   *  @brief Creates a generic iterator object
   *  The object must be destroyed by the caller
   */
  virtual MapAdaptorIterator *create_iterator () const = 0;

  /**
   *  @brief Adds a new member to the map
   *  The key and value is expected as a serialized object on the SerialArgs buffers.
   *  The heap must be provided to account for cases where temporary objects need to be created
   *  upon reading (for example a map<int, vector *>).
   */
  virtual void insert (SerialArgs &rr, tl::Heap &heap) = 0;

  /**
   *  @brief Copies the content of this to the target
   */
  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    MapAdaptor *v = dynamic_cast<MapAdaptor *>(target);
    tl_assert (v);

    v->clear ();
    gsi::SerialArgs rr (serial_size ());
    tl_assert (v->serial_size () == serial_size ());

    std::unique_ptr<MapAdaptorIterator> i (create_iterator ());
    while (! i->at_end ()) {
      rr.reset ();
      i->get (rr, heap);
      v->insert (rr, heap);
      i->inc ();
    }
  }
};

/**
 *  @brief Abstract accessor for maps (STL type)
 *  This template provides abstract insert and getters.
 */
template <class Cont>
struct map_access 
{
  typedef typename Cont::key_type key_type;
  typedef typename Cont::mapped_type value_type;
  typedef typename Cont::const_iterator const_iterator;

  static void insert (Cont &c, const key_type &k, const value_type &v)
  {
    c.insert (std::make_pair (k, v));
  }

  static const key_type &get_key (const_iterator i)
  {
    return i->first;
  }

  static const value_type &get_value (const_iterator i)
  {
    return i->second;
  }
};

#if defined(HAVE_QT)

/**
 *  @brief Specialization for QMap
 */
template <class X, class Y>
struct map_access<QMap<X, Y> >
{
  typedef QMap<X, Y> cont;
  typedef typename cont::key_type key_type;
  typedef typename cont::mapped_type value_type;
  typedef typename cont::const_iterator const_iterator;

  static void insert (cont &c, const key_type &k, const value_type &v)
  {
    c.insert (k, v);
  }

  static const key_type &get_key (const_iterator i)
  {
    return i.key ();
  }

  static const value_type &get_value (const_iterator i)
  {
    return i.value ();
  }
};

/**
 *  @brief Specialization for QHash
 */
template <class X, class Y>
struct map_access<QHash<X, Y> >
{
  typedef QHash<X, Y> cont;
  typedef typename cont::key_type key_type;
  typedef typename cont::mapped_type value_type;
  typedef typename cont::const_iterator const_iterator;

  static void insert (cont &c, const key_type &k, const value_type &v)
  {
    c.insert (k, v);
  }

  static const key_type &get_key (const_iterator i)
  {
    return i.key ();
  }

  static const value_type &get_value (const_iterator i)
  {
    return i.value ();
  }
};

#endif

/**
 *  @brief Implementation of the generic iterator adaptor for a specific container
 */
template <class Cont>
class GSI_PUBLIC_TEMPLATE MapAdaptorIteratorImpl
  : public MapAdaptorIterator
{
public:
  typedef typename Cont::key_type key_type;
  typedef typename Cont::mapped_type value_type;

  MapAdaptorIteratorImpl (const Cont &v)
    : m_b (v.begin ()), m_e (v.end ())
  {
  }

  void get (SerialArgs &ww, tl::Heap &) const 
  {
    ww.write<key_type> (map_access<Cont>::get_key (m_b));
    ww.write<value_type> (map_access<Cont>::get_value (m_b));
  }

  bool at_end () const 
  {
    return m_b == m_e;
  }

  void inc () 
  {
    ++m_b;
  }

private:
  typename Cont::const_iterator m_b, m_e;
};

/**
 *  @brief Implementation of the generic adaptor for a specific container
 */
template <class Cont>
class GSI_PUBLIC_TEMPLATE MapAdaptorImpl
  : public MapAdaptor
{
public:
  typedef typename Cont::key_type key_type;
  typedef typename Cont::mapped_type value_type;

  MapAdaptorImpl (Cont *m) 
    : mp_m (m), m_is_const (false) 
  { 
    //  .. nothing yet ..
  }

  MapAdaptorImpl (const Cont *m) 
    : mp_m (const_cast<Cont *> (m)), m_is_const (true) 
  { 
    //  .. nothing yet ..
  }

  MapAdaptorImpl (const Cont &m) 
    : m_is_const (false), m_m (m)
  { 
    mp_m = &m_m; 
  }

  MapAdaptorImpl () 
    : m_is_const (false)
  { 
    mp_m = &m_m; 
  }

  virtual ~MapAdaptorImpl () 
  { 
    //  .. nothing yet ..
  }

  virtual size_t size () const 
  { 
    return mp_m->size (); 
  }

  virtual void clear () 
  {
    if (! m_is_const) {
      mp_m->clear ();
    }
  }

  virtual size_t serial_size () const
  {
    return (gsi::type_traits<key_type>::serial_size () + gsi::type_traits<value_type>::serial_size ());
  }

  virtual MapAdaptorIterator *create_iterator () const 
  {
    return new MapAdaptorIteratorImpl<Cont> (*mp_m);
  }

  virtual void insert (SerialArgs &rr, tl::Heap &heap) 
  {
    if (! m_is_const) {
      key_type x = rr.read<key_type> (heap);
      value_type y = rr.read<value_type> (heap);
      map_access<Cont>::insert (*mp_m, x, y);
    }
  }

  virtual void copy_to (AdaptorBase *target, tl::Heap &heap) const
  {
    //  simplify copies to identical kinds
    MapAdaptorImpl<Cont> *t = dynamic_cast<MapAdaptorImpl<Cont> *> (target);
    if (t) {
      if (! t->m_is_const) {
        *t->mp_m = *mp_m;
      }
    } else {
      MapAdaptor::copy_to (target, heap);
    }
  }

private:
  Cont *mp_m;
  bool m_is_const;
  Cont m_m;
};

// ------------------------------------------------------------
//  Create adaptors for various categories

template <class X, class V>
inline AdaptorBase *create_adaptor_by_category(const vector_adaptor_tag & /*tag*/, V v)
{
  return new VectorAdaptorImpl<X> (v);
}

template <class X, class V>
inline AdaptorBase *create_adaptor_by_category(const map_adaptor_tag & /*tag*/, V v)
{
  return new MapAdaptorImpl<X> (v);
}

template <class X, class V>
inline AdaptorBase *create_adaptor_by_category(const string_adaptor_tag & /*tag*/, V v)
{
  return new StringAdaptorImpl<X> (v);
}

template <class X, class V>
inline AdaptorBase *create_adaptor_by_category(const byte_array_adaptor_tag & /*tag*/, V v)
{
  return new ByteArrayAdaptorImpl<X> (v);
}

template <class X, class V>
inline AdaptorBase *create_adaptor_by_category(const variant_adaptor_tag & /*tag*/, const V &v)
{
  return new VariantAdaptorImpl<X> (v);
}

template <class Tag, class X>
inline AdaptorBase *create_adaptor2 (const adaptor_direct_tag & /*tag*/, const Tag &tag2, const X &v)
{
  return create_adaptor_by_category<X> (tag2, v);
}

template <class Tag, class X>
inline AdaptorBase *create_adaptor2 (const adaptor_cref_tag & /*tag*/, const Tag &tag2, const X &v)
{
  return create_adaptor_by_category<X> (tag2, &v);
}

template <class Tag, class X>
inline AdaptorBase *create_adaptor2 (const adaptor_ref_tag & /*tag*/, const Tag &tag2, X &v)
{
  return create_adaptor_by_category<X> (tag2, &v);
}

template <class Tag, class X>
inline AdaptorBase *create_adaptor2 (const adaptor_cptr_tag & /*tag*/, const Tag &tag2, const X *v)
{
  return create_adaptor_by_category<X> (tag2, v);
}

template <class Tag, class X>
inline AdaptorBase *create_adaptor2 (const adaptor_ptr_tag & /*tag*/, const Tag &tag2, X *v)
{
  return create_adaptor_by_category<X> (tag2, v);
}

template <class Tag, class X> 
inline AdaptorBase *adaptor_factory<Tag, X>::get (const X &v)
{
  Tag tag;
  return create_adaptor2 (tag, tag, v);
}

// ------------------------------------------------------------
//  A generic function to transfer adaptor-managed information into a C++ object

template <class X>
inline void copy_to (AdaptorBase &a, X &x, tl::Heap &heap)
{
  std::unique_ptr<AdaptorBase> t (adaptor_factory<typename gsi::type_traits<X &>::tag, X &>::get (x));
  a.copy_to (t.get (), heap);
}

template <class X>
inline void tie_copies (AdaptorBase *a, X &x, tl::Heap &heap)
{
  AdaptorBase *t (adaptor_factory<typename gsi::type_traits<X &>::tag, X &>::get (x));
  a->tie_copies (t, heap);
}

}

#endif

