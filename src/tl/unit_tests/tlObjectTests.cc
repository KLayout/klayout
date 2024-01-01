
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include "tlObjectCollection.h"
#include "tlUnitTest.h"

namespace {

  class MyClass : public tl::Object
  {
  public:
    MyClass (int attr = 0)
      : tl::Object (), m_attr (attr)
    {
      ++s_myclass_instances;
    }

    ~MyClass ()
    {
      --s_myclass_instances;
    }

    int attr () const { return m_attr; }
    void set_attr (int a) { m_attr = a; }
    static int instances () { return s_myclass_instances; }
    static void reset_instance_counter () { s_myclass_instances = 0; }

  private:
    int m_attr;
    static int s_myclass_instances;
  };

  int MyClass::s_myclass_instances = 0;

}

TEST(1)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass ();
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_ptr<MyClass> sp0;
    EXPECT_EQ (sp0.get () == 0, true);

    tl::shared_ptr<MyClass> sp (o);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (sp.get () == o, true);
    EXPECT_EQ (sp ? 1 : 0, 1);

    sp0 = sp;
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (sp0.get () == o, true);
    EXPECT_EQ (sp0 ? 1 : 0, 1);

    //  installing the same pointer does not change anything
    sp0.reset (sp.get ());
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (sp0.get () == o, true);
    EXPECT_EQ (sp.get () == o, true);

    delete o;
    o = 0;
    EXPECT_EQ (sp.get () == 0, true);
    EXPECT_EQ (sp ? 1 : 0, 0);
    EXPECT_EQ (sp0.get () == 0, true);
    EXPECT_EQ (sp0 ? 1 : 0, 0);
    EXPECT_EQ (MyClass::instances (), 0);

    MyClass *oo = new MyClass ();
    sp.reset (oo);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (sp.get () == oo, true);
    //  resetting again does not change anything
    sp.reset (oo);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (sp.get () == oo, true);

  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(2)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_ptr<MyClass> sp (o);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (sp.get () == o, true);
    EXPECT_EQ (sp->attr (), 1);
    EXPECT_EQ (sp ? 1 : 0, 1);

    sp = new MyClass (2);
    EXPECT_EQ (sp.get () == o, false);
    EXPECT_EQ (sp.get () == 0, false);
    EXPECT_EQ (sp->attr (), 2);
    EXPECT_EQ (sp ? 1 : 0, 1);
    EXPECT_EQ (MyClass::instances (), 1);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}


