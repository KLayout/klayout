
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "layMacroController.h"
#include "layTechnologyController.h"
#include "laySaltController.h"
#include "layMacroEditorDialog.h"
#include "layMainWindow.h"
#include "layMainConfigPages.h"
#include "layConfig.h"
#include "layApplication.h"
#include "lymMacroInterpreter.h"
#include "lymMacro.h"
#include "gsiDecl.h"

#include <QDir>
#include <QUrl>
#include <QMessageBox>

namespace lay
{

MacroController::MacroController ()
  : mp_macro_editor (0), mp_mw (0), m_no_implicit_macros (false), m_file_watcher (0),
    dm_do_update_menu_with_macros (this, &MacroController::do_update_menu_with_macros),
    dm_do_sync_with_external_sources (this, &MacroController::do_sync_with_external_sources),
    dm_sync_file_watcher (this, &MacroController::sync_file_watcher),
    dm_sync_files (this, &MacroController::sync_files)
{
  //  .. nothing yet ..
}

void
MacroController::add_macro_category (const std::string &name, const std::string &description, const std::vector<std::string> &folders)
{
  lay::MacroController::MacroCategory cat;
  cat.name = name;
  cat.description = description;
  cat.folders = folders;

  //  replace an existing category or add to back
  for (auto c = m_macro_categories.begin (); c != m_macro_categories.end (); ++c) {
    if (c->name == name) {
      *c = cat;
      return;
    }
  }

  m_macro_categories.push_back (cat);
}

void
MacroController::finish ()
{
  lym::MacroCollection &root = lym::MacroCollection::root ();

  root.clear ();

  //  Scan built-in macros
  //  These macros are always taken, even if there are no macros requested (they are required to
  //  fully form the API).
  root.add_folder (tl::to_string (QObject::tr ("Built-In")), ":/built-in-macros", "macros", true);
  root.add_folder (tl::to_string (QObject::tr ("Built-In")), ":/built-in-pymacros", "pymacros", true);

  //  scans the macros from techs and packages (this will allow autorun-early on them)
  //  and updates m_external_paths
  sync_macro_sources ();

  //  Scan for macros and set interpreter path
  for (std::vector <InternalPathDescriptor>::const_iterator p = m_internal_paths.begin (); p != m_internal_paths.end (); ++p) {

    if (! m_no_implicit_macros) {

      for (size_t c = 0; c < m_macro_categories.size (); ++c) {

        if (p->cat.empty ()) {

          for (std::vector<std::string>::const_iterator f = m_macro_categories[c].folders.begin (); f != m_macro_categories[c].folders.end (); ++f) {

            std::string mp = tl::to_string (QDir (tl::to_qstring (p->path)).absoluteFilePath (tl::to_qstring (*f)));

            std::string description = p->description;
            if (*f != m_macro_categories[c].name) {
              description += " - " + tl::to_string (tr ("%1 branch").arg (tl::to_qstring (*f)));
            }

            root.add_folder (description, mp, m_macro_categories[c].name, p->readonly);

          }

        } else if (p->cat == m_macro_categories[c].name) {

          root.add_folder (p->description, p->path, m_macro_categories[c].name, p->readonly);

        }

      }

    }

    //  Add the unspecific paths as "package locations", so we get "ruby", "python" and similar folders as
    //  path components inside the interpreters.
    if (p->cat.empty ()) {
      for (tl::Registrar<gsi::Interpreter>::iterator i = gsi::interpreters.begin (); i != gsi::interpreters.end (); ++i) {
        i->add_package_location (p->path);
      }
    }

  }


  //  Scan for macros in packages and techs

  if (! m_no_implicit_macros) {
    for (std::vector <ExternalPathDescriptor>::const_iterator p = m_external_paths.begin (); p != m_external_paths.end (); ++p) {
      lym::MacroCollection *mc = root.add_folder (p->description, p->path, p->cat, p->readonly);
      if (mc) {
        mc->set_virtual_mode (p->type);
      }
    }
  }

  //  Set the interpreter path to packages too

  sync_package_paths ();
}

void
MacroController::initialized (lay::Dispatcher *root)
{
  connect (&m_temp_macros, SIGNAL (menu_needs_update ()), this, SLOT (macro_collection_changed ()));
  connect (&m_temp_macros, SIGNAL (macro_collection_changed (lym::MacroCollection *)), this, SLOT (macro_collection_changed ()));

  mp_mw = lay::MainWindow::instance ();
  if (mp_mw) {
    mp_macro_editor = new lay::MacroEditorDialog (root, &lym::MacroCollection::root ());
    mp_macro_editor->setModal (false);
  }

  if (! m_file_watcher) {
    m_file_watcher = new tl::FileSystemWatcher (this);
    connect (m_file_watcher, SIGNAL (fileChanged (const QString &)), this, SLOT (file_watcher_triggered ()));
    connect (m_file_watcher, SIGNAL (fileRemoved (const QString &)), this, SLOT (file_watcher_triggered ()));
  }

  connect (&lym::MacroCollection::root (), SIGNAL (menu_needs_update ()), this, SLOT (macro_collection_changed ()));
  connect (&lym::MacroCollection::root (), SIGNAL (macro_collection_changed (lym::MacroCollection *)), this, SLOT (macro_collection_changed ()));
  if (lay::TechnologyController::instance ()) {
    connect (lay::TechnologyController::instance (), SIGNAL (active_technology_changed ()), this, SLOT (macro_collection_changed ()));
    connect (lay::TechnologyController::instance (), SIGNAL (technologies_edited ()), this, SLOT (sync_with_external_sources ()));
  }
  if (lay::SaltController::instance ()) {
    connect (lay::SaltController::instance (), SIGNAL (salt_changed ()), this, SLOT (sync_with_external_sources ()));
  }

  //  synchronize the macro collection with all external sources
  sync_implicit_macros (false);

  //  update the menus with the macro menu bindings as late as possible (now we
  //  can be sure that the menus are created propertly)
  macro_collection_changed ();
}

void
MacroController::uninitialize (lay::Dispatcher * /*root*/)
{
  disconnect (&lym::MacroCollection::root (), SIGNAL (menu_needs_update ()), this, SLOT (macro_collection_changed ()));
  disconnect (&lym::MacroCollection::root (), SIGNAL (macro_collection_changed (lym::MacroCollection *)), this, SLOT (macro_collection_changed ()));
  if (lay::TechnologyController::instance ()) {
    disconnect (lay::TechnologyController::instance (), SIGNAL (active_technology_changed ()), this, SLOT (macro_collection_changed ()));
    disconnect (lay::TechnologyController::instance (), SIGNAL (technologies_edited ()), this, SLOT (sync_with_external_sources ()));
  }
  if (lay::SaltController::instance ()) {
    disconnect (lay::SaltController::instance (), SIGNAL (salt_changed ()), this, SLOT (sync_with_external_sources ()));
  }

  if (m_file_watcher) {
    disconnect (m_file_watcher, SIGNAL (fileChanged (const QString &)), this, SLOT (file_watcher_triggered ()));
    disconnect (m_file_watcher, SIGNAL (fileRemoved (const QString &)), this, SLOT (file_watcher_triggered ()));
    delete m_file_watcher;
    m_file_watcher = 0;
  }

  delete mp_macro_editor;
  mp_macro_editor = 0;
  mp_mw = 0;
}

bool
MacroController::configure (const std::string &key, const std::string &value)
{
  if (key == cfg_key_bindings) {
    m_key_bindings = unpack_key_binding (value);
  } else if (key == cfg_menu_items_hidden) {
    m_menu_items_hidden = unpack_menu_items_hidden (value);
  }
  return false;
}

void
MacroController::config_finalize()
{
  //  .. nothing yet ..
}

bool
MacroController::can_exit (lay::Dispatcher * /*root*/) const
{
  if (mp_macro_editor) {
    return mp_macro_editor->can_exit ();
  } else {
    return true;
  }
}

bool
MacroController::accepts_drop (const std::string &path_or_url) const
{
  QUrl url (tl::to_qstring (path_or_url));
  QFileInfo file_info (url.path ());
  QString suffix = file_info.suffix ().toLower ();

  if (suffix == QString::fromUtf8 ("rb") ||
      suffix == QString::fromUtf8 ("py") ||
      suffix == QString::fromUtf8 ("lym")) {
    return true;
  }

  //  check the suffixes in the DSL interpreter declarations
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (suffix == tl::to_qstring (cls->suffix ())) {
      return true;
    }
  }

