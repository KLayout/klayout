
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
 *  Main body:
 *    [version|description|unit|top|layer|connect|global|circuit|class|device|message-entry|any]*
 *
 *  [version]:
 *    version(<number>)             - file format version [short key: V]
 *
 *  [description]:
 *    description(<text>)           - an arbitrary description text [short key: B]
 *
 *  [unit]:
 *    unit(<unit>)                  - specifies the database unit [short key: U]
 *
 *  [top]:
 *    top(<circuit>)                - specifies the name of the top circuit [short key: W]
 *
 *  [layer]:
 *    layer(<name> <source-spec>?)  - define a layer [short key: L]
 *
 *  [connect]:
 *    connect(<layer1> <name> ...)  - connects layer1 with the following layers [short key: C]
 *
 *  [global]:
 *    global(<layer> <net-name> ...)
 *                                  - connects the shapes of the layer with the given global
 *                                    nets [short key: G]
 *
 *  [circuit]:
 *    circuit(<name> [circuit-def]) - circuit (cell) [short key: X]
 *
 *  [class]:
 *    class(<name> <template> [template-def])  - a device class definition (template: RES,CAP,...) [short key: K]
 *
 *  [device]:
 *    device(<name> <class> [device-abstract-terminal|any]*)
 *                                  - device abstract [short key: D]
 *
 *  [circuit-def]:
 *    [boundary|property|circuit-net|circuit-pin|circuit-device|subcircuit|any]*
 *
 *  [circuit-net]:
 *    net(<id> [name]? [geometries-def])
 *                                  - net geometry [short key: N]
 *                                    A net declaration shall be there also if no geometry
 *                                    is present. The ID is a numerical shortcut for the net.
 *
 *  [circuit-pin]:
 *    pin(<net-id> [name]?)         - outgoing pin connection [short key: P]
 *                                    Statement order specifies pin order.
 *
 *  [circuit-device]:
 *    device(<id> <abstract-or-class> [name|trans|combined-device|terminal-route|param|device-terminal|any]*)
 *                                  - device with connections [short key: D]
 *
 *  [subcircuit]:
 *    circuit(<id> [name]? [property|trans|subcircuit-pin|any])
 *                                  - subcircuit with connections [short key: X]
 *
 *  [boundary]:
 *    polygon([coord] ...) |        - defines a polygon [short key: Q]
 *                                    "*" for <x> or <y> means take previous
 *    rect([coord] [coord])         - defines a rectangle [short key: R]
 *                                    coordinates are bottom/left and top/right
 *
 *  [combined-device]:
 *    device(<abstract> [trans])
 *                                  - specifies an additional device component
 *                                    (for combined devices) with abstract <abstract>
 *                                    and offset dx, dy.
 *
 *  [terminal-route]:
 *    connect(<device-index> <outer-terminal-name> <inner-terminal-name>)
 *                                  - connects the outer terminal with the terminal
 *                                    of the device component with <device-index>:
 *                                    0 is the basic device, 1 the first combined
 *                                    device etc.
 *
 *  [name]:
 *    name(<name>)                  - specify net name [short key: I]
 *
 *  [geometries-def]:
 *    [property|polygon|rect|text|any]*
 *
 *  [property]:
 *    property(<prop-name> <prop-value>)
 *                                  - specifies a property value/key pair [short key: F]
 *                                    prop-name and prop-value are variant specifications
 *                                    in klayout notation: #x is an integer, ##y a floating-point
 *                                    value, a word or quoted literal is a string.
 *
 *  [polygon]:
 *    polygon(<layer> [coord] ...)  - defines a polygon [short key: Q]
 *                                    "*" for <x> or <y> means take previous
 *
 *  [rect]:
 *    rect(<layer> [coord] [coord]) - defines a rectangle [short key: R]
 *                                    coordinates are bottom/left and top/right
 *
 *  [text]:
 *    text(<layer> <string> [coord]) - defines a label [short key: J]
 *
 *  [coord]:
 *    <x> <y>                       - absolute coordinates
 *    (<x> <y>)                     - relative coordinates (reference is reset to 0,0
 *                                    for each net or terminal in device abstract)
 *
 *  [template-def]:
 *    [template-param|template-terminal|any]*
 *
 *  [template-param]:
 *    param(<name> <primary>? <default-value>*)    - defines a template parameter [short key: E]
 *                                    ('primary' is a value: 0 or 1)
 *
 *  [template-terminal]:
 *    terminal(<name>)              - defines a terminal [short key: T]
 *
 *  [device-abstract-terminal]:
 *    terminal(<terminal-name> [geometries-def])
 *                                  - specifies the terminal geometry [short key: T]
 *
 *  [param]:
 *    param(<name> <value>)         - defines a parameter [short key: E]
 *
 *  [device-terminal]:
 *    terminal(<terminal-name> <net-id>)
 *                                  - specifies connection of the terminal with a net (short key: T)
 *
 *  [subcircuit-pin]:
 *    pin(<pin-id> <net-id>)        - specifies connection of the pin with a net [short key: P]
 *
 *  [trans]:
 *    location(<x> <y>)             - location of the instance [short key: Y]
 *    rotation(<angle>)             - rotation angle (in degree, default is 0) [short key: O]
 *    mirror                        - if specified, the instance is mirrored before rotation [short key: M]
 *    scale(<mag>)                  - magnification (default is 1) [short key: S]
 *
 *  [message-entry]:
 *    message([severity] [message|message-geometry|message-cell|message-category|any]*) - message entry [short key: H]
 *
 *  [message]:
 *    description(<name>)           - message text [short key: B]
 *
 *  [message-geometry]:
 *    polygon(<string>)             - message geometry polygon in string-serialized form [short key: Q]
 *
 *  [message-cell]:
 *    cell(<name>)                  - message cell [short key: C]
 *
 *  [message-category]:
 *    cat(<name> <name>?)           - message category with optional description [short key: X]
 *
 *  [severity]:
 *    info |                        - [short key: I]
 *    warning |                     - [short key: W]
 *    error                         - [short key: E]
 *
 *  [any]:
 *    * |
 *    <token> |
 *    <token> ( [any]* ) |
 *    <float> |
 *    <quoted-string>
 */

