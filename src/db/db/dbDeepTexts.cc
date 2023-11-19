
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


#include "dbDeepTexts.h"
#include "dbCellGraphUtils.h"
#include "dbDeepEdges.h"
#include "dbDeepRegion.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"
#include "dbLocalOperation.h"
#include "dbTextsUtils.h"
#include "dbHierProcessor.h"
#include "dbRegion.h"

#include <sstream>
#include <unordered_set>

namespace db
{

/**
 *  @brief An iterator delegate for the deep region
 *  TODO: this is kind of redundant with OriginalLayerIterator ..
 */
class DB_PUBLIC DeepTextsIterator
  : public TextsIteratorDelegate
{
public:
  typedef db::Text value_type;

  DeepTextsIterator (const db::RecursiveShapeIterator &iter)
    : m_iter (iter), m_prop_id (0)
  {
    set ();
  }

  virtual ~DeepTextsIterator () { }

  virtual bool at_end () const
  {
    return m_iter.at_end ();
  }

  virtual void increment ()
  {
    ++m_iter;
    set ();
  }

  virtual bool is_addressable() const
  {
    return false;
  }

  virtual const value_type *get () const
  {
    return &m_text;
  }

  virtual db::properties_id_type prop_id () const
  {
    return m_prop_id;
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
  {
    const DeepTextsIterator *o = dynamic_cast<const DeepTextsIterator *> (other);
    return o && o->m_iter == m_iter;
  }

  virtual TextsIteratorDelegate *clone () const
  {
    return new DeepTextsIterator (*this);
  }

  virtual void do_reset (const db::Box &region, bool overlapping)
  {
    m_iter.set_region (region);
    m_iter.set_overlapping (overlapping);
    set ();
  }

  virtual db::Box bbox () const
  {
    return m_iter.bbox ();
  }

private:
  friend class Texts;

  db::RecursiveShapeIterator m_iter;
  mutable value_type m_text;
  mutable db::properties_id_type m_prop_id;

  void set () const
  {
    if (! m_iter.at_end ()) {
      m_iter->text (m_text);
      m_text.transform (m_iter.trans ());
      m_prop_id = m_iter->prop_id ();
    }
  }
};

DeepTexts::DeepTexts ()
  : MutableTexts ()
{
  //  .. nothing yet ..
}

DeepTexts::DeepTexts (const db::Texts &other, DeepShapeStore &dss)
  : MutableTexts ()
{
  set_deep_layer (dss.create_from_flat (other));
}

DeepTexts::DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss)
  : MutableTexts ()
{
  set_deep_layer (dss.create_text_layer (si));
}

DeepTexts::DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans)
  : MutableTexts ()
{
  set_deep_layer (dss.create_text_layer (si, trans));
}

DeepTexts::DeepTexts (const DeepTexts &other)
  : MutableTexts (other), DeepShapeCollectionDelegateBase (other)
{
  //  .. nothing yet ..
}

DeepTexts &
DeepTexts::operator= (const DeepTexts &other)
{
  if (this != &other) {
    AsIfFlatTexts::operator= (other);
    DeepShapeCollectionDelegateBase::operator= (other);
  }
  return *this;
}

DeepTexts::DeepTexts (const DeepLayer &dl)
  : MutableTexts ()
{
  set_deep_layer (dl);
}

DeepTexts::~DeepTexts ()
{
  //  .. nothing yet ..
}

TextsDelegate *DeepTexts::clone () const
{
  return new DeepTexts (*this);
}

void DeepTexts::do_insert (const db::Text &text)
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {
    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    top_cell.shapes (deep_layer ().layer ()).insert (db::TextRef (text, layout.shape_repository ()));
  }

  invalidate_bbox ();
  set_is_merged (false);
}

