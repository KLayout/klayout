
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

#ifndef HDR_dbCompoundOperation
#define HDR_dbCompoundOperation

#include "dbCommon.h"
#include "dbLocalOperation.h"
#include "dbHierProcessor.h"
#include "dbEdgeProcessor.h"
#include "dbRegionDelegate.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbRegionProcessors.h"
#include "dbRegionLocalOperations.h"
#include "dbLocalOperationUtils.h"
#include "dbTypes.h"

#include "gsiObject.h"
#include "tlObject.h"

#include <memory>

namespace db
{

inline db::Region *subject_regionptr () { return (db::Region *) 0; }
inline db::Region *foreign_regionptr () { return (db::Region *) 1; }
inline bool is_subject_regionptr (const db::Region *ptr) { return ptr == subject_regionptr () || ptr == foreign_regionptr (); }

class CompoundRegionOperationNode;

/**
 *  @brief A per-node cache for results of the compound operations
 *
 *  This cache is important to avoid duplicate evaluation of the same node in
 *  a diamond-graph structure of nodes.
 */
class DB_PUBLIC CompoundRegionOperationCache
{
public:
  template <class TR>
  std::pair<bool, std::vector<std::unordered_set<TR> > *> get (const CompoundRegionOperationNode *node)
  {
    bool valid = false;
    std::vector<std::unordered_set<TR> > *cache = 0;
    get_cache (cache, valid, node);
    return std::make_pair (valid, cache);
  }

private:
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<db::PolygonRef> > > m_cache_polyref;
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<db::Polygon> > > m_cache_poly;
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<db::Edge> > > m_cache_edge;
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<db::EdgePair> > > m_cache_edge_pair;

  template <class TR>
  void get_cache_generic (std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<TR> > > &caches, std::vector<std::unordered_set<TR> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)
  {
    typename std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<TR> > >::iterator c = caches.find (node);
    if (c != caches.end ()) {
      valid = true;
      cache_ptr = &c->second;
    } else {
      cache_ptr = &caches [node];
    }
  }

