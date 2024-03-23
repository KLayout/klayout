
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

#include "tlUnitTest.h"
#include "dbNetlistDeviceClasses.h"
#include "dbNetlistCompare.h"
#include "dbNetlistCrossReference.h"
#include "dbNetlistSpiceReader.h"
#include "dbNetlistCompareUtils.h"

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

  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching, const std::string & /*msg*/)
  {
    out ("end_circuit " + circuit2str (a) + " " + circuit2str (b) + " " + (matching ? "MATCH" : "NOMATCH"));
  }

  virtual void circuit_skipped (const db::Circuit *a, const db::Circuit *b, const std::string & /*msg*/)
  {
    out ("circuit_skipped " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b, const std::string & /*msg*/)
  {
    out ("circuit_mismatch " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void device_class_mismatch (const db::DeviceClass *a, const db::DeviceClass *b, const std::string & /*msg*/)
  {
    out ("device_class_mismatch " + dc2str (a) + " " + dc2str (b));
  }

  virtual void match_nets (const db::Net *a, const db::Net *b)
  {
    out ("match_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b, const std::string & /*msg*/)
  {
    out ("match_ambiguous_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void net_mismatch (const db::Net *a, const db::Net *b, const std::string & /*msg*/)
  {
    out ("net_mismatch " + net2str (a) + " " + net2str (b));
  }

  virtual void match_devices (const db::Device *a, const db::Device *b)
  {
    out ("match_devices " + device2str (a) + " " + device2str (b));
  }

  virtual void device_mismatch (const db::Device *a, const db::Device *b, const std::string & /*msg*/)
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

  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b, const std::string & /*msg*/)
  {
    out ("pin_mismatch " + pin2str (a) + " " + pin2str (b));
  }

  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    out ("match_subcircuits " + subcircuit2str (a) + " " + subcircuit2str (b));
  }

  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b, const std::string & /*msg*/)
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

  EXPECT_EQ (dc.equal (d1, d2), false);
  EXPECT_EQ (dc.equal (d2, d1), false);
  EXPECT_EQ (dc.less (d1, d2), false);
  EXPECT_EQ (dc.less (d2, d1), true);

  *eqp += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_W, true);  //  ignore W

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

TEST(0_NetNameEquivalence)
{
  db::Netlist a, b;
  a.set_case_sensitive (true);
  b.set_case_sensitive (false);

  EXPECT_EQ (db::combined_case_sensitive (&a, &b), false);

  b.set_case_sensitive (true);
  EXPECT_EQ (db::combined_case_sensitive (&a, &b), true);

  a.set_case_sensitive (false);
  EXPECT_EQ (db::combined_case_sensitive (&a, &b), false);

  db::Circuit *ca = new db::Circuit ();
  ca->set_name ("C");
  a.add_circuit (ca);

  db::Circuit *cb = new db::Circuit ();
  cb->set_name ("C");
  b.add_circuit (cb);

  db::Net *na = new db::Net ("net1");
  ca->add_net (na);

  db::Net *nb = new db::Net ("net1");
  cb->add_net (nb);

  EXPECT_EQ (db::name_compare (na, nb), 0);

  nb->set_name ("NET1");
  EXPECT_EQ (db::name_compare (na, nb), 0);

  nb->set_name ("NET2");
  EXPECT_EQ (db::name_compare (na, nb), -1);

  nb->set_name ("NET11");
  EXPECT_EQ (db::name_compare (na, nb), -1);

  nb->set_name ("net11");
  EXPECT_EQ (db::name_compare (na, nb), -1);

  nb->set_name ("net0abc");
  EXPECT_EQ (db::name_compare (na, nb), 1);

  nb->set_name ("NET0");
  EXPECT_EQ (db::name_compare (na, nb), 1);

  a.set_case_sensitive (true);
  b.set_case_sensitive (true);

  nb->set_name ("net1");
  EXPECT_EQ (db::name_compare (na, nb), 0);

  nb->set_name ("net2");
  EXPECT_EQ (db::name_compare (na, nb), -1);

  nb->set_name ("net11");
  EXPECT_EQ (db::name_compare (na, nb), -1);

  nb->set_name ("net0");
  EXPECT_EQ (db::name_compare (na, nb), 1);

  nb->set_name ("NET1");
  EXPECT_EQ (db::name_compare (na, nb), 1);

  na->set_name ("NET1");
  nb->set_name ("net1");
  EXPECT_EQ (db::name_compare (na, nb), -1);

  b.set_case_sensitive (false);

  //  colon terminates the net name, so that NET:I and NET and identical

  na->set_name ("NET1:I");
  nb->set_name ("net1");
  EXPECT_EQ (db::name_compare (na, nb), 0);

  na->set_name ("NET1:I");
  nb->set_name ("net1:O");
  EXPECT_EQ (db::name_compare (na, nb), -1);

  na->set_name ("NET1");
  nb->set_name ("net1:O");
  EXPECT_EQ (db::name_compare (na, nb), 0);

  na->set_name ("NET2");
  nb->set_name ("net1:O");
  EXPECT_EQ (db::name_compare (na, nb), 1);

  na->set_name ("NET1");
  nb->set_name ("net1abc:O");
  EXPECT_EQ (db::name_compare (na, nb), -1);
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
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);
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
  comp.set_dont_consider_net_names (true);

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
     "match_devices $1 $4\n"
     "device_mismatch (null) $3\n"
     "end_circuit INV INV NOMATCH"
  );
  EXPECT_EQ (good, false);

  db::NetlistCrossReference xref;
  db::NetlistComparer comp_xref (&xref);
  comp.set_dont_consider_net_names (true);

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
    " device $2:$2 [MatchWithWarning]\n"
    " device $3:$1 [Match]\n"
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

  xref.clear ();

  comp_xref.exclude_caps (-1);
  comp_xref.exclude_resistors (-1);
  comp_xref.same_device_classes (0, nl2.device_class_by_name ("RES"));
  comp_xref.same_device_classes (0, nl2.device_class_by_name ("CAP"));
  comp_xref.same_device_classes (nl1.device_class_by_name ("RES"), 0);
  comp_xref.same_device_classes (nl1.device_class_by_name ("CAP"), 0);

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
  comp.set_dont_consider_net_names (true);

  const db::Circuit *ca = nl1.circuit_by_name ("INV");
  const db::Circuit *cb = nl2.circuit_by_name ("INV");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV INV\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
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
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);

  //  Forcing the power nets into equality makes the parameter error harder to detect
  const db::Circuit *ca = nl1.circuit_by_name ("BUF");
  const db::Circuit *cb = nl2.circuit_by_name ("BUF");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "net_mismatch IN IN\n"
    "match_nets INT $10\n"
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
  db::EqualDeviceParameters *eql = new db::EqualDeviceParameters ();
  *eql += db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 1.5, 0.0);
  nl1.device_class_by_name ("NMOS")->set_parameter_compare_delegate (eql);
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
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
  nl1.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.0, 0.0));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "net_mismatch IN IN\n"
    "match_nets INT $10\n"
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
  nl1.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.0, 0.2));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "net_mismatch IN IN\n"
    "match_nets INT $10\n"
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
  nl1.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassMOS3Transistor::param_id_L, 0.0, 0.4));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
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
  nl1.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (eq_dp));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
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
  nl1.device_class_by_name ("NMOS")->set_parameter_compare_delegate (new db::EqualDeviceParameters (eq_dp));
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "net_mismatch IN IN\n"
    "match_nets INT $10\n"
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
  comp.set_dont_consider_net_names (true);

  //  NOTE: adding this power hint makes the device class error harder to detect
  const db::Circuit *ca = nl1.circuit_by_name ("BUF");
  const db::Circuit *cb = nl2.circuit_by_name ("BUF");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets INT $10\n"
    "match_nets IN IN\n"
    "net_mismatch INT2 $11\n"
    "net_mismatch OUT OUT\n"
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
  comp.set_dont_consider_net_names (true);

  //  Forcing the power nets into equality makes the resistor error harder to detect
  const db::Circuit *ca = nl1.circuit_by_name ("BUF");
  const db::Circuit *cb = nl2.circuit_by_name ("BUF");
  comp.same_nets (ca->net_by_name ("VDD"), cb->net_by_name ("VDD"));
  comp.same_nets (ca->net_by_name ("VSS"), cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "net_mismatch INT $10\n"
    "match_nets IN IN\n"
    "net_mismatch INT2 $11\n"
    "match_nets OUT OUT\n"
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

  comp.set_depth_first (false);
  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "net_mismatch INT $10\n"
    "match_nets OUT OUT\n"
    "net_mismatch INT2 $11\n"
    "match_nets IN IN\n"
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
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets INT $11\n"
    "net_mismatch VDD VDD\n"
    "match_nets IN IN\n"
    "net_mismatch INT2 $10\n"
    "net_mismatch VSS VSS\n"
    "net_mismatch OUT OUT\n"
    "match_pins $0 $1\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_devices $5 $1\n"
    "match_devices $7 $2\n"
    "match_devices $1 $4\n"
    "match_devices $3 $5\n"
    "match_devices $6 $6\n"
    "match_devices $8 $7\n"
    "match_devices $2 $8\n"
    "match_devices $4 $9\n"
    "device_mismatch (null) $3\n"
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
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);

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
    "match_devices $2 $4\n"
    "device_mismatch (null) $3\n"
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
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit TRIANGLE TRIANGLE\n"
    "net_mismatch P1 (null)\n"
    "net_mismatch P2 (null)\n"
    "net_mismatch P3 (null)\n"
    "net_mismatch (null) P1\n"
    "net_mismatch (null) P2\n"
    "net_mismatch (null) P3\n"
    "match_pins $0 (null)\n"
    "match_pins $1 (null)\n"
    "match_pins $2 (null)\n"
    "match_pins (null) $0\n"
    "match_pins (null) $1\n"
    "match_pins (null) $2\n"
    "device_mismatch $1 $1\n"
    "device_mismatch $2 $2\n"
    "device_mismatch $3 $3\n"
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
  comp.set_dont_consider_net_names (true);

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
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets IN IN\n"
    "match_nets VDD VDD\n"
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
  comp.set_dont_consider_net_names (true);

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
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets IN IN\n"
    "match_nets VDD VDD\n"
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
    "  subcircuit INV $1 ($0=IN,$1=INT,$2=VDD,$3=VSS);\n"
    "  subcircuit INV $2 ($0=INT,$1=OUT,$2=VDD,$3=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($0=VDD,$1=IN,$2=VSS,$3=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    //  wrong wiring:
    "  device PMOS $2 (S=IN,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=OUT,$1=VDD,$2=IN,$3=VSS);\n"
    "  subcircuit INV $1 ($0=VDD,$1=INT,$2=VSS,$3=OUT);\n"
    "  subcircuit INV $2 ($0=VDD,$1=IN,$2=VSS,$3=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

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
    "device_mismatch $1 $2\n"
    "end_circuit INV INV NOMATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets IN IN\n"
    "match_nets VDD VDD\n"
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
  comp_xref.set_dont_consider_net_names (true);

  good = comp_xref.compare (&nl1, &nl2);

  EXPECT_EQ (xref2s (xref),
    "INV:INV [NoMatch]:\n"
    " pin $0:$1 [Match]\n"
    " pin $1:$3 [Match]\n"
    " pin $2:$0 [Match]\n"
    " pin $3:$2 [Match]\n"
    " net IN:IN [Mismatch]\n"
    "  terminal (null):$2[S]\n"
    "  terminal $1[G]:$2[G]\n"
    "  terminal $2[G]:$1[G]\n"
    "  pin $0:$1\n"
    " net OUT:OUT [Mismatch]\n"
    "  terminal $1[D]:$2[D]\n"
    "  terminal $2[D]:$1[S]\n"
    "  pin $1:$3\n"
    " net VDD:VDD [Mismatch]\n"
    "  terminal $1[S]:(null)\n"
    "  pin $2:$0\n"
    " net VSS:VSS [Match]\n"
    "  terminal $2[S]:$1[D]\n"
    "  pin $3:$2\n"
    " device $2:$1 [Match]\n"
    " device $1:$2 [Mismatch]\n"
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

  xref.clear ();

  //  ignore the subcircuits to make them match
  comp_xref.same_circuits (nl1.circuit_by_name ("INV"), 0);
  comp_xref.same_circuits (0, nl2.circuit_by_name ("INV"));
  good = comp_xref.compare (&nl1, &nl2);

  //  nets are now ambiguous
  EXPECT_EQ (xref2s (xref),
    "TOP:TOP [Match]:\n"
    " pin $0:$2 [Match]\n"
    " pin $1:$0 [Match]\n"
    " pin $2:$1 [Match]\n"
    " pin $3:$3 [Match]\n"
    " net IN:IN [MatchWithWarning]\n"
    "  pin $0:$2\n"
    "  subcircuit_pin (null):$2[$1]\n"
    "  subcircuit_pin $1[$0]:(null)\n"
    " net OUT:OUT [MatchWithWarning]\n"
    "  pin $1:$0\n"
    "  subcircuit_pin (null):$1[$3]\n"
    "  subcircuit_pin $2[$1]:(null)\n"
    " net VDD:VDD [MatchWithWarning]\n"
    "  pin $2:$1\n"
    "  subcircuit_pin (null):$1[$0]\n"
    "  subcircuit_pin (null):$2[$0]\n"
    "  subcircuit_pin $1[$2]:(null)\n"
    "  subcircuit_pin $2[$2]:(null)\n"
    " net VSS:VSS [MatchWithWarning]\n"
    "  pin $3:$3\n"
    "  subcircuit_pin (null):$1[$2]\n"
    "  subcircuit_pin (null):$2[$2]\n"
    "  subcircuit_pin $1[$3]:(null)\n"
    "  subcircuit_pin $2[$3]:(null)\n"
  );
  EXPECT_EQ (good, true);
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
  comp.set_dont_consider_net_names (true);

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
    "net_mismatch INT INT\n"
    "net_mismatch VSS VSS\n"
    "net_mismatch VDD VDD\n"
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
  comp.set_dont_consider_net_names (true);

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
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets IN IN\n"
    "match_nets VDD VDD\n"
    "match_pins $0 $1\n"
    "match_pins $1 $0\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "subcircuit_mismatch $2 (null)\n"
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
  comp.set_dont_consider_net_names (true);
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
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets INT INT\n"
    "match_nets IN2 IN2\n"
    "match_nets IN1 IN1\n"
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
  comp.set_dont_consider_net_names (true);
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
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "net_mismatch INT IN1\n"
    "net_mismatch IN1 INT\n"
    "net_mismatch IN2 IN2\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $3 $3\n"
    "match_pins $4 $4\n"
    "match_pins $0 (null)\n"
    "match_pins (null) $0\n"
    "match_subcircuits $2 $1\n"
    "subcircuit_mismatch $1 $2\n"
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
    " device $3:$3 [Match]\n"
    " device $4:$4 [Match]\n"
    " device $1:$1 [Match]\n"
    " device $2:$2 [Match]\n"
    "TOP:TOP [NoMatch]:\n"
    " pin (null):$0 [Match]\n"
    " pin $0:(null) [Match]\n"
    " pin $1:$1 [Match]\n"
    " pin $2:$2 [Match]\n"
    " pin $3:$3 [Match]\n"
    " pin $4:$4 [Match]\n"
    " net IN1:INT [Mismatch]\n"
    "  pin $0:(null)\n"
    "  subcircuit_pin $1[$0]:$2[$2]\n"
    "  subcircuit_pin $2[$0]:$1[$0]\n"
    " net IN2:IN2 [Mismatch]\n"
    "  pin $1:$1\n"
    "  subcircuit_pin $1[$1]:$2[$0]\n"
    " net INT:IN1 [Mismatch]\n"
    "  pin (null):$0\n"
    "  subcircuit_pin $1[$2]:$2[$1]\n"
    "  subcircuit_pin $2[$1]:$1[$1]\n"
    " net OUT:OUT [Match]\n"
    "  pin $2:$2\n"
    "  subcircuit_pin $2[$2]:$1[$2]\n"
    " net VDD:VDD [Match]\n"
    "  pin $3:$3\n"
    "  subcircuit_pin $1[$3]:$2[$3]\n"
    "  subcircuit_pin $2[$3]:$1[$3]\n"
    " net VSS:VSS [Match]\n"
    "  pin $4:$4\n"
    "  subcircuit_pin $1[$4]:$2[$4]\n"
    "  subcircuit_pin $2[$4]:$1[$4]\n"
    " subcircuit $2:$1 [Match]\n"
    " subcircuit $1:$2 [Mismatch]\n"
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
  comp.set_dont_consider_net_names (true);
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
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets INT INT\n"
    "match_nets IN2 IN2\n"
    "match_nets IN1 IN1\n"
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
    //  This circuit is an abstract and its pins are defined by the pin names
    "circuit TRANS (G=$1,S=$2,D=$3);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

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
    //  This circuit is an abstract and its pins are not defined by the pin names ->
    //  they are internally marked as swappable
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

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
  comp.set_dont_consider_net_names (true);

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
    "match_nets BULK BULK\n"
    "match_nets $I6 $I6\n"
    "match_nets $I5 $I5\n"
    "match_nets $I4 $I4\n"
    "match_nets $I3 $I3\n"
    "match_nets $I7 $I7\n"
    "match_nets $I1 $I1\n"
    "match_nets $I8 $I8\n"
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
    "match_nets VDD VDD\n"
    "match_nets FB FB\n"
    "match_nets $I13 $I13\n"
    "match_nets $I5 $I5\n"
    "match_nets $I6 $I6\n"
    "match_nets $I7 $I7\n"
    "match_nets BULK,VSS BULK,VSS\n"
    "match_nets $I25 $I25\n"
    "match_nets $I24 $I24\n"
    "match_nets $I23 $I23\n"
    "match_nets $I22 $I22\n"
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
  comp.set_dont_consider_net_names (true);

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
    "match_nets NQ0 NQ0\n"
    "match_nets NQ1 NQ1\n"
    "match_nets NQ2 NQ2\n"
    "match_nets NQ3 NQ3\n"
    "match_nets NA NA\n"
    "match_nets NB NB\n"
    "match_nets A A\n"
    "match_nets B B\n"
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

  comp.set_dont_consider_net_names (false);

  logger.clear ();
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
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets NQ0 NQ0\n"
    "match_nets NQ1 NQ1\n"
    "match_nets NQ2 NQ2\n"
    "match_nets NQ3 NQ3\n"
    "match_nets NA NA\n"
    "match_nets NB NB\n"
    "match_nets A A\n"
    "match_nets B B\n"
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

  logger.clear ();
  comp.set_dont_consider_net_names (true);
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
    "match_nets A A\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets NB NB\n"
    "match_nets NA NA\n"
    "match_nets NQ1 NQ1\n"
    "match_nets NQ3 NQ3\n"
    "match_nets NQ2 NQ2\n"
    "match_nets NQ0 NQ0\n"
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

  comp.set_depth_first (false);
  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit NAND NAND\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets OUT OUT\n"
    "match_nets A A\n"
    "match_nets INT INT\n"
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
    "match_nets A A\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets NA NA\n"
    "match_nets NB NB\n"
    "match_nets B B\n"
    "match_nets NQ1 NQ1\n"
    "match_nets NQ3 NQ3\n"
    "match_nets NQ2 NQ2\n"
    "match_nets NQ0 NQ0\n"
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
  comp.set_dont_consider_net_names (false);

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
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets S S\n"
    "match_nets SX SX\n"
    "match_nets SX SX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXXX SXXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
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

  logger.clear ();

  comp.set_dont_consider_net_names (true);
  good = comp.compare (&nl1, &nl2);

  txt = logger.text ();
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
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets S S\n"
    "match_ambiguous_nets SX SX\n"
    "match_ambiguous_nets SX SX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
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

  comp.set_depth_first (false);
  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  txt = logger.text ();
  //  because L/R matching is ambiguous, we need to do this to
  //  establish reproducability on different platforms:
  txt = tl::replaced (txt, "L", "X");
  txt = tl::replaced (txt, "R", "X");

  EXPECT_EQ (txt,
    "begin_circuit INV INV\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_pins IN IN\n"
    "match_pins OUT OUT\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TXEE TXEE\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets S S\n"
    "match_ambiguous_nets SX SX\n"
    "match_ambiguous_nets SX SX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
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

  comp.set_depth_first (false);
  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  txt = logger.text ();
  //  because L/R matching is ambiguous, we need to do this to
  //  establish reproducability on different platforms:
  txt = tl::replaced (txt, "L", "X");
  txt = tl::replaced (txt, "R", "X");

  EXPECT_EQ (txt,
    "begin_circuit INV INV\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_pins IN IN\n"
    "match_pins OUT OUT\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TXEE TXEE\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets S S\n"
    "match_ambiguous_nets SX SX\n"
    "match_ambiguous_nets SX SX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_nets SXX SXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
    "match_ambiguous_nets SXXX SXXX\n"
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

TEST(19_SymmetricCircuit)
{
  //  Test test requires a certain depth, name sensitivity to resolve ambiguities and
  //  tests the backtracking paths.

  const char *nls1 =
      "circuit DECODE (VDD=VDD,nn1_=nn1_,nn1=nn1,q0=q0,q0_=q0_,q1_=q1_,q1=q1,nn2=nn2,nn2_=nn2_,a0=a0,a0_=a0_,g1=g1,g0=g0,gtp=gtp,VSS=VSS,WELL=$14);"
      "  device PMOS4 $1 (S=VDD,G=$44,D=q1_,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 $2 (S=q1_,G=$44,D=q1,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 $3 (S=q1,G=$44,D=VDD,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 $4 (S=VDD,G=$44,D=q0,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 $5 (S=q0,G=$44,D=q0_,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 $6 (S=q0_,G=$44,D=VDD,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 $7 (S=q0_,G=$11,D=nn2_,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 $8 (S=nn2_,G=$13,D=q1_,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 $9 (S=VDD,G=g0,D=$39,B=VDD) (L=1.2,W=3);"
      "  device PMOS4 $10 (S=$39,G=g1,D=VDD,B=VDD) (L=1.2,W=3);"
      "  device PMOS4 $11 (S=VDD,G=a0_,D=$11,B=VDD) (L=1.2,W=10);"
      "  device PMOS4 $12 (S=$11,G=$44,D=VDD,B=VDD) (L=1.2,W=10);"
      "  device PMOS4 $13 (S=q0,G=$11,D=nn2,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 $14 (S=nn1_,G=$13,D=$9,B=$14) (L=1.2,W=11);"
      "  device PMOS4 $15 (S=$4,G=$11,D=nn1_,B=$14) (L=1.2,W=11);"
      "  device PMOS4 $16 (S=VDD,G=$44,D=$13,B=VDD) (L=1.2,W=10);"
      "  device PMOS4 $17 (S=$13,G=a0,D=VDD,B=VDD) (L=1.2,W=10);"
      "  device PMOS4 $18 (S=$6,G=$11,D=nn1,B=$14) (L=1.2,W=11);"
      "  device PMOS4 $19 (S=nn1,G=$13,D=$8,B=$14) (L=1.2,W=11);"
      "  device PMOS4 $20 (S=VDD,G=$41,D=$44,B=VDD) (L=1.2,W=15);"
      "  device PMOS4 $21 (S=nn2,G=$13,D=q1,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 $22 (S=VDD,G=$39,D=$37,B=VDD) (L=1.2,W=6);"
      "  device PMOS4 $23 (S=VDD,G=$42,D=$41,B=VDD) (L=1.2,W=3);"
      "  device PMOS4 $24 (S=gtp,G=$39,D=$42,B=VDD) (L=1.2,W=16);"
      "  device NMOS4 $25 (S=$44,G=$41,D=VSS,B=VSS) (L=1.2,W=22);"
      "  device NMOS4 $26 (S=VSS,G=$39,D=$37,B=VSS) (L=1.2,W=9);"
      "  device NMOS4 $27 (S=q0_,G=$6,D=VSS,B=VSS) (L=1.2,W=1);"
      "  device NMOS4 $28 (S=VSS,G=$8,D=q1_,B=VSS) (L=1.2,W=1);"
      "  device NMOS4 $29 (S=$13,G=$44,D=$35,B=VSS) (L=1.2,W=19);"
      "  device NMOS4 $30 (S=$35,G=a0,D=VSS,B=VSS) (L=1.2,W=19);"
      "  device NMOS4 $31 (S=gtp,G=$37,D=$42,B=VSS) (L=1.2,W=16);"
      "  device NMOS4 $32 (S=VSS,G=$39,D=$42,B=VSS) (L=1.2,W=20);"
      "  device NMOS4 $33 (S=VSS,G=$4,D=q0,B=VSS) (L=1.2,W=1);"
      "  device NMOS4 $34 (S=VSS,G=$11,D=$6,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 $35 (S=$9,G=$13,D=VSS,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 $36 (S=VSS,G=$11,D=$4,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 $37 (S=q1,G=$9,D=VSS,B=VSS) (L=1.2,W=1);"
      "  device NMOS4 $38 (S=$8,G=$13,D=VSS,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 $39 (S=VSS,G=a0_,D=$34,B=VSS) (L=1.2,W=19);"
      "  device NMOS4 $40 (S=$34,G=$44,D=$11,B=VSS) (L=1.2,W=19);"
      "  device NMOS4 $41 (S=VSS,G=g0,D=$40,B=VSS) (L=1.2,W=17);"
      "  device NMOS4 $42 (S=$40,G=g1,D=$39,B=VSS) (L=1.2,W=17);"
      "  device NMOS4 $43 (S=VSS,G=$42,D=$41,B=VSS) (L=1.2,W=2);"
      "end;"
    ;

  const char *nls2 =
      "circuit DECODE (A0=A0,A0_=A0_,Q0=Q0,Q0_=Q0_,Q1=Q1,Q1_=Q1_,NN2=NN2,NN2_=NN2_,NN1=NN1,NN1_=NN1_,G0=G0,G1=G1,NN3=NN3,VDD=VDD,VSS=VSS,WELL=WELL);"
      "  device NMOS4 '0' (S=HNET44,G=A0,D=VSS,B=VSS) (L=1.2,W=19);"
      "  device NMOS4 '1' (S=CS1,G=YI,D=HNET44,B=VSS) (L=1.2,W=19);"
      "  device PMOS4 '10' (S=VDD,G=G0,D=NET194,B=VDD) (L=1.2,W=3);"
      "  device PMOS4 '11' (S=VDD,G=G1,D=NET194,B=VDD) (L=1.2,W=3);"
      "  device NMOS4 '12' (S=NET200,G=CS0,D=VSS,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 '13' (S=VSS,G=CS1,D=NET175,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 '14' (S=VSS,G=CS0,D=NET181,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 '15' (S=NET215,G=CS1,D=VSS,B=VSS) (L=1.2,W=18);"
      "  device NMOS4 '16' (S=Q1,G=NET175,D=VSS,B=VSS) (L=1.2,W=1);"
      "  device NMOS4 '17' (S=VSS,G=NET200,D=Q0,B=VSS) (L=1.2,W=1);"
      "  device NMOS4 '18' (S=Q0_,G=NET181,D=VSS,B=VSS) (L=1.2,W=1);"
      "  device NMOS4 '19' (S=VSS,G=NET215,D=Q1_,B=VSS) (L=1.2,W=1);"
      "  device PMOS4 '2' (S=VDD,G=A0,D=CS1,B=VDD) (L=1.2,W=10);"
      "  device NMOS4 '20' (S=NET189,G=NET193,D=NN3,B=VSS) (L=1.2,W=16);"
      "  device NMOS4 '21' (S=VSS,G=NET194,D=NET189,B=VSS) (L=1.2,W=20);"
      "  device PMOS4 '22' (S=VDD,G=NET194,D=NET193,B=VDD) (L=1.2,W=6);"
      "  device NMOS4 '23' (S=NET193,G=NET194,D=VSS,B=VSS) (L=1.2,W=9);"
      "  device PMOS4 '24' (S=VDD,G=NET189,D=WL1_EN_,B=VDD) (L=1.2,W=3);"
      "  device NMOS4 '25' (S=WL1_EN_,G=NET189,D=VSS,B=VSS) (L=1.2,W=2);"
      "  device PMOS4 '26' (S=VDD,G=WL1_EN_,D=YI,B=VDD) (L=1.2,W=15);"
      "  device NMOS4 '27' (S=YI,G=WL1_EN_,D=VSS,B=VSS) (L=1.2,W=22);"
      "  device PMOS4 '28' (S=NN1_,G=CS0,D=NET200,B=WELL) (L=1.2,W=11);"
      "  device PMOS4 '29' (S=NET175,G=CS1,D=NN1_,B=WELL) (L=1.2,W=11);"
      "  device PMOS4 '3' (S=VDD,G=YI,D=CS1,B=VDD) (L=1.2,W=10);"
      "  device PMOS4 '30' (S=NET181,G=CS0,D=NN1,B=WELL) (L=1.2,W=11);"
      "  device PMOS4 '31' (S=Q1,G=CS1,D=NN2,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 '32' (S=NN2,G=CS0,D=Q0,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 '33' (S=NN1,G=CS1,D=NET215,B=WELL) (L=1.2,W=11);"
      "  device PMOS4 '34' (S=Q0_,G=CS0,D=NN2_,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 '35' (S=NN2_,G=CS1,D=Q1_,B=VDD) (L=1.2,W=5);"
      "  device PMOS4 '36' (S=NN3,G=NET194,D=NET189,B=VDD) (L=1.2,W=16);"
      "  device PMOS4 '37' (S=VDD,G=YI,D=Q1,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 '38' (S=VDD,G=YI,D=Q0_,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 '39' (S=VDD,G=YI,D=Q0,B=VDD) (L=1.2,W=7);"
      "  device NMOS4 '4' (S=HNET48,G=A0_,D=VSS,B=VSS) (L=1.2,W=19);"
      "  device PMOS4 '40' (S=Q0_,G=YI,D=Q0,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 '41' (S=Q1,G=YI,D=Q1_,B=VDD) (L=1.2,W=7);"
      "  device PMOS4 '42' (S=VDD,G=YI,D=Q1_,B=VDD) (L=1.2,W=7);"
      "  device NMOS4 '5' (S=CS0,G=YI,D=HNET48,B=VSS) (L=1.2,W=19);"
      "  device PMOS4 '6' (S=VDD,G=A0_,D=CS0,B=VDD) (L=1.2,W=10);"
      "  device PMOS4 '7' (S=VDD,G=YI,D=CS0,B=VDD) (L=1.2,W=10);"
      "  device NMOS4 '8' (S=HNET52,G=G0,D=VSS,B=VSS) (L=1.2,W=17);"
      "  device NMOS4 '9' (S=NET194,G=G1,D=HNET52,B=VSS) (L=1.2,W=17);"
      "end;"
    ;

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  std::string txt = logger.text ();

  EXPECT_EQ (txt,
    "begin_circuit DECODE DECODE\n"
    "match_nets $41 WL1_EN_\n"
    "match_nets VDD VDD\n"
    "match_nets $39 NET194\n"
    "match_nets g0 G0\n"
    "match_nets $40 HNET52\n"
    "match_nets VSS VSS\n"
    "match_nets $42 NET189\n"
    "match_nets gtp NN3\n"
    "match_nets $37 NET193\n"
    "match_nets g1 G1\n"
    "match_nets $44 YI\n"
    "match_nets $14 WELL\n"
    "match_ambiguous_nets nn2 NN2\n"
    "match_ambiguous_nets nn2_ NN2_\n"
    "match_nets q0 Q0\n"
    "match_nets q1 Q1\n"
    "match_nets $11 CS0\n"
    "match_nets $13 CS1\n"
    "match_nets q0_ Q0_\n"
    "match_nets q1_ Q1_\n"
    "match_nets a0 A0\n"
    "match_nets a0_ A0_\n"
    "match_nets $35 HNET44\n"
    "match_nets $34 HNET48\n"
    "match_nets $4 NET200\n"
    "match_nets nn1_ NN1_\n"
    "match_nets $9 NET175\n"
    "match_nets $6 NET181\n"
    "match_nets nn1 NN1\n"
    "match_nets $8 NET215\n"
    "match_pins VDD VDD\n"
    "match_pins nn1_ NN1_\n"
    "match_pins nn1 NN1\n"
    "match_pins q0 Q0\n"
    "match_pins q0_ Q0_\n"
    "match_pins q1_ Q1_\n"
    "match_pins q1 Q1\n"
    "match_pins nn2 NN2\n"
    "match_pins nn2_ NN2_\n"
    "match_pins a0 A0\n"
    "match_pins a0_ A0_\n"
    "match_pins g1 G1\n"
    "match_pins g0 G0\n"
    "match_pins gtp NN3\n"
    "match_pins VSS VSS\n"
    "match_pins WELL WELL\n"
    "match_devices $30 0\n"
    "match_devices $29 1\n"
    "match_devices $9 10\n"
    "match_devices $10 11\n"
    "match_devices $36 12\n"
    "match_devices $35 13\n"
    "match_devices $34 14\n"
    "match_devices $38 15\n"
    "match_devices $37 16\n"
    "match_devices $33 17\n"
    "match_devices $27 18\n"
    "match_devices $28 19\n"
    "match_devices $17 2\n"
    "match_devices $31 20\n"
    "match_devices $32 21\n"
    "match_devices $22 22\n"
    "match_devices $26 23\n"
    "match_devices $23 24\n"
    "match_devices $43 25\n"
    "match_devices $20 26\n"
    "match_devices $25 27\n"
    "match_devices $15 28\n"
    "match_devices $14 29\n"
    "match_devices $16 3\n"
    "match_devices $18 30\n"
    "match_devices $21 31\n"
    "match_devices $13 32\n"
    "match_devices $19 33\n"
    "match_devices $7 34\n"
    "match_devices $8 35\n"
    "match_devices $24 36\n"
    "match_devices $3 37\n"
    "match_devices $6 38\n"
    "match_devices $4 39\n"
    "match_devices $39 4\n"
    "match_devices $5 40\n"
    "match_devices $2 41\n"
    "match_devices $1 42\n"
    "match_devices $40 5\n"
    "match_devices $11 6\n"
    "match_devices $12 7\n"
    "match_devices $41 8\n"
    "match_devices $42 9\n"
    "end_circuit DECODE DECODE MATCH"
  );
  EXPECT_EQ (good, true);

  comp.set_depth_first (false);
  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit DECODE DECODE\n"
    "match_nets $41 WL1_EN_\n"
    "match_nets g0 G0\n"
    "match_nets g1 G1\n"
    "match_nets $40 HNET52\n"
    "match_nets $37 NET193\n"
    "match_nets gtp NN3\n"
    "match_nets $42 NET189\n"
    "match_nets $39 NET194\n"
    "match_nets $14 WELL\n"
    "match_nets $44 YI\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_ambiguous_nets nn2 NN2\n"
    "match_ambiguous_nets nn2_ NN2_\n"
    "match_nets q0 Q0\n"
    "match_nets q1 Q1\n"
    "match_nets $11 CS0\n"
    "match_nets $13 CS1\n"
    "match_nets q0_ Q0_\n"
    "match_nets q1_ Q1_\n"
    "match_nets a0 A0\n"
    "match_nets a0_ A0_\n"
    "match_nets $35 HNET44\n"
    "match_nets $34 HNET48\n"
    "match_nets $4 NET200\n"
    "match_nets $6 NET181\n"
    "match_nets $8 NET215\n"
    "match_nets $9 NET175\n"
    "match_nets nn1 NN1\n"
    "match_nets nn1_ NN1_\n"
    "match_pins VDD VDD\n"
    "match_pins nn1_ NN1_\n"
    "match_pins nn1 NN1\n"
    "match_pins q0 Q0\n"
    "match_pins q0_ Q0_\n"
    "match_pins q1_ Q1_\n"
    "match_pins q1 Q1\n"
    "match_pins nn2 NN2\n"
    "match_pins nn2_ NN2_\n"
    "match_pins a0 A0\n"
    "match_pins a0_ A0_\n"
    "match_pins g1 G1\n"
    "match_pins g0 G0\n"
    "match_pins gtp NN3\n"
    "match_pins VSS VSS\n"
    "match_pins WELL WELL\n"
    "match_devices $30 0\n"
    "match_devices $29 1\n"
    "match_devices $9 10\n"
    "match_devices $10 11\n"
    "match_devices $36 12\n"
    "match_devices $35 13\n"
    "match_devices $34 14\n"
    "match_devices $38 15\n"
    "match_devices $37 16\n"
    "match_devices $33 17\n"
    "match_devices $27 18\n"
    "match_devices $28 19\n"
    "match_devices $17 2\n"
    "match_devices $31 20\n"
    "match_devices $32 21\n"
    "match_devices $22 22\n"
    "match_devices $26 23\n"
    "match_devices $23 24\n"
    "match_devices $43 25\n"
    "match_devices $20 26\n"
    "match_devices $25 27\n"
    "match_devices $15 28\n"
    "match_devices $14 29\n"
    "match_devices $16 3\n"
    "match_devices $18 30\n"
    "match_devices $21 31\n"
    "match_devices $13 32\n"
    "match_devices $19 33\n"
    "match_devices $7 34\n"
    "match_devices $8 35\n"
    "match_devices $24 36\n"
    "match_devices $3 37\n"
    "match_devices $6 38\n"
    "match_devices $4 39\n"
    "match_devices $39 4\n"
    "match_devices $5 40\n"
    "match_devices $2 41\n"
    "match_devices $1 42\n"
    "match_devices $40 5\n"
    "match_devices $11 6\n"
    "match_devices $12 7\n"
    "match_devices $41 8\n"
    "match_devices $42 9\n"
    "end_circuit DECODE DECODE MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(20_BusLikeConnections)
{
  //  Test test requires a certain depth and tests the backtracking paths.

  const char *nls1 =
    "circuit INV (IN=IN,OUT=OUT,VDD=VDD,VSS=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit INV8 (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I1 (IN=IN1,OUT=OUT1,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I2 (IN=IN2,OUT=OUT2,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I3 (IN=IN3,OUT=OUT3,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I4 (IN=IN4,OUT=OUT4,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I5 (IN=IN5,OUT=OUT5,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I6 (IN=IN6,OUT=OUT6,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I7 (IN=IN7,OUT=OUT7,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I8 (IN=IN8,OUT=OUT8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit INV8_WRAP (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8 INV8 (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit TOP (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8_WRAP INV8 (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
  ;

  const char *nls2 =
    "circuit INV (OUT=OUT,IN=IN,VDD=VDD,VSS=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit INV8 (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I1 (OUT=Q1,IN=A1,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I8 (OUT=Q8,IN=A8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I3 (OUT=Q3,IN=A3,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I7 (OUT=Q7,IN=A7,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I4 (OUT=Q4,IN=A4,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I2 (OUT=Q2,IN=A2,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I6 (OUT=Q6,IN=A6,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV I5 (OUT=Q5,IN=A5,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit INV8_WRAP (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8 INV8 (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit TOP (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8_WRAP INV8 (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
  ;

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  std::string txt = logger.text ();

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
    "begin_circuit INV8 INV8\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_ambiguous_nets IN1 A1\n"
    "match_ambiguous_nets IN2 A2\n"
    "match_ambiguous_nets IN3 A3\n"
    "match_ambiguous_nets IN4 A4\n"
    "match_ambiguous_nets IN5 A5\n"
    "match_ambiguous_nets IN6 A6\n"
    "match_ambiguous_nets IN7 A7\n"
    "match_ambiguous_nets IN8 A8\n"
    "match_nets OUT1 Q1\n"
    "match_nets OUT2 Q2\n"
    "match_nets OUT3 Q3\n"
    "match_nets OUT4 Q4\n"
    "match_nets OUT5 Q5\n"
    "match_nets OUT6 Q6\n"
    "match_nets OUT7 Q7\n"
    "match_nets OUT8 Q8\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits I1 I1\n"
    "match_subcircuits I8 I8\n"
    "match_subcircuits I3 I3\n"
    "match_subcircuits I7 I7\n"
    "match_subcircuits I4 I4\n"
    "match_subcircuits I2 I2\n"
    "match_subcircuits I6 I6\n"
    "match_subcircuits I5 I5\n"
    "end_circuit INV8 INV8 MATCH\n"
    "begin_circuit INV8_WRAP INV8_WRAP\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets OUT8 Q8\n"
    "match_nets OUT7 Q7\n"
    "match_nets OUT6 Q6\n"
    "match_nets OUT5 Q5\n"
    "match_nets OUT4 Q4\n"
    "match_nets OUT3 Q3\n"
    "match_nets OUT2 Q2\n"
    "match_nets OUT1 Q1\n"
    "match_ambiguous_nets IN1 A1\n"
    "match_ambiguous_nets IN2 A2\n"
    "match_ambiguous_nets IN3 A3\n"
    "match_ambiguous_nets IN4 A4\n"
    "match_ambiguous_nets IN5 A5\n"
    "match_ambiguous_nets IN6 A6\n"
    "match_ambiguous_nets IN7 A7\n"
    "match_ambiguous_nets IN8 A8\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits INV8 INV8\n"
    "end_circuit INV8_WRAP INV8_WRAP MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets OUT8 Q8\n"
    "match_nets OUT7 Q7\n"
    "match_nets OUT6 Q6\n"
    "match_nets OUT5 Q5\n"
    "match_nets OUT4 Q4\n"
    "match_nets OUT3 Q3\n"
    "match_nets OUT2 Q2\n"
    "match_nets OUT1 Q1\n"
    "match_ambiguous_nets IN1 A1\n"
    "match_ambiguous_nets IN2 A2\n"
    "match_ambiguous_nets IN3 A3\n"
    "match_ambiguous_nets IN4 A4\n"
    "match_ambiguous_nets IN5 A5\n"
    "match_ambiguous_nets IN6 A6\n"
    "match_ambiguous_nets IN7 A7\n"
    "match_ambiguous_nets IN8 A8\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits INV8 INV8\n"
    "end_circuit TOP TOP MATCH"
  );
  EXPECT_EQ (good, true);

  logger.clear ();

  comp.set_dont_consider_net_names (false);
  good = comp.compare (&nl1, &nl2);

  txt = logger.text ();

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
    "begin_circuit INV8 INV8\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_ambiguous_nets IN1 A1\n"
    "match_ambiguous_nets IN2 A2\n"
    "match_ambiguous_nets IN3 A3\n"
    "match_ambiguous_nets IN4 A4\n"
    "match_ambiguous_nets IN5 A5\n"
    "match_ambiguous_nets IN6 A6\n"
    "match_ambiguous_nets IN7 A7\n"
    "match_ambiguous_nets IN8 A8\n"
    "match_nets OUT1 Q1\n"
    "match_nets OUT2 Q2\n"
    "match_nets OUT3 Q3\n"
    "match_nets OUT4 Q4\n"
    "match_nets OUT5 Q5\n"
    "match_nets OUT6 Q6\n"
    "match_nets OUT7 Q7\n"
    "match_nets OUT8 Q8\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits I1 I1\n"
    "match_subcircuits I8 I8\n"
    "match_subcircuits I3 I3\n"
    "match_subcircuits I7 I7\n"
    "match_subcircuits I4 I4\n"
    "match_subcircuits I2 I2\n"
    "match_subcircuits I6 I6\n"
    "match_subcircuits I5 I5\n"
    "end_circuit INV8 INV8 MATCH\n"
    "begin_circuit INV8_WRAP INV8_WRAP\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets OUT8 Q8\n"
    "match_nets OUT7 Q7\n"
    "match_nets OUT6 Q6\n"
    "match_nets OUT5 Q5\n"
    "match_nets OUT4 Q4\n"
    "match_nets OUT3 Q3\n"
    "match_nets OUT2 Q2\n"
    "match_nets OUT1 Q1\n"
    "match_ambiguous_nets IN1 A1\n"
    "match_ambiguous_nets IN2 A2\n"
    "match_ambiguous_nets IN3 A3\n"
    "match_ambiguous_nets IN4 A4\n"
    "match_ambiguous_nets IN5 A5\n"
    "match_ambiguous_nets IN6 A6\n"
    "match_ambiguous_nets IN7 A7\n"
    "match_ambiguous_nets IN8 A8\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits INV8 INV8\n"
    "end_circuit INV8_WRAP INV8_WRAP MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets OUT8 Q8\n"
    "match_nets OUT7 Q7\n"
    "match_nets OUT6 Q6\n"
    "match_nets OUT5 Q5\n"
    "match_nets OUT4 Q4\n"
    "match_nets OUT3 Q3\n"
    "match_nets OUT2 Q2\n"
    "match_nets OUT1 Q1\n"
    "match_ambiguous_nets IN1 A1\n"
    "match_ambiguous_nets IN2 A2\n"
    "match_ambiguous_nets IN3 A3\n"
    "match_ambiguous_nets IN4 A4\n"
    "match_ambiguous_nets IN5 A5\n"
    "match_ambiguous_nets IN6 A6\n"
    "match_ambiguous_nets IN7 A7\n"
    "match_ambiguous_nets IN8 A8\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits INV8 INV8\n"
    "end_circuit TOP TOP MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(21_BusLikeAmbiguousConnections)
{
  //  Test test requires a certain depth and tests the backtracking paths.

  const char *nls1 =
    "circuit INV8 (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    //  NOTE: passive nets make the pins ambiguous
    "end;\n"
    "circuit INV8_WRAP (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8 INV8 (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit TOP (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8_WRAP INV8 (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
  ;

  const char *nls2 =
    "circuit INV8 (IN1=IN1,OUT1=OUT1,IN2=IN2,OUT2=OUT2,IN3=IN3,OUT3=OUT3,IN4=IN4,OUT4=OUT4,IN5=IN5,OUT5=OUT5,IN6=IN6,OUT6=OUT6,IN7=IN7,OUT7=OUT7,IN8=IN8,OUT8=OUT8,VDD=VDD,VSS=VSS);\n"
    //  NOTE: passive nets make the pins ambiguous
    "end;\n"
    "circuit INV8_WRAP (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8 INV8 (IN1=A1,OUT1=Q1,IN2=A2,OUT2=Q2,IN3=A3,OUT3=Q3,IN4=A4,OUT4=Q4,IN5=A5,OUT5=Q5,IN6=A6,OUT6=Q6,IN7=A7,OUT7=Q7,IN8=A8,OUT8=Q8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit TOP (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV8_WRAP INV8 (Q1=Q1,A1=A1,Q2=Q2,A2=A2,Q3=Q3,A3=A3,Q4=Q4,A4=A4,Q5=Q5,A5=A5,Q6=Q6,A6=A6,Q7=Q7,A7=A7,Q8=Q8,A8=A8,VDD=VDD,VSS=VSS);\n"
    "end;\n"
  ;

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  std::string txt = logger.text ();

  EXPECT_EQ (txt,
    "begin_circuit INV8 INV8\n"
    "match_nets IN1 IN1\n"
    "match_nets IN2 IN2\n"
    "match_nets IN3 IN3\n"
    "match_nets IN4 IN4\n"
    "match_nets IN5 IN5\n"
    "match_nets IN6 IN6\n"
    "match_nets IN7 IN7\n"
    "match_nets IN8 IN8\n"
    "match_nets OUT1 OUT1\n"
    "match_nets OUT2 OUT2\n"
    "match_nets OUT3 OUT3\n"
    "match_nets OUT4 OUT4\n"
    "match_nets OUT5 OUT5\n"
    "match_nets OUT6 OUT6\n"
    "match_nets OUT7 OUT7\n"
    "match_nets OUT8 OUT8\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_pins IN1 IN1\n"
    "match_pins OUT1 OUT1\n"
    "match_pins IN2 IN2\n"
    "match_pins OUT2 OUT2\n"
    "match_pins IN3 IN3\n"
    "match_pins OUT3 OUT3\n"
    "match_pins IN4 IN4\n"
    "match_pins OUT4 OUT4\n"
    "match_pins IN5 IN5\n"
    "match_pins OUT5 OUT5\n"
    "match_pins IN6 IN6\n"
    "match_pins OUT6 OUT6\n"
    "match_pins IN7 IN7\n"
    "match_pins OUT7 OUT7\n"
    "match_pins IN8 IN8\n"
    "match_pins OUT8 OUT8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "end_circuit INV8 INV8 MATCH\n"
    "begin_circuit INV8_WRAP INV8_WRAP\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets OUT8 Q8\n"
    "match_nets IN8 A8\n"
    "match_nets OUT7 Q7\n"
    "match_nets IN7 A7\n"
    "match_nets OUT6 Q6\n"
    "match_nets IN6 A6\n"
    "match_nets OUT5 Q5\n"
    "match_nets IN5 A5\n"
    "match_nets OUT4 Q4\n"
    "match_nets IN4 A4\n"
    "match_nets OUT3 Q3\n"
    "match_nets IN3 A3\n"
    "match_nets OUT2 Q2\n"
    "match_nets IN2 A2\n"
    "match_nets OUT1 Q1\n"
    "match_nets IN1 A1\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits INV8 INV8\n"
    "end_circuit INV8_WRAP INV8_WRAP MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets IN8 A8\n"
    "match_nets OUT8 Q8\n"
    "match_nets IN7 A7\n"
    "match_nets OUT7 Q7\n"
    "match_nets IN6 A6\n"
    "match_nets OUT6 Q6\n"
    "match_nets IN5 A5\n"
    "match_nets OUT5 Q5\n"
    "match_nets IN4 A4\n"
    "match_nets OUT4 Q4\n"
    "match_nets IN3 A3\n"
    "match_nets OUT3 Q3\n"
    "match_nets IN2 A2\n"
    "match_nets OUT2 Q2\n"
    "match_nets IN1 A1\n"
    "match_nets OUT1 Q1\n"
    "match_pins IN1 A1\n"
    "match_pins OUT1 Q1\n"
    "match_pins IN2 A2\n"
    "match_pins OUT2 Q2\n"
    "match_pins IN3 A3\n"
    "match_pins OUT3 Q3\n"
    "match_pins IN4 A4\n"
    "match_pins OUT4 Q4\n"
    "match_pins IN5 A5\n"
    "match_pins OUT5 Q5\n"
    "match_pins IN6 A6\n"
    "match_pins OUT6 Q6\n"
    "match_pins IN7 A7\n"
    "match_pins OUT7 Q7\n"
    "match_pins IN8 A8\n"
    "match_pins OUT8 Q8\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits INV8 INV8\n"
    "end_circuit TOP TOP MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(22_NodesRemoved)
{
  const char *nls1 =
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  const char *nls2 =
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 ($2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC);\n"
    "  subcircuit INV2PAIR $2 ($2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I21);\n"
    "  subcircuit INV2PAIR $3 ($2=$I23,$3=VDD,$4=VSS,$5=$I21,$6=$I5);\n"
    "  subcircuit INV2PAIR $4 ($2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6);\n"
    "  subcircuit INV2PAIR $5 ($2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7);\n"
    "end;\n"
    "circuit INV2PAIR ($2=$I8,$3=$I5,$4=$I4,$5=$I3,$6=$I2);\n"
    "  subcircuit INV2 $1 (IN=$I3,$3=$I7,OUT=$I6,VSS=$I4,VDD=$I5);\n"
    "  subcircuit INV2 $2 (IN=$I6,$3=$I8,OUT=$I2,VSS=$I4,VDD=$I5);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  std::string txt = logger.text ();

  EXPECT_EQ (txt,
    "begin_circuit INV2 INV2\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets $3 $3\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets $1 (null)\n"
    "match_nets BULK (null)\n"
    "match_pins IN IN\n"
    "match_pins $2 $1\n"
    "match_pins OUT OUT\n"
    "match_pins VSS VSS\n"
    "match_pins VDD VDD\n"
    "match_pins $0 (null)\n"
    "match_pins BULK (null)\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit INV2 INV2 MATCH\n"
    "begin_circuit INV2PAIR INV2PAIR\n"
    "match_nets $I2 $I2\n"
    "match_nets $I6 $I5\n"
    "match_nets $I5 $I4\n"
    "match_nets $I4 $I6\n"
    "match_nets $I3 $I3\n"
    "match_nets $I7 $I7\n"
    "match_nets $I8 $I8\n"
    "match_nets BULK (null)\n"
    "match_nets $I1 (null)\n"
    "match_pins $1 $0\n"
    "match_pins $2 $1\n"
    "match_pins $3 $2\n"
    "match_pins $4 $3\n"
    "match_pins $5 $4\n"
    "match_pins BULK (null)\n"
    "match_pins $6 (null)\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "end_circuit INV2PAIR INV2PAIR MATCH\n"
    "begin_circuit RINGO RINGO\n"
    "match_nets OSC OSC\n"
    "match_nets $I7 $I7\n"
    "match_nets $I6 $I6\n"
    "match_nets $I5 $I5\n"
    "match_nets $I13 $I21\n"
    "match_nets FB FB\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets $I22 $I22\n"
    "match_nets $I23 $I23\n"
    "match_nets $I24 $I24\n"
    "match_nets $I25 $I25\n"
    "match_pins FB FB\n"
    "match_pins OSC OSC\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "match_subcircuits $3 $3\n"
    "match_subcircuits $4 $4\n"
    "match_subcircuits $5 $5\n"
    "end_circuit RINGO RINGO MATCH"
  );
  EXPECT_EQ (good, true);

  logger.clear ();
  good = comp.compare (&nl2, &nl1);

  txt = logger.text ();

  //  additional nodes are not ignored when they come from the reference side (second)
  EXPECT_EQ (txt,
    "begin_circuit INV2 INV2\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets $3 $3\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets (null) $1\n"
    "match_nets (null) BULK\n"
    "match_pins IN IN\n"
    "match_pins $1 $2\n"
    "match_pins OUT OUT\n"
    "match_pins VSS VSS\n"
    "match_pins VDD VDD\n"
    "match_pins (null) $0\n"
    "match_pins (null) BULK\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit INV2 INV2 MATCH\n"
    "begin_circuit INV2PAIR INV2PAIR\n"
    "match_nets $I2 $I2\n"
    "match_nets $I5 $I6\n"
    "match_nets $I4 $I5\n"
    "match_nets $I6 $I4\n"
    "match_nets $I3 $I3\n"
    "match_nets $I7 $I7\n"
    "match_nets $I8 $I8\n"
    "match_nets (null) BULK\n"
    "match_nets (null) $I1\n"
    "match_pins $0 $1\n"
    "match_pins $1 $2\n"
    "match_pins $2 $3\n"
    "match_pins $3 $4\n"
    "match_pins $4 $5\n"
    "match_pins (null) BULK\n"
    "match_pins (null) $6\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "end_circuit INV2PAIR INV2PAIR MATCH\n"
    "begin_circuit RINGO RINGO\n"
    "match_nets OSC OSC\n"
    "match_nets $I7 $I7\n"
    "match_nets $I6 $I6\n"
    "match_nets $I5 $I5\n"
    "match_nets $I21 $I13\n"
    "match_nets FB FB\n"
    "match_nets $I22 $I22\n"
    "match_nets $I23 $I23\n"
    "match_nets $I24 $I24\n"
    "match_nets $I25 $I25\n"
    "net_mismatch VDD (null)\n"
    "net_mismatch VSS (null)\n"
    "net_mismatch (null) VDD\n"
    "net_mismatch (null) VSS\n"
    "match_pins FB FB\n"
    "match_pins OSC OSC\n"
    "match_pins VDD (null)\n"
    "match_pins VSS (null)\n"
    "match_pins (null) VDD\n"
    "match_pins (null) VSS\n"
    "subcircuit_mismatch $1 $1\n"
    "subcircuit_mismatch $2 $2\n"
    "subcircuit_mismatch $3 $3\n"
    "subcircuit_mismatch $4 $4\n"
    "subcircuit_mismatch $5 $5\n"
    "end_circuit RINGO RINGO NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(23_NodesRemovedWithError)
{
  const char *nls1 =
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 ($2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC);\n"
    "  subcircuit INV2PAIR $2 ($2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I21);\n"
    "  subcircuit INV2PAIR $3 ($2=$I23,$3=VDD,$4=VSS,$5=$I21,$6=$I5);\n"
    "  subcircuit INV2PAIR $4 ($2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6);\n"
    "  subcircuit INV2PAIR $5 ($2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7);\n"
    "end;\n"
    "circuit INV2PAIR ($2=$I8,$3=$I5,$4=$I4,$5=$I3,$6=$I2);\n"
    //  NOTE: $1 pin should not be connected to different nets, although its not functional
    "  subcircuit INV2 $1 ($1=$3,IN=$I3,$3=$I7,OUT=$I6,VSS=$I4,VDD=$I5);\n"
    "  subcircuit INV2 $2 ($1=$6,IN=$I6,$3=$I8,OUT=$I2,VSS=$I4,VDD=$I5);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  const char *nls2 =
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  std::string txt = logger.text ();

  EXPECT_EQ (txt,
    "begin_circuit INV2 INV2\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets $3 $3\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_ambiguous_nets $1 $1\n"
    "match_nets (null) BULK\n"
    "match_pins $0 $0\n"
    "match_pins IN IN\n"
    "match_pins $2 $2\n"
    "match_pins OUT OUT\n"
    "match_pins VSS VSS\n"
    "match_pins VDD VDD\n"
    "match_pins (null) BULK\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit INV2 INV2 MATCH\n"
    "begin_circuit INV2PAIR INV2PAIR\n"
    "match_nets $I2 $I2\n"
    "match_nets $I5 $I6\n"
    "match_nets $I4 $I5\n"
    "match_nets $I6 $I4\n"
    "match_nets $I3 $I3\n"
    "match_nets $I7 $I7\n"
    "net_mismatch $3 $I1\n"
    "match_nets $I8 $I8\n"
    "net_mismatch $6 BULK\n"
    "match_pins $0 $1\n"
    "match_pins $1 $2\n"
    "match_pins $2 $3\n"
    "match_pins $3 $4\n"
    "match_pins $4 $5\n"
    "pin_mismatch (null) BULK\n"
    "pin_mismatch (null) $6\n"
    "match_subcircuits $1 $1\n"
    "subcircuit_mismatch $2 $2\n"
    "end_circuit INV2PAIR INV2PAIR NOMATCH\n"
    "circuit_skipped RINGO RINGO"
  );
  EXPECT_EQ (good, false);
}

TEST(24_NodesRemovedButConnectedInOther)
{
  const char *nls1 =
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    //  rewired here: BULK->VSS (pin $4), $1->$I3/$I4 (both rewired pins are deleted in the first netlist)
    //  This proves that we can basically do everything with the dropped pins.
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I3,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=$I5);\n"
    "  subcircuit INV2 $2 ($1=$I4,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=$I5);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  const char *nls2 =
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 ($2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC);\n"
    "  subcircuit INV2PAIR $2 ($2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I21);\n"
    "  subcircuit INV2PAIR $3 ($2=$I23,$3=VDD,$4=VSS,$5=$I21,$6=$I5);\n"
    "  subcircuit INV2PAIR $4 ($2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6);\n"
    "  subcircuit INV2PAIR $5 ($2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7);\n"
    "end;\n"
    "circuit INV2PAIR ($2=$I8,$3=$I5,$4=$I4,$5=$I3,$6=$I2);\n"
    "  subcircuit INV2 $1 (IN=$I3,$3=$I7,OUT=$I6,VSS=$I4,VDD=$I5);\n"
    "  subcircuit INV2 $2 (IN=$I6,$3=$I8,OUT=$I2,VSS=$I4,VDD=$I5);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  bool good = comp.compare (&nl1, &nl2);

  std::string txt = logger.text ();

  EXPECT_EQ (txt,
    "begin_circuit INV2 INV2\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets $3 $3\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets $1 (null)\n"
    "match_nets BULK (null)\n"
    "match_pins IN IN\n"
    "match_pins $2 $1\n"
    "match_pins OUT OUT\n"
    "match_pins VSS VSS\n"
    "match_pins VDD VDD\n"
    "match_pins $0 (null)\n"
    "match_pins BULK (null)\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit INV2 INV2 MATCH\n"
    "begin_circuit INV2PAIR INV2PAIR\n"
    "match_nets $I2 $I2\n"
    "match_nets $I6 $I5\n"
    "match_nets $I5 $I4\n"
    "match_nets $I4 $I6\n"
    "match_nets $I3 $I3\n"
    "match_nets $I7 $I7\n"
    "match_nets $I8 $I8\n"
    "match_nets BULK (null)\n"
    "match_nets $I1 (null)\n"
    "match_pins $1 $0\n"
    "match_pins $2 $1\n"
    "match_pins $3 $2\n"
    "match_pins $4 $3\n"
    "match_pins $5 $4\n"
    "match_pins BULK (null)\n"
    "match_pins $6 (null)\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "end_circuit INV2PAIR INV2PAIR MATCH\n"
    "begin_circuit RINGO RINGO\n"
    "match_nets OSC OSC\n"
    "match_nets $I7 $I7\n"
    "match_nets $I6 $I6\n"
    "match_nets $I5 $I5\n"
    "match_nets $I13 $I21\n"
    "match_nets FB FB\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets $I22 $I22\n"
    "match_nets $I23 $I23\n"
    "match_nets $I24 $I24\n"
    "match_nets $I25 $I25\n"
    "match_pins FB FB\n"
    "match_pins OSC OSC\n"
    "match_pins VDD VDD\n"
    "match_pins VSS VSS\n"
    "match_subcircuits $1 $1\n"
    "match_subcircuits $2 $2\n"
    "match_subcircuits $3 $3\n"
    "match_subcircuits $4 $4\n"
    "match_subcircuits $5 $5\n"
    "end_circuit RINGO RINGO MATCH"
  );
  EXPECT_EQ (good, true);


  logger.clear ();
  good = comp.compare (&nl2, &nl1);

  txt = logger.text ();

  //  NOTE: additional nets are ignored in the first netlist but not from the second
  EXPECT_EQ (txt,
    "begin_circuit INV2 INV2\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets $3 $3\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets (null) $1\n"
    "match_nets (null) BULK\n"
    "match_pins IN IN\n"
    "match_pins $1 $2\n"
    "match_pins OUT OUT\n"
    "match_pins VSS VSS\n"
    "match_pins VDD VDD\n"
    "match_pins (null) $0\n"
    "match_pins (null) BULK\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit INV2 INV2 MATCH\n"
    "begin_circuit INV2PAIR INV2PAIR\n"
    "match_nets $I2 $I2\n"
    "match_nets $I5 $I6\n"
    "match_nets $I8 $I8\n"
    "match_nets $I7 $I7\n"
    "net_mismatch $I6 $I4\n"
    "net_mismatch $I4 (null)\n"
    "net_mismatch $I3 (null)\n"
    "net_mismatch (null) BULK\n"
    "net_mismatch (null) $I5\n"
    "net_mismatch (null) $I3\n"
    "net_mismatch (null) $I1\n"
    "match_pins $0 $1\n"
    "match_pins $1 $2\n"
    "match_pins $4 $5\n"
    "pin_mismatch $2 (null)\n"
    "pin_mismatch $3 (null)\n"
    "pin_mismatch (null) BULK\n"
    "pin_mismatch (null) $3\n"
    "pin_mismatch (null) $4\n"
    "pin_mismatch (null) $6\n"
    "subcircuit_mismatch $1 $1\n"
    "subcircuit_mismatch $2 $2\n"
    "end_circuit INV2PAIR INV2PAIR NOMATCH\n"
    "circuit_skipped RINGO RINGO"
  );
  EXPECT_EQ (good, false);
}

TEST(25_JoinSymmetricNets)
{
  const char *nls =
    "circuit NAND2 (A=A,B=B,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1);\n"
    //  NOTE: $1 and $2 are separate nets, but can be joined due to symmetry
    "  device NMOS $3 (S=$1,G=A,D=OUT) (L=0.25,W=1);\n"
    "  device NMOS $4 (S=$2,G=A,D=OUT) (L=0.25,W=1);\n"
    "  device NMOS $5 (S=$1,G=B,D=VSS) (L=0.25,W=1);\n"
    "  device NMOS $6 (S=$2,G=B,D=VSS) (L=0.25,W=1);\n"
    "end;\n";

  db::Netlist nl;
  prep_nl (nl, nls);

  db::NetlistComparer comp;
  comp.join_symmetric_nets (nl.circuit_by_name ("NAND2"));
  comp.set_dont_consider_net_names (true);

  //  NOTE $1 and $2 are joined because they are symmetric
  EXPECT_EQ (nl.to_string (),
    "circuit NAND2 (A=A,B=B,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=$1,G=A,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$1,G=A,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $5 (S=$1,G=B,D=VSS) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $6 (S=$1,G=B,D=VSS) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  )
}

TEST(25b_JoinSymmetricNetsMultiple)
{
  const char *nls =
    "circuit NAND3 (A=A,B=B,C=C,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1);\n"
    "  device PMOS $3 (S=VDD,G=C,D=OUT) (L=0.25,W=1);\n"
    //  NOTE: $1 and $2 are separate nets, but can be joined due to symmetry
    "  device NMOS $4 (S=$1,G=A,D=OUT) (L=0.25,W=1);\n"
    "  device NMOS $5 (S=$2,G=A,D=OUT) (L=0.25,W=1);\n"
    "  device NMOS $6 (S=$3,G=A,D=OUT) (L=0.25,W=1);\n"
    "  device NMOS $7 (S=$1,G=B,D=$4) (L=0.25,W=1);\n"
    "  device NMOS $8 (S=$2,G=B,D=$5) (L=0.25,W=1);\n"
    "  device NMOS $9 (S=$3,G=B,D=$6) (L=0.25,W=1);\n"
    "  device NMOS $10 (S=$4,G=C,D=VSS) (L=0.25,W=1);\n"
    "  device NMOS $11 (S=$5,G=C,D=VSS) (L=0.25,W=1);\n"
    "  device NMOS $12 (S=$6,G=C,D=VSS) (L=0.25,W=1);\n"
    "end;\n";

  db::Netlist nl;
  prep_nl (nl, nls);

  db::NetlistComparer comp;
  comp.join_symmetric_nets (nl.circuit_by_name ("NAND3"));
  comp.set_dont_consider_net_names (true);

  nl.combine_devices ();

  //  NOTE $1 and $2 are joined because they are symmetric
  EXPECT_EQ (nl.to_string (),
    "circuit NAND3 (A=A,B=B,C=C,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $3 (S=VDD,G=C,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$1,G=A,D=OUT) (L=0.25,W=3,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $7 (S=$1,G=B,D=$4) (L=0.25,W=3,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $10 (S=$4,G=C,D=VSS) (L=0.25,W=3,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  )
}

TEST(25c_JoinSymmetricNetsMultipleMessedUp)
{
  const char *nls =
    "circuit NOR3 (A=A,C=C,B=B,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $9 (S=$6,G=B,D=$3) (L=0.27,W=1.1);\n"
    "  device NMOS $1 (S=OUT,G=A,D=VSS) (L=0.23,W=2.05);\n"
    "  device PMOS $5 (S=$2,G=A,D=OUT) (L=0.27,W=1.1);\n"
    "  device NMOS $2 (S=OUT,G=B,D=VSS) (L=0.23,W=2.05);\n"
    "  device PMOS $8 (S=$2,G=B,D=$5) (L=0.27,W=1.1);\n"
    "  device PMOS $4 (S=$21,G=A,D=OUT) (L=0.27,W=1.1);\n"
    "  device PMOS $7 (S=$21,G=B,D=$4) (L=0.27,W=1.1);\n"
    "  device PMOS $12 (S=VDD,G=C,D=$6) (L=0.27,W=1.1);\n"
    "  device PMOS $10 (S=$4,G=C,D=VDD) (L=0.27,W=1.1);\n"
    "  device PMOS $6 (S=$3,G=A,D=OUT) (L=0.27,W=1.1);\n"
    "  device PMOS $11 (S=VDD,G=C,D=$5) (L=0.27,W=1.1);\n"
    "  device NMOS $3 (S=VSS,G=C,D=OUT) (L=0.23,W=2.05);\n"
    "end;\n";

  db::Netlist nl;
  prep_nl (nl, nls);

  db::NetlistComparer comp;
  comp.join_symmetric_nets (nl.circuit_by_name ("NOR3"));
  comp.set_dont_consider_net_names (true);

  nl.combine_devices ();

  //  NOTE $1 and $2 are joined because they are symmetric
  EXPECT_EQ (nl.to_string (),
    "circuit NOR3 (A=A,C=C,B=B,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=$6,G=B,D=$3) (L=0.27,W=3.3,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $2 (S=OUT,G=A,D=VSS) (L=0.23,W=2.05,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $3 (S=$3,G=A,D=OUT) (L=0.27,W=3.3,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=OUT,G=B,D=VSS) (L=0.23,W=2.05,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $8 (S=VDD,G=C,D=$6) (L=0.27,W=3.3,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $12 (S=VSS,G=C,D=OUT) (L=0.23,W=2.05,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  )
}

TEST(26_JoinSymmetricNets)
{
  const char *nls =
    "circuit RESCUBE (A=A,B=B);\n"
    "  device RES $1 (A=A,B=$1) (R=1000);\n"
    "  device RES $2 (A=A,B=$2) (R=1000);\n"
    "  device RES $3 (A=A,B=$3) (R=1000);\n"
    "  device RES $4 (A=$1,B=$4) (R=1000);\n"
    "  device RES $5 (A=$2,B=$4) (R=1000);\n"
    "  device RES $6 (A=$2,B=$5) (R=1000);\n"
    "  device RES $7 (A=$3,B=$5) (R=1000);\n"
    "  device RES $8 (A=$3,B=$6) (R=1000);\n"
    "  device RES $9 (A=$1,B=$6) (R=1000);\n"
    "  device RES $9 (A=$4,B=B) (R=1000);\n"
    "  device RES $10 (A=$5,B=B) (R=1000);\n"
    "  device RES $11 (A=$6,B=B) (R=1000);\n"
    "end;\n";

  db::Netlist nl;
  prep_nl (nl, nls);

  db::NetlistComparer comp;
  comp.join_symmetric_nets (nl.circuit_by_name ("RESCUBE"));
  comp.set_dont_consider_net_names (true);

  EXPECT_EQ (nl.to_string (),
    "circuit RESCUBE (A=A,B=B);\n"
    "  device RES $1 (A=A,B=$1) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $2 (A=A,B=$1) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $3 (A=A,B=$1) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $4 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $5 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $6 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $7 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $8 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $9 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $10 (A=$4,B=B) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $11 (A=$4,B=B) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  device RES $12 (A=$4,B=B) (R=1000,L=0,W=0,A=0,P=0);\n"
    "end;\n"
  )

  nl.combine_devices ();
  EXPECT_EQ (nl.to_string (),
    "circuit RESCUBE (A=A,B=B);\n"
    "  device RES $10 (A=A,B=B) (R=833.333333333,L=0,W=0,A=0,P=0);\n"
    "end;\n"
  )
}

TEST(27_DontJoinSymmetricNetsWithPins)
{
  const char *nls =
    "circuit NAND2 (A=A,B=B,OUT=OUT,VSS=VSS,VDD=VDD,X=$1,Y=$2);\n"
    "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1);\n"
    //  NOTE: $1 and $2 are separate nets, but can be joined due to symmetry
    "  device NMOS $3 (S=$1,G=A,D=OUT) (L=0.25,W=1);\n"
    "  device NMOS $4 (S=$2,G=A,D=OUT) (L=0.25,W=1);\n"
    "  device NMOS $5 (S=$1,G=B,D=VSS) (L=0.25,W=1);\n"
    "  device NMOS $6 (S=$2,G=B,D=VSS) (L=0.25,W=1);\n"
    "end;\n";

  db::Netlist nl;
  prep_nl (nl, nls);

  db::NetlistComparer comp;
  comp.join_symmetric_nets (nl.circuit_by_name ("NAND2"));
  comp.set_dont_consider_net_names (true);

  //  NOTE $1 and $2 are NOT joined because they have pins
  EXPECT_EQ (nl.to_string (),
    "circuit NAND2 (A=A,B=B,OUT=OUT,VSS=VSS,VDD=VDD,X=$1,Y=$2);\n"
    "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=$1,G=A,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$2,G=A,D=OUT) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $5 (S=$1,G=B,D=VSS) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $6 (S=$2,G=B,D=VSS) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  )
}

TEST(28_NoSymmetryDetectionCases)
{
  {
    const char *nls =
      "circuit NAND2 (A=A,B=B,OUT=OUT,VSS=VSS,VDD=VDD);\n"
      "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1);\n"
      "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1);\n"
      //  NOTE: $1 and $2 are separate nets, but cannot be joined because of different W
      "  device NMOS $3 (S=$1,G=A,D=OUT) (L=0.25,W=1);\n"
      "  device NMOS $4 (S=$2,G=A,D=OUT) (L=0.25,W=1.1);\n"
      "  device NMOS $5 (S=$1,G=B,D=VSS) (L=0.25,W=1);\n"
      "  device NMOS $6 (S=$2,G=B,D=VSS) (L=0.25,W=1);\n"
      "end;\n";

    db::Netlist nl;
    prep_nl (nl, nls);

    db::NetlistComparer comp;
    comp.set_dont_consider_net_names (true);

    std::string sref = nl.to_string ();
    comp.join_symmetric_nets (nl.circuit_by_name ("NAND2"));

    EXPECT_EQ (nl.to_string (), sref);
  }

  {
    const char *nls =
      "circuit NAND2 (A=A,B=B,OUT=OUT,VSS=VSS,VSS2=VSS2,VDD=VDD);\n"
      "  device PMOS $1 (S=OUT,G=A,D=VDD) (L=0.25,W=1);\n"
      "  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=1);\n"
      "  device NMOS $3 (S=$1,G=A,D=OUT) (L=0.25,W=1);\n"
      "  device NMOS $4 (S=$2,G=A,D=OUT) (L=0.25,W=1);\n"
      //  NOTE: $1 and $2 are separate nets, but cannot be joined because of different D connections
      "  device NMOS $5 (S=$1,G=B,D=VSS) (L=0.25,W=1);\n"
      "  device NMOS $6 (S=$2,G=B,D=VSS2) (L=0.25,W=1);\n"
      "end;\n";

    db::Netlist nl;
    prep_nl (nl, nls);

    db::NetlistComparer comp;
    comp.set_dont_consider_net_names (true);

    std::string sref = nl.to_string ();
    comp.join_symmetric_nets (nl.circuit_by_name ("NAND2"));

    EXPECT_EQ (nl.to_string (), sref);
  }
}

TEST(28_JoinSymmetricNets)
{
  const char *nls =
    "circuit INV2LOAD (A=A,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=OUT1,G=A,D=VDD) (L=0.25,W=1);\n"
    "  device PMOS $2 (S=VDD,G=A,D=OUT1) (L=0.25,W=1);\n"
    "  device NMOS $3 (S=VSS,G=A,D=OUT1) (L=0.25,W=1);\n"
    "  device NMOS $4 (S=VSS,G=A,D=OUT1) (L=0.25,W=1.5);\n"
    "  device PMOS $5 (S=OUT2,G=A,D=VDD) (L=0.25,W=1);\n"
    "  device PMOS $6 (S=VDD,G=A,D=OUT2) (L=0.25,W=1);\n"
    "  device NMOS $7 (S=OUT2,G=A,D=VSS) (L=0.25,W=1);\n"
    "  device NMOS $8 (S=OUT2,G=A,D=VSS) (L=0.25,W=1);\n"
    "  device PMOS $9 (S=OUT3,G=A,D=VDD) (L=0.25,W=1);\n"
    "  device PMOS $10 (S=VDD,G=A,D=OUT3) (L=0.25,W=1);\n"
    "  device NMOS $11 (S=VSS,G=A,D=OUT3) (L=0.25,W=1);\n"
    "  device NMOS $12 (S=OUT3,G=A,D=VSS) (L=0.25,W=1);\n"
    "end;\n";

  db::Netlist nl;
  prep_nl (nl, nls);

  db::NetlistComparer comp;
  comp.set_dont_consider_net_names (true);
  comp.join_symmetric_nets (nl.circuit_by_name ("INV2LOAD"));

  //  NOTE $1 and $2 are joined because they are symmetric
  EXPECT_EQ (nl.to_string (),
    "circuit INV2LOAD (A=A,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=OUT1,G=A,D=VDD) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=VDD,G=A,D=OUT1) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=VSS,G=A,D=OUT1) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=VSS,G=A,D=OUT1) (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $5 (S='OUT2,OUT3',G=A,D=VDD) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $6 (S=VDD,G=A,D='OUT2,OUT3') (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $7 (S='OUT2,OUT3',G=A,D=VSS) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $8 (S='OUT2,OUT3',G=A,D=VSS) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $9 (S='OUT2,OUT3',G=A,D=VDD) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $10 (S=VDD,G=A,D='OUT2,OUT3') (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $11 (S=VSS,G=A,D='OUT2,OUT3') (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $12 (S='OUT2,OUT3',G=A,D=VSS) (L=0.25,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  )
}

TEST(29_EmptySubCircuitsFromSPICE)
{
  db::Netlist a, b, c;

  tl::InputStream fa (tl::testdata () + "/algo/nl_compare_29_a.cir");
  tl::InputStream fb (tl::testdata () + "/algo/nl_compare_29_b.cir");
  tl::InputStream fc (tl::testdata () + "/algo/nl_compare_29_c.cir");

  db::NetlistSpiceReader reader;
  reader.read (fa, a);
  reader.read (fb, b);
  reader.read (fc, c);

  db::NetlistComparer comp;
  EXPECT_EQ (comp.compare (&a, &b), true);
  EXPECT_EQ (comp.compare (&a, &c), false);
}

TEST(30_ComparePrimaryAndOtherParameters)
{
  db::Netlist nl1, nl2;
  db::DeviceClass *dc1, *dc2;
  db::Circuit *circuit;
  db::Device *d;
  db::Net *n;
  std::string txt;
  bool good;

  dc1 = new db::DeviceClassResistor ();
  dc1->set_name ("RES");
  nl1.add_device_class (dc1);
  circuit = new db::Circuit ();
  circuit->set_name ("X");
  d = new db::Device ();
  d->set_device_class (dc1);
  d->set_name ("D");
  d->set_parameter_value (db::DeviceClassResistor::param_id_L, 1.0);
  d->set_parameter_value (db::DeviceClassResistor::param_id_R, 10.0);
  d->set_parameter_value (db::DeviceClassResistor::param_id_W, 0.25);
  circuit->add_device (d);
  n = new db::Net ();
  circuit->add_net (n);
  n->set_name ("1");
  d->connect_terminal (db::DeviceClassResistor::terminal_id_A, n);
  n->add_pin (db::NetPinRef (circuit->add_pin ("1").id ()));
  n = new db::Net ();
  circuit->add_net (n);
  n->set_name ("2");
  d->connect_terminal (db::DeviceClassResistor::terminal_id_B, n);
  n->add_pin (db::NetPinRef (circuit->add_pin ("2").id ()));
  nl1.add_circuit (circuit);

  dc2 = new db::DeviceClassResistor ();
  dc2->set_name ("RES");
  nl2.add_device_class (dc2);
  circuit = new db::Circuit ();
  circuit->set_name ("X");
  d = new db::Device ();
  d->set_device_class (dc2);
  d->set_name ("D");
  d->set_parameter_value (db::DeviceClassResistor::param_id_L, 1.1);   //  differs
  d->set_parameter_value (db::DeviceClassResistor::param_id_R, 10.0);
  d->set_parameter_value (db::DeviceClassResistor::param_id_W, 0.20);  //  differs
  circuit->add_device (d);
  n = new db::Net ();
  circuit->add_net (n);
  n->set_name ("1");
  d->connect_terminal (db::DeviceClassResistor::terminal_id_A, n);
  n->add_pin (db::NetPinRef (circuit->add_pin ("1").id ()));
  n = new db::Net ();
  circuit->add_net (n);
  n->set_name ("2");
  d->connect_terminal (db::DeviceClassResistor::terminal_id_B, n);
  n->add_pin (db::NetPinRef (circuit->add_pin ("2").id ()));
  nl2.add_circuit (circuit);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  good = comp.compare (&nl1, &nl2);

  txt = logger.text ();

  //  we did not enable L and W, hence that works
  EXPECT_EQ (good, true);

  EXPECT_EQ (txt,
    "begin_circuit X X\n"
    "match_ambiguous_nets 1 1\n"
    "match_ambiguous_nets 2 2\n"
    "match_pins 1 1\n"
    "match_pins 2 2\n"
    "match_devices D D\n"
    "end_circuit X X MATCH"
  );

  //  changing R will make the compare fail

  d->set_parameter_value (db::DeviceClassResistor::param_id_R, 12.0);

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, false);

  //  if we install a delegate which introduces a tolerance (absolute 1) it will still not match

  dc1->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassResistor::param_id_R, 1.0, 0.0));

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, false);

  //  if we install a delegate which introduces a tolerance (absolute 2.0) it will match

  dc1->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassResistor::param_id_R, 2.0, 0.0));

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, true);

  //  removing the comparer will make it non-matching

  dc1->set_parameter_compare_delegate (0);

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, false);

  //  disabling the parameter will make it match too

  dc1->parameter_definition_non_const (db::DeviceClassResistor::param_id_R)->set_is_primary (false);

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, true);

  //  enabling the parameter again will make it mismatch again

  dc1->parameter_definition_non_const (db::DeviceClassResistor::param_id_R)->set_is_primary (true);

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, false);

  //  we can install an ignore handler to make it match again

  dc1->set_parameter_compare_delegate (new db::EqualDeviceParameters (db::DeviceClassResistor::param_id_R, true));

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, true);

  //  if we enable the L parameter we'll get a mismatch again

  dc1->parameter_definition_non_const (db::DeviceClassResistor::param_id_L)->set_is_primary (true);

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, false);

  //  until we install another tolerance

  *dynamic_cast<db::EqualDeviceParameters *> (dc1->parameter_compare_delegate ()) += db::EqualDeviceParameters (db::DeviceClassResistor::param_id_L, 0.11, 0.0);

  logger.clear ();
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (good, true);
}

