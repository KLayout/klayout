
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


#include "dbHierProcessor.h"
#include "dbBoxScanner.h"
#include "dbRecursiveShapeIterator.h"
#include "dbBoxConvert.h"
#include "dbEdgeProcessor.h"
#include "dbEdgeBoolean.h"
#include "dbPolygonGenerators.h"
#include "dbLocalOperationUtils.h"
#include "dbShapeFlags.h"
#include "dbCellVariants.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlInternational.h"

// ---------------------------------------------------------------------------------------------
//  Cronology debugging support (TODO: experimental)

#if defined(HAVE_CRONOLOGY)

#include <cronology/events.hpp>

CRONOLOGY_MAKE_EVENT(event_compute_contexts, "Compute contexts")
CRONOLOGY_MAKE_EVENT(event_compute_contexts_unlocked, "Compute contexts (unlocked)")

cronology::events::event_collection<event_compute_contexts, event_compute_contexts_unlocked> collect_events;

#define CRONOLOGY_COLLECTION_BRACKET(event_name) cronology::EventBracket __bracket##event_name (collect_events.threadwise ()->event<event_name> ().event ());

CRONOLOGY_MAKE_EVENT(event_compute_results, "Compute results")
CRONOLOGY_MAKE_EVENT(event_compute_local_cell, "Compute local cell results")
CRONOLOGY_MAKE_EVENT(event_propagate, "Propagate local cell results")

cronology::events::event_collection<event_compute_results, event_compute_local_cell, event_propagate> compute_events;

#define CRONOLOGY_COMPUTE_BRACKET(event_name) cronology::EventBracket __bracket##event_name (compute_events.threadwise ()->event<event_name> ().event ());

#else
#define CRONOLOGY_COLLECTION_BRACKET(event_name)
#define CRONOLOGY_COMPUTE_BRACKET(event_name)
#endif

namespace db
{

//  Heuristic parameter to control the recursion of the cell-to-cell intruder
//  detection: do not recurse if the intruder cell's bounding box is smaller
//  than the overlap box times this factor.
const double area_ratio_for_recursion = 3.0;

// ---------------------------------------------------------------------------------------------
//  Shape reference translator

template <class Ref> class shape_reference_translator;
template <class Ref, class Trans> class shape_reference_translator_with_trans;

template <class Ref>
class shape_reference_translator
{
public:
  typedef typename Ref::shape_type shape_type;
  typedef typename Ref::trans_type ref_trans_type;

  shape_reference_translator (db::Layout *target_layout)
    : mp_layout (target_layout)
  {
    //  .. nothing yet ..
  }

  Ref operator() (const Ref &ref) const
  {
    typename std::unordered_map<const shape_type *, const shape_type *>::const_iterator m = m_cache.find (ref.ptr ());
    if (m != m_cache.end ()) {

      return Ref (m->second, ref.trans ());

    } else {

      const shape_type *ptr;
      {
        tl::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (ref.obj ());
      }

      m_cache[ref.ptr ()] = ptr;
      return Ref (ptr, ref.trans ());

    }
  }

  template <class Trans>
  Ref operator() (const Ref &ref, const Trans &tr) const
  {
    shape_type sh = ref.obj ().transformed (tr * Trans (ref.trans ()));
    ref_trans_type red_trans;
    sh.reduce (red_trans);

    typename std::unordered_map<shape_type, const shape_type *>::const_iterator m = m_cache_by_shape.find (sh);
    if (m != m_cache_by_shape.end ()) {

      return Ref (m->second, red_trans);

    } else {

      const shape_type *ptr;
      {
        tl::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (sh);
      }

      m_cache_by_shape[sh] = ptr;
      return Ref (ptr, red_trans);

    }
  }

private:
  db::Layout *mp_layout;
  mutable std::unordered_map<const shape_type *, const shape_type *> m_cache;
  mutable std::unordered_map<shape_type, const shape_type *> m_cache_by_shape;
};

template <class Shape>
class simple_shape_reference_translator
{
public:
  typedef Shape shape_type;

  simple_shape_reference_translator ()
  {
    //  .. nothing yet ..
  }

  const shape_type &operator() (const shape_type &s) const
  {
    return s;
  }

  template <class Trans>
  shape_type operator() (const shape_type &s, const Trans &tr) const
  {
    return s.transformed (tr);
  }
};

template <>
class shape_reference_translator<db::Edge>
  : public simple_shape_reference_translator<db::Edge>
{
public:
  shape_reference_translator (db::Layout * /*target_layout*/) { }
};

template <>
class shape_reference_translator<db::Polygon>
  : public simple_shape_reference_translator<db::Polygon>
{
public:
  shape_reference_translator (db::Layout * /*target_layout*/) { }
};

template <>
class shape_reference_translator<db::Text>
  : public simple_shape_reference_translator<db::Text>
{
public:
  shape_reference_translator (db::Layout * /*target_layout*/) { }
};

template<class Basic>
class shape_reference_translator<db::object_with_properties<Basic> >
  : public shape_reference_translator<Basic>
{
public:
  typedef db::object_with_properties<Basic> shape_type;

  shape_reference_translator (db::Layout *target_layout)
    : shape_reference_translator<Basic> (target_layout)
  {
    //  .. nothing yet ..
  }

  shape_type operator() (const shape_type &s) const
  {
    return shape_type (shape_reference_translator<Basic>::operator () (s), s.properties_id ());
  }

  template <class Trans>
  shape_type operator() (const shape_type &s, const Trans &tr) const
  {
    return shape_type (shape_reference_translator<Basic>::operator () (s, tr), s.properties_id ());
  }
};

template <class Ref, class Trans>
class shape_reference_translator_with_trans_from_shape_ref
{
public:
  typedef typename Ref::shape_type shape_type;
  typedef typename Ref::trans_type ref_trans_type;

  shape_reference_translator_with_trans_from_shape_ref (db::Layout *target_layout)
    : mp_layout (target_layout)
  {
    //  .. nothing yet ..
  }

  void set_trans (const Trans &trans)
  {
    m_trans = trans;
    m_ref_trans = ref_trans_type (trans);
    m_bare_trans = Trans (m_ref_trans.inverted ()) * trans;
  }

  Ref operator() (const Ref &ref) const
  {
    auto m = m_cache.find (std::make_pair (ref.ptr (), m_bare_trans));
    if (m != m_cache.end ()) {

      return Ref (m->second.first, ref_trans_type (m_trans * Trans (ref.trans ())) * m->second.second);

    } else {

      shape_type sh = ref.obj ().transformed (m_bare_trans);
      ref_trans_type red_trans;
      sh.reduce (red_trans);

      const shape_type *ptr;
      {
        tl::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (sh);
      }

      m_cache[std::make_pair (ref.ptr (), m_bare_trans)] = std::make_pair (ptr, red_trans);

      return Ref (ptr, ref_trans_type (m_trans * Trans (ref.trans ())) * red_trans);

    }
  }

private:
  db::Layout *mp_layout;
  Trans m_trans;
  ref_trans_type m_ref_trans;
  Trans m_bare_trans;
  mutable std::unordered_map<std::pair<const shape_type *, Trans>, std::pair<const shape_type *, ref_trans_type> > m_cache;
};

template <class Trans>
class shape_reference_translator_with_trans<db::PolygonRef, Trans>
  : public shape_reference_translator_with_trans_from_shape_ref<db::PolygonRef, Trans>
{
public:
  shape_reference_translator_with_trans (db::Layout *target_layout)
    : shape_reference_translator_with_trans_from_shape_ref<db::PolygonRef, Trans> (target_layout)
  {
    //  .. nothing yet ..
  }
};

template <class Sh, class Trans>
class shape_reference_translator_with_trans
{
public:
  typedef Sh shape_type;

  shape_reference_translator_with_trans (db::Layout * /*target_layout*/)
  {
    //  .. nothing yet ..
  }

  void set_trans (const Trans &trans)
  {
    m_trans = trans;
  }

