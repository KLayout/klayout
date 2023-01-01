
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


#ifndef HDR_dbMatrix
#define HDR_dbMatrix

#include "dbCommon.h"

#include <dbVector.h>
#include <dbPoint.h>

#include <string>
#include <algorithm>

namespace tl {
  class Extractor;
}

namespace db
{

/**
 *  @brief A class representing a 2d matrix, mainly to represent a rotation or shear transformation of 2d vectors
 */
template <class C>
class DB_PUBLIC matrix_2d
{
public:
  /**
   *  @brief typedefs for compatibility with the other transformations
   */
  typedef C target_coord_type;
  typedef C coord_type;
  typedef db::point<C> displacement_type;
  typedef matrix_2d<C> inverse_trans;

  /**
   *  @brief Default ctor
   *
   *  Creates a null matrix
   */
  matrix_2d ()
    : m_m11 (0.0), m_m12 (0.0), m_m21 (0.0), m_m22 (0.0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Full ctor
   *
   *  Creates a matrix (m11, m12) (m21, m22)
   */
  matrix_2d (double m11, double m12, double m21, double m22)
    : m_m11 (m11), m_m12 (m12), m_m21 (m21), m_m22 (m22)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Full ctor
   *
   *  Creates a matrix (m11, m12) (m21, m22)
   */
  template <class D>
  matrix_2d (const matrix_2d<D> &m)
    : m_m11 (m.m11 ()), m_m12 (m.m12 ()), m_m21 (m.m21 ()), m_m22 (m.m22 ())
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Scalar ctor
   *
   *  Creates a matrix (d, 0) (0, d)
   */
  matrix_2d (double d)
    : m_m11 (d), m_m12 (0.0), m_m21 (0.0), m_m22 (d)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Diagonal ctor
   *
   *  Creates a matrix (d1, 0) (0, d2)
   */
  matrix_2d (double d1, double d2)
    : m_m11 (d1), m_m12 (0.0), m_m21 (0.0), m_m22 (d2)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Make a matrix from a transformation
   */
  template <class Tr> 
  matrix_2d (const Tr &t)
  {
    *this = t.to_matrix2d ();
  }

  /**
   *  @brief Add operator
   */
  template <class D>
  matrix_2d operator+ (const matrix_2d<D> &other) const
  {
    matrix_2d m (*this);
    m += other;
    return m;
  }

  /**
   *  @brief Add to operator
   */
  template <class D>
  matrix_2d &operator+= (const matrix_2d<D> &other)
  {
    m_m11 += other.m_m11;
    m_m12 += other.m_m12;
    m_m21 += other.m_m21;
    m_m22 += other.m_m22;
    return *this;
  }

  /**
   *  @brief Product of two matrices
   */
  template <class D>
  matrix_2d operator* (const matrix_2d<D> &other) const
  {
    return matrix_2d (m_m11 * other.m_m11 + m_m12 * other.m_m21,
                      m_m11 * other.m_m12 + m_m12 * other.m_m22,
                      m_m21 * other.m_m11 + m_m22 * other.m_m21,
                      m_m21 * other.m_m12 + m_m22 * other.m_m22);
  }

  /**
   *  @brief Multiply another to this matrix
   */
  matrix_2d &operator*= (const matrix_2d &other)
  {
    *this = (*this * other);
    return *this;
  }

  /**
   *  @brief Multiply with a scalar
   */
  matrix_2d operator* (double d) const
  {
    matrix_2d m (*this);
    m *= d;
    return m;
  }

  /**
   *  @brief Multiply a scalar to this matrix
   */
  matrix_2d &operator*= (double d)
  {
    m_m11 *= d;
    m_m12 *= d;
    m_m21 *= d;
    m_m22 *= d;
    return *this;
  }

  /**
   *  @brief Transformation of a vector
   */
  db::vector<C> operator* (const db::vector<C> &v) const
  {
    return db::vector<C> (m_m11 * v.x () + m_m12 * v.y (), m_m21 * v.x () + m_m22 * v.y ());
  }

  /**
   *  @brief "trans" alias for compatibility with the other transformations
   */
  db::vector<C> trans (const db::vector<C> &p) const
  {
    return operator* (p);
  }

  /**
   *  @brief "operator()" alias for compatibility with the other transformations
   */
  db::vector<C> operator() (const db::vector<C> &p) const
  {
    return operator* (p);
  }

  /**
   *  @brief Transformation of a point
   */
  db::point<C> operator* (const db::point<C> &v) const
  {
    return db::point<C> (m_m11 * v.x () + m_m12 * v.y (), m_m21 * v.x () + m_m22 * v.y ());
  }

  /**
   *  @brief "trans" alias for compatibility with the other transformations
   */
  db::point<C> trans (const db::point<C> &p) const
  {
    return operator* (p);
  }

  /**
   *  @brief "operator()" alias for compatibility with the other transformations
   */
  db::point<C> operator() (const db::point<C> &p) const
  {
    return operator* (p);
  }

  /**
   *  @brief Return the transposed matrix
   */
  matrix_2d<C> transposed () const
  {
    return matrix_2d<C> (m_m11, m_m21, m_m12, m_m22);
  }

  /**
   *  @brief In-place transpose
   */
  void transpose () 
  {
    std::swap (m_m21, m_m12);
  }

  /**
   *  @brief Get determinant
   */
  double det () const
  {
    return m_m11 * m_m22 - m_m12 * m_m21;
  }

  /**
   *  @brief Return the inverted matrix
   */
  matrix_2d<C> inverted () const
  {
    matrix_2d<C> m (*this);
    m.invert ();
    return m;
  }

  /**
   *  @brief In-place invert
   */
  void invert () 
  {
    double d = det ();
    std::swap (m_m11, m_m22);
    std::swap (m_m12, m_m12);
    m_m11 /= d; 
    m_m12 /= -d; 
    m_m21 /= -d; 
    m_m22 /= d; 
  }

  /**
   *  @brief m11 element accessor
   */
  double m11 () const { return m_m11; }

  /**
   *  @brief m12 element accessor
   */
  double m12 () const { return m_m12; }

  /**
   *  @brief m21 element accessor
   */
  double m21 () const { return m_m21; }

  /**
   *  @brief m22 element accessor
   */
  double m22 () const { return m_m22; }

  /**
   *  @brief Return the magnification component of the matrix
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the x and y magnification
   *  components. The order of the execution is mirror, magnification, shear and rotation.
   */
  std::pair<double, double> mag2 () const;

  /**
   *  @brief For compatibility with other transformations
   */
  double mag () const
  {
    return mag2 ().first;
  }

  /**
   *  @brief Return the x magnification component of the matrix
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the x magnification
   *  component. The order of the execution is mirror, magnification, shear and rotation.
   */
  double mag_x () const
  {
    return mag2 ().first;
  }

  /**
   *  @brief Return the y magnification component of the matrix
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the y magnification
   *  component. The order of the execution is mirror, magnification, shear and rotation.
   */
  double mag_y () const
  {
    return mag2 ().second;
  }

  /**
   *  @brief Create the magnification matrix
   *
   *  @param mx The x magnification 
   *  @param my The y magnification
   */
  static matrix_2d<C> mag (double mx, double my)
  {
    return matrix_2d<C> (mx, 0.0, 0.0, my);
  }

  /**
   *  @brief Create the magnification matrix with isotropic magnification
   *
   *  @param m The magnification 
   */
  static matrix_2d<C> mag (double m)
  {
    return matrix_2d<C> (m, 0.0, 0.0, m);
  }

  /**
   *  @brief A dummy displacement accessor (matrix2d does not have a displacement)
   */
  db::vector<coord_type> disp () const
  {
    return db::vector<coord_type> ();
  }

  /**
   *  @brief For compatibility with other transformations
   */
  coord_type ctrans (coord_type c) const
  {
    return db::coord_traits<coord_type>::rounded (mag2 ().first * c);
  }

  /**
   *  @brief Return the mirror component of the matrix
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the mirror
   *  component (mirroring at the y axis). The order of the execution is mirror, magnification, shear and rotation.
   *  The mirror base transformation is M(mirror) = (1, 0 | 0, -1)
   */
  bool is_mirror () const
  {
    return det () < 0.0;
  }

  /**
   *  @brief Create the mirror matrix
   */
  static matrix_2d<C> mirror (bool m)
  {
    return matrix_2d<C> (1.0, 0.0, 0.0, m ? -1.0 : 1.0);
  }

  /**
   *  @brief Determine the rotation component of the matrix and return the angle in degree
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the rotation angle.
   *  The rotation base transformation is M(a) = (cos(a), -sin(a) | sin(a), cos(a))
   */
  double angle () const;

  /**
   *  @brief Determine whether the matrix has a rotation component
   *
   *  Basically this is equivalent to checking angle for != 0, but faster.
   */
  bool has_rotation () const;

  /**
   *  @brief Create the rotation matrix from the given angle
   */
  static matrix_2d<C> rotation (double a);

  /**
   *  @brief Determine the shear component of the matrix and return the shear angle in degree
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the shear angle.
   *  The shear angle is the title angle of the y axis towards x and vice versa. The 
   *  area of a rectangle remains constant under shear.
   *  The shear base transformation is M(a) = (1/sqrt(cos^2(a)-sin^2(a)))*(cos(a), sin(a) | sin(a), cos(a))
   */
  double shear_angle () const;

  /**
   *  @brief Determine whether the matrix has a shear component
   *
   *  Basically this is equivalent to checking shear_angle for != 0, but faster.
   */
  bool has_shear () const;

  /**
   *  @brief Create the shear matrix from the given angle
   */
  static matrix_2d<C> shear (double a);

  /**
   *  @brief Determine whether the matrix represents an orthogonal transformation
   *
   *  This method is provided for compatibility to the other transformations.
   */
  bool is_ortho () const;

  /**
   *  @brief Determine whether the matrix represents an unit transformation
   *
   *  This method is provided for compatibility to the other transformations.
   */
  bool is_unity () const;

  /**
   *  @brief Convert to a string
   */
  std::string to_string () const;

  /**
   *  @brief A fuzzy compare operator (equal)
   */
  bool equal (const matrix_2d<C> &d) const;

  /**
   *  @brief A fuzzy compare operator (less)
   */
  bool less (const matrix_2d<C> &d) const;

private:
  double m_m11, m_m12, m_m21, m_m22;
};

typedef matrix_2d<db::DCoord> Matrix2d;
typedef matrix_2d<db::Coord> IMatrix2d;

/**
 *  @brief A class representing a 3d matrix, mainly to represent a rotation, shear or perspective transformation of 2d vectors
 */
template <class C>
class DB_PUBLIC matrix_3d
{
public:
  /**
   *  @brief typedefs for compatibility with the other transformations
   */
  typedef C target_coord_type;
  typedef C coord_type;
  typedef db::point<C> displacement_type;
  typedef matrix_3d<C> inverse_trans;

