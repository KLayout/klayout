
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


#ifndef HDR_dbRegionUtils
#define HDR_dbRegionUtils

#include "dbCommon.h"
#include "dbRegion.h"
#include "dbCellVariants.h"
#include "dbBoxScanner.h"

namespace db {

/**
 *  @brief A perimeter filter for use with Region::filter or Region::filtered
 *
 *  This filter has two parameters: pmin and pmax.
 *  It will filter all polygons for which the perimeter is >= pmin and < pmax.
 *  There is an "invert" flag which allows selecting all polygons not
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
  RegionPerimeterFilter (perimeter_type pmin, perimeter_type pmax, bool inverse);

  /**
   *  @brief Returns true if the polygon's perimeter matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const;

  /**
   *  @brief Returns true if the polygon's perimeter matches the criterion
   */
  virtual bool selected (const db::PolygonRef &poly) const;

  /**
   *  @brief Returns true if the polygon's perimeter sum matches the criterion
   */
  virtual bool selected_set (const std::unordered_set<db::PolygonRef> &polygons) const;

  /**
   *  @brief Returns true if the polygon's perimeter sum matches the criterion
   */
  virtual bool selected_set (const std::unordered_set<db::Polygon> &polygons) const;

  /**
   *  @brief This filter is isotropic
   */
  virtual const TransformationReducer *vars () const;

  /**
   *  @brief This filter prefers producing variants
   */
  virtual bool wants_variants () const { return true; }

  /**
   *  @brief This filter wants merged input
   */
  virtual bool requires_raw_input () const { return false; }

private:
  perimeter_type m_pmin, m_pmax;
  bool m_inverse;
  db::MagnificationReducer m_vars;

  bool check (perimeter_type p) const;
};

/**
 *  @brief An area filter for use with Region::filter or Region::filtered
 *
 *  This filter has two parameters: amin and amax.
 *  It will filter all polygons for which the area is >= amin and < amax.
 *  There is an "invert" flag which allows selecting all polygons not
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
  RegionAreaFilter (area_type amin, area_type amax, bool inverse);

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const;

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::PolygonRef &poly) const;

  /**
   *  @brief Returns true if the polygon's area sum matches the criterion
   */
  virtual bool selected_set (const std::unordered_set<db::PolygonRef> &polygons) const;

  /**
   *  @brief Returns true if the polygon's area sum matches the criterion
   */
  virtual bool selected_set (const std::unordered_set<db::Polygon> &polygons) const;

  /**
   *  @brief This filter is isotropic
   */
  virtual const TransformationReducer *vars () const;

  /**
   *  @brief This filter prefers producing variants
   */
  virtual bool wants_variants () const { return true; }

  /**
   *  @brief This filter wants merged input
   */
  virtual bool requires_raw_input () const { return false; }

private:
  area_type m_amin, m_amax;
  bool m_inverse;
  db::MagnificationReducer m_vars;

  bool check (area_type a) const;
};

/**
 *  @brief A filter implementation which implements the set filters through "all must match"
 */

struct DB_PUBLIC AllMustMatchFilter
  : public PolygonFilterBase
{
  /**
   *  @brief Constructor
   */
  AllMustMatchFilter () { }

  virtual bool selected_set (const std::unordered_set<db::PolygonRef> &polygons) const
  {
    for (std::unordered_set<db::PolygonRef>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
      if (! selected (*p)) {
        return false;
      }
    }
    return true;
  }

  virtual bool selected_set (const std::unordered_set<db::Polygon> &polygons) const
  {
    for (std::unordered_set<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
      if (! selected (*p)) {
        return false;
      }
    }
    return true;
  }

};

/**
 *  @brief A filter for rectilinear polygons
 *
 *  This filter will select all polygons which are rectilinear.
 */

