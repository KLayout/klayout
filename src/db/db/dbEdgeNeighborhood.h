
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



#ifndef HDR_dbEdgeNeighborhood
#define HDR_dbEdgeNeighborhood

#include "dbCommon.h"
#include "dbCompoundOperation.h"
#include "dbBoxScanner.h"
#include "dbMatrix.h"

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
  typedef std::vector<db::PolygonWithProperties> neighbor_shapes_type;
  typedef std::map<input_key_type, neighbor_shapes_type> neighbors_per_interval_type;
  typedef std::vector<std::pair<position_interval_type, neighbors_per_interval_type> > neighbors_type;

  /**
   *  @brief Constructor
   */
  EdgeNeighborhoodVisitor ();

  /**
   *  @brief Destructor
   */
  virtual ~EdgeNeighborhoodVisitor () { }

  /**
   *  @brief Configure the polygon output
   */
  void connect_output (db::Layout * /*layout*/, std::unordered_set<db::PolygonWithProperties> *polygons) const;

  /**
   *  @brief Configure the polygon ref output
   */
  void connect_output (db::Layout *layout, std::unordered_set<db::PolygonRefWithProperties> *polygons) const;

  /**
   *  @brief Configure the edge output
   */
  void connect_output (db::Layout * /*layout*/, std::unordered_set<db::EdgeWithProperties> *edges) const;

  /**
   *  @brief Configure the edge pair output
   */
  void connect_output (db::Layout * /*layout*/, std::unordered_set<db::EdgePairWithProperties> *edge_pairs) const;

  /**
   *  @brief Disconnects output
   */
  void disconnect_outputs () const;

  /**
   *  @brief Event handler called when a new polygon is encountered
   *  Following this event, the edges with their neighborhood are reported.
   *  After the edges are reported, "end_polygon" is called.
   */
  virtual void begin_polygon (const db::Layout * /*layout*/, const db::Cell * /*cell*/, const db::PolygonWithProperties & /*polygon*/) { }

  /**
   *  @brief Event handler called after the polygon was processed
   */
  virtual void end_polygon () { }

  /**
   *  @brief Event handler for each edge plus it's neighborhood
   */
  virtual void on_edge (const db::Layout * /*layout*/, const db::Cell * /*cell*/, const db::EdgeWithProperties & /*edge*/, const neighbors_type & /*neighbors*/) { }

  /**
   *  @brief Gets a transformation to transform from edge-local space to original space
   */
  static db::IMatrix3d to_original_trans (const db::Edge &edge);

  /**
   *  @brief Gets a transformation to transform from original space into edge-local space
   */
  static db::IMatrix3d to_edge_local_trans (const db::Edge &edge);

  /**
   *  @brief Sets the result type
   */
  void set_result_type (db::CompoundRegionOperationNode::ResultType result_type)
  {
    m_result_type = result_type;
  }

  /**
   *  @brief Gets the result type
   */
  db::CompoundRegionOperationNode::ResultType result_type () const
  {
    return m_result_type;
  }

  /**
   *  @brief Delivers a polygon
   *  This function is only permitted if the result type is Region.
   */
  void output_polygon (const PolygonWithProperties &poly);

  /**
   *  @brief Delivers an edge
   *  This function is only permitted if the result type is Edges.
   */
  void output_edge (const db::EdgeWithProperties &edge);

  /**
   *  @brief Delivers an edge pair object
   *  This function is only permitted if the result type is EdgePairs.
   */
  void output_edge_pair (const db::EdgePairWithProperties &edge_pair);

private:
  db::CompoundRegionOperationNode::ResultType m_result_type;
  mutable std::unordered_set<db::PolygonWithProperties> *mp_polygons;
  mutable std::unordered_set<db::PolygonRefWithProperties> *mp_polygon_refs;
  mutable std::unordered_set<db::EdgeWithProperties> *mp_edges;
  mutable std::unordered_set<db::EdgePairWithProperties> *mp_edge_pairs;
  mutable db::Layout *mp_layout;
};

/**
 *  @brief A local operation for implementation of the neighborhood visitor
 */
class DB_PUBLIC EdgeNeighborhoodCompoundOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  EdgeNeighborhoodCompoundOperationNode (const std::vector<CompoundRegionOperationNode *> &children, EdgeNeighborhoodVisitor *visitor, db::Coord bext, db::Coord eext, db::Coord din, db::Coord dout);

  virtual ResultType result_type () const
  {
    return mp_visitor ? mp_visitor->result_type () : CompoundRegionOperationNode::Edges;
  }

  virtual bool wants_caching () const
  {
    return false;
  }

protected:
  virtual db::Coord computed_dist () const;
  virtual std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> & /*interactions*/,       std::vector<std::unordered_set<db::PolygonWithProperties> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const;
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> & /*interactions*/,       std::vector<std::unordered_set<db::EdgeWithProperties> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const;
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePairWithProperties> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const;
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRefWithProperties> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const;
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> & /*interactions*/, std::vector<std::unordered_set<db::EdgeWithProperties> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const;
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> & /*interactions*/, std::vector<std::unordered_set<db::EdgePairWithProperties> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const;

private:
  db::Coord m_bext, m_eext, m_din, m_dout;
  tl::weak_ptr<EdgeNeighborhoodVisitor> mp_visitor;

  void do_collect_neighbors (db::box_scanner2<db::EdgeWithProperties, unsigned int, db::PolygonWithProperties, unsigned int> &scanner, const db::Layout *layout, const db::Cell *cell) const;

  template <class T, class TR>
  void compute_local_impl (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<T, T> & /*interactions*/, std::vector<std::unordered_set<TR> > & /*results*/, const db::LocalProcessorBase * /*proc*/) const;
};

}

#endif