  /**
   *  @brief Default ctor
   *
   *  Creates a null matrix
   */
  matrix_3d ()
  {
    set (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  }

  /**
   *  @brief Matrix2d ctor from four components
   *
   *  Creates a matrix (m11, m12, 0) (m21, m22, 0) (0, 0, 1)
   */
  matrix_3d (double m11, double m12, double m21, double m22)
  {
    set (m11, m12, 0.0, m21, m22, 0.0, 0.0, 0.0, 1.0);
  }

  /**
   *  @brief Matrix2d ctor from nine components
   *
   *  Creates a matrix (m11, m12, m13) (m21, m22, m23) (m31, m32, m33)
   */
  matrix_3d (double m11, double m12, double m13, double m21, double m22, double m23, double m31, double m32, double m33)
  {
    set (m11, m12, m13, m21, m22, m23, m31, m32, m33);
  }

  /**
   *  @brief Full ctor
   *
   *  Creates a matrix (m11, m12) (m21, m22)
   */
  template <class D>
  matrix_3d (const matrix_3d<D> &m)
  {
    const double (&mm) [3][3] = m.m ();
    for (unsigned int i = 0; i < 3; ++i) {
      for (unsigned int j = 0; j < 3; ++j) {
        m_m [i][j] = mm [i][j];
      }
    }
  }

  /**
   *  @brief Matrix2d ctor from eight components
   *
   *  Creates a matrix (m11, m12, d1) (m21, m22, d2) (p1, p2, 1)
   */
  matrix_3d (double m11, double m12, double m21, double m22, double d1, double d2, double p1, double p2)
  {
    set (m11, m12, d1, m21, m22, d2, p1, p2, 1.0);
  }

  /**
   *  @brief Matrix2d ctor
   *
   *  Creates a matrix representing the given Matrix2d.
   */
  template <class D>
  explicit matrix_3d (const matrix_2d<D> &m)
  {
    set (m.m11 (), m.m12 (), 0.0, m.m21 (), m.m22 (), 0.0, 0.0, 0.0, 1.0);
  }

  /**
   *  @brief Make a matrix from a transformation
   */
  template <class Tr> 
  explicit matrix_3d (const Tr &t)
  {
    *this = t.to_matrix3d ();
  }

  /**
   *  @brief Scalar ctor
   *
   *  Creates a matrix (d, 0, 0) (0, d, 0) (0, 0, 1)
   */
  explicit matrix_3d (double d)
  {
    set (d, 0.0, 0.0, 0.0, d, 0.0, 0.0, 0.0, 1.0);
  }

  /**
   *  @brief Add operator
   */
  template <class D>
  matrix_3d operator+ (const matrix_3d<D> &other) const
  {
    matrix_3d m (*this);
    m += other;
    return m;
  }

  /**
   *  @brief Add to operator
   */
  template <class D>
  matrix_3d &operator+= (const matrix_3d<D> &other)
  {
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        m_m [i][j] += other.m_m [i][j];
      }
    }
    return *this;
  }

