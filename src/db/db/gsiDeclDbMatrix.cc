
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


#include "gsiDecl.h"
#include "dbMatrix.h"
#include "dbTrans.h"
#include "dbBox.h"
#include "dbPolygon.h"
#include "dbEdge.h"

namespace gsi
{

// ---------------------------------------------------------------
//  Matrix2d binding

template <class C>
static db::matrix_2d<C> *new_matrix2d ()
{
  return new db::matrix_2d<C> (1.0);
}

template <class C>
static db::matrix_2d<C> *new_matrix2d_m (double mag)
{
  return new db::matrix_2d<C> (mag);
}

template <class C>
static db::matrix_2d<C> *new_matrix2d_m2 (double mx, double my)
{
  return new db::matrix_2d<C> (mx, my);
}

template <class C>
static db::matrix_2d<C> *new_matrix2d_t (const db::DCplxTrans &t)
{
  return new db::matrix_2d<C> (t);
}

template <class C>
static db::matrix_2d<C> *new_matrix2d_mrm (double mag, double rot, bool m)
{
  return new db::matrix_2d<C> (db::matrix_2d<C>::rotation (rot) * db::matrix_2d<C>::mag (mag) * db::matrix_2d<C>::mirror (m));
}

template <class C>
static db::matrix_2d<C> *new_matrix2d_smrm (double shear, double mx, double my, double rot, bool m)
{
  return new db::matrix_2d<C> (db::matrix_2d<C>::rotation (rot) * db::matrix_2d<C>::shear (shear) * db::matrix_2d<C>::mag (mx, my) * db::matrix_2d<C>::mirror (m));
}

template <class C>
static db::matrix_2d<C> *new_matrix2d_m4 (double m11, double m12, double m21, double m22)
{
  return new db::matrix_2d<C> (m11, m12, m21, m22);
}

template <class C>
static db::complex_trans<C, C> to_cplx_trans (const db::matrix_2d<C> *m)
{
  return db::complex_trans<C, C> (db::matrix_3d<C> (*m));
}

template <class C>
static db::matrix_2d<C> sum_m (const db::matrix_2d<C> *m, const db::matrix_2d<C> &d)
{
  return *m + d;
}

template <class C>
static db::matrix_2d<C> prod_m (const db::matrix_2d<C> *m, const db::matrix_2d<C> &d)
{
  return *m * d;
}

template <class C>
static db::point<C> trans_p (const db::matrix_2d<C> *m, const db::point<C> &p)
{
  return *m * p;
}

template <class C>
static db::vector<C> trans_v (const db::matrix_2d<C> *m, const db::vector<C> &p)
{
  return *m * p;
}

template <class C>
static db::polygon<C> trans_polygon (const db::matrix_2d<C> *m, const db::polygon<C> &p)
{
  return p.transformed (*m);
}

template <class C>
static db::simple_polygon<C> trans_simple_polygon (const db::matrix_2d<C> *m, const db::simple_polygon<C> &p)
{
  return p.transformed (*m);
}

template <class C>
static db::box<C> trans_box (const db::matrix_2d<C> *m, const db::box<C> &p)
{
  return p.transformed (*m);
}

template <class C>
static db::edge<C> trans_edge (const db::matrix_2d<C> *m, const db::edge<C> &e)
{
  return e.transformed (*m);
}

template <class C>
static double coeff_m (const db::matrix_2d<C> *m, int i, int j)
{
  if (i == 0 && j == 0) {
    return m->m11 ();
  } else if (i == 0 && j == 1) {
    return m->m12 ();
  } else if (i == 1 && j == 0) {
    return m->m21 ();
  } else if (i == 1 && j == 1) {
    return m->m22 ();
  } else {
    return 0.0;
  }
}

template <class C>
gsi::Methods
matrix2d_methods ()
{
  return
    gsi::constructor ("new", &new_matrix2d<C>,
      "@brief Create a new Matrix2d representing a unit transformation"
    ) +
    gsi::constructor ("new", &new_matrix2d_m<C>, gsi::arg ("m"),
      "@brief Create a new Matrix2d representing an isotropic magnification\n"
      "@param m The magnification\n"
    ) +
    gsi::constructor ("new", &new_matrix2d_m2<C>, gsi::arg ("mx"), gsi::arg ("my"),
      "@brief Create a new Matrix2d representing an anisotropic magnification\n"
      "@param mx The magnification in x direction\n"
      "@param my The magnification in y direction\n"
    ) +
    gsi::constructor ("new", &new_matrix2d_t<C>, gsi::arg ("t"),
      "@brief Create a new Matrix2d from the given complex transformation"
      "@param t The transformation from which to create the matrix (not taking into account the displacement)\n"
    ) +
    gsi::constructor ("newc", &new_matrix2d_mrm<C>, gsi::arg ("mag"), gsi::arg ("rotation"), gsi::arg ("mirror"),
      "@brief Create a new Matrix2d representing an isotropic magnification, rotation and mirroring\n"
      "@param mag The magnification in x direction\n"
      "@param rotation The rotation angle (in degree)\n"
      "@param mirror The mirror flag (at x axis)\n"
      "\n"
      "This constructor is provided to construct a matrix similar to the complex transformation.\n"
      "This constructor is called 'newc' to distinguish it from the constructors taking matrix coefficients ('c' is for composite).\n"
      "The order of execution of the operations is mirror, magnification, rotation (as for complex transformations).\n"
    ) +
    gsi::constructor ("newc", &new_matrix2d_smrm<C>, gsi::arg ("shear"), gsi::arg ("mx"), gsi::arg ("my"), gsi::arg ("rotation"), gsi::arg ("mirror"),
      "@brief Create a new Matrix2d representing a shear, anisotropic magnification, rotation and mirroring\n"
      "@param shear The shear angle\n"
      "@param mx The magnification in x direction\n"
      "@param my The magnification in y direction\n"
      "@param rotation The rotation angle (in degree)\n"
      "@param mirror The mirror flag (at x axis)\n"
      "\n"
      "The order of execution of the operations is mirror, magnification, shear and rotation.\n"
      "This constructor is called 'newc' to distinguish it from the constructor taking the four matrix coefficients ('c' is for composite).\n"
    ) +
    gsi::constructor ("new", &new_matrix2d_m4<C>, gsi::arg ("m11"), gsi::arg ("m12"), gsi::arg ("m21"), gsi::arg ("m22"),
      "@brief Create a new Matrix2d from the four coefficients\n"
    ) +
    gsi::method ("m11", &db::matrix_2d<C>::m11,
      "@brief Gets the m11 coefficient.\n"
      "@return The value of the m11 coefficient\n"
    ) +
    gsi::method ("m12", &db::matrix_2d<C>::m12,
      "@brief Gets the m12 coefficient.\n"
      "@return The value of the m12 coefficient\n"
    ) +
    gsi::method ("m21", &db::matrix_2d<C>::m21,
      "@brief Gets the m21 coefficient.\n"
      "@return The value of the m21 coefficient\n"
    ) +
    gsi::method ("m22", &db::matrix_2d<C>::m22,
      "@brief Gets the m22 coefficient.\n"
      "@return The value of the m22 coefficient\n"
    ) +
    gsi::method_ext ("m", &coeff_m<C>, gsi::arg ("i"), gsi::arg ("j"),
      "@brief Gets the m coefficient with the given index.\n"
      "@return The coefficient [i,j]\n"
    ) +
    gsi::method ("to_s", &db::matrix_2d<C>::to_string,
      "@brief Convert the matrix to a string.\n"
      "@return The string representing this matrix\n"
    ) +
    gsi::method ("inverted", &db::matrix_2d<C>::inverted,
      "@brief The inverse of this matrix.\n"
      "@return The inverse of this matrix\n"
    ) +
    gsi::method_ext ("trans|*", &trans_p<C>, gsi::arg ("p"),
      "@brief Transforms a point with this matrix.\n"
      "@param p The point to transform.\n"
      "@return The transformed point\n"
    ) +
    gsi::method_ext ("*", &trans_v<C>, gsi::arg ("v"),
      "@brief Transforms a vector with this matrix.\n"
      "@param v The vector to transform.\n"
      "@return The transformed vector\n"
    ) +
    gsi::method_ext ("*", &trans_edge<C>, gsi::arg ("e"),
      "@brief Transforms an edge with this matrix.\n"
      "@param e The edge to transform.\n"
      "@return The transformed edge\n"
    ) +
    gsi::method_ext ("*", &trans_box<C>, gsi::arg ("box"),
      "@brief Transforms a box with this matrix.\n"
      "@param box The box to transform.\n"
      "@return The transformed box\n"
      "\n"
      "Please note that the box remains a box, even though the matrix supports shear and rotation. The returned box "
      "will be the bounding box of the sheared and rotated rectangle."
    ) +
    gsi::method_ext ("*", &trans_simple_polygon<C>, gsi::arg ("p"),
      "@brief Transforms a simple polygon with this matrix.\n"
      "@param p The simple polygon to transform.\n"
      "@return The transformed simple polygon\n"
    ) +
    gsi::method_ext ("*", &trans_polygon<C>, gsi::arg ("p"),
      "@brief Transforms a polygon with this matrix.\n"
      "@param p The polygon to transform.\n"
      "@return The transformed polygon\n"
    ) +
    gsi::method_ext ("*", &prod_m<C>, gsi::arg ("m"),
      "@brief Product of two matrices.\n"
      "@param m The other matrix.\n"
      "@return The matrix product self*m\n"
    ) +
    gsi::method_ext ("+", &sum_m<C>, gsi::arg ("m"),
      "@brief Sum of two matrices.\n"
      "@param m The other matrix.\n"
      "@return The (element-wise) sum of self+m\n"
    ) +
    gsi::method_ext ("cplx_trans", &to_cplx_trans<C>,
      "@brief Converts this matrix to a complex transformation (if possible).\n"
      "@return The complex transformation.\n"
      "This method is successful only if the matrix does not contain shear components and the magnification must be isotropic.\n"
    ) +
    gsi::method ("angle", &db::matrix_2d<C>::angle,
      "@brief Returns the rotation angle of the rotation component of this matrix.\n"
      "@return The angle in degree.\n"
      "The matrix is decomposed into basic transformations assuming an execution order of "
      "mirroring at the x axis, rotation, magnification and shear."
    ) +
    gsi::method ("mag_x", (double (db::matrix_2d<C>::*) () const) &db::matrix_2d<C>::mag_x,
      "@brief Returns the x magnification of the magnification component of this matrix.\n"
      "@return The magnification factor.\n"
      "The matrix is decomposed into basic transformations assuming an execution order of "
      "mirroring at the x axis, magnification, shear and rotation."
    ) +
    gsi::method ("mag_y", (double (db::matrix_2d<C>::*) () const) &db::matrix_2d<C>::mag_y,
      "@brief Returns the y magnification of the magnification component of this matrix.\n"
      "@return The magnification factor.\n"
      "The matrix is decomposed into basic transformations assuming an execution order of "
      "mirroring at the x axis, magnification, shear and rotation."
    ) +
    gsi::method ("shear_angle", &db::matrix_2d<C>::shear_angle,
      "@brief Returns the magnitude of the shear component of this matrix.\n"
      "@return The shear angle in degree.\n"
      "The matrix is decomposed into basic transformations assuming an execution order of "
      "mirroring at the x axis, rotation, magnification and shear.\n"
      "The shear basic transformation will tilt the x axis towards the y axis and vice versa. The shear angle "
      "gives the tilt angle of the axes towards the other one. The possible range for this angle is -45 to 45 degree."
    ) +
    gsi::method ("is_mirror?", &db::matrix_2d<C>::is_mirror,
      "@brief Returns the mirror flag of this matrix.\n"
      "@return True if this matrix has a mirror component.\n"
      "The matrix is decomposed into basic transformations assuming an execution order of "
      "mirroring at the x axis, rotation, magnification and shear."
    );
}

gsi::Class<db::Matrix2d> decl_Matrix2d ("db", "Matrix2d",
  matrix2d_methods<db::DCoord> (),
  "@brief A 2d matrix object used mainly for representing rotation and shear transformations.\n"
  "\n"
  "This object represents a 2x2 matrix. This matrix is used to implement affine transformations "
  "in the 2d space mainly. It can be decomposed into basic transformations: mirroring, rotation and shear. "
  "In that case, the assumed execution order of the basic transformations is "
  "mirroring at the x axis, rotation, magnification and shear.\n"
  "\n"
  "The matrix is a generalization of the transformations and is of limited use in a layout database context. "
  "It is useful however to implement shear transformations on polygons, edges and polygon or edge collections."
  "\n\n"
  "This class was introduced in version 0.22.\n"
);

gsi::Class<db::IMatrix2d> decl_IMatrix2d ("db", "IMatrix2d",
  matrix2d_methods<db::Coord> (),
  "@brief A 2d matrix object used mainly for representing rotation and shear transformations (integer coordinate version).\n"
  "\n"
  "This object represents a 2x2 matrix. This matrix is used to implement affine transformations "
  "in the 2d space mainly. It can be decomposed into basic transformations: mirroring, rotation and shear. "
  "In that case, the assumed execution order of the basic transformations is "
  "mirroring at the x axis, rotation, magnification and shear."
  "\n\n"
  "The integer variant was introduced in version 0.27.\n"
);

// ---------------------------------------------------------------
//  Matrix3d binding

template <class C>
static db::matrix_3d<C> *new_matrix3d ()
{
  return new db::matrix_3d<C> (1.0);
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_t (const db::complex_trans<C, C> &t)
{
  return new db::matrix_3d<C> (t);
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_m (double mag)
{
  return new db::matrix_3d<C> (mag);
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_mrm (double mag, double rot, bool m)
{
  return new db::matrix_3d<C> (db::matrix_3d<C>::rotation (rot) * db::matrix_3d<C>::mag (mag) * db::matrix_3d<C>::mirror (m));
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_smrm (double shear, double mx, double my, double rot, bool m)
{
  return new db::matrix_3d<C> (db::matrix_3d<C>::rotation (rot) * db::matrix_3d<C>::shear (shear) * db::matrix_3d<C>::mag (mx, my) * db::matrix_3d<C>::mirror (m));
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_dsmrm (const db::vector<C> &d, double shear, double mx, double my, double rot, bool m)
{
  return new db::matrix_3d<C> (db::matrix_3d<C>::disp (d) * db::matrix_3d<C>::rotation (rot) * db::matrix_3d<C>::shear (shear) * db::matrix_3d<C>::mag (mx, my) * db::matrix_3d<C>::mirror (m));
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_pdsmrm (double tx, double ty, double z, const db::vector<C> &d, double shear, double mx, double my, double rot, bool m)
{
  return new db::matrix_3d<C> (db::matrix_3d<C>::disp (d) * db::matrix_3d<C>::perspective (tx, ty, z) * db::matrix_3d<C>::rotation (rot) * db::matrix_3d<C>::shear (shear) * db::matrix_3d<C>::mag (mx, my) * db::matrix_3d<C>::mirror (m));
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_m4 (double m11, double m12, double m21, double m22)
{
  return new db::matrix_3d<C> (m11, m12, m21, m22);
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_m6 (double m11, double m12, double m21, double m22, double dx, double dy)
{
  return new db::matrix_3d<C> (m11, m12, m21, m22, dx, dy, 0.0, 0.0);
}

template <class C>
static db::matrix_3d<C> *new_matrix3d_m9 (double m11, double m12, double m13, double m21, double m22, double m23, double m31, double m32, double m33)
{
  return new db::matrix_3d<C> (m11, m12, m13, m21, m22, m23, m31, m32, m33);
}

template <class C>
static db::DCplxTrans to_cplx_trans3 (const db::matrix_3d<C> *m)
{
  return db::DCplxTrans (*m);
}

template <class C>
static db::matrix_3d<C> sum_m3 (const db::matrix_3d<C> *m, const db::matrix_3d<C> &d)
{
  return *m + d;
}

template <class C>
static db::matrix_3d<C> prod_m3 (const db::matrix_3d<C> *m, const db::matrix_3d<C> &d)
{
  return *m * d;
}

template <class C>
static db::point<C> trans_p3 (const db::matrix_3d<C> *m, const db::point<C> &p)
{
  return *m * p;
}

template <class C>
static db::vector<C> trans_v3 (const db::matrix_3d<C> *m, const db::vector<C> &p)
{
  return *m * p;
}

template <class C>
static db::polygon<C> trans_polygon3 (const db::matrix_3d<C> *m, const db::polygon<C> &p)
{
  return p.transformed (*m);
}

template <class C>
static db::simple_polygon<C> trans_simple_polygon3 (const db::matrix_3d<C> *m, const db::simple_polygon<C> &p)
{
  return p.transformed (*m);
}

template <class C>
static db::box<C> trans_box3 (const db::matrix_3d<C> *m, const db::box<C> &p)
{
  return p.transformed (*m);
}

template <class C>
static db::edge<C> trans_edge3 (const db::matrix_3d<C> *m, const db::edge<C> &e)
{
  return e.transformed (*m);
}

template <class C>
static double coeff_m3 (const db::matrix_3d<C> *m, int i, int j)
{
  if (i < 0 || i >= 3 || j < 0 || j >= 3) {
    return 0.0;
  } else {
    return m->m ()[i][j];
  }
}

static void adjust (db::Matrix3d *m, const std::vector <db::DPoint> &landmarks_before, const std::vector <db::DPoint> &landmarks_after, int flags, int fixed_point)
{
  db::adjust_matrix (*m, landmarks_before, landmarks_after, db::MatrixAdjustFlags::Flags (flags), fixed_point);
}

static int adjust_none ()
{
  return db::MatrixAdjustFlags::None;
}

static int adjust_displacement ()
{
  return db::MatrixAdjustFlags::Displacement;
}

static int adjust_rotation ()
{
  return db::MatrixAdjustFlags::Rotation;
}

static int adjust_rotation_mirror ()
{
  return db::MatrixAdjustFlags::RotationMirror;
}

static int adjust_magnification ()
{
  return db::MatrixAdjustFlags::Magnification;
}

static int adjust_shear ()
{
  return db::MatrixAdjustFlags::Shear;
}

static int adjust_perspective ()
{
  return db::MatrixAdjustFlags::Perspective;
}

static int adjust_all ()
{
  return db::MatrixAdjustFlags::All;
}

template <class C>
gsi::Methods
matrix3d_methods ()
{
  return
    gsi::constructor ("new", &new_matrix3d<C>,
      "@brief Create a new Matrix3d representing a unit transformation"
    ) +
    gsi::constructor ("new", &new_matrix3d_m<C>, gsi::arg ("m"),
      "@brief Create a new Matrix3d representing a magnification\n"
      "@param m The magnification\n"
    ) +
    gsi::constructor ("new", &new_matrix3d_t<C>, gsi::arg ("t"),
      "@brief Create a new Matrix3d from the given complex transformation"
      "@param t The transformation from which to create the matrix\n"
    ) +
    gsi::constructor ("newc", &new_matrix3d_mrm<C>, gsi::arg ("mag"), gsi::arg ("rotation"), gsi::arg ("mirrx"),
      "@brief Create a new Matrix3d representing a isotropic magnification, rotation and mirroring\n"
      "@param mag The magnification\n"
      "@param rotation The rotation angle (in degree)\n"
      "@param mirrx The mirror flag (at x axis)\n"
      "\n"
      "The order of execution of the operations is mirror, magnification and rotation.\n"
      "This constructor is called 'newc' to distinguish it from the constructors taking coefficients ('c' is for composite).\n"
    ) +
    gsi::constructor ("newc", &new_matrix3d_smrm<C>, gsi::arg ("shear"), gsi::arg ("mx"), gsi::arg ("my"), gsi::arg ("rotation"), gsi::arg ("mirrx"),
      "@brief Create a new Matrix3d representing a shear, anisotropic magnification, rotation and mirroring\n"
      "@param shear The shear angle\n"
      "@param mx The magnification in x direction\n"
      "@param mx The magnification in y direction\n"
      "@param rotation The rotation angle (in degree)\n"
      "@param mirrx The mirror flag (at x axis)\n"
      "\n"
      "The order of execution of the operations is mirror, magnification, rotation and shear.\n"
      "This constructor is called 'newc' to distinguish it from the constructor taking the four matrix coefficients ('c' is for composite).\n"
    ) +
    gsi::constructor ("newc", &new_matrix3d_dsmrm<C>, gsi::arg ("u"), gsi::arg ("shear"), gsi::arg ("mx"), gsi::arg ("my"), gsi::arg ("rotation"), gsi::arg ("mirrx"),
      "@brief Create a new Matrix3d representing a displacement, shear, anisotropic magnification, rotation and mirroring\n"
      "@param u The displacement\n"
      "@param shear The shear angle\n"
      "@param mx The magnification in x direction\n"
      "@param mx The magnification in y direction\n"
      "@param rotation The rotation angle (in degree)\n"
      "@param mirrx The mirror flag (at x axis)\n"
      "\n"
      "The order of execution of the operations is mirror, magnification, rotation, shear and displacement.\n"
      "This constructor is called 'newc' to distinguish it from the constructor taking the four matrix coefficients ('c' is for composite).\n"
      "\n"
      "Starting with version 0.25 the displacement is of vector type."
    ) +
    gsi::constructor ("newc", &new_matrix3d_pdsmrm<C>, gsi::arg ("tx"), gsi::arg ("ty"), gsi::arg ("z"), gsi::arg ("u"), gsi::arg ("shear"), gsi::arg ("mx"), gsi::arg ("my"), gsi::arg ("rotation"), gsi::arg ("mirrx"),
      "@brief Create a new Matrix3d representing a perspective distortion, displacement, shear, anisotropic magnification, rotation and mirroring\n"
      "@param tx The perspective tilt angle x (around the y axis)\n"
      "@param ty The perspective tilt angle y (around the x axis)\n"
      "@param z The observer distance at which the tilt angles are given\n"
      "@param u The displacement\n"
      "@param shear The shear angle\n"
      "@param mx The magnification in x direction\n"
      "@param mx The magnification in y direction\n"
      "@param rotation The rotation angle (in degree)\n"
      "@param mirrx The mirror flag (at x axis)\n"
      "\n"
      "The order of execution of the operations is mirror, magnification, rotation, shear, perspective distortion and displacement.\n"
      "This constructor is called 'newc' to distinguish it from the constructor taking the four matrix coefficients ('c' is for composite).\n"
      "\n"
      "The tx and ty parameters represent the perspective distortion. They denote a tilt of the xy plane around the y axis (tx) or the x axis (ty) in degree. "
      "The same effect is achieved for different tilt angles for different observer distances. Hence, the observer distance must be given at which the tilt angles are given. "
      "If the magnitude of the tilt angle is not important, z can be set to 1.\n"
      "\n"
      "Starting with version 0.25 the displacement is of vector type."
    ) +
    gsi::constructor ("new", &new_matrix3d_m4<C>, gsi::arg ("m11"), gsi::arg ("m12"), gsi::arg ("m21"), gsi::arg ("m22"),
      "@brief Create a new Matrix3d from the four coefficients of a Matrix2d\n"
    ) +
    gsi::constructor ("new", &new_matrix3d_m6<C>, gsi::arg ("m11"), gsi::arg ("m12"), gsi::arg ("m21"), gsi::arg ("m22"), gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Create a new Matrix3d from the four coefficients of a Matrix2d plus a displacement\n"
    ) +
    gsi::constructor ("new", &new_matrix3d_m9<C>, gsi::arg ("m11"), gsi::arg ("m12"), gsi::arg ("m13"), gsi::arg ("m21"), gsi::arg ("m22"), gsi::arg ("m23"), gsi::arg ("m31"), gsi::arg ("m32"), gsi::arg ("m33"),
      "@brief Create a new Matrix3d from the nine matrix coefficients\n"
    ) +
    gsi::method_ext ("m", &coeff_m3<C>, gsi::arg ("i"), gsi::arg ("j"),
      "@brief Gets the m coefficient with the given index.\n"
      "@return The coefficient [i,j]\n"
    ) +
    gsi::method ("to_s", &db::matrix_3d<C>::to_string,
      "@brief Convert the matrix to a string.\n"
      "@return The string representing this matrix\n"
    ) +
    gsi::method ("inverted", &db::matrix_3d<C>::inverted,
      "@brief The inverse of this matrix.\n"
      "@return The inverse of this matrix\n"
    ) +
    gsi::method_ext ("*", &prod_m3<C>, gsi::arg ("m"),
      "@brief Product of two matrices.\n"
      "@param m The other matrix.\n"
      "@return The matrix product self*m\n"
    ) +
    gsi::method_ext ("+", &sum_m3<C>, gsi::arg ("m"),
      "@brief Sum of two matrices.\n"
      "@param m The other matrix.\n"
      "@return The (element-wise) sum of self+m\n"
    ) +
    gsi::method_ext ("trans|*", &trans_p3<C>, gsi::arg ("p"),
      "@brief Transforms a point with this matrix.\n"
      "@param p The point to transform.\n"
      "@return The transformed point\n"
    ) +
    gsi::method_ext ("*", &trans_v3<C>, gsi::arg ("v"),
      "@brief Transforms a vector with this matrix.\n"
      "@param v The vector to transform.\n"
      "@return The transformed vector\n"
    ) +
    gsi::method_ext ("*", &trans_edge3<C>, gsi::arg ("e"),
      "@brief Transforms an edge with this matrix.\n"
      "@param e The edge to transform.\n"
      "@return The transformed edge\n"
    ) +
    gsi::method_ext ("*", &trans_box3<C>, gsi::arg ("box"),
      "@brief Transforms a box with this matrix.\n"
      "@param box The box to transform.\n"
      "@return The transformed box\n"
      "\n"
      "Please note that the box remains a box, even though the matrix supports shear and rotation. The returned box "
      "will be the bounding box of the sheared and rotated rectangle."
    ) +
    gsi::method_ext ("*", &trans_simple_polygon3<C>, gsi::arg ("p"),
      "@brief Transforms a simple polygon with this matrix.\n"
      "@param p The simple polygon to transform.\n"
      "@return The transformed simple polygon\n"
    ) +
    gsi::method_ext ("*", &trans_polygon3<C>, gsi::arg ("p"),
      "@brief Transforms a polygon with this matrix.\n"
      "@param p The polygon to transform.\n"
      "@return The transformed polygon\n"
    ) +
    gsi::method_ext ("cplx_trans", &to_cplx_trans3<C>,
      "@brief Converts this matrix to a complex transformation (if possible).\n"
      "@return The complex transformation.\n"
      "This method is successful only if the matrix does not contain shear or perspective distortion components and the magnification must be isotropic.\n"
    ) +
    gsi::method ("mag_x", (double (db::matrix_3d<C>::*) () const) &db::matrix_3d<C>::mag_x,
      "@brief Returns the x magnification of the magnification component of this matrix.\n"
      "@return The magnification factor.\n"
    ) +
    gsi::method ("mag_y", (double (db::matrix_3d<C>::*) () const) &db::matrix_3d<C>::mag_y,
      "@brief Returns the y magnification of the magnification component of this matrix.\n"
      "@return The magnification factor.\n"
    ) +
    gsi::method ("angle", &db::matrix_3d<C>::angle,
      "@brief Returns the rotation angle of the rotation component of this matrix.\n"
      "@return The angle in degree.\n"
      "See the description of this class for details about the basic transformations."
    ) +
    gsi::method ("shear_angle", &db::matrix_3d<C>::shear_angle,
      "@brief Returns the magnitude of the shear component of this matrix.\n"
      "@return The shear angle in degree.\n"
      "The shear basic transformation will tilt the x axis towards the y axis and vice versa. The shear angle "
      "gives the tilt angle of the axes towards the other one. The possible range for this angle is -45 to 45 degree."
      "See the description of this class for details about the basic transformations."
    ) +
    gsi::method ("disp", (db::vector<C> (db::matrix_3d<C>::*) () const) &db::matrix_3d<C>::disp,
      "@brief Returns the displacement vector of this transformation.\n"
      "\n"
      "Starting with version 0.25 this method returns a vector type instead of a point.\n"
      "@return The displacement vector.\n"
    ) +
    gsi::method ("tx", &db::matrix_3d<C>::perspective_tilt_x, gsi::arg ("z"),
      "@brief Returns the perspective tilt angle tx.\n"
      "@param z The observer distance at which the tilt angle is computed.\n"
      "@return The tilt angle tx.\n"
      "The tx and ty parameters represent the perspective distortion. They denote a tilt of the xy plane around the y axis (tx) or the x axis (ty) in degree. "
      "The same effect is achieved for different tilt angles at different observer distances. Hence, the observer distance must be specified at which the tilt angle is computed. "
      "If the magnitude of the tilt angle is not important, z can be set to 1.\n"
    ) +
    gsi::method ("ty", &db::matrix_3d<C>::perspective_tilt_y, gsi::arg ("z"),
      "@brief Returns the perspective tilt angle ty.\n"
      "@param z The observer distance at which the tilt angle is computed.\n"
      "@return The tilt angle ty.\n"
      "The tx and ty parameters represent the perspective distortion. They denote a tilt of the xy plane around the y axis (tx) or the x axis (ty) in degree. "
      "The same effect is achieved for different tilt angles at different observer distances. Hence, the observer distance must be specified at which the tilt angle is computed. "
      "If the magnitude of the tilt angle is not important, z can be set to 1.\n"
    ) +
    gsi::method ("is_mirror?", &db::matrix_3d<C>::is_mirror,
      "@brief Returns the mirror flag of this matrix.\n"
      "@return True if this matrix has a mirror component.\n"
      "See the description of this class for details about the basic transformations."
    );
}


gsi::Class<db::Matrix3d> decl_Matrix3d ("db", "Matrix3d",
  matrix3d_methods<db::DCoord> () +
  gsi::method_ext ("adjust", &adjust, gsi::arg ("landmarks_before"), gsi::arg ("landmarks_after"), gsi::arg ("flags"), gsi::arg ("fixed_point"),
    "@brief Adjust a 3d matrix to match the given set of landmarks\n"
    "\n"
    "This function tries to adjust the matrix\n"
    "such, that either the matrix is changed as little as possible (if few landmarks are given) \n"
    "or that the \"after\" landmarks will match as close as possible to the \"before\" landmarks \n"
    "(if the problem is overdetermined).\n"
    "\n"
    "@param landmarks_before The points before the transformation.\n"
    "@param landmarks_after The points after the transformation.\n"
    "@param mode Selects the adjustment mode. Must be one of the Adjust... constants.\n"
    "@param fixed_point The index of the fixed point (one that is definitely mapped to the target) or -1 if there is none\n"
  ) +
  gsi::method ("AdjustNone", &adjust_none,
    "@brief Mode for \\adjust: adjust nothing\n"
  ) +
  gsi::method ("AdjustDisplacement", &adjust_displacement,
    "@brief Mode for \\adjust: adjust displacement only\n"
  ) +
  gsi::method ("AdjustRotation", &adjust_rotation,
    "@brief Mode for \\adjust: adjust rotation only\n"
  ) +
  gsi::method ("AdjustRotationMirror", &adjust_rotation_mirror,
    "@brief Mode for \\adjust: adjust rotation and mirror option\n"
  ) +
  gsi::method ("AdjustMagnification", &adjust_magnification,
    "@brief Mode for \\adjust: adjust rotation, mirror option and magnification\n"
  ) +
  gsi::method ("AdjustShear", &adjust_shear,
    "@brief Mode for \\adjust: adjust rotation, mirror option, magnification and shear\n"
  ) +
  gsi::method ("AdjustPerspective", &adjust_perspective,
    "@brief Mode for \\adjust: adjust whole matrix including perspective transformation\n"
  ) +
  gsi::method ("AdjustAll", &adjust_all,
    "@brief Mode for \\adjust: currently equivalent to \\adjust_perspective\n"
  ),
  "@brief A 3d matrix object used mainly for representing rotation, shear, displacement and perspective transformations.\n"
  "\n"
  "This object represents a 3x3 matrix. This matrix is used to implement generic geometrical transformations "
  "in the 2d space mainly. It can be decomposed into basic transformations: mirroring, rotation, shear, displacement and perspective distortion. "
  "In that case, the assumed execution order of the basic transformations is "
  "mirroring at the x axis, rotation, magnification, shear, displacement and perspective distortion."
  "\n\n"
  "This class was introduced in version 0.22.\n"
);

gsi::Class<db::IMatrix3d> decl_IMatrix3d ("db", "IMatrix3d",
  matrix3d_methods<db::Coord> (),
  "@brief A 3d matrix object used mainly for representing rotation, shear, displacement and perspective transformations (integer coordinate version).\n"
  "\n"
  "This object represents a 3x3 matrix. This matrix is used to implement generic geometrical transformations "
  "in the 2d space mainly. It can be decomposed into basic transformations: mirroring, rotation, shear, displacement and perspective distortion. "
  "In that case, the assumed execution order of the basic transformations is "
  "mirroring at the x axis, rotation, magnification, shear, displacement and perspective distortion."
  "\n\n"
  "The integer variant was introduced in version 0.27.\n"
);

}
