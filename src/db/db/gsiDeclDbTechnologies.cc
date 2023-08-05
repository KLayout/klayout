
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

#include "gsiDecl.h"
#include "dbTechnology.h"

#include "tlXMLWriter.h"
#include "tlXMLParser.h"

namespace gsi
{

static std::vector<std::string> technology_names ()
{
  std::vector<std::string> names;
  for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
    names.push_back (t->name ());
  }
  return names;
}

static db::Technology *technology_by_name (const std::string &name)
{
  return db::Technologies::instance ()->technology_by_name (name);
}

static db::Technology *create_technology (const std::string &name)
{
  db::Technology *tech = new db::Technology ();
  tech->set_name (name);
  db::Technologies::instance ()->add_new (tech);
  return tech;
}

static void remove_technology (const std::string &name)
{
  db::Technologies::instance ()->remove (name);
}

static bool has_technology (const std::string &name)
{
  return db::Technologies::instance ()->has_technology (name);
}

static std::string technologies_to_xml ()
{
  return db::Technologies::instance ()->to_xml ();
}

static void technologies_from_xml (const std::string &s)
{
  db::Technologies::instance ()->load_from_xml (s);
}

static void clear_technologies ()
{
  db::Technologies::instance ()->clear ();
}

static db::Technology technology_from_xml (const std::string &s)
{
  db::Technology tech;
  tl::XMLStringSource source (s);
  tl::XMLStruct<db::Technology> xml_struct ("technology", db::Technology::xml_elements ());
  xml_struct.parse (source, tech);
  return tech;
}

static std::string technology_to_xml (const db::Technology *tech)
{
  if (! tech) {
    return std::string ();
  } else {
    tl::OutputStringStream os;
    tl::XMLStruct<db::Technology> xml_struct ("technology", db::Technology::xml_elements ());
    tl::OutputStream oss (os);
    xml_struct.write (oss, *tech);
    return os.string ();
  }
}

static db::TechnologyComponent *get_component (db::Technology *tech, const std::string &name)
{
  return tech->component_by_name (name);
}

static std::vector<std::string> get_component_names (const db::Technology *tech)
{
  return tech->component_names ();
}

gsi::Class<db::TechnologyComponent> technology_component_decl ("db", "TechnologyComponent",
  gsi::method ("name", &db::TechnologyComponent::name,
    "@brief Gets the formal name of the technology component\n"
    "This is the name by which the component can be obtained from a technology using "
    "\\Technology#component."
  ) +
  gsi::method ("description", &db::TechnologyComponent::description,
    "@brief Gets the human-readable description string of the technology component\n"
  ),
  "@brief A part of a technology definition\n"
  "Technology components extend technology definitions (class \\Technology) by "
  "specialized subfeature definitions. For example, the net tracer supplies "
  "its technology-dependent specification through a technology component called "
  "\\NetTracerTechnology.\n"
  "\n"
  "Components are managed within technologies and can be accessed from a technology "
  "using \\Technology#component.\n"
  "\n"
  "This class has been introduced in version 0.25."
);

DB_PUBLIC gsi::Class<db::TechnologyComponent> &decl_dbTechnologyComponent () { return technology_component_decl; }

