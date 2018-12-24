
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "dbNetlist.h"

#include "tlUnitTest.h"
#include "tlString.h"

#include <memory>

static std::string pd2string (const db::DevicePortDefinition &pd)
{
  return pd.name () + "(" + pd.description () + ") #" + tl::to_string (pd.id ());
}

static std::string pd2string (const db::DeviceParameterDefinition &pd)
{
  return pd.name () + "(" + pd.description () + ")=" + tl::to_string (pd.default_value ()) + " #" + tl::to_string (pd.id ());
}

TEST(1_DevicePortDefinition)
{
  db::DevicePortDefinition pd;

  EXPECT_EQ (pd2string (pd), "() #0");
  pd.set_name ("name");
  pd.set_description ("nothing yet");
  EXPECT_EQ (pd2string (pd), "name(nothing yet) #0");

  db::DevicePortDefinition pd2;
  pd2 = pd;
  EXPECT_EQ (pd2string (pd2), "name(nothing yet) #0");
  pd2.set_name ("name2");
  pd2.set_description ("now it has something");
  EXPECT_EQ (pd2string (pd2), "name2(now it has something) #0");

  db::DeviceClass dc;
  dc.add_port_definition (pd);
  dc.add_port_definition (pd2);
  EXPECT_EQ (pd2string (dc.port_definitions ()[0]), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (dc.port_definitions ()[1]), "name2(now it has something) #1");

  dc.clear_port_definitions ();
  EXPECT_EQ (dc.port_definitions ().empty (), true);

  db::DeviceParameterDefinition ppd ("P1", "Parameter 1", 1.0);
  dc.add_parameter_definition (ppd);

  db::DeviceParameterDefinition ppd2 ("P2", "Parameter 2");
  dc.add_parameter_definition (ppd2);

  EXPECT_EQ (pd2string (dc.parameter_definitions ()[0]), "P1(Parameter 1)=1 #0");
  EXPECT_EQ (pd2string (dc.parameter_definitions ()[1]), "P2(Parameter 2)=0 #1");

  dc.clear_parameter_definitions ();
  EXPECT_EQ (dc.parameter_definitions ().empty (), true);

}

TEST(2_DeviceClass)
{
  db::DevicePortDefinition pd;
  pd.set_name ("name");
  pd.set_description ("nothing yet");

  db::DevicePortDefinition pd2;
  pd2.set_name ("name2");
  pd2.set_description ("now it has something");

  db::GenericDeviceClass dc;
  dc.set_name ("devname");
  dc.set_description ("devdesc");
  EXPECT_EQ (dc.name (), "devname");
  EXPECT_EQ (dc.description (), "devdesc");
  dc.add_port_definition (pd);
  dc.add_port_definition (pd2);
  EXPECT_EQ (dc.port_definitions ().size (), size_t (2));
  EXPECT_EQ (pd2string (dc.port_definitions ()[0]), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (dc.port_definitions ()[1]), "name2(now it has something) #1");

  EXPECT_EQ (pd2string (*dc.port_definition (dc.port_definitions ()[0].id ())), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (*dc.port_definition (dc.port_definitions ()[1].id ())), "name2(now it has something) #1");
  EXPECT_EQ (dc.port_definition (3), 0);

  db::GenericDeviceClass dc2 = dc;
  EXPECT_EQ (dc2.name (), "devname");
  EXPECT_EQ (dc2.description (), "devdesc");
  EXPECT_EQ (dc2.port_definitions ().size (), size_t (2));
  EXPECT_EQ (pd2string (*dc2.port_definition (dc2.port_definitions ()[0].id ())), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (*dc2.port_definition (dc2.port_definitions ()[1].id ())), "name2(now it has something) #1");
  EXPECT_EQ (dc2.port_definition (3), 0);
}

static std::string pins2string (const db::Circuit &c)
{
  std::string res;
  for (db::Circuit::const_pin_iterator i = c.begin_pins (); i != c.end_pins (); ++i) {
    if (!res.empty ()) {
      res += ",";
    }
    res += i->name ();
    res += "#" + tl::to_string (i->id ());
  }
  return res;
}

