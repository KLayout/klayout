
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


#ifndef HDR_tlXMLWriter
#define HDR_tlXMLWriter

#include "tlCommon.h"

#include <iostream>

namespace tl
{

/**
 *  @brief A preliminary class for writing XML files
 */

class TL_PUBLIC XMLWriter 
{
public:
  XMLWriter (std::ostream &os);

  void start_document ();
  void start_document (const std::string &header);
  void start_element (const std::string &name);
  void write_attribute (const std::string &name, const std::string &value);
  void cdata (const std::string &text);
  void end_element (const std::string &name);
  void end_document ();

private:
  int m_indent;
  std::ostream &m_os;
  bool m_open;
  bool m_has_children;

  void write_indent ();
  void write_string (const std::string &s);
};

}

#endif

