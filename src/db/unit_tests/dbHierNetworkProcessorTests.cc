
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


#include "tlUnitTest.h"
#include "dbHierNetworkProcessor.h"
#include "dbTestSupport.h"
#include "dbShapeRepository.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbText.h"
#include "dbLayout.h"
#include "dbStream.h"
#include "dbCommonReader.h"

static std::string l2s (db::Connectivity::layer_iterator b, db::Connectivity::layer_iterator e)
{
  std::string s;
  for (db::Connectivity::layer_iterator i = b; i != e; ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first);
    if (i->second < 0) {
      s += "-S";
    } else if (i->second > 0) {
      s += "+S";
    }
  }
  return s;
}

static std::string al2s (db::Connectivity::all_layer_iterator b, db::Connectivity::all_layer_iterator e)
{
  std::string s;
  for (db::Connectivity::all_layer_iterator i = b; i != e; ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (*i);
  }
  return s;
}

static std::string gn2s (db::Connectivity::global_nets_iterator b, db::Connectivity::global_nets_iterator e)
{
  std::string s;
  for (db::Connectivity::global_nets_iterator i = b; i != e; ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first);
    if (i->second < 0) {
      s += "-S";
    } else if (i->second > 0) {
      s += "+S";
    }
  }
  return s;
}

TEST(1_Connectivity)
{
  db::Connectivity conn;

  EXPECT_EQ (al2s (conn.begin_layers (), conn.end_layers ()), "");

  conn.connect (0);
  EXPECT_EQ (al2s (conn.begin_layers (), conn.end_layers ()), "0");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "");

  conn.connect (0, 1);
  EXPECT_EQ (al2s (conn.begin_layers (), conn.end_layers ()), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0");

  conn.connect (1);
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0,1");

  conn.connect (0, 2);
  conn.connect (2);

  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1,2");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (2), conn.end_connected (2)), "0,2");

  EXPECT_EQ (conn.connect_global (0, "GLOBAL"), size_t (0));
  EXPECT_EQ (gn2s (conn.begin_global_connections (2), conn.end_global_connections (2)), "");
  EXPECT_EQ (gn2s (conn.begin_global_connections (0), conn.end_global_connections (0)), "0");
  EXPECT_EQ (conn.connect_global (2, "GLOBAL2"), size_t (1));
  EXPECT_EQ (gn2s (conn.begin_global_connections (2), conn.end_global_connections (2)), "1");
  EXPECT_EQ (conn.connect_global (0, "GLOBAL2"), size_t (1));
  EXPECT_EQ (gn2s (conn.begin_global_connections (0), conn.end_global_connections (0)), "0,1");

  EXPECT_EQ (conn.global_net_name (0), "GLOBAL");
  EXPECT_EQ (conn.global_net_name (1), "GLOBAL2");

  db::Connectivity conn2 = conn;

  EXPECT_EQ (l2s (conn2.begin_connected (0), conn2.end_connected (0)), "0,1,2");
  EXPECT_EQ (l2s (conn2.begin_connected (1), conn2.end_connected (1)), "0,1");
  EXPECT_EQ (l2s (conn2.begin_connected (2), conn2.end_connected (2)), "0,2");

  EXPECT_EQ (gn2s (conn2.begin_global_connections (0), conn2.end_global_connections (0)), "0,1");
  EXPECT_EQ (conn2.global_net_name (0), "GLOBAL");
  EXPECT_EQ (conn2.global_net_name (1), "GLOBAL2");
}

TEST(1_ConnectivitySoft)
{
  db::Connectivity conn;

  EXPECT_EQ (al2s (conn.begin_layers (), conn.end_layers ()), "");

  conn.connect (0);
  EXPECT_EQ (al2s (conn.begin_layers (), conn.end_layers ()), "0");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "");

  conn.soft_connect (0, 1);
  EXPECT_EQ (al2s (conn.begin_layers (), conn.end_layers ()), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1-S");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0+S");

  conn.connect (1);
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0+S,1");

  conn.soft_connect (2, 0);
  conn.connect (2);

  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1-S,2+S");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0+S,1");
  EXPECT_EQ (l2s (conn.begin_connected (2), conn.end_connected (2)), "0-S,2");

  conn.connect (2, 0);

  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1-S,2");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0+S,1");
  EXPECT_EQ (l2s (conn.begin_connected (2), conn.end_connected (2)), "0,2");

  EXPECT_EQ (conn.soft_connect_global (0, "GLOBAL"), size_t (0));
  EXPECT_EQ (gn2s (conn.begin_global_connections (2), conn.end_global_connections (2)), "");
  EXPECT_EQ (gn2s (conn.begin_global_connections (0), conn.end_global_connections (0)), "0-S");
  EXPECT_EQ (conn.soft_connect_global (2, "GLOBAL2"), size_t (1));
  EXPECT_EQ (gn2s (conn.begin_global_connections (2), conn.end_global_connections (2)), "1-S");
  EXPECT_EQ (conn.connect_global (0, "GLOBAL2"), size_t (1));
  EXPECT_EQ (gn2s (conn.begin_global_connections (0), conn.end_global_connections (0)), "0-S,1");

  EXPECT_EQ (conn.global_net_name (0), "GLOBAL");
  EXPECT_EQ (conn.global_net_name (1), "GLOBAL2");

  db::Connectivity conn2 = conn;

  EXPECT_EQ (l2s (conn2.begin_connected (0), conn2.end_connected (0)), "0,1-S,2");
  EXPECT_EQ (l2s (conn2.begin_connected (1), conn2.end_connected (1)), "0+S,1");
  EXPECT_EQ (l2s (conn2.begin_connected (2), conn2.end_connected (2)), "0,2");

  EXPECT_EQ (gn2s (conn2.begin_global_connections (0), conn2.end_global_connections (0)), "0-S,1");
  EXPECT_EQ (conn2.global_net_name (0), "GLOBAL");
  EXPECT_EQ (conn2.global_net_name (1), "GLOBAL2");
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

  int soft = std::numeric_limits<int>::max ();
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 0, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t2, soft), true);  // t2*ref1 == ref2
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 1, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t2, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 0, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 0, t2, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 0, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t3, soft), false);  // t3*ref1 == ref3
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 1, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t3, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 2, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 2, t2, soft), false);
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

  int soft = std::numeric_limits<int>::max ();
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 0, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t2, soft), true);  // t2*ref1 == ref2
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 1, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t2, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 0, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 0, t2, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 0, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t3, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref4, 0, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 0, t4, soft), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 1, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref1, 1, t3, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 2, soft), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref1, 2, t2, soft), false);
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
  cluster.add_attr (1);
  EXPECT_EQ (cluster.bbox ().to_string (), "(0,0;1000,1000)");

  db::local_cluster<db::PolygonRef> cluster2;
  cluster2.add (db::PolygonRef (poly, repo).transformed (db::Disp (db::Vector (10, 20))), 1);
  cluster2.add_attr (2);

  cluster.join_with (cluster2);
  EXPECT_EQ (cluster.bbox ().to_string (), "(0,0;1010,1020)");

  EXPECT_EQ (cluster.begin_attr () == cluster.end_attr (), false);
  db::local_cluster<db::PolygonRef>::attr_iterator a = cluster.begin_attr ();
  EXPECT_EQ (*a++, 1u);
  EXPECT_EQ (*a++, 2u);
  EXPECT_EQ (a == cluster.end_attr (), true);
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
  int soft;

  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false);

  cluster.add (db::PolygonRef (poly, repo), 0);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false);

  cluster2.add (db::PolygonRef (poly, repo), 0);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (10, 20))), conn, soft), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1000))), conn, soft), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1001))), conn, soft), false);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 2000))), conn, soft), false);

  cluster.clear ();
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false);
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
  int soft;

  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false);

  cluster.add (db::PolygonRef (poly, repo), 0);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false);

  cluster2.add (db::PolygonRef (poly, repo), 1);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (10, 20))), conn, soft), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1000))), conn, soft), true);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 1001))), conn, soft), false);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (db::Trans (db::Vector (0, 2000))), conn, soft), false);

  cluster.clear ();
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false);
  cluster.add (db::PolygonRef (poly, repo), 2);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false); //  not connected

  cluster.clear ();
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), false);
  cluster.add (db::PolygonRef (poly, repo), 1);
  EXPECT_EQ (cluster.interacts (cluster2, db::ICplxTrans (), conn, soft), true);
}

