
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "tlUnitTest.h"
#include "dbHierNetworkProcessor.h"
#include "dbTestSupport.h"
#include "dbShapeRepository.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbText.h"
#include "dbLayout.h"

static std::string l2s (db::Connectivity::layer_iterator b, db::Connectivity::layer_iterator e)
{
  std::string s;
  for (db::Connectivity::layer_iterator i = b; i != e; ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (*i);
  }
  return s;
}

TEST(1_Connectivity)
{
  db::Connectivity conn;

  EXPECT_EQ (l2s (conn.begin_layers (), conn.end_layers ()), "");

  conn.connect (0);
  EXPECT_EQ (l2s (conn.begin_layers (), conn.end_layers ()), "0");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "");

  conn.connect (0, 1);
  EXPECT_EQ (l2s (conn.begin_layers (), conn.end_layers ()), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0");

  conn.connect (1);
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0,1");

  conn.connect (0, 2);
  conn.connect (2);

  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1,2");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (2), conn.end_connected (2)), "0,2");
}

TEST(2_ShapeInteractions)
{
  db::Connectivity conn;

  conn.connect (0);
  conn.connect (1);
  conn.connect (0, 1);

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;1000,1000;1000,0)", poly);
  db::GenericRepository repo;
  db::PolygonRef ref1 (poly, repo);
  db::ICplxTrans t2 (db::Trans (db::Vector (0, 10)));
  db::PolygonRef ref2 (poly.transformed (t2), repo);
  db::ICplxTrans t3 (db::Trans (db::Vector (0, 2000)));
  db::PolygonRef ref3 (poly.transformed (t3), repo);

  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t2), true);  // t2*ref1 == ref2
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 1), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t2), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 0, t2), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 0), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t3), false);  // t3*ref1 == ref3
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 1), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t3), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 2), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 2, t2), false);
}

TEST(2_ShapeInteractionsRealPolygon)
{
  db::Connectivity conn;

  conn.connect (0);
  conn.connect (1);
  conn.connect (0, 1);

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;500,1000;500,1500;1000,1500;1000,0)", poly);
  db::GenericRepository repo;
  db::PolygonRef ref1 (poly, repo);
  db::ICplxTrans t2 (db::Trans (db::Vector (0, 10)));
  db::PolygonRef ref2 (poly.transformed (t2), repo);
  db::ICplxTrans t3 (db::Trans (db::Vector (0, 2000)));
  db::PolygonRef ref3 (poly.transformed (t3), repo);
  db::ICplxTrans t4 (db::Trans (db::Vector (0, 1500)));
  db::PolygonRef ref4 (poly.transformed (t4), repo);

  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t2), true);  // t2*ref1 == ref2
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 1), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t2), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 0, t2), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 0), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t3), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref4, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t4), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 1), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t3), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 2), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 2, t2), false);
}

TEST(10_LocalClusterBasic)
{
  db::GenericRepository repo;

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;1000,1000;1000,0)", poly);

  db::local_cluster<db::PolygonRef> cluster;
  EXPECT_EQ (cluster.bbox ().to_string (), "()");
  EXPECT_EQ (cluster.id (), size_t (0));

  cluster.add (db::PolygonRef (poly, repo), 0);
  EXPECT_EQ (cluster.bbox ().to_string (), "(0,0;1000,1000)");

  db::local_cluster<db::PolygonRef> cluster2;
  cluster.add (db::PolygonRef (poly, repo).transformed (db::Trans (db::Vector (10, 20))), 1);

  cluster.join_with (cluster2);
  EXPECT_EQ (cluster.bbox ().to_string (), "(0,0;1010,1020)");
}

