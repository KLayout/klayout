
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

static std::string net2string (const db::Net &n, const db::Circuit *c = 0)
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
    res += i->pin (c) ? i->pin (c)->name () : "(null)";
  }
  return res;
}

static std::string nets2string (const db::Circuit &c)
{
  std::string res;
  for (db::Circuit::const_net_iterator n = c.begin_nets (); n != c.end_nets (); ++n) {
    res += net2string (*n, &c);
    res += "\n";
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

  db::GenericDeviceClass dc2;
  dc2.set_name ("dc2");
  dc2.add_port_definition (db::DevicePortDefinition ("A", ""));
  dc2.add_port_definition (db::DevicePortDefinition ("B", ""));

  std::auto_ptr<db::Circuit> c (new db::Circuit ());
  db::Device *d1 = new db::Device (&dc1, "d1");
  db::Device *d2a = new db::Device (&dc2, "d2a");
  db::Device *d2b = new db::Device (&dc2, "d2b");
  c->add_device (d1);
  c->add_device (d2a);
  c->add_device (d2b);

  db::Net *n1 = new db::Net ();
  EXPECT_EQ (n1->circuit (), 0);
  c->add_net (n1);
  n1->add_port (db::NetPortRef (d1, 0));
  n1->add_port (db::NetPortRef (d2a, 0));
  EXPECT_EQ (n1->circuit (), c.get ());

  db::Net *n2 = new db::Net ();
  c->add_net (n2);
  n2->add_port (db::NetPortRef (d1, 1));
  n2->add_port (db::NetPortRef (d2a, 1));
  n2->add_port (db::NetPortRef (d2b, 0));

  db::Net *n3 = new db::Net ();
  c->add_net (n3);
  n3->add_port (db::NetPortRef (d1, 2));
  n3->add_port (db::NetPortRef (d2b, 1));

  EXPECT_EQ (nets2string (*c),
    "d1:S,d2a:A\n"
    "d1:G,d2a:B,d2b:A\n"
    "d1:D,d2b:B\n"
  );

  db::Circuit cc = *c;
  c.reset (0);
  EXPECT_EQ (cc.begin_nets ()->circuit (), &cc);

  EXPECT_EQ (nets2string (cc),
    "d1:S,d2a:A\n"
    "d1:G,d2a:B,d2b:A\n"
    "d1:D,d2b:B\n"
  );
}

static std::string netlist2string (const db::Netlist &nl)
{
  std::string res;
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {
    res += "[" + c->name () + "]\n";
    res += nets2string (*c);
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
  c1->add_sub_circuit (sc1);

  db::SubCircuit *sc2 = new db::SubCircuit (c2);
  c1->add_sub_circuit (sc2);

  db::Net *n2a = new db::Net ();
  n2a->add_pin (db::NetPinRef (0));
  n2a->add_port (db::NetPortRef (d, 0));
  c2->add_net (n2a);

  db::Net *n2b = new db::Net ();
  n2b->add_port (db::NetPortRef (d, 1));
  n2b->add_pin (db::NetPinRef (1));
  c2->add_net (n2b);

  db::Net *n1a = new db::Net ();
  n1a->add_pin (db::NetPinRef (0));
  n1a->add_pin (db::NetPinRef (0, sc1));
  c1->add_net (n1a);

  db::Net *n1b = new db::Net ();
  n1b->add_pin (db::NetPinRef (1, sc1));
  n1b->add_pin (db::NetPinRef (0, sc2));
  c1->add_net (n1b);

  db::Net *n1c = new db::Net ();
  n1c->add_pin (db::NetPinRef (1, sc2));
  n1c->add_pin (db::NetPinRef (1));
  c1->add_net (n1c);

  EXPECT_EQ (netlist2string (*nl),
    "[c1]\n"
    "+c1p1,c2:c2p1\n"
    "c2:c2p2,c2:c2p1\n"
    "c2:c2p2,+c1p2\n"
    "[c2]\n"
    "D:A,+c2p1\n"
    "D:B,+c2p2\n"
  );

  db::Netlist nl2 = *nl;
  nl.reset (0);

  EXPECT_EQ (nl2.begin_circuits ()->netlist (), &nl2);

  EXPECT_EQ (netlist2string (nl2),
    "[c1]\n"
    "+c1p1,c2:c2p1\n"
    "c2:c2p2,c2:c2p1\n"
    "c2:c2p2,+c1p2\n"
    "[c2]\n"
    "D:A,+c2p1\n"
    "D:B,+c2p2\n"
  );
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