TEST(3_CircuitBasic)
{
  db::Circuit c;
  c.set_name ("name");
  EXPECT_EQ (c.name (), "name");

  db::Pin p1 ("p1");
  db::Pin p2 ("p2");
  c.add_pin (p1);
  c.add_pin (p2);
  EXPECT_EQ (pins2string (c), "p1#0,p2#1");

  EXPECT_EQ (c.pin_by_id (0)->name (), "p1");
  EXPECT_EQ (c.pin_by_id (1)->name (), "p2");
  EXPECT_EQ (c.pin_by_id (2), 0);

  db::Circuit c2 = c;
  EXPECT_EQ (c2.name (), "name");
  EXPECT_EQ (pins2string (c), "p1#0,p2#1");

  EXPECT_EQ (c2.pin_by_id (0)->name (), "p1");
  EXPECT_EQ (c2.pin_by_id (1)->name (), "p2");
  EXPECT_EQ (c2.pin_by_id (2), 0);
}

static std::string net2string (const db::Net &n)
{
  std::string res;
  for (db::Net::const_port_iterator i = n.begin_ports (); i != n.end_ports (); ++i) {
    if (! res.empty ()) {
      res += ",";
    }
    res += i->device () ? i->device ()->name () : "(null)";
    res += ":";
    res += i->port_def () ? i->port_def ()->name () : "(null)";
  }
  for (db::Net::const_pin_iterator i = n.begin_pins (); i != n.end_pins (); ++i) {
    if (! res.empty ()) {
      res += ",";
    }
    if (i->subcircuit ()) {
      res += i->subcircuit ()->circuit () ? i->subcircuit ()->circuit ()->name () : "(null)";
      res += ":";
    } else {
      res += "+";
    }
    res += i->pin () ? i->pin ()->name () : "(null)";
  }
  return res;
}

static std::string nets2string (const db::Circuit &c)
{
  std::string res;
  for (db::Circuit::const_net_iterator n = c.begin_nets (); n != c.end_nets (); ++n) {
    res += net2string (*n);
    res += "\n";
  }
  return res;
}

//  dual form of netlist
static std::string netlist2 (const db::Circuit &c)
{
  std::string res;

  std::string pins;
  for (db::Circuit::const_pin_iterator p = c.begin_pins (); p != c.end_pins (); ++p) {
    if (! pins.empty ()) {
      pins += ",";
    }
    const db::Net *net = c.net_for_pin (p->id ());
    pins += p->name ();
    pins += "=";
    pins += net ? net->name () : std::string ("(null)");
  }

  res += c.name () + ":" + pins + "\n";

  for (db::Circuit::const_device_iterator d = c.begin_devices (); d != c.end_devices (); ++d) {
    if (! d->device_class ()) {
      continue;
    }
    pins.clear ();
    for (size_t i = 0; i < d->device_class ()->port_definitions ().size (); ++i) {
      if (! pins.empty ()) {
        pins += ",";
      }
      const db::Net *net = d->net_for_port (i);
      pins += d->device_class ()->port_definitions () [i].name ();
      pins += "=";
      pins += net ? net->name () : std::string ("(null)");
    }
    res += "  D" + d->name () + ":" + pins + "\n";
  }

  for (db::Circuit::const_sub_circuit_iterator s = c.begin_sub_circuits (); s != c.end_sub_circuits (); ++s) {
    if (! s->circuit ()) {
      continue;
    }
    pins.clear ();
    for (size_t i = 0; i < s->circuit ()->pin_count (); ++i) {
      if (! pins.empty ()) {
        pins += ",";
      }
      pins += s->circuit ()->pin_by_id (i)->name ();
      pins += "=";
      const db::Net *net = s->net_for_pin (i);
      pins += net ? net->name () : std::string ("(null)");
    }
    res += "  X" + s->name () + ":" + pins + "\n";
  }

  return res;
}

