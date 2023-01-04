
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

#ifndef _HDR_dbDevice
#define _HDR_dbDevice

#include "dbCommon.h"
#include "dbNet.h"
#include "dbPoint.h"
#include "dbVector.h"
#include "dbTrans.h"
#include "dbMemStatistics.h"

#include "tlObject.h"

#include <vector>

namespace db
{

class Circuit;
class DeviceClass;
class DeviceAbstract;

/**
 *  @brief A structure describing a terminal reference into another device abstract
 */
struct DeviceReconnectedTerminal
{
  DeviceReconnectedTerminal (size_t _device_index, unsigned int _other_terminal_id)
    : device_index (_device_index), other_terminal_id (_other_terminal_id)
  {
    //  .. nothing yet ..
  }

  DeviceReconnectedTerminal ()
    : device_index (0), other_terminal_id (0)
  {
    //  .. nothing yet ..
  }

  size_t device_index;
  unsigned int other_terminal_id;
};

/**
 *  @brief A structure describing a reference to another device abstract
 *
 *  This structure is used within Device to reference more than the standard
 *  device abstract.
 */
struct DeviceAbstractRef
{
  DeviceAbstractRef (const db::DeviceAbstract *_device_abstract, const db::DCplxTrans &_trans)
    : device_abstract (_device_abstract), trans (_trans)
  {
    //  .. nothing yet ..
  }

  DeviceAbstractRef ()
    : device_abstract (0), trans ()
  {
    //  .. nothing yet ..
  }

