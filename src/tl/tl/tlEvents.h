
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef _HDR_tlEvents
#define _HDR_tlEvents

#include "tlObject.h"
#include "tlException.h"

#include <vector>
#include <algorithm>

namespace tl
{

/**
 *  @brief A helper function to handle event exceptions of tl::Exception type
 */
TL_PUBLIC void handle_event_exception (tl::Exception &ex);

/**
 *  @brief A helper function to handle event exceptions of std::exception type
 */
TL_PUBLIC void handle_event_exception (std::exception &ex);

/**
 *  @brief A framework of observer and observables
 *
 *  This framework competes with the signal/event concept of Qt. It is provided because
 *  the latter has some weaknesses, namely:
 *
 *   - Signals/slots cannot be a defined for templates
 *   - The involved objects need to be derived from QObject
 *   - The Qt concept requires the meta object compiler
 *
 *  the framework provided herein is based on C++ meta programming alone and avoids
 *  some of these issues.
 *
 *  Events are used the following way:
 *   - Instantiate an event object inside the observable class
 *   - Expose this event object
 *   - Use add/remove to attach or detach receivers
 *
 *  @code
 *
 *  //  Object with event
 *  class Observed
 *  {
 *  public:
 *    tl::event<Observed *, int> &event () { return m_event; }
 *    void trigger_event ()
 *    {
 *      //  issue event
 *      m_event (this, -1);
 *    }
 *  private:
 *    tl::event<Observed *, int> m_event;
 *  };
 *
 *  //  Observer
 *  class Observer : public tl::Object
 *  {
 *  public:
 *    void receives_event (Observed *, int) { ... }
 *  };
 *
 *  //  Connect this
 *  Observed x;
 *  Observer y;
 *  x.event ().add (&y, &Observed::receives_event);
 *  x.trigger_event ();
 *
 *  @endcode
 *
 *  Events can be given a disambiguation or client data parameter:
 *
 *  @code
 *
 *  //  Observer
 *  class Observer : public tl::Object
 *  {
 *  public:
 *    //  The first parameter is the client data parameter which is given as
 *    //  third parameter in "add"
 *    void receives_event (int, Observed *, int) { ... }
 *  };
 *
 *  //  Connect this
 *  Observed x;
 *  Observer y;
 *  x.event ().add (&y, &Observed::receives_event, 1);
 *  x.trigger_event ();
 *
 *  @endcode
 */

template <class... Args>
class TL_PUBLIC_TEMPLATE event_function_base
  : public tl::Object
{
public:
  event_function_base () : tl::Object () { }
  virtual ~event_function_base () { }
  virtual void call (tl::Object *object, Args... args) = 0;
  virtual bool equals (const event_function_base<Args...> &other) = 0;
};

template <class T, class... Args>
class TL_PUBLIC_TEMPLATE event_function
  : public event_function_base<Args...>
{
public:
  event_function (void (T::*m) (Args...))
    : m_m (m)
  {
    //  .. nothing yet ..
  }

  virtual void call (tl::Object *object, Args... args)
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      (t->*m_m) (args...);
    }
  }

  virtual bool equals (const event_function_base<Args...> &other)
  {
    const event_function<T, Args...> *o = dynamic_cast<const event_function<T, Args...> *> (&other);
    return o && o->m_m == m_m;
  }

private:
  void (T::*m_m) (Args...);
};

template <class T, class D, class... Args>
class TL_PUBLIC_TEMPLATE event_function_with_data
  : public event_function_base<Args...>
{
public:
  event_function_with_data (void (T::*m) (D, Args...), D d)
    : m_m (m), m_d (d)
  {
    //  .. nothing yet ..
  }

  virtual void call (tl::Object *object, Args... args)
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      (t->*m_m) (m_d, args...);
    }
  }

  virtual bool equals (const event_function_base<Args...> &other)
  {
    const event_function_with_data<T, D, Args...> *o = dynamic_cast<const event_function_with_data<T, D, Args...> *> (&other);
    return o && o->m_m == m_m && o->m_d == m_d;
  }

private:
  void (T::*m_m) (D, Args...);
  D m_d;
};

template <class T, class... Args>
class TL_PUBLIC_TEMPLATE generic_event_function
  : public event_function_base<Args...>
{
public:
  generic_event_function (void (T::*m) (int, void **))
    : m_m (m)
  {
    //  .. nothing yet ..
  }

  virtual void call (tl::Object *object, Args... args)
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      void *argv[sizeof... (Args) + 1] = { const_cast<void *> (static_cast<const void *> (&args))..., nullptr };
      (t->*m_m) (int (sizeof... (Args)), &(argv[0]));
    }
  }

  virtual bool equals (const event_function_base<Args...> &other)
  {
    const generic_event_function<T, Args...> *o = dynamic_cast<const generic_event_function<T, Args...> *> (&other);
    return o && o->m_m == m_m;
  }

private:
  void (T::*m_m) (int, void **);
};

