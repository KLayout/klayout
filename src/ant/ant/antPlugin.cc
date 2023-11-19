
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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
#include "layConverters.h"
#include "layDispatcher.h"
#include "layAbstractMenu.h"
#include "tlColor.h"
#include "tlLog.h"
#if defined(HAVE_QT)
#  include "layConfigurationDialog.h"
#endif
#include "antConfig.h"
#if defined(HAVE_QT)
#  include "antConfigPage.h"
#endif
#include "antPlugin.h"

#if defined(HAVE_QT)
#  include <QApplication>
#endif

namespace ant
{

static PluginDeclaration *sp_instance = 0;

static std::vector<ant::Template> make_standard_templates ()
{
  std::vector<ant::Template> templates;

  templates.push_back (ant::Template (tl::to_string (tr ("Ruler")), "$X", "$Y", "$D", ant::Object::STY_ruler, ant::Object::OL_diag, true, lay::AC_Global, "_ruler"));

  templates.push_back (ant::Template (tl::to_string (tr ("Multi-ruler")), "$X", "$Y", "$D", ant::Object::STY_ruler, ant::Object::OL_diag, true, lay::AC_Global, "_multi_ruler"));
  templates.back ().set_mode (ant::Template::RulerMultiSegment);

  templates.push_back (ant::Template (tl::to_string (tr ("Cross")), "", "", "$U,$V", ant::Object::STY_cross_both, ant::Object::OL_diag, true, lay::AC_Global, "_cross"));
  templates.back ().set_mode (ant::Template::RulerSingleClick);

  templates.push_back (ant::Template (tl::to_string (tr ("Measure")), "$X", "$Y", "$D", ant::Object::STY_ruler, ant::Object::OL_diag, true, lay::AC_Global, "_measure"));
  templates.back ().set_mode (ant::Template::RulerAutoMetric);

  templates.push_back (ant::Template (tl::to_string (tr ("Angle")), "", "", "$(sprintf('%.5g',G))°", ant::Object::STY_line, ant::Object::OL_angle, true, lay::AC_Global, "_angle"));
  templates.back ().set_mode (ant::Template::RulerThreeClicks);

  templates.push_back (ant::Template (tl::to_string (tr ("Radius")), "", "", "R=$D", ant::Object::STY_arrow_end, ant::Object::OL_radius, true, lay::AC_Global, "_radius"));
  templates.back ().set_mode (ant::Template::RulerThreeClicks);
  templates.back ().set_main_position (ant::Object::POS_center);

  templates.push_back (ant::Template (tl::to_string (tr ("Ellipse")), "W=$(abs(X))", "H=$(abs(Y))", "", ant::Object::STY_line, ant::Object::OL_ellipse, true, lay::AC_Global, std::string ()));

  templates.push_back (ant::Template (tl::to_string (tr ("Box")), "W=$(abs(X))", "H=$(abs(Y))", "", ant::Object::STY_line, ant::Object::OL_box, true, lay::AC_Global, std::string ()));

  return templates;
}

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
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_color, lay::ColorConverter ().to_string (tl::Color ())));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_halo, "true"));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_snap_mode, ACConverter ().to_string (lay::AC_Any)));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_obj_snap, tl::to_string (true)));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_grid_snap, tl::to_string (false)));
  options.push_back (std::pair<std::string, std::string> (cfg_ruler_templates, std::string ()));
  options.push_back (std::pair<std::string, std::string> (cfg_current_ruler_template, "0"));
  //  grid-micron is not configured here since some other entity is supposed to do this.
}

#if defined(HAVE_QT)
std::vector<std::pair <std::string, lay::ConfigPage *> > 
PluginDeclaration::config_pages (QWidget *parent) const 
{
  std::vector<std::pair <std::string, lay::ConfigPage *> > pages;
  pages.push_back (std::make_pair (tl::to_string (tr ("Rulers And Annotations|Snapping")), new ant::ConfigPage (parent)));
  pages.push_back (std::make_pair (tl::to_string (tr ("Rulers And Annotations|Appearance")), new ant::ConfigPage2 (parent)));
  pages.push_back (std::make_pair (tl::to_string (tr ("Rulers And Annotations|Angle")), new ant::ConfigPage3 (parent)));
  pages.push_back (std::make_pair (tl::to_string (tr ("Rulers And Annotations|Templates")), new ant::ConfigPage4 (parent)));
  return pages;
}
#endif

void 
PluginDeclaration::get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
{
  lay::PluginDeclaration::get_menu_entries (menu_entries);
  menu_entries.push_back (lay::separator ("rulers_group", "edit_menu.end"));
  menu_entries.push_back (lay::menu_item ("ant::clear_all_rulers", "clear_all_rulers:edit", "edit_menu.end", tl::to_string (tr ("Clear All Rulers And Annotations(Ctrl+K)"))));
  menu_entries.push_back (lay::menu_item ("ant::configure", "configure_rulers", "edit_menu.end", tl::to_string (tr ("Ruler And Annotation Setup"))));
}

lay::Plugin *
PluginDeclaration::create_plugin (db::Manager *manager, lay::Dispatcher *, lay::LayoutViewBase *view) const
{
  return new ant::Service (manager, view);
}

