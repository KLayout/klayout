
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
//  Shape reference translator

template <class Ref>
class shape_reference_translator
{
public:
  typedef typename Ref::shape_type shape_type;

  shape_reference_translator (db::Layout *target_layout)
    : mp_layout (target_layout)
  {
    //  .. nothing yet ..
  }

  Ref operator() (const Ref &ref) const
  {
    shape_type sh = ref.obj ().transformed (ref.trans ());
    return Ref (sh, mp_layout->shape_repository ());
  }

  template <class Trans>
  Ref operator() (const Ref &ref, const Trans &tr) const
  {
    shape_type sh = ref.obj ().transformed (tr * Trans (ref.trans ()));
    return Ref (sh, mp_layout->shape_repository ());
  }

private:
  db::Layout *mp_layout;
};

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

    db::Layout *subject_layout = d->parent->layout ();
    shape_reference_translator<db::PolygonRef> rt (subject_layout);
    for (std::set<db::PolygonRef>::const_iterator r = res.begin (); r != res.end (); ++r) {
      d->parent_context->propagated ().insert (rt (*r, d->cell_inst));
    }

  }

}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorCellContexts implementation

LocalProcessorCellContexts::LocalProcessorCellContexts ()
  : mp_intruder_cell (0)
{
  //  .. nothing yet ..
}

LocalProcessorCellContexts::LocalProcessorCellContexts (const db::Cell *intruder_cell)
  : mp_intruder_cell (intruder_cell)
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
LocalProcessorCellContexts::compute_results (LocalProcessorContexts &contexts, db::Cell *cell, const LocalOperation *op, unsigned int output_layer, LocalProcessor *proc)
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
      proc->compute_local_cell (contexts, cell, mp_intruder_cell, op, c->first, common);
      first = false;

    } else {

      std::set<db::PolygonRef> res = c->second.propagated ();
      proc->compute_local_cell (contexts, cell, mp_intruder_cell, op, c->first, res);

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

  proc->push_results (cell, output_layer, common);
}

// ---------------------------------------------------------------------------------------------

ShapeInteractions::ShapeInteractions ()
  : m_id (0)
{
  //  .. nothing yet ..
}

bool
ShapeInteractions::has_shape_id (unsigned int id) const
{
  return m_shapes.find (id) != m_shapes.end ();
}

void
ShapeInteractions::add_shape (unsigned int id, const db::PolygonRef &shape)
{
  m_shapes [id] = shape;
}

void
ShapeInteractions::add_subject (unsigned int id, const db::PolygonRef &shape)
{
  add_shape (id, shape);
  m_interactions.insert (std::make_pair (id, container::value_type::second_type ()));
}

void
ShapeInteractions::add_interaction (unsigned int subject_id, unsigned int intruder_id)
{
  m_interactions [subject_id].push_back (intruder_id);
}

const std::vector<unsigned int> &
ShapeInteractions::intruders_for (unsigned int subject_id) const
{
  iterator i = m_interactions.find (subject_id);
  if (i == m_interactions.end ()) {
    static std::vector<unsigned int> empty;
    return empty;
  } else {
    return i->second;
  }
}

