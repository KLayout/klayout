
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


#ifndef HDR_tlSleep
#define HDR_tlSleep

#include "tlCommon.h"

namespace tl
{

/**
 *  @brief Sleeps the given number of microseconds
 */
TL_PUBLIC void usleep (unsigned long us);

/**
 *  @brief Sleeps the given number of milliseconds
 */
TL_PUBLIC void msleep (unsigned long ms);

} // namespace tl

#endif

