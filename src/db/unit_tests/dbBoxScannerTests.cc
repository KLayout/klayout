
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


#include "dbBoxScanner.h"
#include "dbShapeProcessor.h"
#include "dbRecursiveShapeIterator.h"
#include "tlUnitTest.h"

#include "tlTimer.h"
#include "tlLog.h"

struct BoxScannerTestRecorder
{
  void finish (const db::Box * /*box*/, size_t p) {
    str += "<" + tl::to_string (p) + ">";
  }

  bool stop () const { return false; }
  void initialize () { str += "[i]"; }
  void finalize (bool) { str += "[f]"; }

  void add (const db::Box * /*b1*/, size_t p1, const db::Box * /*b2*/, size_t p2)
  {
    str += "(" + tl::to_string (p1) + "-" + tl::to_string (p2) + ")";
  }

  std::string str;
};

struct BoxScannerTestRecorderStopping
{
  BoxScannerTestRecorderStopping () : do_stop (false) { }

  void finish (const db::Box * /*box*/, size_t p) {
    str += "<" + tl::to_string (p) + ">";
  }

  bool stop () const { return do_stop; }
  void initialize () { str += "[i]"; }
  void finalize (bool s) { str += s ? "[f+]" : "[f-]"; }

  void add (const db::Box * /*b1*/, size_t p1, const db::Box * /*b2*/, size_t p2)
  {
    str += "(" + tl::to_string (p1) + "-" + tl::to_string (p2) + ")";
    do_stop = true;
  }

  std::string str;
  bool do_stop;
};

struct BoxScannerTestRecorder2
{
  void finish (const db::Box *, size_t) { }

  bool stop () const { return false; }
  void initialize () { }
  void finalize (bool) { }

  void add (const db::Box * /*b1*/, size_t p1, const db::Box * /*b2*/, size_t p2)
  {
    interactions.insert (std::make_pair (p1, p2));
    interactions.insert (std::make_pair (p2, p1));
  }

  std::set<std::pair<size_t, size_t> > interactions;
};

struct BoxScannerTestRecorderTwo
{
  void finish1 (const db::Box * /*box*/, size_t p) {
    str += "<" + tl::to_string (p) + ">";
  }

  void finish2 (const db::SimplePolygon * /*poly*/, int p) {
    str += "<" + tl::to_string (p) + ">";
  }

  bool stop () const { return false; }
  void initialize () { str += "[i]"; }
  void finalize (bool) { str += "[f]"; }

  void add (const db::Box * /*b1*/, size_t p1, const db::SimplePolygon * /*b2*/, int p2)
  {
    str += "(" + tl::to_string (p1) + "-" + tl::to_string (p2) + ")";
  }

  std::string str;
};

struct BoxScannerTestRecorderTwoStopping
{
  BoxScannerTestRecorderTwoStopping () : do_stop (false) { }

  void finish1 (const db::Box * /*box*/, size_t p) {
    str += "<" + tl::to_string (p) + ">";
  }

  void finish2 (const db::SimplePolygon * /*poly*/, int p) {
    str += "<" + tl::to_string (p) + ">";
  }

  bool stop () const { return do_stop; }
  void initialize () { str += "[i]"; }
  void finalize (bool s) { str += s ? "[f+]" : "[f-]"; }

  void add (const db::Box * /*b1*/, size_t p1, const db::SimplePolygon * /*b2*/, int p2)
  {
    str += "(" + tl::to_string (p1) + "-" + tl::to_string (p2) + ")";
    do_stop = true;
  }

  std::string str;
  bool do_stop;
};

struct BoxScannerTestRecorder2Two
{
  void finish1 (const db::Box *, size_t) { }
  void finish2 (const db::SimplePolygon *, int) { }

  bool stop () const { return false; }
  void initialize () { }
  void finalize (bool) { }

  void add (const db::Box * /*b1*/, size_t p1, const db::SimplePolygon * /*b2*/, int p2)
  {
    interactions.insert (std::make_pair (p1, p2));
  }

  std::set<std::pair<size_t, int> > interactions;
};

TEST(1)
{
  db::box_scanner<db::Box, size_t> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 210, 200, 310));
  bb.push_back (db::Box (10, 220, 210, 320));
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (50, 50, 150, 150));
  bb.push_back (db::Box (10, 10, 110, 110));
  bb.push_back (db::Box (100, 10, 200, 110));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  EXPECT_EQ (bs.process (tr, 1, bc), true);
  EXPECT_EQ (tr.str, "[i](4-2)(5-2)(5-4)(3-2)(3-4)(5-3)<2><5><4><3>(1-0)<0><1>[f]");

  BoxScannerTestRecorderStopping trstop;
  EXPECT_EQ (bs.process (trstop, 1, bc), false);
  EXPECT_EQ (trstop.str, "[i](4-2)[f-]");
}

