
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

#include "tlResources.h"
#include "tlGlobPattern.h"

#include <map>
#include <vector>

namespace tl
{

namespace {

class ResourceDict
{
public:
  struct DictEntry
  {
    std::string name;
    const unsigned char *data;
    size_t data_size;
    bool compressed;
  };

  typedef std::vector<DictEntry>::const_iterator iterator;

  ResourceDict () { }

  resource_id_type add (const char *name, bool compressed, const unsigned char *data, size_t data_size)
  {
    m_dict[std::string (name)] = m_entries.size ();
    m_entries.push_back (DictEntry ());
    m_entries.back ().name = name;
    m_entries.back ().data = data;
    m_entries.back ().data_size = data_size;
    m_entries.back ().compressed = compressed;
    return m_entries.size () - 1;
  }

  void remove (resource_id_type id)
  {
    if (id < m_entries.size ()) {
      m_entries [id].name.clear ();
      m_entries [id].data = 0;
      m_entries [id].data_size = 0;
    }
  }

  DictEntry *entry (const char *name)
  {
    auto i = m_dict.find (std::string (name));
    if (i != m_dict.end () && i->second < m_entries.size ()) {
      return &m_entries [i->second];
    } else {
      return 0;
    }
  }

  iterator begin ()
  {
    return m_entries.begin ();
  }

  iterator end ()
  {
    return m_entries.end ();
  }

private:
  std::map<std::string, resource_id_type> m_dict;
  std::vector<DictEntry> m_entries;
};

}

static ResourceDict *ms_dict = 0;

resource_id_type register_resource (const char *name, bool compressed, const unsigned char *data, size_t data_size)
{
  if (! ms_dict) {
    ms_dict = new ResourceDict ();
  }
  return ms_dict->add (name, compressed, data, data_size);
}

void unregister_resource (size_t id)
{
  if (ms_dict) {
    ms_dict->remove (id);
  }
}

std::pair<tl::InputStreamBase *, bool> get_resource_reader (const char *name)
{
  if (! ms_dict) {
    return std::pair<tl::InputStreamBase *, bool> (0, false);
  }

  ResourceDict::DictEntry *entry = ms_dict->entry (name);
  if (! entry || ! entry->data) {
    return std::pair<tl::InputStreamBase *, bool> (0, false);
  }

  if (entry->compressed) {

    tl_assert (entry->data_size > 6);

    //  NOTE: zlib compression (used in pyqrc) adds two bytes header before the data block and
    //  4 bytes after (CRC32)
    return std::make_pair (new tl::InputMemoryStream ((const char *) entry->data + 2, entry->data_size - 6), true);

  } else {

    //  raw data
    return std::make_pair (new tl::InputMemoryStream ((const char *) entry->data, entry->data_size), false);

  }

}

tl::InputStream *get_resource (const char *name)
{
  std::pair<tl::InputStreamBase *, bool> rr = get_resource_reader (name);
  if (! rr.first) {
    return 0;
  } else {
    auto stream = new tl::InputStream (rr.first);
    if (rr.second) {
      stream->inflate_always ();
    }
    return stream;
  }
}

std::vector<std::string>
find_resources (const std::string &pattern)
{
  if (! ms_dict) {
    return std::vector<std::string> ();
  }

  std::vector<std::string> res;
  tl::GlobPattern p (pattern);

  for (ResourceDict::iterator i = ms_dict->begin (); i != ms_dict->end (); ++i) {
    if (i->data && p.match (i->name)) {
      res.push_back (i->name);
    }
  }

  return res;
}

}