  return false;
}

void
MacroController::drop_url (const std::string &path_or_url)
{
  //  Normalize the URL to become either a normal path or a URL
  std::string path = path_or_url;

  QUrl url (tl::to_qstring (path_or_url));
  QString file_name = QFileInfo (url.path ()).fileName ();

  if (url.scheme () == QString::fromUtf8 ("file")) {
    path = tl::to_string (url.toLocalFile ());
  }

  //  load and run macro
  std::unique_ptr<lym::Macro> macro (new lym::Macro ());
  macro->load_from (path);
  macro->set_file_path (path);

  if ((macro->is_autorun () || macro->show_in_menu ()) && ! lay::ApplicationBase::instance ()->appdata_path ().empty ()) {

    //  install macro permanently
    if (QMessageBox::question (mp_mw,
                               QObject::tr ("Install Macro"),
                               QObject::tr ("Install macro '%1' permanently?\n\nPress 'Yes' to install the macro in the application settings folder permanently.").arg (file_name),
                               QMessageBox::Yes | QMessageBox::No,
                               QMessageBox::No) == QMessageBox::Yes) {

      //  Use the application data folder
      QDir folder (tl::to_qstring (lay::ApplicationBase::instance ()->appdata_path ()));

      std::string cat = "macros";
      if (! macro->category ().empty ()) {
        cat = macro->category ();
      }

      if (! folder.cd (tl::to_qstring (cat))) {
        throw tl::Exception (tl::to_string (QObject::tr ("Folder '%s' does not exists in installation path '%s' - cannot install")).c_str (), cat, lay::ApplicationBase::instance ()->appdata_path ());
      }

      QFileInfo target (folder, file_name);

      if (! target.exists () || QMessageBox::question (mp_mw,
                                                       QObject::tr ("Overwrite Macro"),
                                                       QObject::tr ("Overwrite existing macro?"),
                                                       QMessageBox::Yes | QMessageBox::No,
                                                       QMessageBox::No) == QMessageBox::Yes) {

        QFile target_file (target.filePath ());
        if (target.exists () && ! target_file.remove ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("Unable to remove file '%1'").arg (target.filePath ())));
        }

        macro->set_file_path (tl::to_string (target.filePath ()));

        //  run the macro now - if it fails, it is not installed, but the file path is already set to
        //  the target path.
        if (macro->is_autorun ()) {
          macro->run ();
        }

        macro->save ();

      }

    } else {

      if (macro->is_autorun ()) {
        //  If it is not installed, run it now ..
        macro->run ();
      } else if (macro->show_in_menu ()) {
        //  .. or add as temporary macro so it is shown in the menu.
        add_temp_macro (macro.release ());
      }

    }

  } else {
    macro->run ();
  }
}