const db::PolygonRef &
ShapeInteractions::shape (unsigned int id) const
{
  std::map<unsigned int, db::PolygonRef>::const_iterator i = m_shapes.find (id);
  if (i == m_shapes.end ()) {
    static db::PolygonRef s;
    return s;
  } else {
    return i->second;
  }
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
  : db::box_scanner_receiver2<db::PolygonRef, unsigned int, db::PolygonRef, unsigned int>
{
public:
  InteractionRegistrationShape2Shape (db::Layout *layout, ShapeInteractions *result)
    : mp_result (result), mp_layout (layout)
  {
    //  nothing yet ..
  }

  void add (const db::PolygonRef *ref1, unsigned int id1, const db::PolygonRef *ref2, unsigned int id2)
  {
    mp_result->add_shape (id1, *ref1);

    if (mp_layout) {
      //  In order to guarantee the refs come from the subject layout, we'd need to
      //  rewrite them to the subject layout if required.
      if (!mp_result->has_shape_id (id2)) {
        db::shape_reference_translator<db::PolygonRef> rt (mp_layout);
        mp_result->add_shape (id2, rt (*ref2));
      }
    } else {
      mp_result->add_shape (id2, *ref2);
    }

    mp_result->add_interaction (id1, id2);
  }

private:
  ShapeInteractions *mp_result;
  db::Layout *mp_layout;
};

struct InteractionRegistrationShape1
  : db::box_scanner_receiver<db::PolygonRef, unsigned int>
{
public:
  InteractionRegistrationShape1 (ShapeInteractions *result)
    : mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::PolygonRef *ref1, unsigned int id1, const db::PolygonRef *ref2, unsigned int id2)
  {
    mp_result->add_shape (id1, *ref1);
    mp_result->add_shape (id2, *ref2);
    mp_result->add_interaction (id1, id2);
  }

private:
  ShapeInteractions *mp_result;
};

struct InteractionRegistrationShape2Inst
  : db::box_scanner_receiver2<db::PolygonRef, unsigned int, db::CellInstArray, unsigned int>
{
public:
  InteractionRegistrationShape2Inst (db::Layout *subject_layout, const db::Layout *intruder_layout, unsigned int intruder_layer, db::Coord dist, ShapeInteractions *result)
    : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), m_intruder_layer (intruder_layer), m_dist (dist), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::PolygonRef *ref, unsigned int id1, const db::CellInstArray *inst, unsigned int inst_id)
  {
    const db::Cell &intruder_cell = mp_intruder_layout->cell (inst->object ().cell_index ());
    db::box_convert <db::CellInst, true> inst_bc (*mp_intruder_layout, m_intruder_layer);
    mp_result->add_shape (id1, *ref);

    //  Find all instance array members that potentially interact with the shape and use
    //  add_shapes_from_intruder_inst on them
    for (db::CellInstArray::iterator n = inst->begin_touching (ref->box ().enlarged (db::Vector (m_dist - 1, m_dist - 1)), inst_bc); !n.at_end (); ++n) {
      db::ICplxTrans tn = inst->complex_trans (*n);
      db::Box region = ref->box ().transformed (tn.inverted ()).enlarged (db::Vector (m_dist, m_dist)) & intruder_cell.bbox (m_intruder_layer).enlarged (db::Vector (m_dist, m_dist));
      if (! region.empty ()) {
        add_shapes_from_intruder_inst (id1, intruder_cell, tn, inst_id, region);
      }
    }
  }

private:
  db::Layout *mp_subject_layout;
  const db::Layout *mp_intruder_layout;
  unsigned int m_intruder_layer;
  db::Coord m_dist;
  ShapeInteractions *mp_result;
  std::map<std::pair<unsigned int, const db::PolygonRef *>, unsigned int> m_inst_shape_ids;

  void add_shapes_from_intruder_inst (unsigned int id1, const db::Cell &intruder_cell, const db::ICplxTrans &tn, unsigned int inst_id, const db::Box &region)
  {
    db::shape_reference_translator<db::PolygonRef> rt (mp_subject_layout);

    //  Look up all shapes from the intruder instance which interact with the subject shape
    //  (given through region)
    //  @@@ TODO: should be lighter, cache, handle arrays ..
    db::RecursiveShapeIterator si (*mp_intruder_layout, intruder_cell, m_intruder_layer, region);
    si.shape_flags (polygon_ref_flags ());
    while (! si.at_end ()) {

      const db::PolygonRef *ref2 = si.shape ().basic_ptr (db::PolygonRef::tag ());

      //  reuse the same id for shapes from the same instance -> this avoid duplicates with different IDs on
      //  the intruder side.
      std::map<std::pair<unsigned int, const db::PolygonRef *>, unsigned int>::const_iterator k = m_inst_shape_ids.find (std::make_pair (inst_id, ref2));
      if (k == m_inst_shape_ids.end ()) {

        k = m_inst_shape_ids.insert (std::make_pair (std::make_pair (inst_id, ref2), mp_result->next_id ())).first;

        //  NOTE: we intentionally rewrite to the *subject* layout - this way polygon refs in the context come from the
        //  subject, not from the intruder.
        mp_result->add_shape (k->second, rt (*ref2, tn * si.trans ()));

      }

      mp_result->add_interaction (id1, k->second);

      ++si;

    }
  }
};

