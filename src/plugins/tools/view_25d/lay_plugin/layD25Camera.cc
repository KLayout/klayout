
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


#include "layD25Camera.h"

#include <QWidget>

#include "math.h"

namespace lay
{

D25Camera::D25Camera ()
{
  camera_init ();
}

D25Camera::~D25Camera ()
{
  //  .. nothing yet ..
}

void
D25Camera::camera_init ()
{
  m_fov = 45.0;
  m_cam_azimuth = m_cam_elevation = 0.0;
  m_top_view = false;
}

void
D25Camera::camera_reset ()
{
  camera_init ();
  camera_changed ();
}

double
D25Camera::cam_fov () const
{
  return m_fov;
}

double
D25Camera::cam_dist () const
{
  return 4.0;
}

QVector3D
D25Camera::cam_direction () const
{
  return cam_trans ().inverted ().map (QVector3D (0, 0, -1));
}

QVector3D
D25Camera::cam_position () const
{
  return cam_direction () * -cam_dist ();
}

double
D25Camera::cam_azimuth () const
{
  return m_cam_azimuth;
}

double
D25Camera::cam_elevation () const
{
  return m_top_view ? -90.0 : m_cam_elevation;
}

QMatrix4x4
D25Camera::cam_perspective () const
{
  QMatrix4x4 t;
  t.perspective (cam_fov (), aspect_ratio (), 0.1f, 10000.0f);
  t.translate (QVector3D (0.0, 0.0, -cam_dist ()));
  return t;
}

QMatrix4x4
D25Camera::cam_trans () const
{
  QMatrix4x4 t;
  t.rotate (-cam_elevation (), 1.0, 0.0, 0.0);
  t.rotate (cam_azimuth (), 0.0, 1.0, 0.0);
  return t;
}

}