  const db::DeviceAbstract *device_abstract;
  db::DCplxTrans trans;
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
  : public db::NetlistObject
{
public:
  typedef std::vector<std::pair<size_t, size_t> > global_connections;
  typedef global_connections::const_iterator global_connections_iterator;

  /**
   *  @brief Default constructor
   */
  Device ();

  /**
   *  @brief The constructor
   */
  Device (DeviceClass *device_class, const std::string &name = std::string ());

  /**
   *  @brief The constructor
   */
  Device (DeviceClass *device_class, DeviceAbstract *device_abstract, const std::string &name = std::string ());

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
   *  @brief Sets the device class
   */
  void set_device_class (DeviceClass *dc)
  {
    mp_device_class = dc;
  }

  /**
   *  @brief Gets the device abstract
   */
  const DeviceAbstract *device_abstract () const
  {
    return mp_device_abstract;
  }

  /**
   *  @brief Sets the device abstract
   */
  void set_device_abstract (DeviceAbstract *dm)
  {
    mp_device_abstract = dm;
  }

  /**
   *  @brief Gets the device ID
   *  The ID is a unique integer which identifies the device.
   *  It can be used to retrieve the device from the circuit using Circuit::device_by_id.
   *  When assigned, the device ID is not 0.
   */
  size_t id () const
  {
    return m_id;
  }

  /**
   *  @brief Gets the circuit the device lives in (const version)
   *  This pointer is 0 if the device isn't added to a circuit
   */
  const Circuit *circuit () const
  {
    return mp_circuit;
  }

  /**
   *  @brief Gets the circuit the device lives in (non-const version)
   *  This pointer is 0 if the device isn't added to a circuit
   */
  Circuit *circuit ()
  {
    return mp_circuit;
  }

  /**
   *  @brief Gets the netlist, the device lives in
   */
  const Netlist *netlist () const;

  /**
   *  @brief Gets the netlist, the device lives in
   */
  Netlist *netlist ();

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
   *  @brief Gets a name which always non-empty
   *  This method will pick a name like "$<id>" if the explicit name is empty.
   */
  std::string expanded_name () const;

  /**
   *  @brief Sets the device position
   *  The device position should be the center and orientation of the recognition shape or something similar.
   *  Giving the device a position allows combining multiple devices with the same
   *  relative geometry into a single cell.
   *  The transformation has to be given in micrometer units.
   */
  void set_trans (const db::DCplxTrans &tr);

  /**
   *  @brief Gets the device position
   */
  const db::DCplxTrans &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Gets the net attached to a specific terminal
   *  Returns 0 if no net is attached.
   */
  const Net *net_for_terminal (size_t terminal_id) const;

  /**
   *  @brief Gets the net attached to a specific terminal (non-const version)
   *  Returns 0 if no net is attached.
   */
  Net *net_for_terminal (size_t terminal_id)
  {
    return const_cast<Net *> (((const Device *) this)->net_for_terminal (terminal_id));
  }

  /**
   *  @brief Connects the given terminal to the given net
   *  If the net is 0 the terminal is disconnected.
   *  If non-null, a NetTerminalRef object will be inserted into the
   *  net and connected with the given terminal.
   *  If the terminal is connected to a global net, it will be
   *  disconnected from there.
   */
  void connect_terminal (size_t terminal_id, Net *net);

  /**
   *  @brief Gets the value for the parameter with the given ID
   */
  double parameter_value (size_t param_id) const;

  /**
   *  @brief Sets the value for the parameter with the given ID
   */
  void set_parameter_value (size_t param_id, double v);

  /**
   *  @brief Gets the value for the parameter with the given name
   *  If the name is not valid, an exception is thrown.
   */
  double parameter_value (const std::string &name) const;

  /**
   *  @brief Sets the value for the parameter with the given name
   *  If the name is not valid, an exception is thrown.
   */
  void set_parameter_value (const std::string &name, double v);

  /**
   *  @brief Used for device combination: join terminals with other device
   */
  void join_terminals (unsigned int this_terminal, db::Device *other, unsigned int other_terminal);

  /**
   *  @brief Used for device combination: reroute terminal to other device
   *
   *  This will disconnect "this_terminal" from the device and make a connection to
   *  "other_terminal" of the "other" device instead.
   *
   *  An internal connection between "this_terminal" and "from_other_terminal" is
   *  implied.
   */
  void reroute_terminal (unsigned int this_terminal, db::Device *other, unsigned int from_other_terminal, unsigned int other_terminal);

  /**
   *  @brief Gets the set of other terminal references
   *
   *  This method will return 0 if the device isn't a combined device or the given terminal
   *  is not connector to a different abstract.
   *
   *  The returned vector (if any) is a complete list of terminals connected to the given
   *  logical device terminal.
   */
  const std::vector<DeviceReconnectedTerminal> *reconnected_terminals_for (unsigned int this_terminal) const
  {
    std::map<unsigned int, std::vector<DeviceReconnectedTerminal> >::const_iterator t = m_reconnected_terminals.find (this_terminal);
    if (t != m_reconnected_terminals.end ()) {
      return & t->second;
    } else {
      return 0;
    }
  }

  /**
   *  @brief Gets the map of reconnected terminals
   */
  const std::map<unsigned int, std::vector<DeviceReconnectedTerminal> > &reconnected_terminals () const
  {
    return m_reconnected_terminals;
  }

  /**
   *  @brief Gets the map of reconnected terminals (non-const version)
   *
   *  NOTE: don't use this method to modify this container! It's provided for persistence implementation only.
   */
  std::map<unsigned int, std::vector<DeviceReconnectedTerminal> > &reconnected_terminals ()
  {
    return m_reconnected_terminals;
  }

  /**
   *  @brief Gets the set of other device abstracts
   *
   *  This list does not include the intrinsic original abstract of the device.
   *  This vector is non-empty if this device is a combined one.
   */
  const std::vector<DeviceAbstractRef> &other_abstracts () const
  {
    return m_other_abstracts;
  }

  /**
   *  @brief Gets the set of other device abstracts (non-const version)
   *
   *  NOTE: don't use this method to modify this container! It's provided for persistence implementation only.
   */
  std::vector<DeviceAbstractRef> &other_abstracts ()
  {
    return m_other_abstracts;
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
    db::mem_stat (stat, purpose, cat, m_trans, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_terminal_refs, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_parameters, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_other_abstracts, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_reconnected_terminals, true, (void *) this);
  }

private:
  friend class Circuit;
  friend class Net;

  DeviceClass *mp_device_class;
  DeviceAbstract *mp_device_abstract;
  std::string m_name;
  db::DCplxTrans m_trans;
  std::vector<Net::terminal_iterator> m_terminal_refs;
  std::vector<double> m_parameters;
  size_t m_id;
  Circuit *mp_circuit;
  std::vector<DeviceAbstractRef> m_other_abstracts;
  std::map<unsigned int, std::vector<DeviceReconnectedTerminal> > m_reconnected_terminals;

  /**
   * @brief Translates the device abstracts
   */
  void translate_device_abstracts (const std::map<const DeviceAbstract *, DeviceAbstract *> &map);

  /**
   *  @brief Joins this device with another
   */
  void join_device (db::Device *other);

  /**
   *  @brief Sets the terminal reference for a specific terminal
   */
  void set_terminal_ref_for_terminal (size_t terminal_id, Net::terminal_iterator iter);

  /**
   *  @brief Sets the device ID
   */
  void set_id (size_t id)
  {
    m_id = id;
  }

  /**
   *  @brief Sets the circuit
   */
  void set_circuit (Circuit *circuit);

  void add_others_terminals (unsigned int this_terminal, db::Device *other, unsigned int other_terminal);
  void init_terminal_routes ();
};

/**
 *  @brief Memory statistics for Device
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const Device &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif
