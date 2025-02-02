
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



#ifndef HDR_dbPolygonNeighborhood
#define HDR_dbPolygonNeighborhood

#include "dbCommon.h"
#include "dbCompoundOperation.h"
#include "dbBoxScanner.h"
#include "dbMatrix.h"

namespace db
{

/**
 *  @brief A visitor for the neighbors of an edge
 */
class DB_PUBLIC PolygonNeighborhoodVisitor
  : public gsi::ObjectBase, public tl::Object
{
public:
  typedef std::pair<double, double> position_interval_type;
  typedef unsigned int input_key_type;
  typedef std::vector<db::PolygonWithProperties> neighbor_shapes_type;
  typedef std::map<input_key_type, neighbor_shapes_type> neighbors_type;

  /**
   *  @brief Constructor
   */
  PolygonNeighborhoodVisitor ();

  /**
   *  @brief Destructor
   */
  virtual ~PolygonNeighborhoodVisitor () { }

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
   *  This will report the central polygon and the neighbors.
   */
  virtual void neighbors (const db::Layout * /*layout*/, const db::Cell * /*cell*/, const db::PolygonWithProperties & /*polygon*/, const neighbors_type & /*neighbors*/) { }

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
class DB_PUBLIC PolygonNeighborhoodCompoundOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  PolygonNeighborhoodCompoundOperationNode (const std::vector<CompoundRegionOperationNode *> &children, PolygonNeighborhoodVisitor *visitor, db::Coord dist);

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
  db::Coord m_dist;
  tl::weak_ptr<PolygonNeighborhoodVisitor> mp_visitor;

  template <class T, class TR>
  void compute_local_impl (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<T, T> & /*interactions*/, std::vector<std::unordered_set<TR> > & /*results*/, const db::LocalProcessorBase * /*proc*/) const;
};

}

#endif

