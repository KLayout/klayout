
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

#ifndef HDR_layEditorOptionsPage
#define HDR_layEditorOptionsPage

#include "laybasicCommon.h"

#include "tlObject.h"

#include <set>

namespace db
{
  struct LayerProperties;
}

namespace lay
{

class PluginDeclaration;
class Dispatcher;
class LayoutViewBase;
class Plugin;
class CellView;
class EditorOptionsPage;
class EditorOptionsPageWidget;

/**
 *  @brief An interface managing a collection of EditorOptionPage objects
 */
class LAYBASIC_PUBLIC EditorOptionsPageCollection
{
public:
  EditorOptionsPageCollection ();
  virtual ~EditorOptionsPageCollection () { }

  virtual void unregister_page (EditorOptionsPage *page) = 0;
  virtual bool has_content () const = 0;
  virtual bool has_modal_content () const = 0;
  virtual void make_page_current (EditorOptionsPage *page) = 0;
  virtual void activate_page (EditorOptionsPage *page) = 0;
  virtual void activate (const lay::Plugin *plugin) = 0;
  virtual bool exec_modal (EditorOptionsPage *page) = 0;
  virtual std::vector<lay::EditorOptionsPage *> editor_options_pages (const lay::PluginDeclaration *plugin) = 0;
  virtual std::vector<lay::EditorOptionsPage *> editor_options_pages () = 0;
  virtual lay::EditorOptionsPage *page_with_name (const std::string &name) = 0;
};

/**
 *  @brief The base class for a editor options page
 *
 *  The object properties page is shown in the editor options panel
 *  for the active plugin.
 *
 *  Pages can be toolbox widgets, i.e. they are shown in the drawing area
 *  at the top of the canvas, instead of being shown in the editor options
 *  panel.
 */
class LAYBASIC_PUBLIC EditorOptionsPage
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   */
  EditorOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);

  /**
   *  @brief Default constructor
   */
  EditorOptionsPage ();

  /**
   *  @brief Destructor
   */
  virtual ~EditorOptionsPage ();

  /**
   *  @brief The title of the page
   *  This title is used for the tab title the page appears
   */
  virtual std::string title () const = 0;

  /**
   *  @brief The order in which the pages appear
   *  This index specifies the position of the page. The page with the
   *  lower index appears left.
   *  The page with order 0 is the default page, picked when the plugin
   *  becomes active.
   */
  virtual int order () const = 0;

  /**
   *  @brief The page name
   *  Giving a page name allows looking up a page by name.
   *  The page name is optional and if not specified, a zero pointer
   *  should be returned.
   */
  virtual const char *name () const { return 0; }

  /**
   *  @brief A callback to apply all values
   *  The page is expected to issue "config_set" calls to the dispatcher
   *  to deliver the settings.
   *  This callback is not used for toolbox widgets.
   */
  virtual void apply (lay::Dispatcher * /*root*/) { }

  /**
   *  @brief A callback to setup the page
   *  This callback is expected to set up the page values from
   *  the configuration, stored inside the dispatcher.
   *  This callback is not used for toolbox widgets.
   */
  virtual void setup (lay::Dispatcher * /*root*/) { }

  /**
   *  @brief A callback to cancel the page edits
   *  This callback is used for toolbox widgets if the user presses
   *  "Escape".
   */
  virtual void cancel () { }

  /**
   *  @brief A callback to commit the values
   *  This callback is used for toolbox widgets if the user presses
   *  "Enter". It can either commit values to the dispatcher
   *  through "config_set", or perform other functions.
   */
  virtual void commit (lay::Dispatcher * /*root*/) { }

  /**
   *  @brief Configure the page
   *  This interface can be used by plugin implementation to transfer
   *  data from the plugin to a toolbox widget. This method is not
   *  used by the system directly.
   */
  virtual void configure (const std::string & /*name*/, const std::string & /*value*/) { }

  /**
   *  @brief Called by the system to commit the current settings into some "recently used" list
   */
  virtual void commit_recent (lay::Dispatcher * /*root*/) { }

  /**
   *  @brief Called by the system to restore recent settings for a given layer
   */
  virtual void config_recent_for_layer (lay::Dispatcher * /*root*/, const db::LayerProperties & /*lp*/, int /*cv_index*/) { }

  /**
   *  @brief Sets the focus to the page
   *  This function is called by the system to establish the focus on this page.
   */
  virtual void set_focus () { }

  /**
   *  @brief Returns the widget for the page
   *  The page itself is not a Qt object. To fetch the corresponding widget, use this method.
   */
  virtual EditorOptionsPageWidget *widget () { return 0; }

  /**
   *  @brief Gets a value indicating whether the page is visible
   */
  virtual bool is_visible () const { return false; }

  /**
   *  @brief Changes the visibility of the page
   */
  virtual void set_visible (bool /*visible*/) { }

  /**
   *  @brief Returns a flag indicating whether the page is a focus page
   *  Focus pages are pages that are activated when the user presses the Tab
   *  key in the canvas. Toolbox widgets receive the focus and modal pages
   *  are shown modally.
   */
  bool is_focus_page () const { return m_focus_page; }

  /**
   *  @brief Sets a flag indicating whether the page is a focus page
   */
  void set_focus_page (bool f) { m_focus_page = f; }

  /**
   *  @brief Returns a flag indicating whether the page is a modal page
   *  Modal pages are shown in a modal dialog when they receive the focus.
   *  Otherwise they remain invisible.
   */
  bool is_modal_page () const { return m_modal_page; }

  /**
   *  @brief Sets a flag indicating whether the page is a modal page
   */
  void set_modal_page (bool f) { m_modal_page = f; }

  /**
   *  @brief Returns a flag indicating whether the page is a toolbox widget
   */
  bool is_toolbox_widget () const { return m_toolbox_widget; }

  /**
   *  @brief Sets a flag indicating whether the page is a toolbox widget
   */
  void set_toolbox_widget (bool f) { m_toolbox_widget = f; }

  /**
   *  @brief Gets a value indicating whether the page is active
   *  A page is active when the corresponding plugin is active
   */
  bool active () const { return m_active; }

  /**
   *  @brief Activates a page
   *  This function is called when the system activates a page because the
   *  corresponding plugin was activate.
   */
  void activate (bool active);

  /**
   *  @brief Sets the owner of the page
   *  This function is used by the system and must not be used otherwise.
   */
  void set_owner (EditorOptionsPageCollection *owner);

  /**
   *  @brief Shows the editor page
   *  @return -1, if the page is shown non-modal, otherwise 1 or 0 if the dialog was accepted (1) or rejected (0)
   */
  int show ();

  /**
   *  @brief Gets a value indicating whether the page is for that specific plugin (given by a declaration object)
   */
  bool for_plugin_declaration (const lay::PluginDeclaration *pd)
  {
    return m_plugin_declarations.find (pd) != m_plugin_declarations.end ();
  }

  /**
   *  @brief Sets the plugin the page is associated with
   *  This function is used by the system and must not be used otherwise.
   */
  void set_plugin_declaration (const lay::PluginDeclaration *pd)
  {
    m_plugin_declarations.clear ();
    m_plugin_declarations.insert (pd);
  }

  /**
   *  @brief Sets the plugins the page is associated with
   *  This function is used by the system and must not be used otherwise.
   */
  void set_plugin_declarations (const std::vector<const lay::PluginDeclaration *> &pd)
  {
    m_plugin_declarations.clear ();
    m_plugin_declarations.insert (pd.begin (), pd.end ());
  }

  /**
   *  @brief Gets the dispatcher the page is connected to
   */
  lay::Dispatcher *dispatcher () const
  {
    return mp_dispatcher;
  }

  /**
   *  @brief Gets the view the page is connected to
   */
  lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

  /**
   *  @brief Initializes the page
   *  This function is used by the system and must not be used otherwise.
   */
  void init (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);

