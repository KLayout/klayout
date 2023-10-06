
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


#include "dbLEFImporter.h"

#include "tlStream.h"

#include <cctype>

namespace db
{

// -----------------------------------------------------------------------------------
//  LEFImporter implementation

LEFImporter::LEFImporter (int warn_level)
  : LEFDEFImporter (warn_level)
{
  //  .. nothing yet ..
}

LEFImporter::~LEFImporter ()
{
  //  .. nothing yet ..
}

double
LEFImporter::layer_ext (const std::string &layer, double def_ext) const
{
  std::map<std::string, double>::const_iterator l = m_default_ext.find (layer);
  if (l != m_default_ext.end ()) {
    return l->second;
  } else {
    return def_ext;
  }
}

std::pair<double, double>
LEFImporter::min_layer_width (const std::string &layer) const
{
  std::map<std::string, std::pair<double, double> >::const_iterator l = m_min_widths.find (layer);
  if (l != m_min_widths.end ()) {
    return l->second;
  } else {
    return std::make_pair (0.0, 0.0);
  }
}

std::pair<double, double>
LEFImporter::layer_width (const std::string &layer, const std::string &nondefaultrule, const std::pair<double, double> &def_width) const
{
  std::map<std::string, std::map<std::string, std::pair<double, double> > >::const_iterator nd = m_nondefault_widths.find (nondefaultrule);

  std::map<std::string, std::pair<double, double> >::const_iterator l;
  bool has_width = false;

  if (! nondefaultrule.empty () && nd != m_nondefault_widths.end ()) {
    l = nd->second.find (layer);
    if (l != nd->second.end ()) {
      has_width = true;
    }
  }

  if (! has_width) {
    l = m_default_widths.find (layer);
    if (l != m_default_widths.end ()) {
      has_width = true;
    }
  }

  if (has_width) {
    return l->second;
  } else {
    return def_width;
  }
}

std::vector <db::Trans> 
LEFImporter::get_iteration (double dbu)
{
  test ("DO");
  long nx = get_long ();
  test ("BY");
  long ny = get_long ();

  test ("STEP");
  double dx = get_double ();
  double dy = get_double ();

  std::vector <db::Trans> t;
  for (long i = 0; i < nx; ++i) {
    for (long j = 0; j < ny; ++j) {
      t.push_back (db::Trans (db::Vector (db::DVector (dx * i / dbu, dy * j / dbu))));
    }
  }
  return t;
}

template <class Shape, class Trans>
static db::Shape insert_shape (db::Cell &cell, unsigned int layer_id, const Shape &shape, const Trans &trans, db::properties_id_type prop_id)
{
  if (prop_id == 0) {
    return cell.shapes (layer_id).insert (shape.transformed (trans));
  } else {
    return cell.shapes (layer_id).insert (db::object_with_properties<Shape> (shape.transformed (trans), prop_id));
  }
}

template <class Shape>
static db::Shape insert_shape (db::Cell &cell, unsigned int layer_id, const Shape &shape, db::properties_id_type prop_id)
{
  if (prop_id == 0) {
    return cell.shapes (layer_id).insert (shape);
  } else {
    return cell.shapes (layer_id).insert (db::object_with_properties<Shape> (shape, prop_id));
  }
}

static db::Box box_for_label (const db::Polygon &p)
{
  if (p.is_box ()) {
    return p.box ();
  } else if (p.begin_hull () != p.end_hull ()) {
    return db::Box (*p.begin_hull (), *p.begin_hull ());
  } else {
    return db::Box ();
  }
}

static db::Box box_for_label (const db::Path &p)
{
  if (p.begin () != p.end ()) {
    return db::Box (*p.begin (), *p.begin ());
  } else {
    return db::Box ();
  }
}

void
LEFImporter::read_geometries (GeometryBasedLayoutGenerator *lg, double dbu, LayerPurpose purpose, std::map<std::string, db::Box> *collect_boxes_for_labels, db::properties_id_type prop_id)
{
  std::string layer_name;
  double w = 0.0;

  while (true) {

    if (test ("CLASS")) {

      //  accept CLASS token for PORT definitions
      while (! at_end () && ! test (";")) {
        take ();
      }

    } else if (test ("LAYER")) {

      layer_name = get ();

      w = 0.0;
      std::map<std::string, std::pair<double, double> >::const_iterator dw = m_default_widths.find (layer_name);
      if (dw != m_default_widths.end ()) {
        w = dw->second.first;
      }

      while (! at_end () && ! test (";")) {
        take ();
      }

    } else if (test ("WIDTH")) {

      w = get_double ();
      expect (";");

    } else if (test ("PATH")) {

      std::vector<db::Point> points;

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      bool iterate = test ("ITERATE");

      while (! peek (";") && ! peek ("DO")) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");
      }

      if (lg) {

        db::Coord iw = db::coord_traits<db::Coord>::rounded (w / dbu);
        db::Path p (points.begin (), points.end (), iw, iw / 2, iw / 2, false);

        if (iterate) {

          std::vector<db::Trans> ti = get_iteration (dbu);

          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            db::Path pt = p.transformed (*t);
            lg->add_path (layer_name, purpose, pt, mask, prop_id);
            if (collect_boxes_for_labels) {
              collect_boxes_for_labels->insert (std::make_pair (layer_name, db::Box ())).first->second = box_for_label (pt);
            }
          }

        } else {

          lg->add_path (layer_name, purpose, p, mask, prop_id);
          if (collect_boxes_for_labels) {
            collect_boxes_for_labels->insert (std::make_pair (layer_name, db::Box ())).first->second = box_for_label (p);
          }

        }

      }

      expect (";");

    } else if (test ("POLYGON")) {

      std::vector<db::Point> points;

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      bool iterate = test ("ITERATE");

      while (! peek (";") && ! peek ("DO")) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");
      }

