
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


#include "tlDataMapping.h"
#include "tlString.h"
#include "tlTimer.h"
#include "tlUnitTest.h"
#include "string"
#include "algorithm"
#include "vector"
#include "tlUnitTest.h"

std::string dm_to_string (tl::DataMappingBase &dm)
{
  std::string r;
  std::vector<std::pair<double, double> > table;

  dm.generate_table(table);

  r += tl::to_string (dm.xmin ()) + ".." + tl::to_string (dm.xmax ()) + ":";
  for (std::vector<std::pair<double, double> >::const_iterator t = table.begin (); t != table.end (); ++t) {
    r += tl::to_string (t->first) + "," + tl::to_string (t->second) + ";";
  }

  return r;
}

class MyDataMapping
  : public tl::DataMappingBase
{
public:
  MyDataMapping (double _dx = 0.0) : dx (_dx) { }
  double xmin () const { return 0.0; }
  double xmax () const { return 2.0; }
  void generate_table (std::vector<std::pair<double, double> > &table)
  {
    table.push_back (std::make_pair (0.0 + dx, 0.5));
    table.push_back (std::make_pair (1.0 + dx, 1.0));
    table.push_back (std::make_pair (2.0 + dx, 2.0));
  }
  void dump () const { }
private:
  double dx;
};
  
TEST(1) 
{
  MyDataMapping dm;
  EXPECT_EQ (dm_to_string (dm), "0..2:0,0.5;1,1;2,2;");

  tl::DataMappingLookupTable lt (new MyDataMapping ());
  lt.update_table (-1.0, 3.0, 0.2, 1);
  EXPECT_EQ (lt.dump (), "xmin=-1.1,dx=0.2:0.5;0.5;0.5;0.5;0.5;0.5;0.6;0.7;0.8;0.9;1;1.2;1.4;1.6;1.8;2;2;2;2;2;");

  EXPECT_EQ (tl::to_string (lt[-1.0]), "0.5");
  EXPECT_EQ (tl::to_string (lt[0.0]), "0.5");
  EXPECT_EQ (tl::to_string (lt[0.49]), "0.7");
  EXPECT_EQ (tl::to_string (lt[0.51]), "0.8");
  EXPECT_EQ (tl::to_string (lt[1.0]), "1");
  // unstable: EXPECT_EQ (tl::to_string (lt[1.5]), "1.4"); 
  EXPECT_EQ (tl::to_string (lt[1.49]), "1.4");
  EXPECT_EQ (tl::to_string (lt[1.51]), "1.6");
  EXPECT_EQ (tl::to_string (lt[2.0]), "2");
  EXPECT_EQ (tl::to_string (lt[2.5]), "2");
}

TEST(2) 
{
  tl::CombinedDataMapping dm (new MyDataMapping (), new MyDataMapping ());
  EXPECT_EQ (dm_to_string (dm), "0..2:0,0.75;1,1;2,2;");
}

TEST(3) 
{
  tl::CombinedDataMapping dm (new MyDataMapping (), new MyDataMapping (0.2));
  EXPECT_EQ (dm_to_string (dm), "0..2:0.2,0.75;1.2,1;2.2,2;");
}

TEST(4) 
{
  tl::CombinedDataMapping dm (new MyDataMapping (0.2), new MyDataMapping ());
  EXPECT_EQ (dm_to_string (dm), "0..2:0,0.65;1,0.9;1.2,1;2,1.8;");
}

TEST(5) 
{
  tl::LinearCombinationDataMapping dm (-1.0, new MyDataMapping (), 1.0, new MyDataMapping (), 2.0);
  EXPECT_EQ (dm_to_string (dm), "0..2:0,0.5;1,2;2,5;");
}

TEST(6) 
{
  tl::LinearCombinationDataMapping dm (0.0, new MyDataMapping (), 1.0, new MyDataMapping (0.2), 1.0);
  EXPECT_EQ (dm_to_string (dm), "0..2:0,1;0.2,1.1;1,1.9;1.2,2.2;2,3.8;2.2,4;");
}

TEST(7) 
{
  tl::LinearCombinationDataMapping dm (0.0, new MyDataMapping (), 1.0, new MyDataMapping (0.2), 0.5);
  EXPECT_EQ (dm_to_string (dm), "0..2:0,0.75;0.2,0.85;1,1.45;1.2,1.7;2,2.9;2.2,3;");
}

TEST(8) 
{
  tl::LinearCombinationDataMapping dm (-1.0, new MyDataMapping (0.2), 1.0, new MyDataMapping (), 2.0);
  EXPECT_EQ (dm_to_string (dm), "0..2:0,0.5;0.2,0.7;1,1.9;1.2,2.4;2,4.8;2.2,5;");
}

