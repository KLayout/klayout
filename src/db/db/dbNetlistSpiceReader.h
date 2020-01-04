
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
 *  The reader delegate can be configured to recieve subcircuit elements too.
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
   *  @brief Returns true, if the delegate wants subcircuit elements with this name
   *
   *  The name is always upper case.
   */
  virtual bool wants_subcircuit (const std::string &circuit_name);

  /**
   *  @brief Makes a device from an element line
   *
   *  @param circuit The circuit that is currently read.
   *  @param element The upper-case element code ("M", "R", ...).
   *  @param name The element's name.
   *  @param model The upper-case model name (may be empty).
   *  @param value The default value (e.g. registance for resistors) and may be zero.
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
   *  @brief Produces an error with the given message
   */
  virtual void error (const std::string &msg);
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
  db::Netlist *mp_netlist;
  db::Circuit *mp_circuit;
  std::auto_ptr<tl::TextInputStream> mp_stream;
  tl::weak_ptr<NetlistSpiceReaderDelegate> mp_delegate;
  std::vector<std::pair<tl::InputStream *, tl::TextInputStream *> > m_streams;
  std::auto_ptr<std::map<std::string, db::Net *> > mp_nets_by_name;
  std::string m_stored_line;
  std::map<std::string, bool> m_captured;
  std::vector<std::string> m_global_nets;
  std::set<const db::Circuit *> m_circuits_read;

  void push_stream (const std::string &path);
  void pop_stream ();
  bool at_end ();
  void read_pin_and_parameters (tl::Extractor &ex, std::vector<std::string> &nn, std::map<std::string, double> &pv);
  bool read_element (tl::Extractor &ex, const std::string &element, const std::string &name);
  void read_subcircuit (const std::string &sc_name, const std::string &nc_name, const std::vector<db::Net *> &nets);
  void read_circuit (tl::Extractor &ex, const std::string &name);
  void skip_circuit (tl::Extractor &ex);
  bool read_card ();
  double read_value (tl::Extractor &ex);
  std::string read_name_with_case (tl::Extractor &ex);
  std::string read_name (tl::Extractor &ex);
  double read_atomic_value (tl::Extractor &ex);
  double read_dot_expr (tl::Extractor &ex);
  double read_bar_expr (tl::Extractor &ex);
  std::string get_line ();
  void unget_line (const std::string &l);
  void error (const std::string &msg);
  void warn (const std::string &msg);
  void finish ();
  db::Net *make_net (const std::string &name);
  void ensure_circuit ();
  bool subcircuit_captured (const std::string &nc_name);
};

}

namespace tl
{

template <>
struct type_traits<db::NetlistSpiceReader>
  : public tl::type_traits<void>
{
  typedef tl::false_tag has_default_constructor;
  typedef tl::false_tag has_copy_constructor;
};

}

#endif
