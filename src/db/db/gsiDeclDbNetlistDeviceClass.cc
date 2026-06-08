
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

#include "gsiDecl.h"
#include "dbNetlist.h"

namespace gsi
{

static db::DeviceTerminalDefinition *new_terminal_definition (const std::string &name, const std::string &description)
{
  return new db::DeviceTerminalDefinition (name, description);
}

Class<db::DeviceTerminalDefinition> decl_dbDeviceTerminalDefinition ("db", "DeviceTerminalDefinition",
  gsi::constructor ("new", &gsi::new_terminal_definition, gsi::arg ("name"), gsi::arg ("description", std::string ()),
    "@brief Creates a new terminal definition."
  ) +
  gsi::method ("name", &db::DeviceTerminalDefinition::name,
    "@brief Gets the name of the terminal."
  ) +
  gsi::method ("name=", &db::DeviceTerminalDefinition::set_name, gsi::arg ("name"),
    "@brief Sets the name of the terminal."
  ) +
  gsi::method ("description", &db::DeviceTerminalDefinition::description,
    "@brief Gets the description of the terminal."
  ) +
  gsi::method ("description=", &db::DeviceTerminalDefinition::set_description, gsi::arg ("description"),
    "@brief Sets the description of the terminal."
  ) +
  gsi::method ("id", &db::DeviceTerminalDefinition::id,
    "@brief Gets the ID of the terminal.\n"
    "The ID of the terminal is used in some places to refer to a specific terminal (e.g. in "
    "the \\NetTerminalRef object)."
  ),
  "@brief A terminal descriptor\n"
  "This class is used inside the \\DeviceClass class to describe a terminal of the device.\n"
  "\n"
  "This class has been added in version 0.26."
);

static db::DeviceParameterDefinition *new_parameter_definition (const std::string &name, const std::string &description, double default_value, bool is_primary, double si_scaling, double geo_scaling_exponent)
{
  return new db::DeviceParameterDefinition (name, description, default_value, is_primary, si_scaling, geo_scaling_exponent);
}

Class<db::DeviceParameterDefinition> decl_dbDeviceParameterDefinition ("db", "DeviceParameterDefinition",
  gsi::constructor ("new", &gsi::new_parameter_definition, gsi::arg ("name"), gsi::arg ("description", std::string ()), gsi::arg ("default_value", 0.0), gsi::arg ("is_primary", true), gsi::arg ("si_scaling", 1.0), gsi::arg ("geo_scaling_exponent", 0.0),
    "@brief Creates a new parameter definition.\n"
    "@param name The name of the parameter\n"
    "@param description The human-readable description\n"
    "@param default_value The initial value\n"
    "@param is_primary True, if the parameter is a primary parameter (see \\is_primary=)\n"
    "@param si_scaling The scaling factor to SI units\n"
    "@param geo_scaling_exponent Indicates how the parameter scales with geometrical scaling (0: no scaling, 1.0: linear, 2.0: quadratic)\n"
  ) +
  gsi::method ("name", &db::DeviceParameterDefinition::name,
    "@brief Gets the name of the parameter."
  ) +
  gsi::method ("name=", &db::DeviceParameterDefinition::set_name, gsi::arg ("name"),
    "@brief Sets the name of the parameter."
  ) +
  gsi::method ("description", &db::DeviceParameterDefinition::description,
    "@brief Gets the description of the parameter."
  ) +
  gsi::method ("description=", &db::DeviceParameterDefinition::set_description, gsi::arg ("description"),
    "@brief Sets the description of the parameter."
  ) +
  gsi::method ("default_value", &db::DeviceParameterDefinition::default_value,
    "@brief Gets the default value of the parameter."
  ) +
  gsi::method ("default_value=", &db::DeviceParameterDefinition::set_default_value, gsi::arg ("default_value"),
    "@brief Sets the default value of the parameter.\n"
    "The default value is used to initialize parameters of \\Device objects."
  ) +
  gsi::method ("is_primary?", &db::DeviceParameterDefinition::is_primary,
    "@brief Gets a value indicating whether the parameter is a primary parameter\n"
    "See \\is_primary= for details about this predicate."
  ) +
  gsi::method ("is_primary=", &db::DeviceParameterDefinition::set_is_primary, gsi::arg ("primary"),
    "@brief Sets a value indicating whether the parameter is a primary parameter\n"
    "If this flag is set to true (the default), the parameter is considered a primary parameter.\n"
    "Only primary parameters are compared by default.\n"
  ) +
  gsi::method ("si_scaling", &db::DeviceParameterDefinition::si_scaling,
    "@brief Gets the scaling factor to SI units.\n"
    "For parameters in micrometers - for example W and L of MOS devices - this factor can be set to 1e-6 to reflect "
    "the unit. SI unit scaling is only applied to parameters of the 'float' type. It is not applied to integer-type "
    "or string parameters."
  ) +
  gsi::method ("si_scaling=", &db::DeviceParameterDefinition::set_si_scaling, gsi::arg ("flag"),
    "@brief Sets the scaling factor to SI units.\n"
    "See \\si_scaling for details.\n"
    "\n"
    "This setter has been added in version 0.28.6."
  ) +
  gsi::method ("geo_scaling_exponent", &db::DeviceParameterDefinition::geo_scaling_exponent,
    "@brief Gets the geometry scaling exponent.\n"
    "This value is used when applying '.options scale' in the SPICE reader for example. "
    "It is zero for 'no scaling', 1.0 for linear scaling and 2.0 for quadratic scaling.\n"
    "Geometric scaling is only applied to parameters of the 'float' type. It is not applied to integer-type "
    "or string parameters.\n"
    "\n"
    "This attribute has been added in version 0.28.6."
  ) +
  gsi::method ("geo_scaling_exponent=", &db::DeviceParameterDefinition::set_geo_scaling_exponent, gsi::arg ("expo"),
    "@brief Sets the geometry scaling exponent.\n"
    "See \\geo_scaling_exponent for details.\n"
    "\n"
    "This attribute has been added in version 0.28.6."
  ) +
  gsi::method ("id", &db::DeviceParameterDefinition::id,
    "@brief Gets the ID of the parameter.\n"
    "The ID of the parameter is used in some places to refer to a specific parameter (e.g. in "
    "the \\NetParameterRef object)."
  ),
  "@brief A parameter descriptor\n"
  "This class is used inside the \\DeviceClass class to describe a parameter of the device.\n"
  "\n"
  "This class has been added in version 0.26."
);

namespace
{

/**
 *  @brief A DeviceParameterCompare implementation that allows reimplementation of the virtual methods
 */
class GenericDeviceParameterCompare
  : public db::EqualDeviceParameters
{
public:
  GenericDeviceParameterCompare ()
    : db::EqualDeviceParameters ()
  {
    //  .. nothing yet ..
  }

  virtual bool less (const db::Device &a, const db::Device &b) const
  {
    if (cb_less.can_issue ()) {
      return cb_less.issue<db::EqualDeviceParameters, bool, const db::Device &, const db::Device &> (&db::EqualDeviceParameters::less, a, b);
    } else {
      return db::EqualDeviceParameters::less (a, b);
    }
  }

  gsi::Callback cb_less;
};

/**
 *  @brief A DeviceCombiner implementation that allows reimplementation of the virtual methods
 */
class GenericDeviceCombiner
  : public db::DeviceCombiner
{
public:
  GenericDeviceCombiner ()
    : db::DeviceCombiner ()
  {
    //  .. nothing yet ..
  }

  virtual bool combine_devices (db::Device *a, db::Device *b) const
  {
    if (cb_combine.can_issue ()) {
      return cb_combine.issue<db::DeviceCombiner, bool, db::Device *, db::Device *> (&db::DeviceCombiner::combine_devices, a, b);
    } else {
      return false;
    }
  }

  gsi::Callback cb_combine;
};

}

db::EqualDeviceParameters *make_equal_dp (size_t param_id, double absolute, double relative)
{
  return new db::EqualDeviceParameters (param_id, absolute, relative);
}

db::EqualDeviceParameters *make_ignore_dp (size_t param_id)
{
  return new db::EqualDeviceParameters (param_id, true);
}

Class<db::EqualDeviceParameters> decl_dbEqualDeviceParameters ("db", "EqualDeviceParameters",
  gsi::constructor ("new", &make_equal_dp, gsi::arg ("param_id"), gsi::arg ("absolute", 0.0), gsi::arg ("relative", 0.0),
    "@brief Creates a device parameter comparer for a single parameter.\n"
    "'absolute' is the absolute deviation allowed for the parameter values. "
    "'relative' is the relative deviation allowed for the parameter values (a value between 0 and 1).\n"
    "\n"
    "A value of 0 for both absolute and relative deviation means the parameters have to match exactly.\n"
    "\n"
    "If 'absolute' and 'relative' are both given, their deviations will add to the allowed difference between "
    "two parameter values. The relative deviation will be applied to the mean value of both parameter values. "
    "For example, when comparing parameter values of 40 and 60, a relative deviation of 0.35 means an absolute "
    "deviation of 17.5 (= 0.35 * average of 40 and 60) which does not make both values match."
  ) +
  gsi::constructor ("ignore", &make_ignore_dp, gsi::arg ("param_id"),
    "@brief Creates a device parameter comparer which ignores the parameter.\n"
    "\n"
    "This specification can be used to make a parameter ignored. Starting with version 0.27.4, all primary parameters "
    "are compared. Before 0.27.4, giving a tolerance meant only those parameters are compared. To exclude a primary "
    "parameter from the compare, use the 'ignore' specification for that parameter.\n"
    "\n"
    "This constructor has been introduced in version 0.27.4.\n"
  ) +
  gsi::method ("+", &db::EqualDeviceParameters::operator+, gsi::arg ("other"),
    "@brief Combines two parameters for comparison.\n"
    "The '+' operator will join the parameter comparers and produce one that checks the combined parameters.\n"
  ) +
  gsi::method ("+=", &db::EqualDeviceParameters::operator+, gsi::arg ("other"),
    "@brief Combines two parameters for comparison (in-place).\n"
    "The '+=' operator will join the parameter comparers and produce one that checks the combined parameters.\n"
  ) +
  gsi::method ("to_string", &db::EqualDeviceParameters::to_string, "@hide"),
  "@brief A device parameter equality comparer.\n"
  "Attach this object to a device class with \\DeviceClass#equal_parameters= to make the device "
  "class use this comparer:\n"
  "\n"
  "@code\n"
  "# 20nm tolerance for length:\n"
  "equal_device_parameters = RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS4Transistor::PARAM_L, 0.02, 0.0)\n"
  "# one percent tolerance for width:\n"
  "equal_device_parameters += RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS4Transistor::PARAM_W, 0.0, 0.01)\n"
  "# applies the compare delegate:\n"
  "netlist.device_class_by_name(\"NMOS\").equal_parameters = equal_device_parameters\n"
  "@/code\n"
  "\n"
  "You can use this class to specify fuzzy equality criteria for the comparison of device parameters in "
  "netlist verification or to confine the equality of devices to certain parameters only.\n"
  "\n"
  "This class has been added in version 0.26."
);

Class<GenericDeviceParameterCompare> decl_GenericDeviceParameterCompare (decl_dbEqualDeviceParameters, "db", "GenericDeviceParameterCompare",
  gsi::callback ("less", &GenericDeviceParameterCompare::less, &GenericDeviceParameterCompare::cb_less, gsi::arg ("device_a"), gsi::arg ("device_b"),
    "@brief Compares the parameters of two devices for a begin less than b.\n"
    "Returns true, if the parameters of device a are considered less than those of device b."
    "The 'less' implementation needs to ensure strict weak ordering. Specifically, less(a,b) == false and less(b,a) implies that a is equal to b and "
    "less(a,b) == true implies that less(b,a) is false and vice versa. If not, an internal error "
    "will be encountered on netlist compare."
  ),
  "@brief A class implementing the comparison of device parameters.\n"
  "Reimplement this class to provide a custom device parameter compare scheme.\n"
  "Attach this object to a device class with \\DeviceClass#equal_parameters= to make the device "
  "class use this comparer.\n"
  "\n"
  "This class is intended for special cases. In most scenarios it is easier to use \\EqualDeviceParameters instead of "
  "implementing a custom comparer class.\n"
  "\n"
  "This class has been added in version 0.26. The 'equal' method has been dropped in 0.27.1 as it can be expressed as !less(a,b) && !less(b,a)."
);

Class<GenericDeviceCombiner> decl_GenericDeviceCombiner ("db", "GenericDeviceCombiner",
  gsi::callback ("combine_devices", &GenericDeviceCombiner::combine_devices, &GenericDeviceCombiner::cb_combine, gsi::arg ("device_a"), gsi::arg ("device_b"),
    "@brief Combines two devices if possible.\n"
    "This method needs to test, whether the two devices can be combined. Both devices "
    "are guaranteed to share the same device class. "
    "If they cannot be combined, this method shall do nothing and return false. "
    "If they can be combined, this method shall reconnect the nets of the first "
    "device and entirely disconnect the nets of the second device. "
    "The second device will be deleted afterwards. "
  ),
  "@brief A class implementing the combination of two devices (parallel or serial mode).\n"
  "Reimplement this class to provide a custom device combiner.\n"
  "Device combination requires 'supports_paralell_combination' or 'supports_serial_combination' to be set "
  "to true for the device class. In the netlist device combination step, the algorithm will try to identify "
  "devices which can be combined into single devices and use the combiner object to implement the actual "
  "joining of such devices.\n"
  "\n"
  "Attach this object to a device class with \\DeviceClass#combiner= to make the device "
  "class use this combiner.\n"
  "\n"
  "This class has been added in version 0.27.3."
);

static const std::string &get_element (const db::DeviceClass::SpiceProfile *profile)
{
  return profile->element;
}

static void set_element (db::DeviceClass::SpiceProfile *profile, const std::string &element)
{
  profile->element = element;
}

static const std::vector<std::string> &get_terminal_order (const db::DeviceClass::SpiceProfile *profile)
{
  return profile->terminal_order;
}

static void set_terminal_order (db::DeviceClass::SpiceProfile *profile, const std::vector<std::string> &terminal_order)
{
  profile->terminal_order = terminal_order;
}

static const std::map<std::string, std::string> &get_incoming_parameters (const db::DeviceClass::SpiceProfile *profile)
{
  return profile->incoming_parameters;
}

static void set_incoming_parameters (db::DeviceClass::SpiceProfile *profile, const std::map<std::string, std::string> &pd)
{
  profile->incoming_parameters = pd;
}

static const std::map<std::string, std::string> &get_outgoing_parameters (const db::DeviceClass::SpiceProfile *profile)
{
  return profile->outgoing_parameters;
}

static void set_outgoing_parameters (db::DeviceClass::SpiceProfile *profile, const std::map<std::string, std::string> &pd)
{
  profile->outgoing_parameters = pd;
}

Class<db::DeviceClass::SpiceProfile> decl_dbDeviceClassSpiceProfile ("db", "DeviceClassSpiceProfile",
  gsi::method_ext ("element", &get_element,
    "@brief Gets the SPICE element to use for this device.\n"
    "The element is the SPICE code for the element - i.e. 'X', 'M', 'R' etc.\n"
  ) +
  gsi::method_ext ("element=", &set_element,
    "@brief Sets the SPICE element to use for this device.\n"
    "See \\element for a description of this attribute.\n"
  ) +
  gsi::method_ext ("terminal_order", &get_terminal_order,
    "@brief Gets the terminal order to use for this device.\n"
    "The terminal order is a list of terminal names that specifies which device terminal to tie to the "
    "nets given in the SPICE statement. The list items must be strings corresponding to valid terminal "
    "names of the device."
  ) +
  gsi::method_ext ("terminal_order=", &set_terminal_order,
    "@brief Sets the terminal order to use for this device.\n"
    "See \\terminal_order for a description of this attribute.\n"
  ) +
  gsi::method_ext ("incoming_parameters", &get_incoming_parameters,
    "@brief Gets the mapping of incoming parameters from SPICE files\n"
    "\n"
    "The key is the parameter name to be produced in the device\n"
    "and the value is a formula that references parameters from the SPICE line.\n"
    "For caseless netlists, the parameter names need to be upper case.\n"
    "\n"
    "Special wildcards can be used for the key:\n"
    "\n"
    "@ul\n"
    "@li \"*\": all pre-defined parameters plus the ones from the SPICE card @/li\n"
    "@li \"**\": all pre-defined parameters @/li\n"
    "@li \"*!\": all pre-defined primary parameters @/li\n"
    "@li \"*?\": all pre-defined secondary parameters @/li\n"
    "@/ul\n"
    "\n"
    "Specific names have precendence over wildcard names.\n"
    "\n"
    "The value is a formula in KLayout expression notation that specifies\n"
    "how the value of the parameter is computed from incoming SPICE line parameters.\n"
    "SPICE parameters are referenced by name. If a parameter is not given,\n"
    "the value will be the default from the parameter declaration or \"nil\".\n"
    "Hence you can implement a default value of 0 for parameter \"P\" using \"P||0.0\".\n"
    "\"_\" is the value of the same SPICE parameter. This is useful for generating\n"
    "catch-all rules, such as '\"*\": \"_\"' (copy all parameters).\n"
    "\n"
    "\"$\" represents the direct value in expressions. This is used for\n"
    "elements like \"R\", \"L\" or \"C\", when the component value is not given\n"
    "as a named parameter, but as an explicit value. This case is used mainly\n"
    "internally, as the value parameter is typically also available as \"R\",\n"
    "\"L\" or \"C\" parameter.\n"
    "\n"
    "If the formula returns a nil value, the parameter is not generated in the\n"
    "device or the default value is used if the parameter is a declared one.\n"
    "An empty expression string is equal to \"nil\", so you can drop a parameter\n"
    "by mapping the name to an empty string, e.g. '\"M\": \"\"' (drops \"M\").\n"
  ) +
  gsi::method_ext ("incoming_parameters=", &set_incoming_parameters,
    "@brief Sets the mapping of incoming parameters from SPICE files\n"
    "See \\incoming_parameters for a description of this attribute.\n"
  ) +
  gsi::method_ext ("outgoing_parameters", &get_outgoing_parameters,
    "@brief Gets the mapping tables for outgoing parameters\n"
    "\n"
    "This attribute is a dictionary. "
    "The keys are the parameter name produced in the SPICE file\n"
    "and the value is a formula that references parameters from the\n"
    "device.\n"
    "For caseless netlists, the parameter names need to be upper case.\n"
    "\n"
    "Special directives apply for the key:\n"
    "\n"
    "@ul\n"
    "@li \"*\" or \"**\": all parameters from the device @/li\n"
    "@li \"*!\": all primary parameters @/li\n"
    "@li \"*?\": all secondary parameters @/li\n"
    "@/ul\n"
    "\n"
    "Specific names have precendence over wildcard names.\n"
    "\n"
    "The value is a formula in KLayout expression notation that specifies\n"
    "how the value of the parameter is computed from device parameters.\n"
    "\"_\" is the value of the same SPICE parameter. This is useful for generating\n"
    "catch-all rules, such as '\"*\": \"_\"' (copy all parameters).\n"
  ) +
  gsi::method_ext ("outgoing_parameters=", &set_outgoing_parameters,
    "@brief Sets the mapping tables for outgoing parameters\n"
    "See \\outgoing_parameters for a description of this attribute.\n"
  ),
  "@brief Declares a SPICE profile of a device\n"
  "Objects of this type are used inside the \\DeviceClass object to specify how the device "
  "is written to and read from SPICE files. See \\DeviceClass#set_spice_profile for a "
  "description of that concept.\n"
  "\n"
  "This class has been added in version 0.31.0."
);

static tl::id_type id_of_device_class (const db::DeviceClass *cls)
{
  return tl::id_of (cls);
}

static void equal_parameters (db::DeviceClass *cls, db::EqualDeviceParameters *comparer)
{
  cls->set_parameter_compare_delegate (comparer);
}

static db::EqualDeviceParameters *get_equal_parameters (db::DeviceClass *cls)
{
  return dynamic_cast<db::EqualDeviceParameters *> (cls->parameter_compare_delegate ());
}

static void set_combiner (db::DeviceClass *cls, GenericDeviceCombiner *combiner)
{
  cls->set_device_combiner (combiner);
}

static GenericDeviceCombiner *get_combiner (db::DeviceClass *cls)
{
  return dynamic_cast<GenericDeviceCombiner *> (cls->device_combiner ());
}

static void enable_parameter (db::DeviceClass *cls, size_t id, bool en)
{
  db::DeviceParameterDefinition *pd = cls->parameter_definition_non_const (id);
  if (pd) {
    pd->set_is_primary (en);
  }
}

static void enable_parameter2 (db::DeviceClass *cls, const std::string &name, bool en)
{
  if (! cls->has_parameter_with_name (name)) {
    return;
  }

  size_t id = cls->parameter_id_for_name (name);
  db::DeviceParameterDefinition *pd = cls->parameter_definition_non_const (id);
  if (pd) {
    pd->set_is_primary (en);
  }
}

static const db::DeviceParameterDefinition *parameter_definition2 (const db::DeviceClass *cls, const std::string &name)
{
  if (! cls->has_parameter_with_name (name)) {
    return 0;
  } else {
    return cls->parameter_definition (cls->parameter_id_for_name (name));
  }
}

static void dc_add_terminal_definition (db::DeviceClass *cls, db::DeviceTerminalDefinition *terminal_def)
{
  if (terminal_def) {
    *terminal_def = cls->add_terminal_definition (*terminal_def);
  }
}

static void dc_add_parameter_definition (db::DeviceClass *cls, db::DeviceParameterDefinition *parameter_def)
{
  if (parameter_def) {
    *parameter_def = cls->add_parameter_definition (*parameter_def);
  }
}

Class<db::DeviceClass> decl_dbDeviceClass ("db", "DeviceClass",
  gsi::method ("name", &db::DeviceClass::name,
    "@brief Gets the name of the device class."
  ) +
  gsi::method ("name=", &db::DeviceClass::set_name, gsi::arg ("name"),
    "@brief Sets the name of the device class."
  ) +
  gsi::method ("has_spice_profile?", &db::DeviceClass::has_spice_profile, gsi::arg ("name"),
    "@brief Gets a value indicating whether a device class supports a specific SPICE profile.\n"
    "See \\set_spice_profile for a description of the SPICE profile concept.\n"
    "\n"
    "SPICE profiles have been introduced in version 0.31.0."
  ) +
  gsi::method ("remove_spice_profile", &db::DeviceClass::remove_spice_profile, gsi::arg ("name"),
    "@brief Removes a SPICE profile with the given name.\n"
    "See \\set_spice_profile for a description of the SPICE profile concept.\n"
    "\n"
    "SPICE profiles have been introduced in version 0.31.0."
  ) +
  gsi::method ("clear_spice_profiles", &db::DeviceClass::clear_spice_profiles,
    "@brief Removes all SPICE profiles from the device class.\n"
    "See \\set_spice_profile for a description of the SPICE profile concept.\n"
    "\n"
    "SPICE profiles have been introduced in version 0.31.0."
  ) +
  gsi::method ("set_spice_profile", &db::DeviceClass::set_spice_profile, gsi::arg ("profile_name"), gsi::arg ("spice_profile"),
    "@brief Defines a SPICE profile.\n"
    "SPICE profiles are a way to declare SPICE representations for a specific device. "
    "Each device class can support multiple profiles. An empty name declares the default profile, "
    "'*' is the fallback profile used when there is no profile with a requested name. Profiles "
    "are requested by the SPICE reader or writer, unless they use delegates to implement "
    "a customized SPICE representation.\n"
    "\n"
    "SPICE profiles can declare element names for example or specify the terminal count and order in SPICE files.\n"
    "See \\DeviceClassSpiceProfile for the available attributes.\n"
    "\n"
    "Note that this method stores a copy of the profile object, so changing at afterwards will not have any effect.\n"
    "\n"
    "SPICE profiles have been introduced in version 0.31.0."
  ) +
  gsi::method ("spice_profile", &db::DeviceClass::spice_profile, gsi::arg ("profile_name"),
    "@brief Gets a SPICE profile with the given name.\n"
    "If the name is not corresponding to a valid profile and a fallback profile '*' exists, the latter is returned. "
    "Otherwise, a default profile is returned, which triggers the default serialization.\n"
    "See \\set_spice_profile for a description of the SPICE profile concept.\n"
    "\n"
    "Note that this method returns a copy of the profile object. If you want to manipulate a SPICE profile, "
    "you have to fetch the current profile, change it and store it again:\n"
    "\n"
    "@code\n"
    "dc = ... # some DeviceClass\n"
    "profile = dc.spice_profile(\"\")  # default profile\n"
    "profile.element = \"X\"\n"
    "dc.set_spice_profile(\"\", profile)\n"
    "@/code\n"
    "\n"
    "SPICE profiles have been introduced in version 0.31.0."
  ) +
  gsi::method ("strict?", &db::DeviceClass::is_strict,
    "@brief Gets a value indicating whether this class performs strict terminal mapping\n"
    "See \\strict= for details about this attribute."
  ) +
  gsi::method ("strict=", &db::DeviceClass::set_strict, gsi::arg ("s"),
    "@brief Sets a value indicating whether this class performs strict terminal mapping\n"
    "\n"
    "Classes with this flag set never allow terminal swapping, even if the device symmetry supports that. "
    "If two classes are involved in a netlist compare,\n"
    "terminal swapping will be disabled if one of the classes is in strict mode.\n"
    "\n"
    "By default, device classes are not strict and terminal swapping is allowed as far as the "
    "device symmetry supports that."
  ) +
  gsi::method ("description", &db::DeviceClass::description,
    "@brief Gets the description text of the device class."
  ) +
  gsi::method ("description=", &db::DeviceClass::set_description, gsi::arg ("description"),
    "@brief Sets the description of the device class."
  ) +
  gsi::method ("netlist", (db::Netlist *(db::DeviceClass::*) ()) &db::DeviceClass::netlist,
    "@brief Gets the netlist the device class lives in."
  ) +
  gsi::method_ext ("id", &gsi::id_of_device_class,
    "@brief Gets the unique ID of the device class\n"
    "The ID is a unique integer that identifies the device class. Use the ID "
    "to check for object identity - i.e. to determine whether two devices share the "
    "same device class."
  ) +
  gsi::method ("terminal_definitions", &db::DeviceClass::terminal_definitions,
    "@brief Gets the list of terminal definitions of the device.\n"
    "See the \\DeviceTerminalDefinition class description for details."
  ) +
  gsi::method ("terminal_definition", &db::DeviceClass::terminal_definition, gsi::arg ("terminal_id"),
    "@brief Gets the terminal definition object for a given ID.\n"
    "Terminal definition IDs are used in some places to reference a specific terminal of a device. "
    "This method obtains the corresponding definition object."
  ) +
  gsi::method ("parameter_definitions", &db::DeviceClass::parameter_definitions,
    "@brief Gets the list of parameter definitions of the device.\n"
    "See the \\DeviceParameterDefinition class description for details."
  ) +
  gsi::method ("parameter_definition", &db::DeviceClass::parameter_definition, gsi::arg ("parameter_id"),
    "@brief Gets the parameter definition object for a given ID.\n"
    "Parameter definition IDs are used in some places to reference a specific parameter of a device. "
    "This method obtains the corresponding definition object."
  ) +
  gsi::method_ext ("parameter_definition", &parameter_definition2, gsi::arg ("parameter_name"),
    "@brief Gets the parameter definition object for a given ID.\n"
    "Parameter definition IDs are used in some places to reference a specific parameter of a device. "
    "This method obtains the corresponding definition object."
    "\n"
    "This version accepts a parameter name.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("enable_parameter", &enable_parameter, gsi::arg ("parameter_id"), gsi::arg ("enable"),
    "@brief Enables or disables a parameter.\n"
    "Some parameters are 'secondary' parameters which are extracted but not handled in device compare and are not shown in the netlist browser. "
    "For example, the 'W' parameter of the resistor is such a secondary parameter. This method allows turning a parameter in a primary one ('enable') or "
    "into a secondary one ('disable').\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("enable_parameter", &enable_parameter2, gsi::arg ("parameter_name"), gsi::arg ("enable"),
    "@brief Enables or disables a parameter.\n"
    "Some parameters are 'secondary' parameters which are extracted but not handled in device compare and are not shown in the netlist browser. "
    "For example, the 'W' parameter of the resistor is such a secondary parameter. This method allows turning a parameter in a primary one ('enable') or "
    "into a secondary one ('disable').\n"
    "\n"
    "This version accepts a parameter name.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method ("has_parameter?", &db::DeviceClass::has_parameter_with_name, gsi::arg ("name"),
    "@brief Returns true, if the device class has a parameter with the given name.\n"
  ) +
  gsi::method ("parameter_id", &db::DeviceClass::parameter_id_for_name, gsi::arg ("name"),
    "@brief Returns the parameter ID of the parameter with the given name.\n"
    "An exception is thrown if there is no parameter with the given name. Use \\has_parameter to check "
    "whether the name is a valid parameter name."
  ) +
  gsi::method ("has_terminal?", &db::DeviceClass::has_terminal_with_name, gsi::arg ("name"),
    "@brief Returns true, if the device class has a terminal with the given name.\n"
  ) +
  gsi::method ("terminal_id", &db::DeviceClass::terminal_id_for_name, gsi::arg ("name"),
    "@brief Returns the terminal ID of the terminal with the given name.\n"
    "An exception is thrown if there is no terminal with the given name. Use \\has_terminal to check "
    "whether the name is a valid terminal name."
  ) +
  gsi::method_ext ("equal_parameters", &get_equal_parameters,
    "@brief Gets the device parameter comparer for netlist verification or nil if no comparer is registered.\n"
    "See \\equal_parameters= for the setter.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method_ext ("equal_parameters=", &equal_parameters, gsi::arg ("comparer"),
    "@brief Specifies a device parameter comparer for netlist verification.\n"
    "By default, all devices are compared with all parameters. If you want to select only certain parameters "
    "for comparison or use a fuzzy compare criterion, use an \\EqualDeviceParameters object and assign it "
    "to the device class of one netlist. You can also chain multiple \\EqualDeviceParameters objects with the '+' operator "
    "for specifying multiple parameters in the equality check.\n"
    "\n"
    "You can assign nil for the parameter comparer to remove it.\n"
    "\n"
    "In special cases, you can even implement a custom compare scheme by deriving your own comparer from the \\GenericDeviceParameterCompare class.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method_ext ("add_terminal", &gsi::dc_add_terminal_definition, gsi::arg ("terminal_def"),
    "@brief Adds the given terminal definition to the device class\n"
    "This method will define a new terminal. The new terminal is added at the end of existing terminals. "
    "The terminal definition object passed as the argument is modified to contain the "
    "new ID of the terminal.\n"
    "\n"
    "The terminal is copied into the device class. Modifying the terminal object later "
    "does not have the effect of changing the terminal definition.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("clear_terminals", &db::DeviceClass::clear_terminal_definitions,
    "@brief Clears the list of terminals\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method_ext ("add_parameter", &gsi::dc_add_parameter_definition, gsi::arg ("parameter_def"),
    "@brief Adds the given parameter definition to the device class\n"
    "This method will define a new parameter. The new parameter is added at the end of existing parameters. "
    "The parameter definition object passed as the argument is modified to contain the "
    "new ID of the parameter."
    "\n"
    "The parameter is copied into the device class. Modifying the parameter object later "
    "does not have the effect of changing the parameter definition.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("clear_parameters", &db::DeviceClass::clear_parameter_definitions,
    "@brief Clears the list of parameters\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method_ext ("combiner=", &set_combiner, gsi::arg ("combiner"),
    "@brief Specifies a device combiner (parallel or serial device combination).\n"
    "\n"
    "You can assign nil for the combiner to remove it.\n"
    "\n"
    "In special cases, you can even implement a custom combiner by deriving your own comparer from the \\GenericDeviceCombiner class.\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method_ext ("combiner", &get_combiner,
    "@brief Gets a device combiner or nil if none is registered.\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method ("supports_parallel_combination=", &db::DeviceClass::set_supports_parallel_combination, gsi::arg ("f"),
    "@brief Specifies whether the device supports parallel device combination.\n"
    "Parallel device combination means that all terminals of two combination candidates are connected to the same nets. "
    "If the device does not support this combination mode, this predicate can be set to false. This will make the device "
    "extractor skip the combination test in parallel mode and improve performance somewhat.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("supports_serial_combination=", &db::DeviceClass::set_supports_serial_combination, gsi::arg ("f"),
    "@brief Specifies whether the device supports serial device combination.\n"
    "Serial device combination means that the devices are connected by internal nodes. "
    "If the device does not support this combination mode, this predicate can be set to false. This will make the device "
    "extractor skip the combination test in serial mode and improve performance somewhat.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("equivalent_terminal_id", &db::DeviceClass::equivalent_terminal_id, gsi::arg ("original_id"), gsi::arg ("equivalent_id"),
    "@brief Specifies a terminal to be equivalent to another.\n"
    "Use this method to specify two terminals to be exchangeable. For example to make S and D of a MOS transistor equivalent, "
    "call this method with S and D terminal IDs. In netlist matching, S will be translated to D and thus made equivalent to D.\n"
    "\n"
    "Note that terminal equivalence is not effective if the device class operates in strict mode (see \\DeviceClass#strict=).\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("clear_equivalent_terminal_ids", &db::DeviceClass::clear_equivalent_terminal_ids,
    "@brief Clears all equivalent terminal ids\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ),
  "@brief A class describing a specific type of device.\n"
  "Device class objects live in the context of a \\Netlist object. After a "
  "device class is created, it must be added to the netlist using \\Netlist#add. "
  "The netlist will own the device class object. When the netlist is destroyed, the "
  "device class object will become invalid.\n"
  "\n"
  "The \\DeviceClass class is the base class for other device classes.\n"
  "\n"
  "This class has been added in version 0.26. In version 0.27.3, the 'GenericDeviceClass' has been integrated with \\DeviceClass "
  "and the device class was made writeable in most respects. This enables manipulating built-in device classes."
);

}

