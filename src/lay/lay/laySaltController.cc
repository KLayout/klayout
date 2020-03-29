
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

#include "laySaltController.h"
#include "laySaltManagerDialog.h"
#include "laySaltDownloadManager.h"
#include "layConfig.h"
#include "layMainWindow.h"
#include "layQtTools.h"
#include "tlLog.h"

#include <QDir>

namespace lay
{

static const std::string cfg_salt_manager_window_state ("salt-manager-window-state");

SaltController::SaltController ()
  : mp_salt_dialog (0), mp_mw (0), mp_plugin_root (0), m_file_watcher (0),
    dm_sync_file_watcher (this, &SaltController::sync_file_watcher),
    dm_sync_files (this, &SaltController::sync_files)
{
}

void
SaltController::initialize (lay::Dispatcher *root)
{
  mp_mw = lay::MainWindow::instance ();
  mp_plugin_root = root;
}

void
SaltController::initialized (lay::Dispatcher * /*root*/)
{
  if (! m_file_watcher) {
    m_file_watcher = new tl::FileSystemWatcher (this);
    connect (m_file_watcher, SIGNAL (fileChanged (const QString &)), this, SLOT (file_watcher_triggered ()));
    connect (m_file_watcher, SIGNAL (fileRemoved (const QString &)), this, SLOT (file_watcher_triggered ()));
  }

  connect (&m_salt, SIGNAL (collections_changed ()), this, SIGNAL (salt_changed ()));
}

void
SaltController::uninitialize (lay::Dispatcher * /*root*/)
{
  disconnect (&m_salt, SIGNAL (collections_changed ()), this, SIGNAL (salt_changed ()));

  if (m_file_watcher) {
    disconnect (m_file_watcher, SIGNAL (fileChanged (const QString &)), this, SLOT (file_watcher_triggered ()));
    disconnect (m_file_watcher, SIGNAL (fileRemoved (const QString &)), this, SLOT (file_watcher_triggered ()));
    delete m_file_watcher;
    m_file_watcher = 0;
  }

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
SaltController::can_exit (lay::Dispatcher * /*root*/) const
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

    mp_salt_dialog = new lay::SaltManagerDialog (mp_mw, &m_salt, m_salt_mine_url);

  }

  if (mp_salt_dialog) {

    std::string s = mp_plugin_root->config_get (cfg_salt_manager_window_state);
    if (! s.empty ()) {
      lay::restore_dialog_state (mp_salt_dialog, s);
    }

    //  while running the dialog, don't watch file events - that would interfere with
    //  the changes applied by the dialog itself.
    m_file_watcher->enable (false);
    mp_salt_dialog->exec ();
    m_file_watcher->enable (true);

    mp_plugin_root->config_set (cfg_salt_manager_window_state, lay::save_dialog_state (mp_salt_dialog));

    sync_file_watcher ();

  }
}

void
SaltController::sync_file_watcher ()
{
  if (m_file_watcher) {
    m_file_watcher->clear ();
    m_file_watcher->enable (false);
    for (lay::Salt::flat_iterator g = m_salt.begin_flat (); g != m_salt.end_flat (); ++g) {
      m_file_watcher->add_file ((*g)->path ());
    }
    m_file_watcher->enable (true);
  }
}

void
SaltController::sync_files ()
{
  tl::log << tl::to_string (tr ("Detected file system change in packages - updating"));
  emit salt_changed ();
}

bool
SaltController::install_packages (const std::vector<std::string> &packages, bool with_dep)
{
  lay::SaltDownloadManager manager;

  lay::Salt salt_mine;
  if (! m_salt_mine_url.empty ()) {
    tl::log << tl::to_string (tr ("Downloading package repository from %1").arg (tl::to_qstring (m_salt_mine_url)));
    salt_mine.load (m_salt_mine_url);
  }

  for (std::vector<std::string>::const_iterator p = packages.begin (); p != packages.end (); ++p) {

    if (p->empty ()) {
      continue;
    }

    std::string v, n = *p;

    size_t br = p->find ("(");
    if (br != std::string::npos) {
      n = std::string (*p, 0, br);
      v = std::string (*p, br + 1);
      size_t brr = v.find (")");
      if (brr != std::string::npos) {
        v = std::string (v, 0, brr);
      }
    }

    if (n.find ("http:") == 0 || n.find ("https:") == 0 || n.find ("file:") == 0 || n[0] == '/' || n[0] == '\\') {
      //  it's a URL
      manager.register_download (std::string (), std::string (), n, v);
    } else {
      //  it's a plain name
      manager.register_download (n, std::string (), std::string (), v);
    }

  }

  if (with_dep) {
    manager.compute_dependencies (m_salt, salt_mine);
  } else {
    manager.compute_packages (m_salt, salt_mine);
  }

  return manager.execute (0, m_salt);
}

void
SaltController::add_path (const std::string &path)
{
  try {

    std::string tp = tl::to_string (QDir (tl::to_qstring (path)).filePath (QString::fromUtf8 ("salt")));

    tl::log << tl::to_string (tr ("Scanning %1 for packages").arg (tl::to_qstring (tp)));
    m_salt.add_location (tp);

    dm_sync_file_watcher ();

  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
  }
}

void
SaltController::file_watcher_triggered ()
{
  dm_sync_files ();
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

