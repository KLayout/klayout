
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



#include "layDiffToolDialog.h"
#include "rdb.h"
#include "dbLayoutDiff.h"
#include "dbRecursiveShapeIterator.h"
#include "dbShapeProcessor.h"
#include "tlTimer.h"
#include "tlProgress.h"
#include "layCellView.h"
#include "layLayoutViewBase.h"
#include "tlExceptions.h"

#include "ui_DiffToolDialog.h"

#include <stdio.h>

namespace lay
{

std::string cfg_diff_run_xor ("diff-run-xor");
std::string cfg_diff_detailed ("diff-detailed");
std::string cfg_diff_smart ("diff-smart");
std::string cfg_diff_summarize ("diff-summarize");
std::string cfg_diff_expand_cell_arrays ("diff-expand-cell-arrays");
std::string cfg_diff_exact ("diff-exact");
std::string cfg_diff_ignore_duplicates ("diff-ignore-duplicates");

// ------------------------------------------------------------------------------
//  RdbDifferenceReceiver definition

class RdbDifferenceReceiver
  : public db::DifferenceReceiver
{
public:
  RdbDifferenceReceiver (const db::Layout &la, const db::Layout &lb, rdb::Database *rdb, bool detailed, bool with_properties, bool run_xor);

  void dbu_differs (double dbu_a, double dbu_b);
  void layer_in_a_only (const db::LayerProperties &la);
  void layer_in_b_only (const db::LayerProperties &lb);
  void layer_name_differs (const db::LayerProperties &la, const db::LayerProperties &lb);
  void cell_in_a_only (const std::string &cellname, db::cell_index_type ci);
  void cell_in_b_only (const std::string &cellname, db::cell_index_type ci);
  void cell_name_differs (const std::string &cellname_a, db::cell_index_type cia, const std::string &cellname_b, db::cell_index_type cib);
  void bbox_differs (const db::Box &ba, const db::Box &bb);
  void begin_cell (const std::string &cellname, db::cell_index_type cia, db::cell_index_type cib);
  void begin_inst_differences ();
  void instances_in_a (const std::vector <db::CellInstArrayWithProperties> &insts_a, const std::vector <std::string> &cell_names, const db::PropertiesRepository &props);
  void instances_in_b (const std::vector <db::CellInstArrayWithProperties> &insts_b, const std::vector <std::string> &cell_names, const db::PropertiesRepository &props);
  void instances_in_a_only (const std::vector <db::CellInstArrayWithProperties> &anotb, const db::Layout &a);
  void instances_in_b_only (const std::vector <db::CellInstArrayWithProperties> &bnota, const db::Layout &b);
  void begin_layer (const db::LayerProperties &layer, unsigned int layer_index_a, bool is_valid_a, unsigned int layer_index_b, bool is_valid_b);
  void end_layer ();
  void per_layer_bbox_differs (const db::Box &ba, const db::Box &bb);
  void begin_polygon_differences ();
  void detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &a, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &b);
  void begin_path_differences ();
  void detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Path, db::properties_id_type> > &a, const std::vector <std::pair <db::Path, db::properties_id_type> > &b);
  void begin_box_differences ();
  void detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Box, db::properties_id_type> > &a, const std::vector <std::pair <db::Box, db::properties_id_type> > &b);
  void begin_edge_differences ();
  void detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Edge, db::properties_id_type> > &a, const std::vector <std::pair <db::Edge, db::properties_id_type> > &b);
  void begin_text_differences ();
  void detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Text, db::properties_id_type> > &a, const std::vector <std::pair <db::Text, db::properties_id_type> > &b);

