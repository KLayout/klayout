
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#ifndef _HDR_dbDeviceClass
#define _HDR_dbDeviceClass

#include "dbCommon.h"

#include "gsiObject.h"
#include "tlObject.h"
#include "tlUniqueId.h"

#include <string>
#include <vector>

namespace db
{

class Netlist;
class Device;

/**
 *  @brief A device terminal definition
 */
class DB_PUBLIC DeviceTerminalDefinition
{
public:
  /**
   *  @brief Creates an empty device terminal definition
   */
  DeviceTerminalDefinition ()
    : m_name (), m_description (), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a device terminal definition with the given name and description
   */
  DeviceTerminalDefinition (const std::string &name, const std::string &description)
    : m_name (name), m_description (description), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the terminal name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the terminal name
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the terminal description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the terminal description
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the terminal ID
   */
  size_t id () const
  {
    return m_id;
  }

private:
  friend class DeviceClass;

  std::string m_name, m_description;
  size_t m_id;

  void set_id (size_t id)
  {
    m_id = id;
  }
};

/**
 *  @brief A device parameter definition
 */
class DB_PUBLIC DeviceParameterDefinition
{
public:
  /**
   *  @brief Creates an empty device parameter definition
   */
  DeviceParameterDefinition ()
    : m_name (), m_description (), m_default_value (0.0), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a device parameter definition with the given name and description
   */
  DeviceParameterDefinition (const std::string &name, const std::string &description, double default_value = 0.0)
    : m_name (name), m_description (description), m_default_value (default_value), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the parameter name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the parameter name
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the parameter description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the parameter description
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the parameter default value
   */
  double default_value () const
  {
    return m_default_value;
  }

  /**
   *  @brief Sets the parameter description
   */
  void set_default_value (double d)
  {
    m_default_value = d;
  }

  /**
   *  @brief Gets the parameter ID
   */
  size_t id () const
  {
    return m_id;
  }

private:
  friend class DeviceClass;

  std::string m_name, m_description;
  double m_default_value;
  size_t m_id;

  void set_id (size_t id)
  {
    m_id = id;
  }
};

/**
 *  @brief A device class
 *
 *  A device class describes a type of device.
 */
class DB_PUBLIC DeviceClass
  : public gsi::ObjectBase, public tl::Object, public tl::UniqueId
{
public:
  typedef size_t terminal_id_type;

  /**
   *  @brief Constructor
   *
   *  Creates an empty circuit.
   */
  DeviceClass ();

  /**
   *  @brief Copy constructor
   *  NOTE: do not use this copy constructor as the device class
   *  is intended to subclassing.
   */
  DeviceClass (const DeviceClass &other);

  /**
   *  @brief Assignment
   *  NOTE: do not use this copy constructor as the device class
   *  is intended to subclassing.
   */
  DeviceClass &operator= (const DeviceClass &other);

  /**
   *  @brief Gets the netlist the device class lives in
   */
  db::Netlist *netlist ()
  {
    return mp_netlist;
  }

  /**
   *  @brief Gets the netlist the device class lives in (const version)
   */
  const db::Netlist *netlist () const
  {
    return mp_netlist;
  }

  /**
   *  @brief Gets the name of the device class
   *
   *  The name is a formal name which identifies the class.
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the device name
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the description text for the device class
   *
   *  The description text is a human-readable text that
   *  identifies the device class.
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the description text
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the terminal definitions
   *
   *  The terminal definitions indicate what terminals the device offers.
   *  The number of terminals is constant per class. The index of the terminal
   *  is used as an ID of the terminal, hence the order must be static.
   */
  const std::vector<DeviceTerminalDefinition> &terminal_definitions () const
  {
    return m_terminal_definitions;
  }

  /**
   *  @brief Adds a terminal definition
   */
  const DeviceTerminalDefinition &add_terminal_definition (const DeviceTerminalDefinition &pd);

  /**
   *  @brief Clears the terminal definition
   */
  void clear_terminal_definitions ();

  /**
   *  @brief Gets the terminal definition from the ID
   */
  const DeviceTerminalDefinition *terminal_definition (size_t id) const;

  /**
   *  @brief Gets the parameter definitions
   */
  const std::vector<DeviceParameterDefinition> &parameter_definitions () const
  {
    return m_parameter_definitions;
  }

  /**
   *  @brief Adds a parameter definition
   */
  const DeviceParameterDefinition &add_parameter_definition (const DeviceParameterDefinition &pd);

  /**
   *  @brief Clears the parameter definition
   */
  void clear_parameter_definitions ();

  /**
   *  @brief Gets the parameter definition from the ID
   */
  const DeviceParameterDefinition *parameter_definition (size_t id) const;

  /**
   *  @brief Returns true, if the device has a parameter with the given name
   */
  bool has_parameter_with_name (const std::string &name) const;

  /**
   *  @brief Returns the parameter ID for the parameter with the given name
   *  If the name is invalid, an exception is thrown.
   */
  size_t parameter_id_for_name (const std::string &name) const;

  /**
   *  @brief Returns true, if the device has a terminal with the given name
   */
  bool has_terminal_with_name (const std::string &name) const;

  /**
   *  @brief Returns the parameter ID for the terminal with the given name
   *  If the name is invalid, an exception is thrown.
   */
  size_t terminal_id_for_name (const std::string &name) const;

  /**
   *  @brief Clears the circuit
   */
  virtual DeviceClass *clone () const
  {
    return new DeviceClass (*this);
  }

  /**
   *  @brief Combines two devices
   *
   *  This method shall test, whether the two devices can be combined. Both devices
   *  are guaranteed to share the same device class (this).
   *  If they cannot be combined, this method shall do nothing and return false.
   *  If they can be combined, this method shall reconnect the nets of the first
   *  device and entirely disconnect the nets of the second device.
   *  The second device will be deleted afterwards.
   */
  virtual bool combine_devices (db::Device * /*a*/, db::Device * /*b*/) const
  {
    return false;
  }

  /**
   *  @brief Returns true if the device class supports device combination in parallel mode
   */
  virtual bool supports_parallel_combination () const
  {
    return false;
  }

  /**
   *  @brief Returns true if the device class supports device combination in serial mode
   */
  virtual bool supports_serial_combination () const
  {
    return false;
  }

private:
  friend class Netlist;

  std::string m_name, m_description;
  std::vector<DeviceTerminalDefinition> m_terminal_definitions;
  std::vector<DeviceParameterDefinition> m_parameter_definitions;
  db::Netlist *mp_netlist;

  void set_netlist (db::Netlist *nl)
  {
    mp_netlist = nl;
  }
};

}

#endif
