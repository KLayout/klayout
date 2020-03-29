
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
  DEFImporter ();

  /**
   *  @brief Read the given LEF file prior to the DEF file
   *
   *  This method reads the layout specified into the given layout.
   *  Multiple LEF files can be read.
   */
  void read_lef (tl::InputStream &stream, db::Layout &layout, LEFDEFLayerDelegate &ld);

protected:
  void do_read (db::Layout &layout);

private:
  LEFImporter m_lef_importer;
  std::map<std::string, std::map<std::string, double> > m_nondefault_widths;

  db::FTrans get_orient (bool optional);
  void read_polygon (db::Polygon &poly, double scale);
  void read_rect (db::Polygon &poly, double scale);
  db::Coord get_wire_width_for_rule(const std::string &rule, const std::string &ln, double dbu);
};

}

#endif

