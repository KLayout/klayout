
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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
#include "dbGDS2WriterBase.h"
#include "dbOASISWriter.h"
#include "dbDXFWriter.h"
#include "dbCIFWriter.h"

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
    add_options (cmd, m_gds2_writer_options.format_name ());
  }

  /**
   *  @brief Adds the generic options to the command line parser object for the OASIS format
   */
  void add_options_for_oasis (tl::CommandLineOptions &cmd)
  {
    add_options (cmd, m_oasis_writer_options.format_name ());
  }

  /**
   *  @brief Adds the generic options to the command line parser object for the CIF format
   */
  void add_options_for_cif (tl::CommandLineOptions &cmd)
  {
    add_options (cmd, m_cif_writer_options.format_name ());
  }

  /**
   *  @brief Adds the generic options to the command line parser object for the DXF format
   */
  void add_options_for_dxf (tl::CommandLineOptions &cmd)
  {
    add_options (cmd, m_dxf_writer_options.format_name ());
  }

  /**
   *  @brief Configures the writer options object with the options stored in this object
   *  The layout is required in order to derive the cell and layer ID's.
   */
  void configure (db::SaveLayoutOptions &save_options, const db::Layout &layout) const;

private:
  double m_scale_factor;
  double m_dbu;
  bool m_dont_write_empty_cells;
  bool m_keep_instances;
  bool m_write_context_info;
  std::string m_cell_selection;
  db::GDS2WriterOptions m_gds2_writer_options;
  db::OASISWriterOptions m_oasis_writer_options;
  db::CIFWriterOptions m_cif_writer_options;
  db::DXFWriterOptions m_dxf_writer_options;

  void set_oasis_substitution_char (const std::string &text);
};

}

#endif
