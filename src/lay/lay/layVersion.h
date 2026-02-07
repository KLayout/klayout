
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


#ifndef HDR_layVersion
#define HDR_layVersion

#include "layCommon.h"

namespace lay {

/**
 *  @brief This class provides the program's version information
 */

class LAY_PUBLIC Version
{
public:
  static const char *exe_name ();
  static const char *name ();
  static const char *version ();
  static const char *subversion ();
  static const char *about_text ();

  static void set_exe_name (const char *s);
  static void set_name (const char *s);
  static void set_version (const char *s);
  static void set_subversion (const char *s);
  static void set_about_text (const char *s);
};

}

#endif

