
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "dbLayoutConnectivity.h"
#include "dbLayout.h"
#include "dbPropertiesRepository.h"
#include "tlUnitTest.h"

namespace
{

TEST(1_LayoutPinBasic)
{
  db::TempPropertiesRepository tmp_rep;

  db::LayoutPin pin;

  EXPECT_EQ (pin.name (), "");
  EXPECT_EQ (pin.must_connect (), false);

  pin = db::LayoutPin ("XYZ", true);

  EXPECT_EQ (pin.name (), "XYZ");
  EXPECT_EQ (pin.must_connect (), true);

  pin.set_must_connect (false);
  EXPECT_EQ (pin.must_connect (), false);

  pin.set_name ("ABC");
  EXPECT_EQ (pin.name (), "ABC");

  pin.set_name ("ABC$1234");
  EXPECT_EQ (pin.name (), "ABC$1234");
  EXPECT_EQ (pin.basic_name (), "ABC");

  //  not a valid disambigurator
  pin.set_name ("ABC$1234n");
  EXPECT_EQ (pin.name (), "ABC$1234n");
  EXPECT_EQ (pin.basic_name (), "ABC$1234n");

  //  only last suffix acts as disambiguator
  pin.set_name ("ABC$1234$123");
  EXPECT_EQ (pin.name (), "ABC$1234$123");
  EXPECT_EQ (pin.basic_name (), "ABC$1234");
}

TEST(2_LayoutPinVariantCompatibility)
{
  db::TempPropertiesRepository tmp_rep;

  db::LayoutPin pin ("ABC", true);

  std::string s = tl::Variant (pin).to_parsable_string ();
  EXPECT_EQ (s, "[layoutpin:{'must_connect'=>true,'name'=>'ABC'}]");

  tl::Variant v;
  tl::Extractor ex (s.c_str ());

  ex.read (v);

  EXPECT_EQ (v.is_user (), true);
  db::LayoutPin pin2 = v.to_user<db::LayoutPin> ();

  EXPECT_EQ (pin2.name (), "ABC");
  EXPECT_EQ (pin2.must_connect (), true);

  s = tl::Variant (db::LayoutPin ()).to_parsable_string ();
  EXPECT_EQ (s, "[layoutpin:{'name'=>''}]");

  ex = tl::Extractor (s.c_str ());
  ex.read (v);
  EXPECT_EQ (v.is_user (), true);
  pin2 = v.to_user<db::LayoutPin> ();

  EXPECT_EQ (pin2.name (), "");
  EXPECT_EQ (pin2.must_connect (), false);
}

TEST(10_LayoutInstanceConnectionsBasic)
{
  db::TempPropertiesRepository tmp_rep;

  db::LayoutInstanceConnections conn;

  conn.add_connection ("A", "net");
  EXPECT_EQ (conn.net_for_pin ("A"), "net");
  EXPECT_EQ (conn.net_id_for_pin (db::property_names_id ("A")), db::property_names_id ("net"));
  EXPECT_EQ (conn.net_id_for_pin ("A"), db::property_names_id ("net"));
  EXPECT_EQ (conn.has_pin ("A"), true);
  EXPECT_EQ (conn.has_pin (db::property_names_id ("A")), true);
  EXPECT_EQ (conn.net_for_pin ("B"), 0);
  EXPECT_EQ (conn.net_id_for_pin (db::property_names_id ("B")), db::property_names_id_type (0));
  EXPECT_EQ (conn.net_id_for_pin ("B"), db::property_names_id_type (0));
  EXPECT_EQ (conn.has_pin ("B"), false);
  EXPECT_EQ (conn.has_pin (db::property_names_id ("B")), false);

  conn.add_connection ("B", "net2");
  EXPECT_EQ (conn.net_for_pin ("A"), "net");
  EXPECT_EQ (conn.net_id_for_pin (db::property_names_id ("A")), db::property_names_id ("net"));
  EXPECT_EQ (conn.net_id_for_pin ("A"), db::property_names_id ("net"));
  EXPECT_EQ (conn.has_pin ("A"), true);
  EXPECT_EQ (conn.has_pin (db::property_names_id ("A")), true);
  EXPECT_EQ (conn.net_for_pin ("B"), "net2");
  EXPECT_EQ (conn.net_id_for_pin (db::property_names_id ("B")), db::property_names_id ("net2"));
  EXPECT_EQ (conn.net_id_for_pin ("B"), db::property_names_id ("net2"));
  EXPECT_EQ (conn.has_pin ("B"), true);
  EXPECT_EQ (conn.has_pin (db::property_names_id ("B")), true);

  conn.clear ();
  EXPECT_EQ (conn.has_pin ("A"), false);
  EXPECT_EQ (conn.has_pin ("B"), false);
}

TEST(11_LayoutInstanceConnectionsVariantCompatibility)
{
  db::TempPropertiesRepository tmp_rep;

  db::LayoutInstanceConnections conn;
  conn.add_connection ("A", "net");
  conn.add_connection ("B", "net2");

  std::string s = tl::Variant (conn).to_parsable_string ();
  EXPECT_EQ (s, "[layoutinstanceconnections:{'connections'=>{'A'=>'net','B'=>'net2'}}]");

  tl::Variant v;
  tl::Extractor ex (s.c_str ());

  ex.read (v);
  EXPECT_EQ (v.is_user (), true);

  db::LayoutInstanceConnections conn2;
  conn2 = v.to_user<db::LayoutInstanceConnections> ();

  EXPECT_EQ (tl::Variant (conn2).to_parsable_string (), s);
}

static db::properties_id_type pin_props (const std::string &net_name, const std::string &pin_name, bool must_connect)
{
  db::LayoutPin pin_info;
  pin_info.set_name (pin_name);
  pin_info.set_must_connect (must_connect);

  db::PropertiesSet ps;
  ps.insert (db::pin_property_name_id, pin_info);
  if (! net_name.empty ()) {
    ps.insert (db::shape_net_property_name_id, net_name);
  }

  return db::properties_id (ps);
}

static db::properties_id_type shape_props (const std::string &net_name)
{
  db::PropertiesSet ps;
  ps.insert (db::shape_net_property_name_id, net_name);
  return db::properties_id (ps);
}

static db::properties_id_type inst_props (const db::LayoutInstanceConnections &inst_conn)
{
  db::PropertiesSet ps;
  ps.insert (db::instance_connections_property_name_id, inst_conn);
  return db::properties_id (ps);
}

static std::string index_nets2string (const db::Layout &ly, const db::LayoutConnectivityIndex &index)
{
  std::string res;
  for (auto n = index.begin_nets (); n != index.end_nets (); ++n) {
    res += std::string (db::property_name (n->first).to_string ()) + ":\n";
    if (! n->second.pins.empty ()) {
      res += "  Pin shapes:\n";
      for (auto i = n->second.pins.begin (); i != n->second.pins.end (); ++i) {
        res += "    [" + ly.get_properties (i->layer).to_string () + "] " + i->shape.bbox ().to_string () + " '" + db::property_name (i->pin_name).to_string () + "'\n";
      }
    }
    if (! n->second.shapes.empty ()) {
      res += "  Net shapes:\n";
      for (auto i = n->second.shapes.begin (); i != n->second.shapes.end (); ++i) {
        res += "    [" + ly.get_properties (i->layer).to_string () + "] " + i->shape.bbox ().to_string () + "\n";
      }
    }
    if (! n->second.instance_pins.empty ()) {
      res += "  Instance pins:\n";
      for (auto i = n->second.instance_pins.begin (); i != n->second.instance_pins.end (); ++i) {
        res += "    [" + std::string (ly.cell_name (i->instance.cell_index ())) + ": " + i->instance.cell_inst ().front ().to_string () + "] '" + db::property_name (i->pin_name).to_string () + "'\n";
      }
    }
  }

  return res;
}

static std::string index_pins2string (const db::Layout &ly, const db::LayoutConnectivityIndex &index)
{
  std::string res;
  for (auto p = index.begin_pins (); p != index.end_pins (); ++p) {
    res += std::string (db::property_name (p->first).to_string ()) + ":\n";
    res += std::string ("  must_connect: ") + (p->second.must_connect ? "true" : "false") + "\n";
    if (! p->second.pin_shapes.empty ()) {
      for (auto i = p->second.pin_shapes.begin (); i != p->second.pin_shapes.end (); ++i) {
        res += "  [" + ly.get_properties (i->first).to_string () + "] " + i->second.bbox ().to_string () + "\n";
      }
    }
  }

  return res;
}

TEST(20_LayoutConnectivityIndexTest)
{
  db::TempPropertiesRepository tmp_rep;

  db::Layout ly;
  auto l1 = ly.insert_layer (db::LayerProperties (1, 0));
  auto l2 = ly.insert_layer (db::LayerProperties (2, 0));

  db::Cell &device = ly.cell (ly.add_cell ("DEVICE"));
  device.shapes (l1).insert (db::BoxWithProperties (db::Box (0, 0, 1000, 1000), pin_props ("NET_D", "D", false)));
  device.shapes (l1).insert (db::BoxWithProperties (db::Box (2000, 0, 3000, 1000), pin_props ("NET_G1", "G$1", false)));
  device.shapes (l1).insert (db::BoxWithProperties (db::Box (4000, 0, 5000, 1000), pin_props ("NET_G2", "G$2", true)));

  db::Cell &top = ly.cell (ly.add_cell ("TOP"));
  top.shapes (l1).insert (db::BoxWithProperties (db::Box (0, 2000, 1000, 3000), shape_props ("NET1")));
  top.shapes (l2).insert (db::BoxWithProperties (db::Box (0, 2000, 1000, 3000), pin_props ("NET2", "A", false)));
  top.shapes (l2).insert (db::BoxWithProperties (db::Box (1000, 2500, 5000, 3000), shape_props ("NET2")));

  db::LayoutInstanceConnections inst_conn;
  inst_conn.add_connection ("D", "NET1");
  inst_conn.add_connection ("G$1", "NET2");
  inst_conn.add_connection ("G$2", "NET2");

  top.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (device.cell_index ()), db::Trans ()), inst_props (inst_conn)));

  db::LayoutConnectivityIndex top_index (&top);
  top_index.ensure_nets ();
  EXPECT_EQ (index_nets2string (ly, top_index),
    "NET1:\n"
    "  Net shapes:\n"
    "    [1/0] (0,2000;1000,3000)\n"
    "  Instance pins:\n"
    "    [DEVICE: r0 0,0] 'D'\n"
    "NET2:\n"
    "  Pin shapes:\n"
    "    [2/0] (0,2000;1000,3000) 'A'\n"
    "  Net shapes:\n"
    "    [2/0] (1000,2500;5000,3000)\n"
    "  Instance pins:\n"
    "    [DEVICE: r0 0,0] 'G$2'\n"
    "    [DEVICE: r0 0,0] 'G$1'\n"
  );

  EXPECT_EQ (index_pins2string (ly, top_index),
    "A:\n"
    "  must_connect: false\n"
    "  [2/0] (0,2000;1000,3000)\n"
  );

  db::LayoutConnectivityIndex device_index (&device);
  device_index.ensure_nets ();
  EXPECT_EQ (index_nets2string (ly, device_index),
    "NET_D:\n"
    "  Pin shapes:\n"
    "    [1/0] (0,0;1000,1000) 'D'\n"
    "NET_G1:\n"
    "  Pin shapes:\n"
    "    [1/0] (2000,0;3000,1000) 'G$1'\n"
    "NET_G2:\n"
    "  Pin shapes:\n"
    "    [1/0] (4000,0;5000,1000) 'G$2'\n"
  );

  EXPECT_EQ (index_pins2string (ly, device_index),
    "D:\n"
    "  must_connect: false\n"
    "  [1/0] (0,0;1000,1000)\n"
    "G:\n"
    "  must_connect: true\n"
    "  [1/0] (4000,0;5000,1000)\n"
    "  [1/0] (2000,0;3000,1000)\n"
  );
}

}
