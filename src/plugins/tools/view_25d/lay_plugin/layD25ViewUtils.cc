
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

#include "layD25ViewUtils.h"

#include <QMatrix4x4>
#include <QMatrix3x3>

#include <math.h>

namespace lay
{

const double epsilon = 1e-10;

std::pair<bool, QVector3D>
cutpoint_line_with_plane (const QVector3D &line, const QVector3D &dir, const QVector3D &plane, const QVector3D &plane_normal)
{
  double dn = QVector3D::dotProduct (dir, plane_normal);
  if (fabs (dn) < epsilon) {
    return std::make_pair (false, QVector3D ());
  } else {
    return std::make_pair (true, line + dir * QVector3D::dotProduct (plane - line, plane_normal) / dn);
  }
}

std::pair<bool, QVector3D>
cutpoint_line_with_face (const QVector3D &line, const QVector3D &dir, const QVector3D &plane, const QVector3D &u, const QVector3D &v)
{
  QVector3D n = QVector3D::crossProduct (u, v);
  std::pair<bool, QVector3D> r = cutpoint_line_with_plane (line, dir, plane, n);
  if (! r.first) {
    return r;
  }

  double pu = QVector3D::dotProduct (r.second - plane, u);
  double pv = QVector3D::dotProduct (r.second - plane, v);

  //  test whether the cut point is inside the face
  if (pu < -epsilon || pu > u.lengthSquared () + epsilon || pv < -epsilon || pv > v.lengthSquared () + epsilon) {
    return std::make_pair (false, QVector3D ());
  } else {
    return r;
  }
}

static std::pair<bool, QVector3D> plane_or_face (const QVector3D &line, const QVector3D &line_dir, const QVector3D &corner, const QVector3D &u, const QVector3D &v, bool face)
{
  if (face) {
    return cutpoint_line_with_face (line, line_dir, corner, u, v);
  } else {
    return cutpoint_line_with_plane (line, line_dir, corner, QVector3D::crossProduct (u, v));
  }
}

std::pair<bool, QVector3D>
hit_point_with_cuboid (const QVector3D &line, const QVector3D &line_dir, const QVector3D &corner, const QVector3D &dim)
{
  std::vector<std::pair<bool, QVector3D> > cutpoints;
  cutpoints.reserve (6);  //  6 faces

  for (int pass = 0; pass < 2; ++pass) {

    bool face = (pass == 0);

    if (face) {
      bool in_x = (line.x () > corner.x () - epsilon) && (line.x () < corner.x () + dim.x () + epsilon);
      bool in_y = (line.y () > corner.y () - epsilon) && (line.y () < corner.y () + dim.y () + epsilon);
      bool in_z = (line.z () > corner.z () - epsilon) && (line.z () < corner.z () + dim.z () + epsilon);
      if (in_x && in_y && in_z) {
        //  inside cuboid
        return std::make_pair (true, line);
      }
    }

    cutpoints.clear ();

    //  front
    cutpoints.push_back (plane_or_face (line, line_dir, corner, QVector3D (dim.x (), 0, 0), QVector3D (0, dim.y (), 0), face));
    //  back
    cutpoints.push_back (plane_or_face (line, line_dir, corner + QVector3D (0, 0, dim.z ()), QVector3D (dim.x (), 0, 0), QVector3D (0, dim.y (), 0), face));

    if (face) {
      //  bottom
      cutpoints.push_back (plane_or_face (line, line_dir, corner, QVector3D (dim.x (), 0, 0), QVector3D (0, 0, dim.z ()), face));
      //  top
      cutpoints.push_back (plane_or_face (line, line_dir, corner + QVector3D (0, dim.y (), 0), QVector3D (dim.x (), 0, 0), QVector3D (0, 0, dim.z ()), face));
      //  left
      cutpoints.push_back (plane_or_face (line, line_dir, corner, QVector3D (0, 0, dim.z ()), QVector3D (0, dim.y (), 0), face));
      //  right
      cutpoints.push_back (plane_or_face (line, line_dir, corner + QVector3D (dim.x (), 0, 0), QVector3D (0, 0, dim.z ()), QVector3D (0, dim.y (), 0), face));
    }

    double min_dist = 0.0;
    int min_dist_index = -1;
    QVector3D ld_norm = line_dir.normalized ();

    for (std::vector<std::pair<bool, QVector3D> >::const_iterator i = cutpoints.begin (); i != cutpoints.end (); ++i) {
      if (i->first) {
        double dist = QVector3D::dotProduct (i->second - line, ld_norm);
        if (dist < -epsilon) {
          //  ignore all cutpoints behind us
        } else if (min_dist_index < 0) {
          min_dist = dist;
          min_dist_index = int (i - cutpoints.begin ());
        } else if (dist < min_dist) {
          min_dist = dist;
          min_dist_index = int (i - cutpoints.begin ());
        }
      }
    }

    if (min_dist_index >= 0) {
      return cutpoints [min_dist_index];
    }

  }

  return std::make_pair (false, QVector3D ());
}

std::pair<QVector3D, QVector3D>
camera_normal (const QMatrix4x4 &camera_trans, double x, double y)
{
  QVector3D p = camera_trans.inverted ().map (QVector3D (x, y, 1.0));

  QVector4D pv = camera_trans.row (3);

  QMatrix4x4 m (camera_trans);

  float values[] = {
    float (x * pv.x ()),  float (x * pv.y ()),  float (x * pv.z ()),  0.0f,
    float (y * pv.x ()),  float (y * pv.y ()),  float (y * pv.z ()),  0.0f,
    float (pv.x ()),      float (pv.y ()),      float (pv.z ()),      0.0f,
    0.0f,                 0.0f,                 0.0f,                 0.0f
  };
  m -= QMatrix4x4 (values);

  QMatrix3x3 nm = m.normalMatrix ();

  QVector3D u (nm (2, 0), nm (2, 1), nm (2, 2));
  return (std::make_pair (p, u.normalized ()));
}

}
