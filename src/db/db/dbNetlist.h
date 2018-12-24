
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#ifndef _HDR_dbNetlist
#define _HDR_dbNetlist

#include "dbCommon.h"
#include "dbTypes.h"
#include "dbTrans.h"
#include "tlObjectCollection.h"
#include "tlVector.h"
#include "tlUniqueId.h"
#include "gsiObject.h"

#include <list>
#include <map>
#include <string>


namespace db
{

class Circuit;
class SubCircuit;
class Pin;
class Device;
class DeviceClass;
class DevicePortDefinition;
class Netlist;
class Net;

/**
 *  @brief A reference to a port of a device
 *
 *  A port must always refer to a device inside the current circuit.
 */
class DB_PUBLIC NetPortRef
{
public:
  /**
   *  @brief Default constructor
   */
  NetPortRef ();

  /**
   *  @brief Creates a pin reference to the given pin of the current circuit
   */
  NetPortRef (Device *device, size_t port_id);

  /**
   *  @brief Copy constructor
   */
  NetPortRef (const NetPortRef &other);

  /**
   *  @brief Assignment
   */
  NetPortRef &operator= (const NetPortRef &other);

  /**
   *  @brief Comparison
   */
  bool operator< (const NetPortRef &other) const
  {
    if (mp_device != other.mp_device) {
      return mp_device < other.mp_device;
    }
    return m_port_id < other.m_port_id;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const NetPortRef &other) const
  {
    return (mp_device == other.mp_device && m_port_id == other.m_port_id);
  }

  /**
   *  @brief Gets the device reference
   */
  Device *device ()
  {
    return mp_device;
  }

  /**
   *  @brief Gets the device reference (const version)
   */
  const Device *device () const
  {
    return mp_device;
  }

  /**
   *  @brief Gets the port index
   */
  size_t port_id () const
  {
    return m_port_id;
  }

  /**
   *  @brief Gets the port definition
   *
   *  Returns 0 if the port is not a valid port reference.
   */
  const DevicePortDefinition *port_def () const;

  /**
   *  @brief Returns the device class
   */
  const DeviceClass *device_class () const;

  /**
   *  @brief Gets the net the port lives in
   */
  Net *net ()
  {
    return mp_net;
  }

  /**
   *  @brief Gets the net the port lives in (const version)
   */
  const Net *net () const
  {
    return mp_net;
  }

private:
  friend class Net;

  size_t m_port_id;
  Device *mp_device;
  Net *mp_net;

  /**
   *  @brief Sets the net the port lives in
   */
  void set_net (Net *net)
  {
    mp_net = net;
  }
};

/**
 *  @brief A reference to a pin inside a net
 *
 *  A pin belongs to a subcircuit.
 *  If the subcircuit reference is 0, the pin is a pin of the current circuit
 *  (upward pin).
 */
class DB_PUBLIC NetPinRef
{
public:
  /**
   *  @brief Default constructor
   */
  NetPinRef ();

  /**
   *  @brief Creates a pin reference to the given pin of the current circuit
   */
  NetPinRef (size_t pin_id);

  /**
   *  @brief Creates a pin reference to the given pin of the given subcircuit
   */
  NetPinRef (SubCircuit *circuit, size_t pin_id);

  /**
   *  @brief Copy constructor
   */
  NetPinRef (const NetPinRef &other);

  /**
   *  @brief Assignment
   */
  NetPinRef &operator= (const NetPinRef &other);

  /**
   *  @brief Comparison
   */
  bool operator< (const NetPinRef &other) const
  {
    if (mp_subcircuit != other.mp_subcircuit) {
      return mp_subcircuit < other.mp_subcircuit;
    }
    return m_pin_id < other.m_pin_id;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const NetPinRef &other) const
  {
    return (mp_subcircuit == other.mp_subcircuit && m_pin_id == other.m_pin_id);
  }

  /**
   *  @brief Gets the pin reference (const version)
   */
  size_t pin_id () const
  {
    return m_pin_id;
  }

  /**
   *  @brief Gets the pin reference from the pin id
   *  If the pin cannot be resolved, null is returned.
   */
  const Pin *pin () const;

