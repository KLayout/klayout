
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_dbMALYFormat
#define HDR_dbMALYFormat

#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"
#include "dbPluginCommon.h"

namespace db
{

/**
 *  @brief Structure that holds the MALY specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC MALYReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  MALYReaderOptions ()
    : dbu (0.001),
      create_other_layers (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specify the database unit to produce
   *
   *  Specify the database unit which the resulting layout will receive.
   */
  double dbu;

  /**
   *  @brief Specifies a layer mapping
   *
   *  If a layer mapping is specified, only the given layers are read.
   *  Otherwise, all layers are read.
   *  Setting "create_other_layers" to true will make the reader
   *  create other layers for all layers not given in the layer map.
   *  Setting an empty layer map and create_other_layers to true effectively
   *  enables all layers for reading.
   */
  db::LayerMap layer_map;

  /**
   *  @brief A flag indicating that a new layers shall be created
   *
   *  If this flag is set to true, layers not listed in the layer map a created
   *  too.
   */
  bool create_other_layers;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new MALYReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("MALY");
    return n;
  }
};

}

#endif

