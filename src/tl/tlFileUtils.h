
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

#ifndef HDR_tlFileUtils
#define HDR_tlFileUtils

#include "tlCommon.h"
#include "tlString.h"
#include <QString>

namespace tl
{

/**
 *  @brief Returns a value indicating whether the parent path is a parent directory of the path
 */
bool TL_PUBLIC is_parent_path (const QString &parent, const QString &path);

/**
 *  @brief Returns a value indicating whether the parent path is a parent directory of the path (version with std::string)
 */
inline bool TL_PUBLIC is_parent_path (const std::string &parent, const std::string &path)
{
  return is_parent_path (tl::to_qstring (parent), tl::to_qstring (path));
}

/**
 *  @brief Recursively remove the given directory, the files from that directory and all sub-directories
 *  @return True, if successful. false otherwise.
 */
bool TL_PUBLIC rm_dir_recursive (const QString &path);

/**
 *  @brief Recursively remove the given directory, the files from that directory and all sub-directories (version with std::string)
 *  @return True, if successful. false otherwise.
 */
inline bool TL_PUBLIC rm_dir_recursive (const std::string &path)
{
  return rm_dir_recursive (tl::to_qstring (path));
}

/**
 *  @brief Recursively copies a given directory to a target directory
 *  Both target and source directories need to exist. New directories are created in the target
 *  directory if required.
 *  @return True, if successful. false otherwise.
 */
bool TL_PUBLIC cp_dir_recursive (const QString &source, const QString &target);

/**
 *  @brief Recursively remove the given directory, the files from that directory and all sub-directories (version with std::string)
 *  @return True, if successful. false otherwise.
 */
inline bool TL_PUBLIC cp_dir_recursive (const std::string &source, const std::string &target)
{
  return cp_dir_recursive (tl::to_qstring (source), tl::to_qstring (target));
}

}

#endif
