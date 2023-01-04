
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

#ifndef HDR_dbNetlistReader
#define HDR_dbNetlistReader

#include "dbCommon.h"
#include "tlTypeTraits.h"

#include <string>

namespace tl
{
  class InputStream;
}

namespace db
{

class Netlist;

/**
 *  @brief A common base class for netlist writers
 */
class DB_PUBLIC NetlistReader
{
public:
  NetlistReader () { }
  virtual ~NetlistReader () { }

  virtual void read (tl::InputStream &stream, db::Netlist &netlist) = 0;
};

}

#endif
