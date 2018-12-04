
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
#include "dbStream.h"
#include "dbCommonReader.h"

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
  EXPECT_EQ (ix->inst () == db::InstElement (i1), true);
  ++ix;
  EXPECT_EQ (ix->id (), size_t (2));
  EXPECT_EQ (ix->inst () == db::InstElement (i2), true);
  ++ix;
  EXPECT_EQ (ix->id (), size_t (1));
  EXPECT_EQ (ix->inst () == db::InstElement (i2), true);

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

static void normalize_layer (db::Layout &layout, unsigned int layer)
{
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes s (layout.is_editable ());
    s.swap (c->shapes (layer));
    for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes); !i.at_end (); ++i) {
      db::Polygon poly;
      i->polygon (poly);
      c->shapes (layer).insert (db::PolygonRef (poly, layout.shape_repository ()));
    }
  }
}

static void copy_cluster_shapes (db::Shapes &out, db::cell_index_type ci, const db::hier_clusters<db::PolygonRef> &hc, db::local_cluster<db::PolygonRef>::id_type cluster_id, const db::ICplxTrans &trans, const db::Connectivity &conn)
{
  //  use property #1 to code the cell name

  db::PropertiesRepository &pr = out.layout ()->properties_repository ();
  db::property_names_id_type pn_id = pr.prop_name_id (tl::Variant (1));
  db::PropertiesRepository::properties_set pm;
  pm.insert (std::make_pair (pn_id, tl::Variant (out.layout ()->cell_name (ci))));
  db::properties_id_type cell_pid = pr.properties_id (pm);

  const db::connected_clusters<db::PolygonRef> &clusters = hc.clusters_per_cell (ci);
  const db::local_cluster<db::PolygonRef> &lc = clusters.cluster_by_id (cluster_id);

  //  copy the shapes from this cell
  for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {
    for (db::local_cluster<db::PolygonRef>::shape_iterator s = lc.begin (*l); ! s.at_end (); ++s) {
      db::Polygon poly = s->obj ().transformed (trans * db::ICplxTrans (s->trans ()));
      out.insert (db::PolygonWithProperties (poly, cell_pid));
    }
  }

  out.layout ()->cell_name (ci);

  //  copy the shapes from the child cells too
  typedef db::connected_clusters<db::PolygonRef>::connections_type connections_type;
  const connections_type &connections = clusters.connections_for_cluster (cluster_id);
  for (connections_type::const_iterator i = connections.begin (); i != connections.end (); ++i) {

    db::ICplxTrans t = trans * i->inst ().complex_trans ();

    db::cell_index_type cci = i->inst ().inst_ptr.cell_index ();
    copy_cluster_shapes (out, cci, hc, i->id (), t, conn);

  }
}

static void run_hc_test (tl::TestBase *_this, const std::string &file, const std::string &au_file)
{
  db::Layout ly;
  unsigned int l1 = 0, l2 = 0, l3 = 0;

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

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  normalize_layer (ly, l1);
  normalize_layer (ly, l2);
  normalize_layer (ly, l3);

  //  connect 1 to 1, 1 to 2 and 1 to 3, but *not* 2 to 3
  db::Connectivity conn;
  conn.connect (l1, l1);
  conn.connect (l2, l2);
  conn.connect (l3, l3);
  conn.connect (l1, l2);
  conn.connect (l1, l3);

  db::hier_clusters<db::PolygonRef> hc;
  hc.build (ly, ly.cell (*ly.begin_top_down ()), db::ShapeIterator::Polygons, conn);

  std::vector<std::pair<db::Polygon::area_type, unsigned int> > net_layers;

  for (db::Layout::top_down_const_iterator td = ly.begin_top_down (); td != ly.end_top_down (); ++td) {

    const db::connected_clusters<db::PolygonRef> &clusters = hc.clusters_per_cell (*td);
    for (db::connected_clusters<db::PolygonRef>::all_iterator c = clusters.begin_all (); ! c.at_end (); ++c) {

      net_layers.push_back (std::make_pair (0, ly.insert_layer ()));

      unsigned int lout = net_layers.back ().second;

      db::Shapes &out = ly.cell (*td).shapes (lout);
      copy_cluster_shapes (out, *td, hc, *c, db::ICplxTrans (), conn);

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
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/" + au_file);
}

TEST(41_HierClusters)
{
  run_hc_test (_this, "hc_test_l1.gds", "hc_test_au1.gds");
}

TEST(42_HierClusters)
{
  run_hc_test (_this, "hc_test_l2.gds", "hc_test_au2.gds");
}

TEST(43_HierClusters)
{
  run_hc_test (_this, "hc_test_l3.gds", "hc_test_au3.gds");
}

TEST(44_HierClusters)
{
  run_hc_test (_this, "hc_test_l4.gds", "hc_test_au4.gds");
}

TEST(45_HierClusters)
{
  run_hc_test (_this, "hc_test_l5.gds", "hc_test_au5.gds");
}

TEST(46_HierClusters)
{
  run_hc_test (_this, "hc_test_l6.gds", "hc_test_au6.gds");
}

TEST(47_HierClusters)
{
  run_hc_test (_this, "hc_test_l7.gds", "hc_test_au7.gds");
}

TEST(48_HierClusters)
{
  run_hc_test (_this, "hc_test_l8.gds", "hc_test_au8.gds");
}