TEST(1a)
{
  db::box_scanner<db::Box, size_t> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 0, 200, 310));
  bb.push_back (db::Box (0, 0, 100, 100));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (tr, 1, bc);
  EXPECT_EQ (tr.str, "[i](1-0)<0><1>[f]");
}

TEST(1b)
{
  db::box_scanner<db::Box, size_t> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (200, 0, 300, 100));
  bb.push_back (db::Box (400, 0, 500, 100));
  bb.push_back (db::Box (100, 0, 200, 100));
  bb.push_back (db::Box (300, 0, 400, 100));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (tr, 1, bc);
  EXPECT_EQ (tr.str, "[i](3-0)(1-3)(4-1)(2-4)<0><3><1><4><2>[f]");
}

TEST(1c)
{
  db::box_scanner<db::Box, size_t> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (0, 200, 100, 300));
  bb.push_back (db::Box (0, 400, 100, 500));
  bb.push_back (db::Box (0, 100, 100, 200));
  bb.push_back (db::Box (0, 300, 100, 400));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (tr, 1, bc);
  EXPECT_EQ (tr.str, "[i](3-0)(1-3)<0>(4-1)<3>(2-4)<1><4><2>[f]");
}

TEST(1d)
{
  db::box_scanner<db::Box, size_t> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 0, 101, 100));
  bb.push_back (db::Box (200, 0, 300, 100));
  bb.push_back (db::Box (400, 0, 500, 100));
  bb.push_back (db::Box (100, 0, 200, 100));
  bb.push_back (db::Box (300, 0, 400, 100));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (tr, 0, bc);
  EXPECT_EQ (tr.str, "[i](3-0)<0><3><1><4><2>[f]");
}

TEST(1e)
{
  db::box_scanner<db::Box, size_t> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 0, 101, 100));
  bb.push_back (db::Box (200, 0, 300, 100));
  bb.push_back (db::Box (400, 0, 500, 100));
  bb.push_back (db::Box (100, 0, 200, 100));
  bb.push_back (db::Box (300, 0, 400, 100));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.process (tr, 0, bc);
  EXPECT_EQ (tr.str, "[i](0-3)<0><1><2><3><4>[f]");
}

TEST(1f)
{
  //  trivial case
  db::box_scanner<db::Box, size_t> bs;
  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.process (tr, 0, bc);
  EXPECT_EQ (tr.str, "[i][f]");
}

TEST(1g)
{
  //  empty elements
  db::box_scanner<db::Box, size_t> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 0, 101, 100));
  bb.push_back (db::Box (200, 0, 300, 100));
  bb.push_back (db::Box ());
  bb.push_back (db::Box (100, 0, 200, 100));
  bb.push_back (db::Box ());
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.process (tr, 0, bc);
  EXPECT_EQ (tr.str, "[i]<2><4>(0-3)<0><1><3>[f]");
}

void run_test2 (tl::TestBase *_this, size_t n, double ff, db::Coord spread, bool touch = true)
{
  std::vector<db::Box> bb;
  for (size_t i = 0; i < n; ++i) {
    db::Coord x = rand () % spread;
    db::Coord y = rand () % spread;
    bb.push_back (db::Box (x, y, x + 100, y + 100));
  }

  db::box_scanner<db::Box, size_t> bs;
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  BoxScannerTestRecorder2 tr;
  bs.set_fill_factor (ff);
  db::box_convert<db::Box> bc;
  {
    tl::SelfTimer timer ("box-scanner");
    bs.set_scanner_threshold (0);
    bs.process (tr, touch ? 1 : 0, bc);
  }

  std::set<std::pair<size_t, size_t> > interactions;
  {
    tl::SelfTimer timer ("brute-force");
    for (size_t i = 0; i < bb.size (); ++i) {
      for (size_t j = i + 1; j < bb.size (); ++j) {
        if ((touch && bb[i].touches (bb[j])) || (!touch && bb[i].overlaps (bb[j]))) {
          interactions.insert (std::make_pair (i, j));
          interactions.insert (std::make_pair (j, i));
        }
      }
    }
  }

  if (interactions != tr.interactions) {
    tl::info << "Interactions in 'brute force' but not in 'box-scanner':";
    for (std::set<std::pair<size_t, size_t> >::const_iterator i = interactions.begin (); i != interactions.end (); ++i) {
      if (tr.interactions.find (*i) == tr.interactions.end ()) {
        tl::info << "   " << i->first << "-" << i->second;
      }
    }
    tl::info << "Interactions in 'box-scanner' but not in 'brute force':";
    for (std::set<std::pair<size_t, size_t> >::const_iterator i = tr.interactions.begin (); i != tr.interactions.end (); ++i) {
      if (interactions.find (*i) == interactions.end ()) {
        tl::info << "   " << i->first << "-" << i->second;
      }
    }
  }
  EXPECT_EQ (interactions == tr.interactions, true);

}