TEST(4_CircuitDevices)
{
  db::GenericDeviceClass dc1;
  dc1.set_name ("dc1");
  dc1.add_port_definition (db::DevicePortDefinition ("S", "Source"));
  dc1.add_port_definition (db::DevicePortDefinition ("G", "Gate"));
  dc1.add_port_definition (db::DevicePortDefinition ("D", "Drain"));
  dc1.add_parameter_definition (db::DeviceParameterDefinition ("U", "", 1.0));
  dc1.add_parameter_definition (db::DeviceParameterDefinition ("V", "", 2.0));

  db::GenericDeviceClass dc2;
  dc2.set_name ("dc2");
  dc2.add_port_definition (db::DevicePortDefinition ("A", ""));
  dc2.add_port_definition (db::DevicePortDefinition ("B", ""));
  dc2.add_parameter_definition (db::DeviceParameterDefinition ("U", "", 2.0));
  dc2.add_parameter_definition (db::DeviceParameterDefinition ("V", "", 1.0));

  std::auto_ptr<db::Circuit> c (new db::Circuit ());
  c->set_name ("c");

  EXPECT_EQ (netlist2 (*c),
    "c:\n"
  );

  db::Device *d1 = new db::Device (&dc1, "d1");
  db::Device *d2a = new db::Device (&dc2, "d2a");
  db::Device *d2b = new db::Device (&dc2, "d2b");
  c->add_device (d1);
  c->add_device (d2a);
  c->add_device (d2b);

  EXPECT_EQ (d1->parameter_value (0), 1.0);
  EXPECT_EQ (d1->parameter_value (1), 2.0);
  EXPECT_EQ (d2a->parameter_value (0), 2.0);
  EXPECT_EQ (d2a->parameter_value (1), 1.0);
  d1->set_parameter_value (1, 1.5);
  EXPECT_EQ (d1->parameter_value (0), 1.0);
  EXPECT_EQ (d1->parameter_value (1), 1.5);
  d1->set_parameter_value (0, 0.5);
  EXPECT_EQ (d1->parameter_value (0), 0.5);
  EXPECT_EQ (d1->parameter_value (1), 1.5);

  d2a->set_parameter_value (0, -1.0);
  EXPECT_EQ (d2a->parameter_value (0), -1.0);
  EXPECT_EQ (d2a->parameter_value (1), 1.0);

  EXPECT_EQ (netlist2 (*c),
    "c:\n"
    "  Dd1:S=(null),G=(null),D=(null)\n"
    "  Dd2a:A=(null),B=(null)\n"
    "  Dd2b:A=(null),B=(null)\n"
  );

  db::Net *n1 = new db::Net ();
  n1->set_name ("n1");
  EXPECT_EQ (n1->circuit (), 0);
  c->add_net (n1);
  n1->add_port (db::NetPortRef (d1, 0));
  n1->add_port (db::NetPortRef (d2a, 0));
  EXPECT_EQ (n1->circuit (), c.get ());

  db::Net *n2 = new db::Net ();
  n2->set_name ("n2");
  c->add_net (n2);
  n2->add_port (db::NetPortRef (d1, 1));
  n2->add_port (db::NetPortRef (d2a, 1));
  n2->add_port (db::NetPortRef (d2b, 0));

  EXPECT_EQ (netlist2 (*c),
    "c:\n"
    "  Dd1:S=n1,G=n2,D=(null)\n"
    "  Dd2a:A=n1,B=n2\n"
    "  Dd2b:A=n2,B=(null)\n"
  );

  db::Net *n3 = new db::Net ();
  n3->set_name ("n3");
  c->add_net (n3);
  n3->add_port (db::NetPortRef (d1, 2));
  n3->add_port (db::NetPortRef (d2b, 1));

  EXPECT_EQ (nets2string (*c),
    "d1:S,d2a:A\n"
    "d1:G,d2a:B,d2b:A\n"
    "d1:D,d2b:B\n"
  );

  EXPECT_EQ (netlist2 (*c),
    "c:\n"
    "  Dd1:S=n1,G=n2,D=n3\n"
    "  Dd2a:A=n1,B=n2\n"
    "  Dd2b:A=n2,B=n3\n"
  );

  db::Circuit cc = *c;
  c.reset (0);
  EXPECT_EQ (cc.begin_nets ()->circuit (), &cc);

  EXPECT_EQ (nets2string (cc),
    "d1:S,d2a:A\n"
    "d1:G,d2a:B,d2b:A\n"
    "d1:D,d2b:B\n"
  );

  EXPECT_EQ (netlist2 (cc),
    "c:\n"
    "  Dd1:S=n1,G=n2,D=n3\n"
    "  Dd2a:A=n1,B=n2\n"
    "  Dd2b:A=n2,B=n3\n"
  );
}