  shape_type operator() (const shape_type &s) const
  {
    return s.transformed (m_trans);
  }

private:
  Trans m_trans;
};

template <class Basic, class Trans>
class shape_reference_translator_with_trans<db::object_with_properties<Basic>, Trans>
  : public shape_reference_translator_with_trans<Basic, Trans>
{
public:
  typedef db::object_with_properties<Basic> shape_type;

  shape_reference_translator_with_trans (db::Layout *target_layout)
    : shape_reference_translator_with_trans<Basic, Trans> (target_layout)
  {
    //  .. nothing yet ..
  }

  shape_type operator() (const shape_type &s) const
  {
    //  CAUTION: no property ID translation happens here (reasoning: the main use case is fake ID for net tagging)
    return shape_type (shape_reference_translator_with_trans<Basic, Trans>::operator () (s), s.properties_id ());
  }
};

// ---------------------------------------------------------------------------------------------

/**
 *  @brief a utility to capture insert attempts of the wrong type into a box scanner
 *  These attempts can happen because the generic nature of the interaction detector code
 */

template <class T, class P>
void safe_insert2_into_box_scanner (db::box_scanner2 <T, P, T, P> &scanner, const T *t, const P &p)
{
  scanner.insert2 (t, p);
}

template <class T1, class P1, class T2, class P2>
void safe_insert2_into_box_scanner (db::box_scanner2 <T1, P1, T2, P2> &scanner, const T2 *t, const P2 &p)
{
  scanner.insert2 (t, p);
}

template <class T1, class P1, class T2, class P2>
void safe_insert2_into_box_scanner (db::box_scanner2 <T1, P1, T2, P2> &, const T1 *, const P1 &)
{
  tl_assert (false);
}

// ---------------------------------------------------------------------------------------------

/**
 *  @brief Safe enlargement of a box
 *  Boxes must not vanish when augmented for overlapping queries. Hence we must not make
 *  the boxes shrinked too much on enlarge.
 */
db::Box safe_box_enlarged (const db::Box &box, db::Coord dx, db::Coord dy)
{
  if (box.empty ()) {
    return box;
  } else if (box == db::Box::world ()) {
    return box;
  } else {
    db::Coord w2 = db::Coord (box.width () / 2);
    db::Coord h2 = db::Coord (box.height () / 2);
    if (dx + w2 < 0) {
      dx = -w2;
    }
    if (dy + h2 < 0) {
      dy = -h2;
    }
    return box.enlarged (db::Vector (dx, dy));
  }
}

// ---------------------------------------------------------------------------------------------
//  Debugging utility: dump the cell contexts

template <class TS, class TI, class TR>
static void dump_cell_contexts (local_processor_contexts<TS, TI, TR> &contexts, const db::Layout *subject_layout, const db::Layout *intruder_layout)
{
  for (auto cc = contexts.begin (); cc != contexts.end (); ++cc) {
    tl::info << "Cell " << subject_layout->cell_name (cc->first->cell_index ()) << ":";
    int i = 0;
    for (auto c = cc->second.begin (); c != cc->second.end (); ++c) {
      tl::info << "  Context #" << ++i;
      tl::info << "    Instances:";
      for (auto i = c->first.first.begin (); i != c->first.first.end (); ++i) {
        const db::CellInstArray &ci = *i;
        tl::info << "      " << intruder_layout->cell_name (ci.object ().cell_index ()) << " @ " << ci.complex_trans (*ci.begin ()).to_string () << " (" << ci.size () << ")";
      }
      tl::info << "    Shapes:";
      for (auto i = c->first.second.begin (); i != c->first.second.end (); ++i) {
        for (auto s = i->second.begin (); s != i->second.end (); ++s) {
          tl::info << "      " << intruder_layout->get_properties (i->first).to_string () << ": " << s->to_string ();
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorCellContext implementation

template <class TS, class TI, class TR>
local_processor_cell_context<TS, TI, TR>::local_processor_cell_context ()
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
local_processor_cell_context<TS, TI, TR>::local_processor_cell_context (const local_processor_cell_context &other)
  : m_propagated (other.m_propagated), m_drops (other.m_drops)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
local_processor_cell_context<TS, TI, TR> &
local_processor_cell_context<TS, TI, TR>::operator= (const local_processor_cell_context &other)
{
  if (this != &other) {
    m_propagated = other.m_propagated;
    m_drops = other.m_drops;
  }
  return *this;
}

template <class TS, class TI, class TR>
void
local_processor_cell_context<TS, TI, TR>::add (db::local_processor_cell_context<TS, TI, TR> *parent_context, db::Cell *parent, const db::ICplxTrans &cell_inst)
{
  m_drops.push_back (local_processor_cell_drop<TS, TI, TR> (parent_context, parent, cell_inst));
}

template <class TS, class TI, class TR>
void
local_processor_cell_context<TS, TI, TR>::propagate (unsigned int output_layer, const std::unordered_set<TR> &res)
{
  if (res.empty ()) {
    return;
  }

  db::Layout *subject_layout = 0;
  shape_reference_translator_with_trans<TR, db::ICplxTrans> rt (subject_layout);

  for (typename std::vector<local_processor_cell_drop<TS, TI, TR> >::const_iterator d = m_drops.begin (); d != m_drops.end (); ++d) {

    tl_assert (d->parent_context != 0);
    tl_assert (d->parent != 0);

    if (subject_layout != d->parent->layout ()) {
      subject_layout = d->parent->layout ();
      rt = shape_reference_translator_with_trans<TR, db::ICplxTrans> (subject_layout);
    }

    rt.set_trans (d->cell_inst);
    std::vector<TR> new_refs;
    new_refs.reserve (res.size ());
    for (typename std::unordered_set<TR>::const_iterator r = res.begin (); r != res.end (); ++r) {
      new_refs.push_back (rt (*r));
    }

    {
      tl::MutexLocker locker (&d->parent_context->lock ());
      d->parent_context->propagated (output_layer).insert (new_refs.begin (), new_refs.end ());
    }

  }
}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorCellContexts implementation

template <class TS, class TI, class TR>
local_processor_cell_contexts<TS, TI, TR>::local_processor_cell_contexts ()
  : mp_intruder_cell (0)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
local_processor_cell_contexts<TS, TI, TR>::local_processor_cell_contexts (const db::Cell *intruder_cell)
  : mp_intruder_cell (intruder_cell)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::local_processor_cell_context<TS, TI, TR> *
local_processor_cell_contexts<TS, TI, TR>::find_context (const context_key_type &intruders)
{
  typename std::unordered_map<context_key_type, db::local_processor_cell_context<TS, TI, TR> >::iterator c = m_contexts.find (intruders);
  return c != m_contexts.end () ? &c->second : 0;
}

template <class TS, class TI, class TR>
db::local_processor_cell_context<TS, TI, TR> *
local_processor_cell_contexts<TS, TI, TR>::create (const context_key_type &intruders)
{
  return &m_contexts[intruders];
}

template <class TR>
static void
subtract_set (std::unordered_set<TR> &res, const std::unordered_set<TR> &other)
{
  //  for everything else, we don't use a boolean core but just set intersection
  for (typename std::unordered_set<TR>::const_iterator o = other.begin (); o != other.end (); ++o) {
    res.erase (*o);
  }
}

template <class TS, class TI>
static void
subtract (std::unordered_set<db::PolygonRef> &res, const std::unordered_set<db::PolygonRef> &other, db::Layout *layout, const db::local_processor<TS, TI, db::PolygonRef> *proc)
{
  if (other.empty ()) {
    return;
  }

  if (! proc->boolean_core ()) {
    subtract_set (res, other);
    return;
  }

  size_t max_vertex_count = proc->max_vertex_count ();
  double area_ratio = proc->area_ratio ();

  db::EdgeProcessor ep;
  ep.set_base_verbosity (proc->base_verbosity () + 30);

  size_t p1 = 0, p2 = 1;

  for (std::unordered_set<db::PolygonRef>::const_iterator i = res.begin (); i != res.end (); ++i) {
    const db::PolygonRef &subject = *i;
    for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, p1);
    }
    p1 += 2;
  }

  for (std::unordered_set<db::PolygonRef>::const_iterator i = other.begin (); i != other.end (); ++i) {
    const db::PolygonRef &subject = *i;
    for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, p2);
    }
    p2 += 2;
  }

  res.clear ();
  db::BooleanOp op (db::BooleanOp::ANotB);
  db::PolygonRefGenerator pr (layout, res);
  db::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
  db::PolygonGenerator pg (splitter, true, true);
  ep.process (pg, op);
}

template <class TS, class TI>
static void
subtract (std::unordered_set<db::PolygonRefWithProperties> &res, const std::unordered_set<db::PolygonRefWithProperties> &other, db::Layout *layout, const db::local_processor<TS, TI, db::PolygonRefWithProperties> *proc)
{
  if (other.empty ()) {
    return;
  }

  if (! proc->boolean_core ()) {
    subtract_set (res, other);
    return;
  }

  size_t max_vertex_count = proc->max_vertex_count ();
  double area_ratio = proc->area_ratio ();

  std::unordered_set<db::PolygonRefWithProperties> first;
  first.swap (res);

  std::map<db::properties_id_type, std::pair<std::vector<const db::PolygonRefWithProperties *>, std::vector<const db::PolygonRefWithProperties *> > > by_prop_id;
  for (auto i = first.begin (); i != first.end (); ++i)   {
    by_prop_id [i->properties_id ()].first.push_back (i.operator-> ());
  }
  for (auto i = other.begin (); i != other.end (); ++i)   {
    by_prop_id [i->properties_id ()].second.push_back (i.operator-> ());
  }

  db::EdgeProcessor ep;
  ep.set_base_verbosity (proc->base_verbosity () + 30);

  for (auto s2p = by_prop_id.begin (); s2p != by_prop_id.end (); ++s2p) {

    db::properties_id_type prop_id = s2p->first;
    size_t p1 = 0, p2 = 1;

    ep.clear ();

    for (auto i = s2p->second.first.begin (); i != s2p->second.first.end (); ++i) {
      ep.insert (**i, p1);
      p1 += 2;
    }

    for (auto i = s2p->second.second.begin (); i != s2p->second.second.end (); ++i) {
      ep.insert (**i, p2);
      p2 += 2;
    }

    db::BooleanOp op (db::BooleanOp::ANotB);
    db::polygon_ref_generator_with_properties<db::PolygonRefWithProperties> pr (layout, res, prop_id);
    db::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
    db::PolygonGenerator pg (splitter, true, true);
    ep.process (pg, op);

  }
}

template <class TS, class TI>
static void
subtract (std::unordered_set<db::Edge> &res, const std::unordered_set<db::Edge> &other, db::Layout * /*layout*/, const db::local_processor<TS, TI, db::Edge> *proc)
{
  if (other.empty ()) {
    return;
  }

  if (! proc->boolean_core ()) {
    subtract_set (res, other);
    return;
  }

  db::box_scanner<db::Edge, size_t> scanner;
  scanner.reserve (res.size () + other.size ());

  for (std::unordered_set<Edge>::const_iterator i = res.begin (); i != res.end (); ++i) {
    scanner.insert (i.operator-> (), 0);
  }
  for (std::unordered_set<Edge>::const_iterator i = other.begin (); i != other.end (); ++i) {
    scanner.insert (i.operator-> (), 1);
  }

  std::unordered_set<db::Edge> result;
  EdgeBooleanClusterCollector<std::unordered_set<db::Edge> > cluster_collector (&result, EdgeNot);
  scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

  res.swap (result);
}

template <class TS, class TI>
static void
subtract (std::unordered_set<db::EdgeWithProperties> &res, const std::unordered_set<db::EdgeWithProperties> &other, db::Layout * /*layout*/, const db::local_processor<TS, TI, db::EdgeWithProperties> *proc)
{
  if (other.empty ()) {
    return;
  }

  if (! proc->boolean_core ()) {
    subtract_set (res, other);
    return;
  }

  std::unordered_set<db::EdgeWithProperties> first;
  first.swap (res);

  std::map<db::properties_id_type, std::pair<std::vector<const db::EdgeWithProperties *>, std::vector<const db::EdgeWithProperties *> > > by_prop_id;
  for (auto i = first.begin (); i != first.end (); ++i)   {
    by_prop_id [i->properties_id ()].first.push_back (i.operator-> ());
  }
  for (auto i = other.begin (); i != other.end (); ++i)   {
    by_prop_id [i->properties_id ()].second.push_back (i.operator-> ());
  }

  for (auto s2p = by_prop_id.begin (); s2p != by_prop_id.end (); ++s2p) {

    if (s2p->second.second.empty ()) {

      for (auto i = s2p->second.first.begin (); i != s2p->second.first.end (); ++i) {
        res.insert (**i);
      }

    } else {

      db::box_scanner<db::Edge, size_t> scanner;
      scanner.reserve (s2p->second.first.size () + s2p->second.second.size ());

      for (auto i = s2p->second.first.begin (); i != s2p->second.first.end (); ++i) {
        scanner.insert (*i, 0);
      }
      for (auto i = s2p->second.second.begin (); i != s2p->second.second.end (); ++i) {
        scanner.insert (*i, 1);
      }

      db::property_injector<db::Edge, std::unordered_set<db::EdgeWithProperties> > prop_inject (&res, s2p->first);
      EdgeBooleanClusterCollector<db::property_injector<db::Edge, std::unordered_set<db::EdgeWithProperties> > > cluster_collector (&prop_inject, EdgeNot);
      scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    }

  }
}

template <class TS, class TI, class TR>
static void
subtract (std::unordered_set<TR> &res, const std::unordered_set<TR> &other, db::Layout * /*layout*/, const db::local_processor<TS, TI, TR> * /*proc*/)
{
  subtract_set (res, other);
}

//  determines the default boolean core flag per result type

namespace
{

template <class TR>
struct default_boolean_core
{
  bool operator() () const { return false; }
};

template <>
struct default_boolean_core<db::PolygonRef>
{
  bool operator() () const { return true; }
};

}

namespace {

template <class TS, class TI, class TR>
struct context_sorter
{
  bool operator () (const std::pair<const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type *, db::local_processor_cell_context<TS, TI, TR> *> &a,
                    const std::pair<const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type *, db::local_processor_cell_context<TS, TI, TR> *> &b)
  {
    return *a.first < *b.first;
  }
};

}

template <class TS, class TI, class TR>
void
local_processor_cell_contexts<TS, TI, TR>::compute_results (const local_processor_contexts<TS, TI, TR> &contexts, db::Cell *cell, const local_operation<TS, TI, TR> *op, const std::vector<unsigned int> &output_layers, const local_processor<TS, TI, TR> *proc)
{
  CRONOLOGY_COMPUTE_BRACKET(event_compute_results)

  bool first = true;
  std::vector<std::unordered_set<TR> > common;
  common.resize (output_layers.size ());

  int index = 0;
  int total = int (m_contexts.size ());

  //  NOTE: we use the ordering provided by key_type::operator< rather than the unordered map to achieve
  //  reproducibility across different platforms. unordered_map is faster, but for processing them,
  //  strict ordering is a more robust choice.
  std::vector<std::pair<const context_key_type *, db::local_processor_cell_context<TS, TI, TR> *> > sorted_contexts;
  sorted_contexts.reserve (m_contexts.size ());
  for (typename std::unordered_map<context_key_type, db::local_processor_cell_context<TS, TI, TR> >::iterator c = m_contexts.begin (); c != m_contexts.end (); ++c) {
    sorted_contexts.push_back (std::make_pair (&c->first, &c->second));
  }

  std::sort (sorted_contexts.begin (), sorted_contexts.end (), context_sorter<TS, TI, TR> ());

  for (typename std::vector<std::pair<const context_key_type *, db::local_processor_cell_context<TS, TI, TR> *> >::const_iterator c = sorted_contexts.begin (); c != sorted_contexts.end (); ++c) {

    proc->next ();
    ++index;

    if (tl::verbosity () >= proc->base_verbosity () + 20) {
      tl::log << tr ("Computing local results for ") << cell->layout ()->cell_name (cell->cell_index ()) << " (context " << index << "/" << total << ")";
    }

    if (first) {

      {
        tl::MutexLocker locker (&c->second->lock ());
        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
          common [o - output_layers.begin ()] = c->second->propagated (*o);
        }
      }

      CRONOLOGY_COMPUTE_BRACKET(event_compute_local_cell)
      proc->compute_local_cell (contexts, cell, mp_intruder_cell, op, *c->first, common);
      first = false;

    } else {

      std::vector<std::unordered_set<TR> > res;
      res.resize (output_layers.size ());

      {
        tl::MutexLocker locker (&c->second->lock ());
        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
          res [o - output_layers.begin ()] = c->second->propagated (*o);
        }
      }

      {
        CRONOLOGY_COMPUTE_BRACKET(event_compute_local_cell)
        proc->compute_local_cell (contexts, cell, mp_intruder_cell, op, *c->first, res);
      }

      if (common.empty ()) {

        CRONOLOGY_COMPUTE_BRACKET(event_propagate)
        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
          c->second->propagate (*o, res [o - output_layers.begin ()]);
        }

//  gcc 4.4.7 (at least) doesn't have an operator== in std::unordered_set, so we skip this
//  optimization
#if defined(__GNUC__) && __GNUC__ == 4
      } else {
#else
      } else if (res != common) {
#endif

        CRONOLOGY_COMPUTE_BRACKET(event_propagate)

        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {

          size_t oi = o - output_layers.begin ();

          std::unordered_set<TR> lost;

          for (typename std::unordered_set<TR>::const_iterator i = common[oi].begin (); i != common[oi].end (); ++i) {
            if (res[oi].find (*i) == res[oi].end ()) {
              lost.insert (*i);
            }
          }

          if (! lost.empty ()) {

            subtract (lost, res[oi], cell->layout (), proc);

            if (! lost.empty ()) {
              subtract (common[oi], lost, cell->layout (), proc);
              for (typename std::vector<std::pair<const context_key_type *, db::local_processor_cell_context<TS, TI, TR> *> >::const_iterator cc = sorted_contexts.begin (); cc != c; ++cc) {
                cc->second->propagate (*o, lost);
              }
            }

          }

        }

        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {

          std::unordered_set<TR> gained;

          size_t oi = o - output_layers.begin ();
          for (typename std::unordered_set<TR>::const_iterator i = res[oi].begin (); i != res[oi].end (); ++i) {
            if (common[oi].find (*i) == common[oi].end ()) {
              gained.insert (*i);
            }
          }

          if (! gained.empty ()) {

            subtract (gained, common[oi], cell->layout (), proc);

            if (! gained.empty ()) {
              c->second->propagate (*o, gained);
            }

          }

        }

      }

    }

  }

  for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
    size_t oi = o - output_layers.begin ();
    proc->push_results (cell, *o, common[oi]);
  }
}

