
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
#include "dbCellMapping.h"
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

  void merge_with_other_layout (const std::string &path)
  {
    m_other_paths.push_back (path);
    merge_impl (path);
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

    for (auto p = m_other_paths.begin (); p != m_other_paths.end (); ++p) {
      merge_impl (*p);
    }

    return name;
  }

private:
  std::string m_path;
  std::list<std::string> m_other_paths;

  void merge_impl (const std::string &path)
  {
    db::Layout ly;

    tl::InputStream stream (path);
    db::Reader reader (stream);
    reader.read (ly);

    std::vector<db::cell_index_type> target_cells, source_cells;

    //  collect the cells to pull in (all top cells of the library layout)
    //  NOTE: cells are not overwritten - the first layout wins, in terms
    //  of cell names and also in terms of database unit.
    for (auto c = ly.begin_top_down (); c != ly.end_top_cells (); ++c) {
      std::string cn = ly.cell_name (*c);
      if (! layout ().has_cell (cn.c_str ())) {
        source_cells.push_back (*c);
        target_cells.push_back (layout ().add_cell (cn.c_str ()));
      }
    }

    db::CellMapping cm;
    cm.create_multi_mapping_full (layout (), target_cells, ly, source_cells);
    layout ().copy_tree_shapes (ly, cm);
  }
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

      tl::log << "Scanning library path '" << tl::to_string (lp.absolutePath ()) << "'";

      QStringList name_filters;
      name_filters << QString::fromUtf8 ("*");

      //  NOTE: this should return a list sorted by name
      QStringList libs = lp.entryList (name_filters, QDir::Files);

      bool needs_load = false;

      for (QStringList::const_iterator im = libs.begin (); im != libs.end () && ! needs_load; ++im) {

        std::string lib_path = tl::to_string (lp.absoluteFilePath (*im));
        QFileInfo fi (tl::to_qstring (lib_path));

        auto ll = m_lib_files.find (lib_path);
        if (ll == m_lib_files.end ()) {
          needs_load = true;
        } else if (fi.lastModified () > ll->second.time) {
          needs_load = true;
        }

      }

      if (! needs_load) {

        //  If not reloading, register the existing files as known ones - this allows detecting if
        //  a file got removed.
        for (QStringList::const_iterator im = libs.begin (); im != libs.end () && ! needs_load; ++im) {

          std::string lib_path = tl::to_string (lp.absoluteFilePath (*im));
          auto ll = m_lib_files.find (lib_path);
          if (ll != m_lib_files.end ()) {
            new_lib_files.insert (*ll);
          }

        }

      } else {

        std::map<std::string, FileBasedLibrary *> libs_by_name_here;

        //  Reload all files
        for (QStringList::const_iterator im = libs.begin (); im != libs.end (); ++im) {

          std::string lib_path = tl::to_string (lp.absoluteFilePath (*im));
          QFileInfo fi (tl::to_qstring (lib_path));

          try {

            std::unique_ptr<FileBasedLibrary> lib (new FileBasedLibrary (lib_path));
            if (! p->second.empty ()) {
              lib->set_technology (p->second);
            }

            tl::log << "Reading library '" << lib_path << "'";
            std::string libname = lib->reload ();

            //  merge with existing lib if there is already one in this folder with the right name
            auto il = libs_by_name_here.find (libname);
            if (il != libs_by_name_here.end ()) {

              tl::log << "Merging with other library file with the same name: " << libname;

              il->second->merge_with_other_layout (lib_path);

              //  now, we can forget the new library as it is included in the first one

            } else {

              //  otherwise register the new library

              if (! p->second.empty ()) {
                tl::log << "Registering as '" << libname << "' for tech '" << p->second << "'";
              } else {
                tl::log << "Registering as '" << libname << "'";
              }

              LibInfo li;
              li.name = libname;
              li.time = fi.lastModified ();
              if (! p->second.empty ()) {
                li.tech.insert (p->second);
              }
              new_lib_files.insert (std::make_pair (lib_path, li));

              lib->set_name (libname);
              libs_by_name_here.insert (std::make_pair (libname, lib.release ()));

            }

          } catch (tl::Exception &ex) {
            tl::error << ex.msg ();
          }

        }

        //  Register the libs (NOTE: this needs to happen after the merge)
        for (auto l = libs_by_name_here.begin (); l != libs_by_name_here.end (); ++l) {
          try {
            db::LibraryManager::instance ().register_lib (l->second);
          } catch (tl::Exception &ex) {
            tl::error << ex.msg ();
          } catch (...) {
          }
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
          if (! lf->second.tech.empty ()) {
            tl::log << "Unregistering lib '" << lf->second.name << "' for technology '" << *lf->second.tech.begin () << "' as the file no longer exists: " << lf->first;
          } else {
            tl::log << "Unregistering lib '" << lf->second.name << "' as the file no longer exists: " << lf->first;
          }
          db::LibraryManager::instance ().delete_lib (db::LibraryManager::instance ().lib (li.second));
        }
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
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