  void get_cache (std::vector<std::unordered_set<db::PolygonRef> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node) { get_cache_generic (m_cache_polyref, cache_ptr, valid, node); }
  void get_cache (std::vector<std::unordered_set<db::Polygon> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)    { get_cache_generic (m_cache_poly, cache_ptr, valid, node); }
  void get_cache (std::vector<std::unordered_set<db::Edge> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)       { get_cache_generic (m_cache_edge, cache_ptr, valid, node); }
  void get_cache (std::vector<std::unordered_set<db::EdgePair> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)   { get_cache_generic (m_cache_edge_pair, cache_ptr, valid, node); }
};

/**
 *  @brief A node of the compound operation tree
 *
 *  A compound operation if formed of a tree of basic operations. The root node
 *  will act as the main entrance and is fed into a local processor for performing
 *  the complex operation.
 *
 *  The tree nodes will
 */

class DB_PUBLIC CompoundRegionOperationNode
  : public gsi::ObjectBase, public tl::Object
{
public:
  enum  ResultType { Region, Edges, EdgePairs };

  CompoundRegionOperationNode ();
  virtual ~CompoundRegionOperationNode ();

  /**
   *  @brief Returns the description set with "set_description"
   */
  const std::string &raw_description () const
  {
    return m_description;
  }

  /**
   *  @brief Gets the interaction distance of this operation
   *  The returned value will be the minimum distance (set_dist) or the computed distance
   *  (compute_dist), whichever is larger.
   */
  db::Coord dist () const
  {
    return std::max (m_dist, computed_dist ());
  }

  /**
   *  @brief Sets a minmum distance
   */
  void set_dist (db::Coord d)
  {
    m_dist = d;
  }

  /**
   *  @brief Gets the description for this node
   */
  std::string description () const;

  /**
   *  @brief Sets the description for this node
   *  If no description is set, the generated description will be used.
   */
  void set_description (const std::string &d);

  /**
   *  @brief Gets a value indicating whether the result is definitely merged
   *  The default implementation is based on a heuristic analysis of the inputs.
   */
  virtual bool is_merged () const;

  /**
   *  @brief Returns a value indicating whether this node provides external inputs
   *  A node has external inputs if it has inputs and does not refer to the primary alone.
   */
  virtual bool has_external_inputs () const;

  /**
   *  @brief Returns a value giving a hint how to handle the case of empty intruders
   */
  //  NOTE: it's probably going to be difficult to compute a different value here:
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const { return OnEmptyIntruderHint::Ignore; }

  /**
   *  @brief Returns the inputs this node depends on
   *  The returned vector may use subject_regionptr() or forein_regionptr() as abstract values
   *  for the subject layer and the subject layer in "foreign" mode (asymmetric: other subjects are treated
   *  as different shapes).
   */
  virtual std::vector<db::Region *> inputs () const = 0;

  /**
   *  @brief Specifies the type of shapes this node delivers
   */
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

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  void compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  template <class T>
  bool compute_local_bool (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, const db::LocalProcessorBase *proc) const
  {
    if (result_type () == Region) {

      std::vector<std::unordered_set<T> > res;
      res.push_back (std::unordered_set<T> ());
      compute_local (cache, layout, cell, interactions, res, proc);
      return ! res.front ().empty ();

    } else if (result_type () == Edges) {

      std::vector<std::unordered_set<db::Edge> > res;
      res.push_back (std::unordered_set<db::Edge> ());
      compute_local (cache, layout, cell, interactions, res, proc);
      return ! res.front ().empty ();

    } else if (result_type () == EdgePairs) {

      std::vector<std::unordered_set<db::EdgePair> > res;
      res.push_back (std::unordered_set<db::EdgePair> ());
      compute_local (cache, layout, cell, interactions, res, proc);
      return ! res.front ().empty ();

    } else {
      return false;
    }
  }

protected:
  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

  virtual db::Coord computed_dist () const = 0;

  virtual std::string generated_description () const;

private:
  std::string m_description;
  db::Coord m_dist;

  template <class TS, class TI, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
  {
    //  TODO: confine caching to those nodes which need it.

    std::pair<bool, std::vector<std::unordered_set<TR> > *> cp = cache->get<TR> (this);

    if (! cp.first) {

      std::vector<std::unordered_set<TR> > uncached_results;
      uncached_results.resize (results.size ());

      do_compute_local (cache, layout, cell, interactions, uncached_results, proc);

      cp.second->swap (uncached_results);

    }

    tl_assert (results.size () == cp.second->size ());
    for (size_t r = 0; r < results.size (); ++r) {
      results[r].insert ((*cp.second)[r].begin (), (*cp.second)[r].end ());
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


class DB_PUBLIC CompoundRegionOperationEmptyNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationEmptyNode (ResultType type)
    : CompoundRegionOperationNode (), m_type (type)
  { }

  virtual ~CompoundRegionOperationEmptyNode ()
  { }

  virtual std::vector<db::Region *> inputs () const { return std::vector<db::Region *> (); }
  virtual db::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return m_type; }

private:
  ResultType m_type;
};


class DB_PUBLIC CompoundRegionOperationPrimaryNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationPrimaryNode ();
  virtual ~CompoundRegionOperationPrimaryNode ();

  virtual std::vector<db::Region *> inputs () const;
  virtual db::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
};

class DB_PUBLIC CompoundRegionOperationForeignNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationForeignNode ();
  virtual ~CompoundRegionOperationForeignNode ();

  virtual std::vector<db::Region *> inputs () const;
  virtual db::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
};

class DB_PUBLIC CompoundRegionOperationSecondaryNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationSecondaryNode (db::Region *input);
  virtual ~CompoundRegionOperationSecondaryNode ();

  virtual std::vector<db::Region *> inputs () const;
  virtual db::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

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

  //  NOTE: equality does not really matter here as we use it only on leaf reducers
  virtual bool equals (const TransformationReducer * /*other*/) const { return false; }

private:
  std::vector<const db::TransformationReducer *> m_vars;
};

