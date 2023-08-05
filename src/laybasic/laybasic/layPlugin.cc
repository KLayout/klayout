
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


#include "laybasicCommon.h"

#include "tlException.h"
#include "tlAssert.h"
#include "tlXMLParser.h"
#include "tlLog.h"

#include "layPlugin.h"
#include "layDispatcher.h"
#include "tlExceptions.h"
#include "tlClassRegistry.h"

#if defined(HAVE_QT)
#include "gtf.h"
#endif

#include <iostream>
#include <fstream>
#include <memory>

namespace lay
{

// ----------------------------------------------------------------
//  PluginDeclaration implementation

static int s_next_id = 0;

PluginDeclaration::PluginDeclaration ()
  : m_id (++s_next_id), m_editable_enabled (true)
{
    // .. nothing yet ..
}

PluginDeclaration::~PluginDeclaration ()
{
  if (Dispatcher::instance ()) {
    Dispatcher::instance ()->plugin_removed (this);
  }
}

#if defined(HAVE_QT)
void 
PluginDeclaration::toggle_editable_enabled ()
{
  BEGIN_PROTECTED
  set_editable_enabled (! editable_enabled ());
  END_PROTECTED
}
#endif

std::vector<std::string>
PluginDeclaration::menu_symbols ()
{
  std::vector<std::string> symbols;

  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {

    std::vector<lay::MenuEntry> menu_entries;
    cls->get_menu_entries (menu_entries);

    for (std::vector<lay::MenuEntry>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {
      if (! m->symbol.empty ()) {
        symbols.push_back (m->symbol);
      }
    }

  }

  std::sort (symbols.begin (), symbols.end ());
  symbols.erase (std::unique (symbols.begin (), symbols.end ()), symbols.end ());

  return symbols;
}

namespace {

class GenericMenuAction
  : public Action
{
public:
  GenericMenuAction (Dispatcher *dispatcher, const std::string &title, const std::string &symbol)
    : Action (title), mp_dispatcher (dispatcher), m_symbol (symbol)
  { }

  void triggered ()
  {
    if (mp_dispatcher) {
      mp_dispatcher->menu_activated (m_symbol);
    }
  }

private:
  Dispatcher *mp_dispatcher;
  std::string m_symbol;
};

class ModeAction
  : public Action
{
public:
  ModeAction (Dispatcher *dispatcher, const std::string &title, int mode)
    : Action (title), mp_dispatcher (dispatcher), m_mode (mode)
  { }

  void triggered ()
  {
    if (mp_dispatcher) {
      mp_dispatcher->select_mode (m_mode);
      set_checked (true);
    }
  }

  bool is_for_mode (int mode_id) const
  {
    return mode_id == m_mode;
  }

private:
  Dispatcher *mp_dispatcher;
  int m_mode;
};

}

void
PluginDeclaration::init_menu (lay::Dispatcher *dispatcher)
{
  lay::AbstractMenu &menu = *dispatcher->menu ();

  mp_editable_mode_action.reset ((Action *) 0);
  mp_mouse_mode_action.reset ((Action *) 0);

  std::string title;

  //  make all plugins that return true on "implements_editable" into menu entries and
  //  set up the actions accordingly

  if (implements_editable (title) && menu.is_valid ("edit_menu.select_menu")) {

    //  extract first part, which is the name, separated by a tab from the title.
    std::string name = tl::sprintf ("pi_enable_%d", id ());
    std::string t (title);
    const char *tab = strchr (t.c_str (), '\t');
    if (tab) {
      name = std::string (t, 0, tab - t.c_str ());
      title = tab + 1;
    } 

    mp_editable_mode_action.reset (new Action (title));
#if defined(HAVE_QT)
    gtf::action_connect (mp_editable_mode_action->qaction (), SIGNAL (triggered ()), this, SLOT (toggle_editable_enabled ()));
#endif
    mp_editable_mode_action->set_checkable (true);
    mp_editable_mode_action->set_checked (m_editable_enabled);

    menu.insert_item ("edit_menu.select_menu.end", name, mp_editable_mode_action.get ());

  }

  //  add all the custom menus from the plugins
  std::vector<lay::MenuEntry> menu_entries;
  get_menu_entries (menu_entries);

  for (std::vector<lay::MenuEntry>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {

    if (! m->copy_from.empty ()) {

      menu.insert_item (m->insert_pos, m->menu_name, menu.action (m->copy_from));

    } else if (m->separator) {

      menu.insert_separator (m->insert_pos, m->menu_name);

    } else if (m->sub_menu) {

      menu.insert_menu (m->insert_pos, m->menu_name, m->title);

    } else {

      Action *action = 0;

      if (! m->cname.empty ()) {
        action = new ConfigureAction (m->title, m->cname, m->cvalue);
      } else {
        action = new GenericMenuAction (dispatcher, m->title, m->symbol);
      }

      m_menu_actions.push_back (action);
      menu.insert_item (m->insert_pos, m->menu_name, action);

      if (! m->exclusive_group.empty ()) {
        action->add_to_exclusive_group (&menu, m->exclusive_group);
      }

      if (m->checkable) {
        action->set_checkable (true);
      }

    }

  }

  //  Fill the mode menu file items from the mouse modes

  std::vector<std::pair<std::string, std::pair<std::string, int> > > modes;

  title = std::string ();
  if (implements_mouse_mode (title)) {
    modes.push_back (std::make_pair (title, std::make_pair ("edit_menu.mode_menu.end;@toolbar.end_modes", id ())));
  }

  //  the primary mouse modes (special for LayoutView)
  implements_primary_mouse_modes (modes);

  for (std::vector<std::pair<std::string, std::pair<std::string, int> > >::const_iterator m = modes.begin (); m != modes.end (); ++m) {

    //  extract first part, which is the name, separated by a tab from the title.
    std::string name;
    if (m->second.second <= 0) {
      name = tl::sprintf ("mode_i%d", 1 - m->second.second);
    } else {
      name = tl::sprintf ("mode_%d", m->second.second);
    }
    std::string title = m->first;

    const char *tab = strchr (title.c_str (), '\t');
    if (tab) {
      name = std::string (title, 0, tab - title.c_str ());
      title = std::string (tab + 1);
    } 

    mp_mouse_mode_action.reset (new ModeAction (dispatcher, title, m->second.second));
    mp_mouse_mode_action->add_to_exclusive_group (&menu, "mouse_mode_exclusive_group");
    mp_mouse_mode_action->set_checkable (true);

    menu.insert_item (m->second.first, name + ":mode_group", mp_mouse_mode_action.get ());

  }
}

void
PluginDeclaration::remove_menu_items (Dispatcher *dispatcher)
{
  lay::AbstractMenu *menu = dispatcher->menu ();
  menu->delete_items (mp_editable_mode_action.get ());
  menu->delete_items (mp_mouse_mode_action.get ());

  std::vector<lay::Action *> actions;
  for (tl::weak_collection <lay::Action>::iterator a = m_menu_actions.begin (); a != m_menu_actions.end (); ++a) {
    if (a.operator-> ()) {
      actions.push_back (a.operator-> ());
    }
  }
  for (std::vector<lay::Action *>::const_iterator a = actions.begin (); a != actions.end (); ++a) {
    menu->delete_items (*a);
  }
  m_menu_actions.clear ();
}

void 
PluginDeclaration::set_editable_enabled (bool f)
{
  if (f != m_editable_enabled) {
    m_editable_enabled = f;
    if (mp_editable_mode_action.get ()) {
      mp_editable_mode_action->set_checked (f);
    }
    editable_enabled_changed_event ();
  }
}

void  
PluginDeclaration::register_plugin ()
{
  if (Dispatcher::instance ()) {
    Dispatcher::instance ()->plugin_registered (this);
    initialize (Dispatcher::instance ());
  }
}

// ----------------------------------------------------------------
//  Plugin implementation

Plugin::Plugin (Plugin *parent, bool standalone)
  : mp_parent (parent), mp_plugin_declaration (0), dm_finalize_config (this, &lay::Plugin::config_end), m_standalone (standalone)
{
  if (! parent) {
    if (! standalone) {
      //  load the root with the default configuration
      for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
        std::vector< std::pair<std::string, std::string> > pairs;
        cls->get_options (pairs);
        m_repository.insert (pairs.begin (), pairs.end ());
      }
    }
  } else {
    mp_parent->m_children.push_back (this);
  }
}

Plugin::~Plugin ()
{
  if (mp_parent) {
    mp_parent->unregister_plugin (this);
  }
  //  remove us from the children's parent
  for (auto c = m_children.begin (); c != m_children.end (); ++c) {
    c->mp_parent = 0;
  }
}

void 
Plugin::config_setup ()
{
  do_config_setup (this);
  do_config_end ();
}
  
void 
Plugin::config_set (const std::string &name, const char *value)
{
  config_set (name, std::string (value));
}

void 
Plugin::config_set (const std::string &name, const std::string &value)
{
  std::map <std::string, std::string>::iterator m = m_repository.find (name);
  if (m != m_repository.end ()) {
    //  if the value did not change, do nothing 
    if (m->second == value) {
      return;
    } 
    m->second = value;
  } else {
    //  install the value in the repository
    m_repository.insert (std::make_pair (name, value));
  } 

  //  look for plugins that receive that configuration statically if the root is addressed
  if (! mp_parent && ! m_standalone) {
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      try {
        if ((const_cast<lay::PluginDeclaration *> (&*cls))->configure (name, value)) {
          return;
        }
      } catch (tl::Exception &ex) {
        tl::error << tl::to_string (tr ("Error on configure")) << " " << name << "='" << value << "': " << ex.msg ();
      }
    }
  }

  do_config_set (name, value, false);

  //  schedule a configuration finalization call (once for all config_set calls)
  dm_finalize_config ();
}

void 
Plugin::config_end ()
{
  //  finish static configurations for plugins if the root is addressed
  if (! mp_parent && ! m_standalone) {
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      (const_cast<lay::PluginDeclaration *> (&*cls))->config_finalize ();
    }
  }

