
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


#include "dbPropertiesRepository.h"
#include "tlString.h"
#include "tlUnitTest.h"


TEST(1) 
{
  tl::Variant v;

  EXPECT_EQ (std::string (v.to_parsable_string ()) == "nil", true);

  v = 1l;
  EXPECT_EQ (std::string (v.to_parsable_string ()) == "#1", true);
  EXPECT_EQ (v.to_long () == 1, true);

  v = "102";
  EXPECT_EQ (std::string (v.to_parsable_string ()) == "'102'", true);

  v = 2l;
  EXPECT_EQ (std::string (v.to_parsable_string ()) == "#2", true);
  EXPECT_EQ (v.is_long (), true);
  EXPECT_EQ (v.is_double (), false);
  EXPECT_EQ (v.is_a_string (), false);
  EXPECT_EQ (v.to_long () == 2, true);
  EXPECT_EQ (v.to_double () == 2, true);

  v = tl::Variant ();
  EXPECT_EQ (std::string (v.to_parsable_string ()) == "nil", true);
  EXPECT_EQ (v.is_double (), false);
  EXPECT_EQ (v.is_a_string (), false);
  EXPECT_EQ (v.is_long (), false);
  EXPECT_EQ (v.is_nil (), true);
  EXPECT_EQ (v.is_list (), false);

  v = tl::Variant ((long) 1);
  EXPECT_EQ (std::string (v.to_parsable_string ()) == "#1", true);
  EXPECT_EQ (v.to_long () == 1, true);
  EXPECT_EQ (v.to_double () == 1, true);
  EXPECT_EQ (v.is_double (), false);
  EXPECT_EQ (v.is_a_string (), false);

  v = tl::Variant ("A");
  EXPECT_EQ (std::string (v.to_parsable_string ()) == "'A'", true);
  EXPECT_EQ (v.is_double (), false);
  EXPECT_EQ (v.is_a_string (), true);
  EXPECT_EQ (v.is_long (), false);

  EXPECT_EQ (v < tl::Variant (), false);
  EXPECT_EQ (tl::Variant (1l) < v, true);
  EXPECT_EQ (tl::Variant ("B") < v, false);
  EXPECT_EQ (tl::Variant ("A") < v, false);
  EXPECT_EQ (tl::Variant (" ") < v, true);
}

TEST(2) 
{
  db::PropertiesRepository rep;

  tl::Variant n1 ("Hallo");
  tl::Variant n2 ("AAA");

  size_t id1 = rep.prop_name_id (n1);
  size_t id2 = rep.prop_name_id (n2);

  EXPECT_EQ (id1, size_t (0));
  EXPECT_EQ (id2, size_t (1));

  id2 = rep.prop_name_id (n2);
  id1 = rep.prop_name_id (n1);

  EXPECT_EQ (id1, size_t (0));
  EXPECT_EQ (id2, size_t (1));

  EXPECT_EQ (rep.prop_name (0) == n1, true);
  EXPECT_EQ (rep.prop_name (1) == n2, true);

  db::PropertiesRepository rep2;
  rep2 = rep;

  id2 = rep2.prop_name_id (n2);
  id1 = rep2.prop_name_id (n1);

  EXPECT_EQ (id1, size_t (0));
  EXPECT_EQ (id2, size_t (1));

  EXPECT_EQ (rep2.prop_name (0) == n1, true);
  EXPECT_EQ (rep2.prop_name (1) == n2, true);

  db::PropertiesRepository empty_rep;
  rep2 = empty_rep;

  id1 = rep2.prop_name_id (n2);
  EXPECT_EQ (id1, size_t (0));
}

