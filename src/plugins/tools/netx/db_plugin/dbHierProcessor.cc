
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
#include "tlTimer.h"
#include "tlInternational.h"

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

LocalOperation::on_empty_intruder_mode
BoolAndOrNotLocalOperation::on_empty_intruder_hint () const
{
  return m_is_and ? LocalOperation::Drop : LocalOperation::Copy;
}

std::string
BoolAndOrNotLocalOperation::description () const
{
  return m_is_and ? tl::to_string (tr ("AND operation")) : tl::to_string (tr ("NOT operation"));
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

  int index = 0;
  int total = int (m_contexts.size ());
  for (std::map<key_type, db::LocalProcessorCellContext>::iterator c = m_contexts.begin (); c != m_contexts.end (); ++c) {

    ++index;

    if (tl::verbosity () >= 30) {
      tl::log << tr ("Computing local results for ") << cell->layout ()->cell_name (cell->cell_index ()) << " (context " << index << "/" << total << ")";
    }

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
    : mp_layout (layout), m_intruder_layer (intruder_layer), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::PolygonRef *ref, int, const db::CellInstArray *inst, int)
  {
    const db::Cell &intruder_cell = mp_layout->cell (inst->object ().cell_index ());
    db::box_convert <db::CellInst, true> inst_bc (*mp_layout, m_intruder_layer);

    for (db::CellInstArray::iterator n = inst->begin_touching (ref->box ().enlarged (db::Vector (-1, -1)), inst_bc); !n.at_end (); ++n) {

      db::ICplxTrans tn = inst->complex_trans (*n);

      db::Box region = ref->box ().transformed (tn.inverted ()) & intruder_cell.bbox (m_intruder_layer);
      if (! region.empty ()) {

        //  @@@ TODO: should be lighter, cache, handle arrays ..
        db::RecursiveShapeIterator si (*mp_layout, intruder_cell, m_intruder_layer, region);
        si.shape_flags (polygon_ref_flags ());
        while (! si.at_end ()) {

          //  @@@ should be easier to transform references
          const db::PolygonRef *ref2 = si.shape ().basic_ptr (db::PolygonRef::tag ());
          db::Polygon poly = ref2->obj ().transformed (tn * si.trans () * db::ICplxTrans (ref2->trans ()));
          (*mp_result)[*ref].push_back (db::PolygonRef (poly, mp_layout->shape_repository()));

          ++si;

        }

      }

    }
  }

private:
  db::Layout *mp_layout;
  unsigned int m_intruder_layer;
  std::map<db::PolygonRef, std::vector<db::PolygonRef> > *mp_result;
};

static bool
instances_interact (const db::Layout *layout1, const db::CellInstArray *inst1, unsigned int layer1, const db::Layout *layout2, const db::CellInstArray *inst2, unsigned int layer2)
{
  //  TODO: this algorithm is not in particular effective for identical arrays

  const db::Cell &cell1 = layout1->cell (inst1->object ().cell_index ());
  const db::Cell &cell2 = layout2->cell (inst2->object ().cell_index ());
  db::box_convert <db::CellInst, true> inst2_bc (*layout2, layer2);

  std::set<db::ICplxTrans> relative_trans_seen;

  for (db::CellInstArray::iterator n = inst1->begin (); ! n.at_end (); ++n) {

    db::ICplxTrans tn1 = inst1->complex_trans (*n);
    db::ICplxTrans tni1 = tn1.inverted ();
    db::Box ibox1 = tn1 * cell1.bbox (layer1);

    if (! ibox1.empty ()) {

      //  @@@ TODO: in some cases, it may be possible to optimize this for arrays

      for (db::CellInstArray::iterator k = inst2->begin_touching (ibox1.enlarged (db::Vector (-1, -1)), inst2_bc); ! k.at_end (); ++k) {

        if (inst1 == inst2 && *n == *k) {
          //  skip self-interactions - this is handled inside the cell
          continue;
        }

        db::ICplxTrans tn2 = inst2->complex_trans (*k);
        db::Box ibox2 = tn2 * cell2.bbox (layer2);

        db::ICplxTrans tn21 = tni1 * tn2;
        if (! relative_trans_seen.insert (tn21).second) {
          //  this relative transformation was already seen
          continue;
        }

        db::Box cbox = ibox1 & ibox2;
        if (! cbox.empty ()) {

          db::ICplxTrans tni2 = tn2.inverted ();

          //  not very strong, but already useful: the cells interact if there is a layer1 in cell1
          //  in the common box and a layer2 in the cell2 in the common box
          if (! db::RecursiveShapeIterator (*layout1, cell1, layer1, tni1 * cbox, true).at_end () &&
              ! db::RecursiveShapeIterator (*layout2, cell2, layer2, tni2 * cbox, true).at_end ()) {
            return true;
          }

        }

      }

    }

  }

  return false;
}