// ---------------------------------------------------------------------------------------------

template <class TS, class TI>
shape_interactions<TS, TI>::shape_interactions ()
  : m_id (0)
{
  //  .. nothing yet ..
}

template <class TS, class TI>
bool
shape_interactions<TS, TI>::has_intruder_shape_id (unsigned int id) const
{
  return m_intruder_shapes.find (id) != m_intruder_shapes.end ();
}

template <class TS, class TI>
bool
shape_interactions<TS, TI>::has_subject_shape_id (unsigned int id) const
{
  return m_subject_shapes.find (id) != m_subject_shapes.end ();
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_intruder_shape (unsigned int id, unsigned int layer, const TI &shape)
{
  m_intruder_shapes [id] = std::make_pair (layer, shape);
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_subject_shape (unsigned int id, const TS &shape)
{
  m_subject_shapes [id] = shape;
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_subject (unsigned int id, const TS &shape)
{
  m_subject_shapes [id] = shape;
  m_interactions.insert (std::make_pair (id, container::value_type::second_type ()));
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_interaction (unsigned int subject_id, unsigned int intruder_id)
{
  m_interactions [subject_id].push_back (intruder_id);
}

template <class TS, class TI>
const std::vector<unsigned int> &
shape_interactions<TS, TI>::intruders_for (unsigned int subject_id) const
{
  iterator i = m_interactions.find (subject_id);
  if (i == m_interactions.end ()) {
    static std::vector<unsigned int> empty;
    return empty;
  } else {
    return i->second;
  }
}

template <class TS, class TI>
const TS &
shape_interactions<TS, TI>::subject_shape (unsigned int id) const
{
  typename std::unordered_map<unsigned int, TS>::const_iterator i = m_subject_shapes.find (id);
  if (i == m_subject_shapes.end ()) {
    static TS s;
    return s;
  } else {
    return i->second;
  }
}

template <class TS, class TI>
const std::pair<unsigned int, TI> &
shape_interactions<TS, TI>::intruder_shape (unsigned int id) const
{
  typename std::unordered_map<unsigned int, std::pair<unsigned int, TI> >::const_iterator i = m_intruder_shapes.find (id);
  if (i == m_intruder_shapes.end ()) {
    static std::pair<unsigned int, TI> s;
    return s;
  } else {
    return i->second;
  }
}

// ---------------------------------------------------------------------------------------------
//  Helper classes for the LocalProcessor

namespace
{

template <class TS, class TI>
struct interaction_registration_shape2shape
  : db::box_scanner_receiver2<TS, unsigned int, TI, unsigned int>
{
public:
  interaction_registration_shape2shape (db::Layout *layout, shape_interactions<TS, TI> *result, unsigned int intruder_layer_index)
    : mp_result (result), mp_layout (layout), m_intruder_layer_index (intruder_layer_index)
  {
    //  nothing yet ..
  }

  void add (const TS *ref1, unsigned int id1, const TI *ref2, unsigned int id2)
  {
    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }

    if (!mp_result->has_intruder_shape_id (id2)) {
      if (mp_layout) {
        //  In order to guarantee the refs come from the subject layout, we'd need to
        //  rewrite them
        db::shape_reference_translator<TI> rt (mp_layout);
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, rt (*ref2));
      } else {
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
      }
    }

    mp_result->add_interaction (id1, id2);
  }

  void same (unsigned int, unsigned int)
  {
    //  ignore. Two shapes of a different kind can't be the same.
  }

private:
  shape_interactions<TS, TI> *mp_result;
  db::Layout *mp_layout;
  unsigned int m_intruder_layer_index;
};

template <class T>
struct interaction_registration_shape2shape<T, T>
  : db::box_scanner_receiver2<T, unsigned int, T, unsigned int>
{
public:
  interaction_registration_shape2shape (db::Layout *layout, shape_interactions<T, T> *result, unsigned int intruder_layer_index)
    : mp_result (result), mp_layout (layout), m_intruder_layer_index (intruder_layer_index)
  {
    //  nothing yet ..
  }

  void add (const T *ref1, unsigned int id1, const T *ref2, unsigned int id2)
  {
    if (! m_same.empty () && (m_same.find (std::make_pair (id1, id2)) != m_same.end () || m_same.find (std::make_pair (id2, id1)) != m_same.end ())) {
      //  ignore self-interactions
      return;
    }

    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }

    if (!mp_result->has_intruder_shape_id (id2)) {
      if (mp_layout) {
        //  In order to guarantee the refs come from the subject layout, we'd need to
        //  rewrite them
        db::shape_reference_translator<T> rt (mp_layout);
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, rt (*ref2));
      } else {
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
      }
    }

    mp_result->add_interaction (id1, id2);
  }

  void same (unsigned int a, unsigned int b)
  {
    m_same.insert (std::make_pair (a, b));
  }

private:
  shape_interactions<T, T> *mp_result;
  std::unordered_set<std::pair<unsigned int, unsigned int> > m_same;
  db::Layout *mp_layout;
  unsigned int m_intruder_layer_index;
};

template <class TS, class TI>
struct interaction_registration_shape1
  : db::box_scanner_receiver2<TS, unsigned int, TI, unsigned int>
{
public:
  interaction_registration_shape1 (shape_interactions<TS, TI> *result, unsigned int intruder_layer_index)
    : mp_result (result), m_intruder_layer_index (intruder_layer_index)
  {
    //  .. nothing yet ..
  }

  void add (const TS *ref1, unsigned int id1, const TI *ref2, unsigned int id2)
  {
    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }
    if (!mp_result->has_intruder_shape_id (id2)) {
      mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
    }
    mp_result->add_interaction (id1, id2);
  }

private:
  shape_interactions<TS, TI> *mp_result;
  unsigned int m_intruder_layer_index;
};

template <class T>
struct interaction_registration_shape1<T, T>
  : db::box_scanner_receiver<T, unsigned int>
{
public:
  interaction_registration_shape1 (shape_interactions<T, T> *result, unsigned int intruder_layer_index)
    : mp_result (result), m_intruder_layer_index (intruder_layer_index)
  {
    //  nothing yet ..
  }

  void add (const T *ref1, unsigned int id1, const T *ref2, unsigned int id2)
  {
    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }
    if (!mp_result->has_intruder_shape_id (id2)) {
      mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
    }
    mp_result->add_interaction (id1, id2);
  }

private:
  shape_interactions<T, T> *mp_result;
  unsigned int m_intruder_layer_index;
};

template <class TS, class TI>
struct interaction_registration_shape2inst
  : db::box_scanner_receiver2<TS, unsigned int, db::CellInstArray, unsigned int>
{
public:
  interaction_registration_shape2inst (db::Layout *subject_layout, const db::Layout *intruder_layout, unsigned int intruder_layer, unsigned int intruder_layer_index, db::Coord dist, shape_interactions<TS, TI> *result)
    : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), m_intruder_layer (intruder_layer), m_intruder_layer_index (intruder_layer_index), m_dist (dist), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const TS *ref, unsigned int id1, const db::CellInstArray *inst, unsigned int inst_id)
  {
    const db::Cell &intruder_cell = mp_intruder_layout->cell (inst->object ().cell_index ());
    db::box_convert <db::CellInst, true> inst_bc (*mp_intruder_layout, m_intruder_layer);
    mp_result->add_subject_shape (id1, *ref);

    //  Find all instance array members that potentially interact with the shape and use
    //  add_shapes_from_intruder_inst on them
    db::Box ref_box = db::box_convert<TS> () (*ref);
    for (db::CellInstArray::iterator n = inst->begin_touching (safe_box_enlarged (ref_box, m_dist - 1, m_dist - 1), inst_bc); !n.at_end (); ++n) {
      db::ICplxTrans tn = inst->complex_trans (*n);
      db::Box region = ref_box.enlarged (db::Vector (m_dist, m_dist)).transformed (tn.inverted ()) & intruder_cell.bbox (m_intruder_layer);
      if (! region.empty ()) {
        add_shapes_from_intruder_inst (id1, intruder_cell, tn, inst_id, region);
      }
    }
  }

