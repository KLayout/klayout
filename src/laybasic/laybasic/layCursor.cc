
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

#if defined(HAVE_QT)

#include "layCursor.h"

#include <QCursor>

namespace lay
{

QCursor 
Cursor::qcursor (cursor_shape cursor)
{
  switch (int (cursor)) {
  case 0:
    return QCursor (Qt::ArrowCursor);
  case 1:
    return QCursor (Qt::UpArrowCursor);
  case 2:
    return QCursor (Qt::CrossCursor);
  case 3:
    return QCursor (Qt::WaitCursor);
  case 4:
    return QCursor (Qt::IBeamCursor);
  case 5:
    return QCursor (Qt::SizeVerCursor);
  case 6:
    return QCursor (Qt::SizeHorCursor);
  case 7:
    return QCursor (Qt::SizeBDiagCursor);
  case 8:
    return QCursor (Qt::SizeFDiagCursor);
  case 9:
    return QCursor (Qt::SizeAllCursor);
  case 10:
    return QCursor (Qt::BlankCursor);
  case 11:
    return QCursor (Qt::SplitVCursor);
  case 12:
    return QCursor (Qt::SplitHCursor);
  case 13:
    return QCursor (Qt::PointingHandCursor);
  case 14:
    return QCursor (Qt::ForbiddenCursor);
  case 15:
    return QCursor (Qt::WhatsThisCursor);
  case 16:
    return QCursor (Qt::BusyCursor);
  case 17:
    return QCursor (Qt::OpenHandCursor);
  case 18:
    return QCursor (Qt::ClosedHandCursor);
  default:
    return QCursor (Qt::BlankCursor);
  };
}

}

#endif
