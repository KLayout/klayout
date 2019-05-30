
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

#include "tlUnitTest.h"
#include "dbNetlistDeviceClasses.h"
#include "dbNetlistCompare.h"
#include "dbNetlistCrossReference.h"

class NetlistCompareTestLogger
  : public db::NetlistCompareLogger
{
public:
  NetlistCompareTestLogger () { }

  void out (const std::string &text)
  {
    m_texts.push_back (text);
#if defined(PRINT_DEBUG_NETCOMPARE)
    tl::log << m_texts.back ();
#endif
  }

  virtual void begin_circuit (const db::Circuit *a, const db::Circuit *b)
  {
    out ("begin_circuit " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching)
  {
    out ("end_circuit " + circuit2str (a) + " " + circuit2str (b) + " " + (matching ? "MATCH" : "NOMATCH"));
  }

  virtual void circuit_skipped (const db::Circuit *a, const db::Circuit *b)
  {
    out ("circuit_skipped " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b)
  {
    out ("circuit_mismatch " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void device_class_mismatch (const db::DeviceClass *a, const db::DeviceClass *b)
  {
    out ("device_class_mismatch " + dc2str (a) + " " + dc2str (b));
  }

  virtual void match_nets (const db::Net *a, const db::Net *b)
  {
    out ("match_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b)
  {
    out ("match_ambiguous_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void net_mismatch (const db::Net *a, const db::Net *b)
  {
    out ("net_mismatch " + net2str (a) + " " + net2str (b));
  }

  virtual void match_devices (const db::Device *a, const db::Device *b)
  {
    out ("match_devices " + device2str (a) + " " + device2str (b));
  }

  virtual void device_mismatch (const db::Device *a, const db::Device *b)
  {
    out ("device_mismatch " + device2str (a) + " " + device2str (b));
  }

  virtual void match_devices_with_different_parameters (const db::Device *a, const db::Device *b)
  {
    out ("match_devices_with_different_parameters " + device2str (a) + " " + device2str (b));
  }

  virtual void match_devices_with_different_device_classes (const db::Device *a, const db::Device *b)
  {
    out ("match_devices_with_different_device_classes " + device2str (a) + " " + device2str (b));
  }

  virtual void match_pins (const db::Pin *a, const db::Pin *b)
  {
    out ("match_pins " + pin2str (a) + " " + pin2str (b));
  }

  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b)
  {
    out ("pin_mismatch " + pin2str (a) + " " + pin2str (b));
  }

  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    out ("match_subcircuits " + subcircuit2str (a) + " " + subcircuit2str (b));
  }

  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    out ("subcircuit_mismatch " + subcircuit2str (a) + " " + subcircuit2str (b));
  }

  std::string text () const
  {
    return tl::join (m_texts, "\n");
  }

  void clear ()
  {
    m_texts.clear ();
  }

private:
  std::vector<std::string> m_texts;

  std::string dc2str (const db::DeviceClass *x) const
  {
    return x ? x->name () : "(null)";
  }

  std::string circuit2str (const db::Circuit *x) const
  {
    return x ? x->name () : "(null)";
  }

  std::string device2str (const db::Device *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }

  std::string net2str (const db::Net *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }

  std::string pin2str (const db::Pin *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }

  std::string subcircuit2str (const db::SubCircuit *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }
};

std::string xref_status2s (db::NetlistCrossReference::Status status)
{
  if (status == db::NetlistCrossReference::Match) {
    return "Match";
  } else if (status == db::NetlistCrossReference::Mismatch) {
    return "Mismatch";
  } else if (status == db::NetlistCrossReference::NoMatch) {
    return "NoMatch";
  } else if (status == db::NetlistCrossReference::MatchWithWarning) {
    return "MatchWithWarning";
  } else if (status == db::NetlistCrossReference::Skipped) {
    return "Skipped";
  } else {
    return "None";
  }
}

template <class Obj>
static std::string name_of (const Obj *obj)
{
  return obj ? obj->name () : std::string ("(null)");
}

template <class Obj>
static std::string expanded_name_of (const Obj *obj)
{
  return obj ? obj->expanded_name () : std::string ("(null)");
}

template <class PairData>
static
std::string dp2s (const PairData &dp)
{
  return expanded_name_of (dp.pair.first) + ":" + expanded_name_of (dp.pair.second) + " [" + xref_status2s (dp.status) + "]";
}

static std::string ne2s (const db::NetTerminalRef *ref)
{
  if (ref) {
    return expanded_name_of (ref->device ()) + "[" + name_of (ref->terminal_def ()) + "]";
  } else {
    return "(null)";
  }
}

static std::string ne2s (const db::NetSubcircuitPinRef *ref)
{
  if (ref) {
    return expanded_name_of (ref->subcircuit ()) + "[" + expanded_name_of (ref->pin ()) + "]";
  } else {
    return "(null)";
  }
}

static std::string ne2s (const db::NetPinRef *ref)
{
  if (ref) {
    return expanded_name_of (ref->pin ());
  } else {
    return "(null)";
  }
}

template <class Data>
static std::string nep2s (const std::pair<const Data *, const Data *> &p)
{
  return ne2s (p.first) + ":" + ne2s (p.second);
}

std::string xref2s (const db::NetlistCrossReference &xref)
{
  std::string s;

  for (db::NetlistCrossReference::circuits_iterator c = xref.begin_circuits (); c != xref.end_circuits (); ++c) {

    const db::NetlistCrossReference::PerCircuitData *pcd = xref.per_circuit_data_for (*c);
    tl_assert (pcd != 0);

    s += name_of (c->first) + ":" + name_of (c->second) + " [" + xref_status2s (pcd->status) + "]:\n";

    for (db::NetlistCrossReference::PerCircuitData::pin_pairs_const_iterator i = pcd->pins.begin (); i != pcd->pins.end (); ++i) {
      s += " pin " + dp2s (*i) + "\n";
    }
    for (db::NetlistCrossReference::PerCircuitData::net_pairs_const_iterator i = pcd->nets.begin (); i != pcd->nets.end (); ++i) {
      s += " net " + dp2s (*i) + "\n";
      const db::NetlistCrossReference::PerNetData *pnd = xref.per_net_data_for (i->pair);
      for (db::NetlistCrossReference::PerNetData::terminal_pairs_const_iterator j = pnd->terminals.begin (); j != pnd->terminals.end (); ++j) {
         s += "  terminal " + nep2s (*j) + "\n";
      }
      for (db::NetlistCrossReference::PerNetData::pin_pairs_const_iterator j = pnd->pins.begin (); j != pnd->pins.end (); ++j) {
         s += "  pin " + nep2s (*j) + "\n";
      }
      for (db::NetlistCrossReference::PerNetData::subcircuit_pin_pairs_const_iterator j = pnd->subcircuit_pins.begin (); j != pnd->subcircuit_pins.end (); ++j) {
         s += "  subcircuit_pin " + nep2s (*j) + "\n";
      }
    }
    for (db::NetlistCrossReference::PerCircuitData::device_pairs_const_iterator i = pcd->devices.begin (); i != pcd->devices.end (); ++i) {
      s += " device " + dp2s (*i) + "\n";
    }
    for (db::NetlistCrossReference::PerCircuitData::subcircuit_pairs_const_iterator i = pcd->subcircuits.begin (); i != pcd->subcircuits.end (); ++i) {
      s += " subcircuit " + dp2s (*i) + "\n";
    }

  }

  return s;
}

static void prep_nl (db::Netlist &nl, const char *str)
{
  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOSB");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOSB");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS4Transistor ();
  dc->set_name ("PMOS4");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS4Transistor ();
  dc->set_name ("NMOS4");
  nl.add_device_class (dc);

  dc = new db::DeviceClassResistor ();
  dc->set_name ("RES");
  nl.add_device_class (dc);

  dc = new db::DeviceClassCapacitor ();
  dc->set_name ("CAP");
  nl.add_device_class (dc);

  dc = new db::DeviceClassInductor ();
  dc->set_name ("IND");
  nl.add_device_class (dc);

  dc = new db::DeviceClassDiode ();
  dc->set_name ("DIODE");
  nl.add_device_class (dc);

  nl.from_string (str);
}

TEST(0_EqualDeviceParameters)
{
  db::DeviceClassMOS3Transistor dc;

  db::EqualDeviceParameters *eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.0, 0.0);
  dc.set_parameter_compare_delegate (eqp);

  db::Device d1 (&dc);
  db::Device d2 (&dc);

  d1.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 40.0);
  d2.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 40.0);

  EXPECT_EQ (dc.equal (d1, d2), true);
  EXPECT_EQ (dc.equal (d2, d1), true);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), false);

  //  AD, AS, PD and PS aren't a primary parameter, so we don't compare it.
  d2.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 1.0);
  d2.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 1.0);
  d2.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 1.0);
  d2.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 1.0);

  EXPECT_EQ (dc.equal (d1, d2), true);
  EXPECT_EQ (dc.equal (d2, d1), true);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), false);

  d2.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 41.0);

  EXPECT_EQ (dc.equal (d1, d2), false);
  EXPECT_EQ (dc.equal (d2, d1), false);
  EXPECT_EQ (dc.less (d1, d2), true);
  EXPECT_EQ (dc.less (d2, d1), false);

  eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.9, 0.0);
  dc.set_parameter_compare_delegate (eqp);

  EXPECT_EQ (dc.equal (d1, d2), false);
  EXPECT_EQ (dc.equal (d2, d1), false);
  EXPECT_EQ (dc.less (d1, d2), true);
  EXPECT_EQ (dc.less (d2, d1), false);

  eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 1.0, 0.0);
  dc.set_parameter_compare_delegate (eqp);

  EXPECT_EQ (dc.equal (d1, d2), true);
  EXPECT_EQ (dc.equal (d2, d1), true);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), false);

  eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 1.1, 0.0);
  dc.set_parameter_compare_delegate (eqp);

  EXPECT_EQ (dc.equal (d1, d2), true);
  EXPECT_EQ (dc.equal (d2, d1), true);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), false);

  eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.5, 0.01);
  dc.set_parameter_compare_delegate (eqp);

  EXPECT_EQ (dc.equal (d1, d2), false);
  EXPECT_EQ (dc.equal (d2, d1), false);
  EXPECT_EQ (dc.less (d1, d2), true);
  EXPECT_EQ (dc.less (d2, d1), false);

  eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.5, 0.013);
  dc.set_parameter_compare_delegate (eqp);

  EXPECT_EQ (dc.equal (d1, d2), true);
  EXPECT_EQ (dc.equal (d2, d1), true);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), false);

  d1.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.5);
  d2.set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.2);

  EXPECT_EQ (dc.equal (d1, d2), true);
  EXPECT_EQ (dc.equal (d2, d1), true);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), false);

  eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.5, 0.013);
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_W);
  dc.set_parameter_compare_delegate (eqp);

  EXPECT_EQ (dc.equal (d1, d2), false);
  EXPECT_EQ (dc.equal (d2, d1), false);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), true);

  eqp = new db::EqualDeviceParameters ();
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.5, 0.013);
  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_W, 0.3, 1e-6);
  dc.set_parameter_compare_delegate (eqp);

  EXPECT_EQ (dc.equal (d1, d2), true);
  EXPECT_EQ (dc.equal (d2, d1), true);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), false);
}

