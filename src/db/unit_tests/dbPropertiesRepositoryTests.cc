
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


#include "dbPropertiesRepository.h"
#include "dbTestSupport.h"
#include "tlString.h"
#include "tlUnitTest.h"

namespace {

/**
 *  @brief Installs a temporary repository instance for testing
 *
 *  By using a temp instance, we do not disturb other tests.
 */
class TempPropertiesRepository
{
public:
  TempPropertiesRepository ()
  {
    db::PropertiesRepository::replace_instance_temporarily (&m_temp);
  }

  ~TempPropertiesRepository ()
  {
    db::PropertiesRepository::replace_instance_temporarily (0);
  }

private:
  db::PropertiesRepository m_temp;
};

}

TEST(BasicNames)
{
  db::PropertiesRepository rep;

  tl::Variant n1 ("Hallo");
  tl::Variant n2 ("AAA");

  db::property_names_id_type id1 = rep.prop_name_id (n1);
  db::property_names_id_type id2 = rep.prop_name_id (n2);
  db::property_names_id_type id1_copy = id1, id2_copy = id2;

  id2 = rep.prop_name_id (n2);
  id1 = rep.prop_name_id (n1);

  EXPECT_EQ (id1, id1_copy);
  EXPECT_EQ (id2, id2_copy);

  EXPECT_EQ (db::property_name (id1) == n1, true);
  EXPECT_EQ (db::property_name (id2) == n2, true);
}

TEST(BasicValues)
{
  db::PropertiesRepository rep;

  tl::Variant v1 ("X");
  tl::Variant v2 (17);

  db::property_values_id_type id1 = rep.prop_value_id (v1);
  db::property_values_id_type id2 = rep.prop_value_id (v2);
  db::property_values_id_type id1_copy = id1, id2_copy = id2;

  id2 = rep.prop_value_id (v2);
  id1 = rep.prop_value_id (v1);

  EXPECT_EQ (id1, id1_copy);
  EXPECT_EQ (id2, id2_copy);

  EXPECT_EQ (db::property_value (id1) == v1, true);
  EXPECT_EQ (db::property_value (id2) == v2, true);
}

TEST(BasicPropertySetsInRepository)
{
  db::PropertiesSet set1;
  db::PropertiesSet set2;

  set1.insert_by_id (0, 1);
  set1.insert (10, 2);

  set2.insert (0, 1);
  set2.insert (9, 3);
  set2.insert (2, 5);

  db::PropertiesRepository rep;

  db::properties_id_type id1 = rep.properties_id (set1);
  db::properties_id_type id2 = rep.properties_id (set2);
  db::properties_id_type id1_copy = id1, id2_copy = id2;

  id2 = rep.properties_id (set2);
  id1 = rep.properties_id (set1);

  EXPECT_EQ (id1, id1_copy);
  EXPECT_EQ (id2, id2_copy);

  EXPECT_EQ (db::properties (id1) == set1, true);
  EXPECT_EQ (db::properties (id2) == set2, true);
}

TEST(PropertySets)
{
  TempPropertiesRepository tmp_repo;

  db::PropertiesSet ps1;
  db::PropertiesSet ps2;

  EXPECT_EQ (ps1.empty (), true);
  EXPECT_EQ (ps1.size (), size_t (0));
  EXPECT_EQ (ps1.value (tl::Variant (17)).is_nil (), true);
  EXPECT_EQ (ps1 == ps2, true);
  EXPECT_EQ (ps1 != ps2, false);
  EXPECT_EQ (ps1 < ps2, false);
  EXPECT_EQ (ps2 < ps1, false);

  ps1.insert (tl::Variant (17), tl::Variant ("value"));
  EXPECT_EQ (ps1.empty (), false);
  EXPECT_EQ (ps1.size (), size_t (1));
  EXPECT_EQ (ps1.has_value (tl::Variant (17)), true);
  EXPECT_EQ (ps1.has_value (tl::Variant ()), false);
  EXPECT_EQ (ps1.has_value (tl::Variant ("x")), false);
  EXPECT_EQ (ps1.value (tl::Variant (17)).to_string (), "value");
  EXPECT_EQ (ps1.value (tl::Variant ("x")).is_nil (), true);
  EXPECT_EQ (ps1.value (db::property_names_id (17)).to_string (), "value");
  EXPECT_EQ (ps1.value (db::property_names_id ("x")).is_nil (), true);
  EXPECT_EQ (ps1 [tl::Variant (17)].to_string (), "value");
  EXPECT_EQ (ps1 [tl::Variant ("x")].is_nil (), true);
  EXPECT_EQ (ps1 == ps2, false);
  EXPECT_EQ (ps1 != ps2, true);
  EXPECT_EQ (ps1 < ps2, false);
  EXPECT_EQ (ps2 < ps1, true);

  ps2.swap (ps1);
  EXPECT_EQ (ps1.value (tl::Variant (17)).is_nil (), true);
  EXPECT_EQ (ps2.value (tl::Variant (17)).to_string (), "value");

  ps1 = ps2;
  EXPECT_EQ (ps1.value (tl::Variant (17)).to_string (), "value");
  EXPECT_EQ (ps1.value (tl::Variant ("x")).is_nil (), true);
  EXPECT_EQ (ps1 == ps2, true);
  EXPECT_EQ (ps1 != ps2, false);
  EXPECT_EQ (ps1 < ps2, false);
  EXPECT_EQ (ps2 < ps1, false);

  ps2.erase (tl::Variant (17));
  EXPECT_EQ (ps2.value (tl::Variant (17)).is_nil (), true);
  EXPECT_EQ (ps2.has_value (tl::Variant (17)), false);
  EXPECT_EQ (ps2.size (), size_t (0));
  EXPECT_EQ (ps2.empty (), true);

  ps1.clear ();
  EXPECT_EQ (ps1.size (), size_t (0));
  EXPECT_EQ (ps1.empty (), true);
}

