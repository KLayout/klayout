
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


#ifndef HDR_layMacroController
#define HDR_layMacroController

#include "layCommon.h"
#include "layPlugin.h"
#include "layMacro.h"
#include "tlObject.h"
#include "tlDeferredExecution.h"

#include <string>
#include <map>
#include <set>

#include <QObject>

namespace lay
{

class MacroEditorDialog;
class MainWindow;
class Technology;

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
   *  @brief Default constructor
   */
  MacroController ();

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
  virtual bool configure (const std::string &key, const std::string &value);

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual void config_finalize();

  /**
   *  @brief Reimplementation of the PluginDeclaration interface
   */
  virtual bool can_exit (lay::PluginRoot *root) const;

  /**
   *  @brief Gets a value indicating whether the plugin will accept a dropped file with the given URL or path
   */
  virtual bool accepts_drop (const std::string &path_or_url) const;

  /**
   *  @brief Gets called when a file or URL is dropped on the plugin
   */
  virtual void drop_url (const std::string &path_or_url);

  /**
   *  @brief Shows the macro editor
   *
   *  Depending on the category, a different tip dialog will be shown.
   *  If "force_add" is true, a new macro will be created, otherwise only
   *  if none exists yet.
   */
  void show_editor (const std::string &cat = std::string (), bool force_add = false);

  /**
   *  @brief Reloads all macros from the paths registered
   */
  void refresh ();

  /**
   *  @brief Adds a search path to the macros
   *  After adding the paths, "load" needs to be called to actually load the macros.
   */
  void add_path (const std::string &path, const std::string &description, const std::string &category, bool readonly);

  /**
   *  @brief Loads the macros from the predefined paths
   *  This method will also establish the macro categories.
   */
  void load ();

  /**
   *  @brief Adds a temporary macro
   *
   *  Temporary macros are such present on the command line or
   *  dragged into the main window without installing.
   *  They need to be present so they participate in the
   *  menu building. Hence they are stored temporarily.
   *  The MainWindow object will become owner of the macro object.
   */
  void add_temp_macro (lay::Macro *m);

  /**
   *  @brief Obtain the list of macro categories
   */
  const std::vector< std::pair<std::string, std::string> > &macro_categories () const
  {
    return m_macro_categories;
  }

  /**
   *  @brief Gets the singleton instance for this object
   */
  static MacroController *instance ();

public slots:
  /**
   *  @brief Update the menu with macros bound to a menu
   */
  void update_menu_with_macros ();

private:
  lay::MacroEditorDialog *mp_macro_editor;
  lay::MainWindow *mp_mw;
  tl::DeferredMethod<MacroController> dm_do_update_menu_with_macros;
  std::vector<lay::Action> m_macro_actions;
  std::map<QAction *, lay::Macro *> m_action_to_macro;
  lay::MacroCollection m_temp_macros;
  std::vector< std::pair<std::string, std::pair<std::string, std::pair<std::string, bool> > > > m_paths;
  std::vector< std::pair<std::string, std::string> > m_macro_categories;

  void add_macro_items_to_menu (lay::MacroCollection &collection, int &n, std::set<std::string> &groups, const lay::Technology *tech, std::vector<std::pair<std::string, std::string> > *key_bindings);
  void do_update_menu_with_macros ();
};

}

#endif

