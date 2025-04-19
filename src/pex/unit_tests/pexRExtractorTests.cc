
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "pexRExtractor.h"
#include "tlUnitTest.h"

TEST(network_basic)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::Internal, 1);
  pex::RNode *n2 = rn.create_node (pex::RNode::Internal, 2);
  /* pex::RElement *e12 = */ rn.create_element (0.5, n1, n2);

  EXPECT_EQ (rn.to_string (),
    "R $1 $2 2"
  );

  pex::RNode *n3 = rn.create_node (pex::RNode::Internal, 3);
  /* pex::RElement *e13 = */ rn.create_element (0.25, n1, n3);
  pex::RElement *e23 = rn.create_element (1.0, n2, n3);

  EXPECT_EQ (rn.to_string (),
    "R $1 $2 2\n"
    "R $1 $3 4\n"
    "R $2 $3 1"
  );

  pex::RElement *e23b = rn.create_element (4.0, n2, n3);
  EXPECT_EQ (e23 == e23b, true);

  EXPECT_EQ (rn.to_string (),
    "R $1 $2 2\n"
    "R $1 $3 4\n"
    "R $2 $3 0.2"
  );

  rn.remove_element (e23);

  EXPECT_EQ (rn.to_string (),
    "R $1 $2 2\n"
    "R $1 $3 4"
  );

  rn.remove_node (n3);

  EXPECT_EQ (rn.to_string (),
    "R $1 $2 2"
  );

  rn.clear ();

  EXPECT_EQ (rn.to_string (), "");
}

