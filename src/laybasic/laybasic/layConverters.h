
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


#ifndef HDR_layConverters
#define HDR_layConverters

#include "laybasicCommon.h"
#include "tlColor.h"

#if defined(HAVE_QT)
#  include <QColor>
#endif

namespace lay
{

/**
 *  @brief A color converter class for converting colors to strings and vice versa
 */
struct LAYBASIC_PUBLIC ColorConverter 
{
#if defined(HAVE_QT)
  std::string to_string (const QColor &c) const;
  void from_string (const std::string &s, QColor &c) const;
#endif
  std::string to_string (const tl::Color &c) const;
  void from_string (const std::string &s, tl::Color &c) const;
};

}

#endif

