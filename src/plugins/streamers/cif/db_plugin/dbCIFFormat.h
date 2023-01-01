
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

#ifndef HDR_dbCIFFormat
#define HDR_dbCIFFormat

#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"
#include "dbPluginCommon.h"

namespace db
{

/**
 *  @brief Structure that holds the CIF specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC CIFReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  CIFReaderOptions ()
    : wire_mode (0),
      dbu (0.001),
      create_other_layers (true),
      keep_layer_names (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief How to read 'W' objects
   *
   *  This property specifies how to read 'W' (wire) objects.
   *  Allowed values are 0 (as square ended paths), 1 (as flush ended paths), 2 (as round paths)
   */
  unsigned int wire_mode;

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
   *  @brief A flag indicating whether the names of layers shall be kept as such
   *
   *  If this flag is set to false (the default), layer name translation happens.
   *  If set to true, translation will not happen.
   *  Name translation will try to extract GDS layer/datatype numbers from the
   *  layer names. If this value is set to true, no name translation happens.
   */
  bool keep_layer_names;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new CIFReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("CIF");
    return n;
  }
};

/**
 *  @brief Structure that holds the CIF specific options for the Writer
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC CIFWriterOptions
  : public FormatSpecificWriterOptions
{
public:
  /**
   *  @brief The constructor
   */
  CIFWriterOptions ()
    : dummy_calls (false), blank_separator (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief A flag indicating whether dummy calls shall be written
   *  If this flag is true, the writer will produce dummy cell calls on global
   *  level for all top cells.
   */
  bool dummy_calls;

  /**
   *  @brief A flag indicating whether to use blanks as x/y separators
   *  If this flag is true, blank characters will be used to separate x and y values.
   *  Otherwise comma characters will be used.
   */
  bool blank_separator;

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual FormatSpecificWriterOptions *clone () const
  {
    return new CIFWriterOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual const std::string &format_name () const
  {
    static std::string n ("CIF");
    return n;
  }
};

}

#endif