gsi::Class<db::Technology> technology_decl ("db", "Technology",
  gsi::method ("name", &db::Technology::name,
    "@brief Gets the name of the technology"
  ) +
  gsi::method ("name=", &db::Technology::set_name, gsi::arg ("name"),
    "@brief Sets the name of the technology"
  ) +
  gsi::method ("base_path", &db::Technology::base_path,
    "@brief Gets the base path of the technology\n"
    "\n"
    "The base path is the effective path where files are read from if their "
    "file path is a relative one. If the explicit path is set (see \\explicit_base_path=), it is\n"
    "used. If not, the default path is used. The default path is the one from which\n"
    "a technology file was imported. The explicit one is the one that is specified\n"
    "explicitly with \\explicit_base_path=.\n"
  ) +
  gsi::method ("default_base_path", &db::Technology::default_base_path,
    "@brief Gets the default base path\n"
    "\n"
    "See \\base_path for details about the default base path.\n"
  ) +
  gsi::method ("default_base_path=", &db::Technology::set_default_base_path, gsi::arg ("path"),
    "@hide\n" // only for testing
  ) +
  gsi::method ("correct_path", &db::Technology::correct_path, gsi::arg ("path"),
    "@brief Makes a file path relative to the base path if one is specified\n"
    "\n"
    "This method turns an absolute path into one relative to the base path. "
    "Only files below the base path will be made relative. Files above or beside "
    "won't be made relative.\n"
    "\n"
    "See \\base_path for details about the default base path.\n"
  ) +
  gsi::method ("eff_path", &db::Technology::build_effective_path, gsi::arg ("path"),
    "@brief Makes a file path relative to the base path if one is specified\n"
    "\n"
    "This method will return the actual path for a file from the file's path. "
    "If the input path is a relative one, it will be made absolute by using the "
    "base path.\n"
    "\n"
    "See \\base_path for details about the default base path.\n"
  ) +
  gsi::method ("explicit_base_path", &db::Technology::explicit_base_path,
    "@brief Gets the explicit base path\n"
    "\n"
    "See \\base_path for details about the explicit base path.\n"
  ) +
  gsi::method ("explicit_base_path=", &db::Technology::set_explicit_base_path, gsi::arg ("path"),
    "@brief Sets the explicit base path\n"
    "\n"
    "See \\base_path for details about the explicit base path.\n"
  ) +
  gsi::method ("description", &db::Technology::description,
    "@brief Gets the description\n"
    "\n"
    "The technology description is shown to the user in technology selection dialogs and for "
    "display purposes."
  ) +
  gsi::method ("description=", &db::Technology::set_description, gsi::arg ("description"),
    "@brief Sets the description\n"
  ) +
  gsi::method ("group", &db::Technology::group,
    "@brief Gets the technology group\n"
    "\n"
    "The technology group is used to group certain technologies together in the technology selection menu. "
    "Technologies with the same group are put under a submenu with that group title.\n"
    "\n"
    "The 'group' attribute has been introduced in version 0.26.2.\n"
  ) +
  gsi::method ("group=", &db::Technology::set_group, gsi::arg ("group"),
    "@brief Sets the technology group\n"
    "See \\group for details about this attribute.\n"
    "\n"
    "The 'group' attribute has been introduced in version 0.26.2.\n"
  ) +
  gsi::method ("dbu", &db::Technology::dbu,
    "@brief Gets the default database unit\n"
    "\n"
    "The default database unit is the one used when creating a layout for example."
  ) +
  gsi::method ("dbu=", &db::Technology::set_dbu, gsi::arg ("dbu"),
    "@brief Sets the default database unit\n"
  ) +
  gsi::method ("layer_properties_file", &db::Technology::layer_properties_file,
    "@brief Gets the path of the layer properties file\n"
    "\n"
    "If empty, no layer properties file is associated with the technology. "
    "If non-empty, this path will be corrected by the base path (see \\correct_path) and "
    "this layer properties file will be loaded for layouts with this technology."
  ) +
  gsi::method ("layer_properties_file=", &db::Technology::set_layer_properties_file, gsi::arg ("file"),
    "@brief Sets the path of the layer properties file\n"
    "\n"
    "See \\layer_properties_file for details about this property."
  ) +
  gsi::method ("eff_layer_properties_file", &db::Technology::eff_layer_properties_file,
    "@brief Gets the effective path of the layer properties file\n"
  ) +
  gsi::method ("add_other_layers?", &db::Technology::add_other_layers,
    "@brief Gets the flag indicating whether to add other layers to the layer properties\n"
  ) +
  gsi::method ("add_other_layers=", &db::Technology::set_add_other_layers, gsi::arg ("add"),
    "@brief Sets the flag indicating whether to add other layers to the layer properties\n"
  ) +
  gsi::method ("load_layout_options", &db::Technology::load_layout_options,
    "@brief Gets the layout reader options\n"
    "\n"
    "This method returns the layout reader options that are used when reading layouts "
    "with this technology.\n"
    "\n"
    "Change the reader options by modifying the object and using the setter to change it:\n"
    "\n"
    "@code\n"
    "opt = tech.load_layout_options\n"
    "opt.dxf_dbu = 2.5\n"
    "tech.load_layout_options = opt\n"
    "@/code\n"
  ) +
  gsi::method ("load_layout_options=", &db::Technology::set_load_layout_options, gsi::arg ("options"),
    "@brief Sets the layout reader options\n"
    "\n"
    "See \\load_layout_options for a description of this property.\n"
  ) +
  gsi::method ("save_layout_options", &db::Technology::save_layout_options,
    "@brief Gets the layout writer options\n"
    "\n"
    "This method returns the layout writer options that are used when writing layouts "
    "with this technology.\n"
    "\n"
    "Change the reader options by modifying the object and using the setter to change it:\n"
    "\n"
    "@code\n"
    "opt = tech.save_layout_options\n"
    "opt.dbu = 0.01\n"
    "tech.save_layout_options = opt\n"
    "@/code\n"
  ) +
  gsi::method ("save_layout_options=", &db::Technology::set_save_layout_options, gsi::arg ("options"),
    "@brief Sets the layout writer options\n"
    "\n"
    "See \\save_layout_options for a description of this property.\n"
  ) +
  gsi::method ("load", &db::Technology::load, gsi::arg ("file"),
    "@brief Loads the technology definition from a file\n"
  ) +
  gsi::method ("save", &db::Technology::save, gsi::arg ("file"),
    "@brief Saves the technology definition to a file\n"
  ) +
  gsi::method ("technology_names", &technology_names,
    "@brief Gets a list of technology names defined in the system\n"
  ) +
  gsi::method ("technology_by_name", &technology_by_name, gsi::arg ("name"),
    "@brief Gets the technology object for a given name\n"
  ) +
  gsi::method ("has_technology?", &has_technology, gsi::arg ("name"),
    "@brief Returns a value indicating whether there is a technology with this name\n"
  ) +
  gsi::method ("create_technology", &create_technology, gsi::arg ("name"),
    "@brief Creates a new (empty) technology with the given name\n"
    "\n"
    "This method returns a reference to the new technology."
  ) +
  gsi::method ("remove_technology", &remove_technology, gsi::arg ("name"),
    "@brief Removes the technology with the given name\n"
  ) +
  gsi::method ("technologies_to_xml", &technologies_to_xml,
    "@brief Returns a XML representation of all technologies registered in the system\n"
    "\n"
    "\\technologies_from_xml can be used to restore the technology definitions. "
    "This method is provided mainly as a substitute for the pre-0.25 way of accessing "
    "technology data through the 'technology-data' configuration parameter. This method "
    "will return the equivalent string."
  ) +
  gsi::method_ext ("to_xml", &technology_to_xml,
    "@brief Returns a XML representation of this technolog\n"
    "\n"
    "\\technology_from_xml can be used to restore the technology definition."
  ) +
  gsi::method ("clear_technologies", &clear_technologies,
    "@brief Clears all technologies\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method ("technologies_from_xml", &technologies_from_xml, gsi::arg ("xml"),
    "@brief Loads the technologies from a XML representation\n"
    "\n"
    "See \\technologies_to_xml for details. This method is the corresponding setter."
  ) +
  gsi::method ("technology_from_xml", &technology_from_xml, gsi::arg ("xml"),
    "@brief Loads the technology from a XML representation\n"
    "\n"
    "See \\technology_to_xml for details."
  ) +
  gsi::method_ext ("component_names", &get_component_names,
    "@brief Gets the names of all components available for \\component"
  ) +
  gsi::method_ext ("component", &get_component, gsi::arg ("name"),
    "@brief Gets the technology component with the given name\n"
    "The names are unique system identifiers. For all names, use \\component_names."
  ),
  "@brief Represents a technology\n"
  "\n"
  "This class represents one technology from a set of technologies. The set of technologies "
  "available in the system can be obtained with \\technology_names. Individual technology "
  "definitions are returned with \\technology_by_name. Use \\create_technology to register "
  "new technologies and \\remove_technology to delete technologies.\n"
  "\n"
  "The Technology class has been introduced in version 0.25.\n"
);

}
