
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#ifndef HDR_dbRegionProcessors
#define HDR_dbRegionProcessors

#include "dbCommon.h"
#include "dbRegionDelegate.h"
#include "dbPolygonTools.h"
#include "dbEdgesUtils.h"
#include "dbPLCTriangulation.h"
#include "dbEdgePairRelations.h"

namespace db
{

// -----------------------------------------------------------------------------------
//  Corner detection

/**
 *  @brief An interface to accept corners
 */
class DB_PUBLIC CornerPointDelivery
{
public:
  virtual void make_point (const db::Point &pt, const db::Edge &e1, const db::Edge &e2) const = 0;
  virtual void make_point (const db::Point &pt, const db::Edge &e1, const db::Edge &e2, db::properties_id_type prop_id) const = 0;
};

/**
 *  @brief An interface to accept corners and turns them into rectangles with 2*dim x 2*dim
 */
class DB_PUBLIC CornerRectDelivery
  : public CornerPointDelivery
{
public:
  CornerRectDelivery (db::Coord dim, std::vector<db::Polygon> &result)
    : m_d (dim, dim), mp_result (&result), mp_result_wp (0)
  { }

  CornerRectDelivery (db::Coord dim, std::vector<db::PolygonWithProperties> &result_wp)
    : m_d (dim, dim), mp_result (0), mp_result_wp (&result_wp)
  { }

  virtual void make_point (const db::Point &pt, const db::Edge &, const db::Edge &) const
  {
    if (mp_result) {
      mp_result->push_back (db::Polygon (db::Box (pt - m_d, pt + m_d)));
    }
  }

  virtual void make_point (const db::Point &pt, const db::Edge &, const db::Edge &, db::properties_id_type prop_id) const
  {
    if (mp_result_wp) {
      mp_result_wp->push_back (db::PolygonWithProperties (db::Polygon (db::Box (pt - m_d, pt + m_d)), prop_id));
    }
  }

private:
  db::Vector m_d;
  std::vector<db::Polygon> *mp_result;
  std::vector<db::PolygonWithProperties> *mp_result_wp;
};

/**
 *  @brief An interface to accept corners and turns them into degenerated edges (dots)
 */
class DB_PUBLIC CornerDotDelivery
  : public CornerPointDelivery
{
public:
  CornerDotDelivery (std::vector<db::Edge> &result)
    : mp_result (&result), mp_result_wp (0)
  { }

  CornerDotDelivery (std::vector<db::EdgeWithProperties> &result)
    : mp_result (0), mp_result_wp (&result)
  { }

  virtual void make_point (const db::Point &pt, const db::Edge &, const db::Edge &) const
  {
    if (mp_result) {
      mp_result->push_back (db::Edge (pt, pt));
    }
  }

  virtual void make_point (const db::Point &pt, const db::Edge &, const db::Edge &, db::properties_id_type prop_id) const
  {
    if (mp_result_wp) {
      mp_result_wp->push_back (db::EdgeWithProperties (db::Edge (pt, pt), prop_id));
    }
  }

private:
  db::Vector m_d;
  std::vector<db::Edge> *mp_result;
  std::vector<db::EdgeWithProperties> *mp_result_wp;
};

/**
 *  @brief An interface to accept corners and turns them into edge pairs
 */
class DB_PUBLIC CornerEdgePairDelivery
  : public CornerPointDelivery
{
public:
  CornerEdgePairDelivery (std::vector<db::EdgePair> &result)
    : mp_result (&result), mp_result_wp (0)
  { }

  CornerEdgePairDelivery (std::vector<db::EdgePairWithProperties> &result)
    : mp_result (0), mp_result_wp (&result)
  { }

  virtual void make_point (const db::Point &, const db::Edge &e1, const db::Edge &e2) const
  {
    if (mp_result) {
      mp_result->push_back (db::EdgePair (e1, e2));
    }
  }

  virtual void make_point (const db::Point &, const db::Edge &e1, const db::Edge &e2, db::properties_id_type prop_id) const
  {
    if (mp_result_wp) {
      mp_result_wp->push_back (db::EdgePairWithProperties (db::EdgePair (e1, e2), prop_id));
    }
  }

private:
  std::vector<db::EdgePair> *mp_result;
  std::vector<db::EdgePairWithProperties> *mp_result_wp;
};

/**
 *  @brief A helper class implementing the core corner detection algorithm
 */
class DB_PUBLIC CornerDetectorCore
{
public:
  CornerDetectorCore (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end, bool inverse, bool absolute);
  virtual ~CornerDetectorCore () { }

