
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

#include "dbPropertiesFilter.h"
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

TEST(1)
{
  TempPropertiesRepository temp_pr;

  db::PropertiesSet ps;
  ps.insert ("net", 17);
  auto net17 = properties_id (ps);

  ps.clear ();
  ps.insert ("net", 1);
  auto net1 = properties_id (ps);

  ps.clear ();
  ps.insert ("net", 42);
  ps.insert ("not", "never");
  auto net42 = properties_id (ps);

  EXPECT_EQ (db::PropertiesFilter ("not", tl::Variant ("never"), false).prop_selected (net17), false);
  EXPECT_EQ (db::PropertiesFilter ("not", tl::Variant ("never"), true).prop_selected (net17), true);
  EXPECT_EQ (db::PropertiesFilter ("not", tl::Variant ("never"), false).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("not", tl::Variant ("never"), true).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("not", tl::Variant ("never"), false).prop_selected (net42), true);
  EXPECT_EQ (db::PropertiesFilter ("not", tl::Variant ("never"), true).prop_selected (net42), false);
  EXPECT_EQ (db::PropertiesFilter ("doesnotexist", tl::Variant ("never"), false).prop_selected (net42), false);
  EXPECT_EQ (db::PropertiesFilter ("doesnotexist", tl::Variant ("never"), true).prop_selected (net42), true);
  EXPECT_EQ (db::PropertiesFilter ("net", 17, false).prop_selected (net17), true);
  EXPECT_EQ (db::PropertiesFilter ("net", 17, true).prop_selected (net17), false);
  EXPECT_EQ (db::PropertiesFilter ("net", 17, false).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", 17, true).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::Variant (), 17, false).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::Variant (), 17, true).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::Variant (), 1, false).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::Variant (), 1, true).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", 0, 2, false).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", 0, 2, true).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", 0, 1, false).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", 0, 1, true).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", 1, 2, false).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", 1, 2, true).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", -1, tl::Variant (), false).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", -1, tl::Variant (), true).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", 2, tl::Variant (), false).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("net", 2, tl::Variant (), true).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::GlobPattern ("1*"), false).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::GlobPattern ("1*"), true).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("not", tl::GlobPattern ("1*"), false).prop_selected (net1), false);
  EXPECT_EQ (db::PropertiesFilter ("not", tl::GlobPattern ("1*"), true).prop_selected (net1), true);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::GlobPattern ("1*"), false).prop_selected (net17), true);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::GlobPattern ("1*"), true).prop_selected (net17), false);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::GlobPattern ("1*"), false).prop_selected (net42), false);
  EXPECT_EQ (db::PropertiesFilter ("net", tl::GlobPattern ("1*"), true).prop_selected (net42), true);
}
