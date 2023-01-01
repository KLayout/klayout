
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


#ifndef _HDR_gsiSignals
#define _HDR_gsiSignals

#include "tlTypeTraits.h"
#include "tlObject.h"
#include "tlEvents.h"
#include "gsiTypes.h"
#include "gsiIterators.h"
#include "gsiCallback.h"
#include "gsiMethods.h"
#include "gsiSerialisation.h"

//  On MinGW, the "access" macro will interfere with QMetaMethod's access method
#if defined(access)
#  undef access
#endif

#if defined(HAVE_QT)
#  include <QMetaMethod>
#endif

/**
 *  @brief A signal exposure framework
 *
 *  Signals are a concept by which a piece of client code (script)
 *  can be bound to an event triggered by C++ code.
 *
 *  This framework provides two basic interfaces: one for
 *  Qt signals and another one for tl::Event signals.
 *
 *  1.) Binding of tl::Event signals
 *
 *  In C++:
 *  @code{.cpp}
 *  //  tl::Event signals
 *  class X
 *  {
 *  public:
 *    void s_trigger(int n) { s(n); }
 *
 *  public:
 *    //  needs to be public
 *    tl::event<int> s;
 *  };
 *
 *  gsi::ClassBase<X> cls_decl("X",
 *    gsi::event(&X::s, "s") +
 *    gsi::method(&x::s_trigger, "s_trigger")
 *  );
 *  @endcode
 *
 *  In Ruby:
 *  @code{.rb}
 *  x = X::new()
 *  x.s { puts "x.s triggered" }
 *  x.s_trigger
 *  @endcode
 *
 *  2.) Binding of Qt signals:
 *
 *  In C++:
 *  @code{.cpp}
 *  class Y
 *    : public QObject
 *  {
 *  Q_OBJECT
 *
 *  signals:
 *    void s(int n);
 *
 *  public:
 *    void s_trigger(int n) { emit s(n); }
 *  };
 *
 *  gsi::ClassBase<Y> cls_decl("Y",
 *    gsi::qt_signal<int>("s(int)", "s") +
 *    gsi::method(&Y::s_trigger, "s_trigger")
 *  );
 *  @endcode
 *
 *  In Ruby:
 *  @code{.rb}
 *  y = Y::new()
 *  y.s { puts "y.s triggered" }
 *  y.s_trigger
 *  @endcode
 */

namespace gsi
{

struct empty_list_t { };
template <class T, class H> struct type_pair_t { };

class SignalAdaptor;

/**
 *  @brief The signal handler provided by the client implementation
 *  The handler and the signal adaptor implement a double dispatch pattern:
 *  the handler implements the strategy how to call a signal on the client side.
 *  It is owned by the client. The adaptor implements the strategy how a signal
 *  is generated on the C++ side. The adaptor is owned by the handler.
 */
class GSI_PUBLIC SignalHandler
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   */
  SignalHandler () { }

  /**
   *  @brief Destructor
   */
  virtual ~SignalHandler () { }

  /**
   *  @brief Provides the implementation for the call of the signal
   *  The client-side implementation needs to reimplement this method to provide the mechanism
   *  how a signal is called. The implementation is supposed to take the arguments, call the
   *  function associated with this handler and place the return value in the ret serial buffer.
   *  @param method The method descriptor for the signal
   *  @param args The serialized arguments
   *  @param ret The return value
   */
  virtual void call (const MethodBase *method, SerialArgs &args, SerialArgs &ret) const = 0;

  /**
   *  @brief Ties the lifetime of a signal adaptor to that of the handler
   *  The system will call this method to install an adaptor with this handler.
   *  If the handler is deleted, the adaptor will be deleted too.
   */
  void set_adaptor (SignalAdaptor *adaptor)
  {
    mp_adaptor.reset (adaptor);
  }

private:
  tl::shared_ptr<SignalAdaptor> mp_adaptor;
};

