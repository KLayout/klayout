
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

#include "layAbstractMenu.h"

#include "tlUnitTest.h"

std::string node_to_string (const lay::AbstractMenuItem &item)
{
  std::string s = item.name ();
  if (! item.children.empty ()) {
    s += "(";
    for (std::list<lay::AbstractMenuItem>::const_iterator c = item.children.begin (); c != item.children.end (); ++c) {
      if (c != item.children.begin ()) {
        s += ",";
      }
      s += node_to_string (*c);
    }
    s += ")";
  }
  return s;
}

std::string menu_to_string (const lay::AbstractMenu &menu)
{
  return node_to_string (menu.root ());
}

TEST(1)
{
  lay::AbstractMenu menu (0);
  EXPECT_EQ (menu_to_string (menu), "");

  try {
    EXPECT_EQ (menu.action ("n1").get_title (), "");
    EXPECT_EQ (true, false);
  } catch (...) {
  }

  EXPECT_EQ (menu.is_valid ("n1"), false);

  menu.insert_menu ("end", "n1", lay::Action ("title:n1"));
  EXPECT_EQ (menu_to_string (menu), "(n1)");
  EXPECT_EQ (tl::join (menu.items (""), ","), "n1");
  EXPECT_EQ (menu.is_menu ("n1"), true);
  EXPECT_EQ (menu.action ("n1").get_title (), "title:n1");

  EXPECT_EQ (menu.is_valid ("n1"), true);
  EXPECT_EQ (menu.is_valid ("n2"), false);

  menu.insert_menu ("end", "n2", lay::Action ("title:n2"));
  EXPECT_EQ (menu_to_string (menu), "(n1,n2)");
  EXPECT_EQ (tl::join (menu.items (""), ","), "n1,n2");
  EXPECT_EQ (menu.is_menu ("n2"), true);
  EXPECT_EQ (menu.action ("n2").get_title (), "title:n2");

  EXPECT_EQ (menu.is_valid ("n2"), true);

  menu.insert_menu ("end", "n1", lay::Action ("title:n1"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1)");
  EXPECT_EQ (menu.is_menu ("n1"), true);
  EXPECT_EQ (menu.action ("n1").get_title (), "title:n1");

  menu.insert_item ("n1.begin", "c1", lay::Action ("title:c1"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c1))");
  EXPECT_EQ (tl::join (menu.items ("n1"), ","), "n1.c1");
  EXPECT_EQ (menu.action ("n1.c1").get_title (), "title:c1");

  menu.insert_item ("n1.end", "c2", lay::Action ("title:c2"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c1,n1.c2))");
  EXPECT_EQ (tl::join (menu.items ("n1"), ","), "n1.c1,n1.c2");
  EXPECT_EQ (menu.is_menu ("n1.c2"), false);
  EXPECT_EQ (menu.action ("n1.c2").get_title (), "title:c2");

  menu.insert_item ("n1.begin", "c1", lay::Action ("title:c1a"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c1,n1.c2))");
  EXPECT_EQ (tl::join (menu.items ("n1"), ","), "n1.c1,n1.c2");
  EXPECT_EQ (menu.action ("n1.c1").get_title (), "title:c1a");

  menu.insert_item ("n1.c1", "c3", lay::Action ("title:c3"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c3,n1.c1,n1.c2))");

  menu.insert_item ("n1.c1+", "c4", lay::Action ("title:c4"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c3,n1.c1,n1.c4,n1.c2))");
  EXPECT_EQ (menu.action ("n1.c4").get_title (), "title:c4");

  menu.delete_item ("n1.c1");
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c3,n1.c4,n1.c2))");

  menu.delete_item ("n1");
  EXPECT_EQ (menu_to_string (menu), "(n2)");

  menu.insert_item ("n1>end(title).end", "c1", lay::Action ("title:c1"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c1))");
  EXPECT_EQ (menu.action ("n1.c1").get_title (), "title:c1");

  menu.insert_item ("n1>end(title).end", "c2", lay::Action ("title:c2"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c1,n1.c2))");

  menu.delete_item ("n1.c1");
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c2))");

  menu.delete_item ("n1.c1");
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c2))");

  menu.delete_item ("n1.c2");
  EXPECT_EQ (menu_to_string (menu), "(n2)");

  menu.clear_menu ("n1");
  EXPECT_EQ (menu_to_string (menu), "(n2)");

  menu.insert_menu ("end", "n1", lay::Action ("title:n1"));
  menu.insert_item ("n1.begin", "c1", lay::Action ("title:c1"));
  menu.insert_item ("n1.end", "c2", lay::Action ("title:c2"));
  EXPECT_EQ (menu_to_string (menu), "(n2,n1(n1.c1,n1.c2))");
  menu.clear_menu ("n1");
  EXPECT_EQ (menu_to_string (menu), "(n2,n1)");
}