void
MacroController::show_editor (const std::string &cat, bool force_add)
{
  if (macro_categories ().empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Application has not been compiled with scripting support - no macro IDE available")));
  }

  if (mp_macro_editor) {
    mp_macro_editor->show (cat, force_add);
  }
}

void
MacroController::enable_implicit_macros (bool enable)
{
  m_no_implicit_macros = !enable;
}

void
MacroController::sync_package_paths ()
{
  std::vector<std::string> package_locations;

  lay::SaltController *sc = lay::SaltController::instance ();
  if (sc) {
    lay::Salt &salt = sc->salt ();
    for (lay::Salt::flat_iterator i = salt.begin_flat (); i != salt.end_flat (); ++i) {
      package_locations.push_back ((*i)->path ());
    }
  }

  //  refresh the package locations by first removing the package locations and then rebuilding
  //  TODO: maybe that is a performance bottleneck, but right now, remove_package_location doesn't do a lot.

  for (std::vector<std::string>::const_iterator p = m_package_locations.begin (); p != m_package_locations.end (); ++p) {
    for (tl::Registrar<gsi::Interpreter>::iterator i = gsi::interpreters.begin (); i != gsi::interpreters.end (); ++i) {
      i->remove_package_location (*p);
    }
  }
  
  m_package_locations = package_locations;
  
  for (std::vector<std::string>::const_iterator p = m_package_locations.begin (); p != m_package_locations.end (); ++p) {
    for (tl::Registrar<gsi::Interpreter>::iterator i = gsi::interpreters.begin (); i != gsi::interpreters.end (); ++i) {
      i->add_package_location (*p);
    }
  }
}

