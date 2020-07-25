
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

LEFImporter::LEFImporter ()
{
  //  .. nothing yet ..
}

LEFImporter::~LEFImporter ()
{
  //  .. nothing yet ..
}

db::Box
LEFImporter::macro_bbox_by_name (const std::string &name) const
{
  std::map<std::string, db::Box>::const_iterator m = m_macro_bboxes_by_name.find (name);
  if (m != m_macro_bboxes_by_name.end ()) {
    return m->second;
  } else {
    return db::Box ();
  }
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

std::pair<db::Cell *, db::Trans>
LEFImporter::macro_by_name (const std::string &name) const
{
  std::map<std::string, std::pair<db::Cell *, db::Trans> >::const_iterator m = m_macros_by_name.find (name);
  if (m != m_macros_by_name.end ()) {
    return m->second;
  } else {
    return std::make_pair ((db::Cell *) 0, db::Trans ());
  }
}

std::vector <db::Trans> 
LEFImporter::get_iteration (db::Layout &layout)
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
      t.push_back (db::Trans (db::Vector (db::DVector (dx * i / layout.dbu (), dy * j / layout.dbu ()))));
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

void
LEFImporter::read_geometries (db::Layout &layout, db::Cell &cell, LayerPurpose purpose, std::map<std::string, db::Box> *collect_bboxes, db::properties_id_type prop_id)
{
  std::string layer_name;
  double dbu = layout.dbu ();
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

      int layer_id = -1;
      std::pair <bool, unsigned int> dl = open_layer (layout, layer_name, purpose, mask);
      if (dl.first) {
        layer_id = int (dl.second);
      }

      bool iterate = test ("ITERATE");

      while (! peek (";") && ! peek ("DO")) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");
      }

      db::Coord iw = db::coord_traits<db::Coord>::rounded (w / dbu);
      db::Path p (points.begin (), points.end (), iw, iw / 2, iw / 2, false);

      if (iterate) {
        std::vector<db::Trans> ti = get_iteration (layout);
        if (layer_id >= 0) {
          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            db::Shape s = insert_shape (cell, layer_id, p, *t, prop_id);
            if (collect_bboxes) {
              collect_bboxes->insert (std::make_pair (layer_name, db::Box ())).first->second = s.bbox ();
            }
          }
        }
      } else if (layer_id >= 0) {
        db::Shape s = insert_shape (cell, layer_id, p, prop_id);
        if (collect_bboxes) {
          collect_bboxes->insert (std::make_pair (layer_name, db::Box ())).first->second = s.bbox ();
        }
      }

      expect (";");

    } else if (test ("POLYGON")) {

      std::vector<db::Point> points;

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      int layer_id = -1;
      std::pair <bool, unsigned int> dl = open_layer (layout, layer_name, purpose, mask);
      if (dl.first) {
        layer_id = int (dl.second);
      }

      bool iterate = test ("ITERATE");

      while (! peek (";") && ! peek ("DO")) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");
      }

      db::Polygon p;
      p.assign_hull (points.begin (), points.end ());

      if (iterate) {
        std::vector<db::Trans> ti = get_iteration (layout);
        if (layer_id >= 0) {
          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            db::Shape s = insert_shape (cell, layer_id, p, *t, prop_id);
            if (collect_bboxes) {
              collect_bboxes->insert (std::make_pair (layer_name, db::Box ())).first->second = s.bbox ();
            }
          }
        }
      } else if (layer_id >= 0) {
        db::Shape s = insert_shape (cell, layer_id, p, prop_id);
        if (collect_bboxes) {
          collect_bboxes->insert (std::make_pair (layer_name, db::Box ())).first->second = s.bbox ();
        }
      }

      expect (";");

    } else if (test ("RECT")) {

      std::vector<db::Point> points;

      unsigned int mask = 0;
      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      int layer_id = -1;
      std::pair <bool, unsigned int> dl = open_layer (layout, layer_name, purpose, mask);
      if (dl.first) {
        layer_id = int (dl.second);
      }

      bool iterate = test ("ITERATE");

      for (int i = 0; i < 2; ++i) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::Point (db::DPoint (x / dbu, y / dbu)));
        test (")");
      }

      db::Box b (points [0], points [1]);

      if (iterate) {
        std::vector<db::Trans> ti = get_iteration (layout);
        if (layer_id >= 0) {
          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            db::Shape s = insert_shape (cell, layer_id, b, *t, prop_id);
            if (collect_bboxes) {
              collect_bboxes->insert (std::make_pair (layer_name, db::Box ())).first->second = s.bbox ();
            }
          }
        }
      } else if (layer_id >= 0) {
        db::Shape s = insert_shape (cell, layer_id, b, prop_id);
        if (collect_bboxes) {
          collect_bboxes->insert (std::make_pair (layer_name, db::Box ())).first->second = s.bbox ();
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

      int layer_id = -1;
      std::pair <bool, unsigned int> dl = open_layer (layout, layer_name, purpose, mask);
      if (dl.first) {
        layer_id = int (dl.second);
      }

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
      db::Cell *vc = reader_state ()->via_cell (vn, layout, mask_bottom, mask_cut, mask_top, this);
      if (! vc) {
        warn (tl::to_string (tr ("Unknown via: ")) + vn);
      }

      if (iterate) {
        std::vector<db::Trans> ti = get_iteration (layout);
        if (vc) {
          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            cell.insert (db::CellInstArray (db::CellInst (vc->cell_index ()), *t * db::Trans (points [0])));
          }
        }
      } else if (vc) {
        cell.insert (db::CellInstArray (db::CellInst (vc->cell_index ()), db::Trans (points [0])));
      }

      expect (";");

    } else if (test ("PROPERTY")) {

      //  skip properties
      while (! at_end () && ! test (";")) {
        take ();
      }

    } else {
      //  stop at unknown token
      break;
    }

  }
}