TEST(1_SimpleInverter)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets VDD VDD\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_nets VSS VSS\n"
     "match_pins $0 $1\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_devices $2 $1\n"
     "match_devices $1 $2\n"
     "end_circuit INV INV MATCH"
  );
  EXPECT_EQ (good, true);

  db::NetlistCrossReference xref;
  db::NetlistComparer comp_xref (&xref);

  good = comp_xref.compare (&nl1, &nl2);

  EXPECT_EQ (xref2s (xref),
    "INV:INV [Match]:\n"
    " pin $0:$1 [Match]\n"
    " pin $1:$3 [Match]\n"
    " pin $2:$0 [Match]\n"
    " pin $3:$2 [Match]\n"
    " net IN:IN [Match]\n"
    "  terminal $1[G]:$2[G]\n"
    "  terminal $2[G]:$1[G]\n"
    "  pin $0:$1\n"
    " net OUT:OUT [Match]\n"
    "  terminal $1[D]:$2[D]\n"
    "  terminal $2[D]:$1[S]\n"
    "  pin $1:$3\n"
    " net VDD:VDD [Match]\n"
    "  terminal $1[S]:$2[S]\n"
    "  pin $2:$0\n"
    " net VSS:VSS [Match]\n"
    "  terminal $2[S]:$1[D]\n"
    "  pin $3:$2\n"
    " device $2:$1 [Match]\n"
    " device $1:$2 [Match]\n"
  );
  EXPECT_EQ (good, true);
}

