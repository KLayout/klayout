
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

namespace db
{

namespace l2n_std_reader {

  Brace::Brace (db::LayoutToNetlistStandardReader *reader) : mp_reader (reader), m_checked (false)
  {
    m_has_brace = reader->test ("(");
  }

  Brace::operator bool ()
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

  void Brace::done ()
  {
    if (m_has_brace && ! m_checked) {
      mp_reader->expect (")");
      m_checked = true;
    }
  }

}

typedef l2n_std_format::keys<true> skeys;
typedef l2n_std_format::keys<false> lkeys;

LayoutToNetlistStandardReader::LayoutToNetlistStandardReader (tl::InputStream &stream)
  : m_stream (stream), m_path (stream.absolute_path ()), m_dbu (0.0),
    m_progress (tl::to_string (tr ("Reading L2N database")), 1000)
{
  m_progress.set_format (tl::to_string (tr ("%.0fk lines")));
  m_progress.set_format_unit (1000.0);
  m_progress.set_unit (100000.0);

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

bool
LayoutToNetlistStandardReader::try_read_int (int &i)
{
  i = 0;
  return m_ex.try_read (i);
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
  skip ();
  return (m_ex.at_end () && m_stream.at_end ());
}

void
LayoutToNetlistStandardReader::skip ()
{
  while (m_ex.at_end () || *m_ex.skip () == '#') {
    if (m_stream.at_end ()) {
      m_ex = tl::Extractor ();
      return;
    }
    m_progress.set (m_stream.line_number ());
    m_line = m_stream.get_line ();
    m_ex = tl::Extractor (m_line.c_str ());
  }
}

void LayoutToNetlistStandardReader::skip_element ()
{
  std::string s;
  double f;

  if (m_ex.try_read_word (s)) {

    //  skip bracket elements after token key
    Brace br (this);
    while (br) {
      skip_element ();
    }
    br.done ();

  } else if (m_ex.test ("*")) {

    //  asterisk is allowed as element (e.g. inside point)

  } else if (m_ex.try_read_quoted (s)) {

    //  skip string

  } else if (m_ex.try_read (f)) {

    //  skip numeric value

  } else {

    Brace br (this);
    if (br) {

      //  skip bracket elements without token
      while (br) {
        skip_element ();
      }
      br.done ();

    } else {
      throw tl::Exception (tl::to_string (tr ("Unexpected token")));
    }

  }
}

bool LayoutToNetlistStandardReader::read_message (std::string &msg)
{
  if (test (skeys::description_key) || test (lkeys::description_key)) {
    Brace br (this);
    read_word_or_quoted (msg);
    br.done ();
    return true;
  } else {
    return false;
  }
}

bool LayoutToNetlistStandardReader::read_severity (db::Severity &severity)
{
  if (test (skeys::info_severity_key) || test (lkeys::info_severity_key)) {
    severity = db::Info;
    return true;
  } else if (test (skeys::warning_severity_key) || test (lkeys::warning_severity_key)) {
    severity = db::Warning;
    return true;
  } else if (test (skeys::error_severity_key) || test (lkeys::error_severity_key)) {
    severity = db::Error;
    return true;
  } else {
    return false;
  }
}

bool LayoutToNetlistStandardReader::read_message_cell (std::string &cell_name)
{
  if (test (skeys::cell_key) || test (lkeys::cell_key)) {
    Brace br (this);
    read_word_or_quoted (cell_name);
    br.done ();
    return true;
  } else {
    return false;
  }
}

bool LayoutToNetlistStandardReader::read_message_geometry (db::DPolygon &polygon)
{
  if (test (skeys::polygon_key) || test (lkeys::polygon_key)) {
    Brace br (this);
    std::string s;
    read_word_or_quoted (s);
    tl::Extractor ex (s.c_str ());
    ex.read (polygon);
    br.done ();
    return true;
  } else {
    return false;
  }
}

bool LayoutToNetlistStandardReader::read_message_cat (std::string &category_name, std::string &category_description)
{
  if (test (skeys::cat_key) || test (lkeys::cat_key)) {
    Brace br (this);
    read_word_or_quoted (category_name);
    if (br) {
      read_word_or_quoted (category_description);
    }
    br.done ();
    return true;
  } else {
    return false;
  }
}

void LayoutToNetlistStandardReader::read_message_entry (db::LogEntryData &data)
{
  Severity severity (db::NoSeverity);
  std::string msg, cell_name, category_name, category_description;
  db::DPolygon geometry;

  Brace br (this);
  while (br) {
    if (read_severity (severity)) {
      //  continue
    } else if (read_message (msg)) {
      //  continue
    } else if (read_message_cell (cell_name)) {
      //  continue
    } else if (read_message_cat (category_name, category_description)) {
      //  continue
    } else if (read_message_geometry (geometry)) {
      //  continue
    } else {
      skip_element ();
    }
  }
  br.done ();

  data.set_severity (severity);
  data.set_message (msg);
  data.set_cell_name (cell_name);
  data.set_category_description (category_description);
  data.set_category_name (category_name);
  data.set_geometry (geometry);
}

void LayoutToNetlistStandardReader::do_read (db::LayoutToNetlist *l2n)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("File read: ")) + m_path);

  try {
    read_netlist (0, l2n);
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

void LayoutToNetlistStandardReader::read_netlist (db::Netlist *netlist, db::LayoutToNetlist *l2n, LayoutToNetlistStandardReader::Brace *nested, std::map<const db::Circuit *, ObjectMap> *map_per_circuit)
{
  m_dbu = 0.001;
  int version = 0;
  std::string description;

  if (l2n) {

    tl_assert (netlist == 0);

    tl_assert (l2n->internal_layout ());
    l2n->internal_layout ()->dbu (1.0); //  mainly for testing

    if (l2n->internal_layout ()->cells () == 0) {
      l2n->internal_layout ()->add_cell ("TOP");
    }
    tl_assert (l2n->internal_top_cell () != 0);

    netlist = l2n->make_netlist ();

  } else {
    tl_assert (netlist != 0);
  }

  db::LayoutLocker layout_locker (l2n ? l2n->internal_layout () : 0);

  while (nested ? *nested : ! at_end ()) {

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
      m_dbu = read_double ();
      if (l2n) {
        l2n->internal_layout ()->dbu (m_dbu);
      }
      br.done ();

    } else if (l2n && (test (skeys::top_key) || test (lkeys::top_key))) {

      Brace br (this);
      std::string top;
      read_word_or_quoted (top);
      l2n->internal_layout ()->rename_cell (l2n->internal_top_cell ()->cell_index (), top.c_str ());
      br.done ();

    } else if (l2n && (test (skeys::layer_key) || test (lkeys::layer_key))) {

      Brace br (this);
      std::string layer, lspec;
      read_word_or_quoted (layer);
      if (br) {
        read_word_or_quoted (lspec);
      }

      std::unique_ptr<db::Region> region (l2n->make_layer (layer));
      if (! lspec.empty ()) {
        unsigned int layer_index = l2n->layer_of (*region);
        tl::Extractor ex (lspec.c_str ());
        db::LayerProperties lp;
        lp.read (ex);
        l2n->internal_layout ()->set_properties (layer_index, lp);
      }

      br.done ();

    } else if (test (skeys::class_key) || test (lkeys::class_key)) {

      Brace br (this);
      std::string class_name, templ_name;
      read_word_or_quoted (class_name);
      read_word_or_quoted (templ_name);

      if (netlist->device_class_by_name (class_name) != 0) {
        throw tl::Exception (tl::to_string (tr ("Duplicate definition of device class: ")) + class_name);
      }

      db::DeviceClassTemplateBase *dct = db::DeviceClassTemplateBase::template_by_name (templ_name);
      if (! dct) {
        throw tl::Exception (tl::to_string (tr ("Invalid device class template: ")) + templ_name);
      }

      db::DeviceClass *dc = dct->create ();
      dc->set_name (class_name);
      netlist->add_device_class (dc);

      while (br) {

        if (test (skeys::terminal_key) || test (lkeys::terminal_key)) {

          Brace br (this);

          std::string terminal_name;
          read_word_or_quoted (terminal_name);
          if (! dc->has_terminal_with_name (terminal_name)) {
            db::DeviceTerminalDefinition td;
            td.set_name (terminal_name);
            dc->add_terminal_definition (td);
          }

          br.done ();

        } else if (test (skeys::param_key) || test (lkeys::param_key)) {

          Brace br (this);

          std::string param_name;
          read_word_or_quoted (param_name);
          int primary = read_int ();
          int default_value = read_double ();
          if (! dc->has_parameter_with_name (param_name)) {
            db::DeviceParameterDefinition pd;
            pd.set_name (param_name);
            pd.set_is_primary (primary);
            pd.set_default_value (default_value);
            dc->add_parameter_definition (pd);
          } else {
            db::DeviceParameterDefinition *pd = dc->parameter_definition_non_const (dc->parameter_id_for_name (param_name));
            pd->set_default_value (default_value);
            pd->set_is_primary (primary);
          }

          br.done ();

        } else {
          skip_element ();
        }

      }

      br.done ();

    } else if (l2n && (test (skeys::connect_key) || test (lkeys::connect_key))) {

      Brace br (this);
      std::string l1;
      read_word_or_quoted (l1);
      while (br) {
        std::string l2;
        read_word_or_quoted (l2);
        l2n->connect (layer_by_name (l2n, l1), layer_by_name (l2n, l2));
      }
      br.done ();

    } else if (l2n && (test (skeys::message_key) || test (lkeys::message_key))) {

      db::LogEntryData data;
      read_message_entry (data);

      l2n->log_entry (data);

    } else if (l2n && (test (skeys::global_key) || test (lkeys::global_key))) {

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
      netlist->add_circuit (circuit);

      db::cell_index_type device_cell_index = 0;

      db::CplxTrans dbu (m_dbu);

      if (l2n) {

        db::Layout *ly = l2n->internal_layout ();
        std::pair<bool, db::cell_index_type> ci_old = ly->cell_by_name (name.c_str ());
        device_cell_index = ci_old.first ? ci_old.second : ly->add_cell (name.c_str ());
        circuit->set_cell_index (device_cell_index);

      }

      std::map<db::CellInstArray, std::list<Connections> > connections;
      ObjectMap map_local;
      ObjectMap *map = &map_local;
      if (map_per_circuit) {
        map = &(*map_per_circuit)[circuit];
      }

      while (br) {

        if (test (skeys::property_key) || test (lkeys::property_key)) {
          read_property (circuit);
        } else if (test (skeys::rect_key) || test (lkeys::rect_key)) {
          circuit->set_boundary (db::DPolygon (dbu * read_rect ()));
        } else if (test (skeys::polygon_key) || test (lkeys::polygon_key)) {
          circuit->set_boundary (read_polygon ().transformed (dbu));
        } else if (test (skeys::net_key) || test (lkeys::net_key)) {
          read_net (netlist, l2n, circuit, *map);
        } else if (test (skeys::pin_key) || test (lkeys::pin_key)) {
          read_pin (netlist, l2n, circuit, *map);
        } else if (test (skeys::device_key) || test (lkeys::device_key)) {
          read_device (netlist, l2n, circuit, *map, connections);
        } else if (test (skeys::circuit_key) || test (lkeys::circuit_key)) {
          read_subcircuit (netlist, l2n, circuit, *map, connections);
        } else if (at_end ()) {
          throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside circuit definition (rect, polygon, net, pin, device or circuit expected)")));
        } else {
          skip_element ();
        }

      }
      br.done ();

      if (l2n) {

        db::Layout *ly = l2n->internal_layout ();
        db::Cell &ccell = ly->cell (device_cell_index);

        //  connections needs to be made after the instances (because in a readonly Instances container
        //  the Instance pointers will invalidate when new instances are added)
        for (db::Cell::const_iterator i = ccell.begin (); ! i.at_end (); ++i) {
          std::map<db::CellInstArray, std::list<Connections> >::const_iterator c = connections.find (i->cell_inst ());
          if (c != connections.end ()) {
            for (std::list<Connections>::const_iterator j = c->second.begin (); j != c->second.end (); ++j) {
              l2n->net_clusters ().clusters_per_cell (device_cell_index).add_connection (j->from_cluster, db::ClusterInstance (j->to_cluster, i->cell_index (), i->complex_trans (), i->prop_id ()));
            }
          }
        }

      }

    } else if (test (skeys::device_key) || test (lkeys::device_key)) {

      Brace br (this);
      std::string name;
      read_word_or_quoted (name);

      db::DeviceAbstract *dm = new db::DeviceAbstract ();
      dm->set_name (name);
      netlist->add_device_abstract (dm);

      if (l2n) {
        db::cell_index_type ci = l2n->internal_layout ()->add_cell (name.c_str ());
        dm->set_cell_index (ci);
      }

      std::string cls;
      read_word_or_quoted (cls);

      db::DeviceClass *dc = netlist->device_class_by_name (cls);

      //  use a generic device class unless the right one is registered already.
      bool gen_dc = (dc == 0);
      if (gen_dc) {
        dc = new db::DeviceClass ();
        dc->set_name (cls);
        netlist->add_device_class (dc);
      }

      dm->set_device_class (dc);

      while (br) {

        if (test (skeys::terminal_key) || test (lkeys::terminal_key)) {
          read_abstract_terminal (l2n, dm, gen_dc ? dc : 0);
        } else if (at_end ()) {
          throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside device abstract definition (terminal expected)")));
        } else {
          skip_element ();
        }

      }

      br.done ();

    } else if (at_end ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside device abstract definition (terminal expected)")));
    } else {
      skip_element ();
    }

  }

  if (l2n) {
    l2n->set_netlist_extracted ();
  }

  if (version > 1) {
    throw tl::Exception (tl::to_string (tr ("This program version only supports version 1 of the L2N DB format. File version is: ")) + tl::to_string (version));
  }
}

