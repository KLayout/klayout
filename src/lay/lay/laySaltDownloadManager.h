
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "layLogViewerDialog.h"
#include "tlProgress.h"

#include "ui_SaltManagerInstallConfirmationDialog.h"

#include <QObject>
#include <string>
#include <map>

namespace tl
{
  class InputHttpStreamCallback;
}

namespace lay
{

class Salt;
class SaltManagerDialog;

class ConfirmationDialog
  : public QDialog, private Ui::SaltManagerInstallConfirmationDialog
{
Q_OBJECT

public:
  ConfirmationDialog (QWidget *parent);

  void add_info (const std::string &name, bool update, const std::string &version, const std::string &url);

  bool is_confirmed () const  { return m_confirmed; }
  bool is_cancelled () const  { return m_cancelled; }
  bool is_aborted () const  { return m_aborted; }

  void start ();
  void separator ();
  void finish ();

  void mark_fetching (const std::string &name);
  void mark_error (const std::string &name);
  void mark_success (const std::string &name);
  void set_progress (const std::string &name, double progress);

private slots:
  void confirm_pressed ()     { m_confirmed = true; }
  void cancel_pressed ()      { m_cancelled = true; }
  void abort_pressed ()       { m_aborted = true; }
  void close_pressed ()       { hide (); }

private:
  bool m_confirmed, m_cancelled, m_aborted;
  lay::LogFile m_file;
  std::map<std::string, QTreeWidgetItem *> m_items_by_name;

  void set_icon_for_name (const std::string &name, const QIcon &icon);
};

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
   *  @brief Gets a flag indicating whether to always download package information
   */
  bool always_download_package_information () const
  {
    return m_always_download_package_information;
  }

  /**
   *  @brief Sets a flag indicating whether to always download package information
   */
  void set_always_download_package_information (bool f)
  {
    m_always_download_package_information = f;
  }

  /**
   *  @brief Registers an URL (with version) for download in the given target directory
   *
   *  The target directory can be empty. In this case, the downloader will pick an appropriate one.
   */
  void register_download (const std::string &name, const std::string &token, const std::string &url, const std::string &version);

  /**
   *  @brief Computes the dependencies after all required packages have been registered
   *
   *  This method will compute the dependencies. Packages not present in the list of
   *  packages ("salt" argument), will be scheduled for download too. Dependency packages
   *  are looked up in "salt_mine" if no download URL is given.
   */
  void compute_dependencies (const lay::Salt &salt, const Salt &salt_mine);

  /**
   *  @brief Computes the list of packages after all required packages have been registered
   *
   *  This method will removes packages that are already installed and satisfy the version requirements.
   *  Packages not present in the list of packages ("salt" argument), will be scheduled for download too.
   *  No dependencies are checked in this version.
   */
  void compute_packages (const lay::Salt &salt, const Salt &salt_mine);

  /**
   *  @brief Actually execute the downloads
   *
   *  If parent is non-null, this method will show a confirmation dialog and start installation
   *  if this dialog is confirmed. It will return false if the dialog was cancelled and an exception
   *  if something goes wrong.
   *
   *  If dialog is null, no confirmation dialog will be shown and installation happens in non-GUI
   *  mode.
   *
   *  The return value will be true if the packages were installed successfully.
   */
  bool execute (lay::SaltManagerDialog *dialog, lay::Salt &salt);

private:
  struct Descriptor
  {
    Descriptor (const std::string &_name, const std::string &_token, const std::string &_url, const std::string &_version)
      : name (_name), token (_token), url (_url), version (_version), downloaded (false)
    { }

    bool operator< (const Descriptor &other) const
    {
      if (name != other.name) {
        return name < other.name;
      } else {
        return lay::SaltGrain::compare_versions (version, other.version) < 0;
      }
    }

    bool operator== (const Descriptor &other) const
    {
      if (name != other.name) {
        return false;
      } else {
        return lay::SaltGrain::compare_versions (version, other.version) == 0;
      }
    }

    std::string name;
    std::string token;
    std::string url;
    std::string version;
    bool downloaded;
    lay::SaltGrain grain;
  };

  std::vector<Descriptor> m_registry;
  bool m_always_download_package_information;

  bool needs_iteration ();
  void fetch_missing (const lay::Salt &salt, const lay::Salt &salt_mine, tl::AbsoluteProgress &progress);
  lay::ConfirmationDialog *make_confirmation_dialog (QWidget *parent, const lay::Salt &salt);
  void compute_list (const lay::Salt &salt, const lay::Salt &salt_mine, bool with_dep);
};

}

#endif
