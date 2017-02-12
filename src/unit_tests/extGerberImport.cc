
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


#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbOASISWriter.h"
#include "dbLoadLayoutOptions.h"
#include "dbReader.h"

#include <utHead.h>

#include <stdlib.h>

static void run_test (ut::TestBase *_this, const char *dir)
{
  db::LoadLayoutOptions options;

  db::Layout layout;

  {
    std::string fn (ut::testsrc_private ());
    fn += "/testdata/pcb/";
    fn += dir;
    fn += "/import.pcb";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  _this->compare_layouts (layout, ut::testsrc_private () + "/testdata/pcb/" + dir + "/au.oas.gz", ut::WriteOAS);
}

TEST(1)
{
  run_test (_this, "microchip-1");
}

TEST(2)
{
  run_test (_this, "allegro");
}

TEST(3)
{
  run_test (_this, "sample-board");
}

TEST(4)
{
  run_test (_this, "exc-test");
}

TEST(5)
{
  run_test (_this, "burstDrive");
}

TEST(6)
{
  run_test (_this, "mentor");
}

TEST(7)
{
  run_test (_this, "pcb-1");
}

TEST(8)
{
  run_test (_this, "microchip-2");
}

TEST(9)
{
  run_test (_this, "microchip-3");
}

TEST(10)
{
  run_test (_this, "gerbv_examples/am-test");
}

TEST(11)
{
  run_test (_this, "gerbv_examples/trailing");
}

TEST(12)
{
  run_test (_this, "gerbv_examples/nollezappare");
}

TEST(13)
{
  run_test (_this, "gerbv_examples/thermal");
}

TEST(14)
{
  run_test (_this, "gerbv_examples/amacro-ref");
}

TEST(15)
{
  run_test (_this, "gerbv_examples/polarity");
}

TEST(16)
{
  run_test (_this, "gerbv_examples/protel-pnp");
}

TEST(17)
{
  run_test (_this, "gerbv_examples/hellboard");
}

TEST(18)
{
  run_test (_this, "gerbv_examples/274X");
}

TEST(19)
{
  run_test (_this, "gerbv_examples/eaglecad1");
}

TEST(20)
{
  run_test (_this, "gerbv_examples/jj");
}

TEST(21)
{
  run_test (_this, "gerbv_examples/orcad");
}

TEST(22)
{
  run_test (_this, "gerbv_examples/pick-and-place");
}

TEST(23)
{
  run_test (_this, "gerbv_examples/Mentor-BoardStation");
}

TEST(24)
{
  run_test (_this, "sr-sample");
}

TEST(25)
{
  run_test (_this, "sr-sample2");
}

TEST(26)
{
  run_test (_this, "pos-neg");
}