db::Point
LayoutToNetlistStandardReader::read_point ()
{
  db::Coord x = m_ref.x (), y = m_ref.y ();

  if (test ("(")) {
    x += read_coord ();
    y += read_coord ();
    expect (")");
  } else {
    if (! test ("*")) {
      x = read_coord ();
    }
    if (! test ("*")) {
      y = read_coord ();
    }
  }

  m_ref = db::Point (x, y);
  return m_ref;
}

void
LayoutToNetlistStandardReader::read_property (db::NetlistObject *obj)
{
  Brace br (this);

  tl::Variant k, v;
  m_ex.read (k);
  m_ex.read (v);

  if (obj) {
    obj->set_property (k, v);
  }

  br.done ();
}

db::Box
LayoutToNetlistStandardReader::read_rect ()
{
  m_ref = db::Point ();

  Brace br (this);

  db::Point lb = read_point ();
  db::Point rt = read_point ();
  db::Box box (lb, rt);

  br.done ();

  return box;
}

db::Polygon
LayoutToNetlistStandardReader::read_polygon ()
{
  m_ref = db::Point ();

  Brace br (this);

  std::vector<db::Point> pt;
  while (br) {
    pt.push_back (read_point ());
  }
  br.done ();

  db::Polygon poly;
  poly.assign_hull (pt.begin (), pt.end ());
  return poly;
}

