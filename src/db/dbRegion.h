
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#ifndef HDR_dbRegion
#define HDR_dbRegion

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbTrans.h"
#include "dbShape.h"
#include "dbShapes.h"
#include "dbShapes2.h"
#include "dbEdgePairRelations.h"
#include "dbShapeProcessor.h"
#include "dbEdges.h"
#include "dbRecursiveShapeIterator.h"
#include "dbEdgePairs.h"
#include "tlString.h"
#include "gsiObject.h"

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
  bool operator() (const db::Polygon &poly) const
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

private:
  perimeter_type m_pmin, m_pmax;
  bool m_inverse;
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
  bool operator() (const db::Polygon &poly) const
  {
    area_type a = poly.area (); 
    if (! m_inverse) {
      return a >= m_amin && a < m_amax;
    } else {
      return ! (a >= m_amin && a < m_amax);
    }
  }

private:
  area_type m_amin, m_amax;
  bool m_inverse;
};

/**
 *  @brief A filter for rectilinear polygons
 *
 *  This filter will select all polygons which are rectilinear.
 */

struct DB_PUBLIC RectilinearFilter
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
  bool operator() (const db::Polygon &poly) const
  {
    return poly.is_rectilinear () != m_inverse;
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
  bool operator() (const db::Polygon &poly) const
  {
    return poly.is_box () != m_inverse;
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
  bool operator() (const db::Polygon &poly) const
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

private:
  value_type m_vmin, m_vmax;
  bool m_inverse;
  parameter_type m_parameter;
};

/**
 *  @brief A region iterator
 *
 *  The iterator delivers the polygons of the region
 */

class DB_PUBLIC RegionIterator
{
public:
  typedef db::Polygon value_type; 
  typedef const db::Polygon &reference;
  typedef const db::Polygon *pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @Returns true, if the iterator is at the end
   */
  bool at_end () const
  {
    return m_from == m_to && m_rec_iter.at_end ();
  }

  /**
   *  @brief Increment
   */
  RegionIterator &operator++ () 
  {
    inc ();
    set ();
    return *this;
  }

  /**
   *  @brief Access
   */
  reference operator* () const
  {
    if (m_rec_iter.at_end ()) {
      return *m_from;
    } else {
      return m_polygon;
    }
  }

  /**
   *  @brief Access
   */
  pointer operator-> () const
  {
    if (m_rec_iter.at_end ()) {
      return &*m_from;
    } else {
      return &m_polygon;
    }
  }

private:
  friend class Region;

  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator iterator_type;

  db::RecursiveShapeIterator m_rec_iter;
  db::ICplxTrans m_iter_trans;
  db::Polygon m_polygon;
  iterator_type m_from, m_to;

  /**
   *  @brief ctor from a recursive shape iterator
   */
  RegionIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
    : m_rec_iter (iter), m_iter_trans (trans), m_from (), m_to ()
  { 
    //  NOTE: the following initialization appears to be required on some compilers
    //  (specifically MacOS/clang) to ensure the proper initialization of the iterators
    m_from = m_to;
    set ();
  }

  /**
   *  @brief ctor from a range of polygons inside a vector
   */
  RegionIterator (iterator_type from, iterator_type to)
    : m_from (from), m_to (to)
  { 
    //  no required yet: set ();
  }

  /**
   *  @brief Establish the iterator at the current position
   */
  void set ()
  {
    while (! m_rec_iter.at_end () && ! (m_rec_iter.shape ().is_polygon () || m_rec_iter.shape ().is_path () || m_rec_iter.shape ().is_box ())) {
      inc ();
    }
    if (! m_rec_iter.at_end ()) {
      m_rec_iter.shape ().polygon (m_polygon);
      m_polygon.transform (m_iter_trans * m_rec_iter.trans (), false);
    } 
  }

  /**
   *  @brief Increment the iterator
   */
  void inc ()
  {
    if (! m_rec_iter.at_end ()) {
      ++m_rec_iter;
    } else {
      ++m_from;
    }
  }
};

/**
 *  @brief A region
 *
 *  A region basically is a set of polygons. It supports a variety of operations, i.e.
 *  boolean operations with other regions, sizing etc.
 *
 *  Regions can have different states. Specifically a region can be merged (no overlapping
 *  polygons are present, touching polygons are merged, self-intersections of polygons are
 *  removed) or non-merged (polygons may overlap or polygons may be self-intersecting). In
 *  merged state, the wrap count at every point is either zero or 1, in non-merged state 
 *  it can be every value.
 *
 *  Polygons inside the region may contain holes if the region is merged.
 */

class DB_PUBLIC Region
  : public gsi::ObjectBase
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Polygon polygon_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type; 
  typedef coord_traits::perimeter_type perimeter_type; 
  typedef coord_traits::area_type area_type; 
  typedef RegionIterator const_iterator;

  /** 
   *  @brief Default constructor
   *
   *  Creates an empty region.
   */
  Region ()
    : m_polygons (false), m_merged_polygons (false)
  {
    init ();
  }

  /**
   *  @brief Constructor from an object
   *
   *  Creates a region representing a single instance of that object
   */
  template <class Sh>
  Region (const Sh &s)
    : m_polygons (false), m_merged_polygons (false)
  {
    init ();
    insert (s);
  }

  /**
   *  @brief Sequence constructor
   *
   *  Creates a region from a sequence of objects. The objects can be boxes, 
   *  polygons, paths or shapes. This version accepts iterators of the begin ... end
   *  style.
   */
  template <class Iter>
  Region (const Iter &b, const Iter &e)
    : m_polygons (false), m_merged_polygons (false)
  {
    init ();
    reserve (e - b);
    for (Iter i = b; i != e; ++i) {
      insert (*i);
    }
  }

  /**
   *  @brief Constructor from a RecursiveShapeIterator
   *
   *  Creates a region from a recursive shape iterator. This allows to feed a region
   *  from a hierarchy of cells.
   */
  Region (const RecursiveShapeIterator &si);

  /**
   *  @brief Constructor from a RecursiveShapeIterator with a transformation
   *
   *  Creates a region from a recursive shape iterator. This allows to feed a region
   *  from a hierarchy of cells. The transformation is useful to scale to a specific
   *  DBU for example.
   */
  Region (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics = true);

  /**
   *  @brief Enable progress reporting
   *
   *  @param progress_text The description text of the progress object
   */
  void enable_progress (const std::string &progress_desc = std::string ());

  /**
   *  @brief Disable progress reporting
   */
  void disable_progress ();

  /**
   *  @brief Iterator of the region
   *
   *  The iterator delivers the polygons of the region.
   *  It follows the at_end semantics.
   */
  const_iterator begin () const
  {
    if (has_valid_polygons ()) {
      return const_iterator (m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
    } else {
      return const_iterator (m_iter, m_iter_trans);
    }
  }

  /**
   *  @brief Returns the merged polygons if merge semantics applies 
   *
   *  If merge semantics is not enabled, this iterator delivers the individual polygons.
   */
  const_iterator begin_merged () const;

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the polygons plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the merged polygons plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  /**
   *  @brief Insert a box into the region
   */
  void insert (const db::Box &box);

  /**
   *  @brief Insert a path into the region
   */
  void insert (const db::Path &path);

  /**
   *  @brief Insert a simple polygon into the region
   */
  void insert (const db::SimplePolygon &polygon);

  /**
   *  @brief Insert a polygon into the region
   */
  void insert (const db::Polygon &polygon);

  /**
   *  @brief Insert a shape into the region
   */
  void insert (const db::Shape &shape);

  /**
   *  @brief Insert a transformed shape into the region
   */
  template <class T>
  void insert (const db::Shape &shape, const T &trans)
  {
    if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
      ensure_valid_polygons ();
      db::Polygon poly;
      shape.polygon (poly);
      poly.transform (trans);
      m_polygons.insert (poly);
      m_is_merged = false;
      invalidate_cache ();
    }
  }

  /**
   *  @brief Returns true if the region is empty
   */
  bool empty () const
  {
    return has_valid_polygons () && m_polygons.empty ();
  }

  /**
   *  @brief Returns the number of polygons in the region
   */
  size_t size () const;

  /**
   *  @brief Returns a string representing the region
   *
   *  nmax specifies how many polygons are included (set to std::numeric_limits<size_t>::max() for "all".
   */
  std::string to_string (size_t nmax = 10) const;

  /**
   *  @brief Clear the region
   */
  void clear ();

  /**
   *  @brief Reserve memory for the given number of polygons
   */
  void reserve (size_t n)
  {
    m_polygons.reserve (db::Polygon::tag (), n);
  }

  /**
   *  @brief Sets the minimum-coherence flag
   *
   *  If minimum coherence is set, the merge operations (explicit merge with \merge or
   *  implicit merge through merged_semantics) are performed using minimum coherence mode.
   *  The coherence mode determines how kissing-corner situations are resolved. If
   *  minimum coherence is selected, they are resolved such that multiple polygons are 
   *  created which touch at a corner).
   */
  void set_min_coherence (bool f)
  {
    if (m_merge_min_coherence != f) {
      m_merge_min_coherence = f;
      invalidate_cache ();
    }
  }

  /**
   *  @brief Gets the minimum coherence flag
   */
  bool min_coherence () const
  {
    return m_merge_min_coherence;
  }

  /**
   *  @brief Sets the merged-semantics flag
   *
   *  If merged semantics is enabled (the default), coherent polygons will be considered 
   *  as single regions and artificial edges such as cut-lines will not be considered. 
   *  Merged semantics thus is equivalent to considering coherent areas rather than
   *  single polygons.
   */
  void set_merged_semantics (bool f);

  /**
   *  @brief Gets the merged-semantics flag
   */
  bool merged_semantics () const
  {
    return m_merged_semantics;
  }

  /**
   *  @brief Enables or disables strict handling
   *
   *  Strict handling means to leave away some optimizations. Specifically the 
   *  output of boolean operations will be merged even if one input is empty.
   *  Without strict handling, the operation will be optimized and output 
   *  won't be merged.
   *
   *  Strict handling is disabled by default.
   */
  void set_strict_handling (bool f);

  /**
   *  @brief Gets a valid indicating whether strict handling is enabled
   */
  bool strict_handling () const
  {
    return m_strict_handling;
  }

  /**
   *  @brief Returns true if the region is a single box
   *
   *  If the region is not merged, this method may return false even
   *  if the merged region would be a box.
   */
  bool is_box () const;

  /**
   *  @brief Returns true if the region is merged 
   */
  bool is_merged () const
  {
    return m_is_merged;
  }

  /**
   *  @brief Returns the area of the region
   *
   *  This method returns the area sum over all polygons. 
   *  Merged semantics applies. In merged semantics, the area is the correct area covered by the 
   *  polygons. Without merged semantics, overlapping parts are counted twice.
   *
   *  If a box is given, the computation is restricted to that box.
   */
  area_type area (const db::Box &box = db::Box ()) const;

  /**
   *  @brief Returns the perimeter sum of the region
   *
   *  This method returns the perimeter sum over all polygons. 
   *  Merged semantics applies. In merged semantics, the perimeter is the true perimeter. 
   *  Without merged semantics, inner edges contribute to the perimeter.
   *
   *  If a box is given, the computation is restricted to that box.
   *  Edges coincident with the box edges are counted only if the form outer edges at the box edge.
   */
  perimeter_type perimeter (const db::Box &box = db::Box ()) const;

  /**
   *  @brief Returns the bounding box of the region
   */
  Box bbox () const
  {
    ensure_bbox_valid ();
    return m_bbox;
  }

  /**
   *  @brief Filters the polygons 
   *
   *  This method will keep all polygons for which the filter returns true.
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged polygons.
   */
  template <class F>
  Region &filter (F &filter)
  {
    polygon_iterator_type pw = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin ();
    for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
      if (filter (*p)) {
        if (pw == m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ()) {
          m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().insert (*p);
          pw = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ();
        } else {
          m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().replace (pw++, *p);
        } 
      }
    }
    m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().erase (pw, m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
    m_merged_polygons.clear ();
    m_is_merged = m_merged_semantics;
    m_iter = db::RecursiveShapeIterator ();
    return *this;
  }

  /**
   *  @brief Returns the filtered polygons
   *
   *  This method will return a new region with only those polygons which 
   *  conform to the filter criterion.
   */
  template <class F>
  Region filtered (F &filter) const
  {
    Region d;
    for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
      if (filter (*p)) {
        d.insert (*p);
      }
    }
    return d;
  }

  /**
   *  @brief Applies a width check and returns EdgePairs which correspond to violation markers
   *
   *  The width check will create a edge pairs if the width of the area between the 
   *  edges is less than the specified threshold d. Without "whole_edges", the parts of
   *  the edges are returned which violate the condition. If "whole_edges" is true, the 
   *  result will contain the complete edges participating in the result.
   *
   *  The metrics parameter specifies which metrics to use. "Euclidian", "Square" and "Projected"
   *  metrics are available.
   *
   *  ingore_angle allows specification of a maximum angle that connected edges can have to not participate
   *  in the check. By choosing 90 degree, edges with angles of 90 degree and larger are not checked,
   *  but acute corners are for example. 
   *
   *  With min_projection and max_projection it is possible to specify how edges must be related 
   *  to each other. If the length of the projection of either edge on the other is >= min_projection
   *  or < max_projection, the edges are considered for the check.
   *
   *  The order of the edges in the resulting edge pairs is undefined.
   *
   *  Merged semantics applies.
   */
  EdgePairs width_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_single_polygon_check (db::WidthRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a space check and returns EdgePairs which correspond to violation markers
   *
   *  For the parameters see \width_check. The space check reports edges for which the space is
   *  less than the specified threshold d.
   *
   *  Merged semantics applies.
   */
  EdgePairs space_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::SpaceRelation, false, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a isolation check (space of polygon vs. other polygons) and returns EdgePairs which correspond to violation markers
   *
   *  For the parameters see \width_check. The space check reports edges for which the notch space is
   *  less than the specified threshold d.
   *
   *  Merged semantics applies.
   */
  EdgePairs isolated_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::SpaceRelation, true, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a notch check (space in polygon vs. itself) and returns EdgePairs which correspond to violation markers
   *
   *  For the parameters see \width_check. The space check reports edges for which the notch space is
   *  less than the specified threshold d.
   *
   *  Merged semantics applies.
   */
  EdgePairs notch_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_single_polygon_check (db::SpaceRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a enclosed check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region is enclosing the polygons of the other 
   *  region by less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs enclosing_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::OverlapRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a overlap check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region overlaps the polygons of the other 
   *  region by less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs overlap_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::WidthRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a separation check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region is separated by polygons of the other 
   *  region by less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs separation_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::SpaceRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a inside check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region is inside by less than the threshold d inside the polygons of the other 
   *  region.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs inside_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::InsideRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Returns an edge set containing all edges of the polygons in this region
   *
   *  Merged semantics applies. In merged semantics, only full, outer edges are delivered.
   */
  Edges edges () const;

  /**
   *  @brief Returns an edge set containing all edges of the polygons in this region 
   *
   *  This version allows to specify a filter by which the edges are filtered before they are 
   *  returned.
   *
   *  Merged semantics applies. In merged semantics, only full, outer edges are delivered.
   */
  template <class F>
  Edges edges (F &filter) const
  {
    Edges edges;
    for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
      for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {
        if (filter (*e)) {
          edges.insert (*e);
        }
      }
    }
    return edges;
  }

  /**
   *  @brief Transform the region
   */
  template <class T>
  Region &transform (const T &trans)
  {
    if (! trans.is_unity ()) {
      ensure_valid_polygons ();
      for (polygon_iterator_type p = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (); p != m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end (); ++p) {
        m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      m_iter_trans = db::ICplxTrans (trans) * m_iter_trans;
      m_bbox_valid = false;
    }
    return *this;
  }

  /**
   *  @brief Returns the transformed region
   */
  template <class T>
  Region transformed (const T &trans) const
  {
    Region d (*this);
    d.transform (trans);
    return d;
  }

  /**
   *  @brief Performs an off-grid check on the polygons inside the region
   *
   *  The method returns single-point edge pairs for each vertex found off-grid.
   *  The grid can be specified differently in x and y direction.
   */
  EdgePairs grid_check (db::Coord gx, db::Coord gy) const;

  /**
   *  @brief Performs an angle check
   *
   *  The method returns edge pairs for each connected edges having 
   *  an angle between min and max (exclusive max, in degree) or 
   *  not between min and max (if inverse is true).
   */
  EdgePairs angle_check (double min, double max, bool inverse) const;

  /**
   *  @brief Grid-snaps the region
   *
   *  Snaps the vertices of the polygons to the specified grid.
   *  different grids can be specified int x and y direction. 
   */
  void snap (db::Coord gx, db::Coord gy);

  /**
   *  @brief Returns the snapped region
   */
  Region snapped (db::Coord gx, db::Coord gy) const
  {
    Region d (*this);
    d.snap (gx, gy);
    return d;
  }

  /**
   *  @brief Performs a check for "strange" polygons
   *
   *  This check will return a region with all self-overlapping or 
   *  non-orientable parts of polygons.
   *
   *  Naturally this method will ignore the merged_semantics setting.
   */
  Region strange_polygon_check () const;

  /**
   *  @brief Swap with the other region
   */
  void swap (db::Region &other);

  /**
   *  @brief Merge the region
   *
   *  This method merges the polygons of the region if they are not merged already.
   *  It returns a reference to this region.
   *  An out-of-place merge version is "merged".
   */
  Region &merge ();

  /*
   *  @brief Returns the merged region
   *
   *  This is the out-of-place merge. It returns a new region but does not modify 
   *  the region it is called on. An in-place version is "merge".
   */
  Region merged () const
  {
    Region d (*this);
    d.merge ();
    return d;
  }

  /**
   *  @brief Merge the region with options
   *
   *  This will merge the region and provides some options while doing so.
   *
   *  A result is generated if the wrap count (wc)
   *  of a point is larger than the given min_wrapcount value.
   *  A min_wrapcount of 1 will produce output where at least two polygons overlap.
   *
   *  This method will always execute the merge, even if the region is already merged.
   *
   *  @param min_coherence Set this parameter to true to get minimum polygons (kissing corner problem)
   *  @param min_wrapcount See the description above
   *  @return A reference to this region
   */
  Region &merge (bool min_coherence, unsigned int min_wc = 0);

  /**
   *  @brief Returns the merged region with options
   *
   *  This is the out-of-place version of "merge" with options (see there).
   */
  Region merged (bool min_coherence, unsigned int min_wc = 0) const
  {
    Region d (*this);
    d.merge (min_coherence, min_wc);
    return d;
  }

  /**
   *  @brief Size the region
   *
   *  This method applies a sizing to the region. Before the sizing is done, the
   *  region is merged if this is not the case already.
   *
   *  The result is a set of polygons which may be overlapping, but are not self-
   *  intersecting. 
   *
   *  Merged semantics applies.
   *
   *  @param d The (isotropic) sizing value
   *  @param mode The sizing mode (see EdgeProcessor) for a description of the sizing mode which controls the miter distance.
   *  @return A reference to self
   */
  Region &size (coord_type d, unsigned int mode = 2)
  {
    return size (d, d, mode);
  }

  /**
   *  @brief Anisotropic sizing
   *
   *  This version provides anisotropic sizing by allowing to specify a distance int x and y
   *  direction.
   *
   *  Merged semantics applies.
   *
   *  @param dx The horizontal sizing
   *  @param dy The vertical sizing
   *  @param mode The sizing mode (see EdgeProcessor) for a description of the sizing mode which controls the miter distance.
   */
  Region &size (coord_type dx, coord_type dy, unsigned int mode = 2);

  /**
   *  @brief Returns the sized region
   *
   *  This is an out-of-place version of the size method with isotropic sizing
   *  "merged polygon" semantics applies if merged_polygon_semantics is true (see set_auto_merge).
   *
   *  Merged semantics applies.
   */
  Region sized (coord_type d, unsigned int mode = 2) const
  {
    Region r (*this);
    r.size (d, mode);
    return r;
  }

  /**
   *  @brief Returns the sized region
   *
   *  This is an out-of-place version of the size method with anisotropic sizing
   *  "merged polygon" semantics applies if merged_polygon_semantics is true (see set_auto_merge).
   *
   *  Merged semantics applies.
   */
  Region sized (coord_type dx, coord_type dy, unsigned int mode = 2) const
  {
    Region r (*this);
    r.size (dx, dy, mode);
    return r;
  }

  /**
   *  @brief Boolean AND operator
   */
  Region operator& (const Region &other) const
  {
    Region d (*this);
    d &= other;
    return d;
  }

  /**
   *  @brief In-place boolean AND operator
   *
   *  This method does not necessarily merge the region. To ensure the region
   *  is merged, call merge afterwards.
   */
  Region &operator&= (const Region &other);

  /**
   *  @brief Boolean NOT operator
   */
  Region operator- (const Region &other) const
  {
    Region d (*this);
    d -= other;
    return d;
  }

  /**
   *  @brief In-place boolean NOT operator
   *
   *  This method does not necessarily merge the region. To ensure the region
   *  is merged, call merge afterwards.
   */
  Region &operator-= (const Region &other);

  /**
   *  @brief Boolean XOR operator
   */
  Region operator^ (const Region &other) const
  {
    Region d (*this);
    d ^= other;
    return d;
  }

  /**
   *  @brief In-place boolean XOR operator
   *
   *  This method does not necessarily merge the region. To ensure the region
   *  is merged, call merge afterwards.
   */
  Region &operator^= (const Region &other);

  /**
   *  @brief Boolean OR operator
   *
   *  This method merges the polygons of both regions.
   */
  Region operator| (const Region &other) const
  {
    Region d (*this);
    d |= other;
    return d;
  }

  /**
   *  @brief In-place boolean OR operator
   */
  Region &operator|= (const Region &other);

  /**
   *  @brief Joining of regions
   *
   *  This method joins the regions but does not merge them afterwards.
   */
  Region operator+ (const Region &other) const
  {
    Region d (*this);
    d += other;
    return d;
  }

  /**
   *  @brief In-place region joining
   */
  Region &operator+= (const Region &other);

  /**
   *  @brief Selects all polygons of this region which are completly outside polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_outside (const Region &other)
  {
    select_interacting_generic (other, 1, false, false);
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which are not completly outside polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_not_outside (const Region &other)
  {
    select_interacting_generic (other, 1, false, true);
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which are completly outside polygons from the other region
   *
   *  This method is an out-of-place version of select_outside.
   *
   *  Merged semantics applies.
   */
  Region selected_outside (const Region &other) const
  {
    return selected_interacting_generic (other, 1, false, false);
  }

  /**
   *  @brief Returns all polygons of this which are not completly outside polygons from the other region
   *
   *  This method is an out-of-place version of select_not_outside.
   *
   *  Merged semantics applies.
   */
  Region selected_not_outside (const Region &other) const
  {
    return selected_interacting_generic (other, 1, false, true);
  }

  /**
   *  @brief Selects all polygons of this region which are completly inside polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_inside (const Region &other)
  {
    select_interacting_generic (other, -1, false, false);
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which are not completly inside polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_not_inside (const Region &other)
  {
    select_interacting_generic (other, -1, false, true);
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which are completly inside polygons from the other region
   *
   *  This method is an out-of-place version of select_inside.
   *
   *  Merged semantics applies.
   */
  Region selected_inside (const Region &other) const
  {
    return selected_interacting_generic (other, -1, false, false);
  }

  /**
   *  @brief Returns all polygons of this which are not completly inside polygons from the other region
   *
   *  This method is an out-of-place version of select_not_inside.
   *
   *  Merged semantics applies.
   */
  Region selected_not_inside (const Region &other) const
  {
    return selected_interacting_generic (other, -1, false, true);
  }

  /**
   *  @brief Selects all polygons of this region which overlap or touch polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_interacting (const Region &other)
  {
    select_interacting_generic (other, 0, true, false);
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which do not overlap or touch polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_not_interacting (const Region &other)
  {
    select_interacting_generic (other, 0, true, true);
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which overlap or touch polygons from the other region
   *
   *  This method is an out-of-place version of select_interacting.
   *
   *  Merged semantics applies.
   */
  Region selected_interacting (const Region &other) const
  {
    return selected_interacting_generic (other, 0, true, false);
  }

  /**
   *  @brief Returns all polygons of this which do not overlap or touch polygons from the other region
   *
   *  This method is an out-of-place version of select_not_interacting.
   *
   *  Merged semantics applies.
   */
  Region selected_not_interacting (const Region &other) const
  {
    return selected_interacting_generic (other, 0, true, true);
  }

  /**
   *  @brief Selects all polygons of this region which overlap polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_overlapping (const Region &other)
  {
    select_interacting_generic (other, 0, false, false);
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which do not overlap polygons from the other region
   *
   *  This method does not merge the polygons before using them. If whole connected
   *  regions shall be selected, merge the region first.
   *
   *  Merged semantics applies.
   */
  Region &select_not_overlapping (const Region &other)
  {
    select_interacting_generic (other, 0, false, true);
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which overlap polygons from the other region
   *
   *  This method is an out-of-place version of select_overlapping.
   *
   *  Merged semantics applies.
   */
  Region selected_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, false);
  }

  /**
   *  @brief Returns all polygons of this which do not overlap polygons from the other region
   *
   *  This method is an out-of-place version of select_not_overlapping.
   *
   *  Merged semantics applies.
   */
  Region selected_not_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, true);
  }

  /**
   *  @brief Returns the holes 
   *
   *  This method returns the holes of the polygons. 
   *
   *  Merged semantics applies.
   */
  Region holes () const;

  /**
   *  @brief Returns the hulls 
   *
   *  This method returns the hulls of the polygons. It does not merge the
   *  polygons before the hulls are derived. 
   *
   *  Merged semantics applies.
   */
  Region hulls () const;

  /**
   *  @brief Returns all polygons which are in the other region 
   *
   *  This method will return all polygons which are part of another region. 
   *  The match is done exactly.
   *  The "invert" flag can be used to invert the sense, i.e. with 
   *  "invert" set to true, this method will return all polygons not
   *  in the other region.
   *
   *  Merged semantics applies.
   */
  Region in (const Region &other, bool invert = false) const;

  /**
   *  @brief Round corners (in-place)
   *
   *  @param rinner The inner radius in DBU units
   *  @param router The outer radius in DBU units
   *  @param n The number of points to use per circle
   */
  void round_corners (double rinner, double router, unsigned int n)
  {
    *this = rounded_corners (rinner, router, n);
  }

  /**
   *  @brief Returns a new region with rounded corners (out of place)
   */
  Region rounded_corners (double rinner, double router, unsigned int n) const;

  /**
   *  @brief Returns the nth polygon 
   *
   *  This method will force the polygons to be inside the polygon vector.
   *  If that happens, the method may be costly and will invalidate any iterator. 
   *  The iterator should be used whenever possible.
   */
  const db::Polygon *nth (size_t n) const
  {
    ensure_valid_polygons ();
    return n < m_polygons.size () ? &m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin () [n] : 0;
  }

  /**
   *  @brief Returns true, if the region has valid polygons stored within itself
   */
  bool has_valid_polygons () const
  {
    //  Note: we take a copy of the iterator since the at_end method may
    //  validate the iterator which will make it refer to a specifc configuration.
    return db::RecursiveShapeIterator (m_iter).at_end ();
  }

  /**
   *  @brief Ensures the region has valid polygons
   *
   *  This method is const since it has const semantics.
   */
  void ensure_valid_polygons () const;

  /**
   *  @brief Ensures the region has valid merged polygons
   *
   *  It will make sure that begin_merged will deliver an 
   *  iterator to a polygon with a unique memory location.
   */
  void ensure_valid_merged_polygons () const;

  /**
   *  @brief Equality
   */
  bool operator== (const db::Region &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const db::Region &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const db::Region &other) const;

