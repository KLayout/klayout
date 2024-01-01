
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


#include "layTechnologyController.h"
#include "layTechSetupDialog.h"
#include "layMainWindow.h"
#include "layApplication.h"
#include "laySaltController.h"
#include "layConfig.h"
#include "layQtTools.h"
#include "laybasicConfig.h"

#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

namespace lay
{

static const std::string cfg_tech_editor_window_state ("tech-editor-window-state");

std::string tech_string_from_name (const std::string &tn)
{
  if (tn.empty ()) {
    return tl::to_string (QObject::tr ("(Default)"));
  } else {
    return tn;
  }
}

TechnologyController::TechnologyController ()
  : PluginDeclaration (), mp_editor (0), mp_mw (0), mp_dispatcher (0), mp_active_technology (0)
{
  m_configure_enabled = true;
  m_current_technology_updated = false;
  m_technologies_configured = false;
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
TechnologyController::initialize (lay::Dispatcher *dispatcher)
{
  mp_dispatcher = dispatcher;
  mp_mw = lay::MainWindow::instance ();
  if (mp_mw) {
    mp_editor = new lay::TechSetupDialog (mp_mw);
    mp_editor->setModal (false);
  }
}

void
TechnologyController::initialized (lay::Dispatcher *dispatcher)
{
  tl_assert (dispatcher == mp_dispatcher);

  update_menu (mp_dispatcher);
  view_changed ();

  if (lay::SaltController::instance ()) {
    connect (lay::SaltController::instance (), SIGNAL (salt_changed ()), this, SLOT (sync_with_external_sources ()));
  }
}

void
TechnologyController::uninitialize (lay::Dispatcher *dispatcher)
{
  tl_assert (dispatcher == mp_dispatcher);

  m_tech_actions.clear ();
  tl::Object::detach_from_all_events ();

  if (lay::SaltController::instance ()) {
    disconnect (lay::SaltController::instance (), SIGNAL (salt_changed ()), this, SLOT (sync_with_external_sources ()));
  }
}

void
TechnologyController::get_options (std::vector < std::pair<std::string, std::string> > &options) const
{
  options.push_back (std::pair<std::string, std::string> (cfg_initial_technology, ""));
  options.push_back (std::pair<std::string, std::string> (cfg_tech_editor_window_state, ""));
}

void
TechnologyController::get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
{
  lay::PluginDeclaration::get_menu_entries (menu_entries);
  menu_entries.push_back (lay::menu_item ("technology_selector:apply_technology", "technology_selector:tech_selector_group", "@toolbar.end", tl::to_string (QObject::tr ("Technology<:techs_24px.png>{Select technology (click to apply)}"))));
}

void
TechnologyController::view_changed ()
{
  update_active_technology ();

  //  NOTE: the whole concept is a but strange here: the goal is to
  //  connect to the current view's active_cellview_changed event and
  //  the active cellview's technology_changed event. We could register
  //  events tracking the current view and active cellview which detach
  //  and attach event handlers. This is more tedious than doing this:
  //  we detach and re-attach the events whenever something changes.
  //  The event system supports this case, hence we do so.

  tl::Object::detach_from_all_events ();

  db::Technologies::instance ()->technology_changed_event.add (this, &TechnologyController::technology_changed);
  db::Technologies::instance ()->technologies_changed_event.add (this, &TechnologyController::technologies_changed);

  if (mp_mw) {

    //  NOTE: the "real" call needs to come before the re-connect handler because
    //  the latter will remove the update call
    mp_mw->current_view_changed_event.add (this, &TechnologyController::view_changed);

    if (mp_mw->current_view ()) {

      //  NOTE: the "real" call needs to come before the re-connect handler because
      //  the latter will remove the update call
      mp_mw->current_view ()->active_cellview_changed_event.add (this, &TechnologyController::view_changed);

      if (mp_mw->current_view ()->active_cellview_index () >= 0 && mp_mw->current_view ()->active_cellview_index () <= int (mp_mw->current_view ()->cellviews ())) {
        mp_mw->current_view ()->active_cellview ()->technology_changed_event.add (this, &TechnologyController::update_active_technology);
      }

    }

  }
}

db::Technology *
TechnologyController::active_technology () const
{
  return mp_active_technology;
}

void
TechnologyController::update_active_technology ()
{
  db::Technology *active_tech = 0;
  if (mp_mw && mp_mw->current_view () && mp_mw->current_view ()->active_cellview_index () >= 0 && mp_mw->current_view ()->active_cellview_index () <= int (mp_mw->current_view ()->cellviews ())) {

    std::string tn = mp_mw->current_view ()->active_cellview ()->tech_name ();
    if (db::Technologies::instance ()->has_technology (tn)) {
      active_tech = db::Technologies::instance ()->technology_by_name (tn);
    }

  }


  mp_active_technology = active_tech;

  if (mp_mw) {
    if (active_tech) {
      mp_mw->tech_message (tech_string_from_name (active_tech->name ()));
    } else {
      mp_mw->tech_message (std::string ());
    }
  }

  if (mp_active_technology != active_tech) {
    emit active_technology_changed ();
  }

#if 0 
  //  Hint with this implementation, the current technology follows the current layout.
  //  Although that's a nice way to display the current technology, it's pretty confusing
  lay::Dispatcher *pr = mp_plugin_root;
  if (pr) {
    pr->config_set (cfg_initial_technology, active_tech);
  }
#endif
}

void
TechnologyController::technologies_changed ()
{
  //  update the configuration to reflect the persisted technologies
  lay::Dispatcher *dispatcher = mp_dispatcher;
  if (dispatcher) {
    m_configure_enabled = false;
    try {
      dispatcher->config_set (cfg_technologies, db::Technologies::instance ()->to_xml ());
      m_configure_enabled = true;
    } catch (...) {
      m_configure_enabled = true;
      throw;
    }
  }

  update_menu (dispatcher);
  emit technologies_edited ();
}

void
TechnologyController::technology_changed (db::Technology *)
{
  technologies_changed ();
}

bool
TechnologyController::configure (const std::string &name, const std::string &value)
{
  if (! m_configure_enabled) {

    //  ignore configuration request (prevent recursion

  } else if (name == cfg_initial_technology) {

    if (value != m_current_technology) {
      m_current_technology = value;
      m_current_technology_updated = true;
    }

  } else if (name == cfg_tech_editor_window_state) {

    lay::restore_dialog_state (mp_editor, value);

  } else if (name == cfg_technologies) {

    if (! value.empty ()) {

      try {
        db::Technologies new_tech = *db::Technologies::instance ();
        new_tech.load_from_xml (value);
        replace_technologies (new_tech);
        m_technologies_configured = true;
      } catch (...) {
      }

    }

  }
  return false;
}

void
TechnologyController::config_finalize ()
{
  if (m_technologies_configured) {
    update_menu (mp_dispatcher);
    emit technologies_edited ();
    m_technologies_configured = false;
  }

  if (m_current_technology_updated) {
    update_current_technology (mp_dispatcher);
    m_current_technology_updated = false;
  }
}

bool
TechnologyController::menu_activated (const std::string &symbol) const
{
  if (symbol == "technology_selector:apply_technology") {

    if (lay::LayoutView::current () && lay::LayoutView::current ()->active_cellview ().is_valid ()) {

      if (mp_mw) {

        //  apply technology with undo
        mp_mw->manager ().transaction (tl::sprintf (tl::to_string (tr ("Apply technology '%s'")), m_current_technology));
        try {
          lay::LayoutView::current ()->active_cellview ()->apply_technology (m_current_technology);
          mp_mw->manager ().commit ();
        } catch (...) {
          mp_mw->manager ().cancel ();
          throw;
        }

      } else {
        lay::LayoutView::current ()->active_cellview ()->apply_technology (m_current_technology);
      }

    }

    return true;

  } else {
    return lay::PluginDeclaration::menu_activated (symbol);
  }
}

void
TechnologyController::update_current_technology (lay::Dispatcher *dispatcher)
{
  if (! dispatcher || ! dispatcher->has_ui ()) {
    return;
  }

  std::string title = tech_string_from_name (m_current_technology);

  std::vector<std::string> menu_entries = dispatcher->menu ()->group ("tech_selector_group");
  for (std::vector<std::string>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {
    lay::Action *action = dispatcher->menu ()->action (*m);
    action->set_title (title);
  }

  std::map<std::string, const db::Technology *> tech_by_name;
  for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
    tech_by_name.insert (std::make_pair (t->name (), t.operator-> ()));
  }

  size_t it = 0;
  for (std::map<std::string, const db::Technology *>::const_iterator t = tech_by_name.begin (); t != tech_by_name.end () && it < m_tech_actions.size (); ++t, ++it) {
    m_tech_actions[it]->set_checked (t->second->name () == m_current_technology);
  }
}

void
TechnologyController::update_menu (lay::Dispatcher *dispatcher)
{
  if (! dispatcher || ! dispatcher->has_ui ()) {
    return;
  }

  if (lay::LayoutView::current () && lay::LayoutView::current ()->active_cellview ().is_valid ()) {
    m_current_technology = lay::LayoutView::current ()->active_cellview ()->tech_name ();
  }

  if (! db::Technologies::instance()->has_technology (m_current_technology)) {
    m_current_technology = std::string ();
  }

  std::string title = tech_string_from_name (m_current_technology);

  size_t ntech = 0;
  for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
    ++ntech;
  }

  std::vector<std::string> tech_group = dispatcher->menu ()->group ("tech_selector_group");

  for (std::vector<std::string>::const_iterator t = tech_group.begin (); t != tech_group.end (); ++t) {
    lay::Action *action = dispatcher->menu ()->action (*t);
    action->set_title (title);
    action->set_enabled (ntech > 1);
    std::vector<std::string> items = dispatcher->menu ()->items (*t);
    for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
      dispatcher->menu ()->delete_item (*i);
    }
  }

