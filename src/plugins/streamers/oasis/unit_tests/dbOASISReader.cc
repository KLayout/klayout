
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


#include "dbOASISReader.h"
#include "dbTextWriter.h"
#include "dbTestSupport.h"
#include "tlLog.h"
#include "tlUnitTest.h"
#include "tlStream.h"

#include <stdlib.h>

TEST(1_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t1.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(1_2)
{
  const char *expected = 
    "begin_lib 2\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t1.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(1_3)
{
  const char *expected = 
    "begin_lib 0.4\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t1.3.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(1_4)
{
  const char *expected = 
    "begin_lib 0.08\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t1.4.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(1_5)
{
  const char *expected = 
    "begin_lib 0.08\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t1.5.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(10_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {0 0} {10 20}\n"
    "box 1 2 {100 -100} {110 -80}\n"
    "box 1 2 {200 -200} {210 -180}\n"
    "box 1 2 {300 -300} {310 -280}\n"
    "text 2 1 0 0 {0 0} {A}\n"
    "text 2 1 0 0 {100 -100} {A}\n"
    "text 2 1 0 0 {200 -200} {A}\n"
    "text 2 1 0 0 {300 -300} {A}\n"
    "end_cell\n"
    "begin_cell {B}\n"
    "sref {A} 0 0 1 {0 0}\n"
    "sref {A} 0 0 1 {50 50}\n"
    "box 1 2 {0 0} {20 10}\n"
    "box 1 2 {100 100} {120 110}\n"
    "box 1 2 {200 200} {220 210}\n"
    "box 1 2 {300 300} {320 310}\n"
    "text 2 1 0 0 {0 0} {B}\n"
    "text 2 1 0 0 {100 100} {B}\n"
    "text 2 1 0 0 {200 200} {B}\n"
    "text 2 1 0 0 {300 300} {B}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {B} 0 0 1 {0 0}\n"
    "box 1 2 {0 0} {50 5}\n"
    "text 2 1 0 0 {0 0} {TOP}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t10.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3000 0} {3000 100} {3100 100} {3100 50} {3150 50} {3150 0} {3000 0}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2000 0} {2150 0} {2150 50} {2100 50}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {0 0} {100 200}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {0 1000} {100 1200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {0 2000} {100 2200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 3000} {100 3200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 4000} {100 4200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1000 0} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_2)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3000 0} {3000 100} {3100 100} {3100 50} {3150 50} {3150 0} {3000 0}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3300 0} {3300 100} {3400 100} {3400 50} {3450 50} {3450 0} {3300 0}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3600 0} {3600 100} {3700 100} {3700 50} {3750 50} {3750 0} {3600 0}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3000 320} {3000 420} {3100 420} {3100 370} {3150 370} {3150 320} {3000 320}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3300 320} {3300 420} {3400 420} {3400 370} {3450 370} {3450 320} {3300 320}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3600 320} {3600 420} {3700 420} {3700 370} {3750 370} {3750 320} {3600 320}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2000 0} {2150 0} {2150 50} {2100 50}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2300 0} {2450 0} {2450 50} {2400 50}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2600 0} {2750 0} {2750 50} {2700 50}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2000 320} {2150 320} {2150 370} {2100 370}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2300 320} {2450 320} {2450 370} {2400 370}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2600 320} {2750 320} {2750 370} {2700 370}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {0 0} {100 200}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {300 0} {400 200}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {600 0} {700 200}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {0 320} {100 520}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {300 320} {400 520}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {600 320} {700 520}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {0 1000} {100 1200}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {300 1000} {400 1200}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {600 1000} {700 1200}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {0 1320} {100 1520}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {300 1320} {400 1520}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {600 1320} {700 1520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {0 2000} {100 2200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {300 2000} {400 2200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {600 2000} {700 2200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {0 2320} {100 2520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {300 2320} {400 2520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {600 2320} {700 2520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 3000} {100 3200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 3000} {400 3200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 3000} {700 3200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 3320} {100 3520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 3320} {400 3520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 3320} {700 3520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 4000} {100 4200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 4000} {400 4200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 4000} {700 4200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 4320} {100 4520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 4320} {400 4520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 4320} {700 4520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1000 0} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1300 0} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1600 0} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1000 320} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1300 320} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1600 320} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_3)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "boundaryp $props 1 2 {3000 0} {3000 100} {3100 100} {3100 50} {3150 50} {3150 0} {3000 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2000 0} {2150 0} {2150 50} {2100 50}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "boxp $props 1 2 {0 1000} {100 1200}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 2000} {100 2200}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 3000} {100 3200}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 4000} {100 4200}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 5000} {100 5200}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1000 0} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.3.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_4)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {-300 400}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {0 200}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {0 400}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {300 400}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 1 1 {700 400}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 90 0 1 {700 1400}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 90 1 1 {700 2400}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 4 {2000 0} {2900 0} {2000 1200}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 4 {4000 0} {4900 0} {4000 1200}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 1 {6000 0} {6960 0} {6000 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 1 4 {8000 0} {8000 0} {8000 1240}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10000 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10320 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10650 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10990 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 4 {12000 0} {12930 960} {10680 1320}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.4.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_5)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3000 0} {3000 100} {3100 100} {3100 50} {3150 50} {3150 0} {3000 0}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3300 0} {3300 100} {3400 100} {3400 50} {3450 50} {3450 0} {3300 0}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3600 0} {3600 100} {3700 100} {3700 50} {3750 50} {3750 0} {3600 0}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3000 320} {3000 420} {3100 420} {3100 370} {3150 370} {3150 320} {3000 320}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3300 320} {3300 420} {3400 420} {3400 370} {3450 370} {3450 320} {3300 320}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "  {{PROP1} {nil}}\n"
    "}\n"
    "boundaryp $props 1 2 {3600 320} {3600 420} {3700 420} {3700 370} {3750 370} {3750 320} {3600 320}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2000 0} {2150 0} {2150 50} {2100 50}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2300 0} {2450 0} {2450 50} {2400 50}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2600 0} {2750 0} {2750 50} {2700 50}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2000 320} {2150 320} {2150 370} {2100 370}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2300 320} {2450 320} {2450 370} {2400 370}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "pathp $props 1 2 20 5 -5 {2600 320} {2750 320} {2750 370} {2700 370}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {0 0} {100 200}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {300 0} {400 200}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {600 0} {700 200}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {0 320} {100 520}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {300 320} {400 520}\n"
    "set props {\n"
    "  {{PROPX} {nil}}\n"
    "}\n"
    "boxp $props 1 2 {600 320} {700 520}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {0 1000} {100 1200}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {300 1000} {400 1200}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {600 1000} {700 1200}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {0 1320} {100 1520}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {300 1320} {400 1520}\n"
    "set props {\n"
    "  {{PROP0} {-5}}\n"
    "}\n"
    "boxp $props 1 2 {600 1320} {700 1520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {0 2000} {100 2200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {300 2000} {400 2200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {600 2000} {700 2200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {0 2320} {100 2520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {300 2320} {400 2520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,PROP_VALUE2,PropStringId12}}\n"
    "}\n"
    "boxp $props 1 2 {600 2320} {700 2520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 3000} {100 3200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 3000} {400 3200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 3000} {700 3200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 3320} {100 3520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 3320} {400 3520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 3320} {700 3520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 4000} {100 4200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 4000} {400 4200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 4000} {700 4200}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {0 4320} {100 4520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {300 4320} {400 4520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "boxp $props 1 2 {600 4320} {700 4520}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1000 0} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1300 0} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1600 0} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1000 320} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1300 320} {A}\n"
    "set props {\n"
    "  {{PROP0} {25,-124,Property string value for ID 13}}\n"
    "}\n"
    "textp $props 2 1 0 0 {1600 320} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.5.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_6)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "  {26 {PROP_VALUE26}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {-300 400}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {0 400}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {0 400}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 0 1 {300 400}\n"
    "set props {\n"
    "  {10 {Property string value for ID 13}}\n"
    "}\n"
    "srefp $props {A} 0 1 1 {700 400}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 90 0 1 {700 1400}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 90 1 1 {700 2400}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 4 {2000 0} {2900 0} {2000 1200}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 4 {4000 0} {4900 0} {4000 1200}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 1 {6000 0} {6960 0} {6000 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 1 4 {8000 0} {8000 0} {8000 1240}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10000 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10320 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10650 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "srefp $props {A} 270 1 1 {10990 0}\n"
    "set props {\n"
    "  {25 {PROP_VALUE2}}\n"
    "}\n"
    "arefp $props {A} 270 1 1 3 4 {12000 0} {12930 960} {10680 1320}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.6.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_7)
{
  const char *expected = 
    "set props {\n"
    "  {{FileProp1} {FileProp1Value}}\n"
    "  {{FileProp2} {FileProp1Value}}\n"
    "}\n"
    "begin_libp $props 0.001\n"
    "set props {\n"
    "  {{CellProp0} {CPValue0}}\n"
    "  {{CellProp1} {CPValue}}\n"
    "  {{CellProp2} {CPValue}}\n"
    "}\n"
    "begin_cellp $props {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.7.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(11_8)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.8.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Modal variable accessed before being defined: last-value-list (position=96, cell=)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(11_9)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t11.9.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Modal variable accessed before being defined: last-value-list (position=118, cell=)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(12_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "path 1 2 300 150 150 {-100 200}\n"
    "path 1 2 300 150 150 {-100 600}\n"
    "path 1 2 2 1 1 {-100 1400}\n"
    "path 1 2 12 6 6 {-100 1800}\n"
    "path 1 2 40 20 20 {-100 2200}\n"
    "path 1 2 200 100 100 {-100 2600}\n"
    "path 1 2 200 100 100 {300 2600}\n"
    "path 1 2 200 100 100 {700 2600}\n"
    "path 1 2 200 100 100 {-100 2900}\n"
    "path 1 2 200 100 100 {300 2900}\n"
    "path 1 2 200 100 100 {700 2900}\n"
    "path 1 2 200 100 100 {-100 3200}\n"
    "path 1 2 200 100 100 {300 3200}\n"
    "path 1 2 200 100 100 {700 3200}\n"
    "path 1 2 200 100 100 {-100 3500}\n"
    "path 1 2 200 100 100 {300 3500}\n"
    "path 1 2 200 100 100 {700 3500}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t12.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(13_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "path 1 2 300 150 150 {1000 2000}\n"
    "path 1 5 300 150 150 {1000 5000}\n"
    "path 1 6 300 150 150 {1000 6000}\n"
    "path 1 8 300 150 150 {1000 8000}\n"
    "path 5 2 300 150 150 {5000 2000}\n"
    "path 5 5 300 150 150 {5000 5000}\n"
    "path 5 6 300 150 150 {5000 6000}\n"
    "path 5 8 300 150 150 {5000 8000}\n"
    "path 6 2 300 150 150 {6000 2000}\n"
    "path 6 5 300 150 150 {6000 5000}\n"
    "path 6 6 300 150 150 {6000 6000}\n"
    "path 6 8 300 150 150 {6000 8000}\n"
    "path 7 2 300 150 150 {7000 2000}\n"
    "path 7 5 300 150 150 {7000 5000}\n"
    "path 7 6 300 150 150 {7000 6000}\n"
    "path 7 8 300 150 150 {7000 8000}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t13.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
    EXPECT_EQ (map.to_string (), "layer_map('1/2 : \\'AA;L5A\\' (1/2)';'1/5 : \\'AA;L5A\\' (1/5)';'1/6 : \\'AA;L5A\\' (1/6)';'1/8 : \\'AA;L5A\\' (1/8)';'5/2 : \\'AA;L5A;H5A;E5A;I56A;E5L4\\' (5/2)';'5/5 : \\'AA;L5A;H5A;E5A;I56A;E5H4;E5I47\\' (5/5)';'5/6 : \\'AA;L5A;H5A;E5A;I56A;E5H4;E5I47\\' (5/6)';'5/8 : \\'AA;L5A;H5A;E5A;I56A;E5H4\\' (5/8)';'6/2 : \\'AA;H5A;I56A\\' (6/2)';'6/5 : \\'AA;H5A;I56A\\' (6/5)';'6/6 : \\'AA;H5A;I56A\\' (6/6)';'6/8 : \\'AA;H5A;I56A\\' (6/8)';'7/2 : \\'AA;H5A\\' (7/2)';'7/5 : \\'AA;H5A\\' (7/5)';'7/6 : \\'AA;H5A\\' (7/6)';'7/8 : \\'AA;H5A\\' (7/8)')")
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(13_2)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "text 1 2 0 0 {1000 2000} {A}\n"
    "text 1 5 0 0 {1000 5000} {A}\n"
    "text 1 6 0 0 {1000 6000} {A}\n"
    "text 1 8 0 0 {1000 8000} {A}\n"
    "text 5 2 0 0 {5000 2000} {A}\n"
    "text 5 5 0 0 {5000 5000} {A}\n"
    "text 5 6 0 0 {5000 6000} {A}\n"
    "text 5 8 0 0 {5000 8000} {A}\n"
    "text 6 2 0 0 {6000 2000} {A}\n"
    "text 6 5 0 0 {6000 5000} {A}\n"
    "text 6 6 0 0 {6000 6000} {A}\n"
    "text 6 8 0 0 {6000 8000} {A}\n"
    "text 7 2 0 0 {7000 2000} {A}\n"
    "text 7 5 0 0 {7000 5000} {A}\n"
    "text 7 6 0 0 {7000 6000} {A}\n"
    "text 7 8 0 0 {7000 8000} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t13.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
    EXPECT_EQ (map.to_string (), "layer_map('1/2 : \\'AA;L5A\\' (1/2)';'1/5 : \\'AA;L5A\\' (1/5)';'1/6 : \\'AA;L5A\\' (1/6)';'1/8 : \\'AA;L5A\\' (1/8)';'5/2 : \\'AA;L5A;H5A;E5A;I56A\\' (5/2)';'5/5 : \\'AA;L5A;H5A;E5A;I56A\\' (5/5)';'5/6 : \\'AA;L5A;H5A;E5A;I56A\\' (5/6)';'5/8 : \\'AA;L5A;H5A;E5A;I56A\\' (5/8)';'6/2 : \\'AA;H5A;I56A\\' (6/2)';'6/5 : \\'AA;H5A;I56A\\' (6/5)';'6/6 : \\'AA;H5A;I56A\\' (6/6)';'6/8 : \\'AA;H5A;I56A\\' (6/8)';'7/2 : \\'AA;H5A\\' (7/2)';'7/5 : \\'AA;H5A\\' (7/5)';'7/6 : \\'AA;H5A\\' (7/6)';'7/8 : \\'AA;H5A\\' (7/8)')");
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(13_3)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "path 1 2 300 150 150 {1000 2000}\n"
    "text 1 2 0 0 {1000 2000} {A}\n"
    "path 1 5 300 150 150 {1000 5000}\n"
    "text 1 5 0 0 {1000 5000} {A}\n"
    "path 1 6 300 150 150 {1000 6000}\n"
    "text 1 6 0 0 {1000 6000} {A}\n"
    "path 1 8 300 150 150 {1000 8000}\n"
    "text 1 8 0 0 {1000 8000} {A}\n"
    "path 5 2 300 150 150 {5000 2000}\n"
    "text 5 2 0 0 {5000 2000} {A}\n"
    "path 5 5 300 150 150 {5000 5000}\n"
    "text 5 5 0 0 {5000 5000} {A}\n"
    "path 5 6 300 150 150 {5000 6000}\n"
    "text 5 6 0 0 {5000 6000} {A}\n"
    "path 5 8 300 150 150 {5000 8000}\n"
    "text 5 8 0 0 {5000 8000} {A}\n"
    "path 6 2 300 150 150 {6000 2000}\n"
    "text 6 2 0 0 {6000 2000} {A}\n"
    "path 6 5 300 150 150 {6000 5000}\n"
    "text 6 5 0 0 {6000 5000} {A}\n"
    "path 6 6 300 150 150 {6000 6000}\n"
    "text 6 6 0 0 {6000 6000} {A}\n"
    "path 6 8 300 150 150 {6000 8000}\n"
    "text 6 8 0 0 {6000 8000} {A}\n"
    "path 7 2 300 150 150 {7000 2000}\n"
    "text 7 2 0 0 {7000 2000} {A}\n"
    "path 7 5 300 150 150 {7000 5000}\n"
    "text 7 5 0 0 {7000 5000} {A}\n"
    "path 7 6 300 150 150 {7000 6000}\n"
    "text 7 6 0 0 {7000 6000} {A}\n"
    "path 7 8 300 150 150 {7000 8000}\n"
    "text 7 8 0 0 {7000 8000} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t13.3.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
    EXPECT_EQ (map.to_string (), "layer_map('1/2 : \\'TAA;TL5A;AA;L5A\\' (1/2)';'1/5 : \\'TAA;TL5A;AA;L5A\\' (1/5)';'1/6 : \\'TAA;TL5A;AA;L5A\\' (1/6)';'1/8 : \\'TAA;TL5A;AA;L5A\\' (1/8)';'5/2 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/2)';'5/5 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/5)';'5/6 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/6)';'5/8 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/8)';'6/2 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/2)';'6/5 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/5)';'6/6 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/6)';'6/8 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/8)';'7/2 : \\'TAA;TH5A;AA;H5A\\' (7/2)';'7/5 : \\'TAA;TH5A;AA;H5A\\' (7/5)';'7/6 : \\'TAA;TH5A;AA;H5A\\' (7/6)';'7/8 : \\'TAA;TH5A;AA;H5A\\' (7/8)')");
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(13_4)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "path 1 2 300 150 150 {1000 2000}\n"
    "text 1 2 0 0 {1000 2000} {A}\n"
    "path 1 5 300 150 150 {1000 5000}\n"
    "text 1 5 0 0 {1000 5000} {A}\n"
    "path 1 6 300 150 150 {1000 6000}\n"
    "text 1 6 0 0 {1000 6000} {A}\n"
    "path 1 8 300 150 150 {1000 8000}\n"
    "text 1 8 0 0 {1000 8000} {A}\n"
    "path 5 2 300 150 150 {5000 2000}\n"
    "text 5 2 0 0 {5000 2000} {A}\n"
    "path 5 5 300 150 150 {5000 5000}\n"
    "text 5 5 0 0 {5000 5000} {A}\n"
    "path 5 6 300 150 150 {5000 6000}\n"
    "text 5 6 0 0 {5000 6000} {A}\n"
    "path 5 8 300 150 150 {5000 8000}\n"
    "text 5 8 0 0 {5000 8000} {A}\n"
    "path 6 2 300 150 150 {6000 2000}\n"
    "text 6 2 0 0 {6000 2000} {A}\n"
    "path 6 5 300 150 150 {6000 5000}\n"
    "text 6 5 0 0 {6000 5000} {A}\n"
    "path 6 6 300 150 150 {6000 6000}\n"
    "text 6 6 0 0 {6000 6000} {A}\n"
    "path 6 8 300 150 150 {6000 8000}\n"
    "text 6 8 0 0 {6000 8000} {A}\n"
    "path 7 2 300 150 150 {7000 2000}\n"
    "text 7 2 0 0 {7000 2000} {A}\n"
    "path 7 5 300 150 150 {7000 5000}\n"
    "text 7 5 0 0 {7000 5000} {A}\n"
    "path 7 6 300 150 150 {7000 6000}\n"
    "text 7 6 0 0 {7000 6000} {A}\n"
    "path 7 8 300 150 150 {7000 8000}\n"
    "text 7 8 0 0 {7000 8000} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t13.4.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
    EXPECT_EQ (map.to_string (), "layer_map('1/2 : \\'TAA;TL5A;AA;L5A\\' (1/2)';'1/5 : \\'TAA;TL5A;AA;L5A\\' (1/5)';'1/6 : \\'TAA;TL5A;AA;L5A\\' (1/6)';'1/8 : \\'TAA;TL5A;AA;L5A\\' (1/8)';'5/2 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/2)';'5/5 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/5)';'5/6 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/6)';'5/8 : \\'TAA;TL5A;TH5A;TE5A;TI56A;AA;L5A;H5A;E5A;I56A\\' (5/8)';'6/2 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/2)';'6/5 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/5)';'6/6 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/6)';'6/8 : \\'TAA;TH5A;TI56A;AA;H5A;I56A\\' (6/8)';'7/2 : \\'TAA;TH5A;AA;H5A\\' (7/2)';'7/5 : \\'TAA;TH5A;AA;H5A\\' (7/5)';'7/6 : \\'TAA;TH5A;AA;H5A\\' (7/6)';'7/8 : \\'TAA;TH5A;AA;H5A\\' (7/8)')");
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(14_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABCDH}\n"
    "box 0 0 {110 1270} {650 1800}\n"
    "boundary 1 0 {1970 1590} {1490 1640} {1520 2000} {2150 2020} {1970 1590}\n"
    "boundary 1 0 {120 680} {50 900} {-400 860} {-370 1260} {-400 2630} {940 2570} {900 1750} {690 1740} {690 1840} {80 1850} {80 1240} {700 1230} {680 1700} {1340 1700} {1320 2170} {2130 2160} {2120 2060} {1490 2040} {1440 1540} {870 1530} {870 870} {130 890} {120 680}\n"
    "boundary 1 0 {2220 610} {1730 660} {1690 1420} {2330 1410} {2220 610}\n"
    "boundary 1 0 {-210 -100} {-420 810} {-50 850} {90 470} {430 460} {460 360} {140 380} {270 -80} {-210 -100}\n"
    "boundary 1 0 {1620 640} {1560 780} {170 830} {180 860} {900 840} {920 860} {1610 860} {1600 1510} {1580 1540} {1490 1530} {1500 1600} {1970 1570} {1990 1450} {1670 1450} {1710 660} {1620 640}\n"
    "boundary 1 0 {1690 -80} {370 -40} {300 330} {610 300} {620 520} {870 480} {910 260} {1250 270} {1230 560} {160 580} {160 810} {1540 750} {1690 -80}\n"
    "boundary 1 0 {970 1740} {970 2590} {1900 2530} {1910 2200} {1290 2220} {1300 1740} {970 1740}\n"
    "boundary 1 0 {2030 1450} {2020 1590} {2160 2000} {2150 2190} {1940 2200} {1930 2530} {2430 2480} {2300 1450} {2030 1450}\n"
    "box 1 0 {900 890} {1580 1500}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t14.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(2_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {XYZ}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t2.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(2_2)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "end_cell\n"
    "begin_cell {XYZ}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t2.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(2_3)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t2.3.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Explicit and implicit CELLNAME modes cannot be mixed (position=45, cell=)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(2_4)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "end_cell\n"
    "begin_cell {XYZ}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t2.4.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(2_5)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t2.5.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: No cellname declared for cell id 2 (position=305, cell=#2)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(2_6)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "end_cell\n"
    "begin_cell { XYZ}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t2.6.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(3_1)
{
  const char *expected;
  if (db::default_editable_mode ()) {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -400} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -300} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -500} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -500} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -500} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -488} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -488} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -488} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -476} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -476} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -476} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -464} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -464} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -464} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -700} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -700} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -700} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -688} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -688} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -688} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -676} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -676} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -676} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -664} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -664} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -664} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -900} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -900} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -900} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1100} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1088} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1076} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1064} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {312 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {325 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {339 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {312 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {327 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {345 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1700} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1690} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1679} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1890} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1875} {TEXT_ABC}\n"
      "text 2 1 0 0 {200 -400} {TEXT_ABC}\n"
      "text 2 1 0 0 {267 -2136} {TEXT_ABC}\n"
      "text 2 1 0 0 {270 -2270} {TEXT_ABC}\n"
      "text 2 1 0 0 {270 -2670} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -2100} {TEXT_ABC}\n"
      "text 2 1 0 0 {289 -2112} {TEXT_ABC}\n"
      "text 2 1 0 0 {299 -2112} {TEXT_ABC}\n"
      "text 2 1 0 0 {309 -2112} {TEXT_ABC}\n"
      "text 2 1 0 0 {278 -2124} {TEXT_ABC}\n"
      "text 2 1 0 0 {288 -2124} {TEXT_ABC}\n"
      "text 2 1 0 0 {298 -2124} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1900} {TEXT_ABC}\n"
      "text 2 1 0 0 {277 -2136} {TEXT_ABC}\n"
      "text 2 1 0 0 {287 -2136} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2300} {TEXT_ABC}\n"
      "text 2 1 0 0 {311 -2288} {TEXT_ABC}\n"
      "text 2 1 0 0 {322 -2276} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -2290} {TEXT_ABC}\n"
      "text 2 1 0 0 {301 -2278} {TEXT_ABC}\n"
      "text 2 1 0 0 {312 -2266} {TEXT_ABC}\n"
      "text 2 1 0 0 {280 -2280} {TEXT_ABC}\n"
      "text 2 1 0 0 {291 -2268} {TEXT_ABC}\n"
      "text 2 1 0 0 {302 -2256} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2100} {TEXT_ABC}\n"
      "text 2 1 0 0 {281 -2258} {TEXT_ABC}\n"
      "text 2 1 0 0 {292 -2246} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2500} {TEXT_ABC}\n"
      "text 2 1 0 0 {311 -2488} {TEXT_ABC}\n"
      "text 2 1 0 0 {322 -2476} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2700} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -2690} {TEXT_ABC}\n"
      "text 2 1 0 0 {280 -2680} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2100} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2900} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2900} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2890} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2890} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2930} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2920} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2910} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -2920} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2940} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3100} {TEXT_ABC}\n"
      "text 2 1 0 0 {289 -3088} {TEXT_ABC}\n"
      "text 2 1 0 0 {299 -3098} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3300} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3300} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3290} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3290} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3330} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3320} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3310} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -3320} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3340} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3500} {TEXT_ABC}\n"
      "text 2 1 0 0 {288 -3488} {TEXT_ABC}\n"
      "text 2 1 0 0 {297 -3497} {TEXT_ABC}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  } else {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {TEXT_ABC}\n"
      "text 2 1 0 0 {200 -400} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -400} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -300} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -500} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -500} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -500} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -488} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -488} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -488} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -476} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -476} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -476} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -464} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -464} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -464} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -700} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -700} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -700} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -688} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -688} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -688} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -676} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -676} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -676} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -664} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -664} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -664} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -900} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -900} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -900} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1100} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1088} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1076} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1064} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {312 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {325 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {339 -1300} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {312 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {327 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {345 -1500} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1700} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1690} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1679} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1900} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1890} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -1875} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2100} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2100} {TEXT_ABC}\n"
      "text 2 1 0 0 {320 -2100} {TEXT_ABC}\n"
      "text 2 1 0 0 {289 -2112} {TEXT_ABC}\n"
      "text 2 1 0 0 {299 -2112} {TEXT_ABC}\n"
      "text 2 1 0 0 {309 -2112} {TEXT_ABC}\n"
      "text 2 1 0 0 {278 -2124} {TEXT_ABC}\n"
      "text 2 1 0 0 {288 -2124} {TEXT_ABC}\n"
      "text 2 1 0 0 {298 -2124} {TEXT_ABC}\n"
      "text 2 1 0 0 {267 -2136} {TEXT_ABC}\n"
      "text 2 1 0 0 {277 -2136} {TEXT_ABC}\n"
      "text 2 1 0 0 {287 -2136} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2300} {TEXT_ABC}\n"
      "text 2 1 0 0 {311 -2288} {TEXT_ABC}\n"
      "text 2 1 0 0 {322 -2276} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -2290} {TEXT_ABC}\n"
      "text 2 1 0 0 {301 -2278} {TEXT_ABC}\n"
      "text 2 1 0 0 {312 -2266} {TEXT_ABC}\n"
      "text 2 1 0 0 {280 -2280} {TEXT_ABC}\n"
      "text 2 1 0 0 {291 -2268} {TEXT_ABC}\n"
      "text 2 1 0 0 {302 -2256} {TEXT_ABC}\n"
      "text 2 1 0 0 {270 -2270} {TEXT_ABC}\n"
      "text 2 1 0 0 {281 -2258} {TEXT_ABC}\n"
      "text 2 1 0 0 {292 -2246} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2500} {TEXT_ABC}\n"
      "text 2 1 0 0 {311 -2488} {TEXT_ABC}\n"
      "text 2 1 0 0 {322 -2476} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2700} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -2690} {TEXT_ABC}\n"
      "text 2 1 0 0 {280 -2680} {TEXT_ABC}\n"
      "text 2 1 0 0 {270 -2670} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2900} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2900} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2890} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2890} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2930} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2920} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -2910} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -2920} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -2940} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3100} {TEXT_ABC}\n"
      "text 2 1 0 0 {289 -3088} {TEXT_ABC}\n"
      "text 2 1 0 0 {299 -3098} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3300} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3300} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3290} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3290} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3330} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3320} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3310} {TEXT_ABC}\n"
      "text 2 1 0 0 {290 -3320} {TEXT_ABC}\n"
      "text 2 1 0 0 {310 -3340} {TEXT_ABC}\n"
      "text 2 1 0 0 {300 -3500} {TEXT_ABC}\n"
      "text 2 1 0 0 {288 -3488} {TEXT_ABC}\n"
      "text 2 1 0 0 {297 -3497} {TEXT_ABC}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  }

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(3_10)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "text 1 2 0 0 {100 0} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.10.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(3_11)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.11.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Modal variable accessed before being defined: text-string (position=50, cell=ABC)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(3_12)
{
  const char *expected;
  if (db::default_editable_mode ()) {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {A}\n"
      "text 2 1 0 0 {300 -400} {B}\n"
      "text 2 1 0 0 {300 -300} {B}\n"
      "text 2 1 0 0 {300 -500} {A}\n"
      "text 2 1 0 0 {310 -500} {A}\n"
      "text 2 1 0 0 {320 -500} {A}\n"
      "text 2 1 0 0 {300 -488} {A}\n"
      "text 2 1 0 0 {310 -488} {A}\n"
      "text 2 1 0 0 {320 -488} {A}\n"
      "text 2 1 0 0 {300 -476} {A}\n"
      "text 2 1 0 0 {310 -476} {A}\n"
      "text 2 1 0 0 {320 -476} {A}\n"
      "text 2 1 0 0 {300 -464} {A}\n"
      "text 2 1 0 0 {310 -464} {A}\n"
      "text 2 1 0 0 {320 -464} {A}\n"
      "text 2 1 0 0 {300 -700} {A}\n"
      "text 2 1 0 0 {310 -700} {A}\n"
      "text 2 1 0 0 {320 -700} {A}\n"
      "text 2 1 0 0 {300 -688} {A}\n"
      "text 2 1 0 0 {310 -688} {A}\n"
      "text 2 1 0 0 {320 -688} {A}\n"
      "text 2 1 0 0 {300 -676} {A}\n"
      "text 2 1 0 0 {310 -676} {A}\n"
      "text 2 1 0 0 {320 -676} {A}\n"
      "text 2 1 0 0 {300 -664} {A}\n"
      "text 2 1 0 0 {310 -664} {A}\n"
      "text 2 1 0 0 {320 -664} {A}\n"
      "text 2 1 0 0 {300 -900} {A}\n"
      "text 2 1 0 0 {310 -900} {A}\n"
      "text 2 1 0 0 {320 -900} {A}\n"
      "text 2 1 0 0 {300 -1100} {A}\n"
      "text 2 1 0 0 {300 -1088} {A}\n"
      "text 2 1 0 0 {300 -1076} {A}\n"
      "text 2 1 0 0 {300 -1064} {A}\n"
      "text 2 1 0 0 {300 -1300} {A}\n"
      "text 2 1 0 0 {312 -1300} {A}\n"
      "text 2 1 0 0 {325 -1300} {A}\n"
      "text 2 1 0 0 {339 -1300} {A}\n"
      "text 2 1 0 0 {300 -1500} {A}\n"
      "text 2 1 0 0 {312 -1500} {A}\n"
      "text 2 1 0 0 {327 -1500} {A}\n"
      "text 2 1 0 0 {345 -1500} {A}\n"
      "text 2 1 0 0 {300 -1700} {A}\n"
      "text 2 1 0 0 {300 -1690} {A}\n"
      "text 2 1 0 0 {300 -1679} {A}\n"
      "text 2 1 0 0 {300 -1890} {A}\n"
      "text 2 1 0 0 {300 -1875} {A}\n"
      "text 2 1 0 0 {200 -400} {B}\n"
      "text 2 1 0 0 {267 -2136} {A}\n"
      "text 2 1 0 0 {270 -2270} {A}\n"
      "text 2 1 0 0 {270 -2670} {A}\n"
      "text 2 1 0 0 {320 -2100} {A}\n"
      "text 2 1 0 0 {289 -2112} {A}\n"
      "text 2 1 0 0 {299 -2112} {A}\n"
      "text 2 1 0 0 {309 -2112} {A}\n"
      "text 2 1 0 0 {278 -2124} {A}\n"
      "text 2 1 0 0 {288 -2124} {A}\n"
      "text 2 1 0 0 {298 -2124} {A}\n"
      "text 2 1 0 0 {300 -1900} {A}\n"
      "text 2 1 0 0 {277 -2136} {A}\n"
      "text 2 1 0 0 {287 -2136} {A}\n"
      "text 2 1 0 0 {300 -2300} {A}\n"
      "text 2 1 0 0 {311 -2288} {A}\n"
      "text 2 1 0 0 {322 -2276} {A}\n"
      "text 2 1 0 0 {290 -2290} {A}\n"
      "text 2 1 0 0 {301 -2278} {A}\n"
      "text 2 1 0 0 {312 -2266} {A}\n"
      "text 2 1 0 0 {280 -2280} {A}\n"
      "text 2 1 0 0 {291 -2268} {A}\n"
      "text 2 1 0 0 {302 -2256} {A}\n"
      "text 2 1 0 0 {300 -2100} {A}\n"
      "text 2 1 0 0 {281 -2258} {A}\n"
      "text 2 1 0 0 {292 -2246} {A}\n"
      "text 2 1 0 0 {300 -2500} {A}\n"
      "text 2 1 0 0 {311 -2488} {A}\n"
      "text 2 1 0 0 {322 -2476} {A}\n"
      "text 2 1 0 0 {300 -2700} {A}\n"
      "text 2 1 0 0 {290 -2690} {A}\n"
      "text 2 1 0 0 {280 -2680} {A}\n"
      "text 2 1 0 0 {310 -2100} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {310 -2900} {A}\n"
      "text 2 1 0 0 {310 -2890} {A}\n"
      "text 2 1 0 0 {300 -2890} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {310 -2890} {A}\n"
      "text 2 1 0 0 {300 -2880} {A}\n"
      "text 2 1 0 0 {290 -2890} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {300 -3100} {A}\n"
      "text 2 1 0 0 {289 -3088} {A}\n"
      "text 2 1 0 0 {299 -3098} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {310 -3300} {A}\n"
      "text 2 1 0 0 {310 -3290} {A}\n"
      "text 2 1 0 0 {300 -3290} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {310 -3290} {A}\n"
      "text 2 1 0 0 {300 -3280} {A}\n"
      "text 2 1 0 0 {290 -3290} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {300 -3500} {A}\n"
      "text 2 1 0 0 {288 -3488} {A}\n"
      "text 2 1 0 0 {297 -3497} {A}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  } else {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {A}\n"
      "text 2 1 0 0 {200 -400} {B}\n"
      "text 2 1 0 0 {300 -400} {B}\n"
      "text 2 1 0 0 {300 -300} {B}\n"
      "text 2 1 0 0 {300 -500} {A}\n"
      "text 2 1 0 0 {310 -500} {A}\n"
      "text 2 1 0 0 {320 -500} {A}\n"
      "text 2 1 0 0 {300 -488} {A}\n"
      "text 2 1 0 0 {310 -488} {A}\n"
      "text 2 1 0 0 {320 -488} {A}\n"
      "text 2 1 0 0 {300 -476} {A}\n"
      "text 2 1 0 0 {310 -476} {A}\n"
      "text 2 1 0 0 {320 -476} {A}\n"
      "text 2 1 0 0 {300 -464} {A}\n"
      "text 2 1 0 0 {310 -464} {A}\n"
      "text 2 1 0 0 {320 -464} {A}\n"
      "text 2 1 0 0 {300 -700} {A}\n"
      "text 2 1 0 0 {310 -700} {A}\n"
      "text 2 1 0 0 {320 -700} {A}\n"
      "text 2 1 0 0 {300 -688} {A}\n"
      "text 2 1 0 0 {310 -688} {A}\n"
      "text 2 1 0 0 {320 -688} {A}\n"
      "text 2 1 0 0 {300 -676} {A}\n"
      "text 2 1 0 0 {310 -676} {A}\n"
      "text 2 1 0 0 {320 -676} {A}\n"
      "text 2 1 0 0 {300 -664} {A}\n"
      "text 2 1 0 0 {310 -664} {A}\n"
      "text 2 1 0 0 {320 -664} {A}\n"
      "text 2 1 0 0 {300 -900} {A}\n"
      "text 2 1 0 0 {310 -900} {A}\n"
      "text 2 1 0 0 {320 -900} {A}\n"
      "text 2 1 0 0 {300 -1100} {A}\n"
      "text 2 1 0 0 {300 -1088} {A}\n"
      "text 2 1 0 0 {300 -1076} {A}\n"
      "text 2 1 0 0 {300 -1064} {A}\n"
      "text 2 1 0 0 {300 -1300} {A}\n"
      "text 2 1 0 0 {312 -1300} {A}\n"
      "text 2 1 0 0 {325 -1300} {A}\n"
      "text 2 1 0 0 {339 -1300} {A}\n"
      "text 2 1 0 0 {300 -1500} {A}\n"
      "text 2 1 0 0 {312 -1500} {A}\n"
      "text 2 1 0 0 {327 -1500} {A}\n"
      "text 2 1 0 0 {345 -1500} {A}\n"
      "text 2 1 0 0 {300 -1700} {A}\n"
      "text 2 1 0 0 {300 -1690} {A}\n"
      "text 2 1 0 0 {300 -1679} {A}\n"
      "text 2 1 0 0 {300 -1900} {A}\n"
      "text 2 1 0 0 {300 -1890} {A}\n"
      "text 2 1 0 0 {300 -1875} {A}\n"
      "text 2 1 0 0 {300 -2100} {A}\n"
      "text 2 1 0 0 {310 -2100} {A}\n"
      "text 2 1 0 0 {320 -2100} {A}\n"
      "text 2 1 0 0 {289 -2112} {A}\n"
      "text 2 1 0 0 {299 -2112} {A}\n"
      "text 2 1 0 0 {309 -2112} {A}\n"
      "text 2 1 0 0 {278 -2124} {A}\n"
      "text 2 1 0 0 {288 -2124} {A}\n"
      "text 2 1 0 0 {298 -2124} {A}\n"
      "text 2 1 0 0 {267 -2136} {A}\n"
      "text 2 1 0 0 {277 -2136} {A}\n"
      "text 2 1 0 0 {287 -2136} {A}\n"
      "text 2 1 0 0 {300 -2300} {A}\n"
      "text 2 1 0 0 {311 -2288} {A}\n"
      "text 2 1 0 0 {322 -2276} {A}\n"
      "text 2 1 0 0 {290 -2290} {A}\n"
      "text 2 1 0 0 {301 -2278} {A}\n"
      "text 2 1 0 0 {312 -2266} {A}\n"
      "text 2 1 0 0 {280 -2280} {A}\n"
      "text 2 1 0 0 {291 -2268} {A}\n"
      "text 2 1 0 0 {302 -2256} {A}\n"
      "text 2 1 0 0 {270 -2270} {A}\n"
      "text 2 1 0 0 {281 -2258} {A}\n"
      "text 2 1 0 0 {292 -2246} {A}\n"
      "text 2 1 0 0 {300 -2500} {A}\n"
      "text 2 1 0 0 {311 -2488} {A}\n"
      "text 2 1 0 0 {322 -2476} {A}\n"
      "text 2 1 0 0 {300 -2700} {A}\n"
      "text 2 1 0 0 {290 -2690} {A}\n"
      "text 2 1 0 0 {280 -2680} {A}\n"
      "text 2 1 0 0 {270 -2670} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {310 -2900} {A}\n"
      "text 2 1 0 0 {310 -2890} {A}\n"
      "text 2 1 0 0 {300 -2890} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {310 -2890} {A}\n"
      "text 2 1 0 0 {300 -2880} {A}\n"
      "text 2 1 0 0 {290 -2890} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {300 -3100} {A}\n"
      "text 2 1 0 0 {289 -3088} {A}\n"
      "text 2 1 0 0 {299 -3098} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {310 -3300} {A}\n"
      "text 2 1 0 0 {310 -3290} {A}\n"
      "text 2 1 0 0 {300 -3290} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {310 -3290} {A}\n"
      "text 2 1 0 0 {300 -3280} {A}\n"
      "text 2 1 0 0 {290 -3290} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {300 -3500} {A}\n"
      "text 2 1 0 0 {288 -3488} {A}\n"
      "text 2 1 0 0 {297 -3497} {A}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  }

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.12.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(3_2)
{
  const char *expected;
  if (db::default_editable_mode ()) {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {A}\n"
      "text 2 1 0 0 {300 -400} {B}\n"
      "text 2 1 0 0 {300 -300} {B}\n"
      "text 2 1 0 0 {300 -500} {A}\n"
      "text 2 1 0 0 {310 -500} {A}\n"
      "text 2 1 0 0 {320 -500} {A}\n"
      "text 2 1 0 0 {300 -488} {A}\n"
      "text 2 1 0 0 {310 -488} {A}\n"
      "text 2 1 0 0 {320 -488} {A}\n"
      "text 2 1 0 0 {300 -476} {A}\n"
      "text 2 1 0 0 {310 -476} {A}\n"
      "text 2 1 0 0 {320 -476} {A}\n"
      "text 2 1 0 0 {300 -464} {A}\n"
      "text 2 1 0 0 {310 -464} {A}\n"
      "text 2 1 0 0 {320 -464} {A}\n"
      "text 2 1 0 0 {300 -700} {A}\n"
      "text 2 1 0 0 {310 -700} {A}\n"
      "text 2 1 0 0 {320 -700} {A}\n"
      "text 2 1 0 0 {300 -688} {A}\n"
      "text 2 1 0 0 {310 -688} {A}\n"
      "text 2 1 0 0 {320 -688} {A}\n"
      "text 2 1 0 0 {300 -676} {A}\n"
      "text 2 1 0 0 {310 -676} {A}\n"
      "text 2 1 0 0 {320 -676} {A}\n"
      "text 2 1 0 0 {300 -664} {A}\n"
      "text 2 1 0 0 {310 -664} {A}\n"
      "text 2 1 0 0 {320 -664} {A}\n"
      "text 2 1 0 0 {300 -900} {A}\n"
      "text 2 1 0 0 {310 -900} {A}\n"
      "text 2 1 0 0 {320 -900} {A}\n"
      "text 2 1 0 0 {300 -1100} {A}\n"
      "text 2 1 0 0 {300 -1088} {A}\n"
      "text 2 1 0 0 {300 -1076} {A}\n"
      "text 2 1 0 0 {300 -1064} {A}\n"
      "text 2 1 0 0 {300 -1300} {A}\n"
      "text 2 1 0 0 {312 -1300} {A}\n"
      "text 2 1 0 0 {325 -1300} {A}\n"
      "text 2 1 0 0 {339 -1300} {A}\n"
      "text 2 1 0 0 {300 -1500} {A}\n"
      "text 2 1 0 0 {312 -1500} {A}\n"
      "text 2 1 0 0 {327 -1500} {A}\n"
      "text 2 1 0 0 {345 -1500} {A}\n"
      "text 2 1 0 0 {300 -1700} {A}\n"
      "text 2 1 0 0 {300 -1690} {A}\n"
      "text 2 1 0 0 {300 -1679} {A}\n"
      "text 2 1 0 0 {300 -1890} {A}\n"
      "text 2 1 0 0 {300 -1875} {A}\n"
      "text 2 1 0 0 {200 -400} {B}\n"
      "text 2 1 0 0 {267 -2136} {A}\n"
      "text 2 1 0 0 {270 -2270} {A}\n"
      "text 2 1 0 0 {270 -2670} {A}\n"
      "text 2 1 0 0 {320 -2100} {A}\n"
      "text 2 1 0 0 {289 -2112} {A}\n"
      "text 2 1 0 0 {299 -2112} {A}\n"
      "text 2 1 0 0 {309 -2112} {A}\n"
      "text 2 1 0 0 {278 -2124} {A}\n"
      "text 2 1 0 0 {288 -2124} {A}\n"
      "text 2 1 0 0 {298 -2124} {A}\n"
      "text 2 1 0 0 {300 -1900} {A}\n"
      "text 2 1 0 0 {277 -2136} {A}\n"
      "text 2 1 0 0 {287 -2136} {A}\n"
      "text 2 1 0 0 {300 -2300} {A}\n"
      "text 2 1 0 0 {311 -2288} {A}\n"
      "text 2 1 0 0 {322 -2276} {A}\n"
      "text 2 1 0 0 {290 -2290} {A}\n"
      "text 2 1 0 0 {301 -2278} {A}\n"
      "text 2 1 0 0 {312 -2266} {A}\n"
      "text 2 1 0 0 {280 -2280} {A}\n"
      "text 2 1 0 0 {291 -2268} {A}\n"
      "text 2 1 0 0 {302 -2256} {A}\n"
      "text 2 1 0 0 {300 -2100} {A}\n"
      "text 2 1 0 0 {281 -2258} {A}\n"
      "text 2 1 0 0 {292 -2246} {A}\n"
      "text 2 1 0 0 {300 -2500} {A}\n"
      "text 2 1 0 0 {311 -2488} {A}\n"
      "text 2 1 0 0 {322 -2476} {A}\n"
      "text 2 1 0 0 {300 -2700} {A}\n"
      "text 2 1 0 0 {290 -2690} {A}\n"
      "text 2 1 0 0 {280 -2680} {A}\n"
      "text 2 1 0 0 {310 -2100} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {310 -2900} {A}\n"
      "text 2 1 0 0 {310 -2890} {A}\n"
      "text 2 1 0 0 {300 -2890} {A}\n"
      "text 2 1 0 0 {300 -2930} {A}\n"
      "text 2 1 0 0 {310 -2920} {A}\n"
      "text 2 1 0 0 {300 -2910} {A}\n"
      "text 2 1 0 0 {290 -2920} {A}\n"
      "text 2 1 0 0 {310 -2940} {A}\n"
      "text 2 1 0 0 {300 -3100} {A}\n"
      "text 2 1 0 0 {289 -3088} {A}\n"
      "text 2 1 0 0 {299 -3098} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {310 -3300} {A}\n"
      "text 2 1 0 0 {310 -3290} {A}\n"
      "text 2 1 0 0 {300 -3290} {A}\n"
      "text 2 1 0 0 {300 -3330} {A}\n"
      "text 2 1 0 0 {310 -3320} {A}\n"
      "text 2 1 0 0 {300 -3310} {A}\n"
      "text 2 1 0 0 {290 -3320} {A}\n"
      "text 2 1 0 0 {310 -3340} {A}\n"
      "text 2 1 0 0 {300 -3500} {A}\n"
      "text 2 1 0 0 {288 -3488} {A}\n"
      "text 2 1 0 0 {297 -3497} {A}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  } else {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {A}\n"
      "text 2 1 0 0 {200 -400} {B}\n"
      "text 2 1 0 0 {300 -400} {B}\n"
      "text 2 1 0 0 {300 -300} {B}\n"
      "text 2 1 0 0 {300 -500} {A}\n"
      "text 2 1 0 0 {310 -500} {A}\n"
      "text 2 1 0 0 {320 -500} {A}\n"
      "text 2 1 0 0 {300 -488} {A}\n"
      "text 2 1 0 0 {310 -488} {A}\n"
      "text 2 1 0 0 {320 -488} {A}\n"
      "text 2 1 0 0 {300 -476} {A}\n"
      "text 2 1 0 0 {310 -476} {A}\n"
      "text 2 1 0 0 {320 -476} {A}\n"
      "text 2 1 0 0 {300 -464} {A}\n"
      "text 2 1 0 0 {310 -464} {A}\n"
      "text 2 1 0 0 {320 -464} {A}\n"
      "text 2 1 0 0 {300 -700} {A}\n"
      "text 2 1 0 0 {310 -700} {A}\n"
      "text 2 1 0 0 {320 -700} {A}\n"
      "text 2 1 0 0 {300 -688} {A}\n"
      "text 2 1 0 0 {310 -688} {A}\n"
      "text 2 1 0 0 {320 -688} {A}\n"
      "text 2 1 0 0 {300 -676} {A}\n"
      "text 2 1 0 0 {310 -676} {A}\n"
      "text 2 1 0 0 {320 -676} {A}\n"
      "text 2 1 0 0 {300 -664} {A}\n"
      "text 2 1 0 0 {310 -664} {A}\n"
      "text 2 1 0 0 {320 -664} {A}\n"
      "text 2 1 0 0 {300 -900} {A}\n"
      "text 2 1 0 0 {310 -900} {A}\n"
      "text 2 1 0 0 {320 -900} {A}\n"
      "text 2 1 0 0 {300 -1100} {A}\n"
      "text 2 1 0 0 {300 -1088} {A}\n"
      "text 2 1 0 0 {300 -1076} {A}\n"
      "text 2 1 0 0 {300 -1064} {A}\n"
      "text 2 1 0 0 {300 -1300} {A}\n"
      "text 2 1 0 0 {312 -1300} {A}\n"
      "text 2 1 0 0 {325 -1300} {A}\n"
      "text 2 1 0 0 {339 -1300} {A}\n"
      "text 2 1 0 0 {300 -1500} {A}\n"
      "text 2 1 0 0 {312 -1500} {A}\n"
      "text 2 1 0 0 {327 -1500} {A}\n"
      "text 2 1 0 0 {345 -1500} {A}\n"
      "text 2 1 0 0 {300 -1700} {A}\n"
      "text 2 1 0 0 {300 -1690} {A}\n"
      "text 2 1 0 0 {300 -1679} {A}\n"
      "text 2 1 0 0 {300 -1900} {A}\n"
      "text 2 1 0 0 {300 -1890} {A}\n"
      "text 2 1 0 0 {300 -1875} {A}\n"
      "text 2 1 0 0 {300 -2100} {A}\n"
      "text 2 1 0 0 {310 -2100} {A}\n"
      "text 2 1 0 0 {320 -2100} {A}\n"
      "text 2 1 0 0 {289 -2112} {A}\n"
      "text 2 1 0 0 {299 -2112} {A}\n"
      "text 2 1 0 0 {309 -2112} {A}\n"
      "text 2 1 0 0 {278 -2124} {A}\n"
      "text 2 1 0 0 {288 -2124} {A}\n"
      "text 2 1 0 0 {298 -2124} {A}\n"
      "text 2 1 0 0 {267 -2136} {A}\n"
      "text 2 1 0 0 {277 -2136} {A}\n"
      "text 2 1 0 0 {287 -2136} {A}\n"
      "text 2 1 0 0 {300 -2300} {A}\n"
      "text 2 1 0 0 {311 -2288} {A}\n"
      "text 2 1 0 0 {322 -2276} {A}\n"
      "text 2 1 0 0 {290 -2290} {A}\n"
      "text 2 1 0 0 {301 -2278} {A}\n"
      "text 2 1 0 0 {312 -2266} {A}\n"
      "text 2 1 0 0 {280 -2280} {A}\n"
      "text 2 1 0 0 {291 -2268} {A}\n"
      "text 2 1 0 0 {302 -2256} {A}\n"
      "text 2 1 0 0 {270 -2270} {A}\n"
      "text 2 1 0 0 {281 -2258} {A}\n"
      "text 2 1 0 0 {292 -2246} {A}\n"
      "text 2 1 0 0 {300 -2500} {A}\n"
      "text 2 1 0 0 {311 -2488} {A}\n"
      "text 2 1 0 0 {322 -2476} {A}\n"
      "text 2 1 0 0 {300 -2700} {A}\n"
      "text 2 1 0 0 {290 -2690} {A}\n"
      "text 2 1 0 0 {280 -2680} {A}\n"
      "text 2 1 0 0 {270 -2670} {A}\n"
      "text 2 1 0 0 {300 -2900} {A}\n"
      "text 2 1 0 0 {310 -2900} {A}\n"
      "text 2 1 0 0 {310 -2890} {A}\n"
      "text 2 1 0 0 {300 -2890} {A}\n"
      "text 2 1 0 0 {300 -2930} {A}\n"
      "text 2 1 0 0 {310 -2920} {A}\n"
      "text 2 1 0 0 {300 -2910} {A}\n"
      "text 2 1 0 0 {290 -2920} {A}\n"
      "text 2 1 0 0 {310 -2940} {A}\n"
      "text 2 1 0 0 {300 -3100} {A}\n"
      "text 2 1 0 0 {289 -3088} {A}\n"
      "text 2 1 0 0 {299 -3098} {A}\n"
      "text 2 1 0 0 {300 -3300} {A}\n"
      "text 2 1 0 0 {310 -3300} {A}\n"
      "text 2 1 0 0 {310 -3290} {A}\n"
      "text 2 1 0 0 {300 -3290} {A}\n"
      "text 2 1 0 0 {300 -3330} {A}\n"
      "text 2 1 0 0 {310 -3320} {A}\n"
      "text 2 1 0 0 {300 -3310} {A}\n"
      "text 2 1 0 0 {290 -3320} {A}\n"
      "text 2 1 0 0 {310 -3340} {A}\n"
      "text 2 1 0 0 {300 -3500} {A}\n"
      "text 2 1 0 0 {288 -3488} {A}\n"
      "text 2 1 0 0 {297 -3497} {A}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  }

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(3_3)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.3.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Explicit and implicit TEXTSTRING modes cannot be mixed (position=41, cell=)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(3_4)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.4.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: No text string defined for id 2 (must be declared before text is used) (position=48, cell=ABC)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(3_5)
{
  const char *expected;
  if (db::default_editable_mode ()) {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {B}\n"
      "text 2 1 0 0 {300 -400} {A}\n"
      "text 2 1 0 0 {300 -300} {A}\n"
      "text 2 1 0 0 {300 -500} {B}\n"
      "text 2 1 0 0 {310 -500} {B}\n"
      "text 2 1 0 0 {320 -500} {B}\n"
      "text 2 1 0 0 {300 -488} {B}\n"
      "text 2 1 0 0 {310 -488} {B}\n"
      "text 2 1 0 0 {320 -488} {B}\n"
      "text 2 1 0 0 {300 -476} {B}\n"
      "text 2 1 0 0 {310 -476} {B}\n"
      "text 2 1 0 0 {320 -476} {B}\n"
      "text 2 1 0 0 {300 -464} {B}\n"
      "text 2 1 0 0 {310 -464} {B}\n"
      "text 2 1 0 0 {320 -464} {B}\n"
      "text 2 1 0 0 {300 -700} {B}\n"
      "text 2 1 0 0 {310 -700} {B}\n"
      "text 2 1 0 0 {320 -700} {B}\n"
      "text 2 1 0 0 {300 -688} {B}\n"
      "text 2 1 0 0 {310 -688} {B}\n"
      "text 2 1 0 0 {320 -688} {B}\n"
      "text 2 1 0 0 {300 -676} {B}\n"
      "text 2 1 0 0 {310 -676} {B}\n"
      "text 2 1 0 0 {320 -676} {B}\n"
      "text 2 1 0 0 {300 -664} {B}\n"
      "text 2 1 0 0 {310 -664} {B}\n"
      "text 2 1 0 0 {320 -664} {B}\n"
      "text 2 1 0 0 {300 -900} {B}\n"
      "text 2 1 0 0 {310 -900} {B}\n"
      "text 2 1 0 0 {320 -900} {B}\n"
      "text 2 1 0 0 {300 -1100} {B}\n"
      "text 2 1 0 0 {300 -1088} {B}\n"
      "text 2 1 0 0 {300 -1076} {B}\n"
      "text 2 1 0 0 {300 -1064} {B}\n"
      "text 2 1 0 0 {300 -1300} {B}\n"
      "text 2 1 0 0 {312 -1300} {B}\n"
      "text 2 1 0 0 {325 -1300} {B}\n"
      "text 2 1 0 0 {339 -1300} {B}\n"
      "text 2 1 0 0 {300 -1500} {B}\n"
      "text 2 1 0 0 {312 -1500} {B}\n"
      "text 2 1 0 0 {327 -1500} {B}\n"
      "text 2 1 0 0 {345 -1500} {B}\n"
      "text 2 1 0 0 {300 -1700} {B}\n"
      "text 2 1 0 0 {300 -1690} {B}\n"
      "text 2 1 0 0 {300 -1679} {B}\n"
      "text 2 1 0 0 {300 -1890} {B}\n"
      "text 2 1 0 0 {300 -1875} {B}\n"
      "text 2 1 0 0 {200 -400} {A}\n"
      "text 2 1 0 0 {267 -2136} {B}\n"
      "text 2 1 0 0 {270 -2270} {B}\n"
      "text 2 1 0 0 {270 -2670} {B}\n"
      "text 2 1 0 0 {320 -2100} {B}\n"
      "text 2 1 0 0 {289 -2112} {B}\n"
      "text 2 1 0 0 {299 -2112} {B}\n"
      "text 2 1 0 0 {309 -2112} {B}\n"
      "text 2 1 0 0 {278 -2124} {B}\n"
      "text 2 1 0 0 {288 -2124} {B}\n"
      "text 2 1 0 0 {298 -2124} {B}\n"
      "text 2 1 0 0 {300 -1900} {B}\n"
      "text 2 1 0 0 {277 -2136} {B}\n"
      "text 2 1 0 0 {287 -2136} {B}\n"
      "text 2 1 0 0 {300 -2300} {B}\n"
      "text 2 1 0 0 {311 -2288} {B}\n"
      "text 2 1 0 0 {322 -2276} {B}\n"
      "text 2 1 0 0 {290 -2290} {B}\n"
      "text 2 1 0 0 {301 -2278} {B}\n"
      "text 2 1 0 0 {312 -2266} {B}\n"
      "text 2 1 0 0 {280 -2280} {B}\n"
      "text 2 1 0 0 {291 -2268} {B}\n"
      "text 2 1 0 0 {302 -2256} {B}\n"
      "text 2 1 0 0 {300 -2100} {B}\n"
      "text 2 1 0 0 {281 -2258} {B}\n"
      "text 2 1 0 0 {292 -2246} {B}\n"
      "text 2 1 0 0 {300 -2500} {B}\n"
      "text 2 1 0 0 {311 -2488} {B}\n"
      "text 2 1 0 0 {322 -2476} {B}\n"
      "text 2 1 0 0 {300 -2700} {B}\n"
      "text 2 1 0 0 {290 -2690} {B}\n"
      "text 2 1 0 0 {280 -2680} {B}\n"
      "text 2 1 0 0 {310 -2100} {B}\n"
      "text 2 1 0 0 {300 -2900} {B}\n"
      "text 2 1 0 0 {310 -2900} {B}\n"
      "text 2 1 0 0 {310 -2890} {B}\n"
      "text 2 1 0 0 {300 -2890} {B}\n"
      "text 2 1 0 0 {300 -2930} {B}\n"
      "text 2 1 0 0 {310 -2920} {B}\n"
      "text 2 1 0 0 {300 -2910} {B}\n"
      "text 2 1 0 0 {290 -2920} {B}\n"
      "text 2 1 0 0 {310 -2940} {B}\n"
      "text 2 1 0 0 {300 -3100} {B}\n"
      "text 2 1 0 0 {289 -3088} {B}\n"
      "text 2 1 0 0 {299 -3098} {B}\n"
      "text 2 1 0 0 {300 -3300} {B}\n"
      "text 2 1 0 0 {310 -3300} {B}\n"
      "text 2 1 0 0 {310 -3290} {B}\n"
      "text 2 1 0 0 {300 -3290} {B}\n"
      "text 2 1 0 0 {300 -3330} {B}\n"
      "text 2 1 0 0 {310 -3320} {B}\n"
      "text 2 1 0 0 {300 -3310} {B}\n"
      "text 2 1 0 0 {290 -3320} {B}\n"
      "text 2 1 0 0 {310 -3340} {B}\n"
      "text 2 1 0 0 {300 -3500} {B}\n"
      "text 2 1 0 0 {288 -3488} {B}\n"
      "text 2 1 0 0 {297 -3497} {B}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  } else {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "text 1 2 0 0 {100 -200} {B}\n"
      "text 2 1 0 0 {200 -400} {A}\n"
      "text 2 1 0 0 {300 -400} {A}\n"
      "text 2 1 0 0 {300 -300} {A}\n"
      "text 2 1 0 0 {300 -500} {B}\n"
      "text 2 1 0 0 {310 -500} {B}\n"
      "text 2 1 0 0 {320 -500} {B}\n"
      "text 2 1 0 0 {300 -488} {B}\n"
      "text 2 1 0 0 {310 -488} {B}\n"
      "text 2 1 0 0 {320 -488} {B}\n"
      "text 2 1 0 0 {300 -476} {B}\n"
      "text 2 1 0 0 {310 -476} {B}\n"
      "text 2 1 0 0 {320 -476} {B}\n"
      "text 2 1 0 0 {300 -464} {B}\n"
      "text 2 1 0 0 {310 -464} {B}\n"
      "text 2 1 0 0 {320 -464} {B}\n"
      "text 2 1 0 0 {300 -700} {B}\n"
      "text 2 1 0 0 {310 -700} {B}\n"
      "text 2 1 0 0 {320 -700} {B}\n"
      "text 2 1 0 0 {300 -688} {B}\n"
      "text 2 1 0 0 {310 -688} {B}\n"
      "text 2 1 0 0 {320 -688} {B}\n"
      "text 2 1 0 0 {300 -676} {B}\n"
      "text 2 1 0 0 {310 -676} {B}\n"
      "text 2 1 0 0 {320 -676} {B}\n"
      "text 2 1 0 0 {300 -664} {B}\n"
      "text 2 1 0 0 {310 -664} {B}\n"
      "text 2 1 0 0 {320 -664} {B}\n"
      "text 2 1 0 0 {300 -900} {B}\n"
      "text 2 1 0 0 {310 -900} {B}\n"
      "text 2 1 0 0 {320 -900} {B}\n"
      "text 2 1 0 0 {300 -1100} {B}\n"
      "text 2 1 0 0 {300 -1088} {B}\n"
      "text 2 1 0 0 {300 -1076} {B}\n"
      "text 2 1 0 0 {300 -1064} {B}\n"
      "text 2 1 0 0 {300 -1300} {B}\n"
      "text 2 1 0 0 {312 -1300} {B}\n"
      "text 2 1 0 0 {325 -1300} {B}\n"
      "text 2 1 0 0 {339 -1300} {B}\n"
      "text 2 1 0 0 {300 -1500} {B}\n"
      "text 2 1 0 0 {312 -1500} {B}\n"
      "text 2 1 0 0 {327 -1500} {B}\n"
      "text 2 1 0 0 {345 -1500} {B}\n"
      "text 2 1 0 0 {300 -1700} {B}\n"
      "text 2 1 0 0 {300 -1690} {B}\n"
      "text 2 1 0 0 {300 -1679} {B}\n"
      "text 2 1 0 0 {300 -1900} {B}\n"
      "text 2 1 0 0 {300 -1890} {B}\n"
      "text 2 1 0 0 {300 -1875} {B}\n"
      "text 2 1 0 0 {300 -2100} {B}\n"
      "text 2 1 0 0 {310 -2100} {B}\n"
      "text 2 1 0 0 {320 -2100} {B}\n"
      "text 2 1 0 0 {289 -2112} {B}\n"
      "text 2 1 0 0 {299 -2112} {B}\n"
      "text 2 1 0 0 {309 -2112} {B}\n"
      "text 2 1 0 0 {278 -2124} {B}\n"
      "text 2 1 0 0 {288 -2124} {B}\n"
      "text 2 1 0 0 {298 -2124} {B}\n"
      "text 2 1 0 0 {267 -2136} {B}\n"
      "text 2 1 0 0 {277 -2136} {B}\n"
      "text 2 1 0 0 {287 -2136} {B}\n"
      "text 2 1 0 0 {300 -2300} {B}\n"
      "text 2 1 0 0 {311 -2288} {B}\n"
      "text 2 1 0 0 {322 -2276} {B}\n"
      "text 2 1 0 0 {290 -2290} {B}\n"
      "text 2 1 0 0 {301 -2278} {B}\n"
      "text 2 1 0 0 {312 -2266} {B}\n"
      "text 2 1 0 0 {280 -2280} {B}\n"
      "text 2 1 0 0 {291 -2268} {B}\n"
      "text 2 1 0 0 {302 -2256} {B}\n"
      "text 2 1 0 0 {270 -2270} {B}\n"
      "text 2 1 0 0 {281 -2258} {B}\n"
      "text 2 1 0 0 {292 -2246} {B}\n"
      "text 2 1 0 0 {300 -2500} {B}\n"
      "text 2 1 0 0 {311 -2488} {B}\n"
      "text 2 1 0 0 {322 -2476} {B}\n"
      "text 2 1 0 0 {300 -2700} {B}\n"
      "text 2 1 0 0 {290 -2690} {B}\n"
      "text 2 1 0 0 {280 -2680} {B}\n"
      "text 2 1 0 0 {270 -2670} {B}\n"
      "text 2 1 0 0 {300 -2900} {B}\n"
      "text 2 1 0 0 {310 -2900} {B}\n"
      "text 2 1 0 0 {310 -2890} {B}\n"
      "text 2 1 0 0 {300 -2890} {B}\n"
      "text 2 1 0 0 {300 -2930} {B}\n"
      "text 2 1 0 0 {310 -2920} {B}\n"
      "text 2 1 0 0 {300 -2910} {B}\n"
      "text 2 1 0 0 {290 -2920} {B}\n"
      "text 2 1 0 0 {310 -2940} {B}\n"
      "text 2 1 0 0 {300 -3100} {B}\n"
      "text 2 1 0 0 {289 -3088} {B}\n"
      "text 2 1 0 0 {299 -3098} {B}\n"
      "text 2 1 0 0 {300 -3300} {B}\n"
      "text 2 1 0 0 {310 -3300} {B}\n"
      "text 2 1 0 0 {310 -3290} {B}\n"
      "text 2 1 0 0 {300 -3290} {B}\n"
      "text 2 1 0 0 {300 -3330} {B}\n"
      "text 2 1 0 0 {310 -3320} {B}\n"
      "text 2 1 0 0 {300 -3310} {B}\n"
      "text 2 1 0 0 {290 -3320} {B}\n"
      "text 2 1 0 0 {310 -3340} {B}\n"
      "text 2 1 0 0 {300 -3500} {B}\n"
      "text 2 1 0 0 {288 -3488} {B}\n"
      "text 2 1 0 0 {297 -3497} {B}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  }

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.5.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(3_6)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.6.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Modal variable accessed before being defined: repetition (position=52, cell=ABC)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(3_7)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.7.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Modal variable accessed before being defined: textlayer (position=50, cell=ABC)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(3_8)
{
  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.8.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    /*
      *** ERROR: Modal variable accessed before being defined: texttype (position=50, cell=ABC)
    */
    error = true;
  }
  EXPECT_EQ (error, true)
}

TEST(3_9)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "text 1 2 0 0 {0 -200} {A}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t3.9.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(4_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "box 1 2 {400 -500} {500 -300}\n"
    "box 1 2 {600 -300} {700 -100}\n"
    "box 1 2 {800 -300} {900 -100}\n"
    "box 2 3 {800 -600} {900 -400}\n"
    "box 2 3 {800 -900} {900 -700}\n"
    "box 2 3 {800 -1200} {900 -1000}\n"
    "box 2 3 {800 -1500} {950 -1350}\n"
    "box 2 3 {800 -1800} {950 -1650}\n"
    "box 2 3 {800 500} {950 650}\n"
    "box 2 3 {1000 500} {1150 650}\n"
    "box 2 3 {1200 500} {1350 650}\n"
    "box 2 3 {800 800} {950 950}\n"
    "box 2 3 {1000 800} {1150 950}\n"
    "box 2 3 {1200 800} {1350 950}\n"
    "box 2 3 {800 1100} {950 1250}\n"
    "box 2 3 {1000 1100} {1150 1250}\n"
    "box 2 3 {1200 1100} {1350 1250}\n"
    "box 2 3 {800 1400} {950 1550}\n"
    "box 2 3 {1000 1400} {1150 1550}\n"
    "box 2 3 {1200 1400} {1350 1550}\n"
    "box 2 3 {800 2000} {950 2150}\n"
    "box 2 3 {1000 2000} {1150 2150}\n"
    "box 2 3 {1300 2000} {1450 2150}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t4.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(4_2)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 1 2 {300 -400} {400 -200}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 1 2 {400 -500} {500 -300}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 1 2 {600 -300} {700 -100}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 1 2 {800 -300} {900 -100}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 -600} {900 -400}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 -900} {900 -700}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 -1200} {900 -1000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 -1500} {950 -1350}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 -1800} {950 -1650}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 500} {950 650}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1000 500} {1150 650}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1200 500} {1350 650}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 800} {950 950}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1000 800} {1150 950}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1200 800} {1350 950}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 1100} {950 1250}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1000 1100} {1150 1250}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1200 1100} {1350 1250}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 1400} {950 1550}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1000 1400} {1150 1550}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1200 1400} {1350 1550}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {800 2000} {950 2150}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1000 2000} {1150 2150}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boxp $props 2 3 {1300 2000} {1450 2150}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t4.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(5_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "boundary 1 2 {0 100} {0 200} {100 200} {100 150} {150 150} {150 100} {0 100}\n"
    "boundary 1 2 {-200 400} {-200 500} {-100 500} {-100 450} {-50 450} {-50 400} {-200 400}\n"
    "boundary 2 3 {0 400} {0 500} {100 500} {100 450} {150 450} {150 400} {0 400}\n"
    "boundary 2 3 {0 1000} {0 1100} {100 1100} {100 1050} {150 1050} {150 1000} {0 1000}\n"
    "boundary 2 3 {200 1000} {200 1150} {250 1150} {250 1100} {300 1100} {300 1000} {200 1000}\n"
    "boundary 2 3 {400 1000} {400 1050} {450 1050} {450 1100} {500 1100} {500 1050} {550 1050} {550 1000} {400 1000}\n"
    "boundary 2 3 {675 1000} {625 1050} {625 1100} {675 1150} {725 1150} {775 1100} {775 1050} {725 1000} {700 1000} {675 1000}\n"
    "boundary 2 3 {860 1000} {835 1025} {825 1100} {875 1150} {925 1150} {975 1100} {975 1050} {925 1000} {900 1000} {860 1000}\n"
    "boundary 2 3 {1100 1000} {1095 1575} {1135 1575} {1200 1550} {1275 1450} {1300 1300} {1275 1150} {1200 1050} {1125 1000} {1100 1000}\n"
    "boundary 2 3 {0 2000} {0 2150} {50 2150} {50 2100} {100 2100} {100 2000} {0 2000}\n"
    "boundary 2 3 {200 2000} {200 2150} {250 2150} {250 2100} {300 2100} {300 2000} {200 2000}\n"
    "boundary 2 3 {400 2000} {400 2150} {450 2150} {450 2100} {500 2100} {500 2000} {400 2000}\n"
    "boundary 2 3 {0 2300} {0 2450} {50 2450} {50 2400} {100 2400} {100 2300} {0 2300}\n"
    "boundary 2 3 {200 2300} {200 2450} {250 2450} {250 2400} {300 2400} {300 2300} {200 2300}\n"
    "boundary 2 3 {400 2300} {400 2450} {450 2450} {450 2400} {500 2400} {500 2300} {400 2300}\n"
    "boundary 2 3 {0 2600} {0 2750} {50 2750} {50 2700} {100 2700} {100 2600} {0 2600}\n"
    "boundary 2 3 {200 2600} {200 2750} {250 2750} {250 2700} {300 2700} {300 2600} {200 2600}\n"
    "boundary 2 3 {400 2600} {400 2750} {450 2750} {450 2700} {500 2700} {500 2600} {400 2600}\n"
    "boundary 2 3 {0 2900} {0 3050} {50 3050} {50 3000} {100 3000} {100 2900} {0 2900}\n"
    "boundary 2 3 {200 2900} {200 3050} {250 3050} {250 3000} {300 3000} {300 2900} {200 2900}\n"
    "boundary 2 3 {400 2900} {400 3050} {450 3050} {450 3000} {500 3000} {500 2900} {400 2900}\n"
    "boundary 2 1 {1000 2000} {1000 2150} {1050 2150} {1050 2100} {1100 2100} {1100 2000} {1000 2000}\n"
    "boundary 2 1 {1200 2000} {1200 2150} {1250 2150} {1250 2100} {1300 2100} {1300 2000} {1200 2000}\n"
    "boundary 2 1 {1400 2000} {1400 2150} {1450 2150} {1450 2100} {1500 2100} {1500 2000} {1400 2000}\n"
    "boundary 2 1 {1000 2300} {1000 2450} {1050 2450} {1050 2400} {1100 2400} {1100 2300} {1000 2300}\n"
    "boundary 2 1 {1200 2300} {1200 2450} {1250 2450} {1250 2400} {1300 2400} {1300 2300} {1200 2300}\n"
    "boundary 2 1 {1400 2300} {1400 2450} {1450 2450} {1450 2400} {1500 2400} {1500 2300} {1400 2300}\n"
    "boundary 2 1 {1000 2600} {1000 2750} {1050 2750} {1050 2700} {1100 2700} {1100 2600} {1000 2600}\n"
    "boundary 2 1 {1200 2600} {1200 2750} {1250 2750} {1250 2700} {1300 2700} {1300 2600} {1200 2600}\n"
    "boundary 2 1 {1400 2600} {1400 2750} {1450 2750} {1450 2700} {1500 2700} {1500 2600} {1400 2600}\n"
    "boundary 2 1 {1000 2900} {1000 3050} {1050 3050} {1050 3000} {1100 3000} {1100 2900} {1000 2900}\n"
    "boundary 2 1 {1200 2900} {1200 3050} {1250 3050} {1250 3000} {1300 3000} {1300 2900} {1200 2900}\n"
    "boundary 2 1 {1400 2900} {1400 3050} {1450 3050} {1450 3000} {1500 3000} {1500 2900} {1400 2900}\n"
    "boundary 2 1 {2000 2000} {2000 2150} {2050 2150} {2050 2100} {2100 2100} {2100 2000} {2000 2000}\n"
    "boundary 2 1 {2000 2200} {2000 2350} {2050 2350} {2050 2300} {2100 2300} {2100 2200} {2000 2200}\n"
    "boundary 2 1 {2000 2500} {2000 2650} {2050 2650} {2050 2600} {2100 2600} {2100 2500} {2000 2500}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t5.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(5_2)
{
  std::string expected;
  tl::InputStream is (tl::testsrc () + "/testdata/oasis/dbOASISReader_5_2_au.txt");
  const char *cp = 0;
  while ((cp = is.get (1)) != 0) {
    expected += *cp;
  }

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t5.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), expected)
}

TEST(5_3)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 1 2 {0 100} {0 200} {100 200} {100 150} {150 150} {150 100} {0 100}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 1 2 {-200 400} {-200 500} {-100 500} {-100 450} {-50 450} {-50 400} {-200 400}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {0 400} {0 500} {100 500} {100 450} {150 450} {150 400} {0 400}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {0 1000} {0 1100} {100 1100} {100 1050} {150 1050} {150 1000} {0 1000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {200 1000} {200 1150} {250 1150} {250 1100} {300 1100} {300 1000} {200 1000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {400 1000} {400 1050} {450 1050} {450 1100} {500 1100} {500 1050} {550 1050} {550 1000} {400 1000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {675 1000} {625 1050} {625 1100} {675 1150} {725 1150} {775 1100} {775 1050} {725 1000} {700 1000} {675 1000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {860 1000} {835 1025} {825 1100} {875 1150} {925 1150} {975 1100} {975 1050} {925 1000} {900 1000} {860 1000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {1100 1000} {1095 1575} {1135 1575} {1200 1550} {1275 1450} {1300 1300} {1275 1150} {1200 1050} {1125 1000} {1100 1000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {0 2000} {0 2150} {50 2150} {50 2100} {100 2100} {100 2000} {0 2000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {200 2000} {200 2150} {250 2150} {250 2100} {300 2100} {300 2000} {200 2000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {400 2000} {400 2150} {450 2150} {450 2100} {500 2100} {500 2000} {400 2000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {0 2300} {0 2450} {50 2450} {50 2400} {100 2400} {100 2300} {0 2300}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {200 2300} {200 2450} {250 2450} {250 2400} {300 2400} {300 2300} {200 2300}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {400 2300} {400 2450} {450 2450} {450 2400} {500 2400} {500 2300} {400 2300}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {0 2600} {0 2750} {50 2750} {50 2700} {100 2700} {100 2600} {0 2600}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {200 2600} {200 2750} {250 2750} {250 2700} {300 2700} {300 2600} {200 2600}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {400 2600} {400 2750} {450 2750} {450 2700} {500 2700} {500 2600} {400 2600}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {0 2900} {0 3050} {50 3050} {50 3000} {100 3000} {100 2900} {0 2900}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {200 2900} {200 3050} {250 3050} {250 3000} {300 3000} {300 2900} {200 2900}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 3 {400 2900} {400 3050} {450 3050} {450 3000} {500 3000} {500 2900} {400 2900}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1000 2000} {1000 2150} {1050 2150} {1050 2100} {1100 2100} {1100 2000} {1000 2000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1200 2000} {1200 2150} {1250 2150} {1250 2100} {1300 2100} {1300 2000} {1200 2000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1400 2000} {1400 2150} {1450 2150} {1450 2100} {1500 2100} {1500 2000} {1400 2000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1000 2300} {1000 2450} {1050 2450} {1050 2400} {1100 2400} {1100 2300} {1000 2300}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1200 2300} {1200 2450} {1250 2450} {1250 2400} {1300 2400} {1300 2300} {1200 2300}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1400 2300} {1400 2450} {1450 2450} {1450 2400} {1500 2400} {1500 2300} {1400 2300}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1000 2600} {1000 2750} {1050 2750} {1050 2700} {1100 2700} {1100 2600} {1000 2600}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1200 2600} {1200 2750} {1250 2750} {1250 2700} {1300 2700} {1300 2600} {1200 2600}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1400 2600} {1400 2750} {1450 2750} {1450 2700} {1500 2700} {1500 2600} {1400 2600}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1000 2900} {1000 3050} {1050 3050} {1050 3000} {1100 3000} {1100 2900} {1000 2900}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1200 2900} {1200 3050} {1250 3050} {1250 3000} {1300 3000} {1300 2900} {1200 2900}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {1400 2900} {1400 3050} {1450 3050} {1450 3000} {1500 3000} {1500 2900} {1400 2900}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {2000 2000} {2000 2150} {2050 2150} {2050 2100} {2100 2100} {2100 2000} {2000 2000}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {2000 2200} {2000 2350} {2050 2350} {2050 2300} {2100 2300} {2100 2200} {2000 2200}\n"
    "set props {\n"
    "  {{PROP0} {0.2}}\n"
    "}\n"
    "boundaryp $props 2 1 {2000 2500} {2000 2650} {2050 2650} {2050 2600} {2100 2600} {2100 2500} {2000 2500}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t5.3.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(6_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {ABC}\n"
    "path 1 2 20 5 -5 {0 100} {150 100} {150 150} {100 150}\n"
    "path 1 2 20 5 -5 {0 300} {150 300} {150 350} {100 350}\n"
    "path 1 2 20 0 -5 {0 500} {150 500} {150 550} {100 550}\n"
    "path 1 2 24 0 0 {0 700} {150 700} {150 750} {100 750}\n"
    "path 1 2 24 12 12 {0 900} {150 900} {150 950} {100 950}\n"
    "path 2 3 24 12 12 {0 1100} {150 1100} {150 1150} {100 1150}\n"
    "path 2 3 24 12 12 {0 1300} {150 1300} {150 1350} {100 1350}\n"
    "path 2 3 24 12 12 {200 1300} {350 1300} {350 1350} {300 1350}\n"
    "path 2 3 24 12 12 {400 1300} {550 1300} {550 1350} {500 1350}\n"
    "path 2 3 24 12 12 {0 1600} {150 1600} {150 1650} {100 1650}\n"
    "path 2 3 24 12 12 {200 1600} {350 1600} {350 1650} {300 1650}\n"
    "path 2 3 24 12 12 {400 1600} {550 1600} {550 1650} {500 1650}\n"
    "path 2 3 24 12 12 {0 1900} {150 1900} {150 1950} {100 1950}\n"
    "path 2 3 24 12 12 {200 1900} {350 1900} {350 1950} {300 1950}\n"
    "path 2 3 24 12 12 {400 1900} {550 1900} {550 1950} {500 1950}\n"
    "path 2 3 24 12 12 {0 2200} {150 2200} {150 2250} {100 2250}\n"
    "path 2 3 24 12 12 {200 2200} {350 2200} {350 2250} {300 2250}\n"
    "path 2 3 24 12 12 {400 2200} {550 2200} {550 2250} {500 2250}\n"
    "path 1 3 24 12 12 {1000 1300} {1150 1300} {1150 1350} {1100 1350}\n"
    "path 1 3 24 12 12 {1200 1300} {1350 1300} {1350 1350} {1300 1350}\n"
    "path 1 3 24 12 12 {1400 1300} {1550 1300} {1550 1350} {1500 1350}\n"
    "path 1 3 24 12 12 {1000 1600} {1150 1600} {1150 1650} {1100 1650}\n"
    "path 1 3 24 12 12 {1200 1600} {1350 1600} {1350 1650} {1300 1650}\n"
    "path 1 3 24 12 12 {1400 1600} {1550 1600} {1550 1650} {1500 1650}\n"
    "path 1 3 24 12 12 {1000 1900} {1150 1900} {1150 1950} {1100 1950}\n"
    "path 1 3 24 12 12 {1200 1900} {1350 1900} {1350 1950} {1300 1950}\n"
    "path 1 3 24 12 12 {1400 1900} {1550 1900} {1550 1950} {1500 1950}\n"
    "path 1 3 24 12 12 {1000 2200} {1150 2200} {1150 2250} {1100 2250}\n"
    "path 1 3 24 12 12 {1200 2200} {1350 2200} {1350 2250} {1300 2250}\n"
    "path 1 3 24 12 12 {1400 2200} {1550 2200} {1550 2250} {1500 2250}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t6.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(7_1)
{
  const char *expected; 

  if (db::default_editable_mode ()) {
    expected =
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "boundary 1 2 {20 100} {0 150} {100 150} {60 100} {20 100}\n"
      "boundary 1 2 {100 400} {0 420} {0 450} {100 410} {100 400}\n"
      "boundary 1 2 {150 700} {0 720} {0 730} {150 750} {150 700}\n"
      "boundary 1 2 {0 1000} {20 1050} {130 1050} {150 1000} {0 1000}\n"
      "boundary 1 2 {200 1000} {220 1050} {330 1050} {350 1000} {200 1000}\n"
      "boundary 1 2 {400 1000} {420 1050} {530 1050} {550 1000} {400 1000}\n"
      "boundary 1 2 {0 1300} {20 1350} {130 1350} {150 1300} {0 1300}\n"
      "boundary 1 2 {200 1300} {220 1350} {330 1350} {350 1300} {200 1300}\n"
      "boundary 1 2 {400 1300} {420 1350} {530 1350} {550 1300} {400 1300}\n"
      "boundary 1 2 {0 1600} {20 1650} {130 1650} {150 1600} {0 1600}\n"
      "boundary 1 2 {200 1600} {220 1650} {330 1650} {350 1600} {200 1600}\n"
      "boundary 1 2 {400 1600} {420 1650} {530 1650} {550 1600} {400 1600}\n"
      "boundary 1 2 {0 1900} {20 1950} {130 1950} {150 1900} {0 1900}\n"
      "boundary 1 2 {200 1900} {220 1950} {330 1950} {350 1900} {200 1900}\n"
      "boundary 1 2 {400 1900} {420 1950} {530 1950} {550 1900} {400 1900}\n"
      "boundary 1 2 {1020 100} {1000 150} {1100 150} {1100 100} {1020 100}\n"
      "boundary 1 2 {1100 400} {1000 420} {1000 450} {1100 450} {1100 400}\n"
      "boundary 1 2 {1150 700} {1000 720} {1000 750} {1150 750} {1150 700}\n"
      "boundary 1 2 {1000 1000} {1020 1050} {1150 1050} {1150 1000} {1000 1000}\n"
      "boundary 1 2 {1200 1000} {1220 1050} {1350 1050} {1350 1000} {1200 1000}\n"
      "boundary 1 2 {1400 1000} {1420 1050} {1550 1050} {1550 1000} {1400 1000}\n"
      "boundary 1 2 {1000 1300} {1020 1350} {1150 1350} {1150 1300} {1000 1300}\n"
      "boundary 1 2 {1200 1300} {1220 1350} {1350 1350} {1350 1300} {1200 1300}\n"
      "boundary 1 2 {1400 1300} {1420 1350} {1550 1350} {1550 1300} {1400 1300}\n"
      "boundary 1 2 {1000 1600} {1020 1650} {1150 1650} {1150 1600} {1000 1600}\n"
      "boundary 1 2 {1200 1600} {1220 1650} {1350 1650} {1350 1600} {1200 1600}\n"
      "boundary 1 2 {1400 1600} {1420 1650} {1550 1650} {1550 1600} {1400 1600}\n"
      "boundary 1 2 {1000 1900} {1020 1950} {1150 1950} {1150 1900} {1000 1900}\n"
      "boundary 1 2 {1200 1900} {1220 1950} {1350 1950} {1350 1900} {1200 1900}\n"
      "boundary 1 2 {1400 1900} {1420 1950} {1550 1950} {1550 1900} {1400 1900}\n"
      "boundary 1 2 {2000 100} {2000 150} {2100 150} {2060 100} {2000 100}\n"
      "boundary 1 2 {2000 400} {2000 450} {2100 410} {2100 400} {2000 400}\n"
      "boundary 1 2 {2000 700} {2000 730} {2150 750} {2150 700} {2000 700}\n"
      "boundary 1 2 {2000 1000} {2000 1050} {2130 1050} {2150 1000} {2000 1000}\n"
      "boundary 1 2 {2200 1000} {2200 1050} {2330 1050} {2350 1000} {2200 1000}\n"
      "boundary 1 2 {2400 1000} {2400 1050} {2530 1050} {2550 1000} {2400 1000}\n"
      "boundary 1 2 {2000 1300} {2000 1350} {2130 1350} {2150 1300} {2000 1300}\n"
      "boundary 1 2 {2200 1300} {2200 1350} {2330 1350} {2350 1300} {2200 1300}\n"
      "boundary 1 2 {2400 1300} {2400 1350} {2530 1350} {2550 1300} {2400 1300}\n"
      "boundary 1 2 {2000 1600} {2000 1650} {2130 1650} {2150 1600} {2000 1600}\n"
      "boundary 1 2 {2200 1600} {2200 1650} {2330 1650} {2350 1600} {2200 1600}\n"
      "boundary 1 2 {2400 1600} {2400 1650} {2530 1650} {2550 1600} {2400 1600}\n"
      "boundary 1 2 {2000 1900} {2000 1950} {2130 1950} {2150 1900} {2000 1900}\n"
      "boundary 1 2 {2200 1900} {2200 1950} {2330 1950} {2350 1900} {2200 1900}\n"
      "boundary 1 2 {2400 1900} {2400 1950} {2530 1950} {2550 1900} {2400 1900}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  } else {
    expected = 
      "begin_lib 0.001\n"
      "begin_cell {ABC}\n"
      "boundary 1 2 {20 100} {0 150} {100 150} {60 100} {20 100}\n"
      "boundary 1 2 {100 400} {0 420} {0 450} {100 410} {100 400}\n"
      "boundary 1 2 {150 700} {0 720} {0 730} {150 750} {150 700}\n"
      "boundary 1 2 {1020 100} {1000 150} {1100 150} {1100 100} {1020 100}\n"
      "boundary 1 2 {1100 400} {1000 420} {1000 450} {1100 450} {1100 400}\n"
      "boundary 1 2 {1150 700} {1000 720} {1000 750} {1150 750} {1150 700}\n"
      "boundary 1 2 {2000 100} {2000 150} {2100 150} {2060 100} {2000 100}\n"
      "boundary 1 2 {2000 400} {2000 450} {2100 410} {2100 400} {2000 400}\n"
      "boundary 1 2 {2000 700} {2000 730} {2150 750} {2150 700} {2000 700}\n"
      "boundary 1 2 {0 1000} {20 1050} {130 1050} {150 1000} {0 1000}\n"
      "boundary 1 2 {200 1000} {220 1050} {330 1050} {350 1000} {200 1000}\n"
      "boundary 1 2 {400 1000} {420 1050} {530 1050} {550 1000} {400 1000}\n"
      "boundary 1 2 {0 1300} {20 1350} {130 1350} {150 1300} {0 1300}\n"
      "boundary 1 2 {200 1300} {220 1350} {330 1350} {350 1300} {200 1300}\n"
      "boundary 1 2 {400 1300} {420 1350} {530 1350} {550 1300} {400 1300}\n"
      "boundary 1 2 {0 1600} {20 1650} {130 1650} {150 1600} {0 1600}\n"
      "boundary 1 2 {200 1600} {220 1650} {330 1650} {350 1600} {200 1600}\n"
      "boundary 1 2 {400 1600} {420 1650} {530 1650} {550 1600} {400 1600}\n"
      "boundary 1 2 {0 1900} {20 1950} {130 1950} {150 1900} {0 1900}\n"
      "boundary 1 2 {200 1900} {220 1950} {330 1950} {350 1900} {200 1900}\n"
      "boundary 1 2 {400 1900} {420 1950} {530 1950} {550 1900} {400 1900}\n"
      "boundary 1 2 {1000 1000} {1020 1050} {1150 1050} {1150 1000} {1000 1000}\n"
      "boundary 1 2 {1200 1000} {1220 1050} {1350 1050} {1350 1000} {1200 1000}\n"
      "boundary 1 2 {1400 1000} {1420 1050} {1550 1050} {1550 1000} {1400 1000}\n"
      "boundary 1 2 {1000 1300} {1020 1350} {1150 1350} {1150 1300} {1000 1300}\n"
      "boundary 1 2 {1200 1300} {1220 1350} {1350 1350} {1350 1300} {1200 1300}\n"
      "boundary 1 2 {1400 1300} {1420 1350} {1550 1350} {1550 1300} {1400 1300}\n"
      "boundary 1 2 {1000 1600} {1020 1650} {1150 1650} {1150 1600} {1000 1600}\n"
      "boundary 1 2 {1200 1600} {1220 1650} {1350 1650} {1350 1600} {1200 1600}\n"
      "boundary 1 2 {1400 1600} {1420 1650} {1550 1650} {1550 1600} {1400 1600}\n"
      "boundary 1 2 {1000 1900} {1020 1950} {1150 1950} {1150 1900} {1000 1900}\n"
      "boundary 1 2 {1200 1900} {1220 1950} {1350 1950} {1350 1900} {1200 1900}\n"
      "boundary 1 2 {1400 1900} {1420 1950} {1550 1950} {1550 1900} {1400 1900}\n"
      "boundary 1 2 {2000 1000} {2000 1050} {2130 1050} {2150 1000} {2000 1000}\n"
      "boundary 1 2 {2200 1000} {2200 1050} {2330 1050} {2350 1000} {2200 1000}\n"
      "boundary 1 2 {2400 1000} {2400 1050} {2530 1050} {2550 1000} {2400 1000}\n"
      "boundary 1 2 {2000 1300} {2000 1350} {2130 1350} {2150 1300} {2000 1300}\n"
      "boundary 1 2 {2200 1300} {2200 1350} {2330 1350} {2350 1300} {2200 1300}\n"
      "boundary 1 2 {2400 1300} {2400 1350} {2530 1350} {2550 1300} {2400 1300}\n"
      "boundary 1 2 {2000 1600} {2000 1650} {2130 1650} {2150 1600} {2000 1600}\n"
      "boundary 1 2 {2200 1600} {2200 1650} {2330 1650} {2350 1600} {2200 1600}\n"
      "boundary 1 2 {2400 1600} {2400 1650} {2530 1650} {2550 1600} {2400 1600}\n"
      "boundary 1 2 {2000 1900} {2000 1950} {2130 1950} {2150 1900} {2000 1900}\n"
      "boundary 1 2 {2200 1900} {2200 1950} {2330 1950} {2350 1900} {2200 1900}\n"
      "boundary 1 2 {2400 1900} {2400 1950} {2530 1950} {2550 1900} {2400 1900}\n"
      "end_cell\n"
      "end_lib\n"
    ;
  }

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t7.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 1 {-300 400}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 90 1 1 {700 2400}\n"
    "aref {A} 270 1 1 3 4 {2000 0} {2900 0} {2000 1200}\n"
    "aref {A} 270 1 1 3 4 {4000 0} {4900 0} {4000 1200}\n"
    "aref {A} 270 1 1 3 1 {6000 0} {6960 0} {6000 0}\n"
    "aref {A} 270 1 1 1 4 {8000 0} {8000 0} {8000 1240}\n"
    "sref {A} 270 1 1 {10000 0}\n"
    "sref {A} 270 1 1 {10320 0}\n"
    "sref {A} 270 1 1 {10650 0}\n"
    "sref {A} 270 1 1 {10990 0}\n"
    "aref {A} 270 1 1 3 4 {12000 0} {12930 960} {10680 1320}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_2)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 1 {-300 400}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 90 1 1 {700 2400}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_3)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 1 {-300 400}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 90 1 1 {700 2400}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.3.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_4)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "aref {A} 0 0 1 3 4 {-300 400} {-240 400} {-300 520}\n"
    "aref {A} 0 0 1 3 4 {-300 800} {-240 800} {-300 920}\n"
    "aref {A} 0 0 1 3 4 {-300 1200} {-240 1200} {-300 1320}\n"
    "aref {A} 0 0 1 3 4 {0 1200} {60 1200} {0 1320}\n"
    "aref {A} 0 1 1 3 4 {700 400} {760 400} {700 520}\n"
    "aref {A} 90 0 1 3 4 {700 1400} {760 1400} {700 1520}\n"
    "aref {A} 90 1 1 3 4 {700 2400} {760 2400} {700 2520}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.4.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_5)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 1 {-300 400}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 90 1 1 {700 2400}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.5.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_6)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 0.5 {-150 200}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 270 1 1 {700 2400}\n"
    "end_cell\n"
    "begin_cell {TOPTOP}\n"
    "sref {TOP} 90 0 0.5 {100 0}\n"
    "sref {TOP} 0 0 1 {200 1000}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.6.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_7)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 1 {-300 400}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 90 1 1 {700 2400}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.7.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(8_8)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "box 1 2 {30 -40} {130 160}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 2 {-100 100}\n"
    "sref {A} 45 0 1 {-150 1100}\n"
    "aref {A} 135 1 0.5 3 4 {-200 2100} {400 2100} {-200 3300}\n"
    "end_cell\n"
    "begin_cell {TOPTOP}\n"
    "sref {TOP} 22.5 0 0.5 {100 0}\n"
    "sref {TOP} 0 0 1 {1100 0}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t8.8.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(9_1)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "boundary 1 2 {-100 200} {-100 400} {0 400} {0 200} {-100 200}\n"
    "boundary 1 2 {-100 600} {-100 800} {0 800} {0 600} {-100 600}\n"
    "boundary 1 2 {-100 1000} {-100 1100} {50 1100} {150 1000} {-100 1000}\n"
    "boundary 1 2 {-100 1400} {-100 1500} {150 1500} {50 1400} {-100 1400}\n"
    "boundary 1 2 {-100 1800} {0 1900} {150 1900} {150 1800} {-100 1800}\n"
    "boundary 1 2 {0 2200} {-100 2300} {150 2300} {150 2200} {0 2200}\n"
    "boundary 1 2 {-100 2600} {0 2700} {50 2700} {150 2600} {-100 2600}\n"
    "boundary 1 2 {0 3000} {-100 3100} {150 3100} {50 3000} {0 3000}\n"
    "boundary 1 2 {-100 3400} {0 3500} {150 3500} {50 3400} {-100 3400}\n"
    "boundary 1 2 {0 3800} {-100 3900} {50 3900} {150 3800} {0 3800}\n"
    "boundary 1 2 {-100 4200} {-100 4450} {0 4350} {0 4200} {-100 4200}\n"
    "boundary 1 2 {-100 4600} {-100 4750} {0 4850} {0 4600} {-100 4600}\n"
    "boundary 1 2 {-100 5000} {-100 5250} {0 5250} {0 5100} {-100 5000}\n"
    "boundary 1 2 {0 5400} {-100 5500} {-100 5650} {0 5650} {0 5400}\n"
    "boundary 1 2 {-100 5800} {-100 6050} {0 5950} {0 5900} {-100 5800}\n"
    "boundary 1 2 {0 6200} {-100 6300} {-100 6350} {0 6450} {0 6200}\n"
    "boundary 1 2 {-100 6600} {-100 6750} {0 6850} {0 6700} {-100 6600}\n"
    "boundary 1 2 {0 7000} {-100 7100} {-100 7250} {0 7150} {0 7000}\n"
    "boundary 1 2 {-100 7400} {-100 7650} {150 7400} {-100 7400}\n"
    "boundary 1 2 {-100 7800} {-100 8050} {150 8050} {-100 7800}\n"
    "boundary 1 2 {-100 8200} {150 8450} {150 8200} {-100 8200}\n"
    "boundary 1 2 {150 8600} {-100 8850} {150 8850} {150 8600}\n"
    "boundary 1 2 {-100 9000} {0 9100} {100 9000} {-100 9000}\n"
    "boundary 1 2 {0 9400} {-100 9500} {100 9500} {0 9400}\n"
    "boundary 1 2 {-100 9800} {-100 10000} {0 9900} {-100 9800}\n"
    "boundary 1 2 {0 10200} {-100 10300} {0 10400} {0 10200}\n"
    "boundary 1 2 {-100 10600} {-100 10700} {150 10700} {150 10600} {-100 10600}\n"
    "boundary 1 2 {-100 11000} {-100 11250} {150 11250} {150 11000} {-100 11000}\n"
    "boundary 2 3 {-100 11400} {-100 11650} {150 11650} {150 11400} {-100 11400}\n"
    "boundary 2 3 {300 11400} {300 11650} {550 11650} {550 11400} {300 11400}\n"
    "boundary 2 3 {700 11400} {700 11650} {950 11650} {950 11400} {700 11400}\n"
    "boundary 2 3 {-100 11700} {-100 11950} {150 11950} {150 11700} {-100 11700}\n"
    "boundary 2 3 {300 11700} {300 11950} {550 11950} {550 11700} {300 11700}\n"
    "boundary 2 3 {700 11700} {700 11950} {950 11950} {950 11700} {700 11700}\n"
    "boundary 2 3 {-100 12000} {-100 12250} {150 12250} {150 12000} {-100 12000}\n"
    "boundary 2 3 {300 12000} {300 12250} {550 12250} {550 12000} {300 12000}\n"
    "boundary 2 3 {700 12000} {700 12250} {950 12250} {950 12000} {700 12000}\n"
    "boundary 2 3 {-100 12300} {-100 12550} {150 12550} {150 12300} {-100 12300}\n"
    "boundary 2 3 {300 12300} {300 12550} {550 12550} {550 12300} {300 12300}\n"
    "boundary 2 3 {700 12300} {700 12550} {950 12550} {950 12300} {700 12300}\n"
    "box 2 3 {-100 600} {0 800}\n"
    "box 2 3 {-100 1000} {150 1100}\n"
    "box 2 3 {-100 1400} {150 1500}\n"
    "box 2 3 {-100 1800} {150 1900}\n"
    "box 2 3 {-100 2200} {150 2300}\n"
    "box 2 3 {-100 2600} {150 2700}\n"
    "box 2 3 {-100 3000} {150 3100}\n"
    "box 2 3 {-100 3400} {150 3500}\n"
    "box 2 3 {-100 3800} {150 3900}\n"
    "box 2 3 {-100 4200} {0 4450}\n"
    "box 2 3 {-100 4600} {0 4850}\n"
    "box 2 3 {-100 5000} {0 5250}\n"
    "box 2 3 {-100 5400} {0 5650}\n"
    "box 2 3 {-100 5800} {0 6050}\n"
    "box 2 3 {-100 6200} {0 6450}\n"
    "box 2 3 {-100 6600} {0 6850}\n"
    "box 2 3 {-100 7000} {0 7250}\n"
    "box 2 3 {-100 7400} {150 7650}\n"
    "box 2 3 {-100 7800} {150 8050}\n"
    "box 2 3 {-100 8200} {150 8450}\n"
    "box 2 3 {-100 8600} {150 8850}\n"
    "box 2 3 {-100 9000} {100 9100}\n"
    "box 2 3 {-100 9400} {100 9500}\n"
    "box 2 3 {-100 9800} {0 10000}\n"
    "box 2 3 {-100 10200} {0 10400}\n"
    "box 2 3 {-100 10600} {150 10700}\n"
    "box 2 3 {-100 11000} {150 11250}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t9.1.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(9_2)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {B}\n"
    "boundary 1 2 {-100 200} {100 400} {300 200} {-100 200}\n"
    "boundary 1 2 {-100 600} {100 800} {300 600} {-100 600}\n"
    "end_cell\n"
    "begin_cell {A}\n"
    "boundary 1 2 {-100 200} {-100 400} {100 200} {-100 200}\n"
    "boundary 1 2 {-100 600} {-100 800} {100 600} {-100 600}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);
  std::string fn (tl::testsrc ());
  fn += "/testdata/oasis/t9.2.oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);
  
  bool error = false;
  try {
    db::LayerMap map = reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

//  Tests add-on reading 
TEST(99)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "boundary 1 2 {-100 200} {-100 400} {100 200} {-100 200}\n"
    "boundary 1 2 {-100 600} {-100 800} {100 600} {-100 600}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 1 {-300 400}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 90 1 1 {700 2400}\n"
    "end_cell\n"
    "begin_cell {B}\n"
    "boundary 1 2 {-100 200} {100 400} {300 200} {-100 200}\n"
    "boundary 1 2 {-100 600} {100 800} {300 600} {-100 600}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);

  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/oasis/t9.2.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    
    bool error = false;
    try {
      db::LayerMap map = reader.read (layout);
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
      error = true;
    }
    EXPECT_EQ (error, false)
  }

  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/oasis/t8.7.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    
    bool error = false;
    try {
      db::LayerMap map = reader.read (layout);
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
      error = true;
    }
    EXPECT_EQ (error, false)
  }

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

//  XGEOMTRY tests (#773)
TEST(100)
{
  const char *expected = 
    "begin_lib 0.0001\n"
    "begin_cell {mask}\n"
    "boundary 7 1 {13237 5356} {13210 5490} {13192 5530} {13170 5563} {13130 5586} {13090 5583} {13070 5570} {13050 5551} {13037 5530} {13021 5490} {12988 5378} {12938 5390} {12963 5530} {12977 5570} {12998 5610} {13034 5650} {13051 5663} {13090 5678} {13130 5679} {13171 5667} {13210 5638} {13232 5611} {13253 5570} {13274 5490} {13291 5365} {13237 5356}\n"
    "boundary 4 0 {10772 1658} {10772 1744} {14510 1744} {14510 1658} {10772 1658}\n"
    "boundary 4 0 {14510 1658} {14510 1744} {15672 1744} {15672 1658} {14510 1658}\n"
    "boundary 4 0 {18157 647} {18157 676} {21642 676} {21642 647} {18157 647}\n"
    "boundary 6 0 {6743 2449} {6743 4230} {9061 4230} {9061 2449} {6743 2449}\n"
    "boundary 2 3 {21642 3613} {21642 4005} {19409 4005} {19409 6980} {21812 6980} {21812 4958} {21942 4958} {21942 4005} {21812 4005} {21812 3613} {21642 3613}\n"
    "boundary 2 4 {21642 4005} {21642 4958} {21812 4958} {21812 4005} {21642 4005}\n"
    "boundary 8 0 {21680 4106} {21640 4107} {21600 4118} {21574 4130} {21560 4138} {21520 4163} {21509 4170} {21480 4194} {21458 4210} {21440 4227} {21411 4250} {21400 4262} {21366 4290} {21360 4298} {21324 4330} {21320 4335} {21282 4370} {21280 4373} {21241 4410} {21240 4411} {21200 4450} {21160 4490} {21159 4490} {21039 4610} {21000 4650} {20960 4690} {20960 4691} {20921 4730} {20920 4732} {20896 4770} {20886 4810} {20882 4850} {20880 4930} {20880 5330} {20920 5370} {20960 5370} {21000 5340} {21013 5330} {21040 5325} {21080 5309} {21120 5291} {21121 5290} {21160 5276} {21200 5258} {21210 5250} {21240 5240} {21280 5222} {21295 5210} {21320 5202} {21360 5181} {21374 5170} {21400 5160} {21440 5136} {21447 5130} {21480 5112} {21510 5090} {21520 5086} {21560 5058} {21568 5050} {21600 5027} {21617 5010} {21640 4993} {21662 4970} {21680 4955} {21701 4930} {21720 4910} {21735 4890} {21760 4856} {21764 4850} {21786 4810} {21800 4781} {21805 4770} {21818 4730} {21828 4690} {21836 4650} {21840 4616} {21841 4610} {21845 4530} {21845 4450} {21844 4410} {21841 4370} {21840 4358} {21836 4330} {21829 4290} {21818 4250} {21803 4210} {21800 4205} {21778 4170} {21760 4148} {21738 4130} {21720 4118} {21680 4106}\n"
    "boundary 1 0 {17922 6288} {17922 6510} {18150 6510} {18150 6288} {17922 6288}\n"
    "boundary 1 0 {18157 647} {18157 676} {21630 676} {21630 647} {18157 647}\n"
    "boundary 1 0 {21956 0} {21956 89} {22047 89} {22047 0} {21956 0}\n"
    "boundary 3 0 {15392 1744} {15392 1774} {15672 1774} {15672 1744} {15392 1744}\n"
    "boundary 5 1 {15550 1658} {15550 1673} {15570 1673} {15570 1658} {15550 1658}\n"
    "boundary 5 1 {15661 1657} {15641 1659} {15642 1671} {15662 1669} {15661 1657}\n"
    "boundary 5 1 {18150 7440} {18150 7460} {18162 7460} {18162 7440} {18150 7440}\n"
    "boundary 5 1 {18150 8488} {18150 8508} {18162 8508} {18162 8488} {18150 8488}\n"
    "boundary 5 1 {18150 9480} {18150 9500} {18162 9500} {18162 9480} {18150 9480}\n"
    "boundary 5 1 {18670 3411} {18670 3468} {18690 3468} {18690 3411} {18670 3411}\n"
    "boundary 5 1 {19470 3411} {19470 3468} {19490 3468} {19490 3411} {19470 3411}\n"
    "boundary 5 1 {20217 3411} {20217 3468} {20237 3468} {20237 3411} {20217 3411}\n"
    "boundary 5 1 {21630 2048} {21630 2068} {21642 2068} {21642 2048} {21630 2048}\n"
    "boundary 5 1 {21630 2293} {21630 2313} {21642 2313} {21642 2293} {21630 2293}\n"
    "boundary 5 1 {21930 9308} {21930 9328} {21942 9328} {21942 9308} {21930 9308}\n"
    "boundary 5 1 {21930 9600} {21930 9620} {21942 9620} {21942 9600} {21930 9600}\n"
    "boundary 5 1 {23570 6128} {23570 6148} {23582 6148} {23582 6128} {23570 6128}\n"
    "boundary 5 1 {23570 6147} {23570 6167} {23582 6167} {23582 6147} {23570 6147}\n"
    "boundary 5 1 {25710 1978} {25710 1998} {25722 1998} {25722 1978} {25710 1978}\n"
    "boundary 5 1 {25710 2800} {25710 2820} {25722 2820} {25722 2800} {25710 2800}\n"
    "boundary 5 2 {18074 6408} {17971 6486} {17983 6502} {18086 6424} {18074 6408}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m;
  db::Layout layout (&m);

  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/oasis/xgeometry_test.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    
    reader.read (layout);
  }

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(Bug_121_1)
{
  db::Manager m;
  db::Layout layout (&m);

  {
    tl::InputStream file (tl::testsrc () + "/testdata/oasis/bug_121a.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  {
    tl::InputStream file (tl::testsrc () + "/testdata/oasis/bug_121b.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  std::string fn_au (tl::testsrc () + "/testdata/oasis/bug_121_au1.gds");
  db::compare_layouts (_this, layout, fn_au, db::WriteGDS2, 1);
}

TEST(Bug_121_2)
{
  db::Manager m;
  db::Layout layout (&m);

  {
    tl::InputStream file (tl::testsrc () + "/testdata/oasis/bug_121a.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  {
    tl::InputStream file (tl::testsrc () + "/testdata/oasis/bug_121c.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  std::string fn_au (tl::testsrc () + "/testdata/oasis/bug_121_au2.gds");
  db::compare_layouts (_this, layout, fn_au, db::WriteGDS2, 1);
}
