
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_laySignalHandler
#define HDR_laySignalHandler

#include <QString>
#include "layCommon.h"

namespace lay
{

/**
 *  @brief Installs global signal handlers for SIGSEGV and similar
 */
void install_signal_handlers ();

/**
 *  @brief For debugging purposes: get the symbol name from a memory address
 */
QString get_symbol_name_from_address (const QString &mod_name, size_t addr);

/**
 *  @brief Enables GUI support for signal handlers
 */
void LAY_PUBLIC enable_signal_handler_gui (bool en);

}

#endif
