
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
#include "dbPolygonGenerators.h"
#include "tlLog.h"

namespace db
{


// ---------------------------------------------------------------------------------------------
//  BoolAndOrNotLocalOperation implementation

namespace {

class PolygonRefGenerator
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  PolygonRefGenerator (db::Layout *layout, std::set<db::PolygonRef> &polyrefs)
    : PolygonSink (), mp_layout (layout), mp_polyrefs (&polyrefs)
  { }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon)
  {
    mp_polyrefs->insert (db::PolygonRef (polygon, mp_layout->shape_repository ()));
  }

private:
  db::Layout *mp_layout;
  std::set<db::PolygonRef> *mp_polyrefs;
};

}

BoolAndOrNotLocalOperation::BoolAndOrNotLocalOperation (bool is_and)
  : m_is_and (is_and)
{
  //  .. nothing yet ..
}

void
BoolAndOrNotLocalOperation::compute_local (db::Layout *layout, const std::map<db::PolygonRef, std::vector<db::PolygonRef> > &interactions, std::set<db::PolygonRef> &result) const
{
  db::EdgeProcessor ep;

  size_t p1 = 0, p2 = 1;

  std::set<db::PolygonRef> others;
  for (std::map<db::PolygonRef, std::vector<db::PolygonRef> >::const_iterator r = interactions.begin (); r != interactions.end (); ++r) {

    //  TODO: vector could be set
    bool found = false;
    for (std::vector<db::PolygonRef>::const_iterator i = r->second.begin (); i != r->second.end () && ! found; ++i) {
      found = (*i == r->first);
    }
    if (found) {
      //  shortcut (and: keep, not: drop)
      if (m_is_and) {
        result.insert (r->first);
      }
    } else if (r->second.empty ()) {
      //  shortcut (not: keep, and: drop)
      if (! m_is_and) {
        result.insert (r->first);
      }
    } else {
      for (db::PolygonRef::polygon_edge_iterator e = r->first.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
      others.insert (r->second.begin (), r->second.end ());
    }

  }

  if (! others.empty () || p1 > 0) {

    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p2);
      }
      p2 += 2;
    }

    db::BooleanOp op (m_is_and ? db::BooleanOp::And : db::BooleanOp::ANotB);
    db::PolygonRefGenerator pr (layout, result);
    db::PolygonGenerator pg (pr, true, true);
    ep.process (pg, op);

  }
}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorCellContext implementation

LocalProcessorCellContext::LocalProcessorCellContext ()
{
  //  .. nothing yet ..
}

void
LocalProcessorCellContext::add (db::LocalProcessorCellContext *parent_context, db::Cell *parent, const db::ICplxTrans &cell_inst)
{
  m_drops.push_back (LocalProcessorCellDrop (parent_context, parent, cell_inst));
}

void
LocalProcessorCellContext::propagate (const std::set<db::PolygonRef> &res)
{
  if (res.empty ()) {
    return;
  }

  for (std::vector<LocalProcessorCellDrop>::const_iterator d = m_drops.begin (); d != m_drops.end (); ++d) {

    tl_assert (d->parent_context != 0);
    tl_assert (d->parent != 0);

    db::Layout *layout = d->parent->layout ();

    for (std::set<db::PolygonRef>::const_iterator r = res.begin (); r != res.end (); ++r) {
      db::Polygon poly = r->obj ().transformed (d->cell_inst * db::ICplxTrans (r->trans ()));
      d->parent_context->propagated ().insert (db::PolygonRef (poly, layout->shape_repository ()));
    }

  }

}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorCellContexts implementation

LocalProcessorCellContexts::LocalProcessorCellContexts ()
{
  //  .. nothing yet ..
}

db::LocalProcessorCellContext *
LocalProcessorCellContexts::find_context (const key_type &intruders)
{
  std::map<key_type, db::LocalProcessorCellContext>::iterator c = m_contexts.find (intruders);
  return c != m_contexts.end () ? &c->second : 0;
}

db::LocalProcessorCellContext *
LocalProcessorCellContexts::create (const key_type &intruders)
{
  return &m_contexts[intruders];
}

