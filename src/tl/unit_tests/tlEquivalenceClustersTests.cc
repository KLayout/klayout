
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


#include "tlEquivalenceClusters.h"
#include "tlUnitTest.h"

//  basics
TEST(1_basics)
{
  tl::equivalence_clusters<int> eq;

  eq.same (1, 5);
  eq.same (2, 3);
  eq.same (5, 4);

  EXPECT_EQ (eq.cluster_id (1), size_t (1));
  EXPECT_EQ (eq.cluster_id (2), size_t (2));
  EXPECT_EQ (eq.cluster_id (3), size_t (2));
  EXPECT_EQ (eq.cluster_id (4), size_t (1));
  EXPECT_EQ (eq.cluster_id (5), size_t (1));
  EXPECT_EQ (eq.cluster_id (6), size_t (0));

  EXPECT_EQ (eq.size (), size_t (2));

  eq.same (2, 6);
  EXPECT_EQ (eq.cluster_id (6), size_t (2));
  EXPECT_EQ (eq.cluster_id (7), size_t (0));

  EXPECT_EQ (eq.size (), size_t (2));

  eq.same (7, 8);
  EXPECT_EQ (eq.size (), size_t (3));
  EXPECT_EQ (eq.cluster_id (6), size_t (2));
  EXPECT_EQ (eq.cluster_id (7), size_t (3));
  EXPECT_EQ (eq.cluster_id (8), size_t (3));
}

//  joining of clusters
TEST(2_join)
{
  tl::equivalence_clusters<int> eq;

  eq.same (1, 2);
  eq.same (3, 4);
  eq.same (5, 6);

  EXPECT_EQ (eq.cluster_id (1), size_t (1));
  EXPECT_EQ (eq.cluster_id (2), size_t (1));
  EXPECT_EQ (eq.cluster_id (3), size_t (2));
  EXPECT_EQ (eq.cluster_id (4), size_t (2));
  EXPECT_EQ (eq.cluster_id (5), size_t (3));
  EXPECT_EQ (eq.cluster_id (6), size_t (3));

  eq.same (3, 2);

  EXPECT_EQ (eq.cluster_id (1), size_t (2));
  EXPECT_EQ (eq.cluster_id (2), size_t (2));
  EXPECT_EQ (eq.cluster_id (3), size_t (2));
  EXPECT_EQ (eq.cluster_id (4), size_t (2));
  EXPECT_EQ (eq.cluster_id (5), size_t (3));
  EXPECT_EQ (eq.cluster_id (6), size_t (3));

  eq.same (4, 5);

  EXPECT_EQ (eq.cluster_id (1), size_t (2));
  EXPECT_EQ (eq.cluster_id (2), size_t (2));
  EXPECT_EQ (eq.cluster_id (3), size_t (2));
  EXPECT_EQ (eq.cluster_id (4), size_t (2));
  EXPECT_EQ (eq.cluster_id (5), size_t (2));
  EXPECT_EQ (eq.cluster_id (6), size_t (2));

  eq.same (10, 11);
  eq.same (12, 13);

  EXPECT_EQ (eq.cluster_id (10), size_t (3));
  EXPECT_EQ (eq.cluster_id (11), size_t (3));

  EXPECT_EQ (eq.cluster_id (12), size_t (1));
  EXPECT_EQ (eq.cluster_id (13), size_t (1));
}

//  size
TEST(3_size)
{
  tl::equivalence_clusters<int> eq;

  eq.same (1, 2);
  EXPECT_EQ (eq.size (), size_t (1));
  eq.same (2, 4);
  EXPECT_EQ (eq.size (), size_t (1));
  eq.same (5, 6);
  EXPECT_EQ (eq.size (), size_t (2));
}

//  has_attribute
TEST(4_has_attribute)
{
  tl::equivalence_clusters<int> eq;

  eq.same (1, 1);
  EXPECT_EQ (eq.has_attribute (1), true);
  EXPECT_EQ (eq.has_attribute (2), false);
  eq.same (1, 2);
  EXPECT_EQ (eq.has_attribute (1), true);
  EXPECT_EQ (eq.has_attribute (2), true);
  EXPECT_EQ (eq.has_attribute (3), false);
}

std::string eq2string (const tl::equivalence_clusters<int> &eq)
{
  std::string res;
  for (size_t c = 1; c <= eq.size (); ++c) {
    for (tl::equivalence_clusters<int>::cluster_iterator i = eq.begin_cluster (c); i != eq.end_cluster (c); ++i) {
      if (i != eq.begin_cluster (c)) {
        res += ",";
      } else if (! res.empty ()) {
        res += ";";
      }
      res += tl::to_string ((*i)->first);
    }
  }
  return res;
}

//  iterator
TEST(5_iterator)
{
  tl::equivalence_clusters<int> eq;

  eq.same (1, 1);
  EXPECT_EQ (eq2string (eq), "1");

  eq.same (1, 2);
  EXPECT_EQ (eq2string (eq), "1,2");

  eq.same (3, 4);
  EXPECT_EQ (eq2string (eq), "1,2;3,4");

  eq.same (10, 11);
  EXPECT_EQ (eq2string (eq), "1,2;3,4;10,11");

  eq.same (1, 10);
  EXPECT_EQ (eq2string (eq), "1,2,10,11;3,4");
}

//  apply_other_equivalences
TEST(6_apply_equivalences)
{
  tl::equivalence_clusters<int> eq;

  eq.same (1, 1);
  eq.same (2, 2);
  eq.same (3, 4);
  eq.same (5, 6);
  EXPECT_EQ (eq2string (eq), "1;2;3,4;5,6");

  tl::equivalence_clusters<int> eq2;

  eq2.same (2, 2);
  eq2.same (4, 5);
  eq2.same (4, 10);
  eq2.same (11, 11);
  EXPECT_EQ (eq2string (eq2), "2;4,5,10;11");

  eq.apply_equivalences (eq2);
  EXPECT_EQ (eq2string (eq), "1;2;5,6,3,4");
}

//  merge
TEST(7_merge)
{
  tl::equivalence_clusters<int> eq;

  eq.same (1, 1);
  eq.same (2, 2);
  eq.same (3, 4);
  eq.same (5, 6);
  EXPECT_EQ (eq2string (eq), "1;2;3,4;5,6");

  tl::equivalence_clusters<int> eq2;

  eq2.same (2, 2);
  eq2.same (4, 5);
  eq2.same (4, 10);
  eq2.same (11, 11);
  EXPECT_EQ (eq2string (eq2), "2;4,5,10;11");

  eq.merge (eq2);
  EXPECT_EQ (eq2string (eq), "1;2;3,4,5,6,10;11");
}

