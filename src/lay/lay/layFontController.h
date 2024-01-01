
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


#ifndef HDR_layFontController
#define HDR_layFontController

#include "layCommon.h"
#include "layPlugin.h"
#include "tlFileSystemWatcher.h"
#include "tlDeferredExecution.h"

#include <vector>
#include <string>

#include <QObject>

namespace lay
{

class FontManagerDialog;
class MainWindow;

/**
 *  @brief A controller for the fonts
 *
 *  This object is a singleton that acts as a controller
 *  for the font management for the Glyphs. The controller is responsible
 *  to managing the fonts and notifying library consumers
 *  of changes.
 *
 *  By making the controller a PluginDeclaration it will receive
 *  initialization and configuration calls.
 */
class FontController
  : public lay::PluginDeclaration, public tl::Object
{
Q_OBJECT

public:
  /**
   *  @brief Default constructor
   */
  FontController ();

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
   *  @brief Gets the singleton instance for this object
   */
  static FontController *instance ();

private slots:
  /**
   *  @brief Called when the file watcher detects a change in the file system
   */
  void file_watcher_triggered ();

  /**
   *  @brief Called when the salt (packages) has changed
   */
  void sync_with_external_sources ();

private:
  tl::FileSystemWatcher *m_file_watcher;
  tl::DeferredMethod<FontController> dm_sync_dirs;

  void sync_dirs ();
};

}

#endif

