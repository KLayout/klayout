
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


#ifndef HDR_dbOriginalLayerRegion
#define HDR_dbOriginalLayerRegion

#include "dbCommon.h"

#include "dbAsIfFlatRegion.h"

namespace db {

class EdgesDelegate;
class RegionDelegate;
class DeepShapeStore;

/**
 *  @brief An original layerregion based on a RecursiveShapeIterator
 */
class DB_PUBLIC OriginalLayerRegion
  : public AsIfFlatRegion
{
public:
  OriginalLayerRegion ();
  OriginalLayerRegion (const OriginalLayerRegion &other);
  OriginalLayerRegion (const RecursiveShapeIterator &si, bool is_merged = false);
  OriginalLayerRegion (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics, bool is_merged = false);
  virtual ~OriginalLayerRegion ();

  RegionDelegate *clone () const;

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;

  virtual bool is_merged () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;

  virtual const db::Polygon *nth (size_t n) const;
  virtual db::properties_id_type nth_prop_id (size_t) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const db::RecursiveShapeIterator *iter () const;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

protected:
  virtual void merged_semantics_changed ();
  virtual void min_coherence_changed ();

private:
  OriginalLayerRegion &operator= (const OriginalLayerRegion &other);

  bool m_is_merged;
  mutable db::Shapes m_merged_polygons;
  mutable bool m_merged_polygons_valid;
  mutable db::RecursiveShapeIterator m_iter;
  db::ICplxTrans m_iter_trans;

  void init ();
  void ensure_merged_polygons_valid () const;
};

}

#endif

