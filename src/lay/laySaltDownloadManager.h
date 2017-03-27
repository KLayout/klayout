
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
#include "laySaltGrain.h"
#include "tlProgress.h"

#include <QObject>
#include <string>
#include <map>

namespace lay
{

class Salt;

/**
 *  @brief The download manager
 *
 *  This class is responsible for handling the downloads for
 *  grains. The basic sequence is:
 *   + "register_download" (multiple times) to register the packages intended for download
 *   + "compute_dependencies" to determine all related packages
 *   + (optional) "show_confirmation_dialog"
 *   + "execute" to actually execute the downloads
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
   *  @brief Registers an URL (with version) for download in the given target directory
   *
   *  The target directory can be empty. In this case, the downloader will pick an approriate one.
   */
  void register_download (const std::string &name, const std::string &url, const std::string &version);

  /**
   *  @brief Computes the dependencies after all required packages have been registered
   *
   *  This method will compute the dependencies. Packages not present in the list of
   *  packages ("salt" argument), will be scheduled for download too. Dependency packages
   *  are looked up in "salt_mine" if no download URL is given.
   */
  void compute_dependencies (const lay::Salt &salt, const Salt &salt_mine);

  /**
   *  @brief Presents a dialog showing the packages scheduled for download
   *
   *  This method requires all dependencies to be computed. It will return false
   *  if the dialog is not confirmed.
   *
   *  "salt" needs to be the currently installed packages so the dialog can
   *  indicate which packages will be updated.
   */
  bool show_confirmation_dialog (QWidget *parent, const lay::Salt &salt);

  /**
   *  @brief Actually execute the downloads
   *
   *  This method will return false if anything goes wrong.
   *  Failed packages will be removed entirely after they have been listed in
   *  an error dialog.
   */
  bool execute (lay::Salt &salt);

private:
  struct Descriptor
  {
    Descriptor (const std::string &_url, const std::string &_version)
      : url (_url), version (_version), downloaded (false)
    { }

    std::string url;
    std::string version;
    bool downloaded;
    lay::SaltGrain grain;
  };

  std::map<std::string, Descriptor> m_registry;

  bool needs_iteration ();
  void fetch_missing (const lay::Salt &salt_mine, tl::AbsoluteProgress &progress);
};

}

#endif
