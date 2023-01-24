
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


#include "dbOriginalLayerEdges.h"
#include "dbFlatEdges.h"
#include "dbEdges.h"
#include "dbEdgeBoolean.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  OriginalLayerEdges implementation

namespace
{

  class OriginalLayerEdgesIterator
    : public EdgesIteratorDelegate
  {
  public:
    typedef db::Edge value_type;

    OriginalLayerEdgesIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
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

    virtual EdgesIteratorDelegate *clone () const
    {
      return new OriginalLayerEdgesIterator (*this);
    }

    virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
    {
      const OriginalLayerEdgesIterator *o = dynamic_cast<const OriginalLayerEdgesIterator *> (other);
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
    friend class Edges;

    db::RecursiveShapeIterator m_rec_iter;
    db::ICplxTrans m_iter_trans;
    value_type m_shape;
    db::properties_id_type m_prop_id;

    void set ()
    {
      while (! m_rec_iter.at_end () && !m_rec_iter->is_edge ()) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter->edge (m_shape);
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

OriginalLayerEdges::OriginalLayerEdges ()
  : AsIfFlatEdges (), m_merged_edges (false)
{
  init ();
}

OriginalLayerEdges::OriginalLayerEdges (const OriginalLayerEdges &other)
  : AsIfFlatEdges (other),
    m_is_merged (other.m_is_merged),
    m_merged_edges (other.m_merged_edges),
    m_merged_edges_valid (other.m_merged_edges_valid),
    m_iter (other.m_iter),
    m_iter_trans (other.m_iter_trans)
{
  //  .. nothing yet ..
}

OriginalLayerEdges::OriginalLayerEdges (const RecursiveShapeIterator &si, bool is_merged)
  : AsIfFlatEdges (), m_merged_edges (false), m_iter (si)
{
  init ();

  m_is_merged = is_merged;
}

OriginalLayerEdges::OriginalLayerEdges (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics, bool is_merged)
  : AsIfFlatEdges (), m_merged_edges (false), m_iter (si), m_iter_trans (trans)
{
  init ();

  m_is_merged = is_merged;
  set_merged_semantics (merged_semantics);
}

OriginalLayerEdges::~OriginalLayerEdges ()
{
  //  .. nothing yet ..
}

EdgesDelegate *
OriginalLayerEdges::clone () const
{
  return new OriginalLayerEdges (*this);
}

void
OriginalLayerEdges::merged_semantics_changed ()
{
  m_merged_edges.clear ();
  m_merged_edges_valid = false;
}

EdgesIteratorDelegate *
OriginalLayerEdges::begin () const
{
  return new OriginalLayerEdgesIterator (m_iter, m_iter_trans);
}

EdgesIteratorDelegate *
OriginalLayerEdges::begin_merged () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_edges_valid ();
    return new FlatEdgesIterator (&m_merged_edges);
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
OriginalLayerEdges::begin_iter () const
{
  return std::make_pair (m_iter, m_iter_trans);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
OriginalLayerEdges::begin_merged_iter () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_edges_valid ();
    return std::make_pair (db::RecursiveShapeIterator (m_merged_edges), db::ICplxTrans ());
  }
}

bool
OriginalLayerEdges::empty () const
{
  return m_iter.at_end ();
}

bool
OriginalLayerEdges::is_merged () const
{
  return m_is_merged;
}

const db::Edge *
OriginalLayerEdges::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to edges is available only for flat collections")));
}

bool
OriginalLayerEdges::has_valid_edges () const
{
  return false;
}

bool
OriginalLayerEdges::has_valid_merged_edges () const
{
  return merged_semantics () && ! m_is_merged;
}

const db::RecursiveShapeIterator *
OriginalLayerEdges::iter () const
{
  return &m_iter;
}

void
OriginalLayerEdges::apply_property_translator (const db::PropertiesTranslator &pt)
{
  m_iter.apply_property_translator (pt);

  m_merged_edges_valid = false;
  m_merged_edges.clear ();
}

db::PropertiesRepository *
OriginalLayerEdges::properties_repository ()
{
  return m_iter.layout () ? &const_cast<db::Layout * >(m_iter.layout ())->properties_repository () : 0;
}

const db::PropertiesRepository *
OriginalLayerEdges::properties_repository () const
{
  return m_iter.layout () ? &m_iter.layout ()->properties_repository () : 0;
}

bool
OriginalLayerEdges::equals (const Edges &other) const
{
  const OriginalLayerEdges *other_delegate = dynamic_cast<const OriginalLayerEdges *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return true;
  } else {
    return AsIfFlatEdges::equals (other);
  }
}

bool
OriginalLayerEdges::less (const Edges &other) const
{
  const OriginalLayerEdges *other_delegate = dynamic_cast<const OriginalLayerEdges *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return false;
  } else {
    return AsIfFlatEdges::less (other);
  }
}

void
OriginalLayerEdges::init ()
{
  m_is_merged = false;
  m_merged_edges_valid = false;
}

void
OriginalLayerEdges::ensure_merged_edges_valid () const
{
  if (! m_merged_edges_valid) {

    m_merged_edges.clear ();

    db::Shapes tmp (false);
    EdgeBooleanClusterCollectorToShapes cluster_collector (&tmp, EdgeOr);

    db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
    scanner.reserve (count ());

    AddressableEdgeDelivery e (begin ());

    for ( ; ! e.at_end (); ++e) {
      if (! e->is_degenerate ()) {
        scanner.insert (e.operator-> (), 0);
      }
    }

    scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    m_merged_edges.swap (tmp);
    m_merged_edges_valid = true;

  }
}

}
