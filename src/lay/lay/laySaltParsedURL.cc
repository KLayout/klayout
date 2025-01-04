
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "laySaltParsedURL.h"
#include "laySaltGrain.h"
#include "tlString.h"

namespace lay
{

static void
parse_git_url (tl::Extractor &ex, std::string &url, std::string &branch, std::string &subfolder)
{
  const char *org_str = ex.get ();

  std::string w;
  // protocol (http:)
  ex.try_read_word (w, "") && ex.test (":");
  while (! ex.at_end () && ex.test ("/")) {
    ;
  }
  // server ("www.klayout.de")
  while (! ex.at_end () && (*ex != '/' && *ex != '+' && *ex != '[')) {
    ++ex;
  }

  while (! ex.at_end ()) {

    ++ex;

    //  next component
    const char *c1 = ex.get ();
    while (! ex.at_end () && (*ex != '/' && *ex != '+' && *ex != '[')) {
      ++ex;
    }
    const char *c2 = ex.get ();

    std::string comp (c1, c2 - c1);

    if ((! ex.at_end () && (*ex == '+' || *ex == '[')) || comp.find (".git") == comp.size () - 4) {
      //  subfolder starts here
      break;
    }

  }

  url = std::string (org_str, ex.get () - org_str);

  if (ex.at_end ()) {
    return;
  }

  //  skip URL/subfolder separator
  if (*ex == '/') {
    while (! ex.at_end () && *ex == '/') {
      ++ex;
    }
  } else if (*ex == '+') {
    ++ex;
  }

  //  subfolders
  {
    const char *c1 = ex.get ();
    while (! ex.at_end () && *ex != '[') {
      ++ex;
    }
    const char *c2 = ex.get ();
    subfolder = std::string (c1, c2 - c1);
  }

  if (! ex.at_end () && *ex == '[') {

    //  explicit branch
    ++ex;
    const char *c1 = ex.get ();
    while (! ex.at_end () && *ex != ']') {
      ++ex;
    }
    const char *c2 = ex.get ();
    branch = std::string (c1, c2 - c1);

  } else if (! subfolder.empty ()) {

    //  SVN emulation

    auto parts = tl::split (subfolder, "/");
    if (parts.size () >= 1 && parts.front () == "trunk") {

      branch = "HEAD";
      parts.erase (parts.begin ());
      subfolder = tl::join (parts, "/");

    } else if (parts.size () >= 2 && parts.front () == "tags") {

      branch = "refs/tags/" + parts[1];
      parts.erase (parts.begin (), parts.begin () + 2);
      subfolder = tl::join (parts, "/");

    } else if (parts.size () >= 2 && parts.front () == "branches") {

      branch = "refs/heads/" + parts[1];
      parts.erase (parts.begin (), parts.begin () + 2);
      subfolder = tl::join (parts, "/");

    }

  }
}

SaltParsedURL::SaltParsedURL (const std::string &url)
  : m_protocol (lay::DefaultProtocol)
{
  tl::Extractor ex (url.c_str ());
  if (ex.test ("svn") && ex.test ("+")) {
    m_protocol = lay::WebDAV;
    m_url = ex.get ();
    return;
  }

  ex = tl::Extractor (url.c_str ());
  if (ex.test ("git") && ex.test ("+")) {
    m_protocol = lay::Git;
    parse_git_url (ex, m_url, m_branch, m_subfolder);
    return;
  }

  m_url = url;
}

}
