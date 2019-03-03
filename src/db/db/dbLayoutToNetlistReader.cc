
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

#include "dbLayoutToNetlistReader.h"
#include "dbLayoutToNetlistFormatDefs.h"
#include "dbLayoutToNetlist.h"

namespace db
{

namespace l2n_std_reader {

class Brace
{
public:
  Brace (LayoutToNetlistStandardReader *reader) : mp_reader (reader), m_checked (false)
  {
    m_has_brace = reader->test ("(");
  }

  operator bool ()
  {
    if (! m_has_brace) {
      m_checked = true;
      return false;
    } else if (mp_reader->test (")")) {
      m_checked = true;
      return false;
    } else {
      return true;
    }
  }

  void done ()
  {
    if (m_has_brace && ! m_checked) {
      mp_reader->expect (")");
      m_checked = true;
    }
  }

private:
  LayoutToNetlistStandardReader *mp_reader;
  bool m_checked;
  bool m_has_brace;
};

}

typedef l2n_std_format::keys<true> skeys;
typedef l2n_std_format::keys<false> lkeys;

LayoutToNetlistStandardReader::LayoutToNetlistStandardReader (tl::InputStream &stream)
  : m_stream (stream), m_path (stream.absolute_path ())
{
  skip ();
}

bool
LayoutToNetlistStandardReader::test (const std::string &token)
{
  skip ();
  return ! at_end () && m_ex.test (token.c_str ());
}

void
LayoutToNetlistStandardReader::expect (const std::string &token)
{
  m_ex.expect (token.c_str ());
}

void
LayoutToNetlistStandardReader::read_word_or_quoted (std::string &s)
{
  m_ex.read_word_or_quoted (s);
}

int
LayoutToNetlistStandardReader::read_int ()
{
  int i = 0;
  m_ex.read (i);
  return i;
}

db::Coord
LayoutToNetlistStandardReader::read_coord ()
{
  db::Coord i = 0;
  m_ex.read (i);
  return i;
}

double
LayoutToNetlistStandardReader::read_double ()
{
  double d = 0;
  m_ex.read (d);
  return d;
}

bool
LayoutToNetlistStandardReader::at_end ()
{
  return (m_ex.at_end () && m_stream.at_end ());
}

void
LayoutToNetlistStandardReader::skip ()
{
  while (m_ex.at_end () || *m_ex.skip () == '#') {
    if (m_stream.at_end ()) {
      return;
    }
    m_line = m_stream.get_line ();
    m_ex = tl::Extractor (m_line.c_str ());
  }
}

void LayoutToNetlistStandardReader::read (db::LayoutToNetlist *l2n)
{
  try {
    do_read (l2n);
  } catch (tl::Exception &ex) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("%s in line: %d of %s")), ex.msg (), m_stream.line_number (), m_path));
  }
}

static db::Region &layer_by_name (db::LayoutToNetlist *l2n, const std::string &name)
{
  db::Region *l = l2n->layer_by_name (name);
  if (! l) {
    throw tl::Exception (tl::to_string (tr ("Not a valid layer name: ")) + name);
  }
  return *l;
}

