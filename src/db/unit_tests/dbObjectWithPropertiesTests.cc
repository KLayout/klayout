
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


#include "dbObjectWithProperties.h"
#include "tlUnitTest.h"

namespace {

TEST(1) 
{
  db::PropertiesSet ps;
  ps.insert (tl::Variant (1), tl::Variant ("one"));
  ps.insert (tl::Variant ("key"), tl::Variant (42.0));

  db::PolygonWithProperties pwp (db::Polygon (db::Box (0, 0, 100, 200)), db::properties_id (ps));

  EXPECT_EQ (pwp.to_string (), "(0,0;0,200;100,200;100,0) props={1=>one,key=>42}");

  db::PolygonWithProperties pwp2;

  std::string s;
  tl::Extractor ex (s.c_str ());

  EXPECT_EQ (ex.try_read (pwp2), false);

  s = "  (0,0;0,200;100,200;100,0)  props= {1 => \"one\", key => 42} ";
  ex = tl::Extractor (s.c_str ());

  EXPECT_EQ (ex.try_read (pwp2), true);
  EXPECT_EQ (pwp2.to_string (), "(0,0;0,200;100,200;100,0) props={1=>one,key=>42}");
}

}
