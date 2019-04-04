
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "layAbstractMenuProvider.h"
#include "laybasicCommon.h"

#include "tlException.h"
#include "tlAssert.h"
#include "tlXMLParser.h"
#include "tlLog.h"

#include "layPlugin.h"
#include "tlExceptions.h"
#include "tlClassRegistry.h"

#include "gtf.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <QObject>

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
  if (PluginRoot::instance ()) {
    PluginRoot::instance ()->plugin_removed (this);
  }
}

void 
PluginDeclaration::toggle_editable_enabled ()
{
  BEGIN_PROTECTED
  set_editable_enabled (! editable_enabled ());
  END_PROTECTED
}

void 
PluginDeclaration::generic_menu ()
{
  BEGIN_PROTECTED

  QAction *action = dynamic_cast <QAction *> (sender ());
  tl_assert (action);

  std::string symbol = tl::to_string (action->data ().toString ());

  //  Global handler: give the declaration a chance to handle the menu request globally
  if (menu_activated (symbol)) {
    return;
  }

  //  Forward the request to the plugin root which will propagate it down to the plugins
  lay::PluginRoot::instance ()->menu_activated (symbol);

  END_PROTECTED
}

void 
PluginDeclaration::mode_triggered ()
{
  BEGIN_PROTECTED

  QAction *action = dynamic_cast<QAction *> (sender ());
  if (action) {

    int mode = action->data ().toInt ();

    if (lay::PluginRoot::instance ()) {
      lay::PluginRoot::instance ()->select_mode (mode);
    }

    action->setChecked (true);

  }

  END_PROTECTED
}

void 
PluginDeclaration::init_menu ()
{
  if (! lay::AbstractMenuProvider::instance () || ! lay::AbstractMenuProvider::instance ()->menu ()) {
    return;
  }

  lay::AbstractMenu &menu = *lay::AbstractMenuProvider::instance ()->menu ();

  //  pre-initialize to allow multiple init_menu calls
  m_editable_mode_action = lay::Action ();
  m_mouse_mode_action = lay::Action ();
  m_menu_actions.clear ();

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

    m_editable_mode_action = Action (title);
    gtf::action_connect (m_editable_mode_action.qaction (), SIGNAL (triggered ()), this, SLOT (toggle_editable_enabled ()));
    m_editable_mode_action.qaction ()->setData (id ());
    m_editable_mode_action.set_checkable (true);
    m_editable_mode_action.set_checked (m_editable_enabled);

    menu.insert_item ("edit_menu.select_menu.end", name, m_editable_mode_action);

  }

  //  add all the custom menus from the plugins
  std::vector<lay::MenuEntry> menu_entries;
  get_menu_entries (menu_entries);

  for (std::vector<lay::MenuEntry>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {

    if (m->title.empty ()) {
      menu.insert_separator (m->insert_pos, m->menu_name);
    } else {

      if (m->sub_menu) {
        menu.insert_menu (m->insert_pos, m->menu_name, m->title);
      } else {

        Action action (m->title);
        action.qaction ()->setData (QVariant (tl::to_qstring (m->symbol)));
        gtf::action_connect (action.qaction (), SIGNAL (triggered ()), this, SLOT (generic_menu ()));
        menu.insert_item (m->insert_pos, m->menu_name, action);

        m_menu_actions.push_back (action);

      }

    }

  }

  //  Fill the mode menu file items from the mouse modes 

  title = std::string ();
  if (implements_mouse_mode (title)) {

    //  extract first part, which is the name, separated by a tab from the title.
    std::string name = tl::sprintf ("mode_%d", id ());
    const char *tab = strchr (title.c_str (), '\t');
    if (tab) {
      name = std::string (title, 0, tab - title.c_str ());
      title = std::string (tab + 1);
    } 

    m_mouse_mode_action = Action (title);
    m_mouse_mode_action.add_to_exclusive_group (&menu, "mouse_mode_exclusive_group");

    m_mouse_mode_action.set_checkable (true);
    m_mouse_mode_action.qaction ()->setData (QVariant (id ()));

    menu.insert_item ("edit_menu.mode_menu.end", name, m_mouse_mode_action);
    menu.insert_item ("@toolbar.end_modes", name, m_mouse_mode_action);

    gtf::action_connect (m_mouse_mode_action.qaction (), SIGNAL (triggered ()), this, SLOT (mode_triggered ()));

  }
}

void
PluginDeclaration::remove_menu_items ()
{
  if (! lay::AbstractMenuProvider::instance () || ! lay::AbstractMenuProvider::instance ()->menu ()) {
    return;
  }

  lay::AbstractMenu *menu = lay::AbstractMenuProvider::instance ()->menu ();
  menu->delete_items (m_editable_mode_action);
  menu->delete_items (m_mouse_mode_action);
  for (std::vector <lay::Action>::const_iterator a = m_menu_actions.begin (); a != m_menu_actions.end (); ++a) {
    menu->delete_items (*a);
  }
}

void 
PluginDeclaration::set_editable_enabled (bool f)
{
  if (f != m_editable_enabled) {
    m_editable_enabled = f;
    m_editable_mode_action.set_checked (f);
    editable_enabled_changed_event ();
  }
}

