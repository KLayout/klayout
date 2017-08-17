
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
  void configure (db::LoadLayoutOptions &load_options);

private:
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
