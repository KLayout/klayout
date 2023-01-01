
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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


#include "dbMatrix.h"
#include "dbTrans.h"
#include "tlString.h"

#include <sstream>
#include <limits>
#include <cmath>

namespace db
{

inline double mnorm (double x)
{
  return fabs (x) < 1e-14 ? 0.0 : x;
}

template <class C>
std::string 
matrix_2d<C>::to_string () const
{
  return tl::sprintf ("(%.12g,%.12g) (%.12g,%.12g)", mnorm (m_m11), mnorm (m_m12), mnorm (m_m21), mnorm (m_m22));
}

template <class C>
std::pair<double, double>
matrix_2d<C>::mag2 () const
{
  double s1 = sqrt (m_m11 * m_m11 + m_m21 * m_m21);
  double s2 = sqrt (m_m12 * m_m12 + m_m22 * m_m22);
  double n = sqrt (fabs (det ()) / (s1 * s2));
  return std::make_pair (n * s1, n * s2);
}

template <class C>
bool
matrix_2d<C>::has_rotation () const
{
  return fabs (m_m11 - 1.0) > 1e-10 || fabs (m_m12) > 1e-10 || fabs (m_m21) > 1e-10 || fabs (m_m22 - 1.0) > 1e-10;
}

template <class C>
double
matrix_2d<C>::angle () const
{
  std::pair <double, double> m = mag2 ();
  double u1 = m.first;
  double u2 = is_mirror () ? -m.second : m.second;
  double n11 = m_m11 / u1;
  double n12 = m_m12 / u2;
  double n21 = m_m21 / u1;
  double n22 = m_m22 / u2;

  //  due to rounding error, the sqrt argument can be a negative (but very small)
  double sin_a = 0.5 * sqrt (std::max (0.0, (n21 - n12) * (n21 - n12) - (n11 - n22) * (n11 - n22)));
  double cos_a = 0.5 * sqrt (std::max (0.0, (n11 + n22) * (n11 + n22) - (n21 + n12) * (n21 + n12)));
  if (n11 + n22 < 0.0) {
    cos_a = -cos_a;
  }
  if (n21 - n12 < 0.0) {
    sin_a = -sin_a;
  }

  return 180.0 * atan2 (sin_a, cos_a) / M_PI;
}

template <class C>
matrix_2d<C>
matrix_2d<C>::rotation (double a)
{
  a *= M_PI / 180.0;
  return Matrix2d (cos (a), -sin (a), sin (a), cos (a));
}

template <class C>
bool
matrix_2d<C>::has_shear () const
{
  std::pair <double, double> m = mag2 ();
  double u1 = m.first;
  double u2 = is_mirror () ? -m.second : m.second;
  double n11 = m_m11 / u1;
  double n12 = m_m12 / u2;
  double n21 = m_m21 / u1;
  double n22 = m_m22 / u2;

  double fsin_a = 0.5 * sqrt ((n21 + n12) * (n21 + n12) + (n11 - n22) * (n11 - n22));
  return fabs (fsin_a) > 1e-10;
}

template <class C>
double
matrix_2d<C>::shear_angle () const
{
  std::pair <double, double> m = mag2 ();
  double u1 = m.first;
  double u2 = is_mirror () ? -m.second : m.second;
  double n11 = m_m11 / u1;
  double n12 = m_m12 / u2;
  double n21 = m_m21 / u1;
  double n22 = m_m22 / u2;

  double fsin_a = 0.5 * sqrt ((n21 + n12) * (n21 + n12) + (n11 - n22) * (n11 - n22));
  double fcos_a = 0.5 * sqrt ((n21 - n12) * (n21 - n12) + (n11 + n22) * (n11 + n22));
  if ((n21 - n12) * (n22 - n11) < -1e-10 || (n21 + n12) * (n11 + n22) < -1e-10) {
    fsin_a = -fsin_a;
  }

  return 180.0 * atan2 (fsin_a, fcos_a) / M_PI;
}

template <class C>
matrix_2d<C>
matrix_2d<C>::shear (double a)
{
  a *= M_PI / 180.0;
  double cos_a = cos (a);
  double sin_a = sin (a);
  double f = 1.0 / sqrt (cos_a * cos_a - sin_a * sin_a);
  return Matrix2d (f * cos_a, f * sin_a, f * sin_a, f * cos_a);
}

template <class C>
bool
matrix_2d<C>::is_ortho () const
{
  return fabs (m_m11 * m_m12 + m_m21 * m_m22) < 1e-10 && fabs (m_m11 * m_m12) < 1e-10 && fabs (m_m21 * m_m22) < 1e-10; 
}

template <class C>
bool
matrix_2d<C>::is_unity () const
{
  static matrix_2d<C> u;
  return equal (u);
}

template <class C>
bool
matrix_2d<C>::equal (const matrix_2d<C> &d) const
{
  return fabs (m_m11 - d.m_m11) < 1e-10 && fabs (m_m12 - d.m_m12) < 1e-10 && 
         fabs (m_m21 - d.m_m21) < 1e-10 && fabs (m_m22 - d.m_m22) < 1e-10;
}

template <class C>
bool
matrix_2d<C>::less (const matrix_2d<C> &d) const
{
  if (fabs (m_m11 - d.m_m11) > 1e-10) {
    return m_m11 < d.m_m11;
  }
  if (fabs (m_m12 - d.m_m12) > 1e-10) {
    return m_m12 < d.m_m12;
  }
  if (fabs (m_m21 - d.m_m21) > 1e-10) {
    return m_m21 < d.m_m21;
  }
  if (fabs (m_m22 - d.m_m22) > 1e-10) {
    return m_m22 < d.m_m22;
  }
  return false;
}

template class matrix_2d<db::Coord>;
template class matrix_2d<db::DCoord>;

// --------------------------------------------------------------------------------------------

template <class C>
double
matrix_3d<C>::det () const
{
  double d = 0.0;
  for (int i0 = 0; i0 < 3; ++i0) {
    for (int j = 0; j < 2; ++j) {
      int i1 = (i0 + j + 1) % 3;
      int i2 = (i1 + 1) % 3;
      double s = (((i0 + i1 + i2) % 2) == 0) ? -1.0 : 1.0;
      d += s * m_m [0][i0] * m_m [1][i1] * m_m [2][i2];
    }
  }
  return d;
}

template <class C>
db::vector<C>
matrix_3d<C>::trans (const db::point<C> &p, const db::vector<C> &v) const
{
  double t[2][2];
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      t[i][j] = (m_m[i][j] * m_m[2][1 - j] - m_m[i][1 - j] * m_m[2][j]) * (j == 0 ? p.y() : p.x()) + (m_m[2][2] * m_m[i][j] - m_m[i][2] * m_m[2][j]);
    }
  }
  return db::vector<C>(v.x() * t[0][0] + v.y() * t[0][1], v.x() * t[1][0] + v.y() * t[1][1]);
}

