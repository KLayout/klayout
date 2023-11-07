
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

#ifndef _HDR_dbNet
#define _HDR_dbNet

#include "dbCommon.h"
#include "dbNetlistObject.h"
#include "dbMemStatistics.h"

#include "tlObject.h"

#include <list>
#include <string>

namespace db
{

class Device;
class Net;
class SubCircuit;
class Circuit;
class DeviceTerminalDefinition;
class DeviceClass;
class Pin;
class Netlist;

/**
 *  @brief A reference to a terminal of a device
 *
 *  A terminal must always refer to a device inside the current circuit.
 */
class DB_PUBLIC NetTerminalRef
{
public:
  /**
   *  @brief Default constructor
   */
  NetTerminalRef ();

  /**
   *  @brief Creates a pin reference to the given pin of the current circuit
   */
  NetTerminalRef (Device *device, size_t terminal_id);

  /**
   *  @brief Copy constructor
   */
  NetTerminalRef (const NetTerminalRef &other);

  /**
   *  @brief Assignment
   */
  NetTerminalRef &operator= (const NetTerminalRef &other);

  /**
   *  @brief Comparison
   */
  bool operator< (const NetTerminalRef &other) const
  {
    if (mp_device != other.mp_device) {
      return mp_device < other.mp_device;
    }
    return m_terminal_id < other.m_terminal_id;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const NetTerminalRef &other) const
  {
    return (mp_device == other.mp_device && m_terminal_id == other.m_terminal_id);
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
   *  @brief Gets the terminal index
   */
  size_t terminal_id () const
  {
    return m_terminal_id;
  }

  /**
   *  @brief Gets the terminal definition
   *
   *  Returns 0 if the terminal is not a valid terminal reference.
   */
  const DeviceTerminalDefinition *terminal_def () const;

  /**
   *  @brief Returns the device class
   */
  const DeviceClass *device_class () const;

  /**
   *  @brief Gets the net the terminal lives in
   */
  Net *net ()
  {
    return mp_net;
  }

  /**
   *  @brief Gets the net the terminal lives in (const version)
   */
  const Net *net () const
  {
    return mp_net;
  }

private:
  friend class Net;

  size_t m_terminal_id;
  Device *mp_device;
  Net *mp_net;

  /**
   *  @brief Sets the net the terminal lives in
   */
  void set_net (Net *net)
  {
    mp_net = net;
  }
};

/**
 *  @brief A reference to a pin inside a net
 *
 *  This object describes a connection to an outgoing pin.
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
    return m_pin_id < other.m_pin_id;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const NetPinRef &other) const
  {
    return (m_pin_id == other.m_pin_id);
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
  friend class Circuit;

  size_t m_pin_id;
  Net *mp_net;

  /**
   *  @brief Sets the net the terminal lives in
   */
  void set_net (Net *net)
  {
    mp_net = net;
  }

  /**
   *  @brief Sets the pin ID
   */
  void set_pin_id (size_t id)
  {
    m_pin_id = id;
  }
};

/**
 *  @brief A reference to a pin inside a net
 *
 *  This object describes a connection to a pin of a subcircuit.
 */
class DB_PUBLIC NetSubcircuitPinRef
{
public:
  /**
   *  @brief Default constructor
   */
  NetSubcircuitPinRef ();

  /**
   *  @brief Creates a pin reference to the given pin of the given subcircuit
   */
  NetSubcircuitPinRef (SubCircuit *circuit, size_t pin_id);

  /**
   *  @brief Copy constructor
   */
  NetSubcircuitPinRef (const NetSubcircuitPinRef &other);

  /**
   *  @brief Assignment
   */
  NetSubcircuitPinRef &operator= (const NetSubcircuitPinRef &other);

  /**
   *  @brief Comparison
   */
  bool operator< (const NetSubcircuitPinRef &other) const
  {
    if (mp_subcircuit != other.mp_subcircuit) {
      return mp_subcircuit < other.mp_subcircuit;
    }
    return m_pin_id < other.m_pin_id;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const NetSubcircuitPinRef &other) const
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
  friend class SubCircuit;

  size_t m_pin_id;
  SubCircuit *mp_subcircuit;
  Net *mp_net;

  /**
   *  @brief Sets the net the terminal lives in
   */
  void set_net (Net *net)
  {
    mp_net = net;
  }

