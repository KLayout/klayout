
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

 
#include "tlClassRegistry.h"

#include <map>

namespace tl
{

struct ti_compare_f
{
  bool operator() (const std::type_info *a, const std::type_info *b) const
  {
    return a->before (*b);
  }
};

typedef std::map<const std::type_info *, RegistrarBase *, ti_compare_f> inst_map_type;
static inst_map_type s_inst_map;

TL_PUBLIC void set_registrar_instance_by_type (const std::type_info &ti, RegistrarBase *rb)
{
  if (rb) {
    s_inst_map[&ti] = rb;
  } else {
    s_inst_map.erase (&ti);
  }
}

TL_PUBLIC RegistrarBase *registrar_instance_by_type (const std::type_info &ti)
{
  inst_map_type::const_iterator im = s_inst_map.find (&ti);
  if (im != s_inst_map.end ()) {
    return im->second;
  } else {
    return 0;
  }
}

}