private:
  db::Layout *mp_subject_layout;
  const db::Layout *mp_intruder_layout;
  unsigned int m_intruder_layer, m_intruder_layer_index;
  db::Coord m_dist;
  shape_interactions<TS, TI> *mp_result;
  std::unordered_map<TI, unsigned int> m_inst_shape_ids;

  void add_shapes_from_intruder_inst (unsigned int id1, const db::Cell &intruder_cell, const db::ICplxTrans &tn, unsigned int /*inst_id*/, const db::Box &region)
  {
    db::shape_reference_translator<TI> rt (mp_subject_layout);
    db::shape_to_object<TI> s2o;

    //  Look up all shapes from the intruder instance which interact with the subject shape
    //  (given through region)
    //  TODO: should be lighter, cache, handle arrays ..
    db::RecursiveShapeIterator si (*mp_intruder_layout, intruder_cell, m_intruder_layer, region);
    si.shape_flags (shape_flags<TI> ());
    while (! si.at_end ()) {

      //  NOTE: we intentionally rewrite to the *subject* layout - this way polygon refs in the context come from the
      //  subject, not from the intruder.
      TI ref2 = rt (s2o (si.shape ()), tn * si.trans ());

      //  reuse the same id for shapes from the same instance -> this avoid duplicates with different IDs on
      //  the intruder side.
      typename std::unordered_map<TI, unsigned int>::const_iterator k = m_inst_shape_ids.find (ref2);
      if (k == m_inst_shape_ids.end ()) {

        k = m_inst_shape_ids.insert (std::make_pair (ref2, mp_result->next_id ())).first;
        mp_result->add_intruder_shape (k->second, m_intruder_layer_index, ref2);

      }

      mp_result->add_interaction (id1, k->second);

      ++si;

    }
  }
};

template <class TS, class TI, class TR>
struct interaction_registration_inst2inst
  : db::box_scanner_receiver2<db::CellInstArray, unsigned int, db::CellInstArray, unsigned int>
{
public:
  typedef typename local_processor_cell_contexts<TS, TI, TR>::context_key_type interactions_value_type;
  typedef std::unordered_map<std::pair<db::cell_index_type, db::ICplxTrans>, interactions_value_type> interactions_type;

  interaction_registration_inst2inst (const db::Layout *subject_layout, unsigned int subject_layer, const db::Layout *intruder_layout, unsigned int intruder_layer, bool foreign, db::Coord dist, interactions_type *result)
    : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), m_subject_layer (subject_layer), m_intruder_layer (intruder_layer), m_dist (dist), mp_result (result), m_foreign (foreign)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst1, unsigned int id1, const db::CellInstArray *inst2, unsigned int id2)
  {
    //  NOTE: self-interactions are possible for arrays: different elements of the
    //  array may interact which is a cell-external interaction.
    if (mp_subject_layout != mp_intruder_layout || id1 != id2 || inst1->size () > 1) {

      bool ignore = false;
      if (mp_subject_layout == mp_intruder_layout && m_subject_layer == m_intruder_layer && ! m_foreign) {
        if (m_interactions.find (std::make_pair (id2, id1)) != m_interactions.end ()) {
          //  for self interactions ignore the reverse interactions
          ignore = true;
        } else {
          m_interactions.insert (std::make_pair (id1, id2));
        }
      }

      if (! ignore) {
        collect_instance_interactions (inst1, inst2);
      }

    }
  }

private:
  const db::Layout *mp_subject_layout, *mp_intruder_layout;
  unsigned int m_subject_layer, m_intruder_layer;
  db::Coord m_dist;
  interactions_type *mp_result;
  std::unordered_set<std::pair<unsigned int, unsigned int> > m_interactions;
  bool m_foreign;

  void
  collect_instance_interactions (const db::CellInstArray *inst1, const db::CellInstArray *inst2)
  {
    //  TODO: this algorithm is not in particular effective for identical arrays or for arrays
    //  vs. single instances

    const db::Cell &cell1 = mp_subject_layout->cell (inst1->object ().cell_index ());
    const db::Cell &cell2 = mp_intruder_layout->cell (inst2->object ().cell_index ());
    db::box_convert <db::CellInst, true> inst2_bc (*mp_intruder_layout, m_intruder_layer);
    db::box_convert <db::CellInst, true> inst1_bc (*mp_subject_layout, m_subject_layer);

    db::Box iibox2 = inst2->bbox (inst2_bc).enlarged (db::Vector (m_dist, m_dist));
    if (iibox2.empty ()) {
      return;
    }

    std::unordered_map<db::ICplxTrans, std::list<std::pair<db::cell_index_type, db::ICplxTrans> > > interactions_cache;

    for (db::CellInstArray::iterator n = inst1->begin_touching (safe_box_enlarged (iibox2, -1, -1), inst1_bc); ! n.at_end (); ++n) {

      db::ICplxTrans tn1 = inst1->complex_trans (*n);
      db::ICplxTrans tni1 = tn1.inverted ();
      db::Box ibox1 = (tn1 * cell1.bbox (m_subject_layer)).enlarged (db::Vector (m_dist, m_dist));

      std::set<db::CellInstArray> *insts = 0;

      if (! ibox1.empty ()) {

        //  TODO: in some cases, it may be possible to optimize this for arrays

        for (db::CellInstArray::iterator k = inst2->begin_touching (safe_box_enlarged (ibox1, -1, -1), inst2_bc); ! k.at_end (); ++k) {

          if (inst1 == inst2 && *n == *k) {
            //  skip self-interactions - this is handled inside the cell
            continue;
          }

          db::ICplxTrans tn2 = inst2->complex_trans (*k);

          //  NOTE: we need to enlarge both subject *and* intruder boxes - either object comes close to intruder or the other way around
          db::Box ibox2 = (tn2 * cell2.bbox (m_intruder_layer)).enlarged (db::Vector (m_dist, m_dist));

          db::Box cbox = ibox1 & ibox2;
          if (! cbox.empty () && (cbox == ibox1 || cell1.has_shapes_touching (m_subject_layer, safe_box_enlarged (tni1 * cbox, -1, -1)))) {

            db::ICplxTrans tn21 = tni1 * tn2;

            std::list<std::pair<db::cell_index_type, db::ICplxTrans> > *interactions = 0;

            std::unordered_map<db::ICplxTrans, std::list<std::pair<db::cell_index_type, db::ICplxTrans> > >::iterator ic = interactions_cache.find (tn21);
            if (ic != interactions_cache.end ()) {
              interactions = &ic->second;
            } else {
              interactions = &interactions_cache [tn21];
              collect_intruder_tree_interactions (cell1, cell2, tni1, tn21, cbox, *interactions);
            }

            for (std::list<std::pair<db::cell_index_type, db::ICplxTrans> >::const_iterator i = interactions->begin (); i != interactions->end (); ++i) {
              if (! insts) {
                insts = & (*mp_result) [std::make_pair (cell1.cell_index (), tn1)].first;
              }
              insts->insert (db::CellInstArray (db::CellInst (i->first), i->second));
            }

          }

        }

      }

    }
  }

  void collect_intruder_tree_interactions (const db::Cell &subject_cell, const db::Cell &intruder_cell, const db::ICplxTrans &tni1, const db::ICplxTrans &tn21, const db::Box &cbox, std::list<std::pair<db::cell_index_type, db::ICplxTrans> > &interactions)
  {
    db::ICplxTrans tni2 = tn21.inverted () * tni1;
    db::Box tbox2 = safe_box_enlarged (tni2 * cbox, -1, -1);

    //  do not recurse further if we're overlapping with shapes from the intruder
    //  or the intruder cell is not much bigger than the region of interest (cbox)
    if (intruder_cell.bbox (m_intruder_layer).area () < area_ratio_for_recursion * cbox.area ()
        || ! intruder_cell.shapes (m_intruder_layer).begin_touching (tbox2, ShapeIterator::All).at_end ()) {

      interactions.push_back (std::make_pair (intruder_cell.cell_index (), tn21));
      return;

    }

    for (db::Cell::touching_iterator i = intruder_cell.begin_touching (tbox2); ! i.at_end (); ++i) {

      const db::Cell &ic = mp_intruder_layout->cell (i->cell_index ());

      for (db::CellInstArray::iterator ia = i->cell_inst ().begin_touching (tbox2, db::box_convert<db::CellInst> (*mp_intruder_layout, m_intruder_layer)); ! ia.at_end (); ++ia) {

        db::ICplxTrans it = i->complex_trans (*ia);

        db::Box ibox2 = (tni2.inverted () * it * ic.bbox (m_intruder_layer)).enlarged (db::Vector (m_dist, m_dist));
        db::Box ccbox = cbox & ibox2;

        if (! ccbox.empty ()) {
          collect_intruder_tree_interactions (subject_cell, ic, tni1, tn21 * it, ccbox, interactions);
        }

      }

    }
  }
};

template <class TS, class TI, class TR>
struct interaction_registration_inst2shape
  : db::box_scanner_receiver2<db::CellInstArray, unsigned int, TI, unsigned int>
{
public:
  typedef typename local_processor_cell_contexts<TS, TI, TR>::context_key_type interactions_value_type;
  typedef std::unordered_map<std::pair<db::cell_index_type, db::ICplxTrans>, interactions_value_type> interactions_type;

  interaction_registration_inst2shape (db::Layout *subject_layout, unsigned int subject_layer, db::Coord dist, interactions_type *result)
    : mp_subject_layout (subject_layout), m_subject_layer (subject_layer), m_dist (dist), mp_result (result), m_rt (subject_layout)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst, unsigned int, const TI *ref, unsigned int layer)
  {
    collect_instance_shape_interactions (inst, layer, *ref, m_dist);
  }

private:
  db::Layout *mp_subject_layout;
  unsigned int m_subject_layer;
  db::Coord m_dist;
  interactions_type *mp_result;
  db::shape_reference_translator_with_trans<TI, db::ICplxTrans> m_rt;

  void
  collect_instance_shape_interactions (const db::CellInstArray *inst, unsigned int layer, const TI &ref, db::Coord dist)
  {
    const db::Cell &cell = mp_subject_layout->cell (inst->object ().cell_index ());
    db::box_convert <db::CellInst, true> inst_bc (*mp_subject_layout, m_subject_layer);
    db::Box rbox = db::box_convert<TI> () (ref);

    for (db::CellInstArray::iterator n = inst->begin_touching (safe_box_enlarged (rbox, dist - 1, dist - 1), inst_bc); ! n.at_end (); ++n) {

      db::ICplxTrans tn = inst->complex_trans (*n);
      db::Box cbox = (tn * cell.bbox (m_subject_layer)).enlarged (db::Vector (dist, dist)) & rbox.enlarged (db::Vector (dist, dist));

      if (! cbox.empty ()) {

        db::ICplxTrans tni = tn.inverted ();
        m_rt.set_trans (tni);

        std::set<TI> *shapes = 0;

        //  not very strong, but already useful: the cells interact if there is a layer in cell
        //  in the common box
        //  NOTE: don't use overlapping mode here, because this will not select point-like objects as texts or
        //  dot edges. Instead safe-shrink the search box and use touching mode.
        for (db::RecursiveShapeIterator s (*mp_subject_layout, cell, m_subject_layer, safe_box_enlarged (tni * cbox, -1, -1), false); !s.at_end (); ++s) {
          if (! shapes) {
            shapes = & (*mp_result) [std::make_pair (cell.cell_index (), tn)].second [layer];
          }
          shapes->insert (m_rt (ref));
        }
      }

    }
  }
};

}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorContextComputationTask implementation