TEST(2)
{
  run_test2(_this, 1000, 0.0, 1000);
  run_test2(_this, 1000, 2, 1000);
  run_test2(_this, 1000, 2, 1000, false);
  run_test2(_this, 1000, 2, 500);
  run_test2(_this, 1000, 2, 100);
  run_test2(_this, 10000, 2, 10000);
}


struct TestCluster
  : public db::cluster<db::Box, size_t>
{
  typedef db::cluster<db::Box, size_t> base_class;

  TestCluster (std::set<std::set<size_t> > *cl)
    : clusters (cl)
  { }

  void add (const db::Box *box, size_t p)
  {
    props.insert (p);
    base_class::add (box, p);
  }

  void join (const TestCluster &other) 
  {
    props.insert (other.props.begin (), other.props.end ());
    base_class::join (other);
  }

  void finish ()
  {
    clusters->insert(props);
  }

  std::set<size_t> props;
  std::set<std::set<size_t> > *clusters;
};

std::string c2s (const std::set<size_t> &cl) 
{
  std::string r;
  r += "(";
  for (std::set<size_t>::const_iterator cc = cl.begin (); cc != cl.end (); ++cc) {
    if (cc != cl.begin ()) {
      r += ",";
    }
    r += tl::to_string (*cc);
  }
  r += ")";
  return r;
}

std::string cl2s (const std::set<std::set<size_t> > &clusters)
{
  std::string r;
  for (std::set<std::set<size_t> >::const_iterator c = clusters.begin (); c != clusters.end (); ++c) {
    if (!r.empty ()) {
      r += ",";
    }
    r += c2s (*c);
  }
  return r;
}

TEST(10)
{
  db::box_scanner<db::Box, size_t> bs;
  std::set<std::set<size_t> > clusters;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 210, 200, 310));
  bb.push_back (db::Box (10, 220, 210, 320));
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (50, 50, 150, 150));
  bb.push_back (db::Box (10, 10, 110, 110));
  bb.push_back (db::Box (100, 10, 200, 110));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  TestCluster clt (&clusters);
  db::cluster_collector<db::Box, size_t, TestCluster> coll (clt);

  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (coll, 1, bc);
  EXPECT_EQ (cl2s (clusters), "(0,1),(2,3,4,5)");
}

TEST(10a)
{
  db::box_scanner<db::Box, size_t> bs;
  std::set<std::set<size_t> > clusters;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 210, 200, 310));
  bb.push_back (db::Box (10, 220, 210, 320));
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (50, 50, 150, 150));
  bb.push_back (db::Box (10, 10, 110, 110));
  bb.push_back (db::Box (100, 10, 200, 110));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  TestCluster clt (&clusters);
  db::cluster_collector<db::Box, size_t, TestCluster> coll (clt);

  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.process (coll, 1, bc);
  EXPECT_EQ (cl2s (clusters), "(0,1),(2,3,4,5)");
}

TEST(10b)
{
  db::box_scanner<db::Box, size_t> bs;
  std::set<std::set<size_t> > clusters;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 210, 100, 310));
  bb.push_back (db::Box (110, 220, 210, 320));
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (50, 50, 150, 150));
  bb.push_back (db::Box (10, 10, 110, 110));
  bb.push_back (db::Box (100, 10, 200, 110));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  TestCluster clt (&clusters);
  db::cluster_collector<db::Box, size_t, TestCluster> coll (clt);

  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.process (coll, 10, bc);
  EXPECT_EQ (cl2s (clusters), "(0),(1),(2,3,4,5)");

  clusters.clear ();
  bs.process (coll, 11, bc);
  EXPECT_EQ (cl2s (clusters), "(0,1),(2,3,4,5)");

  clusters.clear ();
  bs.process (coll, 60, bc);
  EXPECT_EQ (cl2s (clusters), "(0,1),(2,3,4,5)");
 
  clusters.clear ();
  bs.process (coll, 61, bc);
  EXPECT_EQ (cl2s (clusters), "(0,1,2,3,4,5)");
}

