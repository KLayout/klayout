
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "laySaltGrain.h"
#include "tlString.h"
#include "tlXMLParser.h"

#include <QDir>
#include <QFileInfo>

namespace lay
{

static const std::string grain_filename = "grain.xml";

SaltGrain::SaltGrain ()
{
  //  .. nothing yet ..
}

bool
SaltGrain::operator== (const SaltGrain &other) const
{
  return m_name == other.m_name &&
         m_path == other.m_path &&
         m_version == other.m_version &&
         m_url == other.m_url &&
         m_title == other.m_title &&
         m_doc == other.m_doc &&
         m_dependencies == other.m_dependencies &&
         m_author == other.m_author &&
         m_author_contact == other.m_author_contact &&
         m_license == other.m_license &&
         m_authored_time == other.m_authored_time &&
         m_installed_time == other.m_installed_time;
}

void
SaltGrain::set_name (const std::string &n)
{
  m_name = n;
}

void
SaltGrain::set_version (const std::string &v)
{
  m_version = v;
}

void
SaltGrain::set_path (const std::string &p)
{
  m_path = p;
}

void
SaltGrain::set_url (const std::string &u)
{
  m_url = u;
}

void
SaltGrain::set_title (const std::string &t)
{
  m_title = t;
}

void
SaltGrain::set_doc (const std::string &t)
{
  m_doc = t;
}

void
SaltGrain::set_author (const std::string &a)
{
  m_author = a;
}

void
SaltGrain::set_author_contact (const std::string &a)
{
  m_author_contact = a;
}

void
SaltGrain::set_license (const std::string &l)
{
  m_license = l;
}

void
SaltGrain::set_authored_time (const QDateTime &t)
{
  m_authored_time = t;
}

void
SaltGrain::set_installed_time (const QDateTime &t)
{
  m_installed_time = t;
}

int
SaltGrain::compare_versions (const std::string &v1, const std::string &v2)
{
  tl::Extractor ex1 (v1.c_str ());
  tl::Extractor ex2 (v2.c_str ());

  while (true) {

    if (ex1.at_end () && ex2.at_end ()) {
      return 0;
    }

    int n1 = 0, n2 = 0;
    if (! ex1.at_end ()) {
      ex1.try_read (n1);
    }
    if (! ex2.at_end ()) {
      ex2.try_read (n2);
    }

    if (n1 != n2) {
      return n1 < n2 ? -1 : 1;
    }

    while (! ex1.at_end ()) {
      char c = *ex1;
      ++ex1;
      if (c == '.') {
        break;
      }
    }

    while (! ex2.at_end ()) {
      char c = *ex2;
      ++ex2;
      if (c == '.') {
        break;
      }
    }

  }
}

struct TimeConverter
{
  std::string to_string (const QDateTime &time) const
  {
    if (time.isNull ()) {
      return std::string ();
    } else {
      return tl::to_string (time.toString (Qt::ISODate));
    }
  }

  void from_string (const std::string &time, QDateTime &res) const
  {
    if (time.empty ()) {
      res = QDateTime ();
    } else {
      res = QDateTime::fromString (tl::to_qstring (time), Qt::ISODate);
    }
  }
};

static tl::XMLStruct<lay::SaltGrain> xml_struct ("salt-grain",
  tl::make_member (&SaltGrain::name, &SaltGrain::set_name, "name") +
  tl::make_member (&SaltGrain::version, &SaltGrain::set_version, "version") +
  tl::make_member (&SaltGrain::title, &SaltGrain::set_title, "title") +
  tl::make_member (&SaltGrain::doc, &SaltGrain::set_doc, "doc") +
  tl::make_member (&SaltGrain::url, &SaltGrain::set_url, "url") +
  tl::make_member (&SaltGrain::license, &SaltGrain::set_license, "license") +
  tl::make_member (&SaltGrain::author, &SaltGrain::set_author, "author") +
  tl::make_member (&SaltGrain::author_contact, &SaltGrain::set_author_contact, "author-contact") +
  tl::make_member (&SaltGrain::authored_time, &SaltGrain::set_authored_time, "authored-time", TimeConverter ()) +
  tl::make_member (&SaltGrain::installed_time, &SaltGrain::set_installed_time, "installed-time", TimeConverter ()) +
  tl::make_element (&SaltGrain::begin_dependencies, &SaltGrain::end_dependencies, &SaltGrain::add_dependency, "depends",
    tl::make_member (&SaltGrain::Dependency::name, "name") +
    tl::make_member (&SaltGrain::Dependency::url, "url") +
    tl::make_member (&SaltGrain::Dependency::version, "version")
  )
);

bool
SaltGrain::is_readonly () const
{
  return QFileInfo (tl::to_qstring (path ())).isWritable ();
}

void
SaltGrain::load (const std::string &p)
{
  tl::XMLFileSource source (p);
  xml_struct.parse (source, *this);
}

void
SaltGrain::save () const
{
  save (tl::to_string (QDir (tl::to_qstring (path ())).filePath (tl::to_qstring (grain_filename))));
}

void
SaltGrain::save (const std::string &p) const
{
  tl::OutputStream os (p, tl::OutputStream::OM_Plain);
  xml_struct.write (os, *this);
}

SaltGrain
SaltGrain::from_path (const std::string &path)
{
  QDir dir (tl::to_qstring (path));

  SaltGrain g;
  g.load (tl::to_string (dir.filePath (tl::to_qstring (grain_filename))));
  g.set_path (tl::to_string (dir.absolutePath ()));
  return g;
}

bool
SaltGrain::is_grain (const std::string &path)
{
  QDir dir (tl::to_qstring (path));
  QString gf = dir.filePath (tl::to_qstring (grain_filename));
  return QFileInfo (gf).exists ();
}

}
