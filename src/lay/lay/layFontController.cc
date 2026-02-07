
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

#include "layFontController.h"
#include "layApplication.h"
#include "laySaltController.h"
#include "layConfig.h"
#include "layMainWindow.h"
#include "dbGlyphs.h"
#include "tlLog.h"

#include <QDir>

namespace lay
{

FontController::FontController ()
  : m_file_watcher (0),
    dm_sync_dirs (this, &FontController::sync_dirs)
{
}

void
FontController::initialize (lay::Dispatcher * /*root*/)
{
  //  NOTE: we initialize the dirs in the stage once to have them available for the autorun
  //  macros. We'll do that later again in order to pull in the dirs from the packages.
  sync_dirs ();
}

void
FontController::initialized (lay::Dispatcher * /*root*/)
{
  if (lay::SaltController::instance ()) {
    connect (lay::SaltController::instance (), SIGNAL (salt_changed ()), this, SLOT (sync_with_external_sources ()));
  }

  if (! m_file_watcher) {
    m_file_watcher = new tl::FileSystemWatcher (this);
    connect (m_file_watcher, SIGNAL (fileChanged (const QString &)), this, SLOT (file_watcher_triggered ()));
    connect (m_file_watcher, SIGNAL (fileRemoved (const QString &)), this, SLOT (file_watcher_triggered ()));
  }

  sync_dirs ();
}

void
FontController::uninitialize (lay::Dispatcher * /*root*/)
{
  if (m_file_watcher) {
    disconnect (m_file_watcher, SIGNAL (fileChanged (const QString &)), this, SLOT (file_watcher_triggered ()));
    disconnect (m_file_watcher, SIGNAL (fileRemoved (const QString &)), this, SLOT (file_watcher_triggered ()));
    delete m_file_watcher;
    m_file_watcher = 0;
  }

  if (lay::SaltController::instance ()) {
    disconnect (lay::SaltController::instance (), SIGNAL (salt_changed ()), this, SLOT (sync_with_external_sources ()));
  }
}

void
FontController::get_options (std::vector < std::pair<std::string, std::string> > & /*options*/) const
{
  //  .. nothing yet ..
}

void
FontController::get_menu_entries (std::vector<lay::MenuEntry> & /*menu_entries*/) const
{
  //  .. nothing yet ..
}

bool
FontController::configure (const std::string & /*name*/, const std::string & /*value*/)
{
  return false;
}

void
FontController::config_finalize()
{
  //  .. nothing yet ..
}

bool
FontController::can_exit (lay::Dispatcher * /*root*/) const
{
  //  .. nothing yet ..
  return true;
}

void
FontController::sync_dirs ()
{
  if (m_file_watcher) {
    m_file_watcher->clear ();
    m_file_watcher->enable (false);
  }

  std::vector<std::string> paths = lay::ApplicationBase::instance ()->klayout_path ();

  //  add the salt grains as potential sources for library definitions

  lay::SaltController *sc = lay::SaltController::instance ();
  if (sc) {
    for (lay::Salt::flat_iterator g = sc->salt ().begin_flat (); g != sc->salt ().end_flat (); ++g) {
      paths.push_back ((*g)->path ());
    }
  }

  //  scan for font directories

  std::vector<std::string> font_paths;

  for (std::vector <std::string>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    QDir fp = QDir (tl::to_qstring (*p)).filePath (tl::to_qstring ("fonts"));
    if (fp.exists ()) {
      if (m_file_watcher) {
        m_file_watcher->add_file (tl::to_string (fp.absolutePath ()));
      }
      font_paths.push_back (tl::to_string (fp.absolutePath ()));
    }
  }

  db::TextGenerator::set_font_paths (font_paths);

  if (m_file_watcher) {
    m_file_watcher->enable (true);
  }
}

void
FontController::sync_with_external_sources ()
{
  tl::log << tl::to_string (tr ("Package updates - updating fonts"));
  dm_sync_dirs ();
}

void
FontController::file_watcher_triggered ()
{
  tl::log << tl::to_string (tr ("Detected file system change in fonts - updating"));
  dm_sync_dirs ();
}

FontController *
FontController::instance ()
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    FontController *sc = dynamic_cast <FontController *> (cls.operator-> ());
    if (sc) {
      return sc;
    }
  }
  return 0;
}

//  The singleton instance of the library controller
static tl::RegisteredClass<lay::PluginDeclaration> font_controller_decl (new lay::FontController (), 160, "FontController");

}

