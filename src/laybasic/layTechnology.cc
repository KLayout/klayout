
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


#include "layTechnology.h"
#include "layPlugin.h"
#include "layStream.h"

#include <stdio.h>

#include <QFileInfo>
#include <QDir>

namespace lay
{

// -----------------------------------------------------------------------------

Technologies::Technologies ()
{
  m_technologies.push_back (new Technology (std::string (""), "(Default)"));
  m_changed = false;
  m_in_update = false;
}

Technologies::Technologies (const Technologies &other)
  : tl::Object ()
{
  operator= (other);
}

Technologies::~Technologies ()
{
  // .. nothing yet ..
}

Technologies &
Technologies::operator= (const Technologies &other)
{
  if (&other != this) {
    m_technologies = other.m_technologies;
    technologies_changed_event ();
  }

  return *this;
}

lay::Technologies *
Technologies::instance ()
{
  static lay::Technologies s_technologies;
  return &s_technologies;
}

static tl::XMLElementList xml_elements () 
{
  return make_element ((Technologies::const_iterator (Technologies::*) () const) &Technologies::begin, (Technologies::const_iterator (Technologies::*) () const) &Technologies::end, &Technologies::add, "technology",
    Technology::xml_elements ()
  );
}

std::string 
Technologies::to_xml () const
{
  //  create a copy to filter out the ones which are not persisted
  lay::Technologies copy;
  for (const_iterator t = begin (); t != end (); ++t) {
    if (t->is_persisted ()) {
      copy.add (new Technology (*t));
    }
  }

  tl::OutputStringStream os;
  tl::XMLStruct<lay::Technologies> xml_struct ("technologies", xml_elements ());
  tl::OutputStream oss (os);
  xml_struct.write (oss, copy);
  return os.string ();
}

void 
Technologies::load_from_xml (const std::string &s)
{
  //  create a copy to filter out the ones which are not persisted and remain
  lay::Technologies copy;
  for (const_iterator t = begin (); t != end (); ++t) {
    if (! t->is_persisted ()) {
      copy.add (new Technology (*t));
    }
  }

  tl::XMLStringSource source (s);
  tl::XMLStruct<lay::Technologies> xml_struct ("technologies", xml_elements ());
  xml_struct.parse (source, copy);

  *this = copy;
}

void 
Technologies::add (Technology *technology)
{
  for (tl::stable_vector<Technology>::iterator t = m_technologies.begin (); technology && t != m_technologies.end (); ++t) {
    if (t->name () == technology->name ()) {
      *t = *technology;
      delete technology;
      technology = 0;
    }
  }

  if (technology) {
    m_technologies.push_back (technology);
    technology->technology_changed_with_sender_event.add (this, &Technologies::technology_changed);
  }

  technologies_changed ();
}

void 
Technologies::remove (const std::string &name)
{
  for (tl::stable_vector<Technology>::iterator t = m_technologies.begin (); t != m_technologies.end (); ++t) {
    if (t->name () == name) {
      m_technologies.erase (t);
      technologies_changed ();
      break;
    }
  }
}

void
Technologies::clear ()
{
  if (! m_technologies.empty ()) {
    m_technologies.clear ();
    technologies_changed ();
  }
}

void
Technologies::technologies_changed ()
{
  if (m_in_update) {
    m_changed = true;
  } else {
    technologies_changed_event ();
  }
}

void
Technologies::begin_updates ()
{
  tl_assert (! m_in_update);
  m_in_update = true;
  m_changed = false;
}

void
Technologies::end_updates ()
{
  if (m_in_update) {
    m_in_update = false;
    if (m_changed) {
      m_changed = false;
      technologies_changed ();
    }
  }
}

void
Technologies::end_updates_no_event ()
{
  m_in_update = false;
  m_changed = false;
}

bool 
Technologies::has_technology (const std::string &name) const
{
  for (tl::stable_vector<Technology>::const_iterator t = m_technologies.begin (); t != m_technologies.end (); ++t) {
    if (t->name () == name) {
      return true;
    }
  }

  return false;
}

Technology *
Technologies::technology_by_name (const std::string &name) 
{
  for (tl::stable_vector<Technology>::iterator t = m_technologies.begin (); t != m_technologies.end (); ++t) {
    if (t->name () == name) {
      return &*t;
    }
  }

  return &*m_technologies.begin ();
}

// -----------------------------------------------------------------------------
//  Technology implementation

Technology::Technology ()
  : m_name (), m_description (), m_dbu (0.001), m_persisted (true), m_readonly (false)
{
  init ();
}

Technology::Technology (const std::string &name, const std::string &description)
  : m_name (name), m_description (description), m_dbu (0.001), m_persisted (true), m_readonly (false)
{
  init ();
}

void 
Technology::init ()
{
  m_add_other_layers = true;

  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    if (cls->technology_component_provider () != 0) {
      m_components.push_back (cls->technology_component_provider ()->create_component ());
    }
  }
}

Technology::~Technology ()
{
  for (std::vector <TechnologyComponent *>::const_iterator c = m_components.begin (); c != m_components.end (); ++c) {
    delete *c;
  }
  m_components.clear ();
}

Technology::Technology (const Technology &d)
  : tl::Object (),
    m_name (d.m_name), m_description (d.m_description), m_dbu (d.m_dbu),
    m_explicit_base_path (d.m_explicit_base_path), m_default_base_path (d.m_default_base_path),
    m_load_layout_options (d.m_load_layout_options),
    m_save_layout_options (d.m_save_layout_options),
    m_lyp_path (d.m_lyp_path), m_add_other_layers (d.m_add_other_layers), m_persisted (d.m_persisted),
    m_readonly (d.m_readonly), m_lyt_file (d.m_lyt_file)
{
  for (std::vector <TechnologyComponent *>::const_iterator c = d.m_components.begin (); c != d.m_components.end (); ++c) {
    m_components.push_back ((*c)->clone ());
  }
}