  /**
   *  @brief Gets the subcircuit reference
   */
  SubCircuit *subcircuit ()
  {
    return mp_subcircuit;
  }

  /**
   *  @brief Gets the subcircuit reference (const version)
   */
  const SubCircuit *subcircuit () const
  {
    return mp_subcircuit;
  }

  /**
   *  @brief Gets the net the pin lives in
   */
  Net *net ()
  {
    return mp_net;
  }

  /**
   *  @brief Gets the net the pin lives in (const version)
   */
  const Net *net () const
  {
    return mp_net;
  }

private:
  friend class Net;

  size_t m_pin_id;
  SubCircuit *mp_subcircuit;
  Net *mp_net;

  /**
   *  @brief Sets the net the port lives in
   */
  void set_net (Net *net)
  {
    mp_net = net;
  }
};

/**
 *  @brief A net
 *
 *  A net connects ports of devices and pins or circuits
 */
class DB_PUBLIC Net
  : public tl::Object
{
public:
  typedef std::list<NetPortRef> port_list;
  typedef port_list::const_iterator const_port_iterator;
  typedef port_list::iterator port_iterator;
  typedef std::list<NetPinRef> pin_list;
  typedef pin_list::const_iterator const_pin_iterator;
  typedef pin_list::iterator pin_iterator;

  /**
   *  @brief Constructor
   *  Creates an empty circuit.
   */
  Net ();

  /**
   *  @brief Copy constructor
   */
  Net (const Net &other);

  /**
   *  @brief Destructor
   */
  ~Net ();

  /**
   *  @brief Assignment
   */
  Net &operator= (const Net &other);

  /**
   *  @brief Gets the circuit the net lives in
   *  This pointer is 0 if the net is not part of a circuit.
   */
  Circuit *circuit ()
  {
    return mp_circuit;
  }

  /**
   *  @brief Gets the circuit the net lives in (const version)
   *  This pointer is 0 if the net is not part of a circuit.
   */
  const Circuit *circuit () const
  {
    return mp_circuit;
  }

  /**
   *  @brief Clears the circuit
   */
  void clear ();

  /**
   *  @brief Sets the name of the circuit
   */
  void set_name (const std::string &name);

  /**
   *  @brief Gets the name of the circuit
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the cluster ID of this net
   *
   *  The cluster ID links the net to a cluster from the
   *  hierarchical layout netlist extractor.
   */
  void set_cluster_id (size_t ci);

  /**
   *  @brief Gets the cluster ID
   */
  size_t cluster_id () const
  {
    return m_cluster_id;
  }

  /**
   *  @brief Adds a pin to this net
   */
  void add_pin (const NetPinRef &pin);

  /**
   *  @brief Erases the given pin from this net
   */
  void erase_pin (pin_iterator iter);

  /**
   *  @brief Begin iterator for the pins of the net (const version)
   */
  const_pin_iterator begin_pins () const
  {
    return m_pins.begin ();
  }

  /**
   *  @brief End iterator for the pins of the net (const version)
   */
  const_pin_iterator end_pins () const
  {
    return m_pins.end ();
  }

  /**
   *  @brief Begin iterator for the pins of the net (non-const version)
   */
  pin_iterator begin_pins ()
  {
    return m_pins.begin ();
  }

  /**
   *  @brief End iterator for the pins of the net (non-const version)
   */
  pin_iterator end_pins ()
  {
    return m_pins.end ();
  }

  /**
   *  @brief Adds a port to this net
   */
  void add_port (const NetPortRef &port);

  /**
   *  @brief Erases the given port from this net
   */
  void erase_port (port_iterator iter);

  /**
   *  @brief Begin iterator for the ports of the net (const version)
   */
  const_port_iterator begin_ports () const
  {
    return m_ports.begin ();
  }

  /**
   *  @brief End iterator for the ports of the net (const version)
   */
  const_port_iterator end_ports () const
  {
    return m_ports.end ();
  }

  /**
   *  @brief Begin iterator for the ports of the net (non-const version)
   */
  port_iterator begin_ports ()
  {
    return m_ports.begin ();
  }

  /**
   *  @brief End iterator for the ports of the net (non-const version)
   */
  port_iterator end_ports ()
  {
    return m_ports.end ();
  }

  /**
   *  @brief Returns true, if the net is floating (has no or only a single connection)
   */
  bool floating () const
  {
    return (m_pins.size () + m_ports.size ()) < 2;
  }

private:
  friend class Circuit;

  port_list m_ports;
  pin_list m_pins;
  std::string m_name;
  size_t m_cluster_id;
  Circuit *mp_circuit;

  void set_circuit (Circuit *circuit);
};

/**
 *  @brief The definition of a pin of a circuit
 *
 *  A pin is some place other nets can connect to a circuit.
 */
class DB_PUBLIC Pin
{
public:
  /**
   *  @brief Default constructor
   */
  Pin ();

