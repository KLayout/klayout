
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


#ifndef HDR_dbRegionLocalOperations
#define HDR_dbRegionLocalOperations

#include "dbCommon.h"
#include "dbEdgePairRelations.h"
#include "dbLocalOperation.h"
#include "dbEdgeProcessor.h"

namespace db
{

/**
 *  @brief Specifies an error filter on rectangular shapes
 */
enum RectFilter
{
  /**
   *  @brief No filter
   */
  NoSideAllowed,

  /**
   *  @brief Allow errors on one side
   */
  OneSideAllowed,

  /**
   *  @brief Allow errors on two sides (not specified which)
   */
  TwoSidesAllowed,

  /**
   *  @brief Allow errors on two sides ("L" configuration)
   */
  TwoConnectedSidesAllowed,

  /**
   *  @brief Allow errors on two opposite sides
   */
  TwoOppositeSidesAllowed,

  /**
   *  @brief Allow errors on three sides
   */
  ThreeSidesAllowed
};

/**
 *  @brief Specifies an error filter for opposite errors
 */
enum OppositeFilter
{
  /**
   *  @brief No filter
   */
  NoOppositeFilter,

  /**
   *  @brief Only errors appearing on opposite sides of a figure will be reported
   */
  OnlyOpposite,

  /**
   *  @brief Only errors NOT appearing on opposite sides of a figure will be reported
   */
  NotOpposite
};

template <class TS, class TI>
class check_local_operation
  : public local_operation<TS, TI, db::EdgePair>
{
public:
  check_local_operation (const EdgeRelationFilter &check, bool different_polygons, bool has_other, bool other_is_merged, bool shielded, db::OppositeFilter opposite_filter, db::RectFilter rect_filter);

  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;

  virtual db::Coord dist () const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  EdgeRelationFilter m_check;
  bool m_different_polygons;
  bool m_has_other;
  bool m_other_is_merged;
  bool m_shielded;
  db::OppositeFilter m_opposite_filter;
  db::RectFilter m_rect_filter;
};

typedef check_local_operation<db::PolygonRef, db::PolygonRef> CheckLocalOperation;

template <class TS, class TI, class TR>
class interacting_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_local_operation (int mode, bool touching, bool inverse, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  int m_mode;
  bool m_touching;
  bool m_inverse;
  size_t m_min_count, m_max_count;
};

typedef interacting_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> InteractingLocalOperation;

template <class TS, class TI, class TR>
class pull_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  pull_local_operation (int mode, bool touching);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  int m_mode;
  bool m_touching;
};

typedef pull_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef> PullLocalOperation;

template <class TS, class TI, class TR>
class interacting_with_edge_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_with_edge_local_operation (bool inverse, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_inverse;
  size_t m_min_count, m_max_count;
};

typedef interacting_with_edge_local_operation<db::PolygonRef, db::Edge, db::PolygonRef> InteractingWithEdgeLocalOperation;

template <class TS, class TI, class TR>
class pull_with_edge_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  pull_with_edge_local_operation ();

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

typedef pull_with_edge_local_operation<db::PolygonRef, db::Edge, db::Edge> PullWithEdgeLocalOperation;

template <class TS, class TI, class TR>
class interacting_with_text_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  interacting_with_text_local_operation (bool inverse, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_inverse;
  size_t m_min_count, m_max_count;
};

typedef interacting_with_text_local_operation<db::PolygonRef, db::TextRef, db::PolygonRef> InteractingWithTextLocalOperation;

template <class TS, class TI, class TR>
class pull_with_text_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  pull_with_text_local_operation ();

  virtual db::Coord dist () const;
  virtual void compute_local (db::Layout *, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

typedef pull_with_text_local_operation<db::PolygonRef, db::TextRef, db::TextRef> PullWithTextLocalOperation;

} // namespace db

#endif