void
LayoutToNetlistStandardReader::read_geometries (db::NetlistObject *obj, Brace &br, db::LayoutToNetlist *l2n, db::local_cluster<db::NetShape> &lc, db::Cell &cell)
{
  m_ref = db::Point ();
  std::string lname;

  while (br) {

    if (test (skeys::property_key) || test (lkeys::property_key)) {

      read_property (obj);

    } else if (test (skeys::rect_key) || test (lkeys::rect_key)) {

      Brace br (this);

      read_word_or_quoted (lname);
      unsigned int lid = l2n->layer_of (layer_by_name (l2n, lname));

      db::Point lb = read_point ();
      db::Point rt = read_point ();
      db::Box box (lb, rt);

      br.done ();

      NetShape n (db::PolygonRef (db::Polygon (box), l2n->internal_layout ()->shape_repository ()));

      lc.add (n, lid);
      n.insert_into (cell.shapes (lid));

    } else if (test (skeys::polygon_key) || test (lkeys::polygon_key)) {

      Brace br (this);

      read_word_or_quoted (lname);
      unsigned int lid = l2n->layer_of (layer_by_name (l2n, lname));

      std::vector<db::Point> pt;
      while (br) {
        pt.push_back (read_point ());
      }
      br.done ();

      db::Polygon poly;
      poly.assign_hull (pt.begin (), pt.end ());
      NetShape n (db::PolygonRef (poly, l2n->internal_layout ()->shape_repository ()));

      lc.add (n, lid);
      n.insert_into (cell.shapes (lid));

    } else if (test (skeys::text_key) || test (lkeys::text_key)) {

      Brace br (this);

      read_word_or_quoted (lname);
      unsigned int lid = l2n->layer_of (layer_by_name (l2n, lname));

      std::string text;
      read_word_or_quoted (text);

      db::Point pt = read_point ();

      br.done ();

      NetShape n (db::TextRef (db::Text (text, db::Trans (pt - db::Point ())), l2n->internal_layout ()->shape_repository ()));

      lc.add (n, lid);
      n.insert_into (cell.shapes (lid));

    } else if (at_end ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file (polygon, text or rect expected)")));
    } else {
      skip_element ();
    }
  }
}

