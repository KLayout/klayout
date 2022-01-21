
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

#ifndef _HDR_dbShapeCollection
#define _HDR_dbShapeCollection

#include "dbCommon.h"
#include "dbDeepShapeStore.h"
#include "tlUniqueId.h"
#include "gsiObject.h"

namespace db
{

/**
 *  @brief A base class for the deep collection delegates
 */
class DB_PUBLIC DeepShapeCollectionDelegateBase
{
public:
  DeepShapeCollectionDelegateBase () { }

  DeepShapeCollectionDelegateBase (const DeepShapeCollectionDelegateBase &other)
  {
    m_deep_layer = other.m_deep_layer.copy ();
  }

  DeepShapeCollectionDelegateBase &operator= (const DeepShapeCollectionDelegateBase &other)
  {
    if (this != &other) {
      m_deep_layer = other.m_deep_layer.copy ();
    }
    return *this;
  }

  const db::DeepLayer &deep_layer () const
  {
    return m_deep_layer;
  }

  db::DeepLayer &deep_layer ()
  {
    return m_deep_layer;
  }

protected:
  virtual void set_deep_layer (const db::DeepLayer &dl)
  {
    m_deep_layer = dl;
  }

private:
  db::DeepLayer m_deep_layer;
};

/**
 *  @brief A base class for the shape collection delegates
 */
class DB_PUBLIC ShapeCollectionDelegateBase
  : public tl::UniqueId
{
public:
  ShapeCollectionDelegateBase () { }
  virtual ~ShapeCollectionDelegateBase () { }

  virtual DeepShapeCollectionDelegateBase *deep () { return 0; }
};

/**
 *  @brief A base class for the shape collections such as Region, Edges, EdgePairs etc.
 */
class DB_PUBLIC ShapeCollection
  : public gsi::ObjectBase
{
public:
  ShapeCollection () { }
  virtual ~ShapeCollection () { }

  virtual ShapeCollectionDelegateBase *get_delegate () const = 0;
};

}

#endif
