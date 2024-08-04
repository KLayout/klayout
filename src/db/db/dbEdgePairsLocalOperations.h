
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef HDR_dbEdgePairsLocalOperation
#define HDR_dbEdgePairsLocalOperation

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbLocalOperation.h"
#include "dbEdgePairsUtils.h"

namespace db
{

/**
 *  @brief Implements edge pair-to-edge interactions
 */
class DB_PUBLIC EdgePair2EdgeInteractingLocalOperation
  : public local_operation<db::EdgePair, db::Edge, db::EdgePair>
{
public:
  enum output_mode_t { Normal, Inverse, Both };

  EdgePair2EdgeInteractingLocalOperation (output_mode_t output_mode, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, db::Edge> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  output_mode_t m_output_mode;
  size_t m_min_count, m_max_count;
};

/**
 *  @brief Implements edge pair-to-edge interactions (pull mode)
 */
class DB_PUBLIC EdgePair2EdgePullLocalOperation
  : public local_operation<db::EdgePair, db::Edge, db::Edge>
{
public:
  EdgePair2EdgePullLocalOperation ();

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

/**
 *  @brief Implements edge-to-polygon interactions
 */
template<class TI>
class DB_PUBLIC edge_pair_to_polygon_interacting_local_operation
  : public local_operation<db::EdgePair, TI, db::EdgePair>
{
public:
  enum output_mode_t { Normal, Inverse, Both };

  edge_pair_to_polygon_interacting_local_operation (EdgePairInteractionMode mode, output_mode_t output_mode, size_t min_count, size_t max_count);

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, TI> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  EdgePairInteractionMode m_mode;
  output_mode_t m_output_mode;
  size_t m_min_count, m_max_count;
};

/**
 *  @brief Implements edge-to-polygon interactions (pull mode)
 */
class DB_PUBLIC EdgePair2PolygonPullLocalOperation
  : public local_operation<db::EdgePair, db::PolygonRef, db::PolygonRef>
{
public:
  EdgePair2PolygonPullLocalOperation ();

  virtual db::Coord dist () const;
  virtual void do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase * /*proc*/) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;
};

}

#endif
