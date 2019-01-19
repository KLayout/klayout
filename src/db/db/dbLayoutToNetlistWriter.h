
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#ifndef HDR_dbLayoutToNetlistWriter
#define HDR_dbLayoutToNetlistWriter

#include "dbCommon.h"
#include "tlStream.h"

namespace db
{

class LayoutToNetlist;

/**
 *  @brief The base class for a LayoutToNetlist writer
 */
class DB_PUBLIC LayoutToNetlistWriterBase
{
public:
  LayoutToNetlistWriterBase () { }
  virtual ~LayoutToNetlistWriterBase () { }

  virtual void write (const db::LayoutToNetlist *l2n) = 0;
};

/**
 *  @brief The standard writer
 */
class DB_PUBLIC LayoutToNetlistStandardWriter
  : public LayoutToNetlistWriterBase
{
public:
  LayoutToNetlistStandardWriter (tl::OutputStream &stream, bool short_version);

  void write (const db::LayoutToNetlist *l2n);

private:
  tl::OutputStream *mp_stream;
  bool m_short_version;
};

}

#endif
