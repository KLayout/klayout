
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
#include "dbHierProcessorUtils.h"
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

  std::map<db::properties_id_type, std::pair<std::vector<const db::PolygonRefWithProperties *>, std::vector<const db::PolygonRefWithProperties *> >, ComparePropertiesIds> by_prop_id;
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

  std::map<db::properties_id_type, std::pair<std::vector<const db::EdgeWithProperties *>, std::vector<const db::EdgeWithProperties *> >, ComparePropertiesIds> by_prop_id;
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

//  NOTE: don't forget to update the explicit instantiations in dbHierProcessor.cc

//  explicit instantiations
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Polygon, db::Edge>;
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
template class DB_PUBLIC local_processor_cell_context<db::PolygonWithProperties, db::EdgeWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonWithProperties, db::EdgeWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::PolygonRef, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_processor_cell_context<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::Polygon, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::Edge, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::EdgePair, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::EdgePair, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::EdgePair, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<db::EdgePair, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_cell_context<db::EdgePair, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::EdgePair, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_context<db::TextRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<db::TextRef, db::PolygonRef, db::TextRef>;

//  explicit instantiations
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Polygon, db::Edge>;
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
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonWithProperties, db::EdgeWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonWithProperties, db::EdgeWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::PolygonRef, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::Polygon, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::Edge, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::EdgePair, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::EdgePair, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::EdgePair, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<db::EdgePair, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<db::EdgePair, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::EdgePair, db::Edge, db::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<db::TextRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<db::TextRef, db::PolygonRef, db::TextRef>;

}

