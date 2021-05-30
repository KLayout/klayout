
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
  db::MagnificationAndOrientationReducer m_anisotropic_vars;

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
  db::MagnificationAndOrientationReducer m_anisotropic_vars;
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
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
class DB_PUBLIC Edge2EdgeCheckBase
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  Edge2EdgeCheckBase (const EdgeRelationFilter &check, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges);

  /**
   *  @brief Call this to initiate a new pass until the return value is false
   */
  bool prepare_next_pass ();

  /**
   *  @brief Before the scanner is run, this method must be called to feed additional edges into the scanner
   *  (required for negative edge output - cancellation of perpendicular edges)
   */
  bool feed_pseudo_edges (db::box_scanner<db::Edge, size_t> &scanner);

  /**
   *  @brief Reimplementation of the box_scanner_receiver interface
   */
  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2);

  /**
   *  @brief Reimplementation of the box_scanner_receiver interface
   */
  void finish (const Edge *o, const size_t &);

  /**
   *  @brief Gets a value indicating whether the check requires different layers
   */
  bool requires_different_layers () const;

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_requires_different_layers (bool f);

  /**
   *  @brief Gets a value indicating whether the check requires different layers
   */
  bool different_polygons () const;

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_different_polygons (bool f);

  /**
   *  @brief Sets a flag indicating that this class wants negative edge output
   */
  void set_has_negative_edge_output (bool f)
  {
    m_has_negative_edge_output = f;
  }

  /**
   *  @brief Gets a flag indicating that this class wants negative edge output
   */
  bool has_negative_edge_output () const
  {
    return m_has_negative_edge_output;
  }

  /**
   *  @brief Sets a flag indicating that this class wants normal edge pair output
   */
  void set_has_edge_pair_output (bool f)
  {
    m_has_edge_pair_output = f;
  }

  /**
   *  @brief Gets a flag indicating that this class wants normal edge pair output
   */
  bool has_edge_pair_output () const
  {
    return m_has_edge_pair_output;
  }

  /**
   *  @brief Gets the distance value
   */
  EdgeRelationFilter::distance_type distance () const;

protected:
  /**
   *  @brief Normal edge pair output (violations)
   */
  virtual void put (const db::EdgePair & /*edge*/, bool /*intra-polygon*/) const { }

  /**
   *  @brief Negative edge output
   */
  virtual void put_negative (const db::Edge & /*edge*/, int /*layer*/) const { }

private:
  const EdgeRelationFilter *mp_check;
  bool m_requires_different_layers;
  bool m_different_polygons;
  EdgeRelationFilter::distance_type m_distance;
  std::vector<db::EdgePair> m_ep;
  std::multimap<std::pair<db::Edge, size_t>, size_t> m_e2ep;
  std::set<std::pair<db::Edge, size_t> > m_pseudo_edges;
  size_t m_first_pseudo;
  std::vector<bool> m_ep_discarded, m_ep_intra_polygon;
  bool m_with_shielding;
  bool m_symmetric_edges;
  bool m_has_edge_pair_output;
  bool m_has_negative_edge_output;
  unsigned int m_pass;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 */
template <class Output>
class DB_PUBLIC_TEMPLATE edge2edge_check
  : public Edge2EdgeCheckBase
{
public:
  edge2edge_check (const EdgeRelationFilter &check, Output &output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges)
    : Edge2EdgeCheckBase (check, different_polygons, requires_different_layers, with_shielding, symmetric_edges), mp_output_inter (&output), mp_output_intra (0)
  {
    //  .. nothing yet ..
  }

  edge2edge_check (const EdgeRelationFilter &check, Output &output_inter, Output &output_intra, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges)
    : Edge2EdgeCheckBase (check, different_polygons, requires_different_layers, with_shielding, symmetric_edges), mp_output_inter (&output_inter), mp_output_intra (&output_intra)
  {
    //  .. nothing yet ..
  }

protected:
  void put (const db::EdgePair &edge, bool inter_polygon) const
  {
    if (! inter_polygon || ! mp_output_intra) {
      mp_output_inter->insert (edge);
    } else {
      mp_output_intra->insert (edge);
    }
  }

private:
  Output *mp_output_inter;
  Output *mp_output_intra;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 *  This version allows delivery of the negative edges.
 */
template <class Output, class NegativeEdgeOutput>
class DB_PUBLIC_TEMPLATE edge2edge_check_with_negative_output
  : public edge2edge_check<Output>
{
public:
  edge2edge_check_with_negative_output (const EdgeRelationFilter &check, Output &output, NegativeEdgeOutput &l1_negative_output, NegativeEdgeOutput &l2_negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges)
    : edge2edge_check<Output> (check, output, different_polygons, requires_different_layers, with_shielding, symmetric_edges),
      mp_l1_negative_output (&l1_negative_output),
      mp_l2_negative_output (&l2_negative_output)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (true);
  }

  edge2edge_check_with_negative_output (const EdgeRelationFilter &check, Output &output_inter, Output &output_intra, NegativeEdgeOutput &l1_negative_output, NegativeEdgeOutput &l2_negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges)
    : edge2edge_check<Output> (check, output_inter, output_intra, different_polygons, requires_different_layers, with_shielding, symmetric_edges),
      mp_l1_negative_output (&l1_negative_output),
      mp_l2_negative_output (&l2_negative_output)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (true);
  }

