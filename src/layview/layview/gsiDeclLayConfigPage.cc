
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

#if defined(HAVE_QTBINDINGS)

#include "gsiDeclLayConfigPage.h"

#include "gsiQtGuiExternals.h"
#include "gsiQtWidgetsExternals.h"  //  for Qt5

namespace gsi
{

ConfigPageImpl::ConfigPageImpl (const std::string &title)
  : lay::ConfigPage (0), m_title (title)
{
  //  .. nothing yet ..
}

void
ConfigPageImpl::commit_impl (lay::Dispatcher *root)
{
  lay::ConfigPage::commit (root);
}

void
ConfigPageImpl::commit (lay::Dispatcher *root)
{
  if (f_commit.can_issue ()) {
    f_commit.issue<ConfigPageImpl, lay::Dispatcher *> (&ConfigPageImpl::commit_impl, root);
  } else {
    ConfigPageImpl::commit_impl (root);
  }
}

void
ConfigPageImpl::setup_impl (lay::Dispatcher *root)
{
  lay::ConfigPage::setup (root);
}

void
ConfigPageImpl::setup (lay::Dispatcher *root)
{
  if (f_setup.can_issue ()) {
    f_setup.issue<ConfigPageImpl, lay::Dispatcher *> (&ConfigPageImpl::setup_impl, root);
  } else {
    ConfigPageImpl::setup_impl (root);
  }
}

ConfigPageImpl *new_config_page (const std::string &title)
{
  return new ConfigPageImpl (title);
}

Class<ConfigPageImpl> decl_ConfigPage (QT_EXTERNAL_BASE (QFrame) "lay", "ConfigPage",
  constructor ("new", &new_config_page, gsi::arg ("title"),
    "@brief Creates a new ConfigPage object\n"
    "@param title The title of the page and also the position in the configuration page tree\n"
    "\n"
    "The title has the form 'Group|Page' - e.g. 'Application|Macro Development IDE' will place "
    "the configuration page in the 'Application' group and into the 'Macro Development IDE' page."
  ) +
  callback ("apply", &ConfigPageImpl::commit, &ConfigPageImpl::f_commit, gsi::arg ("dispatcher"),
    "@brief Reimplement this method to transfer data from the page to the configuration\n"
    "In this method, you should transfer all widget data into corresponding configuration updates.\n"
    "Use \\Dispatcher#set_config on the dispatcher object ('dispatcher' argument) to set a configuration parameter.\n"
  ) +
  callback ("setup", &ConfigPageImpl::setup, &ConfigPageImpl::f_setup, gsi::arg ("dispatcher"),
    "@brief Reimplement this method to transfer data from the configuration to the page\n"
    "In this method, you should transfer all configuration data to the widgets.\n"
    "Use \\Dispatcher#get_config on the dispatcher object ('dispatcher' argument) to get a configuration parameter "
    "and set the editing widget's state accordingly.\n"
  ),
  "@brief The plugin framework's configuration page\n"
  "\n"
  "This object provides a way to establish plugin-specific configuration pages.\n"
  "\n"
  "The only way of communication between the page and the plugin is through "
  "configuration parameters. One advantage of this approach is that the current state is "
  "automatically persisted. Configuration parameters can be obtained by the plugin "
  "directly from the \\Dispatcher object) or by listening to 'configure' calls.\n"
  "\n"
  "For the purpose of data transfer, the configuration page has two methods: 'apply' which is supposed to transfer "
  "the editor widget's state into configuration parameters. 'setup' does the inverse and transfer "
  "configuration parameters into editor widget states. Both methods are called by the system when "
  "some transfer is needed.\n"
  "\n"
  "This class has been introduced in version 0.30.4.\n"
);

}

#endif