template <class TS, class TI, class TR>
local_processor_context_computation_task<TS, TI, TR>::local_processor_context_computation_task (const local_processor<TS, TI, TR> *proc, local_processor_contexts<TS, TI, TR> &contexts, db::local_processor_cell_context<TS, TI, TR> *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, db::Coord dist)
  : tl::Task (),
    mp_proc (proc), mp_contexts (&contexts), mp_parent_context (parent_context),
    mp_subject_parent (subject_parent), mp_subject_cell (subject_cell), m_subject_cell_inst (subject_cell_inst),
    mp_intruder_cell (intruder_cell), m_dist (dist)
{
  //  This is quick, but will take away the intruders from the caller
  m_intruders.swap (intruders);
}

template <class TS, class TI, class TR>
void
local_processor_context_computation_task<TS, TI, TR>::perform ()
{
  mp_proc->compute_contexts (*mp_contexts, mp_parent_context, mp_subject_parent, mp_subject_cell, m_subject_cell_inst, mp_intruder_cell, m_intruders, m_dist);
}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorResultComputationTask implementation

template <class TS, class TI, class TR>
local_processor_result_computation_task<TS, TI, TR>::local_processor_result_computation_task (const local_processor<TS, TI, TR> *proc, local_processor_contexts<TS, TI, TR> &contexts, db::Cell *cell, local_processor_cell_contexts<TS, TI, TR> *cell_contexts, const local_operation<TS, TI, TR> *op, const std::vector<unsigned int> &output_layers)
  : mp_proc (proc), mp_contexts (&contexts), mp_cell (cell), mp_cell_contexts (cell_contexts), mp_op (op), m_output_layers (output_layers)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
void
local_processor_result_computation_task<TS, TI, TR>::perform ()
{
  mp_cell_contexts->compute_results (*mp_contexts, mp_cell, mp_op, m_output_layers, mp_proc);

  //  erase the contexts we don't need any longer
  {
    tl::MutexLocker locker (& mp_contexts->lock ());

#if defined(ENABLE_DB_HP_SANITY_ASSERTIONS)
    std::set<const db::local_processor_cell_context<TS, TI, TR> *> td;
    for (typename db::local_processor_cell_contexts<TS, TI, TR>::iterator i = mp_cell_contexts->begin (); i != mp_cell_contexts->end (); ++i) {
      td.insert (&i->second);
    }
    for (typename db::local_processor_cell_contexts<TS, TI, TR>::contexts_per_cell_type::iterator pcc = mp_contexts->context_map ().begin (); pcc != mp_contexts->context_map ().end (); ++pcc) {
      for (typename db::local_processor_cell_contexts<TS, TI, TR>::iterator i = pcc->second.begin (); i != pcc->second.end (); ++i) {
        for (typename db::local_processor_cell_context<TS, TI, TR>::drop_iterator j = i->second.begin_drops (); j != i->second.end_drops (); ++j) {
          if (td.find (j->parent_context) != td.end ()) {
            tl_assert (false);
          }
        }
      }
    }
#endif

    mp_contexts->context_map ().erase (mp_cell);
  }
}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorBase implementation

LocalProcessorBase::LocalProcessorBase ()
  : m_report_progress (true), m_nthreads (0), m_max_vertex_count (0), m_area_ratio (0.0), m_boolean_core (false),
    m_base_verbosity (30), mp_vars (0), mp_current_cell (0)
{
  //  .. nothing yet ..
}

db::Coord
LocalProcessorBase::dist_for_cell (const db::Cell *cell, db::Coord dist) const
{
  return cell ? dist_for_cell (cell->cell_index (), dist) : dist;
}

db::Coord
LocalProcessorBase::dist_for_cell (db::cell_index_type cell_index, db::Coord dist) const
{
  if (mp_vars) {

    const db::ICplxTrans &tr = mp_vars->single_variant_transformation (cell_index);
    double mag = tr.mag ();
    return db::coord_traits<db::Coord>::rounded (dist / mag);

  } else {
    return dist;
  }
}

// ---------------------------------------------------------------------------------------------
//  LocalProcessor implementation

template <class TS, class TI, class TR>
local_processor<TS, TI, TR>::local_processor (db::Layout *layout, db::Cell *top, const std::set<db::cell_index_type> *breakout_cells)
  : mp_subject_layout (layout), mp_intruder_layout (layout),
    mp_subject_top (top), mp_intruder_top (top),
    mp_subject_breakout_cells (breakout_cells), mp_intruder_breakout_cells (breakout_cells),
    m_progress (0), mp_progress (0)
{
  set_boolean_core (default_boolean_core<TR> () ());
}

