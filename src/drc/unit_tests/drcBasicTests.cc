
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "utHead.h"
#include "dbReader.h"
#include "lymMacro.h"

TEST(1)
{
  std::string input = ut::testsrc ();
  input += "/testdata/drc/drctest.gds";
  std::string au = ut::testsrc ();
  au += "/testdata/drc/drcBasicTests_au.gds";

  std::string output = this->tmp_file ("tmp.gds");

  lym::Macro drc;
  drc.set_text (tl::sprintf (
      "source(\"%s\", \"TOP\")\n"
      "target(\"%s\", \"TOP\")\n"
      "l1 = input(1, 0)\n"
      "l2 = input(2, 0)\n"
      "l3 = input(3, 0)\n"
      "l1.output(1, 0)\n"
      "l2.output(2, 0)\n"
      "l3.output(3, 0)\n"
      "l1.space(0.5, projection).output(10, 0)\n"
      "(l2 & l3).output(11, 0)\n"
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

  this->compare_layouts (layout, au, ut::NoNormalization);
}

