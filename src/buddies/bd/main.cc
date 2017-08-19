
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "bdInit.h"

BD_PUBLIC int BD_TARGET (int argc, char *argv []);

/**
 *  @brief Provides a main () implementation
 *
 *  NOTE:
 *  This file is not part of the bd sources, but the template for the
 *  main() function of the various applications. It's configured through the
 *  BD_TARGET macro which is set to the application name in the app's .pro
 *  files.
 */
int main (int argc, char *argv [])
{
  return bd::_main_impl (&BD_TARGET, argc, argv);
}
