
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

#ifndef HDR_dbEdgesUtils
#define HDR_dbEdgesUtils

#include "dbCommon.h"
#include "dbHash.h"
#include "dbEdges.h"
#include "dbBoxScanner.h"
#include "dbPolygonTools.h"
#include "tlSelect.h"

#include <unordered_set>

namespace db {

/**
 *  @brief The operation mode for the interaction filters
 */
enum EdgeInteractionMode { EdgesInteract, EdgesInside, EdgesOutside };

class PolygonSink;

/**
 *  @brief An edge length filter for use with Edges::filter or Edges::filtered
 *
 *  This filter has two parameters: lmin and lmax.
 *  It will filter all edges for which the length is >= lmin and < lmax.
 *  There is an "invert" flag which allows selecting all edges not
 *  matching the criterion.
 */

struct DB_PUBLIC EdgeLengthFilter
  : public EdgeFilterBase
{
  typedef db::Edge::distance_type length_type;

  /**
   *  @brief Constructor
   *
   *  @param lmin The minimum length
   *  @param lmax The maximum length
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  EdgeLengthFilter (length_type lmin, length_type lmax, bool inverse)
    : m_lmin (lmin), m_lmax (lmax), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the edge length matches the criterion
   */
  virtual bool selected (const db::Edge &edge) const
  {
    return check (edge.length ());
  }

  /**
   *  @brief Returns true if the total edge length matches the criterion
   */
  bool selected (const std::unordered_set<db::Edge> &edges) const
  {
    length_type l = 0;
    for (std::unordered_set<db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
      l += e->length ();
    }
    return check (l);
  }

  /**
   *  @brief This filter is isotropic
   */
  virtual const TransformationReducer *vars () const
  {
    return &m_vars;
  }

  /**
   *  @brief Requires merged input
   */
  virtual bool requires_raw_input () const
  {
    return false;
  }

  /**
   *  @brief Wants to build variants
   */
  virtual bool wants_variants () const
  {
    return true;
  }

private:
  length_type m_lmin, m_lmax;
  bool m_inverse;
  db::MagnificationReducer m_vars;

  virtual bool check (length_type l) const
  {
    if (! m_inverse) {
      return l >= m_lmin && l < m_lmax;
    } else {
      return ! (l >= m_lmin && l < m_lmax);
    }
  }
};

/**
 *  @brief An angle detector
 *
 *  This detector can check whether the angle between two edges is within a certain angle interval.
 *  It takes two edges: a and b. If b "turns left" (b following a), the angle will be positive, if it "turns" right,
 *  the angle will be negative. The angle can be between -180 and 180 degree. The case of reflection
 *  (exactly 180 degree) is not considered.
 *
 *  The constraint can be given in terms of a minimum and maximum angle. "include" specifies whether the
 *  angle value itself is included. The operator() will return true, if the angle between the given
 *  edges a and b in matching the constraint.
 */
class DB_PUBLIC EdgeAngleChecker
{
public:
  EdgeAngleChecker (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end);

  bool operator() (const db::Edge &a, const db::Edge &b) const
  {
    return m_all || check (a.d (), b.d ());
  }

  bool operator() (const db::Vector &a, const db::Vector &b) const
  {
    return m_all || check (a, b);
  }

private:
  db::CplxTrans m_t_start, m_t_end;
  bool m_include_start, m_include_end;
  bool m_big_angle, m_all;

