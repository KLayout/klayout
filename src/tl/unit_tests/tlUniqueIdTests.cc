
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

#include "tlUniqueId.h"
#include "tlUnitTest.h"

#include <memory>

namespace {
  class MyClass : public tl::UniqueId {
  public:
    MyClass () { }
  };
}

//  basic parsing ability
TEST(1)
{
  tl::id_type id, id0;

  id = tl::id_of (0);
  EXPECT_EQ (id, tl::id_type (0));

  std::unique_ptr<MyClass> ptr;
  ptr.reset (new MyClass ());
  id0 = id = tl::id_of (ptr.get ());
  EXPECT_NE (id, tl::id_type (0));

  ptr.reset (new MyClass ());
  id = tl::id_of (ptr.get ());
  EXPECT_EQ (id, id0 + 1);
}
