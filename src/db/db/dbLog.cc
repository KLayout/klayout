
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

#include "dbCommon.h"
#include "dbLog.h"
#include "tlThreads.h"

namespace db
{

// ------------------------------------------------------------------
//  A string repository for keeping the memory footprint low for
//  the log entries

class LogEntryStringRepository
{
public:
  LogEntryStringRepository ()
  {
    //  .. nothing yet ..
  }

  size_t id_for_string (const std::string &s)
  {
    if (s.empty ()) {
      return 0;
    }

    tl::MutexLocker locker (&m_lock);

    auto m = m_id_to_string.find (s);
    if (m == m_id_to_string.end ()) {
      m_strings.push_back (s);
      size_t id = m_strings.size ();
      m_id_to_string.insert (std::make_pair (s, id));
      return id;
    } else {
      return m->second;
    }
  }

  const std::string &string_for_id (size_t id) const
  {
    if (id == 0) {
      static const std::string empty;
      return empty;
    }

    tl::MutexLocker locker (&m_lock);
    return m_strings [id - 1];
  }

private:
  mutable tl::Mutex m_lock;
  std::vector<std::string> m_strings;
  std::map<std::string, size_t> m_id_to_string;
};

static LogEntryStringRepository s_strings;

// ------------------------------------------------------------------
//  LogEntryData implementation

LogEntryData::LogEntryData ()
  : m_severity (NoSeverity), m_cell_name (0), m_message (0), m_category_name (0), m_category_description (0)
{
  //  .. nothing yet ..
}

LogEntryData::LogEntryData (Severity s, const std::string &msg)
  : m_severity (s), m_cell_name (0), m_message (s_strings.id_for_string (msg)), m_category_name (0), m_category_description (0)
{
  //  .. nothing yet ..
}

LogEntryData::LogEntryData (Severity s, const std::string &cell_name, const std::string &msg)
  : m_severity (s), m_cell_name (s_strings.id_for_string (cell_name)), m_message (s_strings.id_for_string (msg)), m_category_name (0), m_category_description (0)
{
  //  .. nothing yet ..
}

bool
LogEntryData::operator== (const LogEntryData &other) const
{
  return m_severity == other.m_severity &&
         m_message == other.m_message &&
         m_cell_name == other.m_cell_name &&
         m_geometry == other.m_geometry &&
         m_category_name == other.m_category_name &&
         m_category_description == other.m_category_description;
}

const std::string &
LogEntryData::category_name () const
{
  return s_strings.string_for_id (m_category_name);
}

void
LogEntryData::set_category_name (const std::string &s)
{
  m_category_name = s_strings.id_for_string (s);
}

const std::string &
LogEntryData::category_description () const
{
  return s_strings.string_for_id (m_category_description);
}

void
LogEntryData::set_category_description (const std::string &s)
{
  m_category_description = s_strings.id_for_string (s);
}

const std::string &
LogEntryData::message () const
{
  return s_strings.string_for_id (m_message);
}

void
LogEntryData::set_message (const std::string &n)
{
  m_message = s_strings.id_for_string (n);
}

const std::string &
LogEntryData::cell_name () const
{
  return s_strings.string_for_id (m_cell_name);
}

void
LogEntryData::set_cell_name (const std::string &n)
{
  m_cell_name = s_strings.id_for_string (n);
}

std::string
LogEntryData::to_string (bool with_geometry) const
{
  std::string res;

  if (m_category_name != 0) {
    if (m_category_description == 0) {
      res += "[" + category_name () + "] ";
    } else {
      res += "[" + category_description () + "] ";
    }
  }

  if (m_cell_name != 0) {
    res += tl::to_string (tr ("In cell "));
    res += cell_name ();
    res += ": ";
  }

  res += message ();

  if (with_geometry && ! m_geometry.box ().empty ()) {
    res += tl::to_string (tr (", shape: ")) + m_geometry.to_string ();
  }

  return res;
}

}