template <class C>
bool
matrix_3d<C>::can_transform (const db::point<C> &p) const
{
  double r[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; ++i) {
    r[i] = m_m[i][0] * p.x() + m_m[i][1] * p.y() + m_m[i][2];
  }

  return (r[2] > (std::abs (r[0]) + std::abs (r[1])) * 1e-10);
}

template <class C>
db::point<C>
matrix_3d<C>::trans (const db::point<C> &p) const
{
  double r[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; ++i) {
    r[i] = m_m[i][0] * p.x() + m_m[i][1] * p.y() + m_m[i][2];
  }

  //  safe approximation to the forbidden area where z <= 0
  double z = std::max (r [2], (std::abs (r[0]) + std::abs (r[1])) * 1e-10);
  return db::point<C> (r[0] / z, r[1] / z);
}

template <class C>
matrix_3d<C>
matrix_3d<C>::inverted () const
{
  double m[3][3];
  matrix_3d<C> r (1.0);

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      m[i][j] = m_m[i][j];
    }
  }

  for (int i = 0; i < 2; ++i) {
    for (int j = 2; j > i; --j) {
      if (std::abs (m[j][i]) > std::abs (m[j - 1][i])) {
        for (int k = 0; k < 3; ++k) {
          std::swap (m[j][k], m[j - 1][k]);
          std::swap (r.m_m[j][k], r.m_m[j - 1][k]);
        }
      }
    }
  }

  for (int i = 0; i < 3; ++i) {
    for (int j = i + 1; j < 3; ++j) {
      double f = m[j][i] / m[i][i];
      for (int k = 0; k < 3; ++k) {
        m[j][k] -= f * m[i][k];
        r.m_m[j][k] -= f * r.m_m[i][k];
      }
    }
    double f = 1.0 / m[i][i];
    for (int k = 0; k < 3; ++k) {
      m[i][k] *= f;
      r.m_m[i][k] *= f;
    }
  }

  for (int i = 2; i > 0; --i) {
    for (int j = 0; j < i; ++j) {
      double f = m[j][i];
      for (int k = 0; k < 3; ++k) {
        r.m_m[j][k] -= f * r.m_m[i][k];
      }
    }
  }

  return r;
}