void LayoutToNetlistStandardReader::do_read (db::LayoutToNetlist *l2n)
{
  int version = 0;
  std::string description;

  tl_assert (l2n->internal_layout ());
  l2n->internal_layout ()->dbu (1.0); //  mainly for testing

  if (l2n->internal_layout ()->cells () == 0) {
    l2n->internal_layout ()->add_cell ("TOP");
  }
  tl_assert (l2n->internal_top_cell () != 0);

  l2n->make_netlist ();

  while (! at_end ()) {

    if (test (skeys::version_key) || test (lkeys::version_key)) {

      Brace br (this);
      version = read_int ();
      br.done ();

    } else if (test (skeys::description_key) || test (lkeys::description_key)) {

      Brace br (this);
      read_word_or_quoted (description);
      br.done ();

    } else if (test (skeys::unit_key) || test (lkeys::unit_key)) {

      Brace br (this);
      double dbu = read_double ();
      l2n->internal_layout ()->dbu (dbu);
      br.done ();

    } else if (test (skeys::top_key) || test (lkeys::top_key)) {

      Brace br (this);
      std::string top;
      read_word_or_quoted (top);
      l2n->internal_layout ()->rename_cell (l2n->internal_top_cell ()->cell_index (), top.c_str ());
      br.done ();

    } else if (test (skeys::layer_key) || test (lkeys::layer_key)) {

      Brace br (this);
      std::string layer;
      read_word_or_quoted (layer);
      delete l2n->make_layer (layer);
      br.done ();

    } else if (test (skeys::connect_key) || test (lkeys::connect_key)) {

      Brace br (this);
      std::string l1;
      read_word_or_quoted (l1);
      while (br) {
        std::string l2;
        read_word_or_quoted (l2);
        l2n->connect (layer_by_name (l2n, l1), layer_by_name (l2n, l2));
      }
      br.done ();

    } else if (test (skeys::global_key) || test (lkeys::global_key)) {

      Brace br (this);
      std::string l1;
      read_word_or_quoted (l1);
      while (br) {
        std::string g;
        read_word_or_quoted (g);
        l2n->connect_global (layer_by_name (l2n, l1), g);
      }
      br.done ();

    } else if (test (skeys::circuit_key) || test (lkeys::circuit_key)) {

      Brace br (this);
      std::string name;
      read_word_or_quoted (name);

      db::Circuit *circuit = new db::Circuit ();
      circuit->set_name (name);
      l2n->netlist ()->add_circuit (circuit);

      db::Layout *ly = l2n->internal_layout ();
      std::pair<bool, db::cell_index_type> ci_old = ly->cell_by_name (name.c_str ());
      db::cell_index_type ci = ci_old.first ? ci_old.second : ly->add_cell (name.c_str ());
      circuit->set_cell_index (ci);

      std::map<db::CellInstArray, std::list<Connections> > connections;
      std::map<unsigned int, Net *> id2net;

      while (br) {

        if (test (skeys::net_key) || test (lkeys::net_key)) {
          read_net (l2n, circuit, id2net);
        } else if (test (skeys::pin_key) || test (lkeys::pin_key)) {
          read_pin (l2n, circuit, id2net);
        } else if (test (skeys::device_key) || test (lkeys::device_key)) {
          std::list<Connections> conn;
          db::CellInstArray ia = read_device (l2n, circuit, conn, id2net);
          connections[ia] = conn;
        } else if (test (skeys::circuit_key) || test (lkeys::circuit_key)) {
          std::list<Connections> conn;
          db::CellInstArray ia = read_subcircuit (l2n, circuit, conn, id2net);
          connections[ia] = conn;
        } else {
          throw tl::Exception (tl::to_string (tr ("Invalid keyword inside circuit definition (net, pin, device or circuit expected)")));
        }

      }
      br.done ();

      db::Cell &ccell = ly->cell (ci);

      //  connections needs to be made after the instances (because in a readonly Instances container
      //  the Instance pointers will invalidate when new instances are added)
      for (db::Cell::const_iterator i = ccell.begin (); ! i.at_end (); ++i) {
        std::map<db::CellInstArray, std::list<Connections> >::const_iterator c = connections.find (i->cell_inst ());
        if (c != connections.end ()) {
          for (std::list<Connections>::const_iterator j = c->second.begin (); j != c->second.end (); ++j) {
            l2n->net_clusters ().clusters_per_cell (ci).add_connection (j->from_cluster, db::ClusterInstance (j->to_cluster, i->cell_index (), i->complex_trans (), i->prop_id ()));
          }
        }
      }

    } else if (test (skeys::device_key) || test (lkeys::device_key)) {

      Brace br (this);
      std::string name;
      read_word_or_quoted (name);

      db::DeviceAbstract *dm = new db::DeviceAbstract ();
      dm->set_name (name);
      l2n->netlist ()->add_device_abstract (dm);

      db::cell_index_type ci = l2n->internal_layout ()->add_cell (name.c_str ());
      dm->set_cell_index (ci);

      std::string cls;
      read_word_or_quoted (cls);

      db::DeviceClass *dc = 0;
      for (db::Netlist::device_class_iterator i = l2n->netlist ()->begin_device_classes (); i != l2n->netlist ()->end_device_classes (); ++i) {
        if (i->name () == cls) {
          dc = i.operator-> ();
        }
      }

      //  use a generic device class unless the right one is registered already.
      bool gen_dc = (dc == 0);
      if (gen_dc) {
        dc = new db::DeviceClass ();
        dc->set_name (cls);
        l2n->netlist ()->add_device_class (dc);
      }

      dm->set_device_class (dc);

      while (br) {

        if (test (skeys::terminal_key) || test (lkeys::terminal_key)) {
          read_abstract_terminal (l2n, dm, gen_dc ? dc : 0);
        } else {
          throw tl::Exception (tl::to_string (tr ("Invalid keyword inside device abstract definition (terminal expected)")));
        }

      }

      br.done ();

    }

  }

  l2n->set_netlist_extracted ();
}

