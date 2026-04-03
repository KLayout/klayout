
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

#include "layLibraryController.h"
#include "layTechnology.h"
#include "layApplication.h"
#include "laySaltController.h"
#include "layConfig.h"
#include "layMainWindow.h"
#include "layQtTools.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbFileBasedLibrary.h"
#include "tlLog.h"
#include "tlStream.h"
#include "tlFileUtils.h"
#include "tlEnv.h"

#include <QDir>

namespace lay
{

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

  //  scan for library definition files

  std::string lib_file = tl::get_env ("KLAYOUT_LIB");

  std::vector <std::pair<std::string, std::string> > lib_files;

  if (lib_file.empty ()) {
    for (std::vector <std::pair<std::string, std::string> >::const_iterator p = paths.begin (); p != paths.end (); ++p) {
      std::string lf = tl::combine_path (p->first, "klayout.lib");
      if (tl::is_readable (lf)) {
        lib_files.push_back (std::make_pair (lf, p->second));
      }
    }
  } else if (tl::is_readable (lib_file)) {
    lib_files.push_back (std::make_pair (lib_file, std::string ()));
  }

  for (auto lf = lib_files.begin (); lf != lib_files.end (); ++lf) {

    tl::log << "Reading lib file '" << lf->first << "'";

    try {

      std::vector<LibFileInfo> libs;
      read_lib_file (lf->first, lf->second, libs);

      read_libs (libs, new_lib_files);

      if (m_file_watcher) {
        m_file_watcher->add_file (tl::absolute_file_path (lf->first));
      }

    } catch (tl::Exception &ex) {
      tl::error << tl::to_string (tr ("Error reading lib file")) << " " << lf->first << ":" << tl::endl << ex.msg ();
    } catch (...) {
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
      QStringList entries = lp.entryList (name_filters, QDir::Files);

      std::vector<LibFileInfo> libs;
      libs.reserve (entries.size ());

      for (auto e = entries.begin (); e != entries.end (); ++e) {
        libs.push_back (LibFileInfo ());
        libs.back ().path = tl::to_string (lp.absoluteFilePath (*e));
        if (! p->second.empty ()) {
          libs.back ().tech.insert (p->second);
        }
      }

      read_libs (libs, new_lib_files);

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

    std::pair<bool, db::lib_id_type> li = db::LibraryManager::instance ().lib_by_name (lf->second.name, lf->second.tech);
    if (! li.first) {
      continue;  //  should not happen
    }

    db::Library *lib = db::LibraryManager::instance ().lib (li.second);

    if (new_names.find (lf->second.name) == new_names.end ()) {

      try {
        if (! lf->second.tech.empty ()) {
          tl::log << "Unregistering lib '" << lf->second.name << "' for technology '" << *lf->second.tech.begin () << "' as the file no longer exists: " << lf->first;
        } else {
          tl::log << "Unregistering lib '" << lf->second.name << "' as the file no longer exists: " << lf->first;
        }
        db::LibraryManager::instance ().delete_lib (lib);
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      } catch (...) {
      }

    }

  }

  //  establish the new libraries

  m_lib_files = new_lib_files;
}

namespace
{

struct LibFileFunctionContext
{
  std::string lib_file;
  std::string tech;
  std::vector<LibraryController::LibFileInfo> *lib_files;
};

static void do_read_lib_file (LibFileFunctionContext &fc);

class DefineFunction
  : public tl::EvalFunction
{
public:
  DefineFunction (LibFileFunctionContext *fc)
    : mp_fc (fc)
  { }

  virtual bool supports_keyword_parameters () const { return true; }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant & /*out*/, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> *kwargs) const
  {
    if (args.size () < 1 || args.size () > 2) {
      throw tl::EvalError (tl::to_string (tr ("'define' function needs one or two arguments (a path or a name and path)")), context);
    }

    std::string lf, name;
    if (args.size () == 1) {
      lf = args[0].to_string ();
    } else {
      name = args[0].to_string ();
      lf = args[1].to_string ();
    }

    LibraryController::LibFileInfo fi;
    fi.name = name;
    fi.path = tl::is_absolute (lf) ? lf : tl::combine_path (tl::absolute_path (mp_fc->lib_file), lf);
    if (! mp_fc->tech.empty ()) {
      fi.tech.insert (mp_fc->tech);
    }

    if (kwargs) {

      for (auto k = kwargs->begin (); k != kwargs->end (); ++k) {

        static const std::string replicate_key ("replicate");
        static const std::string technology_key ("technology");
        static const std::string technologies_key ("technologies");

        if (k->first == replicate_key) {

          fi.replicate = k->second.to_bool ();

        } else if (k->first == technology_key) {

          fi.tech.clear ();
          std::string tn = k->second.to_string ();
          if (! tn.empty () && tn != "*") {
            fi.tech.insert (tn);
          }

        } else if (k->first == technologies_key) {

          fi.tech.clear ();
          if (k->second.is_list ()) {
            for (auto t = k->second.begin (); t != k->second.end (); ++t) {
              fi.tech.insert (t->to_string ());
            }
          }

        } else {

          throw tl::EvalError (tl::sprintf (tl::to_string (tr ("Unknown keyword argument '%s' for 'define' function - the only allowed keyword arguments are 'replicate', 'technology' and 'technologies")), k->first), context);

        }

      }

    }

    mp_fc->lib_files->push_back (fi);

  }

private:
  LibFileFunctionContext *mp_fc;
};

class IncludeFunction
  : public tl::EvalFunction
{
public:
  IncludeFunction (LibFileFunctionContext *fc)
    : mp_fc (fc)
  { }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant & /*out*/, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () != 1) {
      throw tl::EvalError (tl::to_string (tr ("'include' function needs exactly one argument (the include file path)")), context);
    }