static std::string nl2string (const db::Netlist &nl)
{
  std::string res;
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {
    res += "[" + c->name () + "]\n";
    res += nets2string (*c);
  }
  return res;
}

//  dual form of netlist
static std::string netlist2 (const db::Netlist &nl)
{
  std::string res;
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {
    res += netlist2 (*c);
  }
  return res;
}

TEST(4_NetlistSubcircuits)
{
  std::auto_ptr<db::Netlist> nl (new db::Netlist ());

  db::GenericDeviceClass *dc = new db::GenericDeviceClass ();
  dc->set_name ("dc2");
  dc->add_port_definition (db::DevicePortDefinition ("A", ""));
  dc->add_port_definition (db::DevicePortDefinition ("B", ""));
  nl->add_device_class (dc);

  db::Circuit *c1 = new db::Circuit ();
  EXPECT_EQ (c1->netlist (), 0);
  c1->set_name ("c1");
  c1->add_pin (db::Pin ("c1p1"));
  c1->add_pin (db::Pin ("c1p2"));
  nl->add_circuit (c1);
  EXPECT_EQ (c1->netlist (), nl.get ());

  db::Circuit *c2 = new db::Circuit ();
  c2->set_name ("c2");
  c2->add_pin (db::Pin ("c2p1"));
  c2->add_pin (db::Pin ("c2p2"));
  nl->add_circuit (c2);

  db::Device *d = new db::Device (dc, "D");
  c2->add_device (d);

  db::SubCircuit *sc1 = new db::SubCircuit (c2);
  sc1->set_name ("sc1");
  c1->add_sub_circuit (sc1);

  db::SubCircuit *sc2 = new db::SubCircuit (c2);
  sc2->set_name ("sc2");
  c1->add_sub_circuit (sc2);

  db::Net *n2a = new db::Net ();
  c2->add_net (n2a);
  n2a->set_name ("n2a");
  n2a->add_pin (db::NetPinRef (0));
  n2a->add_port (db::NetPortRef (d, 0));

  db::Net *n2b = new db::Net ();
  c2->add_net (n2b);
  n2b->set_name ("n2b");
  n2b->add_port (db::NetPortRef (d, 1));
  n2b->add_pin (db::NetPinRef (1));

  db::Net *n1a = new db::Net ();
  c1->add_net (n1a);
  n1a->set_name ("n1a");
  n1a->add_pin (db::NetPinRef (0));
  n1a->add_pin (db::NetPinRef (sc1, 0));

  db::Net *n1b = new db::Net ();
  c1->add_net (n1b);
  n1b->set_name ("n1b");
  n1b->add_pin (db::NetPinRef (sc1, 1));
  n1b->add_pin (db::NetPinRef (sc2, 0));

  db::Net *n1c = new db::Net ();
  c1->add_net (n1c);
  n1c->set_name ("n1c");
  n1c->add_pin (db::NetPinRef (sc2, 1));
  n1c->add_pin (db::NetPinRef (1));

  EXPECT_EQ (nl2string (*nl),
    "[c1]\n"
    "+c1p1,c2:c2p1\n"
    "c2:c2p2,c2:c2p1\n"
    "c2:c2p2,+c1p2\n"
    "[c2]\n"
    "D:A,+c2p1\n"
    "D:B,+c2p2\n"
  );

  EXPECT_EQ (netlist2 (*nl),
    "c1:c1p1=n1a,c1p2=n1c\n"
    "  Xsc1:c2p1=n1a,c2p2=n1b\n"
    "  Xsc2:c2p1=n1b,c2p2=n1c\n"
    "c2:c2p1=n2a,c2p2=n2b\n"
    "  DD:A=n2a,B=n2b\n"
  );

  //  check netlist
  for (db::Netlist::circuit_iterator c = nl->begin_circuits (); c != nl->end_circuits (); ++c) {
    for (db::Circuit::net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      for (db::Net::port_iterator i = n->begin_ports (); i != n->end_ports (); ++i) {
        EXPECT_EQ (i->net (), n.operator-> ());
      }
      for (db::Net::pin_iterator i = n->begin_pins (); i != n->end_pins (); ++i) {
        EXPECT_EQ (i->net (), n.operator-> ());
      }
    }
  }

  db::Netlist nl2 = *nl;
  nl.reset (0);

  EXPECT_EQ (nl2.begin_circuits ()->netlist (), &nl2);

  EXPECT_EQ (nl2string (nl2),
    "[c1]\n"
    "+c1p1,c2:c2p1\n"
    "c2:c2p2,c2:c2p1\n"
    "c2:c2p2,+c1p2\n"
    "[c2]\n"
    "D:A,+c2p1\n"
    "D:B,+c2p2\n"
  );

  EXPECT_EQ (netlist2 (nl2),
    "c1:c1p1=n1a,c1p2=n1c\n"
    "  Xsc1:c2p1=n1a,c2p2=n1b\n"
    "  Xsc2:c2p1=n1b,c2p2=n1c\n"
    "c2:c2p1=n2a,c2p2=n2b\n"
    "  DD:A=n2a,B=n2b\n"
  );

  //  check netlist
  for (db::Netlist::circuit_iterator c = nl2.begin_circuits (); c != nl2.end_circuits (); ++c) {
    for (db::Circuit::net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      for (db::Net::port_iterator i = n->begin_ports (); i != n->end_ports (); ++i) {
        EXPECT_EQ (i->net (), n.operator-> ());
      }
      for (db::Net::pin_iterator i = n->begin_pins (); i != n->end_pins (); ++i) {
        EXPECT_EQ (i->net (), n.operator-> ());
      }
    }
  }
}