TEST(31_ParallelMOSFets)
{
  db::Netlist a, b, c;

  tl::InputStream fa (tl::testdata () + "/algo/nl_compare_31_a.cir");
  tl::InputStream fb (tl::testdata () + "/algo/nl_compare_31_b.cir");
  tl::InputStream fc (tl::testdata () + "/algo/nl_compare_31_c.cir");

  db::NetlistSpiceReader reader;
  reader.read (fa, a);
  reader.read (fb, b);
  reader.read (fc, c);

  NetlistCompareTestLogger logger;
  std::string txt;
  bool good;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  good = comp.compare (&a, &b);

  txt = logger.text ();

  EXPECT_EQ (good, true);

  EXPECT_EQ (txt,
    "begin_circuit LVS_TEST LVS_TEST\n"
    "match_nets 2 2\n"
    "match_nets 1 1\n"
    "match_pins 1 1\n"
    "match_pins 2 2\n"
    "match_devices 1I39A $1\n"
    "match_devices 1I38 $150\n"
    "end_circuit LVS_TEST LVS_TEST MATCH"
  );

  logger.clear ();

  good = comp.compare (&a, &b);

  txt = logger.text ();

  EXPECT_EQ (good, true);

  EXPECT_EQ (txt,
    "begin_circuit LVS_TEST LVS_TEST\n"
    "match_nets 2 2\n"
    "match_nets 1 1\n"
    "match_pins 1 1\n"
    "match_pins 2 2\n"
    "match_devices 1I39A $1\n"
    "match_devices 1I38 $150\n"
    "end_circuit LVS_TEST LVS_TEST MATCH"
  );
}

