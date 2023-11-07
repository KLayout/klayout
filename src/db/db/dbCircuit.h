
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

#ifndef _HDR_dbCircuit
#define _HDR_dbCircuit

#include "dbCommon.h"
#include "dbTypes.h"
#include "dbNet.h"
#include "dbDevice.h"
#include "dbPin.h"
#include "dbSubCircuit.h"
#include "dbNetlistUtils.h"
#include "dbPolygon.h"
#include "dbMemStatistics.h"

#include "tlObject.h"
#include "tlObjectCollection.h"
#include "tlVector.h"
#include "gsiObject.h"

namespace db
{

class Netlist;
class Layout;

/**
 *  @brief An iterator wrapper for the child and parent circuit iterator
 */
template <class Iter, class Value>
struct DB_PUBLIC_TEMPLATE dereferencing_iterator
  : public Iter
{
public:
  typedef Value *pointer;
  typedef Value &reference;
  typedef typename Iter::difference_type difference_type;

  dereferencing_iterator () { }
  dereferencing_iterator (const dereferencing_iterator &d) : Iter (d) { }
  dereferencing_iterator (const Iter &d) : Iter (d) { }
  dereferencing_iterator &operator= (const dereferencing_iterator &d)
  {
    Iter::operator= (d);
    return *this;
  }

  dereferencing_iterator operator+ (difference_type offset) const
  {
    return dereferencing_iterator (Iter::operator+ (offset));
  }

  dereferencing_iterator &operator+= (difference_type offset)
  {
    Iter::operator+= (offset);
    return *this;
  }

  pointer operator-> () const { return Iter::operator* (); }
  reference operator* () const { return *Iter::operator* (); }
};

/**
 *  @brief A circuit
 *
 *  A circuit is a list of nets, of subcircuit references and actual
 *  devices.
 */
class DB_PUBLIC Circuit
  : public db::NetlistObject, public gsi::ObjectBase
{
public:
  typedef std::list<Pin> pin_list;
  typedef pin_list::const_iterator const_pin_iterator;
  typedef pin_list::iterator pin_iterator;
  typedef tl::shared_collection<Device> device_list;
  typedef device_list::const_iterator const_device_iterator;
  typedef device_list::iterator device_iterator;
  typedef tl::shared_collection<Net> net_list;
  typedef net_list::const_iterator const_net_iterator;
  typedef net_list::iterator net_iterator;
  typedef tl::shared_collection<SubCircuit> subcircuit_list;
  typedef subcircuit_list::const_iterator const_subcircuit_iterator;
  typedef subcircuit_list::iterator subcircuit_iterator;
  typedef tl::weak_collection<SubCircuit>::const_iterator const_refs_iterator;
  typedef tl::weak_collection<SubCircuit>::iterator refs_iterator;
  typedef dereferencing_iterator<tl::vector<Circuit *>::const_iterator, Circuit> child_circuit_iterator;
  typedef dereferencing_iterator<tl::vector<const Circuit *>::const_iterator, const Circuit> const_child_circuit_iterator;
  typedef dereferencing_iterator<tl::vector<Circuit *>::const_iterator, Circuit> parent_circuit_iterator;
  typedef dereferencing_iterator<tl::vector<const Circuit *>::const_iterator, const Circuit> const_parent_circuit_iterator;

  /**
   *  @brief Constructor
   *
   *  Creates an empty circuit.
   */
  Circuit ();

  /**
   *  @brief Constructor
   *
   *  Creates a circuit corresponding to a layout cell
   */
  Circuit (const db::Layout &layout, db::cell_index_type ci);

  /**
   *  @brief Copy constructor
   */
  Circuit (const Circuit &other);

  /**
   *  @brief Destructor
   */
  ~Circuit ();

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
   *  @brief Sets the boundary
   */
  void set_boundary (const db::DPolygon &boundary);

  /**
   *  @brief Gets the boundary
   */
  const db::DPolygon &boundary () const
  {
    return m_boundary;
  }

  /**
   *  @brief Sets or resets the "don't purge" flag
   *  This flag will prevent "purge" from deleting this circuit. It is set by "blank".
   */
  void set_dont_purge (bool dp);

  /**
   *  @brief Gets or resets the "don't purge" flag
   */
  bool dont_purge () const
  {
    return m_dont_purge;
  }

  /**
   *  @brief The index of the circuit in the netlist
   *  CAUTION: this attribute is used for internal purposes and may not be valid always.
   */
  size_t index () const
  {
    return m_index;
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
   *  @brief Gets the references to this circuit (begin, non-const version)
   *  This iterator will deliver all subcircuits referencing this circuit
   */
  refs_iterator begin_refs ()
  {
    return m_refs.begin ();
  }

  /**
   *  @brief Gets the references to this circuit (end, non-const version)
   *  This iterator will deliver all subcircuits referencing this circuit
   */
  refs_iterator end_refs ()
  {
    return m_refs.end ();
  }

  /**
   *  @brief Gets the references to this circuit (begin, const version)
   *  This iterator will deliver all subcircuits referencing this circuit
   */
  const_refs_iterator begin_refs () const
  {
    return m_refs.begin ();
  }

  /**
   *  @brief Gets the references to this circuit (end, const version)
   *  This iterator will deliver all subcircuits referencing this circuit
   */
  const_refs_iterator end_refs () const
  {
    return m_refs.end ();
  }

  /**
   *  @brief Returns a value indicating whether the circuit has references
   */
  bool has_refs () const
  {
    return begin_refs () != end_refs ();
  }

  /**
   *  @brief Gets the child circuits iterator (begin)
   *  The child circuits are the circuits referenced by all subcircuits
   *  in the circuit.
   */
  child_circuit_iterator begin_children ();

  /**
   *  @brief Gets the child circuits iterator (end)
   */
  child_circuit_iterator end_children ();

  /**
   *  @brief Gets the child circuits iterator (begin, const version)
   *  The child circuits are the circuits referenced by all subcircuits
   *  in the circuit.
   */
  const_child_circuit_iterator begin_children () const;

  /**
   *  @brief Gets the child circuits iterator (end, const version)
   */
  const_child_circuit_iterator end_children () const;

  /**
   *  @brief Gets the parent circuits iterator (begin)
   *  The parents circuits are the circuits referencing this circuit via subcircuits.
   */
  parent_circuit_iterator begin_parents ();

  /**
   *  @brief Gets the parent circuits iterator (end)
   */
  parent_circuit_iterator end_parents ();

  /**
   *  @brief Gets the parent circuits iterator (begin, const version)
   *  The parents circuits are the circuits referencing this circuit via subcircuits.
   */
  const_parent_circuit_iterator begin_parents () const;

  /**
   *  @brief Gets the parent circuits iterator (end, const version)
   */
  const_parent_circuit_iterator end_parents () const;

  /**
   *  @brief Clears the pins
   */
  void clear_pins ();

  /**
   *  @brief Adds a pin to this circuit
   */
  Pin &add_pin (const std::string &name);

  /**
   *  @brief Adds a pin to this circuit
   *  This version uses the given pin as the template.
   */
  Pin &add_pin (const Pin &pin);

  /**
   *  @brief Removes the pin with the given ID
   */
  void remove_pin (size_t id);

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
   *  @brief Gets the pin by ID (the ID is basically the index) - non-const version
   */
  Pin *pin_by_id (size_t id)
  {
    return const_cast<Pin *> (((const db::Circuit *) this)->pin_by_id (id));
  }

  /**
   *  @brief Gets the pin by name
   *
   *  If there is no pin with that name, null is returned.
   *  NOTE: this is a linear search, so it's performance may not be good for many pins.
   */
  const Pin *pin_by_name (const std::string &name) const;

  /**
   *  @brief Gets the pin by name - non-const version
   */
  Pin *pin_by_name (const std::string &name)
  {
    return const_cast<Pin *> (((const db::Circuit *) this)->pin_by_name (name));
  }

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
   *  @brief Joins the second net with the first one and removes the second net
   */
  void join_nets (Net *net, Net *with);

  /**
   *  @brief Gets the number of nets
   */
  size_t net_count () const
  {
    return m_nets.size ();
  }

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
   *  @brief Gets the net from a given cluster ID (const version)
   *
   *  If the cluster ID is not valid, null is returned.
   */
  const Net *net_by_cluster_id (size_t cluster_id) const
  {
    return (const_cast<Circuit *> (this)->net_by_cluster_id (cluster_id));
  }

  /**
   *  @brief Gets the net from a given cluster ID (non-const version)
   *
   *  If the cluster ID is not valid, null is returned.
   */
  Net *net_by_cluster_id (size_t cluster_id)
  {
    return m_net_by_cluster_id.object_by (cluster_id);
  }

  /**
   *  @brief Gets the net from a given name (const version)
   *
   *  If the name is not valid, null is returned.
   */
  const Net *net_by_name (const std::string &name) const
  {
    return (const_cast<Circuit *> (this)->net_by_name (name));
  }

  /**
   *  @brief Gets the net from a given name (non-const version)
   *
   *  If the name is not valid, null is returned.
   */
  Net *net_by_name (const std::string &name);

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
   *  @brief Gets the number of devices
   */
  size_t device_count () const
  {
    return m_devices.size ();
  }

  /**
   *  @brief Gets the device from a given ID (const version)
   *
   *  If the ID is not valid, null is returned.
   */
  const Device *device_by_id (size_t id) const
  {
    return (const_cast<Circuit *> (this)->device_by_id (id));
  }

  /**
   *  @brief Gets the device from a given ID (non-const version)
   *
   *  If the ID is not valid, null is returned.
   */
  Device *device_by_id (size_t id)
  {
    return m_device_by_id.object_by (id);
  }

  /**
   *  @brief Gets the device from a given name (const version)
   *
   *  If the name is not valid, null is returned.
   */
  const Device *device_by_name (const std::string &name) const
  {
    return (const_cast<Circuit *> (this)->device_by_name (name));
  }

  /**
   *  @brief Gets the device from a given name (non-const version)
   *
   *  If the name is not valid, null is returned.
   */
  Device *device_by_name (const std::string &name);

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
  void add_subcircuit (SubCircuit *subcircuit);

  /**
   *  @brief Deletes a subcircuit from the circuit
   */
  void remove_subcircuit (SubCircuit *subcircuit);

  /**
   *  @brief Gets the number of subcircuits
   */
  size_t subcircuit_count () const
  {
    return m_subcircuits.size ();
  }

  /**
   *  @brief Gets the subcircuit from a given ID (const version)
   *
   *  If the ID is not valid, null is returned.
   */
  const SubCircuit *subcircuit_by_id (size_t id) const
  {
    return (const_cast<Circuit *> (this)->subcircuit_by_id (id));
  }

  /**
   *  @brief Gets the subcircuit from a given ID (non-const version)
   *
   *  If the ID is not valid, null is returned.
   */
  SubCircuit *subcircuit_by_id (size_t id)
  {
    return m_subcircuit_by_id.object_by (id);
  }

  /**
   *  @brief Gets the subcircuit from a given name (const version)
   *
   *  If the name is not valid, null is returned.
   */
  const SubCircuit *subcircuit_by_name (const std::string &name) const
  {
    return (const_cast<Circuit *> (this)->subcircuit_by_name (name));
  }

  /**
   *  @brief Gets the subcircuit from a given name (non-const version)
   *
   *  If the name is not valid, null is returned.
   */
  SubCircuit *subcircuit_by_name (const std::string &name);

  /**
   *  @brief Begin iterator for the subcircuits of the circuit (non-const version)
   */
  subcircuit_iterator begin_subcircuits ()
  {
    return m_subcircuits.begin ();
  }

  /**
   *  @brief End iterator for the subcircuits of the circuit (non-const version)
   */
  subcircuit_iterator end_subcircuits ()
  {
    return m_subcircuits.end ();
  }

  /**
   *  @brief Begin iterator for the subcircuits of the circuit (const version)
   */
  const_subcircuit_iterator begin_subcircuits () const
  {
    return m_subcircuits.begin ();
  }

  /**
   *  @brief End iterator for the subcircuits of the circuit (const version)
   */
  const_subcircuit_iterator end_subcircuits () const
  {
    return m_subcircuits.end ();
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
   *  @brief Adds a pin to the given net
   *  The pin will be added to the net. If there is already a pin
   *  on the net, the existing and new pin will be joined.
   *  This usually implies that nets further up in the hierarchy
   *  are joined too.
   */
  void join_pin_with_net (size_t pin_id, Net *net);

  /**
   *  @brief Renames the pin with the given ID
   */
  void rename_pin (size_t pin_id, const std::string &name);

  /**
   *  @brief Purge unused nets
   *
   *  This method will purge all nets which return "is_passive".
   *  Pins on these nets will also be removed.
   */
  void purge_nets ();

  /**
   *  @brief Purge unused nets but
   *
   *  This method will purge all nets which return "is_passive".
   *  Pins on these nets will be kept but their net will be 0.
   */
  void purge_nets_keep_pins ();

  /**
   *  @brief Combine devices
   *
   *  This method will combine devices that can be combined according
   *  to their device classes "combine_devices" method.
   */
  void combine_devices ();

  /**
   *  @brief Flattens the given subcircuit
   *
   *  The subcircuit is resolved into the parent circuit and finally removed.
   *  Net, device and subcircuit names are decorated with the subcircuit's name
   *  if required.
   */
  void flatten_subcircuit (SubCircuit *subcircuit);

  /**
   *  @brief Blanks out the circuit
   *
   *  This will remove all innards of the circuit (nets, devices, subcircuits)
   *  and circuits which will itself are not longer be called after this.
   *  This operation will eventually leave a blackbox model of the circuit
   *  containing only pins.
   */
  void blank ();

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_boundary, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_nets, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_pins, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_pin_by_id, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_devices, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_subcircuits, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_pin_refs, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_device_by_id, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_subcircuit_by_id, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_net_by_cluster_id, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_device_by_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_subcircuit_by_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_net_by_name, true, (void *) this);
    db::mem_stat (stat, purpose, cat, m_refs, true, (void *) this);
  }

private:
  friend class Netlist;
  friend class Net;
  friend class SubCircuit;
  friend class Device;

  std::string m_name;
  db::DPolygon m_boundary;
  bool m_dont_purge;
  db::cell_index_type m_cell_index;
  net_list m_nets;
  pin_list m_pins;
  std::vector<pin_list::iterator> m_pin_by_id;
  device_list m_devices;
  subcircuit_list m_subcircuits;
  Netlist *mp_netlist;
  std::vector<Net::pin_iterator> m_pin_refs;
  object_by_attr<Circuit, Circuit::device_iterator, id_attribute<Device> > m_device_by_id;
  object_by_attr<Circuit, Circuit::subcircuit_iterator, id_attribute<SubCircuit> > m_subcircuit_by_id;
  object_by_attr<Circuit, Circuit::net_iterator, cluster_id_attribute<Net> > m_net_by_cluster_id;
  object_by_attr<Circuit, Circuit::device_iterator, name_attribute<Device> > m_device_by_name;
  object_by_attr<Circuit, Circuit::subcircuit_iterator, name_attribute<SubCircuit> > m_subcircuit_by_name;
  object_by_attr<Circuit, Circuit::net_iterator, name_attribute<Net> > m_net_by_name;
  tl::weak_collection<SubCircuit> m_refs;
  size_t m_index;

  void set_index (size_t index)
  {
    m_index = index;
  }

  void set_pin_ref_for_pin (size_t ppin_id, Net::pin_iterator iter);

  void register_ref (SubCircuit *sc);
  void unregister_ref (SubCircuit *sc);

  void translate_circuits (const std::map<const Circuit *, Circuit *> &map);
  void translate_device_classes (const std::map<const DeviceClass *, DeviceClass *> &map);
  void translate_device_abstracts (const std::map<const DeviceAbstract *, DeviceAbstract *> &map);
  void set_netlist (Netlist *netlist);
  bool combine_parallel_devices (const db::DeviceClass &cls);
  bool combine_serial_devices (const db::DeviceClass &cls);
  void do_purge_nets (bool keep_pins);
  void join_pins (size_t pin_id, size_t with);
  void devices_changed ();
  void subcircuits_changed ();
  void nets_changed ();
};

/**
 *  @brief Memory statistics for Circuit
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const Circuit &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif
