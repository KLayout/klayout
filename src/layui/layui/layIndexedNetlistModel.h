
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#if defined(HAVE_QT)

#ifndef HDR_layIndexedNetlistModel
#define HDR_layIndexedNetlistModel

#include "layuiCommon.h"
#include "dbNetlistCrossReference.h"  //  for Status enum

#include <map>
#include <vector>
#include <algorithm>
#include <limits>

namespace db
{
  class Net;
  class Circuit;
  class Netlist;
  class NetSubcircuitPinRef;
  class NetTerminalRef;
  class NetPinRef;
  class Device;
  class Pin;
  class SubCircuit;
  class Netlist;
}

namespace lay
{

const size_t no_netlist_index = std::numeric_limits<size_t>::max ();

/**
 *  @brief An interface to supply the netlist browser model with indexed items
 */
class LAYUI_PUBLIC IndexedNetlistModel
{
public:
  typedef db::NetlistCrossReference::Status Status;

  IndexedNetlistModel () { }
  virtual ~IndexedNetlistModel () { }

  virtual bool is_single () const = 0;

  typedef std::pair<const db::Circuit *, const db::Circuit *> circuit_pair;
  typedef std::pair<const db::Net *, const db::Net *> net_pair;
  typedef std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *> net_subcircuit_pin_pair;
  typedef std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *> net_terminal_pair;
  typedef std::pair<const db::NetPinRef *, const db::NetPinRef *> net_pin_pair;
  typedef std::pair<const db::Device *, const db::Device *> device_pair;
  typedef std::pair<const db::Pin *, const db::Pin *> pin_pair;
  typedef std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuit_pair;

  virtual size_t top_circuit_count () const = 0;
  virtual size_t circuit_count () const = 0;
  virtual size_t net_count (const circuit_pair &circuits) const = 0;
  virtual size_t net_terminal_count (const net_pair &nets) const = 0;
  virtual size_t net_subcircuit_pin_count (const net_pair &nets) const = 0;
  virtual size_t net_pin_count (const net_pair &nets) const = 0;
  virtual size_t device_count (const circuit_pair &circuits) const = 0;
  virtual size_t subcircuit_pin_count (const subcircuit_pair &subcircuits) const = 0;
  virtual size_t pin_count (const circuit_pair &circuits) const = 0;
  virtual size_t subcircuit_count (const circuit_pair &circuits) const = 0;
  virtual size_t child_circuit_count (const circuit_pair &circuits) const = 0;

  virtual circuit_pair parent_of (const net_pair &net_pair) const = 0;
  virtual circuit_pair parent_of (const device_pair &device_pair) const = 0;
  virtual circuit_pair parent_of (const subcircuit_pair &subcircuit_pair) const = 0;

  virtual std::pair<circuit_pair, std::pair<Status, std::string> > top_circuit_from_index (size_t index) const = 0;
  virtual std::pair<circuit_pair, std::pair<Status, std::string> > child_circuit_from_index (const circuit_pair &circuits, size_t index) const = 0;
  virtual std::pair<circuit_pair, std::pair<Status, std::string> > circuit_from_index (size_t index) const = 0;
  virtual std::pair<net_pair, std::pair<Status, std::string> > net_from_index (const circuit_pair &circuits, size_t index) const = 0;
  virtual const db::Net *second_net_for (const db::Net *first) const = 0;
  virtual const db::Circuit *second_circuit_for (const db::Circuit *first) const = 0;
  virtual net_subcircuit_pin_pair net_subcircuit_pinref_from_index (const net_pair &nets, size_t index) const = 0;
  virtual net_subcircuit_pin_pair subcircuit_pinref_from_index (const subcircuit_pair &subcircuits, size_t index) const = 0;
  virtual net_terminal_pair net_terminalref_from_index (const net_pair &nets, size_t index) const = 0;
  virtual net_pin_pair net_pinref_from_index (const net_pair &nets, size_t index) const = 0;
  virtual std::pair<device_pair, std::pair<Status, std::string> > device_from_index (const circuit_pair &circuits, size_t index) const = 0;
  virtual std::pair<pin_pair, std::pair<Status, std::string> > pin_from_index (const circuit_pair &circuits, size_t index) const = 0;
  virtual std::pair<subcircuit_pair, std::pair<Status, std::string> > subcircuit_from_index (const circuit_pair &circuits, size_t index) const = 0;

  virtual std::string top_circuit_status_hint (size_t /*index*/) const { return std::string (); }
  virtual std::string circuit_status_hint (size_t /*index*/) const { return std::string (); }
  virtual std::string child_circuit_status_hint (const circuit_pair &/*circuits*/, size_t /*index*/) const { return std::string (); }
  virtual std::string circuit_pair_status_hint (const std::pair<circuit_pair, std::pair<Status, std::string> > & /*cp*/) const { return std::string (); }
  virtual std::string net_status_hint (const circuit_pair &/*circuits*/, size_t /*index*/) const { return std::string (); }
  virtual std::string device_status_hint (const circuit_pair &/*circuits*/, size_t /*index*/) const { return std::string (); }
  virtual std::string pin_status_hint (const circuit_pair &/*circuits*/, size_t /*index*/) const { return std::string (); }
  virtual std::string subcircuit_status_hint (const circuit_pair & /*circuits*/, size_t /*index*/) const { return std::string (); }

