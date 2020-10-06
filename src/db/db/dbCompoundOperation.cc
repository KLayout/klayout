
#include "dbCompoundOperation.h"
#include "dbRegion.h"

/*@@@@

TODO:

* Transform variants?
* "result is merged"?
* "requires raw input"?
* Make nodes shared pointers/GSI objects for better compatibility with GSI, at least "keep" them.

* Edge generation nodes
* Boolean and interaction nodes (interact, bool with edge, bool with polygon ...)

* Interactions with shapes over some distance for neighborhood analysis

* Sized subject shapes as inputs for other operations? how to compute distance then?

*/

namespace db
{

// ---------------------------------------------------------------------------------------------

void
CompoundRegionOperationNode::compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<db::Polygon> > intermediate;
  do_compute_local (layout, interactions, intermediate, max_vertex_count, area_ratio);

  tl_assert (layout != 0);
  for (std::vector<std::unordered_set<db::Polygon> >::const_iterator r = intermediate.begin (); r != intermediate.end (); ++r) {
    results.push_back (std::unordered_set<db::PolygonRef> ());
    for (std::unordered_set<db::Polygon>::const_iterator p = r->begin (); p != r->end (); ++p) {
      results.back ().insert (db::PolygonRef (*p, layout->shape_repository ()));
    }
  }
}

void
CompoundRegionOperationNode::compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<db::PolygonRef> > intermediate;
  do_compute_local (layout, interactions, intermediate, max_vertex_count, area_ratio);

  for (std::vector<std::unordered_set<db::PolygonRef> >::const_iterator r = intermediate.begin (); r != intermediate.end (); ++r) {
    results.push_back (std::unordered_set<db::Polygon> ());
    for (std::unordered_set<db::PolygonRef>::const_iterator p = r->begin (); p != r->end (); ++p) {
      results.back ().insert (p->obj ().transformed (p->trans ()));
    }
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationPrimaryNode::CompoundRegionOperationPrimaryNode ()
{
  //  .. nothing yet ..
}

std::vector<db::Region *> CompoundRegionOperationPrimaryNode::inputs () const
{
  return std::vector<db::Region *> ();
}

void CompoundRegionOperationPrimaryNode::do_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t, double) const
{
  for (shape_interactions<db::Polygon, db::Polygon>::subject_iterator i = interactions.begin_subjects (); i != interactions.begin_subjects (); ++i) {
    results.front ().insert (i->second);
  }
}

void CompoundRegionOperationPrimaryNode::do_compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t, double) const
{
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::subject_iterator i = interactions.begin_subjects (); i != interactions.begin_subjects (); ++i) {
    results.front ().insert (i->second);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationSecondaryNode::CompoundRegionOperationSecondaryNode (db::Region *input)
  : mp_input (input)
{
  //  .. nothing yet ..
}

std::vector<db::Region *> CompoundRegionOperationSecondaryNode::inputs () const
{
  std::vector<db::Region *> iv;
  iv.push_back (mp_input);
  return iv;
}

void CompoundRegionOperationSecondaryNode::do_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t, double) const
{
  for (shape_interactions<db::Polygon, db::Polygon>::intruder_iterator i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    results.front ().insert (i->second.second);
  }
}