  /**
   *  @brief Sets the pin ID
   */
  void set_pin_id (size_t pin)
  {
    m_pin_id = pin;
  }
};

/**
 *  @brief A net
 *
 *  A net connects terminals of devices and pins or circuits
 */
class DB_PUBLIC Net
  : public db::NetlistObject
{
public:
  typedef std::list<NetTerminalRef> terminal_list;
  typedef terminal_list::const_iterator const_terminal_iterator;
  typedef terminal_list::iterator terminal_iterator;
  typedef std::list<NetPinRef> pin_list;
  typedef pin_list::const_iterator const_pin_iterator;
  typedef pin_list::iterator pin_iterator;
  typedef std::list<NetSubcircuitPinRef> subcircuit_pin_list;
  typedef subcircuit_pin_list::const_iterator const_subcircuit_pin_iterator;
  typedef subcircuit_pin_list::iterator subcircuit_pin_iterator;

  /**
   *  @brief Constructor
   *  Creates an empty circuit.
   */
  Net ();

  /**
   *  @brief Creates a empty net with the give name
   */
  Net (const std::string &name);

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
   *  @brief Gets the netlist the net lives in
   *  This pointer is 0 if the net is not part of a netlist.
   */
  Netlist *netlist ();

  /**
   *  @brief Gets the netlist the net lives in (const version)
   *  This pointer is 0 if the net is not part of a netlist.
   */
  const Netlist *netlist () const;

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
   *  @brief Gets the expanded name
   *
   *  The "expanded name" is a non-empty name for the net. It uses the
   *  cluster ID if no name is set.
   */
  std::string expanded_name () const;

  /**
   *  @brief Gets the qualified name
   *
   *  The qualified name is like the expanded name, but preceded with the
   *  Circuit name if known (e.g. "CIRCUIT:NET")
   */
  std::string qname () const;

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
   *  @brief Provided for API compatibility with the other objects
   *  This ID is not well-formed like for other objects.
   */
  size_t id () const
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
   *  @brief Adds a subcircuit pin to this net
   */
  void add_subcircuit_pin (const NetSubcircuitPinRef &pin);

  /**
   *  @brief Erases the given subcircuit pin from this net
   */
  void erase_subcircuit_pin (subcircuit_pin_iterator iter);

  /**
   *  @brief Begin iterator for the pins of the net (const version)
   */
  const_subcircuit_pin_iterator begin_subcircuit_pins () const
  {
    return m_subcircuit_pins.begin ();
  }

  /**
   *  @brief End iterator for the pins of the net (const version)
   */
  const_subcircuit_pin_iterator end_subcircuit_pins () const
  {
    return m_subcircuit_pins.end ();
  }

  /**
   *  @brief Begin iterator for the pins of the net (non-const version)
   */
  subcircuit_pin_iterator begin_subcircuit_pins ()
  {
    return m_subcircuit_pins.begin ();
  }

  /**
   *  @brief End iterator for the pins of the net (non-const version)
   */
  subcircuit_pin_iterator end_subcircuit_pins ()
  {
    return m_subcircuit_pins.end ();
  }

  /**
   *  @brief Adds a terminal to this net
   */
  void add_terminal (const NetTerminalRef &terminal);

  /**
   *  @brief Erases the given terminal from this net
   */
  void erase_terminal (terminal_iterator iter);

  /**
   *  @brief Begin iterator for the terminals of the net (const version)
   */
  const_terminal_iterator begin_terminals () const
  {
    return m_terminals.begin ();
  }

  /**
   *  @brief End iterator for the terminals of the net (const version)
   */
  const_terminal_iterator end_terminals () const
  {
    return m_terminals.end ();
  }

  /**
   *  @brief Begin iterator for the terminals of the net (non-const version)
   */
  terminal_iterator begin_terminals ()
  {
    return m_terminals.begin ();
  }

  /**
   *  @brief End iterator for the terminals of the net (non-const version)
   */
  terminal_iterator end_terminals ()
  {
    return m_terminals.end ();
  }

  /**
   *  @brief Returns true, if the net is floating (there is no device, no subcircuit and no pin)
   */
  bool is_floating () const
  {
    return (m_subcircuit_pins.size () + m_terminals.size () + m_pins.size ()) < 1;
  }

  /**
   *  @brief Returns true, if the net is passive (there is no active element on the net)
   */
  bool is_passive () const
  {
    return (m_subcircuit_pins.size () + m_terminals.size ()) < 1;
  }

  /**
   *  @brief Returns true, if the net is an internal node (connects two terminals only)
   */
  bool is_internal () const
  {
    return m_pins.size () == 0 && m_subcircuit_pins.size () == 0 && m_terminals.size () == 2;
  }

  /**
   *  @brief Returns the number of outgoing pins connected
   */
  size_t pin_count () const
  {
    return m_pins.size ();
  }

  /**
   *  @brief Returns the number of subcircuit pins connected
   */
  size_t subcircuit_pin_count () const
  {
    return m_subcircuit_pins.size ();
  }

  /**
   *  @brief Returns the number of terminals connected
   */
  size_t terminal_count () const
  {
    return m_terminals.size ();
  }

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_terminals, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_pins, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_subcircuit_pins, true, (void *) this);
  }

private:
  friend class Circuit;

  terminal_list m_terminals;
  pin_list m_pins;
  subcircuit_pin_list m_subcircuit_pins;
  std::string m_name;
  size_t m_cluster_id;
  Circuit *mp_circuit;

  void set_circuit (Circuit *circuit);
};

/**
 *  @brief Memory statistics for Net
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const Net &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif
