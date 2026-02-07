
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_dbVia
#define HDR_dbVia

#include "dbCommon.h"
#include "dbLayerProperties.h"
#include "dbLibrary.h"

#include <string>

namespace db
{

/**
 *  @brief A descriptor for a via
 *
 *  This object describes one flavor of a via provided by via PCells.
 */
class DB_PUBLIC ViaType
{
public:
  /**
   *  @brief The default constructor
   */
  ViaType ()
  {
    init ();
  }

  /**
   *  @brief The constructor with a name
   */
  ViaType (const std::string &_name)
    : name (_name)
  {
    init ();
  }

  /**
   *  @brief The constructor with a name and description
   */
  ViaType (const std::string &_name, const std::string &_description)
    : name (_name), description (_description)
  {
    init ();
  }

  /**
   *  @brief The minimum width of the bottom layer of the via
   */
  double wbmin;

  /**
   *  @brief The maximum width of the bottom layer of the via
   *
   *  A negative value means "not specified" or "infinite".
   */
  double wbmax;

  /**
   *  @brief The minimum height of the bottom layer of the via
   */
  double hbmin;

  /**
   *  @brief The maximum height of the bottom layer of the via
   *
   *  A negative value means "not specified" or "infinite".
   */
  double hbmax;

  /**
   *  @brief The minimum width of the top layer of the via
   */
  double wtmin;

  /**
   *  @brief The maximum width of the top layer of the via
   *
   *  A negative value means "not specified" or "infinite".
   */
  double wtmax;

  /**
   *  @brief The minimum height of the top layer of the via
   */
  double htmin;

  /**
   *  @brief The maximum height of the top layer of the via
   *
   *  A negative value means "not specified" or "infinite".
   */
  double htmax;

  /**
   *  @brief The bottom layer
   */
  db::LayerProperties bottom;

  /**
   *  @brief A flag indicating whether the bottom layer is wired
   *
   *  For example, sheet layers such as diffusion are not wired.
   *  By default, layers are wired.
   */
  bool bottom_wired;

  /**
   *  @brief The grid of the bottom layer
   *
   *  Via dimensions are rounded to this grid on the bottom layer, if non-zero.
   */
  double bottom_grid;

  /**
   *  @brief The cut layer
   */
  db::LayerProperties cut;

  /**
   *  @brief The top layer
   */
  db::LayerProperties top;

  /**
   *  @brief A flag indicating whether the top layer is wired
   *
   *  For example, sheet layers such as diffusion are not wired.
   *  By default, layers are wired.
   */
  bool top_wired;

  /**
   *  @brief The grid of the top layer
   *
   *  Via dimensions are rounded to this grid on the top layer, if non-zero.
   */
  double top_grid;

  /**
   *  @brief The name of the via
   *
   *  The name is a formal name to identify the via.
   */
  std::string name;

  /**
   *  @brief The description of the via
   *
   *  This is a human-readable description. This attribute is optional.
   */
  std::string description;

private:
  void init ();
};

/**
 *  @brief Provides a via definition that is selected by "find_via_definitions_for"
 */
struct SelectedViaDefinition
{
  SelectedViaDefinition ()
    : lib (0), pcell (0)
  { }

  SelectedViaDefinition (db::Library *_lib, db::pcell_id_type _pcell, const db::ViaType &_via_type)
    : lib (_lib), pcell (_pcell), via_type (_via_type)
  { }

  /**
   *  @brief The library from which the via is taken
   */
  db::Library *lib;

  /**
   *  @brief The PCell from which the via is taken
   */
  db::pcell_id_type pcell;

  /**
   *  @brief The selected via type
   */
  db::ViaType via_type;
};

DB_PUBLIC std::vector<SelectedViaDefinition>
find_via_definitions_for (const std::string &technology, const db::LayerProperties &layer, int dir);

DB_PUBLIC std::vector<SelectedViaDefinition>
get_via_definitions (const std::string &technology);

}

#endif