TEST(PropertySetsMerge)
{
  TempPropertiesRepository tmp_repo;

  db::PropertiesSet ps1, ps2;
  ps1.insert (tl::Variant (17), tl::Variant ("value"));
  ps2.insert (tl::Variant ("x"), tl::Variant (42));

  ps1.merge (ps2);

  EXPECT_EQ (ps1.to_dict_var ().to_string (), "{17=>value,x=>42}");
}

TEST(PropertySetsConversions)
{
  TempPropertiesRepository tmp_repo;

  db::PropertiesSet ps1;
  ps1.insert (tl::Variant (17), tl::Variant ("value"));
  ps1.insert (tl::Variant ("x"), tl::Variant (42));

  EXPECT_EQ (ps1.to_dict_var ().to_string (), "{17=>value,x=>42}");
  EXPECT_EQ (ps1.to_list_var ().to_string (), "((17,value),(x,42))");

  auto ps1_map = ps1.to_map ();
  EXPECT_EQ (ps1_map.size (), size_t (2));
  EXPECT_EQ (ps1_map.find (tl::Variant (17))->second.to_string (), "value");
  EXPECT_EQ (ps1_map.find (tl::Variant ("x"))->second.to_string (), "42");
  EXPECT_EQ (ps1_map.find (tl::Variant (42)) == ps1_map.end (), true);
}