TEST(5_SubCircuit)
{
  db::SubCircuit sc, sc2;

  sc.set_name ("sc");
  EXPECT_EQ (sc.name (), "sc");
  sc.set_trans (db::DCplxTrans (2.5));
  EXPECT_EQ (sc.trans ().to_string (), "r0 *2.5 0,0");

  sc2 = sc;
  EXPECT_EQ (sc2.name (), "sc");
  EXPECT_EQ (sc2.trans ().to_string (), "r0 *2.5 0,0");
}

TEST(6_Net)
{
  db::Net n, n2;

  n.set_name ("n");
  EXPECT_EQ (n.name (), "n");
  n.set_cluster_id (17);
  EXPECT_EQ (int (n.cluster_id ()), 17);

  n2 = n;
  EXPECT_EQ (n2.name (), "n");
  EXPECT_EQ (int (n2.cluster_id ()), 17);

  n.clear ();
  EXPECT_EQ (n.name (), "");
  EXPECT_EQ (int (n.cluster_id ()), 0);
}

TEST(7_NetPortsEditing)
{
  db::Circuit c;
  db::GenericDeviceClass dc;
  dc.add_port_definition (db::DevicePortDefinition ("A", ""));
  dc.add_port_definition (db::DevicePortDefinition ("B", ""));

  db::Device *d1 = new db::Device (&dc, "D1");
  c.add_device (d1);
  db::Device *d2 = new db::Device (&dc, "D2");
  c.add_device (d2);

  db::Net *n1 = new db::Net ();
  n1->set_name ("n1");
  c.add_net (n1);

  db::Net *n2 = new db::Net ();
  n2->set_name ("n2");
  c.add_net (n2);

  d1->connect_port (0, n1);
  d1->connect_port (1, n2);

  d2->connect_port (1, n1);
  d2->connect_port (0, n2);

  EXPECT_EQ (d1->net_for_port (0), n1);
  EXPECT_EQ (d1->net_for_port (1), n2);
  EXPECT_EQ (d2->net_for_port (0), n2);
  EXPECT_EQ (d2->net_for_port (1), n1);

  EXPECT_EQ (net2string (*n1), "D1:A,D2:B");
  EXPECT_EQ (net2string (*n2), "D1:B,D2:A");

  d1->connect_port (0, n2);
  d1->connect_port (1, n1);

  EXPECT_EQ (d1->net_for_port (0), n2);
  EXPECT_EQ (d1->net_for_port (1), n1);

  EXPECT_EQ (net2string (*n1), "D2:B,D1:B");
  EXPECT_EQ (net2string (*n2), "D2:A,D1:A");

  d1->connect_port (0, 0);
  EXPECT_EQ (d1->net_for_port (0), 0);

  EXPECT_EQ (net2string (*n1), "D2:B,D1:B");
  EXPECT_EQ (net2string (*n2), "D2:A");

  delete d1;
  d1 = 0;

  EXPECT_EQ (c.begin_devices ()->name (), "D2");
  EXPECT_EQ (++c.begin_devices () == c.end_devices (), true);

  EXPECT_EQ (net2string (*n1), "D2:B");
  EXPECT_EQ (net2string (*n2), "D2:A");

  delete n1;
  n1 = 0;

  EXPECT_EQ (c.begin_nets ()->name (), "n2");
  EXPECT_EQ (++c.begin_nets () == c.end_nets (), true);

  EXPECT_EQ (net2string (*n2), "D2:A");

  EXPECT_EQ (d2->net_for_port (0), n2);
  EXPECT_EQ (d2->net_for_port (1), 0);
}

