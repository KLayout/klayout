
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



#ifndef HDR_dbEdgeNeighborhood
#define HDR_dbEdgeNeighborhood

#include "dbCommon.h"
#include "dbCompoundOperation.h"
#include "dbBoxScanner.h"

namespace db
{

/**
 *  @brief A visitor for the neighbors of an edge
 */
class DB_PUBLIC EdgeNeighborhoodVisitor
  : public gsi::ObjectBase, public tl::Object
{
public:
  typedef std::pair<double, double> position_interval_type;
  typedef unsigned int input_key_type;
  typedef std::list<db::Polygon> neighbor_shapes_type;
  typedef std::map<input_key_type, neighbor_shapes_type> neighbors_per_interval_type;
  typedef std::vector<std::pair<position_interval_type, neighbors_per_interval_type> > neighbors_type;

  /**
   *  @brief Constructor
   */
  EdgeNeighborhoodVisitor () { }

  /**
   *  @brief Destructor
   */
  virtual ~EdgeNeighborhoodVisitor () { }

  /**
   *  @brief Event handler for each edge plus it's neighborhood
   */
  void on_edge (const db::Layout *layout, const db::Cell *cell, const db::Edge &edge, const neighbors_type &neighbors);
};

/**
 *  @brief A local operation for implementation of the neighborhood visitor
 */
class DB_PUBLIC EdgeNeighborhoodCompoundOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  EdgeNeighborhoodCompoundOperationNode (const std::vector<CompoundRegionOperationNode *> &children, EdgeNeighborhoodVisitor *visitor, const db::Coord bext, db::Coord eext, db::Coord din, db::Coord dout);

  virtual ResultType result_type () const
  {
    return CompoundRegionOperationNode::Edges;
  }

  virtual bool wants_caching () const
  {
    return false;
  }

protected:
  virtual db::Coord computed_dist () const;
  virtual std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const;
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const;

  //  not implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  db::Coord m_bext, m_eext, m_din, m_dout;
  tl::weak_ptr<EdgeNeighborhoodVisitor> mp_visitor;

  void do_collect_neighbors (db::box_scanner2<db::Edge, unsigned int, db::Polygon, unsigned int> &scanner, const db::Layout *layout, const db::Cell *cell) const;
};

}

#endif