static std::string obj2string (const db::PolygonRef &ref)
{
  return ref.obj ().transformed (ref.trans ()).to_string ();
}

static std::string obj2string (const db::Edge &ref)
{
  return ref.to_string ();
}

template <class T>
static std::string local_cluster_to_string (const db::local_cluster<T> &cluster, const db::Connectivity &conn)
{
  std::string res;
  for (db::Connectivity::all_layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {
    for (typename db::local_cluster<T>::shape_iterator s = cluster.begin (*l); ! s.at_end (); ++s) {
      if (! res.empty ()) {
        res += ";";
      }
      res += "[" + tl::to_string (*l) + "]" + obj2string (*s);
    }
  }
  for (typename db::local_cluster<T>::attr_iterator a = cluster.begin_attr (); a != cluster.end_attr (); ++a) {
    res += "%" + tl::to_string (*a);
  }
  for (typename db::local_cluster<T>::global_nets_iterator g = cluster.begin_global_nets (); g != cluster.end_global_nets (); ++g) {
    res += "+" + conn.global_net_name (*g);
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
  for (typename db::local_clusters<T>::const_iterator c = clusters.begin (); c != clusters.end (); ++c) {
    auto sc = clusters.upward_soft_connections (c->id ());
    for (auto i = sc.begin (); i != sc.end (); ++i) {
      if (! s.empty ()) {
        s += "\n";
      }
      s += "(#" + tl::to_string (*i) + "->#" + tl::to_string (c->id ()) + ")";
    }
  }
  return s;
}

TEST(12_LocalClusterSplitByAreaRatio)
{
  db::GenericRepository repo;
  db::Connectivity conn;
  conn.connect (0);
  conn.connect (1);
  conn.connect (2);

  db::local_cluster<db::PolygonRef> cluster (17);
  cluster.add (db::PolygonRef (db::Polygon (db::Box (0, 0, 20, 20)), repo), 0);
  cluster.add (db::PolygonRef (db::Polygon (db::Box (0, 0, 1000, 20)), repo), 0);
  cluster.add (db::PolygonRef (db::Polygon (db::Box (1000, 0, 1020, 1000)), repo), 1);
  cluster.add (db::PolygonRef (db::Polygon (db::Box (0, 1000, 1000, 1020)), repo), 2);

  std::list<db::local_cluster<db::PolygonRef> > out;
  std::back_insert_iterator<std::list<db::local_cluster<db::PolygonRef> > > iout = std::back_inserter (out);
  size_t n = cluster.split (10.0, iout);

  EXPECT_EQ (n, size_t (3));
  EXPECT_EQ (out.size (), size_t (3));

  std::list<db::local_cluster<db::PolygonRef> >::const_iterator i = out.begin ();
  EXPECT_EQ (local_cluster_to_string (*i, conn), "[0](0,0;0,20;20,20;20,0);[0](0,0;0,20;1000,20;1000,0)");
  EXPECT_EQ (i->id (), size_t (17));
  ++i;
  EXPECT_EQ (local_cluster_to_string (*i, conn), "[1](1000,0;1000,1000;1020,1000;1020,0)");
  EXPECT_EQ (i->id (), size_t (17));
  ++i;
  EXPECT_EQ (local_cluster_to_string (*i, conn), "[2](0,1000;0,1020;1000,1020;1000,1000)");
  EXPECT_EQ (i->id (), size_t (17));
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

  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0)");

  //  one more shape
  cell.shapes (0).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (10, 20))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)");

  //  one more shape creating a new cluster
  cell.shapes (2).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1100))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)\n"
    "#2:[2](0,1100;0,2100;1000,2100;1000,1100)"
  );

  //  one more shape connecting these
  cell.shapes (2).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1000))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20);[2](0,1000;0,2000;1000,2000;1000,1000);[2](0,1100;0,2100;1000,2100;1000,1100)"
  );

  //  one more shape opening a new cluster
  cell.shapes (1).insert (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1100))), repo));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20);[2](0,1000;0,2000;1000,2000;1000,1000);[2](0,1100;0,2100;1000,2100;1000,1100)\n"
    "#2:[1](0,1100;0,2100;1000,2100;1000,1100)"
  );
}

TEST(21_LocalClustersBasicWithAttributes)
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

  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0)");

  //  one more shape
  cell.shapes (0).insert (db::PolygonRefWithProperties (db::PolygonRef (poly.transformed (db::Trans (db::Vector (10, 20))), repo), 1));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)%1");

  //  one more shape creating a new cluster
  cell.shapes (2).insert (db::PolygonRefWithProperties (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1100))), repo), 2));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)%1\n"
    "#2:[2](0,1100;0,2100;1000,2100;1000,1100)%2"
  );

  //  one more shape connecting these
  cell.shapes (2).insert (db::PolygonRefWithProperties (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1000))), repo), 3));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20);[2](0,1000;0,2000;1000,2000;1000,1000);[2](0,1100;0,2100;1000,2100;1000,1100)%1%2%3"
  );

  //  one more shape opening a new cluster
  cell.shapes (1).insert (db::PolygonRefWithProperties (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1100))), repo), 4));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20);[2](0,1000;0,2000;1000,2000;1000,1000);[2](0,1100;0,2100;1000,2100;1000,1100)%1%2%3\n"
    "#2:[1](0,1100;0,2100;1000,2100;1000,1100)%4"
  );
}

TEST(22_LocalClustersWithGlobal)
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

  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0)");

  //  one more shape
  cell.shapes (0).insert (db::PolygonRefWithProperties (db::PolygonRef (poly.transformed (db::Trans (db::Vector (10, 20))), repo), 1));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)%1");

  //  one more shape creating a new cluster
  cell.shapes (2).insert (db::PolygonRefWithProperties (db::PolygonRef (poly.transformed (db::Trans (db::Vector (0, 1100))), repo), 2));

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)%1\n"
    "#2:[2](0,1100;0,2100;1000,2100;1000,1100)%2"
  );

  conn.connect_global (0, "GLOBAL");

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)%1+GLOBAL\n"
    "#2:[2](0,1100;0,2100;1000,2100;1000,1100)%2"
  );

  conn.connect_global (2, "GLOBAL2");

  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20)%1+GLOBAL\n"
    "#2:[2](0,1100;0,2100;1000,2100;1000,1100)%2+GLOBAL2"
  );

  conn.connect_global (0, "GLOBAL2");

  //  now, GLOBAL2 will connect these clusters
  clusters.clear ();
  clusters.build_clusters (cell, conn);
  EXPECT_EQ (local_clusters_to_string (clusters, conn),
    "#1:[0](0,0;0,1000;1000,1000;1000,0);[0](10,20;10,1020;1010,1020;1010,20);[2](0,1100;0,2100;1000,2100;1000,1100)%1%2+GLOBAL+GLOBAL2"
  );
}

TEST(23_LocalClustersWithEdges)
{
  db::Layout layout;
  db::Cell &cell = layout.cell (layout.add_cell ("TOP"));

  db::Edge edge;

  tl::from_string ("(0,0;0,500)", edge);
  cell.shapes (0).insert (edge);

  tl::from_string ("(0,500;0,1000)", edge);
  cell.shapes (0).insert (edge);

  tl::from_string ("(0,1000;2000,1000)", edge);
  cell.shapes (0).insert (edge);

  tl::from_string ("(2000,1000;2000,500)", edge);
  cell.shapes (0).insert (edge);

  tl::from_string ("(2000,500;1000,250)", edge);
  cell.shapes (0).insert (edge);

  tl::from_string ("(1500,375;0,0)", edge);
  cell.shapes (0).insert (edge);

  {
    //  edge clusters are for intra-layer mainly
    db::Connectivity conn;
    conn.connect (0);

    db::local_clusters<db::Edge> clusters;
    clusters.build_clusters (cell, conn);
    EXPECT_EQ (local_clusters_to_string (clusters, conn),
      "#1:[0](0,0;0,500);[0](0,500;0,1000)\n"
      "#2:[0](2000,500;1000,250);[0](1500,375;0,0)\n"
      "#3:[0](0,1000;2000,1000)\n"
      "#4:[0](2000,1000;2000,500)"
    );
  }

  {
    //  edge clusters are for intra-layer mainly
    db::Connectivity conn (db::Connectivity::EdgesConnectByPoints);
    conn.connect (0);

    db::local_clusters<db::Edge> clusters;
    clusters.build_clusters (cell, conn);
    EXPECT_EQ (local_clusters_to_string (clusters, conn), "#1:[0](0,0;0,500);[0](0,500;0,1000);[0](1500,375;0,0);[0](0,1000;2000,1000);[0](2000,1000;2000,500);[0](2000,500;1000,250)");
  }
}

TEST(24_LocalClustersWithSoftConnections)
{
  db::Layout layout;
  db::Cell &cell = layout.cell (layout.add_cell ("TOP"));
  db::GenericRepository &repo = layout.shape_repository ();

  auto dbu = db::CplxTrans (layout.dbu ()).inverted ();

  unsigned int nwell = 0;
  unsigned int ntie = 1;
  unsigned int ptie = 2;
  unsigned int contact = 3;
  unsigned int metal1 = 4;

  cell.shapes (nwell).insert (db::PolygonRef (dbu * db::DPolygon (db::DBox (0.0, 4.0, 2.0, 8.0)), repo));
  cell.shapes (ntie).insert (db::PolygonRef (dbu * db::DPolygon (db::DBox (0.5, 5.0, 1.5, 7.0)), repo));
  cell.shapes (contact).insert (db::PolygonRef (dbu * db::DPolygon (db::DBox (0.8, 6.0, 1.2, 6.5)), repo));
  cell.shapes (metal1).insert (db::PolygonRef (dbu * db::DPolygon (db::DBox (0.0, 5.0, 2.0, 7.0)), repo));

  cell.shapes (ptie).insert (db::PolygonRef (dbu * db::DPolygon (db::DBox (0.5, 1.0, 1.5, 3.0)), repo));
  cell.shapes (contact).insert (db::PolygonRef (dbu * db::DPolygon (db::DBox (0.8, 2.0, 1.2, 2.5)), repo));
  cell.shapes (metal1).insert (db::PolygonRef (dbu * db::DPolygon (db::DBox (0.0, 1.0, 2.0, 3.0)), repo));

  db::Connectivity conn;
  conn.connect (nwell);
  conn.connect (ntie);
  conn.connect (ptie);
  conn.connect (contact);
  conn.connect (metal1);
  conn.soft_connect (ntie, nwell);
  conn.soft_connect (contact, ntie);
  conn.connect (metal1, contact);

  {
    db::local_clusters<db::PolygonRef> clusters;
    clusters.build_clusters (cell, conn);
    EXPECT_EQ (local_clusters_to_string (clusters, conn),
      "#1:[0](0,4000;0,8000;2000,8000;2000,4000)\n"
      "#2:[1](500,5000;500,7000;1500,7000;1500,5000)\n"
      "#3:[3](800,6000;800,6500;1200,6500;1200,6000);[4](0,5000;0,7000;2000,7000;2000,5000)\n"
      "#4:[3](800,2000;800,2500;1200,2500;1200,2000);[4](0,1000;0,3000;2000,3000;2000,1000)\n"
      "#5:[2](500,1000;500,3000;1500,3000;1500,1000)\n"
      "(#2->#1)\n"
      "(#3->#2)"
    );
  }

  conn.soft_connect (contact, ptie);

  {
    db::local_clusters<db::PolygonRef> clusters;
    clusters.build_clusters (cell, conn);
    EXPECT_EQ (local_clusters_to_string (clusters, conn),
      "#1:[0](0,4000;0,8000;2000,8000;2000,4000)\n"
      "#2:[1](500,5000;500,7000;1500,7000;1500,5000)\n"
      "#3:[3](800,6000;800,6500;1200,6500;1200,6000);[4](0,5000;0,7000;2000,7000;2000,5000)\n"
      "#4:[2](500,1000;500,3000;1500,3000;1500,1000)\n"
      "#5:[3](800,2000;800,2500;1200,2500;1200,2000);[4](0,1000;0,3000;2000,3000;2000,1000)\n"
      "(#2->#1)\n"
      "(#3->#2)\n"
      "(#5->#4)"
    );
  }

  conn.soft_connect_global (ptie, "BULK");

  {
    db::local_clusters<db::PolygonRef> clusters;
    clusters.build_clusters (cell, conn);
    EXPECT_EQ (local_clusters_to_string (clusters, conn),
      "#1:[0](0,4000;0,8000;2000,8000;2000,4000)\n"
      "#2:[1](500,5000;500,7000;1500,7000;1500,5000)\n"
      "#3:[3](800,6000;800,6500;1200,6500;1200,6000);[4](0,5000;0,7000;2000,7000;2000,5000)\n"
      "#4:[2](500,1000;500,3000;1500,3000;1500,1000)\n"
      "#5:[3](800,2000;800,2500;1200,2500;1200,2000);[4](0,1000;0,3000;2000,3000;2000,1000)\n"
      "#6:+BULK\n"
      "(#2->#1)\n"
      "(#3->#2)\n"
      "(#5->#4)\n"
      "(#4->#6)"
    );
  }
}

TEST(30_LocalConnectedClusters)
{
  db::Layout layout;
  db::cell_index_type ci1 = layout.add_cell ("C1");
  db::cell_index_type ci2 = layout.add_cell ("C2");
  db::cell_index_type ci3 = layout.add_cell ("C3");

  db::Instance i1 = layout.cell (ci1).insert (db::CellInstArray (db::CellInst (ci2), db::Trans ()));
  db::Instance i2 = layout.cell (ci2).insert (db::CellInstArray (db::CellInst (ci3), db::Trans ()));

  db::connected_clusters<db::PolygonRef> cc;

  db::connected_clusters<db::PolygonRef>::connections_type x;
  db::connected_clusters<db::PolygonRef>::connections_type::const_iterator ix;

  x = cc.connections_for_cluster (1);
  EXPECT_EQ (x.size (), size_t (0));
  x = cc.connections_for_cluster (2);
  EXPECT_EQ (x.size (), size_t (0));

  //  after this:
  //   [#1] -> i1:#1
  //        -> i2:#2
  cc.add_connection (1, db::ClusterInstance (1, db::InstElement (i1)));
  cc.add_connection (1, db::ClusterInstance (2, db::InstElement (i2)));

  x = cc.connections_for_cluster (1);
  EXPECT_EQ (x.size (), size_t (2));
  x = cc.connections_for_cluster (2);
  EXPECT_EQ (x.size (), size_t (0));

  //  after this:
  //   [#1] -> i1:#1
  //        -> i2:#2
  //   [#2] -> i2:#1
  cc.add_connection (2, db::ClusterInstance (1, db::InstElement (i2)));
  x = cc.connections_for_cluster (2);
  EXPECT_EQ (x.size (), size_t (1));

  cc.join_cluster_with (1, 2);
  x = cc.connections_for_cluster (1);
  EXPECT_EQ (x.size (), size_t (3));
  ix = x.begin ();
  EXPECT_EQ (ix->id (), size_t (1));
  EXPECT_EQ (*ix == db::ClusterInstance (ix->id (), i1.cell_index (), i1.complex_trans (), i1.prop_id ()), true);
  ++ix;
  EXPECT_EQ (ix->id (), size_t (2));
  EXPECT_EQ (*ix == db::ClusterInstance (ix->id (), i2.cell_index (), i2.complex_trans (), i2.prop_id ()), true);
  ++ix;
  EXPECT_EQ (ix->id (), size_t (1));
  EXPECT_EQ (*ix == db::ClusterInstance (ix->id (), i2.cell_index (), i2.complex_trans (), i2.prop_id ()), true);

  x = cc.connections_for_cluster (2);
  EXPECT_EQ (x.size (), size_t (0));

  //  after this:
  //   [#1] -> i1:#1
  //        -> i2:#2
  //   [#2] -> i2:#1
  //        -> i1:#3
  cc.add_connection (2, db::ClusterInstance (3, db::InstElement (i1)));

  EXPECT_EQ (cc.find_cluster_with_connection (db::ClusterInstance (3, db::InstElement (i1))), size_t (2));
  EXPECT_EQ (cc.find_cluster_with_connection (db::ClusterInstance (2, db::InstElement (i1))), size_t (0));
  EXPECT_EQ (cc.find_cluster_with_connection (db::ClusterInstance (2, db::InstElement (i2))), size_t (1));

  //  after this:
  //   [#1] -> i1:#1
  //        -> i2:#2
  //        -> i2:#1
  //        -> i1:#3
  cc.join_cluster_with (1, 2);
  EXPECT_EQ (cc.find_cluster_with_connection (db::ClusterInstance (3, db::InstElement (i1))), size_t (1));
  EXPECT_EQ (cc.find_cluster_with_connection (db::ClusterInstance (1, db::InstElement (i1))), size_t (1));
  EXPECT_EQ (cc.find_cluster_with_connection (db::ClusterInstance (2, db::InstElement (i1))), size_t (0));
  EXPECT_EQ (cc.find_cluster_with_connection (db::ClusterInstance (2, db::InstElement (i2))), size_t (1));

  x = cc.connections_for_cluster (1);
  EXPECT_EQ (x.size (), size_t (4));
  x = cc.connections_for_cluster (2);
  EXPECT_EQ (x.size (), size_t (0));
}

static db::PolygonRef make_box (db::Layout &ly, const db::Box &box)
{
  return db::PolygonRef (db::Polygon (box), ly.shape_repository ());
}

TEST(40_HierClustersBasic)
{
  db::hier_clusters<db::PolygonRef> hc;

  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));

  db::Cell &top = ly.cell (ly.add_cell ("TOP"));
  top.shapes (l1).insert (make_box (ly, db::Box (0, 0, 1000, 1000)));

  db::Cell &c1 = ly.cell (ly.add_cell ("C1"));
  c1.shapes (l1).insert (make_box (ly, db::Box (0, 0, 2000, 500)));
  top.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans ()));

  db::Cell &c2 = ly.cell (ly.add_cell ("C2"));
  c2.shapes (l1).insert (make_box (ly, db::Box (0, 0, 500, 2000)));
  c2.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans ()));
  top.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans ()));

  db::Connectivity conn;
  conn.connect (l1, l1);

  hc.build (ly, top, conn);

  int n, nc;
  const db::connected_clusters<db::PolygonRef> *cluster;

  //  1 cluster in TOP with 2 connections
  n = 0;
  cluster = &hc.clusters_per_cell (top.cell_index ());
  for (db::connected_clusters<db::PolygonRef>::const_iterator i = cluster->begin (); i != cluster->end (); ++i) {
    ++n;
  }
  EXPECT_EQ (n, 1);
  EXPECT_EQ (cluster->bbox ().to_string (), "(0,0;1000,1000)")
  nc = 0;
  for (db::connected_clusters<db::PolygonRef>::connections_iterator i = cluster->begin_connections (); i != cluster->end_connections (); ++i) {
    nc += int (i->second.size ());
  }
  EXPECT_EQ (nc, 2);

  //  1 cluster in C1 without connection
  n = 0;
  cluster = &hc.clusters_per_cell (c1.cell_index ());
  for (db::connected_clusters<db::PolygonRef>::const_iterator i = cluster->begin (); i != cluster->end (); ++i) {
    ++n;
  }
  EXPECT_EQ (n, 1);
  EXPECT_EQ (cluster->bbox ().to_string (), "(0,0;2000,500)")
  nc = 0;
  for (db::connected_clusters<db::PolygonRef>::connections_iterator i = cluster->begin_connections (); i != cluster->end_connections (); ++i) {
    nc += int (i->second.size ());
  }
  EXPECT_EQ (nc, 0);

  //  1 cluster in C2 with one connection
  n = 0;
  cluster = &hc.clusters_per_cell (c2.cell_index ());
  for (db::connected_clusters<db::PolygonRef>::const_iterator i = cluster->begin (); i != cluster->end (); ++i) {
    ++n;
  }
  EXPECT_EQ (n, 1);
  EXPECT_EQ (cluster->bbox ().to_string (), "(0,0;500,2000)")
  nc = 0;
  for (db::connected_clusters<db::PolygonRef>::connections_iterator i = cluster->begin_connections (); i != cluster->end_connections (); ++i) {
    nc += int (i->second.size ());
  }
  EXPECT_EQ (nc, 1);
}

static std::string path2string (const db::Layout &ly, db::cell_index_type ci, const std::vector<db::ClusterInstance> &path)
{
  std::string res = ly.cell_name (ci);
  for (std::vector<db::ClusterInstance>::const_iterator p = path.begin (); p != path.end (); ++p) {
    res += "/";
    res += ly.cell_name (p->inst_cell_index ());
  }
  return res;
}

static std::string rcsiter2string (const db::Layout &ly, db::cell_index_type ci, db::recursive_cluster_shape_iterator<db::PolygonRef> si, db::cell_index_type ci2skip = std::numeric_limits<db::cell_index_type>::max ())
{
  std::string res;
  while (! si.at_end ()) {
    if (si.cell_index () == ci2skip) {
      si.skip_cell ();
      continue;
    }
    db::Polygon poly = si->obj ();
    poly.transform (si->trans ());
    poly.transform (si.trans ());
    if (! res.empty ()) {
      res += ";";
    }
    res += path2string (ly, ci, si.inst_path ());
    res += ":";
    res += poly.to_string ();
    ++si;
  }
  return res;
}

static std::string rciter2string (const db::Layout &ly, db::cell_index_type ci, db::recursive_cluster_iterator<db::PolygonRef> si)
{
  std::string res;
  while (! si.at_end ()) {
    if (! res.empty ()) {
      res += ";";
    }
    res += path2string (ly, ci, si.inst_path ());
    ++si;
  }
  return res;
}