      if (lg) {

        db::Polygon p;
        p.assign_hull (points.begin (), points.end ());

        if (iterate) {

          std::vector<db::Trans> ti = get_iteration (dbu);
          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            db::Polygon pt = p.transformed (*t);
            lg->add_polygon (layer_name, purpose, pt, mask, prop_id);
            if (collect_boxes_for_labels) {
              collect_boxes_for_labels->insert (std::make_pair (layer_name, db::Box ())).first->second = box_for_label (pt);
            }
          }

        } else {

          lg->add_polygon (layer_name, purpose, p, mask, prop_id);
          if (collect_boxes_for_labels) {
            collect_boxes_for_labels->insert (std::make_pair (layer_name, db::Box ())).first->second = box_for_label (p);
          }

        }

      }

      expect (";");

    } else if (test ("RECT")) {

      std::vector<db::Point> points;

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      bool iterate = test ("ITERATE");

      for (int i = 0; i < 2; ++i) {

        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");

      }

      if (lg) {

        db::Box b (points [0], points [1]);

        if (iterate) {

          std::vector<db::Trans> ti = get_iteration (dbu);

          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            db::Box bt = b.transformed (*t);
            lg->add_box (layer_name, purpose, bt, mask, prop_id);
            if (collect_boxes_for_labels) {
              collect_boxes_for_labels->insert (std::make_pair (layer_name, db::Box ())).first->second = bt;
            }
          }

        } else {

          lg->add_box (layer_name, purpose, b, mask, prop_id);
          if (collect_boxes_for_labels) {
            collect_boxes_for_labels->insert (std::make_pair (layer_name, db::Box ())).first->second = b;
          }

        }

      }

      expect (";");

    } else if (test ("VIA")) {

      std::vector<db::Vector> points;

      //  note: the 5.8 spec says ITERATE comes before MASK for VIA
      bool iterate = test ("ITERATE");

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      unsigned int mask_bottom = mask % 10;
      unsigned int mask_cut = (mask / 10) % 10;
      unsigned int mask_top = (mask / 100) % 10;

      double x = 0.0, y = 0.0;
      if (test ("(")) {
        x = get_double ();
        y = get_double ();
        test (")");
      } else {
        x = get_double ();
        y = get_double ();
      }
      points.push_back (db::Vector (db::DVector (x / dbu, y / dbu)));

      std::string vn = get ();

      if (lg) {

        if (iterate) {

          std::vector<db::Trans> ti = get_iteration (dbu);
          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            lg->add_via (vn, *t * db::Trans (points [0]), mask_bottom, mask_cut, mask_top);
          }

        } else {

          lg->add_via (vn, db::Trans (points [0]), mask_bottom, mask_cut, mask_top);

        }

      }

      expect (";");

    } else if (test ("PROPERTY")) {

      //  skip properties
      skip_entry ();

    } else {
      //  stop at unknown token
      break;
    }

  }
}