protected:
  void put_negative (const db::Edge &edge, int layer) const
  {
    if (layer == 0) {
      mp_l1_negative_output->insert (edge);
    }
    if (layer == 1) {
      mp_l2_negative_output->insert (edge);
    }
  }

private:
  NegativeEdgeOutput *mp_l1_negative_output, *mp_l2_negative_output;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 *  This version has only negative edge output.
 */
template <class NegativeEdgeOutput>
class DB_PUBLIC_TEMPLATE edge2edge_check_negative
  : public Edge2EdgeCheckBase
{
public:
  edge2edge_check_negative (const EdgeRelationFilter &check, NegativeEdgeOutput &l1_negative_output, NegativeEdgeOutput &l2_negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding)
    : Edge2EdgeCheckBase (check, different_polygons, requires_different_layers, with_shielding, false),
      mp_l1_negative_output (&l1_negative_output),
      mp_l2_negative_output (&l2_negative_output)
  {
    set_has_negative_edge_output (true);
    set_has_edge_pair_output (false);
  }

protected:
  void put_negative (const db::Edge &edge, int layer) const
  {
    if (layer == 0) {
      mp_l1_negative_output->insert (edge);
    }
    if (layer == 1) {
      mp_l2_negative_output->insert (edge);
    }
  }

private:
  NegativeEdgeOutput *mp_l1_negative_output, *mp_l2_negative_output;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 *  This version has positive or negative output. Negative output is mapped to edge pairs
 *  as well.
 */
template <class Output>
class DB_PUBLIC_TEMPLATE edge2edge_check_negative_or_positive
  : public edge2edge_check<Output>
{
public:
  edge2edge_check_negative_or_positive (const EdgeRelationFilter &check, Output &output, bool negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric)
    : edge2edge_check<Output> (check, output, different_polygons, requires_different_layers, with_shielding, symmetric)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (negative_output);
    edge2edge_check<Output>::set_has_edge_pair_output (! negative_output);
  }

  edge2edge_check_negative_or_positive (const EdgeRelationFilter &check, Output &output_inter, Output &output_intra, bool negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric)
    : edge2edge_check<Output> (check, output_inter, output_intra, different_polygons, requires_different_layers, with_shielding, symmetric)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (negative_output);
    edge2edge_check<Output>::set_has_edge_pair_output (! negative_output);
  }

protected:
  void put_negative (const db::Edge &edge, int layer) const
  {
    if (layer == 0) {
      edge2edge_check<Output>::put (db::EdgePair (edge, edge.swapped_points ()), false);
    }
#if 0
    //  NOTE: second-input negative edge output isn't worth a lot as the second input often is not merged, hence
    //  the outer edges to not represent the actual contour.
    if (layer == 1) {
      edge2edge_check<Output>::put (db::EdgePair (edge.swapped_points (), edge), false);
    }
#endif
  }
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
template <class PolygonType>
class DB_PUBLIC poly2poly_check
  : public db::box_scanner_receiver<PolygonType, size_t>
{
public:
  poly2poly_check (Edge2EdgeCheckBase &output);

  void finish (const PolygonType *o, size_t p);
  void enter (const PolygonType&o, size_t p);
  void add (const PolygonType *o1, size_t p1, const PolygonType *o2, size_t p2);
  void enter (const PolygonType &o1, size_t p1, const PolygonType &o2, size_t p2);

private:
  db::Edge2EdgeCheckBase *mp_output;
  db::box_scanner<db::Edge, size_t> m_scanner;
  std::vector<db::Edge> m_edges;
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

private:
  db::edge_relation_type m_relation;
  db::Coord m_d;
  db::RegionCheckOptions m_options;
};