template <class C>
db::vector<C>
matrix_3d<C>::disp () const
{
  return db::vector<C> (m_m[0][2] / m_m[2][2], m_m[1][2] / m_m[2][2]);
}

template <class C>
double
matrix_3d<C>::perspective_tilt_x (double z) const
{
  db::vector<C> d = disp ();
  db::matrix_3d<C> m = db::matrix_3d<C>::disp (-d) * *this;
  return 180 * atan (z * (m.m ()[2][0] * m.m ()[1][1] - m.m ()[2][1] * m.m ()[1][0]) / (m.m ()[0][0] * m.m ()[1][1] - m.m ()[0][1] * m.m ()[1][0])) / M_PI;
}

template <class C>
double
matrix_3d<C>::perspective_tilt_y (double z) const
{
  db::vector<C> d = disp ();
  db::matrix_3d<C> m = db::matrix_3d<C>::disp (-d) * *this;
  return 180 * atan (z * (m.m ()[2][1] * m.m ()[0][0] - m.m ()[2][0] * m.m ()[0][1]) / (m.m ()[0][0] * m.m ()[1][1] - m.m ()[0][1] * m.m ()[1][0])) / M_PI;
}

template <class C>
bool
matrix_3d<C>::has_perspective () const
{
  return fabs (m_m[2][0]) + fabs (m_m[2][1]) > 1e-10;
}

template <class C>
matrix_3d<C>
matrix_3d<C>::perspective (double tx, double ty, double z)
{
  tx *= M_PI / 180.0;
  ty *= M_PI / 180.0;
  return matrix_3d<C> (1.0, 0.0, 0.0, 1.0, 0.0, 0.0, tan (tx) / z, tan (ty) / z);
}

template <class C>
matrix_2d<C>
matrix_3d<C>::m2d () const
{
  db::vector<C> d = disp ();
  db::matrix_3d<C> m = db::matrix_3d<C>::disp (-d) * *this;

  if (has_perspective ()) {
    m = matrix_3d<C>::perspective (-perspective_tilt_x (1.0), -perspective_tilt_y (1.0), 1.0) * m;
  }

  return matrix_2d<C> (m.m_m[0][0] / m.m_m[2][2], m.m_m[0][1] / m.m_m[2][2], m.m_m[1][0] / m.m_m[2][2], m.m_m[1][1] / m.m_m[2][2]);
}

template <class C>
std::string
matrix_3d<C>::to_string () const
{
  return tl::sprintf ("(%.12g,%.12g,%.12g)", mnorm (m_m[0][0]), mnorm (m_m[0][1]), mnorm (m_m[0][2])) + " " 
       + tl::sprintf ("(%.12g,%.12g,%.12g)", mnorm (m_m[1][0]), mnorm (m_m[1][1]), mnorm (m_m[1][2])) + " "
       + tl::sprintf ("(%.12g,%.12g,%.12g)", mnorm (m_m[2][0]), mnorm (m_m[2][1]), mnorm (m_m[2][2]));
}

template <class C>
bool
matrix_3d<C>::is_ortho () const
{
  return ! has_perspective () && m2d ().is_ortho ();
}

template <class C>
bool
matrix_3d<C>::is_unity () const
{
  static matrix_3d<C> u;
  return equal (u);
}

template <class C>
bool
matrix_3d<C>::equal (const matrix_3d<C> &d) const
{
  for (unsigned int i = 0; i < 3; ++i) {
    for (unsigned int j = 0; j < 3; ++j) {
      if (fabs (m_m[i][j] - d.m_m[i][j]) > 1e-10) {
        return false;
      }
    }
  }
  return true;
}