void
LEFImporter::read_nondefaultrule (db::Layout &layout)
{
  //  read NONDEFAULTRULE sections
  std::string n = get ();

  while (! at_end () && ! test ("END")) {

    if (test ("LAYER")) {

      std::string l = get ();

      //  read the width for the layer
      while (! at_end () && ! test ("END")) {
        if (test ("WIDTH")) {
          double w = get_double ();
          test (";");
          m_nondefault_widths[n][l] = std::make_pair (w, w);
        } else {
          skip_entry ();
        }
      }

      test (l);

    } else if (test ("VIA")) {

      read_viadef (layout, n);

    } else {

      std::string token = get ();

      if (token == "SPACING") {
        //  read over sections we do not need
        while (! at_end () && ! test ("END")) {
          skip_entry ();
        }
        test (token);
      } else if (token != ";") {
        //  read over lines we do not need
        skip_entry ();
      }

    }

  }

  test (n);
}

void
LEFImporter::read_viadef_by_rule (RuleBasedViaGenerator *vg, ViaDesc &via_desc, const std::string & /*n*/, double dbu)
{
  while (! at_end () && ! test ("END")) {

    double x, y;

    if (test ("CUTSIZE")) {

      x = get_double ();
      y = get_double ();
      vg->set_cutsize (db::Vector (db::DVector (x / dbu, y / dbu)));

      test (";");

    } else if (test ("CUTSPACING")) {

      x = get_double ();
      y = get_double ();
      vg->set_cutspacing (db::Vector (db::DVector (x / dbu, y / dbu)));

      test (";");

    } else if (test ("ORIGIN")) {

      x = get_double ();
      y = get_double ();
      vg->set_offset (db::Point (db::DPoint (x / dbu, y / dbu)));

      test (";");

    } else if (test ("ENCLOSURE")) {

      x = get_double ();
      y = get_double ();
      vg->set_be (db::Vector (db::DVector (x / dbu, y / dbu)));

      x = get_double ();
      y = get_double ();
      vg->set_te (db::Vector (db::DVector (x / dbu, y / dbu)));

      test (";");

    } else if (test ("OFFSET")) {

      x = get_double ();
      y = get_double ();
      vg->set_bo (db::Vector (db::DVector (x / dbu, y / dbu)));

      x = get_double ();
      y = get_double ();
      vg->set_to (db::Vector (db::DVector (x / dbu, y / dbu)));

      test (";");

    } else if (test ("ROWCOL")) {

      vg->set_rows (get_long ());
      vg->set_columns (get_long ());

      test (";");

    } else if (test ("PATTERN")) {

      vg->set_pattern (get ());

      test (";");

    } else if (test ("LAYERS")) {

      std::string lb, lc, lt;
      lb = get ();
      lc = get ();
      lt = get ();
      via_desc.m1 = lb;
      via_desc.m2 = lt;

      vg->set_bottom_layer (lb);
      vg->set_cut_layer (lc);
      vg->set_top_layer (lt);

      test (";");

    } else {

      while (! at_end () && ! test (";")) {
        take ();
      }

    }

  }
}

template <class Shape>
static db::DVector
via_size (double dbu, const Shape &shape)
{
  db::Box box = db::box_convert<Shape> () (shape);
  return db::DVector (box.width () * dbu, box.height () * dbu);
}

