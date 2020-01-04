
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

#ifndef HDR_dbLayoutVsSchematicFormatDefs
#define HDR_dbLayoutVsSchematicFormatDefs

#include "dbCommon.h"
#include "dbLayoutToNetlistFormatDefs.h"

#include <string>

namespace db
{

/**
 *  This is the internal persistency format for LayoutVsSchematic
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
 *    #%lvsdb-klayout                  - header line identifies format
 *    version(<number>)                - file format version [short key: V]
 *    description(<text>)              - an arbitrary description text [short key: B]
 *    layout([layout])                 - layout part [short key: J]
 *    reference([reference-def]*)      - reference netlist part [short key: H]
 *    xref([xref-def]*)                - cross-reference part [short key: Z]
 *
 *  [layout]:
 *
 *      ...                            - the LayoutToNetlist dump without version and description
 *
 *  [reference-def]:
 *
 *    circuit(<name> [netlist-circuit-def]*)
 *                                     - circuit [short key: X]
 *  [netlist-circuit-def]:
 *
 *    net(<id> [net-name]?)            - a net declaration [short key: N]
 *    pin(<name> <net-id>)             - outgoing pin connection [short key: P]
 *    device(<name> [device-def]*)     - device with connections [short key: D]
 *    circuit(<name> [subcircuit-def]*)
 *                                     - subcircuit with connections [short key: X]
 *
 *  [net-name]:
 *
 *    name(<net-name>)                 - specify net name [short key: I]
 *
 *  [device-def]:
 *
 *    terminal(<terminal-name> <net-id>)
 *                                     - specifies connection of the terminal with
 *                                       a net [short key: T]
 *
 *  [subcircuit-def]:
 *
 *    pin(<pin-name> <net-id>)         - specifies connection of the pin with a net [short key: P]
 *
 *  [xref-def]:
 *
 *    circuit([non] [non] [status]? [circuit-xrefs])
 *                                     - circuit pair [short key: X]
 *
 *  [circuit-xrefs]:
 *
 *    xref([pair]*)
 *
 *  [pair]
 *
 *    pin([ion] [ion] [status]?)       - a pin pair [short key: P]
 *    device([ion] [ion] [status]?)    - a device pair [short key: D]
 *    circuit([ion] [ion] [status]?)   - a subcircuit pair [short key: X]
 *    net([ion] [ion] [status]?)       - a net pair [short key: N]
 *
 *  [non]
 *
 *    <name> | ()
 *
 *  [ion]
 *
 *    <id> | ()
 *
 *  [status]
 *
 *    mismatch |                       - [short key: 0]
 *    match |                          - [short key: 1]
 *    nomatch |                        - [short key: X]
 *    warning |                        - [short key: W]
 *    skipped                          - [short key: S]
 */

namespace lvs_std_format
{
  struct DB_PUBLIC ShortKeys
  {
    static std::string lvs_magic_string;

    static std::string reference_key;
    static std::string layout_key;
    static std::string xref_key;

    static std::string mismatch_key;
    static std::string match_key;
    static std::string nomatch_key;
    static std::string warning_key;
    static std::string skipped_key;
  };

  struct DB_PUBLIC LongKeys
  {
    static std::string lvs_magic_string;

    static std::string reference_key;
    static std::string layout_key;
    static std::string xref_key;

    static std::string mismatch_key;
    static std::string match_key;
    static std::string nomatch_key;
    static std::string warning_key;
    static std::string skipped_key;
  };

  template <bool Short> struct DB_PUBLIC keys;

  template <> struct DB_PUBLIC keys<true> : public l2n_std_format::keys<true>, public ShortKeys
  {
    typedef l2n_std_format::keys<true> l2n_keys;
    inline static bool is_short () { return true; }
  };

  template <> struct DB_PUBLIC keys<false> : public l2n_std_format::keys<false>, public LongKeys
  {
    typedef l2n_std_format::keys<false> l2n_keys;
    inline static bool is_short () { return false; }
  };
}

}

#endif
