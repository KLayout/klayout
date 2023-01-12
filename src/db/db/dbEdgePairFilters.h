
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


#ifndef HDR_dbEdgePairFilters
#define HDR_dbEdgePairFilters

#include "dbEdgePairs.h"
#include "dbEdgesUtils.h"

namespace db
{

class EdgeFilterBase;

/**
 *  @brief A base class for edge pair filters based on edge filters
 *
 *  If "one_must_match" is true, it is sufficient for one edge to be selected.
 *  If it is false, both edges need to be selected to make the edge pair selected.
 *
 *  NOTE: the edge filter is not owned by the edge pair filter object.
 */
class DB_PUBLIC EdgeFilterBasedEdgePairFilter
  : public EdgePairFilterBase
{
public:
  EdgeFilterBasedEdgePairFilter (EdgeFilterBase *edge_filter, bool one_must_match);
  virtual ~EdgeFilterBasedEdgePairFilter ();

  virtual bool selected (const db::EdgePair &edge_pair) const;
  virtual const TransformationReducer *vars () const;
  virtual bool wants_variants () const;

private:
  EdgeFilterBase *mp_edge_filter;
  bool m_one_must_match;
};

/**
 *  @brief Filters edge pairs based on the distance of the edges.
 *
 *  The distance is measured as the smallest distance between each of the points of the two edges.
 */
class DB_PUBLIC EdgePairFilterByDistance
  : public EdgePairFilterBase
{
public:
  typedef db::coord_traits<db::Coord>::distance_type distance_type;

  EdgePairFilterByDistance (distance_type min_distance, distance_type max_distance, bool inverted);

  virtual bool selected (const db::EdgePair &edge_pair) const;
  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool wants_variants () const { return true; }

private:
  distance_type m_min_distance, m_max_distance;
  bool m_inverted;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief Filters edge pairs based on the distance of the edges.
 *
 *  The distance is measured as the smallest distance between each of the points of the two edges.
 */
class DB_PUBLIC EdgePairFilterByArea
  : public EdgePairFilterBase
{
public:
  typedef db::coord_traits<db::Coord>::area_type area_type;

  EdgePairFilterByArea (area_type min_area, area_type max_area, bool inverted);

  virtual bool selected (const db::EdgePair &edge_pair) const;
  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool wants_variants () const { return true; }

private:
  area_type m_min_area, m_max_area;
  bool m_inverted;
  db::MagnificationReducer m_vars;
};

/**
 *  @brief Filters edge pairs based on the distance of the edges.
 *
 *  The distance is measured as the smallest distance between each of the points of the two edges.
 */
class DB_PUBLIC InternalAngleEdgePairFilter
  : public EdgePairFilterBase
{
public:
  InternalAngleEdgePairFilter (double a, bool inverted);
  InternalAngleEdgePairFilter (double amin, bool include_amin, double amax, bool include_amax, bool inverted);

  virtual bool selected (const db::EdgePair &edge_pair) const;
  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool wants_variants () const { return false; }

private:
  bool m_inverted;
  db::EdgeAngleChecker m_checker;
};

}

#endif