/**
 *  @brief A signal descriptor
 *  This is a specialization of the method descriptor for the signal.
 */
class GSI_PUBLIC Signal
  : public MethodBase
{
public:
  /**
   *  @brief Constructor
   *  @param name The name of the signal
   *  @param doc The documentation string
   */
  Signal (const std::string &name, const std::string &doc)
    : MethodBase (name, doc, false, false)
  {
  }

  /**
   *  @brief Returns a value indicating whether this method is a signal
   */
  bool is_signal () const
  {
    return true;
  }

  /**
   *  @brief Registers a signal handler
   *  This method will register one new signal handler
   *  to the given object.
   *  The SignalHandler object is responsible for performing actions
   *  connected to the signal. The last handler's return value will
   *  be taken as the return value of the event if the event allows
   *  for a return value.
   *  The signal will *not* take ownership over the handler object.
   */
  virtual void add_handler (void *obj, SignalHandler *handler) const = 0;
};

/**
 *  @brief A base class for signal adaptors
 *  Signal adaptors provide the C++ side of the signal double dispatch scheme.
 *  They implement the way a signal is generated.
 */
class GSI_PUBLIC SignalAdaptor
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   */
  SignalAdaptor ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  virtual ~SignalAdaptor ()
  {
    //  .. nothing yet ..
  }
};

// ---------------------------------------------------------------------------------------
//  Adaptors for Qt signals

#if defined(HAVE_QT)

/**
 *  @brief A base class for Qt signals
 *  This object will act as a connector for Qt signals: when a signal is bound, this adaptor
 *  will provide a generic slot to connect the Qt signal to. The specializations will then
 *  forward this signal to the signal handler which itself is responsible for calling the
 *  client-side code.
 */
class GSI_PUBLIC QtSignalAdaptorBase
  : public QObject, public SignalAdaptor
{
Q_OBJECT

public:
  QtSignalAdaptorBase () { }

public slots:
  void generic ()
  {
    tl_assert (false);
  }
};

template <class T>
class QtSignalAdaptor;

/**
 *  @brief A specialization for signals without arguments
 *  In addition, this object provides the generic slot implementation which
 *  forwards the call to the client.
 */
template <>
class QtSignalAdaptor<empty_list_t>
  : public QtSignalAdaptorBase
{
public:
  /**
   *  @brief Constructor
   *  @param method The method descriptor
   *  @param handler The signal handler to call
   */
  QtSignalAdaptor (const MethodBase *method, const SignalHandler *handler)
    : mp_method (method), mp_handler (handler)
  {
    //  .. nothing yet ..
  }

  virtual ~QtSignalAdaptor ()
  {
    //  .. nothing yet ..
  }

protected:
  /**
   *  @brief Provides a generic implementation of the slot
   *  This implementation basically hijacks the slot of QtSignalAdaptorBase. It won't call
   *  the real base class but QObject and handle every method not handled by QObject.
   */
  virtual int qt_metacall (QMetaObject::Call c, int id, void **a)
  {
    id = QObject::qt_metacall (c, id, a);
    if (id < 0) {
      return id;
    }

    //  we consume every invoked method here. This is some kind of dirty trick to
    //  override the moc-generated qt_metacall implementation by our custom one
    //  in the base class.
    if (c == QMetaObject::InvokeMetaMethod && mp_handler) {
       SerialArgs args (mp_method->argsize ());
       write_args (args, a);
       //  .. further
       SerialArgs ret (mp_method->retsize ());
       mp_handler->call (mp_method, args, ret);
    }

    return -1;
  }

protected:
  virtual void write_args (SerialArgs &, void **)
  {
    //  .. nothing for empty list ..
  }

  inline void write_args_non_virtual (SerialArgs &, void **)
  {
    //  .. nothing for empty list ..
  }

private:
  const MethodBase *mp_method;
  const SignalHandler *mp_handler;
};