void
MacroController::sync_implicit_macros (bool ask_before_autorun)
{
  //  gets the external paths (tech, packages) into m_external_paths
  sync_macro_sources ();

  if (m_no_implicit_macros) {

    sync_package_paths ();

  } else {

    //  determine the paths currently in use
    std::map<std::string, ExternalPathDescriptor> prev_folders_by_path;
    for (std::vector<ExternalPathDescriptor>::const_iterator p = m_external_paths.begin (); p != m_external_paths.end (); ++p) {
      prev_folders_by_path.insert (std::make_pair (p->path, *p));
    }

    //  delete macro collections which are no longer required or update description

    std::vector<lym::MacroCollection *> folders_to_delete;

    //  determine the paths that will be in use
    std::map<std::string, const ExternalPathDescriptor *> new_folders_by_path;
    for (std::vector<ExternalPathDescriptor>::const_iterator p = m_external_paths.begin (); p != m_external_paths.end (); ++p) {
      new_folders_by_path.insert (std::make_pair (p->path, p.operator-> ()));
    }

    lym::MacroCollection *root = &lym::MacroCollection::root ();

    for (lym::MacroCollection::child_iterator m = root->begin_children (); m != root->end_children (); ++m) {
      if (m->second->virtual_mode () == lym::MacroCollection::TechFolder ||
          m->second->virtual_mode () == lym::MacroCollection::SaltFolder) {
        std::map<std::string, const ExternalPathDescriptor *>::const_iterator u = new_folders_by_path.find (m->second->path ());
        if (u == new_folders_by_path.end ()) {
          //  no longer used
          folders_to_delete.push_back (m->second);
        } else {
          m->second->set_description (u->second->description);
        }
      }
    }

    for (std::vector<lym::MacroCollection *>::iterator m = folders_to_delete.begin (); m != folders_to_delete.end (); ++m) {
      if (tl::verbosity () >= 20) {
        tl::info << "Removing macro folder " << (*m)->path () << ", category '" << (*m)->category () << "' because no longer in use";
      }
      root->erase (*m);
    }

    //  sync the search paths with the packages
    sync_package_paths ();

    //  add new folders
    std::vector<lym::MacroCollection *> new_folders;

    for (std::vector<ExternalPathDescriptor>::const_iterator p = m_external_paths.begin (); p != m_external_paths.end (); ++p) {

      auto pf = prev_folders_by_path.find (p->path);
      if (pf != prev_folders_by_path.end ()) {

        if (pf->second.version != p->version) {

          if (tl::verbosity () >= 20) {
            tl::info << "New version (" << p->version << " vs. " << pf->second.version << ") of macro folder " << p->path << ", category '" << p->cat << "' for '" << p->description << "'";
          }

          lym::MacroCollection *mc = lym::MacroCollection::root ().folder_by_name (p->path);
          if (mc) {
            new_folders.push_back (mc);
          }

        }

      } else {

        if (tl::verbosity () >= 20) {
          tl::info << "Adding macro folder " << p->path << ", category '" << p->cat << "' for '" << p->description << "'";
        }

        //  Add the folder. Note: it may happen that a macro folder for the tech specific macros already exists in
        //  a non-tech context.
        //  In that case, the add_folder method will return 0.

        //  TODO: is it wise to make this writeable?
        lym::MacroCollection *mc = lym::MacroCollection::root ().add_folder (p->description, p->path, p->cat, p->readonly);
        if (mc) {
          mc->set_virtual_mode (p->type);
          new_folders.push_back (mc);
        }

      }

    }

    {
      //  This prevents the message dialog below to issue deferred methods
      tl::NoDeferredMethods silent;

      bool has_autorun = false;
      for (std::vector<lym::MacroCollection *>::const_iterator m = new_folders.begin (); m != new_folders.end () && ! has_autorun; ++m) {
        has_autorun = (*m)->has_autorun ();
      }

      if (has_autorun) {
        if (! ask_before_autorun || QMessageBox::question (mp_mw, QObject::tr ("Run Macros"), QObject::tr ("Some macros associated with new items are configured to run automatically.\n\nChoose 'Yes' to run these macros now. Choose 'No' to not run them."), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
          for (std::vector<lym::MacroCollection *>::const_iterator m = new_folders.begin (); m != new_folders.end (); ++m) {
            (*m)->autorun ();
          }
        }
      }
    }

  }
}

void
MacroController::sync_macro_sources ()
{
  std::vector<ExternalPathDescriptor> external_paths;

  //  Add additional places where the technologies define some macros

  std::map<std::string, std::vector<std::string> > tech_names_by_path;
  std::map<std::string, std::vector<std::string> > grain_names_by_path;
  std::set<std::string> readonly_paths;

  for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
    if (! t->base_path ().empty ()) {
      QDir base_dir (tl::to_qstring (t->base_path ()));
      if (base_dir.exists ()) {
        std::string path = tl::to_string (base_dir.absolutePath ());
        tech_names_by_path [path].push_back (t->name ());
        if (t->is_readonly ()) {
          readonly_paths.insert (path);
        }
        if (! t->grain_name ().empty ()) {
          grain_names_by_path [path].push_back (t->grain_name ());
        }
      }
    }
  }

  for (std::map<std::string, std::vector<std::string> >::const_iterator t = tech_names_by_path.begin (); t != tech_names_by_path.end (); ++t) {

    for (size_t c = 0; c < macro_categories ().size (); ++c) {

      for (std::vector<std::string>::const_iterator f = macro_categories () [c].folders.begin (); f != macro_categories () [c].folders.end (); ++f) {

        QDir base_dir (tl::to_qstring (t->first));
        QDir macro_dir (base_dir.filePath (tl::to_qstring (*f)));
        if (macro_dir.exists ()) {

          std::string description;
          if (t->second.size () == 1) {
            description = tl::to_string (tr ("Technology %1").arg (tl::to_qstring (t->second.front ())));
          } else {
            description = tl::to_string (tr ("Technologies %1").arg (tl::to_qstring (tl::join (t->second, ","))));
          }

          std::map<std::string, std::vector<std::string> >::const_iterator gn = grain_names_by_path.find (t->first);
          if (gn != grain_names_by_path.end ()) {
            description += " - ";
            if (gn->second.size () == 1) {
              description += tl::to_string (tr ("Package %1").arg (tl::to_qstring (gn->second.front ())));
            } else {
              description += tl::to_string (tr ("Packages %1").arg (tl::to_qstring (tl::join (gn->second, ","))));
            }
          }

          if (*f != macro_categories () [c].name) {
            description += " - " + tl::to_string (tr ("%1 branch").arg (tl::to_qstring (*f)));
          }

          external_paths.push_back (ExternalPathDescriptor (tl::to_string (macro_dir.path ()), description, macro_categories () [c].name, lym::MacroCollection::TechFolder, readonly_paths.find (t->first) != readonly_paths.end ()));

        }

      }

    }

  }

  //  Add additional places where the salt defines macros

  lay::SaltController *sc = lay::SaltController::instance ();
  if (sc) {

    lay::Salt &salt = sc->salt ();
    for (lay::Salt::flat_iterator i = salt.begin_flat (); i != salt.end_flat (); ++i) {

      const lay::SaltGrain *g = *i;

      for (size_t c = 0; c < macro_categories ().size (); ++c) {

        for (std::vector<std::string>::const_iterator f = macro_categories () [c].folders.begin (); f != macro_categories () [c].folders.end (); ++f) {

          QDir base_dir (tl::to_qstring (g->path ()));
          QDir macro_dir (base_dir.filePath (tl::to_qstring (*f)));
          if (macro_dir.exists ()) {

            std::string description = tl::to_string (tr ("Package %1").arg (tl::to_qstring (g->name ())));
            if (*f != macro_categories () [c].name) {
              description += " - " + tl::to_string (tr ("%1 branch").arg (tl::to_qstring (*f)));
            }
            external_paths.push_back (ExternalPathDescriptor (tl::to_string (macro_dir.path ()), description, macro_categories () [c].name, lym::MacroCollection::SaltFolder, g->is_readonly (), g->version ()));

          }

        }

      }

    }

  }

  //  store new paths
  m_external_paths = external_paths;
}