void
LEFImporter::read_viadef_by_geometry (GeometryBasedLayoutGenerator *lg, ViaDesc &via_desc, const std::string &n, double dbu)
{
  std::string layer_name;
  std::set<std::string> seen_layers;
  std::vector<std::string> routing_layers;

  while (true) {

    //  ignore resistance spec
    if (test ("RESISTANCE")) {

      get_double ();
      test (";");

    } else if (test ("FOREIGN")) {

      //  undocumented
      while (! at_end () && ! test (";")) {
        take ();
      }

    } else if (test ("LAYER")) {

      layer_name = get ();

      if (m_routing_layers.find (layer_name) != m_routing_layers.end ()) {

        if (routing_layers.size () == 0) {
          lg->set_maskshift_layer (0, layer_name);
        } else if (routing_layers.size () == 1) {
          lg->set_maskshift_layer (2, layer_name);
        }

        if (seen_layers.find (layer_name) == seen_layers.end ()) {
          seen_layers.insert (layer_name);
          routing_layers.push_back (layer_name);
        }

      } else {
        lg->set_maskshift_layer (1, layer_name);
      }

      while (! at_end () && ! test (";")) {
        take ();
      }

    } else if (test ("POLYGON")) {

      std::vector<db::Point> points;

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      while (! peek (";")) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");
      }

      db::Polygon p;
      p.assign_hull (points.begin (), points.end ());

      lg->add_polygon (layer_name, ViaGeometry, p, mask, 0, via_size (dbu, p));

      expect (";");

    } else if (test ("RECT")) {

      std::vector<db::Point> points;

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      for (int i = 0; i < 2; ++i) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");
      }

      db::Box b (points [0], points [1]);
      lg->add_box (layer_name, ViaGeometry, b, mask, 0, via_size (dbu, b));

      expect (";");

    } else if (test ("PROPERTY")) {

      //  skip properties
      skip_entry ();

    } else {
      //  stop at unknown token
      break;
    }

  }

  //  determine m1 and m2 layers

  if (routing_layers.size () == 2) {
    via_desc.m1 = routing_layers[0];
    via_desc.m2 = routing_layers[1];
  } else {
    warn (tl::to_string (tr ("Can't determine routing layers for via: ")) + n);
  }

  reset_cellname ();

  expect ("END");
}

void
LEFImporter::read_viadef (Layout &layout, const std::string &nondefaultrule)
{
  std::string n = get ();

  ViaDesc &via_desc = m_vias[n];

  while (test ("DEFAULT") || test ("TOPOFSTACKONLY") || test("GENERATED"))
    ;
  test (";");

  if (test ("VIARULE")) {
    std::unique_ptr<RuleBasedViaGenerator> vg (new RuleBasedViaGenerator ());
    read_viadef_by_rule (vg.get (), via_desc, n, layout.dbu ());
    reader_state ()->register_via_cell (n, nondefaultrule, vg.release ());
  } else {
    std::unique_ptr<GeometryBasedLayoutGenerator> vg (new GeometryBasedLayoutGenerator ());
    read_viadef_by_geometry (vg.get (), via_desc, n, layout.dbu ());
    reader_state ()->register_via_cell (n, nondefaultrule, vg.release ());
  }

  test ("VIA");
  expect (n);
}

