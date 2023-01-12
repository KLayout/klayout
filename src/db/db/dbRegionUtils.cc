
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


#include "dbRegionUtils.h"
#include "dbRegionCheckUtils.h"
#include "dbPolygonTools.h"
#include "dbEdgeBoolean.h"
#include "tlSelect.h"

namespace db
{

// -------------------------------------------------------------------------------------
//  RegionPerimeterFilter implementation

RegionPerimeterFilter::RegionPerimeterFilter (perimeter_type pmin, perimeter_type pmax, bool inverse)
  : m_pmin (pmin), m_pmax (pmax), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool RegionPerimeterFilter::check (perimeter_type p) const
{
  if (! m_inverse) {
    return p >= m_pmin && p < m_pmax;
  } else {
    return ! (p >= m_pmin && p < m_pmax);
  }
}

bool RegionPerimeterFilter::selected (const db::Polygon &poly) const
{
  return check (poly.perimeter ());
}

bool RegionPerimeterFilter::selected (const db::PolygonRef &poly) const
{
  return check (poly.perimeter ());
}

bool RegionPerimeterFilter::selected_set (const std::unordered_set<db::Polygon> &poly) const
{
  perimeter_type ps = 0;
  for (std::unordered_set<db::Polygon>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    ps += p->perimeter ();
  }
  return check (ps);
}

bool RegionPerimeterFilter::selected_set (const std::unordered_set<db::PolygonRef> &poly) const
{
  perimeter_type ps = 0;
  for (std::unordered_set<db::PolygonRef>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    ps += p->perimeter ();
  }
  return check (ps);
}

const TransformationReducer *RegionPerimeterFilter::vars () const
{
  return &m_vars;
}

// -------------------------------------------------------------------------------------
//  RegionAreaFilter implementation

RegionAreaFilter::RegionAreaFilter (area_type amin, area_type amax, bool inverse)
  : m_amin (amin), m_amax (amax), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool RegionAreaFilter::check (area_type a) const
{
  if (! m_inverse) {
    return a >= m_amin && a < m_amax;
  } else {
    return ! (a >= m_amin && a < m_amax);
  }
}

bool RegionAreaFilter::selected (const db::Polygon &poly) const
{
  return check (poly.area ());
}

bool RegionAreaFilter::selected (const db::PolygonRef &poly) const
{
  return check (poly.area ());
}

bool RegionAreaFilter::selected_set (const std::unordered_set<db::Polygon> &poly) const
{
  area_type as = 0;
  for (std::unordered_set<db::Polygon>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    as += p->area ();
  }
  return check (as);
}

bool RegionAreaFilter::selected_set (const std::unordered_set<db::PolygonRef> &poly) const
{
  area_type as = 0;
  for (std::unordered_set<db::PolygonRef>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    as += p->area ();
  }
  return check (as);
}

const TransformationReducer *
RegionAreaFilter::vars () const
{
  return &m_vars;
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RectilinearFilter::RectilinearFilter (bool inverse)
  : m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool
RectilinearFilter::selected (const db::Polygon &poly) const
{
  return poly.is_rectilinear () != m_inverse;
}

bool
RectilinearFilter::selected (const db::PolygonRef &poly) const
{
  return poly.is_rectilinear () != m_inverse;
}

const TransformationReducer *
RectilinearFilter::vars () const
{
  return 0;
}

// -------------------------------------------------------------------------------------
//  HoleCountFilter implementation

HoleCountFilter::HoleCountFilter (size_t min_count, size_t max_count, bool inverse)
  : m_min_count (min_count), m_max_count (max_count), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool
HoleCountFilter::selected (const db::Polygon &poly) const
{
  bool ok = poly.holes () < m_max_count && poly.holes () >= m_min_count;
  return ok != m_inverse;
}

bool
HoleCountFilter::selected (const db::PolygonRef &poly) const
{
  bool ok = poly.obj ().holes () < m_max_count && poly.obj ().holes () >= m_min_count;
  return ok != m_inverse;
}

const TransformationReducer *HoleCountFilter::vars () const
{
  return 0;
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RectangleFilter::RectangleFilter (bool is_square, bool inverse)
  : m_is_square (is_square), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool
RectangleFilter::selected (const db::Polygon &poly) const
{
  bool ok = poly.is_box ();
  if (ok && m_is_square) {
    db::Box box = poly.box ();
    ok = box.width () == box.height ();
  }
  return ok != m_inverse;
}

bool
RectangleFilter::selected (const db::PolygonRef &poly) const
{
  bool ok = poly.is_box ();
  if (ok && m_is_square) {
    db::Box box = poly.box ();
    ok = box.width () == box.height ();
  }
  return ok != m_inverse;
}

const TransformationReducer *RectangleFilter::vars () const
{
  return 0;
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RegionBBoxFilter::RegionBBoxFilter (value_type vmin, value_type vmax, bool inverse, parameter_type parameter)
  : m_vmin (vmin), m_vmax (vmax), m_inverse (inverse), m_parameter (parameter)
{
  //  .. nothing yet ..
}

bool
RegionBBoxFilter::check (const db::Box &box) const
{
  value_type v = 0;
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

bool
RegionBBoxFilter::selected (const db::Polygon &poly) const
{
  return check (poly.box ());
}

bool
RegionBBoxFilter::selected (const db::PolygonRef &poly) const
{
  return check (poly.box ());
}

const TransformationReducer *
RegionBBoxFilter::vars () const
{
  if (m_parameter != BoxWidth && m_parameter != BoxHeight) {
    return &m_isotropic_vars;
  } else {
    return &m_anisotropic_vars;
  }
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RegionRatioFilter::RegionRatioFilter (double vmin, bool min_included, double vmax, bool max_included, bool inverse, parameter_type parameter)
  : m_vmin (vmin), m_vmax (vmax), m_vmin_included (min_included), m_vmax_included (max_included), m_inverse (inverse), m_parameter (parameter)
{
  //  .. nothing yet ..
}

template <class P>
static double compute_ratio_parameter (const P &poly, RegionRatioFilter::parameter_type parameter)
{
  double v = 0.0;

  if (parameter == RegionRatioFilter::AreaRatio) {

    v = poly.area_ratio ();

  } else if (parameter == RegionRatioFilter::AspectRatio) {

    db::Box box = poly.box ();
    double f = std::max (box.height (), box.width ());
    double d = std::min (box.height (), box.width ());
    if (d < 1) {
      return false;
    }

    v = f / d;

  } else if (parameter == RegionRatioFilter::RelativeHeight) {

    db::Box box = poly.box ();
    double f = box.height ();
    double d = box.width ();
    if (d < 1) {
      return false;
    }

    v = f / d;

  }

  return v;
}

bool RegionRatioFilter::selected (const db::Polygon &poly) const
{
  double v = compute_ratio_parameter (poly, m_parameter);

  bool ok = (v - (m_vmin_included ? -db::epsilon : db::epsilon) > m_vmin  && v - (m_vmax_included ? db::epsilon : -db::epsilon) < m_vmax);
  return ok != m_inverse;
}

bool RegionRatioFilter::selected (const db::PolygonRef &poly) const
{
  double v = compute_ratio_parameter (poly, m_parameter);

  bool ok = (v - (m_vmin_included ? -db::epsilon : db::epsilon) > m_vmin  && v - (m_vmax_included ? db::epsilon : -db::epsilon) < m_vmax);
  return ok != m_inverse;
}

const TransformationReducer *RegionRatioFilter::vars () const
{
  if (m_parameter != RelativeHeight) {
    return &m_isotropic_vars;
  } else {
    return &m_anisotropic_vars;
  }
}

// -------------------------------------------------------------------------------------
//  SinglePolygonCheck implementation

SinglePolygonCheck::SinglePolygonCheck (db::edge_relation_type rel, db::Coord d, const RegionCheckOptions &options)
  : m_relation (rel), m_d (d), m_options (options)
{ }

void
SinglePolygonCheck::process (const db::Polygon &polygon, std::vector<db::EdgePair> &res) const
{
  std::unordered_set<db::EdgePair> result;

  EdgeRelationFilter check (m_relation, m_d, m_options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (m_options.whole_edges);
  check.set_ignore_angle (m_options.ignore_angle);
  check.set_min_projection (m_options.min_projection);
  check.set_max_projection (m_options.max_projection);

  edge2edge_check_negative_or_positive <std::unordered_set<db::EdgePair> > edge_check (check, result, m_options.negative, false /*=same polygons*/, false /*=same layers*/, m_options.shielded, true /*=symmetric*/);
  poly2poly_check<db::Polygon> poly_check (edge_check);

  do {
    poly_check.single (polygon, 0);
  } while (edge_check.prepare_next_pass ());

  res.insert (res.end (), result.begin (), result.end ());
}

// -------------------------------------------------------------------------------------------------------------
//  Strange polygon processor

namespace {

/**
 *  @brief A helper class to implement the strange polygon detector
 */
struct StrangePolygonInsideFunc
{
  inline bool operator() (int wc) const
  {
    return wc < 0 || wc > 1;
  }
};

}

StrangePolygonCheckProcessor::StrangePolygonCheckProcessor () { }

StrangePolygonCheckProcessor::~StrangePolygonCheckProcessor () { }

void
StrangePolygonCheckProcessor::process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
{
  EdgeProcessor ep;
  ep.insert (poly);

  StrangePolygonInsideFunc inside;
  db::GenericMerge<StrangePolygonInsideFunc> op (inside);
  db::PolygonContainer pc (res, false);
  db::PolygonGenerator pg (pc, false, false);
  ep.process (pg, op);
}

// -------------------------------------------------------------------------------------------------------------
//  Smoothing processor

SmoothingProcessor::SmoothingProcessor (db::Coord d, bool keep_hv) : m_d (d), m_keep_hv (keep_hv) { }

SmoothingProcessor::~SmoothingProcessor () { }

void
SmoothingProcessor::process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
{
  res.push_back (db::smooth (poly, m_d, m_keep_hv));
}

// -------------------------------------------------------------------------------------------------------------
//  Rounded corners processor

RoundedCornersProcessor::RoundedCornersProcessor (double rinner, double router, unsigned int n)
  : m_rinner (rinner), m_router (router), m_n (n)
{ }

RoundedCornersProcessor::~RoundedCornersProcessor ()
{ }

void
RoundedCornersProcessor::process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
{
  res.push_back (db::compute_rounded (poly, m_rinner, m_router, m_n));
}

// -------------------------------------------------------------------------------------------------------------
//  Holes decomposition processor

HolesExtractionProcessor::HolesExtractionProcessor ()
{
}

HolesExtractionProcessor::~HolesExtractionProcessor ()
{
}

void
HolesExtractionProcessor::process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
{
  for (size_t i = 0; i < poly.holes (); ++i) {
    res.push_back (db::Polygon ());
    res.back ().assign_hull (poly.begin_hole ((unsigned int) i), poly.end_hole ((unsigned int) i));
  }
}

// -------------------------------------------------------------------------------------------------------------
//  Hull decomposition processor

HullExtractionProcessor::HullExtractionProcessor ()
{
}

HullExtractionProcessor::~HullExtractionProcessor ()
{
}

void
HullExtractionProcessor::process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
{
  res.push_back (db::Polygon ());
  res.back ().assign_hull (poly.begin_hull (), poly.end_hull ());
}

}
