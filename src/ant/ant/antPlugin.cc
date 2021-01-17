
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include "layPlugin.h"
#include "layAbstractMenu.h"
#include "layConverters.h"
#include "layConfigurationDialog.h"
#include "layDispatcher.h"
#include "antConfigPage.h"
#include "antConfig.h"
#include "antPlugin.h"

#include <QApplication>

namespace ant
{

static PluginDeclaration *sp_instance = 0;

PluginDeclaration::PluginDeclaration ()
  : m_current_template (0), 
    m_current_template_updated (true), m_templates_updated (true)
{
  sp_instance = this;
}

PluginDeclaration::~PluginDeclaration ()
{
  sp_instance = 0;
}

PluginDeclaration *
PluginDeclaration::instance ()
{
  return sp_instance;
}

void 
PluginDeclaration::get_options (std::vector < std::pair<std::string, std::string> > &options) const
{
  options.push_back (std::pair<std::string, std::string> (cfg_max_number_of_rulers, "-1"));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_snap_range, "8"));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_color, lay::ColorConverter ().to_string (QColor ())));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_halo, "true"));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_snap_mode, ACConverter ().to_string (lay::AC_Any)));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_obj_snap, tl::to_string (true)));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_grid_snap, tl::to_string (false)));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_templates, ""));
  options.push_back (std::pair<std::string, std::string> (cfg_current_ruler_template, "0"));
  //  grid-micron is not configured here since some other entity is supposed to do this.
}

std::vector<std::pair <std::string, lay::ConfigPage *> > 
PluginDeclaration::config_pages (QWidget *parent) const 
{
  std::vector<std::pair <std::string, lay::ConfigPage *> > pages;
  pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Rulers And Annotations|Snapping")), new ant::ConfigPage (parent)));
  pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Rulers And Annotations|Appearance")), new ant::ConfigPage2 (parent)));
  pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Rulers And Annotations|Angle")), new ant::ConfigPage3 (parent)));
  pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Rulers And Annotations|Templates")), new ant::ConfigPage4 (parent)));
  return pages;
}

void 
PluginDeclaration::get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
{
  lay::PluginDeclaration::get_menu_entries (menu_entries);
  menu_entries.push_back (lay::separator ("rulers_group", "edit_menu.end"));
  menu_entries.push_back (lay::menu_item ("ant::clear_all_rulers", "clear_all_rulers:edit", "edit_menu.end", tl::to_string (QObject::tr ("Clear All Rulers And Annotations(Ctrl+K)"))));
  menu_entries.push_back (lay::menu_item ("ant::configure", "configure_rulers", "edit_menu.end", tl::to_string (QObject::tr ("Ruler And Annotation Setup"))));
}

lay::Plugin *
PluginDeclaration::create_plugin (db::Manager *manager, lay::Dispatcher *, lay::LayoutView *view) const
{
  return new ant::Service (manager, view);
}

bool 
PluginDeclaration::menu_activated (const std::string &symbol) const
{
  if (symbol == "ant::configure") {

    lay::ConfigurationDialog config_dialog (QApplication::activeWindow (), lay::Dispatcher::instance (), "ant::Plugin");
    config_dialog.exec ();
    
    return true;

  } else {
    return lay::PluginDeclaration::menu_activated (symbol);
  }
}

bool 
PluginDeclaration::implements_editable (std::string &title) const
{
  title = tl::to_string (QObject::tr ("Rulers And Annotations"));
  return true;
}

bool 
PluginDeclaration::implements_mouse_mode (std::string &title) const
{
  title = "ruler:ruler_mode_group:ruler_templates_group\t" + tl::to_string (QObject::tr ("Ruler{Add rulers and annotations}")) + "<:ruler.png>";
  return true;
}

bool 
PluginDeclaration::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_ruler_templates) {

    m_templates = ant::Template::from_string (value);
    m_templates_updated = true;

  } else if (name == cfg_current_ruler_template) {
    
    int n = 0;
    tl::from_string (value, n);
    
    if (n != m_current_template) {
      m_current_template = n;
      m_current_template_updated = true;
    }
    
  }
  return false;
}

void 
PluginDeclaration::config_finalize ()
{
  if (m_templates_updated) {

    update_menu ();
    m_templates_updated = false;
    m_current_template_updated = false;

  } else if (m_current_template_updated) {

    update_current_template ();
    m_current_template_updated = false;

  }
}