void
MacroController::add_path (const std::string &path, const std::string &description, const std::string &category, bool readonly)
{
  m_internal_paths.push_back (InternalPathDescriptor (path, description, category, readonly));
}

void
MacroController::add_temp_macro (lym::Macro *m)
{
  m_temp_macros.add_unspecific (m);
}

static std::string menu_name (std::set<std::string> &used_names, const std::string &org_name)
{
  std::string name;

  if (org_name.empty ()) {

    for (int i = 1; true; ++i) {
      name = "macro_in_menu_" + tl::to_string (i);
      if (used_names.find (name) == used_names.end ()) {
        break;
      }
    }

  } else {

    //  replace special characters with "_" (specifically ".")
    std::string good_name = "macro_in_menu_";
    for (const char *cp = org_name.c_str (); *cp; ++cp) {
      if (isalnum (*cp) || *cp == '_') {
        good_name += *cp;
      } else {
        good_name += "_";
      }
    }

    if (used_names.find (good_name) == used_names.end ()) {
      name = good_name;
    } else {
      for (int i = 1; true; ++i) {
        name = good_name + "_" + tl::to_string (i);
        if (used_names.find (name) == used_names.end ()) {
          break;
        }
      }
    }

  }

  used_names.insert (name);
  return name;
}

namespace {

class RunMacroAction
  : public lay::Action
{
public:
  RunMacroAction (lym::Macro *lym)
    : lay::Action (), mp_lym (lym)
  {
    if (lym->description ().empty ()) {
      set_title (lym->path ());
    } else {
      set_title (lym->description ());
    }
  }

  void triggered ()
  {
    if (mp_lym.get ()) {
      mp_lym->run ();
    }
  }

  lym::Macro *macro () const
  {
    return const_cast<lym::Macro *> (mp_lym.get ());
  }

private:
  tl::weak_ptr<lym::Macro> mp_lym;
};

}

