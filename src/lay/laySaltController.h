
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


#ifndef HDR_laySaltController
#define HDR_laySaltController

#include "layCommon.h"
#include "layPlugin.h"
#include "laySalt.h"

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
  virtual void initialized (lay::PluginRoot *root);

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual void uninitialize (lay::PluginRoot *root);

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
  bool can_exit (lay::PluginRoot *root) const;

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
   *  @brief Specifies the salt mine (package repository) URL
   */
  void set_salt_mine_url (const std::string &url);

  /**
   *  @brief Gets the singleton instance for this object
   */
  static SaltController *instance ();

signals:
  /**
   *  @brief This signal is emitted if the salt changed
   */
  void salt_changed ();

private:
  lay::SaltManagerDialog *mp_salt_dialog;
  lay::MainWindow *mp_mw;
  std::string m_salt_mine_url;
  lay::Salt m_salt, m_salt_mine;
};

}

#endif

