
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

#include "tlCopyOnWrite.h"
#include "tlUnitTest.h"

//  This namespace separates the test structs from other objects
namespace copy_on_write_tests
{

static int x_instances = 0;

class X
{
public:
  X () { ++x_instances; }
  virtual ~X () { --x_instances; }
  virtual const char *name () const = 0;
  virtual X *clone () const = 0;
};

class Y : public X
{
public:
  virtual const char *name () const { return "Y"; }
  virtual X *clone () const { return new Y (); }
};

class Z : public X
{
public:
  virtual const char *name () const { return "Z"; }
  virtual X *clone () const { return new Z (); }
};

static int a_instances = 0;

class A
{
public:
  A () { ++a_instances; }
  A (const A &) { ++a_instances; }
  ~A () { --a_instances; }
  const char *name () const { return "A"; }
};

}

TEST(1)
{
  tl::copy_on_write_ptr<copy_on_write_tests::A> ptr1, ptr2;

  EXPECT_EQ (copy_on_write_tests::a_instances, 0);

  ptr1.reset (new copy_on_write_tests::A ());
  EXPECT_EQ (copy_on_write_tests::a_instances, 1);

  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");
  EXPECT_EQ (ptr1.get_non_const ()->name (), "A");
  EXPECT_EQ (ptr1.ref_count (), 1);

  ptr2 = ptr1;

  EXPECT_EQ (ptr1.ref_count (), 2);
  EXPECT_EQ (ptr2.ref_count (), 2);
  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), true);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");

  EXPECT_EQ (ptr1.get_non_const ()->name (), "A");
  EXPECT_EQ (copy_on_write_tests::a_instances, 2);

  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr2.ref_count (), 1);

  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), false);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");
  EXPECT_EQ (ptr2.get_const ()->name (), "A");

  ptr1.reset (0);
  EXPECT_EQ (copy_on_write_tests::a_instances, 1);
  ptr2.reset (0);
  EXPECT_EQ (copy_on_write_tests::a_instances, 0);
}

TEST(2)
{
  tl::copy_on_write_ptr<copy_on_write_tests::A> ptr1;

  EXPECT_EQ (copy_on_write_tests::a_instances, 0);

  ptr1.reset (new copy_on_write_tests::A ());
  EXPECT_EQ (copy_on_write_tests::a_instances, 1);

  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");
  EXPECT_EQ (ptr1.get_non_const ()->name (), "A");
  EXPECT_EQ (ptr1.ref_count (), 1);

  tl::copy_on_write_ptr<copy_on_write_tests::A> ptr2 (ptr1);

  EXPECT_EQ (ptr1.ref_count (), 2);
  EXPECT_EQ (ptr2.ref_count (), 2);
  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), true);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");

  EXPECT_EQ (ptr1.get_non_const ()->name (), "A");

  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr2.ref_count (), 1);

  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), false);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");
  EXPECT_EQ (ptr2.get_const ()->name (), "A");
}

TEST(3)
{
  tl::copy_on_write_ptr<copy_on_write_tests::A> ptr1;

  EXPECT_EQ (copy_on_write_tests::a_instances, 0);

  ptr1.reset (new copy_on_write_tests::A ());
  EXPECT_EQ (copy_on_write_tests::a_instances, 1);
  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");
  EXPECT_EQ (ptr1.get_non_const ()->name (), "A");
  EXPECT_EQ (ptr1.ref_count (), 1);

  tl::copy_on_write_ptr<copy_on_write_tests::A> ptr2 (ptr1);

  EXPECT_EQ (ptr1.ref_count (), 2);
  EXPECT_EQ (ptr2.ref_count (), 2);
  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), true);
  EXPECT_EQ (ptr1.get_const ()->name (), "A");

  ptr2.reset (0);
  EXPECT_EQ (copy_on_write_tests::a_instances, 1);

  EXPECT_EQ (ptr1.get_non_const ()->name (), "A");

  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr2.ref_count (), 0);

  EXPECT_EQ (ptr1.get_const () == 0, false);
  EXPECT_EQ (ptr2.get_const () == 0, true);
  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), false);

  ptr1.reset (0);
  EXPECT_EQ (ptr1.ref_count (), 0);
  EXPECT_EQ (copy_on_write_tests::a_instances, 0);
}

TEST(4)
{
  tl::copy_on_write_ptr<copy_on_write_tests::X, tl::clone_duplicator<copy_on_write_tests::X> > ptr1, ptr2;

  EXPECT_EQ (copy_on_write_tests::x_instances, 0);

  ptr1.reset (new copy_on_write_tests::Y ());
  EXPECT_EQ (copy_on_write_tests::x_instances, 1);

  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr1.get_const ()->name (), "Y");
  EXPECT_EQ (ptr1.get_non_const ()->name (), "Y");
  EXPECT_EQ (ptr1.ref_count (), 1);

  ptr2 = ptr1;

  EXPECT_EQ (ptr1.ref_count (), 2);
  EXPECT_EQ (ptr2.ref_count (), 2);
  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), true);
  EXPECT_EQ (ptr1.get_const ()->name (), "Y");

  EXPECT_EQ (ptr1.get_non_const ()->name (), "Y");
  EXPECT_EQ (copy_on_write_tests::x_instances, 2);

  EXPECT_EQ (ptr1.ref_count (), 1);
  EXPECT_EQ (ptr2.ref_count (), 1);

  EXPECT_EQ (ptr1.get_const () == ptr2.get_const (), false);
  EXPECT_EQ (ptr1.get_const ()->name (), "Y");
  EXPECT_EQ (ptr2.get_const ()->name (), "Y");

  ptr1.reset (0);
  EXPECT_EQ (copy_on_write_tests::x_instances, 1);
  ptr2.reset (new copy_on_write_tests::Z ());
  EXPECT_EQ (copy_on_write_tests::x_instances, 1);
  EXPECT_EQ (ptr2.get_const ()->name (), "Z");

  ptr2.reset (0);
  EXPECT_EQ (copy_on_write_tests::x_instances, 0);
}

