
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

#include "layMacroController.h"
#include "layMacroEditorDialog.h"
#include "layMacroInterpreter.h"
#include "layMainWindow.h"
#include "layMainConfigPages.h"
#include "layConfig.h"
#include "layApplication.h"

#include <QDir>
#include <QUrl>
#include <QMessageBox>

namespace lay
{

MacroController::MacroController ()
  : mp_macro_editor (0), mp_mw (0),
    dm_do_update_menu_with_macros (this, &MacroController::do_update_menu_with_macros)
{
  connect (&m_temp_macros, SIGNAL (menu_needs_update ()), this, SLOT (update_menu_with_macros ()));
  connect (&m_temp_macros, SIGNAL (macro_collection_changed (MacroCollection *)), this, SLOT (update_menu_with_macros ()));
}

void
MacroController::initialized (lay::PluginRoot *root)
{
  mp_mw = dynamic_cast <lay::MainWindow *> (root);
  if (mp_mw) {
    mp_macro_editor = new lay::MacroEditorDialog (mp_mw, &lay::MacroCollection::root ());
    mp_macro_editor->setModal (false);
  }

  connect (&lay::MacroCollection::root (), SIGNAL (menu_needs_update ()), this, SLOT (update_menu_with_macros ()));
  connect (&lay::MacroCollection::root (), SIGNAL (macro_collection_changed (MacroCollection *)), this, SLOT (update_menu_with_macros ()));

  //  update the menus with the macro menu bindings as late as possible (now we
  //  can be sure that the menus are created propertly)
  do_update_menu_with_macros ();
}

void
MacroController::uninitialize (lay::PluginRoot * /*root*/)
{
  disconnect (&lay::MacroCollection::root (), SIGNAL (menu_needs_update ()), this, SLOT (update_menu_with_macros ()));
  disconnect (&lay::MacroCollection::root (), SIGNAL (macro_collection_changed (MacroCollection *)), this, SLOT (update_menu_with_macros ()));

  delete mp_macro_editor;
  mp_macro_editor = 0;
  mp_mw = 0;
}

bool
MacroController::configure (const std::string &key, const std::string &value)
{
  if (key == cfg_key_bindings && mp_mw) {

    //  Update the shortcuts of the menu item if they have been edited in the configuration editor
    std::vector<std::pair<std::string, std::string> > key_bindings = unpack_key_binding (value);
    for (std::vector<std::pair<std::string, std::string> >::const_iterator kb = key_bindings.begin (); kb != key_bindings.end (); ++kb) {
      if (mp_mw->menu ()->is_valid (kb->first)) {
        lay::Action a = mp_mw->menu ()->action (kb->first);
        if (m_action_to_macro.find (a.qaction ()) != m_action_to_macro.end ()) {
          m_action_to_macro [a.qaction ()]->set_shortcut (kb->second);
        }
      }
    }

  }

  return false;
}

void
MacroController::config_finalize()
{
  //  .. nothing yet ..
}

bool
MacroController::can_exit (lay::PluginRoot * /*root*/) const
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
  for (tl::Registrar<lay::MacroInterpreter>::iterator cls = tl::Registrar<lay::MacroInterpreter>::begin (); cls != tl::Registrar<lay::MacroInterpreter>::end (); ++cls) {
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
  std::auto_ptr<lay::Macro> macro (new lay::Macro ());
  macro->load_from (path);
  macro->set_file_path (path);

  if (macro->is_autorun () || macro->show_in_menu ()) {

    //  install macro permanently
    if (QMessageBox::question (mp_mw,
                               QObject::tr ("Install Macro"),
                               QObject::tr ("Install macro '%1' permanently?\n\nPress 'Yes' to install the macro in the application settings folder permanently.").arg (file_name),
                               QMessageBox::Yes | QMessageBox::No,
                               QMessageBox::No) == QMessageBox::Yes) {

      //  Use the application data folder
      QDir folder (tl::to_qstring (lay::Application::instance ()->appdata_path ()));

      std::string cat = "macros";
      if (! macro->category ().empty ()) {
        cat = macro->category ();
      }

      if (! folder.cd (tl::to_qstring (cat))) {
        throw tl::Exception (tl::to_string (QObject::tr ("Folder '%s' does not exists in installation path '%s' - cannot install")).c_str (), cat, lay::Application::instance ()->appdata_path ());
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

        //  refresh macro editor to show new macro plus to install the menus
        refresh ();

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
  if (mp_macro_editor) {
    mp_macro_editor->show (cat, force_add);
  }
}

void
MacroController::refresh ()
{
  if (mp_macro_editor) {
    mp_macro_editor->refresh ();
  }
}

void
MacroController::add_temp_macro (lay::Macro *m)
{
  m_temp_macros.add_unspecific (m);
}

void
MacroController::add_macro_items_to_menu (lay::MacroCollection &collection, int &n, std::set<std::string> &groups, const lay::Technology *tech, std::vector<std::pair<std::string, std::string> > *key_bindings)
{
  for (lay::MacroCollection::child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {

    //  check whether the macro collection is associated with the selected technology (if there is one)
    bool consider = false;
    if (! tech || c->second->virtual_mode () != lay::MacroCollection::TechFolder) {
      consider = true;
    } else {
      const std::vector<std::pair<std::string, std::string> > &mc = lay::Application::instance ()->macro_categories ();
      for (std::vector<std::pair<std::string, std::string> >::const_iterator cc = mc.begin (); cc != mc.end () && !consider; ++cc) {
        consider = (c->second->path () == tl::to_string (QDir (tl::to_qstring (tech->base_path ())).filePath (tl::to_qstring (cc->first))));
      }
    }

    if (consider) {
      add_macro_items_to_menu (*c->second, n, groups, 0 /*don't check 2nd level and below*/, key_bindings);
    }

  }

  for (lay::MacroCollection::iterator c = collection.begin (); c != collection.end (); ++c) {

    std::string sc = tl::trim (c->second->shortcut ());

    if (c->second->show_in_menu ()) {

      std::string mp = tl::trim (c->second->menu_path ());
      if (mp.empty ()) {
        mp = "macros_menu.end";
      }

      std::string gn = tl::trim (c->second->group_name ());
      if (! gn.empty () && groups.find (gn) == groups.end ()) {
        groups.insert (gn);
        lay::Action as;
        as.set_separator (true);
        m_macro_actions.push_back (as);
        mp_mw->menu ()->insert_item (mp, "macro_in_menu_" + tl::to_string (n++), as);
      }

      lay::Action a;
      if (c->second->description ().empty ()) {
        a.set_title (c->second->path ());
      } else {
        a.set_title (c->second->description ());
      }
      a.set_shortcut (sc);
      m_macro_actions.push_back (a);
      mp_mw->menu ()->insert_item (mp, "macro_in_menu_" + tl::to_string (n++), a);

      m_action_to_macro.insert (std::make_pair (a.qaction (), c->second));

      MacroSignalAdaptor *adaptor = new MacroSignalAdaptor (a.qaction (), c->second);
      QObject::connect (a.qaction (), SIGNAL (triggered ()), adaptor, SLOT (run ()));

      //  store the key bindings in the array
      if (!sc.empty () && key_bindings) {
        key_bindings->push_back (std::make_pair (mp, sc));
      }

    } else if (! sc.empty ()) {

      //  Create actions for shortcut-only actions too and add them to the main window
      //  to register their shortcut.

      lay::Action a;
      if (c->second->description ().empty ()) {
        a.set_title (c->second->path ());
      } else {
        a.set_title (c->second->description ());
      }
      a.set_shortcut (sc);
      m_macro_actions.push_back (a);

      mp_mw->addAction (a.qaction ());
      MacroSignalAdaptor *adaptor = new MacroSignalAdaptor (a.qaction (), c->second);
      QObject::connect (a.qaction (), SIGNAL (triggered ()), adaptor, SLOT (run ()));

    }

  }
}

void
MacroController::update_menu_with_macros ()
{
  //  empty action to macro table now we know it's invalid
  m_action_to_macro.clear ();
  dm_do_update_menu_with_macros ();
}

void
MacroController::do_update_menu_with_macros ()
{
  if (!mp_mw) {
    return;
  }

  //  TODO: implement this by asking the technology manager for the active technology
  const lay::Technology *tech = 0;
  if (mp_mw->current_view () && mp_mw->current_view ()->active_cellview_index () >= 0 && mp_mw->current_view ()->active_cellview_index () <= int (mp_mw->current_view ()->cellviews ())) {
    std::string active_tech = mp_mw->current_view ()->active_cellview ()->tech_name ();
    tech = lay::Technologies::instance ()->technology_by_name (active_tech);
  }

  std::vector<std::pair<std::string, std::string> > key_bindings = unpack_key_binding (mp_mw->config_get (cfg_key_bindings));
  std::sort (key_bindings.begin (), key_bindings.end ());

  std::vector<std::pair<std::string, std::string> > new_key_bindings;
  for (std::vector<std::pair<std::string, std::string> >::const_iterator kb = key_bindings.begin (); kb != key_bindings.end (); ++kb) {
    if (mp_mw->menu ()->is_valid (kb->first)) {
      lay::Action a = mp_mw->menu ()->action (kb->first);
      if (m_action_to_macro.find (a.qaction ()) == m_action_to_macro.end ()) {
        new_key_bindings.push_back (*kb);
      }
    }
  }

  //  delete all existing items
  for (std::vector<lay::Action>::iterator a = m_macro_actions.begin (); a != m_macro_actions.end (); ++a) {
    mp_mw->menu ()->delete_items (*a);
  }
  m_macro_actions.clear ();
  m_action_to_macro.clear ();

  int n = 1;
  std::set<std::string> groups;
  add_macro_items_to_menu (m_temp_macros, n, groups, tech, 0);
  add_macro_items_to_menu (lay::MacroCollection::root (), n, groups, tech, &new_key_bindings);

  //  update the key bindings if required
  std::sort (new_key_bindings.begin (), new_key_bindings.end ());
  if (new_key_bindings != key_bindings) {
    mp_mw->config_set (cfg_key_bindings, pack_key_binding (new_key_bindings));
  }
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

}