class DB_PUBLIC CompoundRegionMultiInputOperationNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionMultiInputOperationNode (const std::vector<CompoundRegionOperationNode *> &children);
  CompoundRegionMultiInputOperationNode ();
  CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *child);
  CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);
  ~CompoundRegionMultiInputOperationNode ();

  virtual db::Coord computed_dist () const;
  virtual std::string generated_description () const;

  virtual std::vector<db::Region *> inputs () const
  {
    return m_inputs;
  }

  virtual const TransformationReducer *vars () const;
  virtual bool wants_variants () const;

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

      if (child (child_index)->on_empty_intruder_hint () == OnEmptyIntruderHint::Drop) {
        child_interactions.add_subject_shape (i->first, interactions.subject_shape (i->first));
      } else {
        //  this includes the subject-without-intruder "interaction"
        child_interactions.add_subject (i->first, interactions.subject_shape (i->first));
      }

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
    return (unsigned int) m_children.size ();
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
  enum LogicalOp { And, Or };

  CompoundRegionLogicalBoolOperationNode (LogicalOp op, bool invert, const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  LogicalOp m_op;
  bool m_invert;

  template <class T>
  void implement_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, const db::LocalProcessorBase *proc) const;
};


class DB_PUBLIC CompoundRegionCountFilterNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionCountFilterNode (CompoundRegionOperationNode *input, bool invert, size_t min_count, size_t max_count);
  ~CompoundRegionCountFilterNode ();

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const
  {
    return child (0)->result_type ();
  }

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

private:
  size_t m_min_count, m_max_count;
  bool m_invert;

  template <class T, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<TR> > one;
    one.push_back (std::unordered_set<TR> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    if ((one.front ().size () >= m_min_count && one.front ().size () < m_max_count) != m_invert) {
      results.front ().insert (one.front ().begin (), one.front ().end ());
    }
  }
};


class DB_PUBLIC CompoundRegionGeometricalBoolOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum  GeometricalOp { And, Not, Or, Xor };

  CompoundRegionGeometricalBoolOperationNode (GeometricalOp op, CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;
  virtual db::Coord computed_dist () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  GeometricalOp m_op;

  template <class T, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const;
  template <class T, class T1, class T2, class TR>
  void implement_bool (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const;
};

template <class TS, class TI, class TR>
class DB_PUBLIC compound_region_generic_operation_node
  : public CompoundRegionMultiInputOperationNode
{
public:
  /**
   *  @brief Constructor
   *  NOTE: the inputs must be given for subjects and one to many intruder layers.
   *  Input 0 is for the subject, input 1 for the first intruder etc.
   *  If original layers are required for input, CompoundRegionOperationPrimaryNode or
   *  CompoundRegionOperationSecondaryNode nodes need to be provided. If no derived node
   *  is requested for input, the input-less constructor may be used which is more efficient.
   */
  compound_region_generic_operation_node (const db::local_operation<TS, TI, TR> *op, const std::vector<CompoundRegionOperationNode *> &inputs, const db::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (inputs), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    //  .. nothing yet ..
  }

  compound_region_generic_operation_node (const db::local_operation<TS, TI, TR> *op, CompoundRegionOperationNode *input, const db::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (input), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    //  .. nothing yet ..
  }

  compound_region_generic_operation_node (const db::local_operation<TS, TI, TR> *op, CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, const db::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (a, b), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    //  .. nothing yet ..
  }

  virtual ResultType result_type () const { return compound_operation_type_traits<TR>::type (); }
  virtual const db::TransformationReducer *vars () const  { return mp_vars; }
  virtual bool wants_variants () const { return m_wants_variants; }

  virtual db::Coord computed_dist () const
  {
    return CompoundRegionMultiInputOperationNode::computed_dist () + m_op->dist ();
  }

  virtual std::vector<db::Region *> inputs () const
  {
    if (! m_inputs.empty ()) {
      return m_inputs;
    } else {
      return CompoundRegionMultiInputOperationNode::inputs ();
    }
  }

protected:
  compound_region_generic_operation_node (const db::local_operation<TS, TI, TR> *op, const std::vector<db::Region *> &inputs, const db::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (), m_op (op), mp_vars (vars), m_wants_variants (want_variants), m_inputs (inputs)
  {
    //  .. nothing yet ..
  }

  compound_region_generic_operation_node (const db::local_operation<TS, TI, TR> *op, db::Region *input, const db::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    m_inputs.push_back (input);
  }

  compound_region_generic_operation_node (const db::local_operation<TS, TI, TR> *op, db::Region *a, db::Region *b, const db::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    m_inputs.push_back (a);
    m_inputs.push_back (b);
  }

  template <class TTS, class TTI, class TTR>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<TTS, TTI> &interactions, std::vector<std::unordered_set<TTR> > &results, const db::LocalProcessorBase *proc) const;

private:
  const db::local_operation<TS, TI, TR> *m_op;
  const db::TransformationReducer *mp_vars;
  bool m_wants_variants;
  std::vector<db::Region *> m_inputs;
  //  required if the inner processor is a PolygonRef type while the outer interface needs Polygon
  db::Layout m_aux_layout;
};

class DB_PUBLIC CompoundRegionInteractOperationNode
  : public compound_region_generic_operation_node<db::Polygon, db::Polygon, db::Polygon>
{
public:
  CompoundRegionInteractOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, int mode, bool touching, bool inverse, size_t min_count = 0, size_t max_count = std::numeric_limits<size_t>::max ());
  CompoundRegionInteractOperationNode (db::Region *a, db::Region *b, int mode, bool touching, bool inverse, size_t min_count = 0, size_t max_count = std::numeric_limits<size_t>::max ());

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  typedef db::interacting_local_operation<db::Polygon, db::Polygon, db::Polygon> op_type;
  op_type m_op;
};