TEST(10c)
{
  db::box_scanner<db::Box, size_t> bs;
  std::set<std::set<size_t> > clusters;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 210, 200, 310));
  bb.push_back (db::Box (0, 0, 100, 100));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  TestCluster clt (&clusters);
  db::cluster_collector<db::Box, size_t, TestCluster> coll (clt);

  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (coll, 1, bc);
  EXPECT_EQ (cl2s (clusters), "(0),(1)");
}

TEST(10d)
{
  db::box_scanner<db::Box, size_t> bs;
  std::set<std::set<size_t> > clusters;

  std::vector<db::Box> bb;
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  TestCluster clt (&clusters);
  db::cluster_collector<db::Box, size_t, TestCluster> coll (clt);

  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (coll, 1, bc);
  EXPECT_EQ (cl2s (clusters), "");
}

TEST(10e)
{
  db::box_scanner<db::Box, size_t> bs;
  std::set<std::set<size_t> > clusters;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 210, 200, 310));
  bb.push_back (db::Box (0, 0, 100, 100));
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  TestCluster clt (&clusters);
  db::cluster_collector<db::Box, size_t, TestCluster> coll (clt);

  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc;
  bs.set_scanner_threshold (0);
  bs.process (coll, 111, bc);
  EXPECT_EQ (cl2s (clusters), "(0,1)");
}

void run_test11 (tl::TestBase *_this, size_t n, double ff, db::Coord spread, bool touch = true)
{
  std::vector<db::Box> bb;
  for (size_t i = 0; i < n; ++i) {
    db::Coord x = rand () % spread;
    db::Coord y = rand () % spread;
    bb.push_back (db::Box (x, y, x + 100, y + 100));
  }

  db::box_scanner<db::Box, size_t> bs;
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert (&*b, b - bb.begin ());
  }

  std::set<std::set<size_t> > clusters;

  bs.set_fill_factor (ff);
  db::box_convert<db::Box> bc;
  {
    tl::SelfTimer timer ("box-scanner");
    bs.set_scanner_threshold (0);
    TestCluster clt (&clusters);
    db::cluster_collector<db::Box, size_t, TestCluster> coll (clt);
    bs.process (coll, touch ? 1 : 0, bc);
  }

  std::set<std::set<size_t > > bf_clusters;
  {
    tl::SelfTimer timer ("brute-force");
    std::set<size_t> seen;
    for (size_t i = 0; i < bb.size (); ++i) {
      if (seen.find (i) != seen.end ()) {
        continue;
      }
      seen.insert (i);
      std::set<size_t> cl;
      cl.insert (i);
      bool any = true;
      while (any) {
        any = false;
        for (size_t j = 0; j < bb.size (); ++j) {
          if (seen.find (j) != seen.end ()) {
            continue;
          }
          for (std::set<size_t>::const_iterator k = cl.begin (); k != cl.end (); ++k) {
            if ((touch && bb[*k].touches (bb[j])) || (!touch && bb[*k].overlaps (bb[j]))) {
              cl.insert (j);
              seen.insert (j);
              any = true;
              break;
            }
          }
        }
      }
      bf_clusters.insert (cl);
    }
  }

  if (clusters != bf_clusters) {
    tl::info << "Clusters in 'brute force' but not in 'box-scanner':";
    for (std::set<std::set<size_t > >::const_iterator i = bf_clusters.begin (); i != bf_clusters.end (); ++i) {
      if (clusters.find (*i) == clusters.end ()) {
        tl::info << "   " << c2s (*i);
      }
    }
    tl::info << "Clusters in 'box-scanner' but not in 'brute force':";
    for (std::set<std::set<size_t > >::const_iterator i = clusters.begin (); i != clusters.end (); ++i) {
      if (bf_clusters.find (*i) == bf_clusters.end ()) {
        tl::info << "   " << c2s (*i);
      }
    }
  }
  EXPECT_EQ (clusters == bf_clusters, true);
}

TEST(11)
{
  run_test11(_this, 1000, 0.0, 1000);
  run_test11(_this, 1000, 2, 1000);
  run_test11(_this, 1000, 2, 1000, false);
  run_test11(_this, 1000, 2, 500);
  run_test11(_this, 1000, 2, 100);
  // brute-force is taking too long: (scanner vs brute-force: 0.07 vs 28s!)
  // run_test11(_this, 10000, 2, 10000);
}


#include "tlStream.h"
#include "dbReader.h"
#include "dbWriter.h"