TEST(41_HierClustersRecursiveClusterShapeIterator)
{
  db::hier_clusters<db::PolygonRef> hc;

  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));

  db::Cell &top = ly.cell (ly.add_cell ("TOP"));
  top.shapes (l1).insert (make_box (ly, db::Box (0, 0, 1000, 1000)));

  db::Cell &c1 = ly.cell (ly.add_cell ("C1"));
  c1.shapes (l1).insert (make_box (ly, db::Box (0, 0, 2000, 500)));
  top.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (0, 10))));

  db::Cell &c2 = ly.cell (ly.add_cell ("C2"));
  c2.shapes (l1).insert (make_box (ly, db::Box (0, 0, 500, 2000)));
  c2.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (0, 20))));
  top.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (0, 30))));

  db::Connectivity conn;
  conn.connect (l1, l1);

  hc.build (ly, top, conn);

  std::string res;
  int n = 0;
  db::connected_clusters<db::PolygonRef> *cluster = &hc.clusters_per_cell (top.cell_index ());
  for (db::connected_clusters<db::PolygonRef>::const_iterator i = cluster->begin (); i != cluster->end (); ++i) {
    res = rcsiter2string (ly, top.cell_index (), db::recursive_cluster_shape_iterator<db::PolygonRef> (hc, l1, top.cell_index (), i->id ()));
    ++n;
  }
  EXPECT_EQ (n, 1);
  EXPECT_EQ (res, "TOP:(0,0;0,1000;1000,1000;1000,0);TOP/C1:(0,10;0,510;2000,510;2000,10);TOP/C2:(0,30;0,2030;500,2030;500,30);TOP/C2/C1:(0,50;0,550;2000,550;2000,50)");

  res.clear ();
  n = 0;
  cluster = &hc.clusters_per_cell (top.cell_index ());
  for (db::connected_clusters<db::PolygonRef>::const_iterator i = cluster->begin (); i != cluster->end (); ++i) {
    res = rcsiter2string (ly, top.cell_index (), db::recursive_cluster_shape_iterator<db::PolygonRef> (hc, l1, top.cell_index (), i->id ()), c1.cell_index ());
    ++n;
  }
  EXPECT_EQ (n, 1);
  EXPECT_EQ (res, "TOP:(0,0;0,1000;1000,1000;1000,0);TOP/C2:(0,30;0,2030;500,2030;500,30)");
}

TEST(41_HierClustersRecursiveClusterIterator)
{
  db::hier_clusters<db::PolygonRef> hc;

  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));

  db::Cell &top = ly.cell (ly.add_cell ("TOP"));
  top.shapes (l1).insert (make_box (ly, db::Box (0, 0, 1000, 1000)));

  db::Cell &c1 = ly.cell (ly.add_cell ("C1"));
  c1.shapes (l1).insert (make_box (ly, db::Box (0, 0, 2000, 500)));
  top.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (0, 10))));

  db::Cell &c2 = ly.cell (ly.add_cell ("C2"));
  c2.shapes (l1).insert (make_box (ly, db::Box (0, 0, 500, 2000)));
  c2.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (0, 20))));
  top.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (0, 30))));

  db::Connectivity conn;
  conn.connect (l1, l1);

  hc.build (ly, top, conn);

  std::string res;
  int n = 0;
  db::connected_clusters<db::PolygonRef> *cluster = &hc.clusters_per_cell (top.cell_index ());
  for (db::connected_clusters<db::PolygonRef>::const_iterator i = cluster->begin (); i != cluster->end (); ++i) {
    res = rciter2string (ly, top.cell_index (), db::recursive_cluster_iterator<db::PolygonRef> (hc, top.cell_index (), i->id ()));
    ++n;
  }
  EXPECT_EQ (n, 1);
  EXPECT_EQ (res, "TOP;TOP/C1;TOP/C2;TOP/C2/C1");
}

static void normalize_layer (db::Layout &layout, std::vector<std::string> &strings, unsigned int &layer)
{
  unsigned int new_layer = layout.insert_layer ();

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    const db::Shapes &s = c->shapes (layer);
    for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::Texts | db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes); !i.at_end (); ++i) {
      if (! i->is_text ()) {
        db::Polygon poly;
        i->polygon (poly);
        c->shapes (new_layer).insert (db::PolygonRef (poly, layout.shape_repository ()));
      } else {
        db::Polygon poly (i->bbox ());
        unsigned int attr_id = (unsigned int) strings.size () + 1;
        strings.push_back (i->text_string ());
        c->shapes (new_layer).insert (db::PolygonRefWithProperties (db::PolygonRef (poly, layout.shape_repository ()), attr_id));
      }
    }
  }

  layer = new_layer;
}

