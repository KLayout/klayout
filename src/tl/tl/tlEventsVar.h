
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


//  Included by tlEvents for variable template arguments

#undef _JOIN
#if _COUNT == 0
#  define _JOIN(A,B) A
#else
#  define _JOIN(A,B) A,B
#endif

template <_TMPLARGS>
class TL_PUBLIC_TEMPLATE event_function_base<_TMPLARGLISTP>
  : public tl::Object
{
public:
  event_function_base () : tl::Object () { }
  virtual ~event_function_base () { }
  virtual void call (_JOIN(tl::Object *object, _CALLARGLIST)) = 0;
  virtual bool equals (const event_function_base<_TMPLARGLISTP> &other) = 0;
};

template <_JOIN(class T, _TMPLARGS)>
class TL_PUBLIC_TEMPLATE event_function<T, _TMPLARGLISTP>
  : public event_function_base<_TMPLARGLIST>
{
public:
  event_function (void (T::*m) (_TMPLARGLIST))
    : m_m (m)
  {
    //  .. nothing yet ..
  }

  virtual void call (_JOIN(tl::Object *object, _CALLARGLIST))
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      (t->*m_m) (_CALLARGS);
    }
  }

  virtual bool equals (const event_function_base<_TMPLARGLISTP> &other)
  {
    const event_function<_JOIN(T, _TMPLARGLIST)> *o = dynamic_cast<const event_function<_JOIN(T, _TMPLARGLIST)> *> (&other);
    return o && o->m_m == m_m;
  }

private:
  void (T::*m_m) (_TMPLARGLIST);
};

template <class T, _JOIN(class D, _TMPLARGS)>
class TL_PUBLIC_TEMPLATE event_function_with_data<T, D, _TMPLARGLISTP>
  : public event_function_base<_TMPLARGLIST>
{
public:
  event_function_with_data (void (T::*m) (_JOIN(D, _TMPLARGLIST)), D d)
    : m_m (m), m_d (d)
  {
    //  .. nothing yet ..
  }

  virtual void call (_JOIN(tl::Object *object, _CALLARGLIST))
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      (t->*m_m) (_JOIN(m_d, _CALLARGS));
    }
  }

  virtual bool equals (const event_function_base<_TMPLARGLISTP> &other)
  {
    const event_function_with_data<T, _JOIN(D, _TMPLARGLIST)> *o = dynamic_cast<const event_function_with_data<T, _JOIN(D, _TMPLARGLIST)> *> (&other);
    return o && o->m_m == m_m && o->m_d == m_d;
  }

private:
  void (T::*m_m) (_JOIN(D, _TMPLARGLIST));
  D m_d;
};

template <_JOIN(class T, _TMPLARGS)>
class TL_PUBLIC_TEMPLATE generic_event_function<T, _TMPLARGLISTP>
  : public event_function_base<_TMPLARGLIST>
{
public:
  generic_event_function (void (T::*m) (int, void **))
    : m_m (m)
  {
    //  .. nothing yet ..
  }

  virtual void call (_JOIN(tl::Object *object, _CALLARGLIST))
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      void *argv[] = { _CALLARGPTRS };
      (t->*m_m) (_COUNT, &(argv[0]));
    }
  }

  virtual bool equals (const event_function_base<_TMPLARGLISTP> &other)
  {
    const generic_event_function<_JOIN(T, _TMPLARGLIST)> *o = dynamic_cast<const generic_event_function<_JOIN(T, _TMPLARGLIST)> *> (&other);
    return o && o->m_m == m_m;
  }

private:
  void (T::*m_m) (int, void **);
};