class DB_PUBLIC CompoundRegionInteractWithEdgeOperationNode
  : public compound_region_generic_operation_node<db::Polygon, db::Edge, db::Polygon>
{
public:
  CompoundRegionInteractWithEdgeOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, bool inverse, size_t min_count = 0, size_t max_count = std::numeric_limits<size_t>::max ());

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  db::interacting_with_edge_local_operation<db::Polygon, db::Edge, db::Polygon> m_op;
};

class DB_PUBLIC CompoundRegionPullOperationNode
  : public compound_region_generic_operation_node<db::Polygon, db::Polygon, db::Polygon>
{
public:
  CompoundRegionPullOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, int mode, bool touching);
  CompoundRegionPullOperationNode (db::Region *a, db::Region *b, int mode, bool touching);

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  db::pull_local_operation<db::Polygon, db::Polygon, db::Polygon> m_op;
};

class DB_PUBLIC CompoundRegionPullWithEdgeOperationNode
  : public compound_region_generic_operation_node<db::Polygon, db::Edge, db::Edge>
{
public:
  CompoundRegionPullWithEdgeOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  db::pull_with_edge_local_operation<db::Polygon, db::Edge, db::Edge> m_op;
};

/**
 *  @brief Implements the joining of results
 *
 *  This operator joins all inputs into a common output.
 *  The types of the inputs must be the same.
 */

class DB_PUBLIC CompoundRegionJoinOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionJoinOperationNode (const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

private:
  template <class T, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const;
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
  CompoundRegionLogicalCaseSelectOperationNode (const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

private:
  template <class T, class TR>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const;

  bool m_multi_layer;
};


class DB_PUBLIC CompoundRegionFilterOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionFilterOperationNode (PolygonFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter = false, bool sum_of_set = false);
  ~CompoundRegionFilterOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

  virtual const TransformationReducer *vars () const { return mp_filter->vars (); }
  virtual bool wants_variants () const { return mp_filter->wants_variants (); }

private:
  PolygonFilterBase *mp_filter;
  bool m_owns_filter;
  bool m_sum_of_set;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    if (m_sum_of_set) {
      if (mp_filter->selected_set (one.front ())) {
        results.front ().insert (one.front ().begin (), one.front ().end ());
      }
    } else {
      for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
        if (mp_filter->selected (*p)) {
          results.front ().insert (*p);
        }
      }
    }
  }
};

class DB_PUBLIC CompoundRegionEdgeFilterOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgeFilterOperationNode (EdgeFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter = false, bool sum_of = false);
  ~CompoundRegionEdgeFilterOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

  virtual const TransformationReducer *vars () const { return mp_filter->vars (); }
  virtual bool wants_variants () const { return mp_filter->wants_variants (); }

private:
  EdgeFilterBase *mp_filter;
  bool m_owns_filter;
  bool m_sum_of;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<db::Edge> > one;
    one.push_back (std::unordered_set<Edge> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    if (m_sum_of) {
      if (mp_filter->selected (one.front ())) {
        results.front ().insert (one.front ().begin (), one.front ().end ());
      }
    } else {
      for (typename std::unordered_set<db::Edge>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
        if (mp_filter->selected (*p)) {
          results.front ().insert (*p);
        }
      }
    }
  }
};

