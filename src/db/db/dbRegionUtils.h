
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#ifndef HDR_dbRegionUtils
#define HDR_dbRegionUtils

#include "dbCommon.h"
#include "dbRegion.h"
#include "dbCellVariants.h"

namespace db {

/**
 *  @brief A perimeter filter for use with Region::filter or Region::filtered
 *
 *  This filter has two parameters: pmin and pmax.
 *  It will filter all polygons for which the perimeter is >= pmin and < pmax.
 *  There is an "invert" flag which allows to select all polygons not
 *  matching the criterion.
 */

struct DB_PUBLIC RegionPerimeterFilter
  : public PolygonFilterBase
{
  typedef db::coord_traits<db::Coord>::perimeter_type perimeter_type;

  /**
   *  @brief Constructor
   *
   *  @param amin The min perimeter (only polygons above this value are filtered)
   *  @param amax The maximum perimeter (only polygons with a perimeter below this value are filtered)
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RegionPerimeterFilter (perimeter_type pmin, perimeter_type pmax, bool inverse)
    : m_pmin (pmin), m_pmax (pmax), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the polygon's perimeter matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const
  {
    perimeter_type p = 0;
    for (db::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end () && p < m_pmax; ++e) {
      p += (*e).length ();
    }

    if (! m_inverse) {
      return p >= m_pmin && p < m_pmax;
    } else {
      return ! (p >= m_pmin && p < m_pmax);
    }
  }

  /**
   *  @brief This filter is isotropic
   */
  virtual const TransformationReducer *vars () const
  {
    return &m_vars;
  }

private:
  perimeter_type m_pmin, m_pmax;
  bool m_inverse;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief An area filter for use with Region::filter or Region::filtered
 *
 *  This filter has two parameters: amin and amax.
 *  It will filter all polygons for which the area is >= amin and < amax.
 *  There is an "invert" flag which allows to select all polygons not
 *  matching the criterion.
 */

struct DB_PUBLIC RegionAreaFilter
  : public PolygonFilterBase
{
  typedef db::Polygon::area_type area_type;

  /**
   *  @brief Constructor
   *
   *  @param amin The min area (only polygons above this value are filtered)
   *  @param amax The maximum area (only polygons with an area below this value are filtered)
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RegionAreaFilter (area_type amin, area_type amax, bool inverse)
    : m_amin (amin), m_amax (amax), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const
  {
    area_type a = poly.area ();
    if (! m_inverse) {
      return a >= m_amin && a < m_amax;
    } else {
      return ! (a >= m_amin && a < m_amax);
    }
  }

  /**
   *  @brief This filter is isotropic
   */
  virtual const TransformationReducer *vars () const
  {
    return &m_vars;
  }

private:
  area_type m_amin, m_amax;
  bool m_inverse;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief A filter for rectilinear polygons
 *
 *  This filter will select all polygons which are rectilinear.
 */

struct DB_PUBLIC RectilinearFilter
  : public PolygonFilterBase
{
  /**
   *  @brief Constructor
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RectilinearFilter (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const
  {
    return poly.is_rectilinear () != m_inverse;
  }

  /**
   *  @brief This filter does not need variants
   */
  virtual const TransformationReducer *vars () const
  {
    return 0;
  }

private:
  bool m_inverse;
};

/**
 *  @brief A rectangle filter
 *
 *  This filter will select all polygons which are rectangles.
 */

struct DB_PUBLIC RectangleFilter
  : public PolygonFilterBase
{
  /**
   *  @brief Constructor
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RectangleFilter (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const
  {
    return poly.is_box () != m_inverse;
  }

  /**
   *  @brief This filter does not need variants
   */
  virtual const TransformationReducer *vars () const
  {
    return 0;
  }

private:
  bool m_inverse;
};

/**
 *  @brief A bounding box filter for use with Region::filter or Region::filtered
 *
 *  This filter has two parameters: vmin and vmax.
 *  It will filter all polygons for which the selected bounding box parameter is >= vmin and < vmax.
 *  There is an "invert" flag which allows to select all polygons not
 *  matching the criterion.
 *
 *  For bounding box parameters the following choices are available:
 *    - (BoxWidth) width
 *    - (BoxHeight) height
 *    - (BoxMaxDim) maximum of width and height
 *    - (BoxMinDim) minimum of width and height
 *    - (BoxAverageDim) average of width and height
 */

struct DB_PUBLIC RegionBBoxFilter
  : public PolygonFilterBase
{
  typedef db::Box::distance_type value_type;

  /**
   *  @brief The parameters available
   */
  enum parameter_type {
    BoxWidth,
    BoxHeight,
    BoxMaxDim,
    BoxMinDim,
    BoxAverageDim
  };

  /**
   *  @brief Constructor
   *
   *  @param vmin The min value (only polygons with bounding box parameters above this value are filtered)
   *  @param vmax The max value (only polygons with bounding box parameters below this value are filtered)
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RegionBBoxFilter (value_type vmin, value_type vmax, bool inverse, parameter_type parameter)
    : m_vmin (vmin), m_vmax (vmax), m_inverse (inverse), m_parameter (parameter)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const
  {
    value_type v = 0;
    db::Box box = poly.box ();
    if (m_parameter == BoxWidth) {
      v = box.width ();
    } else if (m_parameter == BoxHeight) {
      v = box.height ();
    } else if (m_parameter == BoxMinDim) {
      v = std::min (box.width (), box.height ());
    } else if (m_parameter == BoxMaxDim) {
      v = std::max (box.width (), box.height ());
    } else if (m_parameter == BoxAverageDim) {
      v = (box.width () + box.height ()) / 2;
    }
    if (! m_inverse) {
      return v >= m_vmin && v < m_vmax;
    } else {
      return ! (v >= m_vmin && v < m_vmax);
    }
  }

  /**
   *  @brief This filter is isotropic unless the parameter is width or height
   */
  virtual const TransformationReducer *vars () const
  {
    if (m_parameter != BoxWidth && m_parameter != BoxHeight) {
      return &m_isotropic_vars;
    } else {
      return &m_anisotropic_vars;
    }
  }

private:
  value_type m_vmin, m_vmax;
  bool m_inverse;
  parameter_type m_parameter;
  db::MagnificationReducer m_isotropic_vars;
  db::MagnificationAndOrientationReducer m_anisotropic_vars;
};

} // namespace db

#endif