template <class C>
bool
matrix_3d<C>::less (const matrix_3d<C> &d) const
{
  for (unsigned int i = 0; i < 3; ++i) {
    for (unsigned int j = 0; j < 3; ++j) {
      if (fabs (m_m[i][j] - d.m_m[i][j]) > 1e-10) {
        return m_m[i][j] < d.m_m[i][j];
      }
    }
  }
  return false;
}

template class matrix_3d<db::Coord>;
template class matrix_3d<db::DCoord>;

// --------------------------------------------------------------------------------------------

/**
 *  @brief Fits a vector set to another using a linear transformation that is a linear combination of two: M = a*A + b*B
 *
 *  It returns the best-fit parameters a and b. 
 *
 *  @param am The matrix A
 *  @param bm The matrix B
 *  @param p The initial vector set 
 *  @param q The vector set to fit M*p[i] to
 */
static bool
fit_point_set_with_linear_combination (double &a, double &b, const Matrix2d &am, const Matrix2d &bm, const std::vector<db::DVector> &q, const std::vector<db::DVector> &p)
{
  double maa = 0.0, mbaab = 0.0, mbb = 0.0, ca = 0.0, cb = 0.0;

  for (size_t i = 0; i < p.size (); ++i) {

    db::DVector ap = am * p[i];
    db::DVector bp = bm * p[i];

    maa += ap.sq_double_length ();
    mbaab += db::sprod (bp, ap);
    mbb += bp.sq_double_length ();
    ca += db::sprod (q [i], ap);
    cb += db::sprod (q [i], bp);

  }

  Matrix2d m (maa, mbaab, mbaab, mbb);
  if (fabs (m.det ()) < 1e-10) {
    //  fit not possible
    return false;
  } else {

    db::DVector r = Matrix2d (maa, mbaab, mbaab, mbb).inverted () * db::DVector (ca, cb);
    a = r.x ();
    b = r.y ();
    return true;

  }
}

/**
 *  @brief Computes the cost value for a given matrix (sum over the square distances)
 *
 *  @param m The matrix
 *  @param p The initial vector set 
 *  @param q The vector set to fit M*p[i] to
 */
static double
compute_distance (const Matrix2d &m, const std::vector<db::DVector> &q, const std::vector<db::DVector> &p)
{
  double d = 0.0;
  for (size_t i = 0; i < p.size (); ++i) {
    d += (q[i] - m * p[i]).sq_double_length ();
  }
  return d;
}