class DB_PUBLIC CompoundRegionEdgePairFilterOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgePairFilterOperationNode (EdgePairFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter = false);
  ~CompoundRegionEdgePairFilterOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return EdgePairs; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }

  virtual const TransformationReducer *vars () const { return mp_filter->vars (); }
  virtual bool wants_variants () const { return mp_filter->wants_variants (); }

private:
  EdgePairFilterBase *mp_filter;
  bool m_owns_filter;

  bool is_selected (const db::EdgePair &p) const;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<db::EdgePair> > one;
    one.push_back (std::unordered_set<EdgePair> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    for (typename std::unordered_set<db::EdgePair>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
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
  CompoundRegionProcessingOperationNode (PolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false, db::Coord dist_adder = 0);
  ~CompoundRegionProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual db::Coord computed_dist () const { return m_dist_adder + CompoundRegionMultiInputOperationNode::computed_dist (); }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  PolygonProcessorBase *mp_proc;
  bool m_owns_proc;
  db::Coord m_dist_adder;

  void processed (db::Layout *, const db::Polygon &p, std::vector<db::Polygon> &res) const;
  void processed (db::Layout *, const db::PolygonRef &p, std::vector<db::PolygonRef> &res) const;
  void processed (db::Layout *, const db::Polygon &p, const db::ICplxTrans &tr, std::vector<db::Polygon> &res) const;
  void processed (db::Layout *, const db::PolygonRef &p, const db::ICplxTrans &tr, std::vector<db::PolygonRef> &res) const;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    std::vector<T> res;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {

      res.clear ();

      if (proc->vars ()) {

        //  in the presence of variants, handle the object in top level space

        const db::ICplxTrans &tr = proc->vars ()->single_variant_transformation (cell->cell_index ());
        processed (layout, *p, tr, res);

      } else {

        processed (layout, *p, res);

      }

      results.front ().insert (res.begin (), res.end ());

    }
  }
};

class DB_PUBLIC CompoundRegionSizeOperationNode
  : public CompoundRegionProcessingOperationNode
{
public:
  CompoundRegionSizeOperationNode (db::Coord dx, db::Coord dy, unsigned int mode, CompoundRegionOperationNode *input)
    : CompoundRegionProcessingOperationNode (& m_proc, input), m_proc (dx, dy, mode)
  { }

  CompoundRegionSizeOperationNode (db::Coord d, unsigned int mode, CompoundRegionOperationNode *input)
    : CompoundRegionProcessingOperationNode (& m_proc, input), m_proc (d, d, mode)
  { }

  virtual std::string description () const
  {
    return std::string ("sized") + CompoundRegionProcessingOperationNode::generated_description ();
  }

private:
  db::PolygonSizer m_proc;
};

class DB_PUBLIC CompoundRegionMergeOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionMergeOperationNode (bool min_coherence, unsigned int min_wc, CompoundRegionOperationNode *input)
    : CompoundRegionMultiInputOperationNode (input), m_min_coherence (min_coherence), m_min_wc (min_wc)
  { }

  ~CompoundRegionMergeOperationNode () { }

  virtual ResultType result_type () const { return Region; }

  virtual std::string description () const
  {
    return std::string ("merged") + CompoundRegionMultiInputOperationNode::generated_description ();
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const
  {
    implement_compute_local (cache, layout, cell, interactions, results, proc);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  bool m_min_coherence;
  unsigned int m_min_wc;

  template <class T>
  void implement_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    db::EdgeProcessor ep;

    //  count edges and reserve memory
    size_t n = 0;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      ep.insert (*p, n);
      ++n;
    }

    //  and run the merge step
    db::MergeOp op (m_min_wc);
    db::polygon_ref_generator<T> pc (layout, results.front ());
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, m_min_coherence);
    ep.process (pg, op);
  }
};