  /**
   *  @brief Product of two matrices
   */
  template <class D>
  matrix_3d operator* (const matrix_3d<D> &other) const
  {
    matrix_3d res;
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
          res.m_m [i][j] += m_m [i][k] * other.m () [k][j];
        }
      }
    }
    return res;
  }

  /**
   *  @brief Multiply another to this matrix
   */
  template <class D>
  matrix_3d &operator*= (const matrix_3d<D> &other)
  {
    *this = (*this * other);
    return *this;
  }

  /**
   *  @brief Multiply with a scalar
   */
  matrix_3d operator* (double d) const
  {
    matrix_3d m (*this);
    m *= d;
    return m;
  }

  /**
   *  @brief Multiply a scalar to this matrix
   */
  matrix_3d &operator*= (double d)
  {
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        m_m [i][j] *= d;
      }
    }
    return *this;
  }

  /**
   *  @brief Returns true, if the point can be transformed
   *
   *  A point can be transformed if the resulting point is 
   *  located in the positive z plane.
   */
  bool can_transform (const db::point<C> &p) const;

  /**
   *  @brief Transforms a vector which emerges from a certain point
   */
  db::vector<C> trans (const db::point<C> &p, const db::vector<C> &v) const;

  /**
   *  @brief Transforms a point
   */
  db::point<C> trans (const db::point<C> &p) const;

  /**
   *  @brief Transforms a vector
   *
   *  Basically the transformation of a vector is ambiguous for perspective transformation because
   *  in that case the vector will transform differently depending on the point where the
   *  vector started.
   *
   *  In this implementation we assume the vector starts at 0, 0. This at least renders this
   *  feature useful for implementing shear and anisotropic scaling.
   */
  db::vector<C> trans (const db::vector<C> &p) const
  {
    return this->trans (db::point<C> () + p) - this->trans (db::point<C> ());
  }

  /**
   *  @brief "trans" alias for compatibility with the other transformations
   */
  template <class D>
  db::point<C> trans (const db::point<D> &p) const
  {
    return trans (db::point<C> (p));
  }

  /**
   *  @brief "trans" alias for compatibility with the other transformations
   */
  template <class D>
  db::vector<C> trans (const db::vector<D> &p) const
  {
    return trans (db::vector<C> (p));
  }

  /**
   *  @brief "operator()" alias for compatibility with the other transformations
   */
  template <class D>
  db::point<C> operator() (const db::point<D> &p) const
  {
    return trans (p);
  }

  /**
   *  @brief "operator()" alias for compatibility with the other transformations
   */
  template <class D>
  db::vector<C> operator() (const db::vector<D> &p) const
  {
    return trans (p);
  }

  /**
   *  @brief Return the transposed matrix
   */
  matrix_3d transposed () const
  {
    matrix_3d res;
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        res.m_m [i][j] = m_m [j][i];
      }
    }
    return res;
  }

  /**
   *  @brief In-place transpose
   */
  void transpose () 
  {
    *this = transposed ();
  }

  /**
   *  @brief Get determinant
   */
  double det () const;

  /**
   *  @brief Return the inverted matrix
   */
  matrix_3d inverted () const;

  /**
   *  @brief In-place invert
   */
  void invert () 
  {
    *this = inverted ();
  }

  /**
   *  @brief Accessor to the internal matrix
   */
  const double (&m () const) [3][3] { return m_m; }

  /**
   *  @brief Accessor to the internal matrix
   */
  double (&m ()) [3][3] { return m_m; }

  /**
   *  @brief Return the magnification component of the matrix
   *
   *  The mag, rotation, mirror, shear, displacement and perspective components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the magnification
   *  component for both x and y direction (anisotropic magnification). The order of the execution is mirror, magnification, shear, rotation, perspective and displacement.
   */
  std::pair<double, double> mag2 () const
  {
    return m2d ().mag2 ();
  }

  /**
   *  @brief For compatibility with other transformations
   */
  double mag () const
  {
    return mag2 ().first;
  }

  /**
   *  @brief For compatibility with other transformations
   */
  coord_type ctrans (coord_type c) const
  {
    return db::coord_traits<coord_type>::rounded (mag2 ().first * c);
  }

  /**
   *  @brief Return the x magnification component of the matrix
   */
  double mag_x () const
  {
    return mag2 ().first;
  }

  /**
   *  @brief Return the y magnification component of the matrix
   */
  double mag_y () const
  {
    return mag2 ().second;
  }

  /**
   *  @brief Create the magnification matrix with isotropic magnification
   */
  static matrix_3d mag (double m)
  {
    return matrix_3d (m, 0.0, 0.0, m);
  }

  /**
   *  @brief Create the magnification matrix with anisotropic magnification
   */
  static matrix_3d mag (double mx, double my)
  {
    return matrix_3d (mx, 0.0, 0.0, my);
  }

  /**
   *  @brief Return the mirror component of the matrix
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the mirror
   *  component (mirroring at the y axis). The order of the execution is mirror, magnification, shear, rotation, perspective and displacement.
   *  The mirror base transformation is M(mirror) = (1, 0 | 0, -1)
   */
  bool is_mirror () const
  {
    return m2d ().is_mirror ();
  }

  /**
   *  @brief Create the mirror matrix
   */
  static matrix_3d mirror (bool m)
  {
    return matrix_3d (1.0, 0.0, 0.0, m ? -1.0 : 1.0);
  }

  /**
   *  @brief Determine the rotation component of the matrix and return the angle in degree
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the rotation angle.
   *  The rotation base transformation is M(a) = (cos(a), -sin(a) | sin(a), cos(a))
   */
  double angle () const
  {
    return m2d ().angle ();
  }

  /**
   *  @brief Determine whether the matrix has a rotation component
   *
   *  Basically this is equivalent to checking angle for != 0, but faster.
   */
  bool has_rotation () const
  {
    return m2d ().has_rotation ();
  }

  /**
   *  @brief Create the rotation matrix from the given angle
   */
  static matrix_3d rotation (double a)
  {
    return matrix_3d (Matrix2d::rotation (a));
  }

  /**
   *  @brief Determine the shear component of the matrix and return the shear angle in degree
   *
   *  The mag, angle, mirror and shear components can be used to decompose the matrix 
   *  into the geometrical base transformations. This member returns the shear angle.
   *  The shear angle is the title angle of the y axis towards x and vice versa. The 
   *  area of a rectangle remains constant under shear.
   *  The shear base transformation is M(a) = (1/sqrt(cos^2(a)-sin^2(a)))*(cos(a), sin(a) | sin(a), cos(a))
   */
  double shear_angle () const
  {
    return m2d ().shear_angle ();
  }

  /**
   *  @brief Determine whether the matrix has a shear component
   *
   *  Basically this is equivalent to checking shear_angle for != 0, but faster.
   */
  bool has_shear () const
  {
    return m2d ().has_shear ();
  }

  /**
   *  @brief Create the shear matrix from the given angle
   */
  static matrix_3d shear (double a)
  {
    return matrix_3d (Matrix2d::shear (a));
  } 

  /**
   *  @brief Get the x perspective tilt angle in degree
   *
   *  To achieve the same visual effect, a different tilt angle has to be chosen for 
   *  a given observer distance z. This method computes the tilt angle for the given observer
   *  distance.
   */
  double perspective_tilt_x (double z) const;

  /**
   *  @brief Get the y perspective tilt angle in degree
   *
   *  To achieve the same visual effect, a different tilt angle has to be chosen for 
   *  a given observer distance z. This method computes the tilt angle for the given observer
   *  distance.
   */
  double perspective_tilt_y (double z) const;

  /**
   *  @brief Returns true, if this matrix features perspective transformation components
   */
  bool has_perspective () const;

  /**
   *  @brief Returns the matrix for perspective transformation
   *
   *  To achieve the same visual effect, a different tilt angle has to be chosen for 
   *  a given observer distance z. This method computes the tilt angle for the given observer
   *  distance.
   *
   *  @param tx The tilt angle in x direction (around the y axis) in degree for the given observer distance.
   *  @param ty The tilt angle in y direction (around the x axis) in degree for the given observer distance.
   *  @param z The observer distance.
   */
  static matrix_3d perspective (double tx, double ty, double z);

  /**
   *  @brief Get the displacement vector component
   */
  db::vector<C> disp () const;

  /**
   *  @brief Create the mirror matrix
   */
  static matrix_3d disp (const db::vector<C> &d)
  {
    return matrix_3d (1.0, 0.0, 0.0, 1.0, d.x (), d.y (), 0.0, 0.0);
  }

  /**
   *  @brief Determine whether the matrix represents an orthogonal transformation
   *
   *  This method is provided for compatibility to the other transformations.
   */
  bool is_ortho () const;

  /**
   *  @brief Determine whether the matrix represents an unit transformation
   *
   *  This method is provided for compatibility to the other transformations.
   */
  bool is_unity () const;

  /**
   *  @brief Get the 2d matrix component (without perspective transformation or displacement)
   */
  matrix_2d<C> m2d () const;

  /**
   *  @brief Convert to a string
   */
  std::string to_string () const;

  /**
   *  @brief A fuzzy compare operator (equal)
   */
  bool equal (const matrix_3d &d) const;

  /**
   *  @brief A fuzzy compare operator (less)
   */
  bool less (const matrix_3d &d) const;

