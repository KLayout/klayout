
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

#ifndef HDR_layCursor
#define HDR_layCursor

#include "laybasicCommon.h"

#if defined(HAVE_QT)
class QCursor;
#endif

namespace lay
{

/**
 *  @brief A wrapper for the cursor definitions
 */
struct LAYBASIC_PUBLIC Cursor
{
  /**
   *  @brief An enumeration of all cursors defined
   */
  enum cursor_shape
  {
    keep = -2,
    none = -1,
    arrow = 0,
    up_arrow = 1,
    cross = 2,
    wait = 3,
    i_beam = 4,
    size_ver = 5,
    size_hor = 6,
    size_bdiag = 7,
    size_fdiag = 8,
    size_all = 9,
    blank = 10,
    split_v = 11,
    split_h = 12,
    pointing_hand = 13,
    forbidden = 14,
    whats_this = 15,
    busy = 16,
    open_hand = 17,
    closed_hand = 18
  };

#if defined(HAVE_QT)
  /**
   *  @brief Get the QCursor from the lay::cursor_shape enum
   */
  static QCursor qcursor (cursor_shape cursor);
#endif

};

}

#endif