template <class TS, class TI, class TR>
local_processor<TS, TI, TR>::local_processor (db::Layout *subject_layout, db::Cell *subject_top, const db::Layout *intruder_layout, const db::Cell *intruder_top, const std::set<cell_index_type> *subject_breakout_cells, const std::set<cell_index_type> *intruder_breakout_cells)
  : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout),
    mp_subject_top (subject_top), mp_intruder_top (intruder_top),
    mp_subject_breakout_cells (subject_breakout_cells), mp_intruder_breakout_cells (intruder_breakout_cells),
    m_progress (0), mp_progress (0)
{
  set_boolean_core (default_boolean_core<TR> () ());
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::next () const
{
  static tl::Mutex s_lock;
  tl::MutexLocker locker (&s_lock);
  ++m_progress;

  tl::RelativeProgress *rp = dynamic_cast<tl::RelativeProgress *> (mp_progress);
  if (rp) {
    rp->set (m_progress);
  }
}

template <class TS, class TI, class TR>
size_t local_processor<TS, TI, TR>::get_progress () const
{
  size_t p = 0;
  {
    static tl::Mutex s_lock;
    tl::MutexLocker locker (&s_lock);
    p = m_progress;
  }
  return p;
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer, bool make_variants)
{
  std::vector<unsigned int> ol, il;
  ol.push_back (output_layer);
  il.push_back (intruder_layer);
  run (op, subject_layer, il, ol, make_variants);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, unsigned int intruder_layer, const std::vector<unsigned int> &output_layers, bool make_variants)
{
  std::vector<unsigned int> ol, il;
  il.push_back (intruder_layer);
  run (op, subject_layer, il, output_layers, make_variants);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, const std::vector<unsigned int> &intruder_layers, unsigned int output_layer, bool make_variants)
{
  std::vector<unsigned int> ol;
  ol.push_back (output_layer);
  run (op, subject_layer, intruder_layers, ol, make_variants);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, const std::vector<unsigned int> &intruder_layers, const std::vector<unsigned int> &output_layers, bool make_variants)
{
  tl::SelfTimer timer (tl::verbosity () > base_verbosity (), tl::to_string (tr ("Executing ")) + description (op));

  set_vars_owned (0);

  //  Prepare cell variants if needed
  if (make_variants) {

    tl::SelfTimer timer (tl::verbosity () > base_verbosity () + 10, tl::to_string (tr ("Cell variant formation")));

    auto op_vars = op->vars ();
    if (op_vars) {

      db::VariantsCollectorBase *coll = new db::VariantsCollectorBase (op_vars);
      set_vars_owned (coll);

      coll->collect (mp_subject_layout, mp_subject_top->cell_index ());
      coll->separate_variants ();

      if (mp_intruder_layout != mp_subject_layout) {
        db::VariantsCollectorBase vci (op_vars);
        //  NOTE: we don't plan to use separate_variants, so the const cast is in order
        vci.collect (const_cast<db::Layout *> (mp_intruder_layout), mp_intruder_top->cell_index ());
        if (vci.has_variants ()) {
          //  intruder layout needs to be the same one in that case - we do not want the secondary layout to be modified
          throw tl::Exception (tl::to_string (tr ("Can't modify second layout for cell variant formation - this case is not supported as of now")));
        }
      }

    }

  }

  local_processor_contexts<TS, TI, TR> contexts;
  compute_contexts (contexts, op, subject_layer, intruder_layers);
  compute_results (contexts, op, output_layers);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::push_results (db::Cell *cell, unsigned int output_layer, const std::unordered_set<TR> &result) const
{
  if (! result.empty ()) {
    tl::MutexLocker locker (&cell->layout ()->lock ());
    cell->shapes (output_layer).insert (result.begin (), result.end ());
  }
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::compute_contexts (local_processor_contexts<TS, TI, TR> &contexts, const local_operation<TS, TI, TR> *op, unsigned int subject_layer, const std::vector<unsigned int> &intruder_layers) const
{
  try {

    tl::SelfTimer timer (tl::verbosity () > base_verbosity () + 10, tl::to_string (tr ("Computing contexts for ")) + description (op));

    if (threads () > 0) {
      mp_cc_job.reset (new tl::Job<local_processor_context_computation_worker<TS, TI, TR> > (threads ()));
    } else {
      mp_cc_job.reset (0);
    }

    contexts.clear ();
    contexts.set_intruder_layers (intruder_layers);
    contexts.set_subject_layer (subject_layer);

    typename local_processor_cell_contexts<TS, TI, TR>::context_key_type intruders;
    issue_compute_contexts (contexts, 0, 0, mp_subject_top, db::ICplxTrans (), mp_intruder_top, intruders, op->dist ());

    if (mp_cc_job.get ()) {
      mp_cc_job->start ();
      mp_cc_job->wait ();
    }

  } catch (...) {
    mp_cc_job.reset (0);
    throw;
  }
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::issue_compute_contexts (local_processor_contexts<TS, TI, TR> &contexts,
                                                 db::local_processor_cell_context<TS, TI, TR> *parent_context,
                                                 db::Cell *subject_parent,
                                                 db::Cell *subject_cell,
                                                 const db::ICplxTrans &subject_cell_inst,
                                                 const db::Cell *intruder_cell,
                                                 typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders,
                                                 db::Coord dist) const
{
  bool is_small_job = subject_cell->begin ().at_end ();

  if (! is_small_job && mp_cc_job.get ()) {
    mp_cc_job->schedule (new local_processor_context_computation_task<TS, TI, TR> (this, contexts, parent_context, subject_parent, subject_cell, subject_cell_inst, intruder_cell, intruders, dist));
  } else {
    compute_contexts (contexts, parent_context, subject_parent, subject_cell, subject_cell_inst, intruder_cell, intruders, dist);
  }
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::compute_contexts (local_processor_contexts<TS, TI, TR> &contexts,
                                                    db::local_processor_cell_context<TS, TI, TR> *parent_context,
                                                    db::Cell *subject_parent,
                                                    db::Cell *subject_cell,
                                                    const db::ICplxTrans &subject_cell_inst,
                                                    const db::Cell *intruder_cell,
                                                    const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders,
                                                    db::Coord dist_top) const
{
  CRONOLOGY_COLLECTION_BRACKET(event_compute_contexts)

  if (tl::verbosity () >= base_verbosity () + 20) {
    if (! subject_parent) {
      tl::log << tr ("Computing context for top cell ") << mp_subject_layout->cell_name (subject_cell->cell_index ());
    } else {
      tl::log << tr ("Computing context for ") << mp_subject_layout->cell_name (subject_parent->cell_index ()) << " -> " << mp_subject_layout->cell_name (subject_cell->cell_index ()) << " @" << subject_cell_inst.to_string ();
    }
  }

  db::Coord dist = dist_for_cell (subject_cell->cell_index (), dist_top);

  db::local_processor_cell_context<TS, TI, TR> *cell_context = 0;

  //  prepare a new cell context: this has to happen in a thread-safe way as we share the contexts
  //  object between threads

  {
    tl::MutexLocker locker (& contexts.lock ());

    db::local_processor_cell_contexts<TS, TI, TR> &cell_contexts = contexts.contexts_per_cell (subject_cell, intruder_cell);

#if defined(ENABLE_DB_HP_SANITY_ASSERTIONS)
    if (subject_parent) {
      typename db::local_processor_cell_contexts<TS, TI, TR>::contexts_per_cell_type::iterator pcc = contexts.context_map ().find (subject_parent);
      if (pcc == contexts.context_map ().end ()) {
        tl_assert (false);
      }
      tl_assert (pcc->first == subject_parent);
      bool any = false;
      for (typename db::local_processor_cell_contexts<TS, TI, TR>::iterator pcci = pcc->second.begin (); pcci != pcc->second.end () && !any; ++pcci) {
        any = (&pcci->second == parent_context);
      }
      if (!any) {
        tl_assert (false);
      }
    }
 #endif

    cell_context = cell_contexts.find_context (intruders);
    if (cell_context) {
      //  we already have a context for this intruder scheme
      cell_context->add (parent_context, subject_parent, subject_cell_inst);
      return;
    }

    cell_context = cell_contexts.create (intruders);
    cell_context->add (parent_context, subject_parent, subject_cell_inst);
  }

  //  perform the actual task ..

  CRONOLOGY_COLLECTION_BRACKET(event_compute_contexts_unlocked)
  std::map<unsigned int, const db::Shapes *> intruder_shapes;
  if (intruder_cell) {
    for (std::vector<unsigned int>::const_iterator l = contexts.intruder_layers ().begin (); l != contexts.intruder_layers ().end (); ++l) {
      const db::Shapes *s = &intruder_cell->shapes (contexts.actual_intruder_layer (*l));
      if (! s->empty ()) {
        intruder_shapes.insert (std::make_pair (*l, s));
      }
    }
  }

  db::box_convert <db::CellInstArray, true> inst_bcs (*mp_subject_layout, contexts.subject_layer ());

  //  handle top-down interactions (subject instances interacting with intruder shapes)
  //  and sibling interactions

  if (! subject_cell->begin ().at_end ()) {

    //  Key: single instance given by cell index and transformation
    //  Value the contexts for the child cell for this instance
    typedef typename local_processor_cell_contexts<TS, TI, TR>::context_key_type interactions_value_type;
    typedef std::unordered_map<std::pair<db::cell_index_type, db::ICplxTrans>, interactions_value_type> interactions_type;
    interactions_type interactions;

    //  insert dummy interactions to handle at least the child cell vs. itself
    //  - this is important so we will always handle the instances unless they are
    //  entirely empty in the subject layer
    for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
      if (! inst_bcs (i->cell_inst ()).empty ()) {
        db::cell_index_type ci = i->cell_inst ().object ().cell_index ();
        for (db::CellInstArray::iterator n = i->begin (); ! n.at_end (); ++n) {
          db::ICplxTrans tn = i->complex_trans (*n);
          interactions.insert (std::make_pair (std::make_pair (ci, tn), interactions_value_type ()));
        }
      }
    }

    //  TODO: can we shortcut this if interactions is empty?
    for (std::vector<unsigned int>::const_iterator il = contexts.intruder_layers ().begin (); il != contexts.intruder_layers ().end (); ++il) {

      db::box_convert <db::CellInstArray, true> inst_bci (*mp_intruder_layout, contexts.actual_intruder_layer (*il));

      db::box_scanner2<db::CellInstArray, int, db::CellInstArray, int> scanner;
      interaction_registration_inst2inst<TS, TI, TR> rec (mp_subject_layout, contexts.subject_layer (), mp_intruder_layout, contexts.actual_intruder_layer (*il), contexts.is_foreign (*il), dist, &interactions);

      unsigned int id = 0;

      if (subject_cell == intruder_cell) {

        //  Use the same id's for same instances - this way we can easily detect same instances
        //  and don't make them self-interacting

        for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
          unsigned int iid = ++id;
          if (! inst_bcs (i->cell_inst ()).empty () && ! subject_cell_is_breakout (i->cell_index ())) {
            scanner.insert1 (&i->cell_inst (), iid);
          }
          if (! inst_bci (i->cell_inst ()).empty () && ! intruder_cell_is_breakout (i->cell_index ())) {
            scanner.insert2 (&i->cell_inst (), iid);
          }
        }

      } else {

        for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
          if (! inst_bcs (i->cell_inst ()).empty () && ! subject_cell_is_breakout (i->cell_index ())) {
            scanner.insert1 (&i->cell_inst (), ++id);
          }
        }

        if (intruder_cell) {
          for (db::Cell::const_iterator i = intruder_cell->begin (); !i.at_end (); ++i) {
            if (! inst_bci (i->cell_inst ()).empty () && ! intruder_cell_is_breakout (i->cell_index ())) {
              scanner.insert2 (&i->cell_inst (), ++id);
            }
          }
        }

      }

      for (std::set<db::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
        if (! inst_bci (*i).empty ()) {
          scanner.insert2 (i.operator-> (), ++id);
        }
      }

      scanner.process (rec, dist, inst_bcs, inst_bci);

    }

    //  TODO: can we shortcut this if interactions is empty?
    {
      db::box_scanner2<db::CellInstArray, int, TI, int> scanner;
      db::addressable_object_from_shape<TI> heap;
      interaction_registration_inst2shape<TS, TI, TR> rec (mp_subject_layout, contexts.subject_layer (), dist, &interactions);

      for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
        if (! inst_bcs (i->cell_inst ()).empty () && ! subject_cell_is_breakout (i->cell_index ())) {
          scanner.insert1 (&i->cell_inst (), 0);
        }
      }

      for (typename std::map<unsigned int, std::set<TI> >::const_iterator il = intruders.second.begin (); il != intruders.second.end (); ++il) {
        for (typename std::set<TI>::const_iterator i = il->second.begin (); i != il->second.end (); ++i) {
          scanner.insert2 (i.operator-> (), il->first);
        }
      }

      for (std::map<unsigned int, const db::Shapes *>::const_iterator im = intruder_shapes.begin (); im != intruder_shapes.end (); ++im) {
        for (db::Shapes::shape_iterator i = im->second->begin (shape_flags<TI> ()); !i.at_end (); ++i) {
          scanner.insert2 (heap (*i), im->first);
        }
      }

      scanner.process (rec, dist, inst_bcs, db::box_convert<TI> ());
    }

    //  produce the tasks for computing the next-level interactions
    for (typename interactions_type::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      db::Cell *subject_child_cell = &mp_subject_layout->cell (i->first.first);
      db::Cell *intruder_child_cell = (subject_cell == intruder_cell ? subject_child_cell : 0);

      issue_compute_contexts (contexts, cell_context, subject_cell, subject_child_cell, i->first.second, intruder_child_cell, i->second, dist);

    }

  }

}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::compute_results (local_processor_contexts<TS, TI, TR> &contexts, const local_operation<TS, TI, TR> *op, const std::vector<unsigned int> &output_layers) const
{
#if 0
  //  debugging
  dump_cell_contexts (contexts, mp_subject_layout, mp_intruder_layout ? mp_intruder_layout : mp_subject_layout);
#endif

  tl::SelfTimer timer (tl::verbosity () > base_verbosity () + 10, tl::to_string (tr ("Computing results for ")) + description (op));

  //  avoids updates while we work on the layout
  mp_subject_layout->update ();
  db::LayoutLocker layout_update_locker (mp_subject_layout);

  //  prepare a progress for the computation tasks
  size_t comp_effort = 0;
  if (report_progress ()) {
    for (typename local_processor_contexts<TS, TI, TR>::iterator c = contexts.begin (); c != contexts.end (); ++c) {
      comp_effort += c->second.size ();
    }
  }

  tl::RelativeProgress progress (description (op), comp_effort, 1);
  m_progress = 0;
  mp_progress = 0;

  if (threads () > 0) {

    std::unique_ptr<tl::Job<local_processor_result_computation_worker<TS, TI, TR> > > rc_job (new tl::Job<local_processor_result_computation_worker<TS, TI, TR> > (threads ()));

    //  schedule computation jobs in "waves": we need to make sure they are executed
    //  bottom-up. So we identify a new bunch of cells each time we pass through the cell set
    //  and proceed until all cells are removed.

    std::vector<db::cell_index_type> cells_bu;
    cells_bu.reserve (mp_subject_layout->cells ());
    for (db::Layout::bottom_up_const_iterator bu = mp_subject_layout->begin_bottom_up (); bu != mp_subject_layout->end_bottom_up (); ++bu) {
      cells_bu.push_back (*bu);
    }

    int iter = 0;
    while (true) {

      ++iter;
      tl::SelfTimer timer (tl::verbosity () > base_verbosity () + 10, tl::sprintf (tl::to_string (tr ("Computing results iteration #%d")), iter));

      bool any = false;
      std::unordered_set<db::cell_index_type> later;

      std::vector<db::cell_index_type> next_cells_bu;
      next_cells_bu.reserve (cells_bu.size ());

      for (std::vector<db::cell_index_type>::const_iterator bu = cells_bu.begin (); bu != cells_bu.end (); ++bu) {

        tl::MutexLocker locker (& contexts.lock ());

        typename local_processor_contexts<TS, TI, TR>::iterator cpc = contexts.context_map ().find (&mp_subject_layout->cell (*bu));
        if (cpc != contexts.context_map ().end ()) {

          if (later.find (*bu) == later.end ()) {

            rc_job->schedule (new local_processor_result_computation_task<TS, TI, TR> (this, contexts, cpc->first, &cpc->second, op, output_layers));
            any = true;

          } else {
            next_cells_bu.push_back (*bu);
          }

          for (db::Cell::parent_cell_iterator pc = cpc->first->begin_parent_cells (); pc != cpc->first->end_parent_cells (); ++pc) {
            later.insert (*pc);
          }

        }

      }

      cells_bu.swap (next_cells_bu);

      if (! any) {
        break;
      }

      if (rc_job.get ()) {

        try {

          rc_job->start ();
          while (! rc_job->wait (10)) {
            progress.set (get_progress ());
          }

        } catch (...) {
          rc_job->terminate ();
          throw;
        }

      }

    }

  } else {

    try {

      mp_progress = report_progress () ? &progress : 0;

      for (db::Layout::bottom_up_const_iterator bu = mp_subject_layout->begin_bottom_up (); bu != mp_subject_layout->end_bottom_up (); ++bu) {

        typename local_processor_contexts<TS, TI, TR>::iterator cpc = contexts.context_map ().find (&mp_subject_layout->cell (*bu));
        if (cpc != contexts.context_map ().end ()) {
          cpc->second.compute_results (contexts, cpc->first, op, output_layers, this);
          contexts.context_map ().erase (cpc);
        }

      }

      mp_progress = 0;

    } catch (...) {
      mp_progress = 0;
      throw;
    }

  }
}

namespace {

template <class TS, class TI>
struct scan_shape2shape_same_layer
{
  void
  operator () (const db::Shapes *subject_shapes, unsigned int subject_id0, const std::set<TI> &intruders, unsigned int intruder_layer_index, shape_interactions<TS, TI> &interactions, db::Coord dist) const
  {
    db::box_scanner2<TS, int, TI, int> scanner;
    db::addressable_object_from_shape<TS> heap;
    interaction_registration_shape1<TS, TI> rec (&interactions, intruder_layer_index);

    unsigned int id = subject_id0;
    for (db::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i) {
      const TS *ref = heap (*i);
      scanner.insert1 (ref, id++);
    }

    //  TODO: can we confine this search to the subject's (sized) bounding box?
    for (typename std::set<TI>::const_iterator i = intruders.begin (); i != intruders.end (); ++i) {
      scanner.insert2 (i.operator-> (), interactions.next_id ());
    }

    scanner.process (rec, dist, db::box_convert<TS> (), db::box_convert<TI> ());
  }
};

template <class T>
struct scan_shape2shape_same_layer<T, T>
{
  void
  operator () (const db::Shapes *subject_shapes, unsigned int subject_id0, const std::set<T> &intruders, unsigned int intruder_layer, shape_interactions<T, T> &interactions, db::Coord dist) const
  {
    db::box_scanner<T, int> scanner;
    db::addressable_object_from_shape<T> heap;
    interaction_registration_shape1<T, T> rec (&interactions, intruder_layer);

    unsigned int id = subject_id0;
    for (db::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<T> ()); !i.at_end (); ++i) {
      scanner.insert (heap (*i), id++);
    }

    //  TODO: can we confine this search to the subject's (sized) bounding box?
    for (typename std::set<T>::const_iterator i = intruders.begin (); i != intruders.end (); ++i) {
      scanner.insert (i.operator-> (), interactions.next_id ());
    }

    scanner.process (rec, dist, db::box_convert<T> ());
  }
};

template <class TS, class TI>
struct scan_shape2shape_different_layers
{
  void
  operator () (db::Layout *layout, const db::Shapes *subject_shapes, const db::Shapes *intruder_shapes, unsigned int subject_id0, const std::set<TI> *intruders, unsigned int intruder_layer_index, shape_interactions<TS, TI> &interactions, db::Coord dist) const
  {
    db::box_scanner2<TS, int, TI, int> scanner;
    db::addressable_object_from_shape<TS> sheap;
    db::addressable_object_from_shape<TI> iheap;
    interaction_registration_shape2shape<TS, TI> rec (layout, &interactions, intruder_layer_index);

    unsigned int id = subject_id0;
    for (db::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i, ++id) {
      scanner.insert1 (sheap (*i), id);
    }

    //  TODO: can we confine this search to the subject's (sized) bounding box?
    if (intruders) {
      for (typename std::set<TI>::const_iterator i = intruders->begin (); i != intruders->end (); ++i) {
        scanner.insert2 (i.operator-> (), interactions.next_id ());
      }
    }

    if (intruder_shapes == subject_shapes) {

      //  TODO: can we confine this search to the subject's (sized) bounding box?

      //  special case of intra-layer interactions ("foreign"): mark identical shapes as same so that shapes are not reported interacting with
      //  themselves.
      unsigned int id = subject_id0;
      for (db::Shapes::shape_iterator i = intruder_shapes->begin (shape_flags<TI> ()); !i.at_end (); ++i, ++id) {
        unsigned int iid = interactions.next_id ();
        scanner.insert2 (iheap (*i), iid);
        rec.same (id, iid);
      }

    } else if (intruder_shapes) {

      //  TODO: can we confine this search to the subject's (sized) bounding box?
      for (db::Shapes::shape_iterator i = intruder_shapes->begin (shape_flags<TI> ()); !i.at_end (); ++i) {
        scanner.insert2 (iheap (*i), interactions.next_id ());
      }

    }

    scanner.process (rec, dist, db::box_convert<TS> (), db::box_convert<TI> ());
  }
};

}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::compute_local_cell (const db::local_processor_contexts<TS, TI, TR> &contexts, db::Cell *subject_cell, const db::Cell *intruder_cell, const local_operation<TS, TI, TR> *op, const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, std::vector<std::unordered_set<TR> > &result) const
{
  db::Coord dist = dist_for_cell (subject_cell->cell_index (), op->dist ());

  const db::Shapes *subject_shapes = &subject_cell->shapes (contexts.subject_layer ());
  db::shape_to_object<TS> s2o;

  shape_interactions<TS, TI> interactions;

  //  insert dummy interactions to accommodate subject vs. nothing and assign an ID
  //  range for the subject shapes.
  unsigned int subject_id0 = 0;
  for (db::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i) {

    unsigned int id = interactions.next_id ();
    if (subject_id0 == 0) {
      subject_id0 = id;
    }

    if (op->on_empty_intruder_hint () != OnEmptyIntruderHint::Drop) {
      interactions.add_subject (id, s2o (*i));
    }

  }

  unsigned int il_index = 0;
  for (std::vector<unsigned int>::const_iterator il = contexts.intruder_layers ().begin (); il != contexts.intruder_layers ().end (); ++il, ++il_index) {

    unsigned int ail = contexts.actual_intruder_layer (*il);
    bool foreign = contexts.is_foreign (*il);

    const db::Shapes *intruder_shapes = 0;
    if (intruder_cell) {
      intruder_shapes = &intruder_cell->shapes (ail);
      if (intruder_shapes->empty ()) {
        intruder_shapes = 0;
      }
    }

    //  local shapes vs. child cell

    db::box_convert<db::CellInstArray, true> inst_bci (*mp_intruder_layout, ail);

    typename std::map<unsigned int, std::set<TI> >::const_iterator ipl = intruders.second.find (*il);
    static std::set<TI> empty_intruders;

    if (! subject_shapes->empty () && (intruder_shapes || ipl != intruders.second.end ())) {

      if (subject_cell == intruder_cell && contexts.subject_layer () == ail && !foreign) {

        scan_shape2shape_same_layer<TS, TI> () (subject_shapes, subject_id0, ipl == intruders.second.end () ? empty_intruders : ipl->second, il_index, interactions, dist);

      } else {

        db::Layout *target_layout = (mp_subject_layout == mp_intruder_layout ? 0 : mp_subject_layout);
        scan_shape2shape_different_layers<TS, TI> () (target_layout, subject_shapes, intruder_shapes, subject_id0, &(ipl == intruders.second.end () ? empty_intruders : ipl->second), il_index, interactions, dist);

      }

    }

    if (! subject_shapes->empty () && ! ((! intruder_cell || intruder_cell->begin ().at_end ()) && intruders.first.empty ())) {

      db::box_scanner2<TS, int, db::CellInstArray, int> scanner;
      db::addressable_object_from_shape<TS> heap;
      interaction_registration_shape2inst<TS, TI> rec (mp_subject_layout, mp_intruder_layout, ail, il_index, dist, &interactions);

      unsigned int id = subject_id0;
      for (db::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i) {
        scanner.insert1 (heap (*i), id++);
      }

      unsigned int inst_id = 0;

      if (subject_cell == intruder_cell && contexts.subject_layer () == ail && !foreign) {

        //  Same cell, same layer -> no shape to child instance interactions because this will be taken care of
        //  by the instances themselves (and their intruders). This also means, we prefer to deal with
        //  interactions low in the hierarchy.

      } else if (intruder_cell) {
  //  TODO: can we confine this search to the subject's (sized) bounding box?
        for (db::Cell::const_iterator i = intruder_cell->begin (); !i.at_end (); ++i) {
          if (! inst_bci (i->cell_inst ()).empty () && ! intruder_cell_is_breakout (i->cell_index ())) {
            scanner.insert2 (&i->cell_inst (), ++inst_id);
          }
        }
      }

  //  TODO: can we confine this search to the subject's (sized) bounding box?
      for (std::set<db::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
        if (! inst_bci (*i).empty ()) {
          scanner.insert2 (i.operator-> (), ++inst_id);
        }
      }

      scanner.process (rec, dist, db::box_convert<TS> (), inst_bci);

    }

  }

  if (interactions.begin () != interactions.end ()) {

    if (interactions.begin_intruders () == interactions.end_intruders ()) {

      OnEmptyIntruderHint eh = op->on_empty_intruder_hint ();
      if (eh == OnEmptyIntruderHint::Drop) {
        return;
      }

    }

    op->compute_local (mp_subject_layout, subject_cell, interactions, result, this);

  }
}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::run_flat (const db::Shapes *subject_shapes, const db::Shapes *intruders, const local_operation<TS, TI, TR> *op, db::Shapes *result_shapes) const
{
  std::vector<generic_shape_iterator<TI> > is;
  std::vector<bool> foreign;
  if (intruders == subject_idptr () || intruders == foreign_idptr ()) {
    is.push_back (generic_shape_iterator<TI> (subject_shapes));
    foreign.push_back (intruders == foreign_idptr ());
  } else {
    is.push_back (generic_shape_iterator<TI> (intruders));
    foreign.push_back (false);
  }

  std::vector<db::Shapes *> os;
  os.push_back (result_shapes);

  run_flat (generic_shape_iterator<TS> (subject_shapes), is, foreign, op, os);
}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::run_flat (const db::Shapes *subject_shapes, const std::vector<const db::Shapes *> &intruders, const local_operation<TS, TI, TR> *op, const std::vector<db::Shapes *> &result_shapes) const
{
  std::vector<generic_shape_iterator<TI> > is;
  is.reserve (intruders.size ());

  std::vector<bool> foreign;
  foreign.reserve (intruders.size ());

  for (std::vector<const db::Shapes *>::const_iterator i = intruders.begin (); i != intruders.end (); ++i) {
    if (*i == subject_idptr () || *i == foreign_idptr ()) {
      is.push_back (generic_shape_iterator<TI> (subject_shapes));
      foreign.push_back (*i == foreign_idptr ());
    } else {
      is.push_back (generic_shape_iterator<TI> (*i));
      foreign.push_back (false);
    }
  }

  run_flat (generic_shape_iterator<TS> (subject_shapes), is, foreign, op, result_shapes);
}