private:
  const db::Layout *mp_layout_a;
  const db::Layout *mp_layout_b;
  rdb::Database *mp_rdb;
  rdb::Cell *mp_cell;
  rdb::Cell *mp_topcell;
  rdb::Category *mp_general_cat;
  rdb::Category *mp_a_only_cat;
  rdb::Category *mp_b_only_cat;
  std::vector<rdb::Category *> mp_a_only_per_layer_cat;
  std::vector<rdb::Category *> mp_b_only_per_layer_cat;
  std::map<std::pair<int, int>, rdb::Category *> mp_xor_cat;
  std::vector<db::CellInstArrayWithProperties> m_insts_a, m_insts_b;

  std::string m_cellname;
  db::LayerProperties m_layer;
  unsigned int m_layer_index_a, m_layer_index_b;
  bool m_is_valid_layer_index_a, m_is_valid_layer_index_b;
  bool m_diffs_reported;
  bool m_with_properties;
  bool m_detailed;
  bool m_run_xor;
  db::ShapeProcessor m_ep;
  size_t m_obj_index;

  void produce_cell_inst (const db::CellInstArrayWithProperties &ci, const db::Layout *layout, const rdb::Category *cat);
  template <class SH> void produce_diffs_for_xor (const db::PropertiesRepository &pr, const std::vector <std::pair <SH, db::properties_id_type> > &a, const std::vector <std::pair <SH, db::properties_id_type> > &b, double dbu_a, db::Shapes &shapes);
  template <class SH> void produce_diffs (const db::PropertiesRepository &pr, const std::vector <std::pair <SH, db::properties_id_type> > &a, const std::vector <std::pair <SH, db::properties_id_type> > &b, double dbu_a, const rdb::Category *cat);
  template <class SH> void shape_diffs (const db::PropertiesRepository &pr, const std::vector <std::pair <SH, db::properties_id_type> > &a, const std::vector <std::pair <SH, db::properties_id_type> > &b);
  void shape_diffs_found ();
};

RdbDifferenceReceiver::RdbDifferenceReceiver (const db::Layout &layout_a, const db::Layout &layout_b, rdb::Database *rdb, bool detailed, bool with_properties, bool run_xor)
  : mp_layout_a (&layout_a), 
    mp_layout_b (&layout_b), 
    mp_rdb (rdb), 
    mp_cell (0), 
    m_cellname (), 
    m_layer (), 
    m_layer_index_a (0), 
    m_layer_index_b (0), 
    m_is_valid_layer_index_a (false), 
    m_is_valid_layer_index_b (false), 
    m_diffs_reported (false), 
    m_with_properties (with_properties), 
    m_detailed (detailed),
    m_run_xor (run_xor)
{
  mp_topcell = rdb->create_cell ("" /*dummy*/);
  mp_general_cat = rdb->create_category ("Summary");
  mp_general_cat->set_description (tl::to_string (QObject::tr ("Summary of Differences")));
  m_obj_index = 0;

  std::map<db::LayerProperties, std::pair<int, int>, db::LPLogicalLessFunc> layers;
  for (db::Layout::layer_iterator l = layout_a.begin_layers (); l != layout_a.end_layers (); ++l) {
    layers.insert (std::make_pair (*(*l).second, std::make_pair (-1, -1))).first->second.first = (*l).first;
  }
  for (db::Layout::layer_iterator l = layout_b.begin_layers (); l != layout_b.end_layers (); ++l) {
    layers.insert (std::make_pair (*(*l).second, std::make_pair (-1, -1))).first->second.second = (*l).first;
  }

  if (detailed) {

    rdb::Category *instances_cat = rdb->create_category ("Instances");
    instances_cat->set_description (tl::to_string (QObject::tr ("Differences in instances")));

    mp_a_only_cat = rdb->create_category (instances_cat, "A");
    mp_a_only_cat->set_description (tl::to_string (QObject::tr ("Instances in A but not in B")));

    mp_b_only_cat = rdb->create_category (instances_cat, "B");
    mp_b_only_cat->set_description (tl::to_string (QObject::tr ("Instances in B but not in A")));

    for (std::map<db::LayerProperties, std::pair<int, int>, db::LPLogicalLessFunc>::const_iterator l = layers.begin (); l != layers.end (); ++l) {

      while ((int) mp_a_only_per_layer_cat.size () <= l->second.first) {
        mp_a_only_per_layer_cat.push_back (0);
      }
      while ((int) mp_b_only_per_layer_cat.size () <= l->second.second) {
        mp_b_only_per_layer_cat.push_back (0);
      }

      rdb::Category *layer_cat = rdb->create_category (l->first.to_string ());
      layer_cat->set_description (tl::to_string (QObject::tr ("Differences in layer")) + " " + l->first.to_string ());

      if (l->second.first >= 0) {
        mp_a_only_per_layer_cat [(unsigned int) l->second.first] = rdb->create_category (layer_cat, "A");
        mp_a_only_per_layer_cat [(unsigned int) l->second.first]->set_description (tl::to_string (QObject::tr ("Shapes in A but not in B, on Layer ")) + l->first.to_string ());
      }

      if (l->second.second >= 0) {
        mp_b_only_per_layer_cat [(unsigned int) l->second.second] = rdb->create_category (layer_cat, "B");
        mp_b_only_per_layer_cat [(unsigned int) l->second.second]->set_description (tl::to_string (QObject::tr ("Shapes in B but not in A, on Layer ")) + l->first.to_string ());
      }

    }

  } else {
    mp_a_only_cat = 0;
    mp_b_only_cat = 0;
  }

  if (run_xor) {

    for (std::map<db::LayerProperties, std::pair<int, int>, db::LPLogicalLessFunc>::const_iterator l = layers.begin (); l != layers.end (); ++l) {

      rdb::Category *cat = rdb->create_category (std::string ("XOR ") + l->first.to_string ());
      cat->set_description (tl::to_string (QObject::tr ("Geometrical differences on layer")) + " " + l->first.to_string ());
      mp_xor_cat [l->second] = cat;

    }

  }
}

