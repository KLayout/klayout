
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



#ifndef HDR_dbEdgesLocalOperation
#define HDR_dbEdgesLocalOperation

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbEdgeBoolean.h"
#include "dbEdgeProcessor.h"
#include "dbLocalOperation.h"
#include "dbEdgesUtils.h"

namespace db
{

/**
 *  @brief Implements a boolean AND or NOT operation between edges
 */
class DB_PUBLIC EdgeBoolAndOrNotLocalOperation
  : public local_operation<db::Edge, db::Edge, db::Edge>
{
public:
  EdgeBoolAndOrNotLocalOperation (db::EdgeBoolOp op);

  virtual void do_compute_local (db::Layout *layout, db::Cell *cell, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &result, const db::LocalProcessorBase *proc) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

  //  edge interaction distance is 1 to force overlap between edges and edge/boxes
  virtual db::Coord dist () const { return 1; }

private:
  db::EdgeBoolOp m_op;
};

/**
 *  @brief Implements a boolean AND or NOT operation between edges and polygons (polygons as intruders)
 *
 *  "AND" is implemented by "outside == false", "NOT" by "outside == true" with "include_borders == true".
 *  With "include_borders == false" the operations are "INSIDE" and "OUTSIDE".
 */
class DB_PUBLIC EdgeToPolygonLocalOperation
  : public local_operation<db::Edge, db::PolygonRef, db::Edge>
{
public:
  EdgeToPolygonLocalOperation (EdgePolygonOp::mode_t op, bool include_borders);

  virtual void do_compute_local (db::Layout *layout, db::Cell *cell, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &result, const db::LocalProcessorBase *proc) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

  //  edge interaction distance is 1 to force overlap between edges and edge/boxes
  virtual db::Coord dist () const { return m_include_borders ? 1 : 0; }

private:
  db::EdgePolygonOp::mode_t m_op;
  bool m_include_borders;
};

/**
 *  @brief Implements edge-to-edge interactions
 */
class DB_PUBLIC Edge2EdgeInteractingLocalOperation
  : public local_operation<db::Edge, db::Edge, db::Edge>
{
public:
  enum output_mode_t { Normal, Inverse, Both };

  Edge2EdgeInteractingLocalOperation (EdgeInteractionMode mode, output_mode_t output_mode, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  EdgeInteractionMode m_mode;
  output_mode_t m_output_mode;
  size_t m_min_count, m_max_count;
};

/**
 *  @brief Implements edge-to-edge interactions (pull mode)
 */
class DB_PUBLIC Edge2EdgePullLocalOperation
  : public local_operation<db::Edge, db::Edge, db::Edge>
{
public:
  Edge2EdgePullLocalOperation ();

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

/**
 *  @brief Implements edge-to-polygon interactions
 */
template<class TI>
class DB_PUBLIC edge_to_polygon_interacting_local_operation
  : public local_operation<db::Edge, TI, db::Edge>
{
public:
  enum output_mode_t { Normal, Inverse, Both };

  edge_to_polygon_interacting_local_operation (EdgeInteractionMode mode, output_mode_t output_mode, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, TI> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  EdgeInteractionMode m_mode;
  output_mode_t m_output_mode;
  size_t m_min_count, m_max_count;
};

/**
 *  @brief Implements edge-to-polygon interactions (pull mode)
 */
class DB_PUBLIC Edge2PolygonPullLocalOperation
  : public local_operation<db::Edge, db::PolygonRef, db::PolygonRef>
{
public:
  Edge2PolygonPullLocalOperation ();

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

}

#endif