void  
PluginDeclaration::register_plugin ()
{
  if (PluginRoot::instance ()) {
    PluginRoot::instance ()->plugin_registered (this);
    initialize (PluginRoot::instance ());
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
  // .. nothing yet ..
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
        tl::error << tl::to_string (QObject::tr ("Error on configure")) << " " << name << "='" << value << "': " << ex.msg ();
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
  //  finish static conifgurations for plugins if the root is addressed
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

PluginRoot *
Plugin::plugin_root ()
{
  PluginRoot *pr = plugin_root_maybe_null ();
  tl_assert (pr != 0);
  return pr;
}

PluginRoot *
Plugin::plugin_root_maybe_null ()
{
  Plugin *p = this;
  while (p->mp_parent) {
    p = p->mp_parent;
  }

  return dynamic_cast<PluginRoot *> (p);
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
    tl::error << tl::to_string (QObject::tr ("Error on configure")) << " " << name << "='" << value << "': " << ex.msg ();
  }

  //  propagate to all children (not only the first that takes it!)
  for (tl::weak_collection<Plugin>::iterator c = m_children.begin (); c != m_children.end (); ++c) {
    c->do_config_set (name, value, true);
  }

  return false;
}

// ----------------------------------------------------------------
//  PluginRoot implementation

static PluginRoot *ms_root_instance = 0;

PluginRoot::PluginRoot (bool standalone)
  : Plugin (0, standalone)
{
  ms_root_instance = this;
}

PluginRoot::~PluginRoot ()
{
  if (ms_root_instance == this) {
    ms_root_instance = 0;
  }
}

//  Writing and Reading of configuration 

struct ConfigGetAdaptor
{
  ConfigGetAdaptor (const std::string &name)
    : mp_owner (0), m_done (false), m_name (name)
  {
    // .. nothing yet ..
  }
  
  std::string operator () () const
  {
    std::string s;
    mp_owner->config_get (m_name, s);
    return s;
  }

  bool at_end () const 
  {
    return m_done;
  }

  void start (const lay::PluginRoot &owner) 
  {
    mp_owner = &owner;
    m_done = false;
  }

  void next () 
  {
    m_done = true;
  }

private:
  const lay::PluginRoot *mp_owner;
  bool m_done;
  std::string m_name;
};
 
struct ConfigGetNullAdaptor
{
  ConfigGetNullAdaptor ()
  {
    // .. nothing yet ..
  }
  
  std::string operator () () const
  {
    return std::string ();
  }

  bool at_end () const 
  {
    return true;
  }

  void start (const lay::PluginRoot & /*owner*/) { }
  void next () { }
};
 
struct ConfigNamedSetAdaptor
{
  ConfigNamedSetAdaptor ()
  {
    // .. nothing yet ..
  }
  
  void operator () (lay::PluginRoot &w, tl::XMLReaderState &reader, const std::string &name) const
  {
    tl::XMLObjTag<std::string> tag;
    w.config_set (name, *reader.back (tag));
  }
};
 
struct ConfigSetAdaptor
{
  ConfigSetAdaptor (const std::string &name)
    : m_name (name)
  {
    // .. nothing yet ..
  }
  
  void operator () (lay::PluginRoot &w, tl::XMLReaderState &reader) const
  {
    tl::XMLObjTag<std::string> tag;
    w.config_set (m_name, *reader.back (tag));
  }

private:
  std::string m_name;
};
 
//  the configuration file's XML structure is built dynamically
static tl::XMLStruct<lay::PluginRoot> 
config_structure (const lay::PluginRoot *plugin) 
{
  tl::XMLElementList body;
  std::string n_with_underscores;

  std::vector <std::string> names; 
  plugin->get_config_names (names);

  for (std::vector <std::string>::const_iterator n = names.begin (); n != names.end (); ++n) {

    body.append (tl::XMLMember<std::string, lay::PluginRoot, ConfigGetAdaptor, ConfigSetAdaptor, tl::XMLStdConverter <std::string> > ( 
                       ConfigGetAdaptor (*n), ConfigSetAdaptor (*n), *n));

    //  for compatibility, provide an alternative with underscores (i.e. 0.20->0.21 because of default_grids)
    n_with_underscores.clear ();
    for (const char *c = n->c_str (); *c; ++c) {
      n_with_underscores += (*c == '-' ? '_' : *c);
    }

    body.append (tl::XMLMember<std::string, lay::PluginRoot, ConfigGetNullAdaptor, ConfigSetAdaptor, tl::XMLStdConverter <std::string> > ( 
                       ConfigGetNullAdaptor (), ConfigSetAdaptor (*n), n_with_underscores));

  }

  //  add a wildcard member to read all others unspecifically into the repository
  body.append (tl::XMLWildcardMember<std::string, lay::PluginRoot, ConfigNamedSetAdaptor, tl::XMLStdConverter <std::string> > (ConfigNamedSetAdaptor ()));

  return tl::XMLStruct<lay::PluginRoot> ("config", body);
}


bool 
PluginRoot::write_config (const std::string &config_file)
{
  try {
    tl::OutputStream os (config_file, tl::OutputStream::OM_Plain);
    config_structure (this).write (os, *this); 
    return true;
  } catch (...) {
    return false;
  }
}

bool
PluginRoot::read_config (const std::string &config_file)
{
  std::auto_ptr<tl::XMLFileSource> file;

  try {
    file.reset (new tl::XMLFileSource (config_file));
  } catch (...) {
    return false;
  }

  try {
    config_structure (this).parse (*file, *this); 
  } catch (tl::Exception &ex) {
    std::string msg = tl::to_string (QObject::tr ("Problem reading config file ")) + config_file + ": " + ex.msg ();
    throw tl::Exception (msg);
  } 

  config_end ();

  return true;
}

PluginRoot *
PluginRoot::instance ()
{
  return ms_root_instance;
}

}