struct BooleanAndOp
  : public db::box_scanner_receiver<db::Polygon, size_t>
{
  BooleanAndOp (db::EdgeProcessor *ep, db::Shapes *out)
    : mp_ep (ep), mp_out (out)
  {
  }

  void add (const db::Polygon *o1, size_t p1, const db::Polygon *o2, size_t p2) 
  {
    if (p1 == p2) {
      return;
    }

    if (o1->holes () == 0 && o1->hull ().size () == 4 && o2->holes () == 0 && o2->hull ().size () == 4 &&
        o1->area () == o1->box ().area () && o2->area () == o2->box ().area ()) {
      db::Box b = o1->box () & o2->box ();
      if (! b.empty () && b.width () > 0 && b.height () > 0) {
        mp_out->insert (b);
      }
    } else {

      mp_ep->clear ();
      mp_ep->insert (*o1, p1);
      mp_ep->insert (*o2, p2);

      db::ShapeGenerator sg (*mp_out);
      db::PolygonGenerator pg (sg, false);
      db::BooleanOp op (db::BooleanOp::And);
      mp_ep->process (pg, op);

    }
  }

public:
  db::EdgeProcessor *mp_ep;
  db::Shapes *mp_out;
};

struct BooleanAndCluster
  : public db::cluster<db::Polygon, size_t>
{
  typedef db::cluster<db::Polygon, size_t> base_class;

  BooleanAndCluster (db::EdgeProcessor *ep, db::Shapes *out)
    : mp_ep (ep), mp_out (out)
  {
  }

  void add (const db::Polygon *polygon, size_t p)
  {
    m_props.push_back (p);
    base_class::add (polygon, p);
  }

  void join (const BooleanAndCluster &other) 
  {
    m_props.insert (m_props.end (), other.begin_props (), other.end_props ());
    base_class::join (other);
  }

  void finish ()
  {
    if (end () - begin () <= 1) {
      return;
    }

    mp_ep->clear ();

    std::vector<size_t>::const_iterator p = begin_props ();
    for (iterator o = begin (); o != end (); ++o, ++p) {
      tl_assert (p != end_props ());
      mp_ep->insert (*o->first, 2 * (p - begin_props ()) + *p);
    }

    db::ShapeGenerator sg (*mp_out);
    db::PolygonGenerator pg (sg, false);
    db::BooleanOp op (db::BooleanOp::And);
    mp_ep->process (pg, op);
  }

  std::vector<size_t>::const_iterator begin_props () const
  {
    return m_props.begin ();
  }

  std::vector<size_t>::const_iterator end_props () const
  {
    return m_props.end ();
  }

public:
  db::EdgeProcessor *mp_ep;
  db::Shapes *mp_out;
  std::vector<size_t> m_props;
};

struct BooleanAndInteractionClusterCollector
  : public db::cluster_collector<db::Polygon, size_t, BooleanAndCluster> 
{
  typedef db::cluster_collector<db::Polygon, size_t, BooleanAndCluster> base_class;

  BooleanAndInteractionClusterCollector (const BooleanAndCluster &cl)
    : base_class (cl, false /*don't report single*/)
  {
    //  .. nothing yet ..
  }

  void add (const db::Polygon *o1, size_t p1, const db::Polygon *o2, size_t p2)
  {
    if (p1 != p2) {
      base_class::add (o1, p1, o2, p2);
    }
  }
};

