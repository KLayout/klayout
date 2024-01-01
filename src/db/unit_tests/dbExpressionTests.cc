
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


#include "tlExpression.h"
#include "tlUnitTest.h"
#include "dbBox.h"
#include "dbEdge.h"
#include "dbLayout.h"
#include "gsiDecl.h"
#include "dbLayerProperties.h"
#include "dbLayoutContextHandler.h"

#include <stdlib.h>
#include <math.h>

// ref layout
TEST(1)
{
  db::Layout l;
  l.dbu (0.05);
  l.insert_layer (db::LayerProperties (1, 15));
  l.insert_layer (db::LayerProperties ("name"));
  l.add_cell ("c1");
  l.add_cell ("c2");

  tl::Eval e, ee;
  db::LayoutContextHandler ctx (&l);
  e.set_ctx_handler (&ctx);
  tl::Variant v;

  bool error = false;

  v = e.parse ("1um").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20"));
  v = e.parse ("1um2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("400"));
  v = e.parse ("1micron").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20"));
  v = e.parse ("1micron2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("400"));
  v = e.parse ("1mic").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20"));
  v = e.parse ("1mic2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("400"));
  v = e.parse ("1m").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20000000"));
  v = e.parse ("1m2/1e14").execute ();
  EXPECT_EQ (v.to_string (), std::string ("4"));
  v = e.parse ("1mm").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20000"));
  v = e.parse ("1mm2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("400000000"));
  v = e.parse ("50nm").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("<1/15>").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("<   name >").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("<'n' + 'ame'>").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("<<c1>>").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("<<  c2   >>").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("<<'c' + '2'>>").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));

  error = true;
  try {
    v = e.parse ("60nm").execute (); // not a multiple of DBU
    error = false;
  } catch (...) { }
  EXPECT_EQ (error, true);

  error = true;
  try {
    v = ee.parse ("1 um").execute ();
    error = false;
  } catch (...) { }
  EXPECT_EQ (error, true);

  error = true;
  try {
    v = e.parse ("<1/1>").execute ();
    error = false;
  } catch (...) { }
  EXPECT_EQ (error, true);

  error = true;
  try {
    v = ee.parse ("<1/15>").execute ();
    error = false;
  } catch (...) { }
  EXPECT_EQ (error, true);

  error = true;
  try {
    v = ee.parse ("<<c1>>").execute ();
    error = false;
  } catch (...) { }
  EXPECT_EQ (error, true);

  error = true;
  try {
    v = e.parse ("<<c3>>").execute ();
    error = false;
  } catch (...) { }
  EXPECT_EQ (error, true);
}

//  db objects as Variant payload
TEST(10)
{
  tl::Variant v;
  v = tl::Variant::make_variant (db::Box (db::Point (0, 10), db::Point (20, 30)));
  EXPECT_EQ (v.is_user<db::Box> (), true)
  EXPECT_EQ (v.to_parsable_string (), "[box:(0,10;20,30)]");

  std::string s = v.to_parsable_string () + "," + tl::Variant (15.0).to_parsable_string ();
  tl::Variant vv;
  tl::Extractor ex (s.c_str ());
  ex.read (vv);
  ex.test (",");
  ex.read (v);
  EXPECT_EQ (vv.is_user<db::Box> (), true)
  EXPECT_EQ (vv.to_parsable_string (), "[box:(0,10;20,30)]");
  EXPECT_EQ ((int)v.type_code (), (int)tl::Variant::t_double);
  EXPECT_EQ (std::string(v.to_string()), "15");
}

//  db objects as Variant payload - backward compatibility check
TEST(11)
{
  std::string s = "[box:(0,10;20,30)],##15";
  tl::Variant vv;
  tl::Variant v;
  tl::Extractor ex (s.c_str ());
  ex.read (vv);
  ex.test (",");
  ex.read (v);
  EXPECT_EQ (vv.is_user<db::Box> (), true)
  EXPECT_EQ (vv.to_parsable_string (), "[box:(0,10;20,30)]");
  EXPECT_EQ ((int)v.type_code (), (int)tl::Variant::t_double);
  EXPECT_EQ (std::string(v.to_string()), "15");
}

//  db objects as Variant payload - backward compatibility check
TEST(12)
{
  std::string s = "[Box:(0,10;20,30)],##15";
  tl::Variant vv;
  tl::Variant v;
  tl::Extractor ex (s.c_str ());
  ex.read (vv);
  ex.test (",");
  ex.read (v);
  EXPECT_EQ (vv.is_user<db::Box> (), true)
  EXPECT_EQ (vv.to_parsable_string (), "[box:(0,10;20,30)]");
  EXPECT_EQ ((int)v.type_code (), (int)tl::Variant::t_double);
  EXPECT_EQ (std::string(v.to_string()), "15");
}

//  db objects as Variant payload - backward compatibility check
TEST(13)
{
  std::string s = "[layer:1/0],##15";
  tl::Variant vv;
  tl::Variant v;
  tl::Extractor ex (s.c_str ());
  ex.read (vv);
  ex.test (",");
  ex.read (v);
  EXPECT_EQ (vv.is_user<db::LayerProperties> (), true)
  EXPECT_EQ (vv.to_parsable_string (), "[layer:1/0]");
  EXPECT_EQ ((int)v.type_code (), (int)tl::Variant::t_double);
  EXPECT_EQ (std::string(v.to_string()), "15");
}

//  db objects as Variant payload - backward compatibility check
TEST(14)
{
  std::string s = "[LayerInfo:1/0],##15";
  tl::Variant vv;
  tl::Variant v;
  tl::Extractor ex (s.c_str ());
  ex.read (vv);
  ex.test (",");
  ex.read (v);
  EXPECT_EQ (vv.is_user<db::LayerProperties> (), true)
  EXPECT_EQ (vv.to_parsable_string (), "[layer:1/0]");
  EXPECT_EQ ((int)v.type_code (), (int)tl::Variant::t_double);
  EXPECT_EQ (std::string(v.to_string()), "15");
}