TEST(PropertiesTranslator)
{
  EXPECT_EQ (db::PropertiesTranslator ().is_null (), true);
  EXPECT_EQ (db::PropertiesTranslator ().is_pass (), true);
  EXPECT_EQ (db::PropertiesTranslator ().is_empty (), false);
  EXPECT_EQ (db::PropertiesTranslator::make_pass_all ().is_null (), false);
  EXPECT_EQ (db::PropertiesTranslator::make_pass_all ().is_pass (), true);
  EXPECT_EQ (db::PropertiesTranslator::make_pass_all ().is_empty (), false);
  EXPECT_EQ (db::PropertiesTranslator::make_remove_all ().is_null (), false);
  EXPECT_EQ (db::PropertiesTranslator::make_remove_all ().is_pass (), false);
  EXPECT_EQ (db::PropertiesTranslator::make_remove_all ().is_empty (), true);

  db::PropertiesRepository rp;
  db::property_names_id_type key1 = rp.prop_name_id (1);
  db::property_names_id_type key2 = rp.prop_name_id (2);
  db::property_names_id_type key3 = rp.prop_name_id (3);

  db::PropertiesSet ps;
  ps.insert (key1, 100);
  ps.insert (key2, 101);
  db::properties_id_type prop1a = rp.properties_id (ps);
  EXPECT_EQ (db::prop2string (prop1a), "{1=>100,2=>101}");

  ps.clear ();
  ps.insert (key1, 0);
  ps.insert (key2, 101);
  db::properties_id_type prop1b = rp.properties_id (ps);
  EXPECT_EQ (db::prop2string (prop1b), "{1=>0,2=>101}");

  ps.clear ();
  ps.insert (key1, 100);
  ps.insert (key3, 102);
  db::properties_id_type prop2 = rp.properties_id (ps);
  EXPECT_EQ (db::prop2string (prop2), "{1=>100,3=>102}");

  ps.clear ();
  ps.insert (key1, 100);
  db::properties_id_type prop3 = rp.properties_id (ps);
  EXPECT_EQ (db::prop2string (prop3), "{1=>100}");

  db::PropertiesTranslator t;
  EXPECT_EQ (db::prop2string (t (prop1a)), "{1=>100,2=>101}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{1=>0,2=>101}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{1=>100,3=>102}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{1=>100}");

  t = db::PropertiesTranslator::make_pass_all ();
  EXPECT_EQ (db::prop2string (t (prop1a)), "{1=>100,2=>101}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{1=>0,2=>101}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{1=>100,3=>102}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{1=>100}");

  t = db::PropertiesTranslator::make_remove_all ();
  EXPECT_EQ (db::prop2string (t (prop1a)), "{}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{}");

  std::set<tl::Variant> kf;
  kf.insert (1);
  t = db::PropertiesTranslator::make_filter (kf, rp);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{1=>100}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{1=>0}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{1=>100}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{1=>100}");

  kf.insert (3);
  t = db::PropertiesTranslator::make_filter (kf, rp);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{1=>100}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{1=>0}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{1=>100,3=>102}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{1=>100}");

  std::map<tl::Variant, tl::Variant> km;
  km[1] = 4;
  km[3] = 1;

  t = db::PropertiesTranslator::make_key_mapper (km, rp);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{4=>100}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{4=>0}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{1=>102,4=>100}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{4=>100}");

  kf.clear ();
  kf.insert (4);
  t = db::PropertiesTranslator::make_filter (kf, rp) * db::PropertiesTranslator::make_key_mapper (km, rp);
  EXPECT_EQ (t.is_empty (), false);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{4=>100}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{4=>0}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{4=>100}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{4=>100}");

  kf.clear ();
  kf.insert (3);

  t = db::PropertiesTranslator::make_filter (kf, rp) * db::PropertiesTranslator::make_key_mapper (km, rp);
  EXPECT_EQ (t.is_empty (), true);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{}");

  t = db::PropertiesTranslator::make_key_mapper (km, rp) * db::PropertiesTranslator::make_filter (kf, rp);
  EXPECT_EQ (t.is_empty (), false);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{1=>102}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{}");

  t = db::PropertiesTranslator::make_key_mapper (km, rp);
  t = db::PropertiesTranslator::make_filter (kf, rp) * t;
  EXPECT_EQ (t.is_empty (), true);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{}");

  t = db::PropertiesTranslator::make_filter (kf, rp);
  t = db::PropertiesTranslator::make_key_mapper (km, rp) * t;
  EXPECT_EQ (t.is_empty (), false);
  EXPECT_EQ (db::prop2string (t (prop1a)), "{}");
  EXPECT_EQ (db::prop2string (t (prop1b)), "{}");
  EXPECT_EQ (db::prop2string (t (prop2)), "{1=>102}");
  EXPECT_EQ (db::prop2string (t (prop3)), "{}");
}

static std::string ps2s (const db::PropertiesRepository::properties_id_set &ps)
{
  std::string res;
  for (auto p = ps.begin (); p != ps.end (); ++p) {
    if (! res.empty ()) {
      res += ",";
    }
    res += db::properties (*p).to_dict_var ().to_string ();
  }
  return res;
}