Technology &Technology::operator= (const Technology &d)
{
  if (this != &d) {

    m_name = d.m_name;
    m_description = d.m_description;
    m_dbu = d.m_dbu;
    m_default_base_path = d.m_default_base_path;
    m_explicit_base_path = d.m_explicit_base_path;
    m_load_layout_options = d.m_load_layout_options;
    m_save_layout_options = d.m_save_layout_options;
    m_lyp_path = d.m_lyp_path;
    m_add_other_layers = d.m_add_other_layers;
    m_persisted = d.m_persisted;
    m_readonly = d.m_readonly;
    m_lyt_file = d.m_lyt_file;

    for (std::vector <TechnologyComponent *>::const_iterator c = m_components.begin (); c != m_components.end (); ++c) {
      delete *c;
    }
    m_components.clear ();

    for (std::vector <TechnologyComponent *>::const_iterator c = d.m_components.begin (); c != d.m_components.end (); ++c) {
      m_components.push_back ((*c)->clone ());
    }

    technology_changed ();

  }

  return *this;
}

tl::XMLElementList 
Technology::xml_elements () 
{
  tl::XMLElementList elements = 
         tl::make_member (&Technology::name, &Technology::set_name, "name") + 
         tl::make_member (&Technology::description, &Technology::set_description, "description") + 
         tl::make_member (&Technology::dbu, &Technology::set_dbu, "dbu") +
         tl::make_member (&Technology::explicit_base_path, &Technology::set_explicit_base_path, "base-path") +
         tl::make_member (&Technology::default_base_path, &Technology::set_default_base_path, "original-base-path") +
         tl::make_member (&Technology::layer_properties_file, &Technology::set_layer_properties_file, "layer-properties_file") +
         tl::make_member (&Technology::add_other_layers, &Technology::set_add_other_layers, "add-other-layers") +
         tl::make_element (&Technology::load_layout_options, &Technology::set_load_layout_options, "reader-options",
           lay::load_options_xml_element_list ()
         ) +
         tl::make_element (&Technology::save_layout_options, &Technology::set_save_layout_options, "writer-options",
           lay::save_options_xml_element_list ()
         );

  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    if (cls->technology_component_provider () != 0) {
      elements.append (cls->technology_component_provider ()->xml_element ());
    }
  }

  // ignore all unknown elements
  elements.append (tl::make_member<Technology> ("*")); 

  return elements;
}

const TechnologyComponent *
Technology::component_by_name (const std::string &component_name) const
{
  for (std::vector <TechnologyComponent *>::const_iterator c = m_components.begin (); c != m_components.end (); ++c) {
    if ((*c)->name () == component_name) {
      return *c;
    }
  }

  return 0;
}

TechnologyComponent *
Technology::component_by_name (const std::string &component_name)
{
  for (std::vector <TechnologyComponent *>::const_iterator c = m_components.begin (); c != m_components.end (); ++c) {
    if ((*c)->name () == component_name) {
      return *c;
    }
  }

  return 0;
}

std::vector <std::string>
Technology::component_names () const
{
  std::vector <std::string> names;
  for (std::vector <TechnologyComponent *>::const_iterator c = m_components.begin (); c != m_components.end (); ++c) {
    names.push_back ((*c)->name ());
  }
  return names;
}

void
Technology::set_component (TechnologyComponent *component)
{
  for (std::vector <TechnologyComponent *>::iterator c = m_components.begin (); c != m_components.end (); ++c) {
    if ((*c)->name () == component->name ()) {
      if (*c != component) {
        delete *c;
        *c = component;
        technology_changed_event ();
        technology_changed_with_sender_event (this);
      }
      break;
    }
  }
}

std::string 
Technology::correct_path (const std::string &fp) const
{
  if (base_path ().empty ()) {
    return fp;
  }

  QString rfp = QDir (tl::to_qstring (base_path ())).relativeFilePath (tl::to_qstring (fp));
  if (rfp.startsWith (QString::fromUtf8 (".."))) {
    //  upwards or beside - don't correct:
    return fp;
  } else {
    return tl::to_string (rfp);
  }
}

void 
Technology::load (const std::string &fn)
{
  tl::XMLFileSource source (fn);
  tl::XMLStruct<lay::Technology> xml_struct ("technology", xml_elements ());
  xml_struct.parse (source, *this);

  //  use the tech file's path as the default base path
  std::string lyt_file = tl::to_string (QFileInfo (tl::to_qstring (fn)).absoluteDir ().path ());
  set_default_base_path (lyt_file);

  set_tech_file_path (fn);
}

void
Technology::save (const std::string &fn) const
{
  tl::XMLStruct<lay::Technology> xml_struct ("technology", xml_elements ());
  tl::OutputStream os (fn, tl::OutputStream::OM_Plain);
  xml_struct.write (os, *this);
}

std::string 
Technology::build_effective_path (const std::string &p) const
{
  if (p.empty () || base_path ().empty ()) {
    return p;
  }

  QFileInfo f (tl::to_qstring (p));
  if (f.isAbsolute ()) {
    return p;
  } else {
    return tl::to_string (QDir (tl::to_qstring (base_path ())).filePath (tl::to_qstring (p)));
  }
}

}

