
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


#include "dbDXFReader.h"
#include "dbTestSupport.h"
#include "tlUnitTest.h"

#include <stdlib.h>

static db::LayerMap string2lm (const char *map)
{
  db::LayerMap lm;
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
  return lm;
}

static void do_run_test (tl::TestBase *_this, const std::string &fn, const std::string &fn_au, const db::DXFReaderOptions &opt, bool as_oas)
{
  db::LoadLayoutOptions options;
  options.set_options (new db::DXFReaderOptions (opt));

  db::Layout layout;

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (_this, layout, fn_au, as_oas ? db::WriteOAS : db::WriteGDS2, 1);
}

static void run_test (tl::TestBase *_this, const char *file, const char *file_au, const db::DXFReaderOptions &opt = db::DXFReaderOptions (), bool as_oas = false)
{
  std::string fn = tl::testdata_private () + "/dxf/" + file;
  std::string fn_au = tl::testdata_private () + std::string ("/dxf/") + file_au;

  do_run_test (_this, fn, fn_au, opt, as_oas);
}

static void run_test_public (tl::TestBase *_this, const char *file, const char *file_au, const db::DXFReaderOptions &opt = db::DXFReaderOptions (), bool as_oas = false)
{
  std::string fn = tl::testdata () + "/dxf/" + file;
  std::string fn_au = tl::testdata () + std::string ("/dxf/") + file_au;

  do_run_test (_this, fn, fn_au, opt, as_oas);
}

TEST(KeepLN1)
{
  db::DXFReaderOptions opt;
  run_test_public (_this, "keep_ln.dxf.gz", "keep_ln1_au.oas.gz", opt, true /*because of layer names*/);
}

TEST(KeepLN2)
{
  db::DXFReaderOptions opt;
  opt.keep_layer_names = true;
  run_test_public (_this, "keep_ln.dxf.gz", "keep_ln2_au.oas.gz", opt, true /*because of layer names*/);
}

TEST(1a)
{
  run_test (_this, "t1.dxf.gz", "t1a_au.gds.gz");
}

TEST(1b)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.01;
  opt.unit = 5.0;
  run_test (_this, "t1.dxf.gz", "t1b_au.gds.gz", opt);
}

TEST(2)
{
  run_test (_this, "t2.dxf.gz", "t2_au2.gds.gz");
}

TEST(3)
{
  run_test (_this, "t3.dxf.gz", "t3_au.gds.gz");
}

TEST(4)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("Metal:1,Metal2:5");
  opt.create_other_layers = true;
  run_test (_this, "t4.dxf.gz", "t4_au.gds.gz", opt);
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
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("Metal:4,Kommentare:3,Bemassung:2");
  opt.create_other_layers = true;
  run_test (_this, "t8.dxf.gz", "t8_au.gds.gz", opt);
}

TEST(9)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("Bemassung:2,Metal:5,Kommentare:4");
  opt.create_other_layers = true;
  run_test (_this, "t9.dxf.gz", "t9_au.gds.gz", opt);
}

TEST(10)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("METAL:1,KOMMENTARE:4");
  opt.create_other_layers = true;
  run_test (_this, "t10.dxf.gz", "t10_au.gds.gz", opt);
}

TEST(11)
{
  run_test (_this, "t11.dxf.gz", "t11_au.gds.gz");
}

TEST(12)
{
  run_test (_this, "t12.dxf.gz", "t12_au.gds.gz");
}

TEST(14)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("'A11-STRUKTUR__E_TYP_':10,A21_NITRID:11,'B11-KONTAKT':9,'B11-STRUKTUR':3,HELLFELD:7,MASKE:5,NORM_MIN_MAX_WAFER:6,RASTER:2,_BEGRENZUNG_A11_A21_A31_B1:8");
  opt.create_other_layers = true;
  run_test (_this, "t14.dxf.gz", "t14_au.gds.gz", opt);
}

