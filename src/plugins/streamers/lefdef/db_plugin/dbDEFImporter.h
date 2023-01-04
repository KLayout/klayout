
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



#ifndef HDR_dbDEFImporter
#define HDR_dbDEFImporter

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "tlStream.h"
#include "dbLEFImporter.h"

#include <vector>
#include <string>

namespace db
{

struct DEFImporterGroup;

/**
 *  @brief The DEF importer object
 */
class DB_PLUGIN_PUBLIC DEFImporter
  : public LEFDEFImporter 
{
public:
  /**
   *  @brief Default constructor
   */
  DEFImporter (int warn_level = 1);

  /**
   *  @brief Read the given LEF file prior to the DEF file
   *
   *  This method reads the layout specified into the given layout.
   *  Multiple LEF files can be read.
   */
  void read_lef (tl::InputStream &stream, db::Layout &layout, LEFDEFReaderState &state);

  /**
   *  @brief Provided for test purposes
   */
  void finish_lef (Layout &layout);

protected:
  void do_read (db::Layout &layout);

private:
  LEFImporter m_lef_importer;
  std::map<std::string, std::map<std::string, db::Coord> > m_nondefault_widths;
  std::map<std::string, ViaDesc> m_via_desc;
  std::map<int, db::Polygon> m_styles;
  std::vector<std::string> m_component_maskshift;

  void read_polygon (db::Polygon &poly, double scale);
  void read_rect (db::Polygon &poly, double scale);
  std::pair<Coord, Coord> get_wire_width_for_rule(const std::string &rule, const std::string &ln, double dbu);
  std::pair<db::Coord, db::Coord> get_def_ext (const std::string &ln, const std::pair<db::Coord, db::Coord> &wxy, double dbu);
  void read_diearea (db::Layout &layout, db::Cell &design, double scale);
  void read_nondefaultrules (double scale);
  void read_regions (std::map<std::string, std::vector<std::pair<LayerPurpose, std::vector<db::Polygon> > > > &regions, double scale);
  void read_groups (std::list<DEFImporterGroup> &groups, double scale);
  void read_blockages (db::Layout &layout, db::Cell &design, double scale);
  void read_nets (db::Layout &layout, db::Cell &design, double scale, bool specialnets);
  void read_vias (db::Layout &layout, db::Cell &design, double scale);
  void read_pins (db::Layout &layout, db::Cell &design, double scale);
  void read_fills (db::Layout &layout, db::Cell &design, double scale);
  void read_styles (double scale);
  void read_components (Layout &layout, std::list<std::pair<std::string, db::CellInstArray> > &instances, double scale);
  void read_single_net (std::string &nondefaultrule, db::Layout &layout, db::Cell &design, double scale, properties_id_type prop_id, bool specialnets);
  void produce_routing_geometry (db::Cell &design, const db::Polygon *style, unsigned int layer, properties_id_type prop_id, const std::vector<db::Point> &pts, const std::vector<std::pair<db::Coord, db::Coord> > &ext, std::pair<db::Coord, db::Coord> w);
};

}

#endif

