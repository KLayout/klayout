
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

#include "gsiDeclLayEditorOptionsPage.h"

#include "gsiQtGuiExternals.h"
#include "gsiQtWidgetsExternals.h"  //  for Qt5

namespace gsi
{

static int show (lay::EditorOptionsPageWidget *page)
{
  return page->lay::EditorOptionsPage::show ();
}

static void hide (lay::EditorOptionsPageWidget *)
{
  //  overrides QWidget::hide without doing anything
}

static void setVisible (lay::EditorOptionsPageWidget *, bool)
{
  //  overrides QWidget::hide without doing anything
}

static lay::LayoutViewBase *view (lay::EditorOptionsPageWidget *page)
{
  return page->view ();
}

static std::string title (lay::EditorOptionsPageWidget *page)
{
  return page->title ();
}

static int order (lay::EditorOptionsPageWidget *page)
{
  return page->order ();
}

static bool is_focus_page (lay::EditorOptionsPageWidget *page)
{
  return page->is_focus_page ();
}

static void set_focus_page (lay::EditorOptionsPageWidget *page, bool f)
{
  page->set_focus_page (f);
}

static bool is_modal_page (lay::EditorOptionsPageWidget *page)
{
  return page->is_modal_page ();
}

static void set_modal_page (lay::EditorOptionsPageWidget *page, bool f)
{
  page->set_modal_page (f);
}

static bool is_toolbox_widget (lay::EditorOptionsPageWidget *page)
{
  return page->is_toolbox_widget ();
}

static void set_toolbox_widget (lay::EditorOptionsPageWidget *page, bool f)
{
  page->set_toolbox_widget (f);
}

static void apply (lay::EditorOptionsPageWidget *page, lay::Dispatcher *root)
{
  page->apply (root);
}

static void cancel (lay::EditorOptionsPageWidget *page)
{
  page->cancel ();
}

static void commit (lay::EditorOptionsPageWidget *page, lay::Dispatcher *root)
{
  page->commit (root);
}

static void setup (lay::EditorOptionsPageWidget *page, lay::Dispatcher *root)
{
  page->setup (root);
}

Class<lay::EditorOptionsPageWidget> decl_EditorOptionsPageBase (QT_EXTERNAL_BASE (QWidget) "lay", "EditorOptionsPageBase",
  method_ext ("view", &view,
    "@brief Gets the view object this page is associated with\n"
  ) +
  method_ext ("title", &title,
    "@brief Gets the title string of the page\n"
  ) +
  method_ext ("order", &order,
    "@brief Gets the order index of the page\n"
  ) +
  method_ext ("is_focus_page?|focus_page", &is_focus_page,
    "@brief Gets a flag indicating whether the page is a focus page\n"
    "See \\focus_page= for a description is this attribute.\n"
    "\n"
    "The 'focus_page' getter was added in version 0.30.6."
  ) +
  method_ext ("focus_page=", &set_focus_page, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether the page is a focus page\n"
    "The focus page is the page that is selected when the tab key is pressed during some plugin action.\n"
    "\n"
    "By default, this flag is not set.\n"
  ) +
  method_ext ("is_modal_page?|modal_page", &is_modal_page,
    "@brief Gets a flag indicating whether the page is a modal page\n"
    "See \\modal_page= for a description is this attribute.\n"
    "\n"
    "The 'modal_page' getter was added in version 0.30.6."
  ) +
  method_ext ("modal_page=", &set_modal_page, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether the page is a modal page\n"
    "A modal page is shown in a modal dialog upon \\show. Non-modal pages are shown in the "
    "editor options dock.\n"
    "\n"
    "By default, this flag is not set.\n"
  ) +
  method_ext ("is_toolbox_widget?|toolbox_widget", &is_toolbox_widget,
    "@brief Gets a flag indicating whether the page is a toolbox widget\n"
    "See \\toolbox_widget= for a description is this attribute.\n"
    "\n"
    "This attribute was added in version 0.30.6."
  ) +
  method_ext ("toolbox_widget=", &set_toolbox_widget, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether the page is a toolbox widget\n"
    "A toolbox widget is shown at the top of the layout view. It can be made transparent, so the background shows the\n"
    "layout canvas. See \\transparent= for details.\n"
    "\n"
    "A toolbox widget should also be the focus widget. This way, pressing the Tab key will enter the "
    "toolbox widget and enables editing of the widgets there.\n"
    "\n"
    "By default, this flag is not set.\n"
    "\n"
    "This attribute was added in version 0.30.6."
  ) +
  method ("is_transparent?|transparent", &lay::EditorOptionsPageWidget::is_transparent,
    "@brief Gets a flag indicating whether the widget is transparent\n"
    "See \\transparent= for a description is this attribute.\n"
    "\n"
    "This attribute was added in version 0.30.6."
  ) +
  method ("transparent=", &lay::EditorOptionsPageWidget::set_transparent, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether the widget is transparent\n"
    "A transparent widget is useful for toolbox widgets and makes the layout canvas become\n"
    "visible in parts which are not covered by child widgets of the page.\n"
    "\n"
    "By default, the page is not transparent.\n"
    "\n"
    "This attribute was added in version 0.30.6."
  ) +
  method ("is_visible?|visible", &lay::EditorOptionsPageWidget::is_visible,
    "@brief Gets a flag indicating whether the widget is visible\n"
    "See \\visible= for a description is this attribute.\n"
    "\n"
    "This attribute was added in version 0.30.6."
  ) +
  method ("visible=", &lay::EditorOptionsPageWidget::set_visible, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether the widget is visible\n"
    "Instead of using 'show' and 'hide' or 'isVisible', use this attribute to control whether the page is visible or not.\n"
    "The visibility attribute is useful mainly for conditionally enabling editor option pages based on the status of the plugin.\n"
    "\n"
    "By default, the page is visible.\n"
    "\n"
    "This attribute was added in version 0.30.6."
  ) +
  method_ext ("show", &show,
    "@brief Shows the page\n"
    "@return A value indicating whether the page was opened non-modal (-1), accepted (1) or rejected (0)\n"
    "Provided the page is selected because the plugin is active, this method will "
    "open a dialog to show the page if it is modal, or locate the page in the editor options "
    "dock and bring it to the front if it is non-modal.\n"
    "\n"
    "Before the page is shown, \\setup is called. When the page is dismissed (accepted), \\apply is called. "
    "You can overload these methods to transfer data to and from the configuration space or to perform other "
    "actions, not related to configuration parameters."
  ) +
  method_ext ("hide", &hide,
    "@hide\n"
  ) +
  method_ext ("setVisible", &setVisible,
    "@hide\n"
  ) +
  method_ext ("apply", &apply, gsi::arg ("dispatcher"),
    "@brief Transfers data from the page to the configuration\n"
  ) +
  method_ext ("setup", &setup, gsi::arg ("dispatcher"),
    "@brief Transfers data from the configuration to the page\n"
  ) +
  method_ext ("cancel", &cancel,
    "@brief Gets called when the Escape key is pressed on a non-modal page\n"
    "This method has been introduced in version 0.30.6."
  ) +
  method_ext ("commit", &commit,
    "@brief Gets called when the Enter key is pressed on a non-modal page\n"
    "This method has been introduced in version 0.30.6."
  ),
  "@brief The plugin framework's editor options page base class\n"
  "\n"
  "This class is provided as an interface to the base class implementation for various functions.\n"
  "You can use these methods in order to pass down events to the original implementation or access\n"
  "objects not created in script space.\n"
  "\n"
  "It features some useful methods such as 'view' and provides a slot to call for triggering a data "
  "transfer ('edited').\n"
  "\n"
  "Note that even though the page class is derived from QWidget, you can call QWidget methods "
  "but not overload virtual methods from QWidget.\n"
  "\n"
  "This class has been introduced in version 0.30.4.\n"
);

EditorOptionsPageImpl::EditorOptionsPageImpl (const std::string &title, int index)
  : lay::EditorOptionsPageWidget (), m_title (title), m_index (index)
{
  //  .. nothing yet ..
}

void
EditorOptionsPageImpl::call_edited ()
{
  lay::EditorOptionsPageWidget::edited ();
}

static void apply_fb (EditorOptionsPageImpl *ep, lay::Dispatcher *root)
{
  ep->lay::EditorOptionsPageWidget::apply (root);
}

static void setup_fb (EditorOptionsPageImpl *ep, lay::Dispatcher *root)
{
  ep->lay::EditorOptionsPageWidget::setup (root);
}

static void cancel_fb (EditorOptionsPageImpl *ep)
{
  ep->lay::EditorOptionsPageWidget::cancel ();
}

static void commit_fb (EditorOptionsPageImpl *ep, lay::Dispatcher *root)
{
  ep->lay::EditorOptionsPageWidget::commit (root);
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

void
EditorOptionsPageImpl::cancel_impl ()
{
  lay::EditorOptionsPageWidget::cancel ();
}

void
EditorOptionsPageImpl::cancel ()
{
  if (f_cancel.can_issue ()) {
    f_cancel.issue<EditorOptionsPageImpl> (&EditorOptionsPageImpl::cancel_impl);
  } else {
    EditorOptionsPageImpl::cancel_impl ();
  }
}

void
EditorOptionsPageImpl::commit_impl (lay::Dispatcher *root)
{
  lay::EditorOptionsPageWidget::commit (root);
}

void
EditorOptionsPageImpl::commit (lay::Dispatcher *root)
{
  if (f_commit.can_issue ()) {
    f_commit.issue<EditorOptionsPageImpl, lay::Dispatcher *> (&EditorOptionsPageImpl::commit_impl, root);
  } else {
    EditorOptionsPageImpl::commit_impl (root);
  }
}

EditorOptionsPageImpl *new_editor_options_page (const std::string &title, int index)
{
  return new EditorOptionsPageImpl (title, index);
}

Class<EditorOptionsPageImpl> decl_EditorOptionsPage (decl_EditorOptionsPageBase, "lay", "EditorOptionsPage",
  constructor ("new", &new_editor_options_page, gsi::arg ("title"), gsi::arg ("index"),
    "@brief Creates a new EditorOptionsPage object\n"
    "@param title The title of the page\n"
    "@param index The position of the page in the tab bar\n"
  ) +
  method ("edited", &EditorOptionsPageImpl::call_edited,
    "@brief Call this method when some entry widget has changed\n"
    "When some entry widget (for example 'editingFinished' slot of a QLineEdit), "
    "call this method to initiate a transfer of information from the page to the plugin.\n"
    "After calling 'edited', the system will call 'apply' to transfer the information."
  ) +
  //  prevents infinite recursion
  method_ext ("apply", &apply_fb, gsi::arg ("dispatcher"), "@hide") +
  callback ("apply", &EditorOptionsPageImpl::apply, &EditorOptionsPageImpl::f_apply, gsi::arg ("dispatcher"),
    "@brief Reimplement this method to transfer data from the page to the configuration\n"
    "In this method, you should transfer all widget data into corresponding configuration updates.\n"
    "Use \\Dispatcher#set_config on the dispatcher object ('dispatcher' argument) to set a configuration parameter.\n"
    "\n"
    "'apply' is called either when a modal dialog closes or after 'edited' has been called to signal "
    "a change of information.\n"
  ) +
  //  prevents infinite recursion
  method_ext ("setup", &setup_fb, gsi::arg ("dispatcher"), "@hide") +
  callback ("setup", &EditorOptionsPageImpl::setup, &EditorOptionsPageImpl::f_setup, gsi::arg ("dispatcher"),
    "@brief Reimplement this method to transfer data from the configuration to the page\n"
    "In this method, you should transfer all configuration data to the widgets.\n"
    "Use \\Dispatcher#get_config on the dispatcher object ('dispatcher' argument) to get a configuration parameter "
    "and set the editing widget's state accordingly.\n"
  ) +
  //  prevents infinite recursion
  method_ext ("cancel", &cancel_fb, "@hide") +
  callback ("cancel", &EditorOptionsPageImpl::cancel, &EditorOptionsPageImpl::f_cancel,
    "@brief Reimplement this method to receive Escape key events for the page\n"
    "This method has been added in version 0.30.6.\n"
  ) +
  //  prevents infinite recursion
  method_ext ("commit", &commit_fb, gsi::arg ("dispatcher"), "@hide") +
  callback ("commit", &EditorOptionsPageImpl::commit, &EditorOptionsPageImpl::f_commit, gsi::arg ("dispatcher"),
    "@brief Reimplement this method to receive Enter key events for the page\n"
    "This method applies to toolbox widgets (see \\EditorOptionsPageBase#toolbox_widget=). "
    "When the user presses the Enter key, the 'commit' method is called. You can reimplement "
    "this method instead of 'apply' as a opportunity to perform additional actions such as "
    "terminating some operation.\n"
    "\n"
    "This method has been added in version 0.30.6.\n"
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
  "When you want to respond to widget signals and transfer information, call \\edited "
  "in the signal slot. This will trigger a transfer (aka 'apply').\n"
  "\n"
  "This class has been introduced in version 0.30.4.\n"
);

}

#endif
