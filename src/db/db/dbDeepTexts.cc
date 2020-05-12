
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


#include "dbDeepTexts.h"
#include "dbCellGraphUtils.h"
#include "dbDeepEdges.h"
#include "dbDeepRegion.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"

#include <sstream>

namespace db
{

/**
 *  @brief An iterator delegate for the deep text collection
 *  TODO: this is kind of redundant with OriginalLayerIterator ..
 */
class DB_PUBLIC DeepTextsIterator
  : public TextsIteratorDelegate
{
public:
  DeepTextsIterator (const db::RecursiveShapeIterator &iter)
    : m_iter (iter)
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

  virtual const value_type *get () const
  {
    return &m_text;
  }

  virtual TextsIteratorDelegate *clone () const
  {
    return new DeepTextsIterator (*this);
  }

private:
  friend class Texts;

  db::RecursiveShapeIterator m_iter;
  mutable value_type m_text;

  void set () const  {
    if (! m_iter.at_end ()) {
      m_iter.shape ().text (m_text);
      m_text.transform (m_iter.trans ());
    }
  }
};


DeepTexts::DeepTexts ()
  : AsIfFlatTexts (), m_deep_layer ()
{
  //  .. nothing yet ..
}

DeepTexts::DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss)
  : AsIfFlatTexts (), m_deep_layer (dss.create_text_layer (si))
{
  //  .. nothing yet ..
}

DeepTexts::DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans)
  : AsIfFlatTexts (), m_deep_layer (dss.create_text_layer (si, trans))
{
  //  .. nothing yet ..
}

DeepTexts::DeepTexts (const DeepTexts &other)
  : AsIfFlatTexts (other),
    m_deep_layer (other.m_deep_layer.copy ())
{
  //  .. nothing yet ..
}

DeepTexts::DeepTexts (const DeepLayer &dl)
  : AsIfFlatTexts (), m_deep_layer (dl)
{
  //  .. nothing yet ..
}

DeepTexts::~DeepTexts ()
{
  //  .. nothing yet ..
}

TextsDelegate *DeepTexts::clone () const
{
  return new DeepTexts (*this);
}

TextsIteratorDelegate *DeepTexts::begin () const
{
  return new DeepTextsIterator (begin_iter ().first);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> DeepTexts::begin_iter () const
{
  const db::Layout &layout = m_deep_layer.layout ();
  if (layout.cells () == 0) {

    return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

  } else {

    const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    db::RecursiveShapeIterator iter (m_deep_layer.layout (), top_cell, m_deep_layer.layer ());
    return std::make_pair (iter, db::ICplxTrans ());

  }
}

size_t DeepTexts::size () const
{
  size_t n = 0;

  const db::Layout &layout = m_deep_layer.layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (m_deep_layer.layer ()).size ();
  }

  return n;
}

std::string DeepTexts::to_string (size_t nmax) const
{
  return db::AsIfFlatTexts::to_string (nmax);
}

Box DeepTexts::bbox () const
{
  return m_deep_layer.initial_cell ().bbox (m_deep_layer.layer ());
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

  std::auto_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter.vars ()));

    vars->collect (texts.layout (), texts.initial_cell ());

    if (filter.wants_variants ()) {
      const_cast<db::DeepLayer &> (texts).separate_variants (*vars);
    }

  }

  db::Layout &layout = const_cast<db::Layout &> (texts.layout ());
  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  std::auto_ptr<db::DeepTexts> res (new db::DeepTexts (texts.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &s = c->shapes (texts.layer ());

    if (vars.get ()) {

      const std::map<db::ICplxTrans, size_t> &vv = vars->variants (c->cell_index ());
      for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        db::Shapes *st;
        if (vv.size () == 1) {
          st = & c->shapes (res->deep_layer ().layer ());
        } else {
          st = & to_commit [c->cell_index ()] [v->first];
        }

        const db::ICplxTrans &tr = v->first;

        for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Texts); ! si.at_end (); ++si) {
          db::Text text;
          si->text (text);
          if (filter.selected (text.transformed (tr))) {
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
    res->deep_layer ().commit_shapes (*vars, to_commit);
  }

  return res.release ();
}

RegionDelegate *DeepTexts::polygons (db::Coord e) const
{
  db::DeepLayer new_layer = m_deep_layer.derived ();
  db::Layout &layout = const_cast<db::Layout &> (m_deep_layer.layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (m_deep_layer.layer ()).begin (db::ShapeIterator::Texts); ! s.at_end (); ++s) {
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
  db::DeepLayer new_layer = m_deep_layer.derived ();
  db::Layout &layout = const_cast<db::Layout &> (m_deep_layer.layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (m_deep_layer.layer ()).begin (db::ShapeIterator::Texts); ! s.at_end (); ++s) {
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
  if (other_delegate && &other_delegate->m_deep_layer.layout () == &m_deep_layer.layout ()
      && other_delegate->m_deep_layer.layer () == m_deep_layer.layer ()) {
    return true;
  } else {
    return AsIfFlatTexts::equals (other);
  }
}

bool DeepTexts::less (const Texts &other) const
{
  const DeepTexts *other_delegate = dynamic_cast<const DeepTexts *> (other.delegate ());
  if (other_delegate && &other_delegate->m_deep_layer.layout () == &m_deep_layer.layout ()) {
    return other_delegate->m_deep_layer.layer () < m_deep_layer.layer ();
  } else {
    return AsIfFlatTexts::less (other);
  }
}

void DeepTexts::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  m_deep_layer.insert_into (layout, into_cell, into_layer);
}

void DeepTexts::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  m_deep_layer.insert_into_as_polygons (layout, into_cell, into_layer, enl);
}


}