void
LayoutToNetlistStandardReader::read_net (db::Netlist * /*netlist*/, db::LayoutToNetlist *l2n, db::Circuit *circuit, ObjectMap &map)
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

  map.id2net.insert (std::make_pair (id, net));

  if (l2n) {

    db::connected_clusters<db::NetShape> &cc = l2n->net_clusters ().clusters_per_cell (circuit->cell_index ());
    db::local_cluster<db::NetShape> &lc = *cc.insert ();
    net->set_cluster_id (lc.id ());

    db::Cell &cell = l2n->internal_layout ()->cell (circuit->cell_index ());
    read_geometries (net, br, l2n, lc, cell);

  }

  br.done ();
}

void
LayoutToNetlistStandardReader::read_pin (db::Netlist * /*netlist*/, db::LayoutToNetlist * /*l2n*/, db::Circuit *circuit, ObjectMap &map)
{
  Brace br (this);

  db::Net *net = 0;

  db::Pin pin;
  int netid = 0;

  while (br) {

    if (test (skeys::name_key) || test (lkeys::name_key)) {

      if (! pin.name ().empty ()) {
        throw tl::Exception (tl::to_string (tr ("Duplicate pin name")));
      }

      Brace br_name (this);
      std::string n;
      read_word_or_quoted (n);
      pin.set_name (n);
      br_name.done ();

    } else if (test (skeys::property_key) || test (lkeys::property_key)) {

      read_property (&pin);

    } else if (try_read_int (netid)) {

      if (net) {
        throw tl::Exception (tl::to_string (tr ("Duplicate net ID")));
      }

      net = map.id2net [(unsigned int) netid];
      if (!net) {
        throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
      }

    } else {
      skip_element ();
    }

  }

  size_t pin_id = circuit->add_pin (pin).id ();
  //  NOTE: because we identify pins by their order and not by ID we need to ensure the pin IDs are
  //  generated sequentially.
  tl_assert (circuit->pin_count () == pin_id + 1);
  if (net) {
    circuit->connect_pin (pin_id, net);
  }

  br.done ();
}

