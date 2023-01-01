
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


#include "layDispatcher.h"
#include "layAbstractMenu.h"

#include "tlXMLParser.h"
#include "tlXMLWriter.h"

namespace lay
{

// ----------------------------------------------------------------
//  Dispatcher implementation

static Dispatcher *ms_dispatcher_instance = 0;

Dispatcher::Dispatcher (Plugin *parent, bool standalone)
  : Plugin (parent, standalone),
#if defined(HAVE_QT)
    mp_menu_parent_widget (0),
#endif
    mp_delegate (0)
{
  if (! parent && ! ms_dispatcher_instance) {
    ms_dispatcher_instance = this;
  }
}

Dispatcher::Dispatcher (DispatcherDelegate *delegate, Plugin *parent, bool standalone)
  : Plugin (parent, standalone),
#if defined(HAVE_QT)
    mp_menu_parent_widget (0),
#endif
    mp_delegate (delegate)
{
  if (! parent && ! ms_dispatcher_instance) {
    ms_dispatcher_instance = this;
  }
}

Dispatcher::~Dispatcher ()
{
  if (ms_dispatcher_instance == this) {
    ms_dispatcher_instance = 0;
  }
}

#if defined(HAVE_QT)
void Dispatcher::set_menu_parent_widget (QWidget *menu_parent_widget)
{
  mp_menu_parent_widget = menu_parent_widget;
}
#endif

void Dispatcher::make_menu ()
{
  mp_menu.reset (new lay::AbstractMenu (this));
}

bool
Dispatcher::configure (const std::string &name, const std::string &value)
{
  if (mp_menu) {
    std::vector<lay::ConfigureAction *> ca = mp_menu->configure_actions (name);
    for (std::vector<lay::ConfigureAction *>::const_iterator a = ca.begin (); a != ca.end (); ++a) {
      (*a)->configure (value);
    }
  }

  if (mp_delegate) {
    return mp_delegate->configure (name, value);
  } else {
    return false;
  }
}

void
Dispatcher::config_finalize ()
{
  if (mp_delegate) {
    return mp_delegate->config_finalize ();
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

  void start (const lay::Dispatcher &owner)
  {
    mp_owner = &owner;
    m_done = false;
  }

  void next ()
  {
    m_done = true;
  }

private:
  const lay::Dispatcher *mp_owner;
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

  void start (const lay::Dispatcher & /*owner*/) { }
  void next () { }
};

struct ConfigNamedSetAdaptor
{
  ConfigNamedSetAdaptor ()
  {
    // .. nothing yet ..
  }

  void operator () (lay::Dispatcher &w, tl::XMLReaderState &reader, const std::string &name) const
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

  void operator () (lay::Dispatcher &w, tl::XMLReaderState &reader) const
  {
    tl::XMLObjTag<std::string> tag;
    w.config_set (m_name, *reader.back (tag));
  }

private:
  std::string m_name;
};

//  the configuration file's XML structure is built dynamically
static tl::XMLStruct<lay::Dispatcher>
config_structure (const lay::Dispatcher *plugin)
{
  tl::XMLElementList body;
  std::string n_with_underscores;

  std::vector <std::string> names;
  plugin->get_config_names (names);

  for (std::vector <std::string>::const_iterator n = names.begin (); n != names.end (); ++n) {

    body.append (tl::XMLMember<std::string, lay::Dispatcher, ConfigGetAdaptor, ConfigSetAdaptor, tl::XMLStdConverter <std::string> > (
                       ConfigGetAdaptor (*n), ConfigSetAdaptor (*n), *n));

    //  for compatibility, provide an alternative with underscores (i.e. 0.20->0.21 because of default_grids)
    n_with_underscores.clear ();
    for (const char *c = n->c_str (); *c; ++c) {
      n_with_underscores += (*c == '-' ? '_' : *c);
    }

    body.append (tl::XMLMember<std::string, lay::Dispatcher, ConfigGetNullAdaptor, ConfigSetAdaptor, tl::XMLStdConverter <std::string> > (
                       ConfigGetNullAdaptor (), ConfigSetAdaptor (*n), n_with_underscores));

  }

  //  add a wildcard member to read all others unspecifically into the repository
  body.append (tl::XMLWildcardMember<std::string, lay::Dispatcher, ConfigNamedSetAdaptor, tl::XMLStdConverter <std::string> > (ConfigNamedSetAdaptor ()));

  return tl::XMLStruct<lay::Dispatcher> ("config", body);
}


bool
Dispatcher::write_config (const std::string &config_file)
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
Dispatcher::read_config (const std::string &config_file)
{
  std::unique_ptr<tl::XMLFileSource> file;

  try {
    file.reset (new tl::XMLFileSource (config_file));
  } catch (...) {
    return false;
  }

  try {
    config_structure (this).parse (*file, *this);
  } catch (tl::Exception &ex) {
    std::string msg = tl::to_string (tr ("Problem reading config file ")) + config_file + ": " + ex.msg ();
    throw tl::Exception (msg);
  }

  config_end ();

  return true;
}

Dispatcher *
Dispatcher::instance ()
{
  return ms_dispatcher_instance;
}

}