void CompoundRegionOperationSecondaryNode::do_compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t, double) const
{
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::intruder_iterator i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    results.front ().insert (i->second.second);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundTransformationReducer::CompoundTransformationReducer ()
{
  //  .. nothing yet ..
}

void
CompoundTransformationReducer::add (const db::TransformationReducer *reducer)
{
  if (reducer) {
    m_vars.push_back (reducer);
  }
}

db::Trans
CompoundTransformationReducer::reduce_trans (const db::Trans &trans) const
{
  db::Trans t = trans;
  for (std::vector<const db::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce_trans (t);
  }
  return t;
}

db::ICplxTrans
CompoundTransformationReducer::reduce_trans (const db::ICplxTrans &trans) const
{
  db::ICplxTrans t = trans;
  for (std::vector<const db::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce_trans (t);
  }
  return t;
}

db::Trans
CompoundTransformationReducer::reduce (const db::Trans &trans) const
{
  db::Trans t = trans;
  for (std::vector<const db::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce (t);
  }
  return t;
}

db::ICplxTrans
CompoundTransformationReducer::reduce (const db::ICplxTrans &trans) const
{
  db::ICplxTrans t = trans;
  for (std::vector<const db::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce (t);
  }
  return t;
}

bool
CompoundTransformationReducer::is_translation_invariant () const
{
  for (std::vector<const db::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    if (! (*v)->is_translation_invariant ()) {
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------------------------

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode (const std::vector<CompoundRegionOperationNode *> &children)
{
  for (std::vector<CompoundRegionOperationNode *>::const_iterator c = children.begin (); c != children.end (); ++c) {
    m_children.push_back (*c);
  }
  init ();
}

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode ()
{
  init ();
}

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *child)
{
  m_children.push_back (child);
  init ();
}

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b)
{
  m_children.push_back (a);
  m_children.push_back (b);
  init ();
}

void
CompoundRegionMultiInputOperationNode::init ()
{
  std::map<db::Region *, unsigned int> input_index;

  for (tl::shared_collection<CompoundRegionOperationNode>::iterator i = m_children.begin (); i != m_children.end (); ++i) {

    std::vector<db::Region *> child_inputs = i->inputs ();
    unsigned int child_index = 0;
    for (std::vector<db::Region *>::const_iterator ii = child_inputs.begin (); ii != child_inputs.end (); ++ii, ++child_index) {

      std::map<db::Region *, unsigned int>::const_iterator im = input_index.find (*ii);
      unsigned int li = (unsigned int) m_inputs.size ();
      if (im != input_index.end ()) {
        li = im->second;
      } else {
        m_inputs.push_back (*ii);
        input_index.insert (std::make_pair (*ii, li));
      }

      m_map_layer_to_child [std::make_pair (child_index, (unsigned int) (ii - child_inputs.begin ()))] = li;

    }

  }

  //  build the reducer
  for (tl::shared_collection<CompoundRegionOperationNode>::iterator i = m_children.begin (); i != m_children.end (); ++i) {
    m_vars.add (i->vars ());
  }
}

CompoundRegionMultiInputOperationNode::~CompoundRegionMultiInputOperationNode ()
{
  for (tl::shared_collection<CompoundRegionOperationNode>::iterator i = m_children.begin (); i != m_children.end (); ++i) {
    delete i.operator-> ();
  }
  m_children.clear ();
}

void
CompoundRegionMultiInputOperationNode::invalidate_cache () const
{
  for (tl::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    i->invalidate_cache ();
  }
}

db::Coord
CompoundRegionMultiInputOperationNode::dist () const
{
  db::Coord d = 0;
  for (tl::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    d = std::max (d, i->dist ());
  }
  return d;
}

std::string
CompoundRegionMultiInputOperationNode::description () const
{
  std::string r = "(";
  for (tl::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    if (i != m_children.begin ()) {
      r += ",";
    }
    r += i->description ();
  }
  return r;
}

CompoundRegionOperationNode *
CompoundRegionMultiInputOperationNode::child (unsigned int index)
{
  tl::shared_collection<CompoundRegionOperationNode>::iterator c = m_children.begin ();
  while (c != m_children.end () && index > 0) {
    ++c;
    --index;
  }
  return c == m_children.end () ? 0 : c.operator-> ();
}

const CompoundRegionOperationNode *
CompoundRegionMultiInputOperationNode::child (unsigned int index) const
{
  return const_cast<CompoundRegionMultiInputOperationNode *> (this)->child (index);
}

const TransformationReducer *
CompoundRegionMultiInputOperationNode::vars () const
{
  return (m_vars.is_empty () ? &m_vars : 0);
}

bool
CompoundRegionMultiInputOperationNode::wants_variants () const
{
  for (tl::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    if (i->wants_variants ()) {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------------------------

CompoundRegionLogicalBoolOperationNode::CompoundRegionLogicalBoolOperationNode (LogicalOp op, bool invert, const std::vector<CompoundRegionOperationNode *> &inputs)
  : CompoundRegionMultiInputOperationNode (inputs), m_op (op), m_invert (invert)
{
  //  .. nothing yet ..
}

std::string CompoundRegionLogicalBoolOperationNode::description () const
{
  std::string r;
  if (m_invert) {
    r = "!";
  }
  if (m_op == And) {
    r += "and";
  } else if (m_op == Or) {
    r += "or";
  }
  return r + CompoundRegionMultiInputOperationNode::description ();
}

template <class T>
void CompoundRegionLogicalBoolOperationNode::implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
{
  bool ok = (m_op == And ? true : false);

  const T &subject_shape = interactions.subject_shape (interactions.begin ()->first);

  for (unsigned int ci = 0; ci < children (); ++ci) {

    shape_interactions<T, T> child_interactions_computed;
    const shape_interactions<T, T> &child_interactions = interactions_for_child<T, T> (interactions, ci, child_interactions_computed);

    const CompoundRegionOperationNode *node = child (ci);

    bool any = node->compute_local_bool<T> (layout, child_interactions, max_vertex_count, area_ratio);

    if (m_op == And) {
      if (! any) {
        ok = false;
        break;
      }
    } else if (m_op == Or) {
      if (any) {
        ok = true;
        break;
      }
    }

  }

  if (ok) {
    tl_assert (! results.empty ());
    results.front ().insert (subject_shape);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionGeometricalBoolOperationNode::CompoundRegionGeometricalBoolOperationNode (GeometricalOp op, CompoundRegionOperationNode *a, CompoundRegionOperationNode *b)
  : CompoundRegionMultiInputOperationNode (a, b), m_op (op)
{
  //  .. nothing yet ..
}

std::string
CompoundRegionGeometricalBoolOperationNode::description () const
{
  std::string r;
  if (m_op == And) {
    r = "and";
  } else if (m_op == Or) {
    r = "or";
  } else if (m_op == Xor) {
    r = "xor";
  } else if (m_op == Not) {
    r = "not";
  }
  r += CompoundRegionMultiInputOperationNode::description ();
  return r;
}

CompoundRegionGeometricalBoolOperationNode::ResultType
CompoundRegionGeometricalBoolOperationNode::result_type () const
{
  ResultType res_a = child (0)->result_type ();
  ResultType res_b = child (1)->result_type ();

  if (res_a == Region && res_b == Edges && m_op == And) {
    return Edges;
  } else {
    return res_a;
  }
}

template <class T1, class T2, class TR>
static
void run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp, db::Layout *, const std::unordered_set<T1> &, const std::unordered_set<T2> &, const std::unordered_set<TR> &)
{
  tl_assert (false);
}

static void
init_region (db::Region &r, const std::unordered_set<db::PolygonRef> &p)
{
  for (std::unordered_set<db::PolygonRef>::const_iterator i = p.begin (); i != p.end (); ++i) {
    r.insert (i->obj ().transformed (i->trans ()));
  }
}

static void
init_region (db::Region &r, const std::unordered_set<db::Polygon> &p)
{
  for (std::unordered_set<db::Polygon>::const_iterator i = p.begin (); i != p.end (); ++i) {
    r.insert (*i);
  }
}

static void
init_edges (db::Edges &ee, const std::unordered_set<db::Edge> &e)
{
  for (std::unordered_set<db::Edge>::const_iterator i = e.begin (); i != e.end (); ++i) {
    ee.insert (*i);
  }
}

static void
write_result (db::Layout *layout, std::unordered_set<db::PolygonRef> &results, const db::Region &r)
{
  for (db::Region::const_iterator p = r.begin (); ! p.at_end (); ++p) {
    results.insert (db::PolygonRef (*p, layout->shape_repository ()));
  }
}

static void
write_result (db::Layout *, std::unordered_set<db::Polygon> &results, const db::Region &r)
{
  for (db::Region::const_iterator p = r.begin (); ! p.at_end (); ++p) {
    results.insert (*p);
  }
}

static void
write_result (db::Layout *, std::unordered_set<db::Edge> &results, const db::Edges &e)
{
  for (db::Edges::const_iterator i = e.begin (); ! i.at_end (); ++i) {
    results.insert (*i);
  }
}

template <class T>
static void
run_poly_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<T> &a, const std::unordered_set<T> &b, std::unordered_set<T> &res)
{
  //  TODO: it's more efficient to feed the EP directly for polygon-to-polygon bools
  db::Region ra, rb;
  init_region (ra, a);
  init_region (rb, b);

  if (op == CompoundRegionGeometricalBoolOperationNode::And) {
    write_result (layout, res, ra & rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Or) {
    write_result (layout, res, ra + rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Xor) {
    write_result (layout, res, ra ^ rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Not) {
    write_result (layout, res, ra - rb);
  }
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::PolygonRef> &a, const std::unordered_set<db::PolygonRef> &b, std::unordered_set<db::PolygonRef> &res)
{
  run_poly_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::Polygon> &a, const std::unordered_set<db::Polygon> &b, std::unordered_set<db::Polygon> &res)
{
  run_poly_bool (op, layout, a, b, res);
}

template <class T>
static void
run_poly_vs_edge_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<T> &a, const std::unordered_set<db::Edge> &b, std::unordered_set<db::Edge> &res)
{
  if (op != CompoundRegionGeometricalBoolOperationNode::And) {
    return;
  }

  //  TODO: it's more efficient to feed the EP directly for polygon-to-polygon bools
  db::Region ra;
  init_region (ra, a);

  db::Edges eb;
  init_edges (eb, b);

  write_result (layout, res, eb & ra);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::Polygon> &a, const std::unordered_set<db::Edge> &b, std::unordered_set<db::Edge> &res)
{
  run_poly_vs_edge_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::PolygonRef> &a, const std::unordered_set<db::Edge> &b, std::unordered_set<db::Edge> &res)
{
  run_poly_vs_edge_bool (op, layout, a, b, res);
}

template <class T>
static void
run_edge_vs_poly_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::Edge> &a, const std::unordered_set<T> &b, std::unordered_set<db::Edge> &res)
{
  if (op != CompoundRegionGeometricalBoolOperationNode::And) {
    return;
  }

  //  TODO: it's more efficient to feed the EP directly for polygon-to-polygon bools
  db::Region rb;
  init_region (rb, b);

  db::Edges ea;
  init_edges (ea, a);

  if (op != CompoundRegionGeometricalBoolOperationNode::And) {
    write_result (layout, res, ea & rb);
  } else if (op != CompoundRegionGeometricalBoolOperationNode::Not) {
    write_result (layout, res, ea - rb);
  }
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::Edge> &a, const std::unordered_set<db::PolygonRef> &b, std::unordered_set<db::Edge> &res)
{
  run_edge_vs_poly_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::Edge> &a, const std::unordered_set<db::Polygon> &b, std::unordered_set<db::Edge> &res)
{
  run_edge_vs_poly_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, db::Layout *layout, const std::unordered_set<db::Edge> &a, const std::unordered_set<db::Edge> &b, std::unordered_set<db::Edge> &res)
{
  db::Edges ea, eb;
  init_edges (ea, a);
  init_edges (eb, b);

  if (op == CompoundRegionGeometricalBoolOperationNode::And) {
    write_result (layout, res, ea & eb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Or) {
    write_result (layout, res, ea + eb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Xor) {
    write_result (layout, res, ea ^ eb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Not) {
    write_result (layout, res, ea - eb);
  }
}


template <class T, class T1, class T2, class TR>
void
CompoundRegionGeometricalBoolOperationNode::implement_bool (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<T1> > one_a;
  one_a.push_back (std::unordered_set<T1> ());

  shape_interactions<T, T> computed_a;
  child (0)->compute_local (layout, interactions_for_child (interactions, 0, computed_a), one_a, max_vertex_count, area_ratio);

  std::vector<std::unordered_set<T2> > one_b;
  one_b.push_back (std::unordered_set<T2> ());

  shape_interactions<T, T> computed_b;
  child (1)->compute_local (layout, interactions_for_child (interactions, 1, computed_b), one_b, max_vertex_count, area_ratio);

  run_bool (m_op, layout, one_a.front (), one_b.front (), results.front ());
}

template <class T, class TR>
void
CompoundRegionGeometricalBoolOperationNode::implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  ResultType res_a = child (0)->result_type ();
  ResultType res_b = child (1)->result_type ();

  if (res_a == Region && res_b == Region) {
    implement_bool<T, T, T, TR> (layout, interactions, results, max_vertex_count, area_ratio);
  } else if (res_a == Region && res_b == Edges) {
    implement_bool<T, T, db::Edge, TR> (layout, interactions, results, max_vertex_count, area_ratio);
  } else if (res_a == Edges && res_b == Region) {
    implement_bool<T, db::Edge, T, TR> (layout, interactions, results, max_vertex_count, area_ratio);
  } else if (res_a == Edges && res_b == Edges) {
    implement_bool<T, db::Edge, db::Edge, TR> (layout, interactions, results, max_vertex_count, area_ratio);
  }
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}



#if 0 // @@@
// ---------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------

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
#endif

// ---------------------------------------------------------------------------------------------

CompoundRegionLogicalCaseSelectOperationNode::CompoundRegionLogicalCaseSelectOperationNode (bool multi_layer, const std::vector<CompoundRegionOperationNode *> &inputs)
  : CompoundRegionMultiInputOperationNode (inputs), m_multi_layer (multi_layer)
{
  //  .. nothing yet ..
}

std::string CompoundRegionLogicalCaseSelectOperationNode::description () const
{
  //  TODO: could be nicer ...
  std::string r;
  r = "if-then";
  return r + CompoundRegionMultiInputOperationNode::description ();
}

template <class T>
void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
{
  bool ok = false;

  for (unsigned int ci = 0; ci < children (); ++ci) {

    if (! ok && ci % 2 == 1) {
      //  skip false branches
      continue;
    }

    shape_interactions<T, T> computed;
    const shape_interactions<T, T> &child_interactions = interactions_for_child<T, T> (interactions, ci, computed);

    const CompoundRegionOperationNode *node = child (ci);

    if (ci % 2 == 0) {

      ok = node->compute_local_bool<T> (layout, child_interactions, max_vertex_count, area_ratio);

    } else {

      if (m_multi_layer && results.size () > size_t (ci / 2)) {

        std::vector<std::unordered_set<T> > one;
        one.push_back (std::unordered_set<T> ());
        node->compute_local (layout, child_interactions, one, max_vertex_count, area_ratio);
        results[ci / 2].swap (one.front ());

      } else {

        node->compute_local (layout, child_interactions, results, max_vertex_count, area_ratio);

      }

      break;

    }

  }

}

// ---------------------------------------------------------------------------------------------

CompoundRegionFilterOperationNode::CompoundRegionFilterOperationNode (PolygonFilterBase *filter, CompoundRegionOperationNode *input)
  : CompoundRegionMultiInputOperationNode (input), mp_filter (filter)
{ }

std::string
CompoundRegionFilterOperationNode::description () const
{
  //  TODO: some description?
  return "filter";
}

void
CompoundRegionFilterOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionFilterOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

bool
CompoundRegionFilterOperationNode::is_selected (const db::Polygon &p) const
{
  return mp_filter->selected (p);
}

bool
CompoundRegionFilterOperationNode::is_selected (const db::PolygonRef &p) const
{
  return mp_filter->selected (p.obj ().transformed (p.trans ()));
}

// ---------------------------------------------------------------------------------------------

CompoundRegionProcessingOperationNode::CompoundRegionProcessingOperationNode (PolygonProcessorBase *proc, CompoundRegionOperationNode *input)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc)
{ }

std::string
CompoundRegionProcessingOperationNode::description () const
{
  //  TODO: some description?
  return "processor";
}

void
CompoundRegionProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionProcessingOperationNode::processed (db::Layout *, const db::Polygon &p, std::vector<db::Polygon> &res) const
{
  return mp_proc->process (p, res);
}

void
CompoundRegionProcessingOperationNode::processed (db::Layout *layout, const db::PolygonRef &p, std::vector<db::PolygonRef> &res) const
{
  std::vector<db::Polygon> poly;
  mp_proc->process (p.obj ().transformed (p.trans ()), poly);
  for (std::vector<db::Polygon>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    res.push_back (db::PolygonRef (*p, layout->shape_repository ()));
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionToEdgeProcessingOperationNode::CompoundRegionToEdgeProcessingOperationNode (PolygonToEdgeProcessorBase *proc, CompoundRegionOperationNode *input)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc)
{ }

std::string
CompoundRegionToEdgeProcessingOperationNode::description () const
{
  //  TODO: some description?
  return "processor";
}

void
CompoundRegionToEdgeProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgeProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgeProcessingOperationNode::processed (db::Layout *, const db::Polygon &p, std::vector<db::Edge> &res) const
{
  return mp_proc->process (p, res);
}

void
CompoundRegionToEdgeProcessingOperationNode::processed (db::Layout *, const db::PolygonRef &p, std::vector<db::Edge> &res) const
{
  mp_proc->process (p.obj ().transformed (p.trans ()), res);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionToEdgePairProcessingOperationNode::CompoundRegionToEdgePairProcessingOperationNode (PolygonToEdgePairProcessorBase *proc, CompoundRegionOperationNode *input)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc)
{ }

std::string
CompoundRegionToEdgePairProcessingOperationNode::description () const
{
  //  TODO: some description?
  return "processor";
}

void
CompoundRegionToEdgePairProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgePairProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgePairProcessingOperationNode::processed (db::Layout *, const db::Polygon &p, std::vector<db::EdgePair> &res) const
{
  return mp_proc->process (p, res);
}

void
CompoundRegionToEdgePairProcessingOperationNode::processed (db::Layout *, const db::PolygonRef &p, std::vector<db::EdgePair> &res) const
{
  mp_proc->process (p.obj ().transformed (p.trans ()), res);
}

}
