
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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



#ifndef HDR_dbCommonReader
#define HDR_dbCommonReader

#include "dbReader.h"


namespace db
{

/**
 *  @brief Structure that holds the GDS2 and OASIS specific options for the reader
 */
class DB_PUBLIC CommonReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  CommonReaderOptions ()
    : create_other_layers (true),
      enable_text_objects (true),
      enable_properties (true)
  {
    //  .. nothing yet ..
  }

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
   *  @brief A flag indicating whether to read text objects
   *
   *  If this flag is set to true, text objects are read. Otherwise they are ignored.
   */
  bool enable_text_objects;

  /**
   *  @brief A flag indicating whether to read user properties
   *
   *  If this flag is set to true, user properties are read. Otherwise they are ignored.
   */
  bool enable_properties;

  /** 
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new CommonReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("Common");
    return n;
  }
};

}

#endif

