
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#include "dbDXFReader.h"

#include <utHead.h>

#include <stdlib.h>

static void run_test (ut::TestBase *_this, const char *file, const char *file_au, const char *map = 0, double dbu = 0.001, double dxf_unit = 1, int mode = 0, int ncircle = 100, double acircle = 0.0)
{
  db::DXFReaderOptions *opt = new db::DXFReaderOptions();
  opt->dbu = dbu;
  opt->unit = dxf_unit;
  opt->polyline_mode = mode;
  opt->circle_points = ncircle;
  opt->circle_accuracy = acircle;

  db::LayerMap lm;
  if (map) {
    unsigned int ln = 0;
    tl::Extractor ex (map);
    while (! ex.at_end ()) {
      std::string n;
      int l;
      ex.read_word_or_quoted (n);
      ex.test (":");
      ex.read (l);
      ex.test (",");
      lm.map (n, ln++, db::LayerProperties (l, 0));
    }
    opt->layer_map = lm;
    opt->create_other_layers = true;
  }

  db::LoadLayoutOptions options;
  options.set_options (opt);

  db::Layout layout;

  {
    std::string fn (ut::testsrc_private ());
    fn += "/testdata/dxf/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  _this->compare_layouts (layout, ut::testsrc_private () + std::string ("/testdata/dxf/") + file_au);
}

TEST(1a)
{
  run_test (_this, "t1.dxf.gz", "t1a_au.gds.gz");
}

TEST(1b)
{
  run_test (_this, "t1.dxf.gz", "t1b_au.gds.gz", 0, 0.01, 5.0);
}

TEST(2)
{
  run_test (_this, "t2.dxf.gz", "t2_au.gds.gz");
}

TEST(3)
{
  run_test (_this, "t3.dxf.gz", "t3_au.gds.gz");
}

TEST(4)
{
  run_test (_this, "t4.dxf.gz", "t4_au.gds.gz", "Metal:1,Metal2:5");
}

TEST(5)
{
  run_test (_this, "t5.dxf.gz", "t5_au.gds.gz");
}

TEST(6)
{
  run_test (_this, "t6.dxf.gz", "t6_au.gds.gz");
}

TEST(7)
{
  run_test (_this, "t7.dxf.gz", "t7_au.gds.gz");
}

TEST(8)
{
  run_test (_this, "t8.dxf.gz", "t8_au.gds.gz", "Metal:4,Kommentare:3,Bemassung:2");
}

TEST(9)
{
  run_test (_this, "t9.dxf.gz", "t9_au.gds.gz", "Bemassung:2,Metal:5,Kommentare:4");
}

TEST(10)
{
  run_test (_this, "t10.dxf.gz", "t10_au.gds.gz", "METAL:1,KOMMENTARE:4");
}

TEST(11)
{
  run_test (_this, "t11.dxf.gz", "t11_au.gds.gz");
}

TEST(12)
{
  run_test (_this, "t12.dxf.gz", "t12_au.gds.gz");
}

TEST(13)
{
  run_test (_this, "t13.dxf.gz", "t13_au.gds.gz");
}

TEST(14)
{
  run_test (_this, "t14.dxf.gz", "t14_au.gds.gz", "'A11-STRUKTUR__E_TYP_':10,A21_NITRID:11,'B11-KONTAKT':9,'B11-STRUKTUR':3,HELLFELD:7,MASKE:5,NORM_MIN_MAX_WAFER:6,RASTER:2,_BEGRENZUNG_A11_A21_A31_B1:8");
}

TEST(15)
{
  run_test (_this, "t15.dxf.gz", "t15_au.gds.gz", "TEXT:4,IGBT:5,Wire:7,Ceramic:11,LAYER_1:14,Diode:18,'DBC TOP Plate':19,'Terminal Position':20");
}

TEST(16)
{
  run_test (_this, "t16.dxf.gz", "t16_au.gds.gz", "TEXT:4,IGBT:5,Wire:7,Ceramic:11,LAYER_1:14,Diode:18,'DBC TOP Plate':19,'Terminal Position':20");
}

TEST(17)
{
  run_test (_this, "t17.dxf.gz", "t17_au.gds.gz", "TEXT:4,IGBT:5,Wire:7,Ceramic:11,LAYER_1:14,Diode:18,'DBC TOP Plate':19,'Terminal Position':20");
}

TEST(18)
{
  run_test (_this, "t18.dxf.gz", "t18_au.gds.gz");
}

TEST(19)
{
  run_test (_this, "t19.dxf.gz", "t19_au.gds.gz");
}

TEST(20)
{
  run_test (_this, "t20.dxf.gz", "t20_au.gds.gz");
}

TEST(21)
{
  run_test (_this, "t21.dxf.gz", "t21_au.gds.gz");
}

TEST(22)
{
  run_test (_this, "t22.dxf.gz", "t22_au.gds.gz");
}

TEST(23a)
{
  run_test (_this, "t23.dxf.gz", "t23a_au.gds.gz", 0, 0.001, 1, 0, 10);
}

TEST(23b)
{
  run_test (_this, "t23.dxf.gz", "t23b_au.gds.gz", 0, 0.001, 1, 1, 10);
}

TEST(23c)
{
  run_test (_this, "t23.dxf.gz", "t23c_au.gds.gz", 0, 0.001, 1, 2, 10);
}

TEST(23d)
{
  run_test (_this, "t23.dxf.gz", "t23d_au.gds.gz", 0, 0.001, 1, 3, 10);
}

TEST(23e)
{
  run_test (_this, "t23.dxf.gz", "t23e_au.gds.gz", 0, 0.001, 1, 4, 10);
}

TEST(26a)
{
  run_test (_this, "t26.dxf.gz", "t26a_au.gds.gz", 0, 0.001, 1, 0, 100);
}

TEST(26b)
{
  run_test (_this, "t26.dxf.gz", "t26b_au.gds.gz", 0, 0.001, 1, 1, 100);
}

TEST(26c)
{
  run_test (_this, "t26.dxf.gz", "t26c_au.gds.gz", 0, 0.001, 1, 2, 100);
}

TEST(26d)
{
  run_test (_this, "t26.dxf.gz", "t26d_au.gds.gz", 0, 0.001, 1, 3, 100);
}

TEST(26e)
{
  run_test (_this, "t26.dxf.gz", "t26e_au.gds.gz", 0, 0.001, 1, 4, 100);
}

TEST(27a)
{
  run_test (_this, "t27.dxf.gz", "t27a_au.gds.gz", 0, 0.001, 1, 0, 10);
}

TEST(27b)
{
  run_test (_this, "t27.dxf.gz", "t27b_au.gds.gz", 0, 0.001, 1, 1, 10);
}

TEST(27c)
{
  run_test (_this, "t27.dxf.gz", "t27c_au.gds.gz", 0, 0.001, 1, 2, 10);
}

TEST(27d)
{
  run_test (_this, "t27.dxf.gz", "t27d_au.gds.gz", 0, 0.001, 1, 3, 10);
}

TEST(27e)
{
  run_test (_this, "t27.dxf.gz", "t27e_au.gds.gz", 0, 0.001, 1, 4, 10);
}

TEST(28)
{
  run_test (_this, "t28.dxf.gz", "t28_au.gds.gz");
}

TEST(29)
{
  run_test (_this, "t29.dxf.gz", "t29_au.gds.gz");
}

TEST(29a)
{
  run_test (_this, "t29.dxf.gz", "t29a_au.gds.gz", 0, 0.001, 1, 4, 1000, 1);
}

TEST(29b)
{
  run_test (_this, "t29.dxf.gz", "t29b_au.gds.gz", 0, 0.001, 1, 4, 1000, 0.1);
}

TEST(29c)
{
  run_test (_this, "t29.dxf.gz", "t29c_au.gds.gz", 0, 0.001, 1, 4, 1000, 0.01);
}

TEST(29d)
{
  run_test (_this, "t29.dxf.gz", "t29d_au.gds.gz", 0, 0.001, 1, 4, 1000, 0.001);
}