void
LocalProcessorCellContexts::compute_results (db::Cell *cell, LocalProcessor *proc)
{
  bool first = true;
  std::set<db::PolygonRef> common;

  for (std::map<key_type, db::LocalProcessorCellContext>::iterator c = m_contexts.begin (); c != m_contexts.end (); ++c) {

    if (first) {

      common = c->second.propagated ();
      proc->compute_local_cell (cell, c->first, common);
      first = false;

    } else {

      std::set<db::PolygonRef> res = c->second.propagated ();
      proc->compute_local_cell (cell, c->first, res);

      if (common.empty ()) {

        c->second.propagate (res);

      } else if (res != common) {

        std::set<db::PolygonRef> lost;
        std::set_difference (common.begin (), common.end (), res.begin (), res.end (), std::inserter (lost, lost.end ()));

        if (! lost.empty ()) {

          std::set<db::PolygonRef> new_common;
          std::set_intersection (common.begin (), common.end (), res.begin (), res.end (), std::inserter (new_common, new_common.end ()));
          common.swap (new_common);

          for (std::map<key_type, db::LocalProcessorCellContext>::iterator cc = m_contexts.begin (); cc != c; ++cc) {
            cc->second.propagate (lost);
          }

        }

        std::set<db::PolygonRef> gained;
        std::set_difference (res.begin (), res.end (), common.begin (), common.end (), std::inserter (gained, gained.end ()));
        c->second.propagate (gained);

      }

    }

  }

  proc->push_results (cell, common);
}

// ---------------------------------------------------------------------------------------------
//  Helper classes for the LocalProcessor

namespace
{

inline unsigned int polygon_ref_flags ()
{
  return 1 << db::ShapeIterator::PolygonRef;
}

struct InteractionRegistrationShape2Shape
  : db::box_scanner_receiver2<db::PolygonRef, int, db::PolygonRef, int>
{
public:
  InteractionRegistrationShape2Shape (std::map<db::PolygonRef, std::vector<db::PolygonRef> > *result)
    : mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::PolygonRef *ref1, int, const db::PolygonRef *ref2, int)
  {
    (*mp_result) [*ref1].push_back (*ref2);
  }

private:
  std::map<db::PolygonRef, std::vector<db::PolygonRef> > *mp_result;
};

struct InteractionRegistrationShape2Inst
  : db::box_scanner_receiver2<db::PolygonRef, int, db::CellInstArray, int>
{
public:
  InteractionRegistrationShape2Inst (db::Layout *layout, unsigned int intruder_layer, std::map<db::PolygonRef, std::vector<db::PolygonRef> > *result)
    : mp_layout (layout), m_intruder_layer (intruder_layer), m_inst_bc (*layout, intruder_layer), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::PolygonRef *ref, int, const db::CellInstArray *inst, int)
  {
    const db::Cell &intruder_cell = mp_layout->cell (inst->object ().cell_index ());
    db::Box region = ref->box () & m_inst_bc (*inst);
    if (! region.empty ()) {

      //  @@@ TODO: should be lighter, cache, handle arrays ..
      db::RecursiveShapeIterator si (*mp_layout, intruder_cell, m_intruder_layer, region);
      si.shape_flags (polygon_ref_flags ());
      while (! si.at_end ()) {

        //  @@@ should be easier to transform references
        const db::PolygonRef *ref2 = si.shape ().basic_ptr (db::PolygonRef::tag ());
        db::Polygon poly = ref2->obj ().transformed (si.trans () * db::ICplxTrans (ref->trans ()));
        (*mp_result)[*ref].push_back (db::PolygonRef (poly, mp_layout->shape_repository()));

        ++si;

      }

    }
  }

private:
  db::Layout *mp_layout;
  unsigned int m_intruder_layer;
  db::box_convert <db::CellInstArray, true> m_inst_bc;
  std::map<db::PolygonRef, std::vector<db::PolygonRef> > *mp_result;
};

struct InteractionRegistrationInst2Inst
  : db::box_scanner_receiver2<db::CellInstArray, int, db::CellInstArray, int>
{
public:
  InteractionRegistrationInst2Inst (std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *result)
    : mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst1, int, const db::CellInstArray *inst2, int)
  {
    (*mp_result) [inst1].first.insert (inst2);
  }

private:
  std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *mp_result;
};

