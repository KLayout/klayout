
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

double 
LEFImporter::layer_width (const std::string &layer, const std::string &nondefaultrule, double def_width) const
{
  std::map<std::string, std::map<std::string, double> >::const_iterator nd = m_nondefault_widths.find (nondefaultrule);

  std::map<std::string, double>::const_iterator l;
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

db::Cell *
LEFImporter::macro_by_name (const std::string &name) const
{
  std::map<std::string, db::Cell *>::const_iterator m = m_macros_by_name.find (name);
  if (m != m_macros_by_name.end ()) {
    return m->second;
  } else {
    return 0;
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
  int layer_id = -1;
  std::string layer_name;
  double dbu = layout.dbu ();
  double w = 0.0;

  while (true) {

    if (test ("CLASS")) {

      //  accept CLASS token for PORT definitions
      while (! test (";")) {
        take ();
      }

    } else if (test ("LAYER")) {

      layer_name = get ();
      layer_id = -1;

      std::pair <bool, unsigned int> dl = open_layer (layout, layer_name, purpose);
      if (dl.first) {
        layer_id = int (dl.second);
      }

      w = 0.0;
      std::map<std::string, double>::const_iterator dw = m_default_widths.find (layer_name);
      if (dw != m_default_widths.end ()) {
        w = dw->second;
      }

      while (! test (";")) {
        take ();
      }

    } else if (test ("WIDTH")) {

      w = get_double ();
      expect (";");

    } else if (test ("PATH")) {

      std::vector<db::Point> points;

      if (test ("MASK")) {
        //  ignore mask number
        get_long ();
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

      if (test ("MASK")) {
        //  ignore mask number
        get_long ();
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

      if (test ("MASK")) {
        //  ignore mask number
        get_long ();
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

      if (test ("MASK")) {
        //  ignore mask number
        get_long ();
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
      std::pair<bool, db::cell_index_type> cn = layout.cell_by_name (("VIA_" + vn).c_str ());
      if (! cn.first) {
        warn ("Unknown via: " + vn);
      }

      if (iterate) {
        std::vector<db::Trans> ti = get_iteration (layout);
        if (cn.first) {
          for (std::vector<db::Trans>::const_iterator t = ti.begin (); t != ti.end (); ++t) {
            cell.insert (db::CellInstArray (db::CellInst (cn.second), *t * db::Trans (points [0])));
          }
        }
      } else if (cn.first) {
        cell.insert (db::CellInstArray (db::CellInst (cn.second), db::Trans (points [0])));
      }

      expect (";");

    } else {
      //  stop at unknown token
      break;
    }

  }
}

void 
LEFImporter::do_read (db::Layout &layout)
{
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
          while (! test (";")) {
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
              m_nondefault_widths[n][l] = w;
            } else {
              while (! test (";")) {
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
          while (! test (";")) {
            take ();
          }
        }

      }

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

      std::string n = get ();

      //  produce a cell for vias
      std::string cellname = "VIA_" + n;
      db::Cell &cell = layout.cell (layout.add_cell (cellname.c_str ()));

      ViaDesc &via_desc = m_vias[n];
      via_desc.cell = &cell;

      while (test ("DEFAULT") || test ("TOPOFSTACKONLY")) 
        ;
      test (";");

      if (test ("VIARULE")) {

        db::Vector cutsize, cutspacing;
        db::Vector be, te;
        db::Vector bo, to;
        db::Point offset;
        int rows = 1, columns = 1;
        std::string pattern;

        std::vector<std::pair<std::string, std::vector<db::Polygon> > > geometry;
        geometry.push_back (std::pair<std::string, std::vector<db::Polygon> > ());
        geometry.push_back (std::pair<std::string, std::vector<db::Polygon> > ());
        geometry.push_back (std::pair<std::string, std::vector<db::Polygon> > ());

        while (! test ("END")) {

          double x, y;

          if (test ("CUTSIZE")) {

            x = get_double ();
            y = get_double ();
            cutsize = db::Vector (db::DVector (x / layout.dbu (), y / layout.dbu ()));

            test (";");

          } else if (test ("CUTSPACING")) {

            x = get_double ();
            y = get_double ();
            cutspacing = db::Vector (db::DVector (x / layout.dbu (), y / layout.dbu ()));

            test (";");

          } else if (test ("ORIGIN")) {

            x = get_double ();
            y = get_double ();
            offset = db::Point (db::DPoint (x / layout.dbu (), y / layout.dbu ()));

            test (";");

          } else if (test ("ENCLOSURE")) {

            x = get_double ();
            y = get_double ();
            be = db::Vector (db::DVector (x / layout.dbu (), y / layout.dbu ()));

            x = get_double ();
            y = get_double ();
            te = db::Vector (db::DVector (x / layout.dbu (), y / layout.dbu ()));

            test (";");

          } else if (test ("OFFSET")) {

            x = get_double ();
            y = get_double ();
            bo = db::Vector (db::DVector (x / layout.dbu (), y / layout.dbu ()));

            x = get_double ();
            y = get_double ();
            to = db::Vector (db::DVector (x / layout.dbu (), y / layout.dbu ()));

            test (";");

          } else if (test ("ROWCOL")) {

            rows = get_long ();
            columns = get_long ();

            test (";");

          } else if (test ("PATTERN")) {

            pattern = get ();

            test (";");

          } else if (test ("LAYERS")) {

            via_desc.m1 = geometry[0].first = get ();
            geometry[1].first = get ();
            via_desc.m2 = geometry[2].first = get ();

            test (";");

          } else {

            while (! test (";")) {
              take ();
            }

          }

        }

        create_generated_via (geometry [0].second, geometry [1].second, geometry [2].second,
                              cutsize, cutspacing, be, te, bo, to, offset, rows, columns, pattern);

        for (std::vector<std::pair<std::string, std::vector<db::Polygon> > >::const_iterator g = geometry.begin (); g != geometry.end (); ++g) {
          std::pair <bool, unsigned int> dl = open_layer (layout, g->first, ViaGeometry);
          if (dl.first) {
            for (std::vector<db::Polygon>::const_iterator p = g->second.begin (); p != g->second.end (); ++p) {
              cell.shapes (dl.second).insert (*p);
            }
          }
        }

      } else {

        //  ignore resistance spec
        if (test ("RESISTANCE")) {
          get_double ();
          test (";");
        }

        std::map<std::string, db::Box> bboxes;
        read_geometries (layout, cell, ViaGeometry, &bboxes);

        //  determine m1 and m2 layers

        std::vector<std::string> routing_layers;
        for (std::map<std::string, db::Box>::const_iterator b = bboxes.begin (); b != bboxes.end (); ++b) {
          if (m_routing_layers.find (b->first) != m_routing_layers.end ()) {
            routing_layers.push_back (b->first);
          }
        }

        if (routing_layers.size () == 2) {
          via_desc.m1 = routing_layers[0];
          via_desc.m2 = routing_layers[1];
        } else {
          warn ("Can't determine routing layers for via: " + n);
        }

        reset_cellname ();

        expect ("END");

      }

      test ("VIA");
      expect (n);

    } else if (test ("BEGINEXT")) {

      //  read over BEGINEXT sections
      while (! test ("ENDEXT")) {
        take ();
      }

    } else if (test ("LAYER")) {

      std::string ln = get ();

      register_layer (ln);

      //  just extract the width from the layer - we need that as the default width for paths
      while (! at_end ()) {
        if (test ("END")) {
          expect (ln);
          break;
        } else if (test ("TYPE")) {
          if (test ("ROUTING")) {
            m_routing_layers.insert (ln);
          } else if (test ("CUT")) {
            m_cut_layers.insert (ln);
          } else {
            get ();
          }
          expect (";");
        } else if (test ("WIDTH")) {
          double w = get_double ();
          m_default_widths.insert (std::make_pair (ln, w));
          expect (";");
        } else if (test ("WIREEXTENSION")) {
          double w = get_double ();
          m_default_ext.insert (std::make_pair (ln, w));
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
          while (! test (";")) {
            take ();
          }
        } else {
          while (! test (";")) {
            take ();
          }
        }
      }

    } else if (test ("MACRO")) {

      std::string mn = get ();
      set_cellname (mn);

      db::Cell &cell = layout.cell (layout.add_cell (mn.c_str ()));

      m_macros_by_name.insert (std::make_pair (mn, &cell));

      db::Point origin;
      db::Vector size;

      //  read the macro
      while (! at_end ()) {

        if (test ("END")) {
          expect (mn);
          break;

        } else if (test ("ORIGIN")) {

          double x = get_double ();
          double y = get_double ();
          expect (";");
          origin = db::Point (db::DPoint (x / layout.dbu (), y / layout.dbu ()));

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
              read_geometries (layout, cell, Pins, &bboxes, prop_id);

              for (std::map <std::string, db::Box>::const_iterator b = bboxes.begin (); b != bboxes.end (); ++b) {
                std::pair <bool, unsigned int> dl = open_layer (layout, b->first, Label);
                if (dl.first) {
                  cell.shapes (dl.second).insert (db::Text (label.c_str (), db::Trans (b->second.center () - db::Point ())));
                }
              }

              expect ("END");

            } else {
              while (! test (";")) {
                take ();
              }
            }
          }

          expect (pn);

        } else if (test ("OBS")) {

          read_geometries (layout, cell, Obstructions);
          expect ("END");

        } else {
          while (! test (";")) {
            take ();
          }
        }

      }

      std::pair <bool, unsigned int> dl = open_layer (layout, std::string (), Outline);
      if (dl.first) {
        cell.shapes (dl.second).insert (db::Box (-origin, -origin + size));
      }

      m_macro_bboxes_by_name.insert (std::make_pair (mn, db::Box (-origin, -origin + size)));

      reset_cellname ();

    } else {
      while (! test (";")) {
        take ();
      }
    }

  }
}

}