void 
adjust_matrix (Matrix2d &matrix, db::DVector &disp, const std::vector <db::DPoint> &landmarks_before, const std::vector <db::DPoint> &landmarks_after, MatrixAdjustFlags::Flags flags, int fixed_point)
{
  tl_assert (landmarks_before.size () == landmarks_after.size ());

  if (flags > MatrixAdjustFlags::Shear) {
    flags = MatrixAdjustFlags::Shear;
  }

  if (landmarks_before.size () == 0) {
    flags = MatrixAdjustFlags::None;
  } else if (landmarks_before.size () == 1) {
    flags = std::min (MatrixAdjustFlags::Displacement, flags);
  } else if (landmarks_before.size () == 2) {
    flags = std::min (MatrixAdjustFlags::Magnification, flags);
  } else if (landmarks_before.size () == 3) {
    flags = std::min (MatrixAdjustFlags::Shear, flags);
  }

  //  Don't do anything in "none" mode.
  if (flags == MatrixAdjustFlags::None) {
    return;
  }

  //  Determine initial center of weight or fixed displacement (if there is a fixed point) before and after.
  //  Extract the displacement so we have a 0,0-centered vector set.
  //  Use the initial transformation as a basis for the adjustment.

  db::DPoint dp;

  if (fixed_point >= 0 && fixed_point < int (landmarks_before.size ())) {
    dp = matrix * landmarks_before [fixed_point] + disp;
  } else {
    for (std::vector<db::DPoint>::const_iterator p = landmarks_before.begin (); p != landmarks_before.end (); ++p) {
      dp += matrix * db::DVector (*p) + disp;
    }
    dp *= 1.0 / double (landmarks_before.size ());
  }

  std::vector<db::DVector> p;
  p.reserve (landmarks_before.size ());
  for (std::vector<db::DPoint>::const_iterator pp = landmarks_before.begin (); pp != landmarks_before.end (); ++pp) {
    p.push_back ((matrix * *pp + disp) - dp);
  }

  db::DPoint dq;
  if (fixed_point >= 0 && fixed_point < int (landmarks_after.size ())) {
    dq = landmarks_after [fixed_point];
  } else {
    for (std::vector<db::DPoint>::const_iterator q = landmarks_after.begin (); q != landmarks_after.end (); ++q) {
      dq += *q - db::DPoint ();
    }
    dq *= 1.0 / double (landmarks_after.size ());
  }

  std::vector<db::DVector> q;
  q.reserve (landmarks_after.size ());
  for (std::vector<db::DPoint>::const_iterator qq = landmarks_after.begin (); qq != landmarks_after.end (); ++qq) {
    q.push_back (*qq - dq);
  }

  //  In the special case of RotationMirror and exactly 3 points with a fixed point adjust the 
  //  other so they form normal vectors to the fixed point. In that case, the intention is very likely to 
  //  define two axes with their length being not important.
  if (flags == MatrixAdjustFlags::RotationMirror && landmarks_after.size () == 3 && fixed_point >= 0 && fixed_point < 3) {
    for (int i = 0; i < 3; ++i) {
      if (i != fixed_point) {
        double n;
        n = p [i].double_length ();
        if (fabs (n) > 1e-6) {
          p [i] *= 1.0 / n;
        }
        n = q [i].double_length ();
        if (fabs (n) > 1e-6) {
          q [i] *= 1.0 / n;
        }
      }
    }
  }

  //  Extract a Matrix2d rotation matrix according to the extraction mode

  Matrix2d m (1.0, 0.0, 0.0, 1.0);

  if (flags == MatrixAdjustFlags::Shear) {

    //  Do a full fit

    Matrix2d pm;
    Matrix2d qm;

    for (size_t i = 0; i < p.size (); ++i) {
      double px = p [i].x ();
      double py = p [i].y ();
      double qx = q [i].x ();
      double qy = q [i].y ();
      pm += Matrix2d (px * px, px * py, py * px, py * py);
      qm += Matrix2d (qx * px, qx * py, qy * px, qy * py);
    }

    //  Fallback is Magnification mode if the fit cannot be performed
    if (fabs (pm.det ()) < 1e-10) {
      flags = MatrixAdjustFlags::Magnification;
    } else {
      m = qm * pm.inverted ();
    }

  }

  if (flags == MatrixAdjustFlags::Displacement) {

    //  we are done now.

  } else if (flags == MatrixAdjustFlags::Rotation || flags == MatrixAdjustFlags::Magnification) {

    //  Fit a general rotation matrix to the vector sets and drop the magnification part
    Matrix2d am (1.0, 0.0, 0.0, 1.0), bm (0.0, -1.0, 1.0, 0.0);
    double a = 0.0, b = 0.0;
    if (fit_point_set_with_linear_combination (a, b, am, bm, q, p) && fabs (a) + fabs (b) > 1e-6) {
      double n = flags == MatrixAdjustFlags::Magnification ? 1.0 : 1.0 / sqrt (a * a + b * b);
      m = am * (a * n) + bm * (b * n);
    }

  } else if (flags == MatrixAdjustFlags::RotationMirror || flags == MatrixAdjustFlags::MagnificationMirror) {

    //  Same as before but perform two tries (with and without mirror) and don't drop the magnification part 
    //  if magnification adjustment is requested.

    Matrix2d am1 (1.0, 0.0, 0.0, 1.0), bm1 (0.0, -1.0, 1.0, 0.0);
    double a1 = 0.0, b1 = 0.0;
    double d1 = std::numeric_limits<double>::max ();
    if (fit_point_set_with_linear_combination (a1, b1, am1, bm1, q, p) && fabs (a1) + fabs (b1) > 1e-6) {
      d1 = compute_distance (am1 * a1 + bm1 * b1, q, p);
    }

    Matrix2d am2 (1.0, 0.0, 0.0, -1.0), bm2 (0.0, 1.0, 1.0, 0.0);
    double a2 = 0.0, b2 = 0.0;
    double d2 = std::numeric_limits<double>::max ();
    if (fit_point_set_with_linear_combination (a2, b2, am2, bm2, q, p) && fabs (a2) + fabs (b2) > 1e-6) {
      d2 = compute_distance (am2 * a2 + bm2 * b2, q, p);
    }

    if (d1 < std::numeric_limits<double>::max () || d2 < std::numeric_limits<double>::max ()) {
      if (d1 < d2 + 1e-10) {
        double n = flags == MatrixAdjustFlags::MagnificationMirror ? 1.0 : 1.0 / sqrt (a1 * a1 + b1 * b1);
        m = am1 * (a1 * n) + bm1 * (b1 * n);
      } else {
        double n = flags == MatrixAdjustFlags::MagnificationMirror ? 1.0 : 1.0 / sqrt (a2 * a2 + b2 * b2);
        m = am2 * (a2 * n) + bm2 * (b2 * n);
      }
    }

  }

  //  compute the final transformation
  disp = (dq - db::DPoint ()) + m * (disp - (dp - db::DPoint ()));
  matrix = m * matrix;
}