TEST(3) 
{
  db::PropertiesRepository rep;

  tl::Variant n1 (1.5);
  tl::Variant n2 ("AAA");

  size_t id1 = rep.prop_name_id (n1);
  size_t id2 = rep.prop_name_id (n2);

  EXPECT_EQ (id1, size_t (0));
  EXPECT_EQ (id2, size_t (1));

  id2 = rep.prop_name_id (n2);
  id1 = rep.prop_name_id (n1);

  EXPECT_EQ (id1, size_t (0));
  EXPECT_EQ (id2, size_t (1));

  EXPECT_EQ (rep.prop_name (0) == n1, true);
  EXPECT_EQ (rep.prop_name (1) == n2, true);

  db::PropertiesRepository rep2;
  rep2 = rep;

  id2 = rep2.prop_name_id (n2);
  id1 = rep2.prop_name_id (n1);

  EXPECT_EQ (id1, size_t (0));
  EXPECT_EQ (id2, size_t (1));

  EXPECT_EQ (rep2.prop_name (0) == n1, true);
  EXPECT_EQ (rep2.prop_name (1) == n2, true);

  db::PropertiesRepository empty_rep;
  rep2 = empty_rep;

  id1 = rep2.prop_name_id (n2);
  EXPECT_EQ (id1, size_t (0));
}

TEST(4) 
{
  db::PropertiesRepository::properties_set set1;
  db::PropertiesRepository::properties_set set2;

  set1.insert (std::make_pair (0, tl::Variant (15l)));
  set1.insert (std::make_pair (10, tl::Variant (0.125)));

  set2.insert (std::make_pair (0, tl::Variant (15l)));
  set2.insert (std::make_pair (9, tl::Variant ("Ein String")));

  db::PropertiesRepository rep;

  size_t id1 = rep.properties_id (set1);
  size_t id2 = rep.properties_id (set2);

  EXPECT_EQ (id1, size_t (1));
  EXPECT_EQ (id2, size_t (2));

  id2 = rep.properties_id (set2);
  id1 = rep.properties_id (set1);

  EXPECT_EQ (id1, size_t (1));
  EXPECT_EQ (id2, size_t (2));

  EXPECT_EQ (rep.properties (1) == set1, true);
  EXPECT_EQ (rep.properties (2) == set2, true);

  db::PropertiesRepository rep2;
  rep2 = rep;

  id2 = rep2.properties_id (set2);
  id1 = rep2.properties_id (set1);

  EXPECT_EQ (id1, size_t (1));
  EXPECT_EQ (id2, size_t (2));

  EXPECT_EQ (rep2.properties (1) == set1, true);
  EXPECT_EQ (rep2.properties (2) == set2, true);

  db::PropertiesRepository empty_rep;
  rep2 = empty_rep;

  id1 = rep2.properties_id (set2);
  EXPECT_EQ (id1, size_t (1));
}

TEST(5) 
{
  tl::Variant v;
  tl::Extractor ex ("  #10 a");
  ex.read (v);
  ex.expect ("a");
  EXPECT_EQ (v == tl::Variant ((long) 10), true);
  ex = tl::Extractor ("  ##  12.5 a");
  ex.read (v);
  ex.expect ("a");
  EXPECT_EQ (v == tl::Variant (12.5), true);
  ex = tl::Extractor ("  Aber a");
  ex.read (v);
  ex.expect ("a");
  EXPECT_EQ (v == tl::Variant ("Aber"), true);
  ex = tl::Extractor ("  Aber  a");
  ex.read (v);
  ex.expect ("a");
  EXPECT_EQ (v == tl::Variant ("Aber"), true);
  ex = tl::Extractor (" (Aber_, ##2.500, (#05,x)  ,() )  a");
  ex.read (v);
  ex.expect ("a");
  EXPECT_EQ (std::string (v.to_parsable_string ()), "('Aber_',##2.5,(#5,'x'),())");
}

TEST(6) 
{
  db::PropertiesRepository rep;

  tl::Variant n1(1);
  tl::Variant n2(1l);

  EXPECT_EQ (n1 == n2, true);
  EXPECT_EQ (n1 < n2, false);
  EXPECT_EQ (n2 < n1, false);

  size_t id1 = rep.prop_name_id (n1);
  size_t id2 = rep.prop_name_id (n2);

  EXPECT_EQ (id1, size_t (0));
  EXPECT_EQ (id2, size_t (0));

  db::PropertiesRepository::properties_set set1;
  db::PropertiesRepository::properties_set set2;

  set1.insert (std::make_pair (0, tl::Variant ("JTAG_DIO1")));
  set2.insert (std::make_pair (0, tl::Variant (2)));

  size_t pid1 = rep.properties_id (set1);
  size_t pid2 = rep.properties_id (set2);

  EXPECT_EQ (pid1, size_t (1));
  EXPECT_EQ (pid2, size_t (2));
}