private:
  double m_m[3][3];

  void set (double m11, double m12, double m13, double m21, double m22, double m23, double m31, double m32, double m33)
  {
    m_m[0][0] = m11;
    m_m[0][1] = m12;
    m_m[0][2] = m13;
    m_m[1][0] = m21;
    m_m[1][1] = m22;
    m_m[1][2] = m23;
    m_m[2][0] = m31;
    m_m[2][1] = m32;
    m_m[2][2] = m33;
  }
};

typedef matrix_3d<db::DCoord> Matrix3d;
typedef matrix_3d<db::Coord> IMatrix3d;

/**
 *  @brief Some adjustment flags 
 *
 *  These flags can be combined to tell the adjust function which properties to
 *  adjust when the matrix of displacement are adjusted.
 */
struct MatrixAdjustFlags
{
  enum Flags
  {
    None = 0,                 //  Don't adjust anything
    Displacement = 1,         //  Adjust displacement only (needs at least one point)
    Rotation = 2,             //  Adjust displacement plus rotation (needs two points at least)
    RotationMirror = 3,       //  Adjust displacement plus rotation and allow mirror (needs three points at least)
    Magnification = 4,        //  Adjust displacement, rotation (without mirror) and magnification (needs three points at least)
    MagnificationMirror = 5,  //  Adjust displacement, rotation (plus mirror) and magnification (needs three points at least)
    Shear = 6,                //  Adjust displacement, rotation (plus mirror), magnification and shear (needs four points at least). Is equivalent to All for a 2d matrix.
    Perspective = 7,          //  Adjust displacement, rotation (plus mirror), magnification, shear and perspective (needs six points at least). Is equivalent to All for a 3d matrix.
    All = 8                   //  Adjust all (Shear for Matrix2d and Perspective for Matrix3d)
  };
};