/**
 *  @brief A specialization for signal with arguments
 *  This specialization is based on the typelist pattern and provides
 *  an implementation for the first argument - further arguments are
 *  provided by the base class.
 */
template <class H, class T>
class QtSignalAdaptor<type_pair_t<H, T> >
  : public QtSignalAdaptor<T>
{
public:
  QtSignalAdaptor (const MethodBase *method, SignalHandler *handler)
    : QtSignalAdaptor<T> (method, handler)
  {
    // ...
  }

protected:
  /**
   *  @brief A helper class to write an argument of type X
   *  This helper class and it's specializations for references
   *  is required to avoid "taking pointer to reference" compiler
   *  errors.
   */
  template <class X>
  struct writer
  {
    void operator() (SerialArgs &args, void *a)
    {
      args.write<X> (*reinterpret_cast<X *> (a));
    }
  };

  /**
   *  @brief A specialization of the argument writer for a const reference
   */
  template <class X>
  struct writer<const X &>
  {
    void operator() (SerialArgs &args, void *a)
    {
      args.write<const X &> (*reinterpret_cast<const X *> (a));
    }
  };

  /**
   *  @brief A specialization of the argument writer for a non-const reference
   */
  template <class X>
  struct writer<X &>
  {
    void operator() (SerialArgs &args, void *a)
    {
      args.write<X &> (*reinterpret_cast<X *> (a));
    }
  };

  /**
   *  @brief Serializes the arguments from the Qt argument stack to the argument buffer
   *  Takes one argument from the stack of arguments and writes it to the argument buffer.
   *  Then dispatches to the base class for the next arguments
   */
  virtual void write_args (SerialArgs &args, void **a)
  {
    write_args_non_virtual (args, a);
  }

  /**
   *  @brief The actual implementation of the argument serialization
   *  To enable inlining of the argument serialization, this method is provided
   *  in a non-virtual way.
   */
  inline void write_args_non_virtual (SerialArgs &args, void **a)
  {
    writer<H> () (args, *++a);
    QtSignalAdaptor<T>::write_args_non_virtual (args, a);
  }
};

template <class X>
class QtSignalImpl;

template <>
class QtSignalImpl<empty_list_t>
  : public Signal
{
public:
  QtSignalImpl (const char *signal, const std::string &name, const std::string &doc)
    : Signal (name, doc), mp_signal (signal)
  {
  }

  virtual MethodBase *clone () const
  {
    return new QtSignalImpl<empty_list_t> (*this);
  }

  void add_handler (void *obj, SignalHandler *handler) const
  {
    _add_handler<QtSignalAdaptor<empty_list_t> > (obj, handler);
  }

  void initialize ()
  {
    this->clear ();
    _initialize ();
  }

protected:
  void _initialize ()
  {
    //  .. nothing yet ..
  }

#if QT_VERSION >= 0x40800
  template <class A>
  void _add_handler (void *obj, SignalHandler *handler) const
  {
    //  NOTE: this scheme requires 4.8 at least

    QObject *qobj = (QObject *)obj;

    A *adaptor = new A (this, handler);
    //  tie the lifetime of the adaptor to that of the handler
    handler->set_adaptor (adaptor);

    //  NOTE: by connecting through QMetaMethod we force Qt into using the virtual qt_metacall
    //  rather than qt_static_metacall which we can't override ... Let's hope it stays like this ...

    QByteArray sig = QMetaObject::normalizedSignature (mp_signal);
    int sig_index = qobj->metaObject ()->indexOfMethod (sig.constData ());
    if (sig_index < 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("Not a valid signal: %1").arg (sig.constData ())));
    }

    QByteArray slot = QMetaObject::normalizedSignature ("generic()");
    int slot_index = adaptor->metaObject ()->indexOfMethod (slot.constData ());
    if (slot_index < 0) {
      //  NOTE: should not happen
      throw tl::Exception (tl::to_string (QObject::tr ("Not a valid slot: %1").arg (slot.constData ())));
    }

    QObject::connect (qobj, qobj->metaObject ()->method (sig_index), adaptor, adaptor->metaObject ()->method (slot_index));
  }
