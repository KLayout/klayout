
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


#include "layTechnologyController.h"
#include "layTechSetupDialog.h"
#include "layMainWindow.h"
#include "layMacroController.h"
#include "layApplication.h"
#include "laybasicConfig.h"

#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

namespace lay
{

std::string tech_string_from_name (const std::string &tn)
{
  if (tn.empty ()) {
    return tl::to_string (QObject::tr ("(Default)"));
  } else {
    return tn;
  }
}

TechnologyController::TechnologyController ()
  : PluginDeclaration (), mp_editor (0), mp_mw (0), m_no_macros (false)
{
  m_current_technology_updated = false;
}

void
TechnologyController::enable_macros (bool enable)
{
  m_no_macros = !enable;
}

TechnologyController *
TechnologyController::instance ()
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    TechnologyController *tc = dynamic_cast <TechnologyController *> (cls.operator-> ());
    if (tc) {
      return tc;
    }
  }
  return 0;
}

void
TechnologyController::initialize (lay::PluginRoot * /*root*/)
{
  //  .. nothing yet ..
}

void
TechnologyController::initialized (lay::PluginRoot *root)
{
  sync_tech_macro_locations ();

  mp_mw = dynamic_cast <lay::MainWindow *> (root);
  if (mp_mw) {
    mp_editor = new lay::TechSetupDialog (mp_mw);
    mp_editor->setModal (false);
  }

  update_menu ();
  update_after_change ();
}

void
TechnologyController::uninitialize (lay::PluginRoot * /*root*/)
{
  m_tech_actions.clear ();
  tl::Object::detach_from_all_events ();
}

void
TechnologyController::get_options (std::vector < std::pair<std::string, std::string> > &options) const
{
  options.push_back (std::pair<std::string, std::string> (cfg_initial_technology, ""));
}

void
TechnologyController::get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
{
  lay::PluginDeclaration::get_menu_entries (menu_entries);
  menu_entries.push_back (lay::MenuEntry ("technology_selector:apply_technology", "technology_selector:tech_selector_group", "@toolbar.end", tl::to_string (QObject::tr ("Technology<:techs.png>{Select technology (click to apply)}"))));
}

void
TechnologyController::update_after_change ()
{
  //  re-attach all events
  tl::Object::detach_from_all_events ();

  lay::MainWindow *mw = lay::MainWindow::instance ();
  lay::MacroController *mc = lay::MacroController::instance ();

  if (mw) {
    mw->current_view_changed_event.add (this, &TechnologyController::update_after_change);
  }

  lay::Technologies::instance ()->technology_changed_event.add (this, &TechnologyController::technology_changed);
  lay::Technologies::instance ()->technologies_changed_event.add (this, &TechnologyController::technologies_changed);

  if (lay::LayoutView::current ()) {
    lay::LayoutView::current ()->active_cellview_changed_event.add (this, &TechnologyController::update_after_change);
  }

  std::string active_tech;
  if (lay::LayoutView::current () && lay::LayoutView::current ()->active_cellview_index () >= 0 && lay::LayoutView::current ()->active_cellview_index () <= int (lay::LayoutView::current ()->cellviews ())) {
    lay::LayoutView::current ()->active_cellview ()->technology_changed_event.add (this, &TechnologyController::update_after_change);
    active_tech = lay::LayoutView::current ()->active_cellview ()->tech_name ();
  }

  if (m_active_technology != active_tech) {

    m_active_technology = active_tech;

    if (mw) {
      mw->tech_message (tech_string_from_name (active_tech));
    }

    if (mc) {
      //  TODO: let the macro controller monitor the active technology
      //  need to do this since macros may be bound to the new technology
      mc->update_menu_with_macros ();
    }

  }

#if 0 
  //  Hint with this implementation, the current technology follows the current layout.
  //  Although that's a nice way to display the current technology, it's pretty confusing
  lay::PluginRoot *pr = lay::PluginRoot::instance ();
  if (pr) {
    pr->config_set (cfg_initial_technology, active_tech);
    pr->config_finalize ();
  }
#endif
}

void
TechnologyController::technologies_changed ()
{
  //  delay actual update of menu so we can compress multiple events
  update_menu ();
}

void
TechnologyController::technology_changed (lay::Technology *)
{
  //  delay actual update of menu so we can compress multiple events
  update_menu ();
}

bool
TechnologyController::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_initial_technology) {

    if (value != m_current_technology) {
      m_current_technology = value;
      m_current_technology_updated = true;
    }

  }
  return false;
}

void
TechnologyController::config_finalize ()
{
  if (m_current_technology_updated) {
    update_current_technology ();
    m_current_technology_updated = false;
  }
}

bool
TechnologyController::menu_activated (const std::string &symbol) const
{
  if (symbol == "technology_selector:apply_technology") {
    if (lay::LayoutView::current () && lay::LayoutView::current ()->active_cellview ().is_valid ()) {
      lay::LayoutView::current ()->active_cellview ()->apply_technology (m_current_technology);
    }
    return true;
  } else {
    return lay::PluginDeclaration::menu_activated (symbol);
  }
}

