
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include "edtDistribute.h"

template <class Box, class Value>
static std::string plc2string (const edt::distributed_placer<Box, Value> &plc)
{
  std::string s;
  for (typename edt::distributed_placer<Box, Value>::iterator i = plc.begin (); i != plc.end (); ++i) {
    if (! s.empty ()) {
       s += ",";
    }
    s += tl::to_string (i->first);
    s += "[";
    s += tl::to_string (i->second);
    s += "]";
  }
  return s;
}

TEST(1) 
{
  edt::distributed_placer<db::Box, size_t> placer;

  placer.insert (db::Box (1000, 0, 1100, 200), 0);
  placer.insert (db::Box (2000, 0, 2100, 500), 1);
  placer.insert (db::Box (0, -100, 100, 100), 2);
  placer.insert (db::Box (1000, 100, 1050, 250), 3);
  placer.insert (db::Box (1050, -50, 1100, 150), 4);

  edt::distributed_placer<db::Box, size_t> p;

  p = placer;
  p.distribute_h (-1, 0, 100);

  EXPECT_EQ (plc2string (p), "(0,-100;100,100)[2],(200,0;300,200)[0],(400,100;450,250)[3],(550,-50;600,150)[4],(700,0;800,500)[1]");

  p = placer;
  p.distribute_h (-1, 100, 0);

  EXPECT_EQ (plc2string (p), "(0,-100;100,100)[2],(100,0;200,200)[0],(200,100;250,250)[3],(300,-50;350,150)[4],(400,0;500,500)[1]");

  p = placer;
  p.distribute_h (-1, 0, 0);

  EXPECT_EQ (plc2string (p), "(0,-100;100,100)[2],(100,0;200,200)[0],(200,100;250,250)[3],(250,-50;300,150)[4],(300,0;400,500)[1]");

  p = placer;
  p.distribute_h (1, 0, 100);

  EXPECT_EQ (plc2string (p), "(1300,-100;1400,100)[2],(1500,100;1550,250)[3],(1650,-50;1700,150)[4],(1800,0;1900,200)[0],(2000,0;2100,500)[1]");

  p = placer;
  p.distribute_v (-1, 0, 100);

  EXPECT_EQ (plc2string (p), "(0,-100;100,100)[2],(1050,200;1100,400)[4],(1000,500;1100,700)[0],(2000,800;2100,1300)[1],(1000,1400;1050,1550)[3]");
}


TEST(2)
{
  edt::distributed_placer<db::Box, size_t> placer;

  placer.insert (db::Box (-5, 1, 95, 101), 0);
  placer.insert (db::Box (1, 95, 101, 195), 1);
  placer.insert (db::Box (110, 105, 210, 205), 2);
  placer.insert (db::Box (101, 0, 201, 100), 3);

  edt::distributed_placer<db::Box, size_t> p;

  p = placer;
  p.distribute_matrix (-1, 0, 0, -1, 0, 0);

  EXPECT_EQ (plc2string (p), "(-5,0;95,100)[0],(-5,100;95,200)[1],(95,100;195,200)[2],(95,0;195,100)[3]");
}