TEST(8_NetSubCircuitsEditing)
{
  db::Circuit c;
  c.set_name ("c");
  c.add_pin (db::Pin ("X"));
  c.add_pin (db::Pin ("Y"));

  db::Circuit cc1;
  cc1.set_name ("sc1");
  cc1.add_pin (db::Pin ("A"));
  cc1.add_pin (db::Pin ("B"));

  db::Circuit cc2;
  cc2.set_name ("sc2");
  cc2.add_pin (db::Pin ("A"));
  cc2.add_pin (db::Pin ("B"));

  db::SubCircuit *sc1 = new db::SubCircuit (&cc1, "sc1");
  c.add_sub_circuit (sc1);

  db::SubCircuit *sc2 = new db::SubCircuit (&cc2, "sc2");
  c.add_sub_circuit (sc2);

  db::Net *n1 = new db::Net ();
  n1->set_name ("n1");
  c.add_net (n1);

  db::Net *n2 = new db::Net ();
  n2->set_name ("n2");
  c.add_net (n2);

  c.connect_pin (0, n1);
  EXPECT_EQ (c.net_for_pin (0), n1);
  EXPECT_EQ (c.net_for_pin (1), 0);

  sc1->connect_pin (0, n1);
  sc1->connect_pin (1, n2);

  sc2->connect_pin (1, n1);
  sc2->connect_pin (0, n2);

  EXPECT_EQ (sc1->net_for_pin (0), n1);
  EXPECT_EQ (sc1->net_for_pin (1), n2);
  EXPECT_EQ (sc2->net_for_pin (0), n2);
  EXPECT_EQ (sc2->net_for_pin (1), n1);

  EXPECT_EQ (net2string (*n1), "+X,sc1:A,sc2:B");
  EXPECT_EQ (net2string (*n2), "sc1:B,sc2:A");

  c.connect_pin (0, 0);
  EXPECT_EQ (c.net_for_pin (0), 0);

  EXPECT_EQ (net2string (*n1), "sc1:A,sc2:B");
  EXPECT_EQ (net2string (*n2), "sc1:B,sc2:A");

  sc1->connect_pin (0, n2);
  sc1->connect_pin (1, n1);

  EXPECT_EQ (sc1->net_for_pin (0), n2);
  EXPECT_EQ (sc1->net_for_pin (1), n1);

  EXPECT_EQ (net2string (*n1), "sc2:B,sc1:B");
  EXPECT_EQ (net2string (*n2), "sc2:A,sc1:A");

  sc1->connect_pin (0, 0);
  EXPECT_EQ (sc1->net_for_pin (0), 0);

  EXPECT_EQ (net2string (*n1), "sc2:B,sc1:B");
  EXPECT_EQ (net2string (*n2), "sc2:A");

  delete sc1;
  sc1 = 0;

  EXPECT_EQ (c.begin_sub_circuits ()->name (), "sc2");
  EXPECT_EQ (++c.begin_sub_circuits () == c.end_sub_circuits (), true);

  EXPECT_EQ (net2string (*n1), "sc2:B");
  EXPECT_EQ (net2string (*n2), "sc2:A");

  c.connect_pin (1, n1);
  EXPECT_EQ (net2string (*n1), "sc2:B,+Y");
  EXPECT_EQ (c.net_for_pin (1), n1);

  delete n1;
  n1 = 0;

  EXPECT_EQ (c.net_for_pin (1), 0);

  EXPECT_EQ (c.begin_nets ()->name (), "n2");
  EXPECT_EQ (++c.begin_nets () == c.end_nets (), true);

  EXPECT_EQ (net2string (*n2), "sc2:A");

  EXPECT_EQ (sc2->net_for_pin (0), n2);
  EXPECT_EQ (sc2->net_for_pin (1), 0);
}

