
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
#include "dbNetlistReader.h"
#include "dbNetlistSpiceReader.h"
#include "dbNetlistSpiceReaderDelegate.h"

namespace gsi
{

Class<db::NetlistReader> db_NetlistReader ("db", "NetlistReader",
  gsi::Methods (),
  "@brief Base class for netlist readers\n"
  "This class is provided as a base class for netlist readers. It is not intended for reimplementation on script level, but used internally as an interface.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

/**
 *  @brief A helper class wrapping the return values for NetlistSpiceReaderDelegateImpl::parse_element
 */
class ParseElementData
{
public:
  ParseElementData () : m_value (0.0) { }

  const std::string &element_name () const { return m_element; }
  std::string &element_name_nc () { return m_element; }
  void set_element_name (const std::string &element) { m_element = element; }
  const std::string &model_name () const { return m_model; }
  std::string &model_name_nc () { return m_model; }
  void set_model_name (const std::string &model) { m_model = model; }
  double value () const { return m_value; }
  double &value_nc () { return m_value; }
  void set_value (double value) { m_value = value; }
  const std::vector<std::string> &net_names () const { return m_net_names; }
  std::vector<std::string> &net_names_nc () { return m_net_names; }
  void set_net_names (const std::vector<std::string> &nn) { m_net_names = nn; }
  const db::NetlistSpiceReader::parameters_type &parameters () const { return m_parameters; }
  db::NetlistSpiceReader::parameters_type &parameters_nc () { return m_parameters; }
  void set_parameters (const db::NetlistSpiceReader::parameters_type &parameters) { m_parameters = parameters; }

private:
  std::string m_model, m_element;
  double m_value;
  std::vector<std::string> m_net_names;
  db::NetlistSpiceReader::parameters_type m_parameters;
};

/**
 *  @brief A helper class for the return values of NetlistSpiceReaderDelegateImpl::parse_element_components
 */
class ParseElementComponentsData
{
public:
  ParseElementComponentsData () { }

  const std::vector<std::string> &strings () const { return m_strings; }
  std::vector<std::string> &strings_nc () { return m_strings; }
  void set_strings (const std::vector<std::string> &nn) { m_strings = nn; }
  const db::NetlistSpiceReader::parameters_type &parameters () const { return m_parameters; }
  db::NetlistSpiceReader::parameters_type &parameters_nc () { return m_parameters; }
  void set_parameters (const db::NetlistSpiceReader::parameters_type &parameters) { m_parameters = parameters; }

private:
  std::vector<std::string> m_strings;
  db::NetlistSpiceReader::parameters_type m_parameters;
};

/**
 *  @brief A SPICE reader delegate base class for reimplementation
 */
class NetlistSpiceReaderDelegateImpl
  : public db::NetlistSpiceReaderDelegate, public gsi::ObjectBase
{
public:
  NetlistSpiceReaderDelegateImpl ()
    : db::NetlistSpiceReaderDelegate (), mp_variables (0)
  {
    //  .. nothing yet ..
  }

  virtual void error (const std::string &msg)
  {
    //  doing this avoids passing exceptions through script code which spoils the message
    //  (the exception will be decorated with a stack trace). TODO: a better solution was
    //  to define a specific exception type for "raw exception".
    m_error = msg;
    db::NetlistSpiceReaderDelegate::error (msg);
  }

  virtual void start (db::Netlist *netlist)
  {
    try {
      m_error.clear ();
      if (cb_start.can_issue ()) {
        cb_start.issue<db::NetlistSpiceReaderDelegate, db::Netlist *> (&db::NetlistSpiceReaderDelegate::start, netlist);
      } else {
        db::NetlistSpiceReaderDelegate::start (netlist);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
    }
  }

  virtual void finish (db::Netlist *netlist)
  {
    try {
      m_error.clear ();
      if (cb_finish.can_issue ()) {
        cb_finish.issue<db::NetlistSpiceReaderDelegate, db::Netlist *> (&db::NetlistSpiceReaderDelegate::finish, netlist);
      } else {
        db::NetlistSpiceReaderDelegate::finish (netlist);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
    }
  }

  virtual bool control_statement (const std::string &line)
  {
    try {
      m_error.clear ();
      if (cb_control_statement.can_issue ()) {
        return cb_control_statement.issue<db::NetlistSpiceReaderDelegate, bool, const std::string &> (&db::NetlistSpiceReaderDelegate::control_statement, line);
      } else {
        return db::NetlistSpiceReaderDelegate::control_statement (line);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return false;
    }
  }

  virtual bool wants_subcircuit (const std::string &circuit_name)
  {
    try {
      m_error.clear ();
      if (cb_wants_subcircuit.can_issue ()) {
        return cb_wants_subcircuit.issue<db::NetlistSpiceReaderDelegate, bool, const std::string &> (&db::NetlistSpiceReaderDelegate::wants_subcircuit, circuit_name);
      } else {
        return db::NetlistSpiceReaderDelegate::wants_subcircuit (circuit_name);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return false;
    }
  }

  virtual std::string translate_net_name (const std::string &nn)
  {
    try {
      m_error.clear ();
      if (cb_translate_net_name.can_issue ()) {
        return cb_translate_net_name.issue<db::NetlistSpiceReaderDelegate, std::string, const std::string &> (&db::NetlistSpiceReaderDelegate::translate_net_name, nn);
      } else {
        return db::NetlistSpiceReaderDelegate::translate_net_name (nn);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return std::string ();
    }
  }

  ParseElementData parse_element_helper (const std::string &s, const std::string &element)
  {
    //  NOTE: this way of treating the element name is compatible with the old way and
    //  the new way in which the element name can be modified by "parse_element".
    ParseElementData data;
    data.set_element_name (element);
    db::NetlistSpiceReaderDelegate::parse_element (s, data.element_name_nc (), data.model_name_nc (), data.value_nc (), data.net_names_nc (), data.parameters_nc (), variables ());
    return data;
  }

  virtual void parse_element (const std::string &s, std::string &element, std::string &model, double &value, std::vector<std::string> &nn, db::NetlistSpiceReader::parameters_type &pv, const db::NetlistSpiceReader::parameters_type &variables)
  {
    try {

      m_error.clear ();
      mp_variables = &variables;

      ParseElementData data;
      if (cb_parse_element.can_issue ()) {
        data = cb_parse_element.issue<NetlistSpiceReaderDelegateImpl, ParseElementData, const std::string &, const std::string &> (&NetlistSpiceReaderDelegateImpl::parse_element_helper, s, element);
      } else {
        data = parse_element_helper (s, element);
      }

      element = data.element_name ();
      model = data.model_name ();
      value = data.value ();
      nn = data.net_names ();
      pv = data.parameters ();

      mp_variables = 0;

    } catch (tl::Exception &) {
      mp_variables = 0;
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
    } catch (...) {
      mp_variables = 0;
      throw;
    }
  }

  virtual bool element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const db::NetlistSpiceReader::parameters_type &params)
  {
    try {
      m_error.clear ();
      if (cb_element.can_issue ()) {
        return cb_element.issue<db::NetlistSpiceReaderDelegate, bool, db::Circuit *, const std::string &, const std::string &, const std::string &, double, const std::vector<db::Net *> &, const db::NetlistSpiceReader::parameters_type &> (&db::NetlistSpiceReaderDelegate::element, circuit, element, name, model, value, nets, params);
      } else {
        return db::NetlistSpiceReaderDelegate::element (circuit, element, name, model, value, nets, params);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return false;
    }
  }

  const db::NetlistSpiceReader::parameters_type &variables () const
  {
    static db::NetlistSpiceReader::parameters_type empty;
    return mp_variables ? *mp_variables : empty;
  }

  gsi::Callback cb_start;
  gsi::Callback cb_finish;
  gsi::Callback cb_control_statement;
  gsi::Callback cb_wants_subcircuit;
  gsi::Callback cb_translate_net_name;
  gsi::Callback cb_element;
  gsi::Callback cb_parse_element;

private:
  std::string m_error;
  const db::NetlistSpiceReader::parameters_type *mp_variables;
};

static void start_fb (NetlistSpiceReaderDelegateImpl *delegate, db::Netlist *netlist)
{
  delegate->db::NetlistSpiceReaderDelegate::start (netlist);
}

static void finish_fb (NetlistSpiceReaderDelegateImpl *delegate, db::Netlist *netlist)
{
  delegate->db::NetlistSpiceReaderDelegate::finish (netlist);
}

static bool wants_subcircuit_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &model)
{
  return delegate->db::NetlistSpiceReaderDelegate::wants_subcircuit (model);
}

static bool control_statement_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &line)
{
  return delegate->db::NetlistSpiceReaderDelegate::control_statement (line);
}

static std::string translate_net_name_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &name)
{
  return delegate->db::NetlistSpiceReaderDelegate::translate_net_name (name);
}

static bool element_fb (NetlistSpiceReaderDelegateImpl *delegate, db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const db::NetlistSpiceReader::parameters_type &params)
{
  return delegate->db::NetlistSpiceReaderDelegate::element (circuit, element, name, model, value, nets, params);
}

static ParseElementData parse_element_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &s, const std::string &element)
{
  return delegate->parse_element_helper (s, element);
}

static tl::Variant value_from_string (NetlistSpiceReaderDelegateImpl * /*delegate*/, const std::string &s, const db::NetlistSpiceReader::parameters_type &variables)
{
  tl::Variant res;
  double v = 0.0;
  if (db::NetlistSpiceReaderDelegate::try_read_value (s, v, variables)) {
    res = v;
  }
  return res;
}

static ParseElementComponentsData parse_element_components (NetlistSpiceReaderDelegateImpl *delegate, const std::string &s, const db::NetlistSpiceReader::parameters_type &variables)
{
  ParseElementComponentsData data;
  delegate->parse_element_components (s, data.strings_nc (), data.parameters_nc (), variables);
  return data;
}

Class<ParseElementComponentsData> db_ParseElementComponentsData ("db", "ParseElementComponentsData",
  gsi::method ("strings", &ParseElementComponentsData::strings,
    "@brief Gets the (unnamed) string parameters\n"
    "These parameters are typically net names or model name."
  ) +
  gsi::method ("strings=", &ParseElementComponentsData::set_strings, gsi::arg ("list"),
    "@brief Sets the (unnamed) string parameters\n"
  ) +
  gsi::method ("parameters", &ParseElementComponentsData::parameters,
    "@brief Gets the (named) parameters\n"
    "Named parameters are typically (but not neccessarily) numerical, like 'w=0.15u'."
  ) +
  gsi::method ("parameters=", &ParseElementComponentsData::set_parameters, gsi::arg ("dict"),
    "@brief Sets the (named) parameters\n"
  ),
  "@brief Supplies the return value for \\NetlistSpiceReaderDelegate#parse_element_components.\n"
  "This is a structure with two members: 'strings' for the string arguments and 'parameters' for the "
  "named arguments.\n"
  "\n"
  "This helper class has been introduced in version 0.27.1. Starting with version 0.28.6, named parameters can be string types too.\n"
);

Class<ParseElementData> db_ParseElementData ("db", "ParseElementData",
  gsi::method ("value", &ParseElementData::value,
    "@brief Gets the value\n"
  ) +
  gsi::method ("value=", &ParseElementData::set_value, gsi::arg ("v"),
    "@brief Sets the value\n"
  ) +
  gsi::method ("model_name", &ParseElementData::model_name,
    "@brief Gets the model name\n"
  ) +
  gsi::method ("model_name=", &ParseElementData::set_model_name, gsi::arg ("m"),
    "@brief Sets the model name\n"
  ) +
  gsi::method ("element_name", &ParseElementData::element_name,
    "@brief Gets the element name\n"
    "This attribute is available since version 0.31.0.\n"
  ) +
  gsi::method ("element_name=", &ParseElementData::set_element_name, gsi::arg ("e"),
    "@brief Sets the element name\n"
    "This attribute is available since version 0.31.0.\n"
  ) +
  gsi::method ("net_names", &ParseElementData::net_names,
    "@brief Gets the net names\n"
  ) +
  gsi::method ("net_names=", &ParseElementData::set_net_names, gsi::arg ("list"),
    "@brief Sets the net names\n"
  ) +
  gsi::method ("parameters", &ParseElementData::parameters,
    "@brief Gets the (named) parameters\n"
  ) +
  gsi::method ("parameters=", &ParseElementData::set_parameters, gsi::arg ("dict"),
    "@brief Sets the (named) parameters\n"
  ),
  "@brief Supplies the return value for \\NetlistSpiceReaderDelegate#parse_element.\n"
  "This is a structure with four members: 'model_name' for the model name, 'value' for the default numerical value, 'net_names' for the net names and 'parameters' for the "
  "named parameters.\n"
  "\n"
  "This helper class has been introduced in version 0.27.1. Starting with version 0.28.6, named parameters can be string types too.\n"
);

static double get_delegate_scale (const db::NetlistSpiceReaderDelegate *delegate)
{
  return delegate->options ().scale;
}

static void apply_parameter_scaling (const db::NetlistSpiceReaderDelegate *delegate, db::Device *device)
{
  delegate->apply_parameter_scaling (device);
}

Class<NetlistSpiceReaderDelegateImpl> db_NetlistSpiceReaderDelegate ("db", "NetlistSpiceReaderDelegate",
  gsi::method_ext ("start", &start_fb, "@hide") +
  gsi::method_ext ("finish", &finish_fb, "@hide") +
  gsi::method_ext ("wants_subcircuit", &wants_subcircuit_fb, "@hide") +
  gsi::method_ext ("element", &element_fb, "@hide") +
  gsi::method_ext ("parse_element", &parse_element_fb, "@hide") +
  gsi::method_ext ("control_statement", &control_statement_fb, "@hide") +
  gsi::method_ext ("translate_net_name", &translate_net_name_fb, "@hide") +
  gsi::method ("read_all_parameters=", &NetlistSpiceReaderDelegateImpl::read_all_parameters,
    "@brief Sets a flag indicating whether to read all device parameters.\n"
    "If this flag is set to true, all parameters of the devices are read in the default implementation. "
    "If set to false (the default), only known parameters (i.e. only parameters declared by the device classes in the netlist) are read.\n"
    "\n"
    "Note, that you can customize the reader's device input also by reimplementing \\parse_element or \\element. This reimplementation may "
    "chose to ignore the \\read_all_parameters attribute.\n"
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ) +
  gsi::method ("read_all_parameters=", &NetlistSpiceReaderDelegateImpl::set_read_all_parameters, gsi::arg ("f"),
    "@brief Gets a flag indicating whether to read all device parameters.\n"
    "See \\read_all_parameters= for a description of this attribute.\n"
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ) +
  gsi::callback ("start", &NetlistSpiceReaderDelegateImpl::start, &NetlistSpiceReaderDelegateImpl::cb_start, gsi::arg ("netlist"),
    "@brief This method is called when the reader starts reading a netlist\n"
  ) +
  gsi::callback ("finish", &NetlistSpiceReaderDelegateImpl::finish, &NetlistSpiceReaderDelegateImpl::cb_finish, gsi::arg ("netlist"),
    "@brief This method is called when the reader is done reading a netlist successfully\n"
  ) +
  gsi::callback ("wants_subcircuit", &NetlistSpiceReaderDelegateImpl::wants_subcircuit, &NetlistSpiceReaderDelegateImpl::cb_wants_subcircuit, gsi::arg ("circuit_name"),
    "@brief Returns true, if the delegate wants subcircuit elements with this name\n"
    "The name is always upper case.\n"
  ) +
  gsi::callback ("control_statement", &NetlistSpiceReaderDelegateImpl::control_statement, &NetlistSpiceReaderDelegateImpl::cb_control_statement, gsi::arg ("line"),
    "@brief Receives control statements not understood by the standard reader\n"
    "When the reader encounters a control statement not understood by the parser, it will pass the line to the delegate using this method.\n"
    "The delegate can decide if it wants to read this statement. It should return true in this case.\n"
    "\n"
    "This method has been introduced in version 0.27.1\n"
  ) +
  gsi::callback ("translate_net_name", &NetlistSpiceReaderDelegateImpl::translate_net_name, &NetlistSpiceReaderDelegateImpl::cb_translate_net_name, gsi::arg ("net_name"),
    "@brief Translates a net name from the raw net name to the true net name\n"
    "The default implementation will replace backslash sequences by the corresponding character.\n"
    "'translate_net_name' is called before a net name is turned into a net object.\n"
    "The method can be reimplemented to supply a different translation scheme for net names. For example, to translate special characters.\n"
    "\n"
    "This method has been introduced in version 0.27.1\n"
  ) +
  gsi::method ("variables", &NetlistSpiceReaderDelegateImpl::variables,
    "@brief Gets the variables defined inside the SPICE file during execution of 'parse_element'\n"
    "In order to evaluate formulas, this method allows accessing the variables that are "
    "present during the execution of the SPICE reader.\n"
    "\n"
    "This method has been introduced in version 0.28.6."
  ) +
  gsi::callback ("parse_element", &NetlistSpiceReaderDelegateImpl::parse_element_helper, &NetlistSpiceReaderDelegateImpl::cb_parse_element,
    gsi::arg ("s"), gsi::arg ("element"),
    "@brief Parses an element card\n"
    "@param s The specification part of the element line (the part after element code and name).\n"
    "@param element The upper-case element code (\"M\", \"R\", ...).\n"
    "@return A \\ParseElementData object with the parts of the element.\n"
    "\n"
    "This method receives a string with the element specification and the element code. It is supposed to "
    "parse the element line and return a model name, a value, a list of net names and a parameter value dictionary.\n"
    "\n"
    "'parse_element' is called on every element card. The results of this call go into the \\element method "
    "to actually create the device. This method can be reimplemented to support other flavors of SPICE.\n"
    "\n"
    "This method has been introduced in version 0.27.1\n"
  ) +
  gsi::callback ("element", &NetlistSpiceReaderDelegateImpl::element, &NetlistSpiceReaderDelegateImpl::cb_element,
    gsi::arg ("circuit"), gsi::arg ("element"), gsi::arg ("name"), gsi::arg ("model"), gsi::arg ("value"), gsi::arg ("nets"), gsi::arg ("parameters"),
    "@brief Makes a device from an element line\n"
    "@param circuit The circuit that is currently read.\n"
    "@param element The upper-case element code (\"M\", \"R\", ...).\n"
    "@param name The element's name.\n"
    "@param model The upper-case model name (may be empty).\n"
    "@param value The default value (e.g. resistance for resistors) and may be zero.\n"
    "@param nets The nets given in the element line.\n"
    "@param parameters The parameters of the element statement (parameter names are upper case).\n"
    "\n"
    "The default implementation will create corresponding devices for\n"
    "some known elements using the Spice writer's parameter conventions.\n"
    "\n"
    "The method must return true, if the element was was understood and false otherwise.\n"
    "\n"
    "Starting with version 0.28.6, the parameter values can be strings too."
  ) +
  gsi::method ("error", &NetlistSpiceReaderDelegateImpl::error, gsi::arg ("msg"),
    "@brief Issues an error with the given message.\n"
    "Use this method to generate an error."
  ) +
  gsi::method_ext ("get_scale", &get_delegate_scale,
    "@brief Gets the scale factor set with '.options scale=...'\n"
    "This method has been introduced in version 0.28.6."
  ) +
  gsi::method_ext ("apply_parameter_scaling", &apply_parameter_scaling, gsi::arg ("device"),
    "@brief Applies parameter scaling to the given device\n"
    "Applies SI scaling (according to the parameter's si_scaling attribute) and "
    "geometry scaling (according to the parameter's geo_scale_exponent attribute) to "
    "the device parameters. Use this method of finish the device when you have created "
    "a custom device yourself.\n"
    "\n"
    "The geometry scale is taken from the '.options scale=...' control statement.\n"
    "\n"
    "This method has been introduced in version 0.28.6."
  ) +
  gsi::method_ext ("value_from_string", &value_from_string, gsi::arg ("s"), gsi::arg ("variables", db::NetlistSpiceReader::parameters_type (), "{}"),
    "@brief Translates a string into a value\n"
    "This function simplifies the implementation of SPICE readers by providing a translation of a unit-annotated string "
    "into double values. For example, '1k' is translated to 1000.0. In addition, simple formula evaluation is supported, e.g "
    "'(1+3)*2' is translated into 8.0.\n"
    "\n"
    "The variables dictionary defines named variables with the given values.\n"
    "\n"
    "This method has been introduced in version 0.27.1. The variables argument has been added in version 0.28.6.\n"
  ) +
  gsi::method_ext ("parse_element_components", &parse_element_components, gsi::arg ("s"), gsi::arg ("variables", db::NetlistSpiceReader::parameters_type (), "{}"),
    "@brief Parses a string into string and parameter components.\n"
    "This method is provided to simplify the implementation of 'parse_element'. It takes a string and splits it into "
    "string arguments and parameter values. For example, 'a b c=6' renders two string arguments in 'nn' and one parameter ('C'->6.0). "
    "It returns data \\ParseElementComponentsData object with the strings and parameters.\n"
    "The parameter names are already translated to upper case.\n"
    "\n"
    "The variables dictionary defines named variables with the given values.\n"
    "\n"
    "This method has been introduced in version 0.27.1. The variables argument has been added in version 0.28.6.\n"
  ),
  "@brief Provides a delegate for the SPICE reader for translating device statements\n"
  "Supply a customized class to provide a specialized reading scheme for devices. "
  "You need a customized class if you want to implement device reading from model subcircuits or to "
  "translate device parameters.\n"
  "\n"
  "See \\NetlistSpiceReader for more details.\n"
  "\n"
  "This class has been introduced in version 0.26. Profiles have been added to the device classes in version 0.31.0."
);

namespace {

class NetlistSpiceReaderWithOwnership
  : public db::NetlistSpiceReader
{
public:
  NetlistSpiceReaderWithOwnership (NetlistSpiceReaderDelegateImpl *delegate)
    : db::NetlistSpiceReader (delegate), m_ownership (delegate)
  {
    if (delegate) {
      delegate->keep ();
    }
  }

private:
  tl::shared_ptr<NetlistSpiceReaderDelegateImpl> m_ownership;
};

}

db::NetlistSpiceReader *new_spice_reader (const std::string &profile)
{
  auto *reader = new db::NetlistSpiceReader ();
  reader->set_profile (profile);
  return reader;
}

db::NetlistSpiceReader *new_spice_reader2 (NetlistSpiceReaderDelegateImpl *delegate, const std::string &profile)
{
  auto *reader = new NetlistSpiceReaderWithOwnership (delegate);
  reader->set_profile (profile);
  return reader;
}

static bool read_all_parameters (const db::NetlistSpiceReader *reader)
{
  return reader->delegate () ? reader->delegate ()->read_all_parameters () : false;
}

static void set_read_all_parameters (db::NetlistSpiceReader *reader, bool f)
{
  if (reader->delegate ()) {
    reader->delegate ()->set_read_all_parameters (f);
  }
}


Class<db::NetlistSpiceReader> db_NetlistSpiceReader (db_NetlistReader, "db", "NetlistSpiceReader",
  gsi::constructor ("new", &new_spice_reader, gsi::arg ("profile", std::string ()),
    "@brief Creates a new reader.\n"
    "The profile string defines the SPICE profile to use for picking default SPICE mappings from the "
    "device classes declared in the netlist to read.\n"
    "\n"
    "The profile string argument has been added in version 0.31.0."
  ) +
  gsi::constructor ("new", &new_spice_reader2, gsi::arg ("delegate"), gsi::arg ("profile", std::string ()),
    "@brief Creates a new reader with a delegate.\n"
    "The profile string defines the SPICE profile to use for picking default SPICE mappings from the "
    "device classes declared in the netlist to read.\n"
    "\n"
    "The profile string argument has been added in version 0.31.0."
  ) +
  gsi::method_ext ("read_all_parameters", &read_all_parameters,
    "@brief Gets a flag indicating whether all parameters shall be read\n"
    "\n"
    "With this flag set to false, only those parameters which\n"
    "are declared in the device classes of the netlist are read.\n"
    "If set to true (the default), additional parameters are added to the device classes\n"
    "and their values are stored in the device objects.\n"
    "Note that this behavior can be changed in reimplementations of the\n"
    "SPICE reader delegate class.\n"
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ) +
  gsi::method_ext ("read_all_parameters=", &set_read_all_parameters, gsi::arg ("f"),
    "@brief Sets a flag indicating whether all parameters shall be read\n"
    "See \\read_all_parameters for details.\n"
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ),
  "@brief Implements a netlist Reader for the SPICE format.\n"
  "Use the SPICE reader like this:\n"
  "\n"
  "@code\n"
  "reader = RBA::NetlistSpiceReader::new\n"
  "netlist = RBA::Netlist::new\n"
  "netlist.read(path, reader)\n"
  "@/code\n"
  "\n"
  "The translation of SPICE elements can be tailored by providing a \\NetlistSpiceReaderDelegate class or by "
  "supplying device classes in the netlist that declare their SPICe representation in a SPICE profile.\n"
  //  @@@ TODO: example for profiles in device classes
  "\n"
  "The following example is a delegate that turns subcircuits called HVNMOS and HVPMOS into "
  "MOS4 devices with the parameters scaled by 1.5:\n"
  "\n"
  "@code\n"
  "class MyDelegate < RBA::NetlistSpiceReaderDelegate\n"
  "\n"
  "  # says we want to catch these subcircuits as devices\n"
  "  def wants_subcircuit(name)\n"
  "    name == \"HVNMOS\" || name == \"HVPMOS\"\n"
  "  end\n"
  "\n"
  "  # translate the element\n"
  "  def element(circuit, el, name, model, value, nets, params)\n"
  "\n"
  "    if el != \"X\"\n"
  "      # all other elements are left to the standard implementation\n"
  "      return super\n"
  "    end\n"
  "\n"
  "    if nets.size != 4\n"
  "      error(\"Subcircuit #{model} needs four nodes\")\n"
  "    end\n"
  "\n"
  "    # provide a device class\n"
  "    cls = circuit.netlist.device_class_by_name(model)\n"
  "    if ! cls\n"
  "      cls = RBA::DeviceClassMOS4Transistor::new\n"
  "      cls.name = model\n"
  "      circuit.netlist.add(cls)\n"
  "    end\n"
  "\n"
  "    # create a device\n"
  "    device = circuit.create_device(cls, name)\n"
  "\n"
  "    # and configure the device\n"
  "    [ \"S\", \"G\", \"D\", \"B\" ].each_with_index do |t,index|\n"
  "      device.connect_terminal(t, nets[index])\n"
  "    end\n"
  "    params.each do |p,value|\n"
  "      device.set_parameter(p, value * 1.5)\n"
  "    end\n"
  "\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "# usage:\n"
  "\n"
  "mydelegate = MyDelegate::new\n"
  "reader = RBA::NetlistSpiceReader::new(mydelegate)\n"
  "\n"
  "nl = RBA::Netlist::new\n"
  "nl.read(input_file, reader)\n"
  "@/code\n"
  "\n"
  "A somewhat contrived example for using the delegate to translate net names is this:\n"
  "\n"
  "@code\n"
  "class MyDelegate < RBA::NetlistSpiceReaderDelegate\n"
  "\n"
  "  # translates 'VDD' to 'VXX' and leave all other net names as is:\n"
  "  alias translate_net_name_org translate_net_name\n"
  "  def translate_net_name(n)\n"
  "    return n == \"VDD\" ? \"VXX\" : translate_net_name_org(n)}\n"
  "  end\n"
  "\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.26. It has been extended in version 0.27.1. "
  "Profiles have been added to the device classes in version 0.31.0."
);

}

