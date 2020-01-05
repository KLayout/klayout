
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "dbText.h"
#include "dbLayout.h"
#include "tlUnitTest.h"

TEST(1)
{
  db::Text t;
  db::Text empty;
  db::Trans t1 (1, true, db::Vector (0, 0));
  db::Trans t2 (-1, false, db::Vector (200, 100));

  EXPECT_EQ (empty == t, true);

  EXPECT_EQ (std::string (t.string ()), ""); 
  t = db::Text ("abcdef", t1);

  EXPECT_EQ (std::string (t.string ()), "abcdef"); 
  EXPECT_EQ (t.trans (),  t1);
  
  t.transform (t2);

  EXPECT_EQ (t.trans (), t2 * t1);

  EXPECT_EQ (std::string (t.to_string ()), "('abcdef',m0 200,100)");

  db::DText dt (t.string (), db::DTrans (t.trans ()));
  EXPECT_EQ (std::string (dt.to_string ()), "('abcdef',m0 200,100)");

  db::Text it = db::Text (dt);
  EXPECT_EQ (std::string (it.to_string ()), "('abcdef',m0 200,100)")
}

TEST(2)
{
  db::StringRepository rep;

  const db::StringRef *ref = rep.create_string_ref ();
  rep.change_string_ref (ref, "ABER");
  db::Text t (ref, db::Trans ());
  db::Text tt (t);

  EXPECT_EQ (std::string (t.string ()), "ABER");
  EXPECT_EQ (std::string (tt.string ()), "ABER");
  EXPECT_EQ (t == tt, true);
  EXPECT_EQ (t != tt, false);
  EXPECT_EQ (t < tt, false);
  EXPECT_EQ (tt < t, false);

  EXPECT_EQ (rep.size (), size_t (1));

  rep.change_string_ref (ref, "NOCHWAS");
  EXPECT_EQ (std::string (t.string ()), "NOCHWAS");
  EXPECT_EQ (std::string (tt.string ()), "NOCHWAS");

  EXPECT_EQ (t == tt, true);
  EXPECT_EQ (t != tt, false);
  EXPECT_EQ (t < tt, false);
  EXPECT_EQ (tt < t, false);

  EXPECT_EQ (rep.size (), size_t (1));

  t = db::Text ();
  tt = db::Text ();

  EXPECT_EQ (rep.size (), size_t (0));
}

TEST(3)
{
  db::Layout ly1 (true);
  unsigned int l1 = ly1.insert_layer ();
  db::Cell *c1 = &ly1.cell (ly1.add_cell ("TOP"));

  db::Layout ly2 (true);
  unsigned int l2 = ly2.insert_layer ();
  db::Cell *c2 = &ly2.cell (ly2.add_cell ("TOP"));

  const db::StringRef *ref1 = ly1.string_repository ().create_string_ref ();
  ly1.string_repository ().change_string_ref (ref1, "X");

  db::Text t (ref1, db::Trans ());
  db::Shape s1 = c1->shapes (l1).insert (t);
  EXPECT_EQ (std::string (s1.text_string ()), "X");
  
  db::Layout ly1dup (ly1);
  unsigned int l1dup = (*ly1dup.begin_layers ()).first;
  db::Cell *c1dup = &ly1dup.cell (ly1dup.cell_by_name ("TOP").second);
  db::Shape s1dup = *c1dup->shapes (l1dup).begin (db::ShapeIterator::All);
  EXPECT_EQ (std::string (s1dup.text_string ()), "X");

  ly1.string_repository ().change_string_ref (ref1, "U");
  EXPECT_EQ (std::string (s1.text_string ()), "U");
  EXPECT_EQ (std::string (s1dup.text_string ()), "X");

  db::Shape s2a = c2->shapes (l2).insert (s1);

  db::Text tt;
  s1.text (tt);
  EXPECT_EQ (std::string (tt.string ()), "U");
  db::Shape s2b = c2->shapes (l2).insert (tt);

  EXPECT_EQ (std::string (s2a.text_string ()), "U");
  EXPECT_EQ (std::string (s2b.text_string ()), "U");

  ly1.string_repository ().change_string_ref (ref1, "A");
  EXPECT_EQ (std::string (tt.string ()), "U");
  EXPECT_EQ (std::string (s1.text_string ()), "A");

  EXPECT_EQ (std::string (s2a.text_string ()), "U");
  EXPECT_EQ (std::string (s2b.text_string ()), "U");
}


