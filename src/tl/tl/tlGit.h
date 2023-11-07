
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


#ifndef HDR_tlGit
#define HDR_tlGit

#include "tlCommon.h"
#include "tlStream.h"

#include <string>
#include <vector>

#if !defined(HAVE_GIT2)
#  error "tlGit.h can only be used with libgit2 enabled"
#endif

namespace tl
{

class InputHttpStreamCallback;

/**
 *  @brief Represents an object from a Git URL
 *  This object can be a file or collection
 */
class TL_PUBLIC GitObject
{
public:
  /**
   *  @brief Open a stream with the given URL
   *
   *  The local_path is the path where to store the files.
   *  If empty, a temporary folder is created and destroyed once the "GitObject" goes out of scope.
   */
  GitObject (const std::string &local_path = std::string ());

  /**
   *  @brief Destructor
   */
  ~GitObject ();

  /**
   *  @brief Populates the collection from the given URL
   *
   *  "filter" can be a top-level file to download. If filter is non-empty,
   *  sparse mode is chosen.
   */
  void read (const std::string &url, const std::string &filter, const std::string &subfolder, const std::string &branch, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

  /**
   *  @brief Downloads the collection or file with the given URL
   *
   *  This method will download the Git object from url to the file path
   *  given in "target".
   *
   *  For file download, the target must be the path of the target file.
   *  For collection download, the target must be a directory path. In this
   *  case, the target directory must exist already.
   *
   *  Sub-directories are created if required.
   *
   *  This method returns false if the directory structure could
   *  not be obtained or downloading of one file failed.
   *
   *  "branch" is the remote ref to use. This can be a branch name, a tag name,
   *  a remote ref such as "refs/heads/master" or a symbolic name such as "HEAD".
   */
  static bool download (const std::string &url, const std::string &target, const std::string &subfolder, const std::string &branch, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

  /**
   *  @brief Gets a stream object for downloading the single item of the given URL
   *
   *  The file needs to be a top-level object.
   *  The stream object returned needs to be deleted by the caller.
   */
  static tl::InputStream *download_item (const std::string &url, const std::string &file, const std::string &subfolder, const std::string &branch, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

private:
  std::string m_local_path;
  bool m_is_temp;

  const std::string &local_path () const
  {
    return m_local_path;
  }
};

}

#endif

