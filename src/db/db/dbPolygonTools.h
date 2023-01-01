
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


#ifndef HDR_dbPolygonTools
#define HDR_dbPolygonTools

#include "dbCommon.h"

#include "dbPolygon.h"
#include "dbText.h"

#include <vector>
#include <limits>
#include <algorithm>

namespace db {

class SimplePolygonSink;

/**
 *  @brief An inside test operator
 *
 *  This class allows an efficient test whether multiple points are inside a given polygon.
 *  Since the test is efficiently implemented when the polygon edges are sorted, the sorting 
 *  and memory allocation step is performed once in the test operator's constructor while 
 *  each individual test is performed efficiently.
 */

template <class P>
class inside_poly_test 
{
public:
  typedef typename P::point_type point_type;
  typedef typename P::coord_type coord_type;
  typedef typename db::edge<coord_type> edge_type;

  /**
   *  @brief Constructor
   */
  inside_poly_test (const P &polygon);

  /**
   *  @brief Actual test
   *
   *  This function returns 1, if the point is inside (not on)
   *  the polygon. It returns 0, if the point is on the polygon and -1
   *  if outside. 
   */
  int operator() (const point_type &pt) const;

private:
  std::vector<edge_type> m_edges;
};

//  Some helper classes and functions for implementing cut_polygon

template <class Polygon>
class DB_PUBLIC cut_polygon_receiver_base
{
public:
  virtual ~cut_polygon_receiver_base () { }
  virtual void put (const Polygon &) = 0;
};

template <class OutputIter, class Polygon>
class cut_polygon_receiver
  : public cut_polygon_receiver_base<Polygon>
{
public:
  cut_polygon_receiver (const OutputIter &iter)
    : m_iter (iter)
  { }

  virtual void put (const Polygon &polygon)
  {
    *m_iter++ = polygon;
  }

private:
  OutputIter m_iter;
};

template <class PolygonType, class Edge>
void DB_PUBLIC cut_polygon_internal (const PolygonType &input, const Edge &line, cut_polygon_receiver_base<PolygonType> *right_of_line);

/**
 *  @brief Polygon cut function
 *
 *  This functions cuts a polygon at the given line (given by an edge)
 *  and produces all parts of the polygon that are "right" of the line given by "line".
 */
template <class PolygonType, class OutputIter>
void cut_polygon (const PolygonType &input, const typename PolygonType::edge_type &line, OutputIter right_of_line)
{
  cut_polygon_receiver<OutputIter, PolygonType> output (right_of_line);
  cut_polygon_internal (input, line, &output);
}

/**
 *  @brief Split a polygon into two or more parts 
 *
 *  This function splits a polygon into parts using some heuristics to determine a "suitable" cut line.
 *  The cut line is chosen through a vertex close to a center (either horizontal or vertical). The splitting
 *  is supposed to create smaller parts with less vertices or a better area ratio of polygon to bounding box area.
 *
 *  @param polygon The input polygon
 *  @param output The parts 
 */
template <class PolygonType>
void DB_PUBLIC split_polygon (const PolygonType &polygon, std::vector<PolygonType> &output);

/**
 *  @brief Determines whether a polygon and a box interact
 *
 *  This function determines whether the polygon and the box share at least on common point
 *  and returns true in this case.
 */
template<class Polygon, class Box>
bool interact_pb (const Polygon &poly, const Box &box)
{
  if (! poly.box ().touches (box)) {
    return false;
  }

  if (poly.begin_hull () == poly.end_hull ()) {
    return false;
  }

  //  if the box center is inside or at the rim of the polygon, return true
  if (db::inside_poly (poly.begin_edge (), box.center ()) >= 0 ||
      box.contains (*poly.begin_hull ())) {
    return true;
  }

  for (typename Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
    if ((*e).clipped (box).first) {
      return true;
    }
  }

  return false;
}

/**
 *  @brief Determines whether two polygons share at least one common point.
 */
template <class Polygon1, class Polygon2>
bool interact_pp (const Polygon1 &poly1, const Polygon2 &poly2)
{
  typedef typename Polygon1::coord_type coord_type;
  typedef typename Polygon1::polygon_edge_iterator edge_iterator1;
  typedef typename Polygon2::polygon_edge_iterator edge_iterator2;
  typedef db::edge<coord_type> edge_type;

  if (! poly1.box ().touches (poly2.box ())) {
    return false;
  }

  if (poly1.begin_hull () == poly1.end_hull () ||
      poly2.begin_hull () == poly2.end_hull ()) {
    return false;
  }

  //  if at least one point of poly2 is inside or at the rim of poly1, return true
  if (db::inside_poly (poly1.begin_edge (), *poly2.begin_hull ()) >= 0 ||
      db::inside_poly (poly2.begin_edge (), *poly1.begin_hull ()) >= 0) {
    return true;
  }

  //  in all other cases, in intersection happens if at least one of the edges of poly1 or
  //  poly2 intersect. This is checked with a simple scanline algorithm ...

  std::vector <edge_type> edges1;
  edges1.reserve (poly1.vertices ());
  for (edge_iterator1 e = poly1.begin_edge (); ! e.at_end (); ++e) {
    edges1.push_back (*e);
  }

  std::sort (edges1.begin (), edges1.end (), edge_ymin_compare<coord_type> ());

  std::vector <edge_type> edges2;
  edges2.reserve (poly2.vertices ());
  for (edge_iterator2 e = poly2.begin_edge (); ! e.at_end (); ++e) {
    edges2.push_back (*e);
  }

  std::sort (edges2.begin (), edges2.end (), edge_ymin_compare<coord_type> ());

  coord_type y = std::min (std::min (edges1.front ().y1 (), edges1.front ().y2 ()),
                           std::min (edges2.front ().y1 (), edges2.front ().y2 ()));
  
  typename std::vector <edge_type>::iterator ec1 = edges1.begin (); 
  typename std::vector <edge_type>::iterator ef1 = edges1.begin (); 
  typename std::vector <edge_type>::iterator ec2 = edges2.begin (); 
  typename std::vector <edge_type>::iterator ef2 = edges2.begin (); 

  while (ec1 != edges1.end () && ec2 != edges2.end ()) {

    while (ef1 != edges1.end () && edge_ymin (*ef1) <= y) {
      ++ef1;
    }

    while (ef2 != edges2.end () && edge_ymin (*ef2) <= y) {
      ++ef2;
    }

    coord_type yy = std::numeric_limits <coord_type>::max ();

    if (ef1 != edges1.end ()) {
      yy = edge_ymin (*ef1);
    }

    if (ef2 != edges2.end ()) {
      coord_type ynext = edge_ymin (*ef2);
      if (ynext < yy) {
        yy = ynext;
      }
    }

    std::sort (ec1, ef1, edge_xmin_at_yinterval_compare<coord_type> (y, yy));
    std::sort (ec2, ef2, edge_xmin_at_yinterval_compare<coord_type> (y, yy));

    typename std::vector <edge_type>::iterator c1 = ec1;
    typename std::vector <edge_type>::iterator f1 = ec1;
    typename std::vector <edge_type>::iterator c2 = ec2;
    typename std::vector <edge_type>::iterator f2 = ec2;

    coord_type x1 = edge_xmin_at_yinterval (*ec1, y, yy);
    coord_type x2 = edge_xmin_at_yinterval (*ec2, y, yy);
    coord_type x = std::min (x1, x2);

    while (c1 != ef1 && c2 != ef2) {

      while (f1 != ef1 && edge_xmin_at_yinterval (*f1, y, yy) <= x) {
        ++f1;
      }

      while (f2 != ef2 && edge_xmin_at_yinterval (*f2, y, yy) <= x) {
        ++f2;
      }

      coord_type xx = std::numeric_limits <coord_type>::max ();

      if (f1 != ef1) {
        xx = edge_xmin_at_yinterval (*f1, y, yy);
      }

      if (f2 != ef2) {
        coord_type xnext = edge_xmin_at_yinterval (*f2, y, yy);
        if (xnext < xx) {
          xx = xnext;
        }
      }

      for (typename std::vector <edge_type>::iterator a = c1; a != f1; ++a) {
        for (typename std::vector <edge_type>::iterator b = c2; b != f2; ++b) {
          if (a->intersect (*b)) {
            return true;
          }
        }
      }

      x = xx;

      for (typename std::vector <edge_type>::iterator cc = c1; cc != f1; ++cc) {
        if (edge_xmax (*cc) < x || edge_xmax_at_yinterval (*cc, y, yy) < x) {
          if (c1 != cc) {
            std::swap (*cc, *c1);
          }
          ++c1;
        }
      }

      for (typename std::vector <edge_type>::iterator cc = c2; cc != f2; ++cc) {
        if (edge_xmax (*cc) < x || edge_xmax_at_yinterval (*cc, y, yy) < x) {
          if (c2 != cc) {
            std::swap (*cc, *c2);
          }
          ++c2;
        }
      }

    }

    y = yy;

    for (typename std::vector <edge_type>::iterator cc = ec1; cc != ef1; ++cc) {
      if (edge_ymax (*cc) < y) {
        if (ec1 != cc) {
          std::swap (*cc, *ec1);
        }
        ++ec1;
      }
    }

    for (typename std::vector <edge_type>::iterator cc = ec2; cc != ef2; ++cc) {
      if (edge_ymax (*cc) < y) {
        if (ec2 != cc) {
          std::swap (*cc, *ec2);
        }
        ++ec2;
      }
    }

  }

  return false;
}

/**
 *  @brief Determines whether a polygon and an edge share at least one common point.
 */
template <class Polygon, class Edge>
bool interact_pe (const Polygon &poly, const Edge &edge)
{
  //  A polygon and an edge interact if the edge is either inside completely
  //  of at least one edge of the polygon intersects with the edge
  if (poly.box ().contains (edge.p1 ()) && db::inside_poly (poly.begin_edge (), edge.p1 ()) >= 0) {
    return true;
  } else {
    for (typename Polygon::polygon_edge_iterator pe = poly.begin_edge (); ! pe.at_end (); ++pe) {
      if ((*pe).intersect (edge)) {
        return true;
      }
    }
  }

  return false;
}

/**
 *  @brief Determines whether the text is inside the polygon
 */
template <class Polygon, class Text>
bool interact_pt (const Polygon &poly, const Text &text)
{
  typedef typename Text::point_type point_type;
  point_type p;
  p += text.trans ().disp ();
  return (poly.box ().contains (p) && db::inside_poly (poly.begin_edge (), p) >= 0);
}

//  Some specializations that map all combinations to template versions
inline bool interact (const db::Box &box1,              const db::Box &box2)                { return box1.touches (box2); }
inline bool interact (const db::DBox &box1,             const db::DBox &box2)               { return box1.touches (box2); }
inline bool interact (const db::Polygon &poly,          const db::Box &box)                 { return interact_pb (poly, box); }
inline bool interact (const db::SimplePolygon &poly,    const db::Box &box)                 { return interact_pb (poly, box); }
inline bool interact (const db::DPolygon &poly,         const db::DBox &box)                { return interact_pb (poly, box); }
inline bool interact (const db::DSimplePolygon &poly,   const db::DBox &box)                { return interact_pb (poly, box); }
inline bool interact (const db::Polygon &poly,          const db::Edge &edge)               { return interact_pe (poly, edge); }
inline bool interact (const db::SimplePolygon &poly,    const db::Edge &edge)               { return interact_pe (poly, edge); }
inline bool interact (const db::DPolygon &poly,         const db::DEdge &edge)              { return interact_pe (poly, edge); }
inline bool interact (const db::DSimplePolygon &poly,   const db::DEdge &edge)              { return interact_pe (poly, edge); }
inline bool interact (const db::Polygon &poly1,         const db::Polygon &poly2)           { return interact_pp (poly1, poly2); }
inline bool interact (const db::SimplePolygon &poly1,   const db::Polygon &poly2)           { return interact_pp (poly1, poly2); }
inline bool interact (const db::Polygon &poly1,         const db::SimplePolygon &poly2)     { return interact_pp (poly1, poly2); }
inline bool interact (const db::SimplePolygon &poly1,   const db::SimplePolygon &poly2)     { return interact_pp (poly1, poly2); }
inline bool interact (const db::DPolygon &poly1,        const db::DPolygon &poly2)          { return interact_pp (poly1, poly2); }
inline bool interact (const db::DSimplePolygon &poly1,  const db::DPolygon &poly2)          { return interact_pp (poly1, poly2); }
inline bool interact (const db::DPolygon &poly1,        const db::DSimplePolygon &poly2)    { return interact_pp (poly1, poly2); }
inline bool interact (const db::DSimplePolygon &poly1,  const db::DSimplePolygon &poly2)    { return interact_pp (poly1, poly2); }
inline bool interact (const db::Polygon &poly,          const db::Text &text)               { return interact_pt (poly, text); }
inline bool interact (const db::SimplePolygon &poly,    const db::Text &text)               { return interact_pt (poly, text); }
inline bool interact (const db::DPolygon &poly,         const db::DText &text)              { return interact_pt (poly, text); }
inline bool interact (const db::DSimplePolygon &poly,   const db::DText &text)              { return interact_pt (poly, text); }

/**
 *  @brief Extract a corner radius from a contour
 *
 *  This method will determine the radius of a contour if the contour was formed by rounding another contour.
 *  The corners must be formed by soft bending edges. 
 *  It is possible to retrieve the original contour (or an approximation of the latter) by passing a vector
 *  in "new_pts" which will receive the original contour.
 *
 *  @param from, to The iterators describing the contour
 *  @param rinner The inner corner radius (in dbu units) extracted (if return value is true)
 *  @param router The outer corner radius (in dbu units) extracted (if return value is true)
 *  @param n Receives the number of points per full circle (if return value is true)
 *  @param new_pts If != 0, this vector will receive the contour without the rounded corners (if return value is true)
 *  @param fallback Fallback algorithm (less strict) if true 
 *  @return True, if the extraction was successful
 */
bool DB_PUBLIC extract_rad_from_contour (db::Polygon::polygon_contour_iterator from, db::Polygon::polygon_contour_iterator to, double &rinner, double &router, unsigned int &n, std::vector <db::Point> *new_pts = 0, bool fallback = false);

/**
 *  @brief Extract a corner radius from a contour (version for double coordinates)
 */
bool DB_PUBLIC extract_rad_from_contour (db::DPolygon::polygon_contour_iterator from, db::DPolygon::polygon_contour_iterator to, double &rinner, double &router, unsigned int &n, std::vector <db::DPoint> *new_pts = 0, bool fallback = false);

/**
 *  @brief Extract the radius (better: radii) from a polygon and if requested, compute the new polygon without the rounding
 *
 *  See extract_rad_from_contour for details.
 */
bool DB_PUBLIC extract_rad (const db::Polygon &polygon, double &rinner, double &router, unsigned int &n, db::Polygon *new_polygon = 0);

/**
 *  @brief Extract a corner radius from a polygon (version for double coordinates)
 */
bool DB_PUBLIC extract_rad (const db::DPolygon &polygon, double &rinner, double &router, unsigned int &n, db::DPolygon *new_polygon = 0);

/**
 *  @brief Compute the rounded version of a polygon contour
 *
 *  Computes the version of a contour with the corners rounded (inner corners with rinner, outer corners with router, n points per full circle=.
 *  
 *  @param from, to The iterators describing the contour
 *  @param new_pts Receives the new points
 *  @param rinner The inner corner radius (in dbu units)
 *  @param router The outer corner radius (in dbu units)
 *  @param n The number of points per full circle
 */
void DB_PUBLIC compute_rounded_contour (db::Polygon::polygon_contour_iterator from, db::Polygon::polygon_contour_iterator to, std::vector <db::Point> &new_pts, double rinner, double router, unsigned int n);

/**
 *  @brief Compute the rounded version of a polygon contour (double coordinate version)
 */
void DB_PUBLIC compute_rounded_contour (db::DPolygon::polygon_contour_iterator from, db::DPolygon::polygon_contour_iterator to, std::vector <db::DPoint> &new_pts, double rinner, double router, unsigned int n);

/**
 *  @brief Compute the rounded version of the polygon
 *
 *  See compute_rounded_contour for details.
 */
db::Polygon DB_PUBLIC compute_rounded (const db::Polygon &poly, double rinner, double router, unsigned int n);

/**
 *  @brief Compute the rounded version of the polygon (double coordinate version)
 */
db::DPolygon DB_PUBLIC compute_rounded (const db::DPolygon &poly, double rinner, double router, unsigned int n);

#define KLAYOUT_SMOOTH_HAS_KEEP_HV 1

/**
 *  @brief Smooth a contour 
 *
 *  Removes vertexes from a contour which deviate from the "average" line by more than "d".
 *
 *  @param from The start of the contour
 *  @param to The end of the contour
 *  @param new_pts The points that make up the new contour
 *  @param d The distance that determines the smoothing "roughness"
 *  @param keep_hv If true, vertical and horizontal edges are maintained
 */
void DB_PUBLIC smooth_contour (db::Polygon::polygon_contour_iterator from, db::Polygon::polygon_contour_iterator to, std::vector <db::Point> &new_pts, db::Coord d, bool keep_hv);

/**
 *  @brief Smooth a polygon (apply smoothing to the whole polygon)
 */
db::Polygon DB_PUBLIC smooth (const db::Polygon &poly, db::Coord d, bool keep_hv);

/**
 *  @brief Returns a value indicating whether the polygon is an "strange polygon"
 *  "strange polygons" are ones which are non-orientable or have self-overlaps, e.g. their wrap
 *  count after orientation normalization is not 0 or 1.
 *  If "error_parts" is given it will receive markers indicating the parts which violate
 *  this wrap count condition.
 */
bool DB_PUBLIC is_strange_polygon (const db::Polygon &poly, std::vector<db::Polygon> *error_parts = 0);

/**
 *  @brief Returns a value indicating whether the polygon is "non-orientable"
 *  Such polygons contain loops which cannot be oriented, e.g. "8"-type loops.
 *  If "error_parts" is given it will receive markers indicating the parts which are
 *  non-orientable.
 */
bool DB_PUBLIC is_non_orientable_polygon (const db::Polygon &poly, std::vector<db::Polygon> *error_parts = 0);

/**
 *  @brief A area collector
 *
 *  This class provides a generic 2d map of area values. 
 *  It is used for example by the rasterize function to collect area values 
 *  on a per-pixel basis.
 */
class DB_PUBLIC AreaMap
{
public:
  typedef db::coord_traits<db::Coord>::area_type area_type;