size_t
LayoutToNetlistStandardReader::terminal_id (const db::DeviceClass *device_class, const std::string &tname)
{
  const std::vector<db::DeviceTerminalDefinition> &td = device_class->terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    if (t->name () == tname) {
      return t->id ();
    }
  }

  throw tl::Exception (tl::to_string (tr ("Not a valid terminal name: ")) + tname + tl::to_string (tr (" for device class: ")) + device_class->name ());
}

std::pair<db::DeviceAbstract *, const db::DeviceClass *>
LayoutToNetlistStandardReader::device_model_by_name (db::Netlist *netlist, const std::string &dmname)
{
  for (db::Netlist::device_abstract_iterator i = netlist->begin_device_abstracts (); i != netlist->end_device_abstracts (); ++i) {
    if (i->name () == dmname) {
      return std::make_pair (i.operator-> (), i->device_class ());
    }
  }

  db::DeviceClass *cls = netlist->device_class_by_name (dmname);
  if (! cls) {
    throw tl::Exception (tl::to_string (tr ("Not a valid device abstract name: ")) + dmname);
  }

  return std::make_pair ((db::DeviceAbstract *) 0, cls);
}

void
LayoutToNetlistStandardReader::read_device (db::Netlist *netlist, db::LayoutToNetlist *l2n, db::Circuit *circuit, ObjectMap &map, std::map<db::CellInstArray, std::list<Connections> > &connections)
{
  Brace br (this);

  size_t id = size_t (read_int ());

  std::string name;

  std::string dmname;
  read_word_or_quoted (dmname);

  std::pair<db::DeviceAbstract *, const db::DeviceClass *> dm = device_model_by_name (netlist, dmname);

  std::unique_ptr<db::Device> device (new db::Device ());
  device->set_device_class (const_cast<db::DeviceClass *> (dm.second));
  device->set_device_abstract (dm.first);

  db::DCplxTrans trans;
  db::CplxTrans dbu (m_dbu);
  db::VCplxTrans dbu_inv (1.0 / m_dbu);

  size_t max_tid = 0;

  while (br) {

    if (test (skeys::name_key) || test (lkeys::name_key)) {

      Brace br_name (this);
      read_word_or_quoted (name);
      br_name.done ();

    } else if (read_trans_part (trans)) {

      //  .. nothing yet ..

    } else if (test (skeys::property_key) || test (lkeys::property_key)) {

      read_property (device.get ());

    } else if (test (skeys::device_key) || test (lkeys::device_key)) {

      std::string n;
      db::DCplxTrans dm_trans;

      Brace br2 (this);

      read_word_or_quoted (n);

      while (br2) {
        if (! read_trans_part (dm_trans)) {
          throw tl::Exception (tl::to_string (tr ("Invalid keyword inside device definition (location, scale, rotation or mirror expected)")));
        }
      }

      br2.done ();

      db::DeviceAbstract *da = device_model_by_name (netlist, n).first;

      device->other_abstracts ().push_back (db::DeviceAbstractRef (da, dm_trans));

    } else if (test (skeys::connect_key) || test (lkeys::connect_key)) {

      Brace br2 (this);

      int device_comp_index = read_int ();

      std::string touter, tinner;
      read_word_or_quoted (touter);
      read_word_or_quoted (tinner);

      br2.done ();

      if (device_comp_index < 0 || device_comp_index > int (device->other_abstracts ().size ())) {
        throw tl::Exception (tl::to_string (tr ("Not a valid device component index: ")) + tl::to_string (device_comp_index));
      }

      size_t touter_id = terminal_id (dm.second, touter);
      size_t tinner_id = terminal_id (dm.second, tinner);

      device->reconnected_terminals () [(unsigned int) touter_id].push_back (db::DeviceReconnectedTerminal (size_t (device_comp_index), (unsigned int) tinner_id));

    } else if (test (skeys::terminal_key) || test (lkeys::terminal_key)) {

      Brace br2 (this);
      std::string tname;
      read_word_or_quoted (tname);

      size_t tid = terminal_id (dm.second, tname);
      max_tid = std::max (max_tid, tid + 1);

      if (br2) {

        unsigned int netid = (unsigned int) read_int ();
        db::Net *net = map.id2net [netid];
        if (!net) {
          throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
        }

        device->connect_terminal (tid, net);

      }

      br2.done ();

    } else if (test (skeys::param_key) || test (lkeys::param_key)) {

      Brace br2 (this);
      std::string pname;
      read_word_or_quoted (pname);
      double value = read_double ();
      br2.done ();

      size_t pid = std::numeric_limits<size_t>::max ();
      const std::vector<db::DeviceParameterDefinition> &pd = dm.second->parameter_definitions ();
      for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
        if (p->name () == pname) {
          pid = p->id ();
          break;
        }
      }

      //  if no parameter with this name exists, create one
      if (pid == std::numeric_limits<size_t>::max ()) {
        //  TODO: this should only happen for generic devices
        db::DeviceClass *dc = const_cast<db::DeviceClass *> (dm.second);
        pid = dc->add_parameter_definition (db::DeviceParameterDefinition (pname, std::string ())).id ();
      }

      device->set_parameter_value (pid, value);

    } else if (at_end ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside device definition (location, scale, mirror, rotation, param or terminal expected)")));
    } else {
      skip_element ();
    }

  }

  br.done ();

  if (id > 0) {
    map.id2device.insert (std::make_pair ((unsigned int) id, device.get ()));
  }

  device->set_trans (trans);
  device->set_name (name);

  if (l2n && dm.first) {

    db::Cell &ccell = l2n->internal_layout ()->cell (circuit->cell_index ());

    //  make device cell instances
    std::vector<db::CellInstArray> insts;

    db::CellInstArray inst (db::CellInst (dm.first->cell_index ()), dbu_inv * trans * dbu);
    ccell.insert (inst);
    insts.push_back (inst);

    const std::vector<db::DeviceAbstractRef> &other_devices = device->other_abstracts ();
    for (std::vector<db::DeviceAbstractRef>::const_iterator i = other_devices.begin (); i != other_devices.end (); ++i) {

      db::CellInstArray other_inst (db::CellInst (i->device_abstract->cell_index ()), dbu_inv * trans * i->trans * dbu);
      ccell.insert (other_inst);
      insts.push_back (other_inst);

    }

    //  register cluster collections to be made later

    for (size_t tid = 0; tid < max_tid; ++tid) {

      const db::Net *net = device->net_for_terminal (tid);
      if (! net) {
        continue;
      }

      if (! device->reconnected_terminals ().empty ()) {

        const std::vector<db::DeviceReconnectedTerminal> *tr = device->reconnected_terminals_for ((unsigned int) tid);
        if (tr) {

          for (std::vector<db::DeviceReconnectedTerminal>::const_iterator i = tr->begin (); i != tr->end (); ++i) {
            const db::DeviceAbstract *da = dm.first;
            if (i->device_index > 0) {
              da = device->other_abstracts () [i->device_index - 1].device_abstract;
            }
            Connections ref (net->cluster_id (), da->cluster_id_for_terminal (i->other_terminal_id));
            connections [insts [i->device_index]].push_back (ref);
          }

        }

      } else {

        Connections ref (net->cluster_id (), dm.first->cluster_id_for_terminal (tid));
        connections [insts [0]].push_back (ref);

      }

    }

  }

  circuit->add_device (device.release ());
}