TEST(15)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("TEXT:4,IGBT:5,Wire:7,Ceramic:11,LAYER_1:14,Diode:18,'DBC TOP Plate':19,'Terminal Position':20");
  opt.create_other_layers = true;
  run_test (_this, "t15.dxf.gz", "t15_au2_2.gds.gz", opt);
}

TEST(16)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("TEXT:4,IGBT:5,Wire:7,Ceramic:11,LAYER_1:14,Diode:18,'DBC TOP Plate':19,'Terminal Position':20");
  opt.create_other_layers = true;
  run_test (_this, "t16.dxf.gz", "t16_au2_2.gds.gz", opt);
}

TEST(17)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("TEXT:4,IGBT:5,Wire:7,Ceramic:11,LAYER_1:14,Diode:18,'DBC TOP Plate':19,'Terminal Position':20");
  opt.create_other_layers = true;
  run_test (_this, "t17.dxf.gz", "t17_au2_2.gds.gz", opt);
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
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 0;
  opt.circle_points = 10;
  run_test (_this, "t23.dxf.gz", "t23a_au.gds.gz", opt);
}

TEST(23b)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 1;
  opt.circle_points = 10;
  run_test (_this, "t23.dxf.gz", "t23b_au.gds.gz", opt);
}

TEST(23c)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 2;
  opt.circle_points = 10;
  run_test (_this, "t23.dxf.gz", "t23c_au.gds.gz", opt);
}

TEST(23d)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 3;
  opt.circle_points = 10;
  run_test (_this, "t23.dxf.gz", "t23d_au.gds.gz", opt);
}

TEST(23e)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 4;
  opt.circle_points = 10;
  run_test (_this, "t23.dxf.gz", "t23e_au.gds.gz", opt);
}

TEST(26a)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 0;
  opt.circle_points = 100;
  run_test (_this, "t26.dxf.gz", "t26a_au.gds.gz", opt);
}

TEST(26b)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 1;
  opt.circle_points = 100;
  run_test (_this, "t26.dxf.gz", "t26b_au.gds.gz", opt);
}

TEST(26c)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 2;
  opt.circle_points = 100;
  run_test (_this, "t26.dxf.gz", "t26c_au.gds.gz", opt);
}

TEST(26d)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 3;
  opt.circle_points = 100;
  run_test (_this, "t26.dxf.gz", "t26d_au.gds.gz", opt);
}

TEST(26e)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 4;
  opt.circle_points = 100;
  run_test (_this, "t26.dxf.gz", "t26e_au.gds.gz", opt);
}

TEST(27a)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 0;
  opt.circle_points = 10;
  run_test (_this, "t27.dxf.gz", "t27a_au.gds.gz", opt);
}

TEST(27b)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 1;
  opt.circle_points = 10;
  run_test (_this, "t27.dxf.gz", "t27b_au.gds.gz", opt);
}

TEST(27c)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 2;
  opt.circle_points = 10;
  run_test (_this, "t27.dxf.gz", "t27c_au.gds.gz", opt);
}

TEST(27d)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 3;
  opt.circle_points = 10;
  run_test (_this, "t27.dxf.gz", "t27d_au.gds.gz", opt);
}

TEST(27e)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 4;
  opt.circle_points = 10;
  run_test (_this, "t27.dxf.gz", "t27e_au.gds.gz", opt);
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
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 4;
  opt.circle_points = 1000;
  opt.circle_accuracy = 1;
  run_test (_this, "t29.dxf.gz", "t29a_au.gds.gz", opt);
}

TEST(29b)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 4;
  opt.circle_points = 1000;
  opt.circle_accuracy = 0.1;
  run_test (_this, "t29.dxf.gz", "t29b_au.gds.gz", opt);
}

TEST(29c)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 4;
  opt.circle_points = 1000;
  opt.circle_accuracy = 0.01;
  run_test (_this, "t29.dxf.gz", "t29c_au.gds.gz", opt);
}

TEST(29d)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1;
  opt.polyline_mode = 4;
  opt.circle_points = 1000;
  opt.circle_accuracy = 0.001;
  run_test (_this, "t29.dxf.gz", "t29d_au.gds.gz", opt);
}

