
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

#ifndef HDR_tlStringEx
#define HDR_tlStringEx

#include "tlString.h"

#include <set>
#include <map>
#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>

namespace tl
{

template <class Iter>
std::string to_string (Iter b, Iter e)
{
  std::string res;
  for (Iter i = b; i != e; ++i) {
    if (i != b) {
      res += ",";
    }
    res += tl::to_string (*i);
  }
  return res;
}

template <class T1, class T2>
std::string to_string (const std::pair<T1, T2> &p)
{
  return to_string (p.first) + "," + to_string (p.second);
}

template <class T>
std::string to_string (const std::vector<T> &v)
{
  return to_string (v.begin (), v.end ());
}

template <class T>
std::string to_string (const std::list<T> &v)
{
  return to_string (v.begin (), v.end ());
}

template <class T>
std::string to_string (const std::set<T> &v)
{
  return to_string (v.begin (), v.end ());
}

template <class T, class V>
std::string to_string (const std::map<T, V> &v)
{
  return to_string (v.begin (), v.end ());
}

template <class T>
std::string to_string (const std::unordered_set<T> &v)
{
  return to_string (v.begin (), v.end ());
}

template <class T, class V>
std::string to_string (const std::unordered_map<T, V> &v)
{
  return to_string (v.begin (), v.end ());
}

}

#endif
