

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

#include "dbCommon.h"
#include "dbLayoutVsSchematic.h"
#include "dbLayoutVsSchematicWriter.h"
#include "dbLayoutVsSchematicReader.h"
#include "dbNetlistCompareUtils.h"

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

bool
LayoutVsSchematic::flag_missing_ports (const db::Circuit *circuit)
{
  if (! mp_cross_ref.get ()) {
    return false;
  }

  db::NetlistCrossReference::PerCircuitData *pcd = const_cast<db::NetlistCrossReference::PerCircuitData *> (mp_cross_ref->per_circuit_data_for (std::make_pair (circuit, circuit)));
  if (! pcd) {
    return false;
  }

  bool error = false;
  bool any = false;

  for (auto n = pcd->nets.begin (); n != pcd->nets.end (); ++n) {

    const db::Net *schem = n->pair.second;
    const db::Net *layout = n->pair.first;

    if (schem && layout && schem->begin_pins () != schem->end_pins ()) {

      any = true;

      if (db::name_compare (layout, schem) != 0) {

        std::string msg = tl::sprintf (tl::to_string (tr ("Port mismatch '%s' vs. '%s'")), layout->expanded_name (), schem->expanded_name ());
        db::LogEntryData entry (db::Error, msg);
        pcd->log_entries.push_back (entry);

        error = true;

      }
    }

  }

  if (! any) {

    std::string msg = tl::to_string (tr ("No pins found in circuit during 'flag_missing_ports'"));
    db::LogEntryData entry (db::Warning, msg);
    pcd->log_entries.push_back (entry);

  }

  return !error;
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
