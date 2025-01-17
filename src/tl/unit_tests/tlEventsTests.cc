
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


#include "tlEvents.h"
#include "tlUnitTest.h"

#include <memory>

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