    std::string lf = args[0].to_string ();

    LibFileFunctionContext fc = *mp_fc;
    fc.lib_file = tl::is_absolute (lf) ? lf : tl::combine_path (tl::absolute_path (mp_fc->lib_file), lf);

    do_read_lib_file (fc);
  }

private:
  LibFileFunctionContext *mp_fc;
};

static void
do_read_lib_file (LibFileFunctionContext &fc)
{
  tl::Eval eval;

  eval.define_function ("define", new DefineFunction (&fc));
  eval.define_function ("include", new IncludeFunction (&fc));
  eval.set_var ("file", fc.lib_file);
  eval.set_var ("tech", fc.tech);

  tl::InputStream is (fc.lib_file);
  std::string lib_file = tl::TextInputStream (is).read_all ();

  tl::Extractor ex (lib_file.c_str ());
  eval.parse (ex).execute ();
}

}

void
LibraryController::read_lib_file (const std::string &lib_file, const std::string &tech, std::vector<LibraryController::LibFileInfo> &file_info)
{
  LibFileFunctionContext fc;
  fc.tech = tech;
  fc.lib_file = tl::absolute_file_path (lib_file);
  fc.lib_files = &file_info;

  do_read_lib_file (fc);
}

void
LibraryController::read_libs (const std::vector<LibraryController::LibFileInfo> &libs, std::map<std::string, LibraryController::LibInfo> &new_lib_files)
{
  bool needs_load = false;

  for (auto im = libs.begin (); im != libs.end () && ! needs_load; ++im) {

    const std::string &lib_path = im->path;
    QFileInfo fi (tl::to_qstring (lib_path));

    auto ll = m_lib_files.find (lib_path);
    if (ll == m_lib_files.end ()) {
      needs_load = true;
    } else if (fi.lastModified () > ll->second.time || im->tech != ll->second.tech || im->replicate != ll->second.replicate) {
      needs_load = true;
    }

  }

  if (! needs_load) {

    //  If not reloading, register the existing files as known ones - this allows detecting if
    //  a file got removed.
    for (auto im = libs.begin (); im != libs.end () && ! needs_load; ++im) {

      const std::string &lib_path = im->path;
      auto ll = m_lib_files.find (lib_path);
      if (ll != m_lib_files.end ()) {
        new_lib_files.insert (*ll);
      }

    }

  } else {

    std::map<std::string, db::FileBasedLibrary *> libs_by_name_here;

    //  Reload all files
    for (auto im = libs.begin (); im != libs.end (); ++im) {

      const std::string &lib_path = im->path;
      QFileInfo fi (tl::to_qstring (lib_path));

      try {

        std::unique_ptr<db::FileBasedLibrary> lib (new db::FileBasedLibrary (lib_path, im->name));
        lib->set_technologies (im->tech);
        lib->set_replicate (im->replicate);

        tl::log << "Reading library '" << lib_path << "'";
        std::string libname = lib->load ();

        //  merge with existing lib if there is already one in this folder with the right name
        auto il = libs_by_name_here.find (libname);
        if (il != libs_by_name_here.end ()) {

          tl::log << "Merging with other library file with the same name: " << libname;

          il->second->merge_with_other_layout (lib_path);

          //  now, we can forget the new library as it is included in the first one

        } else {

          //  otherwise register the new library

          if (! im->tech.empty ()) {
            tl::log << "Registering as '" << libname << "' for tech '" << tl::join (im->tech.begin (), im->tech.end (), ",") << "'";
          } else {
            tl::log << "Registering as '" << libname << "'";
          }

          LibInfo li;
          li.name = libname;
          li.time = fi.lastModified ();
          li.tech = im->tech;
          li.replicate = im->replicate;
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