TEST(30)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1000;
  opt.polyline_mode = 4;
  opt.circle_points = 1000;
  opt.circle_accuracy = 0.001;
  run_test (_this, "t30.dxf.gz", "t30d_au.gds.gz", opt);
}

//  accuracy
TEST(31)
{
  db::DXFReaderOptions opt;
  opt.dbu = 0.001;
  opt.unit = 1000;

  opt.contour_accuracy = 0;
  run_test (_this, "t31.dxf.gz", "t31a_au.gds.gz", opt);

  opt.contour_accuracy = 0.005;
  run_test (_this, "t31.dxf.gz", "t31b_au.gds.gz", opt);

  opt.contour_accuracy = 0.01;
  run_test (_this, "t31.dxf.gz", "t31c_au.gds.gz", opt);

  opt.contour_accuracy = 0.02;
  run_test (_this, "t31.dxf.gz", "t31d_au.gds.gz", opt);
}

//  issue #198
TEST(32)
{
  db::DXFReaderOptions opt;
  opt.layer_map = string2lm ("L11D0:1,L12D0:2");
  opt.create_other_layers = false;
  opt.polyline_mode = 3;

  opt.contour_accuracy = 0.0;
  run_test_public (_this, "round_path.dxf.gz", "t32a_au.gds.gz", opt);

  opt.contour_accuracy = 0.1;
  run_test_public (_this, "round_path.dxf.gz", "t32b_au.gds.gz", opt);

  opt.contour_accuracy = 1.0;
  run_test_public (_this, "round_path.dxf.gz", "t32c_au.gds.gz", opt);

  opt.polyline_mode = 4;
  run_test_public (_this, "round_path.dxf.gz", "t32d_au.gds.gz", opt);

  opt.polyline_mode = 2;
  run_test_public (_this, "round_path.dxf.gz", "t32e_au.gds.gz", opt);
}

//  issue #704
TEST(33)
{
  db::DXFReaderOptions opt;
  opt.polyline_mode = 3;

  run_test (_this, "t33.dxf.gz", "t33a_au.gds.gz", opt);

  opt.circle_accuracy = 1.0;
  run_test (_this, "t33.dxf.gz", "t33b_au.gds.gz", opt);

  opt.circle_accuracy = 50.0;
  run_test (_this, "t33.dxf.gz", "t33c_au.gds.gz", opt);

  opt.circle_accuracy = 0.0;
  opt.polyline_mode = 4;
  run_test (_this, "t33.dxf.gz", "t33d_au.gds.gz", opt);

  opt.polyline_mode = 2;
  run_test (_this, "t33.dxf.gz", "t33e_au.gds.gz", opt);
}

//  issue #1173
TEST(34)
{
  db::DXFReaderOptions opt;
  opt.polyline_mode = 3;

  run_test_public (_this, "issue_1173.dxf", "issue_1173_au.gds.gz", opt);
}

//  issue #1422
TEST(35a)
{
  db::DXFReaderOptions opt;
  run_test_public (_this, "issue_1422a.dxf", "issue_1422a_au.gds.gz", opt);
}

//  issue #1422
TEST(35b)
{
  db::DXFReaderOptions opt;
  run_test_public (_this, "issue_1422b.dxf", "issue_1422b_au.gds.gz", opt);
}

//  issue #1422
TEST(35c)
{
  db::DXFReaderOptions opt;
  run_test_public (_this, "issue_1422c.dxf", "issue_1422c_au.gds.gz", opt);
}

//  issue #1592, polyline mode 2
TEST(36a)
{
  db::DXFReaderOptions opt;
  opt.dbu = 1e-5;
  opt.polyline_mode = 2;
  run_test_public (_this, "issue_1592.dxf.gz", "issue_1592a_au.oas.gz", opt, true);
}

//  issue #1592
TEST(36b)
{
  db::DXFReaderOptions opt;
  opt.dbu = 1e-5;
  run_test_public (_this, "issue_1592.dxf.gz", "issue_1592b_au.oas.gz", opt, true);
}