TEST(3)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_ptr<MyClass> sp1 (o);
    tl::shared_ptr<MyClass> sp2 (o);

    EXPECT_EQ (MyClass::instances (), 1);

    EXPECT_EQ (sp1.get () == o, true);
    EXPECT_EQ (sp1->attr (), 1);
    EXPECT_EQ (sp1 ? 1 : 0, 1);
    EXPECT_EQ (sp2.get () == o, true);
    EXPECT_EQ (sp2->attr (), 1);
    EXPECT_EQ (sp2 ? 1 : 0, 1);

    sp1->set_attr (42);
    EXPECT_EQ (sp1->attr (), 42);
    EXPECT_EQ (sp2->attr (), 42);

    sp1 = new MyClass (2);
    EXPECT_EQ (sp1.get () == o, false);
    EXPECT_EQ (sp2.get () == o, true);
    EXPECT_EQ (sp1->attr (), 2);
    EXPECT_EQ (sp1 ? 1 : 0, 1);
    EXPECT_EQ (MyClass::instances (), 2);

    sp2 = tl::shared_ptr<MyClass> ();
    EXPECT_EQ (sp1->attr (), 2);
    EXPECT_EQ (sp2.get () == 0, true);
    EXPECT_EQ (MyClass::instances (), 1);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(10)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::weak_ptr<MyClass> wp (o);
    EXPECT_EQ (wp.get () == o, true);
    EXPECT_EQ (wp->attr (), 1);
    EXPECT_EQ (wp ? 1 : 0, 1);
  }

  EXPECT_EQ (MyClass::instances (), 1);

  delete o;
  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(11)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  tl::weak_ptr<MyClass> wp;
  EXPECT_EQ (wp.get () == 0, true);
  EXPECT_EQ (wp ? 1 : 0, 0);

  wp = o;
  EXPECT_EQ (wp.get () == o, true);
  EXPECT_EQ (wp->attr (), 1);
  EXPECT_EQ (wp ? 1 : 0, 1);

  wp = 0;
  EXPECT_EQ (wp.get () == 0, true);
  EXPECT_EQ (wp ? 1 : 0, 0);
  EXPECT_EQ (MyClass::instances (), 1);

  wp = o;
  EXPECT_EQ (wp.get () == o, true);
  EXPECT_EQ (wp->attr (), 1);
  EXPECT_EQ (wp ? 1 : 0, 1);

  delete o;
  EXPECT_EQ (wp.get () == 0, true);
  EXPECT_EQ (wp ? 1 : 0, 0);
  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(12)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  tl::weak_ptr<MyClass> wp(o);
  tl::shared_ptr<MyClass> sp(o);
  EXPECT_EQ (wp.get () == o, true);
  EXPECT_EQ (wp->attr (), 1);
  EXPECT_EQ (wp ? 1 : 0, 1);
  EXPECT_EQ (sp.get () == o, true);
  EXPECT_EQ (sp->attr (), 1);
  EXPECT_EQ (sp ? 1 : 0, 1);

  sp = 0;
  EXPECT_EQ (wp.get () == 0, true);
  EXPECT_EQ (wp ? 1 : 0, 0);
  EXPECT_EQ (sp.get () == 0, true);
  EXPECT_EQ (sp ? 1 : 0, 0);
  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(13)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  tl::weak_ptr<MyClass> wp(o);
  tl::shared_ptr<MyClass> sp(o);
  EXPECT_EQ (wp.get () == o, true);
  EXPECT_EQ (wp->attr (), 1);
  EXPECT_EQ (wp ? 1 : 0, 1);
  EXPECT_EQ (sp.get () == o, true);
  EXPECT_EQ (sp->attr (), 1);
  EXPECT_EQ (sp ? 1 : 0, 1);

  wp = 0;
  EXPECT_EQ (wp.get () == 0, true);
  EXPECT_EQ (wp ? 1 : 0, 0);
  EXPECT_EQ (sp.get () == o, true);
  EXPECT_EQ (sp->attr (), 1);
  EXPECT_EQ (sp ? 1 : 0, 1);
  EXPECT_EQ (MyClass::instances (), 1);

  delete o;
  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(14)
{
  MyClass::reset_instance_counter ();
  MyClass *o1 = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);
  MyClass *o2 = new MyClass (2);
  EXPECT_EQ (MyClass::instances (), 2);

  std::vector<tl::shared_ptr<MyClass> > vsp;
  vsp.push_back (tl::shared_ptr<MyClass> ());
  vsp.push_back (tl::shared_ptr<MyClass> (o1));
  vsp.push_back (tl::shared_ptr<MyClass> (o1));
  vsp.push_back (tl::shared_ptr<MyClass> (o2));
  vsp.push_back (tl::shared_ptr<MyClass> ());
  vsp.push_back (tl::shared_ptr<MyClass> (o1));
  vsp.push_back (tl::shared_ptr<MyClass> (o2));
  vsp.push_back (tl::shared_ptr<MyClass> (o1));
  vsp.push_back (tl::shared_ptr<MyClass> ());
  vsp.push_back (tl::shared_ptr<MyClass> (o2));

  std::vector<tl::shared_ptr<MyClass> > vsp2;
  vsp2 = vsp;
  vsp.clear ();
  EXPECT_EQ (MyClass::instances (), 2);

  vsp2.clear ();
  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(15)
{
  MyClass::reset_instance_counter ();
  MyClass *o1 = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);
  MyClass *o2 = new MyClass (2);
  EXPECT_EQ (MyClass::instances (), 2);

  std::vector<tl::weak_ptr<MyClass> > vwp;
  vwp.push_back (tl::weak_ptr<MyClass> ());
  vwp.push_back (tl::weak_ptr<MyClass> (o1));
  vwp.push_back (tl::weak_ptr<MyClass> (o1));
  vwp.push_back (tl::weak_ptr<MyClass> (o2));
  vwp.push_back (tl::weak_ptr<MyClass> ());
  vwp.push_back (tl::weak_ptr<MyClass> (o1));
  vwp.push_back (tl::weak_ptr<MyClass> (o2));
  vwp.push_back (tl::weak_ptr<MyClass> (o1));
  vwp.push_back (tl::weak_ptr<MyClass> ());
  vwp.push_back (tl::weak_ptr<MyClass> (o2));

  std::vector<tl::weak_ptr<MyClass> > vwp2;
  vwp2 = vwp;
  vwp.clear ();
  EXPECT_EQ (MyClass::instances (), 2);

  vwp2.clear ();
  EXPECT_EQ (MyClass::instances (), 2);

  delete o1;
  delete o2;
}

