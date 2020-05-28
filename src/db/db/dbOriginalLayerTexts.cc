
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
    OriginalLayerTextsIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
      : m_rec_iter (iter), m_iter_trans (trans)
    {
      set ();
    }

    virtual bool at_end () const
    {
      return m_rec_iter.at_end ();
    }

    virtual void increment ()
    {
      inc ();
      set ();
    }

    virtual const value_type *get () const
    {
      return &m_text;
    }

    virtual TextsIteratorDelegate *clone () const
    {
      return new OriginalLayerTextsIterator (*this);
    }

  private:
    friend class Texts;

    db::RecursiveShapeIterator m_rec_iter;
    db::ICplxTrans m_iter_trans;
    db::Text m_text;

    void set ()
    {
      while (! m_rec_iter.at_end () && ! m_rec_iter.shape ().is_text ()) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter.shape ().text (m_text);
        m_text.transform (m_iter_trans * m_rec_iter.trans ());
      }
    }

    void inc ()
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