void
TechnologyController::update_current_technology ()
{
  lay::AbstractMenuProvider *pr = lay::AbstractMenuProvider::instance ();
  if (! pr) {
    return;
  }

  std::string title = tech_string_from_name (m_current_technology);

  std::vector<std::string> menu_entries = pr->menu ()->group ("tech_selector_group");
  for (std::vector<std::string>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {
    lay::Action action = pr->menu ()->action (*m);
    action.set_title (title);
  }

  size_t it = 0;
  for (lay::Technologies::const_iterator t = lay::Technologies::instance ()->begin (); t != lay::Technologies::instance ()->end () && it < m_tech_actions.size (); ++t, ++it) {
    m_tech_actions[it].set_checked (t->name () == m_current_technology);
  }
}

void
TechnologyController::update_menu ()
{
  lay::AbstractMenuProvider *pr = lay::AbstractMenuProvider::instance ();
  if (! pr) {
    return;
  }

  if (lay::LayoutView::current () && lay::LayoutView::current ()->active_cellview ().is_valid ()) {
    m_current_technology = lay::LayoutView::current ()->active_cellview ()->tech_name ();
  }

  std::string title = tech_string_from_name (m_current_technology);

  size_t ntech = 0;
  for (lay::Technologies::const_iterator t = lay::Technologies::instance ()->begin (); t != lay::Technologies::instance ()->end (); ++t) {
    ++ntech;
  }

  std::vector<std::string> tech_group = pr->menu ()->group ("tech_selector_group");

  for (std::vector<std::string>::const_iterator t = tech_group.begin (); t != tech_group.end (); ++t) {
    lay::Action action = pr->menu ()->action (*t);
    action.set_title (title);
    action.set_visible (ntech > 1);
    std::vector<std::string> items = pr->menu ()->items (*t);
    for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
      pr->menu ()->delete_item (*i);
    }
  }

  m_tech_actions.clear ();

  int it = 0;
  for (lay::Technologies::const_iterator t = lay::Technologies::instance ()->begin (); t != lay::Technologies::instance ()->end (); ++t, ++it) {

    std::string title = tech_string_from_name (t->name ());

    m_tech_actions.push_back (pr->create_config_action ("", cfg_initial_technology, t->name ()));
    m_tech_actions.back ().set_title (title); // setting the title here avoids interpretation of '(...)' etc.
    m_tech_actions.back ().set_checkable (true);
    m_tech_actions.back ().set_checked (t->name () == m_current_technology);
    for (std::vector<std::string>::const_iterator t = tech_group.begin (); t != tech_group.end (); ++t) {
      pr->menu ()->insert_item (*t + ".end", "technology_" + tl::to_string (it), m_tech_actions.back ());
    }

  }
}