struct DB_PUBLIC RectilinearFilter
  : public AllMustMatchFilter
{
  /**
   *  @brief Constructor
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RectilinearFilter (bool inverse);

  /**
   *  @brief Returns true if the polygon is rectilinear
   */
  virtual bool selected (const db::Polygon &poly) const;

  /**
   *  @brief Returns true if the polygon is rectilinear
   */
  virtual bool selected (const db::PolygonRef &poly) const;

  /**
   *  @brief This filter does not need variants
   */
  virtual const TransformationReducer *vars () const;

  /**
   *  @brief This filter prefers producing variants
   */
  virtual bool wants_variants () const { return true; }

  /**
   *  @brief This filter wants merged input
   */
  virtual bool requires_raw_input () const { return false; }

private:
  bool m_inverse;
};

/**
 *  @brief A rectangle filter
 *
 *  This filter will select all polygons which are rectangles.
 */

struct DB_PUBLIC RectangleFilter
  : public AllMustMatchFilter
{
  /**
   *  @brief Constructor
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RectangleFilter (bool is_square, bool inverse);

  /**
   *  @brief Returns true if the polygon is a rectangle
   */
  virtual bool selected (const db::Polygon &poly) const;

  /**
   *  @brief Returns true if the polygon is a rectangle
   */
  virtual bool selected (const db::PolygonRef &poly) const;

  /**
   *  @brief This filter does not need variants
   */
  virtual const TransformationReducer *vars () const;

  /**
   *  @brief This filter prefers producing variants
   */
  virtual bool wants_variants () const { return true; }

  /**
   *  @brief This filter wants merged input
   */
  virtual bool requires_raw_input () const { return false; }

private:
  bool m_is_square;
  bool m_inverse;
};

/**
 *  @brief Filters by number of holes
 *
 *  This filter will select all polygons with a hole count between min_holes and max_holes (exclusively)
 */

struct DB_PUBLIC HoleCountFilter
  : public AllMustMatchFilter
{
  /**
   *  @brief Constructor
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  HoleCountFilter (size_t min_count, size_t max_count, bool inverse);

  /**
   *  @brief Returns true if the polygon is a rectangle
   */
  virtual bool selected (const db::Polygon &poly) const;

  /**
   *  @brief Returns true if the polygon is a rectangle
   */
  virtual bool selected (const db::PolygonRef &poly) const;

  /**
   *  @brief This filter does not need variants
   */
  virtual const TransformationReducer *vars () const;

  /**
   *  @brief This filter prefers producing variants
   */
  virtual bool wants_variants () const { return true; }

  /**
   *  @brief This filter wants merged input
   */
  virtual bool requires_raw_input () const { return false; }

private:
  size_t m_min_count, m_max_count;
  bool m_inverse;
};

/**
 *  @brief A bounding box filter for use with Region::filter or Region::filtered
 *
 *  This filter has two parameters: vmin and vmax.
 *  It will filter all polygons for which the selected bounding box parameter is >= vmin and < vmax.
 *  There is an "invert" flag which allows selecting all polygons not
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
  : public AllMustMatchFilter
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
  RegionBBoxFilter (value_type vmin, value_type vmax, bool inverse, parameter_type parameter);

  /**
   *  @brief Returns true if the polygon's bounding box matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const;

  /**
   *  @brief Returns true if the polygon's bounding box matches the criterion
   */
  virtual bool selected (const db::PolygonRef &poly) const;

  /**
   *  @brief This filter is isotropic unless the parameter is width or height
   */
  virtual const TransformationReducer *vars () const;

  /**
   *  @brief This filter prefers producing variants
   */
  virtual bool wants_variants () const { return true; }

  /**
   *  @brief This filter wants merged input
   */
  virtual bool requires_raw_input () const { return false; }

private:
  value_type m_vmin, m_vmax;
  bool m_inverse;
  parameter_type m_parameter;
  db::MagnificationReducer m_isotropic_vars;
  db::XYAnisotropyAndMagnificationReducer m_anisotropic_vars;

  bool check (const db::Box &box) const;
};

/**
 *  @brief A ratio filter for use with Region::filter or Region::filtered
 *
 *  This filter can select polygons based on certain ratio values.
 *  "ratio values" are typically in the order of 1 and floating point
 *  values. Ratio values are always >= 0.
 *
 *  Available ratio values are:
 *    - (AreaRatio) area ratio (bounding box area vs. polygon area)
 *    - (AspectRatio) bounding box aspect ratio (max / min)
 *    - (RelativeHeight) bounding box height to width (tallness)
 */

