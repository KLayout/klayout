
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


#ifndef HDR_dbFlatEdges
#define HDR_dbFlatEdges

#include "dbCommon.h"

#include "dbAsIfFlatEdges.h"
#include "dbShapes.h"
#include "dbShapes2.h"

namespace db {

/**
 *  @brief An iterator delegate for the flat region
 */
class DB_PUBLIC FlatEdgesIterator
  : public EdgesIteratorDelegate
{
public:
  typedef db::layer<db::Edge, db::unstable_layer_tag> edge_layer_type;
  typedef edge_layer_type::iterator iterator_type;

  FlatEdgesIterator (iterator_type from, iterator_type to)
    : m_from (from), m_to (to)
  {
    //  .. nothing yet ..
  }

  virtual bool at_end () const
  {
    return m_from == m_to;
  }

  virtual void increment ()
  {
    ++m_from;
  }

  virtual const value_type *get () const
  {
    return m_from.operator-> ();
  }

  virtual EdgesIteratorDelegate *clone () const
  {
    return new FlatEdgesIterator (*this);
  }

private:
  friend class Edges;

  iterator_type m_from, m_to;
};

/**
 *  @brief A flat, edge-set delegate
 */
class DB_PUBLIC FlatEdges
  : public AsIfFlatEdges
{
public:
  typedef db::Edge value_type;

  typedef db::layer<db::Edge, db::unstable_layer_tag> edge_layer_type;
  typedef edge_layer_type::iterator edge_iterator_type;

  FlatEdges ();
  FlatEdges (const db::Shapes &edges, bool is_merged);
  FlatEdges (bool is_merged);

  FlatEdges (const FlatEdges &other);

  virtual ~FlatEdges ();

  EdgesDelegate *clone () const
  {
    return new FlatEdges (*this);
  }

  void reserve (size_t);

  virtual EdgesIteratorDelegate *begin () const;
  virtual EdgesIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual size_t size () const;
  virtual bool is_merged () const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

  virtual EdgesDelegate *processed_in_place (const EdgeProcessorBase &filter);
  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter);

  virtual EdgesDelegate *add_in_place (const Edges &other);
  virtual EdgesDelegate *add (const Edges &other) const;

  virtual const db::Edge *nth (size_t n) const;
  virtual bool has_valid_edges () const;
  virtual bool has_valid_merged_edges () const;

  virtual const db::RecursiveShapeIterator *iter () const;

  void insert (const db::Box &box);
  void insert (const db::Path &path);
  void insert (const db::SimplePolygon &polygon);
  void insert (const db::Polygon &polygon);
  void insert (const db::Edge &edge);
  void insert (const db::Shape &shape);

  template <class T>
  void insert (const db::Shape &shape, const T &trans)
  {
    if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {

      db::Polygon poly;
      shape.polygon (poly);
      poly.transform (trans);
      insert (poly);

    } else if (shape.is_edge ()) {

      db::Edge edge;
      shape.edge (edge);
      edge.transform (trans);
      insert (edge);

    }
  }

  template <class Iter>
  void insert (const Iter &b, const Iter &e)
  {
    reserve (size () + (e - b));
    for (Iter i = b; i != e; ++i) {
      insert (*i);
    }
  }

  template <class Iter>
  void insert_seq (const Iter &seq)
  {
    for (Iter i = seq; ! i.at_end (); ++i) {
      insert (*i);
    }
  }

  template <class Trans>
  void transform (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      for (edge_iterator_type p = m_edges.template get_layer<db::Edge, db::unstable_layer_tag> ().begin (); p != m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end (); ++p) {
        m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }

  db::Shapes &raw_edges () { return m_edges; }

protected:
  virtual void merged_semantics_changed ();
  virtual Box compute_bbox () const;
  void invalidate_cache ();
  void set_is_merged (bool m);

private:
  friend class AsIfFlatEdges;

  FlatEdges &operator= (const FlatEdges &other);

  bool m_is_merged;
  mutable db::Shapes m_edges;
  mutable db::Shapes m_merged_edges;
  mutable bool m_merged_edges_valid;

  void init ();
  void ensure_merged_edges_valid () const;
};

}

#endif

