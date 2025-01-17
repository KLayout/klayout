
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

#include "tlClassRegistry.h"
#include "tlUnitTest.h"

//  This namespace separates the test structs from other objects
namespace class_registry_test 
{

class X 
{
public:
  X () { }
  virtual ~X () { }
  virtual const char *name () const = 0;
};

class Y : public X
{
public:
  virtual const char *name () const { return "Y"; }
};

class Z : public X
{
public:
  virtual const char *name () const { return "Z"; }
};


class A 
{
public:
  A () { }
  virtual ~A () { }
  virtual const char *name () const = 0;
};

class B : public A
{
  virtual const char *name () const { return "B"; }
};

class C : public A
{
  virtual const char *name () const { return "C"; }
};



TEST(1) 
{
  tl::RegisteredClass<X> y (new Y ());
  tl::RegisteredClass<X> z (new Z ());

  int count = 0;
  for (tl::Registrar<X>::iterator cls = tl::Registrar<X>::begin (); cls != tl::Registrar<X>::end (); ++cls) {
    if (count == 0) { EXPECT_EQ (std::string (cls->name ()), "Z"); }
    if (count == 1) { EXPECT_EQ (std::string (cls->name ()), "Y"); }
    ++count;
  }
  EXPECT_EQ (count, 2);
}

TEST(2) 
{
  tl::RegisteredClass<A> b (new B(), 1);
  tl::RegisteredClass<A> c1 (new C(), 0);
  tl::RegisteredClass<A> c2 (new C(), 2);
  tl::RegisteredClass<A> c3 (new C(), 1);
  tl::RegisteredClass<X> y (new Y());
  tl::RegisteredClass<X> z (new Z());

  int count = 0;
  for (tl::Registrar<A>::iterator cls = tl::Registrar<A>::begin (); cls != tl::Registrar<A>::end (); ++cls) {
    if (count == 0) { EXPECT_EQ (std::string (cls->name ()), "C"); }
    if (count == 1) { EXPECT_EQ (std::string (cls->name ()), "C"); }
    if (count == 2) { EXPECT_EQ (std::string (cls->name ()), "B"); }
    if (count == 3) { EXPECT_EQ (std::string (cls->name ()), "C"); }
    ++count;
  }
  EXPECT_EQ (count, 4);
}

TEST(3) 
{
  EXPECT_EQ (tl::Registrar<A>::get_instance () == 0, true);
  EXPECT_EQ (tl::Registrar<X>::get_instance () == 0, true);
  EXPECT_EQ (tl::Registrar<A>::begin () == tl::Registrar<A>::end (), true);
  EXPECT_EQ (tl::Registrar<X>::begin () == tl::Registrar<X>::end (), true);
}

}