template <class Trans>
static void transform_deep_layer (db::DeepLayer &deep_layer, const Trans &t)
{
  //  TODO: this is a pretty cheap implementation. At least a plain move can be done with orientation variants.

  db::Layout &layout = deep_layer.layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    db::Shapes flat_shapes (layout.is_editable ());
    for (db::RecursiveShapeIterator iter (layout, top_cell, deep_layer.layer ()); !iter.at_end (); ++iter) {
      db::Text text;
      iter->text (text);
      flat_shapes.insert (text.transformed (iter.trans ()).transformed (t));
    }

    layout.clear_layer (deep_layer.layer ());
    top_cell.shapes (deep_layer.layer ()).swap (flat_shapes);

  }
}

void DeepTexts::do_transform (const db::Trans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::do_transform (const db::ICplxTrans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::do_transform (const db::IMatrix2d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::do_transform (const db::IMatrix3d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::reserve (size_t)
{
  //  Not implemented for deep regions
}

void DeepTexts::flatten ()
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    db::Shapes flat_shapes (layout.is_editable ());
    for (db::RecursiveShapeIterator iter (layout, top_cell, deep_layer ().layer ()); !iter.at_end (); ++iter) {
      db::Text text;
      iter->text (text);
      flat_shapes.insert (db::TextRef (text.transformed (iter.trans ()), layout.shape_repository ()));
    }

    layout.clear_layer (deep_layer ().layer ());
    top_cell.shapes (deep_layer ().layer ()).swap (flat_shapes);

  }
}

TextsIteratorDelegate *DeepTexts::begin () const
{
  return new DeepTextsIterator (begin_iter ().first);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> DeepTexts::begin_iter () const
{
  const db::Layout &layout = deep_layer ().layout ();
  if (layout.cells () == 0) {

    return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

  } else {

    const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    db::RecursiveShapeIterator iter (deep_layer ().layout (), top_cell, deep_layer ().layer ());
    return std::make_pair (iter, db::ICplxTrans ());

  }
}

size_t DeepTexts::count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

size_t DeepTexts::hier_count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

std::string DeepTexts::to_string (size_t nmax) const
{
  return db::AsIfFlatTexts::to_string (nmax);
}

Box DeepTexts::bbox () const
{
  return deep_layer ().initial_cell ().bbox (deep_layer ().layer ());
}

bool DeepTexts::empty () const
{
  return begin_iter ().first.at_end ();
}

const db::Text *DeepTexts::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to texts is available only for flat text collections")));
}

bool DeepTexts::has_valid_texts () const
{
  return false;
}

const db::RecursiveShapeIterator *DeepTexts::iter () const
{
  return 0;
}

void DeepTexts::apply_property_translator (const db::PropertiesTranslator &pt)
{
  DeepShapeCollectionDelegateBase::apply_property_translator (pt);
}

db::PropertiesRepository *DeepTexts::properties_repository ()
{
  return &deep_layer ().layout ().properties_repository ();
}

const db::PropertiesRepository *DeepTexts::properties_repository () const
{
  return &deep_layer ().layout ().properties_repository ();
}

TextsDelegate *
DeepTexts::add_in_place (const Texts &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepTexts *other_deep = dynamic_cast <const DeepTexts *> (other.delegate ());
  if (other_deep) {

    deep_layer ().add_from (other_deep->deep_layer ());

  } else {

    //  non-deep to deep merge (flat)

    db::Shapes &shapes = deep_layer ().initial_cell ().shapes (deep_layer ().layer ());
    for (db::Texts::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      shapes.insert (*p);
    }

  }

  return this;
}

TextsDelegate *DeepTexts::add (const Texts &other) const
{
  if (other.empty ()) {
    return clone ();
  } else if (empty ()) {
    return other.delegate ()->clone ();
  } else {
    DeepTexts *new_texts = dynamic_cast<DeepTexts *> (clone ());
    new_texts->add_in_place (other);
    return new_texts;
  }
}

TextsDelegate *DeepTexts::filter_in_place (const TextFilterBase &filter)
{
  //  TODO: implement as really in place
  *this = *apply_filter (filter);
  return this;
}

TextsDelegate *DeepTexts::filtered (const TextFilterBase &filter) const
{
  return apply_filter (filter);
}

DeepTexts *DeepTexts::apply_filter (const TextFilterBase &filter) const
{
  const db::DeepLayer &texts = deep_layer ();
  db::Layout &layout = const_cast<db::Layout &> (texts.layout ());

  std::unique_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter.vars ()));

    vars->collect (&layout, texts.initial_cell ().cell_index ());

    if (filter.wants_variants ()) {
      vars->separate_variants ();
    }

  }

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  std::unique_ptr<db::DeepTexts> res (new db::DeepTexts (texts.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &s = c->shapes (texts.layer ());

    if (vars.get ()) {

      const std::set<db::ICplxTrans> &vv = vars->variants (c->cell_index ());
      for (auto v = vv.begin (); v != vv.end (); ++v) {

        db::Shapes *st;
        if (vv.size () == 1) {
          st = & c->shapes (res->deep_layer ().layer ());
        } else {
          st = & to_commit [c->cell_index ()] [*v];
        }

        for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Texts); ! si.at_end (); ++si) {
          db::Text text;
          si->text (text);
          if (filter.selected (text.transformed (*v))) {
            st->insert (*si);
          }
        }

      }

    } else {

      db::Shapes &st = c->shapes (res->deep_layer ().layer ());

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Texts); ! si.at_end (); ++si) {
        db::Text text;
        si->text (text);
        if (filter.selected (text)) {
          st.insert (*si);
        }
      }

    }

  }

  if (! to_commit.empty () && vars.get ()) {
    vars->commit_shapes (res->deep_layer ().layer (), to_commit);
  }

  return res.release ();
}