TEST(9_NetPortRefBasics)
{
  db::Device d1, d2;

  EXPECT_EQ (db::NetPortRef (&d1, 0) == db::NetPortRef (&d1, 0), true);
  EXPECT_EQ (db::NetPortRef (&d1, 0) == db::NetPortRef (&d1, 1), false);
  EXPECT_EQ (db::NetPortRef (&d1, 0) == db::NetPortRef (&d2, 0), false);

  EXPECT_EQ (db::NetPortRef (&d1, 0) < db::NetPortRef (&d1, 0), false);
  EXPECT_EQ (db::NetPortRef (&d1, 0) < db::NetPortRef (&d1, 1), true);
  EXPECT_EQ (db::NetPortRef (&d1, 1) < db::NetPortRef (&d1, 0), false);
  EXPECT_NE ((db::NetPortRef (&d1, 0) < db::NetPortRef (&d2, 0)), (db::NetPortRef (&d2, 0) < db::NetPortRef (&d1, 0)));
}

TEST(10_NetPinRefBasics)
{
  db::SubCircuit d1, d2;

  EXPECT_EQ (db::NetPinRef (&d1, 0) == db::NetPinRef (&d1, 0), true);
  EXPECT_EQ (db::NetPinRef (&d1, 0) == db::NetPinRef (&d1, 1), false);
  EXPECT_EQ (db::NetPinRef (&d1, 0) == db::NetPinRef (&d2, 0), false);

  EXPECT_EQ (db::NetPinRef (&d1, 0) < db::NetPinRef (&d1, 0), false);
  EXPECT_EQ (db::NetPinRef (&d1, 0) < db::NetPinRef (&d1, 1), true);
  EXPECT_EQ (db::NetPinRef (&d1, 1) < db::NetPinRef (&d1, 0), false);
  EXPECT_NE ((db::NetPinRef (&d1, 0) < db::NetPinRef (&d2, 0)), (db::NetPinRef (&d2, 0) < db::NetPinRef (&d1, 0)));
}
