
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef HDR_dbNetlistSpiceReader
#define HDR_dbNetlistSpiceReader

#include "dbCommon.h"
#include "dbNetlistReader.h"

#include "tlStream.h"
#include "tlObject.h"
#include "tlVariant.h"
#include "tlString.h"

#include <map>
#include <string>
#include <memory>

namespace db
{

class Netlist;
class NetlistSpiceReaderDelegate;

/**
 *  @brief A SPICE format reader for netlists
 */
class DB_PUBLIC NetlistSpiceReader
  : public NetlistReader
{
public:
  typedef std::map<std::string, tl::Variant> parameters_type;

  NetlistSpiceReader (NetlistSpiceReaderDelegate *delegate = 0);
  virtual ~NetlistSpiceReader ();

  virtual void read (tl::InputStream &stream, db::Netlist &netlist);

  /**
   *  @brief Sets or resets strict mode
   *  In strict mode, all subcircuits need to be present in the net list for example.
   */
  void set_strict (bool s)
  {
    m_strict = s;
  }

  /**
   *  @brief Returns true, if the extractor is at the end of the line
   *  "at_eol" is true at the line end or when a midline comment starts.
   */
  static bool at_eol (tl::Extractor &ex);

  /**
   *  @brief Unescapes a name
   *  Replaces backslash sequences with the true character and removes quotes.
   */
  static std::string unescape_name (const std::string &n);

  /**
   *  @brief Parses a netlist component (net name, expression etc.)
   *  Scans over the expression or net name and returns a string representing the latter.
   */
  static std::string parse_component (tl::Extractor &ex);

  /**
   *  @brief Reads a component name
   *  Scans over a component name and returns the
   */

private:
  tl::weak_ptr<NetlistSpiceReaderDelegate> mp_delegate;
  std::unique_ptr<NetlistSpiceReaderDelegate> mp_default_delegate;
  bool m_strict;
};

}

#endif