class DB_PUBLIC CompoundRegionEdgeToPolygonProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgeToPolygonProcessingOperationNode (EdgeToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgeToPolygonProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  EdgeToPolygonProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (db::Layout *, const db::Edge &p, std::vector<db::Polygon> &res) const;
  void processed (db::Layout *layout, const db::Edge &p, std::vector<db::PolygonRef> &res) const;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<db::Edge> > one;
    one.push_back (std::unordered_set<db::Edge> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    std::vector<T> res;
    for (typename std::unordered_set<db::Edge>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {

      res.clear ();

      if (proc->vars ()) {

        //  in the presence of variants, handle the object in top level space

        const db::ICplxTrans &tr = proc->vars ()->single_variant_transformation (cell->cell_index ());
        processed (layout, tr * *p, res);

        if (! res.empty ()) {
          db::ICplxTrans tri = tr.inverted ();
          for (auto r = res.begin (); r != res.end (); ++r) {
            results.front ().insert (tri * *r);
          }
        }

      } else {

        processed (layout, *p, res);

        results.front ().insert (res.begin (), res.end ());

      }

    }
  }
};

class DB_PUBLIC CompoundRegionEdgeProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgeProcessingOperationNode (EdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgeProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  EdgeProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (db::Layout *, const db::Edge &p, std::vector<db::Edge> &res) const;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<db::Edge> > one;
    one.push_back (std::unordered_set<db::Edge> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    std::vector<db::Edge> res;
    for (typename std::unordered_set<db::Edge>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {

      res.clear ();

      if (proc->vars ()) {

        //  in the presence of variants, handle the object in top level space

        const db::ICplxTrans &tr = proc->vars ()->single_variant_transformation (cell->cell_index ());
        processed (layout, tr * *p, res);

        if (! res.empty ()) {
          db::ICplxTrans tri = tr.inverted ();
          for (auto r = res.begin (); r != res.end (); ++r) {
            results.front ().insert (tri * *r);
          }
        }

      } else {

        processed (layout, *p, res);

        results.front ().insert (res.begin (), res.end ());

      }

    }
  }
};

class DB_PUBLIC CompoundRegionEdgePairToPolygonProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgePairToPolygonProcessingOperationNode (EdgePairToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgePairToPolygonProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  EdgePairToPolygonProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (db::Layout *, const db::EdgePair &p, std::vector<db::Polygon> &res) const;
  void processed (db::Layout *layout, const db::EdgePair &p, std::vector<db::PolygonRef> &res) const;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<db::EdgePair> > one;
    one.push_back (std::unordered_set<db::EdgePair> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    std::vector<T> res;
    for (typename std::unordered_set<db::EdgePair>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {

      res.clear ();

      if (proc->vars ()) {

        //  in the presence of variants, handle the object in top level space

        const db::ICplxTrans &tr = proc->vars ()->single_variant_transformation (cell->cell_index ());
        processed (layout, tr * *p, res);

        if (! res.empty ()) {
          db::ICplxTrans tri = tr.inverted ();
          for (auto r = res.begin (); r != res.end (); ++r) {
            results.front ().insert (tri * *r);
          }
        }

      } else {

        processed (layout, *p, res);

        results.front ().insert (res.begin (), res.end ());

      }

    }
  }
};

class DB_PUBLIC CompoundRegionEdgePairToEdgeProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgePairToEdgeProcessingOperationNode (EdgePairToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgePairToEdgeProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  EdgePairToEdgeProcessorBase *mp_proc;
  bool m_owns_proc;

  template <class T>
  void implement_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<db::EdgePair> > one;
    one.push_back (std::unordered_set<db::EdgePair> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    std::vector<db::Edge> res;
    for (typename std::unordered_set<db::EdgePair>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {

      res.clear ();

      if (proc->vars ()) {

        //  in the presence of variants, handle the object in top level space

        const db::ICplxTrans &tr = proc->vars ()->single_variant_transformation (cell->cell_index ());
        mp_proc->process (tr * *p, res);

        if (! res.empty ()) {
          db::ICplxTrans tri = tr.inverted ();
          for (auto r = res.begin (); r != res.end (); ++r) {
            results.front ().insert (tri * *r);
          }
        }

      } else {

        mp_proc->process (*p, res);

        results.front ().insert (res.begin (), res.end ());

      }

    }
  }
};

class DB_PUBLIC CompoundRegionToEdgeProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionToEdgeProcessingOperationNode (PolygonToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionToEdgeProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::EdgePair> > & /*results*/,    const db::LocalProcessorBase * /*proc*/) const { }

private:
  PolygonToEdgeProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (db::Layout *, const db::Polygon &p, std::vector<db::Edge> &res) const;
  void processed (db::Layout *layout, const db::PolygonRef &p, std::vector<db::Edge> &res) const;
  void processed (db::Layout *, const db::Polygon &p, const db::ICplxTrans &tr, std::vector<db::Edge> &res) const;
  void processed (db::Layout *, const db::PolygonRef &p, const db::ICplxTrans &tr, std::vector<db::Edge> &res) const;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    std::vector<db::Edge> res;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {

      res.clear ();

      if (proc->vars ()) {

        //  in the presence of variants, handle the object in top level space

        const db::ICplxTrans &tr = proc->vars ()->single_variant_transformation (cell->cell_index ());
        processed (layout, *p, tr, res);

      } else {

        processed (layout, *p, res);

      }

      results.front ().insert (res.begin (), res.end ());

    }
  }
};

/**
 *  @brief A wrapper for a generic edge-pair producing node
 *
 *  This node takes polygons for input. It will produce edge pairs by means of the PolygonToEdgePairProcessorBase object.
 */
class DB_PUBLIC CompoundRegionToEdgePairProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  /**
   *  @brief Constructor
   *  @param proc The processor which turns polygons into edge pairs (it's reimplementation) - the node will *not* take ownership unless owns_proc is true
   *  @param input The node for the original (the node will take ownership)
   */
  CompoundRegionToEdgePairProcessingOperationNode (PolygonToEdgePairProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionToEdgePairProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return EdgePairs; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }

private:
  PolygonToEdgePairProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (db::Layout *, const db::Polygon &p, std::vector<db::EdgePair> &res) const;
  void processed (db::Layout *layout, const db::PolygonRef &p, std::vector<db::EdgePair> &res) const;
  void processed (db::Layout *, const db::Polygon &p, const db::ICplxTrans &tr, std::vector<db::EdgePair> &res) const;
  void processed (db::Layout *layout, const db::PolygonRef &p, const db::ICplxTrans &tr, std::vector<db::EdgePair> &res) const;

  template <class T>
  void implement_compute_local (db::CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, cell, interactions, one, proc);

    std::vector<db::EdgePair> res;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {

      res.clear ();

      if (proc->vars ()) {

        //  in the presence of variants, handle the object in top level space

        const db::ICplxTrans &tr = proc->vars ()->single_variant_transformation (cell->cell_index ());
        processed (layout, *p, tr, res);

      } else {

        processed (layout, *p, res);

      }

      results.front ().insert (res.begin (), res.end ());

    }
  }
};

