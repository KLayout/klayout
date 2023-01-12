
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

#ifndef HDR_dbNamedLayerReader
#define HDR_dbNamedLayerReader

#include "dbCommon.h"
#include "dbReader.h"

namespace db
{

class Layout;

/**
 *  @brief A reader base class for streams with named-only layers
 *
 *  This class implements the layer name translation logic.
 *  Specifically:
 *    - a number is translated to the corresponding layer, datatype 0
 *    - Lx            is translated to layer x, datatype 0
 *    - Lx_SUFFIX     is translated to layer x, datatype 0, name "SUFFIX"
 *    - LxDy          is translated to layer x, datatype y
 *    - LxDy_SUFFIX   is translated to layer x, datatype y, name "SUFFIX"
 *
 *  Furthermore, the layer map and creation of new layers is handled in this
 *  base class.
 */
class DB_PUBLIC NamedLayerReader
  : public ReaderBase
{
public:
  /**
   *  @brief The constructor
   */
  NamedLayerReader ();

protected:
  /**
   *  @brief Sets a value indicating whether to create new layers
   */
  void set_create_layers (bool f);

  /**
   *  @brief Gets a value indicating whether to create new layers
   */
  bool create_layers () const
  {
    return m_create_layers;
  }

  /**
   *  @brief Sets the layer map
   */
  void set_layer_map (const LayerMap &lm);

  /**
   *  @brief Gets the input layer map
   */
  const LayerMap &layer_map ()
  {
    return m_layer_map;
  }

  /**
   *  @brief Gets the layer map
   */
  const LayerMap &layer_map_out ()
  {
    return m_layer_map_out;
  }

  /**
   *  @brief Sets a value indicating whether layer names are kept
   *  If set to true, no name translation is performed and layers are
   *  always named only. If set the false (the default), layer names will
   *  be translated to GDS layer/datatypes if possible.
   */
  void set_keep_layer_names (bool f);

  /**
   *  @brief Gets a value indicating whether layer names are kept
   */
  bool keep_layer_names () const
  {
    return m_keep_layer_names;
  }

  /**
   *  @brief Opens a new layer
   *  This method will create or locate a layer for a given name.
   *  The result's first attribute is true, if such a layer could be found
   *  or created. In this case, the second attribute is the layer index.
   */
  std::pair <bool, unsigned int> open_layer (db::Layout &layout, const std::string &name);

  /**
   *  @brief Opens a new layer
   *  This method will create or locate a layer for a given name.
   *  The result's first attribute is true, if such a layer could be found
   *  or created. In this case, the second attribute is the layer index.
   */
  std::pair <bool, unsigned int> open_layer (db::Layout &layout, const std::string &name, bool keep_layer_name, bool create_layer);

  /**
   *  @brief Force mapping of a name to a layer index
   */
  void map_layer (const std::string &name, unsigned int layer);

  /**
   *  @brief Finish reading
   *  This method must be called after the reading has been done.
   *  It will finalize the layers.
   */
  void finish_layers (db::Layout &layout);

  /**
   *  @brief Prepares reading
   *  This method must be called before the reading is done.
   */
  void prepare_layers (db::Layout &layout);

private:
  bool m_create_layers;
  bool m_keep_layer_names;
  LayerMap m_layer_map;
  unsigned int m_next_layer_index;
  std::map <std::string, unsigned int> m_new_layers;
  db::LayerMap m_layer_map_out;
  std::map<std::string, std::pair <bool, unsigned int> > m_layer_cache;
  std::map<std::set<unsigned int>, unsigned int> m_multi_mapping_placeholders;

  std::pair <bool, unsigned int> open_layer_uncached (db::Layout &layout, const std::string &name, bool keep_layer_name, bool create_layer);
};

}

#endif
