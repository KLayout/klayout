
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


#ifndef HDR_dbHierNetworkProcessor
#define HDR_dbHierNetworkProcessor

#include "dbCommon.h"

#include <map>
#include <set>

namespace db {

/**
 *  @brief Defines the connectivity
 *
 *  Connectivity is defined in terms of layers. Certain layer pairs
 *  are connected when shapes on their layers interact.
 *  Connectivity includes intra-layer connectivity - i.e.
 *  shapes on a layer are not connected by default. They need to
 *  be connected explicitly using "connect(layer)".
 */
class DB_PUBLIC Connectivity
{
public:
  typedef std::set<unsigned int> layers_type;
  typedef layers_type::const_iterator layer_iterator;

  /**
   *  @brief Creates a connectivity object without any connections
   */
  Connectivity ();

  /**
   *  @brief Adds inter-layer connectivity
   */
  void connect (unsigned int la, unsigned int lb);

  /**
   *  @brief Adds intra-layer connectivity for layer l
   */
  void connect (unsigned int l);

  /**
   *  @brief Begin iterator for the layers involved
   */
  layer_iterator begin_layers ();

  /**
   *  @brief End iterator for the layers involved
   */
  layer_iterator end_layers ();

  /**
   *  @brief Begin iterator for the layers connected to a specific layer
   */
  layer_iterator begin_connected (unsigned int layer);

  /**
   *  @brief End iterator for the layers connected to a specific layer
   */
  layer_iterator end_connected (unsigned int layer);

  /**
   *  @brief Returns true, if the given shapes on the given layers interact
   */
  template <class T>
  bool interacts (const T &a, unsigned int la, T &b, unsigned int lb) const;

private:
  layers_type m_all_layers;
  std::map<unsigned int, layers_type> m_connected;
};

}

#endif
