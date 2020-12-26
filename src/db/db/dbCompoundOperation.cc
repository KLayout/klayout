
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

#include "dbCompoundOperation.h"
#include "dbRegion.h"

/*@@@@

TODO:

* Transform variants?
* "result is merged"?
* "requires raw input"?

* edge pair to edge generation nodes (first, second)

* Interactions with shapes over some distance for neighborhood analysis

* Sized subject shapes as inputs for other operations? how to compute distance then?

* how do the logical boolean ops work?
* what is the "multi_input" for case nodes?

*/

namespace db
{

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationNode::CompoundRegionOperationNode ()
{
  invalidate_cache ();
}

CompoundRegionOperationNode::~CompoundRegionOperationNode ()
{
  // .. nothing yet ..
}

std::string CompoundRegionOperationNode::description() const
{
  if (m_description.empty ()) {
    return generated_description ();
  } else {
    return m_description;
  }
}

void
CompoundRegionOperationNode::set_description (const std::string &d)
{
  m_description = d;
}

std::string
CompoundRegionOperationNode::generated_description () const
{
  return std::string ();
}

// ---------------------------------------------------------------------------------------------

static void translate (db::Layout *layout, const std::vector<std::unordered_set<db::Polygon> > &in, std::vector<std::unordered_set<db::PolygonRef> > &out)
{
  tl_assert (layout != 0);
  for (std::vector<std::unordered_set<db::Polygon> >::const_iterator r = in.begin (); r != in.end (); ++r) {
    out.push_back (std::unordered_set<db::PolygonRef> ());
    for (std::unordered_set<db::Polygon>::const_iterator p = r->begin (); p != r->end (); ++p) {
      out.back ().insert (db::PolygonRef (*p, layout->shape_repository ()));
    }
  }
}

static void translate (db::Layout *, const std::vector<std::unordered_set<db::PolygonRef> > &in, std::vector<std::unordered_set<db::Polygon> > &out)
{
  for (std::vector<std::unordered_set<db::PolygonRef> >::const_iterator r = in.begin (); r != in.end (); ++r) {
    out.push_back (std::unordered_set<db::Polygon> ());
    for (std::unordered_set<db::PolygonRef>::const_iterator p = r->begin (); p != r->end (); ++p) {
      out.back ().insert (p->obj ().transformed (p->trans ()));
    }
  }
}

void
CompoundRegionOperationNode::compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<db::Polygon> > intermediate;
  implement_compute_local (layout, interactions, intermediate, max_vertex_count, area_ratio);
  translate (layout, intermediate, results);
}

void
CompoundRegionOperationNode::compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<db::PolygonRef> > intermediate;
  implement_compute_local (layout, interactions, intermediate, max_vertex_count, area_ratio);
  translate (layout, intermediate, results);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationPrimaryNode::CompoundRegionOperationPrimaryNode ()
{
  set_description (std::string ("this"));
}

CompoundRegionOperationPrimaryNode::~CompoundRegionOperationPrimaryNode ()
{
  //  .. nothing yet ..
}

std::vector<db::Region *> CompoundRegionOperationPrimaryNode::inputs () const
{
  std::vector<db::Region *> is;
  is.push_back (0);
  return is;
}

void CompoundRegionOperationPrimaryNode::do_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t, double) const
{
  for (shape_interactions<db::Polygon, db::Polygon>::subject_iterator i = interactions.begin_subjects (); i != interactions.end_subjects (); ++i) {
    results.front ().insert (i->second);
  }
}

void CompoundRegionOperationPrimaryNode::do_compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t, double) const
{
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::subject_iterator i = interactions.begin_subjects (); i != interactions.end_subjects (); ++i) {
    results.front ().insert (i->second);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationSecondaryNode::CompoundRegionOperationSecondaryNode (db::Region *input)
  : mp_input (input)
{
  set_description ("other");
}

CompoundRegionOperationSecondaryNode::~CompoundRegionOperationSecondaryNode ()
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
    (*c)->keep ();
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
  child->keep ();
  m_children.push_back (child);
  init ();
}

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b)
{
  a->keep ();
  m_children.push_back (a);
  b->keep ();
  m_children.push_back (b);
  init ();
}