  /**
   *  @brief Constructor
   */
  AreaMap ();

  /**
   *  @brief Copy constructor
   */
  AreaMap (const AreaMap &);

  /**
   *  @brief Constructor
   */
  AreaMap (const db::Point &p0, const db::Vector &d, size_t nx, size_t ny);

  /**
   *  @brief Constructor with pixel size
   */
  AreaMap (const db::Point &p0, const db::Vector &d, const db::Vector &p, size_t nx, size_t ny);

  /**
   *  @brief Destructor
   */
  ~AreaMap ();

  /**
   *  @brief Assignment
   */
  AreaMap &operator= (const AreaMap &);

  /**
   *  @brief Reinitialize
   */
  void reinitialize (const db::Point &p0, const db::Vector &d, size_t nx, size_t ny);

  /**
   *  @brief Reinitialize with pixel size
   */
  void reinitialize (const db::Point &p0, const db::Vector &d, const db::Vector &p, size_t nx, size_t ny);

  /**
   *  @brief Swap of two maps
   */
  void swap (AreaMap &other);

  /**
   *  @brief Get the area of one pixel
   */
  area_type &get (size_t x, size_t y)
  {
    return mp_av [y * m_nx + x];
  }

  /**
   *  @brief Get the area of one pixel (const version)
   */
  const area_type &get (size_t x, size_t y) const
  {
    return mp_av [y * m_nx + x];
  }

