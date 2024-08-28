
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

#include "dbAreaCollector.h"
#include "dbEdgeProcessor.h"
#include "tlUnitTest.h"


namespace
{

class AreaReceiver
  : public db::tagged_area_receiver<double>
{
public:
  typedef db::coord_traits<db::Coord>::area_type area_type;

  AreaReceiver () : m_sum (0.0) { }

  virtual void add_area (area_type area, const double &value)
  {
    m_sum += value * area;
  }

  double get () const { return m_sum; }

private:
  double m_sum;
};

}

TEST(1)
{
  db::EdgeProcessor ep;

  ep.insert (db::SimplePolygon (db::Box (0, 0, 1000, 2000)), 0);
  ep.insert (db::SimplePolygon (db::Box (500, 1000, 1500, 3000)), 1);

  //  set up an XOR mask where 1-vs-0 is counted twice
  tl::bit_set_map<double> bsm;
  tl::BitSetMask bs0;
  bs0.set (0, tl::BitSetMask::True);
  bs0.set (1, tl::BitSetMask::False);
  tl::BitSetMask bs1;
  bs1.set (0, tl::BitSetMask::False);
  bs1.set (1, tl::BitSetMask::True);
  bsm.insert (bs0, 1.0);
  bsm.insert (bs1, 2.0);
  bsm.sort ();

  AreaReceiver rec;
  db::tagged_area_collector<double> coll (bsm, rec);
  ep.process (coll, coll);

  EXPECT_EQ (rec.get (), 4500000);
}