void
LEFImporter::read_nondefaultrule (db::Layout & /*layout*/)
{
  //  read NONDEFAULTRULE sections
  std::string n = get ();

  while (! test ("END") || ! test (n)) {

    if (test ("LAYER")) {

      std::string l = get ();

      //  read the width for the layer
      while (! test ("END")) {
        if (test ("WIDTH")) {
          double w = get_double ();
          test (";");
          m_nondefault_widths[n][l] = std::make_pair (w, w);
        } else {
          while (! at_end () && ! test (";")) {
            take ();
          }
        }
      }

      test (l);

    } else if (test ("VIA")) {

      //  ignore VIA statements
      std::string v = get ();
      while (! test ("END") || ! test (v)) {
        take ();
      }

    } else {
      while (! at_end () && ! test (";")) {
        take ();
      }
    }

  }
}

void
LEFImporter::read_viadef_by_rule (RuleBasedViaGenerator *vg, ViaDesc &via_desc, const std::string & /*n*/, double dbu)
{
  while (! test ("END")) {

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

void
LEFImporter::read_viadef_by_geometry (GeometryBasedViaGenerator *vg, ViaDesc &via_desc, const std::string &n, double dbu)
{
  //  ignore resistance spec
  if (test ("RESISTANCE")) {
    get_double ();
    test (";");
  }

  std::string layer_name;
  std::set<std::string> seen_layers;
  std::vector<std::string> routing_layers;

  while (true) {

    if (test ("LAYER")) {

      layer_name = get ();

      if (m_routing_layers.find (layer_name) != m_routing_layers.end () && seen_layers.find (layer_name) == seen_layers.end ()) {
        seen_layers.insert (layer_name);
        routing_layers.push_back (layer_name);
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

      vg->add_polygon (layer_name, p, mask);

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
      vg->add_box (layer_name, b, mask);

      expect (";");

    } else if (test ("PROPERTY")) {

      //  skip properties
      while (! at_end () && ! test (";")) {
        take ();
      }

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
LEFImporter::read_viadef (Layout &layout)
{
  std::string n = get ();

  ViaDesc &via_desc = m_vias[n];

  while (test ("DEFAULT") || test ("TOPOFSTACKONLY"))
    ;
  test (";");

  if (test ("VIARULE")) {
    std::auto_ptr<RuleBasedViaGenerator> vg (new RuleBasedViaGenerator ());
    read_viadef_by_rule (vg.get (), via_desc, n, layout.dbu ());
    reader_state ()->register_via_cell (n, vg.release ());
  } else {
    std::auto_ptr<GeometryBasedViaGenerator> vg (new GeometryBasedViaGenerator ());
    read_viadef_by_geometry (vg.get (), via_desc, n, layout.dbu ());
    reader_state ()->register_via_cell (n, vg.release ());
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
        while (! test ("TABLEENTRIES")) {
          take ();
        }
      }
      while (! at_end () && ! test (";")) {
        take ();
      }

    } else if (test ("PROPERTY")) {

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

      expect (";");

    } else {

      while (! at_end () && ! test (";")) {
        take ();
      }

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

  if (m_macros_by_name.find (mn) != m_macros_by_name.end ()) {
    error (tl::to_string (tr ("Duplicate MACRO name: ")) + mn);
  }

  set_cellname (mn);

  db::Cell &cell = layout.cell (layout.add_cell ());
  db::Cell *foreign_cell = 0;
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

      std::string pn = get ();
      std::string dir;

      while (! at_end ()) {

        if (test ("END")) {

          break;

        } else if (test ("DIRECTION")) {

          dir = get ();
          test (";");

        } else if (test ("PORT")) {

          //  produce pin labels
          //  TODO: put a label on every single object?
          std::string label = pn;
          /* don't add the direction currently, a name is sufficient
          if (! dir.empty ()) {
            label += ":";
            label += dir;
          }
          */

          db::properties_id_type prop_id = 0;
          if (produce_pin_props ()) {
            db::PropertiesRepository::properties_set props;
            props.insert (std::make_pair (pin_prop_name_id (), tl::Variant (label)));
            prop_id = layout.properties_repository ().properties_id (props);
          }

          std::map <std::string, db::Box> bboxes;
          read_geometries (layout, cell, LEFPins, &bboxes, prop_id);

          for (std::map <std::string, db::Box>::const_iterator b = bboxes.begin (); b != bboxes.end (); ++b) {
            std::pair <bool, unsigned int> dl = open_layer (layout, b->first, Label, 0);
            if (dl.first) {
              cell.shapes (dl.second).insert (db::Text (label.c_str (), db::Trans (b->second.center () - db::Point ())));
            }
          }

          expect ("END");

        } else {
          while (! at_end () && ! test (";")) {
            take ();
          }
        }
      }

      expect (pn);

    } else if (test ("FOREIGN")) {

      if (foreign_cell) {
        error (tl::to_string (tr ("Duplicate FOREIGN definition")));
      }

      std::string cn = get ();

      db::Point vec;
      db::FTrans ft;
      if (! peek (";")) {
        vec = get_point (1.0 / layout.dbu ());
        ft = get_orient (true);
      }

      expect (";");

      if (options ().macro_resolution_mode () != 1) {

        db::cell_index_type ci;
        std::pair<bool, db::cell_index_type> c = layout.cell_by_name (cn.c_str ());
        if (c.first) {
          ci = c.second;
        } else {
          ci = layout.add_cell (cn.c_str ());
          layout.cell (ci).set_ghost_cell (true);
        }

        foreign_cell = &layout.cell (ci);
        //  What is the definition of the FOREIGN transformation?
        //  Guessing: this transformation moves the lower-left origin to 0,0
        foreign_trans = db::Trans (db::Point () - vec) * db::Trans (ft);
        foreign_name = cn;

      }

    } else if (test ("OBS")) {

      read_geometries (layout, cell, Obstructions);
      expect ("END");

    } else if (test ("FIXEDMASK")) {

      //  we do actually expect FIXEDMASK to be the case always.
      expect (";");

    } else {
      while (! at_end () && ! test (";")) {
        take ();
      }
    }

  }

  if (! foreign_cell) {

    if (options ().macro_resolution_mode () != 2) {

      //  actually implement the real cell

      layout.rename_cell (cell.cell_index (), mn.c_str ());

      std::pair <bool, unsigned int> dl = open_layer (layout, std::string (), Outline, 0);
      if (dl.first) {
        cell.shapes (dl.second).insert (db::Box (-origin, -origin + size));
      }

      m_macros_by_name.insert (std::make_pair (mn, std::make_pair (&cell, db::Trans ())));

    } else {

      //  macro resolution mode #2 (always create a MACRO reference, no LEF geometry)

      db::cell_index_type ci;
      std::pair<bool, db::cell_index_type> c = layout.cell_by_name (mn.c_str ());
      if (c.first) {
        ci = c.second;
      } else {
        ci = layout.add_cell (mn.c_str ());
        layout.cell (ci).set_ghost_cell (true);
      }

      layout.delete_cell (cell.cell_index ());
      m_macros_by_name.insert (std::make_pair (mn, std::make_pair (&layout.cell (ci), db::Trans ())));

    }

  } else if (foreign_name != mn) {

    warn (tl::to_string (tr ("FOREIGN name differs from MACRO name in macro: ")) + mn);

    layout.rename_cell (cell.cell_index (), mn.c_str ());

    //  clear imported LEF geometry with a foreign cell, but provide a level of indirection so we have
    //  both the MACRO and the FOREIGN name

    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (layout.is_valid_layer (l)) {
        cell.clear (l);
      }
    }

    cell.clear_insts ();

    cell.insert (db::CellInstArray (db::CellInst (foreign_cell->cell_index ()), db::Trans (db::Point () - origin) * foreign_trans));
    m_macros_by_name.insert (std::make_pair (mn, std::make_pair (&cell, db::Trans ())));

  } else {

    //  use FOREIGN cell instead of new one

    layout.delete_cell (cell.cell_index ());
    m_macros_by_name.insert (std::make_pair (mn, std::make_pair (foreign_cell, db::Trans (db::Point () - origin) * foreign_trans)));

  }

  m_macro_bboxes_by_name.insert (std::make_pair (mn, db::Box (-origin, -origin + size)));

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

      //  END LIBRARY terminates the file
      expect ("LIBRARY");
      break;

    } else if (test ("VERSION")) {

      //  ignore VERSION statement currently 
      take ();
      expect (";");

    } else if (test ("UNITS")) {

      //  read over SPACING sections
      while (! test ("END")) {
        if (test ("DATABASE")) {
          expect ("MICRONS");
          //  TODO: what to do with that value
          /* dbu_mic = */ get_double ();
          expect (";");
        } else {
          while (! at_end () && ! test (";")) {
            take ();
          }
        }
      }

      expect ("UNITS");

    } else if (test ("SPACING")) {

      //  read over SPACING sections
      while (! test ("END") || ! test ("SPACING")) {
        take ();
      }

    } else if (test ("PROPERTYDEFINITIONS")) {

      //  read over PROPERTYDEFINITIONS sections
      while (! test ("END") || ! test ("PROPERTYDEFINITIONS")) {
        take ();
      }

    } else if (test ("NONDEFAULTRULE")) {

      read_nondefaultrule (layout);

    } else if (test ("SITE")) {

      //  read over SITE sections
      std::string n = get ();
      while (! test ("END") || ! test (n)) {
        take ();
      }

    } else if (test ("VIARULE")) {

      //  read over VIARULE sections
      std::string n = get ();
      while (! test ("END") || ! test (n)) {
        take ();
      }

    } else if (test ("VIA")) {

      read_viadef (layout);

    } else if (test ("BEGINEXT")) {

      //  read over BEGINEXT sections
      while (! test ("ENDEXT")) {
        take ();
      }

    } else if (test ("LAYER")) {

      read_layer (layout);

    } else if (test ("MACRO")) {

      read_macro (layout);

    } else {
      while (! at_end () && ! test (";")) {
        take ();
      }
    }

  }
}

}

