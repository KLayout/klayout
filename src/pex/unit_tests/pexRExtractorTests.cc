
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


#include "pexRExtractor.h"
#include "pexRNetwork.h"
#include "tlUnitTest.h"

TEST(network_basic)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::Internal, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::Internal, 1, 1);
  EXPECT_EQ (n1 != n2, true);
  pex::RNode *n2_dup = rn.create_node (pex::RNode::Internal, 1, 1);
  EXPECT_EQ (n2 != n2_dup, true);

  /* pex::RElement *e12 = */ rn.create_element (0.5, n1, n2);

  EXPECT_EQ (rn.to_string (),
    "R $1 $1.1 2"
  );
}

TEST(network_basic_vertex_nodes)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::VertexPort, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::VertexPort, 1, 1);
  EXPECT_EQ (n1 != n2, true);
  pex::RNode *n2_dup = rn.create_node (pex::RNode::VertexPort, 1, 1);
  EXPECT_EQ (n2 == n2_dup, true);
  pex::RNode *n2_wrong_type = rn.create_node (pex::RNode::PolygonPort, 1, 1);
  EXPECT_EQ (n2 != n2_wrong_type, true);

  /* pex::RElement *e12 = */ rn.create_element (0.5, n1, n2);

  EXPECT_EQ (rn.to_string (),
    "R V1 V1.1 2"
  );
}

TEST(network_basic_polygon_nodes)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::PolygonPort, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::PolygonPort, 1, 1);
  EXPECT_EQ (n1 != n2, true);
  pex::RNode *n2_dup = rn.create_node (pex::RNode::PolygonPort, 1, 1);
  EXPECT_EQ (n2 == n2_dup, true);
  pex::RNode *n2_wrong_type = rn.create_node (pex::RNode::VertexPort, 1, 1);
  EXPECT_EQ (n2 != n2_wrong_type, true);

  /* pex::RElement *e12 = */ rn.create_element (0.5, n1, n2);

  EXPECT_EQ (rn.to_string (),
    "R P1 P1.1 2"
  );
}

TEST(network_basic_elements)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::Internal, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::Internal, 2, 0);

  /* pex::RElement *e12 = */ rn.create_element (0.5, n1, n2);

  EXPECT_EQ (rn.to_string (),
    "R $1 $2 2"
  );

  pex::RNode *n3 = rn.create_node (pex::RNode::Internal, 3, 0);
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

  pex::RElement *e23c = rn.create_element (5.0, n3, n2);
  EXPECT_EQ (e23 == e23c, true);

  EXPECT_EQ (rn.to_string (),
    "R $1 $2 2\n"
    "R $1 $3 4\n"
    "R $2 $3 0.1"
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

TEST(network_simplify1)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::VertexPort, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::Internal, 2, 0);
  pex::RNode *n3 = rn.create_node (pex::RNode::VertexPort, 3, 0);

  rn.create_element (1, n1, n2);
  rn.create_element (pex::RElement::short_value (), n2, n3);
  rn.create_element (1, n1, n3);

  EXPECT_EQ (rn.to_string (),
    "R $2 V1 1\n"
    "R $2 V3 0\n"
    "R V1 V3 1"
  );

  rn.simplify ();

  EXPECT_EQ (rn.to_string (),
    "R V1 V3 0.5"
  );
}

TEST(network_simplify2)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::VertexPort, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::Internal, 2, 0);
  pex::RNode *n3 = rn.create_node (pex::RNode::Internal, 3, 0);
  pex::RNode *n4 = rn.create_node (pex::RNode::VertexPort, 4, 0);
  pex::RNode *n5 = rn.create_node (pex::RNode::VertexPort, 5, 0);

  rn.create_element (1, n1, n2);
  rn.create_element (pex::RElement::short_value (), n2, n3);
  rn.create_element (1, n3, n4);
  rn.create_element (1, n3, n5);

  EXPECT_EQ (rn.to_string (),
    "R $2 V1 1\n"
    "R $2 $3 0\n"
    "R $3 V4 1\n"
    "R $3 V5 1"
  );

  rn.simplify ();

  EXPECT_EQ (rn.to_string (),
    "R $2 V1 1\n"
    "R $2 V4 1\n"
    "R $2 V5 1"
  );
}

TEST(network_simplify3)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::VertexPort, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::Internal, 2, 0);
  pex::RNode *n3 = rn.create_node (pex::RNode::Internal, 3, 0);
  pex::RNode *n4 = rn.create_node (pex::RNode::VertexPort, 4, 0);

  rn.create_element (1, n1, n2);
  rn.create_element (pex::RElement::short_value (), n2, n3);
  rn.create_element (1, n3, n4);

  EXPECT_EQ (rn.to_string (),
    "R $2 V1 1\n"
    "R $2 $3 0\n"
    "R $3 V4 1"
  );

  rn.simplify ();

  EXPECT_EQ (rn.to_string (),
    "R V1 V4 2"
  );
}

TEST(network_simplify4)
{
  pex::RNetwork rn;
  EXPECT_EQ (rn.to_string (), "");

  pex::RNode *n1 = rn.create_node (pex::RNode::VertexPort, 1, 0);
  pex::RNode *n2 = rn.create_node (pex::RNode::Internal, 2, 0);
  pex::RNode *n3 = rn.create_node (pex::RNode::Internal, 3, 0);
  pex::RNode *n4 = rn.create_node (pex::RNode::VertexPort, 4, 0);

  rn.create_element (1, n1, n4);
  rn.create_element (1, n2, n1);
  rn.create_element (1, n4, n3);

  EXPECT_EQ (rn.to_string (),
    "R V1 V4 1\n"
    "R $2 V1 1\n"
    "R $3 V4 1"
  );

  rn.simplify ();

  EXPECT_EQ (rn.to_string (),
    "R V1 V4 1"
  );
}
