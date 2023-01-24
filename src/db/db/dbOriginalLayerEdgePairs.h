
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


#ifndef HDR_dbOriginalLayerEdgePairs
#define HDR_dbOriginalLayerEdgePairs

#include "dbCommon.h"

#include "dbAsIfFlatEdgePairs.h"
#include "dbShapes.h"
#include "dbRecursiveShapeIterator.h"

namespace db {

/**
 *  @brief An original layerregion based on a RecursiveShapeIterator
 */
class DB_PUBLIC OriginalLayerEdgePairs
  : public AsIfFlatEdgePairs
{
public:
  OriginalLayerEdgePairs ();
  OriginalLayerEdgePairs (const OriginalLayerEdgePairs &other);
  OriginalLayerEdgePairs (const RecursiveShapeIterator &si);
  OriginalLayerEdgePairs (const RecursiveShapeIterator &si, const db::ICplxTrans &trans);
  virtual ~OriginalLayerEdgePairs ();

  EdgePairsDelegate *clone () const;

  virtual EdgePairsIteratorDelegate *begin () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;

  virtual bool empty () const;

  virtual const db::EdgePair *nth (size_t n) const;
  virtual bool has_valid_edge_pairs () const;

  virtual const db::RecursiveShapeIterator *iter () const;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  virtual bool equals (const EdgePairs &other) const;
  virtual bool less (const EdgePairs &other) const;

private:
  OriginalLayerEdgePairs &operator= (const OriginalLayerEdgePairs &other);

  mutable db::RecursiveShapeIterator m_iter;
  db::ICplxTrans m_iter_trans;

  void init ();
};

}

#endif