protected:
  virtual void active_cellview_changed () { }
  virtual void technology_changed (const std::string & /*tech*/) { }
  virtual void activated () { }
  virtual void deactivated () { }

private:
  EditorOptionsPageCollection *mp_owner;
  bool m_active;
  bool m_focus_page;
  bool m_modal_page, m_toolbox_widget;
  std::set<const lay::PluginDeclaration *> m_plugin_declarations;
  lay::Dispatcher *mp_dispatcher;
  lay::LayoutViewBase *mp_view;

  void on_active_cellview_changed ();
  void on_technology_changed ();
  void attach_events ();
};

/**
 *  @brief A basic factory class for editor options pages
 *
 *  We will use it later to provide a registration-based specialized factory
 *  for Qt-enabled option pages, which we should not link here.
 *
 *  A factory has a name - if the name matches a plugin name,
 *  the factory is automatically requested to create a page for
 *  that plugin.
 *
 *  Otherwise, plugins can request additional pages through
 *  "additional_editor_options_pages". This is a list of names
 *  (not plugin names) of page factories. These factories will
 *  be called to provide additional pages then.
 */
class LAYBASIC_PUBLIC EditorOptionsPageFactoryBase
{
public:
  EditorOptionsPageFactoryBase (const char *name)
    : m_name (name)
  {
    //  .. nothing yet ..
  }

  EditorOptionsPageFactoryBase ()
    : m_name ()
  {
    //  .. nothing yet ..
  }

  virtual ~EditorOptionsPageFactoryBase () { }

  const std::string &name () const
  {
    return m_name;
  }

  virtual lay::EditorOptionsPage *create_page (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher) = 0;

private:
  std::string m_name;
};

/**
 *  @brief A specialized editor options page factory class for a specific type
 *
 *  Register the factory using:
 *
 *  #include "tlClassRegistry.h"
 *  static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory (new lay::EditorOptionsPageFactory<MyClass> (), 0, "MyClass");
 *
 *  Later you can create a page from "MyName" using
 *
 *  page = EditorOptionsPageFactoryBase::create_page_by_name ("MyClass", view, dispatcher);
 */
template <class T>
class LAYBASIC_PUBLIC_TEMPLATE EditorOptionsPageFactory
  : public EditorOptionsPageFactoryBase
{
public:
  EditorOptionsPageFactory (const char *plugin_name)
    : EditorOptionsPageFactoryBase (plugin_name)
  {
    //  .. nothing yet ..
  }

  EditorOptionsPageFactory ()
    : EditorOptionsPageFactoryBase ()
  {
    //  .. nothing yet ..
  }

  virtual ~EditorOptionsPageFactory () { }

  virtual lay::EditorOptionsPage *create_page (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  {
    return new T (view, dispatcher);
  }
};

}

#endif

