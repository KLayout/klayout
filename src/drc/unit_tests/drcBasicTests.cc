
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
#include "dbReader.h"
#include "dbTestSupport.h"
#include "lymMacro.h"

TEST(1)
{
  std::string input = tl::testdata ();
  input += "/drc/drctest.gds";
  std::string au = tl::testdata ();
  au += "/drc/drcBasicTests_au.gds";

  std::string output = this->tmp_file ("tmp.gds");

  lym::Macro drc;
  drc.set_text (tl::sprintf (
      "force_gc true\n"
      "source('%s', \"TOP\")\n"
      "target('%s', \"TOP\")\n"
      "l1 = input(1, 0)\n"
      "l1t = labels(1, 0)\n"
      "l2 = input(2, 0)\n"
      "l3 = input(3, 0)\n"
      "l1.output(1, 0)\n"
      "l2.output(2, 0)\n"
      "l3.output(3, 0)\n"
      "l1.space(0.5, projection).output(10, 0)\n"
      "(l2 & l3).output(11, 0)\n"
      "l1t.output(20, 0)\n"
    , input, output)
  );
  drc.set_interpreter (lym::Macro::DSLInterpreter);
  drc.set_dsl_interpreter ("drc-dsl");

  EXPECT_EQ (drc.run (), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (_this, layout, au, db::NoNormalization);
}

TEST(2)
{
  lym::Macro drc;
  drc.set_text (
    "force_gc true\n"
    "dbu 0.001\n"
    "def compare(a, b, ex)\n"
    "  a = a.to_s\n"
    "  b = b.to_s\n"
    "  if a != b\n"
    "    raise(ex + \" (actual=#{a}, ref=#{b})\")\n"
    "  end\n"
    "end\n"
    "compare(0.1.um, 0.1, \"unexpected value when converting um\")\n"
    "compare(0.1.micron, 0.1, \"unexpected value when converting micron\")\n"
    "compare(0.1.um2, 0.1, \"unexpected value when converting um2\")\n"
    "compare(0.1.mm2, 100000.0, \"unexpected value when converting mm2\")\n"
    "compare(120.dbu, 0.12, \"unexpected value when converting dbu\")\n"
    "compare((0.1.um + 120.dbu), 0.22, \"unexpected value when adding values\")\n"
    "compare(0.1.mm, 100.0, \"unexpected value when converting mm\")\n"
    "compare(1e-6.m, 1.0, \"unexpected value when converting m\")\n"
    "compare(1.um, 1.0, \"unexpected value when converting integer um\")\n"
    "compare(1.micron, 1.0, \"unexpected value when convering integer micron\")\n"
    "compare(1.um2, 1.0, \"unexpected value when converting integer um2\")\n"
    "compare(1.mm2, 1000000.0, \"unexpected value when converting integer mm2\")\n"
    "compare((1.um + 120.dbu), 1.12, \"unexpected value when adding integer values\")\n"
    "compare(1.mm, 1000.0, \"unexpected value when converting integer mm\")\n"
    "compare(1.m, 1000000.0, \"unexpected value when converting integer m\")\n"
  );
  drc.set_interpreter (lym::Macro::DSLInterpreter);
  drc.set_dsl_interpreter ("drc-dsl");

  EXPECT_EQ (drc.run (), 0);
}

