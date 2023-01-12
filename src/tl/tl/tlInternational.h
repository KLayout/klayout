
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


#ifndef HDR_tlInternational
#define HDR_tlInternational

#include "tlCommon.h"

#include <string>

#if defined(HAVE_QT)
# include <QString>
//  provides QObject for tr
# include <QObject>
#endif

/**
 *  @brief Generic tr function for non-Qt and Qt builds
 */
#if defined(HAVE_QT)
inline QString tr (const char *s)
{
  return QObject::tr (s);
}
#else
std::string TL_PUBLIC tr (const char *s);
#endif

namespace tl
{

#if defined(HAVE_QT)
/**
 *  @brief Convert a UTF8 std::string to a QString
 */
TL_PUBLIC QString to_qstring (const std::string &s);

/**
 *  @brief Convert a QString to a UTF8 std::string
 */
TL_PUBLIC std::string to_string (const QString &s);
#endif

/**
 *  @brief Dummy conversion for convenience and as non-Qt replacement for to_string (const QString &)
 */
inline const std::string &to_string (const std::string &s)
{
  return s;
}

#ifndef _WIN32
/**
 *  @brief Convert a system encoding std::string to a UTF8 std::string
 */
TL_PUBLIC std::string system_to_string (const std::string &s);

/**
 *  @brief Convert a UTF8 string to a system encoding string
 */
TL_PUBLIC std::string string_to_system (const std::string &s);
#endif

/**
 *  @brief Initialize the codecs
 */
TL_PUBLIC void initialize_codecs ();

}

#endif

