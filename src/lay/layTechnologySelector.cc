
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#include "layTechnologySelector.h"
#include "layPlugin.h"
#include "laybasicConfig.h"
#include "layMainWindow.h"
#include "layTechnology.h"
#include "laybasicConfig.h"
#include "tlDeferredExecution.h"

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

class LAY_PUBLIC TechnologySelector
  : public PluginDeclaration,
    public tl::Object
{
public:
  TechnologySelector () 
    : PluginDeclaration ()
  {
    m_current_technology_updated = false;
  }

  void initialize (lay::PluginRoot * /*root*/)
  {
    //  don't initialize in the -z case (no gui)
    if (! lay::MainWindow::instance ()) {
      return;
    }

    update_menu ();
    update_after_change ();
  }

  void uninitialize (lay::PluginRoot * /*root*/)
  {
    m_tech_actions.clear ();
    tl::Object::detach_from_all_events ();
  }

  void get_options (std::vector < std::pair<std::string, std::string> > &options) const 
  {
    options.push_back (std::pair<std::string, std::string> (cfg_initial_technology, ""));
  }

  void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::MenuEntry ("technology_selector:apply_technology", "technology_selector:tech_selector_group", "@toolbar.end", tl::to_string (QObject::tr ("Technology<:techs.png>{Select technology (click to apply)}"))));
  }

private:
  tl::stable_vector <lay::Action> m_tech_actions;
  std::string m_current_technology;
  std::string m_active_technology;
  bool m_current_technology_updated;

  void update_after_change ()
  {
    //  re-attach all events
    tl::Object::detach_from_all_events ();

    lay::MainWindow::instance ()->current_view_changed_event.add (this, &TechnologySelector::update_after_change);
    lay::Technologies::instance ()->technology_changed_event.add (this, &TechnologySelector::technology_changed);
    lay::Technologies::instance ()->technologies_changed_event.add (this, &TechnologySelector::technologies_changed);

    if (lay::LayoutView::current ()) {
      lay::LayoutView::current ()->active_cellview_changed_event.add (this, &TechnologySelector::update_after_change);
    }

    std::string active_tech;
    if (lay::LayoutView::current () && lay::LayoutView::current ()->active_cellview_index () >= 0 && lay::LayoutView::current ()->active_cellview_index () <= int (lay::LayoutView::current ()->cellviews ())) {
      lay::LayoutView::current ()->active_cellview ()->technology_changed_event.add (this, &TechnologySelector::update_after_change);
      active_tech = lay::LayoutView::current ()->active_cellview ()->tech_name ();
    }

    if (m_active_technology != active_tech) {

      m_active_technology = active_tech;

      lay::MainWindow *mw = lay::MainWindow::instance ();
      if (mw) {
        mw->tech_message (tech_string_from_name (active_tech));
        //  need to do this since macros may be bound to the new technology
        mw->update_menu_with_macros ();
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

  void technologies_changed ()
  {
    //  delay actual update of menu so we can compress multiple events
    update_menu ();
  }

  void technology_changed (lay::Technology *)
  {
    //  delay actual update of menu so we can compress multiple events
    update_menu ();
  }

  bool configure (const std::string &name, const std::string &value)
  {
    if (name == cfg_initial_technology) {

      if (value != m_current_technology) {
        m_current_technology = value;
        m_current_technology_updated = true;
      }
      
    }
    return false;
  }

  void config_finalize ()
  {
    if (m_current_technology_updated) {
      update_current_technology ();
      m_current_technology_updated = false;
    }
  }

  bool menu_activated (const std::string &symbol) const
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

  void update_current_technology ()
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

  void update_menu ()
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
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new TechnologySelector (), 9000, "TechnologySelector");

}


