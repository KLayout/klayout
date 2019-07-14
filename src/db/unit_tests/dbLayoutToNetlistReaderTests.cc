
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbLayoutToNetlist.h"
#include "dbLayoutToNetlistReader.h"
#include "dbLayoutToNetlistWriter.h"
#include "dbStream.h"
#include "dbCommonReader.h"
#include "dbNetlistDeviceExtractorClasses.h"
#include "dbTestSupport.h"

#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

TEST(1_ReaderBasic)
{
  db::LayoutToNetlist l2n;

  std::string in_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au.txt");
  tl::InputStream is_in (in_path);

  db::LayoutToNetlistStandardReader reader (is_in);
  reader.read (&l2n);

  //  verify against the input

  std::string path = tmp_file ("tmp_l2nreader_1.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au.txt");

  tl::InputStream is (path);
  tl::InputStream is_au (au_path);

  if (is.read_all () != is_au.read_all ()) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                               tl::absolute_file_path (path),
                               tl::absolute_file_path (au_path)));
  }

  //  test build_all_nets from read l2n

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    l2n.build_all_nets (cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_Disconnected, 0, "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_1.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    std::vector<const db::Net *> nets;
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VSS"));
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VDD"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, nets);

    l2n.build_nets (&nets, cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_Disconnected, 0, "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_1b.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    std::vector<const db::Net *> nets;
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VSS"));
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VDD"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, nets);

    l2n.build_nets (&nets, cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_Flatten, 0, "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_1c.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    std::vector<const db::Net *> nets;
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VSS"));
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VDD"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, nets);

    l2n.build_nets (&nets, cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_1d.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    std::vector<const db::Net *> nets;
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VSS"));
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VDD"));
    nets.push_back (l2n.netlist ()->circuit_by_name ("INV2")->net_by_name ("IN"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, nets);

    l2n.build_nets (&nets, cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", 0);

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_1e.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    std::vector<const db::Net *> nets;
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VSS"));
    nets.push_back (l2n.netlist ()->circuit_by_name ("RINGO")->net_by_name ("VDD"));
    nets.push_back (l2n.netlist ()->circuit_by_name ("INV2")->net_by_name ("IN"));

    db::CellMapping cm = l2n.const_cell_mapping_into (ly2, top2);

    l2n.build_nets (&nets, cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_1f.gds");

    db::compare_layouts (_this, ly2, au);
  }
}

TEST(1b_ReaderBasicShort)
{
  db::LayoutToNetlist l2n;

  std::string in_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au_s.txt");
  tl::InputStream is_in (in_path);

  db::LayoutToNetlistStandardReader reader (is_in);
  reader.read (&l2n);

  //  verify against the input

  std::string path = tmp_file ("tmp_l2nreader_2.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, true);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au_s.txt");

  tl::InputStream is (path);
  tl::InputStream is_au (au_path);

  if (is.read_all () != is_au.read_all ()) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                               tl::absolute_file_path (path),
                               tl::absolute_file_path (au_path)));
  }

}

TEST(2_ReaderWithGlobalNets)
{
  db::LayoutToNetlist l2n;

  std::string in_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au_2.txt");
  tl::InputStream is_in (in_path);

  db::LayoutToNetlistStandardReader reader (is_in);
  reader.read (&l2n);

  //  verify against the input

  std::string path = tmp_file ("tmp_l2nreader_2.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au_2.txt");

  tl::InputStream is (path);
  tl::InputStream is_au (au_path);

  if (is.read_all () != is_au.read_all ()) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                               tl::absolute_file_path (path),
                               tl::absolute_file_path (au_path)));
  }

  //  test build_all_nets from read l2n

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (12, 0))] = l2n.layer_by_name ("rbulk");
    lmap [ly2.insert_layer (db::LayerProperties (13, 0))] = l2n.layer_by_name ("ptie");
    lmap [ly2.insert_layer (db::LayerProperties (14, 0))] = l2n.layer_by_name ("ntie");
    lmap [ly2.insert_layer (db::LayerProperties (1, 0)) ] = l2n.layer_by_name ("nwell");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    l2n.build_all_nets (cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_2.gds");

    db::compare_layouts (_this, ly2, au);
  }
}

TEST(3_ReaderAbsoluteCoordinates)
{
  db::LayoutToNetlist l2n;

  std::string in_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au_2_abs.txt");
  tl::InputStream is_in (in_path);

  db::LayoutToNetlistStandardReader reader (is_in);
  reader.read (&l2n);

  //  verify against the input

  std::string path = tmp_file ("tmp_l2nreader_2.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_writer_au_2.txt");

  tl::InputStream is (path);
  tl::InputStream is_au (au_path);

  if (is.read_all () != is_au.read_all ()) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                               tl::absolute_file_path (path),
                               tl::absolute_file_path (au_path)));
  }

  //  test build_all_nets from read l2n

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_by_name ("psd");
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_by_name ("nsd");
    lmap [ly2.insert_layer (db::LayerProperties (12, 0))] = l2n.layer_by_name ("rbulk");
    lmap [ly2.insert_layer (db::LayerProperties (13, 0))] = l2n.layer_by_name ("ptie");
    lmap [ly2.insert_layer (db::LayerProperties (14, 0))] = l2n.layer_by_name ("ntie");
    lmap [ly2.insert_layer (db::LayerProperties (1, 0)) ] = l2n.layer_by_name ("nwell");
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_by_name ("poly");
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_by_name ("diff_cont");
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_by_name ("poly_cont");
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_by_name ("metal1");
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_by_name ("via1");
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_by_name ("metal2");

    l2n.build_all_nets (cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_2.gds");

    db::compare_layouts (_this, ly2, au);
  }
}

TEST(4_ReaderCombinedDevices)
{
  db::LayoutToNetlist l2n;

  //  build from: testdata/algo/l2n_reader_4.gds

  std::string in_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_reader_4.l2n");
  tl::InputStream is_in (in_path);

  db::LayoutToNetlistStandardReader reader (is_in);
  reader.read (&l2n);

  //  verify against the input

  std::string path = tmp_file ("tmp_l2nreader_4.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "l2n_reader_au_4.l2n");

  tl::InputStream is (path);
  tl::InputStream is_au (au_path);

  if (is.read_all () != is_au.read_all ()) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                               tl::absolute_file_path (path),
                               tl::absolute_file_path (au_path)));
  }

  //  test build_all_nets from read l2n

  {
    db::Layout ly2;
    ly2.dbu (l2n.internal_layout ()->dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap = l2n.create_layermap (ly2, 1000);

    l2n.build_all_nets (cm, ly2, lmap, "NET_", tl::Variant (), db::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_reader_au_4.gds");

    db::compare_layouts (_this, ly2, au);
  }
}