void 
PluginDeclaration::initialized (lay::Dispatcher *root)
{
  //  Check if we already have templates (initial setup)
  bool any_templates = false;
  for (std::vector<ant::Template>::iterator i = m_templates.begin (); ! any_templates && i != m_templates.end (); ++i) {
    any_templates = ! i->category ().empty ();
  }

  if (! any_templates) {

    //  This is the migration path from <= 0.24 to 0.25: clear all templates unless we
    //  have categorized ones there. Those can't be deleted, so we know we have a 0.25
    //  setup if there are some
    m_templates.clear ();

    //  Set up the templates we want to see (plus some non-categorized templates)

    m_templates.push_back (ant::Template (tl::to_string (QObject::tr ("Ruler")), "$X", "$Y", "$D", ant::Object::STY_ruler, ant::Object::OL_diag, true, lay::AC_Global, "_ruler"));

    m_templates.push_back (ant::Template (tl::to_string (QObject::tr ("Cross")), "", "", "$U,$V", ant::Object::STY_cross_both, ant::Object::OL_diag, true, lay::AC_Global, "_cross"));
    m_templates.back ().set_mode (ant::Template::RulerSingleClick);

    m_templates.push_back (ant::Template (tl::to_string (QObject::tr ("Measure")), "$X", "$Y", "$D", ant::Object::STY_ruler, ant::Object::OL_diag, true, lay::AC_Global, "_measure"));
    m_templates.back ().set_mode (ant::Template::RulerAutoMetric);

    m_templates.push_back (ant::Template (tl::to_string (QObject::tr ("Ellipse")), "W=$(abs(X))", "H=$(abs(Y))", "", ant::Object::STY_line, ant::Object::OL_ellipse, true, lay::AC_Global, std::string ()));

    m_templates.push_back (ant::Template (tl::to_string (QObject::tr ("Box")), "W=$(abs(X))", "H=$(abs(Y))", "", ant::Object::STY_line, ant::Object::OL_box, true, lay::AC_Global, std::string ()));

    root->config_set (cfg_ruler_templates, ant::TemplatesConverter ().to_string (m_templates));
    root->config_end ();

  }
}

void 
PluginDeclaration::uninitialize (lay::Dispatcher *)
{
  m_actions.clear ();
}

void 
PluginDeclaration::update_current_template ()
{
  lay::Dispatcher *mp = lay::Dispatcher::instance ();
  if (! mp || ! mp->has_ui ()) {
    return;
  }

  if (m_current_template >= 0 && m_current_template < int (m_templates.size ())) {

    std::vector<std::string> menu_entries = mp->menu ()->group ("ruler_mode_group");
    for (std::vector<std::string>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {
      lay::Action *action = mp->menu ()->action (*m);
      action->set_title (m_templates [m_current_template].title ());
    }
    
    if (m_templates.size () > 1) {

      tl::weak_collection<lay::ConfigureAction>::iterator it = m_actions.begin ();
      int index = 0;
      for (std::vector<Template>::const_iterator tt = m_templates.begin (); tt != m_templates.end () && it != m_actions.end (); ++tt, ++it, ++index) {
        if (it.operator -> ()) {
          it->set_checked (index == m_current_template);
        }
      }
    }

  }
}

void 
PluginDeclaration::update_menu ()
{
  lay::Dispatcher *mp = lay::Dispatcher::instance ();
  if (! mp || ! mp->has_ui ()) {
    return;
  }

  if (m_current_template < 0 || m_current_template >= int (m_templates.size ())) {
    m_current_template = 0;
  }
    
  if (m_current_template >= 0 && m_current_template < int (m_templates.size ())) {
    std::vector<std::string> menu_entries = mp->menu ()->group ("ruler_mode_group");
    for (std::vector<std::string>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {
      lay::Action *action = mp->menu ()->action (*m);
      action->set_title (m_templates [m_current_template].title ());
    }
  }
  
  std::vector<std::string> tmpl_group = mp->menu ()->group ("ruler_templates_group");
  for (std::vector<std::string>::const_iterator t = tmpl_group.begin (); t != tmpl_group.end (); ++t) {
    std::vector<std::string> items = mp->menu ()->items (*t);        
    for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
      mp->menu ()->delete_item (*i);
    }
  }
    
  m_actions.clear ();

  if (m_templates.size () > 1) {
    int it = 0;
    for (std::vector<Template>::const_iterator tt = m_templates.begin (); tt != m_templates.end (); ++tt, ++it) {
      lay::ConfigureAction *action = new lay::ConfigureAction (tt->title (), cfg_current_ruler_template, tl::to_string (it));
      m_actions.push_back (action);
      action->set_checkable (true);
      action->set_checked (it == m_current_template);
      for (std::vector<std::string>::const_iterator t = tmpl_group.begin (); t != tmpl_group.end (); ++t) {
        mp->menu ()->insert_item (*t + ".end", "ruler_template_" + tl::to_string (it), action);
      }
    }
  }
    
}

void
PluginDeclaration::register_annotation_template (const ant::Template &t)
{
  if (t.category ().empty ()) {
    return;
  }

  for (std::vector<ant::Template>::iterator i = m_templates.begin (); i != m_templates.end (); ++i) {
    if (i->category () == t.category ()) {
      return;
    }
  }

  m_templates.push_back (t);
  lay::Dispatcher::instance ()->config_set (cfg_ruler_templates, ant::TemplatesConverter ().to_string (m_templates));
  lay::Dispatcher::instance ()->config_end ();
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new ant::PluginDeclaration (), 3000, "ant::Plugin");

}

