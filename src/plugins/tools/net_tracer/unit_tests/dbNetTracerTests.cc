
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "dbNetTracerIO.h"
#include "dbNetTracer.h"
#include "dbRecursiveShapeIterator.h"
#include "dbLayoutDiff.h"
#include "dbTestSupport.h"
#include "dbWriter.h"
#include "dbReader.h"

static db::NetTracerConnectionInfo connection (const std::string &a, const std::string &v, const std::string &b)
{
  return db::NetTracerConnectionInfo (db::NetTracerLayerExpressionInfo::compile (a),
                                      db::NetTracerLayerExpressionInfo::compile (v),
                                      db::NetTracerLayerExpressionInfo::compile (b));
}

static db::NetTracerConnectionInfo connection (const std::string &a, const std::string &b)
{
  return db::NetTracerConnectionInfo (db::NetTracerLayerExpressionInfo::compile (a),
                                      db::NetTracerLayerExpressionInfo::compile (b));
}

static db::NetTracerSymbolInfo symbol (const std::string &s, const std::string &e)
{
  return db::NetTracerSymbolInfo (s, e);
}

static int layer_for (const db::Layout &layout, const db::LayerProperties &lp)
{
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if ((*l).second->log_equal (lp)) {
      return int ((*l).first);
    }
  }
  return -1;
}

#if 0
//  not used yet:
static db::NetTracerShape find_shape (const db::Layout &layout, const db::Cell &cell, int l, const db::Point &pt)
{
  if (l < 0 || ! layout.is_valid_layer ((unsigned int) l)) {
    return db::NetTracerShape ();
  } 

  db::RecursiveShapeIterator s (layout, cell, (unsigned int) l, db::Box (pt, pt));
  if (! s.at_end ()) {
    return db::NetTracerShape (s.itrans (), s.shape (), (unsigned int) l, s.cell_index ());
  } else {
    return db::NetTracerShape ();
  }
}
#endif

static db::NetTracerNet trace (db::NetTracer &tracer, const db::Layout &layout, const db::Cell &cell, const db::NetTracerConnectivity &tc, unsigned int l_start, const db::Point &p_start)
{
  db::NetTracerData tracer_data = tc.get_tracer_data (layout);
  tracer.trace (layout, cell, p_start, l_start, tracer_data);
  return db::NetTracerNet (tracer, db::ICplxTrans (), layout, cell.cell_index (), std::string (), std::string (), tracer_data);
}

static db::NetTracerNet trace (db::NetTracer &tracer, const db::Layout &layout, const db::Cell &cell, const db::NetTracerConnectivity &tc, unsigned int l_start, const db::Point &p_start, unsigned int l_stop, const db::Point &p_stop)
{
  db::NetTracerData tracer_data = tc.get_tracer_data (layout);
  tracer.trace (layout, cell, p_start, l_start, p_stop, l_stop, tracer_data);
  return db::NetTracerNet (tracer, db::ICplxTrans (), layout, cell.cell_index (), std::string (), std::string (), tracer_data);
}

void run_test (tl::TestBase *_this, const std::string &file, const db::NetTracerConnectivity &tc, const db::LayerProperties &lp_start, const db::Point &p_start, const std::string &file_au, const char *net_name = 0, size_t depth = 0)
{
  db::Manager m (false);

  db::Layout layout_org (&m);
  {
    std::string fn (tl::testdata ());
    fn += "/net_tracer/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);
  }

  const db::Cell &cell = layout_org.cell (*layout_org.begin_top_down ());

  db::NetTracer tracer;
  tracer.set_trace_depth (depth);
  db::NetTracerNet net = trace (tracer, layout_org, cell, tc, layer_for (layout_org, lp_start), p_start);

  if (net_name) {
    EXPECT_EQ (net.name (), std::string (net_name));
  }

  EXPECT_EQ (net.incomplete (), depth != 0);
  if (depth > 0) {
    EXPECT_EQ (net.size () <= depth, true);
  }

  db::Layout layout_net;
  net.export_net (layout_net, layout_net.cell (layout_net.add_cell ("NET")));

  std::string fn (tl::testdata ());
  fn += "/net_tracer/";
  fn += file_au;

  CHECKPOINT ();
  db::compare_layouts (_this, layout_net, fn, db::WriteOAS);
}

