
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


#include "tlEvents.h"
#include "tlUnitTest.h"

#include <memory>
#include <string>

//  Object with event
class Observed
{
public:
  tl::event<Observed *, int> &event () { return m_event; }
  tl::Event &void_event () { return m_void_event; }

  void trigger_event (int a)
  {
    //  issue event
    m_event (this, a);
  }

  void trigger_void_event ()
  {
    m_void_event ();
  }

private:
  tl::event<Observed *, int> m_event;
  tl::Event m_void_event;
};

//  Observer
class Observer : public tl::Object
{
public:
  Observer () : data (0), events (0), obj (0), arg (0) { }
  void receives_event (Observed *o, int a) { data = -1, obj = o; arg = a; ++events; }
  void receives_event_with_data (int d, Observed *o, int a) { data = d; obj = o; arg = a; ++events; }
  void receives_void_event () { data = -1, obj = 0, arg = -1, ++events; }
  void receives_void_event_with_data (int d) { data = d, obj = 0, arg = -1, ++events; }
  void receives_generic_event (int, void **argv) { data = -1, obj = *(Observed **)argv[0]; arg = *(int *)argv[1]; ++events; }
  void receives_generic_event_with_data (int d, int, void **argv) { data = d; obj = *(Observed **)argv[0]; arg = *(int *)argv[1]; ++events; }

  int data;
  int events;
  Observed *obj;
  int arg;
};

// basics
TEST(1)
{
  Observed x;
  std::unique_ptr<Observer> yp;
  yp.reset (new Observer ());

  EXPECT_EQ (yp->obj == 0, true);
  EXPECT_EQ (yp->arg, 0);
  EXPECT_EQ (yp->events, 0);

  x.event ().add (yp.get (), &Observer::receives_event);
  x.trigger_event (17);

  EXPECT_EQ (yp->events, 1);
  EXPECT_EQ (yp->arg, 17);
  EXPECT_EQ (yp->obj == &x, true);

  Observer y2;

  x.event ().add (yp.get (), &Observer::receives_event);
  x.event ().add (&y2, &Observer::receives_event);
  x.trigger_event (42);

  EXPECT_EQ (yp->events, 2);
  EXPECT_EQ (yp->arg, 42);
  EXPECT_EQ (yp->obj == &x, true);
  EXPECT_EQ (y2.events, 1);
  EXPECT_EQ (y2.arg, 42);
  EXPECT_EQ (y2.obj == &x, true);

  yp->obj = 0;
  y2.obj = 0;
  x.event ().remove (&y2, &Observer::receives_event);
  x.trigger_event (13);

  EXPECT_EQ (yp->events, 3);
  EXPECT_EQ (yp->arg, 13);
  EXPECT_EQ (yp->obj == &x, true);
  EXPECT_EQ (y2.events, 1);
  EXPECT_EQ (y2.arg, 42);
  EXPECT_EQ (y2.obj == 0, true);

  yp.reset (0);
  x.trigger_event (13);

  EXPECT_EQ (y2.events, 1);
  EXPECT_EQ (y2.arg, 42);
  EXPECT_EQ (y2.obj == 0, true);

  x.event ().add (&y2, &Observer::receives_event);
  x.trigger_event (13);

  EXPECT_EQ (y2.events, 2);
  EXPECT_EQ (y2.arg, 13);
  EXPECT_EQ (y2.obj == &x, true);
}

// events with data
TEST(2)
{
  Observed x1, x2;
  std::unique_ptr<Observer> yp;
  yp.reset (new Observer ());

  EXPECT_EQ (yp->obj == 0, true);
  EXPECT_EQ (yp->arg, 0);
  EXPECT_EQ (yp->events, 0);

  x1.event ().add (yp.get (), &Observer::receives_event_with_data, 1);
  x2.event ().add (yp.get (), &Observer::receives_event_with_data, 2);

  x1.trigger_event (17);

  EXPECT_EQ (yp->events, 1);
  EXPECT_EQ (yp->data, 1);
  EXPECT_EQ (yp->arg, 17);
  EXPECT_EQ (yp->obj == &x1, true);

  x2.trigger_event (177);

  EXPECT_EQ (yp->events, 2);
  EXPECT_EQ (yp->data, 2);
  EXPECT_EQ (yp->arg, 177);
  EXPECT_EQ (yp->obj == &x2, true);

  x2.event ().remove (yp.get (), &Observer::receives_event_with_data, 2);

  x1.trigger_event (42);

  EXPECT_EQ (yp->events, 3);
  EXPECT_EQ (yp->data, 1);
  EXPECT_EQ (yp->arg, 42);
  EXPECT_EQ (yp->obj == &x1, true);

  x2.trigger_event (13);

  EXPECT_EQ (yp->events, 3);
  EXPECT_EQ (yp->data, 1);
  EXPECT_EQ (yp->arg, 42);
  EXPECT_EQ (yp->obj == &x1, true);
}

