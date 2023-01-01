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

#include "dbUtils.h"
#include "dbVector.h"
#include "tlException.h"

namespace db
{

/*

Rational B-Splines (NURBS) vs. non-rational B-Splines:
  https://en.wikipedia.org/wiki/Non-uniform_rational_B-spline

De Boor algorithm for NURBS
  https://github.com/caadxyz/DeBoorAlgorithmNurbs

*/

static db::DPoint
b_spline_point (double x, const std::vector<std::pair<db::DPoint, double> > &control_points, int p, const std::vector<double> &t)
{
  int k = (int) (std::lower_bound (t.begin (), t.end (), x + 1e-6) - t.begin ());
  if (k <= p) {
    return control_points.front ().first;
  } else if (k > (int) control_points.size ()) {
    return control_points.back ().first;
  }
  --k;

  std::vector<db::DPoint> d;
  std::vector<double> dw;
  d.reserve(p + 1);
  for (int j = 0; j <= p; ++j) {
    double w = control_points[j + k - p].second;
    d.push_back (control_points[j + k - p].first * w);
    dw.push_back (w);
  }

  for (int r = 1; r <= p; ++r) {
    for (int j = p; j >= r; --j) {
      double alpha = (x - t[j + k - p]) / (t[j + 1 + k - r] - t[j + k - p]);
      d[j] = d[j] * alpha + (d[j - 1] - d[j - 1] * alpha);
      dw[j] = dw[j] * alpha + dw[j - 1] * (1.0 - alpha);
    }
  }

  return d[p] * (1.0 / dw[p]);
}

/**
 *  @brief Inserts new points into a sequence of points to refine the curve
 *
 *  The idea is bisection of the segments until the desired degree of accuracy has been reached.
 *
 *  @param control_points The control points
 *  @param curve_points The list of curve points which is going to be extended
 *  @param current_curve_point The iterator pointing to the current curve point
 *  @param t_start The t (curve parameter) value of the current curve point
 *  @param dt The current t interval
 *  @param degree The degree of the spline
 *  @param knots The knots
 *  @param sin_da The relative accuracy value implied by the circle resolution
 *  @param accu The desired absolute accuracy value
 *
 *  New points are going to be inserted after current_curve_point and current_curve_point + 1 to achieve the
 *  required curvature.
 */
static void
spline_interpolate (std::list<db::DPoint> &curve_points,
                    typename std::list<db::DPoint>::iterator current_curve_point,
                    double t_start,
                    double dt,
                    const std::vector<std::pair<db::DPoint, double> > &control_points,
                    int degree,
                    const std::vector<double> &knots,
                    double sin_da,
                    double accu)
{
  std::list<db::DPoint>::iterator pm = current_curve_point;
  ++pm;
  std::list<db::DPoint>::iterator pe = pm;
  ++pe;

  db::DPoint s1 = b_spline_point (t_start + 0.5 * dt, control_points, degree, knots);
  db::DPoint s2 = b_spline_point (t_start + 1.5 * dt, control_points, degree, knots);

  db::DVector p1 (s1, *current_curve_point);
  db::DVector p2 (*pm, s1);
  double pl1 = p1.length(), pl2 = p2.length();

  if (curve_points.size () < control_points.size () - degree - 1) {

    curve_points.insert (pm, s1);
    spline_interpolate (curve_points, current_curve_point, t_start, dt * 0.5, control_points, degree, knots, sin_da, accu);

    curve_points.insert (pe, s2);
    spline_interpolate (curve_points, pm, t_start + dt, dt * 0.5, control_points, degree, knots, sin_da, accu);

  } else {

    db::DVector q1 (s2, *pm);
    db::DVector q2 (*pe, s2);
    double ql1 = q1.length(), ql2 = q2.length();

    db::DVector p (*pm, *current_curve_point);
    db::DVector q (*pe, *pm);
    double pl = p.length (), ql = q.length ();

    if (fabs (db::vprod (p, q)) > pl * ql * sin_da ||
        fabs (db::vprod (p1, p2)) > pl1 * pl2 * sin_da ||
        fabs (db::vprod (q1, q2)) > ql1 * ql2 * sin_da) {

      //  angle between the segments is bigger than 2PI/n -> circle resolution is
      //  too small. Or: the angle between the new segments that we would introduce
      //  is also bigger (hence, the original segments may have a small angle but the
      //  curve "wiggles" when we increase the resolution)

      if (fabs (db::vprod (p1, p)) > pl * accu) {

        //  In addition, the estimated accuracy is not good enough on the first segment: bisect this
        //  segment.

        curve_points.insert (pm, s1);
        spline_interpolate (curve_points, current_curve_point, t_start, dt * 0.5, control_points, degree, knots, sin_da, accu);

      }

      if (fabs (db::vprod (q1, q)) > ql * accu) {

        //  In addition, the estimated accuracy is not good enough on the first segment: bisect this
        //  segment.

        curve_points.insert (pe, s2);
        spline_interpolate (curve_points, pm, t_start + dt, dt * 0.5, control_points, degree, knots, sin_da, accu);

      }

    }

  }
}

static
std::list<db::DPoint>
do_spline_interpolation (const std::vector<std::pair<db::DPoint, double> > &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
{
  //  TODO: this is quite inefficient
  if (int (knots.size()) != int (control_points.size() + degree + 1)) {
    throw tl::Exception (tl::to_string (tr ("Spline interpolation failed: mismatch between number of knots and points (#knots must be #points+degree+1)")));
  }

  if (int(knots.size ()) <= degree || control_points.empty () || degree <= 1) {
    return std::list<db::DPoint> ();
  }

  double t0 = knots [degree];
  double tn = knots [knots.size () - degree - 1];

  std::list<db::DPoint> new_points;
  new_points.push_back (control_points.front ().first);

  double dt = 0.5 * (tn - t0);

  for (double t = t0 + dt; t < tn + 1e-6; t += dt) {
    db::DPoint s = b_spline_point (t, control_points, degree, knots);
    new_points.push_back (s);
  }

  spline_interpolate (new_points, new_points.begin (), t0, dt, control_points, degree, knots, relative_accuracy, absolute_accuracy);

  return new_points;
}

template <class P>
std::list<P>
spline_interpolation (const std::vector<std::pair<P, double> > &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
{
  std::vector<std::pair<db::DPoint, double> > cp;
  cp.reserve (control_points.size ());

  for (size_t i = 0; i < control_points.size (); ++i) {
    cp.push_back (std::make_pair (db::DPoint (control_points [i].first), control_points [i].second));
  }

  std::list<db::DPoint> result = do_spline_interpolation (cp, degree, knots, relative_accuracy, absolute_accuracy);

  std::list<P> ret;
  for (std::list<db::DPoint>::const_iterator i = result.begin (); i != result.end (); ++i) {
    ret.push_back (P (*i));
  }
  return ret;
}

template <>
DB_PUBLIC std::list<db::DPoint>
spline_interpolation (const std::vector<std::pair<db::DPoint, double> > &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
{
  return do_spline_interpolation (control_points, degree, knots, relative_accuracy, absolute_accuracy);
}

template <class P>
std::list<P>
spline_interpolation (const std::vector<P> &control_points, const std::vector<double> &weights, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
{
  std::vector<std::pair<P, double> > cp;
  cp.reserve (control_points.size ());

  for (size_t i = 0; i < control_points.size (); ++i) {
    if (i >= weights.size ()) {
      cp.push_back (std::make_pair (control_points [i], 1.0));
    } else {
      cp.push_back (std::make_pair (control_points [i], weights [i]));
    }
  }

  return spline_interpolation (cp, degree, knots, relative_accuracy, absolute_accuracy);
}

template <class P>
std::list<P>
spline_interpolation (const std::vector<P> &control_points, int degree, const std::vector<double> &knots, double relative_accuracy, double absolute_accuracy)
{
  std::vector<std::pair<P, double> > cp;
  cp.reserve (control_points.size ());

  for (size_t i = 0; i < control_points.size (); ++i) {
    cp.push_back (std::make_pair (control_points [i], 1.0));
  }

  return spline_interpolation (cp, degree, knots, relative_accuracy, absolute_accuracy);
}

template DB_PUBLIC std::list<db::Point> spline_interpolation (const std::vector<db::Point> &, int, const std::vector<double> &, double, double);
template DB_PUBLIC std::list<db::Point> spline_interpolation (const std::vector<db::Point> &, const std::vector<double> &, int, const std::vector<double> &, double, double);
template DB_PUBLIC std::list<db::Point> spline_interpolation (const std::vector<std::pair<db::Point, double> > &, int, const std::vector<double> &, double, double);

template DB_PUBLIC std::list<db::DPoint> spline_interpolation (const std::vector<db::DPoint> &, int, const std::vector<double> &, double, double);
template DB_PUBLIC std::list<db::DPoint> spline_interpolation (const std::vector<db::DPoint> &, const std::vector<double> &, int, const std::vector<double> &, double, double);

}
