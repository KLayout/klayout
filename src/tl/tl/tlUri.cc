
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


#include "tlUri.h"
#include "tlString.h"

#include <cstring>
#include <cstdio>

namespace tl
{

static bool is_hex (char c)
{
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static char hex2int (char c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return (c - 'A') + 10;
  } else if (c >= 'a' || c <= 'f') {
    return (c - 'a') + 10;
  } else {
    return 0;
  }
}

static char int2hex (char c)
{
  c &= 0xf;
  if (c < 10) {
    return c + '0';
  } else {
    return c - 10 + 'A';
  }
}

static std::string unescape (const std::string &s)
{
  std::string res;
  for (const char *cp = s.c_str (); *cp; ++cp) {
    if (*cp == '%' && is_hex (cp [1]) && is_hex (cp [2])) {
      res += hex2int (cp[1]) * 16 + hex2int (cp[2]);
      cp += 2;
    } else {
      res += *cp;
    }
  }
  return res;
}

static std::string escape (const std::string &s)
{
  const char *special = "?#[]$&'()*+,;";

  std::string res;
  for (const char *cp = s.c_str (); *cp; ++cp) {
    if ((unsigned char) *cp <= 32 || (unsigned char) *cp >= 128 || strchr (special, *cp) != 0) {
      res += "%";
      res += int2hex (*cp >> 4);
      res += int2hex (*cp);
    } else {
      res += *cp;
    }
  }
  return res;
}

URI::URI ()
{
  //  .. nothing yet ..
}


URI::URI (const std::string &uri)
{
  tl::Extractor ex0 (uri.c_str ());
  tl::Extractor ex = ex0;

  //  NOTE: to distinguish a windows drive letter from a scheme, we expect the scheme to
  //  be longer than one character
  if (ex.try_read_word (m_scheme) && *ex == ':' && m_scheme.length () > 1) {
    //  got scheme
    ++ex;
  } else {
    m_scheme.clear ();
    ex = ex0;
  }
  m_scheme = unescape (m_scheme);

  bool prefer_authority = false;
  if (m_scheme == "http" || m_scheme == "https") {
    prefer_authority = true;
  }

  ex0 = ex;
  if (ex.test ("//") || (ex.test ("/") ? prefer_authority : prefer_authority)) {
    //  definitely an authority
    while (! ex.at_end () && *ex != '/') {
      m_authority += *ex;
      ++ex;
    }
  } else {
    ex = ex0;
  }
  m_authority = unescape (m_authority);

  //  parse path
  while (! ex.at_end () && *ex != '?' && *ex != '#') {
    m_path += *ex;
    ++ex;
  }
  m_path = unescape (m_path);

  //  parse parameters
  if (*ex == '?') {
    ++ex;
    while (! ex.at_end () && *ex != '#') {
      std::string k, v;
      while (! ex.at_end () && *ex != '=' && *ex != '&' && *ex != '#') {
        k += *ex;
        ++ex;
      }
      if (*ex == '=') {
        ++ex;
        while (! ex.at_end () && *ex != '&' && *ex != '#') {
          v += *ex;
          ++ex;
        }
      }
      m_query[unescape (k)] = unescape (v);
      if (*ex == '&') {
        ++ex;
      }
    }
  }

  if (*ex == '#') {
    ++ex;
    while (! ex.at_end ()) {
      m_fragment += *ex;
      ++ex;
    }
  }
  m_fragment = unescape (m_fragment);
}

std::string
URI::to_string () const
{
  std::string res;

  if (! m_scheme.empty ()) {
    res += escape (m_scheme);
    res += ":";
  }

  if (! m_authority.empty ()) {
    res += "//";
    res += escape (m_authority);
  }

  if (! m_path.empty ()) {
    res += escape (m_path);
  }

  if (! m_query.empty ()) {
    for (std::map<std::string, std::string>::const_iterator p = m_query.begin (); p != m_query.end (); ++p) {
      res += (p == m_query.begin () ? "?" : "&");
      res += escape (p->first);
      if (! p->second.empty ()) {
        res += "=";
        res += escape (p->second);
      }
    }
  }

  if (! m_fragment.empty ()) {
    res += "#";
    res += m_fragment;
  }

  return res;
}

std::string
URI::to_abstract_path () const
{
  if (m_scheme.empty ()) {
    return path ();
  } else {
    return to_string ();
  }
}

URI
URI::resolved (const URI &other) const
{
  if (! other.scheme ().empty () && other.scheme () != scheme ()) {
    return other;
  }
  if (! other.authority ().empty () && other.authority () != authority ()) {
    return other;
  }

  URI res = *this;

  //  combine paths
  //  TODO: normalize? I.e. replace "x/y/../z" by "x/z"?
  if (! other.path ().empty ()) {
    if (other.path ()[0] == '/') {
      res.m_path = other.path ();
    } else {
      if (! res.m_path.empty ()) {
        res.m_path += "/";
      }
      res.m_path += other.path ();
    }
  }

  res.m_query = other.query ();
  res.m_fragment = other.fragment ();

  return res;
}

}