struct InteractionRegistrationInst2Shape
  : db::box_scanner_receiver2<db::CellInstArray, int, db::PolygonRef, int>
{
public:
  InteractionRegistrationInst2Shape (std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *result)
    : mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst, int, const db::PolygonRef *ref, int)
  {
    (*mp_result) [inst].second.insert (*ref);
  }

private:
  std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *mp_result;
};

}

// ---------------------------------------------------------------------------------------------
//  LocalProcessor implementation

LocalProcessor::LocalProcessor (db::Layout *layout, db::Cell *top, LocalOperation *op, unsigned int scope_layer, unsigned int intruder_layer, unsigned int output_layer)
  : mp_layout (layout), mp_top (top), m_scope_layer (scope_layer), m_intruder_layer (intruder_layer), m_output_layer (output_layer), mp_op (op)
{
  //  .. nothing yet ..
}

void LocalProcessor::run ()
{
  try {

    mp_layout->update ();
    mp_layout->start_changes ();

    std::pair<std::set<db::CellInstArray>, std::set<db::PolygonRef> > intruders;
    compute_contexts (0, 0, mp_top, db::ICplxTrans (), intruders);
    compute_results ();

    mp_layout->end_changes ();

  } catch (...) {
    mp_layout->end_changes ();
    throw;
  }
}

void LocalProcessor::push_results (db::Cell *cell, const std::set<db::PolygonRef> &result)
{
  if (! result.empty ()) {
    cell->shapes (m_output_layer).insert (result.begin (), result.end ());
  }
}

void LocalProcessor::compute_contexts (db::LocalProcessorCellContext *parent_context, db::Cell *parent, db::Cell *cell, const db::ICplxTrans &cell_inst, const std::pair<std::set<CellInstArray>, std::set<PolygonRef> > &intruders)
{
  db::LocalProcessorCellContexts &contexts = m_contexts_per_cell [cell];

  db::LocalProcessorCellContext *context = contexts.find_context (intruders);
  if (context) {
    context->add (parent_context, parent, cell_inst);
    return;
  }

  context = contexts.create (intruders);
  context->add (parent_context, parent, cell_inst);

  const db::Shapes &shapes_intruders = cell->shapes (m_intruder_layer);

  db::box_convert <db::CellInstArray, true> inst_bcs (*mp_layout, m_scope_layer);
  db::box_convert <db::CellInstArray, true> inst_bci (*mp_layout, m_intruder_layer);
  db::box_convert <db::CellInst, true> inst_bcii (*mp_layout, m_intruder_layer);

  if (! cell->begin ().at_end ()) {

    typedef std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > interaction_value_type;

    std::map<const db::CellInstArray *, interaction_value_type> interactions;

    // @@@ TODO: don't do this in AND more for instances without an intruder
    for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
      interactions.insert (std::make_pair (&i->cell_inst (), interaction_value_type ()));
    }

    {
      db::box_scanner2<db::CellInstArray, int, db::CellInstArray, int> scanner;
      InteractionRegistrationInst2Inst rec (&interactions);

      for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
        scanner.insert1 (&i->cell_inst (), 0);
        scanner.insert2 (&i->cell_inst (), 0);
      }

      for (std::set<db::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
        scanner.insert2 (i.operator-> (), 0);
      }

      scanner.process (rec, 0, inst_bcs, inst_bci);
    }

    {
      db::box_scanner2<db::CellInstArray, int, db::PolygonRef, int> scanner;
      InteractionRegistrationInst2Shape rec (&interactions);

      for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
        scanner.insert1 (&i->cell_inst (), 0);
      }

      for (std::set<db::PolygonRef>::const_iterator i = intruders.second.begin (); i != intruders.second.end (); ++i) {
        scanner.insert2 (i.operator-> (), 0);
      }
      for (db::Shapes::shape_iterator i = shapes_intruders.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
        scanner.insert2 (i->basic_ptr (db::PolygonRef::tag ()), 0);
      }

      scanner.process (rec, 0, inst_bcs, db::box_convert<db::PolygonRef> ());
    }

    for (std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > >::const_iterator i = interactions.begin (); i != interactions.end (); ++i) {

      std::pair<std::set<db::CellInstArray>, std::set<db::PolygonRef> > intruders_below;
      intruders_below.second = i->second.second;

      db::Cell &child_cell = mp_layout->cell (i->first->object ().cell_index ());

      for (db::CellInstArray::iterator n = i->first->begin (); ! n.at_end (); ++n) {

        db::ICplxTrans tn = i->first->complex_trans (*n);
        db::ICplxTrans tni = tn.inverted ();
        db::Box nbox = tn * child_cell.bbox (m_intruder_layer);

        if (! nbox.empty ()) {

          intruders_below.first.clear ();

          //  @@@ TODO: in some cases, it may be possible to optimize this for arrays

          for (std::set<const db::CellInstArray *>::const_iterator j = i->second.first.begin (); j != i->second.first.end (); ++j) {
            for (db::CellInstArray::iterator k = (*j)->begin_touching (nbox.enlarged (db::Vector (-1, -1)), inst_bcii); ! k.at_end (); ++k) {
              intruders_below.first.insert (db::CellInstArray (db::CellInst ((*j)->object ().cell_index ()), tni * (*j)->complex_trans (*k)));
            }
          }

          compute_contexts (context, cell, &child_cell, tn, intruders_below);

        }

      }

    }

  }
}