void 
adjust_matrix (Matrix3d &matrix, const std::vector <db::DPoint> &landmarks_before, const std::vector <db::DPoint> &landmarks_after, MatrixAdjustFlags::Flags flags, int fixed_point)
{
  tl_assert (landmarks_before.size () == landmarks_after.size ());

  if (flags > MatrixAdjustFlags::Perspective) {
    flags = MatrixAdjustFlags::Perspective;
  }

  if (landmarks_before.size () == 0) {
    flags = MatrixAdjustFlags::None;
  } else if (landmarks_before.size () == 1) {
    flags = std::min (MatrixAdjustFlags::Displacement, flags);
  } else if (landmarks_before.size () == 2) {
    flags = std::min (MatrixAdjustFlags::Magnification, flags);
  } else if (landmarks_before.size () == 3) {
    flags = std::min (MatrixAdjustFlags::Shear, flags);
  }

  if (flags < MatrixAdjustFlags::Perspective) {

    if (matrix.has_perspective ()) {

      std::vector<db::DPoint> p;
      p.reserve (landmarks_before.size ());
      for (std::vector<db::DPoint>::const_iterator pp = landmarks_before.begin (); pp != landmarks_before.end (); ++pp) {
        p.push_back (matrix * *pp);
      }

      Matrix2d m2d (1.0);
      db::DVector d2d;
      adjust_matrix (m2d, d2d, p, landmarks_after, flags, fixed_point);

      matrix = Matrix3d::disp (d2d) * Matrix3d (m2d) * matrix;

    } else {

      Matrix2d m2d (matrix.m2d ());
      db::DVector d2d (matrix.disp ());
      adjust_matrix (m2d, d2d, landmarks_before, landmarks_after, flags, fixed_point);

      matrix = Matrix3d::disp (d2d) * Matrix3d (m2d);

    }

  } else {

    //  Determine initial center of weight or fixed displacement (if there is a fixed point) before and after.
    //  Extract the displacement so we have a 0,0-centered vector set.
    //  Use the initial transformation as a basis for the adjustment.

    db::DPoint dp;

    if (fixed_point >= 0 && fixed_point < int (landmarks_before.size ())) {
      dp = matrix * landmarks_before [fixed_point];
    }

    std::vector<db::DVector> p;
    p.reserve (landmarks_before.size ());
    for (std::vector<db::DPoint>::const_iterator pp = landmarks_before.begin (); pp != landmarks_before.end (); ++pp) {
      p.push_back (matrix * *pp - dp);
    }

    db::DPoint dq;
    if (fixed_point >= 0 && fixed_point < int (landmarks_after.size ())) {
      dq = landmarks_after [fixed_point];
    }

    std::vector<db::DVector> q;
    q.reserve (landmarks_after.size ());
    for (std::vector<db::DPoint>::const_iterator qq = landmarks_after.begin (); qq != landmarks_after.end (); ++qq) {
      q.push_back (*qq - dq);
    }

    //  Perform the fit of m11, m12, m13, m21, m22, m23, m31, m32.
    //  We needs to solve a set of 8 linear equations whose coefficients we collect in l and whose right side we collect in r.

    double l[8][8];
    double r[8];

    for (int i = 0; i < 8; ++i) {
      r[i] = 0.0;
      for (int j = 0; j < 8; ++j) {
        l[i][j] = 0.0;
      }
    }

    for (size_t n = 0; n < landmarks_after.size (); ++n) {

      l[0][0] += p[n].x() * p[n].x();
      l[0][1] += p[n].x() * p[n].y();
      l[0][2] += p[n].x();
      l[0][6] -= q[n].x() * p[n].x() * p[n].x();
      l[0][7] -= q[n].x() * p[n].x() * p[n].y();
      r[0] += q[n].x() * p[n].x();

      l[1][0] += p[n].y() * p[n].x();
      l[1][1] += p[n].y() * p[n].y();
      l[1][2] += p[n].y();
      l[1][6] -= q[n].x() * p[n].y() * p[n].x();
      l[1][7] -= q[n].x() * p[n].y() * p[n].y();
      r[1] += q[n].x() * p[n].y();

      l[2][0] += p[n].x();
      l[2][1] += p[n].y();
      l[2][2] += 1.0;
      l[2][6] -= q[n].x() * p[n].x();
      l[2][7] -= q[n].x() * p[n].y();
      r[2] += q[n].x();

      l[3][3] += p[n].x() * p[n].x();
      l[3][4] += p[n].x() * p[n].y();
      l[3][5] += p[n].x();
      l[3][6] -= q[n].y() * p[n].x() * p[n].x();
      l[3][7] -= q[n].y() * p[n].x() * p[n].y();
      r[3] += q[n].y() * p[n].x();

      l[4][3] += p[n].y() * p[n].x();
      l[4][4] += p[n].y() * p[n].y();
      l[4][5] += p[n].y();
      l[4][6] -= q[n].y() * p[n].y() * p[n].x();
      l[4][7] -= q[n].y() * p[n].y() * p[n].y();
      r[4] += q[n].y() * p[n].y();

      l[5][3] += p[n].x();
      l[5][4] += p[n].y();
      l[5][5] += 1.0;
      l[5][6] -= q[n].y() * p[n].x();
      l[5][7] -= q[n].y() * p[n].y();
      r[5] += q[n].y();

      l[6][0] += q[n].x() * p[n].x() * p[n].x();
      l[6][1] += q[n].x() * p[n].x() * p[n].y();
      l[6][2] += q[n].x() * p[n].x();
      l[6][3] += q[n].y() * p[n].x() * p[n].x();
      l[6][4] += q[n].y() * p[n].x() * p[n].y();
      l[6][5] += q[n].y() * p[n].x();
      l[6][6] -= q[n].sq_double_length () * p[n].x() * p[n].x();
      l[6][7] -= q[n].sq_double_length () * p[n].x() * p[n].y();
      r[6] += q[n].sq_double_length () * p[n].x();

      l[7][0] += q[n].x() * p[n].y() * p[n].x();
      l[7][1] += q[n].x() * p[n].y() * p[n].y();
      l[7][2] += q[n].x() * p[n].y();
      l[7][3] += q[n].y() * p[n].y() * p[n].x();
      l[7][4] += q[n].y() * p[n].y() * p[n].y();
      l[7][5] += q[n].y() * p[n].y();
      l[7][6] -= q[n].sq_double_length () * p[n].y() * p[n].x();
      l[7][7] -= q[n].sq_double_length () * p[n].y() * p[n].y();
      r[7] += q[n].sq_double_length () * p[n].y();

    }

    //  Now solve the equation system

    for (int i = 0; i < 7; ++i) {
      for (int j = 7; j > i; --j) {
        if (std::abs (l[j][i]) > std::abs (l[j - 1][i])) {
          for (int k = 0; k < 8; ++k) {
            std::swap (l[j][k], l[j - 1][k]);
          }
          std::swap (r[j], r[j - 1]);
        }
      }
    }

    for (int i = 0; i < 8; ++i) {
      for (int j = i + 1; j < 8; ++j) {
        double f = l[j][i] / l[i][i];
        for (int k = 0; k < 8; ++k) {
          l[j][k] -= f * l[i][k];
        }
        r[j] -= f * r[i];
      }
      double f = 1.0 / l[i][i];
      for (int k = 0; k < 8; ++k) {
        l[i][k] *= f;
      }
      r[i] *= f;
    }

    for (int i = 7; i > 0; --i) {
      for (int j = 0; j < i; ++j) {
        r[j] -= l[j][i] * r[i];
      }
    }

    //  compute the final transformation
    matrix = Matrix3d::disp (dq - db::DPoint ()) * Matrix3d (r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 1.0) * Matrix3d::disp (db::DPoint () - dp) * matrix;

  }

}

}

