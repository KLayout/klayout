
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
#include "dbUtils.h"

namespace db
{

/**
 *  @brief A dummy class providing the namespace.
 */
struct UtilsDummy
{
  static std::list<db::DPoint> spi1 (const std::vector<db::DPoint> &control_points, const std::vector<double> &weight, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
  {
    return db::spline_interpolation (control_points, weight, degree, knots, relative_accuracy, absolute_accuracy);
  }

  static std::list<db::Point> spi2 (const std::vector<db::Point> &control_points, const std::vector<double> &weight, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
  {
    return db::spline_interpolation (control_points, weight, degree, knots, relative_accuracy, absolute_accuracy);
  }

  static std::list<db::DPoint> spi3 (const std::vector<db::DPoint> &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
  {
    return db::spline_interpolation (control_points, degree, knots, relative_accuracy, absolute_accuracy);
  }

  static std::list<db::Point> spi4 (const std::vector<db::Point> &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
  {
    return db::spline_interpolation (control_points, degree, knots, relative_accuracy, absolute_accuracy);
  }
};

}

namespace gsi
{

  Class<db::UtilsDummy> decl_dbUtils ("db", "Utils",
    gsi::method ("spline_interpolation", &db::UtilsDummy::spi1, gsi::arg ("control_points"), gsi::arg ("weights"), gsi::arg ("degree"), gsi::arg ("knots"), gsi::arg ("relative_accuracy"), gsi::arg ("absolute_accuracy"),
     "@brief This function computes the Spline curve for a given set of control points (point, weight), degree and knots.\n"
     "\n"
     "The knot vector needs to be padded and its size must fulfill the condition:\n"
     "\n"
     "@code\n"
     "  knots.size == control_points.size + degree + 1\n"
     "@/code\n"
     "\n"
     "The accuracy parameters allow tuning the resolution of the curve to target a specific approximation quality.\n"
     "\"relative_accuracy\" gives the accuracy relative to the local curvature radius, \"absolute\" accuracy gives the\n"
     "absolute accuracy. \"accuracy\" is the allowed deviation of polygon approximation from the ideal curve.\n"
     "The computed curve should meet at least one of the accuracy criteria. Setting both limits to a very small\n"
     "value will result in long run times and a large number of points returned.\n"
     "\n"
     "This function supports both rational splines (NURBS) and non-rational splines. The latter use weights of\n"
     "1.0 for each point.\n"
     "\n"
     "The return value is a list of points forming a path which approximates the spline curve.\n"
    ) +
    gsi::method ("spline_interpolation", &db::UtilsDummy::spi2, gsi::arg ("control_points"), gsi::arg ("weights"), gsi::arg ("degree"), gsi::arg ("knots"), gsi::arg ("relative_accuracy"), gsi::arg ("absolute_accuracy"),
     "@brief This function computes the Spline curve for a given set of control points (point, weight), degree and knots.\n"
     "\n"
     "This is the version for integer-coordinate points."
    ) +
    gsi::method ("spline_interpolation", &db::UtilsDummy::spi3, gsi::arg ("control_points"), gsi::arg ("degree"), gsi::arg ("knots"), gsi::arg ("relative_accuracy"), gsi::arg ("absolute_accuracy"),
     "@brief This function computes the Spline curve for a given set of control points (point, weight), degree and knots.\n"
     "\n"
     "This is the version for non-rational splines. It lacks the weight vector."
    ) +
    gsi::method ("spline_interpolation", &db::UtilsDummy::spi4, gsi::arg ("control_points"), gsi::arg ("degree"), gsi::arg ("knots"), gsi::arg ("relative_accuracy"), gsi::arg ("absolute_accuracy"),
     "@brief This function computes the Spline curve for a given set of control points (point, weight), degree and knots.\n"
     "\n"
     "This is the version for integer-coordinate points for non-rational splines."
    ),
    "@brief This namespace provides a collection of utility functions\n"
    "\n"
    "This class has been introduced in version 0.27."
  );

}