  do_config_end ();
}

void
Plugin::clear_config ()
{
  m_repository.clear ();
  if (! mp_parent && ! m_standalone) {
    //  load the root with the default configuration
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      std::vector< std::pair<std::string, std::string> > pairs;
      cls->get_options (pairs);
      m_repository.insert (pairs.begin (), pairs.end ());
    }
  }
  config_setup ();
}

bool 
Plugin::config_get (const std::string &name, std::string &value) const
{
  std::map<std::string, std::string>::const_iterator p = m_repository.find (name);
  if (p != m_repository.end ()) {
    value = p->second;
    return true;
  } else if (mp_parent) {
    return mp_parent->config_get (name, value);
  } else {
    value = "";
    return false;
  }
}

void 
Plugin::get_config_names (std::vector<std::string> &names) const
{
  names.reserve (m_repository.size ());

  //  retrieve the configuration names
  for (std::map<std::string, std::string>::const_iterator p = m_repository.begin (); p != m_repository.end (); ++p) {
    names.push_back (p->first);
  }
}

Dispatcher *
Plugin::dispatcher ()
{
  Plugin *p = this;
  while (p->mp_parent) {
    p = p->mp_parent;
  }

  return dynamic_cast<Dispatcher *> (p);
}

void 
Plugin::do_config_setup (Plugin *target)
{
  if (mp_parent) {
    mp_parent->do_config_setup (target);
  } 
  //  local configurations override the parent's configuration, i.e. are applied after the parents settings
  for (std::map<std::string, std::string>::const_iterator p = m_repository.begin (); p != m_repository.end (); ++p) {
    target->do_config_set (p->first, p->second, false);
  }
}

