
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


#ifndef _HDR_gsiQt
#define _HDR_gsiQt

#include "gsiDecl.h"
#include "gsiEnums.h"
#include "gsiSignals.h"
#include "gsiQtBasicCommon.h"
#include "tlString.h"
#include "tlException.h"
#include "tlTypeTraits.h"
#include "tlHeap.h"

#include <QList>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QObject>
#include <QProcess>
#include <QHash>
#include <QMap>
#include <QSet>

#if QT_VERSION >= 0x050000
//  contributes the WIN specific typedefs
#  include <QWindow>
#endif

class QGraphicsItem;
class QGraphicsObject;

namespace qt_gsi
{

GSI_QTBASIC_PUBLIC gsi::ObjectBase *get_watcher_object (QObject *qobject, bool required);

/**
 *  @brief A Qt class declaration
 *
 *  The purpose of this class declaration is to modify the behavior for the "native" classes.
 *  It will register a helper object along with the native object that will issue destruction events
 *  when the parent object is destroyed through the gsi::ObjectBase interface.
 *
 *  This template shall be used instead of gsi::Class for Qt native classes.
 */
template <class X>
class QtNativeClass
  : public gsi::Class<X>
{
public:
  QtNativeClass (const std::string &module, const std::string &name, const gsi::Methods &mm, const std::string &doc = std::string ())
    : gsi::Class<X> (module, name, mm, doc)
  {
  }

  template <class B>
  QtNativeClass (const gsi::Class<B> &base, const std::string &module, const std::string &name, const gsi::Methods &mm, const std::string &doc = std::string ())
    : gsi::Class<X> (base, module, name, mm, doc)
  {
  }

  QtNativeClass (const std::string &module, const std::string &name, const std::string &doc = std::string ())
    : gsi::Class<X> (module, name, doc)
  {
  }

  template <class B>
  QtNativeClass (const gsi::Class<B> &base, const std::string &module, const std::string &name, const std::string &doc = std::string ())
    : gsi::Class<X> (base, module, name, doc)
  {
  }

  bool is_managed () const
  {
    return true;
  }

  gsi::ObjectBase *gsi_object (void *p, bool required) const
  {
    return get_watcher_object ((QObject *) p, required);
  }
};

/**
 *  @brief A generic way to implement a method binding
 *
 *  Using that way saves compile time and memory
 */
class GSI_QTBASIC_PUBLIC GenericMethod : public gsi::MethodBase
{
public:
  GenericMethod (const char *name, const char *doc, bool is_const, void (*init_func)(GenericMethod *), void (*call_func)(const GenericMethod *, void *, gsi::SerialArgs &args, gsi::SerialArgs &ret))
    : gsi::MethodBase (name, doc, is_const, false), mp_init_func (init_func), mp_call_func (call_func), mp_set_callback_func (0)
  {
  }

  GenericMethod (const char *name, const char *doc, bool is_const, void (*init_func)(GenericMethod *), void (*call_func)(const GenericMethod *, void *, gsi::SerialArgs &args, gsi::SerialArgs &ret), void (*set_callback_func) (void *v, const gsi::Callback &cb))
    : gsi::MethodBase (name, doc, is_const, false), mp_init_func (init_func), mp_call_func (call_func), mp_set_callback_func (set_callback_func)
  {
  }
  
  virtual void initialize ()
  {
    clear ();
    (*mp_init_func) (this);
  }

  virtual gsi::MethodBase *clone () const
  {
    return new GenericMethod (*this);
  }

  virtual void call (void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) const 
  {
    (*mp_call_func) (this, cls, args, ret);
  }

  virtual bool is_callback () const
  {
    return mp_set_callback_func != 0;
  }

