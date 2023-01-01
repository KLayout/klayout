
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

#ifndef HDR_dbDXFFormat
#define HDR_dbDXFFormat

#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"
#include "dbPluginCommon.h"

namespace db
{

/**
 *  @brief Structure that holds the DXF specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC DXFReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  DXFReaderOptions ()
    : dbu (0.001),
      unit (1.0),
      text_scaling (100.0),
      polyline_mode (0),
      circle_points (100),
      circle_accuracy (0.0),
      contour_accuracy (0.0),
      render_texts_as_polygons (false),
      keep_other_cells (false),
      create_other_layers (true),
      keep_layer_names (false)
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
   *  @brief Specify the unit of the DXF file
   *
   *  Since DXF is unitless, this value allows one to specify the units of the DXF file given as input.
   */
  double unit;

  /**
   *  @brief Text scaling factor
   *
   *  This value specifies text scaling in percent. A value of 100 roughly means that the letter 
   *  pitch of the font will be 92% of the specified text height. That value applies for ROMANS fonts.
   *  When generating GDS texts, a value of 100 generates TEXT objects with 
   *  the specified size. Smaller values generate smaller sizes.
   */
  double text_scaling;

  /**
   *  @brief POLYLINE/LWPOLYLINE mode
   *
   *  0: automatic mode
   *  1: keep lines
   *  2: create polygons from closed POLYLINE/LWPOLYLINE with width == 0
   *  3: merge all lines (width width 0)
   *  4: as 3 and auto-close contours
   */
  int polyline_mode;

  /**
   *  @brief Number of points for a full circle for arc interpolation
   *
   *  See circle_accuracy for another way of specifying the number of
   *  points per circle.
   */
  int circle_points;

  /**
   *  @brief Accuracy of circle approximation
   *
   *  This value specifies the approximation accuracy of the circle and other
   *  "round" structures. If this value is a positive number bigger than the
   *  database unit (see dbu), it will control the number of points the
   *  circle is resolved into. The number of points will be chosen such that
   *  the deviation from the ideal curve is less than this value.
   *
   *  The actual number of points used for the circle approximation is
   *  not larger than circle_points.
   *
   *  The value is given in the units of the DXF file.
   */
  double circle_accuracy;

  /**
   *  @brief Accuracy for closing polylines
   *
   *  When polylines need to be connected or closed, this
   *  value is used to indicate the accuracy. This is the value (in DXF units)
   *  by which points may be separated and still be considered
   *  connected. The default is 0.0 which implies exact
   *  (within one DBU) closing.
   */
  double contour_accuracy;

  /**
   *  @brief If set to true, converts texts to polygons on read
   * 
   *  Converting texts avoids problems with UTF-8 character sets.
   */
  bool render_texts_as_polygons;

  /**
   *  @brief If set to true, cells other than the top cell are kept instead of removed
   */
  bool keep_other_cells;

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
    return new DXFReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("DXF");
    return n;
  }
};

/**
 *  @brief Structure that holds the DXF specific options for the Writer
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC DXFWriterOptions
  : public FormatSpecificWriterOptions
{
public:
  /**
   *  @brief The constructor
   */
  DXFWriterOptions ()
    : polygon_mode (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Polygon mode
   *
   *  0: create POLYLINE
   *  1: create LWPOLYLINE
   *  2: decompose into SOLID
   *  3: create HATCH
   *  4: create LINE: refer to 'void DXFWriter::write_polygon()' definition
   */
  int polygon_mode;

  /** 
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual FormatSpecificWriterOptions *clone () const
  {
    return new DXFWriterOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual const std::string &format_name () const
  {
    static std::string n ("DXF");
    return n;
  }
};

} // namespace db

#endif