std::pair<unsigned int, db::PolygonRef>
LayoutToNetlistStandardReader::read_geometry (db::LayoutToNetlist *l2n)
{
  std::string lname;

  if (test (skeys::rect_key) || test (lkeys::rect_key)) {

    Brace br (this);

    read_word_or_quoted (lname);
    unsigned int lid = l2n->layer_of (layer_by_name (l2n, lname));

    db::Coord l = read_coord ();
    db::Coord b = read_coord ();
    db::Coord r = read_coord ();
    db::Coord t = read_coord ();
    db::Box box (l, b, r, t);

    br.done ();

    return std::make_pair (lid, db::PolygonRef (db::Polygon (box), l2n->internal_layout ()->shape_repository ()));

  } else if (test (skeys::polygon_key) || test (lkeys::polygon_key)) {

    Brace br (this);

    read_word_or_quoted (lname);
    unsigned int lid = l2n->layer_of (layer_by_name (l2n, lname));

    std::vector<db::Point> pt;

    db::Coord x = 0, y = 0;
    while (br) {
      if (! test ("*")) {
        x = read_coord ();
      }
      if (! test ("*")) {
        y = read_coord ();
      }
      pt.push_back (db::Point (x, y));
    }

    br.done ();

    db::Polygon poly;
    poly.assign_hull (pt.begin (), pt.end ());
    return std::make_pair (lid, db::PolygonRef (poly, l2n->internal_layout ()->shape_repository ()));

  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid keyword inside net or terminal definition (polygon or rect expected)")));
  }
}

void
LayoutToNetlistStandardReader::read_net (db::LayoutToNetlist *l2n, db::Circuit *circuit, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);

  unsigned int id = (unsigned int) read_int ();
  std::string name;

  if (test (skeys::name_key) || test (lkeys::name_key)) {
    Brace br_name (this);
    read_word_or_quoted (name);
    br_name.done ();
  }

  db::Net *net = new db::Net ();
  net->set_name (name);
  circuit->add_net (net);

  id2net.insert (std::make_pair (id, net));

  db::connected_clusters<db::PolygonRef> &cc = l2n->net_clusters ().clusters_per_cell (circuit->cell_index ());
  db::local_cluster<db::PolygonRef> &lc = *cc.insert ();
  net->set_cluster_id (lc.id ());

  db::Cell &cell = l2n->internal_layout ()->cell (circuit->cell_index ());

  while (br) {
    std::pair<unsigned int, db::PolygonRef> pr = read_geometry (l2n);
    lc.add (pr.second, pr.first);
    cell.shapes (pr.first).insert (pr.second);
  }

  br.done ();
}

void
LayoutToNetlistStandardReader::read_pin (db::LayoutToNetlist * /*l2n*/, db::Circuit *circuit, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);
  std::string name;
  read_word_or_quoted (name);
  unsigned int netid = (unsigned int) read_int ();
  br.done ();

  const db::Pin &pin = circuit->add_pin (name);

  db::Net *net = id2net [netid];
  if (!net) {
    throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
  }

  circuit->connect_pin (pin.id (), net);
}

