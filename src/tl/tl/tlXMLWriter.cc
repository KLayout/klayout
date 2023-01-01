
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



#include "tlXMLWriter.h"

namespace tl
{

XMLWriter::XMLWriter (std::ostream &os)
  : m_indent (0), m_os (os), m_open (false), m_has_children (false)
{
  // .. nothing yet ..
}

void 
XMLWriter::start_document ()
{
  start_document ("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
}

void 
XMLWriter::start_document (const std::string &header)
{
  m_os << header.c_str();
  m_open = false;
  m_has_children = false;
  m_indent = 0;
}

void 
XMLWriter::start_element (const std::string &name)
{
  if (m_open) {
    m_os << ">";
  } 
  m_os << std::endl;

  write_indent();
  m_os << "<" << name.c_str();
  m_open = true;
  m_has_children = false;

  ++m_indent;
}

void 
XMLWriter::write_attribute (const std::string &name, const std::string &value)
{
  m_os << " " << name.c_str() << "=\"";
  write_string (value);
  m_os << "\"";
}

void 
XMLWriter::cdata (const std::string &text)
{
  if (m_open) {
    m_os << ">";
    m_open = false;
  }

  write_string (text); 

  m_has_children = false;
}

void 
XMLWriter::end_element (const std::string &name)
{
  --m_indent;

  if (m_open) {
    m_os << "/>" << std::endl;
  } else {
    if (m_has_children) {
      m_os << std::endl;
      write_indent ();
    } 
    m_os << "</" << name.c_str() << ">";
  }

  m_open = false;
  m_has_children = true;
}

void 
XMLWriter::end_document ()
{
  m_os << std::endl;
}

void 
XMLWriter::write_indent ()
{
  for (int i = 0; i < m_indent; ++i) {
    m_os << " ";
  }
}

void 
XMLWriter::write_string (const std::string &s)
{
  for (const char *cp = s.c_str (); *cp; ++cp) {
    unsigned char c = (unsigned char) *cp;
    if (c == '&') {
      m_os << "&amp;";
    } else if (c == '<') {
      m_os << "&lt;";
    } else if (c == '>') {
      m_os << "&gt;";
    } else if (c < ' ') {
      m_os << "&#" << int (c) << ";";
    } else {
      m_os << c;
    }
  }
}

}