void
MacroController::add_macro_items_to_menu (lym::MacroCollection &collection, std::set<std::string> &used_names, std::set<std::string> &groups, const db::Technology *tech)
{
  for (lym::MacroCollection::child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {

    //  check whether the macro collection is associated with the selected technology (if there is one)
    bool consider = false;
    if (! tech || c->second->virtual_mode () != lym::MacroCollection::TechFolder) {
      consider = true;
    } else {
      const std::vector<lay::MacroController::MacroCategory> &mc = macro_categories ();
      for (std::vector<lay::MacroController::MacroCategory>::const_iterator cc = mc.begin (); cc != mc.end () && !consider; ++cc) {
        consider = (c->second->path () == tl::to_string (QDir (tl::to_qstring (tech->base_path ())).filePath (tl::to_qstring (cc->name))));
      }
    }

    if (consider) {
      add_macro_items_to_menu (*c->second, used_names, groups, 0 /*don't check 2nd level and below*/);
    }

  }

  for (lym::MacroCollection::iterator c = collection.begin (); c != collection.end (); ++c) {

    std::string sc = tl::trim (c->second->shortcut ());

    if (c->second->show_in_menu ()) {

      std::string mp = tl::trim (c->second->menu_path ());
      if (mp.empty ()) {
        mp = "macros_menu.end";
      }

      std::string gn = tl::trim (c->second->group_name ());
      if (! gn.empty () && groups.find (gn) == groups.end ()) {
        groups.insert (gn);
        lay::Action *as = new lay::Action ();
        as->set_separator (true);
        m_macro_actions.push_back (as);
        mp_mw->menu ()->insert_item (mp, menu_name (used_names, std::string ()), as);
      }

      lay::Action *a = new RunMacroAction (c->second);
      a->set_default_shortcut (sc);
      m_macro_actions.push_back (a);
      mp_mw->menu ()->insert_item (mp, menu_name (used_names, c->second->name ()), a);

    } else if (! sc.empty ()) {

      //  Create actions for shortcut-only actions too and add them to the main window
      //  to register their shortcut.

      lay::Action *a = new RunMacroAction (c->second);
      a->set_shortcut (sc);
      m_macro_actions.push_back (a);
      mp_mw->addAction (a->qaction ());

    }

  }
}

void
MacroController::sync_with_external_sources ()
{
  dm_do_sync_with_external_sources ();
}

void
MacroController::do_sync_with_external_sources ()
{
  try {
    sync_implicit_macros (true);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
  }
}

void
MacroController::macro_collection_changed ()
{
  //  empty action to macro table now we know it's invalid
  dm_do_update_menu_with_macros ();
  dm_sync_file_watcher ();
}

void
MacroController::do_update_menu_with_macros ()
{
  if (!mp_mw) {
    return;
  }

  const db::Technology *tech = 0;
  if (lay::TechnologyController::instance ()) {
    tech = lay::TechnologyController::instance ()->active_technology ();
  }

  //  delete all existing items
  std::vector<lay::Action *> actions;
  for (tl::weak_collection<lay::Action>::iterator a = m_macro_actions.begin (); a != m_macro_actions.end (); ++a) {
    if (a.operator-> ()) {
      actions.push_back (a.operator-> ());
    }
  }
  for (std::vector<lay::Action *>::const_iterator a = actions.begin (); a != actions.end (); ++a) {
    mp_mw->menu ()->delete_items (*a);
  }
  m_macro_actions.clear ();

  std::set<std::string> groups;
  std::set<std::string> used_names;
  add_macro_items_to_menu (m_temp_macros, used_names, groups, tech);
  add_macro_items_to_menu (lym::MacroCollection::root (), used_names, groups, tech);

  //  apply the custom keyboard shortcuts
  for (std::vector<std::pair<std::string, std::string> >::const_iterator kb = m_key_bindings.begin (); kb != m_key_bindings.end (); ++kb) {
    if (mp_mw->menu ()->is_valid (kb->first)) {
      lay::Action *a = mp_mw->menu ()->action (kb->first);
      a->set_shortcut (kb->second);
    }
  }

  //  apply the custom hidden flags
  for (std::vector<std::pair<std::string, bool> >::const_iterator hf = m_menu_items_hidden.begin (); hf != m_menu_items_hidden.end (); ++hf) {
    if (mp_mw->menu ()->is_valid (hf->first)) {
      lay::Action *a = mp_mw->menu ()->action (hf->first);
      a->set_hidden (hf->second);
    }
  }
}

void
MacroController::file_watcher_triggered ()
{
  dm_sync_files ();
}

static void
add_collections_to_file_watcher (const lym::MacroCollection &collection, tl::FileSystemWatcher *watcher)
{
  for (lym::MacroCollection::const_child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {
    if (! c->second->path ().empty () && c->second->path ()[0] != ':') {
      watcher->add_file (c->second->path ());
      add_collections_to_file_watcher (*c->second, watcher);
    }
  }
}

void
MacroController::sync_file_watcher ()
{
  if (m_file_watcher) {
    m_file_watcher->clear ();
    m_file_watcher->enable (false);
    add_collections_to_file_watcher (lym::MacroCollection::root (), m_file_watcher);
    m_file_watcher->enable (true);
  }
}

void
MacroController::sync_files ()
{
  tl::log << tl::to_string (tr ("Detected file system change in macro folders - updating"));
  lym::MacroCollection::root ().reload (true /*safe*/);
}

MacroController *
MacroController::instance ()
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    MacroController *mc = dynamic_cast <MacroController *> (cls.operator-> ());
    if (mc) {
      return mc;
    }
  }
  return 0;
}

//  The singleton instance of the macro controller
static tl::RegisteredClass<lay::PluginDeclaration> macro_controller_decl (new lay::MacroController (), 120, "MacroController");

static lym::Macro *macro_for_action (const lay::Action *action)
{
  const RunMacroAction *rma = dynamic_cast<const RunMacroAction *> (action);
  return rma ? rma->macro () : 0;
}

//  extend lay::Action with the ability to associate a macro with it
static
gsi::ClassExt<lay::Action> decl_ext_action (
  gsi::method_ext ("macro", &macro_for_action,
    "@brief Gets the macro associated with the action\n"
    "If the action is associated with a macro, this method returns a reference to the \\Macro object. "
    "Otherwise, this method returns nil.\n"
    "\n"
    "\nThis method has been added in version 0.25.\n"
  )
);

}

