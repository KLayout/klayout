
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
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
class DB_PUBLIC Edge2EdgeCheckBase
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  Edge2EdgeCheckBase (const EdgeRelationFilter &check, bool different_polygons, bool requires_different_layers);

  /**
   *  @brief Call this to initiate a new pass until the return value is false
   */
  bool prepare_next_pass ();

  /**
   *  @brief Reimplementation of the box_scanner_receiver interface
   */
  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2);

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
   *  @brief Gets the distance value
   */
  EdgeRelationFilter::distance_type distance () const;

protected:
  virtual void put (const db::EdgePair &edge) const = 0;

private:
  const EdgeRelationFilter *mp_check;
  bool m_requires_different_layers;
  bool m_different_polygons;
  EdgeRelationFilter::distance_type m_distance;
  std::vector<db::EdgePair> m_ep;
  std::multimap<std::pair<db::Edge, size_t>, size_t> m_e2ep;
  std::vector<bool> m_ep_discarded;
  unsigned int m_pass;
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
template <class Output>
class DB_PUBLIC_TEMPLATE edge2edge_check
  : public Edge2EdgeCheckBase
{
public:
  edge2edge_check (const EdgeRelationFilter &check, Output &output, bool different_polygons, bool requires_different_layers)
    : Edge2EdgeCheckBase (check, different_polygons, requires_different_layers), mp_output (&output)
  {
    //  .. nothing yet ..
  }

protected:
  void put (const db::EdgePair &edge) const
  {
    mp_output->insert (edge);
  }

private:
  Output *mp_output;
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
class DB_PUBLIC Poly2PolyCheckBase
  : public db::box_scanner_receiver<db::Polygon, size_t>
{
public:
  Poly2PolyCheckBase (Edge2EdgeCheckBase &output);

  void finish (const db::Polygon *o, size_t p);
  void enter (const db::Polygon &o, size_t p);
  void add (const db::Polygon *o1, size_t p1, const db::Polygon *o2, size_t p2);
  void enter (const db::Polygon &o1, size_t p1, const db::Polygon &o2, size_t p2);

private:
  db::Edge2EdgeCheckBase *mp_output;
  db::box_scanner<db::Edge, size_t> m_scanner;
  std::vector<db::Edge> m_edges;
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
template <class Output>
class DB_PUBLIC_TEMPLATE poly2poly_check
  : public Poly2PolyCheckBase
{
public:
  poly2poly_check (edge2edge_check<Output> &output)
    : Poly2PolyCheckBase (output)
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief A helper class for the region to edge interaction functionality
 */
template <class OutputType>
class DB_PUBLIC region_to_edge_interaction_filter_base
  : public db::box_scanner_receiver2<db::Polygon, size_t, db::Edge, size_t>
{
public:
  region_to_edge_interaction_filter_base (bool inverse);

  void preset (const OutputType *s);
  void add (const db::Polygon *p, size_t, const db::Edge *e, size_t);
  void fill_output ();

protected:
  virtual void put (const OutputType &s) const = 0;

private:
  std::set<const OutputType *> m_seen;
  bool m_inverse;
};

/**
 *  @brief A helper class for the region to edge interaction functionality
 */
template <class OutputContainer, class OutputType = typename OutputContainer::value_type>
class DB_PUBLIC_TEMPLATE region_to_edge_interaction_filter
  : public region_to_edge_interaction_filter_base<OutputType>
{
public:
  region_to_edge_interaction_filter (OutputContainer &output, bool inverse)
    : region_to_edge_interaction_filter_base<OutputType> (inverse), mp_output (&output)
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

