
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

#ifndef HDR_bdReaderOptions
#define HDR_bdReaderOptions

#include "bdCommon.h"
#include "dbGDS2Reader.h"
#include "dbCommonReader.h"
#include "dbOASISReader.h"
#include "dbDXFReader.h"
#include "dbCIFReader.h"

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
  db::LayerMap m_layer_map;
  bool m_create_other_layers;
  db::CommonReaderOptions m_common_reader_options;
  db::GDS2ReaderOptions m_gds2_reader_options;
  db::OASISReaderOptions m_oasis_reader_options;
  db::CIFReaderOptions m_cif_reader_options;
  db::DXFReaderOptions m_dxf_reader_options;

  void set_layer_map (const std::string &lm);
  void set_dbu (double dbu);
};

}

#endif