  virtual void set_callback (void *v, const gsi::Callback &cb) const
  {
    if (mp_set_callback_func) {
      (*mp_set_callback_func) (v, cb);
    }
  }

private:
  void (*mp_init_func) (GenericMethod *self);
  void (*mp_call_func) (const GenericMethod *self, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret);
  void (*mp_set_callback_func) (void *cls, const gsi::Callback &cb);
};

/**
 *   @brief A generic way to implement a static method binding
 *
 *   Using that way saves compile time and memory
 */
class GSI_QTBASIC_PUBLIC GenericStaticMethod : public gsi::StaticMethodBase
{
public:
  GenericStaticMethod (const char *name, const char *doc, void (*init_func)(GenericStaticMethod *), void (*call_func)(const GenericStaticMethod *self, gsi::SerialArgs &args, gsi::SerialArgs &ret))
    : gsi::StaticMethodBase (name, doc), mp_init_func (init_func), mp_call_func (call_func)
  {
  }

  virtual void initialize ()
  {
    clear ();
    (*mp_init_func) (this);
  }

  virtual gsi::MethodBase *clone () const
  {
    return new GenericStaticMethod (*this);
  }

  virtual void call (void *, gsi::SerialArgs &args, gsi::SerialArgs &ret) const 
  {
    (*mp_call_func) (this, args, ret);
  }

private:
  void (*mp_init_func) (GenericStaticMethod *self);
  void (*mp_call_func) (const GenericStaticMethod *self, gsi::SerialArgs &args, gsi::SerialArgs &ret);
};

template <class QT> struct Converter;

template <class QT>
class QtToCppAdaptor
{
public:
  typedef Converter<QT> converter_type;
  typedef typename converter_type::target_type target_type;
  typedef typename converter_type::source_type source_type;

  QtToCppAdaptor ()
    : mp_ref (0)
  {
    //  .. nothing yet ..
  }

  QtToCppAdaptor (const QtToCppAdaptor<QT> &d)
    : m_qt (d.m_qt), mp_ref (d.mp_ref)
  {
    //  .. nothing yet ..
  }

  QtToCppAdaptor (const target_type &t)
    : m_qt (converter_type::toq (t)), mp_ref (0)
  {
    //  .. nothing yet ..
  }

  QtToCppAdaptor (const target_type *t)
    : m_qt (converter_type::toq (*t)), mp_ref (0)
  {
    //  .. nothing yet ..
  }

  QtToCppAdaptor (target_type &t)
    : m_qt (converter_type::toq (t)), mp_ref (&t)
  {
    //  .. nothing yet ..
  }

  QtToCppAdaptor (target_type *t)
    : m_qt (converter_type::toq (*t)), mp_ref (t)
  {
    //  .. nothing yet ..
  }

  ~QtToCppAdaptor ()
  {
    if (mp_ref) {
      *mp_ref = converter_type::toc (m_qt);
    }
  }

  source_type *ptr ()
  {
    return &m_qt;
  }

  source_type &ref ()
  {
    return m_qt;
  }

  const source_type *cptr () const
  {
    return &m_qt;
  }

  const source_type &cref () const
  {
    return m_qt;
  }

private:
  source_type m_qt;
  target_type *mp_ref;
};

template <class QT>
class CppToQtReadAdaptor
{
public:
  typedef Converter<QT> converter_type;
  typedef typename converter_type::target_type target_type;
  typedef typename converter_type::source_type source_type;

  CppToQtReadAdaptor ()
    : mp_ref (0)
  {
    //  .. nothing yet ..
  }

  CppToQtReadAdaptor (tl::Heap &heap, const source_type &qt)
  {
    mp_ref = new target_type (converter_type::toc (qt));
    heap.push (mp_ref);
  }

  operator const target_type *() const
  {
    return mp_ref;
  }

  operator const target_type &() const
  {
    return *mp_ref;
  }

private:
  const target_type *mp_ref;

  CppToQtReadAdaptor (const CppToQtReadAdaptor<QT> &d);
  CppToQtReadAdaptor &operator= (const CppToQtReadAdaptor<QT> &d);
};

template <class QT>
class CppToQtAdaptor
{
public:
  typedef Converter<QT> converter_type;
  typedef typename converter_type::target_type target_type;
  typedef typename converter_type::source_type source_type;

