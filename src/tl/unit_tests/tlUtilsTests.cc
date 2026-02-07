
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

#include "tlUtils.h"
#include "tlUnitTest.h"

namespace
{
  class A { };
  class B : public A { };
  class D : public B { };
  class C : public A { };
  class E { };
}

struct XXX
{
  int a, b;
};

class XX : public XXX, private tl::Object { };

TEST(1)
{
  EXPECT_EQ (tl::value_from_type (tl::type_from_value<false>::value ()), false);
  EXPECT_EQ (tl::value_from_type (tl::type_from_value<true>::value ()), true);

  EXPECT_EQ (tl::value_from_type (tl::is_derived<A, A>::value ()), true);
  EXPECT_EQ (tl::value_from_type (tl::is_derived<A, E>::value ()), false);
  EXPECT_EQ (tl::value_from_type (tl::is_derived<A, B>::value ()), true);
  EXPECT_EQ (tl::value_from_type (tl::is_derived<A, C>::value ()), true);
  EXPECT_EQ (tl::value_from_type (tl::is_derived<A, D>::value ()), true);
  EXPECT_EQ (tl::value_from_type (tl::is_derived<B, C>::value ()), false);
  EXPECT_EQ (bool (tl::is_derived<A, D> ()), true);
  EXPECT_EQ (bool (tl::is_derived<B, C> ()), false);

  EXPECT_EQ (tl::value_from_type (tl::is_equal_type<A, D>::value ()), false);
  EXPECT_EQ (tl::value_from_type (tl::is_equal_type<A, A>::value ()), true);
  EXPECT_EQ (bool (tl::is_equal_type<A, D> ()), false);
  EXPECT_EQ (bool (tl::is_equal_type<A, A> ()), true);

  A a;
  B b;
  C c;
  EXPECT_EQ (tl::try_static_cast<A> (&a) == &a, true);
  EXPECT_EQ (tl::try_static_cast<A> (&b) == (A *)&b, true);
  EXPECT_EQ (tl::try_static_cast<A> (&c) == (A *)&c, true);
  EXPECT_EQ (tl::try_static_cast<B> (&c) == 0, true);
}