namespace
{

template <class TS, class TI>
struct interaction_registration_shape1_scanner_combo
{
  interaction_registration_shape1_scanner_combo (shape_interactions<TS, TI> *, unsigned int, bool, const std::string &)
  {
    //  can't have self-interactions with different types
    tl_assert (false);
  }

  void insert (const TS *, unsigned int)
  {
    //  nothing here.
  }

  void process (db::Coord)
  {
    //  nothing here.
  }
};

template <class T>
struct interaction_registration_shape1_scanner_combo<T, T>
{
  interaction_registration_shape1_scanner_combo (shape_interactions<T, T> *interactions, unsigned int intruder_layer_index, bool report_progress, const std::string &progress_description)
    : m_scanner (report_progress, progress_description), m_rec (interactions, intruder_layer_index)
  {
    //  .. nothing yet ..
  }

  void insert (const T *shape, unsigned int id)
  {
    m_scanner.insert (shape, id);
  }

  void process (db::Coord dist)
  {
    m_scanner.process (m_rec, dist, db::box_convert<T> ());
  }

private:
  db::box_scanner<T, int> m_scanner;
  interaction_registration_shape1<T, T> m_rec;
};

}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::run_flat (const generic_shape_iterator<TS> &subjects, const std::vector<generic_shape_iterator<TI> > &intruders, const std::vector<bool> &foreign, const local_operation<TS, TI, TR> *op, const std::vector<db::Shapes *> &result_shapes) const
{
  if (subjects.at_end ()) {
    return;
  }

  tl_assert (mp_subject_top == 0);
  tl_assert (mp_intruder_top == 0);

  std::string process_description, scan_description;

  if (report_progress ()) {

    process_description = description (op);
    if (process_description.empty ()) {
      process_description = tl::to_string (tr ("Processing"));
    } else {
      process_description += tl::to_string (tr (" (processing)"));
    }

    scan_description = description (op);
    if (scan_description.empty ()) {
      scan_description = tl::to_string (tr ("Scanning"));
    } else {
      scan_description += tl::to_string (tr (" (scan)"));
    }

  }

  shape_interactions<TS, TI> interactions;

  bool needs_isolated_subjects = (op->on_empty_intruder_hint () != OnEmptyIntruderHint::Drop);

  //  build the subjects in the intruders list

  db::Coord dist = op->dist ();

  db::Box subjects_box = safe_box_enlarged (subjects.bbox (), dist, dist);

  db::Box intruders_box;
  for (typename std::vector<generic_shape_iterator<TI> >::const_iterator il = intruders.begin (); il != intruders.end (); ++il) {
    intruders_box += il->bbox ();
  }
  intruders_box = safe_box_enlarged (intruders_box, dist, dist);

  db::Box common_box = intruders_box & subjects_box;
  if (common_box.empty () || common_box.width () == 0 || common_box.height () == 0) {

    if (needs_isolated_subjects) {
      for (generic_shape_iterator<TS> is = subjects; ! is.at_end (); ++is) {
        //  create subject for subject vs. nothing interactions
        interactions.add_subject (interactions.next_id (), *is);
      }
    }

  } else {

    common_box = safe_box_enlarged (common_box, -1, -1);

    if (needs_isolated_subjects) {

      unaddressable_shape_delivery<TS> is (subjects);
      for ( ; !is.at_end (); ++is) {
        unsigned int id = interactions.next_id ();
        interactions.add_subject (id, *is);
      }

      unsigned int il_index = 0;
      for (typename std::vector<generic_shape_iterator<TI> >::const_iterator il = intruders.begin (); il != intruders.end (); ++il, ++il_index) {

        bool ff = foreign.size () > il_index && foreign [il_index];

        if (*il == subjects && ! ff) {

          interaction_registration_shape1_scanner_combo<TS, TI> scanner (&interactions, il_index, report_progress (), scan_description);

          for (typename shape_interactions<TS, TI>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
            scanner.insert (&s->second, s->first);
          }

          scanner.process (dist);

        } else {

          db::box_scanner2<TS, unsigned int, TI, unsigned int> scanner (report_progress (), scan_description);
          interaction_registration_shape2shape<TS, TI> rec (0 /*layout*/, &interactions, il_index);

          for (typename shape_interactions<TS, TI>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
            scanner.insert1 (&s->second, s->first);
          }

          if (*il == subjects) {

            //  this is the case of intra-layer interactions ("foreign"): we pretend we have two layers and
            //  reject shape self-interactions by registering them as "same"

            for (typename shape_interactions<TS, TI>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
              unsigned int iid = interactions.next_id ();
              safe_insert2_into_box_scanner (scanner, &s->second, iid);
              rec.same (s->first, iid);
            }

            scanner.process (rec, dist, db::box_convert<TS> (), db::box_convert<TI> ());

          } else {

            addressable_shape_delivery<TI> ii ((*il).confined (common_box, false));
            for (; !ii.at_end (); ++ii) {
              scanner.insert2 (ii.operator-> (), interactions.next_id ());
            }

            scanner.process (rec, dist, db::box_convert<TS> (), db::box_convert<TI> ());

          }

        }
      }

    } else {

      unsigned int id_first = 0;

      {
        //  allocate a range of IDs for the subjects
        generic_shape_iterator<TS> is (subjects);
        if (! is.at_end ()) {
          id_first = interactions.next_id ();
          while (! (++is).at_end ()) {
            interactions.next_id ();
          }
        }
      }

      unsigned int il_index = 0;
      for (typename std::vector<generic_shape_iterator<TI> >::const_iterator il = intruders.begin (); il != intruders.end (); ++il, ++il_index) {

        bool ff = foreign.size () > il_index && foreign [il_index];

        if (*il == subjects && ! ff) {

          interaction_registration_shape1_scanner_combo<TS, TI> scanner (&interactions, il_index, report_progress (), scan_description);

          addressable_shape_delivery<TS> is (subjects.confined (common_box, false));
          unsigned int id = id_first;

          for ( ; ! is.at_end (); ++is, ++id) {
            scanner.insert (is.operator-> (), id);
          }

          scanner.process (dist);

        } else {

          db::box_scanner2<TS, unsigned int, TI, unsigned int> scanner (report_progress (), scan_description);
          interaction_registration_shape2shape<TS, TI> rec (0 /*layout*/, &interactions, il_index);

          if (*il == subjects) {

            //  this is the case of intra-layer interactions ("foreign"): we pretend we have two layers and
            //  reject shape self-interactions by registering them as "same"

            addressable_shape_delivery<TS> is (subjects.confined (common_box, false));

            unsigned int id = id_first;
            for ( ; ! is.at_end (); ++is, ++id) {
              unsigned int iid = interactions.next_id ();
              scanner.insert1 (is.operator-> (), id);
              safe_insert2_into_box_scanner (scanner, is.operator-> (), iid);
              rec.same (id, iid);
            }

            scanner.process (rec, dist, db::box_convert<TS> (), db::box_convert<TI> ());

          } else {

            addressable_shape_delivery<TS> is (subjects.confined (common_box, false));
            addressable_shape_delivery<TI> ii ((*il).confined (common_box, false));

            unsigned int id = id_first;
            for ( ; ! is.at_end (); ++is, ++id) {
              scanner.insert1 (is.operator-> (), id);
            }
            for (; !ii.at_end (); ++ii) {
              scanner.insert2 (ii.operator-> (), interactions.next_id ());
            }

            scanner.process (rec, dist, db::box_convert<TS> (), db::box_convert<TI> ());

          }

        }
      }

    }

  }

  if (interactions.begin () != interactions.end ()) {

    if (interactions.begin_intruders () == interactions.end_intruders ()) {

      OnEmptyIntruderHint eh = op->on_empty_intruder_hint ();
      if (eh == OnEmptyIntruderHint::Drop) {
        return;
      }

    }

    std::vector<std::unordered_set<TR> > result;
    result.resize (result_shapes.size ());
    op->compute_local (mp_subject_layout, 0, interactions, result, this);

    for (std::vector<db::Shapes *>::const_iterator r = result_shapes.begin (); r != result_shapes.end (); ++r) {
      if (*r) {
        const std::unordered_set<TR> rs = result [r - result_shapes.begin ()];
        (*r)->insert (rs.begin (), rs.end ());
      }
    }

  }
}

