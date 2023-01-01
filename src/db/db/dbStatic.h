
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


#ifndef HDR_dbStatic
#define HDR_dbStatic

#include "dbCommon.h"

namespace db
{

// -----------------------------------------------------------
//  editable mode

/**
 *  @brief Global database attribute: editable mode
 *
 *  This attribute reflects editable mode. In editable mode, some restrictions apply, i.e.
 *  only shapes with attributes may be created.
 */
inline bool default_editable_mode ()
{
  extern DB_PUBLIC bool ms_editable;
  return ms_editable;
}

/**
 *  @brief Set editable mode
 *
 *  Hint: this should only be done initially, not at runtime.
 */
void DB_PUBLIC set_default_editable_mode (bool editable);

// -----------------------------------------------------------
//  circle resolution

/**
 *  @brief Returns the number of points per full circle 
 *
 *  This value is used as default in some places.
 */
inline unsigned int num_circle_points () 
{
  extern DB_PUBLIC unsigned int ms_num_circle_points;
  return ms_num_circle_points < 4 ? 4 : ms_num_circle_points;
}

/**
 *  @brief Sets the number of points per full circle.
 */
void DB_PUBLIC set_num_circle_points (unsigned int n);

}

#endif