TEST(100)
{
  std::string fn (tl::testdata_private ());
  fn += "/other/";
  fn += "bs100.oas.gz";

  db::Layout layout;
  tl::InputStream in (fn);
  db::Reader reader (in);
  reader.read (layout, db::LoadLayoutOptions ());

  unsigned int l3 = std::numeric_limits <unsigned int>::max ();
  unsigned int l6 = std::numeric_limits <unsigned int>::max ();
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if ((*l).second->layer == 3 && (*l).second->datatype == 0) {
      l3 = (*l).first;
    } else if ((*l).second->layer == 6 && (*l).second->datatype == 0) {
      l6 = (*l).first;
    }
  }
  tl_assert (l3 != std::numeric_limits <unsigned int>::max ());
  tl_assert (l6 != std::numeric_limits <unsigned int>::max ());

  db::cell_index_type top = *layout.begin_top_down ();
  tl_assert (layout.is_valid_cell_index (top));
  db::Cell &top_cell = layout.cell (top);

  layout.update ();

  unsigned int lclass = layout.insert_layer (db::LayerProperties (100, 0));
  unsigned int llocal = layout.insert_layer (db::LayerProperties (101, 0));
  unsigned int lcluster = layout.insert_layer (db::LayerProperties (102, 0));

  //  classical implementation
  {
    tl::SelfTimer timer ("Classical boolean");

    db::ShapeProcessor sp;
    sp.boolean (layout, top_cell, l3, layout, top_cell, l6, top_cell.shapes (lclass), db::BooleanOp::And, true, false);
  }

  layout.update ();

  //  alternative implementation - local
  {
    db::box_scanner<db::Polygon, size_t> bs;
    std::vector<db::Polygon> polygons [2];
    unsigned int layers [2] = { l3, l6 };

    {
      tl::SelfTimer timer ("Box-scanner implementation - prep");
      for (size_t i = 0; i < 2; ++i) {
        db::RecursiveShapeIterator si (layout, top_cell, layers [i]);
        si.shape_flags (db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes);
        while (! si.at_end ()) {
          polygons [i].push_back (db::Polygon ());
          si.shape ().polygon (polygons [i].back ());
          polygons [i].back ().transform (si.trans ());
          ++si;
        }
        for (std::vector<db::Polygon>::const_iterator p = polygons [i].begin (); p != polygons [i].end (); ++p) {
          bs.insert (&*p, i);
        }
      }
    }

    db::EdgeProcessor ep;
    db::box_convert<db::Polygon> bc;

    {
      tl::SelfTimer timer ("Box-scanner implementation - local");
      BooleanAndOp aop (&ep, &top_cell.shapes (llocal));
      bs.process (aop, 1, bc);
    }

    {
      tl::SelfTimer timer ("Box-scanner implementation - clustering");
      BooleanAndCluster clt (&ep, &top_cell.shapes (lcluster));
      BooleanAndInteractionClusterCollector coll (clt);
      bs.process (coll, 1, bc);
    }

    unsigned int ltmp1 = layout.insert_layer (db::LayerProperties ());
    unsigned int ltmp2 = layout.insert_layer (db::LayerProperties ());

    {
      db::ShapeProcessor sp;
      sp.boolean (layout, top_cell, lclass, layout, top_cell, llocal, top_cell.shapes (ltmp1), db::BooleanOp::Xor, true, false);
      sp.boolean (layout, top_cell, lclass, layout, top_cell, lcluster, top_cell.shapes (ltmp2), db::BooleanOp::Xor, true, false);
    }
    
    if (top_cell.shapes (ltmp1).size () != 0 || top_cell.shapes (ltmp2).size () != 0) {

      const char *file_out = "BoxScanner_100_out.gds";
      if (file_out) {
        tl::OutputStream fo (file_out); 
        db::SaveLayoutOptions opt;
        db::Writer writer (opt);
        writer.write (layout, fo);
        tl::info << file_out << " written.";
      }

    }

    EXPECT_EQ (top_cell.shapes (ltmp1).size (), size_t (0));
    EXPECT_EQ (top_cell.shapes (ltmp2).size (), size_t (0));

  }

}


TEST(two_1)
{
  db::box_scanner2<db::Box, size_t, db::SimplePolygon, int> bs;

  std::vector<db::Box> bb;
  std::vector<db::SimplePolygon> bb2;
  bb.push_back (db::Box (0, 210, 200, 310));
  bb.push_back (db::Box (10, 220, 210, 320));
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (50, 50, 150, 150));
  bb.push_back (db::Box (10, 10, 110, 110));
  bb.push_back (db::Box (100, 10, 200, 110));

  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bb2.push_back (db::SimplePolygon (*b));
  }

  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert1 (&*b, b - bb.begin ());
  }
  for (std::vector<db::SimplePolygon>::const_iterator b = bb2.begin (); b != bb2.end (); ++b) {
    bs.insert2 (&*b, int (b - bb2.begin ()) + 10);
  }

  BoxScannerTestRecorderTwo tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc1;
  db::box_convert<db::SimplePolygon> bc2;
  bs.set_scanner_threshold (0);
  bs.set_scanner_threshold1 (0);
  bs.process (tr, 1, bc1, bc2);
  EXPECT_EQ (tr.str, "[i](2-12)(2-14)(4-12)(4-14)(2-15)(4-15)(5-12)(5-14)(5-15)(2-13)(4-13)(3-12)(3-14)(3-13)(3-15)(5-13)(0-10)<2><5><4><3><12><15><14><13>(0-11)(1-10)(1-11)<0><1><10><11>[f]");
}

