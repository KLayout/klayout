
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


#ifndef HDR_dbOriginalLayerEdges
#define HDR_dbOriginalLayerEdges

#include "dbCommon.h"

#include "dbAsIfFlatEdges.h"
#include "dbShapes.h"
#include "dbRecursiveShapeIterator.h"

namespace db {

/**
 *  @brief An original layerregion based on a RecursiveShapeIterator
 */
class DB_PUBLIC OriginalLayerEdges
  : public AsIfFlatEdges
{
public:
  OriginalLayerEdges ();
  OriginalLayerEdges (const OriginalLayerEdges &other);
  OriginalLayerEdges (const RecursiveShapeIterator &si, bool is_merged = false);
  OriginalLayerEdges (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics, bool is_merged = false);
  virtual ~OriginalLayerEdges ();

  EdgesDelegate *clone () const;

  virtual EdgesIteratorDelegate *begin () const;
  virtual EdgesIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;

  virtual bool is_merged () const;

  virtual const db::Edge *nth (size_t n) const;
  virtual bool has_valid_edges () const;
  virtual bool has_valid_merged_edges () const;

  virtual const db::RecursiveShapeIterator *iter () const;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

protected:
  virtual void merged_semantics_changed ();

private:
  OriginalLayerEdges &operator= (const OriginalLayerEdges &other);

  bool m_is_merged;
  mutable db::Shapes m_merged_edges;
  mutable bool m_merged_edges_valid;
  mutable db::RecursiveShapeIterator m_iter;
  db::ICplxTrans m_iter_trans;

  void init ();
  void ensure_merged_edges_valid () const;
};

}

#endif

