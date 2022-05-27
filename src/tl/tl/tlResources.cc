
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
    const unsigned char *data;
    size_t data_size;
    bool compressed;
  };

  ResourceDict () { }

  resource_id_type add (const char *name, bool compressed, const unsigned char *data, size_t data_size)
  {
    m_dict[std::string (name)] = m_entries.size ();
    m_entries.push_back (DictEntry ());
    m_entries.back ().data = data;
    m_entries.back ().data_size = data_size;
    m_entries.back ().compressed = compressed;
    return m_entries.size () - 1;
  }

  void remove (resource_id_type id)
  {
    if (id < m_entries.size ()) {
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

tl::InputStream *get_resource (const char *name)
{
  if (! ms_dict) {
    return 0;
  }

  ResourceDict::DictEntry *entry = ms_dict->entry (name);
  if (! entry || ! entry->data) {
    return 0;
  }

  if (entry->compressed) {

    tl_assert (entry->data_size > 6);

    //  NOTE: zlib compression (used in pyqrc) adds two bytes header before the data block and
    //  4 bytes after (CRC32)
    auto stream = new tl::InputStream (new tl::InputMemoryStream ((const char *) entry->data + 2, entry->data_size - 6));
    stream->inflate ();
    return stream;

  } else {

    //  raw data
    return new tl::InputStream (new tl::InputMemoryStream ((const char *) entry->data, entry->data_size));

  }
}

}