template <class T, _JOIN(class D, _TMPLARGS)>
class TL_PUBLIC_TEMPLATE generic_event_function_with_data<T, D, _TMPLARGLISTP>
  : public event_function_base<_TMPLARGLIST>
{
public:
  generic_event_function_with_data (void (T::*m) (D, int, void **), D d)
    : m_m (m), m_d (d)
  {
    //  .. nothing yet ..
  }

  virtual void call (_JOIN(tl::Object *object, _CALLARGLIST))
  {
    T *t = dynamic_cast<T *> (object);
    if (t) {
      void *argv[] = { _CALLARGPTRS };
      (t->*m_m) (m_d, _COUNT, &(argv[0]));
    }
  }

  virtual bool equals (const event_function_base<_TMPLARGLISTP> &other)
  {
    const generic_event_function_with_data<T, _JOIN(D, _TMPLARGLIST)> *o = dynamic_cast<const generic_event_function_with_data<T, _JOIN(D, _TMPLARGLIST)> *> (&other);
    return o && o->m_m == m_m && o->m_d == m_d;
  }

private:
  void (T::*m_m) (D, int, void **);
  D m_d;
};

template <_TMPLARGS>
class TL_PUBLIC_TEMPLATE event<_TMPLARGLISTP>
{
public:
  typedef event_function_base<_TMPLARGLISTP> func;
  typedef std::vector<std::pair<tl::weak_ptr<tl::Object>, tl::shared_ptr<func> > > receivers;
#if _COUNT == 0
  //  NOTE: for gcc 4.4.7 (and may others), event is not a template in this case and
  //  typename must not be given.
  typedef receivers::iterator receivers_iterator;
#else
  typedef typename receivers::iterator receivers_iterator;
#endif

  void operator() (_CALLARGLIST)
  {
    //  Issue the events. Because inside the call, other receivers might be added, we make a copy
    //  first. This way added events won't be called now.
    receivers tmp_receivers = m_receivers;
    for (receivers_iterator r = tmp_receivers.begin (); r != tmp_receivers.end (); ++r) {
      if (r->first.get ()) {
        try {
          r->second->call (_JOIN(r->first.get (), _CALLARGS));
        } catch (tl::Exception &ex) {
          handle_event_exception (ex);
        } catch (std::exception &ex) {
          handle_event_exception (ex);
        } catch (...) {
          //  Unknown exceptions are ignored
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
  void add (T *obj, void (T::*m) (_TMPLARGLIST))
  {
    event_function<T, _TMPLARGLISTP> f (m);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new event_function<T, _TMPLARGLISTP> (f));
  }

  template <class T>
  void remove (T *obj, void (T::*m) (_TMPLARGLIST))
  {
    event_function<T, _TMPLARGLISTP> f (m);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        m_receivers.erase (r);
        return;
      }
    }
  }

  template <class T, class D>
  void add (T *obj, void (T::*m) (_JOIN(D, _TMPLARGLIST)), D d)
  {
    event_function_with_data<T, D, _TMPLARGLISTP> f (m, d);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new event_function_with_data<T, D, _TMPLARGLISTP> (f));
  }

  template <class T, class D>
  void remove (T *obj, void (T::*m) (_JOIN(D, _TMPLARGLIST)), D d)
  {
    event_function_with_data<T, _JOIN(D, _TMPLARGLIST)> f (m, d);
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
    generic_event_function<T, _TMPLARGLISTP> f (m);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new generic_event_function<T, _TMPLARGLISTP> (f));
  }

  template <class T>
  void remove (T *obj, void (T::*m) (int, void **))
  {
    generic_event_function<T, _TMPLARGLISTP> f (m);
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
    generic_event_function_with_data<T, _JOIN(D, _TMPLARGLIST)> f (m, d);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        return;
      }
    }
    m_receivers.push_back (typename receivers::value_type ());
    m_receivers.back ().first.reset (obj, true /*is an event*/);
    m_receivers.back ().second.reset (new generic_event_function_with_data<T, D, _TMPLARGLISTP> (f));
  }

  template <class T, class D>
  void remove (T *obj, void (T::*m) (D, int, void **), D d)
  {
    generic_event_function_with_data<T, _JOIN(D, _TMPLARGLIST)> f (m, d);
    for (receivers_iterator r = m_receivers.begin (); r != m_receivers.end (); ++r) {
      if (r->first.get () == obj && r->second->equals (f)) {
        //  this receiver is already registered
        m_receivers.erase (r);
        return;
      }
    }
  }

private:
  receivers m_receivers;
};