// void events
TEST(3)
{
  Observed x;
  Observer y;

  x.void_event ().add (&y, &Observer::receives_void_event);

  EXPECT_EQ (y.obj == 0, true);
  EXPECT_EQ (y.arg, 0);
  EXPECT_EQ (y.events, 0);
  EXPECT_EQ (y.data, 0);

  x.trigger_void_event ();

  EXPECT_EQ (y.obj == 0, true);
  EXPECT_EQ (y.arg, -1);
  EXPECT_EQ (y.events, 1);
  EXPECT_EQ (y.data, -1);

  x.void_event ().remove (&y, &Observer::receives_void_event);
  x.void_event ().add (&y, &Observer::receives_void_event_with_data, 17);

  x.trigger_void_event ();

  EXPECT_EQ (y.obj == 0, true);
  EXPECT_EQ (y.arg, -1);
  EXPECT_EQ (y.events, 2);
  EXPECT_EQ (y.data, 17);
}

// generic events
TEST(4)
{
  Observed x;
  std::unique_ptr<Observer> yp;
  yp.reset (new Observer ());

  EXPECT_EQ (yp->obj == 0, true);
  EXPECT_EQ (yp->arg, 0);
  EXPECT_EQ (yp->events, 0);

  x.event ().add (yp.get (), &Observer::receives_generic_event);
  x.trigger_event (17);

  EXPECT_EQ (yp->events, 1);
  EXPECT_EQ (yp->arg, 17);
  EXPECT_EQ (yp->obj == &x, true);

  Observer y2;

  x.event ().add (yp.get (), &Observer::receives_generic_event);
  x.event ().add (&y2, &Observer::receives_generic_event);
  x.trigger_event (42);

  EXPECT_EQ (yp->events, 2);
  EXPECT_EQ (yp->arg, 42);
  EXPECT_EQ (yp->obj == &x, true);
  EXPECT_EQ (y2.events, 1);
  EXPECT_EQ (y2.arg, 42);
  EXPECT_EQ (y2.obj == &x, true);

  yp->obj = 0;
  y2.obj = 0;
  x.event ().remove (&y2, &Observer::receives_generic_event);
  x.trigger_event (13);

  EXPECT_EQ (yp->events, 3);
  EXPECT_EQ (yp->arg, 13);
  EXPECT_EQ (yp->obj == &x, true);
  EXPECT_EQ (y2.events, 1);
  EXPECT_EQ (y2.arg, 42);
  EXPECT_EQ (y2.obj == 0, true);

  yp.reset (0);
  x.trigger_event (13);

  EXPECT_EQ (y2.events, 1);
  EXPECT_EQ (y2.arg, 42);
  EXPECT_EQ (y2.obj == 0, true);

  x.event ().add (&y2, &Observer::receives_generic_event);
  x.trigger_event (13);

  EXPECT_EQ (y2.events, 2);
  EXPECT_EQ (y2.arg, 13);
  EXPECT_EQ (y2.obj == &x, true);
}

// generic events with data
TEST(5)
{
  Observed x1, x2;
  std::unique_ptr<Observer> yp;
  yp.reset (new Observer ());

  EXPECT_EQ (yp->obj == 0, true);
  EXPECT_EQ (yp->arg, 0);
  EXPECT_EQ (yp->events, 0);

  x1.event ().add (yp.get (), &Observer::receives_generic_event_with_data, 1);
  x2.event ().add (yp.get (), &Observer::receives_generic_event_with_data, 2);

  x1.trigger_event (17);

  EXPECT_EQ (yp->events, 1);
  EXPECT_EQ (yp->data, 1);
  EXPECT_EQ (yp->arg, 17);
  EXPECT_EQ (yp->obj == &x1, true);

  x2.trigger_event (177);

  EXPECT_EQ (yp->events, 2);
  EXPECT_EQ (yp->data, 2);
  EXPECT_EQ (yp->arg, 177);
  EXPECT_EQ (yp->obj == &x2, true);

  x2.event ().remove (yp.get (), &Observer::receives_generic_event_with_data, 2);

  x1.trigger_event (42);

  EXPECT_EQ (yp->events, 3);
  EXPECT_EQ (yp->data, 1);
  EXPECT_EQ (yp->arg, 42);
  EXPECT_EQ (yp->obj == &x1, true);

  x2.trigger_event (13);

  EXPECT_EQ (yp->events, 3);
  EXPECT_EQ (yp->data, 1);
  EXPECT_EQ (yp->arg, 42);
  EXPECT_EQ (yp->obj == &x1, true);
}