  /**
   *  @brief Creates a pin with the given name.
   */
  Pin (const std::string &name);

  /**
   *  @brief Gets the name of the pin
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Gets the ID of the pin (only pins inside circuits have valid ID's)
   */
  size_t id () const
  {
    return m_id;
  }

private:
  friend class Circuit;

  tl::weak_ptr<Circuit> m_circuit;
  std::string m_name;
  size_t m_id;

  void set_id (size_t id)
  {
    m_id = id;
  }
};

/**
 *  @brief An actual device
 *
 *  This class represents the incarnation of a specific device.
 *  The device has a class which specifies a type. This class
 *  is intended for subclassing.
 *  A specific device subclass is supposed to correspond to
 *  a specific device class.
 */
class DB_PUBLIC Device
  : public tl::Object
{
public:
  /**
   *  @brief Default constructor
   */
  Device ();

  /**
   *  @brief The constructor
   */
  Device (DeviceClass *device_class, const std::string &name = std::string ());

  /**
   *  @brief Copy constructor
   */
  Device (const Device &other);

  /**
   *  @brief Assignment
   */
  Device &operator= (const Device &other);

  /**
   *  @brief Destructor
   */
  ~Device ();

  /**
   *  @brief Gets the device class
   */
  const DeviceClass *device_class () const
  {
    return mp_device_class;
  }

  /**
   *  @brief Sets the name
   */
  void set_name (const std::string &n);

  /**
   *  @brief Gets the name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Gets the net attached to a specific port
   *  Returns 0 if no net is attached.
   */
  const Net *net_for_port (size_t port_id) const;

  /**
   *  @brief Gets the net attached to a specific port (non-const version)
   *  Returns 0 if no net is attached.
   */
  Net *net_for_port (size_t port_id)
  {
    return const_cast<Net *> (((const Device *) this)->net_for_port (port_id));
  }

  /**
   *  @brief Connects the given port to the given net
   *  If the net is 0 the port is disconnected.
   *  If non-null, a NetPortRef object will be inserted into the
   *  net and connected with the given port.
   */
  void connect_port (size_t port_id, Net *net);

  /**
   *  @brief Gets the value for the parameter with the given ID
   */
  double parameter_value (size_t param_id) const;

  /**
   *  @brief Sets the value for the parameter with the given ID
   */
  void set_parameter_value (size_t param_id, double v);

private:
  friend class Circuit;
  friend class Net;

  DeviceClass *mp_device_class;
  std::string m_name;
  std::vector<Net::port_iterator> m_port_refs;
  std::vector<double> m_parameters;

  /**
   *  @brief Sets the port reference for a specific port
   */
  void set_port_ref_for_port (size_t port_id, Net::port_iterator iter);

  /**
   *  @brief Sets the device class
   */
  void set_device_class (DeviceClass *dc)
  {
    mp_device_class = dc;
  }
};

/**
 *  @brief A subcircuit of a circuit
 *
 *  This class essentially is a reference to another circuit
 */
class DB_PUBLIC SubCircuit
  : public tl::Object
{
public:
  typedef tl::vector<const Net *> connected_net_list;

  /**
   *  @brief Default constructor
   */
  SubCircuit ();

  /**
   *  @brief Copy constructor
   */
  SubCircuit (const SubCircuit &other);

  /**
   *  @brief Assignment
   */
  SubCircuit &operator= (const SubCircuit &other);

  /**
   *  @brief Creates a subcircuit reference to the given circuit
   */
  SubCircuit (Circuit *circuit, const std::string &name = std::string ());

  /**
   *  @brief Destructor
   */
  ~SubCircuit ();

  /**
   *  @brief Gets the circuit the reference points to (const version)
   */
  const Circuit *circuit () const
  {
    return m_circuit.get ();
  }

  /**
   *  @brief Gets the circuit the reference points to (non-const version)
   */
  Circuit *circuit ()
  {
    return m_circuit.get ();
  }

  /**
   *  @brief Sets the name of the subcircuit
   *
   *  The name is one way to identify the subcircuit. The transformation is
   *  another one.
   */
  void set_name (const std::string &n);

  /**
   *  @brief Gets the name of the subcircuit
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the transformation describing the subcircuit
   *
   *  The transformation is a natural description of a subcircuit
   *  (in contrast to the name) when deriving it from a layout.
   */
  void set_trans (const db::DCplxTrans &trans);

  /**
   *  @brief Gets the transformation describing the subcircuit
   */
  const db::DCplxTrans &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Gets the net attached to a specific pin
   *  Returns 0 if no net is attached.
   */
  const Net *net_for_pin (size_t pin_id) const;

  /**
   *  @brief Gets the net attached to a specific pin (non-const version)
   *  Returns 0 if no net is attached.
   */
  Net *net_for_pin (size_t pin_id)
  {
    return const_cast<Net *> (((const SubCircuit *) this)->net_for_pin (pin_id));
  }

  /**
   *  @brief Connects the given pin to the given net
   *  If the net is 0 the pin is disconnected.
   *  If non-null, a NetPinRef object will be inserted into the
   *  net and connected with the given pin.
   */
  void connect_pin (size_t pin_id, Net *net);

private:
  friend class Circuit;
  friend class Net;

  tl::weak_ptr<Circuit> m_circuit;
  std::string m_name;
  db::DCplxTrans m_trans;
  std::vector<Net::pin_iterator> m_pin_refs;

  /**
   *  @brief Sets the pin reference for a specific pin
   */
  void set_pin_ref_for_pin (size_t ppin_id, Net::pin_iterator iter);

  /**
   *  @brief Sets the circuit reference
   */
  void set_circuit (Circuit *c)
  {
    m_circuit.reset (c);
  }
};

/**
 *  @brief A circuit
 *
 *  A circuit is a list of nets, of subcircuit references and actual
 *  devices.
 */
class DB_PUBLIC Circuit
  : public gsi::ObjectBase, public tl::Object
{
public:
  typedef tl::vector<Pin> pin_list;
  typedef pin_list::const_iterator const_pin_iterator;
  typedef pin_list::iterator pin_iterator;
  typedef tl::shared_collection<Device> device_list;
  typedef device_list::const_iterator const_device_iterator;
  typedef device_list::iterator device_iterator;
  typedef tl::shared_collection<Net> net_list;
  typedef net_list::const_iterator const_net_iterator;
  typedef net_list::iterator net_iterator;
  typedef tl::shared_collection<SubCircuit> sub_circuit_list;
  typedef sub_circuit_list::const_iterator const_sub_circuit_iterator;
  typedef sub_circuit_list::iterator sub_circuit_iterator;

  /**
   *  @brief Constructor
   *
   *  Creates an empty circuit.
   */
  Circuit ();

  /**
   *  @brief Copy constructor
   */
  Circuit (const Circuit &other);

  /**
   *  @brief Assignment
   */
  Circuit &operator= (const Circuit &other);

  /**
   *  @brief Gets the netlist the circuit lives in
   *  This pointer is 0 if the circuit is not part of a netlist.
   */
  Netlist *netlist ()
  {
    return mp_netlist;
  }

  /**
   *  @brief Gets the netlist the circuit lives in (const version)
   *  This pointer is 0 if the circuit is not part of a netlist.
   */
  const Netlist *netlist () const
  {
    return mp_netlist;
  }

  /**
   *  @brief Clears the circuit
   */
  void clear ();

  /**
   *  @brief Sets the name of the circuit
   */
  void set_name (const std::string &name);

  /**
   *  @brief Gets the name of the circuit
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the layout cell reference for this circuit
   *
   *  The layout cell reference links a circuit to a layout cell.
   */
  void set_cell_index (const db::cell_index_type ci);

  /**
   *  @brief Gets the layout cell index
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Adds a pin to this circuit
   *
   *  The circuit takes over ownership of the object.
   */
  const Pin &add_pin(const Pin &pin);

  /**
   *  @brief Begin iterator for the pins of the circuit (non-const version)
   */
  pin_iterator begin_pins ()
  {
    return m_pins.begin ();
  }

  /**
   *  @brief End iterator for the pins of the circuit (non-const version)
   */
  pin_iterator end_pins ()
  {
    return m_pins.end ();
  }

  /**
   *  @brief Gets the pin count
   */
  size_t pin_count () const
  {
    return m_pins.size ();
  }

  /**
   *  @brief Gets the pin by ID (the ID is basically the index)
   */
  const Pin *pin_by_id (size_t id) const;

  /**
   *  @brief Begin iterator for the pins of the circuit (const version)
   */
  const_pin_iterator begin_pins () const
  {
    return m_pins.begin ();
  }

  /**
   *  @brief End iterator for the pins of the circuit (const version)
   */
  const_pin_iterator end_pins () const
  {
    return m_pins.end ();
  }

  /**
   *  @brief Adds a net to this circuit
   *
   *  The circuit takes over ownership of the object.
   */
  void add_net (Net *net);

  /**
   *  @brief Deletes a net from the circuit
   */
  void remove_net (Net *net);

  /**
   *  @brief Begin iterator for the nets of the circuit (non-const version)
   */
  net_iterator begin_nets ()
  {
    return m_nets.begin ();
  }

  /**
   *  @brief End iterator for the nets of the circuit (non-const version)
   */
  net_iterator end_nets ()
  {
    return m_nets.end ();
  }

  /**
   *  @brief Begin iterator for the nets of the circuit (const version)
   */
  const_net_iterator begin_nets () const
  {
    return m_nets.begin ();
  }

  /**
   *  @brief End iterator for the nets of the circuit (const version)
   */
  const_net_iterator end_nets () const
  {
    return m_nets.end ();
  }

  /**
   *  @brief Adds a device to this circuit
   *
   *  The circuit takes over ownership of the object.
   */
  void add_device (Device *device);

  /**
   *  @brief Deletes a device from the circuit
   */
  void remove_device (Device *device);

  /**
   *  @brief Begin iterator for the devices of the circuit (non-const version)
   */
  device_iterator begin_devices ()
  {
    return m_devices.begin ();
  }

  /**
   *  @brief End iterator for the devices of the circuit (non-const version)
   */
  device_iterator end_devices ()
  {
    return m_devices.end ();
  }

  /**
   *  @brief Begin iterator for the devices of the circuit (const version)
   */
  const_device_iterator begin_devices () const
  {
    return m_devices.begin ();
  }

  /**
   *  @brief End iterator for the devices of the circuit (const version)
   */
  const_device_iterator end_devices () const
  {
    return m_devices.end ();
  }

  /**
   *  @brief Adds a subcircuit to this circuit
   *
   *  The circuit takes over ownership of the object.
   */
  void add_sub_circuit (SubCircuit *sub_circuit);

  /**
   *  @brief Deletes a subcircuit from the circuit
   */
  void remove_sub_circuit (SubCircuit *sub_circuit);

  /**
   *  @brief Begin iterator for the subcircuits of the circuit (non-const version)
   */
  sub_circuit_iterator begin_sub_circuits ()
  {
    return m_sub_circuits.begin ();
  }

  /**
   *  @brief End iterator for the subcircuits of the circuit (non-const version)
   */
  sub_circuit_iterator end_sub_circuits ()
  {
    return m_sub_circuits.end ();
  }

  /**
   *  @brief Begin iterator for the subcircuits of the circuit (const version)
   */
  const_sub_circuit_iterator begin_sub_circuits () const
  {
    return m_sub_circuits.begin ();
  }

  /**
   *  @brief End iterator for the subcircuits of the circuit (const version)
   */
  const_sub_circuit_iterator end_sub_circuits () const
  {
    return m_sub_circuits.end ();
  }

  /**
   *  @brief Gets the connected net for a pin with the given id
   *
   *  Returns 0 if the pin is not connected to a net.
   */
  const Net *net_for_pin (size_t pin_id) const;

  /**
   *  @brief Gets the connected net for a pin with the given id (non-const version)
   *
   *  Returns 0 if the pin is not connected to a net.
   */
  Net *net_for_pin (size_t pin_id)
  {
    return const_cast<Net *> (((const Circuit *) this)->net_for_pin (pin_id));
  }

  /**
   *  @brief Connects the given pin to the given net
   *  If the net is 0 the pin is disconnected.
   *  If non-null, a NetPinRef object will be inserted into the
   *  net and connected with the given pin.
   */
  void connect_pin (size_t pin_id, Net *net);

  /**
   *  @brief Purge unused nets
   *
   *  This method will purge all nets which return "floating".
   */
  void purge_nets ();

  /**
   *  @brief Combine devices
   *
   *  This method will combine devices that can be combined according
   *  to their device classes "combine_devices" method.
   */
  void combine_devices ();

private:
  friend class Netlist;
  friend class Net;

  std::string m_name;
  db::cell_index_type m_cell_index;
  net_list m_nets;
  pin_list m_pins;
  device_list m_devices;
  sub_circuit_list m_sub_circuits;
  Netlist *mp_netlist;
  std::vector<Net::pin_iterator> m_pin_refs;

  /**
   *  @brief Sets the pin reference for a specific pin
   */
  void set_pin_ref_for_pin (size_t ppin_id, Net::pin_iterator iter);

  void translate_circuits (const std::map<const Circuit *, Circuit *> &map);
  void translate_device_classes (const std::map<const DeviceClass *, DeviceClass *> &map);
  void set_netlist (Netlist *netlist);
  void combine_parallel_devices (const db::DeviceClass &cls);
  void combine_serial_devices (const db::DeviceClass &cls);
};

/**
 *  @brief A device port definition
 */
class DB_PUBLIC DevicePortDefinition
{
public:
  /**
   *  @brief Creates an empty device port definition
   */
  DevicePortDefinition ()
    : m_name (), m_description (), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a device port definition with the given name and description
   */
  DevicePortDefinition (const std::string &name, const std::string &description)
    : m_name (name), m_description (description), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the port name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the port name
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the port description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the port description
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the port ID
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
  typedef size_t port_id_type;

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
   *  @brief Clears the circuit
   */
  virtual DeviceClass *clone () const
  {
    return new DeviceClass (*this);
  }

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
  virtual const std::string &name () const;

