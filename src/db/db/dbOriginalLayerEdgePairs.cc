
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
    typedef db::EdgePair value_type;

    OriginalLayerEdgePairsIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
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

    virtual EdgePairsIteratorDelegate *clone () const
    {
      return new OriginalLayerEdgePairsIterator (*this);
    }

    virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
    {
      const OriginalLayerEdgePairsIterator *o = dynamic_cast<const OriginalLayerEdgePairsIterator *> (other);
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
    friend class EdgePairs;

    db::RecursiveShapeIterator m_rec_iter;
    db::ICplxTrans m_iter_trans;
    value_type m_shape;
    db::properties_id_type m_prop_id;

    void set ()
    {
      while (! m_rec_iter.at_end () && !m_rec_iter->is_edge_pair ()) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter->edge_pair (m_shape);
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

void
OriginalLayerEdgePairs::apply_property_translator (const db::PropertiesTranslator &pt)
{
  m_iter.apply_property_translator (pt);
}

db::PropertiesRepository *
OriginalLayerEdgePairs::properties_repository ()
{
  return m_iter.layout () ? &const_cast<db::Layout * >(m_iter.layout ())->properties_repository () : 0;
}

const db::PropertiesRepository *
OriginalLayerEdgePairs::properties_repository () const
{
  return m_iter.layout () ? &m_iter.layout ()->properties_repository () : 0;
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