//  Receiver doing the various actions for the nested emission test
class ReentrantObserver : public tl::Object
{
public:
  ReentrantObserver () : event (0), role (0), calls (0) { }

  void receives (int a)
  {
    ++calls;
    if (a == 1 && role == 1) {
      //  nested (re-entrant) emission of the same event
      (*event) (2);
    } else if (a == 2 && role == 2) {
      //  delete the event while it is still emitting
      delete event;
      event = 0;
    }
  }

  tl::event<int> *event;
  int role;
  int calls;
};

//  Receiver which deletes the event and then fails
class DeleteAndThrowObserver : public tl::Object
{
public:
  DeleteAndThrowObserver () : event (0) { }

  void receives (int)
  {
    delete event;
    event = 0;
    throw tl::Exception ("receiver failed");
  }

  tl::event<int> *event;
};

//  event destroyed by a receiver which throws afterwards
TEST(destroy_then_throw)
{
  tl::event<int> *ev = new tl::event<int> ();

  DeleteAndThrowObserver a;
  ReentrantObserver b;
  a.event = ev;
  b.role = 3;

  ev->add (&a, &DeleteAndThrowObserver::receives);
  ev->add (&b, &ReentrantObserver::receives);

  (*ev) (1);

  //  the emission stops after the deletion even though the receiver threw
  EXPECT_EQ (b.calls, 0);
  EXPECT_EQ (a.event == 0, true);
}

//  copying an event while it is emitting does not affect the original emission
TEST(copy_during_emission)
{
  tl::event<int> *ev = new tl::event<int> ();
  tl::event<int> *copy = 0;

  ReentrantObserver a, b;
  a.role = 3;
  b.role = 3;

  struct Copier : public tl::Object
  {
    Copier () : src (0), copy (0) { }
    void receives (int) { copy = new tl::event<int> (*src); }
    tl::event<int> *src;
    tl::event<int> *copy;
  } copier;
  copier.src = ev;

  ev->add (&copier, &Copier::receives);
  ev->add (&a, &ReentrantObserver::receives);
  ev->add (&b, &ReentrantObserver::receives);

  (*ev) (1);
  copy = copier.copy;

  //  destroying the copy must not mark anything of the original
  delete copy;
  EXPECT_EQ (a.calls, 1);
  EXPECT_EQ (b.calls, 1);

  delete ev;
}

// event destroyed during a nested emission
TEST(6)
{
  tl::event<int> *ev = new tl::event<int> ();

  ReentrantObserver a, b, c, d;
  a.event = ev;
  b.event = ev;
  c.event = ev;
  d.event = ev;
  a.role = 0;
  b.role = 1;  //  re-emits on value 1
  c.role = 2;  //  deletes the event on value 2
  d.role = 3;  //  counts

  ev->add (&a, &ReentrantObserver::receives);
  ev->add (&b, &ReentrantObserver::receives);
  ev->add (&c, &ReentrantObserver::receives);
  ev->add (&d, &ReentrantObserver::receives);

  (*ev) (1);

  //  The deletion happened in the nested emission; the outer emission must stop too,
  //  so D is never reached. On the old code the outer loop kept calling D.
  EXPECT_EQ (d.calls, 0);
}

//  Receiver recording the arguments it got, for events of varying arity
class MultiArgObserver : public tl::Object
{
public:
  MultiArgObserver () : events (0), data (0), i (0), d (0), extra (0), p (0) { }

  void recv0 () { events += 1; }
  void recv1 (int a) { events += 1; i = a; }
  void recv2 (int a, double b) { events += 1; i = a; d = b; }
  void recv3 (int a, double b, std::string c) { events += 1; i = a; d = b; s = c; }
  void recv4 (int a, double b, std::string c, const std::string &e) { events += 1; i = a; d = b; s = c; r = e; }
  void recv5 (int a, double b, std::string c, const std::string &e, MultiArgObserver *q) { events += 1; i = a; d = b; s = c; r = e; p = q; }
  void recv6 (int a, double b, std::string c, const std::string &e, MultiArgObserver *q, int f) { events += 1; i = a; d = b; s = c; r = e; p = q; extra = f; }

