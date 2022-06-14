
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

namespace tl
{

/**
 *  @brief Returns a value indicating whether the parent path is a parent directory of the path (version with std::string)
 */
bool TL_PUBLIC is_parent_path (const std::string &parent, const std::string &path);

/**
 *  @brief Returns true if s is an absolute path
 */
bool TL_PUBLIC is_absolute (const std::string &s);

/**
 *  @brief Recursively remove the given directory, the files from that directory and all sub-directories (version with std::string)
 *  @return True, if successful. false otherwise.
 */
bool TL_PUBLIC rm_dir_recursive (const std::string &path);

/**
 *  @brief Creates the given path as far it does not exist yet
 *  @return True, if successful. false otherwise.
 */
bool TL_PUBLIC mkpath (const std::string &path);

/**
 *  @brief Recursively remove the given directory, the files from that directory and all sub-directories (version with std::string)
 *  @return True, if successful. false otherwise.
 */
bool TL_PUBLIC cp_dir_recursive (const std::string &source, const std::string &target);

/**
 *  @brief Gets the absolute path for a given file path
 *  This will deliver the directory of the file as absolute path.
 */
std::string TL_PUBLIC absolute_path (const std::string &s);

/**
 *  @brief Gets the absolute file path for a given file path
 *  This will deliver the directory and the file part as absolute path.
 */
std::string TL_PUBLIC absolute_file_path (const std::string &s);

/**
 *  @brief Gets the directory name for a given file path
 */
std::string TL_PUBLIC dirname (const std::string &s);

/**
 *  @brief Gets the filename for a given file path (file name without directory part)
 */
std::string TL_PUBLIC filename (const std::string &s);

/**
 *  @brief Gets the basename for a given file path (file name without any extensions)
 *  This will strip all extensions (i.e. "archive.tar.gz" will become "archive").
 */
std::string TL_PUBLIC basename (const std::string &s);

/**
 *  @brief Gets the basename for a given file path (file name without the last extensions)
 *  This will strip all extensions (i.e. "archive.tar.gz" will become "archive.tar").
 */
std::string TL_PUBLIC complete_basename (const std::string &s);

/**
 *  @brief Gets the complete extension for a given file path
 */
std::string TL_PUBLIC extension (const std::string &s);

/**
 *  @brief Gets the last extension for a given file path
 */
std::string TL_PUBLIC extension_last (const std::string &s);

/**
 *  @brief Returns true, if the given path exists
 *  If the path is a directory, file_exists will return true, if the directory exists.
 */
bool TL_PUBLIC file_exists (const std::string &s);

/**
 *  @brief Returns true, if the given path is writable
 */
bool TL_PUBLIC is_writable (const std::string &s);

/**
 *  @brief Returns true, if the given path is readable
 */
bool TL_PUBLIC is_readable (const std::string &s);

/**
 *  @brief Returns true, if the given path is a directory
 */
bool TL_PUBLIC is_dir (const std::string &s);

/**
 *  @brief Gets the directory entries for the given directory
 *  This method will NEVER return the ".." entry.
 *  @param with_files Includes all files
 *  @param with_dirs Include all directories
 *  @param without_dotfiles Exclude all ".*" files
 */
std::vector<std::string> TL_PUBLIC dir_entries (const std::string &s, bool with_files = true, bool with_dirs = true, bool without_dotfiles = false);

/**
 *  @brief Rename the given file
 */
bool TL_PUBLIC rename_file (const std::string &path, const std::string &new_name);

/**
 *  @brief Removes the given file and returns true on success
 */
bool TL_PUBLIC rm_file (const std::string &path);

/**
 *  @brief Removes the given directory and returns true on success
 */
bool TL_PUBLIC rm_dir (const std::string &path);

/**
 *  @brief Returns true, if the given path is the same directory of file than the other one
 */
bool TL_PUBLIC is_same_file (const std::string &a, const std::string &b);

/**
 *  @brief Gets the relative path of p vs. base
 *  If p cannot be resolved relative to base, p is returned.
 */
std::string TL_PUBLIC relative_path (const std::string &base, const std::string &p);

/**
 *  @brief Normalizes the path
 *  This function will remove duplicate "/" or "\" and strip any trailing
 *  "/" or "\".
 */
std::string TL_PUBLIC normalize_path (const std::string &s);

/**
 *  @brief Combines the two path components into one path
 *  If "always_join" is true, the path is also built if p2 is empty. This will
 *  essentially add a slash or backslash to p1.
 */
std::string TL_PUBLIC combine_path (const std::string &p1, const std::string &p2, bool always_join = false);

/**
 *  @brief Gets the current directory
 */
std::string TL_PUBLIC current_dir ();

/**
 *  @brief Change the current directory and returns true if the change was successful
 */
bool TL_PUBLIC chdir (const std::string &path);

/**
 *  @brief This function splits the path into it's components
 *  On Windows, the first component may be the drive prefix ("C:") or
 *  UNC server name ("\\server").
 *  The components will keep their path separator, so joining the
 *  parts will render the original path. A trailing empty element is
 *  added if the path terminates with a separator (like "C:\" or "/home/user/").
 *  The idea is that the last element is the file name part.
 *  If "keep_last" is true, the last part will be kept even if it's empty.
 *  With this, a path like "/hello/" becomes "/hello"+"/".
 */
std::vector<std::string> TL_PUBLIC split_path (const std::string &p, bool keep_last = false);

/**
 *  @brief Gets the path of the currently running process
 */
std::string TL_PUBLIC get_inst_path ();

/**
 *  @brief Gets the absolute path of the module (DLL/.so) which contains the given address
 *  "address" is supposed to be the address of a function inside the module.
 */
std::string TL_PUBLIC get_module_path (void *addr);

/**
 *  @brief Gets the line separator (CRLF on windows, LF on linux)
 */
TL_PUBLIC const char *line_separator ();

}

#endif