struct InteractionRegistrationInst2Inst
  : db::box_scanner_receiver2<db::CellInstArray, int, db::CellInstArray, int>
{
public:
  InteractionRegistrationInst2Inst (const db::Layout *subject_layout, unsigned int subject_layer, const db::Layout *intruder_layout, unsigned int intruder_layer, std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *result)
    : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), m_subject_layer (subject_layer), m_intruder_layer (intruder_layer), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst1, int, const db::CellInstArray *inst2, int)
  {
    // @@@ TODO: always insert, if both instances come from different layouts
    //  NOTE: self-interactions are possible for arrays: different elements of the
    //  array may interact which is a cell-external interaction.
    if ((*inst1 != *inst2 || inst1->size () > 1)
        && instances_interact (mp_subject_layout, inst1, m_subject_layer, mp_intruder_layout, inst2, m_intruder_layer)) {
      (*mp_result) [inst1].first.insert (inst2);
    }
  }

private:
  const db::Layout *mp_subject_layout, *mp_intruder_layout;
  unsigned int m_subject_layer, m_intruder_layer;
  std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *mp_result;
};

static bool
instance_shape_interacts (const db::Layout *layout, const db::CellInstArray *inst, unsigned int layer, const db::PolygonRef &ref)
{
  const db::Cell &cell = layout->cell (inst->object ().cell_index ());
  db::box_convert <db::CellInst, true> inst_bc (*layout, layer);
  db::Box rbox = ref.box ();

  for (db::CellInstArray::iterator n = inst->begin_touching (rbox.enlarged (db::Vector (-1, -1)), inst_bc); ! n.at_end (); ++n) {

    db::ICplxTrans tn = inst->complex_trans (*n);
    db::Box cbox = (tn * cell.bbox (layer)) & rbox;

    if (! cbox.empty ()) {

      db::ICplxTrans tni = tn.inverted ();

      //  not very strong, but already useful: the cells interact if there is a layer in cell
      //  in the common box
      if (! db::RecursiveShapeIterator (*layout, cell, layer, tni * cbox, true).at_end ()) {
        return true;
      }

    }

  }

  return false;
}

struct InteractionRegistrationInst2Shape
  : db::box_scanner_receiver2<db::CellInstArray, int, db::PolygonRef, int>
{
public:
  InteractionRegistrationInst2Shape (const db::Layout *subject_layout, unsigned int subject_layer, std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *result)
    : mp_subject_layout (subject_layout), m_subject_layer (subject_layer), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst, int, const db::PolygonRef *ref, int)
  {
    if (instance_shape_interacts (mp_subject_layout, inst, m_subject_layer, *ref)) {
      (*mp_result) [inst].second.insert (*ref);
    }
  }

private:
  const db::Layout *mp_subject_layout;
  unsigned int m_subject_layer;
  std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *mp_result;
};

}

// ---------------------------------------------------------------------------------------------
//  LocalProcessor implementation

LocalProcessor::LocalProcessor (db::Layout *layout, db::Cell *top, LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer)
  : mp_layout (layout), mp_top (top), m_subject_layer (subject_layer), m_intruder_layer (intruder_layer), m_output_layer (output_layer), mp_op (op)
{
  set_description (op->description ());
}

void LocalProcessor::run ()
{
  compute_contexts ();
  compute_results ();
}

void LocalProcessor::push_results (db::Cell *cell, const std::set<db::PolygonRef> &result)
{
  if (! result.empty ()) {
    cell->shapes (m_output_layer).insert (result.begin (), result.end ());
  }
}

void LocalProcessor::compute_contexts ()
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Computing contexts for ")) + description ());

  m_contexts_per_cell.clear ();

  std::pair<std::set<db::CellInstArray>, std::set<db::PolygonRef> > intruders;
  compute_contexts (0, 0, mp_top, db::ICplxTrans (), intruders);
}