void run_test2 (tl::TestBase *_this, const std::string &file, const db::NetTracerConnectivity &tc, const db::LayerProperties &lp_start, const db::Point &p_start, const db::LayerProperties &lp_stop, const db::Point &p_stop, const std::string &file_au, const char *net_name = 0)
{
  db::Manager m (false);

  db::Layout layout_org (&m);
  {
    std::string fn (tl::testdata ());
    fn += "/net_tracer/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);
  }

  const db::Cell &cell = layout_org.cell (*layout_org.begin_top_down ());

  db::NetTracer tracer;
  db::NetTracerNet net = trace (tracer, layout_org, cell, tc, layer_for (layout_org, lp_start), p_start, layer_for (layout_org, lp_stop), p_stop);

  if (net_name) {
    EXPECT_EQ (net.name (), std::string (net_name));
  }

  db::Layout layout_net;
  net.export_net (layout_net, layout_net.cell (layout_net.add_cell ("NET")));

  std::string fn (tl::testdata ());
  fn += "/net_tracer/";
  fn += file_au;

  CHECKPOINT ();
  db::compare_layouts (_this, layout_net, fn, db::WriteOAS);
}

TEST(1) 
{
  std::string file = "t1.oas.gz";
  std::string file_au = "t1_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0", "3/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(1b) 
{
  std::string file = "t1.oas.gz";
  std::string file_au = "t1b_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0", "3/0"));

  //  point is off net ...
  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 15000), file_au);
}

TEST(1c) 
{
  std::string file = "t1.oas.gz";
  std::string file_au = "t1_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add_symbol (symbol ("a", "1/0"));
  tc.add_symbol (symbol ("c", "cc"));
  tc.add_symbol (symbol ("cc", "3/0"));
  tc.add (connection ("a", "2/0", "cc"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(1d) 
{
  std::string file = "t1.oas.gz";
  std::string file_au = "t1d_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "10/0", "11/0"));

  //  some layers are non-existing
  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au);
}

TEST(2) 
{
  std::string file = "t2.oas.gz";
  std::string file_au = "t2_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0", "3/0"));

  run_test2 (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), db::LayerProperties (3, 0), db::Point (4000, -20000), file_au, "THE_NAME");
}

TEST(3) 
{
  std::string file = "t3.oas.gz";
  std::string file_au = "t3_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0", "3/0"));

  std::string msg;
  try {
    run_test2 (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), db::LayerProperties (3, 0), db::Point (4000, -20000), file_au);
  } catch (tl::Exception &ex) {
    msg = ex.msg ();
  }
  EXPECT_EQ (msg, "Nets are not connected");
}

TEST(4) 
{
  std::string file = "t4.oas.gz";
  std::string file_au = "t4_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0", "3/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "");
}

TEST(4b) 
{
  std::string file = "t4.oas.gz";
  std::string file_au = "t4b_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "3/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(5) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0*10/0", "2/0", "3/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(5b) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5b_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0*10/0", "3/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(5c) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5c_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0-11/0", "3/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "");
}

TEST(5d) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5d_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0-12/0", "2/0", "3/0-12/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(5e) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5e_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0-12/0", "2/0", "3/0-12/0"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(5f) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5f_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add_symbol (symbol ("x", "3-14"));
  tc.add (connection ("10-13", "x"));
  tc.add (connection ("x", "2", "1+13"));

  run_test (_this, file, tc, db::LayerProperties (10, 0), db::Point (7000, 1500), file_au, "THE_NAME");
}

TEST(6) 
{
  std::string file = "t6.oas.gz";
  std::string file_au = "t6_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1-10", "2", "3"));
  tc.add (connection ("3", "4", "5"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (-2250, -900), file_au, "A");
}

TEST(6b)
{
  std::string file = "t6.oas.gz";
  std::string file_au = "t6b_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1-10", "2", "3"));
  tc.add (connection ("3", "4", "5"));

  run_test (_this, file, tc, db::LayerProperties (1, 0), db::Point (-2250, -900), file_au, "IN_B", 10);
}

TEST(7)
{
  std::string file = "t7.oas.gz";
  std::string file_au = "t7_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("15", "14", "2-7"));
  tc.add (connection ("15", "14", "7"));

  run_test (_this, file, tc, db::LayerProperties (15, 0), db::Point (-700, 300), file_au, "");
}

// bug #456: OASIS box arrays and net tracer
TEST(8) 
{
  std::string file = "t8.oas.gz";
  std::string file_au = "t8_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("15", "14", "7"));

  run_test (_this, file, tc, db::LayerProperties (15, 0), db::Point (4000, 10000), file_au, "");
}

TEST(9) 
{
  std::string file = "t9.oas.gz";
  std::string file_au = "t9_net.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add_symbol (symbol ("a", "8-12"));
  tc.add_symbol (symbol ("b", "a+7"));
  tc.add_symbol (symbol ("c", "15*26"));
  tc.add (connection ("b", "7"));
  tc.add (connection ("b", "c", "9"));

  run_test (_this, file, tc, db::LayerProperties (8, 0), db::Point (3000, 6800), file_au, "A");
}