// ---------------------------------------------------------------------------------------------

//  NOTE: don't forget to update the explicit instantiations in dbLocalOperation.cc

//  explicit instantiations
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Text, db::Polygon>;
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Edge, db::Polygon>;
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Text, db::Text>;
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::Polygon, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::Edge, db::EdgePair>;

//  explicit instantiations
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Edge, db::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Text, db::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Text, db::Text>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::Edge, db::EdgePair>;

//  explicit instantiations
template class DB_PUBLIC shape_interactions<db::Polygon, db::Polygon>;
template class DB_PUBLIC shape_interactions<db::Polygon, db::Text>;
template class DB_PUBLIC shape_interactions<db::Polygon, db::TextRef>;
template class DB_PUBLIC shape_interactions<db::Polygon, db::Edge>;
template class DB_PUBLIC shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC shape_interactions<db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC shape_interactions<db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC shape_interactions<db::PolygonRef, db::TextRef>;
template class DB_PUBLIC shape_interactions<db::PolygonRef, db::Text>;
template class DB_PUBLIC shape_interactions<db::PolygonRef, db::Edge>;
template class DB_PUBLIC shape_interactions<db::Edge, db::Edge>;
template class DB_PUBLIC shape_interactions<db::Edge, db::PolygonRef>;
template class DB_PUBLIC shape_interactions<db::TextRef, db::TextRef>;
template class DB_PUBLIC shape_interactions<db::TextRef, db::PolygonRef>;

//  explicit instantiations
template class DB_PUBLIC local_processor_context_computation_task<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_context_computation_task<db::Polygon, db::Text, db::Polygon>;
template class DB_PUBLIC local_processor_context_computation_task<db::Polygon, db::Text, db::Text>;
template class DB_PUBLIC local_processor_context_computation_task<db::Polygon, db::Edge, db::Polygon>;
template class DB_PUBLIC local_processor_context_computation_task<db::Polygon, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRef, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_context_computation_task<db::TextRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_context_computation_task<db::TextRef, db::PolygonRef, db::TextRef>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRef, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRef, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_context_computation_task<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_context_computation_task<db::Polygon, db::Polygon, db::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<db::Edge, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_processor_context_computation_task<db::Edge, db::PolygonRef, db::PolygonRef>;

//  explicit instantiations
template class DB_PUBLIC local_processor_result_computation_task<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_result_computation_task<db::Polygon, db::Text, db::Polygon>;
template class DB_PUBLIC local_processor_result_computation_task<db::Polygon, db::Text, db::Text>;
template class DB_PUBLIC local_processor_result_computation_task<db::Polygon, db::Edge, db::Polygon>;
template class DB_PUBLIC local_processor_result_computation_task<db::Polygon, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_processor_result_computation_task<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_result_computation_task<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_result_computation_task<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_result_computation_task<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_result_computation_task<db::Edge, db::Edge, db::EdgePair>;

//  explicit instantiations
template class DB_PUBLIC local_processor<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor<db::Polygon, db::Text, db::Polygon>;
template class DB_PUBLIC local_processor<db::Polygon, db::Text, db::Text>;
template class DB_PUBLIC local_processor<db::Polygon, db::Edge, db::Polygon>;
template class DB_PUBLIC local_processor<db::Polygon, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC local_processor<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePair>;
template class DB_PUBLIC local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePairWithProperties>;
template class DB_PUBLIC local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_processor<db::PolygonRef, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class DB_PUBLIC local_processor<db::PolygonRef, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_processor<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor<db::PolygonRef, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor<db::Polygon, db::Polygon, db::Edge>;
template class DB_PUBLIC local_processor<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor<db::Edge, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor<db::Edge, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_processor<db::TextRef, db::PolygonRef, db::TextRef>;
template class DB_PUBLIC local_processor<db::TextRef, db::PolygonRef, db::PolygonRef>;

}

