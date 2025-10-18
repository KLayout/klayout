
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

#include "layLibraryController.h"
#include "layTechnology.h"
#include "layApplication.h"
#include "laySaltController.h"
#include "layConfig.h"
#include "layMainWindow.h"
#include "layQtTools.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbReader.h"
#include "tlLog.h"
#include "tlStream.h"
#include "tlFileUtils.h"

#include <QDir>

namespace lay
{

// -------------------------------------------------------------------------------------------

class FileBasedLibrary
  : public db::Library
{
public:
  FileBasedLibrary (const std::string &path)
    : db::Library (), m_path (path)
  {
    set_description (tl::filename (path));
  }

  virtual std::string reload ()
  {
    std::string name = tl::basename (m_path);

    layout ().clear ();

    tl::InputStream stream (m_path);
    db::Reader reader (stream);
    reader.read (layout ());

    //  Use the libname if there is one
    db::Layout::meta_info_name_id_type libname_name_id = layout ().meta_info_name_id ("libname");
    for (db::Layout::meta_info_iterator m = layout ().begin_meta (); m != layout ().end_meta (); ++m) {
      if (m->first == libname_name_id && ! m->second.value.is_nil ()) {
        name = m->second.value.to_string ();
        break;
      }
    }

    return name;
  }

private:
  std::string m_path;
};

// -------------------------------------------------------------------------------------------

LibraryController::LibraryController ()
  : m_file_watcher (0),
    dm_sync_files (this, &LibraryController::sync_files)
{
}

void
LibraryController::initialize (lay::Dispatcher * /*root*/)
{
  //  NOTE: we initialize the libraries in the stage once to have them available for the autorun
  //  macros. We'll do that later again in order to pull in the libraries from the packages.
  sync_files ();
}

void
LibraryController::initialized (lay::Dispatcher * /*root*/)
{
  if (lay::SaltController::instance ()) {
    connect (lay::SaltController::instance (), SIGNAL (salt_changed ()), this, SLOT (sync_with_external_sources ()));
  }

  if (! m_file_watcher) {
    m_file_watcher = new tl::FileSystemWatcher (this);
    connect (m_file_watcher, SIGNAL (fileChanged (const QString &)), this, SLOT (file_watcher_triggered ()));
    connect (m_file_watcher, SIGNAL (fileRemoved (const QString &)), this, SLOT (file_watcher_triggered ()));
  }

  sync_files ();
}

void
LibraryController::uninitialize (lay::Dispatcher * /*root*/)
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
LibraryController::get_options (std::vector < std::pair<std::string, std::string> > & /*options*/) const
{
  //  .. nothing yet ..
}

void
LibraryController::get_menu_entries (std::vector<lay::MenuEntry> & /*menu_entries*/) const
{
  //  .. nothing yet ..
}

bool
LibraryController::configure (const std::string & /*name*/, const std::string & /*value*/)
{
  return false;
}

void
LibraryController::config_finalize()
{
  //  .. nothing yet ..
}

bool
LibraryController::can_exit (lay::Dispatcher * /*root*/) const
{
  //  .. nothing yet ..
  return true;
}

void
LibraryController::sync_files ()
{
  if (m_file_watcher) {
    m_file_watcher->clear ();
    m_file_watcher->enable (false);
  }

  std::map<std::string, LibInfo> new_lib_files;

  //  build a list of paths vs. technology
  std::vector<std::pair<std::string, std::string> > paths;

  std::vector<std::string> klayout_path = lay::ApplicationBase::instance ()->klayout_path ();
  for (std::vector<std::string>::const_iterator p = klayout_path.begin (); p != klayout_path.end (); ++p) {
    paths.push_back (std::make_pair (*p, std::string ()));
  }

  //  add the salt grains as potential sources for library definitions

  lay::SaltController *sc = lay::SaltController::instance ();
  if (sc) {
    for (lay::Salt::flat_iterator g = sc->salt ().begin_flat (); g != sc->salt ().end_flat (); ++g) {
      paths.push_back (std::make_pair ((*g)->path (), std::string ()));
    }
  }

  //  add the technologies as potential sources for library definitions

  for (db::Technologies::iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
    if (! t->base_path ().empty ()) {
      paths.push_back (std::make_pair (t->base_path (), t->name ()));
    }
  }

  //  scan for libraries

  for (std::vector <std::pair<std::string, std::string> >::const_iterator p = paths.begin (); p != paths.end (); ++p) {

    QDir lp = QDir (tl::to_qstring (p->first)).filePath (tl::to_qstring ("libraries"));
    if (lp.exists ()) {

      if (m_file_watcher) {
        m_file_watcher->add_file (tl::to_string (lp.absolutePath ()));
      }

      QStringList name_filters;
      name_filters << QString::fromUtf8 ("*");

      QStringList libs = lp.entryList (name_filters, QDir::Files);
      for (QStringList::const_iterator im = libs.begin (); im != libs.end (); ++im) {

        std::string lib_path = tl::to_string (lp.absoluteFilePath (*im));

        try {

          QFileInfo fi (tl::to_qstring (lib_path));

          bool needs_load = false;
          std::map<std::string, LibInfo>::iterator ll = m_lib_files.find (lib_path);
          if (ll == m_lib_files.end ()) {
            needs_load = true;
          } else {
            if (fi.lastModified () > ll->second.time) {
              needs_load = true;
            } else {
              new_lib_files.insert (*ll);
            }
          }

          if (needs_load) {

            std::unique_ptr<db::Library> lib (new FileBasedLibrary (lib_path));
            if (! p->second.empty ()) {
              lib->set_technology (p->second);
            }

            tl::log << "Reading library '" << lib_path << "'";
            lib->set_name (lib->reload ());

            if (! p->second.empty ()) {
              tl::log << "Registering as '" << lib->get_name () << "' for tech '" << p->second << "'";
            } else {
              tl::log << "Registering as '" << lib->get_name () << "'";
            }

            LibInfo li;
            li.name = lib->get_name ();
            li.time = fi.lastModified ();
            if (! p->second.empty ()) {
              li.tech.insert (p->second);
            }
            new_lib_files.insert (std::make_pair (lib_path, li));

            db::LibraryManager::instance ().register_lib (lib.release ());

          }

        } catch (tl::Exception &ex) {
          tl::error << ex.msg ();
        }

      }

    }

  }

  if (m_file_watcher) {
    m_file_watcher->enable (true);
  }

  //  remove libraries which are no longer present

  std::set<std::string> new_names;

  for (std::map<std::string, LibInfo>::const_iterator lf = new_lib_files.begin (); lf != new_lib_files.end (); ++lf) {
    new_names.insert (lf->second.name);
  }

  for (std::map<std::string, LibInfo>::const_iterator lf = m_lib_files.begin (); lf != m_lib_files.end (); ++lf) {
    if (new_names.find (lf->second.name) == new_names.end ()) {
      try {
        std::pair<bool, db::lib_id_type> li = db::LibraryManager::instance ().lib_by_name (lf->second.name, lf->second.tech);
        if (li.first) {
          db::LibraryManager::instance ().delete_lib (db::LibraryManager::instance ().lib (li.second));
        }
      } catch (...) {
      }
    }
  }

  //  establish the new libraries

  m_lib_files = new_lib_files;
}

void
LibraryController::sync_with_external_sources ()
{
  tl::log << tl::to_string (tr ("Package updates - updating libraries"));
  dm_sync_files ();
}

void
LibraryController::file_watcher_triggered ()
{
  tl::log << tl::to_string (tr ("Detected file system change in libraries - updating"));
  dm_sync_files ();
}

LibraryController *
LibraryController::instance ()
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    LibraryController *sc = dynamic_cast <LibraryController *> (cls.operator-> ());
    if (sc) {
      return sc;
    }
  }
  return 0;
}

//  The singleton instance of the library controller
static tl::RegisteredClass<lay::PluginDeclaration> library_controller_decl (new lay::LibraryController (), 150, "LibraryController");

}