  void detect_corners (const db::Polygon &poly, const CornerPointDelivery &delivery) const;
  void detect_corners (const db::PolygonWithProperties &poly, const CornerPointDelivery &delivery) const;

private:
  db::EdgeAngleChecker m_checker;
};

/**
 *  @brief A corner detector delivering small retangles (2*dim x 2*dim) per detected corner
 */
class DB_PUBLIC CornersAsRectangles
  : public db::PolygonProcessorBase, private CornerDetectorCore
{
public:
  CornersAsRectangles (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end, bool inverse, bool absolute, db::Coord dim = 1)
    : CornerDetectorCore (angle_start, include_angle_start, angle_end, include_angle_end, inverse, absolute), m_dim (dim)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
  {
    detect_corners (poly, CornerRectDelivery (m_dim, result));
  }

  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const
  {
    detect_corners (poly, CornerRectDelivery (m_dim, result));
  }

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; } //  overlaps may happen
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }

private:
  db::Coord m_dim;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief A corner detector delivering degenerated edges (dots) for the corners
 */
class DB_PUBLIC CornersAsDots
  : public db::PolygonToEdgeProcessorBase, private CornerDetectorCore
{
public:
  CornersAsDots (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end, bool inverse, bool absolute)
    : CornerDetectorCore (angle_start, include_angle_start, angle_end, include_angle_end, inverse, absolute)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Edge> &result) const
  {
    detect_corners (poly, CornerDotDelivery (result));
  }

  void process (const db::PolygonWithProperties &poly, std::vector<db::EdgeWithProperties> &result) const
  {
    detect_corners (poly, CornerDotDelivery (result));
  }

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  to preserve dots
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }
};

/**
 *  @brief A corner detector delivering edge pairs for the corners
 */
class DB_PUBLIC CornersAsEdgePairs
  : public db::PolygonToEdgePairProcessorBase, private CornerDetectorCore
{
public:
  CornersAsEdgePairs (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end, bool inverse, bool absolute)
    : CornerDetectorCore (angle_start, include_angle_start, angle_end, include_angle_end, inverse, absolute)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::EdgePair> &result) const
  {
    detect_corners (poly, CornerEdgePairDelivery (result));
  }

  void process (const db::PolygonWithProperties &poly, std::vector<db::EdgePairWithProperties> &result) const
  {
    detect_corners (poly, CornerEdgePairDelivery (result));
  }

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  to preserve dots
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }
};

// -----------------------------------------------------------------------------------
//  Extents

/**
 *  @brief A processor delivering the extents (bounding box) of the merged polygons
 */
class DB_PUBLIC Extents
  : public db::PolygonProcessorBase
{
public:
  Extents ()
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }  //  variants are too common, so don't do this
};

/**
 *  @brief A processor delivering the relative extents (bounding box) of the merged polygons
 *  This processor allows over- or undersizing of the resulting box by a given amount and delivery
 *  of a box relative to the original box.
 */
class DB_PUBLIC RelativeExtents
  : public db::PolygonProcessorBase
{
public:
  RelativeExtents (double fx1, double fy1, double fx2, double fy2, db::Coord dx, db::Coord dy)
    : m_fx1 (fx1), m_fy1 (fy1), m_fx2 (fx2), m_fy2 (fy2), m_dx (dx), m_dy (dy)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

  virtual const TransformationReducer *vars () const;
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }  //  variants are too common, so don't do this

private:
  double m_fx1, m_fy1, m_fx2, m_fy2;
  db::Coord m_dx, m_dy;
  db::MagnificationAndOrientationReducer m_anisotropic_reducer;
  db::MagnificationReducer m_isotropic_reducer;
};

/**
 *  @brief A processor delivers one edge per merged polygon
 *  The edge runs from the relative coordinate fx1, fy1 (0: left/bottom, 1: right/top) to
 *  fx2, fy2.
 *  This processor allows over- or undersizing of the resulting box by a given amount and delivery
 *  of a box relative to the original box.
 */
class DB_PUBLIC RelativeExtentsAsEdges
  : public db::PolygonToEdgeProcessorBase
{
public:
  RelativeExtentsAsEdges (double fx1, double fy1, double fx2, double fy2)
    : m_fx1 (fx1), m_fy1 (fy1), m_fx2 (fx2), m_fy2 (fy2)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Edge> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::EdgeWithProperties> &result) const;

  virtual const TransformationReducer *vars () const;
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const;
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }  //  variants are too common, so don't do this

