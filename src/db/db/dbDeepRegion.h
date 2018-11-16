
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#ifndef HDR_dbDeepRegion
#define HDR_dbDeepRegion

#include "dbCommon.h"

#include "dbAsIfFlatRegion.h"
#include "dbDeepShapeStore.h"

namespace db {

/**
 *  @brief An iterator delegate for the deep region
 */
class DB_PUBLIC DeepRegionIterator
  : public RegionIteratorDelegate
{
public:
  typedef db::Polygon value_type;

  DeepRegionIterator (const db::RecursiveShapeIterator &iter)
    : m_iter (iter)
  {
    set ();
  }

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
    return &m_tmp;
  }

  virtual RegionIteratorDelegate *clone () const
  {
    return new DeepRegionIterator (*this);
  }

private:
  friend class Region;

  db::RecursiveShapeIterator m_iter;
  mutable value_type m_tmp;

  void set () const
  {
    if (! m_iter.at_end ()) {
      m_iter->polygon (m_tmp);
    }
  }
};

/**
 *  @brief A flat, polygon-set delegate
 */
class DB_PUBLIC DeepRegion
  : public AsIfFlatRegion
{
public:
  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator polygon_iterator_type;

  DeepRegion ();
  DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio = 3.0, size_t max_vertex_count = 16);
  DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics = true, double area_ratio = 3.0, size_t max_vertex_count = 16);

  DeepRegion (const DeepRegion &other);

  virtual ~DeepRegion ();

  RegionDelegate *clone () const;

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;

  virtual bool is_merged () const;

  virtual const db::Polygon *nth (size_t n) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const db::RecursiveShapeIterator *iter () const;

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

protected:
  virtual void merged_semantics_changed ();

private:
  DeepRegion &operator= (const DeepRegion &other);

  DeepLayer m_deep_layer;
  //  @@@ have hierarchical merged polygons later
  mutable db::Shapes m_merged_polygons;
  mutable bool m_merged_polygons_valid;

  void init ();
  void ensure_merged_polygons_valid () const;
};

}

#endif

