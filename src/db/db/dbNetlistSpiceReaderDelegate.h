
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

#ifndef HDR_dbNetlistSpiceReaderDelegate
#define HDR_dbNetlistSpiceReaderDelegate

#include "dbCommon.h"
#include "tlStream.h"
#include "tlException.h"

#include <string>
#include <vector>
#include <map>

namespace db
{

class Netlist;
class Net;
class Circuit;
class DeviceClass;
class Device;

struct DB_PUBLIC NetlistSpiceReaderOptions
{
  NetlistSpiceReaderOptions ();

  double scale;
  std::map<std::string, std::map<std::string, double> > default_values;
};

/**
 *  @brief A delegate to handle various forms of devices and translates them
 *
 *  The default behavior is controlled by the device classes that
 *  are defined inside the netlist object, before it is passed to the
 *  "start" method. The device class may define rules which are
 *  selected by the "SPICE profile" string. These rules include the
 *  SPICE element that is used for a device and the terminal order.
 *
 *  Instead of using the device profiles, the SPICE reader delegate
 *  can also be customized by re-implementing methods that control
 *  the parsing of SPICE lines and creation of devices from them.
 *
 *  For receiving subcircuit elements, the delegate needs to indicate
 *  this by returning true upon "wants_subcircuit" or by declaring
 *  device classes with "X" elements in their SPICE profile.
 */
class DB_PUBLIC NetlistSpiceReaderDelegate
  : public tl::Object
{
public:
  NetlistSpiceReaderDelegate ();
  virtual ~NetlistSpiceReaderDelegate ();

  /**
   *  @brief Gets the reader options
   *
   *  The reader options are settings collected during the parsing of the SPICE file,
   *  such as the scale value etc.
   */
  const NetlistSpiceReaderOptions &options () const
  {
    return m_options;
  }

  /**
   *  @brief Gets the reader options (non-const)
   */
  NetlistSpiceReaderOptions &options ()
  {
    return m_options;
  }

  /**
   *  @brief Gets a flag indicating whether all parameters shall be read
   *
   *  With this flag set to false (the default), only those parameters which
   *  are declared in the device classes of the netlist are read.
   *  Otherwise, additional parameters are added to the device classes
   *  and their values are stored in the device objects.
   */
  bool read_all_parameters () const;

  /**
   *  @brief Sets a flag indicating whether all parameters shall be read
   */
  void set_read_all_parameters (bool f);

  /**
   *  @brief Gets a flag indicating legacy mode
   *
   *  This flag controls the parsing of SPICE lines in the default
   *  implementation of "parse_element". With this flag set to true (default),
   *  the elements are restricted to their original meaning, i.e. "R"
   *  can have two or three nets and a direct value. With this flag set to
   *  false, all elements can have any number of terminals, the last entry
   *  is the model and a direct value is not allowed.
   *
   *  This flag allows configuring the SPICE reading without
   *  need to reimplementing a delegate.
   */
  bool legacy_mode () const;

  /**
   *  @brief Sets a flag indicating legacy mode
   */
  void set_legacy_mode (bool f);

  /**
   *  @brief Called when the netlist reading starts
   */
  virtual void start (db::Netlist *netlist);

  /**
   *  @brief Called when the netlist reading ends
   */
  virtual void finish (db::Netlist *netlist);

  /**
   *  @brief Called when an unknown control statement is encountered
   *
   *  Returns true if the statement is understood.
   */
  virtual bool control_statement (const std::string &line);

  /**
   *  @brief Returns true, if the delegate wants subcircuit elements with this name
   *
   *  The name is always upper case.
   */
  virtual bool wants_subcircuit (const std::string &circuit_name);

  /**
   *  @brief This method translates a raw net name to a valid net name
   *
   *  The default implementation will unescape backslash sequences into plain characters.
   */
  virtual std::string translate_net_name (const std::string &nn);

  /**
   *  @brief Makes a device from an element line
   *
   *  @param circuit The circuit that is currently read.
   *  @param element The upper-case element code ("M", "R", ...).
   *  @param name The element's name.
   *  @param model The upper-case model name (may be empty).
   *  @param value The default value (e.g. resistance for resistors) and may be zero.
   *  @param nets The nets given in the element line.
   *  @param parameters The parameters of the element statement (parameter names are upper case).
   *
   *  The default implementation will create corresponding devices for
   *  some known elements using the Spice writer's parameter conventions.
   *
   *  This method returns true, if the element was read.
   */
  virtual bool element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const std::map<std::string, tl::Variant> &params);

  /**
   *  @brief Parses an element from a line
   *
   *  @param s The line to parse (the part after the element and name)
   *  @param model Out parameter: the model name if given
   *  @param value Out parameter: the value if given (for R, L, C)
   *  @param nn Out parameter: the net names
   *  @param pv Out parameter: the parameter values (key/value pairs)
   */
  virtual void parse_element (const std::string &s, std::string &element, std::string &model, double &value, std::vector<std::string> &nn, std::map<std::string, tl::Variant> &pv, const std::map<std::string, tl::Variant> &params);

  /**
   *  @brief Produces an error with the given message
   */
  virtual void error (const std::string &msg);

  /**
   *  @brief Reads a set of string components and parameters from the string
   */
  void parse_element_components (const std::string &s, std::vector<std::string> &strings, std::map<std::string, tl::Variant> &pv, const std::map<std::string, tl::Variant> &variables);

  /**
   *  @brief Reads a value from the extractor (with formula evaluation)
   */
  static tl::Variant read_value(tl::Extractor &ex, const std::map<std::string, tl::Variant> &variables);

  /**
   *  @brief Reads a value from the extractor (with formula evaluation and two levels of variables)
   */
  static tl::Variant read_value(tl::Extractor &ex, const std::map<std::string, tl::Variant> &variables1, const std::map<std::string, tl::Variant> &variables2);

  /**
   *  @brief Tries to read a value from the extractor (with formula evaluation)
   */
  static bool try_read_value (const std::string &s, double &v, const std::map<std::string, tl::Variant> &variables);

  /**
   *  @brief External interface for start
   */
  void do_start ();

  /**
   *  @brief External interface for finish
   */
  void do_finish ();

  /**
   *  @brief Sets the netlist and the SPICE profile to be used
   */
  void set_netlist (db::Netlist *netlist, const std::string &profile);

  /**
   *  @brief Applies SI and geometry scaling to the device parameters
   */
  void apply_parameter_scaling (db::Device *device) const;

  /**
   *  @brief Gets the device class registered for an element/model combination
   *
   *  Returns a null pointer if no such combination is registered.
   */
  const db::DeviceClass *device_class (const std::string &element, const std::string &model) const;

  /**
   *  @brief Gets the SPICE profile
   */
  const std::string &profile () const
  {
    return m_profile;
  }

private:
  db::Netlist *mp_netlist;
  NetlistSpiceReaderOptions m_options;
  std::string m_profile;
  bool m_read_all_parameters;
  bool m_legacy_mode;
  std::map<std::pair<std::string, std::string>, const db::DeviceClass *> m_spice_profiles;

  void def_values_per_element (const std::string &element, std::map<std::string, tl::Variant> &pv);
};

}

#endif
