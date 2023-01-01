
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

#ifndef HDR_dbMAGFormat
#define HDR_dbMAGFormat

#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"
#include "dbPluginCommon.h"

namespace db
{

/**
 *  @brief Structure that holds the MAG specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC MAGReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  MAGReaderOptions ()
    : lambda (1.0),
      dbu (0.001),
      create_other_layers (true),
      keep_layer_names (false),
      merge (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specifies the lambda value
   *
   *  The lambda value is the basic scaling parameter
   */
  double lambda;

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
   *  @brief A flag indicating whether to merge boxes into polygons
   *
   *  If this flag is set to true (the default), the boxes of the Magic
   *  layout files are merged into polygons.
   */
  bool merge;

  /**
   *  @brief The library paths
   *
   *  The library paths are the places where library references are looked up from.
   *  Expression interpolation happens inside these paths.
   *  "tech_dir", "tech_file", "tech_name" are variables by which you can refer to
   *  technology parameters. Relative paths will be resolved relative to the current
   *  file read.
   */
  std::vector<std::string> lib_paths;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new MAGReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("MAG");
    return n;
  }
};

/**
 *  @brief Structure that holds the MAG specific options for the Writer
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC MAGWriterOptions
  : public FormatSpecificWriterOptions
{
public:
  /**
   *  @brief The constructor
   */
  MAGWriterOptions ()
    : lambda (0.0), write_timestamp (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specifies the lambda value for writing
   *
   *  The lambda value is the basic scaling parameter.
   *  If this value is set to 0 or negative, the lambda value stored in the layout
   *  is used (meta data "lambda").
   */
  double lambda;

  /**
   *  @brief Specifies the technology value for writing Magic files
   *
   *  If this value is set an empty string, the technology store in the layout's
   *  "technology" meta data is used.
   */
  std::string tech;

  /**
   *  @brief A value indicating whether the real (true) or fake (false) timestamp is written
   *
   *  A fake, static timestamp is useful for comparing files.
   */
  bool write_timestamp;

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual FormatSpecificWriterOptions *clone () const
  {
    return new MAGWriterOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual const std::string &format_name () const
  {
    static std::string n ("MAG");
    return n;
  }
};

}

#endif