/**
 *  @brief A helper class for the region to edge interaction functionality
 */
template <class PolygonType, class EdgeType, class OutputType>
class DB_PUBLIC region_to_edge_interaction_filter_base
  : public db::box_scanner_receiver2<PolygonType, size_t, db::Edge, size_t>
{
public:
  region_to_edge_interaction_filter_base (bool inverse, bool get_all);

  void preset (const OutputType *s);
  void add (const PolygonType *p, size_t, const EdgeType *e, size_t);
  void fill_output ();

protected:
  virtual void put (const OutputType &s) const = 0;

private:
  std::set<const OutputType *> m_seen;
  bool m_inverse, m_get_all;
};

/**
 *  @brief A helper class for the region to edge interaction functionality
 */
template <class PolygonType, class EdgeType, class OutputContainer, class OutputType = typename OutputContainer::value_type>
class DB_PUBLIC_TEMPLATE region_to_edge_interaction_filter
  : public region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>
{
public:
  region_to_edge_interaction_filter (OutputContainer &output, bool inverse, bool get_all = false)
    : region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType> (inverse, get_all), mp_output (&output)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void put (const OutputType &res) const
  {
    mp_output->insert (res);
  }

private:
  OutputContainer *mp_output;
};

/**
 *  @brief A helper class for the region to text interaction functionality
 */
template <class PolygonType, class TextType, class OutputType>
class DB_PUBLIC region_to_text_interaction_filter_base
  : public db::box_scanner_receiver2<PolygonType, size_t, TextType, size_t>
{
public:
  region_to_text_interaction_filter_base (bool inverse, bool get_all);

  void preset (const OutputType *s);
  void add (const PolygonType *p, size_t, const TextType *e, size_t);
  void fill_output ();

protected:
  virtual void put (const OutputType &s) const = 0;

private:
  std::set<const OutputType *> m_seen;
  bool m_inverse, m_get_all;
};

/**
 *  @brief A helper class for the region to text interaction functionality
 */
template <class PolygonType, class TextType, class OutputContainer, class OutputType = typename OutputContainer::value_type>
class DB_PUBLIC_TEMPLATE region_to_text_interaction_filter
  : public region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>
{
public:
  region_to_text_interaction_filter (OutputContainer &output, bool inverse, bool get_all = false)
    : region_to_text_interaction_filter_base<PolygonType, TextType, OutputType> (inverse, get_all), mp_output (&output)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void put (const OutputType &poly) const
  {
    mp_output->insert (poly);
  }

private:
  OutputContainer *mp_output;
};

template <class C>
static inline C snap_to_grid (C c, C g)
{
  //  This form of snapping always snaps g/2 to right/top.
  if (c < 0) {
    c = -g * ((-c + (g - 1) / 2) / g);
  } else {
    c = g * ((c + g / 2) / g);
  }
  return c;
}

/**
 *  @brief Snaps a polygon to the given grid
 *  Heap is a vector of points reused for the point list
 */
DB_PUBLIC db::Polygon snapped_polygon (const db::Polygon &poly, db::Coord gx, db::Coord gy, std::vector<db::Point> &heap);

/**
 *  @brief Scales and snaps a polygon to the given grid
 *  Heap is a vector of points reused for the point list
 *  The coordinate transformation is q = ((p * m + o) snap (g * d)) / d.
 */
DB_PUBLIC db::Polygon scaled_and_snapped_polygon (const db::Polygon &poly, db::Coord gx, db::Coord mx, db::Coord dx, db::Coord ox, db::Coord gy, db::Coord my, db::Coord dy, db::Coord oy, std::vector<db::Point> &heap);

/**
 *  @brief Scales and snaps a vector to the given grid
 *  The coordinate transformation is q = ((p * m + o) snap (g * d)) / d.
 */
DB_PUBLIC db::Vector scaled_and_snapped_vector (const db::Vector &v, db::Coord gx, db::Coord mx, db::Coord dx, db::Coord ox, db::Coord gy, db::Coord my, db::Coord dy, db::Coord oy);

} // namespace db

#endif

