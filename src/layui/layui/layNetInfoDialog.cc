
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

#if defined(HAVE_QT)

#include "layNetInfoDialog.h"
#include "tlXMLWriter.h"

#include "ui_NetInfoDialog.h"

#include <sstream>

namespace lay
{

NetInfoDialog::NetInfoDialog (QWidget *parent)
  : QDialog (parent), m_needs_update (false)
{
  ui = new Ui::NetInfoDialog ();
  ui->setupUi (this);

  connect (ui->detailed_cb, SIGNAL (stateChanged (int)), this, SLOT (detailed_checkbox_clicked ()));
}

NetInfoDialog::~NetInfoDialog ()
{
  delete ui;
  ui = 0;
}

void NetInfoDialog::needs_update ()
{
  if (! isVisible ()) {
    m_needs_update = true;
  } else {
    update_info_text ();
    m_needs_update = false;
  }
}

void NetInfoDialog::set_nets (const db::LayoutToNetlist *l2ndb, const std::vector<const db::Net *> &nets)
{
  mp_l2ndb = const_cast<db::LayoutToNetlist *> (l2ndb);
  mp_nets.clear ();
  for (std::vector<const db::Net *>::const_iterator n = nets.begin (); n != nets.end (); ++n) {
    mp_nets.push_back (const_cast<db::Net *> (*n));
  }
  needs_update ();
}

void NetInfoDialog::detailed_checkbox_clicked ()
{
  needs_update ();
}

void NetInfoDialog::showEvent (QShowEvent * /*event*/)
{
  if (isVisible () && m_needs_update) {
    needs_update ();
  }
}

size_t count_shapes (db::LayoutToNetlist *l2ndb, db::Net *net, unsigned int layer)
{
  if (! net || ! net->circuit ()) {
    return 0;
  }

  db::cell_index_type cell_index = net->circuit ()->cell_index ();
  size_t cluster_id = net->cluster_id ();

  size_t n = 0;
  for (db::recursive_cluster_shape_iterator<db::NetShape> shapes (l2ndb->net_clusters (), layer, cell_index, cluster_id); ! shapes.at_end (); ++shapes) {
    ++n;
  }
  return n;
}

size_t count_shapes (db::LayoutToNetlist *l2ndb, db::Net *net)
{
  size_t n = 0;

  const db::Connectivity &conn = l2ndb->connectivity ();
  for (db::Connectivity::all_layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {
    n += count_shapes (l2ndb, net, *layer);
  }

  return n;
}

static std::string layer_string (const db::LayoutToNetlist *l2ndb, unsigned int layer)
{
  const db::Layout *ly = l2ndb->internal_layout ();

  db::LayerProperties lp = ly->get_properties (layer);
  std::string l = l2ndb->name (layer);
  if (! lp.is_null ()) {
    if (! l.empty ()) {
      l += " ";
    }
    l += lp.to_string ();
  }

  if (l.empty ()) {
    return "<anonymous>";
  } else {
    return l;
  }
}

void NetInfoDialog::update_info_text ()
{
  bool detailed = ui->detailed_cb->isChecked ();

  std::ostringstream info_stream;
  info_stream.imbue (std::locale ("C"));

  tl::XMLWriter info (info_stream);

  info.start_document ("");
  info.start_element ("html");
  info.start_element ("body");

  if (! mp_l2ndb.get () || mp_nets.empty () || ! mp_l2ndb->internal_layout ()) {

    info.start_element ("p");
    info.cdata (tl::to_string (QObject::tr ("No net selected")));
    info.end_element ("p");

  } else {

    info.start_element ("table");
    info.write_attribute ("cellspacing", "6");

    info.start_element ("tr");
    info.start_element ("th");
    info.cdata (tl::to_string (tr ("Net")));
    info.end_element ("th");
    info.start_element ("th");
    info.cdata (tl::to_string (tr ("Circuit")));
    info.end_element ("th");
    info.start_element ("th");
    info.cdata (tl::to_string (tr ("Shapes")));
    info.end_element ("th");
    info.start_element ("th");
    info.cdata (tl::to_string (tr ("Device terminals")));
    info.end_element ("th");
    info.start_element ("th");
    info.cdata (tl::to_string (tr ("Subcircuit pins")));
    info.end_element ("th");
    info.start_element ("th");
    info.cdata (tl::to_string (tr ("Circuit pins")));
    info.end_element ("th");
    info.end_element ("tr");

    size_t shapes = 0, terminals = 0, pins = 0, subcircuit_pins = 0;

    for (tl::weak_collection<db::Net>::iterator net = mp_nets.begin (); net != mp_nets.end (); ++net) {

      info.start_element ("tr");

      info.start_element ("td");
      info.cdata (net->expanded_name ());
      info.end_element ("td");

      info.start_element ("td");
      if (net->circuit ()) {
        info.cdata (net->circuit ()->name ());
      }
      info.end_element ("td");

      size_t n;

      info.start_element ("td");
      n = count_shapes (mp_l2ndb.get (), net.operator-> ());
      shapes += n;
      info.cdata (tl::to_string (n));
      info.end_element ("td");

      info.start_element ("td");
      n = net->terminal_count ();
      terminals += n;
      info.cdata (tl::to_string (n));
      info.end_element ("td");

      info.start_element ("td");
      n = net->subcircuit_pin_count ();
      subcircuit_pins += n;
      info.cdata (tl::to_string (n));
      info.end_element ("td");

      info.start_element ("td");
      n = net->pin_count ();
      pins += n;
      info.cdata (tl::to_string (n));
      info.end_element ("td");

      info.end_element ("tr");

    }

    if (mp_nets.size () > 1) {

      info.start_element ("tr");

      info.start_element ("td");
      info.cdata (tl::to_string (tr ("(total)")));
      info.end_element ("td");

      info.start_element ("td");
      info.cdata (tl::to_string (shapes));
      info.end_element ("td");

      info.start_element ("td");
      info.cdata (tl::to_string (terminals));
      info.end_element ("td");

      info.start_element ("td");
      info.cdata (tl::to_string (subcircuit_pins));
      info.end_element ("td");

      info.start_element ("td");
      info.cdata (tl::to_string (pins));
      info.end_element ("td");

      info.end_element ("tr");

    }

    info.end_element ("table");

    if (mp_nets.size () == 1 && detailed) {

      const db::Net *net = mp_nets.front ();
      const db::Layout *ly = mp_l2ndb->internal_layout ();
      db::cell_index_type cell_index = net->circuit ()->cell_index ();
      size_t cluster_id = net->cluster_id ();

      double dbu_unidir = ly->dbu ();
      db::CplxTrans dbu (ly->dbu ());
      db::VCplxTrans dbuinv = dbu.inverted ();

      size_t max_shapes = 2000;

      info.start_element ("hr");
      info.end_element ("hr");

      std::map<std::string, std::set<std::string> > shapes;

      //  map as (layernumber, group of shapes by layer):
      std::map<unsigned int, std::vector<db::Polygon> > shapes_by_layer;
      std::map<unsigned int, std::string> layer_names;
      std::map<unsigned int, db::coord_traits<db::Coord>::area_type> statinfo_area;
      std::map<unsigned int, db::coord_traits<db::Coord>::perimeter_type> statinfo_perimeter;

      size_t tot_shapes = 0;
      bool incomplete = false;

      const db::Connectivity &conn = mp_l2ndb->connectivity ();
      for (db::Connectivity::all_layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {

        std::string l = layer_string (mp_l2ndb.get (), *layer);

        for (db::recursive_cluster_shape_iterator<db::NetShape> si (mp_l2ndb->net_clusters (), *layer, cell_index, cluster_id); ! si.at_end (); ++si) {

          if (si->type () != db::NetShape::Polygon) {
            continue;
          }

          if (tot_shapes++ >= max_shapes) {
            incomplete = true;
            break;
          }

          //  Check if layer is already detected, otherwise create vector-of-Shape object to hold shapes
          //  plus initialize the perimeter and area sums
          std::map<unsigned int, std::vector<db::Polygon> >::iterator s = shapes_by_layer.find (*layer);
          if (s == shapes_by_layer.end ()) {
            s = shapes_by_layer.insert (std::make_pair (*layer, std::vector<db::Polygon> ())).first;
            layer_names.insert (std::make_pair (*layer, l));
            statinfo_perimeter.insert (std::make_pair (*layer, db::coord_traits<db::Coord>::perimeter_type (0)));
            statinfo_area.insert (std::make_pair (*layer, db::coord_traits<db::Coord>::area_type (0)));
          }

          s->second.push_back (si->polygon_ref ().instantiate ());

          std::string c (ly->cell_name (si.cell_index ()));
          c += " (with ";
          c += (dbu * db::CplxTrans (si.trans ()) * dbuinv).to_string ();
          c += ")";

          std::string t;
          if (s->second.back ().is_box ()) {
            t = tl::to_string (QObject::tr ("box on ")) + l + ": " + (dbu * s->second.back ().box ()).to_string ();
          } else {
            t = tl::to_string (QObject::tr ("polygon on ")) + l + ": " + (dbu * s->second.back ()).to_string ();
          }

          shapes.insert (std::make_pair (c, std::set<std::string> ())).first->second.insert (t);

        }

      }

      //  Try to merge all shaped to polygons, use Map of (layernumber, group of polygons by layer)
      std::map<unsigned int, std::vector<db::Polygon> > polygons_by_layer;
      for (std::map<unsigned int, std::vector<db::Polygon> >::iterator i = shapes_by_layer.begin(); i != shapes_by_layer.end (); ++i) {

        unsigned int l = i->first;

        db::EdgeProcessor ep;
        std::vector <db::Polygon> &merged = polygons_by_layer.insert (std::make_pair (l, std::vector <db::Polygon> ())).first->second;
        ep.merge(i->second, merged, 0, true, true);

        db::coord_traits<db::Coord>::area_type area = 0;
        db::coord_traits<db::Coord>::perimeter_type perimeter = 0;

        //  Despite merging, a multitude of separate non-touching polygons can exist.
        for (std::vector <db::Polygon>::iterator j = merged.begin (); j != merged.end (); ++j) {
          //  Sum area
          area += j->area ();
          //  Sum perimeter for the merged polygon
          perimeter += j->perimeter ();
        }

        statinfo_area [l] += area;
        statinfo_perimeter [l] += perimeter;

      }

      if (! shapes.empty ()) {

        if (! incomplete) {

          info.start_element ("h3");
          info.cdata (tl::to_string (QObject::tr ("Geometry:")));
          info.end_element ("h3");

          db::coord_traits<db::Coord>::area_type total_area = 0;
          db::coord_traits<db::Coord>::perimeter_type total_perimeter = 0;
          size_t nshapes = 0;

          //  Print perimeter and area and sum up total
          info.start_element ("table");
          info.write_attribute ("cellspacing", "6");

          info.start_element ("tr");
          info.start_element ("td");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Layer")));
          info.end_element ("b");
          info.end_element ("td");
          info.start_element ("td");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Shapes")));
          info.end_element ("b");
          info.end_element ("td");
          info.start_element ("td");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Perimeter")));
          info.start_element ("br");
          info.end_element ("br");
          info.cdata (tl::to_string (QObject::tr ("(micron)")));
          info.end_element ("b");
          info.end_element ("td");
          info.start_element ("td");
          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Area")));
          info.start_element ("br");
          info.end_element ("br");
          info.cdata (tl::to_string (QObject::tr ("(square micron)")));
          info.end_element ("b");
          info.end_element ("td");
          info.end_element ("tr");

          for (std::map<unsigned int, db::coord_traits<db::Coord>::area_type>::iterator i = statinfo_area.begin (); i != statinfo_area.end(); ++i) {

            unsigned int l = i->first;
            size_t n;
            double v;

            info.start_element ("tr");
            info.start_element ("td");
            info.cdata (layer_names [l]);
            info.end_element ("td");
            info.start_element ("td");
            n = shapes_by_layer [l].size ();
            nshapes += n;
            info.cdata (tl::to_string (n));
            info.end_element ("td");
            info.start_element ("td");
            v = statinfo_perimeter [l];
            total_perimeter += v;
            info.cdata (tl::micron_to_string (v * dbu_unidir));
            info.end_element ("td");
            info.start_element ("td");
            v = statinfo_area[l];
            total_area += v;
            info.cdata (tl::to_string (v * dbu_unidir * dbu_unidir));
            info.end_element ("td");
            info.end_element ("tr");

          }

          //  Only if more than one layer is involved, print summed values
          if (statinfo_area.size () != 1) {

            info.start_element ("tr");
            info.start_element ("td");
            info.cdata (tl::to_string (QObject::tr ("(total)")));
            info.end_element ("td");
            info.start_element ("td");
            info.cdata (tl::to_string (nshapes));
            info.end_element ("td");
            info.start_element ("td");
            info.cdata (tl::micron_to_string (total_perimeter * dbu_unidir));
            info.end_element ("td");
            info.start_element ("td");
            info.cdata (tl::to_string (total_area * dbu_unidir * dbu_unidir));
            info.end_element ("td");
            info.end_element ("tr");

          }

          info.end_element ("table");

        }

        info.start_element ("h3");
        info.cdata (tl::to_string (QObject::tr ("Shapes:")));
        info.end_element ("h3");

        for (std::map<std::string, std::set<std::string> >::const_iterator s = shapes.begin (); s != shapes.end (); ++s) {

          info.start_element ("p");

          info.start_element ("b");
          info.cdata (tl::to_string (QObject::tr ("Cell ")));
          info.cdata (s->first);
          info.cdata (":");
          info.end_element ("b");

          for (std::set <std::string>::const_iterator l = s->second.begin (); l != s->second.end (); ++l) {
            info.start_element ("br");
            info.end_element ("br");
            info.cdata (*l);
          }

          info.end_element ("p");

        }

        if (incomplete) {
          info.start_element ("p");
          info.cdata ("...");
          info.end_element ("p");
        }

      }

    }

  }

  info.end_element ("body");
  info.end_element ("html");

  ui->net_info_text->setHtml (tl::to_qstring (info_stream.str ()));
}

}

#endif
