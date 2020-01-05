
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include "tlRecipe.h"
#include "tlString.h"

namespace tl
{

Recipe::Recipe (const std::string &name, const std::string &description)
  : tl::RegisteredClass<tl::Recipe> (this, 0, name.c_str (), false)
{
  m_name = name;
  m_description = description;
}

std::string Recipe::generator (const std::map<std::string, tl::Variant> &params)
{
  std::string g;
  g += tl::to_word_or_quoted_string (name ());
  g += ": ";

  for (std::map<std::string, tl::Variant>::const_iterator p = params.begin (); p != params.end (); ++p) {
    if (p != params.begin ()) {
      g += ",";
    }
    g += tl::to_word_or_quoted_string (p->first);
    g += "=";
    g += p->second.to_parsable_string ();
  }

  return g;
}

tl::Variant Recipe::make (const std::string &generator, const std::map<std::string, tl::Variant> &padd)
{
  tl::Extractor ex (generator.c_str ());

  std::string recipe;
  ex.read_word_or_quoted (recipe);
  ex.test (":");

  std::map<std::string, tl::Variant> params;
  while (! ex.at_end ()) {
    std::string key;
    ex.read_word_or_quoted (key);
    ex.test ("=");
    tl::Variant v;
    ex.read (v);
    ex.test (",");
    params.insert (std::make_pair (key, v));
  }

  for (std::map<std::string, tl::Variant>::const_iterator p = padd.begin (); p != padd.end (); ++p) {
    params.insert (*p);
  }

  tl::Recipe *recipe_obj = 0;
  for (tl::Registrar<tl::Recipe>::iterator r = tl::Registrar<tl::Recipe>::begin (); r != tl::Registrar<tl::Recipe>::end (); ++r) {
    if (r->name () == recipe) {
      recipe_obj = r.operator-> ();
    }
  }

  if (! recipe_obj) {
    return tl::Variant ();
  } else {
    return recipe_obj->execute (params);
  }
}

} // namespace tl
