
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

#include "dbNetlistDeviceExtractor.h"
#include "dbLoadLayoutOptions.h"
#include "dbReader.h"
#include "dbRecursiveShapeIterator.h"
#include "dbNetlistDeviceExtractorClasses.h"
#include "dbNetlistDeviceClasses.h"

#include "tlUnitTest.h"
#include "tlFileUtils.h"
#include "tlStream.h"

TEST(1_NetlistDeviceExtractorErrorBasic)
{
  db::LogEntryData error;

  EXPECT_EQ (error.message (), "");
  error.set_message ("x");
  EXPECT_EQ (error.message (), "x");
  error.set_category_name ("cat");
  EXPECT_EQ (error.category_name (), "cat");
  error.set_category_description ("cdesc");
  EXPECT_EQ (error.category_description (), "cdesc");
  error.set_cell_name ("cell");
  EXPECT_EQ (error.cell_name (), "cell");
  error.set_geometry (db::DPolygon (db::DBox (0, 1, 2, 3)));
  EXPECT_EQ (error.geometry ().to_string (), "(0,1;0,3;2,3;2,1)");

  error = db::LogEntryData (db::Error, "cell2", "msg2");
  EXPECT_EQ (error.severity () == db::Error, true);
  EXPECT_EQ (error.cell_name (), "cell2");
  EXPECT_EQ (error.message (), "msg2");
  EXPECT_EQ (error.category_name (), "");
  EXPECT_EQ (error.category_description (), "");
  EXPECT_EQ (error.geometry ().to_string (), "()");

  error.set_severity (db::Warning);
  EXPECT_EQ (error.severity () == db::Warning, true);
}

namespace {
  class DummyDeviceExtractor
    : public db::NetlistDeviceExtractor
  {
  public:
    DummyDeviceExtractor ()
      : db::NetlistDeviceExtractor (std::string ("DUMMY"))
    {
      error ("msg1");
      error ("msg2", db::DPolygon (db::DBox (0, 1, 2, 3)));
      error ("cat1", "desc1", "msg1");
      error ("cat1", "desc1", "msg3", db::DPolygon (db::DBox (10, 11, 12, 13)));
    }
  };
}

static std::string error2string (const db::LogEntryData &e)
{
  return e.cell_name() + ":" + e.category_name () + ":" + e.category_description () + ":" +
         e.geometry ().to_string () + ":" + e.message ();
}

TEST(2_NetlistDeviceExtractorErrors)
{
  DummyDeviceExtractor dummy_ex;

  EXPECT_EQ (dummy_ex.begin_log_entries () != dummy_ex.end_log_entries (), true);

  std::vector<db::LogEntryData> errors (dummy_ex.begin_log_entries (), dummy_ex.end_log_entries ());
  EXPECT_EQ (int (errors.size ()), 4);
  EXPECT_EQ (error2string (errors [0]), ":device-extract::():msg1");
  EXPECT_EQ (error2string (errors [1]), ":device-extract::(0,1;0,3;2,3;2,1):msg2");
  EXPECT_EQ (error2string (errors [2]), ":cat1:desc1:():msg1");
  EXPECT_EQ (error2string (errors [3]), ":cat1:desc1:(10,11;10,13;12,13;12,11):msg3");
}

namespace {

class MyDeviceClass
  : public db::DeviceClassMOS3Transistor
{
public:
  MyDeviceClass () : db::DeviceClassMOS3Transistor () { }
};

}

TEST(3_ClassFactoryTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos3_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("MOS3", false, new db::device_class_factory<MyDeviceClass> ());

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  //  the generated objects are of MyDeviceClassType
  EXPECT_EQ (dynamic_cast<const MyDeviceClass *> (ex.device_class ()) != 0, true);
  EXPECT_EQ (dynamic_cast<const MyDeviceClass *> (nl.device_class_by_name ("MOS3")) != 0, true);
}

TEST(10_MOS3DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos3_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("MOS3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200){TERMINAL_ID=>1}");
}

TEST(11_MOS3DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos3_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("MOS3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200){TERMINAL_ID=>1}");
}

TEST(12_MOS3DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos3_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("MOS3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600){TERMINAL_ID=>1}");
}

TEST(20_MOS4DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos4_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("MOS4");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200){TERMINAL_ID=>1}");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,600;200,600;200,-200){TERMINAL_ID=>3}");
}

TEST(21_MOS4DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos4_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("MOS4");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200){TERMINAL_ID=>1}");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200){TERMINAL_ID=>3}");
}

TEST(22_MOS4DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos4_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("MOS4");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600){TERMINAL_ID=>1}");
  EXPECT_EQ (o4.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600){TERMINAL_ID=>3}");
}

TEST(30_DMOS3DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos3_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("DMOS3", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200){TERMINAL_ID=>1}");
}

TEST(31_DMOS3DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos3_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("DMOS3", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200){TERMINAL_ID=>1}");
}

TEST(32_DMOS3DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos3_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("DMOS3", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600){TERMINAL_ID=>1}");
}

TEST(40_DMOS4DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos4_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("DMOS4", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200){TERMINAL_ID=>1}");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,600;200,600;200,-200){TERMINAL_ID=>3}");
}

TEST(41_DMOS4DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos4_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("DMOS4", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200){TERMINAL_ID=>1}");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200){TERMINAL_ID=>3}");
}

TEST(42_DMOS4DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos4_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("DMOS4", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200){TERMINAL_ID=>0}");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900){TERMINAL_ID=>2}");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600){TERMINAL_ID=>1}");
  EXPECT_EQ (o4.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600){TERMINAL_ID=>3}");
}

