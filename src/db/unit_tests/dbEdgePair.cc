
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



#include "dbEdgePair.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::EdgePair ep;

  EXPECT_EQ (ep.to_string (), "(0,0;0,0)/(0,0;0,0)");
  EXPECT_EQ (ep == db::EdgePair (db::Edge (), db::Edge ()), true);
  EXPECT_EQ (ep != db::EdgePair (db::Edge (), db::Edge ()), false);
  EXPECT_EQ (ep < db::EdgePair (db::Edge (), db::Edge ()), false);

  ep = db::EdgePair (db::Edge (db::Point (10, 30), db::Point (15, 30)), db::Edge (db::Point (0, 30), db::Point (0, 40)));
  EXPECT_EQ (ep.to_string (), "(10,30;15,30)/(0,30;0,40)");
  EXPECT_EQ (ep.normalized ().to_string (), "(15,30;10,30)/(0,30;0,40)");
  EXPECT_EQ (ep.normalized ().normalized ().to_string (), "(15,30;10,30)/(0,30;0,40)");

  ep = db::EdgePair (db::Edge (db::Point (1, 2), db::Point (11, 12)), db::Edge (db::Point (-5, 5), db::Point (5, 15)));
  EXPECT_EQ (ep.to_string (), "(1,2;11,12)/(-5,5;5,15)");
  EXPECT_EQ (ep.normalized ().to_string (), "(11,12;1,2)/(-5,5;5,15)");
  EXPECT_EQ (ep.normalized ().normalized ().to_string (), "(11,12;1,2)/(-5,5;5,15)");
  EXPECT_EQ (ep == db::EdgePair (db::Edge (), db::Edge ()), false);
  EXPECT_EQ (ep != db::EdgePair (db::Edge (), db::Edge ()), true);
  EXPECT_EQ (db::EdgePair (db::Edge (), db::Edge ()) < ep, true);

  EXPECT_EQ (ep.scaled (5).to_string (), "(5,10;55,60)/(-25,25;25,75)");
  EXPECT_EQ ((ep * 2.5).to_string (), "(2.5,5;27.5,30)/(-12.5,12.5;12.5,37.5)");

  std::string s = ep.to_string ();
  tl::Extractor ex (s.c_str ());
  db::EdgePair ep2;
  ex.read (ep2);
  EXPECT_EQ (ep2.to_string () == s, true);
  EXPECT_EQ (ep2 == ep, true);

  db::DEdgePair dep = ep;
  EXPECT_EQ (dep.to_string (), "(1,2;11,12)/(-5,5;5,15)");
  EXPECT_EQ (db::EdgePair (dep).to_string (), "(1,2;11,12)/(-5,5;5,15)");

  EXPECT_EQ (ep.normalized ().to_string (), "(11,12;1,2)/(-5,5;5,15)");
  ep2 = ep;
  ep2.normalize ();
  EXPECT_EQ (ep2.to_string (), "(11,12;1,2)/(-5,5;5,15)");

  EXPECT_EQ (ep.moved (db::Vector (1, 2)).to_string (), "(2,4;12,14)/(-4,7;6,17)");
  ep2 = ep;
  ep2.move (db::Vector (1, 2));
  EXPECT_EQ (ep2.to_string (), "(2,4;12,14)/(-4,7;6,17)");

  EXPECT_EQ (ep.transformed (db::FTrans (1)).to_string (), "(-2,1;-12,11)/(-5,-5;-15,5)");
  ep2 = ep;
  ep2.transform (db::FTrans (1));
  EXPECT_EQ (ep2.to_string (), "(-2,1;-12,11)/(-5,-5;-15,5)");

  ep.set_first (db::Edge (db::Point (0, 0), db::Point (1, 1)));
  ep.set_second (db::Edge (db::Point (2, 2), db::Point (3, 3)));
  EXPECT_EQ (ep.to_string (), "(0,0;1,1)/(2,2;3,3)");
  EXPECT_EQ (ep.to_string (0.5), "(0.00000,0.00000;0.50000,0.50000)/(1.00000,1.00000;1.50000,1.50000)");
  EXPECT_EQ (ep.first ().to_string (), "(0,0;1,1)");
  EXPECT_EQ (ep.second ().to_string (), "(2,2;3,3)");
  ep.swap_edges ();
  EXPECT_EQ (ep.to_string (0.5), "(1.00000,1.00000;1.50000,1.50000)/(0.00000,0.00000;0.50000,0.50000)");
  EXPECT_EQ (ep.bbox ().to_string (), "(0,0;3,3)");
  EXPECT_EQ (ep.is_ortho (), false);
  EXPECT_EQ (ep.parallel (), true);
  EXPECT_EQ (ep.coincident (), false);

  ep.set_second (db::Edge (db::Point (0, 0), db::Point (10, 10)));
  ep.set_first (db::Edge (db::Point (0, 0), db::Point (10, 0)));
  EXPECT_EQ (ep.is_ortho (), false);
  EXPECT_EQ (ep.parallel (), false);
  EXPECT_EQ (ep.coincident (), false);
  ep.set_second (db::Edge (db::Point (10, 10), db::Point (0, 10)));
  EXPECT_EQ (ep.is_ortho (), true);
  EXPECT_EQ (ep.parallel (), true);
  EXPECT_EQ (ep.coincident (), false);
  ep.set_second (db::Edge (db::Point (10, 0), db::Point (0, 0)));
  EXPECT_EQ (ep.is_ortho (), true);
  EXPECT_EQ (ep.parallel (), true);
  EXPECT_EQ (ep.coincident (), true);
}

