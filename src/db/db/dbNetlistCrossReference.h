
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
    typedef db::Net object_type;

    NetPairData (const db::Net *a, const db::Net *b, Status s) : pair (a, b), status (s) { }
    NetPairData () : pair ((const db::Net *)0, (const db::Net *)0), status (None) { }

    std::pair<const db::Net *, const db::Net *> pair;
    Status status;
  };

  struct DevicePairData
  {
    typedef db::Device object_type;

    DevicePairData (const db::Device *a, const db::Device *b, Status s) : pair (a, b), status (s) { }
    DevicePairData () : pair ((const db::Device *)0, (const db::Device *)0), status (None) { }

    std::pair<const db::Device *, const db::Device *> pair;
    Status status;
  };

  struct PinPairData
  {
    typedef db::Pin object_type;

    PinPairData (const db::Pin *a, const db::Pin *b, Status s) : pair (a, b), status (s) { }
    PinPairData () : pair ((const db::Pin *)0, (const db::Pin *)0), status (None) { }

    std::pair<const db::Pin *, const db::Pin *> pair;
    Status status;
  };

  struct SubCircuitPairData
  {
    typedef db::SubCircuit object_type;

    SubCircuitPairData (const db::SubCircuit *a, const db::SubCircuit *b, Status s) : pair (a, b), status (s) { }
    SubCircuitPairData () : pair ((const db::SubCircuit *)0, (const db::SubCircuit *)0), status (None) { }

    std::pair<const db::SubCircuit *, const db::SubCircuit *> pair;
    Status status;
  };

  struct PerCircuitData
  {
    PerCircuitData () : status (None) { }

    typedef std::vector<NetPairData> net_pairs_type;
    typedef net_pairs_type::const_iterator net_pairs_const_iterator;
    typedef std::vector<DevicePairData> device_pairs_type;
    typedef device_pairs_type::const_iterator device_pairs_const_iterator;
    typedef std::vector<PinPairData> pin_pairs_type;
    typedef pin_pairs_type::const_iterator pin_pairs_const_iterator;
    typedef std::vector<SubCircuitPairData> subcircuit_pairs_type;
    typedef subcircuit_pairs_type::const_iterator subcircuit_pairs_const_iterator;

    Status status;
    net_pairs_type nets;
    device_pairs_type devices;
    pin_pairs_type pins;
    subcircuit_pairs_type subcircuits;
  };

  struct PerNetData
  {
    typedef std::vector<std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *> > terminal_pairs_type;
    typedef terminal_pairs_type::const_iterator terminal_pairs_const_iterator;
    typedef std::vector<std::pair<const db::NetPinRef *, const db::NetPinRef *> > pin_pairs_type;
    typedef pin_pairs_type::const_iterator pin_pairs_const_iterator;
    typedef std::vector<std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *> > subcircuit_pin_pairs_type;
    typedef subcircuit_pin_pairs_type::const_iterator subcircuit_pin_pairs_const_iterator;

    std::vector<std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *> > terminals;
    std::vector<std::pair<const db::NetPinRef *, const db::NetPinRef *> > pins;
    std::vector<std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *> > subcircuit_pins;
  };

  //  Generic events - thew NetlistCompareLogger events are mapped to these
  void gen_begin_netlist (const db::Netlist *a, const db::Netlist *b);
  void gen_end_netlist (const db::Netlist *a, const db::Netlist *b);
  void gen_begin_circuit (const db::Circuit *a, const db::Circuit *b);
  void gen_end_circuit (const db::Circuit *a, const db::Circuit *b, Status status);
  void gen_nets (const db::Net *a, const db::Net *b, Status status);
  void gen_devices (const db::Device *a, const db::Device *b, Status status);
  void gen_pins (const db::Pin *a, const db::Pin *b, Status status);
  void gen_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b, Status status);

  //  db::NetlistCompareLogger interface
  virtual void begin_netlist (const db::Netlist *a, const db::Netlist *b)
  {
    gen_begin_netlist (a, b);
  }

  virtual void end_netlist (const db::Netlist *a, const db::Netlist *b)
  {
    gen_end_netlist (a, b);
  }

  virtual void begin_circuit (const db::Circuit *a, const db::Circuit *b)
  {
    gen_begin_circuit (a, b);
  }

  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching)
  {
    gen_end_circuit (a, b, matching ? Match : NoMatch);
  }

  virtual void circuit_skipped (const db::Circuit *a, const db::Circuit *b)
  {
    gen_begin_circuit (a, b);
    gen_end_circuit (a, b, Skipped);
  }

  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b)
  {
    gen_begin_circuit (a, b);
    gen_end_circuit (a, b, Mismatch);
  }

  virtual void match_nets (const db::Net *a, const db::Net *b)
  {
    gen_nets (a, b, Match);
  }

  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b)
  {
    gen_nets (a, b, MatchWithWarning);
  }

  virtual void net_mismatch (const db::Net *a, const db::Net *b)
  {
    gen_nets (a, b, Mismatch);
  }

  virtual void match_devices (const db::Device *a, const db::Device *b)
  {
    gen_devices (a, b, Match);
  }

  virtual void match_devices_with_different_parameters (const db::Device *a, const db::Device *b)
  {
    gen_devices (a, b, MatchWithWarning);
  }

  virtual void match_devices_with_different_device_classes (const db::Device *a, const db::Device *b)
  {
    gen_devices (a, b, MatchWithWarning);
  }

  virtual void device_mismatch (const db::Device *a, const db::Device *b)
  {
    gen_devices (a, b, Mismatch);
  }

  virtual void match_pins (const db::Pin *a, const db::Pin *b)
  {
    gen_pins (a, b, Match);
  }

  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b)
  {
    gen_pins (a, b, Mismatch);
  }

  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    gen_subcircuits (a, b, Match);
  }

  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    gen_subcircuits (a, b, Mismatch);
  }

  void clear ();

  size_t circuit_count () const
  {
    return m_circuits.size ();
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

  const db::Circuit *other_circuit_for (const db::Circuit *circuit) const;
  const db::Net *other_net_for (const db::Net *net) const;
  const PerNetData *per_net_data_for (const std::pair<const db::Net *, const db::Net *> &nets) const;

  const db::Netlist *netlist_a () const
  {
    return mp_netlist_a.get ();
  }

  const db::Netlist *netlist_b () const
  {
    return mp_netlist_b.get ();
  }

private:
  tl::weak_ptr<db::Netlist> mp_netlist_a, mp_netlist_b;
  std::vector<std::pair<const db::Circuit *, const db::Circuit *> > m_circuits;
  std::list<PerCircuitData> m_per_circuit_data;
  std::map<const db::Circuit *, PerCircuitData *> m_data_refs;
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
