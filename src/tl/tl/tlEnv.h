
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


#ifndef HDR_tlAppFlags
#define HDR_tlAppFlags

#include "tlCommon.h"

#include <string>

namespace tl
{

/**
 *  @brief Gets the value for the given environment variable
 *
 *  If the environment variable is not set, the default value will be returned.
 */
std::string TL_PUBLIC get_env (const std::string &name, const std::string &def_value = std::string ());

/**
 *  @brief Gets the value if the given environment variable is set
 */
bool TL_PUBLIC has_env (const std::string &name);

/**
 *  @brief Gets an application flag with the given name
 *
 *  This feature is supposed to deliver special flags for debugging etc.
 *  By using a central access point for these settings, it will be easy to provide
 *  different implementations later.
 *
 *  Currently, application flags are derived by checking whether a corresponding
 *  environment variable exists. Names like "a-b" are translated into "KLAYOUT_A_B"
 *  for the environment variable.
 *
 *  If the corresponding variable exists and does not have a value of "0", true
 *  is returned from this function.
 */
bool TL_PUBLIC app_flag (const std::string &name);

}

#endif

