
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


#ifndef HDR_dbFlatRegion
#define HDR_dbFlatRegion

#include "dbCommon.h"

#include "dbAsIfFlatRegion.h"
#include "dbShapes.h"
#include "dbShapes2.h"

namespace db {

/**
 *  @brief An iterator delegate for the flat region
 */
class DB_PUBLIC FlatRegionIterator
  : public RegionIteratorDelegate
{
public:
  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator iterator_type;

  FlatRegionIterator (iterator_type from, iterator_type to)
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

  virtual RegionIteratorDelegate *clone () const
  {
    return new FlatRegionIterator (*this);
  }

private:
  friend class Region;

  iterator_type m_from, m_to;
};

/**
 *  @brief A flat, polygon-set delegate
 */
class DB_PUBLIC FlatRegion
  : public AsIfFlatRegion
{
public:
  typedef db::Polygon value_type;
  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator polygon_iterator_type;

  FlatRegion ();
  FlatRegion (const db::Shapes &polygons, bool is_merged);
  FlatRegion (bool is_merged);

  FlatRegion (const FlatRegion &other);

  virtual ~FlatRegion ();

  RegionDelegate *clone () const
  {
    return new FlatRegion (*this);
  }

  void reserve (size_t);

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual size_t size () const;
  virtual bool is_merged () const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

  virtual RegionDelegate *merged_in_place ();
  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc);
  virtual RegionDelegate *merged () const;
  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const
  {
    return db::AsIfFlatRegion::merged (min_coherence, min_wc);
  }

  virtual RegionDelegate *process_in_place (const PolygonProcessorBase &filter);
  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter);

  virtual RegionDelegate *add_in_place (const Region &other);
  virtual RegionDelegate *add (const Region &other) const;

  virtual const db::Polygon *nth (size_t n) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const db::RecursiveShapeIterator *iter () const;

  void insert (const db::Box &box);
  void insert (const db::Path &path);
  void insert (const db::SimplePolygon &polygon);
  void insert (const db::Polygon &polygon);
  void insert (const db::Shape &shape);

  template <class T>
  void insert (const db::Shape &shape, const T &trans)
  {
    if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
      db::Polygon poly;
      shape.polygon (poly);
      poly.transform (trans);
      insert (poly);
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
      for (polygon_iterator_type p = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (); p != m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end (); ++p) {
        m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }

  db::Shapes &raw_polygons () { return m_polygons; }

protected:
  virtual void merged_semantics_changed ();
  virtual void min_coherence_changed ();
  virtual Box compute_bbox () const;
  void invalidate_cache ();
  void set_is_merged (bool m);

private:
  friend class AsIfFlatRegion;
  friend class Region;

  FlatRegion &operator= (const FlatRegion &other);

  bool m_is_merged;
  mutable db::Shapes m_polygons;
  mutable db::Shapes m_merged_polygons;
  mutable bool m_merged_polygons_valid;

  void init ();
  void ensure_merged_polygons_valid () const;
};

}

#endif