  void recv_d0 (int dd) { events += 1; data = dd; }
  void recv_d1 (int dd, int a) { events += 1; data = dd; i = a; }
  void recv_d2 (int dd, int a, double b) { events += 1; data = dd; i = a; d = b; }
  void recv_d3 (int dd, int a, double b, std::string c) { events += 1; data = dd; i = a; d = b; s = c; }
  void recv_d4 (int dd, int a, double b, std::string c, const std::string &e) { events += 1; data = dd; i = a; d = b; s = c; r = e; }
  void recv_d5 (int dd, int a, double b, std::string c, const std::string &e, MultiArgObserver *q) { events += 1; data = dd; i = a; d = b; s = c; r = e; p = q; }
  void recv_d6 (int dd, int a, double b, std::string c, const std::string &e, MultiArgObserver *q, int f) { events += 1; data = dd; i = a; d = b; s = c; r = e; p = q; extra = f; }

  int events;
  int data;
  int i;
  double d;
  int extra;
  MultiArgObserver *p;
  std::string s;
  std::string r;
};

//  events with 0 to 4 arguments of mixed types, plain and with_data receivers
TEST(7)
{
  MultiArgObserver y, z;

  tl::event<> ev0;
  tl::event<int> ev1;
  tl::event<int, double> ev2;
  tl::event<int, double, std::string> ev3;
  tl::event<int, double, std::string, const std::string &> ev4;

  ev0.add (&y, &MultiArgObserver::recv0);
  ev1.add (&y, &MultiArgObserver::recv1);
  ev2.add (&y, &MultiArgObserver::recv2);
  ev3.add (&y, &MultiArgObserver::recv3);
  ev4.add (&y, &MultiArgObserver::recv4);

  ev0.add (&z, &MultiArgObserver::recv0);
  ev1.add (&z, &MultiArgObserver::recv1);

  ev1 (42);
  EXPECT_EQ (y.events, 1);
  EXPECT_EQ (y.i, 42);
  EXPECT_EQ (z.events, 1);
  EXPECT_EQ (z.i, 42);

  ev2 (2, 2.5);
  EXPECT_EQ (y.events, 2);
  EXPECT_EQ (y.i, 2);
  EXPECT_EQ (y.d, 2.5);

  ev3 (3, 3.5, std::string ("three"));
  EXPECT_EQ (y.events, 3);
  EXPECT_EQ (y.i, 3);
  EXPECT_EQ (y.d, 3.5);
  EXPECT_EQ (y.s, "three");

  ev4 (4, 4.5, std::string ("four"), std::string ("ref"));
  EXPECT_EQ (y.events, 4);
  EXPECT_EQ (y.i, 4);
  EXPECT_EQ (y.d, 4.5);
  EXPECT_EQ (y.s, "four");
  EXPECT_EQ (y.r, "ref");

  //  remove a receiver
  ev1.remove (&z, &MultiArgObserver::recv1);
  ev1 (43);
  EXPECT_EQ (y.events, 5);
  EXPECT_EQ (y.i, 43);
  EXPECT_EQ (z.events, 1);
  EXPECT_EQ (z.i, 42);

  //  with_data receivers
  MultiArgObserver w;
  ev0.add (&w, &MultiArgObserver::recv_d0, 10);
  ev1.add (&w, &MultiArgObserver::recv_d1, 11);
  ev2.add (&w, &MultiArgObserver::recv_d2, 12);
  ev3.add (&w, &MultiArgObserver::recv_d3, 13);
  ev4.add (&w, &MultiArgObserver::recv_d4, 14);

  ev0 ();
  EXPECT_EQ (w.events, 1);
  EXPECT_EQ (w.data, 10);

  ev1 (5);
  EXPECT_EQ (w.events, 2);
  EXPECT_EQ (w.data, 11);
  EXPECT_EQ (w.i, 5);

  ev2 (6, 6.5);
  EXPECT_EQ (w.events, 3);
  EXPECT_EQ (w.data, 12);
  EXPECT_EQ (w.i, 6);
  EXPECT_EQ (w.d, 6.5);

  ev3 (7, 7.5, std::string ("seven"));
  EXPECT_EQ (w.events, 4);
  EXPECT_EQ (w.data, 13);
  EXPECT_EQ (w.i, 7);
  EXPECT_EQ (w.s, "seven");

  ev4 (8, 8.5, std::string ("eight"), std::string ("ref2"));
  EXPECT_EQ (w.events, 5);
  EXPECT_EQ (w.data, 14);
  EXPECT_EQ (w.i, 8);
  EXPECT_EQ (w.d, 8.5);
  EXPECT_EQ (w.s, "eight");
  EXPECT_EQ (w.r, "ref2");
}