  m_tech_actions.clear ();

  std::map<std::string, std::map<std::string, const db::Technology *> > tech_by_name_and_group;
  for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
    tech_by_name_and_group [tl::trim (t->group ())].insert (std::make_pair (t->name (), t.operator-> ()));
  }

  for (std::vector<std::string>::const_iterator tg = tech_group.begin (); tg != tech_group.end (); ++tg) {

    int ig = 0;
    for (std::map<std::string, std::map<std::string, const db::Technology *> >::const_iterator g = tech_by_name_and_group.begin (); g != tech_by_name_and_group.end (); ++g) {

      std::string tp = *tg;
      if (! g->first.empty ()) {
        std::string gn = "techgroup_" + tl::to_string (++ig);
        dispatcher->menu ()->insert_menu (*tg + ".end", gn, g->first);
        tp = *tg + "." + gn;
      }
      tp += ".end";

      int it = 0;
      for (std::map<std::string, const db::Technology *>::const_iterator t = g->second.begin (); t != g->second.end (); ++t, ++it) {

        std::string title = tech_string_from_name (t->first);

        m_tech_actions.push_back (new lay::ConfigureAction ("", cfg_initial_technology, t->first));
        m_tech_actions.back ()->set_title (title); // setting the title here avoids interpretation of '(...)' etc.
        m_tech_actions.back ()->set_checkable (true);
        m_tech_actions.back ()->set_checked (t->first == m_current_technology);
        dispatcher->menu ()->insert_item (tp, "technology_" + tl::to_string (it), m_tech_actions.back ());

      }

    }

  }

  update_active_technology ();
}

