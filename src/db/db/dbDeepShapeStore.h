
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


#ifndef HDR_dbDeepShapeStore
#define HDR_dbDeepShapeStore

#include "dbCommon.h"

#include "tlObject.h"
#include "dbLayout.h"
#include "dbRecursiveShapeIterator.h"

namespace db {

class DeepShapeStore;

/**
 *  @brief Represents a shape collection from the deep shape store
 *
 *  This is a lightweight class pointing into the deep shape store.
 *  DeepLayer objects are issued by the DeepShapeStore class.
 */
class DB_PUBLIC DeepLayer
{
public:
  /**
   *  @brief Destructor
   */
  ~DeepLayer ();

  /**
   *  @brief Copy constructor
   */
  DeepLayer (const DeepLayer &other);

  /**
   *  @brief Assignment
   */
  DeepLayer &operator= (const DeepLayer &other);

  /**
   *  @brief Gets the layout object
   *
   *  The return value is guaranteed to be non-null.
   */
  db::Layout *layout ();

  /**
   *  @brief Gets the layer
   */
  unsigned int layer () const
  {
    return m_layer;
  }

private:
  friend class DeepShapeStore;

  /**
   *  @brief The constructor
   */
  DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer);

  unsigned int layout () const { return m_layout; }
  unsigned int layer () const { return m_layer; }

  tl::weak_ptr<DeepShapeStore> mp_store;
  unsigned int m_layout;
  unsigned int m_layer;
};

/**
 *  @brief The "deep shape store" is a working model for the hierarchical ("deep") processor
 *
 *  The deep shape store keep temporary data for the deep shape processor.
 *  It mainly consists of layout objects holding the hierarchy trees and layers
 *  for the actual shapes.
 *
 *  The deep shape store provides the basis for working with deep regions. On preparation,
 *  shapes are copied into the deep shape store. After fininishing, the shapes are copied
 *  back into the original layout. The deep shape store provides the methods and
 *  algorithms for doing the preparation and transfer.
 */
class DB_PUBLIC DeepShapeStore
  : public tl::Object
{
public:
  /**
   *  @brief The default constructor
   */
  DeepShapeStore ();

private:
  //  no copying
  DeepShapeStore (const DeepShapeStore &);
  DeepShapeStore &operator= (const DeepShapeStore &);
};

}

#endif

