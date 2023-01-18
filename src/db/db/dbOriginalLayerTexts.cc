
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


#include "dbOriginalLayerTexts.h"
#include "dbTexts.h"
#include "tlInternational.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  OriginalLayerTexts implementation

namespace
{

  class OriginalLayerTextsIterator
    : public TextsIteratorDelegate
  {
  public:
    typedef db::Text value_type;

    OriginalLayerTextsIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
      : m_rec_iter (iter), m_iter_trans (trans), m_prop_id (0)
    {
      set ();
    }

    virtual bool is_addressable() const
    {
      return false;
    }

    virtual bool at_end () const
    {
      return m_rec_iter.at_end ();
    }

    virtual void increment ()
    {
      do_increment ();
      set ();
    }

    virtual const value_type *get () const
    {
      return &m_shape;
    }

    virtual db::properties_id_type prop_id () const
    {
      return m_prop_id;
    }

    virtual OriginalLayerTextsIterator *clone () const
    {
      return new OriginalLayerTextsIterator (*this);
    }

    virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
    {
      const OriginalLayerTextsIterator *o = dynamic_cast<const OriginalLayerTextsIterator *> (other);
      return o && o->m_rec_iter == m_rec_iter && o->m_iter_trans.equal (m_iter_trans);
    }

    virtual void do_reset (const db::Box &region, bool overlapping)
    {
      if (region == db::Box::world ()) {
        m_rec_iter.set_region (region);
      } else {
        m_rec_iter.set_region (m_iter_trans.inverted () * region);
      }
      m_rec_iter.set_overlapping (overlapping);
      set ();
    }

    virtual db::Box bbox () const
    {
      return m_iter_trans * m_rec_iter.bbox ();
    }

  private:
    friend class Texts;

    db::RecursiveShapeIterator m_rec_iter;
    db::ICplxTrans m_iter_trans;
    value_type m_shape;
    db::properties_id_type m_prop_id;

    void set ()
    {
      while (! m_rec_iter.at_end () && !m_rec_iter.shape ().is_text ()) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter->text (m_shape);
        m_shape.transform (m_iter_trans * m_rec_iter.trans ());
        m_prop_id = m_rec_iter.prop_id ();
      }
    }

    void do_increment ()
    {
      if (! m_rec_iter.at_end ()) {
        ++m_rec_iter;
      }
    }
  };

}

OriginalLayerTexts::OriginalLayerTexts ()
  : AsIfFlatTexts ()
{
  init ();
}

OriginalLayerTexts::OriginalLayerTexts (const OriginalLayerTexts &other)
  : AsIfFlatTexts (other),
    m_iter (other.m_iter),
    m_iter_trans (other.m_iter_trans)
{
  //  .. nothing yet ..
}

OriginalLayerTexts::OriginalLayerTexts (const RecursiveShapeIterator &si)
  : AsIfFlatTexts (), m_iter (si)
{
  init ();
}

OriginalLayerTexts::OriginalLayerTexts (const RecursiveShapeIterator &si, const db::ICplxTrans &trans)
  : AsIfFlatTexts (), m_iter (si), m_iter_trans (trans)
{
  init ();
}

OriginalLayerTexts::~OriginalLayerTexts ()
{
  //  .. nothing yet ..
}

TextsDelegate *
OriginalLayerTexts::clone () const
{
  return new OriginalLayerTexts (*this);
}

TextsIteratorDelegate *
OriginalLayerTexts::begin () const
{
  return new OriginalLayerTextsIterator (m_iter, m_iter_trans);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
OriginalLayerTexts::begin_iter () const
{
  return std::make_pair (m_iter, m_iter_trans);
}

bool
OriginalLayerTexts::empty () const
{
  return m_iter.at_end ();
}

const db::Text *
OriginalLayerTexts::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to texts is available only for flat collections")));
}

bool
OriginalLayerTexts::has_valid_texts () const
{
  return false;
}

const db::RecursiveShapeIterator *
OriginalLayerTexts::iter () const
{
  return &m_iter;
}

void
OriginalLayerTexts::apply_property_translator (const db::PropertiesTranslator &pt)
{
  m_iter.apply_property_translator (pt);
}

db::PropertiesRepository *
OriginalLayerTexts::properties_repository ()
{
  return m_iter.layout () ? &const_cast<db::Layout * >(m_iter.layout ())->properties_repository () : 0;
}

const db::PropertiesRepository *
OriginalLayerTexts::properties_repository () const
{
  return m_iter.layout () ? &m_iter.layout ()->properties_repository () : 0;
}

bool
OriginalLayerTexts::equals (const Texts &other) const
{
  const OriginalLayerTexts *other_delegate = dynamic_cast<const OriginalLayerTexts *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return true;
  } else {
    return AsIfFlatTexts::equals (other);
  }
}

bool
OriginalLayerTexts::less (const Texts &other) const
{
  const OriginalLayerTexts *other_delegate = dynamic_cast<const OriginalLayerTexts *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return false;
  } else {
    return AsIfFlatTexts::less (other);
  }
}

void
OriginalLayerTexts::init ()
{
  //  .. nothing yet ..
}

}