void
TechnologyController::replace_technologies (const db::Technologies &technologies)
{
  bool has_active_tech = (mp_active_technology != 0);
  std::string active_tech_name;
  if (mp_active_technology) {
    active_tech_name = mp_active_technology->name ();
  }

  db::Technologies ().instance ()->begin_updates ();
  *db::Technologies ().instance () = technologies;
  db::Technologies ().instance ()->end_updates_no_event ();

  if (has_active_tech) {
    mp_active_technology = db::Technologies::instance ()->technology_by_name (active_tech_name);
  }
}

void
TechnologyController::show_editor ()
{
  db::Technologies new_tech = *db::Technologies ().instance ();

  if (mp_editor && mp_editor->exec_dialog (new_tech)) {

    std::string err_msg;

    //  determine the technology files that need to be deleted and delete them
    std::set<std::string> files_before;
    for (db::Technologies::const_iterator t = new_tech.begin (); t != new_tech.end (); ++t) {
      if (! t->tech_file_path ().empty () && ! t->is_persisted ()) {
        files_before.insert (t->tech_file_path ());
      }
    }
    for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
      if (! t->tech_file_path ().empty () && ! t->is_persisted () && files_before.find (t->tech_file_path ()) == files_before.end ()) {
        //  TODO: issue an error if files could not be removed
        QFile (tl::to_qstring (t->tech_file_path ())).remove ();
      }
    }

    replace_technologies (new_tech);

    //  save the technologies that need to be saved
    //  TODO: save only the ones that really need saving
    for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {

      if (! t->tech_file_path ().empty () && ! t->is_persisted ()) {

        //  create the tech folder if required

        try {

          QDir dir = QFileInfo (tl::to_qstring (t->tech_file_path ())).absoluteDir ();
          QStringList to_create;
          while (! dir.isRoot() && ! dir.exists ()) {
            to_create << dir.dirName ();
            dir = QFileInfo (dir.path ()).absoluteDir ();
          }

          while (! to_create.empty ()) {
            if (! dir.mkdir (to_create.back ())) {
              throw tl::CancelException ();
            }
            if (! dir.cd (to_create.back ())) {
              throw tl::CancelException ();
            }
            to_create.pop_back ();
          }

          t->save (t->tech_file_path ());

        } catch (...) {
          if (! err_msg.empty ()) {
            err_msg += "\n";
          }
          err_msg += t->tech_file_path ();
        }

      }

    }

    if (! err_msg.empty ()) {
      QMessageBox::critical (mp_mw, QObject::tr ("Error Saving Technology Files"),
                                    QObject::tr ("The following files could not be saved:\n\n") + tl::to_qstring (err_msg),
                                    QMessageBox::Ok);
    }

    technologies_changed ();

  }

  mp_dispatcher->config_set (cfg_tech_editor_window_state, lay::save_dialog_state (mp_editor));
}

