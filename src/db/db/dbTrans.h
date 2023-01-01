
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


#ifndef HDR_dbTrans
#define HDR_dbTrans

#include "dbCommon.h"

#include "dbPoint.h"
#include "dbVector.h"
#include "dbMatrix.h"
#include "tlString.h"
#include "tlAssert.h"

#include <string>
#include <iostream>
#include <math.h>

namespace tl {
  class Extractor;
}

namespace db {

template <class I, class F, class R = double> class DB_PUBLIC_TEMPLATE complex_trans;
template <class C> class DB_PUBLIC_TEMPLATE simple_trans;
template <class C> class DB_PUBLIC_TEMPLATE disp_trans;
template <class C> class DB_PUBLIC_TEMPLATE fixpoint_trans;

/**
 *  @brief Provide the default predicates and properties for transformations for the coordinate type C
 */

template <class C>
struct default_trans
{
  typedef C coord_type;
  typedef C target_coord_type;
  typedef vector<C> displacement_type;

  /**
   *  @brief Mirror predicate
   *
   *  If this predicate is true, the transformation will first mirror the coordinates
   *  at the x-axis before applying transformations.
   */
  bool is_mirror () const
  {
    return false;
  }

  /**
   *  @brief This transformation is always unity
   */
  bool is_unity () const
  {
    return true;
  }

  /**
   *  @brief Orthogonal predicate
   *
   *  This predicate tells if the transformation is orthogonal, i.e. does only provide rotations by 
   *  multiple of 90 degree.
   */
  bool is_ortho () const
  {
    return true;
  }

  /**
   *  @brief Magnification predicate
   *
   *  This predicate tells if the transformation is magnifying
   */
  bool is_mag () const
  {
    return false;
  }

  /**
   *  @brief The default rotation code
   */
  int rot () const
  {
    return 0;
  }

  /**
   *  @brief The default rotation code
   */
  fixpoint_trans<C> fp_trans () const
  {
    return fixpoint_trans<C> (0);
  }