/**
 *  @brief A wrapper for a generic DRC check node
 *
 *  This node takes polygons for input. It will produce edge pairs as result of the DRC check.
 */
class DB_PUBLIC CompoundRegionCheckOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  /**
   *  @brief Constructor
   *  @param input The node for the original (the node will take ownership)
   */
  CompoundRegionCheckOperationNode (db::CompoundRegionOperationNode *input, db::edge_relation_type rel, bool different_polygons, db::Coord d, const db::RegionCheckOptions &options);

  /**
   *  @brief Constructor for the two-layer check
   */
  CompoundRegionCheckOperationNode (db::CompoundRegionOperationNode *input, db::CompoundRegionOperationNode *other, db::edge_relation_type rel, bool different_polygons, db::Coord d, const db::RegionCheckOptions &options);

  /**
   *  @brief Constructor for a single-polygon check (width, notch)
   */
  CompoundRegionCheckOperationNode (db::edge_relation_type rel, bool different_polygons, db::Coord d, const db::RegionCheckOptions &options);

  //  specifies the result type
  virtual ResultType result_type () const { return EdgePairs; }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual db::Coord computed_dist () const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool wants_variants () const { return true; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const LocalProcessorBase *proc) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, db::Layout *layout, db::Cell *cell, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const LocalProcessorBase *proc) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Polygon> > & /*results*/,     const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Polygon, db::Polygon> & /*interactions*/,       std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::PolygonRef> > & /*results*/,  const db::LocalProcessorBase * /*proc*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<db::Edge> > & /*results*/,        const db::LocalProcessorBase * /*proc*/) const { }

private:
  db::EdgeRelationFilter m_check;
  bool m_different_polygons;
  db::RegionCheckOptions m_options;
  bool m_has_other;
  bool m_is_other_merged;
  db::MagnificationReducer m_vars;
};


