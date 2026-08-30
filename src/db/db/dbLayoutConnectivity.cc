
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "dbLayoutConnectivity.h"
#include "tlVariant.h"

namespace db
{

db::property_names_id_type pin_property_name_id = db::property_names_id ("klayout:pin");

// ---------------------------------------------------------------------------
//  LayoutPin class

LayoutPin::LayoutPin ()
  : m_name (), m_must_connect (false)
{
  //  .. nothing yet ..
}

LayoutPin::LayoutPin (const std::string &name, bool must_connect)
  : m_name (name), m_must_connect (must_connect)
{
  //  .. nothing yet ..
}

void
LayoutPin::set_name (const std::string &n)
{
  m_name = n;
}

void
LayoutPin::set_must_connect (bool mc)
{
  m_must_connect = mc;
}

//  NOTE: serialization uses the generic tl::Variant dict
//  format with a number of keys. This format will be
//  compatible to future enhancements.

static const std::string lp_name_key ("name");
static const std::string lp_must_connect_key ("must_connect");

std::string
LayoutPin::to_string () const
{
  tl::Variant value = tl::Variant::empty_array ();
  value.insert (lp_name_key, m_name);
  if (m_must_connect) {
    value.insert (lp_must_connect_key, m_must_connect);
  }
  return value.to_parsable_string ();
}

bool
LayoutPin::parse (tl::Extractor &ex)
{
  tl::Variant dict;
  ex.read (dict);
  if (! dict.is_array ()) {
    return false;
  }

  m_name.clear ();
  m_must_connect = false;

  const auto *nv = dict.find (lp_name_key);
  if (nv) {
    m_name = nv->to_string ();
  }

  const auto *mcv = dict.find (lp_must_connect_key);
  if (mcv) {
    m_must_connect = mcv->to_bool ();
  }

  return true;
}

}