void 
Plugin::do_config_end ()
{
  config_finalize ();
  for (tl::weak_collection<Plugin>::iterator c = m_children.begin (); c != m_children.end (); ++c) {
    c->do_config_end ();
  }
}

bool 
Plugin::do_config_set (const std::string &name, const std::string &value, bool for_child)
{
  if (for_child) {
    //  this is the case when we impose a configuration from the parent: in this case we
    //  have to remove it from the repository of local parameters.
    m_repository.erase (name);
  }

  try {
    if (configure (name, value)) {
      //  taken by us - don't propagate to the children
      return true;
    }
  } catch (tl::Exception &ex) {
    tl::error << tl::to_string (tr ("Error on configure")) << " " << name << "='" << value << "': " << ex.msg ();
  }

  //  propagate to all children (not only the first that takes it!)
  for (tl::weak_collection<Plugin>::iterator c = m_children.begin (); c != m_children.end (); ++c) {
    c->do_config_set (name, value, true);
  }

  return false;
}

// ---------------------------------------------------------------------------------------------------
//  Menu item generators

MenuEntry separator (const std::string &menu_name, const std::string &insert_pos)
{
  MenuEntry e;
  e.menu_name = menu_name;
  e.insert_pos = insert_pos;
  e.separator = true;
  return e;
}

MenuEntry menu_item (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &title)
{
  MenuEntry e;
  e.symbol = symbol;
  e.menu_name = menu_name;
  e.insert_pos = insert_pos;
  e.title = title;
  return e;
}

MenuEntry menu_item_copy (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &copy_from)
{
  MenuEntry e;
  e.symbol = symbol;
  e.menu_name = menu_name;
  e.insert_pos = insert_pos;
  e.copy_from = copy_from;
  return e;
}

MenuEntry submenu (const std::string &menu_name, const std::string &insert_pos, const std::string &title)
{
  MenuEntry e;
  e.menu_name = menu_name;
  e.insert_pos = insert_pos;
  e.title = title;
  e.sub_menu = true;
  return e;
}

MenuEntry submenu (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &title)
{
  MenuEntry e;
  e.symbol = symbol;
  e.menu_name = menu_name;
  e.insert_pos = insert_pos;
  e.title = title;
  e.sub_menu = true;
  return e;
}

MenuEntry config_menu_item (const std::string &menu_name, const std::string &insert_pos, const std::string &title, const std::string &cname, const std::string &cvalue)
{
  MenuEntry e;
  e.menu_name = menu_name;
  e.insert_pos = insert_pos;
  e.title = title;
  e.cname = cname;
  e.cvalue = cvalue;
  return e;
}

}