bool 
PluginDeclaration::menu_activated (const std::string &symbol) const
{
  if (symbol == "ant::configure") {

#if defined(HAVE_QT)
    lay::ConfigurationDialog config_dialog (QApplication::activeWindow (), lay::Dispatcher::instance (), "ant::Plugin");
    config_dialog.exec ();
#endif

    return true;

  } else {
    return lay::PluginDeclaration::menu_activated (symbol);
  }
}

bool 
PluginDeclaration::implements_editable (std::string &title) const
{
  title = tl::to_string (tr ("Rulers And Annotations"));
  return true;
}

bool 
PluginDeclaration::implements_mouse_mode (std::string &title) const
{
  title = "ruler:ruler_mode_group:ruler_templates_group\t" + tl::to_string (tr ("Ruler{Add rulers and annotations}")) + "<:ruler_24px.png>";
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

  } else if (m_current_template_updated) {

    update_current_template ();

  }
}

void 
PluginDeclaration::initialized (lay::Dispatcher *root)
{
  //  Check if we already have templates (initial setup)
  //  NOTE: this is not done by using a default value for the configuration item but dynamically.
  //  This provides a migration path from earlier versions (not having templates) to recent ones.
  std::map<std::string, const ant::Template *> cat_names;
  for (auto i = m_templates.begin (); i != m_templates.end (); ++i) {
    if (i->category ().find ("_") == 0) {
      cat_names.insert (std::make_pair (i->category (), i.operator-> ()));
    }
  }

  bool any_missing = false;
  auto std_templates = make_standard_templates ();
  for (auto t = std_templates.begin (); ! any_missing && t != std_templates.end (); ++t) {
    if (! t->category ().empty () &&
        (cat_names.find (t->category ()) == cat_names.end () || cat_names.find (t->category ())->second->version () != ant::Template::current_version ())) {
      any_missing = true;
    }
  }

  if (cat_names.empty ()) {

    //  full initial configuration
    if (tl::verbosity () >= 20) {
      tl::info << "Resetting annotation templates";
    }
    root->config_set (cfg_ruler_templates, ant::TemplatesConverter ().to_string (make_standard_templates ()));
    root->config_end ();

  } else if (any_missing) {

    //  some standard templates are missing - add them now (migration path for later versions)
    decltype (m_templates) new_templates;
    for (auto t = std_templates.begin (); t != std_templates.end (); ++t) {
      if (! t->category ().empty ()) {
        auto tt = cat_names.find (t->category ());
        if (tt != cat_names.end () && tt->second->version () == ant::Template::current_version ()) {
          new_templates.push_back (*tt->second);
        } else {
          if (tl::verbosity () >= 20) {
            tl::info << "Resetting annotation template: " << t->title ();
          }
          new_templates.push_back (*t);
        }
      }
    }
    for (auto i = m_templates.begin (); i != m_templates.end (); ++i) {
      if (i->category ().empty ()) {
        new_templates.push_back (*i);
      }
    }

    //  upgrade
    for (auto i = new_templates.begin (); i != new_templates.end (); ++i) {
      i->version (ant::Template::current_version ());
    }

    root->config_set (cfg_ruler_templates, ant::TemplatesConverter ().to_string (new_templates));
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

  std::vector<std::string> menu_entries = mp->menu ()->group ("ruler_mode_group");
  for (std::vector<std::string>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {
    lay::Action *action = mp->menu ()->action (*m);
    if (m_current_template >= 0 && m_current_template < int (m_templates.size ())) {
      action->set_title (m_templates [m_current_template].title ());
    } else {
      action->set_title (std::string ());
    }
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

  m_current_template_updated = false;
}

void 
PluginDeclaration::update_menu ()
{
  lay::Dispatcher *mp = lay::Dispatcher::instance ();
  if (! mp || ! mp->has_ui ()) {
    return;
  }

  std::vector<std::string> menu_entries = mp->menu ()->group ("ruler_mode_group");
  for (std::vector<std::string>::const_iterator m = menu_entries.begin (); m != menu_entries.end (); ++m) {
    lay::Action *action = mp->menu ()->action (*m);
    if (m_current_template >= 0 && m_current_template < int (m_templates.size ())) {
      action->set_title (m_templates [m_current_template].title ());
    } else {
      action->set_title (std::string ());
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

  m_templates_updated = false;
  m_current_template_updated = false;
}

void
PluginDeclaration::register_annotation_template (const ant::Template &t, lay::Plugin *plugin)
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

  if (! plugin) {
    plugin = lay::Dispatcher::instance ();
  }
  if (plugin) {
    plugin->config_set (cfg_ruler_templates, ant::TemplatesConverter ().to_string (m_templates));
    plugin->config_end ();
  }
}

void
PluginDeclaration::unregister_annotation_template (const std::string &category, lay::Plugin *plugin)
{
  std::vector<ant::Template> tpl;

  if (! category.empty ()) {
    for (auto i = m_templates.begin (); i != m_templates.end (); ++i) {
      if (i->category () != category) {
        tpl.push_back (*i);
      }
    }
  }

  m_templates.swap (tpl);

  if (! plugin) {
    plugin = lay::Dispatcher::instance ();
  }
  if (plugin) {
    plugin->config_set (cfg_ruler_templates, ant::TemplatesConverter ().to_string (m_templates));
    plugin->config_end ();
  }
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new ant::PluginDeclaration (), 3000, "ant::Plugin");

}