RegionDelegate *
DeepTexts::processed_to_polygons (const TextToPolygonProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::Text, db::Polygon, db::DeepRegion> (deep_layer (), filter);
}

RegionDelegate *DeepTexts::polygons (db::Coord e) const
{
  db::DeepLayer new_layer = deep_layer ().derived ();
  db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (db::ShapeIterator::Texts); ! s.at_end (); ++s) {
      db::Box box = s->bbox ();
      box.enlarge (db::Vector (e, e));
      db::Polygon poly (box);
      output.insert (db::PolygonRef (poly, layout.shape_repository ()));
    }
  }

  return new db::DeepRegion (new_layer);
}

EdgesDelegate *DeepTexts::edges () const
{
  db::DeepLayer new_layer = deep_layer ().derived ();
  db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (db::ShapeIterator::Texts); ! s.at_end (); ++s) {
      db::Box box = s->bbox ();
      output.insert (db::Edge (box.p1 (), box.p2 ()));
    }
  }

  return new db::DeepEdges (new_layer);
}

TextsDelegate *DeepTexts::in (const Texts &other, bool invert) const
{
  //  TODO: implement
  return AsIfFlatTexts::in (other, invert);
}

bool DeepTexts::equals (const Texts &other) const
{
  const DeepTexts *other_delegate = dynamic_cast<const DeepTexts *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()
      && other_delegate->deep_layer ().layer () == deep_layer ().layer ()) {
    return true;
  } else {
    return AsIfFlatTexts::equals (other);
  }
}

bool DeepTexts::less (const Texts &other) const
{
  const DeepTexts *other_delegate = dynamic_cast<const DeepTexts *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()) {
    return other_delegate->deep_layer ().layer () < deep_layer ().layer ();
  } else {
    return AsIfFlatTexts::less (other);
  }
}

void DeepTexts::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  deep_layer ().insert_into (layout, into_cell, into_layer);
}

void DeepTexts::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  deep_layer ().insert_into_as_polygons (layout, into_cell, into_layer, enl);
}