void
LocalProcessor::compute_results ()
{
  for (db::Layout::bottom_up_const_iterator bu = mp_layout->begin_bottom_up (); bu != mp_layout->end_bottom_up (); ++bu) {

    contexts_per_cell_type::iterator cpc = m_contexts_per_cell.find (&mp_layout->cell (*bu));
    if (cpc != m_contexts_per_cell.end ()) {
      cpc->second.compute_results (cpc->first, this);
      m_contexts_per_cell.erase (cpc);
    }

  }

}

void
LocalProcessor::compute_local_cell (db::Cell *cell, const std::pair<std::set<CellInstArray>, std::set<db::PolygonRef> > &intruders, std::set<db::PolygonRef> &result) const
{
  const db::Shapes &shapes_scope = cell->shapes (m_scope_layer);
  const db::Shapes &shapes_intruders = cell->shapes (m_intruder_layer);

  //  local shapes vs. child cell

  std::map<db::PolygonRef, std::vector<db::PolygonRef> > interactions;
  db::box_convert <db::CellInstArray, true> inst_bci (*mp_layout, m_intruder_layer);

  // @@@ TODO: don't do this in AND mode (we don't need interactions without an intruder)
  for (db::Shapes::shape_iterator i = shapes_scope.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
    interactions.insert (std::make_pair (*i->basic_ptr (db::PolygonRef::tag ()), std::vector<db::PolygonRef> ()));
  }

  if (! shapes_scope.empty () && ! (shapes_intruders.empty () && intruders.second.empty ())) {

    db::box_scanner2<db::PolygonRef, int, db::PolygonRef, int> scanner;
    InteractionRegistrationShape2Shape rec (&interactions);

    for (db::Shapes::shape_iterator i = shapes_scope.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
      scanner.insert1 (i->basic_ptr (db::PolygonRef::tag ()), 0);
    }

    for (std::set<db::PolygonRef>::const_iterator i = intruders.second.begin (); i != intruders.second.end (); ++i) {
      scanner.insert2 (i.operator-> (), 0);
    }
    for (db::Shapes::shape_iterator i = shapes_intruders.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
      scanner.insert2 (i->basic_ptr (db::PolygonRef::tag ()), 0);
    }

    scanner.process (rec, 0, db::box_convert<db::PolygonRef> (), db::box_convert<db::PolygonRef> ());

  }

  if (! shapes_scope.empty () && ! (cell->begin ().at_end () && intruders.first.empty ())) {

    db::box_scanner2<db::PolygonRef, int, db::CellInstArray, int> scanner;
    InteractionRegistrationShape2Inst rec (mp_layout, m_intruder_layer, &interactions);

    for (db::Shapes::shape_iterator i = shapes_scope.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
      scanner.insert1 (i->basic_ptr (db::PolygonRef::tag ()), 0);
    }

    for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
      scanner.insert2 (&i->cell_inst (), 0);
    }
    for (std::set<db::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
      scanner.insert2 (i.operator-> (), 0);
    }

    scanner.process (rec, 0, db::box_convert<db::PolygonRef> (), inst_bci);

  }

  mp_op->compute_local (mp_layout, interactions, result);
}

}