const std::string &
TechnologyController::default_root ()
{
  tl_assert (!m_paths.empty ());
  return m_paths.front ();
}

void
TechnologyController::load ()
{
  rescan (*db::Technologies::instance ());
}

void
TechnologyController::sync_with_external_sources ()
{
  rescan (*db::Technologies::instance ());
}

void
TechnologyController::rescan (db::Technologies &technologies)
{
  db::Technologies current = technologies;

  //  start with all persisted technologies (at least "default")
  technologies.clear ();
  for (db::Technologies::const_iterator t = current.begin (); t != current.end (); ++t) {
    if (t->is_persisted ()) {
      technologies.add (*t);
    }
  }

  std::vector<std::string> paths = m_paths;
  std::set<std::string> readonly_paths;
  std::map<std::string, std::string> grain_names;

  //  add the salt grains as potential sources for tech definitions
  lay::SaltController *sc = lay::SaltController::instance ();
  if (sc) {
    for (lay::Salt::flat_iterator g = sc->salt ().begin_flat (); g != sc->salt ().end_flat (); ++g) {
      paths.push_back ((*g)->path ());
      grain_names.insert (std::make_pair ((*g)->path (), (*g)->name ()));
      if ((*g)->is_readonly ()) {
        readonly_paths.insert ((*g)->path ());
      }
    }
  }

  for (std::vector<std::string>::const_iterator p = paths.begin (); p != paths.end (); ++p) {

    QDir dir (tl::to_qstring (*p));
    if (! dir.exists ()) {
      continue;
    }

    bool readonly = (readonly_paths.find (*p) != readonly_paths.end ());

    std::string grain_name;
    std::map<std::string, std::string>::const_iterator gn = grain_names.find (*p);
    if (gn != grain_names.end ()) {
      grain_name = gn->second;
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

        db::Technology t;
        t.load (tl::to_string (*lf));
        t.set_persisted (false);   // don't save that one in the configuration
        t.set_readonly (readonly || ! QFileInfo (dir.filePath (*lf)).isWritable ());
        t.set_grain_name (grain_name);
        technologies.add (t);

      } catch (tl::Exception &ex) {
        tl::warn << tl::to_string (QObject::tr ("Unable to auto-import technology file ")) << tl::to_string (*lf) << ": " << ex.msg ();
      }

    }

  }

  for (std::vector<db::Technology>::const_iterator t = m_temp_tech.begin (); t != m_temp_tech.end (); ++t) {

    if (tl::verbosity () >= 20) {
      tl::info << "Registering special technology from " << t->tech_file_path () << " as " << t->name ();
    }

    db::Technology *tech = technologies.add (*t);
    tech->set_persisted (false);                //  don't save that one in the configuration
    tech->set_tech_file_path (std::string ());  //  don't save to a file either
    tech->set_readonly (true);                  //  don't edit

  }
}

void
TechnologyController::add_temp_tech (const db::Technology &t)
{
  m_temp_tech.push_back (t);
}

void
TechnologyController::add_path (const std::string &p)
{
  std::string tp = tl::to_string (QDir (tl::to_qstring (p)).filePath (QString::fromUtf8 ("tech")));
  m_paths.push_back (tp);
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new TechnologyController (), 110, "TechnologyController");

}
