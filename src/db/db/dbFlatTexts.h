
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


#ifndef HDR_dbFlatTexts
#define HDR_dbFlatTexts

#include "dbCommon.h"

#include "dbAsIfFlatTexts.h"
#include "dbShapes.h"

namespace db {

/**
 *  @brief An iterator delegate for the flat text set
 */
class DB_PUBLIC FlatTextsIterator
  : public TextsIteratorDelegate
{
public:
  typedef db::layer<db::Text, db::unstable_layer_tag> edge_pair_layer_type;
  typedef edge_pair_layer_type::iterator iterator_type;

  FlatTextsIterator (iterator_type from, iterator_type to)
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

  virtual TextsIteratorDelegate *clone () const
  {
    return new FlatTextsIterator (*this);
  }

private:
  friend class Texts;

  iterator_type m_from, m_to;
};

/**
 *  @brief The delegate for the actual text set implementation
 */
class DB_PUBLIC FlatTexts
  : public AsIfFlatTexts
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
  virtual size_t size () const;

  virtual TextsDelegate *filter_in_place (const TextFilterBase &filter);

  virtual TextsDelegate *add_in_place (const Texts &other);
  virtual TextsDelegate *add (const Texts &other) const;

  virtual const db::Text *nth (size_t n) const;
  virtual bool has_valid_texts () const;

  virtual const db::RecursiveShapeIterator *iter () const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const;

  void insert (const db::Text &text);
  void insert (const db::Shape &shape);

  template <class T>
  void insert (const db::Shape &shape, const T &trans)
  {
    if (shape.is_edge_pair ()) {

      db::Text t;
      shape.text (t);
      t.transform (trans);
      insert (t);

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
      for (text_iterator_type p = m_texts.template get_layer<db::Text, db::unstable_layer_tag> ().begin (); p != m_texts.template get_layer<db::Text, db::unstable_layer_tag> ().end (); ++p) {
        m_texts.get_layer<db::Text, db::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }

  db::Shapes &raw_texts () { return m_texts; }

protected:
  virtual Box compute_bbox () const;
  void invalidate_cache ();

private:
  friend class AsIfFlatTexts;

  FlatTexts &operator= (const FlatTexts &other);

  mutable db::Shapes m_texts;
};

}

#endif