template <class T, class D, class... Args>
class TL_PUBLIC_TEMPLATE generic_event_function_with_data
  : public event_function_base<Args...>
{
public:
  generic_event_function_with_data (void (T::*m) (D, int, void **), D d)
    : m_m (m), m_d (d)
  {
    //  .. nothing yet ..
  }

  virtual void call (tl::Object *object, Args... args)
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      void *argv[sizeof... (Args) + 1] = { const_cast<void *> (static_cast<const void *> (&args))..., nullptr };
      (t->*m_m) (m_d, int (sizeof... (Args)), &(argv[0]));
    }
  }

  virtual bool equals (const event_function_base<Args...> &other)
  {
    const generic_event_function_with_data<T, D, Args...> *o = dynamic_cast<const generic_event_function_with_data<T, D, Args...> *> (&other);
    return o && o->m_m == m_m && o->m_d == m_d;
  }

private:
  void (T::*m_m) (D, int, void **);
  D m_d;
};

template <class... Args>
class TL_PUBLIC_TEMPLATE event
{
public:
  using func = event_function_base<Args...>;
  using receivers = std::vector<std::pair<tl::weak_ptr<tl::Object>, tl::shared_ptr<func> > >;
  using receivers_iterator = typename receivers::iterator;

  event ()
    : mp_top_frame (0)
  {
    //  .. nothing yet ..
  }

  event (const event &d)
    : mp_top_frame (0), m_receivers (d.m_receivers)
  {
    //  a copy does not take part in the emissions of the original
  }

  event &operator= (const event &d)
  {
    if (this != &d) {
      m_receivers = d.m_receivers;
    }
    return *this;
  }

  ~event ()
  {
    //  mark all active emissions (including outer ones of nested emissions) as destroyed
    for (emission_frame *f = mp_top_frame; f; f = f->prev) {
      f->destroyed = true;
    }
    mp_top_frame = 0;
  }

  void operator() (Args... args)
  {
    emission_frame frame (&mp_top_frame);

    //  Issue the events. Because inside the call, other receivers might be added, we make a copy
    //  first. This way added events won't be called now.
    receivers tmp_receivers = m_receivers;
    for (receivers_iterator r = tmp_receivers.begin (); r != tmp_receivers.end (); ++r) {
      if (r->first.get ()) {
        try {
          r->second->call (r->first.get (), args...);
        } catch (tl::Exception &ex) {
          handle_event_exception (ex);
        } catch (std::exception &ex) {
          handle_event_exception (ex);
        } catch (...) {
          //  Unknown exceptions are ignored
        }
        if (frame.destroyed) {
          //  during the call something deleted us (and maybe threw). Stop immediately and don't touch any member.
          return;
        }
      }
    }

    //  Clean up expired entries afterwards (the call may have expired them)
    receivers_iterator w = m_receivers.begin ();
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get ()) {
        if (w != r) {
          *w = *r;
        }
        ++w;
      }
    }
    if (w != m_receivers.end ()) {
      m_receivers.erase (w, m_receivers.end ());
    }
  }

  void clear ()
  {
    m_receivers.clear ();
  }

  template <class T>
  T *find_receiver ()
  {
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      T *t = dynamic_cast<T *> (r->first.get ());
      if (t) {
        return t;
      }
    }
    return 0;
  }

  template <class T>
  void add (T *obj, void (T::*m) (Args...))
  {
    event_function<T, Args...> f (m);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new event_function<T, Args...> (f));
  }

  template <class T>
  void remove (T *obj, void (T::*m) (Args...))
  {
    event_function<T, Args...> f (m);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        m_receivers.erase (r);
        return;
      }
    }
  }

  template <class T, class D>
  void add (T *obj, void (T::*m) (D, Args...), D d)
  {
    event_function_with_data<T, D, Args...> f (m, d);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new event_function_with_data<T, D, Args...> (f));
  }

  template <class T, class D>
  void remove (T *obj, void (T::*m) (D, Args...), D d)
  {
    event_function_with_data<T, D, Args...> f (m, d);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        m_receivers.erase (r);
        return;
      }
    }
  }

  template <class T>
  void add (T *obj, void (T::*m) (int, void **))
  {
    generic_event_function<T, Args...> f (m);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new generic_event_function<T, Args...> (f));
  }

  template <class T>
  void remove (T *obj, void (T::*m) (int, void **))
  {
    generic_event_function<T, Args...> f (m);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        m_receivers.erase (r);
        return;
      }
    }
  }

  template <class T, class D>
  void add (T *obj, void (T::*m) (D, int, void **), D d)
  {
    generic_event_function_with_data<T, D, Args...> f (m, d);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new generic_event_function_with_data<T, D, Args...> (f));
  }

  template <class T, class D>
  void remove (T *obj, void (T::*m) (D, int, void **), D d)
  {
    generic_event_function_with_data<T, D, Args...> f (m, d);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        m_receivers.erase (r);
        return;
      }
    }
  }

private:
  struct emission_frame
  {
    emission_frame (emission_frame **top)
      : destroyed (false), prev (*top), mp_top (top)
    {
      *top = this;
    }

    ~emission_frame ()
    {
      //  the event is gone if it was destroyed during the emission
      if (! destroyed) {
        *mp_top = prev;
      }
    }

    bool destroyed;
    emission_frame *prev;
    emission_frame **mp_top;
  };

  emission_frame *mp_top_frame;
  receivers m_receivers;
};

typedef event<> Event;

}

#endif
