
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

#ifndef _HDR_rbaInspector
#define _HDR_rbaInspector

namespace gsi
{
  class Inspector;
}

namespace rba
{

/**
 *  @brief Creates an inspector for the given context level above the current frame
 *
 *  Returns 0 if no inspector is available. Otherwise a new'd inspector object is returned.
 *  the caller is responsible for destroying that object.
 */
gsi::Inspector *create_inspector (int context);

}

#endif