  virtual size_t circuit_index (const circuit_pair &circuits) const = 0;
  virtual size_t net_index (const net_pair &nets) const = 0;
  virtual size_t device_index (const device_pair &devices) const = 0;
  virtual size_t pin_index (const pin_pair &pins, const circuit_pair &circuits) const = 0;
  virtual size_t subcircuit_index (const subcircuit_pair &subcircuits) const = 0;

private:
  IndexedNetlistModel &operator= (const IndexedNetlistModel &);
  IndexedNetlistModel (const IndexedNetlistModel &);
};

/**
 *  @brief An incarnation of the indexed netlist model for a single netlist
 */
class LAYUI_PUBLIC SingleIndexedNetlistModel
  : public IndexedNetlistModel
{
public:
  SingleIndexedNetlistModel (const db::Netlist *netlist)
    : mp_netlist (netlist)
  {
    //  .. nothing yet ..
  }

  virtual ~SingleIndexedNetlistModel () { }

  virtual bool is_single () const
  {
    return true;
  }

  virtual size_t circuit_count () const;
  virtual size_t top_circuit_count () const;
  virtual size_t net_count (const circuit_pair &circuits) const;
  virtual size_t net_terminal_count (const net_pair &nets) const;
  virtual size_t net_subcircuit_pin_count (const net_pair &nets) const;
  virtual size_t net_pin_count (const net_pair &nets) const;
  virtual size_t device_count (const circuit_pair &circuits) const;
  virtual size_t pin_count (const circuit_pair &circuits) const;
  virtual size_t subcircuit_pin_count (const subcircuit_pair &subcircuits) const;
  virtual size_t subcircuit_count (const circuit_pair &circuits) const;
  virtual size_t child_circuit_count (const circuit_pair &circuits) const;

  virtual circuit_pair parent_of (const net_pair &nets) const;
  virtual circuit_pair parent_of (const device_pair &devices) const;
  virtual circuit_pair parent_of (const subcircuit_pair &subcircuits) const;

  virtual std::pair<circuit_pair, std::pair<Status, std::string> > top_circuit_from_index (size_t index) const;
  virtual std::pair<circuit_pair, std::pair<Status, std::string> > circuit_from_index (size_t index) const;
  virtual std::pair<circuit_pair, std::pair<Status, std::string> > child_circuit_from_index (const circuit_pair &circuits, size_t index) const;
  virtual std::pair<net_pair, std::pair<Status, std::string>  > net_from_index (const circuit_pair &circuits, size_t index) const;
  virtual const db::Net *second_net_for (const db::Net *first) const;
  virtual const db::Circuit *second_circuit_for (const db::Circuit *first) const;
  virtual net_subcircuit_pin_pair net_subcircuit_pinref_from_index (const net_pair &nets, size_t index) const;
  virtual net_subcircuit_pin_pair subcircuit_pinref_from_index (const subcircuit_pair &subcircuits, size_t index) const;
  virtual net_terminal_pair net_terminalref_from_index (const net_pair &nets, size_t index) const;
  virtual net_pin_pair net_pinref_from_index (const net_pair &nets, size_t index) const;
  virtual std::pair<device_pair, std::pair<Status, std::string>  > device_from_index (const circuit_pair &circuits, size_t index) const;
  virtual std::pair<pin_pair, std::pair<Status, std::string>  > pin_from_index (const circuit_pair &circuits, size_t index) const;
  virtual std::pair<subcircuit_pair, std::pair<Status, std::string> > subcircuit_from_index (const circuit_pair &circuits, size_t index) const;

  virtual size_t circuit_index (const circuit_pair &circuits) const;
  virtual size_t net_index (const net_pair &nets) const;
  virtual size_t device_index (const device_pair &devices) const;
  virtual size_t pin_index (const pin_pair &pins, const circuit_pair &circuits) const;
  virtual size_t subcircuit_index (const subcircuit_pair &subcircuits) const;

private:
  typedef std::pair<const db::Netlist *, const db::Netlist *> netlist_pair;

  const db::Netlist *mp_netlist;

  mutable std::map<netlist_pair, std::vector<circuit_pair> > m_circuit_by_index;
  mutable std::map<circuit_pair, std::vector<circuit_pair> > m_child_circuit_by_circuit_and_index;
  mutable std::map<circuit_pair, std::vector<net_pair> > m_net_by_circuit_and_index;
  mutable std::map<net_pair, std::vector<net_subcircuit_pin_pair> > m_subcircuit_pinref_by_net_and_index;
  mutable std::map<net_pair, std::vector<net_terminal_pair> > m_terminalref_by_net_and_index;
  mutable std::map<net_pair, std::vector<net_pin_pair> > m_pinref_by_net_and_index;
  mutable std::map<circuit_pair, std::vector<device_pair> > m_device_by_circuit_and_index;
  mutable std::map<circuit_pair, std::vector<pin_pair> > m_pin_by_circuit_and_index;
  mutable std::map<circuit_pair, std::vector<subcircuit_pair> > m_subcircuit_by_circuit_and_index;
  mutable std::map<circuit_pair, size_t> m_circuit_index_by_object;
  mutable std::map<net_pair, size_t> m_net_index_by_object;
  mutable std::map<pin_pair, size_t> m_pin_index_by_object;
  mutable std::map<subcircuit_pair, size_t> m_subcircuit_index_by_object;
  mutable std::map<device_pair, size_t> m_device_index_by_object;
  mutable std::map<subcircuit_pair, std::vector<net_subcircuit_pin_pair> > m_subcircuit_pins_by_index;
  mutable std::list<db::NetSubcircuitPinRef> m_synthetic_pinrefs;
};

}

#endif

#endif  //  defined(HAVE_QT)