private:
  double m_fx1, m_fy1, m_fx2, m_fy2;
  db::MagnificationAndOrientationReducer m_anisotropic_reducer;
  db::MagnificationReducer m_isotropic_reducer;
};

/**
 *  @brief A processor that delivers all edges for a polygon
 */
class DB_PUBLIC PolygonToEdgeProcessor
  : public db::PolygonToEdgeProcessorBase
{
public:
  enum EdgeMode { All = 0, Convex, Concave, StepIn, StepOut, Step,
                           NotConvex, NotConcave, NotStepIn, NotStepOut, NotStep };

  PolygonToEdgeProcessor (EdgeMode mode = All);

  void process (const db::Polygon &poly, std::vector<db::Edge> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::EdgeWithProperties> &result) const;

private:
  EdgeMode m_mode;
};

/**
 *  @brief A decomposition processor to deliver convex-only polygons
 */
class DB_PUBLIC ConvexDecomposition
  : public db::PolygonProcessorBase
{
public:
  ConvexDecomposition (db::PreferredOrientation mode)
    : m_mode (mode)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  would spoil the decomposition otherwise
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  db::PreferredOrientation m_mode;
  db::OrientationReducer m_vars;
};

/**
 *  @brief A decomposition processor to deliver trapezoids
 */
class DB_PUBLIC TrapezoidDecomposition
  : public db::PolygonProcessorBase
{
public:
  TrapezoidDecomposition (db::TrapezoidDecompositionMode mode)
    : m_mode (mode)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  would spoil the decomposition otherwise
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  db::TrapezoidDecompositionMode m_mode;
  db::OrientationReducer m_vars;
};

/**
 *  @brief A polygon breaker processor
 *  This processor reduces polygons with more than max_vertex_count vertices and
 *  an bbox-to-polygon area ratio bigger than max_area_ratio.
 *  A zero value for these parameters means "don't care".
 */
class DB_PUBLIC PolygonBreaker
  : public db::PolygonProcessorBase
{
public:
  PolygonBreaker (size_t max_vertex_count, double max_area_ratio)
    : m_max_vertex_count (max_vertex_count), m_max_area_ratio (max_area_ratio)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  would spoil the decomposition otherwise
  virtual bool requires_raw_input () const { return true; }  //  acts on original shapes
  virtual bool wants_variants () const { return false; }

private:
  size_t m_max_vertex_count;
  double m_max_area_ratio;
};

/**
 *  @brief A sizing processor
 */
class DB_PUBLIC PolygonSizer
  : public db::PolygonProcessorBase
{
public:
  PolygonSizer (db::Coord dx, db::Coord dy, unsigned int mode);
  ~PolygonSizer ();

  virtual const TransformationReducer *vars () const { return m_vars; }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

  virtual bool result_is_merged () const;
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  TransformationReducer *m_vars;
  db::Coord m_dx, m_dy;
  unsigned int m_mode;
};

/**
 *  @brief A triangulation processor
 */
class DB_PUBLIC TriangulationProcessor
  : public db::PolygonProcessorBase
{
public:
  TriangulationProcessor (double max_area = 0.0, double min_b = 1.0);

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  db::plc::TriangulationParameters m_param;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief Computes the Minkowski sum between the polygons and the given object
 *  The object can be Edge, Polygon, Box and std::vector<Point>
 */
template <class K>
class DB_PUBLIC_TEMPLATE minkowski_sum_computation
  : public db::PolygonProcessorBase
{
public:
  minkowski_sum_computation (const K &q)
    : m_q (q)
  {
    //  .. nothing yet ..
  }

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
  {
    result.push_back (db::minkowski_sum (poly, m_q, false));
  }

  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const
  {
    result.push_back (db::PolygonWithProperties (db::minkowski_sum (poly, m_q, false), poly.properties_id ()));
  }

  //  TODO: could be less if the object is symmetric
  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  K m_q;
  db::MagnificationAndOrientationReducer m_vars;
};

/**
 *  @brief Computes DRC hulls for DRC space visualization
 */
class DB_PUBLIC_TEMPLATE DRCHullProcessor
  : public db::PolygonProcessorBase
{
public:
  DRCHullProcessor (db::Coord d, db::metrics_type metrics, size_t n_circle = 64);

  void process (const db::Polygon &poly, std::vector<db::Polygon> &result) const;
  void process (const db::PolygonWithProperties &poly, std::vector<db::PolygonWithProperties> &result) const;

private:
  db::Coord m_d;
  db::metrics_type m_metrics;
  size_t m_n_circle;

  void do_process (const db::Polygon &poly, db::PolygonSink &sink) const;
};


}

#endif
