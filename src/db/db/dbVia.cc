
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

#include "dbVia.h"
#include "dbLibraryManager.h"
#include "dbPCellDeclaration.h"

namespace db
{

void
ViaType::init ()
{
  wbmin = 0.0;
  wbmax = -1.0;
  hbmin = 0.0;
  hbmax = -1.0;
  wtmin = 0.0;
  wtmax = -1.0;
  htmin = 0.0;
  htmax = -1.0;
  bottom_wired = true;
  bottom_grid = 0.0;
  top_wired = true;
  top_grid = 0.0;
}

// ---------------------------------------------------------------------------------------

std::vector<SelectedViaDefinition>
find_via_definitions_for (const std::string &technology, const db::LayerProperties &layer, int dir)
{
  std::vector<SelectedViaDefinition> via_defs;

  //  Find vias with corresponding top an bottom layers
  for (auto l = db::LibraryManager::instance ().begin (); l != db::LibraryManager::instance ().end (); ++l) {

    db::Library *lib = db::LibraryManager::instance ().lib (l->second);
    if (lib->for_technologies () && ! lib->is_for_technology (technology)) {
      continue;
    }

    for (auto pc = lib->layout ().begin_pcells (); pc != lib->layout ().end_pcells (); ++pc) {

      const db::PCellDeclaration *pcell = lib->layout ().pcell_declaration (pc->second);

      auto via_types = pcell->via_types ();
      for (auto vt = via_types.begin (); vt != via_types.end (); ++vt) {
        if ((dir >= 0 && vt->bottom.log_equal (layer) && vt->bottom_wired) ||
            (dir <= 0 && vt->top.log_equal (layer) && vt->top_wired)) {
          via_defs.push_back (SelectedViaDefinition (lib, pc->second, *vt));
        }
      }

    }

  }

  return via_defs;
}

}