namespace l2n_std_format
{
  struct DB_PUBLIC ShortKeys
  {
    static std::string l2n_magic_string;

    static std::string version_key;
    static std::string description_key;
    static std::string top_key;
    static std::string unit_key;
    static std::string layer_key;
    static std::string class_key;
    static std::string connect_key;
    static std::string global_key;
    static std::string circuit_key;
    static std::string net_key;
    static std::string name_key;
    static std::string property_key;
    static std::string device_key;
    static std::string subcircuit_key;
    static std::string polygon_key;
    static std::string rect_key;
    static std::string text_key;
    static std::string terminal_key;
    static std::string abstract_key;
    static std::string param_key;
    static std::string location_key;
    static std::string rotation_key;
    static std::string mirror_key;
    static std::string scale_key;
    static std::string pin_key;
    static std::string message_key;
    static std::string indent1;
    static std::string indent2;
    static std::string info_severity_key;
    static std::string warning_severity_key;
    static std::string error_severity_key;
    static std::string cell_key;
    static std::string cat_key;
  };

  struct DB_PUBLIC LongKeys
  {
    static std::string l2n_magic_string;

    static std::string version_key;
    static std::string description_key;
    static std::string top_key;
    static std::string unit_key;
    static std::string layer_key;
    static std::string class_key;
    static std::string connect_key;
    static std::string global_key;
    static std::string circuit_key;
    static std::string net_key;
    static std::string name_key;
    static std::string property_key;
    static std::string device_key;
    static std::string subcircuit_key;
    static std::string polygon_key;
    static std::string rect_key;
    static std::string text_key;
    static std::string terminal_key;
    static std::string abstract_key;
    static std::string param_key;
    static std::string location_key;
    static std::string rotation_key;
    static std::string mirror_key;
    static std::string scale_key;
    static std::string pin_key;
    static std::string message_key;
    static std::string indent1;
    static std::string indent2;
    static std::string info_severity_key;
    static std::string warning_severity_key;
    static std::string error_severity_key;
    static std::string cell_key;
    static std::string cat_key;
  };

  template <bool Short> struct DB_PUBLIC keys;

  template <> struct DB_PUBLIC keys<true> : public ShortKeys
  {
    inline static bool is_short () { return true; }
  };

  template <> struct DB_PUBLIC keys<false> : public LongKeys
  {
    inline static bool is_short () { return false; }
  };
}

}

#endif
