
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


#ifndef HDR_laySaltController
#define HDR_laySaltController

#include "layCommon.h"
#include "layPlugin.h"
#include "laySalt.h"
#include "tlFileSystemWatcher.h"
#include "tlDeferredExecution.h"
#include "tlEvents.h"

#include <vector>
#include <string>

#include <QObject>

namespace lay
{

class SaltManagerDialog;
class MainWindow;

/**
 *  @brief A controller for the salt package manager
 *
 *  This object is a singleton that acts as a controller
 *  for the package management. The controller is responsible
 *  to managing the packages and notifying package consumers
 *  of changes.
 *
 *  It interacts with the SaltManagerDialog which basically
 *  is the view for the packages.
 *
 *  By making the controller a PluginDeclaration it will receive
 *  initialization and configuration calls.
 */
class SaltController
  : public lay::PluginDeclaration, public tl::Object
{
Q_OBJECT

public:
  /**
   *  @brief Default constructor
   */
  SaltController ();

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual void initialize (lay::Dispatcher *root);

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual void initialized (lay::Dispatcher *root);

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual void uninitialize (lay::Dispatcher *root);

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  void get_options (std::vector < std::pair<std::string, std::string> > &options) const;

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const;

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  bool configure (const std::string &key, const std::string &value);

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  void config_finalize();

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  bool can_exit (lay::Dispatcher *root) const;

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  bool accepts_drop (const std::string &path_or_url) const;

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  void drop_url (const std::string &path_or_url);

  /**
   *  @brief Shows the package editor
   */
  void show_editor ();

  /**
   *  @brief Adds a search path to the package manager
   */
  void add_path (const std::string &path);

  /**
   *  @brief Installs the packages from the given list
   *
   *  The list is a list of names or URL's, optionally with a version in round brackets.
   *  If a package with the given name or URL is installed already, it is skipped.
   *  Otherwise, it's downloaded and installed. If URL's are given, the URL is
   *  used for download. If names are given, the URL's are taken from the
   *  salt mine index.
   *
   *  If "with_dep" is used, dependencies are also installed as taken from the
   *  salt mine index.
   *
   *  The method returns true if all packages could be installed successfully.
   */
  bool install_packages (const std::vector<std::string> &packages, bool with_dep);

  /**
   *  @brief Specifies the salt mine (package repository) URL
   */
  void set_salt_mine_url (const std::string &url);

  /**
   *  @brief Gets the salt mine (package repository) URL
   */
  const std::string &salt_mine_url () const
  {
    return m_salt_mine_url;
  }

  /**
   *  @brief Gets the salt
   */
  lay::Salt &salt ()
  {
    return m_salt;
  }

  /**
   *  @brief Gets the salt (const version)
   */
  const lay::Salt &salt () const
  {
    return m_salt;
  }

  /**
   *  @brief Event-style version of "salt_changed"
   */
  tl::Event salt_changed_event;

  /**
   *  @brief Gets the singleton instance for this object
   */
  static SaltController *instance ();

private slots:
  /**
   *  @brief Called when the file watcher detects a change in the file system
   */
  void file_watcher_triggered ();

  /**
   *  @brief Emits a salt_changed event + signal
   */
  void emit_salt_changed ();

signals:
  /**
   *  @brief This signal is emitted if the salt changed
   */
  void salt_changed ();

private:
  lay::SaltManagerDialog *mp_salt_dialog;
  lay::MainWindow *mp_mw;
  lay::Dispatcher * mp_plugin_root;
  std::string m_salt_mine_url;
  lay::Salt m_salt;
  tl::FileSystemWatcher *m_file_watcher;
  tl::DeferredMethod<SaltController> dm_sync_file_watcher;
  tl::DeferredMethod<SaltController> dm_sync_files;

  void sync_file_watcher ();
  void sync_files ();
};

}

#endif

