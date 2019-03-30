
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
     "match_nets VSS VSS\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_pins $1 $3\n"
     "match_pins $0 $1\n"
     "match_devices $2 $1\n"
     "match_devices $1 $2\n"
     "end_circuit INV INV MATCH"
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
  good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets VDD VDD\n"
     "match_nets VSS VSS\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_pins $1 $3\n"
     "match_pins $0 $1\n"
     "match_devices $2 $1\n"
     "match_devices $1 $2\n"
     "end_circuit INV INV MATCH"
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
     "match_pins $2 $0\n"
     "match_pins $3 $2\n"
     "match_pins $1 $3\n"
     "match_pins $0 $1\n"
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
     "match_nets OUT OUT\n"
     "match_nets VDD VDD\n"
     "match_nets IN IN\n"
     "match_nets VSS VSS\n"
     "match_nets INT $10\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $0 $1\n"
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
     "match_nets OUT OUT\n"
     "match_nets VDD VDD\n"
     "match_nets IN IN\n"
     "match_nets VSS VSS\n"
     "match_ambiguous_nets INT $10\n"
     "match_ambiguous_nets INT2 $11\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $0 $1\n"
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
    "match_nets IN IN\n"
    "match_ambiguous_nets INT $10\n"
    "match_nets INT2 $11\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $0 $1\n"
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
    "match_nets IN IN\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets VSS VSS\n"
    "match_nets INT2 $10\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $0 $1\n"
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
    "match_nets P1 P1\n"
    "match_nets P3 P3\n"
    "match_nets P2 P2\n"
    "match_pins $1 $1\n"
    "match_pins $0 $0\n"
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
    "match_nets P1 P1\n"
    "match_nets P3 P3\n"
    "match_pins $1 $1\n"
    "match_pins $0 $0\n"
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
    "match_nets P2 P2\n"
    "match_nets P1 P1\n"
    "match_pins $1 $1\n"
    "match_pins $0 $0\n"
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
    "match_pins $2 $2\n"
    "match_pins $1 $1\n"
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
    "match_ambiguous_nets P1 P3\n"
    "match_nets P2 P1\n"
    "match_nets P3 P2\n"
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
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_pins $1 $3\n"
    "match_pins $0 $1\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets INT INT\n"
    "match_pins $1 $0\n"
    "match_pins $0 $2\n"
    "match_pins $3 $3\n"
    "match_pins $2 $1\n"
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
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_pins $1 $3\n"
    "match_pins $0 $1\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INVB MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets INT INT\n"
    "match_pins $1 $0\n"
    "match_pins $0 $2\n"
    "match_pins $3 $3\n"
    "match_pins $2 $1\n"
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
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "net_mismatch VDD (null)\n"
    "net_mismatch (null) VDD\n"
    "pin_mismatch $2 (null)\n"
    "match_pins $3 $2\n"
    "match_pins $1 $3\n"
    "match_pins $0 $1\n"
    "pin_mismatch (null) $0\n"
    "device_mismatch $1 (null)\n"
    "match_devices $2 $1\n"
    "device_mismatch (null) $2\n"
    "end_circuit INV INV NOMATCH\n"
    "circuit_skipped TOP TOP"
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
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_pins $1 $3\n"
    "match_pins $0 $1\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets IN IN\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "net_mismatch OUT (null)\n"
    "net_mismatch (null) OUT\n"
    "pin_mismatch $1 (null)\n"
    "match_pins $0 $1\n"
    "match_pins $3 $3\n"
    "match_pins $2 $2\n"
    "pin_mismatch (null) $0\n"
    "subcircuit_mismatch $2 (null)\n"
    "subcircuit_mismatch $3 (null)\n"
    "subcircuit_mismatch (null) $1\n"
    "match_subcircuits $1 $2\n"
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
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_pins $2 $0\n"
    "match_pins $3 $2\n"
    "match_pins $1 $3\n"
    "match_pins $0 $1\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets INT INT\n"
    "match_pins $1 $0\n"
    "match_pins $0 $1\n"
    "match_pins $3 $3\n"
    "match_pins $2 $2\n"
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
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets OUT OUT\n"
    "match_nets A A\n"
    "match_nets INT INT\n"
    "match_pins $4 $4\n"
    "match_pins $3 $3\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $0 $0\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets IN2 IN2\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets IN1 IN1\n"
    "match_nets INT INT\n"
    "match_pins $2 $2\n"
    "match_pins $1 $1\n"
    "match_pins $4 $4\n"
    "match_pins $3 $3\n"
    "match_pins $0 $0\n"
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
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets OUT OUT\n"
    "match_nets A A\n"
    "match_nets INT INT\n"
    "match_pins $4 $4\n"
    "match_pins $3 $3\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $0 $0\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets INT IN1\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets IN1 IN2\n"
    "net_mismatch IN2 (null)\n"
    "net_mismatch (null) INT\n"
    "match_pins $2 $2\n"
    "pin_mismatch $1 (null)\n"
    "match_pins $4 $4\n"
    "match_pins $3 $3\n"
    "match_pins $0 $1\n"
    "pin_mismatch (null) $0\n"
    "subcircuit_mismatch $1 (null)\n"
    "subcircuit_mismatch (null) $1\n"
    "subcircuit_mismatch (null) $2\n"
    "subcircuit_mismatch $2 (null)\n"
    "end_circuit TOP TOP NOMATCH"
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
    "match_nets VDD VDD\n"
    "match_nets B B\n"
    "match_nets OUT OUT\n"
    "match_nets A A\n"
    "match_nets INT INT\n"
    "match_pins $4 $4\n"
    "match_pins $3 $3\n"
    "match_pins $1 $1\n"
    "match_pins $2 $2\n"
    "match_pins $0 $0\n"
    "match_devices $1 $1\n"
    "match_devices $2 $2\n"
    "match_devices $3 $3\n"
    "match_devices $4 $4\n"
    "end_circuit NAND NAND MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets OUT OUT\n"
    "match_nets IN2 IN2\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_nets VDD VDD\n"
    "match_nets IN1 IN1\n"
    "match_pins $2 $2\n"
    "match_pins $1 $1\n"
    "match_pins $4 $4\n"
    "match_pins $3 $3\n"
    "match_pins $0 $0\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(15_EmptySubCircuitTest)
{
  const char *nls1 =
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
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
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $4 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $5 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $6 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
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
    "..."
  );

  EXPECT_EQ (good, true);
}