static void 
add_property_text (rdb::Item *item, const db::PropertiesRepository &pr, db::properties_id_type prop_id) 
{
  if (prop_id != 0) {
    const db::PropertiesRepository::properties_set &p = pr.properties (prop_id);
    for (db::PropertiesRepository::properties_set::const_iterator pp = p.begin (); pp != p.end (); ++pp) {
      const tl::Variant &name = pr.prop_name (pp->first);
      std::string r = std::string ("property: ") + name.to_string () + " = " + pp->second.to_string ();
      item->add_value (r);
    }
  }
}

void 
RdbDifferenceReceiver::produce_cell_inst (const db::CellInstArrayWithProperties &ci, const db::Layout *layout, const rdb::Category *cat)
{
  db::box_convert <db::CellInstArrayWithProperties> bc (*layout);

  rdb::Item *item = mp_rdb->create_item (mp_cell->id (), cat->id ());

  std::string r = "item: " + tl::sprintf (tl::to_string (QObject::tr ("instance: (%s) %s")), layout->cell_name (ci.object ().cell_index ()), ci.complex_trans ().to_string ());

  db::Vector a, b;
  unsigned long amax, bmax;
  if (ci.is_regular_array (a, b, amax, bmax)) {
    r += tl::sprintf (" [a=%s, b=%s, na=%ld, nb=%ld]", a.to_string (), b.to_string (), amax, bmax);
  } else if (ci.size () > 1) {
    r += " (+";
    r += tl::to_string (ci.size () - 1);
    r += " irregular placements)";
  }

  item->add_value (r);

  db::Box box = bc (ci);
  item->add_value (box * layout->dbu ());

  if (m_with_properties) {
    add_property_text (item, layout->properties_repository (), ci.properties_id ()); 
  }
}