TEST(11_LocalClusterInteractBasic)
{
  db::GenericRepository repo;

  db::Connectivity conn;
  conn.connect (0);
  conn.connect (1);
  conn.connect (2);
  conn.connect (0, 1);
  conn.connect (0, 2);

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;1000,1000;1000,0)", poly);

  db::local_cluster<db::PolygonRef> cluster;
  db::local_cluster<db::PolygonRef> cluster2;

  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false);

  cluster.add (db::PolygonRef (poly, repo), 0);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false);

  cluster2.add (db::PolygonRef (poly, repo), 0);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (10, 20))), conn), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1000))), conn), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1001))), conn), false);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 2000))), conn), false);

  cluster.clear ();
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false);
}

TEST(11_LocalClusterInteractDifferentLayers)
{
  db::GenericRepository repo;

  db::Connectivity conn;
  conn.connect (0);
  conn.connect (1);
  conn.connect (2);
  conn.connect (0, 1);
  conn.connect (0, 2);

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;1000,1000;1000,0)", poly);

  db::local_cluster<db::PolygonRef> cluster;
  db::local_cluster<db::PolygonRef> cluster2;

  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false);

  cluster.add (db::PolygonRef (poly, repo), 0);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false);

  cluster2.add (db::PolygonRef (poly, repo), 1);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (10, 20))), conn), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1000))), conn), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1001))), conn), false);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 2000))), conn), false);

  cluster.clear ();
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false);
  cluster.add (db::PolygonRef (poly, repo), 2);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false); //  not connected

  cluster.clear ();
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), false);
  cluster.add (db::PolygonRef (poly, repo), 1);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn), true);
}

static std::string obj2string (const db::PolygonRef &ref)
{
  return ref.obj ().transformed (ref.trans ()).to_string ();
}

template <class T>
static std::string local_cluster_to_string (const db::local_cluster<T> &cluster, const db::Connectivity &conn)
{
  std::string res;
  for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {
    for (typename db::local_cluster<T>::shape_iterator s = cluster.begin (*l); ! s.at_end (); ++s) {
      if (! res.empty ()) {
        res += ";";
      }
      res += "[" + tl::to_string (*l) + "]" + obj2string (*s);
    }
  }
  return res;
}

template <class T>
static std::string local_clusters_to_string (const db::local_clusters<T> &clusters, const db::Connectivity &conn)
{
  std::string s;
  for (typename db::local_clusters<T>::const_iterator c = clusters.begin (); c != clusters.end (); ++c) {
    if (! s.empty ()) {
      s += "\n";
    }
    s += "#" + tl::to_string (c->id ()) + ":" + local_cluster_to_string (*c, conn);
  }
  return s;
}

TEST(20_LocalClustersBasic)
{
  db::Layout layout;
  db::Cell &cell = layout.cell (layout.add_cell ("TOP"));
  db::GenericRepository &repo = layout.shape_repository ();

  db::Connectivity conn;
  conn.connect (0);
  conn.connect (1);
  conn.connect (2);
  conn.connect (0, 1);
  conn.connect (0, 2);

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;1000,1000;1000,0)", poly);

  cell.shapes (0).insert (db::PolygonRef (poly, repo));

  db::local_clusters<db::PolygonRef> clusters;
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "");

  clusters.build_clusters (cell, db::ShapeIterator::Polygons, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0)");

  //  one more shape
  cell.shapes (0).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (10, 20))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, db::ShapeIterator::Polygons, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)");

  //  one more shape creating a new cluster
  cell.shapes (2).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1100))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, db::ShapeIterator::Polygons, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)\n"
    "#2:[2](0,1100;0,2100;1000,2100;1000,1100)"
  );

  //  one more shape connecting these
  cell.shapes (2).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1000))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, db::ShapeIterator::Polygons, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20);[2](0,1000;0,2000;1000,2000;1000,1000);[2](0,1100;0,2100;1000,2100;1000,1100)"
  );

  //  one more shape opening a new cluster
  cell.shapes (1).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1100))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, db::ShapeIterator::Polygons, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20);[2](0,1000;0,2000;1000,2000;1000,1000);[2](0,1100;0,2100;1000,2100;1000,1100)\n"
    "#2:[1](0,1100;0,2100;1000,2100;1000,1100)"
  );
}

