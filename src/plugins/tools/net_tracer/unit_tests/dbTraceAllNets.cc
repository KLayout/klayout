
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
#include "dbLayoutToNetlist.h"
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

void run_test (tl::TestBase *_this, const std::string &file, const db::NetTracerConnectivity &tc, const std::string &file_au)
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

  db::RecursiveShapeIterator si (layout_org, cell, std::vector<unsigned int> ());
  db::LayoutToNetlist l2ndb (si);

  db::NetTracerData tracer_data = tc.get_tracer_data (layout_org);
  tracer_data.configure_l2n (l2ndb);

  l2ndb.extract_netlist ();

  db::Layout layout_nets;
  db::Cell &top_cell = layout_nets.cell (layout_nets.add_cell ("NETS"));
  db::CellMapping cm = l2ndb.cell_mapping_into (layout_nets, top_cell);

  l2ndb.build_all_nets (cm, layout_nets, l2ndb.create_layermap (layout_nets, 1000), "NET_", db::NPM_NoProperties, tl::Variant (), db::BNH_SubcircuitCells, "CIRCUIT_", 0);

  std::string fn (tl::testdata ());
  fn += "/net_tracer/";
  fn += file_au;

  CHECKPOINT ();
  db::compare_layouts (_this, layout_nets, fn, db::NormalizationMode (db::WriteOAS | db::AsPolygons));
}

TEST(1) 
{
  std::string file = "t1.oas.gz";
  std::string file_au = "t1_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0", "3/0"));

  run_test (_this, file, tc, file_au);
}

TEST(1c) 
{
  std::string file = "t1.oas.gz";
  std::string file_au = "t1_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add_symbol (symbol ("a", "1/0"));
  tc.add_symbol (symbol ("c", "cc"));
  tc.add_symbol (symbol ("cc", "3/0"));
  tc.add (connection ("a", "2/0", "cc"));

  run_test (_this, file, tc, file_au);
}

TEST(1d) 
{
  std::string file = "t1.oas.gz";
  std::string file_au = "t1d_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "10/0", "11/0"));

  //  some layers are non-existing
  run_test (_this, file, tc, file_au);
}

TEST(4) 
{
  std::string file = "t4.oas.gz";
  std::string file_au = "t4_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0", "3/0"));

  run_test (_this, file, tc, file_au);
}

TEST(4b) 
{
  std::string file = "t4.oas.gz";
  std::string file_au = "t4b_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "3/0"));

  run_test (_this, file, tc, file_au);
}

TEST(5) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0*10/0", "2/0", "3/0"));

  run_test (_this, file, tc, file_au);
}

TEST(5b) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5b_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0*10/0", "3/0"));

  run_test (_this, file, tc, file_au);
}

TEST(5c) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5c_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0", "2/0-11/0", "3/0"));

  run_test (_this, file, tc, file_au);
}

TEST(5d) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5d_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1/0-12/0", "2/0", "3/0-12/0"));

  run_test (_this, file, tc, file_au);
}

TEST(5f) 
{
  std::string file = "t5.oas.gz";
  std::string file_au = "t5f_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add_symbol (symbol ("x", "3-14"));
  tc.add (connection ("10-13", "x"));
  tc.add (connection ("x", "2", "1+13"));

  run_test (_this, file, tc, file_au);
}

TEST(6) 
{
  std::string file = "t6.oas.gz";
  std::string file_au = "t6_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("1-10", "2", "3"));
  tc.add (connection ("3", "4", "5"));

  run_test (_this, file, tc, file_au);
}

TEST(7) 
{
  std::string file = "t7.oas.gz";
  std::string file_au = "t7_all_nets.oas.gz";

  db::NetTracerConnectivity tc;
  tc.add (connection ("15", "14", "2-7"));
  tc.add (connection ("15", "14", "7"));

  run_test (_this, file, tc, file_au);
}

