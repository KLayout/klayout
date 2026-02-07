
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

#if defined(HAVE_QT)

#ifndef HDR_layBusy
#define HDR_layBusy

#include "layuiCommon.h"

namespace lay
{

/**
 *  @brief An interface providing the "busy" methods
 *
 *  There must be one provider implementing this interface.
 */
class LAYUI_PUBLIC BusyMode
{
public:
  BusyMode ();
  virtual ~BusyMode ();

  virtual bool is_busy () const = 0;
  virtual void enter_busy_mode (bool bm) = 0;
};

/**
 *  @brief A RAII implementation of the busy mode setter
 */
class LAYUI_PUBLIC BusySection
{
public:
  BusySection ();
  ~BusySection ();

  static bool is_busy ();

private:
  bool m_previous_mode;
  BusyMode *mp_busy_mode;
};

}

#endif

#endif  //  defined(HAVE_QT)