  bool check (const db::Vector &a, const db::Vector &b) const;
};

/**
 *  @brief An edge orientation filter for use with Edges::filter or Edges::filtered
 *
 *  This filter has two parameters: amin and amax.
 *  It will filter all edges for which the orientation angle is >= amin and < amax.
 *  The orientation angle is measured in degree against the x axis in the mathematical sense.
 *  There is an "invert" flag which allows selecting all edges not
 *  matching the criterion.
 */

struct DB_PUBLIC EdgeOrientationFilter
  : public EdgeFilterBase
{
  /**
   *  @brief Constructor
   *
   *  @param amin The minimum angle (measured against the x axis)
   *  @param amax The maximum angle (measured against the x axis)
   *  @param inverse If set to true, only edges not matching this criterion will be filtered
   *
   *  This filter will filter out all edges whose angle against x axis
   *  is larger or equal to amin and less than amax.
   */
  EdgeOrientationFilter (double amin, bool include_amin, double amax, bool include_amax, bool inverse);

  /**
   *  @brief Constructor
   *
   *  @param a The angle (measured against the x axis)
   *  @param inverse If set to true, only edges not matching this criterion will be filtered
   *
   *  This filter will filter out all edges whose angle against x axis
   *  is equal to a.
   */
  EdgeOrientationFilter (double a, bool inverse);

  /**
   *  @brief Returns true if the edge orientation matches the criterion
   */
  virtual bool selected (const db::Edge &edge) const;

  /**
   *  @brief Returns true if all edge orientations match the criterion
   */
  virtual bool selected (const std::unordered_set<db::Edge> &edges) const
  {
    for (std::unordered_set<db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
      if (! selected (*e)) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief This filter is not isotropic
   */
  virtual const TransformationReducer *vars () const
  {
    return &m_vars;
  }

  /**
   *  @brief Requires merged input
   */
  virtual bool requires_raw_input () const
  {
    return false;
  }

  /**
   *  @brief Wants to build variants
   */
  virtual bool wants_variants () const
  {
    return true;
  }

private:
  bool m_inverse;
  db::MagnificationAndOrientationReducer m_vars;
  EdgeAngleChecker m_checker;
};

/**
 *  @brief An edge orientation filter for use with Edges::filter or Edges::filtered
 *
 *  This filter renders edges with special orientations.
 */

struct DB_PUBLIC SpecialEdgeOrientationFilter
  : public EdgeFilterBase
{
  enum FilterType {
    Ortho = 0,          //  0 and 90 degree
    Diagonal = 1,       //  -45 and 45 degree
    OrthoDiagonal = 2   //  both Ortho and Diagonal
  };

  /**
   *  @brief Constructor
   *
   *  @param type The type of filter
   */
  SpecialEdgeOrientationFilter (FilterType type, bool inverse);

  /**
   *  @brief Returns true if the edge orientation matches the criterion
   */
  virtual bool selected (const db::Edge &edge) const;

  /**
   *  @brief Returns true if all edge orientations match the criterion
   */
  virtual bool selected (const std::unordered_set<db::Edge> &edges) const
  {
    for (std::unordered_set<db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
      if (! selected (*e)) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief This filter is not isotropic
   */
  virtual const TransformationReducer *vars () const
  {
    return &m_vars;
  }

  /**
   *  @brief Requires merged input
   */
  virtual bool requires_raw_input () const
  {
    return false;
  }

  /**
   *  @brief Wants to build variants
   */
  virtual bool wants_variants () const
  {
    return true;
  }

private:
  FilterType m_type;
  bool m_inverse;
  db::OrthogonalTransformationReducer m_vars;
};

/**
 *  @brief A predicate defining edge a interacts with b
 */
DB_PUBLIC bool edge_interacts (const db::Edge &a, const db::Edge &b);

/**
 *  @brief A predicate defining edge a is "inside" b
 */
DB_PUBLIC bool edge_is_inside (const db::Edge &a, const db::Edge &b);

/**
 *  @brief A predicate defining edge a is "outside" b
 */
DB_PUBLIC bool edge_is_outside (const db::Edge &a, const db::Edge &b);

/**
 *  @brief A helper class for the edge interaction functionality which acts as an edge pair receiver
 */
template <class OutputContainer>
class edge_interaction_filter
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  edge_interaction_filter (OutputContainer &output, EdgeInteractionMode mode)
    : mp_output (&output), m_mode (mode)
  {
    //  .. nothing yet ..
  }

  void finish (const db::Edge *o, size_t p)
  {
    if (p == 0 && m_mode == EdgesOutside && m_seen.find (o) == m_seen.end ()) {
      mp_output->insert (*o);
    }
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  {
    //  Select the edges which intersect
    if (p1 != p2) {

      const db::Edge *o = p1 > p2 ? o2 : o1;
      const db::Edge *oo = p1 > p2 ? o1 : o2;

      if ((m_mode == EdgesInteract && db::edge_interacts (*o, *oo)) ||
          (m_mode == EdgesInside && db::edge_is_inside (*o, *oo))) {

        if (m_seen.insert (o).second) {
          mp_output->insert (*o);
        }

      } else if (m_mode == EdgesOutside && ! db::edge_is_outside (*o, *oo)) {

        //  In this case we need to collect edges which are outside always - we report those on "finished".
        m_seen.insert (o);

      }

    }
  }

private:
  OutputContainer *mp_output;
  std::set<const db::Edge *> m_seen;
  EdgeInteractionMode m_mode;
};

/**
 *  @brief A predicate defining edge a interacts with polygon b
 */
DB_PUBLIC bool edge_interacts (const db::Edge &a, const db::Polygon &b);

/**
 *  @brief A predicate defining edge a is "inside" polygon b
 */
DB_PUBLIC bool edge_is_inside (const db::Edge &a, const db::Polygon &b);

/**
 *  @brief A predicate defining edge a is "outside" polygon b
 */
DB_PUBLIC bool edge_is_outside (const db::Edge &a, const db::Polygon &b);

/**
 *  @brief A helper class for the edge to region interaction functionality which acts as an edge pair receiver
 *
 *  Note: This special scanner uses pointers to two different objects: edges and polygons.
 *  It uses odd value pointers to indicate pointers to polygons and even value pointers to indicate
 *  pointers to edges.
 *
 *  There is a special box converter which is able to sort that out as well.
 */
template <class OutputContainer, class OutputType = typename OutputContainer::value_type>
class edge_to_region_interaction_filter
  : public db::box_scanner_receiver2<db::Edge, size_t, db::Polygon, size_t>
{
public:
  edge_to_region_interaction_filter (OutputContainer *output, EdgeInteractionMode mode)
    : mp_output (output), m_mode (mode)
  {
    //  .. nothing yet ..
  }

  void finish (const OutputType *o)
  {
    if (m_mode == EdgesOutside && m_seen.find (o) == m_seen.end ()) {
      mp_output->insert (*o);
    }
  }

  void finish1 (const db::Edge *o, size_t /*p*/)
  {
    const OutputType *ep = 0;
    tl::select (ep, o, (const db::Polygon *) 0);
    if (ep) {
      finish (ep);
    }
  }

  void finish2 (const db::Polygon *o, size_t /*p*/)
  {
    const OutputType *ep = 0;
    tl::select (ep, (const db::Edge *) 0, o);
    if (ep) {
      finish (ep);
    }
  }

  void add (const db::Edge *e, size_t, const db::Polygon *p, size_t)
  {
    const OutputType *ep = 0;
    tl::select (ep, e, p);

    if (m_seen.find (ep) == m_seen.end ()) {

      if ((m_mode == EdgesInteract && db::edge_interacts (*e, *p)) ||
          (m_mode == EdgesInside && db::edge_is_inside (*e, *p))) {

        m_seen.insert (ep);
        mp_output->insert (*ep);

      } else if (m_mode == EdgesOutside && ! db::edge_is_outside (*e, *p)) {

        //  In this case we need to collect edges which are outside always - we report those on "finished".
        m_seen.insert (ep);

      }

    }
  }

private:
  OutputContainer *mp_output;
  std::set<const OutputType *> m_seen;
  EdgeInteractionMode m_mode;
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 *
 *  If will perform a edge by edge check using the provided EdgeRelationFilter
 */
template <class Output>
class edge2edge_check_for_edges
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  edge2edge_check_for_edges (const EdgeRelationFilter &check, Output &output, bool requires_different_layers)
    : mp_check (&check), mp_output (&output)
  {
    m_requires_different_layers = requires_different_layers;
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  {
    //  Overlap or inside checks require input from different layers
    if (! m_requires_different_layers || ((p1 ^ p2) & 1) != 0) {

      //  ensure that the first check argument is of layer 1 and the second of
      //  layer 2 (unless both are of the same layer)
      int l1 = int (p1 & size_t (1));
      int l2 = int (p2 & size_t (1));

      db::EdgePair ep;
      if (mp_check->check (l1 <= l2 ? *o1 : *o2, l1 <= l2 ? *o2 : *o1, &ep)) {
        mp_output->insert (ep);
      }

    }
  }

private:
  const EdgeRelationFilter *mp_check;
  Output *mp_output;
  bool m_requires_different_layers;
};

/**
 *  @brief A helper class to turn joined edge sequences into polygons
 *
 *  This object is an edge cluster so it can connect to a cluster collector
 *  driven by a box scanner.
 */
struct JoinEdgesCluster
  : public db::cluster<db::Edge, size_t>
{
  typedef db::Edge::coord_type coord_type;

  JoinEdgesCluster (db::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i);
  void finish ();

private:
  db::PolygonSink *mp_output;
  coord_type m_ext_b, m_ext_e, m_ext_o, m_ext_i;
};

/**
 *  @brief Implements the extension algorithm to turn an edge into a polygon
 */
db::Polygon extended_edge (const db::Edge &edge, db::Coord ext_b, db::Coord ext_e, db::Coord ext_o, db::Coord ext_i);

/**
 *  @brief Wraps the extension algorithm into a edge to polygon processor
 */
class DB_PUBLIC ExtendedEdgeProcessor
  : public db::EdgeToPolygonProcessorBase
{
public:
  ExtendedEdgeProcessor (db::Coord e)
    : m_ext_b (e), m_ext_e (e), m_ext_o (e), m_ext_i (e)
  { }

  ExtendedEdgeProcessor (db::Coord ext_b, db::Coord ext_e, db::Coord ext_o, db::Coord ext_i)
    : m_ext_b (ext_b), m_ext_e (ext_e), m_ext_o (ext_o), m_ext_i (ext_i)
  { }

  virtual void process (const Edge &edge, std::vector<db::Polygon> &res) const
  {
    res.push_back (extended_edge (edge, m_ext_b, m_ext_e, m_ext_o, m_ext_i));
  }

private:
  db::Coord m_ext_b, m_ext_e, m_ext_o, m_ext_i;
};

/**
 * @brief The EdgeSegmentSelector class
 */
class DB_PUBLIC EdgeSegmentSelector
  : public EdgeProcessorBase
{
public:
  EdgeSegmentSelector (int mode, Edge::distance_type length, double fraction);
  ~EdgeSegmentSelector ();

  virtual void process (const db::Edge &edge, std::vector<db::Edge> &res) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool result_must_not_be_merged () const { return m_length <= 0; }
  virtual bool wants_variants () const { return true; }

private:
  int m_mode;
  db::Edge::distance_type m_length;
  double m_fraction;
  db::MagnificationReducer m_vars;
};

} // namespace db

#endif