/**
 *  @brief Adjust a matrix and displacement vector to match the given set of landmarks
 *
 *  The flags determine what property to adjust. This function tries to adjust the matrix
 *  and displacement such, that either the matrix and/or displacement are changed as 
 *  little as possible (if few landmarks are given) or that the "after" landmarks will 
 *  match as close as possible to the "before" landmarks (if the problem is overdetermined).
 *
 *  @param matrix Input and adjusted output matrix.
 *  @param disp Input and adjusted output displacement.
 *  @param landmarks_before The points before the transformation.
 *  @param landmarks_after The points after the transformation.
 *  @param flags Selects the properties to adjust.
 *  @param fixed_point The index of the fixed point (one that is definitely mapped to the target) or -1 if there is none
 */
void DB_PUBLIC adjust_matrix (Matrix2d &matrix, db::DVector &disp, const std::vector <db::DPoint> &landmarks_before, const std::vector <db::DPoint> &landmarks_after, MatrixAdjustFlags::Flags flags, int fixed_point = -1);

/**
 *  @brief Adjust a 3d matrix to match the given set of landmarks
 *
 *  The flags determine what property to adjust. This function tries to adjust the matrix
 *  such, that either the matrix is changed as little as possible (if few landmarks are given) 
 *  or that the "after" landmarks will match as close as possible to the "before" landmarks 
 *  (if the problem is overdetermined).
 *
 *  @param matrix Input and adjusted output matrix.
 *  @param landmarks_before The points before the transformation.
 *  @param landmarks_after The points after the transformation.
 *  @param flags Selects the properties to adjust.
 *  @param fixed_point The index of the fixed point (one that is definitely mapped to the target) or -1 if there is none
 */