static void copy_cluster_shapes (const std::string *&attrs, db::Shapes &out, db::cell_index_type ci, const db::hier_clusters<db::PolygonRef> &hc, db::local_cluster<db::PolygonRef>::id_type cluster_id, const db::ICplxTrans &trans, const db::Connectivity &conn)
{
  //  use property #1 to code the cell name
  //  use property #2 to code the attrs string for the first shape

  db::properties_id_type cell_pid = 0, cell_and_attr_pid = 0;

  db::PropertiesSet pm;
  pm.insert (tl::Variant (1), tl::Variant (out.layout ()->cell_name (ci)));
  cell_pid = db::properties_id (pm);

  if (attrs && ! attrs->empty ()) {
    pm.insert (tl::Variant (2), tl::Variant (*attrs));
    cell_and_attr_pid = db::properties_id (pm);
  }

  const db::connected_clusters<db::PolygonRef> &clusters = hc.clusters_per_cell (ci);
  const db::local_cluster<db::PolygonRef> &lc = clusters.cluster_by_id (cluster_id);

  //  copy the shapes from this cell
  for (db::Connectivity::all_layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {
    for (db::local_cluster<db::PolygonRef>::shape_iterator s = lc.begin (*l); ! s.at_end (); ++s) {
      db::Polygon poly = s->obj ().transformed (trans * db::ICplxTrans (s->trans ()));
      out.insert (db::PolygonWithProperties (poly, cell_and_attr_pid > 0 ? cell_and_attr_pid : cell_pid));
      cell_and_attr_pid = 0;
      attrs = 0; // used
    }
  }

  out.layout ()->cell_name (ci);

  //  copy the shapes from the child cells too
  typedef db::connected_clusters<db::PolygonRef>::connections_type connections_type;
  const connections_type &connections = clusters.connections_for_cluster (cluster_id);
  for (connections_type::const_iterator i = connections.begin (); i != connections.end (); ++i) {

    db::ICplxTrans t = trans * i->inst_trans ();

    db::cell_index_type cci = i->inst_cell_index ();
    copy_cluster_shapes (attrs, out, cci, hc, i->id (), t, conn);

  }
}

static void run_hc_test (tl::TestBase *_this, const std::string &file, const std::string &au_file)
{
  db::Layout ly;
  unsigned int l1 = 0, l2 = 0, l3 = 0, l4 = 0, l5 = 0, l6 = 0;

  {
    db::LayerProperties p;
    db::LayerMap lmap;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l1 = ly.insert_layer ());
    ly.set_properties (l1, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l2 = ly.insert_layer ());
    ly.set_properties (l2, p);

    p.layer = 3;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l3 = ly.insert_layer ());
    ly.set_properties (l3, p);

    p.layer = 4;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l4 = ly.insert_layer ());
    ly.set_properties (l4, p);

    p.layer = 5;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l5 = ly.insert_layer ());
    ly.set_properties (l5, p);

    p.layer = 6;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l6 = ly.insert_layer ());
    ly.set_properties (l6, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn += "/algo/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  std::vector<std::string> strings;
  normalize_layer (ly, strings, l1);
  normalize_layer (ly, strings, l2);
  normalize_layer (ly, strings, l3);
  normalize_layer (ly, strings, l4);
  normalize_layer (ly, strings, l5);
  normalize_layer (ly, strings, l6);

  //  connect 1 to 1, 1 to 2 and 1 to 3, but *not* 2 to 3
  db::Connectivity conn;
  conn.connect (l1, l1);
  conn.connect (l2, l2);
  conn.connect (l3, l3);
  conn.connect (l1, l2);
  conn.connect (l1, l3);
  conn.connect (l1, l4);
  conn.connect (l1, l5);
  conn.connect (l1, l6);

  conn.connect_global (l4, "BULK");
  conn.connect_global (l5, "BULK2");
  conn.connect_global (l6, "BULK");
  conn.connect_global (l6, "BULK2");

  db::hier_clusters<db::PolygonRef> hc;
  hc.build (ly, ly.cell (*ly.begin_top_down ()), conn);

  std::vector<std::pair<db::Polygon::area_type, unsigned int> > net_layers;

  for (db::Layout::top_down_const_iterator td = ly.begin_top_down (); td != ly.end_top_down (); ++td) {

    const db::connected_clusters<db::PolygonRef> &clusters = hc.clusters_per_cell (*td);
    for (db::connected_clusters<db::PolygonRef>::all_iterator c = clusters.begin_all (); ! c.at_end (); ++c) {

      if (! clusters.is_root (*c)) {
        continue;
      }

      //  collect strings
      std::string attrs;
      for (db::recursive_cluster_iterator<db::PolygonRef> rc (hc, *td, *c); ! rc.at_end (); ++rc) {
        const db::local_cluster<db::PolygonRef> &rcc = hc.clusters_per_cell (rc.cell_index ()).cluster_by_id (rc.cluster_id ());
        for (db::local_cluster<db::PolygonRef>::attr_iterator a = rcc.begin_attr (); a != rcc.end_attr (); ++a) {
          if (! attrs.empty ()) {
            attrs += "/";
          }
          attrs += std::string (ly.cell_name (rc.cell_index ())) + ":" + strings[*a - 1];
        }
      }

      net_layers.push_back (std::make_pair (0, ly.insert_layer ()));

      unsigned int lout = net_layers.back ().second;

      db::Shapes &out = ly.cell (*td).shapes (lout);
      const std::string *attrs_str = &attrs;
      copy_cluster_shapes (attrs_str, out, *td, hc, *c, db::ICplxTrans (), conn);

      db::Polygon::area_type area = 0;
      for (db::Shapes::shape_iterator s = out.begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
        area += s->area ();
      }
      net_layers.back ().first = area;

    }

  }

  //  sort layers by area so we have a consistent numbering
  std::sort (net_layers.begin (), net_layers.end ());
  std::reverse (net_layers.begin (), net_layers.end ());

  int ln = 1000;
  for (std::vector<std::pair<db::Polygon::area_type, unsigned int> >::const_iterator l = net_layers.begin (); l != net_layers.end (); ++l) {
    ly.set_properties (l->second, db::LayerProperties (ln, 0));
    ++ln;
  }

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/" + au_file);
}

static void run_hc_test_with_backannotation (tl::TestBase *_this, const std::string &file, const std::string &au_file)
{
  db::Layout ly;
  unsigned int l1 = 0, l2 = 0, l3 = 0, l4 = 0, l5 = 0, l6 = 0;

  {
    db::LayerProperties p;
    db::LayerMap lmap;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l1 = ly.insert_layer ());
    ly.set_properties (l1, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l2 = ly.insert_layer ());
    ly.set_properties (l2, p);

    p.layer = 3;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l3 = ly.insert_layer ());
    ly.set_properties (l3, p);

    p.layer = 4;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l4 = ly.insert_layer ());
    ly.set_properties (l4, p);

    p.layer = 5;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l5 = ly.insert_layer ());
    ly.set_properties (l5, p);

    p.layer = 6;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l6 = ly.insert_layer ());
    ly.set_properties (l6, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn += "/algo/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  std::vector<std::string> strings;
  normalize_layer (ly, strings, l1);
  normalize_layer (ly, strings, l2);
  normalize_layer (ly, strings, l3);
  normalize_layer (ly, strings, l4);
  normalize_layer (ly, strings, l5);
  normalize_layer (ly, strings, l6);

  //  connect 1 to 1, 1 to 2 and 1 to 3, but *not* 2 to 3
  db::Connectivity conn;
  conn.connect (l1, l1);
  conn.connect (l2, l2);
  conn.connect (l3, l3);
  conn.connect (l1, l2);
  conn.connect (l1, l3);
  conn.connect (l1, l4);
  conn.connect (l1, l5);
  conn.connect (l1, l6);

  conn.connect_global (l4, "BULK");
  conn.connect_global (l5, "BULK2");
  conn.connect_global (l6, "BULK");
  conn.connect_global (l6, "BULK2");

  db::hier_clusters<db::PolygonRef> hc;
  hc.build (ly, ly.cell (*ly.begin_top_down ()), conn);

  std::map<unsigned int, unsigned int> lm;
  lm[l1] = ly.insert_layer (db::LayerProperties (101, 0));
  lm[l2] = ly.insert_layer (db::LayerProperties (102, 0));
  lm[l3] = ly.insert_layer (db::LayerProperties (103, 0));
  lm[l4] = ly.insert_layer (db::LayerProperties (104, 0));
  lm[l5] = ly.insert_layer (db::LayerProperties (105, 0));
  lm[l6] = ly.insert_layer (db::LayerProperties (106, 0));
  hc.return_to_hierarchy (ly, lm);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/" + au_file);
}

TEST(101_HierClusters)
{
  run_hc_test (_this, "hc_test_l1.gds", "hc_test_au1.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l1.gds", "hc_test_au1b.gds");
}