TEST(1_SimpleInverterMatchedDeviceClasses)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOSB $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOSB $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.same_device_classes (nl1.device_class_by_name ("PMOS"), nl2.device_class_by_name ("PMOSB"));

  bool good = comp.compare (&nl1, &nl2);
  EXPECT_EQ (good, false);

  logger.clear ();
  comp.same_device_classes (nl1.device_class_by_name ("NMOS"), nl2.device_class_by_name ("NMOSB"));
  //  avoids device class mismatch errors:
  comp.same_device_classes (nl1.device_class_by_name ("NMOSB"), nl2.device_class_by_name ("NMOS"));
  comp.same_device_classes (nl1.device_class_by_name ("PMOSB"), nl2.device_class_by_name ("PMOS"));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets VDD VDD\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_nets VSS VSS\n"
     "match_pins $0 $1\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_devices $2 $1\n"
     "match_devices $1 $2\n"
     "end_circuit INV INV MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(1_SimpleInverterSkippedDevices)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device CAP $2 (A=OUT,B=IN) (C=1e-12);\n"
    "  device NMOS $3 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device CAP $2 (A=OUT,B=IN) (C=1e-13);\n"
    "  device RES $3 (A=OUT,B=IN) (R=1000);\n"
    "  device PMOS $4 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets VDD VDD\n"
     "net_mismatch OUT OUT\n"
     "match_nets VSS VSS\n"
     "net_mismatch IN IN\n"
     "match_pins $0 $1\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_devices $3 $1\n"
     "match_devices_with_different_parameters $2 $2\n"
     "device_mismatch (null) $3\n"
     "match_devices $1 $4\n"
     "end_circuit INV INV NOMATCH"
  );
  EXPECT_EQ (good, false);

  db::NetlistCrossReference xref;
  db::NetlistComparer comp_xref (&xref);

  good = comp_xref.compare (&nl1, &nl2);

  EXPECT_EQ (xref2s (xref),
    "INV:INV [NoMatch]:\n"
    " pin $0:$1 [Match]\n"
    " pin $1:$3 [Match]\n"
    " pin $2:$0 [Match]\n"
    " pin $3:$2 [Match]\n"
    " net IN:IN [Mismatch]\n"
    "  terminal (null):$3[B]\n"
    "  terminal $1[G]:$4[G]\n"
    "  terminal $2[B]:$2[B]\n"
    "  terminal $3[G]:$1[G]\n"
    "  pin $0:$1\n"
    " net OUT:OUT [Mismatch]\n"
    "  terminal (null):$3[A]\n"
    "  terminal $1[D]:$4[D]\n"
    "  terminal $2[A]:$2[A]\n"
    "  terminal $3[D]:$1[S]\n"
    "  pin $1:$3\n"
    " net VDD:VDD [Match]\n"
    "  terminal $1[S]:$4[S]\n"
    "  pin $2:$0\n"
    " net VSS:VSS [Match]\n"
    "  terminal $3[S]:$1[D]\n"
    "  pin $3:$2\n"
    " device (null):$3 [Mismatch]\n"
    " device $3:$1 [Match]\n"
    " device $2:$2 [MatchWithWarning]\n"
    " device $1:$4 [Match]\n"
  );
  EXPECT_EQ (good, false);

  comp.exclude_caps (1e-11);
  comp.exclude_resistors (900.0);

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets VDD VDD\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_nets VSS VSS\n"
     "match_pins $0 $1\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_devices $3 $1\n"
     "match_devices $1 $4\n"
     "end_circuit INV INV MATCH"
  );
  EXPECT_EQ (good, true);

  xref.clear ();

  comp_xref.exclude_caps (1e-11);
  comp_xref.exclude_resistors (900.0);

  good = comp_xref.compare (&nl1, &nl2);

  EXPECT_EQ (xref2s (xref),
    "INV:INV [Match]:\n"
    " pin $0:$1 [Match]\n"
    " pin $1:$3 [Match]\n"
    " pin $2:$0 [Match]\n"
    " pin $3:$2 [Match]\n"
    " net IN:IN [Match]\n"
    "  terminal (null):$2[B]\n"
    "  terminal (null):$3[B]\n"
    "  terminal $1[G]:$4[G]\n"
    "  terminal $2[B]:(null)\n"
    "  terminal $3[G]:$1[G]\n"
    "  pin $0:$1\n"
    " net OUT:OUT [Match]\n"
    "  terminal (null):$2[A]\n"
    "  terminal (null):$3[A]\n"
    "  terminal $1[D]:$4[D]\n"
    "  terminal $2[A]:(null)\n"
    "  terminal $3[D]:$1[S]\n"
    "  pin $1:$3\n"
    " net VDD:VDD [Match]\n"
    "  terminal $1[S]:$4[S]\n"
    "  pin $2:$0\n"
    " net VSS:VSS [Match]\n"
    "  terminal $3[S]:$1[D]\n"
    "  pin $3:$2\n"
    " device $3:$1 [Match]\n"
    " device $1:$4 [Match]\n"
  );
  EXPECT_EQ (good, true);
}

