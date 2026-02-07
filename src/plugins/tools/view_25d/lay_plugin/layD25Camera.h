
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

#ifndef HDR_layD25Camera
#define HDR_layD25Camera

#include "layPluginCommon.h"

#include <QMatrix4x4>
#include <QVector3D>

namespace lay
{

class LAY_PLUGIN_PUBLIC D25Camera
{
public:
  D25Camera ();
  virtual ~D25Camera ();

  /**
   *  @brief Gets the position of the camera objective in the scene coordinate system
   */
  QVector3D cam_position () const;

  /**
   *  @brief Gets the direction the camera looks into in the scene coordinate system
   */
  QVector3D cam_direction () const;

  /**
   *  @brief Gets the perspective part of the transformation applied transform scene coordinates into the image plane
   *  The full transformation for scene to image plane is cam_perspective * cam_trans.
   */
  QMatrix4x4 cam_perspective () const;

  /**
   *  @brief Gets the azimuth/elevation part of the transformation applied transform scene coordinates into the image plane
   *  The full transformation for scene to image plane is cam_perspective * cam_trans.
   */
  QMatrix4x4 cam_trans () const;

  /**
   *  @brief Gets the distance of the objective in scene coordinates
   */
  double cam_dist () const;

  /**
   *  @brief Gets the field of view of the camera
   *  The field of view is the objective opening angle.
   */
  double cam_fov () const;

  /**
   *  @brief Gets a flag indicating whether top view is enabled
   *  In "top view" mode, the elevation is fixed to -90 degree.
   */
  bool top_view () const
  {
    return m_top_view;
  }

  /**
   *  @brief Sets a flag indicating whether top view is enabled
   */
  void set_top_view (bool f)
  {
    m_top_view = f;
    camera_changed ();
  }

  /**
   *  @brief Gets the elevation angle
   *  A negative angle means the camera looks down, a positive angle means it looks up.
   */
  double cam_elevation () const;

  /**
   *  @brief Sets the elevation angle
   */
  void set_cam_elevation (double e)
  {
    m_cam_elevation = e;
    camera_changed ();
  }

  /**
   *  @brief Gets the azimuth angle
   *  A positive angle means we look from the left. A negative means we look from the right.
   */
  double cam_azimuth () const;

  /**
   *  @brief Sets the azimuth angle
   */
  void set_cam_azimuth (double a)
  {
    m_cam_azimuth = a;
    camera_changed ();
  }

  /**
   *  @brief Resets the camera's orientation
   */
  void camera_reset ();

  /**
   *  @brief Resets the camera's orientation but does not call "camera_changed"
   */
  void camera_init ();

protected:
  virtual void camera_changed () { }
  virtual double aspect_ratio () const { return 1.0; }

private:
  double m_cam_azimuth, m_cam_elevation;
  bool m_top_view;
  QVector3D m_displacement;
  double m_fov;

};

}

#endif

