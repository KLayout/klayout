
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

#ifndef HDR_bdWriterOptions
#define HDR_bdWriterOptions

#include "bdCommon.h"

#include <string>

namespace tl
{
  class CommandLineOptions;
}

namespace db
{
  class Layout;
  class SaveLayoutOptions;
}

namespace bd
{

/**
 *  @brief Generic writer options
 *  This class collects generic writer options and provides command line options for this
 */
class BD_PUBLIC GenericWriterOptions
{
public:
  /**
   *  @brief Constructor
   */
  GenericWriterOptions ();

  /**
   *  @brief Adds the generic options to the command line parser object
   *  The format string gives a hint about the target format. Certain options will be
   *  suppressed if they are known to be unavailable for the given format.
   */
  void add_options (tl::CommandLineOptions &cmd, const std::string &format = std::string ());

  /**
   *  @brief Adds the generic options to the command line parser object for the GDS2 format
   */
  void add_options_for_gds2 (tl::CommandLineOptions &cmd)
  {
    add_options (cmd, gds2_format_name);
  }

  /**
   *  @brief Adds the generic options to the command line parser object for the OASIS format
   */
  void add_options_for_oasis (tl::CommandLineOptions &cmd)
  {
    add_options (cmd, oasis_format_name);
  }

  /**
   *  @brief Adds the generic options to the command line parser object for the CIF format
   */
  void add_options_for_cif (tl::CommandLineOptions &cmd)
  {
    add_options (cmd, cif_format_name);
  }

  /**
   *  @brief Adds the generic options to the command line parser object for the DXF format
   */
  void add_options_for_dxf (tl::CommandLineOptions &cmd)
  {
    add_options (cmd, dxf_format_name);
  }

  /**
   *  @brief Configures the writer options object with the options stored in this object
   *  The layout is required in order to derive the cell and layer ID's.
   */
  void configure (db::SaveLayoutOptions &save_options, const db::Layout &layout) const;

  static const std::string gds2_format_name;
  static const std::string gds2text_format_name;
  static const std::string oasis_format_name;
  static const std::string cif_format_name;
  static const std::string dxf_format_name;
  static const std::string mag_format_name;

private:
  double m_scale_factor;
  double m_dbu;
  bool m_dont_write_empty_cells;
  bool m_keep_instances;
  bool m_write_context_info;
  std::string m_cell_selection;

  unsigned int m_gds2_max_vertex_count;
  bool m_gds2_no_zero_length_paths;
  bool m_gds2_multi_xy_records;
  bool m_gds2_resolve_skew_arrays;
  unsigned int m_gds2_max_cellname_length;
  std::string m_gds2_libname;
  double m_gds2_user_units;
  bool m_gds2_write_timestamps;
  bool m_gds2_write_cell_properties;
  bool m_gds2_write_file_properties;
  double m_gds2_default_text_size;

  int m_oasis_compression_level;
  bool m_oasis_write_cblocks;
  bool m_oasis_strict_mode;
  bool m_oasis_recompress;
  bool m_oasis_permissive;
  int m_oasis_write_std_properties;
  std::string m_oasis_subst_char;

  bool m_cif_dummy_calls;
  bool m_cif_blank_separator;

  double m_magic_lambda;
  std::string m_magic_tech;

  int m_dxf_polygon_mode;

  void set_oasis_substitution_char (const std::string &text);
};

}

#endif