  CppToQtAdaptor ()
    : mp_ref (0)
  {
    //  .. nothing yet ..
  }

  CppToQtAdaptor (const CppToQtAdaptor<QT> &d)
    : m_t (d.m_t), mp_ref (d.mp_ref)
  {
    //  .. nothing yet ..
  }

  CppToQtAdaptor (const source_type &qt)
    : m_t (converter_type::toc (qt)), mp_ref (0)
  {
    //  .. nothing yet ..
  }

  CppToQtAdaptor (source_type &qt)
    : m_t (converter_type::toc (qt)), mp_ref (&qt)
  {
    //  .. nothing yet ..
  }

  ~CppToQtAdaptor ()
  {
    if (mp_ref) {
      *mp_ref = converter_type::toq (m_t);
    }
  }

  operator target_type *() 
  {
    return &m_t;
  }

  operator target_type &() 
  {
    return m_t;
  }

  operator const target_type *() const
  {
    return &m_t;
  }

  operator const target_type &() const
  {
    return m_t;
  }

private:
  target_type m_t;
  source_type *mp_ref;
};

template <class T>
struct Converter
{
  typedef T source_type;
  typedef T target_type;
  static const T &toq (const T &v) { return v; }
  static const T &toc (const T &v) { return v; }
};

template <>
struct Converter<Qt::HANDLE>
{
public:
  typedef Qt::HANDLE source_type;
  typedef size_t target_type;
  static source_type toq (target_type c) { return source_type (c); }
  static target_type toc (source_type qc) { return target_type (qc); }
};

#if QT_VERSION < 0x060000
template <>
struct Converter<Q_PID>
{
public:
  typedef Q_PID source_type;
  typedef size_t target_type;
  static source_type toq (target_type c) { return source_type (c); }
  static target_type toc (source_type qc) { return target_type (qc); }
};
#endif

template <>
struct Converter<QChar>
{
public:
  typedef QChar source_type;
  typedef unsigned int target_type;
  static QChar toq (unsigned int c) { return QChar (c); }
  static unsigned int toc (QChar qc) { return qc.unicode (); }
};

#if QT_VERSION < 0x050000
template <>
struct Converter<QBool>
{
public:
  typedef QBool source_type;
  typedef bool target_type;
  static QBool toq (bool b) { return QBool (b); }
  static bool toc (QBool qb) { return qb; }
};
#endif

#ifdef _WIN32

template <>
struct Converter<WId>
{
public:
  typedef WId source_type;
  typedef size_t target_type;
  static source_type toq (target_type c) { return source_type (c); }
  static target_type toc (source_type qc) { return target_type (qc); }
};

template <>
struct Converter<HCURSOR>
{
public:
  typedef HCURSOR source_type;
  typedef size_t target_type;
  static source_type toq (target_type c) { return source_type (c); }
  static target_type toc (source_type qc) { return target_type (qc); }
};

template <>
struct Converter<HFONT>
{
public:
  typedef HFONT source_type;
  typedef size_t target_type;
  static source_type toq (target_type c) { return source_type (c); }
  static target_type toc (source_type qc) { return target_type (qc); }
};

#endif

class GSI_QTBASIC_PUBLIC AbstractMethodCalledException
  : public tl::Exception
{
public:
  AbstractMethodCalledException (const char *method_name);
};

class GSI_QTBASIC_PUBLIC QtObjectBase
  : public gsi::ObjectBase
{
public:
  void init (void *) 
  {
    //  fallback case: no particular initialization 
  }

  void init (QObject *object);
  void init (QGraphicsItem *object);
  void init (QGraphicsObject *object);
};

/**
 *  @brief An implementation helper for the "keep arg" feature
 *  This helper will call keep on the object or objects, hence passing
 *  ownership to the callee.
 */
template <class T>
inline void qt_keep (T *obj)
{
  QtObjectBase *qt_obj = dynamic_cast<QtObjectBase *>(obj);
  if (qt_obj) {
    qt_obj->keep ();
  }
}

/**
 *  @brief An implementation helper for the "keep arg" feature
 *  This helper will call release on the object, hence returning the
 *  ownership to the script framework.
 */
template <class T>
inline void qt_release (T *obj)
{
  QtObjectBase *qt_obj = dynamic_cast<QtObjectBase *>(obj);
  if (qt_obj) {
    qt_obj->release ();
  }
}

template <class T>
inline void qt_keep (const QList<T *> &list)
{
  for (typename QList<T *>::const_iterator l = list.begin (); l != list.end (); ++l) {
    qt_keep (*l);
  }
}

template <class T>
inline void qt_keep (const std::vector<T *> &list)
{
  for (typename std::vector<T *>::const_iterator l = list.begin (); l != list.end (); ++l) {
    qt_keep (*l);
  }
}

/**
 *  @brief A helper to implement QPair bindings
 */
template <class A, class B>
struct pair_decl
{
  static typename qt_gsi::Converter<A>::target_type pair_first(const QPair<A, B> *pair)
  {
    return qt_gsi::Converter<A>::toc (pair->first);
  }

