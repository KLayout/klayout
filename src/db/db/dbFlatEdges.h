
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


#ifndef HDR_dbFlatEdges
#define HDR_dbFlatEdges

#include "dbCommon.h"

#include "dbMutableEdges.h"
#include "dbShapes.h"
#include "dbShapes2.h"
#include "dbGenericShapeIterator.h"
#include "tlCopyOnWrite.h"

namespace db {

/**
 *  @brief An iterator delegate for the flat edge set
 */
typedef generic_shapes_iterator_delegate<db::Edge> FlatEdgesIterator;

/**
 *  @brief A flat, edge-set delegate
 */
class DB_PUBLIC FlatEdges
  : public MutableEdges
{
public:
  typedef db::Edge value_type;

  typedef db::layer<db::Edge, db::unstable_layer_tag> edge_layer_type;
  typedef edge_layer_type::iterator edge_iterator_type;
  typedef db::layer<db::EdgeWithProperties, db::unstable_layer_tag> edge_layer_wp_type;
  typedef edge_layer_wp_type::iterator edge_iterator_wp_type;

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
  void flatten () { }

  virtual EdgesIteratorDelegate *begin () const;
  virtual EdgesIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;
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
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  void do_insert (const db::Edge &edge, properties_id_type prop_id);

  void do_transform (const db::Trans &t)
  {
    transform_generic (t);
  }

  void do_transform (const db::ICplxTrans &t)
  {
    transform_generic (t);
  }

  void do_transform (const db::IMatrix2d &t)
  {
    transform_generic (t);
  }

  void do_transform (const db::IMatrix3d &t)
  {
    transform_generic (t);
  }

  db::Shapes &raw_edges () { return *mp_edges; }
  const db::Shapes &raw_edges () const { return *mp_edges; }

protected:
  virtual void merged_semantics_changed ();
  virtual Box compute_bbox () const;
  void invalidate_cache ();
  void set_is_merged (bool m);

private:
  friend class AsIfFlatEdges;

  FlatEdges &operator= (const FlatEdges &other);

  bool m_is_merged;
  mutable tl::copy_on_write_ptr<db::Shapes> mp_edges;
  mutable tl::copy_on_write_ptr<db::Shapes> mp_merged_edges;
  mutable bool m_merged_edges_valid;
  mutable tl::copy_on_write_ptr<db::PropertiesRepository> mp_properties_repository;

  void init ();
  void ensure_merged_edges_valid () const;

  template <class Trans>
  void transform_generic (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      db::Shapes &e = *mp_edges;
      for (edge_iterator_type p = e.template get_layer<db::Edge, db::unstable_layer_tag> ().begin (); p != e.get_layer<db::Edge, db::unstable_layer_tag> ().end (); ++p) {
        e.get_layer<db::Edge, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      for (edge_iterator_wp_type p = e.template get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().begin (); p != e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().end (); ++p) {
        e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }
};

}

#endif

