
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
  double defad, defas, defw, defl;
};

/**
 *  @brief A delegate to handle various forms of devices and translates them
 *
 *  The reader delegate can be configured to receive subcircuit elements too.
 *  In this case, parameters are allowed.
 *  For receiving subcircuit elements, the delegate needs to indicate
 *  this by returning true upon "wants_subcircuit".
 */
class DB_PUBLIC NetlistSpiceReaderDelegate
  : public tl::Object
{
public:
  NetlistSpiceReaderDelegate ();
  virtual ~NetlistSpiceReaderDelegate ();

  /**
   *  @brief Gets the reader options
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
  virtual void parse_element (const std::string &s, const std::string &element, std::string &model, double &value, std::vector<std::string> &nn, std::map<std::string, tl::Variant> &pv, const std::map<std::string, tl::Variant> &params);

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
   *  @brief Sets the netlist
   */
  void set_netlist (db::Netlist *netlist);

  /**
   *  @brief Applies SI and geometry scaling to the device parameters
   */
  void apply_parameter_scaling (db::Device *device) const;

private:
  db::Netlist *mp_netlist;
  NetlistSpiceReaderOptions m_options;

  void def_values_per_element (const std::string &element, std::map<std::string, tl::Variant> &pv);
};

}

#endif