private:
  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator polygon_iterator_type;

  bool m_is_merged;
  bool m_merged_semantics;
  bool m_strict_handling;
  bool m_merge_min_coherence;
  mutable db::Shapes m_polygons;
  mutable db::Shapes m_merged_polygons;
  mutable db::Box m_bbox;
  mutable bool m_bbox_valid;
  mutable bool m_merged_polygons_valid;
  mutable db::RecursiveShapeIterator m_iter;
  db::ICplxTrans m_iter_trans;
  bool m_report_progress;
  std::string m_progress_desc;

  void init ();
  void invalidate_cache ();
  void set_valid_polygons ();
  void ensure_bbox_valid () const;
  void ensure_merged_polygons_valid () const;
  EdgePairs run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  EdgePairs run_single_polygon_check (db::edge_relation_type rel, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  void select_interacting_generic (const Region &other, int mode, bool touching, bool inverse);
  Region selected_interacting_generic (const Region &other, int mode, bool touching, bool inverse) const;
};

/**
 *  @brief A polygon receiver putting the polygons into a Region object
 *
 *  This class implements the PolygonSink interface for the edge processor.
 *  Like EdgeContainer, this receiver collects the objects into an existing Region object.
 */
class DB_PUBLIC RegionPolygonSink
  : public db::PolygonSink
{
public:
  /**
   *  @brief Constructor specifying the region where to store the polygons
   *
   *  If "clear" is set to true, the region will be cleared before the 
   *  inserting of polygons starts. This allows to use the region as input and
   *  output for any operation.
   */
  RegionPolygonSink (Region &region, bool clear = false) 
    : PolygonSink (), mp_region (&region), m_clear (clear)
  { }

  /**
   *  @brief Start the sequence
   */
  virtual void start ()
  {
    if (m_clear) {
      mp_region->clear ();
    }
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon)
  {
    mp_region->insert (polygon);
  }

private:
  Region *mp_region;
  bool m_clear;
};

} // namespace db

namespace tl 
{
  /**
   *  @brief The type traits for the region type
   */
  template <>
  struct type_traits <db::Region> : public type_traits<void> 
  {
    typedef true_tag supports_extractor;
    typedef true_tag supports_to_string;
    typedef true_tag has_less_operator;
    typedef true_tag has_equal_operator;
  };

}

#endif