#else
  template <class A>
  void _add_handler (void * /*obj*/, SignalHandler * /*handler*/) const
  {
    throw tl::Exception (tl::to_string (QObject::tr ("Qt signal binding requires Qt version >= 4.8")));
  }
#endif

private:
  const char *mp_signal;
};

template <class H, class T>
class QtSignalImpl<type_pair_t<H, T> >
  : public QtSignalImpl<T>
{
public:
  QtSignalImpl (const char *signal, const std::string &name, const std::string &doc)
    : QtSignalImpl<T> (signal, name, doc), m_s ()
  {
  }

  template <class S>
  QtSignalImpl<T> *def_arg (const gsi::ArgSpec<S> &s)
  {
    m_s = s;
    return this;
  }

  virtual MethodBase *clone () const
  {
    return new QtSignalImpl<type_pair_t<H, T> > (*this);
  }

  void initialize ()
  {
    this->clear ();
    _initialize ();
  }

  void add_handler (void *obj, SignalHandler *handler) const
  {
    QtSignalImpl<empty_list_t>::template _add_handler<QtSignalAdaptor<type_pair_t<H, T> > > (obj, handler);
  }

protected:
  void _initialize ()
  {
    this->template add_arg<H> (m_s);
    QtSignalImpl<T>::_initialize ();
  }

private:
  gsi::ArgSpec<H> m_s;
};

/**
 *  @brief Provides the qt_signal wrapper for zero arguments
 */
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<empty_list_t> (signal, name, doc));
}

/**
 *  @brief Provides the qt_signal wrapper for one argument
 */
template <class A1>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, empty_list_t> > (signal, name, doc));
}

/**
 *  @brief Provides the qt_signal wrapper for one argument
 */
template <class A1,
          class S1>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, empty_list_t> > (signal, name, doc))->def_arg (s1));
}

//  ...
template <class A1, class A2>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, empty_list_t> > > (signal, name, doc));
}

//  ...
template <class A1, class A2,
          class S1, class S2>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, empty_list_t> > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2));
}

//  ...
template <class A1, class A2, class A3>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, empty_list_t> > > > (signal, name, doc));
}

//  ...
template <class A1, class A2, class A3,
          class S1, class S2, class S3>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, empty_list_t> > > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3));
}

//  ...
template <class A1, class A2, class A3, class A4>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, empty_list_t> > > > > (signal, name, doc));
}

//  ...
template <class A1, class A2, class A3, class A4,
          class S1, class S2, class S3, class S4>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, empty_list_t> > > > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, empty_list_t> > > > > > (signal, name, doc));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5,
          class S1, class S2, class S3, class S4, class S5>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, empty_list_t> > > > > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4)->def_arg (s5));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, empty_list_t> > > > > > > (signal, name, doc));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6,
          class S1, class S2, class S3, class S4, class S5, class S6>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, empty_list_t> > > > > > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4)->def_arg (s5)->def_arg (s6));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6, class A7>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, type_pair_t<A7, empty_list_t> > > > > > > > (signal, name, doc));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6, class A7,
          class S1, class S2, class S3, class S4, class S5, class S6, class S7>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, type_pair_t<A7, empty_list_t> > > > > > > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4)->def_arg (s5)->def_arg (s6)->def_arg (s7));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc = std::string ())
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, type_pair_t<A7, type_pair_t<A8, empty_list_t> > > > > > > > > (signal, name, doc));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8,
          class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, type_pair_t<A7, type_pair_t<A8, empty_list_t> > > > > > > > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4)->def_arg (s5)->def_arg (s6)->def_arg (s7)->def_arg (s8));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
inline Methods qt_signal (const char *signal, const std::string &name, const std::string &doc)
{
  return Methods(new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, type_pair_t<A7, type_pair_t<A8, type_pair_t<A9, empty_list_t> > > > > > > > > > (signal, name, doc));
}