TEST(2) 
{
  db::EdgePair ep;
  EXPECT_EQ (ep.to_polygon (0).to_string (), "()");
  EXPECT_EQ (ep.to_polygon (1).to_string (), "(-1,-1;-1,1;1,1;1,-1)");

  ep = db::EdgePair (db::Edge (db::Point (1, 2), db::Point (11, 12)), db::Edge (db::Point (-5, 5), db::Point (5, 15)));
  EXPECT_EQ (ep.to_polygon (0).to_string (), "(1,2;5,15;-5,5;11,12)");
  EXPECT_EQ (ep.normalized ().to_polygon (0).to_string (), "(1,2;-5,5;5,15;11,12)");
  EXPECT_EQ (ep.to_polygon (1).to_string (), "(0,2;5,16;-6,5;11,13)");
  EXPECT_EQ (ep.normalized ().to_polygon (1).to_string (), "(1,1;-6,5;5,16;12,12)");

  ep = db::EdgePair (db::Edge (db::Point (1, 2), db::Point (1, 2)), db::Edge (db::Point (-5, 5), db::Point (5, 15)));
  EXPECT_EQ (ep.to_polygon (0).to_string (), "(1,2;-5,5;5,15)");
  EXPECT_EQ (ep.normalized ().to_polygon (0).to_string (), "(1,2;-5,5;5,15)");
  EXPECT_EQ (ep.to_polygon (1).to_string (), "(1,2;-6,5;5,16)");
  EXPECT_EQ (ep.normalized ().to_polygon (1).to_string (), "(1,2;-6,5;5,16)");

  ep = db::EdgePair (db::Edge (db::Point (1, 2), db::Point (11, 12)), db::Edge (db::Point (-5, 5), db::Point (-5, 5)));
  EXPECT_EQ (ep.to_polygon (0).to_string (), "(1,2;-5,5;11,12)");
  EXPECT_EQ (ep.normalized ().to_polygon (0).to_string (), "(1,2;-5,5;11,12)");
  EXPECT_EQ (ep.to_polygon (1).to_string (), "(0,2;-5,5;11,13)");
  EXPECT_EQ (ep.normalized ().to_polygon (1).to_string (), "(1,1;-5,5;12,12)");

  ep = db::EdgePair (db::Edge (db::Point (1, 2), db::Point (1, 2)), db::Edge (db::Point (-5, 5), db::Point (-5, 5)));
  EXPECT_EQ (ep.to_polygon (0).to_string (), "()");
  EXPECT_EQ (ep.normalized ().to_polygon (0).to_string (), "()");
  EXPECT_EQ (ep.to_polygon (1).to_string (), "(1,1;-5,4;-5,6;1,3)");
  EXPECT_EQ (ep.normalized ().to_polygon (1).to_string (), "(1,1;-5,4;-5,6;1,3)");

  ep = db::EdgePair (db::Edge (db::Point (1, 2), db::Point (1, 2)), db::Edge (db::Point (1, 2), db::Point (1, 2)));
  EXPECT_EQ (ep.to_polygon (0).to_string (), "()");
  EXPECT_EQ (ep.normalized ().to_polygon (0).to_string (), "()");
  EXPECT_EQ (ep.to_polygon (1).to_string (), "(0,1;0,3;2,3;2,1)");
  EXPECT_EQ (ep.normalized ().to_polygon (1).to_string (), "(0,1;0,3;2,3;2,1)");

  ep = db::EdgePair (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 20), db::Point (0, 30)));
  EXPECT_EQ (ep.to_polygon (0).to_string (), "()");
  EXPECT_EQ (ep.normalized ().to_polygon (0).to_string (), "()");
  EXPECT_EQ (ep.to_polygon (1).to_string (), "()");
  EXPECT_EQ (ep.normalized ().to_polygon (1).to_string (), "(1,-1;-1,19;-1,31;1,11)");
}

