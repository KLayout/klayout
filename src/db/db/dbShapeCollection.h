
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

#ifndef _HDR_dbShapeCollection
#define _HDR_dbShapeCollection

#include "dbCommon.h"
#include "dbDeepShapeStore.h"
#include "tlUniqueId.h"
#include "tlVariant.h"
#include "gsiObject.h"

namespace db
{

class PropertiesTranslator;
class PropertiesRepository;

/**
 *  @brief A base class for the deep collection delegates
 */
class DB_PUBLIC DeepShapeCollectionDelegateBase
{
public:
  DeepShapeCollectionDelegateBase ();
  DeepShapeCollectionDelegateBase (const DeepShapeCollectionDelegateBase &other);

  DeepShapeCollectionDelegateBase &operator= (const DeepShapeCollectionDelegateBase &other);

  const db::DeepLayer &deep_layer () const
  {
    return m_deep_layer;
  }

  db::DeepLayer &deep_layer ()
  {
    return m_deep_layer;
  }

  void apply_property_translator (const db::PropertiesTranslator &pt);

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

  virtual void apply_property_translator (const db::PropertiesTranslator & /*pt*/) = 0;
  virtual db::PropertiesRepository *properties_repository () = 0;
  virtual const db::PropertiesRepository *properties_repository () const = 0;

  void remove_properties (bool remove = true)
  {
    if (remove) {
      apply_property_translator (db::PropertiesTranslator::make_remove_all ());
    }
  }
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

  /**
   *  @brief Applies a PropertyTranslator
   *
   *  This method will translate the property IDs according to the given property translator.
   *
   *  Note that the property translator needs to be built from the PropertiesRepository
   *  delivered by "properties_repository".
   */
  void apply_property_translator (const db::PropertiesTranslator &pt);

  /**
   *  @brief Gets the property repository
   *
   *  Use this object to decode and encode property IDs.
   */
  db::PropertiesRepository &properties_repository ();

  /**
   *  @brief Gets the property repository (const version)
   *
   *  Use this object to decode property IDs.
   */
  const db::PropertiesRepository &properties_repository () const;

  /**
   *  @brief Gets a value indicating whether a properties repository is available
   */
  bool has_properties_repository () const;
};

}

#endif