TEST(two_1a)
{
  db::box_scanner2<db::Box, size_t, db::SimplePolygon, int> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box (0, 210, 200, 310));
  //bb.push_back (db::Box (10, 220, 210, 320));
  //bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (50, 50, 150, 150));
  bb.push_back (db::Box (10, 10, 110, 110));
  //bb.push_back (db::Box (100, 10, 200, 110));

  std::vector<db::SimplePolygon> bb2;
  //bb2.push_back (db::SimplePolygon (db::Box (0, 210, 200, 310)));
  bb2.push_back (db::SimplePolygon (db::Box (10, 220, 210, 320)));
  bb2.push_back (db::SimplePolygon (db::Box (0, 0, 100, 100)));
  //bb2.push_back (db::SimplePolygon (db::Box (50, 50, 150, 150)));
  //bb2.push_back (db::SimplePolygon (db::Box (10, 10, 110, 110)));
  bb2.push_back (db::SimplePolygon (db::Box (100, 10, 200, 110)));

  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert1 (&*b, b - bb.begin ());
  }
  for (std::vector<db::SimplePolygon>::const_iterator b = bb2.begin (); b != bb2.end (); ++b) {
    bs.insert2 (&*b, int (b - bb2.begin ()) + 10);
  }

  BoxScannerTestRecorderTwo tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc1;
  db::box_convert<db::SimplePolygon> bc2;
  bs.set_scanner_threshold (0);
  bs.set_scanner_threshold1 (0);
  bs.process (tr, 1, bc1, bc2);
  EXPECT_EQ (tr.str, "[i](2-11)(2-12)(1-11)(1-12)<1><2><11><12>(0-10)<0><10>[f]");
}

TEST(two_1b)
{
  db::box_scanner2<db::Box, size_t, db::SimplePolygon, int> bs;

  std::vector<db::Box> bb;
  //bb.push_back (db::Box (0, 210, 200, 310));
  bb.push_back (db::Box (10, 220, 210, 320));
  bb.push_back (db::Box (0, 0, 100, 100));
  //bb.push_back (db::Box (50, 50, 150, 150));
  //bb.push_back (db::Box (10, 10, 110, 110));
  bb.push_back (db::Box (100, 10, 200, 110));

  std::vector<db::SimplePolygon> bb2;
  bb2.push_back (db::SimplePolygon (db::Box (0, 210, 200, 310)));
  //bb2.push_back (db::SimplePolygon (db::Box (10, 220, 210, 320)));
  //bb2.push_back (db::SimplePolygon (db::Box (0, 0, 100, 100)));
  bb2.push_back (db::SimplePolygon (db::Box (50, 50, 150, 150)));
  bb2.push_back (db::SimplePolygon (db::Box (10, 10, 110, 110)));
  //bb2.push_back (db::SimplePolygon (db::Box (100, 10, 200, 110)));

  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert1 (&*b, b - bb.begin ());
  }
  for (std::vector<db::SimplePolygon>::const_iterator b = bb2.begin (); b != bb2.end (); ++b) {
    bs.insert2 (&*b, int (b - bb2.begin ()) + 10);
  }

  BoxScannerTestRecorderTwo tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc1;
  db::box_convert<db::SimplePolygon> bc2;
  bs.set_scanner_threshold (0);
  bs.set_scanner_threshold1 (0);
  EXPECT_EQ (bs.process (tr, 1, bc1, bc2), true);
  EXPECT_EQ (tr.str, "[i](1-12)(2-12)(1-11)(2-11)<1><2><11><12>(0-10)<0><10>[f]");


  BoxScannerTestRecorderTwoStopping trstop;
  EXPECT_EQ (bs.process (trstop, 1, bc1, bc2), false);
  EXPECT_EQ (trstop.str, "[i](1-12)[f-]");
}

TEST(two_1c)
{
  //  some empty elements
  db::box_scanner2<db::Box, size_t, db::SimplePolygon, int> bs;

  std::vector<db::Box> bb;
  bb.push_back (db::Box ());
  bb.push_back (db::Box (0, 0, 100, 100));
  bb.push_back (db::Box (100, 10, 200, 110));

  std::vector<db::SimplePolygon> bb2;
  bb2.push_back (db::SimplePolygon (db::Box ()));
  bb2.push_back (db::SimplePolygon (db::Box (50, 50, 150, 150)));
  bb2.push_back (db::SimplePolygon (db::Box (10, 10, 110, 110)));

  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert1 (&*b, b - bb.begin ());
  }
  for (std::vector<db::SimplePolygon>::const_iterator b = bb2.begin (); b != bb2.end (); ++b) {
    bs.insert2 (&*b, int (b - bb2.begin ()) + 10);
  }

  BoxScannerTestRecorderTwo tr;
  bs.set_fill_factor (0.0);
  db::box_convert<db::Box> bc1;
  db::box_convert<db::SimplePolygon> bc2;
  bs.set_scanner_threshold (0);
  bs.set_scanner_threshold1 (0);
  EXPECT_EQ (bs.process (tr, 1, bc1, bc2), true);
  EXPECT_EQ (tr.str, "[i]<0><10>(1-12)(2-12)(1-11)(2-11)<1><2><12><11>[f]");
}