  /**
   *  @brief Gets the description text for the device class
   *
   *  The description text is a human-readable text that
   *  identifies the device class.
   */
  virtual const std::string &description () const;

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
   *  @brief Gets the port definitions
   *
   *  The port definitions indicate what ports the device offers.
   *  The number of ports is constant per class. The index of the port
   *  is used as an ID of the port, hence the order must be static.
   */
  const std::vector<DevicePortDefinition> &port_definitions () const
  {
    return m_port_definitions;
  }

  /**
   *  @brief Adds a port definition
   */
  const DevicePortDefinition &add_port_definition (const DevicePortDefinition &pd);

  /**
   *  @brief Clears the port definition
   */
  void clear_port_definitions ();

  /**
   *  @brief Gets the port definition from the ID
   */
  const DevicePortDefinition *port_definition (size_t id) const;

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

private:
  friend class Netlist;

  std::vector<DevicePortDefinition> m_port_definitions;
  std::vector<DeviceParameterDefinition> m_parameter_definitions;
  db::Netlist *mp_netlist;

  void set_netlist (db::Netlist *nl)
  {
    mp_netlist = nl;
  }
};

/**
 *  @brief A generic device class
 *
 *  The generic device class is a push version of the DeviceClassBase
 */
class DB_PUBLIC GenericDeviceClass
  : public DeviceClass
{
public:
  /**
   *  @brief Constructor
   *
   *  Creates an empty circuit.
   */
  GenericDeviceClass ();

  /**
   *  @brief Copy constructor
   */
  GenericDeviceClass (const GenericDeviceClass &other);

  /**
   *  @brief Assignment
   */
  GenericDeviceClass &operator= (const GenericDeviceClass &other);

  /**
   *  @brief Clears the circuit
   */
  virtual DeviceClass *clone () const
  {
    return new GenericDeviceClass (*this);
  }

  /**
   *  @brief Gets the name of the device class
   *
   *  The name is a formal name which identifies the class.
   */
  virtual const std::string &name () const
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
  virtual const std::string &description () const
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

private:
  std::string m_name, m_description;
};

/**
 *  @brief The netlist class
 *
 *  This class represents a hierarchical netlist.
 *  The main components of a netlist are circuits and device classes.
 *  The circuits represent cells, the device classes type of devices.
 */
class DB_PUBLIC Netlist
  : public gsi::ObjectBase, public tl::Object
{
public:
  typedef tl::shared_collection<Circuit> circuit_list;
  typedef circuit_list::const_iterator const_circuit_iterator;
  typedef circuit_list::iterator circuit_iterator;
  typedef tl::shared_collection<DeviceClass> device_class_list;
  typedef device_class_list::const_iterator const_device_class_iterator;
  typedef device_class_list::iterator device_class_iterator;

  /**
   *  @brief Constructor
   *
   *  This constructor creates an empty hierarchical netlist
   */
  Netlist ();

  /**
   *  @brief Copy constructor
   */
  Netlist (const Netlist &other);

  /**
   *  @brief Assignment
   */
  Netlist &operator= (const Netlist &other);

  /**
   *  @brief Clears the netlist
   */
  void clear ();

  /**
   *  @brief Adds a circuit to this netlist
   *
   *  The netlist takes over ownership of the object.
   */
  void add_circuit (Circuit *circuit);

  /**
   *  @brief Deletes a circuit from the netlist
   */
  void remove_circuit (Circuit *circuit);

  /**
   *  @brief Begin iterator for the circuits of the netlist (non-const version)
   */
  circuit_iterator begin_circuits ()
  {
    return m_circuits.begin ();
  }

  /**
   *  @brief End iterator for the circuits of the netlist (non-const version)
   */
  circuit_iterator end_circuits ()
  {
    return m_circuits.end ();
  }

  /**
   *  @brief Begin iterator for the circuits of the netlist (const version)
   */
  const_circuit_iterator begin_circuits () const
  {
    return m_circuits.begin ();
  }

  /**
   *  @brief End iterator for the circuits of the netlist (const version)
   */
  const_circuit_iterator end_circuits () const
  {
    return m_circuits.end ();
  }

  /**
   *  @brief Adds a device class to this netlist
   *
   *  The netlist takes over ownership of the object.
   */
  void add_device_class (DeviceClass *device_class);

  /**
   *  @brief Deletes a device class from the netlist
   */
  void remove_device_class (DeviceClass *device_class);

  /**
   *  @brief Begin iterator for the device classes of the netlist (non-const version)
   */
  device_class_iterator begin_device_classes ()
  {
    return m_device_classes.begin ();
  }

  /**
   *  @brief End iterator for the device classes of the netlist (non-const version)
   */
  device_class_iterator end_device_classes ()
  {
    return m_device_classes.end ();
  }

  /**
   *  @brief Begin iterator for the device classes of the netlist (const version)
   */
  const_device_class_iterator begin_device_classes () const
  {
    return m_device_classes.begin ();
  }

  /**
   *  @brief End iterator for the device classes of the netlist (const version)
   */
  const_device_class_iterator end_device_classes () const
  {
    return m_device_classes.end ();
  }

private:
  circuit_list m_circuits;
  device_class_list m_device_classes;
};

}

#endif
