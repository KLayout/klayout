
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



#ifndef HDR_dbObjectTag
#define HDR_dbObjectTag

namespace db
{

/**
 *  @brief A "shape tag" class
 * 
 *  This class is just a "tag" (a class without a member).
 *  It just provides a typedef by which a shape type
 *  can be retrieved. It's only purpose is to pass a 
 *  shape type without passing an actual shape object.
 */

template <class Obj>
struct object_tag
{
  typedef Obj object_type;
};

}

#endif

