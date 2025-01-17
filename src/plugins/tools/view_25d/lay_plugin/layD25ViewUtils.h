
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

#ifndef HDR_layD25ViewUtils
#define HDR_layD25ViewUtils

#include "layPluginCommon.h"

#include <QVector3D>
#include <QMatrix4x4>
#include <algorithm>

namespace lay
{

/**
 *  @brief Computes the cutpoint between a line and a plane
 *
 *  The line is given by a point and a direction (line, line_dir).
 *  The plane is given by a point and a normal vector (plane, plane_normal)
 *  The "first" component of the returned pair is false if not hit is present.
 */

LAY_PLUGIN_PUBLIC std::pair<bool, QVector3D>
cutpoint_line_with_plane (const QVector3D &line, const QVector3D &line_dir, const QVector3D &plane, const QVector3D &plane_normal);

/**
 *  @brief Computes the cutpoint between a line and a face
 *
 *  The line is given by a point and a direction (line, line_dir).
 *  The face is given by a plane point and two vectors spanning the face.
 *  The "first" component of the returned pair is false if not hit is present.
 */

LAY_PLUGIN_PUBLIC std::pair<bool, QVector3D>
cutpoint_line_with_face (const QVector3D &line, const QVector3D &dir, const QVector3D &plane, const QVector3D &u, const QVector3D &v);

/**
 *  @brief Determines a good hit point of a view line and a cuboid
 *
 *  "corner, dim" are the coordinates for the cuboid (corner is the bottom, left, foremost corner, dim
 *  is (width, height, depth)
 *  "line, line_dir" is the view line where "line_dir" is pointing from the camera to the object.
 *  The returned point is a suitable hit point.
 *  The "first" component of the returned pair is false if no hit is present.
 */
LAY_PLUGIN_PUBLIC std::pair<bool, QVector3D>
hit_point_with_cuboid (const QVector3D &line, const QVector3D &line_dir, const QVector3D &corner, const QVector3D &dim);

/**
 *  @brief For a given pixel coordinate and camera transformation matrix compute a line containing all points corresponding to this pixel
 *
 *  The returned pair contains a point and a direction vector describing the line.
 */
LAY_PLUGIN_PUBLIC std::pair<QVector3D, QVector3D>
camera_normal (const QMatrix4x4 &camera_trans, double x, double y);

/**
 *  @brief Normalizes a scene transformation
 *
 *  Scene transformations consist of a scaling and displacement. Both are
 *  interchangeable to some extent under the presence of a perspective
 *  transformation (further away makes the scene smaller). This normalization
 *  tries to find a displacement which has "ztarget" target value for z. Without normalization
 *  the scene tends to "move away" with respect to z.
 */
LAY_PLUGIN_PUBLIC void
normalize_scene_trans (const QMatrix4x4 &cam_trans, QVector3D &displacement, double &scale, double ztarget = 0.0);

}

#endif

