
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

#ifndef HDR_dbNetlistSpiceReader
#define HDR_dbNetlistSpiceReader

#include "dbCommon.h"
#include "dbNetlistReader.h"
#include "tlStream.h"

#include <string>
#include <set>
#include <map>
#include <memory>
#include <list>

namespace db
{

class Netlist;
class Net;
class Circuit;
class DeviceClass;
class Device;

/**
 *  @brief A specialized exception class to handle netlist reader delegate errors
 */
class DB_PUBLIC NetlistSpiceReaderDelegateError
  : public tl::Exception
{
public:
  NetlistSpiceReaderDelegateError (const std::string &msg)
    : tl::Exception (msg)
  { }
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
  virtual bool element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const std::map<std::string, double> &params);

  /**
   *  @brief Parses an element from a line
   *
   *  @param s The line to parse (the part after the element and name)
   *  @param model Out parameter: the model name if given
   *  @param value Out parameter: the value if given (for R, L, C)
   *  @param nn Out parameter: the net names
   *  @param pv Out parameter: the parameter values (key/value pairs)
   */
  virtual void parse_element (const std::string &s, const std::string &element, std::string &model, double &value, std::vector<std::string> &nn, std::map<std::string, double> &pv, const std::map<std::string, double> &params);

  /**
   *  @brief Produces an error with the given message
   */
  virtual void error (const std::string &msg);

  /**
   *  @brief Reads a set of string components and parameters from the string
   *  A special key "param:" is recognized for starting a parameter list.
   */
  static void parse_element_components (const std::string &s, std::vector<std::string> &strings, std::map<std::string, double> &pv, const std::map<std::string, double> &variables);

  /**
   *  @brief Reads a value from the extractor (with formula evaluation)
   */
  static double read_value (tl::Extractor &ex, const std::map<std::string, double> &variables);

  /**
   *  @brief Tries to read a value from the extractor (with formula evaluation)
   */
  static bool try_read_value (const std::string &s, double &v, const std::map<std::string, double> &variables);

private:
  static double read_atomic_value (tl::Extractor &ex, const std::map<std::string, double> &variables);
  static double read_dot_expr (tl::Extractor &ex, const std::map<std::string, double> &variables);
  static double read_bar_expr (tl::Extractor &ex, const std::map<std::string, double> &variables);
};

/**
 *  @brief A SPICE format reader for netlists
 */
class DB_PUBLIC NetlistSpiceReader
  : public NetlistReader
{
public:
  NetlistSpiceReader (NetlistSpiceReaderDelegate *delegate = 0);
  virtual ~NetlistSpiceReader ();

  virtual void read (tl::InputStream &stream, db::Netlist &netlist);

private:
  tl::weak_ptr<NetlistSpiceReaderDelegate> mp_delegate;
};

}

#endif
