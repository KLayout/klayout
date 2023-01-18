
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


#ifndef HDR_dbDeepTexts
#define HDR_dbDeepTexts

#include "dbCommon.h"

#include "dbMutableTexts.h"
#include "dbDeepShapeStore.h"
#include "dbTexts.h"

namespace db {

/**
 *  @brief Provides hierarchical edges implementation
 */
class DB_PUBLIC DeepTexts
  : public db::MutableTexts, public db::DeepShapeCollectionDelegateBase
{
public:
  DeepTexts ();
  DeepTexts (const db::Texts &other, DeepShapeStore &dss);
  DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss);
  DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans);

  DeepTexts (const DeepTexts &other);
  DeepTexts (const DeepLayer &dl);

  virtual ~DeepTexts ();

  TextsDelegate *clone () const;

  virtual void do_insert (const db::Text &text);

  virtual void do_transform (const db::Trans &t);
  virtual void do_transform (const db::ICplxTrans &t);
  virtual void do_transform (const db::IMatrix2d &t);
  virtual void do_transform (const db::IMatrix3d &t);

  virtual void flatten ();

  virtual void reserve (size_t n);

  virtual TextsIteratorDelegate *begin () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual std::string to_string (size_t) const;
  virtual Box bbox () const;
  virtual bool empty () const;
  virtual const db::Text *nth (size_t n) const;
  virtual bool has_valid_texts () const;
  virtual const db::RecursiveShapeIterator *iter () const;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  virtual TextsDelegate *filter_in_place (const TextFilterBase &filter);
  virtual TextsDelegate *filtered (const TextFilterBase &) const;

  virtual RegionDelegate *processed_to_polygons (const TextToPolygonProcessorBase &filter) const;

  virtual TextsDelegate *add_in_place (const Texts &other);
  virtual TextsDelegate *add (const Texts &other) const;

  virtual RegionDelegate *polygons (db::Coord e) const;
  virtual EdgesDelegate *edges () const;

  virtual TextsDelegate *in (const Texts &, bool) const;

  virtual bool equals (const Texts &other) const;
  virtual bool less (const Texts &other) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const;

  virtual DeepShapeCollectionDelegateBase *deep ()
  {
    return this;
  }

private:
  DeepTexts &operator= (const DeepTexts &other);

  void init ();
  DeepTexts *apply_filter (const TextFilterBase &filter) const;

  virtual TextsDelegate *selected_interacting_generic (const Region &other, bool inverse) const;
  virtual RegionDelegate *pull_generic (const Region &other) const;
};

}

#endif