void
TechnologyController::show_editor ()
{
  if (mp_editor && mp_editor->exec ()) {

    std::vector<lay::MacroCollection *> nm = sync_tech_macro_locations ();

    bool has_autorun = false;
    for (std::vector<lay::MacroCollection *>::const_iterator m = nm.begin (); m != nm.end () && ! has_autorun; ++m) {
      has_autorun = (*m)->has_autorun ();
    }

    if (has_autorun && QMessageBox::question (mp_mw, QObject::tr ("Run Macros"), QObject::tr ("Some macros associated with technologies now are configured to run automatically.\n\nChoose 'Yes' to run these macros now. Choose 'No' to not run them."), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      for (std::vector<lay::MacroCollection *>::const_iterator m = nm.begin (); m != nm.end (); ++m) {
        (*m)->autorun ();
      }
    }

    //  because the macro-tech association might have changed, do this:
    //  TODO: let the macro controller monitor the technologies.
    lay::MacroController *mc = lay::MacroController::instance ();
    if (mc) {
      mc->update_menu_with_macros ();
    }

  }
}

const std::string &
TechnologyController::default_root ()
{
  tl_assert (!m_paths.empty ());
  return m_paths.front ();
}

void
TechnologyController::refresh ()
{
  try {

    lay::Technologies::instance ()->begin_updates ();
    lay::Technologies::instance ()->clear ();

    for (std::vector<std::string>::const_iterator p = m_paths.begin (); p != m_paths.end (); ++p) {

      QDir dir (tl::to_qstring (*p));
      if (! dir.exists ()) {
        continue;
      }

      QStringList name_filters;
      name_filters << QString::fromUtf8 ("*.lyt");

      QStringList lyt_files;

      QDirIterator di (dir.path (), name_filters, QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
      while (di.hasNext ()) {
        lyt_files << di.next ();
      }

      lyt_files.sort ();

      for (QStringList::const_iterator lf = lyt_files.begin (); lf != lyt_files.end (); ++lf) {

        try {

          if (tl::verbosity () >= 20) {
            tl::info << "Auto-importing technology from " << tl::to_string (*lf);
          }

          lay::Technology t;
          t.load (tl::to_string (*lf));
          t.set_persisted (false);   // don't save that one in the configuration
          t.set_readonly (! QFileInfo (dir.filePath (*lf)).isWritable ());
          lay::Technologies::instance ()->add (new lay::Technology (t));

        } catch (tl::Exception &ex) {
          tl::warn << tl::to_string (QObject::tr ("Unable to auto-import technology file ")) << tl::to_string (*lf) << ": " << ex.msg ();
        }

      }

    }

    for (std::vector<lay::Technology>::const_iterator t = m_temp_tech.begin (); t != m_temp_tech.end (); ++t) {

      lay::Technology *tech = new lay::Technology (*t);
      tech->set_persisted (false);    // don't save that one in the configuration
      tech->set_readonly (true);
      lay::Technologies::instance ()->add (tech);

    }

    lay::Technologies::instance ()->end_updates ();

  } catch (...) {
    lay::Technologies::instance ()->end_updates ();
    throw;
  }
}

void
TechnologyController::add_temp_tech (const lay::Technology &t)
{
  m_temp_tech.push_back (t);
}

void
TechnologyController::add_path (const std::string &p)
{
  m_paths.push_back (p);
}

std::vector<lay::MacroCollection *>
TechnologyController::sync_tech_macro_locations ()
{
  lay::MacroController *mc = lay::MacroController::instance ();

  if (! mc || m_no_macros) {
    return std::vector<lay::MacroCollection *> ();
  }

  std::set<std::pair<std::string, std::string> > tech_macro_paths;
  std::map<std::pair<std::string, std::string>, std::string> tech_names_by_path;

  //  Add additional places where the technologies define some macros
  for (lay::Technologies::const_iterator t = lay::Technologies::instance ()->begin (); t != lay::Technologies::instance ()->end (); ++t) {

    if (t->base_path ().empty ()) {
      continue;
    }

    for (size_t c = 0; c < mc->macro_categories ().size (); ++c) {

      QDir base_dir (tl::to_qstring (t->base_path ()));
      if (base_dir.exists ()) {

        QDir macro_dir (base_dir.filePath (tl::to_qstring (mc->macro_categories () [c].first)));
        if (macro_dir.exists ()) {

          std::string mp = tl::to_string (macro_dir.path ());
          std::pair<std::string, std::string> cp (mc->macro_categories () [c].first, mp);
          tech_macro_paths.insert (cp);
          std::string &tn = tech_names_by_path [cp];
          if (! tn.empty ()) {
            tn += ",";
          }
          tn += t->name ();

        }

      }

    }

  }

  //  delete macro collections which are no longer required or update description
  std::vector<lay::MacroCollection *> folders_to_delete;
  std::string desc_prefix = tl::to_string (QObject::tr ("Technology")) + " - ";

  lay::MacroCollection *root = &lay::MacroCollection::root ();

  for (lay::MacroCollection::child_iterator m = root->begin_children (); m != root->end_children (); ++m) {

    std::pair<std::string, std::string> cp (m->second->category (), m->second->path ());
    if (m->second->virtual_mode () == lay::MacroCollection::TechFolder && m_tech_macro_paths.find (cp) != m_tech_macro_paths.end ()) {

      if (tech_macro_paths.find (cp) == tech_macro_paths.end ()) {
        //  no longer used
        folders_to_delete.push_back (m->second);
      } else {
        //  used: update description if required
        std::string desc = desc_prefix + tech_names_by_path [cp];
        m->second->set_description (desc);
      }

    }

  }

  for (std::vector<lay::MacroCollection *>::iterator m = folders_to_delete.begin (); m != folders_to_delete.end (); ++m) {
    if (tl::verbosity () >= 20) {
      tl::info << "Removing macro folder " << (*m)->path () << ", category '" << (*m)->category () << "' because no longer in use";
    }
    root->erase (*m);
  }

  //  store new paths
  m_tech_macro_paths = tech_macro_paths;

  //  add new folders
  for (lay::MacroCollection::child_iterator m = root->begin_children (); m != root->end_children (); ++m) {
    if (m->second->virtual_mode () == lay::MacroCollection::TechFolder) {
      std::pair<std::string, std::string> cp (m->second->category (), m->second->path ());
      tech_macro_paths.erase (cp);
    }
  }

  std::vector<lay::MacroCollection *> new_folders;

  for (std::set<std::pair<std::string, std::string> >::const_iterator p = tech_macro_paths.begin (); p != tech_macro_paths.end (); ++p) {

    const std::string &tn = tech_names_by_path [*p];

    //  TODO: is it wise to make it writeable?
    if (tl::verbosity () >= 20) {
      tl::info << "Adding macro folder " << p->second << ", category '" << p->first << "' for technologies " << tn;
    }

    //  Add the folder. Note: it may happen that a macro folder for the tech specific macros already exists in
    //  a non-tech context.
    //  In that case, the add_folder method will return 0.
    lay::MacroCollection *mc = lay::MacroCollection::root ().add_folder (desc_prefix + tn, p->second, p->first, false);
    if (mc) {

      mc->set_virtual_mode (lay::MacroCollection::TechFolder);
      new_folders.push_back (mc);

    }

  }

  return new_folders;
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new TechnologyController (), 110, "TechnologyController");

}