void run_test2_two (tl::TestBase *_this, size_t n1, size_t n2, double ff, db::Coord spread, bool touch = true, bool no_shortcut = true)
{
  std::vector<db::Box> bb;
  for (size_t i = 0; i < n1; ++i) {
    db::Coord x = rand () % spread;
    db::Coord y = rand () % spread;
    bb.push_back (db::Box (x, y, x + 100, y + 100));
    // std::cout << "Box 1" << bb.back ().to_string () << std::endl;
  }

  std::vector<db::SimplePolygon> bb2;
  for (size_t i = 0; i < n2; ++i) {
    db::Coord x = rand () % spread;
    db::Coord y = rand () % spread;
    bb2.push_back (db::SimplePolygon (db::Box (x, y, x + 100, y + 100)));
    // std::cout << "Polygon 2" << bb2.back ().to_string () << std::endl;
  }

  db::box_scanner2<db::Box, size_t, db::SimplePolygon, int> bs;
  for (std::vector<db::Box>::const_iterator b = bb.begin (); b != bb.end (); ++b) {
    bs.insert1 (&*b, b - bb.begin ());
  }
  for (std::vector<db::SimplePolygon>::const_iterator b2 = bb2.begin (); b2 != bb2.end (); ++b2) {
    bs.insert2 (&*b2, int (b2 - bb2.begin ()));
  }

  BoxScannerTestRecorder2Two tr;
  bs.set_fill_factor (ff);
  db::box_convert<db::Box> bc1;
  db::box_convert<db::SimplePolygon> bc2;
  {
    tl::SelfTimer timer ("box-scanner");
    if (no_shortcut) {
      bs.set_scanner_threshold (0);
      bs.set_scanner_threshold1 (0);
    }
    bs.process (tr, touch ? 1 : 0, bc1, bc2);
  }

  std::set<std::pair<size_t, int> > interactions;
  {
    tl::SelfTimer timer ("brute-force");
    for (size_t i = 0; i < bb.size (); ++i) {
      for (size_t j = 0; j < bb2.size (); ++j) {
        if ((touch && bb[i].touches (bb2[j].box ())) || (!touch && bb[i].overlaps (bb2[j].box ()))) {
          interactions.insert (std::make_pair (i, int (j)));
        }
      }
    }
  }

  if (interactions != tr.interactions) {
    tl::info << "Interactions 1-2 in 'brute force' but not in 'box-scanner':";
    for (std::set<std::pair<size_t, int> >::const_iterator i = interactions.begin (); i != interactions.end (); ++i) {
      if (tr.interactions.find (*i) == tr.interactions.end ()) {
        tl::info << "   " << i->first << "-" << i->second;
      }
    }
    tl::info << "Interactions 1-2 in 'box-scanner' but not in 'brute force':";
    for (std::set<std::pair<size_t, int> >::const_iterator i = tr.interactions.begin (); i != tr.interactions.end (); ++i) {
      if (interactions.find (*i) == interactions.end ()) {
        tl::info << "   " << i->first << "-" << i->second;
      }
    }
  }
  EXPECT_EQ (interactions == tr.interactions, true);

}

TEST(two_2a)
{
  run_test2_two(_this, 10, 10, 0.0, 1000);
  run_test2_two(_this, 10, 10, 0.0, 1000, true, false /*sub-threshold*/);
}

TEST(two_2b)
{
  run_test2_two(_this, 10, 10, 0.0, 100);
  run_test2_two(_this, 10, 10, 0.0, 100, true, false /*sub-threshold*/);
}

TEST(two_2c)
{
  run_test2_two(_this, 10, 10, 0.0, 10);
  run_test2_two(_this, 10, 10, 0.0, 10, true, false /*sub-threshold*/);
}

TEST(two_2d)
{
  run_test2_two(_this, 1000, 1000, 0.0, 1000);
}

TEST(two_2e)
{
  run_test2_two(_this, 1000, 1000, 2, 1000);
}

TEST(two_2f)
{
  run_test2_two(_this, 1000, 1000, 2, 1000, false);
}

TEST(two_2g)
{
  run_test2_two(_this, 1000, 1000, 2, 500);
}

TEST(two_2h)
{
  run_test2_two(_this, 1000, 1000, 2, 100);
}

TEST(two_2i)
{
  run_test2_two(_this, 10000, 1000, 2, 10000);
}

TEST(two_2j)
{
  run_test2_two(_this, 3, 1000, 0.0, 1000);
  run_test2_two(_this, 3, 1000, 0.0, 1000, true, false /*sub-threshold*/);
}

TEST(two_2k)
{
  run_test2_two(_this, 1000, 3, 0.0, 1000);
  run_test2_two(_this, 1000, 3, 0.0, 1000, true, false /*sub-threshold*/);
}
