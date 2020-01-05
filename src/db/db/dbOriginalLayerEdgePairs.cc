
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


#include "dbOriginalLayerEdgePairs.h"
#include "dbEdgePairs.h"
#include "tlInternational.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  OriginalLayerEdgePairs implementation

namespace
{

  class OriginalLayerEdgePairsIterator
    : public EdgePairsIteratorDelegate
  {
  public:
    OriginalLayerEdgePairsIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
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
      return &m_edge_pair;
    }

    virtual EdgePairsIteratorDelegate *clone () const
    {
      return new OriginalLayerEdgePairsIterator (*this);
    }

  private:
    friend class EdgePairs;

    db::RecursiveShapeIterator m_rec_iter;
    db::ICplxTrans m_iter_trans;
    db::EdgePair m_edge_pair;

    void set ()
    {
      while (! m_rec_iter.at_end () && ! m_rec_iter.shape ().is_edge_pair ()) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter.shape ().edge_pair (m_edge_pair);
        m_edge_pair.transform (m_iter_trans * m_rec_iter.trans ());
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

OriginalLayerEdgePairs::OriginalLayerEdgePairs ()
  : AsIfFlatEdgePairs ()
{
  init ();
}

OriginalLayerEdgePairs::OriginalLayerEdgePairs (const OriginalLayerEdgePairs &other)
  : AsIfFlatEdgePairs (other),
    m_iter (other.m_iter),
    m_iter_trans (other.m_iter_trans)
{
  //  .. nothing yet ..
}

OriginalLayerEdgePairs::OriginalLayerEdgePairs (const RecursiveShapeIterator &si)
  : AsIfFlatEdgePairs (), m_iter (si)
{
  init ();
}

OriginalLayerEdgePairs::OriginalLayerEdgePairs (const RecursiveShapeIterator &si, const db::ICplxTrans &trans)
  : AsIfFlatEdgePairs (), m_iter (si), m_iter_trans (trans)
{
  init ();
}

OriginalLayerEdgePairs::~OriginalLayerEdgePairs ()
{
  //  .. nothing yet ..
}

EdgePairsDelegate *
OriginalLayerEdgePairs::clone () const
{
  return new OriginalLayerEdgePairs (*this);
}

EdgePairsIteratorDelegate *
OriginalLayerEdgePairs::begin () const
{
  return new OriginalLayerEdgePairsIterator (m_iter, m_iter_trans);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
OriginalLayerEdgePairs::begin_iter () const
{
  return std::make_pair (m_iter, m_iter_trans);
}

bool
OriginalLayerEdgePairs::empty () const
{
  return m_iter.at_end ();
}

const db::EdgePair *
OriginalLayerEdgePairs::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to edge pairs is available only for flat collections")));
}

bool
OriginalLayerEdgePairs::has_valid_edge_pairs () const
{
  return false;
}

const db::RecursiveShapeIterator *
OriginalLayerEdgePairs::iter () const
{
  return &m_iter;
}

bool
OriginalLayerEdgePairs::equals (const EdgePairs &other) const
{
  const OriginalLayerEdgePairs *other_delegate = dynamic_cast<const OriginalLayerEdgePairs *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return true;
  } else {
    return AsIfFlatEdgePairs::equals (other);
  }
}

bool
OriginalLayerEdgePairs::less (const EdgePairs &other) const
{
  const OriginalLayerEdgePairs *other_delegate = dynamic_cast<const OriginalLayerEdgePairs *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return false;
  } else {
    return AsIfFlatEdgePairs::less (other);
  }
}

void
OriginalLayerEdgePairs::init ()
{
  //  .. nothing yet ..
}

}
