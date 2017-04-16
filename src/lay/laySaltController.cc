
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

#include "laySaltController.h"
#include "laySaltManagerDialog.h"
#include "layConfig.h"
#include "layMainWindow.h"
#include "layQtTools.h"
#include "tlLog.h"

namespace lay
{

static const std::string cfg_salt_manager_window_state ("salt-manager-window-state");

SaltController::SaltController ()
  : mp_salt_dialog (0), mp_mw (0)
{
  //  .. nothing yet ..
}

void
SaltController::initialized (lay::PluginRoot *root)
{
  mp_mw = dynamic_cast <lay::MainWindow *> (root);

  connect (&m_salt, SIGNAL (collections_changed ()), this, SIGNAL (salt_changed ()));
}

void
SaltController::uninitialize (lay::PluginRoot * /*root*/)
{
  disconnect (&m_salt, SIGNAL (collections_changed ()), this, SIGNAL (salt_changed ()));

  delete mp_salt_dialog;
  mp_salt_dialog = 0;
  mp_mw = 0;
}

void
SaltController::get_options (std::vector < std::pair<std::string, std::string> > &options) const
{
  options.push_back (std::pair<std::string, std::string> (cfg_salt_manager_window_state, ""));
}

void
SaltController::get_menu_entries (std::vector<lay::MenuEntry> & /*menu_entries*/) const
{
  //  .. nothing yet ..
}

bool
SaltController::configure (const std::string & /*name*/, const std::string & /*value*/)
{
  return false;
}

void
SaltController::config_finalize()
{
  //  .. nothing yet ..
}

bool
SaltController::can_exit (lay::PluginRoot * /*root*/) const
{
  //  .. nothing yet ..
  return true;
}

bool
SaltController::accepts_drop (const std::string & /*path_or_url*/) const
{
  //  .. nothing yet ..
  return false;
}

void
SaltController::drop_url (const std::string & /*path_or_url*/)
{
  //  .. nothing yet ..
}

void
SaltController::show_editor ()
{
  if (mp_mw && !mp_salt_dialog) {

    try {
      if (! m_salt_mine_url.empty ()) {
        tl::log << tl::to_string (tr ("Downloading package repository from %1").arg (tl::to_qstring (m_salt_mine_url)));
        m_salt_mine.load (m_salt_mine_url);
      }
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
    }

    mp_salt_dialog = new lay::SaltManagerDialog (mp_mw, &m_salt, &m_salt_mine);

  }

  if (mp_salt_dialog) {

    if (mp_mw) {
      std::string s = mp_mw->config_get (cfg_salt_manager_window_state);
      if (! s.empty ()) {
        lay::restore_dialog_state (mp_salt_dialog, s);
      }
    }

    mp_salt_dialog->exec ();

    if (mp_mw) {
      mp_mw->config_set (cfg_salt_manager_window_state, lay::save_dialog_state (mp_salt_dialog));
    }

  }
}

void
SaltController::add_path (const std::string &path)
{
  try {
    tl::log << tl::to_string (tr ("Scanning %1 for packages").arg (tl::to_qstring (path)));
    m_salt.add_location (path);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
  }
}

void
SaltController::set_salt_mine_url (const std::string &url)
{
  m_salt_mine_url = url;
}

SaltController *
SaltController::instance ()
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    SaltController *sc = dynamic_cast <SaltController *> (cls.operator-> ());
    if (sc) {
      return sc;
    }
  }
  return 0;
}

//  The singleton instance of the salt controller
static tl::RegisteredClass<lay::PluginDeclaration> salt_controller_decl (new lay::SaltController (), 100, "SaltController");

}

