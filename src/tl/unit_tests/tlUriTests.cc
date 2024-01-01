
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

#include "tlUri.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"

//  Secret mode switchers for testing
namespace tl
{
TL_PUBLIC void file_utils_force_windows ();
TL_PUBLIC void file_utils_force_linux ();
TL_PUBLIC void file_utils_force_reset ();
}

std::string uri2string (const tl::URI &uri)
{
  std::string res;

  if (! uri.scheme ().empty ()) {
    res += "<" + uri.scheme () + ">";
    res += ":";
  }

  if (! uri.authority ().empty ()) {
    res += "//<";
    res += uri.authority ();
    res += ">";
  }

  if (! uri.path ().empty ()) {
    res += "<" + uri.path () + ">";
  }

  if (! uri.query ().empty ()) {
    for (std::map<std::string, std::string>::const_iterator p = uri.query ().begin (); p != uri.query ().end (); ++p) {
      res += (p == uri.query ().begin () ? "?<" : "&<");
      res += p->first;
      res += ">";
      if (! p->second.empty ()) {
        res += "=<";
        res += p->second;
        res += ">";
      }
    }
  }

  if (! uri.fragment ().empty ()) {
    res += "#<";
    res += uri.fragment ();
    res += ">";
  }

  return res;
}

//  basic parsing ability
TEST(1)
{
  tl::URI uri;
  EXPECT_EQ (uri2string (uri), "");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("http://www.klayout.de"))), "<http>://<www.klayout.de>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("anyfile.txt"))), "<anyfile.txt>");

  uri = tl::URI ("scheme:");
  EXPECT_EQ (uri2string (uri), "<scheme>:");

  uri = tl::URI ("http:www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "<http>://<www.klayout.de></path/to/file>");

  uri = tl::URI ("http:/www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "<http>://<www.klayout.de></path/to/file>");

  uri = tl::URI ("http://www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "<http>://<www.klayout.de></path/to/file>");
  EXPECT_EQ (uri.to_string (), "http://www.klayout.de/path/to/file");

  uri = tl::URI ("www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "<www.klayout.de/path/to/file>");

  uri = tl::URI ("/www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "</www.klayout.de/path/to/file>");

  uri = tl::URI ("//www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "//<www.klayout.de></path/to/file>");
  EXPECT_EQ (uri.to_string (), "//www.klayout.de/path/to/file");

  uri = tl::URI ("file:www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "<file>:<www.klayout.de/path/to/file>");

  uri = tl::URI ("file:/www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "<file>:</www.klayout.de/path/to/file>");

  uri = tl::URI ("file://www.klayout.de/path/to/file");
  EXPECT_EQ (uri2string (uri), "<file>://<www.klayout.de></path/to/file>");

  uri = tl::URI ("file:///path/to/file");
  EXPECT_EQ (uri2string (uri), "<file>:</path/to/file>");

  uri = tl::URI ("file:///c:/path/to/file");
  EXPECT_EQ (uri2string (uri), "<file>:</c:/path/to/file>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("http://www.klayout.de"))), "<http>://<www.klayout.de>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("http:///other"))), "<http>:</other>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("../other"))), "<file>:</c:/path/to/file/../other>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("/other"))), "<file>:</other>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("file:../other"))), "<file>:</c:/path/to/file/../other>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("file:../other?a=b#frag"))), "<file>:</c:/path/to/file/../other>?<a>=<b>#<frag>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("file:/other"))), "<file>:</other>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("file:/other?a=b#frag"))), "<file>:</other>?<a>=<b>#<frag>");

  uri = tl::URI ("//www.klayout.de/path/to/file?a=b");
  EXPECT_EQ (uri2string (uri), "//<www.klayout.de></path/to/file>?<a>=<b>");

  uri = tl::URI ("/path/to/file?a=b");
  EXPECT_EQ (uri2string (uri), "</path/to/file>?<a>=<b>");

  uri = tl::URI ("/path/to/file?a=v1&c=v3&b=v2");
  EXPECT_EQ (uri2string (uri), "</path/to/file>?<a>=<v1>&<b>=<v2>&<c>=<v3>");

  uri = tl::URI ("/path/to/file?a=v1&c=v3&b=v2#fragment");
  EXPECT_EQ (uri2string (uri), "</path/to/file>?<a>=<v1>&<b>=<v2>&<c>=<v3>#<fragment>");
  EXPECT_EQ (uri.to_string (), "/path/to/file?a=v1&b=v2&c=v3#fragment");

  uri = tl::URI ("/path/to/file#fragment");
  EXPECT_EQ (uri2string (uri), "</path/to/file>#<fragment>");
  EXPECT_EQ (uri.to_string (), "/path/to/file#fragment");

  uri = tl::URI ("/path/to/%2c%2C%20%file#fragment");
  EXPECT_EQ (uri2string (uri), "</path/to/,, %file>#<fragment>");
  EXPECT_EQ (uri.to_string (), "/path/to/%2C%2C%20%file#fragment");
  EXPECT_EQ (tl::URI (uri.to_string ()).to_string (), "/path/to/%2C%2C%20%file#fragment");

  uri = tl::URI ("/path/to/file?%61=v%31&%63=v%33&%62=v%32#fragment");
  EXPECT_EQ (uri2string (uri), "</path/to/file>?<a>=<v1>&<b>=<v2>&<c>=<v3>#<fragment>");
  EXPECT_EQ (uri2string (uri.resolved (tl::URI ("../other"))), "</path/to/file/../other>");
}

//  windows file paths compatibility
TEST(2)
{
  try {

    tl::file_utils_force_windows ();

    //  use case taken from Magic writer:

    tl::URI uri ("c:\\users\\myself\\path.txt");
    EXPECT_EQ (uri.scheme (), "");
    EXPECT_EQ (uri.path (), "c:\\users\\myself\\path.txt");

    std::string ext = tl::extension (uri.path ());
    EXPECT_EQ (ext, "txt");

    uri.set_path (tl::dirname (uri.path ()));
    EXPECT_EQ (uri.to_string (), "C:\\users\\myself");

    tl::file_utils_force_reset ();

  } catch (...) {
    tl::file_utils_force_reset ();
    throw;
  }
}

//  issue #733
TEST(3_pathsWithPlus)
{
  EXPECT_EQ (tl::URI ("/users/a_plus_b").resolved (tl::URI ("file.txt")).to_string (), "/users/a_plus_b/file.txt");
  EXPECT_EQ (tl::URI ("/users/a+b").resolved (tl::URI ("file.txt")).to_string (), "/users/a%2Bb/file.txt");
  EXPECT_EQ (tl::URI ("/users/a+b").resolved (tl::URI ("file.txt")).to_abstract_path (), "/users/a+b/file.txt");
  EXPECT_EQ (tl::URI ("file://users/a+b").resolved (tl::URI ("file.txt")).to_string (), "file://users/a%2Bb/file.txt");
  EXPECT_EQ (tl::URI ("file://users/a+b").resolved (tl::URI ("file.txt")).to_abstract_path (), "file://users/a%2Bb/file.txt");
  //  drive-letter paths
  EXPECT_EQ (tl::URI ("c:/users/a+b").resolved (tl::URI ("file.txt")).to_string (), "c:/users/a%2Bb/file.txt");
  EXPECT_EQ (tl::URI ("c:/users/a+b").resolved (tl::URI ("file.txt")).to_abstract_path (), "c:/users/a+b/file.txt");
}
