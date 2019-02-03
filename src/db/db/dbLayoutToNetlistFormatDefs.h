
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

#ifndef HDR_dbLayoutToNetlistFormatDefs
#define HDR_dbLayoutToNetlistFormatDefs

#include "dbCommon.h"

#include <string>

namespace db
{

/**
 *  This is the internal persistency format for LayoutToNetlist
 *
 *  It's intentionally *not* XML to keep the overhead low.
 *
 *  Comments are introduced by hash: # ...
 *  Names are words (alphanumerical plus "$", "_", ".") or enclosed in single or double quotes.
 *  Escape character is backslash.
 *  Separator is either , or whitespace. Keywords and names are case sensitive.
 *  Short keys are provided for compacter representation. Short keys can be
 *  non-alpha (e.g. "*") or empty.
 *  Single-valued attributes can be given without brackets.
 *  All dimensions are in units of database unit.
 *  The file follows the declaration-before-use principle
 *  (circuits before subcircuits, nets before use ...)
 *
 *  Global statements:
 *
 *    version(<number>)             - file format version [short key: V]
 *    description(<text>)           - an arbitrary description text [short key: B]
 *    unit(<unit>)                  - specifies the database unit [short key: U]
 *    top(<circuit>)                - specifies the name of the top circuit [short key: W]
 *    layer(<name>)                 - define a layer [short key: L]
 *    connect(<layer1> <name> ...)  - connects layer1 with the following layers [short key: C]
 *    global(<layer> <net-name> ...)
 *                                  - connects the shapes of the layer with the given global
 *                                    nets [short key: G]
 *    circuit(<name> [circuit-def]) - circuit (cell) [short key: X]
 *    device(<name> <class> [device-abstract-def])
 *                                  - device abstract [short key: D]
 *
 *  [circuit-def]:
 *
 *    net(<id> [net-name]? [geometry-def]*)
 *                                  - net geometry [short key: N]
 *                                    A net declaration shall be there also if no geometry
 *                                    is present. The ID is a numerical shortcut for the net.
 *    pin(<name> <net-id>)          - outgoing pin connection [short key: P]
 *    device(<name> <abstract> [device-def])
 *                                  - device with connections [short key: D]
 *    circuit(<name> [circuit-def]) - subcircuit with connections [short key: X]
 *
 *  [net-name]:
 *    name(<net-name>)              - specify net name [short key:
 *
 *  [geometry-def]:
 *
 *    polygon(<layer> <x> <y> ...)  - defines a polygon [short key: Q]
 *                                    "*" for <x> or <y> means take previous
 *    rect(<layer> <left> <bottom> <right> <top>)
 *                                  - defines a rectangle [short key: R]
 *
 *  [device-abstract-def]:
 *
 *    terminal(<terminal-name> [geometry-def]*)
 *                                  - specifies the terminal geometry [short key: T]
 *
 *  [device-def]:
 *
 *    location(<x> <y>)             - location of the device [short key Y]
 *                                    must be before terminal
 *    param(<name> <value>)         - defines a parameter [short key E]
 *    terminal(<terminal-name> <net-id>)
 *                                  - specifies connection of the terminal with
 *                                    a net (short key: T)
 *
 *  [subcircuit-def]:
 *
 *    location(<x> <y>)             - location of the subcircuit [short key Y]
 *    rotation(<angle>)             - rotation angle (in degree, default is 0) [short key O]
 *    mirror                        - if specified, the instance is mirrored before rotation [short key M]
 *    scale(<mag>)                  - magnification (default is 1) [short key S]
 *    pin(<pin-name> <net-id>)      - specifies connection of the pin with a net [short key: P]
 */

namespace l2n_std_format
{
  template <bool Short>
  struct DB_PUBLIC keys
  {
    static const std::string version_key;
    static const std::string description_key;
    static const std::string top_key;
    static const std::string unit_key;
    static const std::string layer_key;
    static const std::string connect_key;
    static const std::string global_key;
    static const std::string circuit_key;
    static const std::string net_key;
    static const std::string name_key;
    static const std::string device_key;
    static const std::string subcircuit_key;
    static const std::string polygon_key;
    static const std::string rect_key;
    static const std::string terminal_key;
    static const std::string abstract_key;
    static const std::string param_key;
    static const std::string location_key;
    static const std::string rotation_key;
    static const std::string mirror_key;
    static const std::string scale_key;
    static const std::string pin_key;
    static const std::string indent1;
    static const std::string indent2;

    inline static bool is_short () { return Short; }
  };
}

}

#endif
