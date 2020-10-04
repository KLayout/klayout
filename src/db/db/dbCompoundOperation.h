
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

#ifndef HDR_dbCompoundOperation
#define HDR_dbCompoundOperation

#include "dbLocalOperation.h"
#include "dbHierProcessor.h"
#include "dbRegionDelegate.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"

#include "gsiObject.h"
#include "tlObject.h"

#include <memory>

namespace db
{

class DB_PUBLIC CompoundRegionOperationNode
  : public gsi::ObjectBase, public tl::Object
{
public:
  enum  ResultType { Region, Edges, EdgePairs };

  CompoundRegionOperationNode ()
  {
    invalidate_cache ();
  }

  virtual ~CompoundRegionOperationNode () { }

  virtual db::Coord dist () const = 0;
  virtual std::string description () const = 0;
  virtual std::vector<db::Region *> inputs () const = 0;

  //  specifies the result type
  virtual ResultType result_type () const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const  { return 0; }

  /**
   *  @brief Returns true, if the processor wants to build variants
   *  If not true, the processor accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const { return false; }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  template <class T>
  bool compute_local_bool (db::Layout *layout, const shape_interactions<T, T> &interactions, size_t max_vertex_count, double area_ratio) const
  {
    if (result_type () == Region) {

      std::vector<std::unordered_set<T> > res;
      compute_local (layout, interactions, res, max_vertex_count, area_ratio);
      return ! res.empty ();

    } else if (result_type () == Edges) {

      std::vector<std::unordered_set<db::Edge> > res;
      compute_local (layout, interactions, res, max_vertex_count, area_ratio);
      return ! res.empty ();

    } else if (result_type () == EdgePairs) {

      std::vector<std::unordered_set<db::EdgePair> > res;
      compute_local (layout, interactions, res, max_vertex_count, area_ratio);
      return ! res.empty ();

    } else {
      return false;
    }
  }

  virtual void invalidate_cache () const
  {
    m_cache_polyref.clear ();
    m_cache_polyref_valid = false;
    m_cache_poly.clear ();
    m_cache_poly_valid = false;
    m_cache_edge.clear ();
    m_cache_edge_valid = false;
    m_cache_edge_pair.clear ();
    m_cache_edge_pair_valid = false;
  }

protected:
  //  the different computation slots
  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > &    /*results*/, size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > &       /*results*/, size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > &   /*results*/, size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/, size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > &       /*results*/, size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > &   /*results*/, size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  mutable std::vector<std::unordered_set<db::PolygonRef> > m_cache_polyref;
  mutable bool m_cache_polyref_valid;
  mutable std::vector<std::unordered_set<db::Polygon> > m_cache_poly;
  mutable bool m_cache_poly_valid;
  mutable std::vector<std::unordered_set<db::Edge> > m_cache_edge;
  mutable bool m_cache_edge_valid;
  mutable std::vector<std::unordered_set<db::EdgePair> > m_cache_edge_pair;
  mutable bool m_cache_edge_pair_valid;

  void get_cache (std::vector<std::unordered_set<db::PolygonRef> > *&cache_ptr, bool *&valid) const { cache_ptr = &m_cache_polyref;   valid = &m_cache_polyref_valid; }
  void get_cache (std::vector<std::unordered_set<db::Polygon> > *&cache_ptr, bool *&valid) const    { cache_ptr = &m_cache_poly;      valid = &m_cache_poly_valid; }
  void get_cache (std::vector<std::unordered_set<db::Edge> > *&cache_ptr, bool *&valid) const       { cache_ptr = &m_cache_edge;      valid = &m_cache_edge_valid; }
  void get_cache (std::vector<std::unordered_set<db::EdgePair> > *&cache_ptr, bool *&valid) const   { cache_ptr = &m_cache_edge_pair; valid = &m_cache_edge_pair_valid; }

  template <class T, class TR>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<db::PolygonRef> > *cache = 0;
    bool *valid = 0;
    get_cache (cache, valid);
    if (*valid) {
      results = *cache;
    } else {
      do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
      *cache = results;
    }
  }
};


template <class T>
struct compound_operation_type_traits;

template <>
struct compound_operation_type_traits<db::PolygonRef>
{
  typedef db::Region container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::Region; }
};

template <>
struct compound_operation_type_traits<db::Polygon>
{
  typedef db::Region container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::Region; }
};

template <>
struct compound_operation_type_traits<db::Edge>
{
  typedef db::Edges container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::Edges; }
};

template <>
struct compound_operation_type_traits<db::EdgePair>
{
  typedef db::EdgePairs container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::EdgePairs; }
};


class DB_PUBLIC CompoundRegionOperationPrimaryNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationPrimaryNode ();

  virtual std::string description () const;

  virtual std::vector<db::Region *> inputs () const;

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
};


class DB_PUBLIC CompoundRegionOperationSecondaryNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationSecondaryNode (db::Region *input);

  virtual std::string description () const;

  virtual std::vector<db::Region *> inputs () const;

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  db::Region *mp_input;
};


