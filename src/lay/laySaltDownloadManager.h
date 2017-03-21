
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

#ifndef HDR_laySaltDownloadManager
#define HDR_laySaltDownloadManager

#include "layCommon.h"

#include <QObject>
#include <string>

namespace lay
{

/**
 *  @brief The download manager
 *  This class is responsible for handling the downloads for
 *  grains.
 */
class LAY_PUBLIC SaltDownloadManager
  : public QObject
{
Q_OBJECT

public:
  /**
   *  @brief Default constructor
   */
  SaltDownloadManager ();

  /**
   *  @brief Downloads the files from the given URL to the given target location
   *  The target directory needs to exist.
   *  Returns true, if the download was successful, false otherwise.
   */
  bool download (const std::string &url, const std::string &target_dir);
};

}

#endif