db::CellInstArray
LayoutToNetlistStandardReader::read_device (db::LayoutToNetlist *l2n, db::Circuit *circuit, std::list<Connections> &refs, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);

  std::string name;
  read_word_or_quoted (name);

  std::string dmname;
  read_word_or_quoted (dmname);

  db::DeviceAbstract *dm = 0;
  for (db::Netlist::device_abstract_iterator i = l2n->netlist ()->begin_device_abstracts (); i != l2n->netlist ()->end_device_abstracts (); ++i) {
    if (i->name () == dmname) {
      dm = i.operator-> ();
    }
  }

  if (! dm) {
    throw tl::Exception (tl::to_string (tr ("Not a valid device abstract name: ")) + dmname);
  }

  db::Device *device = new db::Device ();
  device->set_device_class (const_cast<db::DeviceClass *> (dm->device_class ()));
  device->set_device_abstract (dm);
  device->set_name (name);
  circuit->add_device (device);

  db::Coord x = 0, y = 0;

  while (br) {

    if (test (skeys::location_key) || test (lkeys::location_key)) {

      Brace br2 (this);
      x = read_coord ();
      y = read_coord ();
      br2.done ();

    } else if (test (skeys::terminal_key) || test (lkeys::terminal_key)) {

      Brace br2 (this);
      std::string tname;
      read_word_or_quoted (tname);
      unsigned int netid = (unsigned int) read_int ();
      br2.done ();

      size_t tid = std::numeric_limits<size_t>::max ();
      const std::vector<db::DeviceTerminalDefinition> &td = dm->device_class ()->terminal_definitions ();
      for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
        if (t->name () == tname) {
          tid = t->id ();
          break;
        }
      }

      if (tid == std::numeric_limits<size_t>::max ()) {
        throw tl::Exception (tl::to_string (tr ("Not a valid terminal name: ")) + tname + tl::to_string (tr (" for device class: ")) + dm->device_class ()->name ());
      }

      db::Net *net = id2net [netid];
      if (!net) {
        throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
      }

      device->connect_terminal (tid, net);
      refs.push_back (Connections (net->cluster_id (), dm->cluster_id_for_terminal (tid)));

    } else if (test (skeys::param_key) || test (lkeys::param_key)) {

      Brace br2 (this);
      std::string pname;
      read_word_or_quoted (pname);
      double value = read_double ();
      br2.done ();

      size_t pid = std::numeric_limits<size_t>::max ();
      const std::vector<db::DeviceParameterDefinition> &pd = dm->device_class ()->parameter_definitions ();
      for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
        if (p->name () == pname) {
          pid = p->id ();
          break;
        }
      }

      //  if no parameter with this name exists, create one
      if (pid == std::numeric_limits<size_t>::max ()) {
        //  TODO: this should only happen for generic devices
        db::DeviceClass *dc = const_cast<db::DeviceClass *> (dm->device_class ());
        pid = dc->add_parameter_definition (db::DeviceParameterDefinition (pname, std::string ())).id ();
      }

      device->set_parameter_value (pid, value);

    } else {
      throw tl::Exception (tl::to_string (tr ("Invalid keyword inside device definition (location, param or terminal expected)")));
    }

  }

  double dbu = l2n->internal_layout ()->dbu ();
  device->set_position (db::DPoint (dbu * x, dbu * y));

  br.done ();

  //  make device cell instance
  db::CellInstArray inst (db::CellInst (dm->cell_index ()), db::Trans (db::Vector (x, y)));
  db::Cell &ccell = l2n->internal_layout ()->cell (circuit->cell_index ());
  ccell.insert (inst);

  return inst;
}

