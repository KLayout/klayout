
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


#ifndef HDR_dbNetlistCrossReference
#define HDR_dbNetlistCrossReference

#include "dbCommon.h"
#include "dbNetlistCompare.h"
#include "tlObject.h"

#include <vector>
#include <map>

namespace db
{

/**
 *  @brief The NetlistCrossReference class
 *
 *  This class stores the results of a netlist compare in a form which is compatible
 *  with the netlist cross-reference model. The intention of this class is twofold:
 *  persisting the results of a netlist compare and display in the netlist browser.
 */
class DB_PUBLIC NetlistCrossReference
  : public db::NetlistCompareLogger, public tl::Object
{
public:
  NetlistCrossReference ();
  ~NetlistCrossReference ();

  enum Status {
    None = 0,
    Match,              //  objects are paired and match
    NoMatch,            //  objects are paired, but don't match
    Skipped,            //  objects are skipped
    MatchWithWarning,   //  objects are paired and match, but with warning (i.e. ambiguous nets)
    Mismatch            //  objects are not paired
  };

  struct NetPairData
  {
    NetPairData (const db::Net *a, const db::Net *b, Status s) : pair (a, b), status (s) { }
    NetPairData () : pair (0, 0), status (None) { }

    std::pair<const db::Net *, const db::Net *> pair;
    Status status;
  };

  struct DevicePairData
  {
    DevicePairData (const db::Device *a, const db::Device *b, Status s) : pair (a, b), status (s) { }
    DevicePairData () : pair (0, 0), status (None) { }

    std::pair<const db::Device *, const db::Device *> pair;
    Status status;
  };

  struct PinPairData
  {
    PinPairData (const db::Pin *a, const db::Pin *b, Status s) : pair (a, b), status (s) { }
    PinPairData () : pair (0, 0), status (None) { }

    std::pair<const db::Pin *, const db::Pin *> pair;
    Status status;
  };

  struct SubCircuitPairData
  {
    SubCircuitPairData (const db::SubCircuit *a, const db::SubCircuit *b, Status s) : pair (a, b), status (s) { }
    SubCircuitPairData () : pair (0, 0), status (None) { }

    std::pair<const db::SubCircuit *, const db::SubCircuit *> pair;
    Status status;
  };

  struct PerCircuitData
  {
    PerCircuitData () : status (None) { }

    Status status;
    std::vector<NetPairData> nets;
    std::vector<DevicePairData> devices;
    std::vector<PinPairData> pins;
    std::vector<SubCircuitPairData> subcircuits;
  };

  struct PerNetData
  {
    std::vector<std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *> > terminals;
    std::vector<std::pair<const db::NetPinRef *, const db::NetPinRef *> > pins;
    std::vector<std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *> > subcircuit_pins;
  };

  virtual void begin_netlist (const db::Netlist *a, const db::Netlist *b);
  virtual void end_netlist (const db::Netlist *a, const db::Netlist *b);
  virtual void device_class_mismatch (const db::DeviceClass *a, const db::DeviceClass *b);
  virtual void begin_circuit (const db::Circuit *a, const db::Circuit *b);
  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching);
  virtual void circuit_skipped (const db::Circuit *a, const db::Circuit *b);
  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b);
  virtual void match_nets (const db::Net *a, const db::Net *b);
  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b);
  virtual void net_mismatch (const db::Net *a, const db::Net *b);
  virtual void match_devices (const db::Device *a, const db::Device *b);
  virtual void match_devices_with_different_parameters (const db::Device *a, const db::Device *b);
  virtual void match_devices_with_different_device_classes (const db::Device *a, const db::Device *b);
  virtual void device_mismatch (const db::Device *a, const db::Device *b);
  virtual void match_pins (const db::Pin *a, const db::Pin *b);
  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b);
  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b);
  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b);

  size_t circuit_count () const
  {
    return m_per_circuit_data.size ();
  }

  typedef std::map<std::pair<const db::Circuit *, const db::Circuit *>, PerCircuitData>::const_iterator per_circuit_data_iterator;

  per_circuit_data_iterator begin_per_circuit_data () const
  {
    return m_per_circuit_data.begin ();
  }

  per_circuit_data_iterator end_per_circuit_data () const
  {
    return m_per_circuit_data.end ();
  }

  const PerCircuitData *per_circuit_data_for (const std::pair<const db::Circuit *, const db::Circuit *> &circuits) const;

  typedef std::vector<std::pair<const db::Circuit *, const db::Circuit *> >::const_iterator circuits_iterator;

  circuits_iterator begin_circuits () const
  {
    return m_circuits.begin ();
  }

  circuits_iterator end_circuits () const
  {
    return m_circuits.end ();
  }

  const db::Net *other_net_for (const db::Net *net) const;
  const PerNetData *per_net_data_for (const std::pair<const db::Net *, const db::Net *> &nets) const;

private:
  tl::weak_ptr<db::Netlist> mp_netlist_a, mp_netlist_b;
  std::vector<std::pair<const db::Circuit *, const db::Circuit *> > m_circuits;
  std::map<std::pair<const db::Circuit *, const db::Circuit *>, PerCircuitData> m_per_circuit_data;
  mutable std::map<std::pair<const db::Net *, const db::Net *>, PerNetData> m_per_net_data;
  std::map<const db::Circuit *, const db::Circuit *> m_other_circuit;
  std::map<const db::Net *, const db::Net *> m_other_net;
  std::map<const db::Device *, const db::Device *> m_other_device;
  std::map<const db::Pin *, const db::Pin *> m_other_pin;
  std::map<const db::SubCircuit *, const db::SubCircuit *> m_other_subcircuit;
  std::pair<const db::Circuit *, const db::Circuit *> m_current_circuits;
  PerCircuitData *mp_per_circuit_data;

  void establish_pair (const db::Circuit *a, const db::Circuit *b);
  void establish_pair (const db::Net *a, const db::Net *b, Status status);
  void establish_pair (const db::Device *a, const db::Device *b, Status status);
  void establish_pair (const db::Pin *a, const db::Pin *b, Status status);
  void establish_pair (const db::SubCircuit *a, const db::SubCircuit *b, Status status);

  void build_per_net_info (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
  void build_subcircuit_pin_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
  void build_pin_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
  void build_terminal_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
};

}

#endif