class DB_PUBLIC CompoundTransformationReducer
  : public db::TransformationReducer
{
public:
  CompoundTransformationReducer ();

  void add (const db::TransformationReducer *reducer);
  bool is_empty () const { return m_vars.empty (); }

  virtual db::Trans reduce_trans (const db::Trans &trans) const;
  virtual db::ICplxTrans reduce_trans (const db::ICplxTrans &trans) const;
  virtual db::Trans reduce (const db::Trans &trans) const;
  virtual db::ICplxTrans reduce (const db::ICplxTrans &trans) const;
  virtual bool is_translation_invariant () const;

private:
  std::vector<const db::TransformationReducer *> m_vars;
};

class DB_PUBLIC CompoundRegionMultiInputOperationNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionMultiInputOperationNode (const std::vector<CompoundRegionOperationNode *> &children);
  CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *child);
  CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);
  ~CompoundRegionMultiInputOperationNode ();

  virtual db::Coord dist () const;
  virtual std::string description () const;

  virtual std::vector<db::Region *> inputs () const
  {
    return m_inputs;
  }

  virtual const TransformationReducer *vars () const;
  virtual bool wants_variants () const;

  virtual void invalidate_cache () const;

protected:
  bool needs_reduce_interactions (unsigned int child_index) const
  {
    if (m_children.size () < 2 || child (child_index)->inputs ().empty ()) {
      return false;
    } else {
      return true;
    }
  }

  template <class TS, class TI>
  const shape_interactions<TS, TI> &interactions_for_child (const shape_interactions<TS, TI> &interactions, unsigned int child_index, shape_interactions<TS, TI> &child_interactions) const
  {
    if (! needs_reduce_interactions (child_index)) {
      return interactions;
    }

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      child_interactions.add_subject_shape (i->first, interactions.subject_shape (i->first));

      for (typename shape_interactions<TS, TI>::iterator2 ii = i->second.begin (); ii != i->second.end (); ++ii) {

        const std::pair<unsigned int, TI> &is = interactions.intruder_shape (*ii);

        std::map<std::pair<unsigned int, unsigned int>, unsigned int>::const_iterator lm = m_map_layer_to_child.find (std::make_pair (child_index, is.first));
        if (lm != m_map_layer_to_child.end ()) {
          child_interactions.add_intruder_shape (*ii, lm->second, is.second);
          child_interactions.add_interaction (i->first, *ii);
        }

      }

    }

    return child_interactions;
  }

  unsigned int children () const
  {
    return m_children.size ();
  }

  CompoundRegionOperationNode *child (unsigned int index);
  const CompoundRegionOperationNode *child (unsigned int index) const;

private:
  tl::shared_collection<CompoundRegionOperationNode> m_children;
  //  maps child#,layer# to layer# of child:
  std::map<std::pair<unsigned int, unsigned int>, unsigned int> m_map_layer_to_child;
  std::vector<db::Region *> m_inputs;
  CompoundTransformationReducer m_vars;

  void init ();
};


class DB_PUBLIC CompoundRegionLogicalBoolOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum  LogicalOp { And, Or };

  CompoundRegionLogicalBoolOperationNode (LogicalOp op, bool invert, const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

private:
  LogicalOp m_op;
  bool m_invert;

  template <class T>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const;
};


class DB_PUBLIC CompoundRegionGeometricalBoolOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum  GeometricalOp { And, Not, Or, Xor };

  CompoundRegionGeometricalBoolOperationNode (GeometricalOp op, CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  GeometricalOp m_op;

  template <class T, class TR>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const;
  template <class T, class T1, class T2, class TR>
  void implement_bool (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const;
};


class DB_PUBLIC CompoundRegionInteractOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum  GeometricalOp { And, Not, Or, Xor };

  CompoundRegionInteractOperationNode (GeometricalOp op, const CompoundRegionOperationNode *a, const CompoundRegionOperationNode *b, size_t min_count = 0, size_t max_count = std::numeric_limits<size_t>::max ());

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  GeometricalOp m_op;
};


class DB_PUBLIC CompoundRegionPullOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum  GeometricalOp { And, Not, Or, Xor };

  CompoundRegionPullOperationNode (GeometricalOp op, const CompoundRegionOperationNode *a, const CompoundRegionOperationNode *b);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  GeometricalOp m_op;
};


class DB_PUBLIC CompoundRegionSizeOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum  GeometricalOp { And, Not, Or, Xor };

  CompoundRegionSizeOperationNode (GeometricalOp op, const CompoundRegionOperationNode *input, db::Coord size_x, db::Coord size_y);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  virtual db::Coord dist () const;
  virtual const TransformationReducer *vars () const;

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  GeometricalOp m_op;
  CompoundTransformationReducer m_vars;
};


class DB_PUBLIC CompoundRegionMergeOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionMergeOperationNode (const CompoundRegionOperationNode *input, size_t min_wrap_count = 1);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
};


... either all or no input ...

