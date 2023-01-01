
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


#ifndef HDR_dbUtils
#define HDR_dbUtils

#include "dbCommon.h"
#include "dbPoint.h"

#include <list>
#include <vector>

namespace db
{

/**
 *  @brief Provides a Spline curve with adjustable accuracy
 *
 *  This function computes the Spline curve for a given set of control points (point, weight), degree and knots.
 *
 *  The knot vector needs to be padded and it's size must fulfill the condition:
 *
 *  @code
 *  knots.size == control_points.size + degree + 1
 *  @/code
 *
 *  The accuracy parameters allow tuning the resolution of the curve to target a specific approximation quality.
 *  "relative_accuracy" gives the accuracy relative to the local curvature radius, "absolute" accuracy gives the
 *  absolute accuracy. "accuracy" is the allowed deviation of polygon approximation from the ideal curve.
 *  The computed curve should meet at least one of the accuracy criteria. Setting both limits to a very small
 *  value will result in long run times and a large number of points returned.
 *
 *  This function supports both rational splines (NURBS) and non-rational splines. The latter use weights of
 *  1.0 for each point.
 *
 *  The return value is a list of points forming a path which approximates the spline curve.
 */
template <class P>
DB_PUBLIC std::list<P>
spline_interpolation (const std::vector<std::pair<P, double> > &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy);

/**
 *  @brief A convenience version of the previous function
 *
 *  This version takes separate vectors for point and weights for the control points.
 */
template <class P>
DB_PUBLIC std::list<P>
spline_interpolation (const std::vector<P> &control_points, const std::vector<double> &weights, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy);

/**
 *  @brief A convenience version of the previous function
 *
 *  This version provides non-rational splines and does not take a weight vector.
 */
template <class P>
DB_PUBLIC std::list<P>
spline_interpolation (const std::vector<P> &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy);

}

#endif


