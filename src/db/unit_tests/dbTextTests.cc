
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
  size_t n = db::StringRepository::instance ()->size ();

  const db::StringRef *ref = db::StringRepository::instance ()->create_string_ref ();
  db::StringRepository::change_string_ref (ref, "ABER");
  db::Text t (ref, db::Trans ());
  db::Text tt (t);

  EXPECT_EQ (std::string (t.string ()), "ABER");
  EXPECT_EQ (std::string (tt.string ()), "ABER");
  EXPECT_EQ (t == tt, true);
  EXPECT_EQ (t != tt, false);
  EXPECT_EQ (t < tt, false);
  EXPECT_EQ (tt < t, false);

  EXPECT_EQ (db::StringRepository::instance ()->size (), n + size_t (1));

  db::StringRepository::change_string_ref (ref, "NOCHWAS");
  EXPECT_EQ (std::string (t.string ()), "NOCHWAS");
  EXPECT_EQ (std::string (tt.string ()), "NOCHWAS");

  EXPECT_EQ (t == tt, true);
  EXPECT_EQ (t != tt, false);
  EXPECT_EQ (t < tt, false);
  EXPECT_EQ (tt < t, false);

  EXPECT_EQ (db::StringRepository::instance ()->size (), n + size_t (1));

  t = db::Text ();
  tt = db::Text ();

  EXPECT_EQ (db::StringRepository::instance ()->size (), n);
}

TEST(3)
{
  db::Layout ly1 (true);
  unsigned int l1 = ly1.insert_layer ();
  db::Cell *c1 = &ly1.cell (ly1.add_cell ("TOP"));

  db::Layout ly2 (true);
  unsigned int l2 = ly2.insert_layer ();
  db::Cell *c2 = &ly2.cell (ly2.add_cell ("TOP"));

  const db::StringRef *ref1 = db::StringRepository::instance ()->create_string_ref ();
  db::StringRepository::change_string_ref (ref1, "X");

  db::Text t (ref1, db::Trans ());
  db::Shape s1 = c1->shapes (l1).insert (t);
  EXPECT_EQ (std::string (s1.text_string ()), "X");
  
  db::Layout ly1dup (ly1);
  unsigned int l1dup = (*ly1dup.begin_layers ()).first;
  db::Cell *c1dup = &ly1dup.cell (ly1dup.cell_by_name ("TOP").second);
  db::Shape s1dup = *c1dup->shapes (l1dup).begin (db::ShapeIterator::All);
  EXPECT_EQ (std::string (s1dup.text_string ()), "X");

  db::StringRepository::change_string_ref (ref1, "U");
  EXPECT_EQ (std::string (s1.text_string ()), "U");
  //  NOTE: as we have a global string repo, modfiying the string reference
  //  also changes the copy:
  EXPECT_EQ (std::string (s1dup.text_string ()), "U");

  db::Shape s2a = c2->shapes (l2).insert (s1);

  db::Text tt;
  s1.text (tt);
  EXPECT_EQ (std::string (tt.string ()), "U");
  db::Shape s2b = c2->shapes (l2).insert (tt);

  EXPECT_EQ (std::string (s2a.text_string ()), "U");
  EXPECT_EQ (std::string (s2b.text_string ()), "U");

  db::StringRepository::change_string_ref (ref1, "A");
  EXPECT_EQ (std::string (tt.string ()), "U");
  EXPECT_EQ (std::string (s1.text_string ()), "A");

  EXPECT_EQ (std::string (s2a.text_string ()), "U");
  EXPECT_EQ (std::string (s2b.text_string ()), "U");
}

std::string string_trip (const db::Text &t)
{
  std::string s = t.to_string ();
  tl::Extractor ex (s.c_str ());

  db::Text t2;
  ex.read (t2);

  return t2.to_string ();
}

TEST(4)
{
  db::Text t ("abc", db::Trans (db::Trans::r90));

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0)");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.size (150);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) s=150");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.size (0);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0)");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.halign (db::HAlignCenter);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) ha=c");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.halign (db::HAlignLeft);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) ha=l");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.halign (db::HAlignRight);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) ha=r");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.valign (db::VAlignCenter);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) ha=r va=c");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.valign (db::VAlignTop);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) ha=r va=t");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.valign (db::VAlignBottom);

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) ha=r va=b");
  EXPECT_EQ (string_trip (t), t.to_string ());

  t.halign (db::NoHAlign);
  t.valign (db::NoVAlign);
  t.font (db::Font (17));

  EXPECT_EQ (t.to_string (), "('abc',r90 0,0) f=17");
  EXPECT_EQ (string_trip (t), t.to_string ());
}