TEST(PropertyIdsByNameAndValue)
{
  db::PropertiesRepository rp;

  db::PropertiesSet ps;
  ps.insert_by_id (rp.prop_name_id (1), rp.prop_value_id ("A"));

  //  1=>"A"
  db::properties_id_type pid1 = rp.properties_id (ps);

  ps.insert_by_id (rp.prop_name_id (2), rp.prop_value_id ("A"));

  //  1=>"A", 2=>"A"
  db::properties_id_type pid2 = rp.properties_id (ps);

  ps.clear ();
  ps.insert_by_id (rp.prop_name_id (2), rp.prop_value_id ("B"));

  //  2=>"B"
  db::properties_id_type pid3 = rp.properties_id (ps);

  ps.insert_by_id (rp.prop_name_id (2), rp.prop_value_id ("C"));

  //  2=>"B",2=>"C"
  db::properties_id_type pid4 = rp.properties_id (ps);

  ps.insert_by_id (rp.prop_name_id (3), rp.prop_value_id ("C"));

  //  2=>"B",2=>"C",3=>"C"
  db::properties_id_type pid5 = rp.properties_id (ps);

  db::PropertiesRepository::properties_id_set res, ref;

  res = rp.properties_ids_by_name (rp.prop_name_id (1));
  ref.clear ();
  ref.insert (pid1);
  ref.insert (pid2);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name (rp.prop_name_id (2));
  ref.clear ();
  ref.insert (pid2);
  ref.insert (pid3);
  ref.insert (pid4);
  ref.insert (pid5);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name (rp.prop_name_id (3));
  ref.clear ();
  ref.insert (pid5);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_value (rp.prop_value_id ("A"));
  ref.clear ();
  ref.insert (pid1);
  ref.insert (pid2);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_value (rp.prop_value_id ("B"));
  ref.clear ();
  ref.insert (pid3);
  ref.insert (pid4);
  ref.insert (pid5);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_value (rp.prop_value_id ("C"));
  ref.clear ();
  ref.insert (pid4);
  ref.insert (pid5);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name_value (rp.prop_name_id (1), rp.prop_value_id ("A"));
  ref.clear ();
  ref.insert (pid1);
  ref.insert (pid2);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name_value (rp.prop_name_id (1), rp.prop_value_id ("B"));
  ref.clear ();
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name_value (rp.prop_name_id (2), rp.prop_value_id ("A"));
  ref.clear ();
  ref.insert (pid2);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name_value (rp.prop_name_id (2), rp.prop_value_id ("B"));
  ref.clear ();
  ref.insert (pid3);
  ref.insert (pid4);
  ref.insert (pid5);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name_value (rp.prop_name_id (2), rp.prop_value_id ("C"));
  ref.clear ();
  ref.insert (pid4);
  ref.insert (pid5);
  EXPECT_EQ (ps2s (res), ps2s (ref));

  res = rp.properties_ids_by_name_value (rp.prop_name_id (3), rp.prop_value_id ("C"));
  ref.clear ();
  ref.insert (pid5);
  EXPECT_EQ (ps2s (res), ps2s (ref));
}

TEST(PropertiesSetHash)
{
  db::PropertiesRepository rp;

  db::PropertiesSet ps;
  EXPECT_EQ (ps.hash (), size_t (0));
  EXPECT_EQ (db::hash_for_properties_id (0), size_t (0));

  ps.insert_by_id (rp.prop_name_id (1), rp.prop_value_id ("A"));
  ps.insert_by_id (rp.prop_name_id (2), rp.prop_value_id ("B"));

  size_t h1 = ps.hash ();
  EXPECT_EQ (db::hash_for_properties_id (rp.properties_id (ps)), h1);

  db::PropertiesSet ps2;
  ps2.insert_by_id (rp.prop_name_id (2), rp.prop_value_id ("B"));
  ps2.insert_by_id (rp.prop_name_id (1), rp.prop_value_id ("A"));

  EXPECT_EQ (ps2.hash (), h1);
  EXPECT_EQ (db::hash_for_properties_id (rp.properties_id (ps2)), h1);
}

TEST(SameValueDifferentTypes)
{
  {
    db::PropertiesRepository rp;

    EXPECT_EQ (db::property_value (rp.prop_value_id ((int) 5)).to_parsable_string (), "#5");
    EXPECT_EQ (db::property_value (rp.prop_value_id ((double) 5)).to_parsable_string (), "##5");
    EXPECT_EQ (db::property_value (rp.prop_value_id ((int) 5)).to_parsable_string (), "#5");
    EXPECT_EQ (db::property_value (rp.prop_value_id ((double) 5)).to_parsable_string (), "##5");

    EXPECT_EQ (db::property_name (rp.prop_name_id ((int) 5)).to_parsable_string (), "#5");
    EXPECT_EQ (db::property_name (rp.prop_name_id ((double) 5)).to_parsable_string (), "##5");
    EXPECT_EQ (db::property_name (rp.prop_name_id ((int) 5)).to_parsable_string (), "#5");
    EXPECT_EQ (db::property_name (rp.prop_name_id ((double) 5)).to_parsable_string (), "##5");
  }

  {
    db::PropertiesRepository rp;

    EXPECT_EQ (db::property_value (rp.prop_value_id ((double) 5)).to_parsable_string (), "##5");
    EXPECT_EQ (db::property_value (rp.prop_value_id ((int) 5)).to_parsable_string (), "#5");
    EXPECT_EQ (db::property_value (rp.prop_value_id ((double) 5)).to_parsable_string (), "##5");
    EXPECT_EQ (db::property_value (rp.prop_value_id ((int) 5)).to_parsable_string (), "#5");

    EXPECT_EQ (db::property_name (rp.prop_name_id ((double) 5)).to_parsable_string (), "##5");
    EXPECT_EQ (db::property_name (rp.prop_name_id ((int) 5)).to_parsable_string (), "#5");
    EXPECT_EQ (db::property_name (rp.prop_name_id ((double) 5)).to_parsable_string (), "##5");
    EXPECT_EQ (db::property_name (rp.prop_name_id ((int) 5)).to_parsable_string (), "#5");
  }
}
