
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

#ifndef HDR_dbEdgesUtils
#define HDR_dbEdgesUtils

#include "dbCommon.h"
#include "dbEdges.h"
#include "dbBoxScanner.h"
#include "dbPolygonTools.h"
#include "tlSelect.h"

namespace db {

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
    length_type l = edge.length ();
    if (! m_inverse) {
      return l >= m_lmin && l < m_lmax;
    } else {
      return ! (l >= m_lmin && l < m_lmax);
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
  EdgeOrientationFilter (double amin, double amax, bool inverse)
    : m_inverse (inverse), m_exact (false)
  {
    m_emin = db::DVector (cos (amin * M_PI / 180.0), sin (amin * M_PI / 180.0));
    m_emax = db::DVector (cos (amax * M_PI / 180.0), sin (amax * M_PI / 180.0));
  }

  /**
   *  @brief Constructor
   *
   *  @param a The angle (measured against the x axis)
   *  @param inverse If set to true, only edges not matching this criterion will be filtered
   *
   *  This filter will filter out all edges whose angle against x axis
   *  is equal to a.
   */
  EdgeOrientationFilter (double a, bool inverse)
    : m_inverse (inverse), m_exact (true)
  {
    m_emin = db::DVector (cos (a * M_PI / 180.0), sin (a * M_PI / 180.0));
  }

  /**
   *  @brief Returns true if the edge orientation matches the criterion
   */
  virtual bool selected (const db::Edge &edge) const
  {
    int smin = db::vprod_sign (m_emin, db::DVector (edge.d ()));
    if (m_exact) {
      if (! m_inverse) {
        return smin == 0;
      } else {
        return smin != 0;
      }
    } else {
      int smax = db::vprod_sign (m_emax, db::DVector (edge.d ()));
      if (! m_inverse) {
        return (smin >= 0 && smax < 0) || (smax > 0 && smin <= 0);
      } else {
        return ! ((smin >= 0 && smax < 0) || (smax > 0 && smin <= 0));
      }
    }
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
  db::DVector m_emin, m_emax;
  bool m_inverse;
  bool m_exact;
  db::MagnificationAndOrientationReducer m_vars;
};

/**
 *  @brief A helper class for the edge interaction functionality which acts as an edge pair receiver
 */
template <class OutputContainer>
class edge_interaction_filter
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  edge_interaction_filter (OutputContainer &output)
    : mp_output (&output)
  {
    //  .. nothing yet ..
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  {
    //  Select the edges which intersect
    if (p1 != p2) {
      const db::Edge *o = p1 > p2 ? o2 : o1;
      const db::Edge *oo = p1 > p2 ? o1 : o2;
      if (o->intersect (*oo)) {
        if (m_seen.insert (o).second) {
          mp_output->insert (*o);
        }
      }
    }
  }

private:
  OutputContainer *mp_output;
  std::set<const db::Edge *> m_seen;
};

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
  edge_to_region_interaction_filter (OutputContainer &output)
    : mp_output (&output)
  {
    //  .. nothing yet ..
  }

  void add (const db::Edge *e, size_t, const db::Polygon *p, size_t)
  {
    const OutputType *ep = 0;
    tl::select (ep, e, p);

    if (m_seen.find (ep) == m_seen.end ()) {
      if (db::interact (*p, *e)) {
        m_seen.insert (ep);
        mp_output->insert (*ep);
      }
    }
  }

private:
  OutputContainer *mp_output;
  std::set<const OutputType *> m_seen;
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

} // namespace db

#endif
