
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

#include "tlRecipe.h"
#include "tlUnitTest.h"

#include <memory>

namespace {

  class MyExecutable : public tl::ExecutableWithParameters
  {
  public:
    static bool cleanup_called;

    MyExecutable (const std::map<std::string, tl::Variant> &params)
      : tl::ExecutableWithParameters (params)
    { }

    tl::Variant execute ()
    {
      int a = get_value (parameters (), "A", 0);
      double b = get_value (parameters (), "B", 0.0);
      double c = get_value (parameters (), "C", 1.0);
      bool crash = get_value (parameters (), "X", false);
      if (crash) {
        throw tl::Exception ("crashed");
      }
      return tl::Variant (b * a * c);
    }

    void cleanup ()
    {
      cleanup_called = true;
    }
  };

  bool MyExecutable::cleanup_called = false;

  class MyRecipe : public tl::Recipe
  {
  public:
    MyRecipe () : tl::Recipe ("test_recipe", "description") { }
    tl::Executable *executable (const std::map<std::string, tl::Variant> &params) const { return new MyExecutable (params); }
  };

  static MyRecipe my_recipe;

}

//  basic abilities
TEST(1)
{
  std::map<std::string, tl::Variant> params;
  params["A"] = tl::Variant (7);
  params["B"] = tl::Variant (6.0);
  std::string g = my_recipe.generator (params);
  EXPECT_EQ (g, "test_recipe: A=#7,B=##6");

  MyExecutable::cleanup_called = false;
  tl::Variant res = tl::Recipe::make (g);
  EXPECT_EQ (res.to_double (), 42.0);
  EXPECT_EQ (MyExecutable::cleanup_called, true);

  std::map<std::string, tl::Variant> padd;
  padd["C"] = tl::Variant(1.5);
  res = tl::Recipe::make (g, padd);
  EXPECT_EQ (res.to_double (), 63.0);

  MyExecutable::cleanup_called = false;
  padd.clear ();
  padd["X"] = tl::Variant(true);
  try {
    res = tl::Recipe::make (g, padd);
    EXPECT_EQ (1, 0);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "crashed");
  }
  EXPECT_EQ (MyExecutable::cleanup_called, true);
}
