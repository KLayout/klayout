
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


#ifndef HDR_dbFlatRegion
#define HDR_dbFlatRegion

#include "dbCommon.h"

#include "dbMutableRegion.h"
#include "dbShapes.h"
#include "dbShapes2.h"
#include "tlCopyOnWrite.h"

namespace db {

/**
 *  @brief An iterator delegate for the flat region
 */
typedef generic_shapes_iterator_delegate<db::Polygon> FlatRegionIterator;

/**
 *  @brief A flat, polygon-set delegate
 */
class DB_PUBLIC FlatRegion
  : public MutableRegion
{
public:
  typedef db::Polygon value_type;
  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator polygon_iterator_type;
  typedef db::layer<db::PolygonWithProperties, db::unstable_layer_tag> polygon_layer_wp_type;
  typedef polygon_layer_wp_type::iterator polygon_iterator_wp_type;

  FlatRegion ();
  FlatRegion (const db::Shapes &polygons, bool is_merged);
  FlatRegion (bool is_merged);

  FlatRegion (const FlatRegion &other);

  virtual ~FlatRegion ();

  RegionDelegate *clone () const
  {
    return new FlatRegion (*this);
  }

  virtual void reserve (size_t);

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;
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
  virtual db::properties_id_type nth_prop_id (size_t) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const db::RecursiveShapeIterator *iter () const;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  void do_insert (const db::Polygon &polygon, db::properties_id_type prop_id);

  void do_transform (const db::Trans &t)
  {
    transform_generic (t);
  }

  void do_transform (const db::ICplxTrans &t)
  {
    transform_generic (t);
  }

  virtual void do_transform (const db::IMatrix2d &t)
  {
    transform_generic (t);
  }

  virtual void do_transform (const db::IMatrix3d &t)
  {
    transform_generic (t);
  }

  void flatten () { }

  db::Shapes &raw_polygons () { return *mp_polygons; }
  const db::Shapes &raw_polygons () const { return *mp_polygons; }

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
  mutable tl::copy_on_write_ptr<db::Shapes> mp_polygons;
  mutable tl::copy_on_write_ptr<db::Shapes> mp_merged_polygons;
  mutable bool m_merged_polygons_valid;
  mutable tl::copy_on_write_ptr<db::PropertiesRepository> mp_properties_repository;

  void init ();
  void ensure_merged_polygons_valid () const;

  template <class Trans>
  void transform_generic (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      db::Shapes &polygons = *mp_polygons;
      for (polygon_iterator_type p = polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (); p != polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end (); ++p) {
        polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      for (polygon_iterator_wp_type p = polygons.get_layer<db::PolygonWithProperties, db::unstable_layer_tag> ().begin (); p != polygons.get_layer<db::PolygonWithProperties, db::unstable_layer_tag> ().end (); ++p) {
        polygons.get_layer<db::PolygonWithProperties, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }
};

}

#endif