db::CellInstArray
LayoutToNetlistStandardReader::read_subcircuit (db::LayoutToNetlist *l2n, db::Circuit *circuit, std::list<Connections> &refs, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);

  std::string name;
  read_word_or_quoted (name);

  std::string xname;
  read_word_or_quoted (xname);

  db::Circuit *circuit_ref = l2n->netlist ()->circuit_by_name (xname);
  if (! circuit_ref) {
    throw tl::Exception (tl::to_string (tr ("Not a valid device circuit name: ")) + xname);
  }

  db::SubCircuit *subcircuit = new db::SubCircuit (circuit_ref);
  subcircuit->set_name (name);
  circuit->add_subcircuit (subcircuit);

  db::Coord x = 0, y = 0;
  bool mirror = false;
  double angle = 0;
  double mag = 1.0;

  db::InstElement ie;
  bool inst_made = false;

  while (br) {

    if (test (skeys::location_key) || test (lkeys::location_key)) {

      Brace br2 (this);
      x = read_coord ();
      y = read_coord ();
      br2.done ();

      if (inst_made) {
        throw tl::Exception (tl::to_string (tr ("location key must come before pin key in subcircuit definition")));
      }

    } else if (test (skeys::rotation_key) || test (lkeys::rotation_key)) {

      Brace br2 (this);
      angle = read_double ();
      br2.done ();

      if (inst_made) {
        throw tl::Exception (tl::to_string (tr ("rotation key must come before pin key in subcircuit definition")));
      }

    } else if (test (skeys::mirror_key) || test (lkeys::mirror_key)) {

      mirror = true;
      if (inst_made) {
        throw tl::Exception (tl::to_string (tr ("mirror key must come before pin key in subcircuit definition")));
      }

    } else if (test (skeys::scale_key) || test (lkeys::scale_key)) {

      Brace br2 (this);
      mag = read_double ();
      br2.done ();

      if (inst_made) {
        throw tl::Exception (tl::to_string (tr ("scale key must come before pin key in subcircuit definition")));
      }

    } else if (test (skeys::pin_key) || test (lkeys::pin_key)) {

      Brace br2 (this);
      std::string pname;
      read_word_or_quoted (pname);
      unsigned int netid = (unsigned int) read_int ();
      br2.done ();

      const db::Pin *sc_pin = circuit_ref->pin_by_name (pname);
      if (! sc_pin) {
        throw tl::Exception (tl::to_string (tr ("Not a valid pin name: ")) + pname + tl::to_string (tr (" for circuit: ")) + circuit_ref->name ());
      }

      db::Net *net = id2net [netid];
      if (!net) {
        throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
      }

      subcircuit->connect_pin (sc_pin->id (), net);
      db::Net *sc_net = circuit_ref->net_for_pin (sc_pin->id ());
      if (sc_net) {
        refs.push_back (Connections (net->cluster_id (), sc_net->cluster_id ()));
      }

    } else {
      throw tl::Exception (tl::to_string (tr ("Invalid keyword inside subcircuit definition (location, rotation, mirror, scale or pin expected)")));
    }

  }

  br.done ();

  double dbu = l2n->internal_layout ()->dbu ();
  subcircuit->set_trans (db::DCplxTrans (mag, angle, mirror, db::DVector (dbu * x, dbu * y)));

  db::CellInstArray inst (db::CellInst (circuit_ref->cell_index ()), db::ICplxTrans (mag, angle, mirror, db::Vector (x, y)));
  db::Cell &ccell = l2n->internal_layout ()->cell (circuit->cell_index ());
  ccell.insert (inst);

  return inst;
}

void
LayoutToNetlistStandardReader::read_abstract_terminal (db::LayoutToNetlist *l2n, db::DeviceAbstract *dm, db::DeviceClass *dc)
{
  Brace br (this);

  std::string name;
  read_word_or_quoted (name);

  size_t tid = std::numeric_limits<size_t>::max ();
  const std::vector<db::DeviceTerminalDefinition> &td = dm->device_class ()->terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    if (t->name () == name) {
      tid = t->id ();
      break;
    }
  }

  //  create a terminal unless one with this name already exists
  if (tid == std::numeric_limits<size_t>::max ()) {
    if (! dc) {
      throw tl::Exception (tl::to_string (tr ("Not a valid terminal name: ")) + name + tl::to_string (tr (" for device class: ")) + dm->device_class ()->name ());
    }
    db::DeviceTerminalDefinition new_td (name, std::string ());
    tid = dc->add_terminal_definition (new_td).id ();
  }

  db::connected_clusters<db::PolygonRef> &cc = l2n->net_clusters ().clusters_per_cell (dm->cell_index ());
  db::local_cluster<db::PolygonRef> &lc = *cc.insert ();
  dm->set_cluster_id_for_terminal (tid, lc.id ());

  db::Cell &cell = l2n->internal_layout ()->cell (dm->cell_index ());

  while (br) {
    std::pair<unsigned int, db::PolygonRef> pr = read_geometry (l2n);
    lc.add (pr.second, pr.first);
    cell.shapes (pr.first).insert (pr.second);
  }

  br.done ();
}

}
