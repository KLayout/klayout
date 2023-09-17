
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
 *  Main body:
 *    #%lvsdb-klayout                  - header line identifies format
 *    [version|description|layout-netlist|reference-netlist|xrefs|any]*
 *
 *  [version]:
 *    version(<number>)                - file format version [short key: V]
 *
 *  [description]:
 *    description(<text>)              - an arbitrary description text [short key: B]
 *
 *  [layout-netlist]:
 *    layout(...)                      - layout netlist part [short key: J]
 *                                       Content is the LayoutToNetlist dump without version and description
 *
 *  [reference-netlist]:
 *    reference(...)
 *                                     - reference netlist part [short key: H]
 *                                       Content is the Netlist dump (reduced version of LayoutToNetlist)
 *
 *  [xrefs]:
 *    xref([xref|log|any]*)            - cross-reference part [short key: Z]
 *
 *  [xref]:
 *    circuit([non] [non] [status|message|log|circuit-xrefs|any]*)
 *                                     - circuit pair [short key: X]
 *
 *  [non]
 *    <name> | ()
 *
 *  [log]:
 *    log([log-entry]*)                - log entries [short key: L]
 *
 *  [log-entry]:
 *    entry([severity] [message|any]*) - log entry [short key: M]
 *
 *  [circuit-xrefs]:
 *    xref([xref-pin|xref-device|xref-circuit|xref-net|any]*)
 *                                     - circuit cross-reference part [short key: Z]
 *
 *  [xref-pin]:
 *    pin([ion] [ion] [status]? [message]? [any]*)
 *                                     - a pin pair [short key: P]
 *
 *  [xref-device]:
 *    device([ion] [ion] [status]? [message]? [any]*)
 *                                     - a device pair [short key: D]
 *
 *  [xref-circuit]:
 *    circuit([ion] [ion] [status]? [message]? [any]*)
 *                                     - a subcircuit pair [short key: X]
 *
 *  [xref-net]:
 *    net([ion] [ion] [status]? [message]? [any]*)
 *                                     - a net pair [short key: N]
 *
 *  [ion]:
 *    <id> | ()
 *
 *  [message]:
 *    description(<name>)              - error description [short key: B]
 *
 *  [severity]:
 *    info |                           - [short key: I]
 *    warning |                        - [short key: W]
 *    error                            - [short key: E]
 *
 *  [status]:
 *    mismatch |                       - [short key: 0]
 *    match |                          - [short key: 1]
 *    nomatch |                        - [short key: X]
 *    warning |                        - [short key: W]
 *    skipped                          - [short key: S]
 *
 *  [any]:
 *    * |
 *    <token> |
 *    <token> ( [any]* ) |
 *    <float> |
 *    <quoted-string>
 */

namespace lvs_std_format
{
  struct DB_PUBLIC ShortKeys
  {
    static std::string lvs_magic_string;

    static std::string reference_key;
    static std::string layout_key;
    static std::string xref_key;
    static std::string log_key;
    static std::string log_entry_key;

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
    static std::string log_key;
    static std::string log_entry_key;

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
