
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


#include "gsiDecl.h"
#include "dbMatrix.h"
#include "dbTrans.h"

namespace gsi
{

// ---------------------------------------------------------------
//  Matrix2d binding

static db::Matrix2d *new_matrix2d ()
{
  return new db::Matrix2d (1.0);
}

static db::Matrix2d *new_matrix2d_m (double mag)
{
  return new db::Matrix2d (mag);
}

static db::Matrix2d *new_matrix2d_m2 (double mx, double my)
{
  return new db::Matrix2d (mx, my);
}

static db::Matrix2d *new_matrix2d_t (const db::DCplxTrans &t)
{
  return new db::Matrix2d (t);
}

static db::Matrix2d *new_matrix2d_mrm (double mag, double rot, bool m)
{
  return new db::Matrix2d (db::Matrix2d::rotation (rot) * db::Matrix2d::mag (mag) * db::Matrix2d::mirror (m));
}

static db::Matrix2d *new_matrix2d_smrm (double shear, double mx, double my, double rot, bool m)
{
  return new db::Matrix2d (db::Matrix2d::rotation (rot) * db::Matrix2d::shear (shear) * db::Matrix2d::mag (mx, my) * db::Matrix2d::mirror (m));
}

static db::Matrix2d *new_matrix2d_m4 (double m11, double m12, double m21, double m22)
{
  return new db::Matrix2d (m11, m12, m21, m22);
}

static db::DCplxTrans to_cplx_trans (const db::Matrix2d *m)
{
  return db::DCplxTrans (db::Matrix3d (*m));
}

static db::Matrix2d sum_m (const db::Matrix2d *m, const db::Matrix2d &d)
{
  return *m + d;
}

static db::Matrix2d prod_m (const db::Matrix2d *m, const db::Matrix2d &d)
{
  return *m * d;
}

static db::DPoint trans_p (const db::Matrix2d *m, const db::DPoint &p)
{
  return *m * p;
}

static double coeff_m (const db::Matrix2d *m, int i, int j)
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

gsi::Class<db::Matrix2d> decl_Matrix2d ("db", "Matrix2d",
  gsi::constructor ("new", &new_matrix2d,
    "@brief Create a new Matrix2d representing a unit transformation"
  ) +
  gsi::constructor ("new", &new_matrix2d_m,
    "@brief Create a new Matrix2d representing an isotropic magnification\n"
    "@args m\n"
    "@param m The magnification\n"
  ) +
  gsi::constructor ("new", &new_matrix2d_m2,
    "@brief Create a new Matrix2d representing an anisotropic magnification\n"
    "@args mx, my\n"
    "@param mx The magnification in x direction\n"
    "@param my The magnification in y direction\n"
  ) +
  gsi::constructor ("new", &new_matrix2d_t,
    "@brief Create a new Matrix2d from the given complex transformation"
    "@args t\n"
    "@param t The transformation from which to create the matrix (not taking into account the displacement)\n"
  ) +
  gsi::constructor ("newc", &new_matrix2d_mrm, 
    "@brief Create a new Matrix2d representing an isotropic magnification, rotation and mirroring\n"
    "@args mag, rotation, mirror\n"
    "@param mag The magnification in x direction\n"
    "@param rotation The rotation angle (in degree)\n"
    "@param mirror The mirror flag (at x axis)\n"
    "\n"
    "This constructor is provided to construct a matrix similar to the complex transformation.\n"
    "This constructor is called 'newc' to distinguish it from the constructors taking matrix coefficients ('c' is for composite).\n"
    "The order of execution of the operations is mirror, magnification, rotation (as for complex transformations).\n"
  ) +
  gsi::constructor ("newc", &new_matrix2d_smrm, 
    "@brief Create a new Matrix2d representing a shear, anisotropic magnification, rotation and mirroring\n"
    "@args shear, mx, my, rotation, mirror\n"
    "@param shear The shear angle\n"
    "@param mx The magnification in x direction\n"
    "@param my The magnification in y direction\n"
    "@param rotation The rotation angle (in degree)\n"
    "@param mirror The mirror flag (at x axis)\n"
    "\n"
    "The order of execution of the operations is mirror, magnification, shear and rotation.\n"
    "This constructor is called 'newc' to distinguish it from the constructor taking the four matrix coefficients ('c' is for composite).\n"
  ) +
  gsi::constructor ("new", &new_matrix2d_m4, 
    "@brief Create a new Matrix2d from the four coefficients\n"
    "@args m11, m12, m21, m22\n"
  ) +
  gsi::method ("m11", &db::Matrix2d::m11, 
    "@brief Gets the m11 coefficient.\n"
    "@return The value of the m11 coefficient\n"
  ) +
  gsi::method ("m12", &db::Matrix2d::m12, 
    "@brief Gets the m12 coefficient.\n"
    "@return The value of the m12 coefficient\n"
  ) +
  gsi::method ("m21", &db::Matrix2d::m21, 
    "@brief Gets the m21 coefficient.\n"
    "@return The value of the m21 coefficient\n"
  ) +
  gsi::method ("m22", &db::Matrix2d::m22, 
    "@brief Gets the m22 coefficient.\n"
    "@return The value of the m22 coefficient\n"
  ) +
  gsi::method_ext ("m", &coeff_m, 
    "@brief Gets the m coefficient with the given index.\n"
    "@args i,j\n"
    "@return The coefficient [i,j]\n"
  ) +
  gsi::method ("to_s", &db::Matrix2d::to_string, 
    "@brief Convert the matrix to a string.\n"
    "@return The string representing this matrix\n"
  ) +
  gsi::method ("inverted", &db::Matrix2d::inverted, 
    "@brief The inverse of this matrix.\n"
    "@return The inverse of this matrix\n"
  ) +
  gsi::method_ext ("trans", &trans_p,
    "@brief Transforms a point with this matrix.\n"
    "@args p\n"
    "@param p The point to transform.\n"
    "@return The product if self and the point p\n"
  ) +
  gsi::method_ext ("*", &prod_m, 
    "@brief Product of two matrices.\n"
    "@args m\n"
    "@param m The other matrix.\n"
    "@return The matrix product self*m\n"
  ) +
  gsi::method_ext ("+", &sum_m, 
    "@brief Sum of two matrices.\n"
    "@args m\n"
    "@param m The other matrix.\n"
    "@return The (element-wise) sum of self+m\n"
  ) +
  gsi::method_ext ("cplx_trans", &to_cplx_trans, 
    "@brief Converts this matrix to a complex transformation (if possible).\n"
    "@return The complex transformation.\n"
    "This method is successful only if the matrix does not contain shear components and the magnification must be isotropic.\n"
  ) +
  gsi::method ("angle", &db::Matrix2d::angle,
    "@brief Returns the rotation angle of the rotation component of this matrix.\n"
    "@return The angle in degree.\n"
    "The matrix is decomposed into basic transformations assuming an execution order of "
    "mirroring at the x axis, rotation, magnification and shear."
  ) +
  gsi::method ("mag_x", (double (db::Matrix2d::*) () const) &db::Matrix2d::mag_x,
    "@brief Returns the x magnification of the magnification component of this matrix.\n"
    "@return The magnification factor.\n"
    "The matrix is decomposed into basic transformations assuming an execution order of "
    "mirroring at the x axis, magnification, shear and rotation."
  ) +
  gsi::method ("mag_y", (double (db::Matrix2d::*) () const) &db::Matrix2d::mag_y,
    "@brief Returns the y magnification of the magnification component of this matrix.\n"
    "@return The magnification factor.\n"
    "The matrix is decomposed into basic transformations assuming an execution order of "
    "mirroring at the x axis, magnification, shear and rotation."
  ) +
  gsi::method ("shear_angle", &db::Matrix2d::shear_angle,
    "@brief Returns the magnitude of the shear component of this matrix.\n"
    "@return The shear angle in degree.\n"
    "The matrix is decomposed into basic transformations assuming an execution order of "
    "mirroring at the x axis, rotation, magnification and shear.\n"
    "The shear basic transformation will tilt the x axis towards the y axis and vice versa. The shear angle "
    "gives the tilt angle of the axes towards the other one. The possible range for this angle is -45 to 45 degree."
  ) +
  gsi::method ("is_mirror?", &db::Matrix2d::is_mirror,
    "@brief Returns the mirror flag of this matrix.\n"
    "@return True if this matrix has a mirror component.\n"
    "The matrix is decomposed into basic transformations assuming an execution order of "
    "mirroring at the x axis, rotation, magnification and shear."
  ),
  "@brief A 2d matrix object used mainly for representing rotation and shear transformations.\n"
  "\n"
  "This object represents a 2x2 matrix. This matrix is used to represent affine transformations "
  "in the 2d space mainly. It can be decomposed into basic transformations: mirroring, rotation and shear. "
  "In that case, the assumed execution order of the basic transformations is "
  "mirroring at the x axis, rotation, magnification and shear."
  "\n\n"
  "This class was introduced in version 0.22.\n"
);

// ---------------------------------------------------------------
//  Matrix2d binding

static db::Matrix3d *new_matrix3d ()
{
  return new db::Matrix3d (1.0);
}

static db::Matrix3d *new_matrix3d_t (const db::DCplxTrans &t)
{
  return new db::Matrix3d (t);
}

static db::Matrix3d *new_matrix3d_m (double mag)
{
  return new db::Matrix3d (mag);
}

static db::Matrix3d *new_matrix3d_mrm (double mag, double rot, bool m)
{
  return new db::Matrix3d (db::Matrix3d::rotation (rot) * db::Matrix3d::mag (mag) * db::Matrix3d::mirror (m));
}

static db::Matrix3d *new_matrix3d_smrm (double shear, double mx, double my, double rot, bool m)
{
  return new db::Matrix3d (db::Matrix3d::rotation (rot) * db::Matrix3d::shear (shear) * db::Matrix3d::mag (mx, my) * db::Matrix3d::mirror (m));
}

static db::Matrix3d *new_matrix3d_dsmrm (const db::DVector &d, double shear, double mx, double my, double rot, bool m)
{
  return new db::Matrix3d (db::Matrix3d::disp (d) * db::Matrix3d::rotation (rot) * db::Matrix3d::shear (shear) * db::Matrix3d::mag (mx, my) * db::Matrix3d::mirror (m));
}

static db::Matrix3d *new_matrix3d_pdsmrm (double tx, double ty, double z, const db::DVector &d, double shear, double mx, double my, double rot, bool m)
{
  return new db::Matrix3d (db::Matrix3d::disp (d) * db::Matrix3d::perspective (tx, ty, z) * db::Matrix3d::rotation (rot) * db::Matrix3d::shear (shear) * db::Matrix3d::mag (mx, my) * db::Matrix3d::mirror (m));
}

static db::Matrix3d *new_matrix3d_m4 (double m11, double m12, double m21, double m22)
{
  return new db::Matrix3d (m11, m12, m21, m22);
}

static db::Matrix3d *new_matrix3d_m6 (double m11, double m12, double m21, double m22, double dx, double dy)
{
  return new db::Matrix3d (m11, m12, m21, m22, dx, dy, 0.0, 0.0);
}

static db::Matrix3d *new_matrix3d_m9 (double m11, double m12, double m13, double m21, double m22, double m23, double m31, double m32, double m33)
{
  return new db::Matrix3d (m11, m12, m13, m21, m22, m23, m31, m32, m33);
}

static db::DCplxTrans to_cplx_trans3 (const db::Matrix3d *m)
{
  return db::DCplxTrans (*m);
}

static db::Matrix3d sum_m3 (const db::Matrix3d *m, const db::Matrix3d &d)
{
  return *m + d;
}

static db::Matrix3d prod_m3 (const db::Matrix3d *m, const db::Matrix3d &d)
{
  return *m * d;
}

static db::DPoint trans_p3 (const db::Matrix3d *m, const db::DPoint &p)
{
  return *m * p;
}

static double coeff_m3 (const db::Matrix3d *m, int i, int j)
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

gsi::Class<db::Matrix3d> decl_Matrix3d ("db", "Matrix3d",
  gsi::constructor ("new", &new_matrix3d,
    "@brief Create a new Matrix3d representing a unit transformation"
  ) +
  gsi::constructor ("new", &new_matrix3d_m,
    "@brief Create a new Matrix3d representing a magnification\n"
    "@args m\n"
    "@param m The magnification\n"
  ) +
  gsi::constructor ("new", &new_matrix3d_t,
    "@brief Create a new Matrix3d from the given complex transformation"
    "@args t\n"
    "@param t The transformation from which to create the matrix\n"
  ) +
  gsi::constructor ("newc", &new_matrix3d_mrm, 
    "@brief Create a new Matrix3d representing a isotropic magnification, rotation and mirroring\n"
    "@args mag, rotation, mirrx\n"
    "@param mag The magnification\n"
    "@param rotation The rotation angle (in degree)\n"
    "@param mirrx The mirror flag (at x axis)\n"
    "\n"
    "The order of execution of the operations is mirror, magnification and rotation.\n"
    "This constructor is called 'newc' to distinguish it from the constructors taking coefficients ('c' is for composite).\n"
  ) +
  gsi::constructor ("newc", &new_matrix3d_smrm, 
    "@brief Create a new Matrix3d representing a shear, anisotropic magnification, rotation and mirroring\n"
    "@args shear, mx, my, rotation, mirrx\n"
    "@param shear The shear angle\n"
    "@param mx The magnification in x direction\n"
    "@param mx The magnification in y direction\n"
    "@param rotation The rotation angle (in degree)\n"
    "@param mirrx The mirror flag (at x axis)\n"
    "\n"
    "The order of execution of the operations is mirror, magnification, rotation and shear.\n"
    "This constructor is called 'newc' to distinguish it from the constructor taking the four matrix coefficients ('c' is for composite).\n"
  ) +
  gsi::constructor ("newc", &new_matrix3d_dsmrm, 
    "@brief Create a new Matrix3d representing a displacement, shear, anisotropic magnification, rotation and mirroring\n"
    "@args u, shear, mx, my, rotation, mirrx\n"
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
  gsi::constructor ("newc", &new_matrix3d_pdsmrm, 
    "@brief Create a new Matrix3d representing a perspective distortion, displacement, shear, anisotropic magnification, rotation and mirroring\n"
    "@args tx, ty, z, u, shear, mx, my, rotation, mirrx\n"
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
  gsi::constructor ("new", &new_matrix3d_m4, 
    "@brief Create a new Matrix3d from the four coefficients of a Matrix2d\n"
    "@args m11, m12, m21, m22\n"
  ) +
  gsi::constructor ("new", &new_matrix3d_m6, 
    "@brief Create a new Matrix3d from the four coefficients of a Matrix2d plus a displacement\n"
    "@args m11, m12, m21, m22, dx, dy\n"
  ) +
  gsi::constructor ("new", &new_matrix3d_m9, 
    "@brief Create a new Matrix3d from the nine matrix coefficients\n"
    "@args m11, m12, m13, m21, m22, m23, m31, m32, m33\n"
  ) +
  gsi::method_ext ("m", &coeff_m3, 
    "@brief Gets the m coefficient with the given index.\n"
    "@args i,j\n"
    "@return The coefficient [i,j]\n"
  ) +
  gsi::method ("to_s", &db::Matrix3d::to_string, 
    "@brief Convert the matrix to a string.\n"
    "@return The string representing this matrix\n"
  ) +
  gsi::method ("inverted", &db::Matrix3d::inverted, 
    "@brief The inverse of this matrix.\n"
    "@return The inverse of this matrix\n"
  ) +
  gsi::method_ext ("trans", &trans_p3,
    "@brief Transforms a point with this matrix.\n"
    "@args p\n"
    "@param p The point to transform.\n"
    "@return The product if self and the point p\n"
  ) +
  gsi::method_ext ("*", &prod_m3, 
    "@brief Product of two matrices.\n"
    "@args m\n"
    "@param m The other matrix.\n"
    "@return The matrix product self*m\n"
  ) +
  gsi::method_ext ("*", &trans_p3, 
    "@brief Transform a point.\n"
    "@args p\n"
    "@param p The point to transform.\n"
    "@return The transformed point\n"
  ) +
  gsi::method_ext ("+", &sum_m3, 
    "@brief Sum of two matrices.\n"
    "@args m\n"
    "@param m The other matrix.\n"
    "@return The (element-wise) sum of self+m\n"
  ) +
  gsi::method_ext ("cplx_trans", &to_cplx_trans3, 
    "@brief Converts this matrix to a complex transformation (if possible).\n"
    "@return The complex transformation.\n"
    "This method is successful only if the matrix does not contain shear or perspective distortion components and the magnification must be isotropic.\n"
  ) +
  gsi::method ("mag_x", (double (db::Matrix3d::*) () const) &db::Matrix3d::mag_x,
    "@brief Returns the x magnification of the magnification component of this matrix.\n"
    "@return The magnification factor.\n"
  ) +
  gsi::method ("mag_y", (double (db::Matrix3d::*) () const) &db::Matrix3d::mag_y,
    "@brief Returns the y magnification of the magnification component of this matrix.\n"
    "@return The magnification factor.\n"
  ) +
  gsi::method ("angle", &db::Matrix3d::angle,
    "@brief Returns the rotation angle of the rotation component of this matrix.\n"
    "@return The angle in degree.\n"
    "See the description of this class for details about the basic transformations."
  ) +
  gsi::method ("shear_angle", &db::Matrix3d::shear_angle,
    "@brief Returns the magnitude of the shear component of this matrix.\n"
    "@return The shear angle in degree.\n"
    "The shear basic transformation will tilt the x axis towards the y axis and vice versa. The shear angle "
    "gives the tilt angle of the axes towards the other one. The possible range for this angle is -45 to 45 degree."
    "See the description of this class for details about the basic transformations."
  ) +
  gsi::method ("disp", (db::DVector (db::Matrix3d::*) () const) &db::Matrix3d::disp,
    "@brief Returns the displacement vector of this transformation.\n"
    "\n"
    "Starting with version 0.25 this method returns a vector type instead of a point.\n"
    "@return The displacement vector.\n"
  ) +
  gsi::method ("tx", &db::Matrix3d::perspective_tilt_x,
    "@brief Returns the perspective tilt angle tx.\n"
    "@args z\n"
    "@param z The observer distance at which the tilt angle is computed.\n"
    "@return The tilt angle tx.\n"
    "The tx and ty parameters represent the perspective distortion. They denote a tilt of the xy plane around the y axis (tx) or the x axis (ty) in degree. "
    "The same effect is achieved for different tilt angles at different observer distances. Hence, the observer distance must be specified at which the tilt angle is computed. "
    "If the magnitude of the tilt angle is not important, z can be set to 1.\n"
  ) +
  gsi::method ("ty", &db::Matrix3d::perspective_tilt_y,
    "@brief Returns the perspective tilt angle ty.\n"
    "@args z\n"
    "@param z The observer distance at which the tilt angle is computed.\n"
    "@return The tilt angle ty.\n"
    "The tx and ty parameters represent the perspective distortion. They denote a tilt of the xy plane around the y axis (tx) or the x axis (ty) in degree. "
    "The same effect is achieved for different tilt angles at different observer distances. Hence, the observer distance must be specified at which the tilt angle is computed. "
    "If the magnitude of the tilt angle is not important, z can be set to 1.\n"
  ) +
  gsi::method ("is_mirror?", &db::Matrix3d::is_mirror,
    "@brief Returns the mirror flag of this matrix.\n"
    "@return True if this matrix has a mirror component.\n"
    "See the description of this class for details about the basic transformations."
  ) +
  gsi::method_ext ("adjust", &adjust,
    "@brief Adjust a 3d matrix to match the given set of landmarks\n"
    "\n"
    "This function tries to adjust the matrix\n"
    "such, that either the matrix is changed as little as possible (if few landmarks are given) \n"
    "or that the \"after\" landmarks will match as close as possible to the \"before\" landmarks \n"
    "(if the problem is overdetermined).\n"
    "\n"
    "@args landmarks_before, landmarks_after, flags, fixed_point\n"
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
  "This object represents a 3x3 matrix. This matrix is used to represent geometrical transformations "
  "in the 2d space mainly. It can be decomposed into basic transformations: mirroring, rotation, shear, displacement and perspective distortion. "
  "In that case, the assumed execution order of the basic transformations is "
  "mirroring at the x axis, rotation, magnification, shear, displacement and perspective distortion."
  "\n\n"
  "This class was introduced in version 0.22.\n"
);

}