  /**
   *  @brief The number of pixels in x-dimension
   */
  size_t nx () const 
  {
    return m_nx;
  }

  /**
   *  @brief The number of pixels in y-dimension
   */
  size_t ny () const
  {
    return m_ny;
  }

  /**
   *  @brief The origin
   */
  const db::Point &p0 () const
  {
    return m_p0;
  }

  /**
   *  @brief Move the origin
   */
  void move (const db::Vector &d)
  {
    m_p0 += d;
  }

  /**
   *  @brief The per-pixel displacement vector (pixel size)
   */
  const db::Vector &d () const
  {
    return m_d;
  }

  /**
   *  @brief The pixel size (must be less than d)
   */
  const db::Vector &p () const
  {
    return m_p;
  }

  /**
   *  @brief Compute the bounding box of the area map
   */
  db::Box bbox () const;

  /**
   *  @brief Compute the total area
   */
  area_type total_area () const;

  /**
   *  @brief Compute the maximum (single-covered) area per pixel
   */
  area_type pixel_area () const
  {
    return area_type (m_p.x ()) * area_type (m_p.y ());
  }

  /**
   *  @brief Clear the values
   */
  void clear ();

private:
  area_type *mp_av;
  db::Point m_p0;
  db::Vector m_d;
  db::Vector m_p;
  size_t m_nx, m_ny;
};

/**
 *  @brief Rasterize the polygon into the given area map
 *
 *  This will decompose the polygon and produce per-pixel area values for the given 
 *  polygon. The area contributions will be added to the given area map.
 *
 *  Returns a value indicating whether the map will be non-empty.
 */
bool DB_PUBLIC rasterize (const db::Polygon &polygon, db::AreaMap &am);

/**
 *  @brief Minkowski sum of an edge and a polygon
 */
db::Polygon DB_PUBLIC minkowski_sum (const db::Polygon &a, const db::Edge &b, bool resolve_holes = false);

/**
 *  @brief Minkowski sum of a polygon and a polygon
 */
db::Polygon DB_PUBLIC minkowski_sum (const db::Polygon &a, const db::Polygon &b, bool resolve_holes = false);

/**
 *  @brief Minkowski sum of a polygon and a box
 */
db::Polygon DB_PUBLIC minkowski_sum (const db::Polygon &a, const db::Box &b, bool resolve_holes = false);

/**
 *  @brief Minkowski sum of a polygon and a contour
 */
db::Polygon DB_PUBLIC minkowski_sum (const db::Polygon &a, const std::vector<db::Point> &c, bool resolve_holes = false);

/**
 *  @brief Resolve holes 
 */
db::Polygon DB_PUBLIC resolve_holes (const db::Polygon &p);

/**
 *  @brief SimplePolygon to Polygon conversion
 */
db::Polygon DB_PUBLIC simple_polygon_to_polygon (const db::SimplePolygon &a);

/**
 *  @brief Polygon to SimplePolygon conversion (resolves holes)
 */
db::SimplePolygon DB_PUBLIC polygon_to_simple_polygon (const db::Polygon &a);

/**
 *  @brief The decomposition mode for decompose_convex
 *  This mode controls how the polygon is being cut to take off parts.
 *  "PO_any" will deliver a "best" cut. "PO_horizontal" will only apply
 *  horizontal cuts, "PO_vertical" only vertical ones. "PO_htrapezoids" will
 *  apply horizontal cuts to favor horizontal trapezoids. "PO_vtrapezoids"
 *  will favor vertical trapezoids.
 */
enum PreferredOrientation
{
  PO_any = 0,
  PO_horizontal = 1,
  PO_vertical = 2,
  PO_htrapezoids = 3,
  PO_vtrapezoids = 4
};

/**
 *  @brief The decomposition mode for decompose_trapezoids
 *  This mode controls the trapezoid decomposition.
 *  "TD_simple" is a simple and fast mode, "TD_htrapezoids" is a mode favoring
 *  horizontal trapezoids. It's slower but will deliver less trapezoids in some
 *  cases. "TD_vtrapezoids" is similar for "TD_htrapezoids" and will produce
 *  vertical trapezoids where the vertical edges are parallel.
 */
enum TrapezoidDecompositionMode
{
  TD_simple = 0,
  TD_htrapezoids = 1,
  TD_vtrapezoids = 2
};

/**
 *  @brief Decompose a polygon into convex (simple) polygons
 *
 *  Returns a set of convex polygon whose sum represents the original polygon.
 *  If the original polygon was convex already, it will not be modified.
 *
 *  The resulting polygons will be sent to the sink. Only "put" events will be generated
 *  to facilitate call chaining of multiple "decompose_convex" calls.
 */
void DB_PUBLIC decompose_convex (const db::Polygon &p, PreferredOrientation po, SimplePolygonSink &sink);

/**
 *  @brief Decompose a simple polygon into convex (simple) polygons
 *
 *  See the "Polygon" version of this function for details.
 */
void DB_PUBLIC decompose_convex (const db::SimplePolygon &p, PreferredOrientation po, SimplePolygonSink &sink);

/**
 *  @brief Returns true, if the given polygon is a convex one
 */
bool DB_PUBLIC is_convex (const db::Polygon &poly);

/**
 *  @brief Returns true, if the given simple polygon is a convex one
 */
bool DB_PUBLIC is_convex (const db::SimplePolygon &poly);

/**
 *  @brief Decomposes the given polygon into trapezoids
 *
 *  @param horizontal If true, delivers htrapezoid objects, otherwise vtrapezoids
 *
 *  The resulting single polygons will be sent to the sink. Only "put" events will be
 *  generated on the sink.
 */
void DB_PUBLIC decompose_trapezoids (const db::Polygon &p, TrapezoidDecompositionMode mode, SimplePolygonSink &sink);

/**
 *  @brief Decomposes the given simple polygon into trapezoids
 *
 *  See the "Polygon" version of this function for details.
 */
void DB_PUBLIC decompose_trapezoids (const db::SimplePolygon &p, TrapezoidDecompositionMode mode, SimplePolygonSink &sink);

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

}

#endif

