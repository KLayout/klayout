
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


#ifndef HDR_dbEmptyTexts
#define HDR_dbEmptyTexts

#include "dbCommon.h"

#include "dbTextsDelegate.h"
#include "dbRecursiveShapeIterator.h"

namespace db {

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC EmptyTexts
  : public TextsDelegate
{
public:
  EmptyTexts ();
  EmptyTexts (const EmptyTexts &other);

  virtual TextsDelegate *clone () const;

  virtual std::string to_string (size_t) const { return std::string (); }

  virtual TextsIteratorDelegate *begin () const { return 0; }
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const { return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ()); }

  virtual bool empty () const { return true; }
  virtual size_t count () const { return 0; }
  virtual size_t hier_count () const { return 0; }

  virtual Box bbox () const { return Box (); }

  virtual TextsDelegate *filter_in_place (const TextFilterBase &) { return this; }
  virtual TextsDelegate *filtered (const TextFilterBase &) const { return new EmptyTexts (); }

  virtual RegionDelegate *processed_to_polygons (const TextToPolygonProcessorBase &) const;

  virtual RegionDelegate *polygons (db::Coord e) const;
  virtual EdgesDelegate *edges () const;

  virtual TextsDelegate *add_in_place (const Texts &other);
  virtual TextsDelegate *add (const Texts &other) const;

  virtual TextsDelegate *in (const Texts &, bool) const { return new EmptyTexts (); }

  virtual const db::Text *nth (size_t) const { tl_assert (false); }
  virtual bool has_valid_texts () const { return true; }

  virtual const db::RecursiveShapeIterator *iter () const { return 0; }
  virtual void apply_property_translator (const db::PropertiesTranslator &) { }
  virtual db::PropertiesRepository *properties_repository () { return 0; }
  virtual const db::PropertiesRepository *properties_repository () const { return 0; }

  virtual bool equals (const Texts &other) const;
  virtual bool less (const Texts &other) const;

  virtual void insert_into (Layout *, db::cell_index_type, unsigned int) const { }
  virtual void insert_into_as_polygons (Layout *, db::cell_index_type, unsigned int, db::Coord) const { }

  virtual RegionDelegate *pull_interacting (const Region &) const;
  virtual TextsDelegate *selected_interacting (const Region &) const;
  virtual TextsDelegate *selected_not_interacting (const Region &) const;

private:
  EmptyTexts &operator= (const EmptyTexts &other);
};

}

#endif