TEST(32_InverterChain)
{
  db::Netlist a, b, c;

  tl::InputStream fa (tl::testdata () + "/algo/nl_compare_32a.cir");
  tl::InputStream fb (tl::testdata () + "/algo/nl_compare_32b.cir");

  db::NetlistSpiceReader reader;
  reader.read (fa, a);
  reader.read (fb, b);

  NetlistCompareTestLogger logger;
  std::string txt;
  bool good;
  db::NetlistComparer comp (&logger);
  comp.set_dont_consider_net_names (true);

  good = comp.compare (&a, &b);

  txt = logger.text ();

  EXPECT_EQ (good, true);

  EXPECT_EQ (txt,

    "begin_circuit INV_CHAIN INV_CHAIN\n"
    "match_nets 20 2\n"
    "match_nets 21 1\n"
    "match_ambiguous_nets 10 11\n"
    "match_nets 11 12\n"
    "match_nets 12 13\n"
    "match_ambiguous_nets 16 5\n"
    "match_nets 17 6\n"
    "match_nets 18 7\n"
    "match_nets 4 18\n"
    "match_nets 5 19\n"
    "match_nets 6 20\n"
    "match_nets 15 4\n"
    "match_nets 2 16\n"
    "match_nets 3 17\n"
    "match_nets 9 10\n"
    "match_nets 8 9\n"
    "match_nets 13 14\n"
    "match_nets 14 3\n"
    "match_nets 19 8\n"
    "match_nets 1 15\n"
    "match_nets 7 21\n"
    "match_pins 20 2\n"
    "match_pins 21 1\n"
    "match_devices $12 2_1.N1\n"
    "match_devices $28 2_1.P1\n"
    "match_devices $13 2_2.N1\n"
    "match_devices $29 2_2.P1\n"
    "match_devices $14 2_3.N1\n"
    "match_devices $30 2_3.P1\n"
    "match_devices $15 2_4.N1\n"
    "match_devices $31 2_4.P1\n"
    "match_devices $16 2_5.N1\n"
    "match_devices $32 2_5.P1\n"
    "match_devices $7 _1.N1\n"
    "match_devices $23 _1.P1\n"
    "match_devices $8 _2.N1\n"
    "match_devices $24 _2.P1\n"
    "match_devices $9 _3.N1\n"
    "match_devices $25 _3.P1\n"
    "match_devices $10 _4.N1\n"
    "match_devices $26 _4.P1\n"
    "match_devices $11 _5.N1\n"
    "match_devices $27 _5.P1\n"
    "match_devices $1 0_1.N1\n"
    "match_devices $17 0_1.P1\n"
    "match_devices $2 0_2.N1\n"
    "match_devices $18 0_2.P1\n"
    "match_devices $3 0_3.N1\n"
    "match_devices $19 0_3.P1\n"
    "match_devices $4 0_4.N1\n"
    "match_devices $20 0_4.P1\n"
    "match_devices $5 0_5.N1\n"
    "match_devices $21 0_5.P1\n"
    "match_devices $6 0_6.N1\n"
    "match_devices $22 0_6.P1\n"
    "end_circuit INV_CHAIN INV_CHAIN MATCH"
  );
}