TEST(20)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (17);
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_collection<MyClass> sc0;
    EXPECT_EQ (sc0.empty (), true);
    EXPECT_EQ (int (sc0.size ()), 0);

    tl::shared_collection<MyClass> sc;
    sc.push_back (o);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (int (sc.size ()), 1);
    EXPECT_EQ (sc.empty (), false);
    EXPECT_EQ (sc.front () == o, true);
    EXPECT_EQ (sc.front ()->attr (), 17);

    delete o;
    o = 0;
    EXPECT_EQ (int (sc.size ()), 0);
    EXPECT_EQ (sc.empty (), true);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(21)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_collection<MyClass> sc;
    sc.push_back (o);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (int (sc.size ()), 1);
    EXPECT_EQ (sc.empty (), false);
    EXPECT_EQ (sc.front ()->attr (), 1);

    sc.clear ();
    EXPECT_EQ (int (sc.size ()), 0);
    EXPECT_EQ (sc.empty (), true);
    EXPECT_EQ (MyClass::instances (), 0);

    sc.push_back (new MyClass (2));
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (int (sc.size ()), 1);
    EXPECT_EQ (sc.empty (), false);
    EXPECT_EQ (sc.front ()->attr (), 2);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(22)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_collection<MyClass> sc;
    sc.push_back (o);
    sc.push_back (o);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (int (sc.size ()), 2);
    EXPECT_EQ (sc.empty (), false);
    EXPECT_EQ (sc.front ()->attr (), 1);
    EXPECT_EQ ((++sc.begin ())->attr (), 1);

    sc.pop_back ();
    EXPECT_EQ (int (sc.size ()), 1);
    EXPECT_EQ (sc.empty (), false);
    EXPECT_EQ (MyClass::instances (), 1);

    sc.pop_back ();
    EXPECT_EQ (int (sc.size ()), 0);
    EXPECT_EQ (sc.empty (), true);
    EXPECT_EQ (MyClass::instances (), 0);

    MyClass *o2 = new MyClass (2);
    sc.push_back (o2);
    sc.push_back (o2);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (int (sc.size ()), 2);
    EXPECT_EQ (sc.empty (), false);
    EXPECT_EQ (sc.front ()->attr (), 2);
    EXPECT_EQ ((++sc.begin ())->attr (), 2);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(23)
{
  MyClass::reset_instance_counter ();
  MyClass *o1 = new MyClass (1);
  MyClass *o2 = new MyClass (2);
  EXPECT_EQ (MyClass::instances (), 2);

  {
    tl::shared_collection<MyClass> sc;
    EXPECT_EQ (sc.begin () == sc.end (), true);
    EXPECT_EQ (((const tl::shared_collection<MyClass> &) sc).begin () == ((const tl::shared_collection<MyClass> &) sc).end (), true);

    sc.push_back (o1);
    EXPECT_EQ (sc.begin () == sc.end (), false);
    EXPECT_EQ (sc.back () == o1, true);
    EXPECT_EQ (sc.front () == o1, true);
    EXPECT_EQ (((const tl::shared_collection<MyClass> &) sc).begin () == ((const tl::shared_collection<MyClass> &) sc).end (), false);
    sc.push_back (tl::shared_ptr<MyClass> (o2));
    EXPECT_EQ (sc.back () == o2, true);
    EXPECT_EQ (sc.front () == o1, true);
    sc.insert (sc.end (), o1);
    EXPECT_EQ (sc.back () == o1, true);
    sc.insert (sc.end (), tl::shared_ptr<MyClass> (o2));
    EXPECT_EQ (sc.back () == o2, true);

    tl::shared_collection<MyClass>::iterator inc = sc.begin ();
    EXPECT_EQ (inc == sc.end (), false);
    EXPECT_EQ (inc->attr (), 1);
    EXPECT_EQ ((*inc).attr (), 1);
    ++inc;
    EXPECT_EQ (inc == sc.end (), false);
    EXPECT_EQ (inc->attr (), 2);
    EXPECT_EQ ((*inc).attr (), 2);
    ++inc;
    EXPECT_EQ (inc == sc.end (), false);
    EXPECT_EQ (inc->attr (), 1);
    EXPECT_EQ ((*inc).attr (), 1);
    ++inc;
    EXPECT_EQ (inc == sc.end (), false);
    EXPECT_EQ (inc->attr (), 2);
    EXPECT_EQ ((*inc).attr (), 2);
    ++inc;
    EXPECT_EQ (inc == sc.end (), true);

    tl::shared_ptr<MyClass> o2t ((++sc.begin ()).operator-> ());
    sc.clear ();
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (o2t->attr (), 2);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(24)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (1);
  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_collection<MyClass> sc1;
    sc1.push_back (o);
    tl::shared_collection<MyClass> sc2;
    sc2.push_back (o);

    EXPECT_EQ (MyClass::instances (), 1);

    EXPECT_EQ (sc1.front ()->attr (), 1);
    EXPECT_EQ (int (sc1.size ()), 1);
    EXPECT_EQ (sc1.empty (), false);
    EXPECT_EQ (sc2.front ()->attr (), 1);
    EXPECT_EQ (int (sc2.size ()), 1);
    EXPECT_EQ (sc2.empty (), false);

    sc1.front ()->set_attr (42);
    EXPECT_EQ (sc1.front ()->attr (), 42);
    EXPECT_EQ (sc2.front ()->attr (), 42);

    sc1.clear ();
    sc1.push_back (new MyClass (2));
    EXPECT_EQ (sc1.front () == o, false);
    EXPECT_EQ (sc2.front () == o, true);
    EXPECT_EQ (sc1.front ()->attr (), 2);
    EXPECT_EQ (MyClass::instances (), 2);

    sc2.clear ();
    EXPECT_EQ (sc1.front ()->attr (), 2);
    EXPECT_EQ (MyClass::instances (), 1);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(30)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (17);

  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_ptr<MyClass> so (o);

    tl::weak_collection<MyClass> wc0;
    EXPECT_EQ (wc0.empty (), true);
    EXPECT_EQ (int (wc0.size ()), 0);

    tl::weak_collection<MyClass> wc;
    wc.push_back (o);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (int (wc.size ()), 1);
    EXPECT_EQ (wc.empty (), false);
    EXPECT_EQ (wc.front () == o, true);
    EXPECT_EQ (wc.front ()->attr (), 17);

    so.reset (0);

    EXPECT_EQ (int (wc.size ()), 0);
    EXPECT_EQ (wc.empty (), true);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(31)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (17);

  EXPECT_EQ (MyClass::instances (), 1);

  {
    tl::shared_ptr<MyClass> so (o);

    tl::weak_collection<MyClass> wc0;
    EXPECT_EQ (wc0.empty (), true);
    EXPECT_EQ (int (wc0.size ()), 0);

    tl::weak_collection<MyClass> wc;
    wc.push_back (o);
    EXPECT_EQ (MyClass::instances (), 1);
    EXPECT_EQ (int (wc.size ()), 1);
    EXPECT_EQ (wc.empty (), false);
    EXPECT_EQ (wc.front () == o, true);
    EXPECT_EQ (wc.front ()->attr (), 17);

    tl::shared_ptr<MyClass> so2 (new MyClass ());
    so2->set_attr (42);
    wc.push_back (so2.get ());
    wc.push_back (o);

    EXPECT_EQ (MyClass::instances (), 2);
    EXPECT_EQ (int (wc.size ()), 3);
    EXPECT_EQ (wc.empty (), false);
    tl::weak_collection<MyClass>::iterator i = wc.begin ();
    EXPECT_EQ (i.operator-> () == o, true);
    EXPECT_EQ (i->attr (), 17);
    ++i;
    EXPECT_EQ (i.operator-> () == so2.get (), true);
    EXPECT_EQ (i->attr (), 42);
    ++i;
    EXPECT_EQ (i.operator-> () == o, true);
    EXPECT_EQ (i->attr (), 17);

    so.reset (0);

    EXPECT_EQ (int (wc.size ()), 1);
    EXPECT_EQ (wc.front () == so2.get (), true);
    EXPECT_EQ (wc.front ()->attr (), 42);

    so2 = so;

    EXPECT_EQ (int (wc.size ()), 0);
    EXPECT_EQ (wc.empty (), true);
  }

  EXPECT_EQ (MyClass::instances (), 0);
}

class WCMonitor : public tl::Object
{
public:
  WCMonitor ()
    : changed_count (0), about_to_change_count (0)
  { }

  void changed () { ++changed_count; }
  void about_to_change () { ++about_to_change_count; }

  void reset ()
  {
    changed_count = 0;
    about_to_change_count = 0;
  }

  int changed_count;
  int about_to_change_count;
};

TEST(40)
{
  MyClass::reset_instance_counter ();
  MyClass *o = new MyClass (17);

  tl::shared_ptr<MyClass> so (o);
  WCMonitor wcm;

  tl::weak_collection<MyClass> wc;
  wc.about_to_change ().add (&wcm, &WCMonitor::about_to_change);
  wc.changed ().add (&wcm, &WCMonitor::changed);

  EXPECT_EQ (wc.empty (), true);
  EXPECT_EQ (int (wc.size ()), 0);

  wc.push_back (o);
  EXPECT_EQ (wcm.about_to_change_count, 1);
  EXPECT_EQ (wcm.changed_count, 1);

  so.reset (0);
  EXPECT_EQ (int (wc.size ()), 0);
  EXPECT_EQ (wcm.about_to_change_count, 2);
  EXPECT_EQ (wcm.changed_count, 2);

  wcm.reset ();

  so.reset (new MyClass (42));
  wc.push_back (so.get ());
  EXPECT_EQ (wcm.about_to_change_count, 1);
  EXPECT_EQ (wcm.changed_count, 1);

  wc.clear ();
  EXPECT_EQ (wcm.about_to_change_count, 2);
  EXPECT_EQ (wcm.changed_count, 2);

  wcm.reset ();

  so.reset (new MyClass (13));
  wc.push_back (so.get ());
  EXPECT_EQ (wcm.about_to_change_count, 1);
  EXPECT_EQ (wcm.changed_count, 1);

  wc.pop_back ();
  EXPECT_EQ (wcm.about_to_change_count, 2);
  EXPECT_EQ (wcm.changed_count, 2);

  so.reset (0);
  EXPECT_EQ (MyClass::instances (), 0);
}

TEST(41)
{
  tl::weak_collection<MyClass> wc;

  std::vector<MyClass *> objects;
  for (size_t i = 0; i < 2000000; ++i) {
    objects.push_back (new MyClass ());
    wc.push_back (objects.back ());
  }

  for (std::vector<MyClass *>::const_iterator i = objects.begin (); i != objects.end (); ++i) {
    delete *i;
  }
  objects.clear ();

  EXPECT_EQ (wc.empty (), true);
}