bool
LayoutToNetlistStandardReader::read_trans_part (db::DCplxTrans &tr)
{
  if (test (skeys::location_key) || test (lkeys::location_key)) {

    Brace br2 (this);
    db::Coord x = read_coord ();
    db::Coord y = read_coord ();
    br2.done ();

    tr = db::DCplxTrans (tr.mag (), tr.angle (), tr.is_mirror (), db::DVector (m_dbu * x, m_dbu * y));
    return true;

  } else if (test (skeys::rotation_key) || test (lkeys::rotation_key)) {

    Brace br2 (this);
    double angle = read_double ();
    br2.done ();

    tr = db::DCplxTrans (tr.mag (), angle, tr.is_mirror (), tr.disp ());
    return true;

  } else if (test (skeys::mirror_key) || test (lkeys::mirror_key)) {

    tr = db::DCplxTrans (tr.mag (), tr.angle (), true, tr.disp ());
    return true;

  } else if (test (skeys::scale_key) || test (lkeys::scale_key)) {

    Brace br2 (this);
    double mag = read_double ();
    br2.done ();

    tr = db::DCplxTrans (mag, tr.angle (), tr.is_mirror (), tr.disp ());
    return true;

  }

  return false;
}

void
LayoutToNetlistStandardReader::read_subcircuit (db::Netlist *netlist, db::LayoutToNetlist *l2n, db::Circuit *circuit, ObjectMap &map, std::map<db::CellInstArray, std::list<Connections> > &connections)
{
  Brace br (this);

  std::list<Connections> refs;

  size_t id = size_t (read_int ());

  std::string name;

  std::string xname;
  read_word_or_quoted (xname);

  db::Circuit *circuit_ref = netlist->circuit_by_name (xname);
  if (! circuit_ref) {
    throw tl::Exception (tl::to_string (tr ("Not a valid device circuit name: ")) + xname);
  }

  std::unique_ptr<db::SubCircuit> subcircuit (new db::SubCircuit (circuit_ref));

  db::DCplxTrans trans;

  while (br) {

    if (test (skeys::name_key) || test (lkeys::name_key)) {

      Brace br_name (this);
      read_word_or_quoted (name);
      br_name.done ();

    } else if (read_trans_part (trans)) {

      //  .. nothing yet ..

    } else if (test (skeys::property_key) || test (lkeys::property_key)) {

      read_property (subcircuit.get ());

    } else if (test (skeys::pin_key) || test (lkeys::pin_key)) {

      Brace br2 (this);

      size_t pin_id = size_t (read_int ());

      unsigned int netid = (unsigned int) read_int ();
      br2.done ();

      const db::Pin *sc_pin = circuit_ref->pin_by_id (pin_id);
      if (! sc_pin) {
        throw tl::Exception (tl::to_string (tr ("Not a valid pin ID: ")) + tl::to_string (pin_id) + tl::to_string (tr (" for circuit: ")) + circuit_ref->name ());
      }

      db::Net *net = map.id2net [netid];
      if (!net) {
        throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
      }

      subcircuit->connect_pin (sc_pin->id (), net);
      db::Net *sc_net = circuit_ref->net_for_pin (sc_pin->id ());
      if (sc_net) {
        refs.push_back (Connections (net->cluster_id (), sc_net->cluster_id ()));
      }

    } else if (at_end ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside subcircuit definition (location, rotation, mirror, scale or pin expected)")));
    } else {
      skip_element ();
    }

  }

  br.done ();

  if (id > 0) {
    map.id2subcircuit.insert (std::make_pair ((unsigned int) id, subcircuit.get ()));
  }

  subcircuit->set_name (name);

  if (l2n) {

    subcircuit->set_trans (trans);

    db::CellInstArray inst (db::CellInst (circuit_ref->cell_index ()), db::CplxTrans (m_dbu).inverted () * trans * db::CplxTrans (m_dbu));
    db::Cell &ccell = l2n->internal_layout ()->cell (circuit->cell_index ());
    ccell.insert (inst);

    connections [inst] = refs;

  }

  circuit->add_subcircuit (subcircuit.release ());
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

  if (l2n) {

    db::connected_clusters<db::NetShape> &cc = l2n->net_clusters ().clusters_per_cell (dm->cell_index ());
    db::local_cluster<db::NetShape> &lc = *cc.insert ();
    dm->set_cluster_id_for_terminal (tid, lc.id ());

    db::Cell &cell = l2n->internal_layout ()->cell (dm->cell_index ());
    read_geometries (0, br, l2n, lc, cell);

  }

  br.done ();
}

}