static bool
instances_interact (const db::Layout *layout1, const db::CellInstArray *inst1, unsigned int layer1, const db::Layout *layout2, const db::CellInstArray *inst2, unsigned int layer2, db::Coord dist)
{
  //  TODO: this algorithm is not in particular effective for identical arrays

  const db::Cell &cell1 = layout1->cell (inst1->object ().cell_index ());
  const db::Cell &cell2 = layout2->cell (inst2->object ().cell_index ());
  db::box_convert <db::CellInst, true> inst2_bc (*layout2, layer2);

  std::set<db::ICplxTrans> relative_trans_seen;

  for (db::CellInstArray::iterator n = inst1->begin (); ! n.at_end (); ++n) {

    db::ICplxTrans tn1 = inst1->complex_trans (*n);
    db::ICplxTrans tni1 = tn1.inverted ();
    db::Box ibox1 = tn1 * cell1.bbox (layer1).enlarged (db::Vector (dist, dist));

    if (! ibox1.empty ()) {

      //  @@@ TODO: in some cases, it may be possible to optimize this for arrays

      for (db::CellInstArray::iterator k = inst2->begin_touching (ibox1.enlarged (db::Vector (-1, -1)), inst2_bc); ! k.at_end (); ++k) {

        if (inst1 == inst2 && *n == *k) {
          //  skip self-interactions - this is handled inside the cell
          continue;
        }

        db::ICplxTrans tn2 = inst2->complex_trans (*k);

        //  NOTE: we need to enlarge both subject *and* intruder boxes - either ubject comes close to intruder or the other way around
        db::Box ibox2 = tn2 * cell2.bbox (layer2).enlarged (db::Vector (dist, dist));

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
  : db::box_scanner_receiver2<db::CellInstArray, unsigned int, db::CellInstArray, unsigned int>
{
public:
  typedef std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > interaction_value_type;

  InteractionRegistrationInst2Inst (const db::Layout *subject_layout, unsigned int subject_layer, const db::Layout *intruder_layout, unsigned int intruder_layer, db::Coord dist, std::map<const db::CellInstArray *, interaction_value_type> *result)
    : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), m_subject_layer (subject_layer), m_intruder_layer (intruder_layer), m_dist (dist), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst1, unsigned int id1, const db::CellInstArray *inst2, unsigned int id2)
  {
    //  NOTE: self-interactions are possible for arrays: different elements of the
    //  array may interact which is a cell-external interaction.
    if (mp_subject_layout != mp_intruder_layout || id1 != id2 || inst1->size () > 1) {

      bool ignore = false;
      if (mp_subject_layout == mp_intruder_layout && m_subject_layer == m_intruder_layer) {
        if (m_interactions.find (std::make_pair (id2, id1)) != m_interactions.end ()) {
          //  for self interactions ignore the reverse interactions
          ignore = true;
        } else {
          m_interactions.insert (std::make_pair (id1, id2));
        }
      }

      if (! ignore && instances_interact (mp_subject_layout, inst1, m_subject_layer, mp_intruder_layout, inst2, m_intruder_layer, m_dist)) {
        (*mp_result) [inst1].first.insert (inst2);
      }

    }
  }

private:
  const db::Layout *mp_subject_layout, *mp_intruder_layout;
  unsigned int m_subject_layer, m_intruder_layer;
  db::Coord m_dist;
  std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *mp_result;
  std::set<std::pair<unsigned int, unsigned int> > m_interactions;
};

static bool
instance_shape_interacts (const db::Layout *layout, const db::CellInstArray *inst, unsigned int layer, const db::PolygonRef &ref, db::Coord dist)
{
  const db::Cell &cell = layout->cell (inst->object ().cell_index ());
  db::box_convert <db::CellInst, true> inst_bc (*layout, layer);
  db::Box rbox = ref.box ();

  for (db::CellInstArray::iterator n = inst->begin_touching (rbox.enlarged (db::Vector (dist - 1, dist - 1)), inst_bc); ! n.at_end (); ++n) {

    db::ICplxTrans tn = inst->complex_trans (*n);
    db::Box cbox = (tn * cell.bbox (layer)).enlarged (db::Vector (dist, dist)) & rbox.enlarged (db::Vector (dist, dist));

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
  : db::box_scanner_receiver2<db::CellInstArray, unsigned int, db::PolygonRef, unsigned int>
{
public:
  InteractionRegistrationInst2Shape (const db::Layout *subject_layout, unsigned int subject_layer, db::Coord dist, std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *result)
    : mp_subject_layout (subject_layout), m_subject_layer (subject_layer), m_dist (dist), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const db::CellInstArray *inst, unsigned int, const db::PolygonRef *ref, unsigned int)
  {
    if (instance_shape_interacts (mp_subject_layout, inst, m_subject_layer, *ref, m_dist)) {
      (*mp_result) [inst].second.insert (*ref);
    }
  }

private:
  const db::Layout *mp_subject_layout;
  unsigned int m_subject_layer;
  db::Coord m_dist;
  std::map<const db::CellInstArray *, std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > > *mp_result;
};

}

// ---------------------------------------------------------------------------------------------
//  LocalProcessor implementation

LocalProcessor::LocalProcessor (db::Layout *layout, db::Cell *top)
  : mp_subject_layout (layout), mp_intruder_layout (layout), mp_subject_top (top), mp_intruder_top (top)
{
  //  .. nothing yet ..
}

LocalProcessor::LocalProcessor (db::Layout *subject_layout, db::Cell *subject_top, const db::Layout *intruder_layout, const db::Cell *intruder_top)
  : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), mp_subject_top (subject_top), mp_intruder_top (intruder_top)
{
  //  .. nothing yet ..
}

void LocalProcessor::run (LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer)
{
  LocalProcessorContexts contexts;
  compute_contexts (contexts, op, subject_layer, intruder_layer);
  compute_results (contexts, op, output_layer);
}

void LocalProcessor::push_results (db::Cell *cell, unsigned int output_layer, const std::set<db::PolygonRef> &result) const
{
  if (! result.empty ()) {
    cell->shapes (output_layer).insert (result.begin (), result.end ());
  }
}

void LocalProcessor::compute_contexts (LocalProcessorContexts &contexts, const LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Computing contexts for ")) + description ());

  contexts.clear ();
  contexts.set_intruder_layer (intruder_layer);
  contexts.set_subject_layer (subject_layer);
  contexts.set_description (op->description ());

  std::pair<std::set<db::CellInstArray>, std::set<db::PolygonRef> > intruders;
  compute_contexts (contexts, 0, 0, mp_subject_top, db::ICplxTrans (), mp_intruder_top, intruders, op->dist ());
}

void LocalProcessor::compute_contexts (LocalProcessorContexts &contexts,
                                       db::LocalProcessorCellContext *parent_context,
                                       db::Cell *subject_parent,
                                       db::Cell *subject_cell,
                                       const db::ICplxTrans &subject_cell_inst,
                                       const db::Cell *intruder_cell,
                                       const std::pair<std::set<CellInstArray>, std::set<PolygonRef> > &intruders,
                                       db::Coord dist)
{
  if (tl::verbosity () >= 30) {
    if (! subject_parent) {
      tl::log << tr ("Computing context for top cell ") << mp_subject_layout->cell_name (subject_cell->cell_index ());
    } else {
      tl::log << tr ("Computing context for ") << mp_subject_layout->cell_name (subject_parent->cell_index ()) << " -> " << mp_subject_layout->cell_name (subject_cell->cell_index ()) << " @" << subject_cell_inst.to_string ();
    }
  }

  db::LocalProcessorCellContexts &cell_contexts = contexts.contexts_per_cell (subject_cell, intruder_cell);

  db::LocalProcessorCellContext *context = cell_contexts.find_context (intruders);
  if (context) {
    context->add (parent_context, subject_parent, subject_cell_inst);
    return;
  }

  context = cell_contexts.create (intruders);
  context->add (parent_context, subject_parent, subject_cell_inst);

  const db::Shapes *intruder_shapes = 0;
  if (intruder_cell) {
    intruder_shapes = &intruder_cell->shapes (contexts.intruder_layer ());
  }

  db::box_convert <db::CellInstArray, true> inst_bcs (*mp_subject_layout, contexts.subject_layer ());
  db::box_convert <db::CellInstArray, true> inst_bci (*mp_intruder_layout, contexts.intruder_layer ());
  db::box_convert <db::CellInst, true> inst_bcii (*mp_intruder_layout, contexts.intruder_layer ());

  //  handle top-down interactions (subject instances interacting with intruder shapes)
  //  and sibling interactions

  if (! subject_cell->begin ().at_end ()) {

    typedef std::pair<std::set<const db::CellInstArray *>, std::set<db::PolygonRef> > interaction_value_type;

    std::map<const db::CellInstArray *, interaction_value_type> interactions;

    //  insert dummy interactions to handle at least the child cell vs. itself
    //  - this is important so we will always handle the instances unless they are
    //  entirely empty in the subject layer
    for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
      if (! inst_bcs (i->cell_inst ()).empty ()) {
        interactions.insert (std::make_pair (&i->cell_inst (), interaction_value_type ()));
      }
    }

    {
      db::box_scanner2<db::CellInstArray, int, db::CellInstArray, int> scanner;
      InteractionRegistrationInst2Inst rec (mp_subject_layout, contexts.subject_layer (), mp_intruder_layout, contexts.intruder_layer (), dist, &interactions);

      unsigned int id = 0;

      if (subject_cell == intruder_cell) {

        //  Use the same id's for same instances - this way we can easily detect same instances
        //  and don't make the self-interacting

        for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
          unsigned int iid = ++id;
          if (! inst_bcs (i->cell_inst ()).empty ()) {
            scanner.insert1 (&i->cell_inst (), iid);
          }
          if (! inst_bci (i->cell_inst ()).empty ()) {
            scanner.insert2 (&i->cell_inst (), iid);
          }
        }

      } else {

        for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
          if (! inst_bcs (i->cell_inst ()).empty ()) {
            scanner.insert1 (&i->cell_inst (), ++id);
          }
        }

        if (intruder_cell) {
          for (db::Cell::const_iterator i = intruder_cell->begin (); !i.at_end (); ++i) {
            if (! inst_bci (i->cell_inst ()).empty ()) {
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

    {
      db::box_scanner2<db::CellInstArray, int, db::PolygonRef, int> scanner;
      InteractionRegistrationInst2Shape rec (mp_subject_layout, contexts.subject_layer (), dist, &interactions);

      for (db::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
        if (! inst_bcs (i->cell_inst ()).empty ()) {
          scanner.insert1 (&i->cell_inst (), 0);
        }
      }

      for (std::set<db::PolygonRef>::const_iterator i = intruders.second.begin (); i != intruders.second.end (); ++i) {
        scanner.insert2 (i.operator-> (), 0);
      }

      if (intruder_shapes) {
        for (db::Shapes::shape_iterator i = intruder_shapes->begin (polygon_ref_flags ()); !i.at_end (); ++i) {
          scanner.insert2 (i->basic_ptr (db::PolygonRef::tag ()), 0);
        }
      }

      scanner.process (rec, dist, inst_bcs, db::box_convert<db::PolygonRef> ());
    }

    for (std::map<const db::CellInstArray *, interaction_value_type>::const_iterator i = interactions.begin (); i != interactions.end (); ++i) {

      db::Cell &subject_child_cell = mp_subject_layout->cell (i->first->object ().cell_index ());
      db::shape_reference_translator<db::PolygonRef> rt (mp_subject_layout);

      for (db::CellInstArray::iterator n = i->first->begin (); ! n.at_end (); ++n) {

        db::ICplxTrans tn = i->first->complex_trans (*n);
        db::ICplxTrans tni = tn.inverted ();
        db::Box nbox = tn * subject_child_cell.bbox (contexts.subject_layer ()).enlarged (db::Vector (dist, dist));

        if (! nbox.empty ()) {

          std::pair<std::set<db::CellInstArray>, std::set<db::PolygonRef> > intruders_below;

          //  @@@ transformation of polygon refs - can this be done more efficiently?
          for (std::set<db::PolygonRef>::const_iterator p = i->second.second.begin (); p != i->second.second.end (); ++p) {
            if (nbox.overlaps (p->box ())) {
              intruders_below.second.insert (rt (*p, tni));
            }
          }

          //  @@@ TODO: in some cases, it may be possible to optimize this for arrays

          for (std::set<const db::CellInstArray *>::const_iterator j = i->second.first.begin (); j != i->second.first.end (); ++j) {
            for (db::CellInstArray::iterator k = (*j)->begin_touching (nbox.enlarged (db::Vector (-1, -1)), inst_bcii); ! k.at_end (); ++k) {
              db::ICplxTrans tk = (*j)->complex_trans (*k);
              //  NOTE: no self-interactions
              if (i->first != *j || tn != tk) {
                intruders_below.first.insert (db::CellInstArray (db::CellInst ((*j)->object ().cell_index ()), tni * tk));
              }
            }
          }

          db::Cell *intruder_child_cell = (subject_cell == intruder_cell ? &subject_child_cell : 0);
          compute_contexts (contexts, context, subject_cell, &subject_child_cell, tn, intruder_child_cell, intruders_below, dist);

        }

      }

    }

  }
}

void
LocalProcessor::compute_results (LocalProcessorContexts &contexts, const LocalOperation *op, unsigned int output_layer)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Computing results for ")) + description ());

  //  avoids updates while we work on the layout
  mp_subject_layout->update ();
  db::LayoutLocker locker (mp_subject_layout);

  for (db::Layout::bottom_up_const_iterator bu = mp_subject_layout->begin_bottom_up (); bu != mp_subject_layout->end_bottom_up (); ++bu) {

    LocalProcessorContexts::iterator cpc = contexts.context_map ().find (&mp_subject_layout->cell (*bu));
    if (cpc != contexts.context_map ().end ()) {
      cpc->second.compute_results (contexts, cpc->first, op, output_layer, this);
      contexts.context_map ().erase (cpc);
    }

  }
}

void
LocalProcessor::compute_local_cell (LocalProcessorContexts &contexts, db::Cell *subject_cell, const db::Cell *intruder_cell, const db::LocalOperation *op, const std::pair<std::set<CellInstArray>, std::set<db::PolygonRef> > &intruders, std::set<db::PolygonRef> &result)
{
  const db::Shapes *subject_shapes = &subject_cell->shapes (contexts.subject_layer ());

  const db::Shapes *intruder_shapes = 0;
  if (intruder_cell) {
    intruder_shapes = &intruder_cell->shapes (contexts.intruder_layer ());
    if (intruder_shapes->empty ()) {
      intruder_shapes = 0;
    }
  }

  //  local shapes vs. child cell

  ShapeInteractions interactions;
  db::box_convert <db::CellInstArray, true> inst_bci (*mp_intruder_layout, contexts.intruder_layer ());

  //  insert dummy interactions to accommodate subject vs. nothing and assign an ID
  //  range for the subject shapes.
  unsigned int subject_id0 = 0;
  for (db::Shapes::shape_iterator i = subject_shapes->begin (polygon_ref_flags ()); !i.at_end (); ++i) {

    unsigned int id = interactions.next_id ();
    if (subject_id0 == 0) {
      subject_id0 = id;
    }

    if (op->on_empty_intruder_hint () != LocalOperation::Drop) {
      const db::PolygonRef *ref = i->basic_ptr (db::PolygonRef::tag ());
      interactions.add_subject (id, *ref);
    }

  }

  if (! subject_shapes->empty () && (intruder_shapes || ! intruders.second.empty ())) {

    if (subject_cell == intruder_cell && contexts.subject_layer () == contexts.intruder_layer ()) {

      db::box_scanner<db::PolygonRef, int> scanner;
      InteractionRegistrationShape1 rec (&interactions);

      unsigned int id = subject_id0;
      for (db::Shapes::shape_iterator i = subject_shapes->begin (polygon_ref_flags ()); !i.at_end (); ++i) {
        const db::PolygonRef *ref = i->basic_ptr (db::PolygonRef::tag ());
        scanner.insert (ref, id++);
      }

      for (std::set<db::PolygonRef>::const_iterator i = intruders.second.begin (); i != intruders.second.end (); ++i) {
        scanner.insert (i.operator-> (), interactions.next_id ());
      }

      scanner.process (rec, op->dist (), db::box_convert<db::PolygonRef> ());

    } else {

      db::box_scanner2<db::PolygonRef, int, db::PolygonRef, int> scanner;
      InteractionRegistrationShape2Shape rec (mp_subject_layout == mp_intruder_layout ? 0 : mp_subject_layout, &interactions);

      unsigned int id = subject_id0;
      for (db::Shapes::shape_iterator i = subject_shapes->begin (polygon_ref_flags ()); !i.at_end (); ++i) {
        const db::PolygonRef *ref = i->basic_ptr (db::PolygonRef::tag ());
        scanner.insert1 (ref, id++);
      }

      for (std::set<db::PolygonRef>::const_iterator i = intruders.second.begin (); i != intruders.second.end (); ++i) {
        scanner.insert2 (i.operator-> (), interactions.next_id ());
      }

      if (intruder_shapes) {
        for (db::Shapes::shape_iterator i = intruder_shapes->begin (polygon_ref_flags ()); !i.at_end (); ++i) {
          scanner.insert2 (i->basic_ptr (db::PolygonRef::tag ()), interactions.next_id ());
        }
      }

      scanner.process (rec, op->dist (), db::box_convert<db::PolygonRef> (), db::box_convert<db::PolygonRef> ());

    }

  }

  if (! subject_shapes->empty () && ! ((! intruder_cell || intruder_cell->begin ().at_end ()) && intruders.first.empty ())) {

    db::box_scanner2<db::PolygonRef, int, db::CellInstArray, int> scanner;
    InteractionRegistrationShape2Inst rec (mp_subject_layout, mp_intruder_layout, contexts.intruder_layer (), op->dist (), &interactions);

    unsigned int id = subject_id0;
    for (db::Shapes::shape_iterator i = subject_shapes->begin (polygon_ref_flags ()); !i.at_end (); ++i) {
      scanner.insert1 (i->basic_ptr (db::PolygonRef::tag ()), id++);
    }

    unsigned int inst_id = 0;

    if (subject_cell == intruder_cell && contexts.subject_layer () == contexts.intruder_layer ()) {

      //  Same cell, same layer -> no shape to child instance interactions because this will be taken care of
      //  by the instances themselves (and their intruders). This also means, we prefer to deal with
      //  interactions low in the hierarchy.

    } else if (intruder_cell) {
      for (db::Cell::const_iterator i = intruder_cell->begin (); !i.at_end (); ++i) {
        if (! inst_bci (i->cell_inst ()).empty ()) {
          scanner.insert2 (&i->cell_inst (), ++inst_id);
        }
      }
    }

    for (std::set<db::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
      if (! inst_bci (*i).empty ()) {
        scanner.insert2 (i.operator-> (), ++inst_id);
      }
    }

    scanner.process (rec, op->dist (), db::box_convert<db::PolygonRef> (), inst_bci);

  }

  op->compute_local (mp_subject_layout, interactions, result);
}

}