void
CompoundRegionMultiInputOperationNode::init ()
{
  std::map<db::Region *, unsigned int> input_index;

  unsigned int child_index = 0;
  for (tl::shared_collection<CompoundRegionOperationNode>::iterator i = m_children.begin (); i != m_children.end (); ++i, ++child_index) {

    std::vector<db::Region *> child_inputs = i->inputs ();
    for (std::vector<db::Region *>::const_iterator ii = child_inputs.begin (); ii != child_inputs.end (); ++ii) {

      std::map<db::Region *, unsigned int>::const_iterator im = input_index.find (*ii);
      unsigned int li = (unsigned int) m_inputs.size ();
      if (im != input_index.end ()) {
        li = im->second;
      } else {
        m_inputs.push_back (*ii);
        input_index.insert (std::make_pair (*ii, li));
      }

      m_map_layer_to_child [std::make_pair (child_index, li)] = (unsigned int) (ii - child_inputs.begin ());

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
CompoundRegionMultiInputOperationNode::generated_description () const
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

bool
CompoundRegionMultiInputOperationNode::wants_merged () const
{
  for (tl::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    if (i->wants_merged ()) {
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

std::string CompoundRegionLogicalBoolOperationNode::generated_description () const
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

  if (m_invert) {
    ok = ! ok;
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
CompoundRegionGeometricalBoolOperationNode::generated_description () const
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

  if (res_a == Edges || (res_a == Region && res_b == Edges && m_op == And)) {
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
  if (op != CompoundRegionGeometricalBoolOperationNode::And && op != CompoundRegionGeometricalBoolOperationNode::Not) {
    return;
  }

  //  TODO: it's more efficient to feed the EP directly for polygon-to-polygon bools
  db::Region rb;
  init_region (rb, b);

  db::Edges ea;
  init_edges (ea, a);

  if (op == CompoundRegionGeometricalBoolOperationNode::And) {
    write_result (layout, res, ea & rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Not) {
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

// ---------------------------------------------------------------------------------------------

namespace
{
  template <class T>
  struct generic_result_adaptor
  {
  public:
    generic_result_adaptor<T> (std::vector<std::unordered_set<T> > *results)
      : mp_results (results)
    {
      m_intermediate.reserve (results->size ());
      for (size_t i = 0; i < results->size (); ++i) {
        m_shapes.push_back (db::Shapes ());
        m_intermediate.push_back (&m_shapes.back ());
      }
    }

    static void insert (db::Layout *, const db::Shape &shape, std::unordered_set<db::Edge> &result)
    {
      result.insert (shape.edge ());
    }

    static void insert (db::Layout *, const db::Shape &shape, std::unordered_set<db::EdgePair> &result)
    {
      result.insert (shape.edge_pair ());
    }

    static void insert (db::Layout *, const db::Shape &shape, std::unordered_set<db::Polygon> &result)
    {
      db::Polygon p;
      shape.polygon (p);
      result.insert (p);
    }

    static void insert (db::Layout *layout, const db::Shape &shape, std::unordered_set<db::PolygonRef> &result)
    {
      db::Polygon p;
      shape.polygon (p);
      result.insert (db::PolygonRef (p, layout->shape_repository ()));
    }

    const std::vector<db::Shapes *> &results ()
    {
      return m_intermediate;
    }

    void finish (db::Layout *layout)
    {
      for (size_t i = 0; i < m_intermediate.size (); ++i) {
        for (db::Shapes::shape_iterator s = m_intermediate [i]->begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
          insert (layout, *s, (*mp_results)[i]);
        }
      }
    }

  private:
    std::vector<std::unordered_set<T> > *mp_results;
    std::vector<db::Shapes *> m_intermediate;
    std::list<db::Shapes> m_shapes;
  };

}

template <class TS, class TI, class TR>
template <class TTS, class TTI, class TTR>
void compound_region_generic_operation_node<TS, TI, TR>::implement_compute_local (db::Layout *layout, const shape_interactions<TTS, TTI> &interactions, std::vector<std::unordered_set<TTR> > &results, size_t max_vertex_count, double area_ratio) const
{
  generic_result_adaptor <TTR> adaptor (&results);

  if (! layout) {
    layout = const_cast<db::Layout *> (&m_aux_layout);
  }

  shape_interactions<TS, TI> internal;

  const CompoundRegionOperationNode *self = child (0);
  std::vector<std::unordered_set<TS> > self_result;
  self_result.push_back (std::unordered_set<TS> ());

  shape_interactions<TTS, TTI> self_interactions_heap;
  const shape_interactions<TTS, TTI> &self_interactions = interactions_for_child (interactions, 0, self_interactions_heap);

  self->compute_local (layout, self_interactions, self_result, max_vertex_count, area_ratio);

  db::generic_shape_iterator <TS> is (self_result.front ().begin (), self_result.front ().end ());

  std::vector<db::generic_shape_iterator<TI> > iiv;
  std::vector<std::unordered_set<TI> > intruder_results;
  intruder_results.reserve (children () - 1);  //  important, so that the memory layout will not change while we generate them

  for (unsigned int ci = 1; ci < children (); ++ci) {

    const CompoundRegionOperationNode *intruder = child (ci);
    std::vector<std::unordered_set<TI> > intruder_result;
    intruder_result.push_back (std::unordered_set<TI> ());

    shape_interactions<TTS, TTI> intruder_interactions_heap;
    const shape_interactions<TTS, TTI> &intruder_interactions = interactions_for_child (interactions, ci, intruder_interactions_heap);

    intruder->compute_local (layout, intruder_interactions, intruder_result, max_vertex_count, area_ratio);

    intruder_results.push_back (std::unordered_set<TI> ());
    intruder_results.back ().swap (intruder_result.front ());

    iiv.push_back (db::generic_shape_iterator<TI> (intruder_results.back ().begin (), intruder_results.back ().end ()));

  }

  db::local_processor <TS, TI, TR> proc (layout);
  proc.run_flat (is, iiv, m_op, adaptor.results ());

  adaptor.finish (layout);
}

//  explicit instantiations
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Polygon, db::Polygon>::implement_compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::PolygonRef> &, std::vector<std::unordered_set<db::PolygonRef> > &, size_t, double) const;
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Polygon, db::Polygon>::implement_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &, std::vector<std::unordered_set<db::Polygon> > &, size_t, double) const;
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Polygon, db::Polygon>::implement_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &, std::vector<std::unordered_set<db::Edge> > &, size_t, double) const;
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Polygon, db::Polygon>::implement_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &, std::vector<std::unordered_set<db::EdgePair> > &, size_t, double) const;
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Edge, db::Polygon>::implement_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &, std::vector<std::unordered_set<db::Polygon> > &, size_t, double) const;
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Edge, db::Polygon>::implement_compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::PolygonRef> &, std::vector<std::unordered_set<db::PolygonRef> > &, size_t, double) const;
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Edge, db::Edge>::implement_compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::PolygonRef> &, std::vector<std::unordered_set<db::Edge> > &, size_t, double) const;
template DB_PUBLIC void compound_region_generic_operation_node<db::Polygon, db::Edge, db::Edge>::implement_compute_local (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &, std::vector<std::unordered_set<db::Edge> > &, size_t, double) const;

// ---------------------------------------------------------------------------------------------

CompoundRegionLogicalCaseSelectOperationNode::CompoundRegionLogicalCaseSelectOperationNode (bool multi_layer, const std::vector<CompoundRegionOperationNode *> &inputs)
  : CompoundRegionMultiInputOperationNode (inputs), m_multi_layer (multi_layer)
{
  //  .. nothing yet ..
}

std::string CompoundRegionLogicalCaseSelectOperationNode::generated_description () const
{
  //  TODO: could be nicer ...
  std::string r;
  r = "if-then";
  return r + CompoundRegionMultiInputOperationNode::description ();
}

CompoundRegionLogicalCaseSelectOperationNode::ResultType
CompoundRegionLogicalCaseSelectOperationNode::result_type () const
{
  ResultType result = Region;
  for (size_t i = 1; i < children (); i += 2) {
    if (i == 1) {
      result = child (i)->result_type ();
    } else {
      tl_assert (result == child (i)->result_type ());
    }
  }
  return result;
}

template <class T, class TR>
void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local (db::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  bool ok = false;

  for (unsigned int ci = 0; ci < children (); ++ci) {

    shape_interactions<T, T> computed;
    const shape_interactions<T, T> &child_interactions = interactions_for_child<T, T> (interactions, ci, computed);

    const CompoundRegionOperationNode *node = child (ci);

    if (ci % 2 == 0) {

      if (ci + 1 < children ()) {

        ok = node->compute_local_bool<T> (layout, child_interactions, max_vertex_count, area_ratio);
        continue;

      } else {
        //  executes the following statement as default branch
        ok = true;
      }

    }

    if (ok) {

      if (m_multi_layer && results.size () > size_t (ci / 2)) {

        std::vector<std::unordered_set<TR> > one;
        one.push_back (std::unordered_set<TR> ());
        node->compute_local (layout, child_interactions, one, max_vertex_count, area_ratio);
        results[ci / 2].swap (one.front ());

      } else {

        node->compute_local (layout, child_interactions, results, max_vertex_count, area_ratio);

      }

      break;

    }

  }

}

template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<db::Polygon> (db::Layout *, const shape_interactions<db::Polygon, db::Polygon> &, std::vector<std::unordered_set<db::Polygon> > &, size_t, double) const;
template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<db::PolygonRef> (db::Layout *, const shape_interactions<db::PolygonRef, db::PolygonRef> &, std::vector<std::unordered_set<db::PolygonRef> > &, size_t, double) const;

// ---------------------------------------------------------------------------------------------

CompoundRegionFilterOperationNode::CompoundRegionFilterOperationNode (PolygonFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter)
  : CompoundRegionMultiInputOperationNode (input), mp_filter (filter), m_owns_filter (owns_filter)
{
  set_description ("filter");
}

CompoundRegionFilterOperationNode::~CompoundRegionFilterOperationNode ()
{
  if (m_owns_filter) {
    delete mp_filter;
  }
  mp_filter = 0;
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

CompoundRegionEdgeFilterOperationNode::CompoundRegionEdgeFilterOperationNode (EdgeFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter)
  : CompoundRegionMultiInputOperationNode (input), mp_filter (filter), m_owns_filter (owns_filter)
{
  set_description ("filter");
}

CompoundRegionEdgeFilterOperationNode::~CompoundRegionEdgeFilterOperationNode ()
{
  if (m_owns_filter) {
    delete mp_filter;
  }
  mp_filter = 0;
}

void
CompoundRegionEdgeFilterOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgeFilterOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

bool
CompoundRegionEdgeFilterOperationNode::is_selected (const db::Edge &p) const
{
  return mp_filter->selected (p);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgePairFilterOperationNode::CompoundRegionEdgePairFilterOperationNode (EdgePairFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter)
  : CompoundRegionMultiInputOperationNode (input), mp_filter (filter), m_owns_filter (owns_filter)
{
  set_description ("filter");
}

CompoundRegionEdgePairFilterOperationNode::~CompoundRegionEdgePairFilterOperationNode ()
{
  if (m_owns_filter) {
    delete mp_filter;
  }
  mp_filter = 0;
}

void
CompoundRegionEdgePairFilterOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairFilterOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

bool
CompoundRegionEdgePairFilterOperationNode::is_selected (const db::EdgePair &p) const
{
  return mp_filter->selected (p);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionProcessingOperationNode::CompoundRegionProcessingOperationNode (PolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionProcessingOperationNode::~CompoundRegionProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
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
  mp_proc->process (p, res);
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

CompoundRegionToEdgeProcessingOperationNode::CompoundRegionToEdgeProcessingOperationNode (PolygonToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionToEdgeProcessingOperationNode::~CompoundRegionToEdgeProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
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
  mp_proc->process (p, res);
}

void
CompoundRegionToEdgeProcessingOperationNode::processed (db::Layout *, const db::PolygonRef &p, std::vector<db::Edge> &res) const
{
  mp_proc->process (p.obj ().transformed (p.trans ()), res);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgeToPolygonProcessingOperationNode::CompoundRegionEdgeToPolygonProcessingOperationNode (EdgeToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionEdgeToPolygonProcessingOperationNode::~CompoundRegionEdgeToPolygonProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::processed (db::Layout *, const db::Edge &e, std::vector<db::Polygon> &res) const
{
  mp_proc->process (e, res);
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::processed (db::Layout *layout, const db::Edge &e, std::vector<db::PolygonRef> &res) const
{
  std::vector<db::Polygon> polygons;
  mp_proc->process (e, polygons);

  for (std::vector<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
    res.push_back (db::PolygonRef (*p, layout->shape_repository ()));
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionToEdgePairProcessingOperationNode::CompoundRegionToEdgePairProcessingOperationNode (PolygonToEdgePairProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionToEdgePairProcessingOperationNode::~CompoundRegionToEdgePairProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
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
  mp_proc->process (p, res);
}

void
CompoundRegionToEdgePairProcessingOperationNode::processed (db::Layout *, const db::PolygonRef &p, std::vector<db::EdgePair> &res) const
{
  mp_proc->process (p.obj ().transformed (p.trans ()), res);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgePairToPolygonProcessingOperationNode::CompoundRegionEdgePairToPolygonProcessingOperationNode (EdgePairToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionEdgePairToPolygonProcessingOperationNode::~CompoundRegionEdgePairToPolygonProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::processed (db::Layout *, const db::EdgePair &e, std::vector<db::Polygon> &res) const
{
  mp_proc->process (e, res);
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::processed (db::Layout *layout, const db::EdgePair &e, std::vector<db::PolygonRef> &res) const
{
  std::vector<db::Polygon> polygons;
  mp_proc->process (e, polygons);

  for (std::vector<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
    res.push_back (db::PolygonRef (*p, layout->shape_repository ()));
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgePairToEdgeProcessingOperationNode::CompoundRegionEdgePairToEdgeProcessingOperationNode (EdgePairToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionEdgePairToEdgeProcessingOperationNode::~CompoundRegionEdgePairToEdgeProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionEdgePairToEdgeProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairToEdgeProcessingOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionCheckOperationNode::CompoundRegionCheckOperationNode (CompoundRegionOperationNode *input, db::edge_relation_type rel, bool different_polygons, db::Coord d, const db::RegionCheckOptions &options)
  : CompoundRegionMultiInputOperationNode (input), m_check (rel, d, options.metrics), m_different_polygons (different_polygons), m_options (options)
{
  set_description ("check");

  m_check.set_include_zero (false);
  m_check.set_whole_edges (options.whole_edges);
  m_check.set_ignore_angle (options.ignore_angle);
  m_check.set_min_projection (options.min_projection);
  m_check.set_max_projection (options.max_projection);
}

CompoundRegionCheckOperationNode::CompoundRegionCheckOperationNode (db::edge_relation_type rel, bool different_polygons, db::Coord d, const db::RegionCheckOptions &options)
  : CompoundRegionMultiInputOperationNode (), m_check (rel, d, options.metrics), m_different_polygons (different_polygons), m_options (options)
{
  set_description ("check");

  m_check.set_include_zero (false);
  m_check.set_whole_edges (options.whole_edges);
  m_check.set_ignore_angle (options.ignore_angle);
  m_check.set_min_projection (options.min_projection);
  m_check.set_max_projection (options.max_projection);
}

db::OnEmptyIntruderHint
CompoundRegionCheckOperationNode::on_empty_intruder_hint () const
{
  return (m_different_polygons || !inputs ().empty ()) ? OnEmptyIntruderHint::Drop : OnEmptyIntruderHint::Ignore;
}

db::Coord
CompoundRegionCheckOperationNode::dist () const
{
  return m_check.distance ();
}

bool
CompoundRegionCheckOperationNode::wants_merged () const
{
  return true;
}

void
CompoundRegionCheckOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::Polygon, db::Polygon> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  bool other_merged = (children () > 0 && !inputs ()[0]);

  db::check_local_operation<db::Polygon, db::Polygon> op (m_check, m_different_polygons, children () > 0, other_merged, m_options.shielded, m_options.opposite_filter, m_options.rect_filter);

  tl_assert (results.size () == 1);
  if (results.front ().empty ()) {
    op.compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  } else {
    std::vector<std::unordered_set<db::EdgePair> > r;
    r.resize (1);
    op.compute_local (layout, interactions, r, max_vertex_count, area_ratio);
    results.front ().insert (r.front ().begin (), r.front ().end ());
  }
}

void
CompoundRegionCheckOperationNode::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  bool other_merged = (children () > 0 && !inputs ()[0]);

  db::check_local_operation<db::PolygonRef, db::PolygonRef> op (m_check, m_different_polygons, children () > 0, other_merged, m_options.shielded, m_options.opposite_filter, m_options.rect_filter);

  tl_assert (results.size () == 1);
  if (results.front ().empty ()) {
    op.compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  } else {
    std::vector<std::unordered_set<db::EdgePair> > r;
    r.resize (1);
    op.compute_local (layout, interactions, r, max_vertex_count, area_ratio);
    results.front ().insert (r.front ().begin (), r.front ().end ());
  }
}

}

