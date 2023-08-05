
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

#if defined(HAVE_QT)

#ifndef HDR_edtPropertiesPageUtils
#define HDR_edtPropertiesPageUtils

#include "dbPath.h"
#include "dbPolygon.h"
#include "dbBox.h"
#include "dbText.h"
#include "dbShape.h"
#include "dbCell.h"

#include <vector>

class QLineEdit;

namespace edt
{

// -------------------------------------------------------------------------
//  ChangeApplicator definition and implementation

/**
 *  @brief A change applicator strategy pattern implementation
 *  The change applicator is an object describing individual changes
 *  applied to shapes.
 */
class ChangeApplicator
{
public:
  ChangeApplicator () { }
  virtual ~ChangeApplicator () { }

  virtual bool supports_relative_mode () const
  {
    return false;
  }

  virtual db::Shape do_apply (db::Shapes & /*shapes*/, const db::Shape & /*shape*/, double /*dbu*/, bool /*relative*/) const 
  { 
    return db::Shape (); 
  }

  virtual db::Instance do_apply_inst (db::Cell & /*cell*/, const db::Instance & /*instance*/, double /*dbu*/, bool /*relative*/) const 
  { 
    return db::Instance ();
  }

private:
  ChangeApplicator (const ChangeApplicator &);
  ChangeApplicator &operator= (const ChangeApplicator &);
};

/**
 *  @brief A combined applicator 
 *  This class combines the actions of multiple applicators into a single one.
 *  The combined applicator takes ownership over the given individual applicators.
 */
class CombinedChangeApplicator
  : public ChangeApplicator
{
public:
  CombinedChangeApplicator ();
  CombinedChangeApplicator (ChangeApplicator *a1);
  CombinedChangeApplicator (ChangeApplicator *a1, ChangeApplicator *a2);
  ~CombinedChangeApplicator ();
      
  void add (ChangeApplicator *a);

  bool supports_relative_mode () const;
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;
  db::Instance do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const;

private:
  std::vector<ChangeApplicator *> m_appl;
};

/**
 *  @brief A property ID change applicator
 */
class ChangePropertiesApplicator
  : public ChangeApplicator
{
public:
  ChangePropertiesApplicator (db::properties_id_type prop_id);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;
  db::Instance do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const;

private:
  db::properties_id_type m_prop_id;
};

/**
 *  @brief A box change applicator
 */
class BoxDimensionsChangeApplicator
  : public ChangeApplicator
{
public:
  BoxDimensionsChangeApplicator (db::Coord dl, db::Coord db, db::Coord dr, db::Coord dt, db::Coord l, db::Coord b, db::Coord r, db::Coord t);

  bool supports_relative_mode () const { return true; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Coord m_dl, m_db, m_dr, m_dt;
  db::Coord m_l, m_b, m_r, m_t;
};

/**
 *  @brief A point change applicator
 */
class PointDimensionsChangeApplicator
  : public ChangeApplicator
{
public:
  PointDimensionsChangeApplicator (const db::Point &point, const db::Point &org_point);

  bool supports_relative_mode () const { return true; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Point m_point, m_org_point;
};

/**
 *  @brief A polygon change applicator
 */
class PolygonChangeApplicator
  : public ChangeApplicator
{
public:
  PolygonChangeApplicator (const db::Polygon &poly, const db::Polygon &org_poly);

  bool supports_relative_mode () const { return true; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Polygon m_poly, m_org_poly;
};

/**
 *  @brief An applicator changing the orientation of a text
 */
class TextOrientationChangeApplicator
  : public ChangeApplicator
{
public:
  TextOrientationChangeApplicator (const db::FTrans &trans);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::FTrans m_trans;
};

/**
 *  @brief An applicator changing the position of a text
 */
class TextPositionChangeApplicator
  : public ChangeApplicator
{
public:
  TextPositionChangeApplicator (const db::Vector &disp, const db::Vector &org_disp);

  bool supports_relative_mode () const { return true; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Vector m_disp, m_org_disp;
};

/**
 *  @brief An applicator changing the horizontal alignment of a text
 */
class TextHAlignChangeApplicator
  : public ChangeApplicator
{
public:
  TextHAlignChangeApplicator (db::HAlign halign);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::HAlign m_halign;
};

/**
 *  @brief An applicator changing the vertical alignment of a text
 */
class TextVAlignChangeApplicator
  : public ChangeApplicator
{
public:
  TextVAlignChangeApplicator (db::VAlign valign);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::VAlign m_valign;
};

/**
 *  @brief An applicator changing the size alignment of a text
 */
class TextSizeChangeApplicator
  : public ChangeApplicator
{
public:
  TextSizeChangeApplicator (db::Coord size);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Coord m_size;
};

/**
 *  @brief An applicator changing the string of a text
 */
class TextStringChangeApplicator
  : public ChangeApplicator
{
public:
  TextStringChangeApplicator (const std::string &string);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  std::string m_string;
};

/**
 *  @brief An applicator changing the points of a path
 */
class PathPointsChangeApplicator
  : public ChangeApplicator
{
public:
  PathPointsChangeApplicator (const std::vector<db::Point> &points, const std::vector<db::Point> &org_points);

  bool supports_relative_mode () const { return true; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  std::vector<db::Point> m_points, m_org_points;
};

/**
 *  @brief An applicator changing the width of a path
 */
class PathWidthChangeApplicator
  : public ChangeApplicator
{
public:
  PathWidthChangeApplicator (db::Coord w, db::Coord org_w);

  bool supports_relative_mode () const { return true; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Coord m_width, m_org_width;
};

/**
 *  @brief An applicator changing the start extensions of a path
 */
class PathStartExtensionChangeApplicator
  : public ChangeApplicator
{
public:
  PathStartExtensionChangeApplicator (db::Coord e);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Coord m_ext;
};

/**
 *  @brief An applicator changing the end extensions of a path
 */
class PathEndExtensionChangeApplicator
  : public ChangeApplicator
{
public:
  PathEndExtensionChangeApplicator (db::Coord e);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  db::Coord m_ext;
};

/**
 *  @brief An applicator changing the round end flag of a path
 */
class PathRoundEndChangeApplicator
  : public ChangeApplicator
{
public:
  PathRoundEndChangeApplicator (bool r);

  bool supports_relative_mode () const { return false; }
  db::Shape do_apply (db::Shapes &shapes, const db::Shape &shape, double dbu, bool relative) const;

private:
  bool m_round;
};

/**
 *  @brief An applicator changing the target cell of an instance
 */
class ChangeTargetCellApplicator
  : public ChangeApplicator
{
public:
  ChangeTargetCellApplicator (db::cell_index_type cell_index);

  bool supports_relative_mode () const { return false; }
  db::Instance do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const;

private:
  db::cell_index_type m_cell_index;
};

/**
 *  @brief An applicator changing the target pcell of an instance
 */
class ChangeTargetPCellApplicator
  : public ChangeApplicator
{
public:
  ChangeTargetPCellApplicator (db::pcell_id_type pcell_id, bool apply_new_id, db::Library *new_lib, bool apply_new_lib, const std::map<std::string, tl::Variant> &modified_parameters);

  bool supports_relative_mode () const { return false; }
  db::Instance do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const;

private:
  db::pcell_id_type m_pcell_id;
  bool m_apply_new_id;
  db::Library *mp_new_lib;
  bool m_apply_new_lib;
  std::map<std::string, tl::Variant> m_modified_parameters;
};

/**
 *  @brief An applicator changing the transformation properties of an instance
 */
class ChangeInstanceTransApplicator
  : public ChangeApplicator
{
public:
  ChangeInstanceTransApplicator (double a, double org_a, bool mirror, bool org_mirror, double m, double org_m, const db::DVector &disp, const db::DVector &org_disp);

  bool supports_relative_mode () const { return true; }
  db::Instance do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const;

private:
  double m_angle, m_org_angle;
  bool m_mirror, m_org_mirror;
  double m_mag, m_org_mag;
  db::DVector m_disp, m_org_disp;
};

/**
 *  @brief An applicator changing the array properties of an instance
 */
class ChangeInstanceArrayApplicator
  : public ChangeApplicator
{
public:
  ChangeInstanceArrayApplicator (const db::DVector &a, bool set_a, const db::DVector &b, bool set_b, unsigned long na, bool set_na, unsigned long nb, bool set_nb);

  bool supports_relative_mode () const { return false; }
  db::Instance do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const;

private:
  db::DVector m_a;
  bool m_set_a;
  db::DVector m_b;
  bool m_set_b;
  unsigned long m_na;
  bool m_set_na;
  unsigned long m_nb;
  bool m_set_nb;
};

/**
 *  @brief An applicator removing the array properties of an instance
 */
class InstanceRemoveArrayApplicator
  : public ChangeApplicator
{
public:
  InstanceRemoveArrayApplicator ();

  bool supports_relative_mode () const { return false; }
  db::Instance do_apply_inst (db::Cell &cell, const db::Instance &instance, double dbu, bool relative) const;
};

// -------------------------------------------------------------------------
//  helper functions to convert coordinates to and from a string

/**
 *  @brief Converts a DBU coordinate to a string
 *
 *  @param dc The coordinate in DBU units
 *  @param dbu The database unit
 *  @param du A flag indicating whether the value shall be given in database units (du = true) or micron (du = false)
 */
std::string coord_to_string (double dc, double dbu, bool du);

/**
 *  @brief Converts a DBU point to a string
 *
 *  @param dp The point in DBU units
 *  @param dbu The database unit
 *  @param du A flag indicating whether the value shall be given in database units (du = true) or micron (du = false)
 */
std::string coords_to_string (const db::DPoint &dp, double dbu, bool du, const char *sep = "\t");

/**
 *  @brief Converts a micron or DBU value to a micron value
 *
 *  @param d The value to convert
 *  @param dbu The database unit
 *  @param du A flag indicating whether the value is given in database units (du = true) or micron (du = false)
 *  @param t A transformation (in DBU space) to apply to the value (which has to be a dimension)
 *  @return The micron-units dimension
 *
 *  The transformation is intended to be a global-to-local transformation so the output value is
 *  a dimension of a shape in local-cell micron units.
 */
db::DCoord dcoord_from_dcoord (double d, double dbu, bool du, const db::CplxTrans &t);

/**
 *  @brief Converts a micron or DBU point to a micron point
 *
 *  @param dp The point to convert
 *  @param dbu The database unit
 *  @param du A flag indicating whether the input point is given in database units (du = true) or micron (du = false)
 *  @param t A transformation (in DBU space) to apply to the point
 *  @return The micron-unit point
 *
 *  The transformation is intended to be a global-to-local transformation so the output value is
 *  a point in local-cell micron units.
 */
db::DPoint dpoint_from_dpoint (const db::DPoint &dp, double dbu, bool du, const db::DCplxTrans &t);

/**
 *  @brief Converts a micron or DBU vector to a micron point
 *
 *  @param dp The point to convert
 *  @param dbu The database unit
 *  @param du A flag indicating whether the input point is given in database units (du = true) or micron (du = false)
 *  @param t A transformation (in DBU space) to apply to the point
 *  @return The micron-unit point
 *
 *  The transformation is intended to be a global-to-local transformation so the output value is
 *  a point in local-cell micron units.
 */
db::DVector dvector_from_dvector (const db::DVector &dp, double dbu, bool du, const db::DCplxTrans &t);

/**
 *  @brief Gets a dimension value from a string
 *
 *  See dcoord_from_dcoord for a description of the arguments.
 */
db::DCoord dcoord_from_string (const char *txt, double dbu, bool du, const db::CplxTrans &t);

/**
 *  @brief Converts a micron or DBU value to a DBU value
 *
 *  @param d The value to convert
 *  @param dbu The database unit
 *  @param du A flag indicating whether the value is given in database units (du = true) or micron (du = false)
 *  @param t A transformation (in DBU space) to apply to the value (which has to be a dimension)
 *  @return The DBU units dimension
 *
 *  The transformation is intended to be a global-to-local transformation so the output value is
 *  a dimension of a shape in local-cell DBU units.
 */
db::Coord coord_from_dcoord (double d, double dbu, bool du, const db::CplxTrans &t);

/**
 *  @brief Converts a micron or DBU point to a DBU point
 *
 *  @param dp The point to convert
 *  @param dbu The database unit
 *  @param du A flag indicating whether the input point is given in database units (du = true) or micron (du = false)
 *  @param t A transformation (in DBU space) to apply to the point
 *  @return The DBU unit point
 *
 *  The transformation is intended to be a global-to-local transformation so the output value is
 *  a point in local-cell DBU units.
 */
db::Point point_from_dpoint (const db::DPoint &dp, double dbu, bool du, const db::VCplxTrans &t);

/**
 *  @brief Gets a dimension value from a string
 *
 *  See coord_from_dcoord for a description of the arguments.
 */
db::Coord coord_from_string (const char *txt, double dbu, bool du, const db::VCplxTrans &t);

}

#endif

#endif
