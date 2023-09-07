
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


#ifndef HDR_layMacroController
#define HDR_layMacroController

#include "layCommon.h"
#include "layPlugin.h"
#include "lymMacroCollection.h"
#include "tlObject.h"
#include "tlDeferredExecution.h"
#include "tlFileSystemWatcher.h"

#include <string>
#include <map>
#include <set>

#include <QObject>

namespace db
{
  class Technology;
}

namespace lay
{

class MacroEditorDialog;
class MainWindow;
class Action;

/**
 *  @brief A controller for the macro environment
 *
 *  This object is a singleton that acts as a controller
 *  for the macro environment. The controller is responsible
 *  to managing the macro folders, autorunning of macros
 *  and other things.
 *
 *  It interacts with the MacroEditorDialog which basically
 *  is the view for the macros.
 *
 *  By making the controller a PluginDeclaration it will receive
 *  initialization and configuration calls.
 */
class MacroController
  : public lay::PluginDeclaration, public tl::Object
{
Q_OBJECT

public:
  /**
   *  @brief A structure describing a macro category
   */
  struct MacroCategory
  {
    MacroCategory () { }

    std::string name;
    std::string description;
    std::vector<std::string> folders;
  };

  /**
   *  @brief Default constructor
   */
  MacroController ();

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
  virtual bool configure (const std::string &key, const std::string &value);

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual void config_finalize();

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual bool can_exit (lay::Dispatcher *root) const;

  /**
   *  @brief Gets a value indicating whether the plugin will accept a dropped file with the given URL or path
   */
  virtual bool accepts_drop (const std::string &path_or_url) const;

  /**
   *  @brief Gets called when a file or URL is dropped on the plugin
   */
  virtual void drop_url (const std::string &path_or_url);

  /**
   *  @brief Enables or disables implicit macros
   *  If implicit macros are enabled, the macro tree contains the macros defined within the technologies
   *  and other implicit sources.
   *  This flag needs to be set initially and before the technology tree is updated.
   */
  void enable_implicit_macros (bool enable);

  /**
   *  @brief Shows the macro editor
   *
   *  Depending on the category, a different tip dialog will be shown.
   *  If "force_add" is true, a new macro will be created, otherwise only
   *  if none exists yet.
   */
  void show_editor (const std::string &cat = std::string (), bool force_add = false);

  /**
   *  @brief Adds a search path to the macros
   *  After adding the paths, "finish" needs to be called to actually load the macros and establish the
   *  library search paths..
   */
  void add_path (const std::string &path, const std::string &description, const std::string &category, bool readonly);

  /**
   *  @brief Loads the macros from the predefined paths and establishes the search paths
   *  This method can be called multiple times.
   */
  void finish ();

  /**
   *  @brief Adds a new macro category
   *  finish() needs to be called after adding a new category.
   */
  void add_macro_category (const std::string &name, const std::string &description, const std::vector<std::string> &folders);

  /**
   *  @brief Adds a temporary macro
   *
   *  Temporary macros are such present on the command line or
   *  dragged into the main window without installing.
   *  They need to be present so they participate in the
   *  menu building. Hence they are stored temporarily.
   *  The MainWindow object will become owner of the macro object.
   */
  void add_temp_macro (lym::Macro *m);

  /**
   *  @brief Obtain the list of macro categories
   */
  const std::vector<MacroCategory> &macro_categories () const
  {
    return m_macro_categories;
  }

  /**
   *  @brief Gets the singleton instance for this object
   */
  static MacroController *instance ();

public slots:
  /**
   *  @brief Updates the menu with macros bound to a menu
   */
  void macro_collection_changed ();

  /**
   *  @brief Called when the technologies or the salt got changed
   */
  void sync_with_external_sources ();

private slots:
  /**
   *  @brief Called when the file watcher detects a change in the file system
   */
  void file_watcher_triggered ();

private:
  /**
   *  @brief A structure describing an external macro location
   */
  struct ExternalPathDescriptor
  {
    ExternalPathDescriptor (const std::string &_path, const std::string &_description, const std::string &_cat, lym::MacroCollection::FolderType _type, bool _readonly, const std::string &_version = std::string ())
      : path (_path), description (_description), cat (_cat), type (_type), version (_version), readonly (_readonly)
    {
      //  .. nothing yet ..
    }

    std::string path;
    std::string description;
    std::string cat;
    lym::MacroCollection::FolderType type;
    std::string version;
    bool readonly;
  };

  /**
   *  @brief A structure describing an internal macro location
   */
  struct InternalPathDescriptor
  {
    InternalPathDescriptor (const std::string &_path, const std::string &_description, const std::string &_cat, bool _readonly)
      : path (_path), description (_description), cat (_cat), readonly (_readonly)
    {
      //  .. nothing yet ..
    }

    std::string path;
    std::string description;
    std::string cat;
    bool readonly;
  };

  lay::MacroEditorDialog *mp_macro_editor;
  lay::MainWindow *mp_mw;
  bool m_no_implicit_macros;
  tl::weak_collection<lay::Action> m_macro_actions;
  lym::MacroCollection m_temp_macros;
  std::vector<MacroCategory> m_macro_categories;
  std::vector<InternalPathDescriptor> m_internal_paths;
  std::vector<ExternalPathDescriptor> m_external_paths;
  std::vector<std::string> m_package_locations;
  tl::FileSystemWatcher *m_file_watcher;
  tl::DeferredMethod<MacroController> dm_do_update_menu_with_macros;
  tl::DeferredMethod<MacroController> dm_do_sync_with_external_sources;
  tl::DeferredMethod<MacroController> dm_sync_file_watcher;
  tl::DeferredMethod<MacroController> dm_sync_files;
  std::vector<std::pair<std::string, std::string> > m_key_bindings;
  std::vector<std::pair<std::string, bool> > m_menu_items_hidden;

  void sync_implicit_macros (bool ask_before_autorun);
  void add_macro_items_to_menu (lym::MacroCollection &collection, std::set<std::string> &used_names, std::set<std::string> &groups, const db::Technology *tech);
  void do_update_menu_with_macros ();
  void do_sync_with_external_sources ();
  void sync_file_watcher ();
  void sync_files ();
  void sync_package_paths ();
  void sync_macro_sources ();
};

}

#endif