  static typename qt_gsi::Converter<B>::target_type pair_second(const QPair<A, B> *pair)
  {
    return qt_gsi::Converter<B>::toc (pair->second);
  }

  static void pair_set_first(QPair<A, B> *pair, const typename qt_gsi::Converter<A>::target_type &s)
  {
    pair->first = qt_gsi::Converter<A>::toq (s);
  }

  static void pair_set_second(QPair<A, B> *pair, const typename qt_gsi::Converter<B>::target_type &s)
  {
    pair->second = qt_gsi::Converter<B>::toq (s);
  }

  static bool pair_equal(const QPair<A, B> *pair, const QPair<A, B> &other)
  {
    return *pair == other;
  }

  /* Not available for all types: (TODO: separate pair declaration for those types which do)
  static bool pair_less(const QPair<A, B> *pair, const QPair<A, B> &other)
  {
    return *pair < other;
  }
  */

  static QPair<A, B> *pair_default_ctor()
  {
    return new QPair<A, B>();
  }

  static QPair<A, B> *pair_ctor(const typename qt_gsi::Converter<A>::target_type &first, const typename qt_gsi::Converter<B>::target_type &second)
  {
    return new QPair<A, B>(qt_gsi::Converter<A>::toq (first), qt_gsi::Converter<B>::toq (second));
  }

  static gsi::Methods methods ()
  {
    return
      gsi::constructor("new", &pair_default_ctor, "@brief Creates a new pair") +
      gsi::constructor("new", &pair_ctor, gsi::arg ("first"), gsi::arg ("second"), "@brief Creates a new pair from the given arguments") +
      gsi::method_ext("first", &pair_first, "@brief Returns the first element of the pair") +
      gsi::method_ext("first=", &pair_set_first, gsi::arg ("first"), "@brief Sets the first element of the pair") +
      gsi::method_ext("second", &pair_second, "@brief Returns the second element of the pair") +
      gsi::method_ext("second=", &pair_set_second, gsi::arg ("second"), "@brief Sets the second element of the pair") +
      gsi::method_ext("==", &pair_equal, gsi::arg ("other"), "@brief Returns true if self is equal to the other pair")
      // not available for all types: (TODO: separate pair declaration for those types which do)
      // gsi::method_ext("<", &pair_less, gsi::arg ("other"), "@brief Returns true if self is less than the other pair")
    ;
  }
};

//  Using this macro on a variable will supress the "unused variable" or "unused argument"
//  warning:
#define __SUPPRESS_UNUSED_WARNING(x) (void)(x)

//  HACK: the Qt binding code takes __null instead of NULL, but
//  MS defines it as empty in sal.h ... better __null was NULL again.
#if defined(__null)
#  undef __null
#  define __null 0
#endif

}
  
#endif

