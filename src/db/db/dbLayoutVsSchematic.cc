

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

#include "dbCommon.h"
#include "dbLayoutVsSchematic.h"
#include "dbLayoutVsSchematicWriter.h"
#include "dbLayoutVsSchematicReader.h"

namespace db
{

LayoutVsSchematic::LayoutVsSchematic (const db::RecursiveShapeIterator &iter)
  : LayoutToNetlist (iter)
{
  //  .. nothing yet ..
}

LayoutVsSchematic::LayoutVsSchematic (db::DeepShapeStore *dss, unsigned int layout_index)
  : LayoutToNetlist (dss, layout_index)
{
  //  .. nothing yet ..
}

LayoutVsSchematic::LayoutVsSchematic (const std::string &topcell_name, double dbu)
  : LayoutToNetlist (topcell_name, dbu)
{
  //  .. nothing yet ..
}

LayoutVsSchematic::LayoutVsSchematic ()
  : LayoutToNetlist ()
{
  //  .. nothing yet ..
}

LayoutVsSchematic::~LayoutVsSchematic ()
{
  //  .. nothing yet ..
}

void LayoutVsSchematic::set_reference_netlist (db::Netlist *ref_netlist)
{
  ref_netlist->keep ();
  mp_reference_netlist.reset (ref_netlist);
  mp_cross_ref.reset (0);
}

bool LayoutVsSchematic::compare_netlists (db::NetlistComparer *compare)
{
  if (! netlist ()) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }

  if (! reference_netlist ()) {
    throw tl::Exception (tl::to_string (tr ("The reference netlist has not been set yet")));
  }

  return compare->compare (netlist (), reference_netlist (), make_cross_ref ());
}

db::NetlistCrossReference *LayoutVsSchematic::make_cross_ref ()
{
  if (! mp_cross_ref.get ()) {
    mp_cross_ref.reset (new db::NetlistCrossReference ());
  }
  return mp_cross_ref.get ();
}


void LayoutVsSchematic::save (const std::string &path, bool short_format)
{
  tl::OutputStream stream (path);
  db::LayoutVsSchematicStandardWriter writer (stream, short_format);
  set_filename (path);
  writer.write (this);
}

void LayoutVsSchematic::load (const std::string &path)
{
  tl::InputStream stream (path);
  db::LayoutVsSchematicStandardReader reader (stream);
  set_filename (path);
  set_name (stream.filename ());
  reader.read (this);
}

}