void LocalProcessor::compute_contexts (db::LocalProcessorCellContext *parent_context, db::Cell *parent, db::Cell *cell, const db::ICplxTrans &cell_inst, const std::pair<std::set<CellInstArray>, std::set<PolygonRef> > &intruders)
{
  if (tl::verbosity () >= 30) {
    if (! parent) {
      tl::log << tr ("Computing context for top cell ") << mp_layout->cell_name (cell->cell_index ());
    } else {
      tl::log << tr ("Computing context for ") << mp_layout->cell_name (parent->cell_index ()) << " -> " << mp_layout->cell_name (cell->cell_index ()) << " @" << cell_inst.to_string ();
    }
  }

  db::LocalProcessorCellContexts &contexts = m_contexts_per_cell [cell];

  db::LocalProcessorCellContext *context = contexts.find_context (intruders);
  if (context) {
    context->add (parent_context, parent, cell_inst);
    return;
  }

  context = contexts.create (intruders);
  context->add (parent_context, parent, cell_inst);

  const db::Shapes &shapes_intruders = cell->shapes (m_intruder_layer);

  db::box_convert <db::CellInstArray, true> inst_bcs (*mp_layout, m_subject_layer);
  db::box_convert <db::CellInstArray, true> inst_bci (*mp_layout, m_intruder_layer);
  db::box_convert <db::CellInst, true> inst_bcii (*mp_layout, m_intruder_layer);

  if (! cell->begin ().at_end ()) {

    typedef std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > interaction_value_type;

    std::map<const db::CellInstArray *, interaction_value_type> interactions;

    //  insert dummy interactions to handle at least the child cell vs. itself
    for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
      interactions.insert (std::make_pair (&i->cell_inst (), interaction_value_type ()));
    }

    {
      db::box_scanner2<db::CellInstArray, int, db::CellInstArray, int> scanner;
      InteractionRegistrationInst2Inst rec (mp_layout, m_subject_layer, mp_layout, m_intruder_layer, &interactions);

      for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
        if (! inst_bcs (i->cell_inst ()).empty ()) {
          scanner.insert1 (&i->cell_inst (), 0);
        }
        if (! inst_bci (i->cell_inst ()).empty ()) {
          scanner.insert2 (&i->cell_inst (), 0);
        }
      }

      for (std::set<db::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
        if (! inst_bci (*i).empty ()) {
          scanner.insert2 (i.operator-> (), 0);
        }
      }

      scanner.process (rec, 0, inst_bcs, inst_bci);
    }

    {
      db::box_scanner2<db::CellInstArray, int, db::PolygonRef, int> scanner;
      InteractionRegistrationInst2Shape rec (mp_layout, m_subject_layer, &interactions);

      for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
        if (! inst_bcs (i->cell_inst ()).empty ()) {
          scanner.insert1 (&i->cell_inst (), 0);
        }
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

      db::Cell &child_cell = mp_layout->cell (i->first->object ().cell_index ());

      for (db::CellInstArray::iterator n = i->first->begin (); ! n.at_end (); ++n) {

        db::ICplxTrans tn = i->first->complex_trans (*n);
        db::ICplxTrans tni = tn.inverted ();
        db::Box nbox = tn * child_cell.bbox (m_subject_layer);

        if (! nbox.empty ()) {

          std::pair<std::set<db::CellInstArray>, std::set<db::PolygonRef> > intruders_below;

          //  @@@ transformation of polygon refs - can this be done more efficiently?
          for (std::set<db::PolygonRef>::const_iterator p = i->second.second.begin (); p != i->second.second.end (); ++p) {
            if (nbox.overlaps (p->box ())) {
              db::Polygon poly = p->obj ().transformed (tni * db::ICplxTrans (p->trans ()));
              intruders_below.second.insert (db::PolygonRef (poly, mp_layout->shape_repository ()));
            }
          }

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
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Computing results for ")) + description ());

  //  avoids updates while we work on the layout
  mp_layout->update ();
  db::LayoutLocker locker (mp_layout);

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
  const db::Shapes &shapes_subject = cell->shapes (m_subject_layer);
  const db::Shapes &shapes_intruders = cell->shapes (m_intruder_layer);

  //  local shapes vs. child cell

  std::map<db::PolygonRef, std::vector<db::PolygonRef> > interactions;
  db::box_convert <db::CellInstArray, true> inst_bci (*mp_layout, m_intruder_layer);

  if (mp_op->on_empty_intruder_hint () != LocalOperation::Drop) {
    //  insert dummy interactions to accommodate subject vs. nothing
    for (db::Shapes::shape_iterator i = shapes_subject.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
      interactions.insert (std::make_pair (*i->basic_ptr (db::PolygonRef::tag ()), std::vector<db::PolygonRef> ()));
    }
  }

  if (! shapes_subject.empty () && ! (shapes_intruders.empty () && intruders.second.empty ())) {

    db::box_scanner2<db::PolygonRef, int, db::PolygonRef, int> scanner;
    InteractionRegistrationShape2Shape rec (&interactions);

    for (db::Shapes::shape_iterator i = shapes_subject.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
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

  if (! shapes_subject.empty () && ! (cell->begin ().at_end () && intruders.first.empty ())) {

    db::box_scanner2<db::PolygonRef, int, db::CellInstArray, int> scanner;
    InteractionRegistrationShape2Inst rec (mp_layout, m_intruder_layer, &interactions);

    for (db::Shapes::shape_iterator i = shapes_subject.begin (polygon_ref_flags ()); !i.at_end (); ++i) {
      scanner.insert1 (i->basic_ptr (db::PolygonRef::tag ()), 0);
    }

    for (db::Cell::const_iterator i = cell->begin (); !i.at_end (); ++i) {
      if (! inst_bci (i->cell_inst ()).empty ()) {
        scanner.insert2 (&i->cell_inst (), 0);
      }
    }
    for (std::set<db::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
      if (! inst_bci (*i).empty ()) {
        scanner.insert2 (i.operator-> (), 0);
      }
    }

    scanner.process (rec, 0, db::box_convert<db::PolygonRef> (), inst_bci);

  }

  mp_op->compute_local (mp_layout, interactions, result);
}

}

