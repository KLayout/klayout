
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


#ifndef HDR_dbTextsDelegate
#define HDR_dbTextsDelegate

#include "dbCommon.h"
#include "dbShapeCollection.h"
#include "dbShapeCollectionUtils.h"
#include "dbGenericShapeIterator.h"
#include "dbText.h"

namespace db {

class RecursiveShapeIterator;
class Texts;
class Region;
class TextFilterBase;
class RegionDelegate;
class EdgesDelegate;
class Layout;

typedef shape_collection_processor<db::Text, db::Polygon> TextToPolygonProcessorBase;

typedef db::generic_shape_iterator_delegate_base <db::Text> TextsIteratorDelegate;

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC TextsDelegate
  : public ShapeCollectionDelegateBase
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Text edge_pair_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;

  TextsDelegate ();
  virtual ~TextsDelegate ();

  TextsDelegate (const TextsDelegate &other);
  TextsDelegate &operator= (const TextsDelegate &other);

  virtual TextsDelegate *clone () const = 0;

  TextsDelegate *remove_properties (bool remove = true)
  {
    ShapeCollectionDelegateBase::remove_properties (remove);
    return this;
  }

  void enable_progress (const std::string &progress_desc);
  void disable_progress ();

  //  dummy features to harmonize the interface of region, edges and edge pair delegates
  void set_merged_semantics (bool) { }
  bool merged_semantics () const { return false; }
  void set_is_merged (bool) { }
  bool is_merged () const { return false; }

  virtual std::string to_string (size_t nmax) const = 0;

  virtual TextsIteratorDelegate *begin () const = 0;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const = 0;

  virtual bool empty () const = 0;
  virtual size_t count () const = 0;
  virtual size_t hier_count () const = 0;

  virtual Box bbox () const = 0;

  virtual TextsDelegate *filter_in_place (const TextFilterBase &filter) = 0;
  virtual TextsDelegate *filtered (const TextFilterBase &filter) const = 0;
  virtual RegionDelegate *processed_to_polygons (const TextToPolygonProcessorBase &filter) const = 0;

  virtual RegionDelegate *polygons (db::Coord e) const = 0;
  virtual EdgesDelegate *edges () const = 0;

  virtual TextsDelegate *add_in_place (const Texts &other) = 0;
  virtual TextsDelegate *add (const Texts &other) const = 0;

  virtual TextsDelegate *in (const Texts &other, bool invert) const = 0;

  virtual const db::Text *nth (size_t n) const = 0;
  virtual bool has_valid_texts () const = 0;

  virtual const db::RecursiveShapeIterator *iter () const = 0;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt) = 0;
  virtual db::PropertiesRepository *properties_repository () = 0;
  virtual const db::PropertiesRepository *properties_repository () const = 0;

  virtual bool equals (const Texts &other) const = 0;
  virtual bool less (const Texts &other) const = 0;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const = 0;
  virtual void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const = 0;

  virtual RegionDelegate *pull_interacting (const Region &) const = 0;
  virtual TextsDelegate *selected_interacting (const Region &other) const = 0;
  virtual TextsDelegate *selected_not_interacting (const Region &other) const = 0;

protected:
  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

private:
  bool m_report_progress;
  std::string m_progress_desc;
};

}

#endif