namespace tl
{
  template<class C> bool test_extractor_impl_matrix2d (tl::Extractor &ex, db::matrix_2d<C> &m)
  {
    double m11 = 0.0, m12 = 0.0, m21 = 0.0, m22 = 0.0;

    if (! ex.test ("(")) {
      return false;
    }
    if (! ex.try_read (m11)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m12)) {
      return false;
    }
    if (! ex.test (")")) {
      return false;
    }

    if (! ex.test ("(")) {
      return false;
    }
    if (! ex.try_read (m21)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m22)) {
      return false;
    }
    if (! ex.test (")")) {
      return false;
    }

    m = db::matrix_2d<C> (m11, m12, m21, m22);
    return true;
  }

  template<class C> void extractor_impl_matrix2d (tl::Extractor &ex, db::matrix_2d<C> &m)
  {
    if (! test_extractor_impl (ex, m)) {
      ex.error (tl::to_string (tr ("Expected a 2d matrix specification")));
    }
  }

  template<class C> bool test_extractor_impl_matrix3d (tl::Extractor &ex, db::matrix_3d<C> &m)
  {
    double m11 = 0.0, m12 = 0.0, m13 = 0.0, m21 = 0.0, m22 = 0.0, m23 = 0.0, m31 = 0.0, m32 = 0.0, m33 = 0.0;

    if (! ex.test ("(")) {
      return false;
    }
    if (! ex.try_read (m11)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m12)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m13)) {
      return false;
    }
    if (! ex.test (")")) {
      return false;
    }

    if (! ex.test ("(")) {
      return false;
    }
    if (! ex.try_read (m21)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m22)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m23)) {
      return false;
    }
    if (! ex.test (")")) {
      return false;
    }

    if (! ex.test ("(")) {
      return false;
    }
    if (! ex.try_read (m31)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m32)) {
      return false;
    }
    if (! ex.test (",")) {
      return false;
    }
    if (! ex.try_read (m33)) {
      return false;
    }
    if (! ex.test (")")) {
      return false;
    }

    m = db::matrix_3d<C> (m11, m12, m13, m21, m22, m23, m31, m32, m33);
    return true;
  }

  template<class C> void extractor_impl_matrix3d (tl::Extractor &ex, db::matrix_3d<C> &m)
  {
    if (! test_extractor_impl (ex, m)) {
      ex.error (tl::to_string (tr ("Expected a 3d matrix specification")));
    }
  }

  template<> void extractor_impl<db::matrix_2d<db::Coord> > (tl::Extractor &ex, db::matrix_2d<db::Coord> &m)
  {
    extractor_impl_matrix2d (ex, m);
  }

  template<> void extractor_impl<db::matrix_2d<db::DCoord> > (tl::Extractor &ex, db::matrix_2d<db::DCoord> &m)
  {
    extractor_impl_matrix2d (ex, m);
  }

  template<> void extractor_impl<db::matrix_3d<db::Coord> > (tl::Extractor &ex, db::matrix_3d<db::Coord> &m)
  {
    extractor_impl_matrix3d (ex, m);
  }

  template<> void extractor_impl<db::matrix_3d<db::DCoord> > (tl::Extractor &ex, db::matrix_3d<db::DCoord> &m)
  {
    extractor_impl_matrix3d (ex, m);
  }

  template<> bool test_extractor_impl<db::matrix_2d<db::Coord> > (tl::Extractor &ex, db::matrix_2d<db::Coord> &m)
  {
    return test_extractor_impl_matrix2d (ex, m);
  }

  template<> bool test_extractor_impl<db::matrix_2d<db::DCoord> > (tl::Extractor &ex, db::matrix_2d<db::DCoord> &m)
  {
    return test_extractor_impl_matrix2d (ex, m);
  }

  template<> bool test_extractor_impl<db::matrix_3d<db::Coord> > (tl::Extractor &ex, db::matrix_3d<db::Coord> &m)
  {
    return test_extractor_impl_matrix3d (ex, m);
  }

  template<> bool test_extractor_impl<db::matrix_3d<db::DCoord> > (tl::Extractor &ex, db::matrix_3d<db::DCoord> &m)
  {
    return test_extractor_impl_matrix3d (ex, m);
  }
}


