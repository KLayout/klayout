
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

#ifndef HDR_dbLayoutVsSchematicWriter
#define HDR_dbLayoutVsSchematicWriter

#include "dbCommon.h"
#include "dbLayoutToNetlistWriter.h"
#include "tlStream.h"

namespace db
{

class Circuit;
class Net;
class LayoutVsSchematic;
class NetlistCrossReference;

/**
 *  @brief The base class for a LayoutVsSchematic writer
 */
class DB_PUBLIC LayoutVsSchematicWriterBase
{
public:
  LayoutVsSchematicWriterBase ();
  virtual ~LayoutVsSchematicWriterBase ();

  void write (const db::LayoutVsSchematic *lvs);

protected:
  virtual void do_write_lvs (const db::LayoutVsSchematic *lvs) = 0;

private:
  std::string m_filename;
};

/**
 *  @brief The standard writer
 */
class DB_PUBLIC LayoutVsSchematicStandardWriter
  : public LayoutVsSchematicWriterBase
{
public:
  LayoutVsSchematicStandardWriter (tl::OutputStream &stream, bool short_version);

protected:
  void do_write_lvs (const db::LayoutVsSchematic *lvs);

private:
  tl::OutputStream *mp_stream;
  bool m_short_version;
};

}

#endif