  /**
   *  @brief The default displacement
   */
  displacement_type disp () const
  {
    return displacement_type ();
  }
};

/**
 *  @brief A dummy unit transformation
 *
 *  This transformation is supplied in order to allow generic transformation
 *  parameters being passed a "dummy" transformation.
 *  Even though this transformation does not require a coordinate type, it is provided
 *  to fulfil the contract.
 */

template <class C>
struct unit_trans
  : public default_trans<C>
{
  typedef C coord_type;
  typedef C target_coord_type;
  typedef typename coord_traits<C>::distance_type distance_type;
  typedef vector<C> displacement_type;
  typedef unit_trans inverse_trans;

  /**
   *  @brief The default constructor (unity transformation)
   */
  unit_trans ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief Copy ctor (which basically does nothing)
   */
  unit_trans (const unit_trans<C> &)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Copy ctor (which basically does nothing)
   */
  template <class D>
  unit_trans (const unit_trans<D> &)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Downcast: does nothing
   */
  explicit unit_trans (const simple_trans<C> &)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Downcast: does nothing
   */
  explicit unit_trans (const disp_trans<C> &)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Downcast: does nothing
   */
  template <class R>
  explicit unit_trans (const complex_trans<C, C, R> &)
  {
    // .. nothing else ..
  }

  /**
   *  @brief (dummy) equality
   */
  bool operator== (unit_trans /*b*/) const
  {
    return true;
  }

  /**
   *  @brief (dummy) fuzzy equality
   */
  bool equal (unit_trans /*b*/) const
  {
    return true;
  }

  /**
   *  @brief A (fuzzy) inequality test
   */
  bool not_equal (const unit_trans &t) const
  {
    return ! equal (t);
  }

  /**
   *  @brief (dummy) inequality
   */
  bool operator!= (unit_trans /*b*/) const
  {
    return false;
  }

  /**
   *  @brief (dummy) comparison
   */
  bool operator< (unit_trans /*b*/) const
  {
    return false;
  }

  /**
   *  @brief (dummy) fuzzy less comparison
   */
  bool less (unit_trans /*b*/) const
  {
    return true;
  }

  /**
   *  @brief Assignment (which basically does nothing)
   */
  unit_trans &operator= (const unit_trans &) 
  {
    return *this;
  }

  /**
   *  @brief Assignment (which basically does nothing)
   */
  template <class D>
  unit_trans &operator= (const unit_trans<D> &) 
  {
    return *this;
  }

  /**
   *  @brief Inversion
   */
  unit_trans inverted () const
  {
    return *this;
  }

  /** 
   *  @brief In-place inversion
   */
  unit_trans invert ()
  {
    return *this;
  }

  /**
   *  @brief The transformation of a point 
   */
  point<C> operator() (const point<C> &p) const
  {
    return p;
  }

  /**
   *  @brief The transformation of a vector 
   */
  vector<C> operator() (const vector<C> &p) const
  {
    return p;
  }

  /**
   *  @brief The transformation of a point (non-operator version)
   */
  point<C> trans (const point<C> &p) const
  {
    return p;
  }

  /**
   *  @brief The transformation of a vector (non-operator version)
   */
  vector<C> trans (const vector<C> &p) const
  {
    return p;
  }

  /**
   *  @brief Transform a distance
   */
  distance_type ctrans (distance_type d) const
  {
    return d;
  }

  /**
   *  @brief String conversion
   */
  std::string to_string () const
  {
    return std::string ("");
  }

  /**
   *  @brief Conversion to a 2d matrix
   */
  Matrix2d to_matrix2d () const
  {
    return Matrix2d (1.0, 0.0, 0.0, 1.0);
  }

  /**
   *  @brief Conversion to a 3d matrix
   */
  Matrix3d to_matrix3d () const
  {
    return Matrix3d (1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0);
  }
};

/**
 *  @brief Dummy Multiplication (concatenation) of unit transformations
 */
template <class C>
inline unit_trans<C> 
operator* (const unit_trans<C> & /*t1*/, const unit_trans<C> & /*t2*/)
{
  return unit_trans<C>();
}

/**
 *  @brief A fixpoint transformation
 *
 *  The fixpoint transformation applies a rotation and/or mirroring operation.
 *  Even though this transformation does not require a coordinate type, it is provided
 *  to fulfil the contract.
 */

template <class C>
class fixpoint_trans
  : public default_trans<C>
{
public:
  typedef C coord_type;
  typedef C target_coord_type;
  typedef typename coord_traits<C>::distance_type distance_type;
  typedef vector<C> displacement_type;
  typedef fixpoint_trans inverse_trans;

  /**
   *  @brief The default constructor (unity transformation)
   */
  fixpoint_trans ()
    : m_f (0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Reduction
   */
  template <class T>
  explicit fixpoint_trans (const T &t)
    : m_f (0)
  {
    *this = t.fp_trans ();
  }

  /**
   *  @brief Reduction from a matrix2d
   */
  explicit fixpoint_trans (const db::matrix_2d<coord_type> &t)
  {
    m_f = ((int (floor (t.angle () / 90.0 + 0.5) + 4)) % 4) + (t.is_mirror () ? 4 : 0);
  }

  /**
   *  @brief Reduction from a matrix3d
   */
  explicit fixpoint_trans (const db::matrix_3d<coord_type> &t)
  {
    m_f = ((int (floor (t.angle () / 90.0 + 0.5) + 4)) % 4) + (t.is_mirror () ? 4 : 0);
  }

  /**
   *  @brief Returns true, if the transformation is unity
   */
  bool is_unity () const
  {
    return m_f == 0;
  }

  /**
   *  @brief The standard constructor using angle and mirror flag
   *
   *  The sequence of operations is: mirroring at x axis,
   *  rotation, application of displacement.
   *  
   *  @param rot The rotation in units of 90 degree
   *  @param mirrx True, if mirrored at x axis
   */
  fixpoint_trans (int rot, bool mirrx)
    : m_f ((rot & 3) + (mirrx ? 4 : 0))
  {
    // .. nothing else ..
  }
  
  /**
   *  @brief The copy constructor 
   *
   *  @param d The source from which to copy
   */
  template <class D>
  explicit fixpoint_trans (const fixpoint_trans<D> &d)
    : m_f (d.rot ())
  { }

  /**
   *  @brief The standard constructor using a code rather than angle and mirror and no displacement
   *  
   *  @param f The rotation/mirror code (r0 .. m135 constants)
   */
  explicit fixpoint_trans (int f)
    : m_f (f)
  {
    // .. nothing else ..
  }

  /**
   *  @brief The standard constructor using a code rather than angle and mirror and no displacement
   *
   *  @param f The rotation/mirror code (r0 .. m135 constants)
   */
  explicit fixpoint_trans (unsigned int f)
    : m_f (f)
  {
    // .. nothing else ..
  }

  /**
   *  @brief The rotation/mirror codes
   */
  static const int r0   = 0;  //  No rotation
  static const int r90  = 1;  //  Rotation by 90 degree counterclockwise
  static const int r180 = 2;  //  Rotation by 180 degree counterclockwise
  static const int r270 = 3;  //  Rotation by 270 degree counterclockwise
  static const int m0   = 4;  //  Mirroring at x-axis
  static const int m45  = 5;  //  Mirroring at 45-degree axis
  static const int m90  = 6;  //  Mirroring at y-axis
  static const int m135 = 7;  //  Mirroring at 135-degree axis

  /**
   *  @brief Inversion
   *
   *  Returns the inverted transformation
   *
   *  @return The inverted transformation
   */
  fixpoint_trans<C> inverted () const
  {
    fixpoint_trans t (*this);
    t.invert ();
    return t;
  }

  /** 
   *  @brief In-place inversion
   *
   *  Inverts the transformation and replaces *this by the
   *  inverted one.
   *
   *  @return The inverted transformation
   */
  fixpoint_trans<C> invert ()
  {
    if (m_f < 4) {
      m_f = (4 - m_f) & 3;
    }
    return *this;
  }

  /**
   *  @brief Conversion to a 2d matrix
   */
  Matrix2d to_matrix2d () const
  {
    vector<C> tx = operator () (vector<C> (1, 0));
    vector<C> ty = operator () (vector<C> (0, 1));
    return Matrix2d (tx.x (), ty.x (), tx.y (), ty.y ());
  }

  /**
   *  @brief Conversion to a 3d matrix
   */
  Matrix3d to_matrix3d () const
  {
    return Matrix3d (to_matrix2d ());
  }

  /**
   *  @brief The transformation of a point 
   *
   *  The operator() method transforms the given point.
   *  q = t(p)
   *  
   *  @param p The point to transform
   *  @return The transformed point
   */
  point<C> operator() (const point<C> &p) const
  {
    switch (m_f) {
    default:
      return point<C> (p.x (), p.y ());
    case 1:
      return point<C> (-p.y (), p.x ());
    case 2:
      return point<C> (-p.x (), -p.y ());
    case 3:
      return point<C> (p.y (), -p.x ());
    case 4:
      return point<C> (p.x (), -p.y ());
    case 5:
      return point<C> (p.y (), p.x ());
    case 6:
      return point<C> (-p.x (), p.y ());
    case 7:
      return point<C> (-p.y (), -p.x ());
    }
  }

  /**
   *  @brief The transformation of a point (non-operator version)
   */
  point<C> trans (const point<C> &p) const
  {
    return operator() (p);
  }

  /**
   *  @brief The transformation of a vector 
   *
   *  The operator() method transforms the given vector.
   *  q = t(p)
   *  
   *  @param p The point to transform
   *  @return The transformed point
   */
  vector<C> operator() (const vector<C> &p) const
  {
    switch (m_f) {
    default:
      return vector<C> (p.x (), p.y ());
    case 1:
      return vector<C> (-p.y (), p.x ());
    case 2:
      return vector<C> (-p.x (), -p.y ());
    case 3:
      return vector<C> (p.y (), -p.x ());
    case 4:
      return vector<C> (p.x (), -p.y ());
    case 5:
      return vector<C> (p.y (), p.x ());
    case 6:
      return vector<C> (-p.x (), p.y ());
    case 7:
      return vector<C> (-p.y (), -p.x ());
    }
  }

  /**
   *  @brief The transformation of a vector (non-operator version)
   */
  vector<C> trans (const vector<C> &p) const
  {
    return operator() (p);
  }

  /**
   *  @brief Transform a fixpoint transformation
   */
  fixpoint_trans ftrans (fixpoint_trans t) const
  {
    fixpoint_trans tt (*this);
    tt *= t;
    return tt;
  }

  /** 
   *  @brief Extract the fixpoint transformation part (this is identical to *this in this case)
   */
  const fixpoint_trans &fp_trans () const
  {
    return *this;
  }

  /**
   *  @brief Transform a distance
   */
  distance_type ctrans (distance_type d) const
  {
    return d;
  }

  /**
   *  @brief Multiplication (concatenation) of transformations
   *
   *  The *= operator modifies the transformation by 
   *  replacing *this with *this * t (t is applied before *this).
   *
   *  @param t The transformation to apply before
   *  @return The modified transformation
   */
  fixpoint_trans &operator*= (const fixpoint_trans &t)
  {
    m_f = ((m_f + (1 - ((m_f & 4) >> 1)) * t.m_f) & 3) + ((m_f ^ t.m_f) & 4);
    return *this;
  }

  /**
   *  @brief A sorting criterion
   */
  bool operator< (const fixpoint_trans &t) const
  {
    return m_f < t.m_f;
  }

  /**
   *  @brief A (dummy) fuzzy less criterion
   */
  bool less (const fixpoint_trans &t) const
  {
    return m_f < t.m_f;
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const fixpoint_trans &t) const
  {
    return m_f == t.m_f;
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const fixpoint_trans &t) const
  {
    return !operator== (t);
  }

  /**
   *  @brief A (dummy) fuzzy equality test
   */
  bool equal (const fixpoint_trans &t) const
  {
    return m_f == t.m_f;
  }

  /**
   *  @brief A (dummy) fuzzy inequality test
   */
  bool not_equal (const fixpoint_trans &t) const
  {
    return ! equal (t);
  }

  /**
   *  @brief String conversion
   */
  std::string to_string () const
  {
    const char *ms [] = { "r0", "r90", "r180", "r270", 
                          "m0", "m45", "m90",  "m135" };

    return std::string ((m_f < 0 || m_f >= 8) ? "*" : ms [m_f]);
  }

  /**
   *  @brief Accessor to the rotation/mirror code
   */
  int rot () const 
  {
    return m_f;
  }

  /**
   *  @brief Accessor to the angle (in units of 90 degree)
   */
  int angle () const 
  {
    return m_f & 3;
  }

  /**
   *  @brief Mirror flag.
   *
   *  The result of this operation is true, if the transformation is
   *  mirroring, i.e. det(M) == -1.
   */
  bool is_mirror () const
  {
    return m_f >= 4;
  }

private:
  int m_f;
};

/**
 *  @brief Multiplication (concatenation) of transformations
 *
 *  t = t1 * t2 is the resulting transformation that is effectively
 *  applied if first t2 and then t1 is applied.
 *
 *  @param t1 The transformation to apply last
 *  @param t2 The transformation to apply first
 *  @return t1 * t2
 */
template <class C>
inline fixpoint_trans<C> 
operator* (const fixpoint_trans<C> &t1, const fixpoint_trans<C> &t2)
{
  fixpoint_trans<C> t (t1);
  t *= t2;
  return t;
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const fixpoint_trans<C> &t)
{
  return (os << t.to_string ());
}

/**
 *  @brief A displacement transformation
 *
 *  The displacement transformation applies a displacement vector, but no rotation.
 */

template <class C>
class disp_trans
  : public default_trans<C>
{
public:
  typedef C coord_type;
  typedef C target_coord_type;
  typedef typename coord_traits<C>::distance_type distance_type;
  typedef vector<C> displacement_type;
  typedef disp_trans<C> inverse_trans;

  /**
   *  @brief The default constructor (unity transformation)
   */
  disp_trans ()
    : m_u ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief The "conversion" from the unit transformation to a displacement
   */
  disp_trans (unit_trans<C>)
    : m_u ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief The copy constructor 
   *
   *  @param d The source from which to copy
   */
  disp_trans (const disp_trans<C> &d)
    : m_u (d.disp ())
  { }

  /**
   *  @brief The copy constructor that converts also
   *
   *  The copy constructor allows converting between different
   *  coordinate types, if possible.
   *
   *  @param d The source from which to copy
   */
  template <class D>
  disp_trans (const disp_trans<D> &d)
    : m_u (d.disp ())
  { }

  /**
   *  @brief The standard constructor using a displacement only (as vector)
   *  
   *  @param u The displacement
   */
  explicit disp_trans (const displacement_type &u)
    : m_u (u.x (), u.y ())
  {
    // .. nothing else ..
  }

  /**
   *  @brief Downcast: extracts the displacement part of a complex transformation
   */
  explicit disp_trans (const simple_trans<C> &st)
    : m_u (st.disp ())
  {
    // .. nothing else ..
  }

  /**
   *  @brief Downcast: extracts the displacement part of a complex transformation
   */
  template <class R>
  explicit disp_trans (const complex_trans<C, C, R> &ct)
    : m_u (ct.disp ())
  {
    // .. nothing else ..
  }

  /**
   *  @brief Assignment
   */
  disp_trans &operator= (const disp_trans<C> &d)
  {
    m_u = d.disp ();
    return *this;
  }

  /**
   *  @brief Assignment with type conversion
   */
  template <class D>
  disp_trans &operator= (const disp_trans<D> &d)
  {
    m_u = d.disp ();
    return *this;
  }

  /**
   *  @brief Returns true, if the transformation is unity
   */
  bool is_unity () const
  {
    return m_u.equal (displacement_type ());
  }

  /**
   *  @brief Inversion
   *
   *  Returns the inverted transformation
   *
   *  @return The inverted transformation
   */
  disp_trans<C> inverted () const
  {
    disp_trans<C> t (*this);
    t.invert ();
    return t;
  }

  /** 
   *  @brief In-place inversion
   *
   *  Inverts the transformation and replaces *this by the
   *  inverted one.
   *
   *  @return The inverted transformation
   */
  disp_trans<C> invert ()
  {
    m_u = -m_u;
    return *this;
  }

  /**
   *  @brief Conversion to a 2d matrix
   */
  Matrix2d to_matrix2d () const
  {
    return Matrix2d (1.0, 0.0, 0.0, 1.0);
  }

  /**
   *  @brief Conversion to a 3d matrix
   */
  Matrix3d to_matrix3d () const
  {
    return Matrix3d (1.0, 0.0, 0.0, 1.0, m_u.x (), m_u.y (), 0.0, 0.0);
  }

  /**
   *  @brief The transformation of a distance
   *
   *  The ctrans method transforms the given distance.
   *  e = t(d). For the displacement transformations, there
   *  is no magnification and no modification of the distance
   *  therefore.
   *  
   *  @param d The distance to transform
   *  @return The transformed distance
   */
  distance_type ctrans (distance_type d) const
  {
    return d;
  }
  
  /**
   *  @brief The transformation of a point 
   *
   *  The operator() method transforms the given point.
   *  q = t(p)
   *  
   *  @param p The point to transform
   *  @return The transformed point
   */
  point<C> operator() (const point<C> &p) const
  {
    return point<C> (p) + m_u;
  }

  /**
   *  @brief The transformation of a point (non-operator version)
   */
  point<C> trans (const point<C> &p) const
  {
    return operator() (p);
  }

  /**
   *  @brief The transformation of a vector 
   *
   *  The operator() method transforms the given vector.
   *  q = t(p)
   *  
   *  @param p The vector to transform
   *  @return The transformed vector
   */
  vector<C> operator() (const vector<C> &p) const
  {
    return p;
  }

  /**
   *  @brief The transformation of a vector (non-operator version)
   */
  vector<C> trans (const vector<C> &p) const
  {
    return p;
  }

  /**
   *  @brief Multiplication (concatenation) of transformations
   *
   *  The *= operator modifies the transformation by 
   *  replacing *this with *this * t (t is applied before *this).
   *
   *  @param t The transformation to apply before
   *  @return The modified transformation
   */
  disp_trans<C> &operator*= (const disp_trans<C> &t)
  {
    m_u += t.m_u;
    return *this;
  }
  
  /**
   *  @brief A sorting criterion
   */
  bool operator< (const disp_trans<C> &t) const
  {
    return m_u < t.m_u;
  }

  /**
   *  @brief A fuzzy sorting criterion
   */
  bool less (const disp_trans<C> &t) const
  {
    return m_u.less (t.m_u);
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const disp_trans<C> &t) const
  {
    return m_u == t.m_u;
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const disp_trans<C> &t) const
  {
    return !operator== (t);
  }

  /**
   *  @brief A fuzzy equality test
   */
  bool equal (const disp_trans<C> &t) const
  {
    return m_u.equal (t.m_u);
  }

  /**
   *  @brief A fuzzy inequality test
   */
  bool not_equal (const disp_trans<C> &t) const
  {
    return ! equal (t);
  }

  /**
   *  @brief String conversion
   */
  std::string to_string () const
  {
    return m_u.to_string ();
  }

  /** 
   *  @brief Gets the displacement
   */
  const displacement_type &disp () const
  {
    return m_u;
  }

  /** 
   *  @brief Sets the displacement
   */
  void disp (const displacement_type &u) 
  {
    m_u = u;
  }

  /**
   *  @brief Mirror predicate
   */
  bool is_mirror () const
  {
    return false;
  }

  /** 
   *  @brief Extract the fixpoint transformation part (this is a r0 contribution in this case)
   */
  fixpoint_trans<C> fp_trans () const
  {
    return fixpoint_trans<C> ();
  }

private:
  displacement_type m_u;
};

/**
 *  @brief Multiplication (concatenation) of transformations
 *
 *  t = t1 * t2 is the resulting transformation that is effectively
 *  applied if first t2 and then t1 is applied.
 *
 *  @param t1 The transformation to apply last
 *  @param t2 The transformation to apply first
 *  @return t1 * t2
 */
template <class C>
inline disp_trans<C> 
operator* (const disp_trans<C> &t1, const disp_trans<C> &t2)
{
  disp_trans<C> t (t1);
  t *= t2;
  return t;
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const disp_trans<C> &t)
{
  return (os << t.to_string ());
}

/**
 *  @brief A simple transformation
 *
 *  The simple transformation applies a displacement vector
 *  and a simple fixpoint transformation. 
 */

template <class C>
class DB_PUBLIC_TEMPLATE simple_trans
  : public fixpoint_trans<C>
{
public:
  typedef C coord_type;
  typedef C target_coord_type;
  typedef typename coord_traits<C>::distance_type distance_type;
  typedef vector<C> displacement_type;
  typedef simple_trans<C> inverse_trans;

  /**
   *  @brief The default constructor (unity transformation)
   */
  simple_trans ()
    : fixpoint_trans<C> (0), m_u ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief Conversion constructor from a fixpoint transformation
   */
  explicit simple_trans (fixpoint_trans<C> f)
    : fixpoint_trans<C> (f), m_u ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief Conversion constructor from a unit transformation
   */
  explicit simple_trans (unit_trans<C>)
    : fixpoint_trans<C> (0), m_u ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief Conversion constructor from a displacement transformation
   */
  explicit simple_trans (const disp_trans<C> &d)
    : fixpoint_trans<C> (0), m_u (d.disp ())
  {
    // .. nothing else ..
  }

  /**
   *  @brief The copy constructor
   *
   *  @param d The source from which to copy
   */
  simple_trans (const simple_trans<C> &d)
    : fixpoint_trans<C> (d.rot ()), m_u (d.disp ())
  { }

  /**
   *  @brief Assignment
   *
   *  @param d The source from which to take the data
   */
  simple_trans &operator= (const simple_trans<C> &d)
  { 
    fixpoint_trans<C>::operator= (d);
    m_u = d.disp ();
    return *this;
  }

  /**
   *  @brief The copy constructor that converts to a different coordinate type also
   *
   *  The copy constructor allows converting between different
   *  coordinate types, if possible.
   *
   *  @param d The source from which to copy
   */
  template <class D>
  explicit simple_trans (const simple_trans<D> &d)
    : fixpoint_trans<C> (d.rot ()), m_u (d.disp ())
  { }

  /**
   *  @brief Assignment which also converts
   *
   *  This assignment implementation will also convert
   *  between different coordinate types if possible.
   *
   *  @param d The source from which to take the data
   */
  template <class D>
  simple_trans &operator= (const simple_trans<D> &d)
  { 
    fixpoint_trans<C>::operator= (d);
    m_u = d.disp ();
    return *this;
  }

  /**
   *  @brief The standard constructor using angle and mirror flag
   *
   *  The sequence of operations is: mirroring at x axis,
   *  rotation, application of displacement.
   *  
   *  @param rot The rotation in units of 90 degree
   *  @param mirrx True, if mirrored at x axis
   *  @param u The displacement
   */
  simple_trans (int rot, bool mirrx, const vector<C> &u)
    : fixpoint_trans<C> (rot, mirrx), m_u (u)
  {
    // .. nothing else ..
  }
  
  /**
   *  @brief The standard constructor for a displacement-only transformation
   *  
   *  @param u The displacement
   */
  explicit simple_trans (const vector<C> &u)
    : fixpoint_trans<C> (), m_u (u)
  {
    // .. nothing else ..
  }

  /**
   *  @brief The standard constructor using a code rather than angle and mirror
   *
   *  @param f The rotation/mirror code (r0 .. m135 constants)
   *  @param u The displacement
   */
  simple_trans (int f, const vector<C> &u)
    : fixpoint_trans<C> (f), m_u (u)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Downcast: extracts the simple transformation part of a complex transformation
   */
  template <class R>
  explicit simple_trans (const complex_trans<C, C, R> &ct)
    : fixpoint_trans<C> (ct.fp_trans ()), m_u (ct.disp ())
  {
    // .. nothing else ..
  }

  /**
   *  @brief The standard constructor using a code rather than angle and mirror and no displacement
   *  
   *  @param f The rotation/mirror code (r0 .. m135 constants)
   */
  explicit simple_trans (int f)
    : fixpoint_trans<C> (f), m_u ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief Inversion
   *
   *  Returns the inverted transformation
   *
   *  @return The inverted transformation
   */
  simple_trans<C> inverted () const
  {
    simple_trans<C> t (*this);
    t.invert ();
    return t;
  }

  /** 
   *  @brief In-place inversion
   *
   *  Inverts the transformation and replaces *this by the
   *  inverted one.
   *
   *  @return The inverted transformation
   */
  simple_trans<C> invert ()
  {
    fixpoint_trans<C>::invert ();
    vector<C> u = m_u;
    m_u = -operator() (u);
    return *this;
  }

  /**
   *  @brief Returns true, if the transformation is unity
   */
  bool is_unity () const
  {
    return m_u.equal (displacement_type ()) && fixpoint_trans<C>::is_unity ();
  }

  /**
   *  @brief The transformation of a distance
   *
   *  The ctrans method transforms the given distance.
   *  e = t(d). For the simple transformations, there
   *  is no magnification and no modification of the distance
   *  therefore.
   *  
   *  @param d The distance to transform
   *  @return The transformed distance
   */
  distance_type ctrans (distance_type d) const
  {
    return d;
  }
  
  /**
   *  @brief The transformation of a point 
   *
   *  The operator() method transforms the given point.
   *  q = t(p)
   *  
   *  @param p The point to transform
   *  @return The transformed point
   */
  point<C> operator() (const point<C> &p) const
  {
    return point<C> (fixpoint_trans<C>::operator() (p)) + m_u;
  }

  /**
   *  @brief The transformation of a point (non-operator version)
   */
  point<C> trans (const point<C> &p) const
  {
    return operator() (p);
  }

  /**
   *  @brief The transformation of a vector 
   *
   *  The operator() method transforms the given vector.
   *  q = t(p)
   *  
   *  @param p The point to transform
   *  @return The transformed vector
   */
  vector<C> operator() (const vector<C> &p) const
  {
    return vector<C> (fixpoint_trans<C>::operator() (p));
  }

  /**
   *  @brief The transformation of a vector (non-operator version)
   */
  vector<C> trans (const vector<C> &p) const
  {
    return operator() (p);
  }

  /**
   *  @brief Transform a fixpoint transformation
   */
  fixpoint_trans<C> ftrans (fixpoint_trans<C> t) const
  {
    return fixpoint_trans<C>::ftrans (t);
  }

  /**
   *  @brief Conversion to a 2d matrix
   */
  Matrix2d to_matrix2d () const
  {
    return fp_trans ().to_matrix2d ();
  }

  /**
   *  @brief Conversion to a 3d matrix
   */
  Matrix3d to_matrix3d () const
  {
    return Matrix3d (1.0, 0.0, 0.0, 1.0, m_u.x (), m_u.y (), 0.0, 0.0) * fp_trans ().to_matrix3d ();
  }

  /**
   *  @brief Multiplication (concatenation) of transformations
   *
   *  The *= operator modifies the transformation by 
   *  replacing *this with *this * t (t is applied before *this).
   *
   *  @param t The transformation to apply before
   *  @return The modified transformation
   */
  simple_trans<C> &operator*= (const simple_trans<C> &t)
  {
    m_u += operator() (t.m_u);
    fixpoint_trans<C>::operator*= (t);
    return *this;
  }

  /**
   *  @brief A method version of operator*, mainly for automation purposes
   */
  simple_trans<C> concat (const simple_trans<C> &t) const
  {
    simple_trans<C> r (*this);
    r *= t;
    return r;
  }

  /**
   *  @brief A sorting criterion
   */
  bool operator< (const simple_trans<C> &t) const
  {
    return fixpoint_trans<C>::operator< (t) || (fixpoint_trans<C>::operator== (t) && m_u < t.m_u);
  }

  /**
   *  @brief A fuzzy sorting criterion
   */
  bool less (const simple_trans<C> &t) const
  {
    return fixpoint_trans<C>::operator< (t) || (fixpoint_trans<C>::operator== (t) && m_u.less (t.m_u));
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const simple_trans<C> &t) const
  {
    return fixpoint_trans<C>::operator== (t) && m_u == t.m_u;
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const simple_trans<C> &t) const
  {
    return !operator== (t);
  }

  /**
   *  @brief A fuzzy equality test
   */
  bool equal (const simple_trans<C> &t) const
  {
    return fixpoint_trans<C>::operator== (t) && m_u.equal (t.m_u);
  }

  /**
   *  @brief A fuzzy inequality test
   */
  bool not_equal (const simple_trans<C> &t) const
  {
    return ! equal (t);
  }

  /**
   *  @brief String conversion
   */
  std::string to_string (double dbu = 0.0) const
  {
    std::string s1 = fixpoint_trans<C>::to_string ();
    std::string s2 = m_u.to_string (dbu);
    if (! s1.empty () && ! s2.empty ()) {
      return s1 + " " + s2;
    } else {
      return s1 + s2;
    }
  }

  /** 
   *  @brief Gets the displacement
   */
  const displacement_type &disp () const
  {
    return m_u;
  }

  /** 
   *  @brief Sets the displacement
   */
  void disp (const displacement_type &u)
  {
    m_u = u;
  }

  /** 
   *  @brief Accessor to the fp_trans
   */
  const fixpoint_trans<C> &fp_trans () const
  {
    return *this;
  }

private:
  displacement_type m_u;
};

/**
 *  @brief Multiplication (concatenation) of transformations
 *
 *  t = t1 * t2 is the resulting transformation that is effectively
 *  applied if first t2 and then t1 is applied.
 *
 *  @param t1 The transformation to apply last
 *  @param t2 The transformation to apply first
 *  @return t1 * t2
 */
template <class C>
inline simple_trans<C> 
operator* (const simple_trans<C> &t1, const simple_trans<C> &t2)
{
  simple_trans<C> t (t1);
  t *= t2;
  return t;
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const simple_trans<C> &t)
{
  return (os << t.to_string ());
}

/**
 *  @brief A complex transformation
 *
 *  A complex transformation provides magnification, mirroring at the x-axis, rotation by an arbitrary
 *  angle and a displacement. The template parameters for this transformation are
 *  I (the input coordinate type), F (the output coordinate type) and R (the representation
 *  type used internally for representing the floating-point members).
 */
template <class I, class F, class R>
class DB_PUBLIC_TEMPLATE complex_trans
{
public:
  typedef I coord_type;
  typedef F target_coord_type;
  typedef typename coord_traits<I>::distance_type distance_type;
  typedef typename coord_traits<F>::distance_type target_distance_type;
  typedef vector<F> displacement_type;
  typedef complex_trans<F, I, R> inverse_trans;
  typedef epsilon_f<R> eps_f;

  /**
   *  @brief The default constructor (unity transformation)
   */
  complex_trans ()
    : m_sin (0.0), m_cos (1.0), m_mag (1.0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Conversion constructor from a unit transformation
   */
  explicit complex_trans (unit_trans<I> /*f*/)
    : m_sin (0.0), m_cos (1.0), m_mag (1.0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Conversion constructor from a fixpoint transformation code
   */
  explicit complex_trans (int fc)
  {
    db::fixpoint_trans<R> f (fc);
    db::point<R> t (1.0, 0.0);
    t = f (t);
    m_cos = t.x ();
    m_sin = t.y ();
    m_mag = f.is_mirror () ? -1.0 : 1.0;
  }

  /**
   *  @brief Conversion constructor from a fixpoint transformation
   */
  explicit complex_trans (fixpoint_trans<I> f)
  {
    db::point<R> t (1.0, 0.0);
    t = fixpoint_trans<R> (f) (t);
    m_cos = t.x ();
    m_sin = t.y ();
    m_mag = f.is_mirror () ? -1.0 : 1.0;
  }

  /**
   *  @brief Conversion constructor from a displacement transformation
   */
  explicit complex_trans (const disp_trans<I> &d)
    : m_u (d.disp ()), m_sin (0.0), m_cos (1.0), m_mag (1.0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Conversion constructor from a simple transformation
   */
  explicit complex_trans (const simple_trans<I> &s)
    : m_u (s.disp ())
  {
    db::point<R> t (1.0, 0.0);
    t = fixpoint_trans<R> (s.fp_trans ()) (t);
    m_cos = t.x ();
    m_sin = t.y ();
    m_mag = s.is_mirror () ? -1.0 : 1.0;
  }

  /**
   *  @brief Migration constructor from a simple transformation to a complex transformation
   *
   *  This constructor is supposed to support migration from a simple to a complex transformation
   *  in an incremental fashion. Basically that means, that everything that is missing in the 
   *  simple transformation is added by additional parameters.
   *  Specifically this added information is the magnification and the incremental rotation angle
   *  to fill up the 90 degree multiples of the simple transformation to the desired rotation.
   *  To save a trigonometric function computation, the added angle is given as the cosine of the
   *  angle (a value from 1.0 representing 0 degree to 0.0 representing 90 degree).
   *
   *  @param s The simple transformation to derive this transformation from
   *  @param acos The cosine of the additional rotation angle
   *  @param mag The magnification
   */
  complex_trans (const simple_trans<I> &s, double acos, double mag)
    : m_u (s.disp ())
  {
    //  This prevents rounding issues (like acos = 1.0000000000002):
    if (acos > 1.0) {
      acos = 1.0;
    } else if (acos < -1.0) {
      acos = -1.0;
    }

    db::point<R> t (1.0, 0.0);
    t = fixpoint_trans<R> (s.fp_trans ()) (t);
    double asin = sqrt (1.0 - acos * acos); // we may to this since we know that the angle is between 0 and 90 degree
    m_cos = t.x () * acos - t.y () * asin;
    m_sin = t.x () * asin + t.y () * acos;
    m_mag = s.is_mirror () ? -mag : mag;
  }

  /**
   *  @brief The standard constructor using a Matrix2d and a displacement
   *
   *  @param m The matrix to take the rotation part of the transformation from
   *  @param u The displacement
   *
   *  The matrix must not contain shear components.
   */
  complex_trans (double mag, double rot, bool mirrx, const displacement_type &u)
    : m_u (u)
  {
    tl_assert (mag > 0.0);
    m_mag = mirrx ? -mag : mag;
    rot *= M_PI / 180.0;
    m_sin = sin (rot);
    m_cos = cos (rot);
  }
  
  /**
   *  @brief The standard constructor using a Matrix3d object
   *
   *  @param m The matrix to take the transformation from
   *
   *  The matrix must not represent perspective distortion nor shear.
   */
  explicit complex_trans (const Matrix3d &m)
    : m_u (m.disp ())
  {
    tl_assert (! m.has_shear ());
    tl_assert (! m.has_perspective ());
    std::pair<double, double> mag = m.mag2 ();
    tl_assert (fabs (mag.first - mag.second) < 1e-10);
    double rot = m.angle () * M_PI / 180.0;
    m_mag = m.is_mirror () ? -mag.first : mag.first;
    m_sin = sin (rot);
    m_cos = cos (rot);
  }
  
  /**
   *  @brief The standard constructor using a Matrix2d object
   *
   *  The sequence of operations is: magnification, mirroring at x axis,
   *  rotation, application of displacement.
   *  
   *  @param mag The magnification
   *  @param rot The rotation angle in units of degree
   *  @param mirrx True, if mirrored at x axis
   *  @param u The displacement
   */
  complex_trans (const Matrix2d &m, const vector<I> &u)
    : m_u (u)
  {
    tl_assert (! m.has_shear ());
    std::pair<double, double> mag = m.mag2 ();
    tl_assert (fabs (mag.first - mag.second) < 1e-10);
    double rot = m.angle () * M_PI / 180.0;
    m_mag = m.is_mirror () ? -mag.first : mag.first;
    m_sin = sin (rot);
    m_cos = cos (rot);
  }
  
  /**
   *  @brief The standard constructor using magnification only
   *
   *  @param mag The magnification
   */
  explicit complex_trans (double mag)
  {
    tl_assert (mag > 0.0);
    m_mag = mag;
    m_sin = 0.0;
    m_cos = 1.0;
  }
  
  /**
   *  @brief The copy constructor that converts also
   *
   *  The copy constructor allows converting between different
   *  coordinate types, if possible.
   *
   *  @param d The source from which to copy
   */
  template <class II, class FF, class RR>
  explicit complex_trans (const complex_trans<II, FF, RR> &d)
    : m_u (d.m_u), m_sin (d.m_sin), m_cos (d.m_cos), m_mag (d.m_mag)
  { }

  /**
   *  @brief The standard constructor using a displacement only
   *  
   *  @param u The displacement
   */
  explicit complex_trans (const displacement_type &u)
    : m_u (u), m_sin (0.0), m_cos (1.0), m_mag (1.0)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Inversion
   *
   *  Returns the inverted transformation. NOTE: in-place inversion is not supported since
   *  for transformations where F != I, the inverse transformation is of a different type.
   *
   *  @return The inverted transformation
   */
  inverse_trans inverted () const
  {
    complex_trans<R, R, R> inv;

    inv.m_mag = 1.0 / m_mag;
    inv.m_sin = -m_sin * (m_mag < 0.0 ? -1.0 : 1.0);
    inv.m_cos = m_cos;
    inv.m_u = inv.operator () (-m_u);

    return inverse_trans (inv);
  }

  /**
   *  @brief In-place inversion
   *
   *  Note that in general, in-place inversion may not be type-consistent if the input and output types
   *  are different. The inversion is not a true inversion in the sense of inv(T) * T == 1 because of
   *  potential rounding effects.
   */
  complex_trans &invert ()
  {
    complex_trans<R, R, R> inv;

    inv.m_mag = 1.0 / m_mag;
    inv.m_sin = -m_sin * (m_mag < 0.0 ? -1.0 : 1.0);
    inv.m_cos = m_cos;
    inv.m_u = inv.operator () (-m_u);

    *this = complex_trans (inv);
    return *this;
  }

  /**
   *  @brief The transformation of a distance
   *
   *  The ctrans method transforms the given distance.
   *  
   *  @param d The distance to transform
   *  @return The transformed distance
   */
  target_distance_type ctrans (distance_type d) const
  {
    return coord_traits<F>::rounded_distance (d * fabs (m_mag));
  }
  
  /**
   *  @brief Conversion to a 2d matrix
   */
  Matrix2d to_matrix2d () const
  {
    return Matrix2d (m_cos * fabs (m_mag), -m_sin * m_mag, m_sin * fabs (m_mag), m_cos * m_mag);
  }

  /**
   *  @brief Conversion to a 3d matrix
   */
  Matrix3d to_matrix3d () const
  {
    return Matrix3d (m_cos * fabs (m_mag), -m_sin * m_mag, m_sin * fabs (m_mag), m_cos * m_mag, m_u.x (), m_u.y (), 0.0, 0.0);
  }

  /**
   *  @brief The transformation of a point 
   *
   *  The operator() method transforms the given point.
   *  q = t(p)
   *  
   *  @param p The point to transform
   *  @return The transformed point
   */
  point<F> operator() (const point<I> &p) const
  {
    db::point<R> mp (m_cos * p.x () * fabs (m_mag) - m_sin * p.y () * m_mag,
                     m_sin * p.x () * fabs (m_mag) + m_cos * p.y () * m_mag);
    return point<F> (mp + m_u);
  }

  /**
   *  @brief The transformation of a point (non-operator version)
   */
  point<F> trans (const point<I> &p) const
  {
    return operator() (p);
  }

  /**
   *  @brief The transformation of a vector 
   *
   *  The operator() method transforms the given vector.
   *  q = t(p)
   *  
   *  @param p The vector to transform
   *  @return The transformed vector
   */
  vector<F> operator() (const vector<I> &p) const
  {
    db::vector<R> mp (m_cos * p.x () * fabs (m_mag) - m_sin * p.y () * m_mag,
                      m_sin * p.x () * fabs (m_mag) + m_cos * p.y () * m_mag);
    return db::vector<F> (mp);
  }

  /**
   *  @brief The transformation of a vector (non-operator version)
   */
  vector<F> trans (const vector<I> &p) const
  {
    return operator() (p);
  }

  /**
   *  @brief Test, whether this is a unit transformation
   */
  bool is_unity () const
  {
    if (fabs (m_mag - 1.0) > eps_f ()) {
      return false;
    }
    if (fabs (m_sin) > eps_f ()) {
      return false;
    }
    if (fabs (m_cos - 1.0) > eps_f ()) {
      return false;
    }
    return disp ().equal (displacement_type ());
  }

  /**
   *  @brief Test, if the transformation is an orthogonal transformation
   *
   *  If the rotation is by a multiple of 90 degree, this method will return true.
   */
  bool is_ortho () const
  {
    return fabs (m_sin * m_cos) <= eps_f ();
  }

  /**
   *  @brief Return the respective rotation code if possible
   *
   *  If this transformation is orthogonal (is_ortho () == true), then this method
   *  will return the corresponding fixpoint transformation, not taking into account
   *  magnification and displacement. If the transformation is not orthogonal, the result
   *  reflects the quadrant the rotation goes into with the guarantee to reproduce the 
   *  correct quadrant in the exact case.
   */
  int rot () const
  {
    return fp_trans ().rot ();
  }

  /**
   *  @brief Return the respective fixpoint trans if possible 
   *
   *  If this transformation is orthogonal (is_ortho () == true), then this method
   *  will return the corresponding fixpoint transformation, not taking into account
   *  magnification and displacement. If the transformation is not orthogonal, the result
   *  reflects the quadrant the rotation goes into with the guarantee to reproduce the 
   *  correct quadrant in the exact case.
   */
  fixpoint_trans<I> fp_trans () const
  {
    int c;
    if (m_cos > eps_f () && m_sin >= -eps_f ()) {
      c = 0 /*r0*/;
    } else if (m_cos <= eps_f () && m_sin > eps_f ()) {
      c = 1 /*r90*/;
    } else if (m_cos < -eps_f () && m_sin <= eps_f ()) {
      c = 2 /*r180*/;
    } else {
      c = 3 /*r270*/;
    }
    return fixpoint_trans<I> (c + (m_mag < 0.0 ? 4 : 0));
  }

  /**
   *  @brief Read accessor for the angle
   *
   *  To check, if the transformation represents a rotation by a angle that
   *  is a multiple of 90 degree, use is_ortho.
   *
   *  @return The rotation angle this transformation provides in degree units (0..360 deg).
   */
  double angle () const
  {
    double a = atan2 (m_sin, m_cos) * (180.0 / M_PI);
    if (a < -eps_f ()) {
      a += 360.0;
    } else if (a <= eps_f ()) {
      a = 0.0;
    }
    return a;
  }

  /**
   *  @brief Write accessor for the angle
   */
  void angle (double rot)
  {
    rot *= M_PI / 180.0;
    m_sin = sin (rot);
    m_cos = cos (rot);
  }

  /**
   *  @brief Read accessor to the cosine part of the transformation matrix
   */
  double mcos () const
  {
    return m_cos;
  }

  /**
   *  @brief Read accessor to the sine part of the transformation matrix
   */
  double msin () const
  {
    return m_sin;
  }

  /** 
   *  @brief Read accessor to the magnification
   */
  double mag () const
  {
    return fabs (m_mag);
  }

  /** 
   *  @brief Test, if the transformation is a magnifying one
   *
   *  This is the recommended test for checking if the transformation represents
   *  a magnification.
   */
  bool is_mag () const
  {
    return fabs (fabs (m_mag) - 1.0) > eps_f ();
  }

  /** 
   *  @brief Write accessor to the magnification
   */
  void mag (double m)
  {
    tl_assert (m > 0.0);
    m_mag = m_mag < 0.0 ? -m : m;
  }

  /**
   *  @brief Returns a value indicating whether the transformation is a complex one
   *  The transformation can safely be converted to a simple transformation if this value is false.
   */
  bool is_complex () const
  {
    return is_mag () || ! is_ortho ();
  }

  /** 
   *  @brief Test, if the transformation is mirroring
   */
  bool is_mirror () const
  {
    return m_mag < 0.0;
  }

  /** 
   *  @brief Write accessor to the mirror flag
   */
  void mirror (bool m)
  {
    m_mag = m ? -fabs (m_mag) : fabs (m_mag);
  }

  /** 
   *  @brief Read accessor to the displacement
   */
  displacement_type disp () const
  {
    return displacement_type (m_u);
  }

  /** 
   *  @brief Write accessor to the displacement
   */
  void disp (const displacement_type &u)
  {
    m_u = vector<R> (u);
  }

  /**
   *  @brief Multiplication (concatenation) of transformations - in-place version
   *
   *  The *= operator modifies the transformation by 
   *  replacing *this with *this * t (t is applied before *this).
   *
   *  @param t The transformation to apply before
   *  @return The modified transformation
   */
  complex_trans &operator*= (const complex_trans &t)
  {
    *this = concat_same (t);
    return *this;
  }

  /**
   *  @brief Multiplication (concatenation) of transformations
   *
   *  The * operator returns a transformation which is identical to the
   *  *this * t (t is applied before *this).
   *
   *  @param t The transformation to apply before
   *  @return The concatenated transformation
   */
  template <class II>
  complex_trans<II, F, R> concat (const complex_trans<II, I, R> &t) const
  {
    complex_trans<II, F, R> res;

    double s1 = m_mag < 0.0 ? -1.0 : 1.0;

    db::vector<R> tu (m_cos * t.m_u.x () * fabs (m_mag) - m_sin * t.m_u.y () * m_mag,
                      m_sin * t.m_u.x () * fabs (m_mag) + m_cos * t.m_u.y () * m_mag);
    res.m_u = m_u + tu;

    res.m_mag = m_mag * t.m_mag;
    res.m_cos = m_cos * t.m_cos - s1 * m_sin * t.m_sin;
    res.m_sin = m_sin * t.m_cos + s1 * m_cos * t.m_sin;

    return res;
  }

  /**
   *  @brief Multiplication (concatenation) of transformations of the same type
   *
   *  The * operator returns a transformation which is identical to the
   *  *this * t (t is applied before *this).
   *
   *  The concatenation of two identical transformations is not strictly type-safe since
   *  formally, the output type of the second transformation needs to be compatible with
   *  the input type of the first. But transformation arithmetics is easier to do with
   *  this definition.
   *
   *  @param t The transformation to apply before
   *  @return The concatenated transformation
   */
  complex_trans concat_same (const complex_trans &t) const
  {
    //  The concatenation is done with double as the intermediate type to avoid
    //  rounding issues as far as possible.
    return complex_trans<double, F, R> (*this).concat (complex_trans<I, double, R>(t));
  }

  /**
   *  @brief Returns the transformation in a different coordinate system
   *
   *  Given a transformation which turns the current coordinate system into a new one,
   *  this method will compute the transformation as it would look like in the new
   *  coordinate system.
   *
   *  The mathematical definition is this:
   *
   *    T = this
   *    U = transformation into the new system
   *
   *    T' = U * T * inverse(U)
   *
   *  @param u The transformation into the new system
   *  @return The transformation in the new system
   */
  template <class U>
  complex_trans transform_into (const U &uin) const
  {
    //  Note: preserving type consistency is a bit tedious here. We assume we can simply
    //  return the same type as *this. Ideally *this would be of type (C,C). U would be
    //  of type (C,F). Then the output would be (F,F).
    complex_trans u (uin);
    complex_trans uinv (u);
    uinv.invert ();
    return u.concat_same (*this).concat_same (uinv);
  }

  /**
   *  @brief Retrieve the residual part of the angle 
   *
   *  The residual part is the cosine of the angle difference to the 
   *  lower next multiple of 90 degree. I.e. the residual part of 135 degree
   *  would be cos(45 deg).
   */
  double rcos () const
  {
    if (m_cos > eps_f () && m_sin >= -eps_f ()) {
      return m_cos;
    } else if (m_cos <= eps_f () && m_sin > eps_f ()) {
      return m_sin;
    } else if (m_cos < -eps_f () && m_sin <= eps_f ()) {
      return -m_cos;
    } else {
      return -m_sin;
    }
  }

  /**
   *  @brief A sorting criterion
   */
  bool operator< (const complex_trans &t) const
  {
    if (m_u != t.m_u) {
      return m_u < t.m_u;
    }
    if (fabs (m_sin - t.m_sin) > eps_f ()) {
      return m_sin < t.m_sin;
    }
    if (fabs (m_cos - t.m_cos) > eps_f ()) {
      return m_cos < t.m_cos;
    }
    if (fabs (m_mag - t.m_mag) > eps_f ()) {
      return m_mag < t.m_mag;
    }
    return false;
  }

  /**
   *  @brief A (fuzzy) sorting criterion
   */
  bool less (const complex_trans &t) const
  {
    if (! m_u.equal (t.m_u)) {
      return m_u.less (t.m_u);
    }
    if (fabs (m_sin - t.m_sin) > eps_f ()) {
      return m_sin < t.m_sin;
    }
    if (fabs (m_cos - t.m_cos) > eps_f ()) {
      return m_cos < t.m_cos;
    }
    if (fabs (m_mag - t.m_mag) > eps_f ()) {
      return m_mag < t.m_mag;
    }
    return false;
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const complex_trans &t) const
  {
    return m_u == t.m_u && 
           fabs (m_sin - t.m_sin) <= eps_f () &&
           fabs (m_cos - t.m_cos) <= eps_f () &&
           fabs (m_mag - t.m_mag) <= eps_f ();
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const complex_trans &t) const
  {
    return !operator== (t);
  }

  /**
   *  @brief A (fuzzy) equality test
   */
  bool equal (const complex_trans &t) const
  {
    return m_u.equal (t.m_u) && 
           fabs (m_sin - t.m_sin) <= eps_f () &&
           fabs (m_cos - t.m_cos) <= eps_f () &&
           fabs (m_mag - t.m_mag) <= eps_f ();
  }

  /**
   *  @brief A (fuzzy) inequality test
   */
  bool not_equal (const complex_trans &t) const
  {
    return ! equal (t);
  }

  /**
   *  @brief String conversion
   *
   *  The lazy and micron flags allow customization of the output to some degree.
   *  When lazy is set to true, output that is not required (i.e. magnification when 1)
   *  is dropped. If dbu is set, the coordinates are multiplied with this factor to render micron units.
   */
  std::string to_string (bool lazy = false, double dbu = 0.0) const
  {
    std::string s;
    if (is_mirror ()) {
      s += "m";
      s += tl::to_string (angle () * 0.5);
    } else {
      s += "r";
      s += tl::to_string (angle ());
    }
    if (! lazy || is_mag ()) {
      s += tl::sprintf (" *%.9g", mag ());
    }
    s += " ";
    s += m_u.to_string (dbu);
    return s;
  }

private:
  template <class FF, class II, class RR> friend class complex_trans;

  vector<R> m_u;
  R m_sin, m_cos;
  R m_mag;
};

/**
 *  @brief Multiplication (concatenation) of transformations
 *
 *  t = t1 * t2 is the resulting transformation that is effectively
 *  applied if first t2 and then t1 is applied.
 *
 *  @param t1 The transformation to apply last
 *  @param t2 The transformation to apply first
 *  @return t1 * t2
 */
template <class II, class I, class F, class R>
inline complex_trans<II, F, R>
operator* (const complex_trans<I, F, R> &t1, const complex_trans<II, I, R> &t2)
{
  return t1.concat (t2);
}

/**
 *  @brief Scaling of a complex transformation with a scalar
 *
 *  The resulting complex transformation will reflex the original one plus
 *  an additional magnification given by the factor m.
 *
 *  @param t The original transformation
 *  @param m The additional magnification
 *  @return t1 * m
 */
template <class I, class F, class R>
inline complex_trans<I, F, R> 
operator* (const complex_trans<I, F, R> &t1, double m)
{
  complex_trans<I, F, R> t (t1);
  t.mag (t.mag () * m);
  return t;
}

/**
 *  @brief Output stream insertion operator
 */
template <class I, class F, class R>
inline std::ostream &
operator<< (std::ostream &os, const complex_trans<I, F, R> &t)
{
  return (os << t.to_string ());
}

/**
 *  @brief A combined transformation
 *  
 *  A combined transformation is the combination of two 
 *  transformations T1 and T2 (T=T1*T2). Although the multiplication
 *  of two transformations may render the same result, but
 *  usually is more efficient. To combine two different
 *  transformations however, the combined_trans template
 *  is better suited.
 */

template <class T1, class T2>
struct combined_trans
{
  typedef typename T2::coord_type coord_type;
  typedef typename T1::target_coord_type target_coord_type;
  typedef combined_trans<typename T2::inverse_trans, typename T1::inverse_trans> inverse_trans;

  /**
   *  @brief Default constructor
   *
   *  Creates a unity transformation
   */
  combined_trans ()
    : t1 (), t2 ()
  {
    // .. nothing else ..
  }

  /**
   *  @brief Standard constructor
   *
   *  Takes two transformations and combines both.
   */
  combined_trans (const T1 &_t1, const T2 &_t2)
    : t1 (_t1), t2 (_t2)
  {
    // .. nothing else ..
  }

  /** 
   *  @brief Inversion
   *
   *  Inverts the transformation and returns the inverted
   *  transformation which swaps T1 and T2 in the type definition.
   *
   *  @return The inverted transformation
   */
  inverse_trans inverted () const
  {
    return inverse_trans (t2.inverted (), t1.inverted ());
  }

  /**
   *  @brief The transformation of a point 
   *
   *  The operator() method transforms the given point.
   *  q = t(p)
   *  
   *  @param p The point to transform
   *  @return The transformed point
   */
  template <class C>
  point<target_coord_type> operator() (const point<C> &p) const
  {
    typedef typename T2::target_coord_type intern_coord_type;
    point<intern_coord_type> q = t2.operator() (p);
    return t1.operator() (q);
  }

  /**
   *  @brief The transformation of a vector
   *
   *  The operator() method transforms the given point.
   *  q = t(p)
   *
   *  @param p The point to transform
   *  @return The transformed point
   */
  template <class C>
  vector<target_coord_type> operator() (const vector<C> &p) const
  {
    typedef typename T2::target_coord_type intern_coord_type;
    vector<intern_coord_type> q = t2.operator() (p);
    return t1.operator() (q);
  }

  /**
   *  @brief The transformation of a distance 
   *
   *  The ctrans method transforms the given distance.
   *  s = t(d)
   *  
   *  @param d The distance to transform
   *  @return The transformed distance
   */
  template <class C>
  target_coord_type ctrans (C p) const
  {
    typedef typename T2::target_coord_type intern_coord_type;
    intern_coord_type q = t2.ctrans (p);
    return t1.ctrans (q);
  }

  /**
   *  @brief The transformation of a fixpoint transformation 
   *
   *  The ftrans method transforms the given fixpoint transformation.
   *  f = t(f)
   *  
   *  @param f The fixpoint transformation to transform
   *  @return The transformed fixpoint transformation
   */
  template <class D>
  fixpoint_trans<D> ftrans (fixpoint_trans<D> f) const
  {
    return t1 (t2 (f));
  }

  /**
   *  @brief A sorting criterion
   */
  bool operator< (const combined_trans<T1, T2> &t) const
  {
    return t1 < t.t1 || (t1 == t.t1 && t2 < t.t2);
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const combined_trans<T1, T2> &t) const
  {
    return t1 == t.t1 && t2 == t.t2;
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const combined_trans<T1, T2> &t) const
  {
    return !operator== (t);
  }

  /**
   *  @brief String conversion
   */
  std::string to_string () const
  {
    std::string s1 = t1.to_string ();
    std::string s2 = t2.to_string ();
    if (! s1.empty () && ! s2.empty ()) {
      return s1 + " " + s2;
    } else {
      return s1 + s2;
    }
  }

  /**
   *  @brief Accessor to the first part of the combined transformation
   */
  const T1 &first () const
  {
    return t1;
  }

  /**
   *  @brief Accessor to the second part of the combined transformation
   */
  const T2 &second () const
  {
    return t2;
  }

  T1 t1;
  T2 t2;
};

/**
 *  @brief Output stream insertion operator
 */
template <class T1, class T2>
inline std::ostream &
operator<< (std::ostream &os, const combined_trans<T1, T2> &t)
{
  return (os << t.to_string ());
}

/**
 *  @brief The standard unit transformation
 */
typedef unit_trans<db::Coord> UnitTrans;

/**
 *  @brief The standard unit transformation for double coordinates
 */
typedef unit_trans<db::DCoord> DUnitTrans;

/**
 *  @brief The standard fixpoint transformation
 */
typedef fixpoint_trans<db::Coord> FTrans;

/**
 *  @brief The standard fixpoint transformation for double types
 */
typedef fixpoint_trans<db::DCoord> DFTrans;

/**
 *  @brief The standard displacement transformation
 */
typedef disp_trans<db::Coord> Disp;

/**
 *  @brief The double coordinate displacement transformation
 */
typedef disp_trans<db::DCoord> DDisp;

/**
 *  @brief The standard transformation
 */
typedef simple_trans<db::Coord> Trans;

/**
 *  @brief The double coordinate transformation
 */
typedef simple_trans<db::DCoord> DTrans;

/**
 *  @brief The standard complex transformation that converts integer to integer coordinates
 */
typedef complex_trans<db::Coord, db::Coord> ICplxTrans;

/**
 *  @brief The standard complex transformation
 */
typedef complex_trans<db::Coord, db::DCoord> CplxTrans;

/**
 *  @brief Specialization: concatenation of CplxTrans
 *
 *  The combination of two of these objects is basically not allowed, since the
 *  output and input types in not compatible. For sake of simplicity however, we
 *  allow this now.
 */
inline CplxTrans operator* (const CplxTrans &a, const CplxTrans &b)
{
  return a.concat_same (b);
}

/**
 *  @brief The inverse of the standard complex transformation
 */
typedef complex_trans<db::DCoord, db::Coord> VCplxTrans;

/**
 *  @brief Specialization: concatenation of VCplxTrans
 *
 *  The combination of two of these objects is basically not allowed, since the
 *  output and input types in not compatible. For sake of simplicity however, we
 *  allow this now.
 */
inline VCplxTrans operator* (const VCplxTrans &a, const VCplxTrans &b)
{
  return a.concat_same (b);
}

/**
 *  @brief The double coordinate complex transformation
 */
typedef complex_trans<db::DCoord, db::DCoord> DCplxTrans;

/**
 *  @brief Transformation operator
 *
 *  Transforms the point with the given transformation
 *  (q = T * p).
 *
 *  @param t The transformation to apply
 *  @param p The point to transform
 *  @return The transformed point
 */
template <class C, class Tr> 
inline point<typename Tr::target_coord_type> 
operator* (const Tr &t, const point<C> &p)
{
  return t.operator() (point<typename Tr::coord_type> (p));
}

/**
 *  @brief Transformation operator
 *
 *  Transforms the vector with the given transformation
 *  (q = T * p).
 *
 *  @param t The transformation to apply
 *  @param p The vector to transform
 *  @return The transformed vector
 */
template <class C, class Tr> 
inline vector<typename Tr::target_coord_type> 
operator* (const Tr &t, const vector<C> &p)
{
  return t.operator() (vector<typename Tr::coord_type> (p));
}

/**
 *  @brief Fuzzy compare function for transformation objects
 */
template <class T>
class trans_less_func 
{
public:
  bool operator() (const T &a, const T &b) const
  {
    return a.less (b);
  }
};

/**
 *  @brief Fuzzy compare function for transformation objects
 */
template <class T>
class trans_equal_func 
{
public:
  bool operator() (const T &a, const T &b) const
  {
    return a.equal (b);
  }
};

} // namespace db

/**
 *  @brief Special extractors for the transformations
 */

namespace tl 
{
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::UnitTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DUnitTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::FTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DFTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Trans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Disp &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DDisp &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::CplxTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::VCplxTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DCplxTrans &t);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::ICplxTrans &t);

  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::UnitTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DUnitTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::FTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DFTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Trans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Disp &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DDisp &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::CplxTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::VCplxTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DCplxTrans &t);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::ICplxTrans &t);

} // namespace tl

#endif