template <class TS, class TI, class TR>
class DB_PUBLIC compound_region_generic_operation_node
  : public CompoundRegionMultiInputOperationNode
{
public:
  compound_region_generic_operation_node (const db::local_operation<TS, TI, TR> &op, const std::vector<CompoundRegionOperationNode *> &inputs, const db::TransformationReducer *vars = 0, bool want_variants = false, const std::string &description = "generic")
    : CompoundRegionMultiInputOperationNode (inputs), m_op (op), m_description (description), mp_vars (vars), m_wants_variants (want_variants)
  {
    //  .. nothing yet ..
  }

  virtual std::string description () const { return m_description; }
  virtual ResultType result_type () const { return compound_operation_type_traits<TR>::type (); }
  virtual const db::TransformationReducer *vars () const  { return mp_vars; }
  virtual bool wants_variants () const { return m_wants_variants; }

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;



private:
  db::local_operation<TS, TI, TR> m_op;
  std::string m_description;
  const db::TransformationReducer *mp_vars;
  bool m_wants_variants;
};


/**
 *  @brief Implements the case selection
 *
 *  Case selection is a sequence of if/then nodes:
 *  (if1 then1 if2 then2 ... default) or (if1 then1 if2 then2 ...)
 *  The first condition is tested. If true, the "then1" result is taken. Otherwise, the
 *  "if2" condition is tested and "then2" is taken if true etc. At the end the
 *  default is evaluated if no other condition matched.
 */

class DB_PUBLIC CompoundRegionLogicalCaseSelectOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionLogicalCaseSelectOperationNode (bool multi_layer, const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  //  the different computation slots
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  }

private:
  template <class T>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const;

  bool m_multi_layer;
};


class DB_PUBLIC CompoundRegionFilterOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionFilterOperationNode (PolygonFilterBase *filter, CompoundRegionOperationNode *input);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  virtual const TransformationReducer *vars () const { return mp_filter->vars (); }
  virtual bool wants_variants () const { return mp_filter->wants_variants (); }

private:
  PolygonFilterBase *mp_filter;

  bool is_selected (const db::Polygon &p) const;
  bool is_selected (const db::PolygonRef &p) const;

  template <class T>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (layout, interactions, one, max_vertex_count, area_ratio);

    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      if (is_selected (*p)) {
        results.front ().insert (*p);
      }
    }
  }
};


class DB_PUBLIC CompoundRegionProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionProcessingOperationNode (PolygonProcessorBase *proc, CompoundRegionOperationNode *input);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  PolygonProcessorBase *mp_proc;

  void processed (db::Layout *, const db::Polygon &p, std::vector<db::Polygon> &res) const;
  void processed (db::Layout *layout, const db::PolygonRef &p, std::vector<db::PolygonRef> &res) const;

  template <class T>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (layout, interactions, one, max_vertex_count, area_ratio);

    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      std::vector<T> res;
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

class DB_PUBLIC CompoundRegionToEdgeProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionToEdgeProcessingOperationNode (PolygonToEdgeProcessorBase *proc, CompoundRegionOperationNode *input);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  PolygonToEdgeProcessorBase *mp_proc;

  void processed (db::Layout *, const db::Polygon &p, std::vector<db::Edge> &res) const;
  void processed (db::Layout *layout, const db::PolygonRef &p, std::vector<db::Edge> &res) const;

  template <class T>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (layout, interactions, one, max_vertex_count, area_ratio);

    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      std::vector<db::Edge> res;
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

class DB_PUBLIC CompoundRegionToEdgePairProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionToEdgePairProcessingOperationNode (PolygonToEdgePairProcessorBase *proc, CompoundRegionOperationNode *input);

  virtual std::string description () const;

  //  specifies the result type
  virtual ResultType result_type () const { return EdgePairs; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  PolygonToEdgePairProcessorBase *mp_proc;

  void processed (db::Layout *, const db::Polygon &p, std::vector<db::EdgePair> &res) const;
  void processed (db::Layout *layout, const db::PolygonRef &p, std::vector<db::EdgePair> &res) const;

  template <class T>
  void implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (layout, interactions, one, max_vertex_count, area_ratio);

    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      std::vector<db::EdgePair> res;
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};


template <class TS, class TI, class TR>
class DB_PUBLIC compound_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  compound_local_operation<TS, TI, TR> (CompoundRegionOperationNode *node)
    : mp_node (node)
  { }

  virtual void compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
  {
    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const TS &subject_shape = interactions.subject_shape (i->first);

      shape_interactions<TS, TI> single_interactions;

      single_interactions.add_subject_shape (i->first, subject_shape);
      const std::vector<unsigned int> &intruders = single_interactions.intruders_for (i->first);
      for (typename std::vector<unsigned int>::const_iterator ii = intruders.begin (); ii != intruders.end (); ++ii) {
        const std::pair<unsigned int, TI> &is = interactions.intruder_shape (*ii);
        single_interactions.add_intruder_shape (*ii, is.first, is.second);
        single_interactions.add_interaction (i->first, *ii);
      }

      mp_node->compute_local (layout, single_interactions, results, max_vertex_count, area_ratio);

    }
  }

  virtual db::Coord dist () const;
  virtual typename local_operation<TS, TI, TR>::on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

  const TransformationReducer *vars () const { return mp_node->vars (); }
  bool wants_variants () const { return mp_node->wants_variants (); }

private:
  std::auto_ptr<CompoundRegionOperationNode> mp_node;
};

}

#endif
