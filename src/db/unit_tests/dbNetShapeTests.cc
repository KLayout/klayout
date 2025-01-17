
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


#include "dbNetShape.h"
#include "dbShapes.h"
#include "tlUnitTest.h"


TEST(1)
{
  db::GenericRepository repo;

  db::NetShape s;
  EXPECT_EQ (s.type () == db::NetShape::None, true);
  EXPECT_EQ (s.bbox ().to_string (), "()");

  db::Polygon p (db::Box (0, 0, 100, 200));
  s = db::NetShape (p, repo);
  EXPECT_EQ (s.type () == db::NetShape::Polygon, true);
  EXPECT_EQ (s.bbox ().to_string (), "(0,0;100,200)");
  EXPECT_EQ (s.polygon_ref ().obj ().to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (db::NetShape (s.polygon_ref ()).type () == db::NetShape::Polygon, true);
  EXPECT_EQ (db::NetShape (s.polygon_ref ()).polygon_ref ().obj ().to_string (), "(0,0;0,200;100,200;100,0)");

  db::Text t ("abc", db::Trans (db::Vector (100, 200)));
  s = db::NetShape (t, repo);
  EXPECT_EQ (s.type () == db::NetShape::Text, true);
  EXPECT_EQ (s.bbox ().to_string (), "(100,200;100,200)");
  EXPECT_EQ (s.text_ref ().obj ().to_string (), "('abc',r0 0,0)");
  EXPECT_EQ (db::NetShape (s.text_ref ()).type () == db::NetShape::Text, true);
  EXPECT_EQ (db::NetShape (s.text_ref ()).text_ref ().obj ().to_string (), "('abc',r0 0,0)");
}

TEST(2)
{
  db::GenericRepository repo;

  db::NetShape s;
  EXPECT_EQ (s.type () == db::NetShape::None, true);
  EXPECT_EQ (s.bbox ().to_string (), "()");

  db::Polygon p (db::Box (0, 0, 100, 200));
  s = db::NetShape (p, repo);
  EXPECT_EQ (s.polygon_ref ().obj ().to_string (), "(0,0;0,200;100,200;100,0)");

  s.transform (db::Disp (db::Vector (10, 20)));
  EXPECT_EQ (s.bbox ().to_string (), "(10,20;110,220)");

  db::Text t ("abc", db::Trans (db::Vector (100, 200)));
  s = db::NetShape (t, repo);
  EXPECT_EQ (s.text_ref ().obj ().to_string (), "('abc',r0 0,0)");

  s.transform (db::Disp (db::Vector (10, 20)));
  EXPECT_EQ (s.text_ref ().obj ().transformed (s.text_ref ().trans ()).to_string (), "('abc',r0 110,220)");
}

TEST(3)
{
  db::GenericRepository repo;

  db::NetShape s, s2;
  EXPECT_EQ (s == db::NetShape (), true);
  EXPECT_EQ (s != db::NetShape (), false);
  EXPECT_EQ (s < db::NetShape (), false);

  db::Polygon p (db::Box (0, 0, 100, 200));
  s = db::NetShape (p, repo);
  s2 = s;
  EXPECT_EQ (s == db::NetShape (), false);
  EXPECT_EQ (s != db::NetShape (), true);
  EXPECT_EQ (s < db::NetShape (), false);
  EXPECT_EQ (s == s2, true);
  EXPECT_EQ (s != s2, false);
  EXPECT_EQ (s < s2, false);
  EXPECT_EQ (s2 < s, false);
  s.transform (db::Disp (db::Vector (10, 20)));
  EXPECT_EQ (s == s2, false);
  EXPECT_EQ (s != s2, true);
  EXPECT_EQ ((s < s2) != (s2 < s), true);

  db::Text t ("abc", db::Trans (db::Vector (100, 200)));
  s = db::NetShape (t, repo);
  EXPECT_EQ (s == s2, false);
  EXPECT_EQ (s != s2, true);
  EXPECT_EQ ((s < s2) != (s2 < s), true);

  s2 = s;
  EXPECT_EQ (s == db::NetShape (), false);
  EXPECT_EQ (s != db::NetShape (), true);
  EXPECT_EQ (s < db::NetShape (), false);
  EXPECT_EQ (s == s2, true);
  EXPECT_EQ (s != s2, false);
  EXPECT_EQ (s < s2, false);
  EXPECT_EQ (s2 < s, false);
  s.transform (db::Disp (db::Vector (10, 20)));
  EXPECT_EQ (s == s2, false);
  EXPECT_EQ (s != s2, true);
  EXPECT_EQ ((s < s2) != (s2 < s), true);
}

TEST(4)
{
  db::GenericRepository repo;

  db::NetShape s;
  EXPECT_EQ (s.type () == db::NetShape::None, true);
  EXPECT_EQ (s.bbox ().to_string (), "()");

  db::Polygon p (db::Box (0, 0, 100, 200));
  s = db::NetShape (p, repo);

  db::Shapes shapes;
  s.insert_into (shapes);

  db::Text t ("abc", db::Trans (db::Vector (100, 200)));
  s = db::NetShape (t, repo);
  s.insert_into (shapes);

  db::ShapeIterator si = shapes.begin (db::ShapeIterator::All);
  EXPECT_EQ (si->to_string (), "polygon (0,0;0,200;100,200;100,0)");
  ++si;
  EXPECT_NE (si.at_end (), true);
  EXPECT_EQ (si->to_string (), "text ('abc',r0 100,200)");
  ++si;
  EXPECT_EQ (si.at_end (), true);
}

TEST(5)
{
  db::GenericRepository repo;

  db::NetShape sp1 (db::Polygon (db::Box (10, 20, 100, 200)), repo);
  db::NetShape sp2 (db::Polygon (db::Box (80, 20, 180, 200)), repo);
  db::NetShape sp3 (db::Polygon (db::Box (10, 320, 100, 500)), repo);

  db::NetShape st1 (db::Text ("abc", db::Trans (db::Vector (0, 0))), repo);
  db::NetShape st2 (db::Text ("xyz", db::Trans (db::Vector (50, 60))), repo);

  EXPECT_EQ (sp1.interacts_with (db::NetShape ()), false);
  EXPECT_EQ (sp1.interacts_with_transformed (db::NetShape (), db::Trans (db::Vector (1000, 0))), false);
  EXPECT_EQ (sp1.interacts_with (sp1), true);
  EXPECT_EQ (sp1.interacts_with_transformed (sp1, db::Trans (db::Vector (1000, 0))), false);
  EXPECT_EQ (sp1.interacts_with (sp2), true);
  EXPECT_EQ (sp2.interacts_with (sp1), true);
  EXPECT_EQ (sp1.interacts_with (sp3), false);
  EXPECT_EQ (sp1.interacts_with_transformed (sp3, db::Trans (db::Vector (50, -200))), true);
  EXPECT_EQ (sp3.interacts_with (sp1), false);
  EXPECT_EQ (sp3.interacts_with_transformed (sp1, db::Trans (db::Vector (50, 200))), true);

  EXPECT_EQ (sp1.interacts_with (st1), false);
  EXPECT_EQ (sp1.interacts_with_transformed (st1, db::Trans (db::Vector (10, 20))), true);
  EXPECT_EQ (sp1.interacts_with_transformed (st1, db::Trans (db::Vector (5, 20))), false);
  EXPECT_EQ (sp1.interacts_with (st2), true);

  EXPECT_EQ (st1.interacts_with (sp1), false);
  EXPECT_EQ (st1.interacts_with_transformed (sp1, db::Trans (db::Vector (-10, -20))), true);
  EXPECT_EQ (st1.interacts_with_transformed (sp1, db::Trans (db::Vector (-5, -20))), false);
  EXPECT_EQ (st2.interacts_with (sp1), true);

  EXPECT_EQ (st1.interacts_with (st1), true);
  EXPECT_EQ (st1.interacts_with_transformed (st1, db::Trans (db::Vector (-5, -20))), false);
  EXPECT_EQ (st2.interacts_with (st1), false);
  EXPECT_EQ (st2.interacts_with_transformed (st1, db::Trans (db::Vector (50, 60))), true);
}