TEST(50_BJT3DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "bjt3_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorBJT3Transistor ex ("BJT3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["E"] = &l0;
  dl["B"] = &l1;
  dl["C"] = &l2;
  dl["tE"] = &o1;
  dl["tB"] = &o2;
  dl["tC"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device BJT3 $1 (C=(null),B=(null),E=(null)) (AE=0.81,PE=3.6,AB=5,PB=9,AC=5,PC=9,NE=1);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(700,400;700,1300;1600,1300;1600,400){TERMINAL_ID=>2}");
  EXPECT_EQ (o2.to_string (), "(0,0;0,2000;2500,2000;2500,0/700,400;1600,400;1600,1300;700,1300){TERMINAL_ID=>1}");
  EXPECT_EQ (o3.to_string (), "(0,0;0,2000;2500,2000;2500,0/700,400;1600,400;1600,1300;700,1300){TERMINAL_ID=>0}");
}

TEST(51_BJT3DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "bjt3_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorBJT3Transistor ex ("BJT3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["E"] = &l0;
  dl["B"] = &l1;
  dl["C"] = &l2;
  dl["tE"] = &o1;
  dl["tB"] = &o2;
  dl["tC"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device BJT3 $1 (C=(null),B=(null),E=(null)) (AE=0.81,PE=3.6,AB=5,PB=9,AC=5,PC=9,NE=1);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(700,400;700,1300;1600,1300;1600,400){TERMINAL_ID=>2}");
  EXPECT_EQ (o2.to_string (), "(0,0;0,2000;2500,2000;2500,0/700,400;1600,400;1600,1300;700,1300){TERMINAL_ID=>1}");
  EXPECT_EQ (o3.to_string (), "(-1000,-500;-1000,2500;3000,2500;3000,-500/0,0;2500,0;2500,2000;0,2000){TERMINAL_ID=>0}");
}

TEST(52_BJT3DeviceExtractorTestLateral)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "bjt3_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorBJT3Transistor ex ("BJT3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["E"] = &l0;
  dl["B"] = &l1;
  dl["C"] = &l2;
  dl["tE"] = &o1;
  dl["tB"] = &o2;
  dl["tC"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device BJT3 $1 (C=(null),B=(null),E=(null)) (AE=0.81,PE=3.6,AB=5,PB=9,AC=0.8,PC=4.8,NE=1);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(700,400;700,1300;1600,1300;1600,400){TERMINAL_ID=>2}");
  EXPECT_EQ (o2.to_string (), "(0,0;0,2000;2100,2000;2100,0/700,400;1600,400;1600,1300;700,1300){TERMINAL_ID=>1}");
  EXPECT_EQ (o3.to_string (), "(2100,0;2100,2000;2500,2000;2500,0){TERMINAL_ID=>0}");
}

TEST(53_BJT3DeviceExtractorTestMultEmitter)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "bjt3_4.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorBJT3Transistor ex ("BJT3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["E"] = &l0;
  dl["B"] = &l1;
  dl["C"] = &l2;
  dl["tE"] = &o1;
  dl["tB"] = &o2;
  dl["tC"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device BJT3 $1 (C=(null),B=(null),E=(null)) (AE=0.5,PE=3,AB=10,PB=14,AC=10,PC=14,NE=1);\n"
    "  device BJT3 $2 (C=(null),B=(null),E=(null)) (AE=0.5,PE=3,AB=10,PB=14,AC=10,PC=14,NE=1);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(1000,500;1000,1500;1500,1500;1500,500){TERMINAL_ID=>2};(3500,500;3500,1500;4000,1500;4000,500){TERMINAL_ID=>2}");
  EXPECT_EQ (o2.to_string (), "(0,0;0,2000;5000,2000;5000,0/1000,500;1500,500;1500,1500;1000,1500/3500,500;4000,500;4000,1500;3500,1500){TERMINAL_ID=>1};(0,0;0,2000;5000,2000;5000,0/1000,500;1500,500;1500,1500;1000,1500/3500,500;4000,500;4000,1500;3500,1500){TERMINAL_ID=>1}");
  EXPECT_EQ (o3.to_string (), "(-500,-500;-500,2500;5500,2500;5500,-500/0,0;5000,0;5000,2000;0,2000){TERMINAL_ID=>0};(-500,-500;-500,2500;5500,2500;5500,-500/0,0;5000,0;5000,2000;0,2000){TERMINAL_ID=>0}");
}

TEST(54_BJT4DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "bjt4_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorBJT4Transistor ex ("BJT4");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["E"] = &l0;
  dl["B"] = &l1;
  dl["C"] = &l2;
  dl["S"] = &l3;
  dl["tE"] = &o1;
  dl["tB"] = &o2;
  dl["tC"] = &o3;
  dl["tS"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device BJT4 $1 (C=(null),B=(null),E=(null),S=(null)) (AE=0.81,PE=3.6,AB=5,PB=9,AC=5,PC=9,NE=1);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(700,400;700,1300;1600,1300;1600,400){TERMINAL_ID=>2}");
  EXPECT_EQ (o2.to_string (), "(0,0;0,2000;2500,2000;2500,0/700,400;1600,400;1600,1300;700,1300){TERMINAL_ID=>1}");
  EXPECT_EQ (o3.to_string (), "(-1000,-500;-1000,2500;3000,2500;3000,-500/0,0;2500,0;2500,2000;0,2000){TERMINAL_ID=>0}");
  EXPECT_EQ (o4.to_string (), "(0,0;0,2000;2500,2000;2500,0){TERMINAL_ID=>3}");
}

