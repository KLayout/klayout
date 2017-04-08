
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


#ifndef HDR_layTechnologyController
#define HDR_layTechnologyController

#include "layCommon.h"
#include "layPlugin.h"
#include "layTechnology.h"
#include "layAbstractMenu.h"

namespace lay
{

class TechSetupDialog;
class MainWindow;

/**
 *  @brief A "controller" for the technologies
 *  The main task of the controller is to establish and manage the
 *  list of technologies and to manage the active technology.
 */
class LAY_PUBLIC TechnologyController
  : public PluginDeclaration,
    public tl::Object
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  TechnologyController ();

  void initialized (lay::PluginRoot *root);
  void uninitialize (lay::PluginRoot *root);

  void get_options (std::vector < std::pair<std::string, std::string> > &options) const;
  void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const;

  void show_editor ();

  /**
   *  @brief Gets the name of the active technology
   *  The active technology is the one the current cellview uses
   */
  const std::string &active_technology () const
  {
    return m_active_technology;
  }

  static TechnologyController *instance ();

signals:
  /**
   *  @brief This signal is emitted if the active technology has changed
   */
  void active_technology_changed ();

  /**
   *  @brief This signal is emitted if the technology list has been edited
   *  This signal is emitted if either the list or one technology has been
   *  edited. It indicates the need for reflecting changes in the technology
   *  setup.
   */
  void technologies_edited ();

private:
  tl::stable_vector <lay::Action> m_tech_actions;
  std::string m_current_technology;
  std::string m_active_technology;
  bool m_current_technology_updated;
  lay::TechSetupDialog *mp_editor;
  lay::MainWindow *mp_mw;

  void update_after_change ();
  void technologies_changed ();
  void technology_changed (lay::Technology *);
  bool configure (const std::string &name, const std::string &value);
  void config_finalize ();
  bool menu_activated (const std::string &symbol) const;
  void update_current_technology ();
  void update_menu ();
};

}

#endif