TEST(102_HierClusters)
{
  run_hc_test (_this, "hc_test_l2.gds", "hc_test_au2.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l2.gds", "hc_test_au2b.gds");
}

TEST(103_HierClusters)
{
  run_hc_test (_this, "hc_test_l3.gds", "hc_test_au3.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l3.gds", "hc_test_au3b.gds");
}

TEST(104_HierClusters)
{
  run_hc_test (_this, "hc_test_l4.gds", "hc_test_au4.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l4.gds", "hc_test_au4b.gds");
}

TEST(105_HierClusters)
{
  run_hc_test (_this, "hc_test_l5.gds", "hc_test_au5.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l5.gds", "hc_test_au5b.gds");
}

TEST(106_HierClusters)
{
  run_hc_test (_this, "hc_test_l6.gds", "hc_test_au6.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l6.gds", "hc_test_au6b.gds");
}

TEST(107_HierClusters)
{
  run_hc_test (_this, "hc_test_l7.gds", "hc_test_au7.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l7.gds", "hc_test_au7b.gds");
}

TEST(108_HierClusters)
{
  run_hc_test (_this, "hc_test_l8.gds", "hc_test_au8.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l8.gds", "hc_test_au8b.gds");
}

TEST(109_HierClusters)
{
  run_hc_test (_this, "hc_test_l9.gds", "hc_test_au9.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l9.gds", "hc_test_au9b.gds");
}

TEST(110_HierClusters)
{
  run_hc_test (_this, "hc_test_l10.gds", "hc_test_au10.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l10.gds", "hc_test_au10b.gds");
}

TEST(111_HierClusters)
{
  run_hc_test (_this, "hc_test_l11.gds", "hc_test_au11.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l11.gds", "hc_test_au11b.gds");
}

TEST(112_HierClusters)
{
  run_hc_test (_this, "hc_test_l12.gds", "hc_test_au12.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l12.gds", "hc_test_au12b.gds");
}

TEST(113_HierClusters)
{
  run_hc_test (_this, "hc_test_l13.gds", "hc_test_au13.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l13.gds", "hc_test_au13b.gds");
}

TEST(114_HierClusters)
{
  run_hc_test (_this, "hc_test_l14.gds", "hc_test_au14.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l14.gds", "hc_test_au14b.gds");
}

TEST(115_HierClusters)
{
  run_hc_test (_this, "hc_test_l15.gds", "hc_test_au15.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l15.gds", "hc_test_au15b.gds");
}

TEST(116_HierClusters)
{
  run_hc_test (_this, "hc_test_l16.gds", "hc_test_au16.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l16.gds", "hc_test_au16b.gds");
}

TEST(117_HierClusters)
{
  run_hc_test (_this, "hc_test_l17.gds", "hc_test_au17.gds");
  run_hc_test_with_backannotation (_this, "hc_test_l17.gds", "hc_test_au17b.gds");
}

TEST(118_HierClustersMeanderArrays)
{
  run_hc_test (_this, "meander.gds.gz", "meander_au1.gds");
  run_hc_test_with_backannotation (_this, "meander.gds.gz", "meander_au2.gds");
}

TEST(119_HierClustersCombArrays)
{
  run_hc_test (_this, "comb.gds", "comb_au1.gds");
  run_hc_test_with_backannotation (_this, "comb.gds", "comb_au2.gds");
}

TEST(120_HierClustersCombArrays)
{
  run_hc_test (_this, "comb2.gds", "comb2_au1.gds");
  run_hc_test_with_backannotation (_this, "comb2.gds", "comb2_au2.gds");
}

static size_t root_nets (const db::connected_clusters<db::PolygonRef> &cc)
{
  size_t n = 0;
  for (db::connected_clusters<db::PolygonRef>::all_iterator c = cc.begin_all (); !c.at_end (); ++c) {
    if (cc.is_root (*c)) {
      ++n;
    }
  }
  return n;
}

//  issue #609
TEST(200_issue609)
{
  db::Layout ly;
  unsigned int l1 = 0, l2 = 0;

  {
    db::LayerProperties p;
    db::LayerMap lmap;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l1 = ly.insert_layer ());
    ly.set_properties (l1, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (p.layer, p.datatype), l2 = ly.insert_layer ());
    ly.set_properties (l2, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn += "/algo/issue-609.oas.gz";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  std::vector<std::string> strings;
  normalize_layer (ly, strings, l1);
  normalize_layer (ly, strings, l2);

  //  connect 1 to 1, 1 to 2
  db::Connectivity conn;
  conn.connect (l1, l1);
  conn.connect (l2, l2);
  conn.connect (l1, l2);

  db::hier_clusters<db::PolygonRef> hc;
  hc.build (ly, ly.cell (*ly.begin_top_down ()), conn);

  db::Layout::top_down_const_iterator td = ly.begin_top_down ();
  EXPECT_EQ (td != ly.end_top_down (), true);
  EXPECT_EQ (root_nets (hc.clusters_per_cell (*td)), size_t (1));
  ++td;

  //  result needs to be a single net
  for ( ; td != ly.end_top_down (); ++td) {
    EXPECT_EQ (root_nets (hc.clusters_per_cell (*td)), size_t (0));
  }
}

//  issue #1126
TEST(201_issue1126)
{
  {
    db::Layout ly;
    unsigned int l1 = 0;

    {
      db::LayerProperties p;
      db::LayerMap lmap;

      p.layer = 1;
      p.datatype = 0;
      lmap.map (db::LDPair (p.layer, p.datatype), l1 = ly.insert_layer ());
      ly.set_properties (l1, p);

      db::LoadLayoutOptions options;
      options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
      options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

      std::string fn (tl::testdata ());
      fn += "/algo/issue-1126.gds.gz";
      tl::InputStream stream (fn);
      db::Reader reader (stream);
      reader.read (ly, options);
    }

    std::vector<std::string> strings;
    normalize_layer (ly, strings, l1);

    //  connect 1 to 1
    db::Connectivity conn;
    conn.connect (l1, l1);

    db::hier_clusters<db::PolygonRef> hc;
    hc.build (ly, ly.cell (*ly.begin_top_down ()), conn);

    // should not assert until here
  }

  //  detailed test:
  run_hc_test (_this, "issue-1126.gds.gz", "issue-1126_au.gds");
}

