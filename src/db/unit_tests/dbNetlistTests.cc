
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


#include "dbNetlist.h"
#include "dbNetlistDeviceClasses.h"

#include "tlUnitTest.h"
#include "tlString.h"

#include <memory>

// ----------------------------------------------------------------------------------------

static std::string pd2string (const db::DeviceTerminalDefinition &pd)
{
  return pd.name () + "(" + pd.description () + ") #" + tl::to_string (pd.id ());
}

static std::string pd2string (const db::DeviceParameterDefinition &pd)
{
  return pd.name () + "(" + pd.description () + ")=" + tl::to_string (pd.default_value ()) + " #" + tl::to_string (pd.id ());
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

static std::string net2string (const db::Net &n)
{
  std::string res;
  for (db::Net::const_terminal_iterator i = n.begin_terminals (); i != n.end_terminals (); ++i) {
    if (! res.empty ()) {
      res += ",";
    }
    res += i->device () ? i->device ()->name () : "(null)";
    res += ":";
    res += i->terminal_def () ? i->terminal_def ()->name () : "(null)";
  }
  for (db::Net::const_pin_iterator i = n.begin_pins (); i != n.end_pins (); ++i) {
    if (! res.empty ()) {
      res += ",";
    }
    res += "+";
    res += i->pin () ? i->pin ()->name () : "(null)";
  }
  for (db::Net::const_subcircuit_pin_iterator i = n.begin_subcircuit_pins (); i != n.end_subcircuit_pins (); ++i) {
    if (! res.empty ()) {
      res += ",";
    }
    res += i->subcircuit ()->circuit_ref () ? i->subcircuit ()->circuit_ref ()->name () : "(null)";
    res += ":";
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
    for (size_t i = 0; i < d->device_class ()->terminal_definitions ().size (); ++i) {
      if (! pins.empty ()) {
        pins += ",";
      }
      const db::Net *net = d->net_for_terminal (i);
      pins += d->device_class ()->terminal_definitions () [i].name ();
      pins += "=";
      pins += net ? net->name () : std::string ("(null)");
    }
    res += "  D" + d->name ();
    if (d->device_abstract ()) {
      res += "/" + d->device_abstract ()->name ();
    }
    res += ":" + pins + "\n";
  }

  for (db::Circuit::const_subcircuit_iterator s = c.begin_subcircuits (); s != c.end_subcircuits (); ++s) {
    if (! s->circuit_ref ()) {
      continue;
    }
    pins.clear ();
    for (size_t i = 0; i < s->circuit_ref ()->pin_count (); ++i) {
      if (! pins.empty ()) {
        pins += ",";
      }
      pins += s->circuit_ref ()->pin_by_id (i)->name ();
      pins += "=";
      const db::Net *net = s->net_for_pin (i);
      pins += net ? net->name () : std::string ("(null)");
    }
    res += "  X" + s->name () + ":" + pins + "\n";
  }

  return res;
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

static std::string refs2string (const db::Circuit *c)
{
  std::string res;
  for (db::Circuit::const_refs_iterator r = c->begin_refs (); r != c->end_refs (); ++r) {
    if (!res.empty ()) {
      res += ",";
    }
    res += r->name ();
  }
  return res;
}

static std::string children2string (const db::Circuit *c)
{
  std::string res;
  for (db::Circuit::const_child_circuit_iterator r = c->begin_children (); r != c->end_children (); ++r) {
    if (!res.empty ()) {
      res += ",";
    }
    res += r->name ();
  }
  return res;
}

static std::string parents2string (const db::Circuit *c)
{
  std::string res;
  for (db::Circuit::const_parent_circuit_iterator r = c->begin_parents (); r != c->end_parents (); ++r) {
    if (!res.empty ()) {
      res += ",";
    }
    res += r->name ();
  }
  return res;
}

static std::string td2string_nc (db::Netlist *nl)
{
  std::string res;
  for (db::Netlist::top_down_circuit_iterator r = nl->begin_top_down (); r != nl->end_top_down (); ++r) {
    if (!res.empty ()) {
      res += ",";
    }
    res += r->name ();
  }
  return res;
}

static std::string td2string (const db::Netlist *nl)
{
  std::string res;
  for (db::Netlist::const_top_down_circuit_iterator r = nl->begin_top_down (); r != nl->end_top_down (); ++r) {
    if (!res.empty ()) {
      res += ",";
    }
    res += r->name ();
  }
  return res;
}

static std::string tcs2string_nc (db::Netlist *nl)
{
  std::string res;
  std::vector<db::Circuit *> tops = nl->top_circuits ();
  for (auto i = tops.begin (); i != tops.end (); ++i) {
    if (!res.empty ()) {
      res += ",";
    }
    res += (*i)->name ();
  }
  return res;
}

static std::string tcs2string (const db::Netlist *nl)
{
  std::string res;
  std::vector<const db::Circuit *> tops = nl->top_circuits ();
  for (auto i = tops.begin (); i != tops.end (); ++i) {
    if (!res.empty ()) {
      res += ",";
    }
    res += (*i)->name ();
  }
  return res;
}

static std::string tc2string_nc (db::Netlist *nl)
{
  const db::Circuit *tc = nl->top_circuit ();
  if (!tc) {
    return "(nil)";
  } else {
    return tc->name ();
  }
}

static std::string tc2string (const db::Netlist *nl)
{
  const db::Circuit *tc = nl->top_circuit ();
  if (!tc) {
    return "(nil)";
  } else {
    return tc->name ();
  }
}

static std::string bu2string_nc (db::Netlist *nl)
{
  std::string res;
  for (db::Netlist::bottom_up_circuit_iterator r = nl->begin_bottom_up (); r != nl->end_bottom_up (); ++r) {
    if (!res.empty ()) {
      res += ",";
    }
    res += r->name ();
  }
  return res;
}

static std::string bu2string (const db::Netlist *nl)
{
  std::string res;
  for (db::Netlist::const_bottom_up_circuit_iterator r = nl->begin_bottom_up (); r != nl->end_bottom_up (); ++r) {
    if (!res.empty ()) {
      res += ",";
    }
    res += r->name ();
  }
  return res;
}

// ----------------------------------------------------------------------------------------

TEST(0_DeviceClassTemplates)
{
  db::DeviceClassMOS3Transistor mos3;
  db::DeviceClass generic;

  EXPECT_EQ (db::DeviceClassTemplateBase::template_by_name ("MOS3") != 0, true);
  EXPECT_EQ (db::DeviceClassTemplateBase::template_by_name ("RES") != 0, true);
  EXPECT_EQ (db::DeviceClassTemplateBase::template_by_name ("DOESNTEXIST") == 0, true);
  EXPECT_EQ (db::DeviceClassTemplateBase::template_by_name ("MOS3")->is_of (&mos3), true);
  EXPECT_EQ (db::DeviceClassTemplateBase::template_by_name ("RES")->is_of (&mos3), false);
  EXPECT_EQ (db::DeviceClassTemplateBase::is_a (&mos3) != 0, true);
  EXPECT_EQ (db::DeviceClassTemplateBase::is_a (&generic) == 0, true);
  EXPECT_EQ (db::DeviceClassTemplateBase::is_a (&mos3)->name (), "MOS3");
}

// ----------------------------------------------------------------------------------------

TEST(1_DeviceTerminalDefinition)
{
  db::DeviceTerminalDefinition pd;

  EXPECT_EQ (pd2string (pd), "() #0");
  pd.set_name ("name");
  pd.set_description ("nothing yet");
  EXPECT_EQ (pd2string (pd), "name(nothing yet) #0");

  db::DeviceTerminalDefinition pd2;
  pd2 = pd;
  EXPECT_EQ (pd2string (pd2), "name(nothing yet) #0");
  pd2.set_name ("name2");
  pd2.set_description ("now it has something");
  EXPECT_EQ (pd2string (pd2), "name2(now it has something) #0");

  db::DeviceClass dc;
  dc.add_terminal_definition (pd);
  dc.add_terminal_definition (pd2);
  EXPECT_EQ (pd2string (dc.terminal_definitions ()[0]), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (dc.terminal_definitions ()[1]), "name2(now it has something) #1");

  dc.clear_terminal_definitions ();
  EXPECT_EQ (dc.terminal_definitions ().empty (), true);

  db::DeviceParameterDefinition ppd ("P1", "Parameter 1", 1.0, false);
  dc.add_parameter_definition (ppd);
  EXPECT_EQ (ppd.is_primary (), false);

  db::DeviceParameterDefinition ppd2 ("P2", "Parameter 2");
  dc.add_parameter_definition (ppd2);
  EXPECT_EQ (ppd2.is_primary (), true);

  EXPECT_EQ (pd2string (dc.parameter_definitions ()[0]), "P1(Parameter 1)=1 #0");
  EXPECT_EQ (pd2string (dc.parameter_definitions ()[1]), "P2(Parameter 2)=0 #1");

  dc.clear_parameter_definitions ();
  EXPECT_EQ (dc.parameter_definitions ().empty (), true);
}

TEST(2_DeviceClass)
{
  db::DeviceTerminalDefinition pd;
  pd.set_name ("name");
  pd.set_description ("nothing yet");

  db::DeviceTerminalDefinition pd2;
  pd2.set_name ("name2");
  pd2.set_description ("now it has something");

  db::DeviceClass dc;
  dc.set_name ("devname");
  dc.set_description ("devdesc");
  EXPECT_EQ (dc.name (), "devname");
  EXPECT_EQ (dc.description (), "devdesc");
  dc.add_terminal_definition (pd);
  dc.add_terminal_definition (pd2);
  EXPECT_EQ (dc.terminal_definitions ().size (), size_t (2));
  EXPECT_EQ (pd2string (dc.terminal_definitions ()[0]), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (dc.terminal_definitions ()[1]), "name2(now it has something) #1");

  EXPECT_EQ (pd2string (*dc.terminal_definition (dc.terminal_definitions ()[0].id ())), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (*dc.terminal_definition (dc.terminal_definitions ()[1].id ())), "name2(now it has something) #1");
  EXPECT_EQ (dc.terminal_definition (3), 0);

  db::DeviceClass dc2 = dc;
  EXPECT_EQ (dc2.name (), "devname");
  EXPECT_EQ (dc2.description (), "devdesc");
  EXPECT_EQ (dc2.terminal_definitions ().size (), size_t (2));
  EXPECT_EQ (pd2string (*dc2.terminal_definition (dc2.terminal_definitions ()[0].id ())), "name(nothing yet) #0");
  EXPECT_EQ (pd2string (*dc2.terminal_definition (dc2.terminal_definitions ()[1].id ())), "name2(now it has something) #1");
  EXPECT_EQ (dc2.terminal_definition (3), 0);
}

TEST(3_CircuitBasic)
{
  db::Circuit c;
  c.set_name ("name");
  EXPECT_EQ (c.name (), "name");

  c.set_boundary (db::DPolygon (db::DBox (0, 1, 2, 3)));
  EXPECT_EQ (c.boundary ().to_string (), "(0,1;0,3;2,3;2,1)");

  db::Pin p1 = c.add_pin ("p1");
  db::Pin p2 = c.add_pin ("p2");
  EXPECT_EQ (pins2string (c), "p1#0,p2#1");

  EXPECT_EQ (c.pin_by_id (0)->name (), "p1");
  EXPECT_EQ (c.pin_by_id (1)->name (), "p2");
  EXPECT_EQ (c.pin_by_id (2), 0);
  EXPECT_EQ (c.pin_by_name ("p1")->name (), "p1");
  EXPECT_EQ (c.pin_by_name ("doesnt_exist") == 0, true);
  EXPECT_EQ (c.pin_by_name ("p2")->name (), "p2");
  EXPECT_EQ (c.pin_by_id (0)->begin_properties () == c.pin_by_id (0)->end_properties (), true);

  EXPECT_EQ (c.pin_by_id (0)->property (17).to_string (), "nil");
  c.pin_by_id (0)->set_property (17, 42);
  EXPECT_EQ (c.pin_by_id (0)->begin_properties () == c.pin_by_id (0)->end_properties (), false);
  EXPECT_EQ (c.pin_by_id (0)->begin_properties ()->second.to_string (), "42");
  EXPECT_EQ (c.pin_by_id (0)->property (17).to_string (), "42");

  db::Circuit c2 = c;
  EXPECT_EQ (c2.pin_by_id (0)->property (17).to_string (), "42");
  EXPECT_EQ (c2.name (), "name");
  EXPECT_EQ (pins2string (c), "p1#0,p2#1");

  EXPECT_EQ (c2.pin_by_id (0)->name (), "p1");
  EXPECT_EQ (c2.pin_by_id (1)->name (), "p2");
  EXPECT_EQ (c2.pin_by_id (2), 0);

  c2.remove_pin (1);
  EXPECT_EQ (c2.pin_by_id (0)->name (), "p1");
  EXPECT_EQ (c2.pin_by_id (1), 0);
  EXPECT_EQ (c2.pin_by_id (2), 0);

  db::Pin p3 = c2.add_pin ("p3");
  EXPECT_EQ (c2.pin_by_id (0)->name (), "p1");
  EXPECT_EQ (c2.pin_by_id (1), 0);
  EXPECT_EQ (c2.pin_by_id (2)->name (), "p3");
  EXPECT_EQ (c2.pin_by_id (3), 0);
}

TEST(4_CircuitDevices)
{
  db::DeviceClass dc1;
  dc1.set_name ("dc1");
  dc1.add_terminal_definition (db::DeviceTerminalDefinition ("S", "Source"));
  dc1.add_terminal_definition (db::DeviceTerminalDefinition ("G", "Gate"));
  dc1.add_terminal_definition (db::DeviceTerminalDefinition ("D", "Drain"));
  dc1.add_parameter_definition (db::DeviceParameterDefinition ("U", "", 1.0));
  dc1.add_parameter_definition (db::DeviceParameterDefinition ("V", "", 2.0));

  db::DeviceClass dc2;
  dc2.set_name ("dc2");
  dc2.add_terminal_definition (db::DeviceTerminalDefinition ("A", ""));
  dc2.add_terminal_definition (db::DeviceTerminalDefinition ("B", ""));
  dc2.add_parameter_definition (db::DeviceParameterDefinition ("U", "", 2.0));
  dc2.add_parameter_definition (db::DeviceParameterDefinition ("V", "", 1.0));

  std::unique_ptr<db::Circuit> c (new db::Circuit ());
  c->set_name ("c");

  EXPECT_EQ (netlist2 (*c),
    "c:\n"
  );

  db::Device *dd = new db::Device (&dc1, "dd");
  db::Device *d1 = new db::Device (&dc1, "d1");
  db::Device *d2a = new db::Device (&dc2, "d2a");
  db::Device *d2b = new db::Device (&dc2, "d2x");

  EXPECT_EQ (d1->circuit () == 0, true);

  c->add_device (d1);
  EXPECT_EQ (d1->circuit () == c.get (), true);
  EXPECT_EQ (d1->id (), size_t (1));
  EXPECT_EQ (c->device_by_id (d1->id ()) == d1, true);
  EXPECT_EQ (c->device_by_name (d1->name ()) == d1, true);

  c->add_device (dd);
  EXPECT_EQ (dd->id (), size_t (2));
  EXPECT_EQ (c->device_by_id (dd->id ()) == dd, true);
  EXPECT_EQ (c->device_by_name (dd->name ()) == dd, true);

  c->add_device (d2a);
  EXPECT_EQ (d2a->id (), size_t (3));
  EXPECT_EQ (c->device_by_id (d2a->id ()) == d2a, true);
  EXPECT_EQ (c->device_by_name (d2a->name ()) == d2a, true);

  c->add_device (d2b);
  EXPECT_EQ (d2b->id (), size_t (4));
  EXPECT_EQ (c->device_by_id (d2b->id ()) == d2b, true);
  EXPECT_EQ (c->device_by_name (d2b->name ()) == d2b, true);

  d2b->set_name ("d2b");
  EXPECT_EQ (c->device_by_id (d2b->id ()) == d2b, true);
  EXPECT_EQ (c->device_by_name (d2b->name ()) == d2b, true);
  EXPECT_EQ (c->device_by_name ("d2x") == 0, true);

  c->remove_device (dd);
  dd = 0;
  EXPECT_EQ (c->device_by_id (d2a->id ()) == d2a, true);
  EXPECT_EQ (c->device_by_id (2) == 0, true);
  EXPECT_EQ (c->device_by_name (d2a->name ()) == d2a, true);
  EXPECT_EQ (c->device_by_name ("doesnt_exist") == 0, true);

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
  n1->set_cluster_id (41);
  EXPECT_EQ (n1->circuit (), 0);
  c->add_net (n1);
  n1->add_terminal (db::NetTerminalRef (d1, 0));
  n1->add_terminal (db::NetTerminalRef (d2a, 0));
  EXPECT_EQ (n1->circuit (), c.get ());
  EXPECT_EQ (c->net_by_cluster_id (17) == 0, true);
  EXPECT_EQ (c->net_by_cluster_id (41) == n1, true);
  EXPECT_EQ (c->net_by_name ("doesnt_exist") == 0, true);
  EXPECT_EQ (c->net_by_name ("n1") == n1, true);

  db::Net *n2 = new db::Net ();
  n2->set_name ("n2x");
  n2->set_cluster_id (17);
  c->add_net (n2);
  n2->add_terminal (db::NetTerminalRef (d1, 1));
  n2->add_terminal (db::NetTerminalRef (d2a, 1));
  n2->add_terminal (db::NetTerminalRef (d2b, 0));
  EXPECT_EQ (c->net_by_cluster_id (17) == n2, true);
  EXPECT_EQ (c->net_by_name ("n2x") == n2, true);

  n2->set_name ("n2");
  n2->set_cluster_id (42);
  EXPECT_EQ (c->net_by_cluster_id (17) == 0, true);
  EXPECT_EQ (c->net_by_name ("n2x") == 0, true);
  EXPECT_EQ (c->net_by_cluster_id (42) == n2, true);
  EXPECT_EQ (c->net_by_name ("n2") == n2, true);

  EXPECT_EQ (netlist2 (*c),
    "c:\n"
    "  Dd1:S=n1,G=n2,D=(null)\n"
    "  Dd2a:A=n1,B=n2\n"
    "  Dd2b:A=n2,B=(null)\n"
  );

  db::Net *n3 = new db::Net ();
  n3->set_name ("n3");
  c->add_net (n3);
  n3->add_terminal (db::NetTerminalRef (d1, 2));
  n3->add_terminal (db::NetTerminalRef (d2b, 1));

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

TEST(4_NetlistSubcircuits)
{
  std::unique_ptr<db::Netlist> nl (new db::Netlist ());

  db::DeviceClass *dc = new db::DeviceClass ();
  dc->set_name ("dc2");
  dc->add_terminal_definition (db::DeviceTerminalDefinition ("A", ""));
  dc->add_terminal_definition (db::DeviceTerminalDefinition ("B", ""));
  nl->add_device_class (dc);

  std::unique_ptr<db::Netlist> nldup (new db::Netlist ());
  nldup->add_device_class (dc->clone ());

  db::DeviceAbstract *dm = new db::DeviceAbstract ();
  dm->set_device_class (dc);
  EXPECT_EQ (dm->device_class () == dc, true);
  dm->set_name ("dm2");
  dm->set_cell_index (42);
  dm->set_cluster_id_for_terminal (0, 17);
  nl->add_device_abstract (dm);

  db::Circuit *c1 = new db::Circuit ();
  c1->set_cell_index (17);
  EXPECT_EQ (c1->netlist (), 0);
  c1->set_name ("c1");
  c1->add_pin ("c1p1");
  c1->add_pin ("c1p2");
  nl->add_circuit (c1);
  EXPECT_EQ (c1->netlist (), nl.get ());
  EXPECT_EQ (nl->circuit_by_name ("c1") == c1, true);
  EXPECT_EQ (nl->circuit_by_name ("doesnt_exist") == 0, true);
  EXPECT_EQ (nl->circuit_by_cell_index (17) == c1, true);
  EXPECT_EQ (nl->circuit_by_cell_index (42) == 0, true);

  db::Circuit *c2 = new db::Circuit ();
  c2->set_name ("c2x");
  c2->add_pin ("c2p1");
  c2->add_pin ("c2p2");
  c2->set_cell_index (41);
  nl->add_circuit (c2);
  EXPECT_EQ (nl->circuit_by_name ("c2x") == c2, true);
  EXPECT_EQ (nl->circuit_by_cell_index (41) == c2, true);

  c2->set_name ("c2");
  EXPECT_EQ (nl->circuit_by_name ("c2x") == 0, true);
  EXPECT_EQ (nl->circuit_by_name ("c2") == c2, true);
  c2->set_cell_index (42);
  EXPECT_EQ (nl->circuit_by_cell_index (41) == 0, true);
  EXPECT_EQ (nl->circuit_by_cell_index (42) == c2, true);

  db::Device *d = new db::Device (dc, dm, "D");
  c2->add_device (d);
  EXPECT_EQ (d->device_abstract ()->name (), "dm2");

  EXPECT_EQ (refs2string (c2), "");
  db::SubCircuit *sc1 = new db::SubCircuit (c2);
  sc1->set_name ("sc1");
  EXPECT_EQ (refs2string (c2), "sc1");
  EXPECT_EQ (sc1->circuit () == 0, true);
  c1->add_subcircuit (sc1);
  EXPECT_EQ (sc1->circuit () == c1, true);
  EXPECT_EQ (sc1->id (), size_t (1));
  EXPECT_EQ (c1->subcircuit_by_id (sc1->id ()) == sc1, true);
  EXPECT_EQ (c1->subcircuit_by_id (2) == 0, true);
  EXPECT_EQ (c1->subcircuit_by_name (sc1->name ()) == sc1, true);
  EXPECT_EQ (c1->subcircuit_by_name ("doesnt_exist") == 0, true);

  db::SubCircuit *sc2 = new db::SubCircuit (c2);
  sc2->set_name ("scx");
  EXPECT_EQ (refs2string (c2), "sc1,scx");
  c1->add_subcircuit (sc2);
  EXPECT_EQ (sc2->id (), size_t (2));
  EXPECT_EQ (c1->subcircuit_by_id (sc2->id ()) == sc2, true);
  EXPECT_EQ (c1->subcircuit_by_name (sc2->name ()) == sc2, true);

  sc2->set_name ("sc2");
  EXPECT_EQ (refs2string (c2), "sc1,sc2");
  EXPECT_EQ (c1->subcircuit_by_id (sc2->id ()) == sc2, true);
  EXPECT_EQ (c1->subcircuit_by_name (sc2->name ()) == sc2, true);
  EXPECT_EQ (c1->subcircuit_by_name ("scx") == 0, true);

  db::Net *n2a = new db::Net ();
  c2->add_net (n2a);
  n2a->set_name ("n2a");
  n2a->add_pin (db::NetPinRef (0));
  n2a->add_terminal (db::NetTerminalRef (d, 0));

  db::Net *n2b = new db::Net ();
  c2->add_net (n2b);
  n2b->set_name ("n2b");
  n2b->add_terminal (db::NetTerminalRef (d, 1));
  n2b->add_pin (db::NetPinRef (1));

  db::Net *n1a = new db::Net ();
  c1->add_net (n1a);
  n1a->set_name ("n1a");
  n1a->add_pin (db::NetPinRef (0));
  n1a->add_subcircuit_pin (db::NetSubcircuitPinRef (sc1, 0));

  db::Net *n1b = new db::Net ();
  c1->add_net (n1b);
  n1b->set_name ("n1b");
  n1b->add_subcircuit_pin (db::NetSubcircuitPinRef (sc1, 1));
  n1b->add_subcircuit_pin (db::NetSubcircuitPinRef (sc2, 0));

  db::Net *n1c = new db::Net ();
  c1->add_net (n1c);
  n1c->set_name ("n1c");
  n1c->add_subcircuit_pin (db::NetSubcircuitPinRef (sc2, 1));
  n1c->add_pin (db::NetPinRef (1));

  EXPECT_EQ (nl2string (*nl),
    "[c1]\n"
    "+c1p1,c2:c2p1\n"
    "c2:c2p2,c2:c2p1\n"
    "+c1p2,c2:c2p2\n"
    "[c2]\n"
    "D:A,+c2p1\n"
    "D:B,+c2p2\n"
  );

  EXPECT_EQ (nl->to_string (),
    "circuit c1 (c1p1=n1a,c1p2=n1c);\n"
    "  subcircuit c2 sc1 (c2p1=n1a,c2p2=n1b);\n"
    "  subcircuit c2 sc2 (c2p1=n1b,c2p2=n1c);\n"
    "end;\n"
    "circuit c2 (c2p1=n2a,c2p2=n2b);\n"
    "  device dc2 D (A=n2a,B=n2b) ();\n"
    "end;\n"
  );

  nldup->from_string (nl->to_string ());

  EXPECT_EQ (nldup->to_string (),
    "circuit c1 (c1p1=n1a,c1p2=n1c);\n"
    "  subcircuit c2 sc1 (c2p1=n1a,c2p2=n1b);\n"
    "  subcircuit c2 sc2 (c2p1=n1b,c2p2=n1c);\n"
    "end;\n"
    "circuit c2 (c2p1=n2a,c2p2=n2b);\n"
    "  device dc2 D (A=n2a,B=n2b) ();\n"
    "end;\n"
  );

  EXPECT_EQ (netlist2 (*nl),
    "c1:c1p1=n1a,c1p2=n1c\n"
    "  Xsc1:c2p1=n1a,c2p2=n1b\n"
    "  Xsc2:c2p1=n1b,c2p2=n1c\n"
    "c2:c2p1=n2a,c2p2=n2b\n"
    "  DD/dm2:A=n2a,B=n2b\n"
  );

  //  check netlist
  for (db::Netlist::circuit_iterator c = nl->begin_circuits (); c != nl->end_circuits (); ++c) {
    for (db::Circuit::net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      for (db::Net::terminal_iterator i = n->begin_terminals (); i != n->end_terminals (); ++i) {
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
    "+c1p2,c2:c2p2\n"
    "[c2]\n"
    "D:A,+c2p1\n"
    "D:B,+c2p2\n"
  );

  EXPECT_EQ (netlist2 (nl2),
    "c1:c1p1=n1a,c1p2=n1c\n"
    "  Xsc1:c2p1=n1a,c2p2=n1b\n"
    "  Xsc2:c2p1=n1b,c2p2=n1c\n"
    "c2:c2p1=n2a,c2p2=n2b\n"
    "  DD/dm2:A=n2a,B=n2b\n"
  );

  //  check netlist
  for (db::Netlist::circuit_iterator c = nl2.begin_circuits (); c != nl2.end_circuits (); ++c) {
    for (db::Circuit::net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      for (db::Net::terminal_iterator i = n->begin_terminals (); i != n->end_terminals (); ++i) {
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
  EXPECT_EQ (n2.expanded_name (), "n");
  n2.set_name ("");
  EXPECT_EQ (n2.expanded_name (), "$17");
  n2.set_cluster_id (std::numeric_limits<size_t>::max () - 2);
  EXPECT_EQ (n2.expanded_name (), "$I3");

  n.clear ();
  EXPECT_EQ (n.name (), "");
  EXPECT_EQ (int (n.cluster_id ()), 0);

  EXPECT_EQ (n.pin_count (), size_t (0));
  EXPECT_EQ (n.terminal_count (), size_t (0));
  EXPECT_EQ (n.is_floating (), true);
  EXPECT_EQ (n.is_passive (), true);
  EXPECT_EQ (n.is_internal (), false);
}

TEST(7_NetTerminalsEditing)
{
  db::Circuit c;
  db::DeviceClass dc;
  dc.add_terminal_definition (db::DeviceTerminalDefinition ("A", ""));
  dc.add_terminal_definition (db::DeviceTerminalDefinition ("B", ""));

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

  d1->connect_terminal (0, n1);
  d1->connect_terminal (1, n2);

  EXPECT_EQ (n1->terminal_count (), size_t (1));
  EXPECT_EQ (n1->pin_count (), size_t (0));
  EXPECT_EQ (n1->is_floating (), false);
  EXPECT_EQ (n1->is_passive (), false);
  EXPECT_EQ (n1->is_internal (), false);

  d2->connect_terminal (1, n1);
  d2->connect_terminal (0, n2);

  EXPECT_EQ (n1->terminal_count (), size_t (2));
  EXPECT_EQ (n1->pin_count (), size_t (0));
  EXPECT_EQ (n1->is_floating (), false);
  EXPECT_EQ (n1->is_passive (), false);
  EXPECT_EQ (n1->is_internal (), true);

  EXPECT_EQ (d1->net_for_terminal (0), n1);
  EXPECT_EQ (d1->net_for_terminal (1), n2);
  EXPECT_EQ (d2->net_for_terminal (0), n2);
  EXPECT_EQ (d2->net_for_terminal (1), n1);

  EXPECT_EQ (net2string (*n1), "D1:A,D2:B");
  EXPECT_EQ (net2string (*n2), "D1:B,D2:A");

  d1->connect_terminal (0, n2);
  d1->connect_terminal (1, n1);

  EXPECT_EQ (d1->net_for_terminal (0), n2);
  EXPECT_EQ (d1->net_for_terminal (1), n1);

  EXPECT_EQ (net2string (*n1), "D2:B,D1:B");
  EXPECT_EQ (net2string (*n2), "D2:A,D1:A");

  d1->connect_terminal (0, 0);
  EXPECT_EQ (d1->net_for_terminal (0), 0);

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

  EXPECT_EQ (d2->net_for_terminal (0), n2);
  EXPECT_EQ (d2->net_for_terminal (1), 0);
}

TEST(8_NetSubCircuitsEditing)
{
  db::Circuit c;
  c.set_name ("c");
  c.add_pin ("X");
  c.add_pin ("Y");

  db::Circuit cc1;
  cc1.set_name ("sc1");
  cc1.add_pin ("A");
  cc1.add_pin ("B");

  db::Circuit cc2;
  cc2.set_name ("sc2");
  cc2.add_pin ("A");
  cc2.add_pin ("B");

  db::SubCircuit *sc1 = new db::SubCircuit (&cc1, "sc1");
  c.add_subcircuit (sc1);

  db::SubCircuit *sc2 = new db::SubCircuit (&cc2, "sc2");
  c.add_subcircuit (sc2);

  db::Net *n1 = new db::Net ();
  n1->set_name ("n1");
  c.add_net (n1);

  db::Net *n2 = new db::Net ();
  n2->set_name ("n2");
  c.add_net (n2);

  EXPECT_EQ (n1->pin_count (), size_t (0));
  c.connect_pin (0, n1);

  EXPECT_EQ (n1->terminal_count (), size_t (0));
  EXPECT_EQ (n1->pin_count (), size_t (1));
  EXPECT_EQ (n1->is_floating (), false);
  EXPECT_EQ (n1->is_passive (), true);
  EXPECT_EQ (n1->is_internal (), false);
  EXPECT_NE (n1->pin_count (), size_t (0));

  EXPECT_EQ (c.net_for_pin (0), n1);
  EXPECT_EQ (c.net_for_pin (1), 0);

  sc1->connect_pin (0, n1);
  sc1->connect_pin (1, n2);
  EXPECT_EQ (n2->pin_count (), size_t (0));

  EXPECT_EQ (n1->terminal_count (), size_t (0));
  EXPECT_EQ (n1->pin_count (), size_t (1));
  EXPECT_EQ (n1->subcircuit_pin_count (), size_t (1));
  EXPECT_EQ (n1->is_floating (), false);
  EXPECT_EQ (n1->is_passive (), false);
  EXPECT_EQ (n1->is_internal (), false);

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

  EXPECT_EQ (c.begin_subcircuits ()->name (), "sc2");
  EXPECT_EQ (++c.begin_subcircuits () == c.end_subcircuits (), true);

  EXPECT_EQ (net2string (*n1), "sc2:B");
  EXPECT_EQ (net2string (*n2), "sc2:A");

  c.connect_pin (1, n1);
  EXPECT_EQ (net2string (*n1), "+Y,sc2:B");
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

TEST(9_NetTerminalRefBasics)
{
  db::Device d1, d2;

  EXPECT_EQ (db::NetTerminalRef (&d1, 0) == db::NetTerminalRef (&d1, 0), true);
  EXPECT_EQ (db::NetTerminalRef (&d1, 0) == db::NetTerminalRef (&d1, 1), false);
  EXPECT_EQ (db::NetTerminalRef (&d1, 0) == db::NetTerminalRef (&d2, 0), false);

  EXPECT_EQ (db::NetTerminalRef (&d1, 0) < db::NetTerminalRef (&d1, 0), false);
  EXPECT_EQ (db::NetTerminalRef (&d1, 0) < db::NetTerminalRef (&d1, 1), true);
  EXPECT_EQ (db::NetTerminalRef (&d1, 1) < db::NetTerminalRef (&d1, 0), false);
  EXPECT_NE ((db::NetTerminalRef (&d1, 0) < db::NetTerminalRef (&d2, 0)), (db::NetTerminalRef (&d2, 0) < db::NetTerminalRef (&d1, 0)));
}

TEST(10_NetPinRefBasics)
{
  db::SubCircuit d1, d2;

  EXPECT_EQ (db::NetSubcircuitPinRef (&d1, 0) == db::NetSubcircuitPinRef (&d1, 0), true);
  EXPECT_EQ (db::NetSubcircuitPinRef (&d1, 0) == db::NetSubcircuitPinRef (&d1, 1), false);
  EXPECT_EQ (db::NetSubcircuitPinRef (&d1, 0) == db::NetSubcircuitPinRef (&d2, 0), false);

  EXPECT_EQ (db::NetSubcircuitPinRef (&d1, 0) < db::NetSubcircuitPinRef (&d1, 0), false);
  EXPECT_EQ (db::NetSubcircuitPinRef (&d1, 0) < db::NetSubcircuitPinRef (&d1, 1), true);
  EXPECT_EQ (db::NetSubcircuitPinRef (&d1, 1) < db::NetSubcircuitPinRef (&d1, 0), false);
  EXPECT_NE ((db::NetSubcircuitPinRef (&d1, 0) < db::NetSubcircuitPinRef (&d2, 0)), (db::NetSubcircuitPinRef (&d2, 0) < db::NetSubcircuitPinRef (&d1, 0)));
}

TEST(11_NetlistCircuitRefs)
{
  std::unique_ptr<db::Netlist> nl (new db::Netlist ());

  db::Circuit *c1 = new db::Circuit ();
  c1->set_name ("c1");
  nl->add_circuit (c1);

  db::Circuit *c2 = new db::Circuit ();
  c2->set_name ("c2");
  nl->add_circuit (c2);

  db::SubCircuit *sc1 = new db::SubCircuit (c2);
  sc1->set_name ("sc1");
  EXPECT_EQ (refs2string (c2), "sc1");
  c1->add_subcircuit (sc1);

  db::SubCircuit *sc2 = new db::SubCircuit (c2);
  sc2->set_name ("sc2");
  EXPECT_EQ (refs2string (c2), "sc1,sc2");
  c1->add_subcircuit (sc2);

  *sc2 = db::SubCircuit ();
  EXPECT_EQ (refs2string (c2), "sc1");

  db::SubCircuit sc3 (c2);
  sc3.set_name ("sc3");
  *sc2 = sc3;
  sc2->set_name ("sc2");
  EXPECT_EQ (refs2string (c2), "sc1,sc3,sc2");

  sc3 = db::SubCircuit ();
  EXPECT_EQ (refs2string (c2), "sc1,sc2");
}

TEST(12_NetlistTopology)
{
  std::unique_ptr<db::Netlist> nl (new db::Netlist ());
  EXPECT_EQ (nl->top_circuit_count (), size_t (0));
  EXPECT_EQ (tcs2string (nl.get ()), "");
  EXPECT_EQ (tcs2string_nc (nl.get ()), "");
  EXPECT_EQ (tc2string (nl.get ()), "(nil)");
  EXPECT_EQ (tc2string_nc (nl.get ()), "(nil)");

  db::Circuit *c1 = new db::Circuit ();
  c1->set_name ("c1");
  nl->add_circuit (c1);
  EXPECT_EQ (nl->top_circuit_count (), size_t (1));
  EXPECT_EQ (td2string (nl.get ()), "c1");
  EXPECT_EQ (td2string_nc (nl.get ()), "c1");
  EXPECT_EQ (tcs2string (nl.get ()), "c1");
  EXPECT_EQ (tcs2string_nc (nl.get ()), "c1");
  EXPECT_EQ (tc2string (nl.get ()), "c1");
  EXPECT_EQ (tc2string_nc (nl.get ()), "c1");
  EXPECT_EQ (bu2string (nl.get ()), "c1");
  EXPECT_EQ (bu2string_nc (nl.get ()), "c1");

  db::Circuit *c2 = new db::Circuit ();
  c2->set_name ("c2");
  nl->add_circuit (c2);
  EXPECT_EQ (nl->top_circuit_count (), size_t (2));
  EXPECT_EQ (td2string (nl.get ()), "c2,c1");
  EXPECT_EQ (td2string_nc (nl.get ()), "c2,c1");
  EXPECT_EQ (tcs2string (nl.get ()), "c2,c1");
  EXPECT_EQ (tcs2string_nc (nl.get ()), "c2,c1");
  try {
    tc2string (nl.get ());
    EXPECT_EQ (true, false);
  } catch (...) {
  }
  try {
    tc2string_nc (nl.get ());
    EXPECT_EQ (true, false);
  } catch (...) {
  }
  EXPECT_EQ (bu2string (nl.get ()), "c1,c2");
  EXPECT_EQ (bu2string_nc (nl.get ()), "c1,c2");

  std::unique_ptr<db::NetlistLocker> locker (new db::NetlistLocker (nl.get ()));

  db::Circuit *c3 = new db::Circuit ();
  c3->set_name ("c3");
  nl->add_circuit (c3);

  //  because we locked, it did not get updated:
  EXPECT_EQ (nl->top_circuit_count (), size_t (2));
  EXPECT_EQ (td2string (nl.get ()), "c2,c1");
  EXPECT_EQ (bu2string (nl.get ()), "c1,c2");
  locker.reset (0);

  //  after removing the lock, it's updated
  EXPECT_EQ (nl->top_circuit_count (), size_t (3));
  EXPECT_EQ (td2string (nl.get ()), "c3,c2,c1");
  EXPECT_EQ (bu2string (nl.get ()), "c1,c2,c3");

  db::SubCircuit *sc1 = new db::SubCircuit (c2);
  sc1->set_name ("sc1");
  c1->add_subcircuit (sc1);
  EXPECT_EQ (children2string (c1), "c2");
  EXPECT_EQ (parents2string (c2), "c1");
  EXPECT_EQ (nl->top_circuit_count (), size_t (2));
  EXPECT_EQ (td2string (nl.get ()), "c3,c1,c2");
  EXPECT_EQ (bu2string (nl.get ()), "c2,c1,c3");

  db::SubCircuit *sc2 = new db::SubCircuit (c2);
  sc2->set_name ("sc2");
  c1->add_subcircuit (sc2);
  EXPECT_EQ (children2string (c1), "c2");
  EXPECT_EQ (parents2string (c2), "c1");
  EXPECT_EQ (nl->top_circuit_count (), size_t (2));
  EXPECT_EQ (td2string (nl.get ()), "c3,c1,c2");
  EXPECT_EQ (bu2string (nl.get ()), "c2,c1,c3");

  db::SubCircuit *sc3 = new db::SubCircuit (c3);
  sc3->set_name ("sc3");
  c1->add_subcircuit (sc3);
  EXPECT_EQ (children2string (c1), "c2,c3");
  EXPECT_EQ (children2string (c2), "");
  EXPECT_EQ (children2string (c3), "");
  EXPECT_EQ (parents2string (c2), "c1");
  EXPECT_EQ (parents2string (c3), "c1");
  EXPECT_EQ (nl->top_circuit_count (), size_t (1));
  EXPECT_EQ (td2string (nl.get ()), "c1,c3,c2");
  EXPECT_EQ (bu2string (nl.get ()), "c2,c3,c1");

  db::SubCircuit *sc4 = new db::SubCircuit (*sc2);
  sc4->set_name ("sc4");
  c3->add_subcircuit (sc4);

  EXPECT_EQ (children2string (c1), "c2,c3");
  EXPECT_EQ (children2string (c2), "");
  EXPECT_EQ (children2string (c3), "c2");
  EXPECT_EQ (parents2string (c2), "c1,c3");
  EXPECT_EQ (parents2string (c3), "c1");
  EXPECT_EQ (nl->top_circuit_count (), size_t (1));
  EXPECT_EQ (td2string (nl.get ()), "c1,c3,c2");
  EXPECT_EQ (bu2string (nl.get ()), "c2,c3,c1");
}

TEST(13_DeviceAbstract)
{
  db::Netlist nl;

  db::DeviceAbstract *dm = new db::DeviceAbstract (0, "name");
  nl.add_device_abstract (dm);
  EXPECT_EQ (dm->netlist () == &nl, true);

  EXPECT_EQ (dm->device_class () == 0, true);
  EXPECT_EQ (dm->name (), "name");
  EXPECT_EQ (nl.device_abstract_by_name ("name") == dm, true);
  EXPECT_EQ (nl.device_abstract_by_name ("name2") == 0, true);
  EXPECT_EQ (nl.device_abstract_by_name ("does_not_exist") == 0, true);
  dm->set_name ("name2");
  EXPECT_EQ (dm->name (), "name2");
  EXPECT_EQ (nl.device_abstract_by_name ("name") == 0, true);
  EXPECT_EQ (nl.device_abstract_by_name ("name2") == dm, true);
  EXPECT_EQ (nl.device_abstract_by_name ("does_not_exist") == 0, true);

  dm->set_cluster_id_for_terminal (1, 17);
  dm->set_cluster_id_for_terminal (0, 42);
  EXPECT_EQ (dm->cluster_id_for_terminal (0), size_t (42));
  EXPECT_EQ (dm->cluster_id_for_terminal (1), size_t (17));

  dm->set_cell_index (5);
  EXPECT_EQ (nl.device_abstract_by_cell_index (5) == dm, true);
  EXPECT_EQ (nl.device_abstract_by_cell_index (17) == 0, true);
  EXPECT_EQ (dm->cell_index (), db::cell_index_type (5));

  EXPECT_EQ (nl.begin_device_abstracts () == nl.end_device_abstracts (), false);
  EXPECT_EQ (nl.begin_device_abstracts ()->name (), "name2");

  nl.remove_device_abstract (dm);

  EXPECT_EQ (nl.begin_device_abstracts () == nl.end_device_abstracts (), true);
}

TEST(20_FlattenSubCircuit)
{
  db::Netlist nl;

  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  nl.from_string (
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
  );

  db::Netlist nl2;

  nl2 = nl;
  db::Circuit *inv2 = nl2.circuit_by_name ("INV2");
  inv2->flatten_subcircuit (inv2->subcircuit_by_name ("SC1"));

  EXPECT_EQ (nl2.to_string (),
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$5,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.flatten_circuit (nl2.circuit_by_name ("PTRANS"));
  nl2.flatten_circuit (nl2.circuit_by_name ("NTRANS"));

  EXPECT_EQ (nl2.to_string (),
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$5,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=$4,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  db::Netlist nl3;

  nl3 = nl;
  nl3.flatten ();

  EXPECT_EQ (nl3.to_string (),
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$5,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=$4,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(21_FlattenSubCircuit2)
{
  db::Netlist nl;

  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  nl.from_string (
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 INV2_SC1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
  );

  db::Netlist nl2;

  nl2 = nl;
  db::Circuit *inv2 = nl2.circuit_by_name ("RINGO");
  inv2->flatten_subcircuit (inv2->subcircuit_by_name ("INV2_SC1"));

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);\n"
    "  subcircuit PTRANS INV2_SC1.SC1 ($1=VDD,$2=FB,$3=$I8);\n"
    "  subcircuit NTRANS INV2_SC1.SC2 ($1=VSS,$2=FB,$3=$I8);\n"
    "  subcircuit PTRANS INV2_SC1.SC3 ($1=VDD,$2=OSC,$3=FB);\n"
    "  subcircuit NTRANS INV2_SC1.SC4 ($1=VSS,$2=OSC,$3=FB);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  inv2->flatten_subcircuit (inv2->subcircuit_by_name ("INV2_SC2"));

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit PTRANS INV2_SC1.SC1 ($1=VDD,$2=FB,$3=$I8);\n"
    "  subcircuit NTRANS INV2_SC1.SC2 ($1=VSS,$2=FB,$3=$I8);\n"
    "  subcircuit PTRANS INV2_SC1.SC3 ($1=VDD,$2=OSC,$3=FB);\n"
    "  subcircuit NTRANS INV2_SC1.SC4 ($1=VSS,$2=OSC,$3=FB);\n"
    "  subcircuit PTRANS INV2_SC2.SC1 ($1=VDD,$2=(null),$3=FB);\n"
    "  subcircuit NTRANS INV2_SC2.SC2 ($1=VSS,$2=(null),$3=FB);\n"
    "  subcircuit PTRANS INV2_SC2.SC3 ($1=VDD,$2=$I8,$3=(null));\n"
    "  subcircuit NTRANS INV2_SC2.SC4 ($1=VSS,$2=$I8,$3=(null));\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2 = nl;
  nl2.flatten_circuit (nl2.circuit_by_name ("INV2"));

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit PTRANS INV2_SC1.SC1 ($1=VDD,$2=FB,$3=$I8);\n"
    "  subcircuit NTRANS INV2_SC1.SC2 ($1=VSS,$2=FB,$3=$I8);\n"
    "  subcircuit PTRANS INV2_SC1.SC3 ($1=VDD,$2=OSC,$3=FB);\n"
    "  subcircuit NTRANS INV2_SC1.SC4 ($1=VSS,$2=OSC,$3=FB);\n"
    "  subcircuit PTRANS INV2_SC2.SC1 ($1=VDD,$2=(null),$3=FB);\n"
    "  subcircuit NTRANS INV2_SC2.SC2 ($1=VSS,$2=(null),$3=FB);\n"
    "  subcircuit PTRANS INV2_SC2.SC3 ($1=VDD,$2=$I8,$3=(null));\n"
    "  subcircuit NTRANS INV2_SC2.SC4 ($1=VSS,$2=$I8,$3=(null));\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );


}

TEST(22_FlattenSubCircuitPinsJoinNets)
{
  db::Netlist nl;

  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  nl.from_string (
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 INV2_SC1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$1);\n"
    "  device PMOS $1 (S=$1,D=$2,G=$1) (L=0.25,W=0.95);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$2);\n"
    "  device NMOS $1 (S=$1,D=$2,G=$2) (L=0.25,W=0.95);\n"
    "end;\n"
  );

  db::Netlist nl2;

  nl2 = nl;
  nl2.flatten_circuit (nl2.circuit_by_name ("PTRANS"));

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD='FB,VDD');\n"
    "  subcircuit INV2 INV2_SC1 (OUT=OSC,$2=VSS,IN='FB,VDD');\n"
    "  subcircuit INV2 INV2_SC2 (OUT='FB,VDD',$2=VSS,IN='FB,VDD');\n"
    "end;\n"
    "circuit INV2 (OUT=OUT,$2=$4,IN=IN);\n"
    "  device PMOS $1 (S=IN,G=IN,D=IN) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=IN,G=IN,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=IN,$3=IN);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=IN);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$2);\n"
    "  device NMOS $1 (S=$1,G=$2,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.flatten_circuit (nl2.circuit_by_name ("NTRANS"));

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,'OSC,VDD'='FB,OSC,VDD',VSS=VSS);\n"
    "  subcircuit INV2 INV2_SC1 ('IN,OUT'='FB,OSC,VDD',$2=VSS);\n"
    "  subcircuit INV2 INV2_SC2 ('IN,OUT'='FB,OSC,VDD',$2=VSS);\n"
    "end;\n"
    "circuit INV2 ('IN,OUT'='IN,OUT',$2=$4);\n"
    "  device PMOS $1 (S='IN,OUT',G='IN,OUT',D='IN,OUT') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S='IN,OUT',G='IN,OUT',D='IN,OUT') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=$4,G='IN,OUT',D='IN,OUT') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$4,G='IN,OUT',D='IN,OUT') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.flatten_circuit (nl2.circuit_by_name ("INV2"));

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,'OSC,VDD'='FB,OSC,VDD',VSS=VSS);\n"
    "  device PMOS $1 (S='FB,OSC,VDD',G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S='FB,OSC,VDD',G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=VSS,G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=VSS,G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $5 (S='FB,OSC,VDD',G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $6 (S='FB,OSC,VDD',G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $7 (S=VSS,G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $8 (S=VSS,G='FB,OSC,VDD',D='FB,OSC,VDD') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(23_BlankCircuit)
{
  db::Netlist nl;

  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  nl.from_string (
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 INV2_SC1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
  );

  db::Netlist nl2;

  nl2 = nl;
  db::Circuit *circuit = nl2.circuit_by_name ("INV2");
  nl2.purge_circuit (circuit);

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit (null);\n"
    "  subcircuit (null);\n"
    "end;\n"
  );

  nl2 = nl;
  circuit = nl2.circuit_by_name ("INV2");
  EXPECT_EQ (circuit->dont_purge (), false);
  circuit->blank ();
  EXPECT_EQ (circuit->dont_purge (), true);

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 INV2_SC1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=(null),$2=(null),OUT=(null),$4=(null),$5=(null));\n"
    "end;\n"
  );

  //  purge won't delete INV2
  nl2.purge ();

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 INV2_SC1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=(null),$2=(null),OUT=(null),$4=(null),$5=(null));\n"
    "end;\n"
  );

  circuit->set_dont_purge (false);

  //  now it will ...
  nl2.purge ();

  EXPECT_EQ (nl2.to_string (),
    ""
  );

  nl2 = nl;
  circuit = nl2.circuit_by_name ("RINGO");
  nl2.purge_circuit (circuit);

  EXPECT_EQ (nl2.to_string (),
    ""
  );

  nl2 = nl;
  circuit = nl2.circuit_by_name ("RINGO");
  circuit->blank ();

  EXPECT_EQ (nl2.to_string (),
    "circuit RINGO (IN=(null),OSC=(null),VSS=(null),VDD=(null));\n"
    "end;\n"
  );
}

TEST(24_NetlistObject)
{
  db::NetlistObject nlo;
  nlo.set_property (1, "hello");
  EXPECT_EQ (nlo.property ("key").to_string (), "nil");
  EXPECT_EQ (nlo.property (1).to_string (), "hello");
  nlo.set_property ("key", 42);
  EXPECT_EQ (nlo.property ("key").to_string (), "42");
  nlo.set_property ("key", tl::Variant ());
  EXPECT_EQ (nlo.property ("key").to_string (), "nil");

  db::Net net ("net_name");
  net.set_property (1, "hello");
  EXPECT_EQ (net.property ("key").to_string (), "nil");
  EXPECT_EQ (net.property (1).to_string (), "hello");

  db::Net net2 (net);
  EXPECT_EQ (net.property ("key").to_string (), "nil");
  EXPECT_EQ (net.property (1).to_string (), "hello");

  db::Net net3;
  EXPECT_EQ (net3.property ("key").to_string (), "nil");
  EXPECT_EQ (net3.property (1).to_string (), "nil");
  net3 = net2;
  EXPECT_EQ (net3.property (1).to_string (), "hello");

  db::SubCircuit sc;
  sc.set_property (1, "hello");
  EXPECT_EQ (sc.property ("key").to_string (), "nil");
  EXPECT_EQ (sc.property (1).to_string (), "hello");

  db::SubCircuit sc2 (sc);
  EXPECT_EQ (sc.property ("key").to_string (), "nil");
  EXPECT_EQ (sc.property (1).to_string (), "hello");

  db::SubCircuit sc3;
  EXPECT_EQ (sc3.property ("key").to_string (), "nil");
  EXPECT_EQ (sc3.property (1).to_string (), "nil");
  sc3 = sc2;
  EXPECT_EQ (sc3.property (1).to_string (), "hello");

  db::Device dev;
  dev.set_property (1, "hello");
  EXPECT_EQ (dev.property ("key").to_string (), "nil");
  EXPECT_EQ (dev.property (1).to_string (), "hello");

  db::Device dev2 (dev);
  EXPECT_EQ (dev.property ("key").to_string (), "nil");
  EXPECT_EQ (dev.property (1).to_string (), "hello");

  db::Device dev3;
  EXPECT_EQ (dev3.property ("key").to_string (), "nil");
  EXPECT_EQ (dev3.property (1).to_string (), "nil");
  dev3 = dev2;
  EXPECT_EQ (dev3.property (1).to_string (), "hello");

  db::Circuit circuit;
  circuit.set_property (1, "hello");
  EXPECT_EQ (circuit.property ("key").to_string (), "nil");
  EXPECT_EQ (circuit.property (1).to_string (), "hello");

  db::Circuit circuit2 (circuit);
  EXPECT_EQ (circuit.property ("key").to_string (), "nil");
  EXPECT_EQ (circuit.property (1).to_string (), "hello");

  db::Circuit circuit3;
  EXPECT_EQ (circuit3.property ("key").to_string (), "nil");
  EXPECT_EQ (circuit3.property (1).to_string (), "nil");
  circuit3 = circuit2;
  EXPECT_EQ (circuit3.property (1).to_string (), "hello");

  db::Pin pin ("pin_name");
  pin.set_property (1, "hello");
  EXPECT_EQ (pin.property ("key").to_string (), "nil");
  EXPECT_EQ (pin.property (1).to_string (), "hello");

  db::Pin pin2 (pin);
  EXPECT_EQ (pin.property ("key").to_string (), "nil");
  EXPECT_EQ (pin.property (1).to_string (), "hello");

  db::Pin pin3;
  EXPECT_EQ (pin3.property ("key").to_string (), "nil");
  EXPECT_EQ (pin3.property (1).to_string (), "nil");
  pin3 = pin2;
  EXPECT_EQ (pin3.property (1).to_string (), "hello");
}

TEST(25_JoinNets)
{
  db::Netlist nl;
  db::Circuit *c;

  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  nl.from_string (
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);\n"
    "end;\n"
  );

  c = nl.circuit_by_name ("INV2");
  c->join_nets (c->net_by_name ("IN"), c->net_by_name ("OUT"));

  EXPECT_EQ (nl.to_string (),
    "circuit INV2 ('IN,OUT'='IN,OUT',$2=$2,$3=$4,$4=$5);\n"
    "  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3='IN,OUT');\n"
    "  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3='IN,OUT');\n"
    "  subcircuit PTRANS SC3 ($1=$5,$2='IN,OUT',$3=$2);\n"
    "  subcircuit NTRANS SC4 ($1=$4,$2='IN,OUT',$3=$2);\n"
    "end;\n"
    "circuit PTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device PMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit NTRANS ($1=$1,$2=$2,$3=$3);\n"
    "  device NMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(26_JoinNets)
{
  db::Netlist nl;
  db::Circuit *c;

  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  nl.from_string (
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$5,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=$4,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  c = nl.circuit_by_name ("INV2");
  c->join_nets (c->net_by_name ("IN"), c->net_by_name ("OUT"));

  EXPECT_EQ (nl.to_string (),
    "circuit INV2 ('IN,OUT'='IN,OUT',$2=$2,$3=$4,$4=$5);\n"
    "  device PMOS $1 (S=$5,G='IN,OUT',D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device PMOS $2 (S=$5,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $3 (S=$4,G='IN,OUT',D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NMOS $4 (S=$4,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

//  Issue #1832 - small caps are not combined properly
TEST(27_CombineSmallC)
{
  db::Netlist nl;

  db::Circuit *circuit = new db::Circuit ();
  circuit->set_name ("TOP");
  nl.add_circuit (circuit);

  db::DeviceClass *device = new db::DeviceClassCapacitor ();
  device->set_name ("model_name");
  nl.add_device_class (device);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);

  auto p1 = circuit->add_pin ("p1");
  auto p2 = circuit->add_pin ("p2");

  circuit->connect_pin (p1.id (), n1);
  circuit->connect_pin (p2.id (), n3);

  db::Device *c1 = new db::Device (device, "c1");
  circuit->add_device (c1);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 1e-15);

  db::Device *c2 = new db::Device (device, "c2");
  circuit->add_device (c2);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 2e-15);

  db::Device *c3 = new db::Device (device, "c3");
  circuit->add_device (c3);
  c3->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 3e-15);

  c1->connect_terminal (db::DeviceClassCapacitor::terminal_id_A, n1);
  c1->connect_terminal (db::DeviceClassCapacitor::terminal_id_B, n2);

  c2->connect_terminal (db::DeviceClassCapacitor::terminal_id_A, n2);
  c2->connect_terminal (db::DeviceClassCapacitor::terminal_id_B, n3);

  c3->connect_terminal (db::DeviceClassCapacitor::terminal_id_A, n1);
  c3->connect_terminal (db::DeviceClassCapacitor::terminal_id_B, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n3);\n"
    "  device model_name c1 (A=n1,B=n2) (C=1e-15,A=0,P=0);\n"
    "  device model_name c2 (A=n2,B=n3) (C=2e-15,A=0,P=0);\n"
    "  device model_name c3 (A=n1,B=n3) (C=3e-15,A=0,P=0);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n3);\n"
    "  device model_name c1 (A=n1,B=n3) (C=3.66666666667e-15,A=0,P=0);\n"
    "end;\n"
  );
}

TEST(27_CombineSmallR)
{
  db::Netlist nl;

  db::Circuit *circuit = new db::Circuit ();
  circuit->set_name ("TOP");
  nl.add_circuit (circuit);

  db::DeviceClass *device = new db::DeviceClassResistor ();
  device->set_name ("model_name");
  nl.add_device_class (device);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);

  auto p1 = circuit->add_pin ("p1");
  auto p2 = circuit->add_pin ("p2");

  circuit->connect_pin (p1.id (), n1);
  circuit->connect_pin (p2.id (), n3);

  db::Device *c1 = new db::Device (device, "c1");
  circuit->add_device (c1);
  c1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1e-15);

  db::Device *c2 = new db::Device (device, "c2");
  circuit->add_device (c2);
  c2->set_parameter_value (db::DeviceClassResistor::param_id_R, 2e-15);

  db::Device *c3 = new db::Device (device, "c3");
  circuit->add_device (c3);
  c3->set_parameter_value (db::DeviceClassResistor::param_id_R, 3e-15);

  c1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);
  c1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);

  c2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);
  c2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);

  c3->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);
  c3->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n3);\n"
    "  device model_name c1 (A=n1,B=n2) (R=1e-15,L=0,W=0,A=0,P=0);\n"
    "  device model_name c2 (A=n2,B=n3) (R=2e-15,L=0,W=0,A=0,P=0);\n"
    "  device model_name c3 (A=n1,B=n3) (R=3e-15,L=0,W=0,A=0,P=0);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n3);\n"
    "  device model_name c1 (A=n1,B=n3) (R=1.5e-15,L=0,W=0,A=0,P=0);\n"
    "end;\n"
  );
}

TEST(27_CombineSmallL)
{
  db::Netlist nl;

  db::Circuit *circuit = new db::Circuit ();
  circuit->set_name ("TOP");
  nl.add_circuit (circuit);

  db::DeviceClass *device = new db::DeviceClassInductor ();
  device->set_name ("model_name");
  nl.add_device_class (device);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);

  auto p1 = circuit->add_pin ("p1");
  auto p2 = circuit->add_pin ("p2");

  circuit->connect_pin (p1.id (), n1);
  circuit->connect_pin (p2.id (), n3);

  db::Device *c1 = new db::Device (device, "c1");
  circuit->add_device (c1);
  c1->set_parameter_value (db::DeviceClassInductor::param_id_L, 1e-15);

  db::Device *c2 = new db::Device (device, "c2");
  circuit->add_device (c2);
  c2->set_parameter_value (db::DeviceClassInductor::param_id_L, 2e-15);

  db::Device *c3 = new db::Device (device, "c3");
  circuit->add_device (c3);
  c3->set_parameter_value (db::DeviceClassInductor::param_id_L, 3e-15);

  c1->connect_terminal (db::DeviceClassInductor::terminal_id_A, n1);
  c1->connect_terminal (db::DeviceClassInductor::terminal_id_B, n2);

  c2->connect_terminal (db::DeviceClassInductor::terminal_id_A, n2);
  c2->connect_terminal (db::DeviceClassInductor::terminal_id_B, n3);

  c3->connect_terminal (db::DeviceClassInductor::terminal_id_A, n1);
  c3->connect_terminal (db::DeviceClassInductor::terminal_id_B, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n3);\n"
    "  device model_name c1 (A=n1,B=n2) (L=1e-15);\n"
    "  device model_name c2 (A=n2,B=n3) (L=2e-15);\n"
    "  device model_name c3 (A=n1,B=n3) (L=3e-15);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n3);\n"
    "  device model_name c1 (A=n1,B=n3) (L=1.5e-15);\n"
    "end;\n"
  );
}

TEST(28_EliminateShortedDevices)
{
  db::Netlist nl;

  db::Circuit *circuit = new db::Circuit ();
  circuit->set_name ("TOP");
  nl.add_circuit (circuit);

  db::DeviceClass *device = new db::DeviceClassCapacitor ();
  device->set_name ("model_name");
  nl.add_device_class (device);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);

  auto p1 = circuit->add_pin ("p1");
  auto p2 = circuit->add_pin ("p2");

  circuit->connect_pin (p1.id (), n1);
  circuit->connect_pin (p2.id (), n2);

  db::Device *c1 = new db::Device (device, "c1");
  circuit->add_device (c1);
  c1->set_parameter_value (db::DeviceClassInductor::param_id_L, 1e-15);

  db::Device *c2 = new db::Device (device, "c2");
  circuit->add_device (c2);
  c2->set_parameter_value (db::DeviceClassInductor::param_id_L, 2e-15);

  c1->connect_terminal (db::DeviceClassInductor::terminal_id_A, n1);
  c1->connect_terminal (db::DeviceClassInductor::terminal_id_B, n1);

  c2->connect_terminal (db::DeviceClassInductor::terminal_id_A, n1);
  c2->connect_terminal (db::DeviceClassInductor::terminal_id_B, n2);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n2);\n"
    "  device model_name c1 (A=n1,B=n1) (C=1e-15,A=0,P=0);\n"
    "  device model_name c2 (A=n1,B=n2) (C=2e-15,A=0,P=0);\n"
    "end;\n"
  );

  nl.purge_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit TOP (p1=n1,p2=n2);\n"
    "  device model_name c2 (A=n1,B=n2) (C=2e-15,A=0,P=0);\n"
    "end;\n"
  );
}
