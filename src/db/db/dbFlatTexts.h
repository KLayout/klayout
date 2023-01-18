
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


#ifndef HDR_dbFlatTexts
#define HDR_dbFlatTexts

#include "dbCommon.h"

#include "dbMutableTexts.h"
#include "dbShapes.h"
#include "tlCopyOnWrite.h"

namespace db {

/**
 *  @brief An iterator delegate for the flat text set
 */
typedef generic_shapes_iterator_delegate<db::Text> FlatTextsIterator;

/**
 *  @brief The delegate for the actual text set implementation
 */
class DB_PUBLIC FlatTexts
  : public MutableTexts
{
public:
  typedef db::Text value_type;

  typedef db::layer<db::Text, db::unstable_layer_tag> text_layer_type;
  typedef text_layer_type::iterator text_iterator_type;

  FlatTexts ();
  FlatTexts (const db::Shapes &texts);

  FlatTexts (const FlatTexts &other);

  virtual ~FlatTexts ();

  TextsDelegate *clone () const
  {
    return new FlatTexts (*this);
  }

  void reserve (size_t);

  virtual TextsIteratorDelegate *begin () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;

  virtual bool empty () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;

  virtual TextsDelegate *filter_in_place (const TextFilterBase &filter);

  virtual TextsDelegate *add_in_place (const Texts &other);
  virtual TextsDelegate *add (const Texts &other) const;

  virtual const db::Text *nth (size_t n) const;
  virtual bool has_valid_texts () const;

  virtual const db::RecursiveShapeIterator *iter () const;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const;

  virtual void flatten () { }

  void do_insert (const db::Text &text);

  virtual void do_transform (const db::Trans &t)
  {
    transform_generic (t);
  }

  virtual void do_transform (const db::ICplxTrans &t)
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

  db::Shapes &raw_texts () { return *mp_texts; }
  const db::Shapes &raw_texts () const { return *mp_texts; }

protected:
  virtual Box compute_bbox () const;
  void invalidate_cache ();

private:
  friend class AsIfFlatTexts;

  FlatTexts &operator= (const FlatTexts &other);

  mutable tl::copy_on_write_ptr<db::Shapes> mp_texts;
  mutable tl::copy_on_write_ptr<db::PropertiesRepository> mp_properties_repository;

  template <class Trans>
  void transform_generic (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      db::Shapes &texts = *mp_texts;
      for (text_iterator_type p = texts.template get_layer<db::Text, db::unstable_layer_tag> ().begin (); p != texts.template get_layer<db::Text, db::unstable_layer_tag> ().end (); ++p) {
        texts.get_layer<db::Text, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }
};

}

#endif