struct DB_PUBLIC RegionRatioFilter
  : public AllMustMatchFilter
{
  /**
   *  @brief The parameters available
   */
  enum parameter_type {
    AreaRatio,
    AspectRatio,
    RelativeHeight
  };

  /**
   *  @brief Constructor
   *
   *  @param vmin The min value (only polygons with bounding box parameters above this value are filtered)
   *  @param vmax The max value (only polygons with bounding box parameters below this value are filtered)
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  RegionRatioFilter (double vmin, bool min_included, double vmax, bool max_included, bool inverse, parameter_type parameter);

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::Polygon &poly) const;

  /**
   *  @brief Returns true if the polygon's area matches the criterion
   */
  virtual bool selected (const db::PolygonRef &poly) const;

  /**
   *  @brief This filter is isotropic unless the parameter is width or height
   */
  virtual const TransformationReducer *vars () const;

  /**
   *  @brief This filter prefers producing variants
   */
  virtual bool wants_variants () const { return true; }

  /**
   *  @brief This filter wants merged input
   */
  virtual bool requires_raw_input () const { return false; }

private:
  double m_vmin, m_vmax;
  bool m_vmin_included, m_vmax_included;
  bool m_inverse;
  parameter_type m_parameter;
  db::MagnificationReducer m_isotropic_vars;
  db::XYAnisotropyAndMagnificationReducer m_anisotropic_vars;
};

/**
 *  @brief A polygon processor filtering strange polygons
 *
 *  "strange polygons" are those which do not have a specific orientation, e.g.
 *  "8" shape polygons.
 */
class DB_PUBLIC StrangePolygonCheckProcessor
  : public PolygonProcessorBase
{
public:
  StrangePolygonCheckProcessor ();
  ~StrangePolygonCheckProcessor ();

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const;

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool requires_raw_input () const { return true; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }
};

/**
 *  @brief A polygon processor applying smoothing
 */
class DB_PUBLIC SmoothingProcessor
  : public PolygonProcessorBase
{
public:
  SmoothingProcessor (db::Coord d, bool keep_hv);
  ~SmoothingProcessor ();

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }

private:
  db::Coord m_d;
  bool m_keep_hv;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief A polygon processor generating rounded corners
 */
class DB_PUBLIC RoundedCornersProcessor
  : public PolygonProcessorBase
{
public:
  RoundedCornersProcessor (double rinner, double router, unsigned int n);
  ~RoundedCornersProcessor ();

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return true; }   //  we believe so ...
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }

private:
  double m_rinner, m_router;
  unsigned int m_n;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief A polygon processor extracting the holes
 */
class DB_PUBLIC HolesExtractionProcessor
  : public PolygonProcessorBase
{
public:
  HolesExtractionProcessor ();
  ~HolesExtractionProcessor ();

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const;

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }  //  isn't merged for nested holes :(
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }
};

/**
 *  @brief A polygon processor extracting the hull
 */
class DB_PUBLIC HullExtractionProcessor
  : public PolygonProcessorBase
{
public:
  HullExtractionProcessor ();
  ~HullExtractionProcessor ();

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const;

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }   //  isn't merged for nested hulls :(
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }
};

/**
 *  @brief A class wrapping the single-polygon checks into a polygon-to-edge pair processor
 */
class DB_PUBLIC SinglePolygonCheck
  : public PolygonToEdgePairProcessorBase
{
public:
  SinglePolygonCheck (db::edge_relation_type rel, db::Coord d, const RegionCheckOptions &options);

  virtual void process (const db::Polygon &polygon, std::vector<db::EdgePair> &res) const;
  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool wants_variants () const { return true; }

private:
  db::edge_relation_type m_relation;
  db::Coord m_d;
  db::RegionCheckOptions m_options;
  db::MagnificationReducer m_vars;
};

} // namespace db

#endif