/**
 *  @brief The generic local operation
 *
 *  This local operation executes the operation tree within a local processor.
 *  When put into a local processor, the operation tree will be executed on each interaction.
 */
template <class TS, class TI, class TR>
class DB_PUBLIC compound_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  /**
   *  @brief Constructor
   *
   *  Creates a local operation which utilizes the operation tree. "node" is the root of the operation tree.
   *  Ownership of the node is *not* transferred to the local operation.
   */
  compound_local_operation<TS, TI, TR> (CompoundRegionOperationNode *node)
    : mp_node (node)
  { }

  virtual const TransformationReducer *vars () const
  {
    return mp_node->vars ();
  }

protected:
  virtual void do_compute_local (db::Layout *layout, db::Cell *cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
  {
    CompoundRegionOperationCache cache;
    mp_node->compute_local (&cache, layout, cell, interactions, results, proc);
  }

  virtual db::Coord dist () const { return mp_node->dist (); }
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const { return mp_node->on_empty_intruder_hint (); }
  virtual bool requests_single_subjects () const { return true; }
  virtual std::string description () const { return mp_node->description (); }

  std::vector<db::Region *> inputs () const { return mp_node->inputs (); }

private:
  tl::weak_ptr<CompoundRegionOperationNode> mp_node;
};

/**
 *  @brief The generic local operation with property support
 *
 *  This local operation executes the operation tree within a local processor.
 *  When put into a local processor, the operation tree will be executed on each interaction.
 */
template <class TS, class TI, class TR>
class DB_PUBLIC compound_local_operation_with_properties
  : public local_operation<db::object_with_properties<TS>, db::object_with_properties<TI>, db::object_with_properties<TR> >
{
public:
  /**
   *  @brief Constructor
   *
   *  Creates a local operation which utilizes the operation tree. "node" is the root of the operation tree.
   *  Ownership of the node is *not* transferred to the local operation.
   */
  compound_local_operation_with_properties<TS, TI, TR> (CompoundRegionOperationNode *node, db::PropertyConstraint prop_constraint, db::PropertiesRepository *target_pr, const db::PropertiesRepository *subject_pr, const std::vector<const db::PropertiesRepository *> &intruder_prs)
    : mp_node (node), m_prop_constraint (prop_constraint), m_pms (target_pr, subject_pr)
  {
    m_pmis.reserve (intruder_prs.size ());
    for (auto i = intruder_prs.begin (); i != intruder_prs.end (); ++i) {
      m_pmis.push_back (db::PropertyMapper (target_pr, *i));
    }
  }

  virtual const TransformationReducer *vars () const
  {
    return mp_node->vars ();
  }

protected:
  virtual void do_compute_local (db::Layout *layout, db::Cell *cell, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::object_with_properties<TR> > > &results, const db::LocalProcessorBase *proc) const
  {
    auto interactions_by_prop_id = separate_interactions_to_interactions_by_properties (interactions, m_prop_constraint, m_pms, m_pmis);
    for (auto s2p = interactions_by_prop_id.begin (); s2p != interactions_by_prop_id.end (); ++s2p) {

      std::vector<std::unordered_set<TR> > results_wo_props;
      results_wo_props.resize (results.size ());

      CompoundRegionOperationCache cache;
      mp_node->compute_local (&cache, layout, cell, s2p->second, results_wo_props, proc);

      for (size_t n = 0; n < results.size (); ++n) {
        for (auto i = results_wo_props [n].begin (); i != results_wo_props [n].end (); ++i) {
          results [n].insert (db::object_with_properties<TR> (*i, pc_norm (m_prop_constraint, s2p->first)));
        }
      }

    }
  }

  virtual db::Coord dist () const { return mp_node->dist (); }
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const { return mp_node->on_empty_intruder_hint (); }
  virtual bool requests_single_subjects () const { return true; }
  virtual std::string description () const { return mp_node->description (); }

  std::vector<db::Region *> inputs () const { return mp_node->inputs (); }

private:
  tl::weak_ptr<CompoundRegionOperationNode> mp_node;
  db::PropertyConstraint m_prop_constraint;
  mutable db::PropertyMapper m_pms;
  mutable std::vector<db::PropertyMapper> m_pmis;
};

}

#endif
