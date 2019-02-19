
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

#ifndef HDR_dbEdgesUtils
#define HDR_dbEdgesUtils

#include "dbCommon.h"
#include "dbEdges.h"

namespace db {

/**
 *  @brief An edge length filter for use with Edges::filter or Edges::filtered
 *
 *  This filter has two parameters: lmin and lmax.
 *  It will filter all edges for which the length is >= lmin and < lmax.
 *  There is an "invert" flag which allows to select all edges not
 *  matching the criterion.
 */

struct DB_PUBLIC EdgeLengthFilter
  : public EdgeFilterBase
{
  typedef db::Edge::distance_type length_type;

  /**
   *  @brief Constructor
   *
   *  @param lmin The minimum length
   *  @param lmax The maximum length
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  EdgeLengthFilter (length_type lmin, length_type lmax, bool inverse)
    : m_lmin (lmin), m_lmax (lmax), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the edge length matches the criterion
   */
  virtual bool selected (const db::Edge &edge) const
  {
    length_type l = edge.length ();
    if (! m_inverse) {
      return l >= m_lmin && l < m_lmax;
    } else {
      return ! (l >= m_lmin && l < m_lmax);
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
  length_type m_lmin, m_lmax;
  bool m_inverse;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief An edge orientation filter for use with Edges::filter or Edges::filtered
 *
 *  This filter has two parameters: amin and amax.
 *  It will filter all edges for which the orientation angle is >= amin and < amax.
 *  The orientation angle is measured in degree against the x axis in the mathematical sense.
 *  There is an "invert" flag which allows to select all edges not
 *  matching the criterion.
 */

struct DB_PUBLIC EdgeOrientationFilter
  : public EdgeFilterBase
{
  /**
   *  @brief Constructor
   *
   *  @param amin The minimum angle (measured against the x axis)
   *  @param amax The maximum angle (measured against the x axis)
   *  @param inverse If set to true, only edges not matching this criterion will be filtered
   *
   *  This filter will filter out all edges whose angle against x axis
   *  is larger or equal to amin and less than amax.
   */
  EdgeOrientationFilter (double amin, double amax, bool inverse)
    : m_inverse (inverse), m_exact (false)
  {
    m_emin = db::DVector (cos (amin * M_PI / 180.0), sin (amin * M_PI / 180.0));
    m_emax = db::DVector (cos (amax * M_PI / 180.0), sin (amax * M_PI / 180.0));
  }

  /**
   *  @brief Constructor
   *
   *  @param a The angle (measured against the x axis)
   *  @param inverse If set to true, only edges not matching this criterion will be filtered
   *
   *  This filter will filter out all edges whose angle against x axis
   *  is equal to a.
   */
  EdgeOrientationFilter (double a, bool inverse)
    : m_inverse (inverse), m_exact (true)
  {
    m_emin = db::DVector (cos (a * M_PI / 180.0), sin (a * M_PI / 180.0));
  }

  /**
   *  @brief Returns true if the edge orientation matches the criterion
   */
  virtual bool selected (const db::Edge &edge) const
  {
    int smin = db::vprod_sign (m_emin, db::DVector (edge.d ()));
    if (m_exact) {
      if (! m_inverse) {
        return smin == 0;
      } else {
        return smin != 0;
      }
    } else {
      int smax = db::vprod_sign (m_emax, db::DVector (edge.d ()));
      if (! m_inverse) {
        return (smin >= 0 && smax < 0) || (smax > 0 && smin <= 0);
      } else {
        return ! ((smin >= 0 && smax < 0) || (smax > 0 && smin <= 0));
      }
    }
  }

  /**
   *  @brief This filter is not isotropic
   */
  virtual const TransformationReducer *vars () const
  {
    return &m_vars;
  }

private:
  db::DVector m_emin, m_emax;
  bool m_inverse;
  bool m_exact;
  db::MagnificationAndOrientationReducer m_vars;
};

} // namespace db

#endif
