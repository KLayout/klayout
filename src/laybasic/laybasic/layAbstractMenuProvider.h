
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#ifndef HDR_layAbstractMenuProvider
#define HDR_layAbstractMenuProvider

#include "laybasicCommon.h"

#include <string>

class QWidget;

namespace lay
{

class AbstractMenu;
class Action;
class ConfigureAction;

/**
 *  @brief An interface for the supplier of the abstract menu object
 */
class LAYBASIC_PUBLIC AbstractMenuProvider
{
public:
  /**
   *  @brief Constructor
   */
  AbstractMenuProvider ();

  /**
   *  @brief Destructor
   */
  virtual ~AbstractMenuProvider ();

  /**
   *  @brief Gets the AbstractMenu object 
   */
  virtual AbstractMenu *menu () = 0;

  /**
   *  @brief Gets the parent widget
   */
  virtual QWidget *menu_parent_widget () = 0;

  /**
   *  @brief Get the action for a slot
   */
  virtual lay::Action &action_for_slot (const char *slot) = 0;

  /**
   *  @brief Create a configuration action with the given title, parameter name and value
   *
   *  The action will be owned by the main window but can be deleted to remove the action from the main window.
   */
  virtual lay::Action *create_config_action (const std::string &title, const std::string &cname, const std::string &cvalue) = 0;
  
  /**
   *  @brief Create a configuration action with the given parameter name and value
   *
   *  The action will be owned by the main window but can be deleted to remove the action from the main window.
   *  This version is provided for applications, where the title is set later.
   */
  virtual lay::Action *create_config_action (const std::string &cname, const std::string &cvalue) = 0;
  
  /**
   *  @brief Register a configuration action with the given name
   */
  virtual void register_config_action (const std::string &name, lay::ConfigureAction *action) = 0;
  
  /**
   *  @brief Unregister a configuration action with the given name
   */
  virtual void unregister_config_action (const std::string &name, lay::ConfigureAction *action) = 0;
  
  /**
   *  @brief Gets the singleton instance of the AbstractMenuProvider object
   */
  static AbstractMenuProvider *instance ();
};

}

#endif