void
RdbDifferenceReceiver::dbu_differs (double dbu_a, double dbu_b) 
{
  rdb::Item *item = mp_rdb->create_item (mp_topcell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Database units differ %g vs. %g")), dbu_a, dbu_b));
}

void 
RdbDifferenceReceiver::layer_in_a_only (const db::LayerProperties &la)
{
  rdb::Item *item = mp_rdb->create_item (mp_topcell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Layer %s is not present in layout B, but in A")), la.to_string ()));
}

void 
RdbDifferenceReceiver::layer_in_b_only (const db::LayerProperties &lb)
{
  rdb::Item *item = mp_rdb->create_item (mp_topcell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Layer %s is not present in layout A, but in B")), lb.to_string ()));
}

void
RdbDifferenceReceiver::layer_name_differs (const db::LayerProperties &la, const db::LayerProperties &lb)
{
  rdb::Item *item = mp_rdb->create_item (mp_topcell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Layer names differ between layout A and B for layer %d/%d: %s vs. %s")), la.layer, la.datatype, la.name, lb.name));
}

void 
RdbDifferenceReceiver::cell_in_a_only (const std::string &cellname, db::cell_index_type /*ci*/)
{
  rdb::Item *item = mp_rdb->create_item (mp_topcell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Cell %s is not present in layout B, but in A")), cellname));
}

void 
RdbDifferenceReceiver::cell_in_b_only (const std::string &cellname, db::cell_index_type /*ci*/)
{
  rdb::Item *item = mp_rdb->create_item (mp_topcell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Cell %s is not present in layout A, but in B")), cellname));
}

void 
RdbDifferenceReceiver::cell_name_differs (const std::string &cellname_a, db::cell_index_type /*cia*/, const std::string &cellname_b, db::cell_index_type /*cib*/)
{
  rdb::Item *item = mp_rdb->create_item (mp_topcell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Cell %s in A is renamed to %s in B")), cellname_a, cellname_b));
}

void 
RdbDifferenceReceiver::begin_cell (const std::string &cellname, db::cell_index_type /*cia*/, db::cell_index_type /*cib*/)
{
  mp_cell = mp_rdb->create_cell (cellname);
  m_diffs_reported = false;

  m_insts_a.clear ();
  m_insts_b.clear ();
}

void 
RdbDifferenceReceiver::bbox_differs (const db::Box &ba, const db::Box &bb)
{
  rdb::Item *item = mp_rdb->create_item (mp_cell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Bounding boxes differ: %s (A) vs. %s (B)")), ba.to_string (), bb.to_string ()));
}

void
RdbDifferenceReceiver::begin_inst_differences ()
{
  rdb::Item *item = mp_rdb->create_item (mp_cell->id (), mp_general_cat->id ());
  item->add_value (tl::to_string (QObject::tr ("Instances differ")));
}

void
RdbDifferenceReceiver::instances_in_a (const std::vector <db::CellInstArrayWithProperties> & /*insts_a*/, const std::vector <std::string> & /*cell_names*/, const db::PropertiesRepository & /*props*/)
{
  // .. nothing ..
}

void
RdbDifferenceReceiver::instances_in_b (const std::vector <db::CellInstArrayWithProperties> & /*insts_b*/, const std::vector <std::string> & /*cell_names*/, const db::PropertiesRepository & /*props*/)
{
  // .. nothing ..
}

void
RdbDifferenceReceiver::instances_in_a_only (const std::vector <db::CellInstArrayWithProperties> &anotb, const db::Layout & /*a*/)
{
  if (m_detailed) {
    for (std::vector <db::CellInstArrayWithProperties>::const_iterator s = anotb.begin (); s != anotb.end (); ++s) {
      produce_cell_inst (*s, mp_layout_a, mp_a_only_cat);
    }
  }

  if (m_run_xor) {
    m_insts_a.insert (m_insts_a.end (), anotb.begin (), anotb.end ());
  }
}

void
RdbDifferenceReceiver::instances_in_b_only (const std::vector <db::CellInstArrayWithProperties> &bnota, const db::Layout & /*b*/)
{
  if (m_detailed) {
    for (std::vector <db::CellInstArrayWithProperties>::const_iterator s = bnota.begin (); s != bnota.end (); ++s) {
      produce_cell_inst (*s, mp_layout_b, mp_b_only_cat);
    }
  }

  if (m_run_xor) {
    m_insts_b.insert (m_insts_b.end (), bnota.begin (), bnota.end ());
  }
}

void
RdbDifferenceReceiver::begin_layer (const db::LayerProperties &layer, unsigned int layer_index_a, bool is_valid_a, unsigned int layer_index_b, bool is_valid_b)
{
  m_layer = layer;
  m_diffs_reported = false;
  m_layer_index_a = layer_index_a;
  m_is_valid_layer_index_a = is_valid_a;
  m_layer_index_b = layer_index_b;
  m_is_valid_layer_index_b = is_valid_b;

  if (m_run_xor) {

    m_obj_index = 0;

    if (is_valid_a) {
      for (std::vector <db::CellInstArrayWithProperties>::const_iterator i = m_insts_a.begin (); i != m_insts_a.end (); ++i) {
        for (db::RecursiveShapeIterator shapes (*mp_layout_a, mp_layout_a->cell (i->object ().cell_index ()), m_layer_index_a); ! shapes.at_end (); ++shapes) {
          for (db::CellInstArray::iterator a = i->begin (); ! a.at_end (); ++a) {
            m_ep.insert (shapes.shape (), i->complex_trans (*a) * shapes.trans (), m_obj_index * 2);
            ++m_obj_index;
          }
        }
      }
    }

    if (is_valid_b) {
      for (std::vector <db::CellInstArrayWithProperties>::const_iterator i = m_insts_b.begin (); i != m_insts_b.end (); ++i) {
        for (db::RecursiveShapeIterator shapes (*mp_layout_b, mp_layout_b->cell (i->object ().cell_index ()), m_layer_index_b); ! shapes.at_end (); ++shapes) {
          for (db::CellInstArray::iterator a = i->begin (); ! a.at_end (); ++a) {
            m_ep.insert (shapes.shape (), i->complex_trans (*a) * shapes.trans (), m_obj_index * 2 + 1);
            ++m_obj_index;
          }
        }
      }
    }

  }
}

void
RdbDifferenceReceiver::end_layer ()
{
  if (m_run_xor) {

    std::vector <db::Polygon> out_polygons;
    db::BooleanOp op (db::BooleanOp::Xor);
    db::PolygonContainer pc (out_polygons);
    db::PolygonGenerator out (pc, false /*don't resolve holes*/, true /*min coherence*/);
    m_ep.process (out, op);

    db::CplxTrans t (mp_layout_a->dbu ());

    rdb::Category *cat = mp_xor_cat [std::make_pair (m_is_valid_layer_index_a ? m_layer_index_a : -1, m_is_valid_layer_index_b ? m_layer_index_b : -1)];
    if (cat) {
      for (std::vector <db::Polygon>::const_iterator x = out_polygons.begin (); x != out_polygons.end (); ++x) {
        rdb::Item *item = mp_rdb->create_item (mp_cell->id (), cat->id ());
        item->add_value (t * *x);
      }
    }

    m_ep.clear ();

  }
}

void 
RdbDifferenceReceiver::per_layer_bbox_differs (const db::Box &ba, const db::Box &bb)
{
  rdb::Item *item = mp_rdb->create_item (mp_cell->id (), mp_general_cat->id ());
  item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Per-layer bounding boxes differ (layer %s): %s (A) vs. %s (B)")), m_layer.to_string (), ba.to_string (), bb.to_string ()));
}

void
RdbDifferenceReceiver::shape_diffs_found ()
{
  if (! m_diffs_reported) {
    rdb::Item *item = mp_rdb->create_item (mp_cell->id (), mp_general_cat->id ());
    item->add_value (tl::sprintf (tl::to_string (QObject::tr ("Shapes differ on layer %s")), m_layer.to_string ()));
    m_diffs_reported = true;
  }
}

void
RdbDifferenceReceiver::begin_polygon_differences ()
{
  shape_diffs_found ();
}

inline std::string shape_type (const db::Polygon &) 
{
  return "polygon";
}

inline std::string shape_type (const db::Path &) 
{
  return "path";
}

inline std::string shape_type (const db::Edge &) 
{
  return "edge";
}

inline std::string shape_type (const db::Text &) 
{
  return "text";
}

inline std::string shape_type (const db::Box &) 
{
  return "box";
}

template <class SH>
void
RdbDifferenceReceiver::produce_diffs_for_xor (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <SH, db::properties_id_type> > &a, const std::vector <std::pair <SH, db::properties_id_type> > &b, double dbu_a, db::Shapes &shapes)
{
  db::CplxTrans t (dbu_a);
  std::vector <std::pair <SH, db::properties_id_type> > anotb;
  std::set_difference (a.begin (), a.end (), b.begin (), b.end (), std::back_inserter (anotb));
  for (typename std::vector <std::pair <SH, db::properties_id_type> >::const_iterator s = anotb.begin (); s != anotb.end (); ++s) {
    shapes.insert (s->first);
  }
}

template <class SH>
void
RdbDifferenceReceiver::produce_diffs (const db::PropertiesRepository &pr, const std::vector <std::pair <SH, db::properties_id_type> > &a, const std::vector <std::pair <SH, db::properties_id_type> > &b, double dbu_a, const rdb::Category *cat)
{
  db::CplxTrans t (dbu_a);
  std::vector <std::pair <SH, db::properties_id_type> > anotb;
  std::set_difference (a.begin (), a.end (), b.begin (), b.end (), std::back_inserter (anotb));
  for (typename std::vector <std::pair <SH, db::properties_id_type> >::const_iterator s = anotb.begin (); s != anotb.end (); ++s) {

    rdb::Item *item = mp_rdb->create_item (mp_cell->id (), cat->id ());

    if (s->second && m_with_properties) {
      item->add_value ("item: " + shape_type (s->first) + " " + tl::to_string (QObject::tr ("with properties")));
    } else {
      item->add_value ("item: " + shape_type (s->first));
    }

    item->add_value (t * s->first);

    if (s->second && m_with_properties) {
      add_property_text (item, pr, s->second); 
    }

  }
}

template <class SH> 
void 
RdbDifferenceReceiver::shape_diffs (const db::PropertiesRepository &pr, const std::vector <std::pair <SH, db::properties_id_type> > &a, const std::vector <std::pair <SH, db::properties_id_type> > &b)
{
  if (m_detailed && m_is_valid_layer_index_a && mp_a_only_per_layer_cat [m_layer_index_a] != 0) {
    produce_diffs (pr, a, b, mp_layout_a->dbu (), mp_a_only_per_layer_cat [m_layer_index_a]);
  }

  if (m_run_xor && m_is_valid_layer_index_a) {
    db::Shapes shapes;
    produce_diffs_for_xor (pr, a, b, mp_layout_a->dbu (), shapes);
    for (db::Shapes::shape_iterator s = shapes.begin (db::Shapes::shape_iterator::All); ! s.at_end (); ++s) {
      m_ep.insert (*s, m_obj_index * 2);
      ++m_obj_index;
    }
  }

  if (m_detailed && m_is_valid_layer_index_b && mp_b_only_per_layer_cat [m_layer_index_b] != 0) {
    produce_diffs (pr, b, a, mp_layout_b->dbu (), mp_b_only_per_layer_cat [m_layer_index_b]);
  }

  if (m_run_xor && m_is_valid_layer_index_b) {
    db::Shapes shapes;
    produce_diffs_for_xor (pr, b, a, mp_layout_b->dbu (), shapes);
    for (db::Shapes::shape_iterator s = shapes.begin (db::Shapes::shape_iterator::All); ! s.at_end (); ++s) {
      m_ep.insert (*s, m_obj_index * 2 + 1);
      ++m_obj_index;
    }
  }
}

void
RdbDifferenceReceiver::detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &a, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &b)
{
  shape_diffs (pr, a, b);
}

void
RdbDifferenceReceiver::begin_path_differences ()
{
  shape_diffs_found ();
}

void
RdbDifferenceReceiver::detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Path, db::properties_id_type> > &a, const std::vector <std::pair <db::Path, db::properties_id_type> > &b)
{
  shape_diffs (pr, a, b);
}

void
RdbDifferenceReceiver::begin_box_differences ()
{
  shape_diffs_found ();
}

void
RdbDifferenceReceiver::detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Box, db::properties_id_type> > &a, const std::vector <std::pair <db::Box, db::properties_id_type> > &b)
{
  shape_diffs (pr, a, b);
}

void
RdbDifferenceReceiver::begin_edge_differences ()
{
  shape_diffs_found ();
}

void
RdbDifferenceReceiver::detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Edge, db::properties_id_type> > &a, const std::vector <std::pair <db::Edge, db::properties_id_type> > &b)
{
  shape_diffs (pr, a, b);
}

void
RdbDifferenceReceiver::begin_text_differences ()
{
  shape_diffs_found ();
}

void
RdbDifferenceReceiver::detailed_diff (const db::PropertiesRepository &pr, const std::vector <std::pair <db::Text, db::properties_id_type> > &a, const std::vector <std::pair <db::Text, db::properties_id_type> > &b)
{
  shape_diffs (pr, a, b);
}

// ------------------------------------------------------------------------------
//  DiffToolDialog definition

DiffToolDialog::DiffToolDialog (QWidget *parent)
  : QDialog (parent), mp_view (0)
{
  mp_ui = new Ui::DiffToolDialog ();
  mp_ui->setupUi (this);

  connect (mp_ui->xor_cbx, SIGNAL (clicked ()), this, SLOT (xor_changed ()));
}

DiffToolDialog::~DiffToolDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

int 
DiffToolDialog::exec_dialog (lay::LayoutViewBase *view)
{
  mp_view = view;

  if (view != mp_ui->layouta->layout_view () || view != mp_ui->layoutb->layout_view ()) {

    mp_ui->layouta->set_layout_view (view);
    mp_ui->layoutb->set_layout_view (view);

    if (view->cellviews () >= 2) {
      mp_ui->layouta->set_current_cv_index (0);
      mp_ui->layoutb->set_current_cv_index (1);
    }

  } else {
    //  force update of the layer list
    //  TODO: the controls should register a listener for the view so this activity is not necessary:
    mp_ui->layouta->set_layout_view (view);
    mp_ui->layoutb->set_layout_view (view);
  }

  lay::Dispatcher *config_root = lay::Dispatcher::instance ();

  bool f = false;
  if (config_root->config_get (cfg_diff_run_xor, f)) {
    mp_ui->xor_cbx->setChecked (f);
  }
  if (config_root->config_get (cfg_diff_detailed, f)) {
    mp_ui->detailed_cbx->setChecked (f);
  }
  if (config_root->config_get (cfg_diff_smart, f)) {
    mp_ui->smart_cbx->setChecked (f);
  }
  if (config_root->config_get (cfg_diff_summarize, f)) {
    mp_ui->summarize_cbx->setChecked (f);
  }
  if (config_root->config_get (cfg_diff_expand_cell_arrays, f)) {
    mp_ui->expand_cell_arrays_cbx->setChecked (f);
  }
  if (config_root->config_get (cfg_diff_exact, f)) {
    mp_ui->exact_cbx->setChecked (f);
  }
  if (config_root->config_get (cfg_diff_ignore_duplicates, f)) {
    mp_ui->ignore_duplicates_cbx->setChecked (f);
  }

  update ();

  int ret = QDialog::exec ();

  if (ret) {
    run_diff ();
  }

  mp_view = 0;
  return ret;
}

void 
DiffToolDialog::accept ()
{
BEGIN_PROTECTED

  int cv_index_a = mp_ui->layouta->current_cv_index ();
  int cv_index_b = mp_ui->layoutb->current_cv_index ();

  const lay::CellView &cva = mp_view->cellview (cv_index_a);
  const lay::CellView &cvb = mp_view->cellview (cv_index_b);

  if (&cva->layout () == &cvb->layout () && cva.cell_index () == cvb.cell_index ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Trying to perform an Diff between identical layouts")));  
  }

  lay::Dispatcher *config_root = lay::Dispatcher::instance ();

  config_root->config_set (cfg_diff_run_xor, mp_ui->xor_cbx->isChecked ());
  config_root->config_set (cfg_diff_detailed, mp_ui->detailed_cbx->isChecked ());
  config_root->config_set (cfg_diff_smart, mp_ui->smart_cbx->isChecked ());
  config_root->config_set (cfg_diff_summarize, mp_ui->summarize_cbx->isChecked ());
  config_root->config_set (cfg_diff_expand_cell_arrays, mp_ui->expand_cell_arrays_cbx->isChecked ());
  config_root->config_set (cfg_diff_exact, mp_ui->exact_cbx->isChecked ());
  config_root->config_set (cfg_diff_ignore_duplicates, mp_ui->ignore_duplicates_cbx->isChecked ());
  config_root->config_end ();

  QDialog::accept ();

END_PROTECTED
}

void
DiffToolDialog::update ()
{
  bool xor_mode = mp_ui->xor_cbx->isChecked ();
  mp_ui->summarize_cbx->setEnabled (! xor_mode);
  mp_ui->detailed_cbx->setEnabled (! xor_mode);
  mp_ui->expand_cell_arrays_cbx->setEnabled (! xor_mode);
  mp_ui->exact_cbx->setEnabled (! xor_mode);
}

void 
DiffToolDialog::run_diff ()
{
  bool smart = mp_ui->smart_cbx->isChecked ();
  bool run_xor = mp_ui->xor_cbx->isChecked ();
  bool detailed = !run_xor && mp_ui->detailed_cbx->isChecked ();
  bool summarize = !run_xor && mp_ui->summarize_cbx->isChecked ();
  bool expand_cell_arrays = !run_xor && mp_ui->expand_cell_arrays_cbx->isChecked ();
  bool exact = !run_xor && mp_ui->exact_cbx->isChecked ();
  bool ignore_duplicates = mp_ui->ignore_duplicates_cbx->isChecked ();

  int cv_index_a = mp_ui->layouta->current_cv_index ();
  int cv_index_b = mp_ui->layoutb->current_cv_index ();

  lay::CellView cva = mp_view->cellview (cv_index_a);
  lay::CellView cvb = mp_view->cellview (cv_index_b);

  unsigned int flags = 0;
  if (detailed || run_xor) {
    flags |= db::layout_diff::f_verbose;
  }
  if (!exact) {
    flags |= db::layout_diff::f_no_text_details;
    flags |= db::layout_diff::f_no_layer_names;
    flags |= db::layout_diff::f_no_text_orientation;
    flags |= db::layout_diff::f_no_properties;
    flags |= db::layout_diff::f_boxes_as_polygons;
    flags |= db::layout_diff::f_paths_as_polygons;
  }
  if (expand_cell_arrays) {
    flags |= db::layout_diff::f_flatten_array_insts;
  }
  if (! summarize) {
    flags |= db::layout_diff::f_dont_summarize_missing_layers;
  }
  if (smart) {
    flags |= db::layout_diff::f_smart_cell_mapping;
  }
  if (ignore_duplicates) {
    flags |= db::layout_diff::f_ignore_duplicates;
  }

  //  TODO: make an parameter
  db::Coord tolerance = 0;

  //  Create the report database or identify the output layout
  rdb::Database *rdb = 0;
  int rdb_index = 0;

  rdb = new rdb::Database ();
  rdb->set_name ("Diff " + cva->name () + "/" + cvb->name ());
  rdb->set_top_cell_name (cva->layout ().cell_name (cva.cell_index ()));

  rdb_index = mp_view->add_rdb (rdb);

  std::string srca = cva->name () + ", Cell " + cva->layout ().cell_name (cva.cell_index ());
  std::string srcb = cvb->name () + ", Cell " + cvb->layout ().cell_name (cvb.cell_index ());
  rdb->set_description ("Diff of '" + srca + "' vs. '" + srcb + "'");

  RdbDifferenceReceiver r (cva->layout (), cvb->layout (), rdb, detailed, exact, run_xor);

  db::compare_layouts (cva->layout (), cva.cell_index (), cvb->layout (), cvb.cell_index (), flags, tolerance, r);

  mp_view->open_rdb_browser (rdb_index, cv_index_a);
  mp_view->update_content ();
}

}