void
LEFImporter::read_layer (Layout & /*layout*/)
{
  std::string ln = get ();
  double wmin = 0.0, wmin_wrongdir = 0.0;
  double w = 0.0, w_wrongdir = 0.0;
  bool is_horizontal = false;

  register_layer (ln);

  //  just extract the width from the layer - we need that as the default width for paths
  while (! at_end ()) {

    if (test ("END")) {

      expect (ln);
      break;

    } else if (test ("TYPE")) {

      std::string type = get ();

      if (type == "ROUTING" || type == "MASTERSLICE") {
        m_routing_layers.insert (ln);
      } else if (type == "CUT") {
        m_cut_layers.insert (ln);
      } else if (type == "OVERLAP") {
        m_overlap_layers.insert (ln);
      }
      expect (";");

    } else if (test ("MASK")) {

      unsigned int num = (unsigned int) std::max (1l, get_long ());
      test (";");
      m_num_masks [ln] = num;

    } else if (test ("WIDTH")) {

      w = get_double ();
      expect (";");

    } else if (test ("MINWIDTH")) {

      wmin = get_double ();
      expect (";");

    } else if (test ("DIRECTION")) {

      if (test ("HORIZONTAL")) {
        is_horizontal = true;
      } else {
        expect ("VERTICAL", "DIAG45", "DIAG135");
      }

    } else if (test ("WIREEXTENSION")) {

      double v = get_double ();
      m_default_ext.insert (std::make_pair (ln, v));
      expect (";");

    } else if (test ("ACCURRENTDENSITY")) {

      //  ACCURRENTDENSITY needs some special attention because it can contain nested WIDTH
      //  blocks following a semicolon
      take ();
      if (test ("FREQUENCY")) {
        while (! at_end () && ! test ("TABLEENTRIES")) {
          take ();
        }
      }
      skip_entry ();

    } else if (test ("PROPERTY")) {

      while (! test (";") && ! at_end ()) {

        std::string name = get ();
        tl::Variant value = get ();

        if (name == "LEF58_MINWIDTH") {

          //  Cadence extension
          tl::Extractor ex (value.to_string ());
          double v = 0.0;
          if (ex.test ("MINWIDTH") && ex.try_read (v)) {
            if (ex.test ("WRONGDIRECTION")) {
              wmin_wrongdir = v;
            } else {
              wmin = v;
            }
          }

        } else if (name == "LEF58_WIDTH") {

          //  Cadence extension
          tl::Extractor ex (value.to_string ());
          double v = 0.0;
          if (ex.test ("WIDTH") && ex.try_read (v)) {
            if (ex.test ("WRONGDIRECTION")) {
              w_wrongdir = v;
            } else {
              w = v;
            }
          }

        }

      }

    } else {
      skip_entry ();
    }
  }

  if (w > 0.0 || w_wrongdir > 0.0) {

    if (w_wrongdir == 0.0) {
      w_wrongdir = w;
    } else if (! is_horizontal) {
      std::swap (w, w_wrongdir);
    }

    m_default_widths.insert (std::make_pair (ln, std::make_pair (w, w_wrongdir)));

  }

  if (wmin > 0.0 || wmin_wrongdir > 0.0) {

    if (wmin_wrongdir == 0.0) {
      wmin_wrongdir = wmin;
    } else if (! is_horizontal) {
      std::swap (wmin, wmin_wrongdir);
    }

    m_min_widths.insert (std::make_pair (ln, std::make_pair (wmin, wmin_wrongdir)));

  }
}

