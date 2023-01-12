
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

#include "dbNamedLayerReader.h"

namespace db
{

// -------------------------------------------------------------------------
//  safe versions (assertion-less) of safe_isdigit, safe_isprint, safe_isalpha, safe_isalnum
//  (required for debug mode of MSVC)

inline bool safe_isdigit (char c)
{
  return c != 0 && static_cast<unsigned char> (c) < 0x80 && isdigit (c);
}

inline bool safe_isspace (char c)
{
  return c != 0 && static_cast<unsigned char> (c) < 0x80 && isspace (c);
}

// ---------------------------------------------------------------
//  NamedLayerReader

NamedLayerReader::NamedLayerReader ()
  : m_create_layers (true), m_keep_layer_names (false), m_next_layer_index (0)
{
  //  .. nothing yet ..
}

void
NamedLayerReader::set_create_layers (bool f)
{
  m_create_layers = f;
}

void
NamedLayerReader::set_keep_layer_names (bool f)
{
  m_keep_layer_names = f;
}

void NamedLayerReader::set_layer_map (const LayerMap &lm)
{
  m_layer_map = lm;
}

static bool
extract_plain_layer (const char *s, int &l)
{
  l = 0;
  if (! *s) {
    return false;
  }
  while (safe_isdigit (*s)) {
    l = l * 10 + static_cast<int> (*s - '0');
    ++s;
  }
  return (*s == 0);
}

static bool
extract_ld (const char *s, int &l, int &d, std::string &n)
{
  l = d = 0;

  if (*s == 'L' || *s == 'l') {
    ++s;
  }

  if (! safe_isdigit (*s)) {
    return false;
  }

  while (safe_isdigit (*s)) {
    l = l * 10 + static_cast<int> (*s - '0');
    ++s;
  }

  if (*s == 'D' || *s == 'd' || *s == '.') {
    ++s;
    if (! safe_isdigit (*s)) {
      return false;
    }
    while (safe_isdigit (*s)) {
      d = d * 10 + static_cast<int> (*s - '0');
      ++s;
    }
  }

  if (safe_isspace (*s) || *s == '_') {
    ++s;
    n = s;
    return true;
  } else if (*s == 0) {
    n.clear ();
    return true;
  } else {
    return false;
  }
}

std::pair <bool, unsigned int>
NamedLayerReader::open_layer (db::Layout &layout, const std::string &n)
{
  return open_layer (layout, n, keep_layer_names (), create_layers ());
}

std::pair <bool, unsigned int>
NamedLayerReader::open_layer (db::Layout &layout, const std::string &n, bool keep_layer_name, bool create_layer)
{
  std::map<std::string, std::pair <bool, unsigned int> >::const_iterator lc = m_layer_cache.find (n);
  if (lc != m_layer_cache.end ()) {
    return lc->second;
  } else {
    std::pair <bool, unsigned int> res = open_layer_uncached (layout, n, keep_layer_name, create_layer);
    m_layer_cache.insert (std::make_pair (n, res));
    return res;
  }
}

std::pair <bool, unsigned int>
NamedLayerReader::open_layer_uncached (db::Layout &layout, const std::string &n, bool keep_layer_name, bool create_layer)
{
  int l = -1, d = -1;
  std::string on;

  std::set<unsigned int> li = m_layer_map.logical (n, layout);
  if (li.empty () && ! keep_layer_name) {

    if (extract_plain_layer (n.c_str (), l)) {

      db::LayerProperties lp;
      lp.layer = l;
      lp.datatype = 0;
      li = m_layer_map.logical (lp, layout);

    } else if (extract_ld (n.c_str (), l, d, on)) {

      db::LayerProperties lp;
      lp.layer = l;
      lp.datatype = d;
      lp.name = on;
      li = m_layer_map.logical (lp, layout);

    }

  }

  if (! li.empty ()) {

    for (std::set<unsigned int>::const_iterator i = li.begin (); i != li.end (); ++i) {
      m_layer_map_out.mmap (n, *i, layout.get_properties (*i));
    }

    if (li.size () == 1) {

      return std::make_pair (true, *li.begin ());

    } else {

      std::map<std::set<unsigned int>, unsigned int>::iterator mmp = m_multi_mapping_placeholders.find (li);
      if (mmp == m_multi_mapping_placeholders.end ()) {
        //  create a placeholder layer for later
        mmp = m_multi_mapping_placeholders.insert (std::make_pair (li, layout.insert_layer ())).first;
      }

      return std::make_pair (true, mmp->second);

    }

  } else if (! create_layer) {

    return std::pair<bool, unsigned int> (false, 0);

  } else {

    std::map <std::string, unsigned int>::const_iterator nl = m_new_layers.find (n);
    if (nl == m_new_layers.end ()) {

      unsigned int ll;
      do {
        ll = m_next_layer_index++;
      } while (! layout.is_free_layer (ll));

      layout.insert_layer (ll, db::LayerProperties ());
      m_new_layers.insert (std::make_pair (n, ll));

      return std::pair<bool, unsigned int> (true, ll);

    } else {
      return std::pair<bool, unsigned int> (true, nl->second);
    }

  }
}

void
NamedLayerReader::map_layer (const std::string &name, unsigned int layer)
{
  m_layer_cache [name] = std::make_pair (true, layer);
  m_layer_map_out.map (name, layer);
}

void
NamedLayerReader::prepare_layers (db::Layout &layout)
{
  m_new_layers.clear ();
  m_next_layer_index = m_layer_map.next_index ();

  m_layer_map_out.clear ();
  m_multi_mapping_placeholders.clear ();
  m_layer_cache.clear ();

  m_layer_map.prepare (layout);
}

void
NamedLayerReader::finish_layers (db::Layout &layout)
{
  //  resolve layer multi-mapping

  for (std::map<std::set<unsigned int>, unsigned int>::const_iterator i = m_multi_mapping_placeholders.begin (); i != m_multi_mapping_placeholders.end (); ++i) {

    if (i->first.size () > 1) {

      bool discard_layer = i->first.find (i->second) == i->first.end ();

      for (std::set<unsigned int>::const_iterator l = i->first.begin (); l != i->first.end (); ++l) {

        //  last one? this one will get a "move"
        std::set<unsigned int>::const_iterator ll = l;
        if (discard_layer && ++ll == i->first.end ()) {
          layout.move_layer (i->second, *l);
          layout.delete_layer (i->second);
        } else {
          layout.copy_layer (i->second, *l);
        }

      }

    }

  }

  //  assign layer numbers to new layers
  if (! m_new_layers.empty () && ! m_keep_layer_names) {

    std::set<std::pair<int, int> > used_ld;
    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
      used_ld.insert (std::make_pair((*l).second->layer, (*l).second->datatype));
    }

    //  assign fixed layer numbers for all layers whose name is a fixed number unless there is already a layer with that number
    for (std::map<std::string, unsigned int>::iterator i = m_new_layers.begin (); i != m_new_layers.end (); ) {

      std::map<std::string, unsigned int>::iterator ii = i;
      ++ii;

      int l = -1;
      if (extract_plain_layer (i->first.c_str (), l) && used_ld.find (std::make_pair (l, 0)) == used_ld.end ()) {

        used_ld.insert (std::make_pair (l, 0));

        db::LayerProperties lp;
        lp.layer = l;
        lp.datatype = 0;
        layout.set_properties (i->second, lp);
        m_layer_map.map (lp, i->second);

        m_new_layers.erase (i);

      }

      i = ii;

    }

    //  assign fixed layer numbers for all layers whose name is a LxDy or Lx notation unless there is already a layer with that layer/datatype
    for (std::map<std::string, unsigned int>::iterator i = m_new_layers.begin (); i != m_new_layers.end (); ) {

      std::map<std::string, unsigned int>::iterator ii = i;
      ++ii;

      int l = -1, d = -1;
      std::string n;

      if (extract_ld (i->first.c_str (), l, d, n) && used_ld.find (std::make_pair (l, d)) == used_ld.end ()) {

        used_ld.insert (std::make_pair (l, d));

        db::LayerProperties lp;
        lp.layer = l;
        lp.datatype = d;
        lp.name = n;
        layout.set_properties (i->second, lp);
        m_layer_map.map (lp, i->second);

        m_new_layers.erase (i);

      }

      i = ii;

    }

  }

  //  insert the remaining ones
  for (std::map<std::string, unsigned int>::const_iterator i = m_new_layers.begin (); i != m_new_layers.end (); ++i) {
    db::LayerProperties lp;
    lp.name = i->first;
    layout.set_properties (i->second, lp);
    m_layer_map.map (lp, i->second);
  }
}

}