//  ...
template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9,
          class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9>
inline Methods qt_signal (const char *signal, const std::string &name,
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const std::string &doc = std::string ())
{
  return Methods((new QtSignalImpl<type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, type_pair_t<A5, type_pair_t<A6, type_pair_t<A7, type_pair_t<A8, type_pair_t<A9, empty_list_t> > > > > > > > > > (signal, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4)->def_arg (s5)->def_arg (s6)->def_arg (s7)->def_arg (s8)->def_arg (s9));
}

#endif

// ---------------------------------------------------------------------------------------
//  Adaptors for tl::event

template <class T>
class EventSignalAdaptor;

/**
 *  @brief A signal adaptor for tl::event signals without arguments
 */
template <>
class EventSignalAdaptor<empty_list_t>
  : public SignalAdaptor
{
public:
  /**
   *  @brief Constructor
   *  @param method The method descriptor
   *  @param handler The signal handler to call
   */
  EventSignalAdaptor (const MethodBase *method, const SignalHandler *handler)
    : mp_method (method), mp_handler (handler)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The actual target for the signal
   */
  void event_receiver (int /*argc*/, void ** /*args*/)
  {
    if (mp_handler) {
       SerialArgs args (mp_method->argsize ());
       SerialArgs ret (mp_method->retsize ());
       mp_handler->call (mp_method, args, ret);
    }
  }

protected:
  const SignalHandler *handler () const
  {
    return mp_handler;
  }

  const MethodBase *method () const
  {
    return mp_method;
  }

  void write_args (SerialArgs &, void **)
  {
    //  .. no arguments to write ..
  }

private:
  const MethodBase *mp_method;
  const SignalHandler *mp_handler;
};

/**
 *  @brief A specialization for signal with arguments
 *  This specialization is based on the typelist pattern and provides
 *  an implementation for the first argument - further arguments are
 *  provided by the base class.
 */
template <class H, class T>
class EventSignalAdaptor<type_pair_t<H, T> >
  : public EventSignalAdaptor<T>
{
public:
  EventSignalAdaptor (const MethodBase *method, SignalHandler *handler)
    : EventSignalAdaptor<T> (method, handler)
  {
    // ...
  }

  /**
   *  @brief The actual target for the signal
   */
  void event_receiver (int /*argc*/, void **a)
  {
    if (this->handler ()) {
       SerialArgs args (this->method ()->argsize ());
       write_args (args, a);
       //  .. further
       SerialArgs ret (this->method ()->retsize ());
       this->handler ()->call (this->method (), args, ret);
    }
  }

protected:
  /**
   *  @brief A helper class to write an argument of type X
   *  This helper class and it's specializations for references
   *  is required to avoid "taking pointer to reference" compiler
   *  errors.
   */
  template <class X>
  struct writer
  {
    void operator() (SerialArgs &args, void *a)
    {
      args.write<X> (*reinterpret_cast<X *> (a));
    }
  };

  /**
   *  @brief A specialization of the argument writer for a const reference
   */
  template <class X>
  struct writer<const X &>
  {
    void operator() (SerialArgs &args, void *a)
    {
      args.write<const X &> (*reinterpret_cast<const X *> (a));
    }
  };

  /**
   *  @brief A specialization of the argument writer for a non-const reference
   */
  template <class X>
  struct writer<X &>
  {
    void operator() (SerialArgs &args, void *a)
    {
      args.write<X &> (*reinterpret_cast<X *> (a));
    }
  };

  /**
   *  @brief Serializes the arguments from the Qt argument stack to the argument buffer
   *  Takes one argument from the stack of arguments and writes it to the argument buffer.
   *  Then dispatches to the base class for the next arguments
   */
  void write_args (SerialArgs &args, void **a)
  {
    writer<H> () (args, *a++);
    EventSignalAdaptor<T>::write_args (args, a);
  }
};

template <class X, class E, class TL>
class EventSignalImpl;

template <class X, class E>
class EventSignalImpl<X, E, empty_list_t>
  : public Signal
{
public:
  typedef E event_type;

  EventSignalImpl (event_type (X::*event), const std::string &name, const std::string &doc)
    : Signal (name, doc), mp_event (event)
  {
  }

  virtual MethodBase *clone () const
  {
    return new EventSignalImpl<X, E, empty_list_t> (*this);
  }

  void add_handler (void *obj, SignalHandler *handler) const
  {
    _add_handler<EventSignalAdaptor<empty_list_t> > (obj, handler);
  }

  void initialize ()
  {
    this->clear ();
    _initialize ();
  }

protected:
  template <class A>
  void _add_handler (void *obj, SignalHandler *handler) const
  {
    A *adaptor = new A (this, handler);
    //  tie the lifetime of the adaptor to that of the handler
    handler->set_adaptor (adaptor);

    X *x = (X *)obj;
    (x->*mp_event).add (adaptor, &A::event_receiver);
  }

  void _initialize ()
  {
    //  .. nothing yet ..
  }

private:
  event_type X::*mp_event;
};

template <class X, class E, class H, class T>
class EventSignalImpl<X, E, type_pair_t<H, T> >
  : public EventSignalImpl<X, E, T>
{
public:
  typedef E event_type;

  EventSignalImpl (event_type (X::*event), const std::string &name, const std::string &doc)
    : EventSignalImpl<X, E, T> (event, name, doc)
  {
  }

  virtual MethodBase *clone () const
  {
    return new EventSignalImpl<X, E, type_pair_t<H, T> > (*this);
  }

  void add_handler (void *obj, SignalHandler *handler) const
  {
    EventSignalImpl<X, E, empty_list_t>::template _add_handler<EventSignalAdaptor<type_pair_t<H, T> > > (obj, handler);
  }

  template <class S>
  EventSignalImpl<X, E, T> *def_arg (const gsi::ArgSpec<S> &s)
  {
    m_s = s;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _initialize ();
  }

protected:
  void _initialize ()
  {
    this->template add_arg<H> (m_s);
    EventSignalImpl<X, E, T>::_initialize ();
  }

private:
  gsi::ArgSpec<H> m_s;
};

template <class X, class E, class TL>
class EventSignalFuncImpl;

template <class X, class E>
class EventSignalFuncImpl<X, E, empty_list_t>
  : public Signal
{
public:
  typedef E event_type;

  EventSignalFuncImpl (event_type &(*event) (X *), const std::string &name, const std::string &doc)
    : Signal (name, doc), mp_event (event)
  {
  }

  virtual MethodBase *clone () const
  {
    return new EventSignalFuncImpl<X, E, empty_list_t> (*this);
  }

  void add_handler (void *obj, SignalHandler *handler) const
  {
    _add_handler<EventSignalAdaptor<empty_list_t> > (obj, handler);
  }

  void initialize ()
  {
    this->clear ();
    _initialize ();
  }

protected:
  template <class A>
  void _add_handler (void *obj, SignalHandler *handler) const
  {
    A *adaptor = new A (this, handler);
    //  tie the lifetime of the adaptor to that of the handler
    handler->set_adaptor (adaptor);

    X *x = (X *)obj;
    (*mp_event) (x).add (adaptor, &A::event_receiver);
  }

  void _initialize ()
  {
    //  .. nothing yet ..
  }

private:
  event_type &(*mp_event) (X *x);
};

template <class X, class E, class H, class T>
class EventSignalFuncImpl<X, E, type_pair_t<H, T> >
  : public EventSignalFuncImpl<X, E, T>
{
public:
  typedef E event_type;

  EventSignalFuncImpl (event_type &(*event) (X *), const std::string &name, const std::string &doc)
    : EventSignalFuncImpl<X, E, T> (event, name, doc)
  {
  }

  virtual MethodBase *clone () const
  {
    return new EventSignalFuncImpl<X, E, type_pair_t<H, T> > (*this);
  }

  void add_handler (void *obj, SignalHandler *handler) const
  {
    EventSignalFuncImpl<X, E, empty_list_t>::template _add_handler<EventSignalAdaptor<type_pair_t<H, T> > > (obj, handler);
  }

  template <class S>
  EventSignalFuncImpl<X, E, T> *def_arg (const gsi::ArgSpec<S> &s)
  {
    m_s = s;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _initialize ();
  }

protected:
  void _initialize ()
  {
    this->template add_arg<H> (m_s);
    EventSignalFuncImpl<X, E, T>::_initialize ();
  }

private:
  gsi::ArgSpec<H> m_s;
};

/**
 *  @brief Provides the tl::event wrapper for zero arguments
 */
template <class X>
Methods event (const std::string &name, tl::event<> (X::*event), const std::string &doc = std::string ())
{
  return Methods(new EventSignalImpl<X, tl::event<>, empty_list_t> (event, name, doc));
}

/**
 *  @brief Provides the tl::event wrapper for one argument
 */
template <class X, class A1>
inline Methods event (const std::string &name, tl::event<A1> (X::*event), const std::string &doc = std::string ())
{
  return Methods(new EventSignalImpl<X, tl::event<A1>, type_pair_t<A1, empty_list_t> > (event, name, doc));
}

/**
 *  @brief Provides the tl::event wrapper for one argument
 */
template <class X, class A1, class S1>
inline Methods event (const std::string &name, tl::event<A1> (X::*event),
                      const ArgSpec<S1> &s1, const std::string &doc = std::string ())
{
  return Methods((new EventSignalImpl<X, tl::event<A1>, type_pair_t<A1, empty_list_t> > (event, name, doc))
                  ->def_arg (s1));
}

//  ...
template <class X, class A1, class A2>
inline Methods event (const std::string &name, tl::event<A1, A2> (X::*event), const std::string &doc = std::string ())
{
  return Methods(new EventSignalImpl<X, tl::event<A1, A2>, type_pair_t<A1, type_pair_t<A2, empty_list_t> > > (event, name, doc));
}

//  ...
template <class X, class A1, class A2, class S1, class S2>
inline Methods event (const std::string &name, tl::event<A1, A2> (X::*event),
                      const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const std::string &doc = std::string ())
{
  return Methods((new EventSignalImpl<X, tl::event<A1, A2>, type_pair_t<A1, type_pair_t<A2, empty_list_t> > > (event, name, doc))
                  ->def_arg (s1)->def_arg (s2));
}

//  ...
template <class X, class A1, class A2, class A3>
inline Methods event (const std::string &name, tl::event<A1, A2, A3> (X::*event), const std::string &doc = std::string ())
{
  return Methods(new EventSignalImpl<X, tl::event<A1, A2, A3>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, empty_list_t> > > > (event, name, doc));
}

//  ...
template <class X, class A1, class A2, class A3, class S1, class S2, class S3>
inline Methods event (const std::string &name, tl::event<A1, A2, A3> (X::*event),
                      const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const std::string &doc = std::string ())
{
  return Methods((new EventSignalImpl<X, tl::event<A1, A2, A3>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, empty_list_t> > > > (event, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3));
}

//  ...
template <class X, class A1, class A2, class A3, class A4>
inline Methods event (const std::string &name, tl::event<A1, A2, A3, A4> (X::*event), const std::string &doc = std::string ())
{
  return Methods(new EventSignalImpl<X, tl::event<A1, A2, A3, A4>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, empty_list_t> > > > > (event, name, doc));
}

//  ...
template <class X, class A1, class A2, class A3, class A4, class S1, class S2, class S3, class S4>
inline Methods event (const std::string &name, tl::event<A1, A2, A3, A4> (X::*event),
                      const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const std::string &doc = std::string ())
{
  return Methods((new EventSignalImpl<X, tl::event<A1, A2, A3, A4>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, empty_list_t> > > > > (event, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4));
}

/**
 *  @brief Provides the externalized tl::event wrapper for zero arguments
 */
template <class X>
Methods event_ext (const std::string &name, tl::event<> &(*event) (X *), const std::string &doc = std::string ())
{
  return Methods(new EventSignalFuncImpl<X, tl::event<>, empty_list_t> (event, name, doc));
}

/**
 *  @brief Provides the externalized tl::event wrapper for one argument
 */
template <class X, class A1>
inline Methods event_ext (const std::string &name, tl::event<A1> &(*event) (X *), const std::string &doc = std::string ())
{
  return Methods(new EventSignalFuncImpl<X, tl::event<A1>, type_pair_t<A1, empty_list_t> > (event, name, doc));
}

/**
 *  @brief Provides the externalized tl::event wrapper for one argument
 */
template <class X, class A1, class S1>
inline Methods event_ext (const std::string &name, tl::event<A1> &(*event) (X *),
                          const ArgSpec<S1> &s1, const std::string &doc = std::string ())
{
  return Methods((new EventSignalFuncImpl<X, tl::event<A1>, type_pair_t<A1, empty_list_t> > (event, name, doc))
                  ->def_arg (s1));
}

//  ...
template <class X, class A1, class A2>
inline Methods event_ext (const std::string &name, tl::event<A1, A2> &(*event) (X *), const std::string &doc = std::string ())
{
  return Methods(new EventSignalFuncImpl<X, tl::event<A1, A2>, type_pair_t<A1, type_pair_t<A2, empty_list_t> > > (event, name, doc));
}

//  ...
template <class X, class A1, class A2, class S1, class S2>
inline Methods event_ext (const std::string &name, tl::event<A1, A2> &(*event) (X *),
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const std::string &doc = std::string ())
{
  return Methods((new EventSignalFuncImpl<X, tl::event<A1, A2>, type_pair_t<A1, type_pair_t<A2, empty_list_t> > > (event, name, doc))
                  ->def_arg (s1)->def_arg (s2));
}

//  ...
template <class X, class A1, class A2, class A3>
inline Methods event_ext (const std::string &name, tl::event<A1, A2, A3> &(*event) (X *), const std::string &doc = std::string ())
{
  return Methods(new EventSignalFuncImpl<X, tl::event<A1, A2, A3>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, empty_list_t> > > > (event, name, doc));
}

//  ...
template <class X, class A1, class A2, class A3, class S1, class S2, class S3>
inline Methods event_ext (const std::string &name, tl::event<A1, A2, A3> &(*event) (X *),
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const std::string &doc = std::string ())
{
  return Methods((new EventSignalFuncImpl<X, tl::event<A1, A2, A3>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, empty_list_t> > > > (event, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3));
}

//  ...
template <class X, class A1, class A2, class A3, class A4>
inline Methods event_ext (const std::string &name, tl::event<A1, A2, A3, A4> &(*event) (X *), const std::string &doc = std::string ())
{
  return Methods(new EventSignalFuncImpl<X, tl::event<A1, A2, A3, A4>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, empty_list_t> > > > > (event, name, doc));
}

//  ...
template <class X, class A1, class A2, class A3, class A4, class S1, class S2, class S3, class S4>
inline Methods event_ext (const std::string &name, tl::event<A1, A2, A3, A4> &(*event) (X *),
                          const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const std::string &doc = std::string ())
{
  return Methods((new EventSignalFuncImpl<X, tl::event<A1, A2, A3, A4>, type_pair_t<A1, type_pair_t<A2, type_pair_t<A3, type_pair_t<A4, empty_list_t> > > > > (event, name, doc))
                  ->def_arg (s1)->def_arg (s2)->def_arg (s3)->def_arg (s4));
}

}

#endif