void
LEFImporter::read_macro (Layout &layout)
{
  std::string mn = get ();

  if (m_macros.find (mn) != m_macros.end ()) {
    error (tl::to_string (tr ("Duplicate MACRO name: ")) + mn);
  }

  set_cellname (mn);

  GeometryBasedLayoutGenerator *mg = new GeometryBasedLayoutGenerator ();
  reader_state ()->register_macro_cell (mn, mg);

  db::Trans foreign_trans;
  std::string foreign_name;

  db::Point origin;
  db::Vector size;

  //  read the macro
  while (! at_end ()) {

    if (test ("END")) {
      expect (mn);
      break;

    } else if (test ("ORIGIN")) {

      origin = get_point (1.0 / layout.dbu ());
      expect (";");

    } else if (test ("SIZE")) {

      double x = get_double ();
      test ("BY");
      double y = get_double ();
      expect (";");
      size = db::Vector (db::DVector (x / layout.dbu (), y / layout.dbu ()));

    } else if (test ("PIN")) {

      LEFDEFSection section (this, "PIN");

      std::string pn = get ();
      std::string dir;

      while (! at_end ()) {

        if (test ("END")) {

          break;

        } else if (test ("DIRECTION")) {

          dir = get ();
          test (";");

        } else if (test ("PORT")) {

          LEFDEFSection section (this, "PORT");

          //  produce pin labels
          //  TODO: put a label on every single object?
          std::string label = pn;
          /* don't add the direction currently, a name is sufficient
          if (! dir.empty ()) {
            label += ":";
            label += dir;
          }
          */

          if (reader_state ()->tech_comp ()->produce_lef_pins ()) {

            db::properties_id_type prop_id = 0;
            if (produce_pin_props ()) {
              db::PropertiesRepository::properties_set props;
              props.insert (std::make_pair (pin_prop_name_id (), tl::Variant (label)));
              prop_id = layout.properties_repository ().properties_id (props);
            }

            std::map <std::string, db::Box> boxes_for_labels;
            read_geometries (mg, layout.dbu (), LEFPins, &boxes_for_labels, prop_id);

            for (std::map <std::string, db::Box>::const_iterator b = boxes_for_labels.begin (); b != boxes_for_labels.end (); ++b) {
              if (! b->second.empty ()) {
                mg->add_text (b->first, LEFLabel, db::Text (label.c_str (), db::Trans (b->second.center () - db::Point ())), 0, 0);
              }
            }

          } else {
            read_geometries (0, layout.dbu (), LEFPins, 0, 0);
          }

          expect ("END");

        } else {
          skip_entry ();
        }
      }

      expect (pn);

    } else if (test ("FOREIGN")) {

      LEFDEFSection section (this, "FOREIGN");

      std::string cn = get ();

      db::Point vec;
      db::FTrans ft;
      if (! peek (";")) {
        if (test ("(")) {
          vec = get_point (1.0 / layout.dbu ());
          expect (")");
        } else {
          vec = get_point (1.0 / layout.dbu ());
        }
        ft = get_orient (true);
      }

      expect (";");

      if (options ().macro_resolution_mode () != 1) {

        if (! foreign_name.empty ()) {
          warn (tl::to_string (tr ("Duplicate FOREIGN definition")));
        }

        //  What is the definition of the FOREIGN transformation?
        //  Guessing: this transformation moves the lower-left origin to 0,0
        foreign_trans = db::Trans (db::Point () - vec) * db::Trans (ft);
        foreign_name = cn;

        if (foreign_name != mn) {
          warn (tl::to_string (tl::sprintf (tl::to_string (tr ("FOREIGN name %s differs from MACRO %s")), foreign_name, mn)));
        }

      }

    } else if (test ("OBS")) {

      LEFDEFSection section (this, "OBS");

      if (reader_state ()->tech_comp ()->produce_obstructions ()) {
        read_geometries (mg, layout.dbu (), Obstructions);
      } else {
        read_geometries (0, layout.dbu (), Obstructions);
      }

      expect ("END");

    } else if (test ("DENSITY")) {

      LEFDEFSection section (this, "DENSITY");

      //  read over DENSITY statements
      while (! at_end () && ! test ("END")) {
        if (test ("LAYER")) {
          get ();
          expect (";");
        } else {
          expect ("RECT");
          for (int i = 0; i < 5; ++i) {
            get_double ();
          }
          expect (";");
        }
      }

    } else if (test ("FIXEDMASK")) {

      mg->set_fixedmask (true);
      expect (";");

    } else {

      std::string token = get ();
      LEFDEFSection section (this, token);

      if (token == "TIMING") {
        //  read over sections we do not need
        while (! at_end () && ! test ("END")) {
          skip_entry ();
        }
        test (token);
      } else if (token != ";") {
        //  read over lines we do not need
        skip_entry ();
      }

    }

  }

  mg->add_box (std::string (), Outline, db::Box (-origin, -origin + size), 0, 0);
  mg->subtract_overlap_from_outline (m_overlap_layers);

  MacroDesc macro_desc;
  macro_desc.foreign_name = foreign_name;
  macro_desc.foreign_trans = foreign_trans;
  macro_desc.bbox = db::Box (-origin, -origin + size);
  macro_desc.origin = origin;
  m_macros.insert (std::make_pair (mn, macro_desc));

  reset_cellname ();
}

void 
LEFImporter::do_read (db::Layout &layout)
{
  db::LayoutLocker locker (&layout);

  //  TODO: what to do with that value?
  //  double dbu_mic = 1000;

  while (! at_end ()) {

    if (test ("END")) {

      expect ("LIBRARY");
      //  END LIBRARY should terminate the file, but we allow to continue, so we can cat LEF files:
      //  break;

    } else if (test ("VERSION")) {

      //  ignore VERSION statement currently 
      take ();
      expect (";");

    } else if (test ("UNITS")) {

      LEFDEFSection section (this, "UNITS");

      while (! at_end () && ! test ("END")) {
        if (test ("DATABASE")) {
          expect ("MICRONS");
          //  TODO: what to do with that value
          /* dbu_mic = */ get_double ();
          expect (";");
        } else {
          skip_entry ();
        }
      }

      expect ("UNITS");

    } else if (test ("SPACING")) {

      LEFDEFSection section (this, "SPACING");

      //  read over SPACING sections
      while (! at_end () && ! test ("END")) {
        skip_entry ();
      }

      test ("SPACING");

    } else if (test ("PROPERTYDEFINITIONS")) {

      LEFDEFSection section (this, "PROPERTYDEFINITIONS");

      //  read over PROPERTYDEFINITIONS sections
      while (! at_end () && ! test ("END")) {
        skip_entry ();
      }

      test ("PROPERTYDEFINITIONS");

    } else if (test ("NONDEFAULTRULE")) {

      LEFDEFSection section (this, "NONDEFAULTRULE");

      read_nondefaultrule (layout);

    } else if (test ("SITE")) {

      LEFDEFSection section (this, "NONDEFAULTRULE");

      //  read over SITE or VIARULE sections
      std::string n = get ();
      while (! at_end () && ! test ("END")) {
        skip_entry ();
      }

      test (n);

    } else if (test ("VIARULE")) {

      LEFDEFSection section (this, "VIARULE");

      //  read over SITE or VIARULE sections
      std::string n = get ();
      while (! at_end () && ! test ("END")) {
        skip_entry ();
      }

      test (n);

    } else if (test ("NOISETABLE")) {

      LEFDEFSection section (this, "NOISETABLE");

      //  read over NOISETABLE sections
      while (! at_end () && ! test ("END")) {
        skip_entry ();
      }

      test ("NOISETABLE");

    } else if (test ("IRDROP")) {

      LEFDEFSection section (this, "IRDROP");

      //  read over IRDROP sections
      while (! at_end () && ! test ("END")) {
        skip_entry ();
      }

      test ("IRDROP");

    } else if (test ("ARRAY")) {

      LEFDEFSection section (this, "ARRAY");

      //  read over ARRAY sections
      std::string n = get ();
      while (! at_end () && ! test ("END")) {
        if (test ("FLOORPLAN")) {
          while (! at_end () && ! test ("END")) {
            skip_entry ();
          }
        } else {
          skip_entry ();
        }
      }

      test (n);

    } else if (test ("VIA")) {

      LEFDEFSection section (this, "VIA");
      read_viadef (layout, std::string ());

    } else if (test ("BEGINEXT")) {

      LEFDEFSection section (this, "BEGINEXT");

      //  read over BEGINEXT sections
      while (! at_end () && ! test ("ENDEXT")) {
        take ();
      }

    } else if (test ("LAYER")) {

      LEFDEFSection section (this, "LAYER");
      read_layer (layout);

    } else if (test ("MACRO")) {

      LEFDEFSection section (this, "MACRO");
      read_macro (layout);

    } else {

      //  read over entries we do not need
      skip_entry ();

    }

  }
}

void
LEFImporter::skip_entry ()
{
  while (! at_end () && ! test (";")) {
    take ();
  }
}

void
LEFImporter::finish_lef (db::Layout &layout)
{
  for (std::map<std::string, MacroDesc>::const_iterator m = m_macros.begin (); m != m_macros.end (); ++m) {
    reader_state ()->macro_cell (m->first, layout, std::vector<std::string> (), std::vector<unsigned int> (), m->second, this);
  }
}

}

