
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "gsiDeclLayEditorOptionsPage.h"

#include "gsiQtGuiExternals.h"
#include "gsiQtWidgetsExternals.h"  //  for Qt5

namespace gsi
{

EditorOptionsPageImpl::EditorOptionsPageImpl (const std::string &title, int index)
  : lay::EditorOptionsPage (), m_title (title), m_index (index)
{
  //  .. nothing yet ..
}

void
EditorOptionsPageImpl::call_edited ()
{
  lay::EditorOptionsPage::edited ();
}

void
EditorOptionsPageImpl::apply_impl (lay::Dispatcher *root)
{
  lay::EditorOptionsPage::apply (root);
}

void
EditorOptionsPageImpl::apply (lay::Dispatcher *root)
{
  if (f_apply.can_issue ()) {
    f_apply.issue<EditorOptionsPageImpl, lay::Dispatcher *> (&EditorOptionsPageImpl::apply_impl, root);
  } else {
    EditorOptionsPageImpl::apply_impl (root);
  }
}

void
EditorOptionsPageImpl::setup_impl (lay::Dispatcher *root)
{
  lay::EditorOptionsPage::setup (root);
}

void
EditorOptionsPageImpl::setup (lay::Dispatcher *root)
{
  if (f_setup.can_issue ()) {
    f_setup.issue<EditorOptionsPageImpl, lay::Dispatcher *> (&EditorOptionsPageImpl::setup_impl, root);
  } else {
    EditorOptionsPageImpl::setup_impl (root);
  }
}

EditorOptionsPageImpl *new_editor_options_page (const std::string &title, int index)
{
  return new EditorOptionsPageImpl (title, index);
}

Class<EditorOptionsPageImpl> decl_EditorOptionsPage (QT_EXTERNAL_BASE (QWidget) "lay", "EditorOptionsPage",
  constructor ("new", &new_editor_options_page, gsi::arg ("title"), gsi::arg ("index"),
    "@brief Creates a new EditorOptionsPage object\n"
    "@param title The title of the page\n"
    "@param index The position of the page in the tab bar\n"
  ) +
  method ("view", &EditorOptionsPageImpl::view,
    "@brief Gets the view object this page is associated with\n"
  ) +
  method ("edited", &EditorOptionsPageImpl::call_edited,
    "@brief Call this method when some entry widget has changed\n"
    "When some entry widget (for example 'editingFinished' slot of a QLineEdit), "
    "call this method to initiate a transfer of information from the page to the plugin.\n"
  ) +
  callback ("apply", &EditorOptionsPageImpl::apply, &EditorOptionsPageImpl::f_apply, gsi::arg ("dispatcher"),
    "@brief Reimplement this method to transfer data from the page to the configuration\n"
    "In this method, you should transfer all widget data into corresponding configuration updates.\n"
    "Use \\Dispatcher#set_config on the dispatcher object ('dispatcher' argument) to set a configuration parameter.\n"
  ) +
  callback ("setup", &EditorOptionsPageImpl::setup, &EditorOptionsPageImpl::f_setup, gsi::arg ("dispatcher"),
    "@brief Reimplement this method to transfer data from the configuration to the page\n"
    "In this method, you should transfer all configuration data to the widgets.\n"
    "Use \\Dispatcher#get_config on the dispatcher object ('dispatcher' argument) to get a configuration parameter "
    "and set the editing widget's state accordingly.\n"
  ),
  "@brief The plugin framework's editor options page\n"
  "\n"
  "This object provides a way to establish plugin-specific editor options pages.\n"
  "\n"
  "The preferred way of communication between the page and the plugin is through "
  "configuration parameters. One advantage of this approach is that the current state is "
  "automatically persisted.\n"
  "\n"
  "For this purpose, the editor options page has two methods: 'apply' which is supposed to transfer "
  "the editor widget's state into configuration parameters. 'setup' does the inverse and transfer "
  "configuration parameters into editor widget states. Both methods are called by the system when "
  "some transfer is needed.\n"
  "\n"
  "This class has been introduced in version 0.30.4.\n"
);

}

#endif