void DB_PUBLIC adjust_matrix (Matrix3d &matrix, const std::vector <db::DPoint> &landmarks_before, const std::vector <db::DPoint> &landmarks_after, MatrixAdjustFlags::Flags flags, int fixed_point = -1);

} // namespace db

/**
 *  @brief Special extractors for the matrix
 */

namespace tl 
{
  template<> DB_PUBLIC void extractor_impl<db::matrix_2d<db::Coord> > (tl::Extractor &ex, db::matrix_2d<db::Coord> &t);
  template<> DB_PUBLIC void extractor_impl<db::matrix_2d<db::DCoord> > (tl::Extractor &ex, db::matrix_2d<db::DCoord> &t);
  template<> DB_PUBLIC void extractor_impl<db::matrix_3d<db::Coord> > (tl::Extractor &ex, db::matrix_3d<db::Coord> &t);
  template<> DB_PUBLIC void extractor_impl<db::matrix_3d<db::DCoord> > (tl::Extractor &ex, db::matrix_3d<db::DCoord> &t);

  template<> DB_PUBLIC bool test_extractor_impl<db::matrix_2d<db::Coord> > (tl::Extractor &ex, db::matrix_2d<db::Coord> &t);
  template<> DB_PUBLIC bool test_extractor_impl<db::matrix_2d<db::DCoord> > (tl::Extractor &ex, db::matrix_2d<db::DCoord> &t);
  template<> DB_PUBLIC bool test_extractor_impl<db::matrix_3d<db::Coord> > (tl::Extractor &ex, db::matrix_3d<db::Coord> &t);
  template<> DB_PUBLIC bool test_extractor_impl<db::matrix_3d<db::DCoord> > (tl::Extractor &ex, db::matrix_3d<db::DCoord> &t);
} // namespace tl

#endif

