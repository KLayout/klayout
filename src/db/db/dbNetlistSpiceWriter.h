
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

#ifndef HDR_dbNetlistSpiceWriter
#define HDR_dbNetlistSpiceWriter

#include "dbCommon.h"
#include "dbNetlistWriter.h"
#include "dbDeviceClass.h"
#include "tlObject.h"

#include <string>
#include <map>
#include <limits>

namespace db
{

class Device;
class Net;
class NetlistSpiceWriter;
class Circuit;
class SubCircuit;

/**
 *  @brief A device writer delegate for the SPICE writer
 *
 *  This delegate is supposed to provide the mapping of devices to parametrized SPICE subcircuits.
 *  It is generic, so it can be used for other cases of device mapping.
 */
class DB_PUBLIC NetlistSpiceWriterDelegate
  : public tl::Object
{
public:
  NetlistSpiceWriterDelegate ();
  virtual ~NetlistSpiceWriterDelegate ();

  bool write_all_parameters () const { return m_write_all_parameters; }
  void set_write_all_parameters (bool f) { m_write_all_parameters = f; }

  virtual void write_header () const;
  virtual void write_device_intro (const db::DeviceClass &cls) const;
  virtual void write_device (const db::Device &dev) const;

  std::string net_to_string (const db::Net *net) const;
  void emit_line (const std::string &line) const;
  void emit_comment (const std::string &comment) const;
  std::string format_name (const std::string &s) const;
  std::string format_terminals (const db::Device &dev, size_t max_terminals = std::numeric_limits<size_t>::max ()) const;
  std::string format_terminals_with_order (const db::Device &dev, const std::vector<std::string> &terminal_order) const;
  std::string format_params (const db::Device &dev, size_t without_id = std::numeric_limits<size_t>::max (), bool only_primary = false) const;

private:
  friend class NetlistSpiceWriter;

  NetlistSpiceWriter *mp_writer;
  const Netlist *mp_netlist;
  bool m_write_all_parameters;

  void attach_writer (NetlistSpiceWriter *writer, const Netlist *netlist);
  void write_device_default (const db::Device &dev) const;
  void write_device_profile (const db::Device &dev, const db::DeviceClass::SpiceProfile &profile) const;
};

/**
 *  @brief A SPICE format writer for netlists
 *
 *  Specialization happens through the device writer delegate.
 */
class DB_PUBLIC NetlistSpiceWriter
  : public NetlistWriter
{
public:
  NetlistSpiceWriter (NetlistSpiceWriterDelegate *delegate = 0);
  virtual ~NetlistSpiceWriter ();

  virtual void write (tl::OutputStream &stream, const db::Netlist &netlist, const std::string &description);

  /**
   *  @brief Sets the SPICE profile name
   */
  void set_profile (const std::string &profile);

  /**
   *  @brief Gets the SPICE profile name
   *
   *  The SPICE profile name is used to collect SPICE reader settings from the
   *  device classes.
   */
  const std::string &profile () const
  {
    return m_profile;
  }

  /**
   *  @brief Sets a string listing the allowed characters in names (beside alphanumeric)
   */
  void set_allowed_name_chars (const std::string &a);

  /**
   *  @brief Gets a string listing the allowed characters in names
   */
  const std::string &allowed_name_chars () const
  {
    return m_allowed_name_chars;
  }

  /**
   *  @brief Sets the prefix for "not connected" nets
   */
  void set_not_connect_prefix (const std::string &p);

  /**
   *  @brief Gets the prefix for "not connected" nets
   */
  const std::string &not_connect_prefix () const
  {
    return m_not_connect_prefix;
  }

  /**
   *  @brief Sets a value indicating whether to use net names
   */
  void set_use_net_names (bool use_net_names);

  /**
   *  @brief Gets a value indicating whether to use net names
   */
  bool use_net_names () const
  {
    return m_use_net_names;
  }

  /**
   *  @brief Sets a value indicating whether to use comments
   */
  void set_with_comments (bool f);

  /**
   *  @brief Gets a value indicating whether to use comments
   */
  bool with_comments () const
  {
    return m_with_comments;
  }

  /**
   *  @brief Gets the delegate
   */
  NetlistSpiceWriterDelegate *delegate ()
  {
    return mp_delegate.get ();
  }

private:
  friend class NetlistSpiceWriterDelegate;

  const db::Netlist *mp_netlist;
  tl::OutputStream *mp_stream;
  tl::weak_ptr<NetlistSpiceWriterDelegate> mp_delegate;
  std::map<const db::Net *, size_t> m_net_to_spice_id;
  std::map<const db::Net *, std::string> m_net_to_spice_name;
  mutable size_t m_next_net_id;
  bool m_use_net_names;
  bool m_with_comments;
  std::string m_profile;
  std::string m_allowed_name_chars;
  std::string m_not_connect_prefix;


  void do_write (const std::string &description);

  std::string net_to_string (const db::Net *net) const;
  void emit_line (const std::string &line) const;
  void emit_comment (const std::string &comment) const;
  std::string format_name (const std::string &name) const;
  void write_subcircuit_call (const db::SubCircuit &subcircuit) const;
  void write_circuit_header (const db::Circuit &circuit) const;
  void write_circuit_end (const db::Circuit &circuit) const;
};

}

#endif