namespace {

class Text2PolygonInteractingLocalOperation
  : public local_operation<db::TextRef, db::PolygonRef, db::TextRef>
{
public:
  Text2PolygonInteractingLocalOperation (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::TextRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::TextRef> > &results, const db::LocalProcessorBase * /*proc*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::TextRef> &result = results.front ();

    db::box_scanner2<db::TextRef, size_t, db::Polygon, size_t> scanner;

    std::set<db::PolygonRef> others;
    for (shape_interactions<db::TextRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::TextRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<db::TextRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::TextRef &subject = interactions.subject_shape (i->first);
      scanner.insert1 (&subject, 0);
    }

    std::list<db::Polygon> heap;
    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert2 (& heap.back (), 1);
    }

    if (m_inverse) {

      std::unordered_set<db::TextRef> interacting;
      text_to_region_interaction_filter<std::unordered_set<db::TextRef>, db::TextRef> filter (interacting);
      scanner.process (filter, 1, db::box_convert<db::TextRef> (), db::box_convert<db::Polygon> ());

      for (shape_interactions<db::TextRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        const db::TextRef &subject = interactions.subject_shape (i->first);
        if (interacting.find (subject) == interacting.end ()) {
          result.insert (subject);
        }
      }

    } else {

      text_to_region_interaction_filter<std::unordered_set<db::TextRef>, db::TextRef> filter (result);
      scanner.process (filter, 1, db::box_convert<db::TextRef> (), db::box_convert<db::Polygon> ());

    }
  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    if (m_inverse) {
      return Copy;
    } else {
      return Drop;
    }
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select interacting texts"));
  }

private:
  bool m_inverse;
};

struct ResultInserter
{
  typedef db::Polygon value_type;

  ResultInserter (db::Layout *layout, std::unordered_set<db::PolygonRef> &result)
    : mp_layout (layout), mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Polygon &p)
  {
    (*mp_result).insert (db::PolygonRef (p, mp_layout->shape_repository ()));
  }

private:
  db::Layout *mp_layout;
  std::unordered_set<db::PolygonRef> *mp_result;
};

class Text2PolygonPullLocalOperation
  : public local_operation<db::TextRef, db::PolygonRef, db::PolygonRef>
{
public:
  Text2PolygonPullLocalOperation ()
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::TextRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase * /*proc*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::PolygonRef> &result = results.front ();

    db::box_scanner2<db::TextRef, size_t, db::Polygon, size_t> scanner;

    std::set<db::PolygonRef> others;
    for (shape_interactions<db::TextRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::TextRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<db::TextRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::TextRef &subject = interactions.subject_shape (i->first);
      scanner.insert1 (&subject, 1);
    }

    std::list<db::Polygon> heap;
    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert2 (& heap.back (), 0);
    }

    ResultInserter inserter (layout, result);
    text_to_region_interaction_filter<ResultInserter, db::TextRef> filter (inserter);
    scanner.process (filter, 1, db::box_convert<db::TextRef> (), db::box_convert<db::Polygon> ());
  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select interacting regions"));
  }
};

}

TextsDelegate *
DeepTexts::selected_interacting_generic (const Region &other, bool inverse) const
{
  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &texts = deep_layer ();

  DeepLayer dl_out (texts.derived ());

  db::Text2PolygonInteractingLocalOperation op (inverse);

  db::local_processor<db::TextRef, db::PolygonRef, db::TextRef> proc (const_cast<db::Layout *> (&texts.layout ()), const_cast<db::Cell *> (&texts.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell ());
  proc.set_base_verbosity (other.base_verbosity ());
  proc.set_threads (texts.store ()->threads ());

  proc.run (&op, texts.layer (), other_deep->deep_layer ().layer (), dl_out.layer ());

  return new db::DeepTexts (dl_out);
}

RegionDelegate *DeepTexts::pull_generic (const Region &other) const
{
  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &texts = deep_layer ();
  const db::DeepLayer &other_polygons = other_deep->merged_deep_layer ();

  DeepLayer dl_out (other_polygons.derived ());

  db::Text2PolygonPullLocalOperation op;

  db::local_processor<db::TextRef, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&texts.layout ()), const_cast<db::Cell *> (&texts.initial_cell ()), &other_polygons.layout (), &other_polygons.initial_cell ());
  proc.set_base_verbosity (other.base_verbosity ());
  proc.set_threads (texts.store ()->threads ());

  proc.run (&op, texts.layer (), other_polygons.layer (), dl_out.layer ());

  return new db::DeepRegion (dl_out);
}

}