TEST(2_SimpleInverterWithForcedNetAssignment)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  const db::Circuit *ca = nl1.circuit_by_name ("INV");
  const db::Circuit *cb = nl2.circuit_by_name ("INV");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_pins $0 $1\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_devices $2 $1\n"
     "match_devices $1 $2\n"
     "end_circuit INV INV MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(3_Buffer)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit BUF BUF\n"
     "match_nets VDD VDD\n"
     "match_nets OUT OUT\n"
     "match_nets INT $10\n"
     "match_nets IN IN\n"
     "match_nets VSS VSS\n"
     "match_pins $0 $1\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_devices $1 $1\n"
     "match_devices $3 $2\n"
     "match_devices $2 $3\n"
     "match_devices $4 $4\n"
     "end_circuit BUF BUF MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(4_BufferTwoPaths)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit BUF BUF\n"
     "match_nets VDD VDD\n"
     "match_nets OUT OUT\n"
     "match_nets VSS VSS\n"
     "match_nets IN IN\n"
     "match_ambiguous_nets INT $10\n"
     "match_ambiguous_nets INT2 $11\n"
     "match_pins $0 $1\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_devices $1 $1\n"
     "match_devices $3 $2\n"
     "match_devices $5 $3\n"
     "match_devices $7 $4\n"
     "match_devices $2 $5\n"
     "match_devices $4 $6\n"
     "match_devices $6 $7\n"
     "match_devices $8 $8\n"
     "end_circuit BUF BUF MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(5_BufferTwoPathsDifferentParameters)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.35,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"   //  NOTE: 0.35 instead of 0.25
    "  device NMOS $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  //  Forcing the power nets into equality makes the parameter error harder to detect
  const db::Circuit *ca = nl1.circuit_by_name ("BUF");
  const db::Circuit *cb = nl2.circuit_by_name ("BUF");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets INT $10\n"
    "net_mismatch IN IN\n"
    "net_mismatch INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices_with_different_parameters $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);

  logger.clear ();
  nl2.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 1.5, 0.0));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_ambiguous_nets INT $10\n"
    "match_ambiguous_nets INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF MATCH"
  );
  EXPECT_EQ (good, true);

  logger.clear ();
  nl2.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.0, 0.0));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets INT $10\n"
    "net_mismatch IN IN\n"
    "net_mismatch INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices_with_different_parameters $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);

  logger.clear ();
  nl2.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.0, 0.2));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets INT $10\n"
    "net_mismatch IN IN\n"
    "net_mismatch INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices_with_different_parameters $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);

  logger.clear ();
  nl2.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.0, 0.4));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_ambiguous_nets INT $10\n"
    "match_ambiguous_nets INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF MATCH"
  );
  EXPECT_EQ (good, true);

  logger.clear ();
  db::EqualDeviceParameters eq_dp = db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_W) + db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.2, 0.0);
  nl2.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (eq_dp));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_ambiguous_nets INT $10\n"
    "match_ambiguous_nets INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF MATCH"
  );
  EXPECT_EQ (good, true);

  logger.clear ();
  eq_dp = db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_W) + db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L);
  nl2.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (eq_dp));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets INT $10\n"
    "net_mismatch IN IN\n"
    "net_mismatch INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices_with_different_parameters $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(5_BufferTwoPathsDifferentDeviceClasses)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOSB $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOSB $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOSB $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  //  NOTE: adding this power hint makes the device class error harder to detect
  const db::Circuit *ca = nl1.circuit_by_name ("BUF");
  const db::Circuit *cb = nl2.circuit_by_name ("BUF");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets INT $10\n"
    "match_nets IN IN\n"
    "net_mismatch OUT OUT\n"
    "net_mismatch INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices_with_different_device_classes $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(6_BufferTwoPathsAdditionalResistor)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOSB $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOSB $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOSB $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOSB $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device RES $9 (A=$10,B=$11) (R=42);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  //  Forcing the power nets into equality makes the resistor error harder to detect
  const db::Circuit *ca = nl1.circuit_by_name ("BUF");
  const db::Circuit *cb = nl2.circuit_by_name ("BUF");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "net_mismatch INT $10\n"
    "match_nets IN IN\n"
    "match_nets OUT OUT\n"
    "net_mismatch INT2 $11\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices $6 $7\n"
    "match_devices $8 $8\n"
    "device_mismatch (null) $9\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(6_BufferTwoPathsAdditionalDevices)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $9 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $7 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $9 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets INT $11\n"
    "net_mismatch VDD VDD\n"
    "match_nets IN IN\n"
    "net_mismatch VSS VSS\n"
    "net_mismatch OUT OUT\n"
    "net_mismatch INT2 $10\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $5 $1\n"
    "match_devices $7 $2\n"
    "device_mismatch (null) $3\n"
    "match_devices $1 $4\n"
    "match_devices $3 $5\n"
    "match_devices $6 $6\n"
    "match_devices $8 $7\n"
    "match_devices $2 $8\n"
    "match_devices $4 $9\n"
    "device_mismatch $9 (null)\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(7_Resistors)
{
  const char *nls1 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device RES $1 (A=P1,B=P2) (R=1.5);\n"
    "  device RES $2 (A=P2,B=P3) (R=2.5);\n"
    "  device RES $3 (A=P3,B=P1) (R=3);\n"
    "end;\n";

  const char *nls2 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device RES $2 (A=P2,B=P3) (R=2.5);\n"
    "  device RES $1 (A=P2,B=P1) (R=1.5);\n"
    "  device RES $3 (A=P3,B=P1) (R=3);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRIANGLE TRIANGLE\n"
    "match_nets P2 P2\n"
    "match_nets P1 P1\n"
    "match_nets P3 P3\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "match_devices $3 $3\n"
    "end_circuit TRIANGLE TRIANGLE MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(7_ResistorsParameterMismatch)
{
  const char *nls1 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device RES $1 (A=P1,B=P2) (R=1.5);\n"
    "  device RES $2 (A=P2,B=P3) (R=2.5);\n"
    "  device RES $3 (A=P3,B=P1) (R=3);\n"
    "end;\n";

  const char *nls2 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device RES $1 (A=P2,B=P1) (R=1.5);\n"
    "  device RES $3 (A=P3,B=P1) (R=3.5);\n"
    "  device RES $2 (A=P2,B=P3) (R=2.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRIANGLE TRIANGLE\n"
    "match_nets P2 P2\n"
    "net_mismatch P1 P1\n"
    "net_mismatch P3 P3\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_devices $1 $1\n"
    "match_devices_with_different_parameters $3 $2\n"
    "match_devices $2 $3\n"
    "end_circuit TRIANGLE TRIANGLE NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(7_ResistorsPlusOneDevice)
{
  const char *nls1 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device RES $1 (A=P1,B=P2) (R=1.5);\n"
    "  device RES $2 (A=P2,B=P3) (R=2.5);\n"
    "  device RES $3 (A=P3,B=P1) (R=3);\n"
    "end;\n";

  const char *nls2 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device RES $1 (A=P2,B=P1) (R=1.5);\n"
    "  device RES $3 (A=P3,B=P1) (R=3);\n"
    "  device CAP $4 (A=P1,B=P2) (C=1e-4);\n"
    "  device RES $2 (A=P2,B=P3) (R=2.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRIANGLE TRIANGLE\n"
    "match_nets P3 P3\n"
    "net_mismatch P2 P2\n"
    "net_mismatch P1 P1\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "device_mismatch (null) $3\n"
    "match_devices $2 $4\n"
    "end_circuit TRIANGLE TRIANGLE NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(8_Diodes)
{
  const char *nls1 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device DIODE $1 (A=P1,C=P2);\n"
    "  device DIODE $2 (A=P1,C=P3);\n"
    "  device DIODE $3 (A=P3,C=P2);\n"
    "end;\n";

  const char *nls2 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device DIODE $1 (A=P3,C=P2);\n"
    "  device DIODE $2 (A=P1,C=P3);\n"
    "  device DIODE $3 (A=P1,C=P2);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRIANGLE TRIANGLE\n"
    "match_nets P1 P1\n"
    "match_nets P3 P3\n"
    "match_nets P2 P2\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_devices $3 $1\n"
    "match_devices $2 $2\n"
    "match_devices $1 $3\n"
    "end_circuit TRIANGLE TRIANGLE MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(8_DiodesDontMatchOnSwappedPins)
{
  const char *nls1 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device DIODE $1 (A=P1,C=P2);\n"
    "  device DIODE $2 (A=P2,C=P3);\n"
    "  device DIODE $3 (A=P3,C=P1);\n"
    "end;\n";

  const char *nls2 =
    "circuit TRIANGLE ($0=P1,$1=P2,$2=P3);\n"
    "  device DIODE $3 (A=P3,C=P1);\n"
    "  device DIODE $1 (A=P2,C=P1);\n"
    "  device DIODE $2 (A=P2,C=P3);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRIANGLE TRIANGLE\n"
    "match_nets P1 P3\n"
    "net_mismatch P2 P1\n"
    "net_mismatch P3 P2\n"
    "match_pins $0 $2\n"
    "match_pins $1 $0\n"
    "match_pins $2 $1\n"
    "match_devices $1 $1\n"
    "device_mismatch (null) $2\n"
    "match_devices $3 $3\n"
    "device_mismatch $2 (null)\n"
    "end_circuit TRIANGLE TRIANGLE NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(10_SimpleSubCircuits)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $1 ($1=IN,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $2 ($1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($1=OUT,$2=VDD,$3=IN,$4=VSS);\n"
    "  subcircuit INV $1 ($1=VDD,$2=INT,$3=VSS,$4=OUT);\n"
    "  subcircuit INV $2 ($1=VDD,$2=IN,$3=VSS,$4=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV INV\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_pins $0 $2\n"
    "match_pins $1 $0\n"
    "match_pins $2 $1\n"
    "match_pins $3 $3\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(10_SimpleSubCircuitsMatchedNames)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $1 ($1=IN,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $2 ($1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INVB ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($1=OUT,$2=VDD,$3=IN,$4=VSS);\n"
    "  subcircuit INVB $1 ($1=VDD,$2=INT,$3=VSS,$4=OUT);\n"
    "  subcircuit INVB $2 ($1=VDD,$2=IN,$3=VSS,$4=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);
  EXPECT_EQ (good, false);

  logger.clear ();
  comp.same_circuits (nl1.circuit_by_name ("INV"), nl2.circuit_by_name ("INVB"));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV INVB\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INVB MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_pins $0 $2\n"
    "match_pins $1 $0\n"
    "match_pins $2 $1\n"
    "match_pins $3 $3\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(11_MismatchingSubcircuits)
{
  const char *nls1 =
    "circuit INV ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  subcircuit INV $1 ($1=IN,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $2 ($1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($0=VDD,$1=IN,$2=VSS,$3=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    //  wrong wiring:
    "  device PMOS $2 (S=IN,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=OUT,$1=VDD,$2=IN,$3=VSS);\n"
    "  subcircuit INV $1 ($1=VDD,$2=INT,$3=VSS,$4=OUT);\n"
    "  subcircuit INV $2 ($1=VDD,$2=IN,$3=VSS,$4=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV INV\n"
    "match_nets VSS VSS\n"
    "net_mismatch OUT OUT\n"
    "net_mismatch IN IN\n"
    "net_mismatch VDD VDD\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $2 $1\n"
    "device_mismatch (null) $2\n"
    "device_mismatch $1 (null)\n"
    "end_circuit INV INV NOMATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_pins $0 $2\n"
    "match_pins $1 $0\n"
    "match_pins $2 $1\n"
    "match_pins $3 $3\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, false);

  db::NetlistCrossReference xref;
  db::NetlistComparer comp_xref (&xref);

  good = comp_xref.compare (&nl1, &nl2);

  EXPECT_EQ (xref2s (xref),
    "INV:INV [NoMatch]:\n"
    " pin $0:$1 [Match]\n"
    " pin $1:$3 [Match]\n"
    " pin $2:$0 [Match]\n"
    " pin $3:$2 [Match]\n"
    " net IN:IN [Mismatch]\n"
    "  terminal (null):$2[S]\n"
    "  terminal (null):$2[G]\n"
    "  terminal $1[G]:(null)\n"
    "  terminal $2[G]:$1[G]\n"
    "  pin $0:$1\n"
    " net OUT:OUT [Mismatch]\n"
    "  terminal (null):$2[D]\n"
    "  terminal $1[D]:(null)\n"
    "  terminal $2[D]:$1[S]\n"
    "  pin $1:$3\n"
    " net VDD:VDD [Mismatch]\n"
    "  terminal $1[S]:(null)\n"
    "  pin $2:$0\n"
    " net VSS:VSS [Match]\n"
    "  terminal $2[S]:$1[D]\n"
    "  pin $3:$2\n"
    " device (null):$2 [Mismatch]\n"
    " device $1:(null) [Mismatch]\n"
    " device $2:$1 [Match]\n"
    "TOP:TOP [Match]:\n"
    " pin $0:$2 [Match]\n"
    " pin $1:$0 [Match]\n"
    " pin $2:$1 [Match]\n"
    " pin $3:$3 [Match]\n"
    " net IN:IN [Match]\n"
    "  pin $0:$2\n"
    "  subcircuit_pin $1[$0]:$2[$1]\n"
    " net INT:INT [Match]\n"
    "  subcircuit_pin $1[$1]:$2[$3]\n"
    "  subcircuit_pin $2[$0]:$1[$1]\n"
    " net OUT:OUT [Match]\n"
    "  pin $1:$0\n"
    "  subcircuit_pin $2[$1]:$1[$3]\n"
    " net VDD:VDD [Match]\n"
    "  pin $2:$1\n"
    "  subcircuit_pin $1[$2]:$2[$0]\n"
    "  subcircuit_pin $2[$2]:$1[$0]\n"
    " net VSS:VSS [Match]\n"
    "  pin $3:$3\n"
    "  subcircuit_pin $1[$3]:$2[$2]\n"
    "  subcircuit_pin $2[$3]:$1[$2]\n"
    " subcircuit $2:$1 [Match]\n"
    " subcircuit $1:$2 [Match]\n"
  );
  EXPECT_EQ (good, false);
}

TEST(12_MismatchingSubcircuitsDuplicates)
{
  const char *nls1 =
    "circuit INV ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  subcircuit INV $1 ($1=IN,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $2 ($1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $3 ($1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($0=VDD,$1=IN,$2=VSS,$3=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=OUT,$1=IN,$2=VDD,$3=VSS);\n"
    "  subcircuit INV $1 ($1=VDD,$2=INT,$3=VSS,$4=OUT);\n"
    "  subcircuit INV $2 ($1=VDD,$2=IN,$3=VSS,$4=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV INV\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets IN IN\n"
    "net_mismatch VDD VDD\n"
    "net_mismatch VSS VSS\n"
    "net_mismatch INT INT\n"
    "net_mismatch OUT OUT\n"
    "match_pins $0 $1\n"
    "match_pins $1 $0\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "subcircuit_mismatch $3 (null)\n"
    "end_circuit TOP TOP NOMATCH"
  );

  EXPECT_EQ (good, false);
}

TEST(13_MismatchingSubcircuitsAdditionalHierarchy)
{
  const char *nls1 =
    "circuit INV ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    //  a typical via:
    "circuit VIA ($0=IN);\n"
    "end;\n"
    "circuit TOP ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  subcircuit INV $1 ($0=IN,$1=INT,$2=VDD,$3=VSS);\n"
    "  subcircuit VIA $3 ($0=IN);\n"
    "  subcircuit INV $2 ($0=INT,$1=OUT,$2=VDD,$3=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($0=VDD,$1=IN,$2=VSS,$3=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=OUT,$1=IN,$2=VDD,$3=VSS);\n"
    "  subcircuit INV $1 ($0=VDD,$1=INT,$2=VSS,$3=OUT);\n"
    "  subcircuit INV $2 ($0=VDD,$1=IN,$2=VSS,$3=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "circuit_mismatch VIA (null)\n"
    "begin_circuit INV INV\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_pins $0 $1\n"
    "match_pins $1 $0\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_subcircuits $3 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, false);
}

TEST(14_Subcircuit2Nand)
{
  const char *nls1 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $1 ($0=IN1,$1=IN2,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=IN1,$1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=IN1,$1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $1 ($0=IN1,$1=IN2,$2=INT,$3=VDD,$4=VSS);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.equivalent_pins (nl2.circuit_by_name ("NAND"), 0, 1);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit NAND NAND\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets A A\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets INT INT\n"
    "match_nets IN2 IN2\n"
    "match_nets IN1 IN1\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(14_Subcircuit2NandMismatchNoSwap)
{
  const char *nls1 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $1 ($0=IN1,$1=IN2,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=IN1,$1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=INT,$1=IN1,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $1 ($0=IN2,$1=IN1,$2=INT,$3=VDD,$4=VSS);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  //  intentionally missing: comp.equivalent_pins (nl2.circuit_by_name ("NAND"), 0, 1);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit NAND NAND\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets A A\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "net_mismatch IN1 INT\n"
    "net_mismatch INT IN1\n"
    "net_mismatch VDD VDD\n"
    "net_mismatch VSS VSS\n"
    "net_mismatch IN2 IN2\n"
    "pin_mismatch $0 (null)\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "pin_mismatch (null) $0\n"
    "match_subcircuits $2 $1\n"
    "subcircuit_mismatch (null) $2\n"
    "subcircuit_mismatch $1 (null)\n"
    "end_circuit TOP TOP NOMATCH"
  );

  EXPECT_EQ (good, false);

  db::NetlistCrossReference xref;
  db::NetlistComparer comp_xref (&xref);

  good = comp_xref.compare (&nl1, &nl2);

  EXPECT_EQ (xref2s (xref),
    "NAND:NAND [Match]:\n"
    " pin $0:$0 [Match]\n"
    " pin $1:$1 [Match]\n"
    " pin $2:$2 [Match]\n"
    " pin $3:$3 [Match]\n"
    " pin $4:$4 [Match]\n"
    " net A:A [Match]\n"
    "  terminal $1[G]:$1[G]\n"
    "  terminal $3[G]:$3[G]\n"
    "  pin $0:$0\n"
    " net B:B [Match]\n"
    "  terminal $2[G]:$2[G]\n"
    "  terminal $4[G]:$4[G]\n"
    "  pin $1:$1\n"
    " net INT:INT [Match]\n"
    "  terminal $3[D]:$3[D]\n"
    "  terminal $4[S]:$4[S]\n"
    " net OUT:OUT [Match]\n"
    "  terminal $1[D]:$1[D]\n"
    "  terminal $2[D]:$2[D]\n"
    "  terminal $4[D]:$4[D]\n"
    "  pin $2:$2\n"
    " net VDD:VDD [Match]\n"
    "  terminal $1[S]:$1[S]\n"
    "  terminal $2[S]:$2[S]\n"
    "  pin $3:$3\n"
    " net VSS:VSS [Match]\n"
    "  terminal $3[S]:$3[S]\n"
    "  pin $4:$4\n"
    " device $1:$1 [Match]\n"
    " device $2:$2 [Match]\n"
    " device $3:$3 [Match]\n"
    " device $4:$4 [Match]\n"
    "TOP:TOP [NoMatch]:\n"
    " pin (null):$0 [Mismatch]\n"
    " pin $0:(null) [Mismatch]\n"
    " pin $1:$1 [Match]\n"
    " pin $2:$2 [Match]\n"
    " pin $3:$3 [Match]\n"
    " pin $4:$4 [Match]\n"
    " net IN1:INT [Mismatch]\n"
    "  pin $0:(null)\n"
    "  subcircuit_pin (null):$2[$2]\n"
    "  subcircuit_pin $1[$0]:(null)\n"
    "  subcircuit_pin $2[$0]:$1[$0]\n"
    " net IN2:IN2 [Mismatch]\n"
    "  pin $1:$1\n"
    "  subcircuit_pin (null):$2[$0]\n"
    "  subcircuit_pin $1[$1]:(null)\n"
    " net INT:IN1 [Mismatch]\n"
    "  pin (null):$0\n"
    "  subcircuit_pin (null):$2[$1]\n"
    "  subcircuit_pin $1[$2]:(null)\n"
    "  subcircuit_pin $2[$1]:$1[$1]\n"
    " net OUT:OUT [Match]\n"
    "  pin $2:$2\n"
    "  subcircuit_pin $2[$2]:$1[$2]\n"
    " net VDD:VDD [Mismatch]\n"
    "  pin $3:$3\n"
    "  subcircuit_pin (null):$2[$3]\n"
    "  subcircuit_pin $1[$3]:(null)\n"
    "  subcircuit_pin $2[$3]:$1[$3]\n"
    " net VSS:VSS [Mismatch]\n"
    "  pin $4:$4\n"
    "  subcircuit_pin (null):$2[$4]\n"
    "  subcircuit_pin $1[$4]:(null)\n"
    "  subcircuit_pin $2[$4]:$1[$4]\n"
    " subcircuit (null):$2 [Mismatch]\n"
    " subcircuit $1:(null) [Mismatch]\n"
    " subcircuit $2:$1 [Match]\n"
  );
  EXPECT_EQ (good, false);
}

TEST(14_Subcircuit2MatchWithSwap)
{
  const char *nls1 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $1 ($0=IN1,$1=IN2,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=IN1,$1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=INT,$1=IN1,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $1 ($0=IN2,$1=IN1,$2=INT,$3=VDD,$4=VSS);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.equivalent_pins (nl2.circuit_by_name ("NAND"), 0, 1);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit NAND NAND\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets A A\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets INT INT\n"
    "match_nets IN2 IN2\n"
    "match_nets IN1 IN1\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(15_EmptySubCircuitTest)
{
  const char *nls1 =
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 (S=$2,G=$4,D=IN);\n"
    "  subcircuit TRANS $2 (S=$2,G=$5,D=IN);\n"
    "  subcircuit TRANS $3 (S=$5,G=OUT,D=$2);\n"
    "  subcircuit TRANS $4 (S=$4,G=OUT,D=$2);\n"
    "end;\n"
    "circuit TRANS (S=$1,G=$2,D=$3);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $1 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 (G=$4,S=$2,D=IN);\n"
    "  subcircuit TRANS $2 (G=$5,S=$2,D=IN);\n"
    "  subcircuit TRANS $3 (G=OUT,S=$5,D=$2);\n"
    "  subcircuit TRANS $4 (G=OUT,S=$4,D=$2);\n"
    "end;\n"
    //  This circuit is an abstract and it's pins are defined by the pin names
    "circuit TRANS (G=$1,S=$2,D=$3);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRANS TRANS\n"
    "match_nets $3 $3\n"
    "match_nets $2 $1\n"
    "match_nets $1 $2\n"
    "match_pins S S\n"
    "match_pins G G\n"
    "match_pins D D\n"
    "end_circuit TRANS TRANS MATCH\n"
    "begin_circuit INV2 INV2\n"
    "match_nets $5 $5\n"
    "match_nets $2 $2\n"
    "match_nets IN IN\n"
    "match_nets $4 $4\n"
    "match_nets OUT OUT\n"
    "match_pins IN IN\n"
    "match_pins $1 $1\n"
    "match_pins OUT OUT\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $2 $3\n"
    "match_devices $4 $4\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "match_subcircuits $3 $3\n"
    "match_subcircuits $4 $4\n"
    "end_circuit INV2 INV2 MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(15_EmptySubCircuitWithoutPinNames)
{
  const char *nls1 =
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $1 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=OUT,$2=$5,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=OUT,$2=$4,$3=$2);\n"
    "end;\n"
    //  This circuit is an abstract and it's pins are not defined by the pin names ->
    //  they are internally marked as swappable
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRANS TRANS\n"
    "match_ambiguous_nets $1 $1\n"
    "match_ambiguous_nets $2 $2\n"
    "match_ambiguous_nets $3 $3\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "end_circuit TRANS TRANS MATCH\n"
    "begin_circuit INV2 INV2\n"
    "match_nets $5 $5\n"
    "match_nets $2 $2\n"
    "match_nets IN IN\n"
    "match_nets $4 $4\n"
    "match_nets OUT OUT\n"
    "match_pins IN IN\n"
    "match_pins $1 $1\n"
    "match_pins OUT OUT\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $2 $3\n"
    "match_devices $4 $4\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "match_subcircuits $3 $3\n"
    "match_subcircuits $4 $4\n"
    "end_circuit INV2 INV2 MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(16_UniqueSubCircuitMatching)
{
  const char *nls1 =
    "circuit RINGO ();\n"
    "  subcircuit INV2PAIR $1 (BULK='BULK,VSS',$2=FB,$3=VDD,$4='BULK,VSS',$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK='BULK,VSS',$2=$I22,$3=VDD,$4='BULK,VSS',$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK='BULK,VSS',$2=$I23,$3=VDD,$4='BULK,VSS',$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK='BULK,VSS',$2=$I24,$3=VDD,$4='BULK,VSS',$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK='BULK,VSS',$2=$I25,$3=VDD,$4='BULK,VSS',$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS4 $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS4 $2 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS4 $3 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS4 $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";
  const char *nls2 =
    "circuit RINGO ();\n"
    "  subcircuit INV2PAIR $1 (BULK='BULK,VSS',$2=FB,$3=VDD,$4='BULK,VSS',$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK='BULK,VSS',$2=$I24,$3=VDD,$4='BULK,VSS',$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK='BULK,VSS',$2=$I23,$3=VDD,$4='BULK,VSS',$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK='BULK,VSS',$2=$I22,$3=VDD,$4='BULK,VSS',$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK='BULK,VSS',$2=$I25,$3=VDD,$4='BULK,VSS',$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS4 $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS4 $2 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS4 $3 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS4 $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV2 INV2\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets $3 $3\n"
    "match_nets IN IN\n"
    "match_nets $1 $1\n"
    "match_nets VSS VSS\n"
    "match_nets BULK BULK\n"
    "match_pins $0 $0\n"
    "match_pins IN IN\n"
    "match_pins $2 $2\n"
    "match_pins OUT OUT\n"
    "match_pins VSS VSS\n"
    "match_pins VDD VDD\n"
    "match_pins BULK BULK\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $2 $3\n"
    "match_devices $4 $4\n"
    "end_circuit INV2 INV2 MATCH\n"
    "begin_circuit INV2PAIR INV2PAIR\n"
    "match_nets $I2 $I2\n"
    "match_nets $I1 $I1\n"
    "match_nets $I3 $I3\n"
    "match_nets $I7 $I7\n"
    "match_nets $I4 $I4\n"
    "match_nets $I5 $I5\n"
    "match_nets $I8 $I8\n"
    "match_nets $I6 $I6\n"
    "match_nets BULK BULK\n"
    "match_pins BULK BULK\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_pins $5 $5\n"
    "match_pins $6 $6\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit INV2PAIR INV2PAIR MATCH\n"
    "begin_circuit RINGO RINGO\n"
    "match_nets OSC OSC\n"
    "match_nets BULK,VSS BULK,VSS\n"
    "match_nets FB FB\n"
    "match_nets $I22 $I22\n"
    "match_nets VDD VDD\n"
    "match_nets $I13 $I13\n"
    "match_nets $I23 $I23\n"
    "match_nets $I5 $I5\n"
    "match_nets $I24 $I24\n"
    "match_nets $I6 $I6\n"
    "match_nets $I7 $I7\n"
    "match_nets $I25 $I25\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $4 $2\n"
    "match_subcircuits $3 $3\n"
    "match_subcircuits $2 $4\n"
    "match_subcircuits $5 $5\n"
    "end_circuit RINGO RINGO MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(17_InherentlyAmbiguousDecoder)
{
  const char *nls1 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit DECODER ($0=A,$1=B,$2=NQ0,$3=NQ1,$4=NQ2,$5=NQ3,$6=VDD,$7=VSS);\n"
    "  subcircuit NAND $1 ($0=A,$1=A,$2=NA,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=B,$1=B,$2=NB,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $3 ($0=NA,$1=NB,$2=NQ0,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $4 ($0=A,$1=NB,$2=NQ1,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $5 ($0=NA,$1=B,$2=NQ2,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $6 ($0=A,$1=B,$2=NQ3,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    //  BTW: this shows that pin swapping can't be automated ...
    "circuit DECODER ($0=B,$1=A,$2=NQ0,$3=NQ1,$4=NQ2,$5=NQ3,$6=VDD,$7=VSS);\n"
    "  subcircuit NAND $1 ($0=A,$1=A,$2=NA,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $2 ($0=B,$1=B,$2=NB,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $3 ($0=A,$1=NB,$2=NQ1,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $4 ($0=A,$1=B,$2=NQ3,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $5 ($0=NA,$1=NB,$2=NQ0,$3=VDD,$4=VSS);\n"
    "  subcircuit NAND $6 ($0=NA,$1=B,$2=NQ2,$3=VDD,$4=VSS);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.equivalent_pins (nl2.circuit_by_name ("NAND"), 0, 1);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit NAND NAND\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets A A\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit DECODER DECODER\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_ambiguous_nets A B\n"
    "match_ambiguous_nets B A\n"
    "match_nets NB NA\n"
    "match_nets NA NB\n"
    "match_nets NQ0 NQ0\n"
    "match_nets NQ2 NQ1\n"
    "match_nets NQ1 NQ2\n"
    "match_nets NQ3 NQ3\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $4\n"
    "match_pins $4 $3\n"
    "match_pins $5 $5\n"
    "match_pins $6 $6\n"
    "match_pins $7 $7\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "match_subcircuits $5 $3\n"
    "match_subcircuits $6 $4\n"
    "match_subcircuits $3 $5\n"
    "match_subcircuits $4 $6\n"
    "end_circuit DECODER DECODER MATCH"
  );

  EXPECT_EQ (good, true);

  logger.clear ();
  comp.same_nets (nl1.circuit_by_name ("DECODER")->net_by_name ("A"), nl2.circuit_by_name ("DECODER")->net_by_name ("A"));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit NAND NAND\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets A A\n"
    "match_pins $0 $0\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit DECODER DECODER\n"
    "match_nets NB NB\n"
    "match_nets B B\n"
    "match_nets NA NA\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets NQ0 NQ0\n"
    "match_nets NQ2 NQ2\n"
    "match_nets NQ1 NQ1\n"
    "match_nets NQ3 NQ3\n"
    "match_pins $0 $1\n"
    "match_pins $1 $0\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_pins $5 $5\n"
    "match_pins $6 $6\n"
    "match_pins $7 $7\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "match_subcircuits $4 $3\n"
    "match_subcircuits $6 $4\n"
    "match_subcircuits $3 $5\n"
    "match_subcircuits $5 $6\n"
    "end_circuit DECODER DECODER MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(18_ClockTree)
{
  const char *nls1 =
    "circuit INV (IN=IN,OUT=OUT,VDD=VDD,VSS=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TREE ();\n"
    "  subcircuit INV T (IN=IN,OUT=S,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TR (IN=S,OUT=SR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TL (IN=S,OUT=SL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRR (IN=SR,OUT=SRR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRL (IN=SR,OUT=SRL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLR (IN=SL,OUT=SLR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLL (IN=SL,OUT=SLL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRRR (IN=SRR,OUT=SRRR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRRL (IN=SRR,OUT=SRRL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRLR (IN=SRL,OUT=SRLR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRLL (IN=SRL,OUT=SRLL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLRR (IN=SLR,OUT=SLRR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLRL (IN=SLR,OUT=SLRL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLLR (IN=SLL,OUT=SLLR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLLL (IN=SLL,OUT=SLLL,VDD=VDD,VSS=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV (IN=IN,OUT=OUT,VDD=VDD,VSS=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TREE ();\n"
    "  subcircuit INV TLRR (IN=SLR,OUT=SLRR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TR (IN=S,OUT=SR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRRL (IN=SRR,OUT=SRRL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRLR (IN=SRL,OUT=SRLR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLR (IN=SL,OUT=SLR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLL (IN=SL,OUT=SLL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRRR (IN=SRR,OUT=SRRR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLLL (IN=SLL,OUT=SLLL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLRL (IN=SLR,OUT=SLRL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV T (IN=IN,OUT=S,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TLLR (IN=SLL,OUT=SLLR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TL (IN=S,OUT=SL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRR (IN=SR,OUT=SRR,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRLL (IN=SRL,OUT=SRLL,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV TRL (IN=SR,OUT=SRL,VDD=VDD,VSS=VSS);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  std::string txt = logger.text ();
  //  because L/R matching is ambiguous, we need to do this to
  //  establish reproducability on different platforms:
  txt = tl::replaced (txt, "L", "X");
  txt = tl::replaced (txt, "R", "X");

  EXPECT_EQ (txt,
    "begin_circuit INV INV\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_pins IN IN\n"
    "match_pins OUT OUT\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TXEE TXEE\n"
    "match_nets IN IN\n"
    "match_nets S S\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_ambiguous_nets SX SX\n"
    "match_ambiguous_nets SX SX\n"
    "match_ambiguous_nets SXX SXX\n"
    "match_ambiguous_nets SXX SXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXX SXX\n"
    "match_ambiguous_nets SXX SXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits TX TX\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits TXX TXX\n"
    "match_subcircuits TXX TXX\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits T T\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits TX TX\n"
    "match_subcircuits TXX TXX\n"
    "match_subcircuits TXXX TXXX\n"
    "match_subcircuits TXX TXX\n"
    "end_circuit TXEE TXEE MATCH"
  );
  EXPECT_EQ (good, true);
}

