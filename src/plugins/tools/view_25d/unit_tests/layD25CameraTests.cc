
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


#include "layD25Camera.h"
#include "tlUnitTest.h"

static std::string v2s (const QVector4D &v)
{
  return tl::to_string (v.x ()) + "," + tl::to_string (v.y ()) + "," + tl::to_string (v.z ());
}

static std::string v2s (const QVector3D &v)
{
  return tl::to_string (v.x ()) + "," + tl::to_string (v.y ()) + "," + tl::to_string (v.z ());
}

TEST(1_Transformations)
{
  lay::D25Camera cam;

  cam.set_cam_azimuth (45.0);
  EXPECT_EQ (cam.cam_azimuth (), 45.0);
  cam.set_cam_elevation (22.0);
  EXPECT_EQ (cam.cam_elevation (), 22.0);

  cam.camera_reset ();
  EXPECT_EQ (cam.cam_azimuth (), 0.0);
  EXPECT_EQ (cam.cam_elevation (), 0.0);

  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (1, 0, 0, 1))), "1,0,0");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 1, 0, 1))), "0,1,0");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 0, 1, 1))), "0,0,1");
  EXPECT_EQ (v2s (cam.cam_direction ()), "0,0,-1");
  EXPECT_EQ (v2s (cam.cam_position ()), "0,0,4");

  //  looking up from the bottom, x axis stays the same (azimuth = 0)
  cam.set_cam_elevation (90.0);

  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (1, 0, 0, 1))), "1,0,0");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 1, 0, 1))), "0,0,-1");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 0, 1, 1))), "0,1,0");

  EXPECT_EQ (v2s (cam.cam_direction ()), "0,1,0");
  EXPECT_EQ (v2s (cam.cam_position ()), "0,-4,0");

  //  looking down from the top, x axis stays the same (azimuth = 0)
  cam.set_cam_elevation (-90.0);

  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (1, 0, 0, 1))), "1,0,0");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 1, 0, 1))), "0,0,1");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 0, 1, 1))), "0,-1,0");

  EXPECT_EQ (v2s (cam.cam_direction ()), "0,-1,0");
  EXPECT_EQ (v2s (cam.cam_position ()), "0,4,0");

  //  looking from the left, y axis stays the same (elevation = 0)
  cam.set_cam_elevation (0.0);
  cam.set_cam_azimuth (90.0);

  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (1, 0, 0, 1))), "0,0,-1");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 1, 0, 1))), "0,1,0");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 0, 1, 1))), "1,0,0");

  EXPECT_EQ (v2s (cam.cam_direction ()), "1,0,0");
  EXPECT_EQ (v2s (cam.cam_position ()), "-4,0,0");

  //  looking from the right, y axis stays the same (elevation = 0)
  cam.set_cam_elevation (0.0);
  cam.set_cam_azimuth (-90.0);

  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (1, 0, 0, 1))), "0,0,1");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 1, 0, 1))), "0,1,0");
  EXPECT_EQ (v2s (cam.cam_trans ().map (QVector4D (0, 0, 1, 1))), "-1,0,0");

  EXPECT_EQ (v2s (cam.cam_direction ()), "-1,0,0");
  EXPECT_EQ (v2s (cam.cam_position ()), "4,0,0");
}
