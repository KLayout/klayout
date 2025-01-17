
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_layNetlistCrossReference
#define HDR_layNetlistCrossReference

#include "layuiCommon.h"
#include "layIndexedNetlistModel.h"

#include "dbNetlistCrossReference.h"

namespace lay
{

/**
 *  @brief An indexed netlist model for the netlist cross-reference
 */
class LAYUI_PUBLIC NetlistCrossReferenceModel
  : public lay::IndexedNetlistModel
{
public:
  NetlistCrossReferenceModel (const db::NetlistCrossReference *cross_ref);

  virtual bool is_single () const { return false; }

  virtual size_t circuit_count () const;
  virtual size_t top_circuit_count () const;
  virtual size_t net_count (const circuit_pair &circuits) const;
  virtual size_t net_terminal_count (const net_pair &nets) const;
  virtual size_t net_subcircuit_pin_count (const net_pair &nets) const;
  virtual size_t subcircuit_pin_count (const subcircuit_pair &subcircuits) const;
  virtual size_t net_pin_count (const net_pair &nets) const;
  virtual size_t device_count (const circuit_pair &circuits) const;
  virtual size_t pin_count (const circuit_pair &circuits) const;
  virtual size_t subcircuit_count (const circuit_pair &circuits) const;
  virtual size_t child_circuit_count (const circuit_pair &circuits) const;

  virtual circuit_pair parent_of (const net_pair &net_pair) const;
  virtual circuit_pair parent_of (const device_pair &device_pair) const;
  virtual circuit_pair parent_of (const subcircuit_pair &subcircuit_pair) const;

  virtual std::pair<circuit_pair, std::pair<Status, std::string> > top_circuit_from_index(size_t index) const;
  virtual std::pair<circuit_pair, std::pair<Status, std::string> > circuit_from_index (size_t index) const;
  virtual std::pair<circuit_pair, std::pair<Status, std::string> > child_circuit_from_index(const circuit_pair &circuits, size_t index) const;
  virtual std::pair<net_pair, std::pair<Status, std::string> > net_from_index (const circuit_pair &circuits, size_t index) const;
  virtual const db::Net *second_net_for (const db::Net *first) const;
  virtual const db::Circuit *second_circuit_for (const db::Circuit *first) const;
  virtual net_subcircuit_pin_pair net_subcircuit_pinref_from_index (const net_pair &nets, size_t index) const;
  virtual net_subcircuit_pin_pair subcircuit_pinref_from_index (const subcircuit_pair &nets, size_t index) const;
  virtual net_terminal_pair net_terminalref_from_index (const net_pair &nets, size_t index) const;
  virtual net_pin_pair net_pinref_from_index (const net_pair &nets, size_t index) const;
  virtual std::pair<device_pair, std::pair<Status, std::string> > device_from_index (const circuit_pair &circuits, size_t index) const;
  virtual std::pair<pin_pair, std::pair<Status, std::string> > pin_from_index (const circuit_pair &circuits, size_t index) const;
  virtual std::pair<subcircuit_pair, std::pair<Status, std::string> > subcircuit_from_index (const circuit_pair &circuits, size_t index) const;

  virtual std::string top_circuit_status_hint (size_t index) const;
  virtual std::string circuit_status_hint (size_t index) const;
  virtual std::string child_circuit_status_hint (const circuit_pair &circuits, size_t index) const;
  virtual std::string circuit_pair_status_hint (const std::pair<circuit_pair, std::pair<Status, std::string> > &cp) const;
  virtual std::string net_status_hint (const circuit_pair &circuits, size_t index) const;
  virtual std::string device_status_hint (const circuit_pair &circuits, size_t index) const;
  virtual std::string pin_status_hint (const circuit_pair &circuits, size_t index) const;
  virtual std::string subcircuit_status_hint (const circuit_pair &circuits, size_t index) const;

  virtual size_t circuit_index (const circuit_pair &circuits) const;
  virtual size_t net_index (const net_pair &nets) const;
  virtual size_t device_index (const device_pair &devices) const;
  virtual size_t pin_index (const pin_pair &pins, const circuit_pair &circuits) const;
  virtual size_t subcircuit_index (const subcircuit_pair &subcircuits) const;

public:
  struct PerCircuitCacheData
  {
    std::map<std::pair<const db::Net *, const db::Net *>, size_t> index_of_nets;
    std::map<std::pair<const db::Device *, const db::Device *>, size_t> index_of_devices;
    std::map<std::pair<const db::Pin *, const db::Pin *>, size_t> index_of_pins;
    std::map<std::pair<const db::SubCircuit *, const db::SubCircuit *>, size_t> index_of_subcircuits;
  };

  struct PerSubCircuitCacheData
  {
    std::vector<std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *> > nets_per_pins;
  };

  tl::weak_ptr<db::NetlistCrossReference> mp_cross_ref;
  mutable std::map<net_pair, circuit_pair> m_parents_of_nets;
  mutable std::map<device_pair, circuit_pair> m_parents_of_devices;
  mutable std::map<pin_pair, circuit_pair> m_parents_of_pins;
  mutable std::map<subcircuit_pair, circuit_pair> m_parents_of_subcircuits;
  mutable std::map<circuit_pair, std::vector<circuit_pair> > m_child_circuits;
  mutable std::vector<circuit_pair> m_top_level_circuits;
  mutable std::map<std::pair<const db::Circuit *, const db::Circuit *>, PerCircuitCacheData> m_per_circuit_data;
  mutable std::map<std::pair<const db::Circuit *, const db::Circuit *>, size_t> m_index_of_circuits;
  mutable std::map<std::pair<const db::SubCircuit *, const db::SubCircuit *>, PerSubCircuitCacheData> m_per_subcircuit_data;

  void ensure_subcircuit_data_built () const;
};

}

#endif

#endif  //  defined(HAVE_QT)
