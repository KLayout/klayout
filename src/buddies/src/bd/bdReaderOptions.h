
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

#ifndef HDR_bdReaderOptions
#define HDR_bdReaderOptions

#include "bdCommon.h"
#include "dbCommonReader.h"

#include <string>

namespace tl
{
  class CommandLineOptions;
}

namespace db
{
  class LoadLayoutOptions;
}

namespace bd
{

/**
 *  @brief Generic reader options
 *  This class collects generic reader options and provides command line options for this
 */
class BD_PUBLIC GenericReaderOptions
{
public:
  /**
   *  @brief Constructor
   */
  GenericReaderOptions ();

  /**
   *  @brief Adds the generic options to the command line parser object
   */
  void add_options (tl::CommandLineOptions &cmd);

  /**
   *  @brief Configures the reader options object with the options stored in this object
   */
  void configure (db::LoadLayoutOptions &load_options) const;

  /**
   *  @brief Sets the option prefix for the short option name
   *  By default, the prefix is set to "i", so the short options are
   *  called "-is", "-id" etc.
   */
  void set_prefix (const std::string &s)
  {
    m_prefix = s;
  }

  /**
   *  @brief Sets the option prefix for the long option name
   *  The prefix is prepended to the name, so with "a-", the long names
   *  are "--a-unit" etc. By default, this prefix is empty.
   */
  void set_long_prefix (const std::string &s)
  {
    m_long_prefix = s;
  }

  /**
   *  @brief Sets the group name prefix
   *  By default, this prefix is "Input", so the group names are
   *  "Input options - GDS2" for example.
   */
  void set_group_prefix (const std::string &s)
  {
    m_group_prefix = s;
  }

private:
  std::string m_prefix, m_long_prefix, m_group_prefix;

  //  generic
  db::LayerMap m_layer_map;
  bool m_create_other_layers;

  //  common GDS2+OASIS
  bool m_common_enable_text_objects;
  bool m_common_enable_properties;

  //  GDS2
  unsigned int m_gds2_box_mode;
  bool m_gds2_allow_big_records;
  bool m_gds2_allow_multi_xy_records;

  //  OASIS
  bool m_oasis_read_all_properties;
  int m_oasis_expect_strict_mode;

  //  CIF
  unsigned int m_cif_wire_mode;
  double m_cif_dbu;
  bool m_cif_keep_layer_names;

  //  DXF
  double m_dxf_dbu;
  double m_dxf_unit;
  double m_dxf_text_scaling;
  int m_dxf_polyline_mode;
  int m_dxf_circle_points;
  double m_dxf_circle_accuracy;
  double m_dxf_contour_accuracy;
  bool m_dxf_render_texts_as_polygons;
  bool m_dxf_keep_layer_names;
  bool m_dxf_keep_other_cells;

  //  MAGIC
  double m_magic_lambda;
  double m_magic_dbu;
  bool m_magic_keep_layer_names;
  bool m_magic_merge;
  std::vector<std::string> m_magic_lib_path;

  void set_layer_map (const std::string &lm);
  void set_dbu (double dbu);
  void set_read_named_layers (bool f);
};

}

#endif
