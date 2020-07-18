
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


#include "dbDEFImporter.h"
#include "dbPolygonTools.h"
#include "tlGlobPattern.h"

#include <cmath>

namespace db
{

struct DEFImporterGroup
{
  DEFImporterGroup (const std::string &n, const std::string &rn, const std::vector<tl::GlobPattern> &m)
    : name (n), region_name (rn), comp_match (m)
  {
    //  .. nothing yet ..
  }

  bool comp_matches (const std::string &name) const
  {
    for (std::vector<tl::GlobPattern>::const_iterator m = comp_match.begin (); m != comp_match.end (); ++m) {
      if (m->match (name)) {
        return true;
      }
    }
    return false;
  }

  std::string name, region_name;
  std::vector<tl::GlobPattern> comp_match;
};

DEFImporter::DEFImporter ()
  : LEFDEFImporter ()
{
  //  .. nothing yet ..
}

void 
DEFImporter::read_lef (tl::InputStream &stream, db::Layout &layout, LEFDEFReaderState &state)
{
  m_lef_importer.read (stream, layout, state);
}

void
DEFImporter::read_polygon (db::Polygon &poly, double scale)
{
  std::vector<db::Point> points;

  double x = 0.0, y = 0.0;

  while (! peek ("+") && ! peek (";") && ! peek ("-")) {

    test ("(");
    if (! test ("*")) {
      x = get_double ();
    }
    if (! test ("*")) {
      y = get_double ();
    }
    points.push_back (db::Point (db::DPoint (x * scale, y * scale)));
    test (")");

  }

  poly.assign_hull (points.begin (), points.end ());
}

void
DEFImporter::read_rect (db::Polygon &poly, double scale)
{
  test ("(");
  db::Point pt1 = get_point (scale);
  test (")");

  test ("(");
  db::Point pt2 = get_point (scale);
  test (")");

  poly = db::Polygon (db::Box (pt1, pt2));
}

std::pair<db::Coord, db::Coord>
DEFImporter::get_wire_width_for_rule (const std::string &rulename, const std::string &ln, double dbu)
{
  std::pair<double, double> wxy = m_lef_importer.layer_width (ln, rulename);
  db::Coord wx = db::coord_traits<db::Coord>::rounded (wxy.first / dbu);
  db::Coord wy = db::coord_traits<db::Coord>::rounded (wxy.second / dbu);

  //  try to find local nondefault rule
  if (! rulename.empty ()) {
    std::map<std::string, std::map<std::string, db::Coord> >::const_iterator nd = m_nondefault_widths.find (rulename);
    if (nd != m_nondefault_widths.end ()) {
      std::map<std::string, db::Coord>::const_iterator ld = nd->second.find (ln);
      if (ld != nd->second.end ()) {
        wx = wy = ld->second;
      }
    }
  }

  std::pair<double, double> min_wxy = m_lef_importer.min_layer_width (ln);
  db::Coord min_wx = db::coord_traits<db::Coord>::rounded (min_wxy.first / dbu);
  db::Coord min_wy = db::coord_traits<db::Coord>::rounded (min_wxy.second / dbu);

  return std::make_pair (std::max (wx, min_wx), std::max (wy, min_wy));
}

std::pair<db::Coord, db::Coord>
DEFImporter::get_def_ext (const std::string &ln, const std::pair<db::Coord, db::Coord> &wxy, double dbu)
{
  //  This implementation assumes the "preferred width" is controlling the default extension and it is
  //  identical to the minimum effective width. This is true if "LEF58_MINWIDTH" with "WRONGDIRECTION" is
  //  used in the proposed way. Which is to specify a larger width for the "wrong" direction.
  db::Coord de = db::coord_traits<db::Coord>::rounded (m_lef_importer.layer_ext (ln, std::min (wxy.first, wxy.second) * 0.5 * dbu) / dbu);
  return std::make_pair (de, de);
}

void
DEFImporter::read_diearea (db::Layout &layout, db::Cell &design, double scale)
{
  std::vector<db::Point> points;

  while (! test (";")) {
    test ("(");
    points.push_back (get_point (scale));
    test (")");
  }

  if (points.size () >= 2) {

    //  create outline shape
    std::pair <bool, unsigned int> dl = open_layer (layout, std::string (), Outline, 0);
    if (dl.first) {
      if (points.size () == 2) {
        design.shapes (dl.second).insert (db::Box (points [0], points [1]));
      } else {
        db::Polygon p;
        p.assign_hull (points.begin (), points.end ());
        design.shapes (dl.second).insert (p);
      }
    }

  }
}

void
DEFImporter::read_nondefaultrules (double scale)
{
  while (test ("-")) {

    std::string n = get ();

    while (test ("+")) {

      if (test ("LAYER")) {

        std::string l = get ();

        //  read the width for the layer
        if (test ("WIDTH")) {
          double w = get_double () * scale;
          m_nondefault_widths[n][l] = db::coord_traits<db::Coord>::rounded (w);
        }

      }

      //  parse over the rest
      while (! peek ("+") && ! peek ("-") && ! peek (";")) {
        take ();
      }

    }

    test (";");

  }
}

void
DEFImporter::read_regions (std::map<std::string, std::vector<db::Polygon> > &regions, double scale)
{
  while (test ("-")) {

    std::string n = get ();
    std::vector<db::Polygon> &polygons = regions [n];

    while (! peek (";")) {

      if (test ("+")) {

        //  ignore other options for now
        while (! peek (";")) {
          take ();
        }
        break;

      } else {

        db::Polygon box;
        read_rect (box, scale);
        polygons.push_back (box);

      }

    }

    test (";");

  }
}
void
DEFImporter::read_groups (std::list<DEFImporterGroup> &groups, double /*scale*/)
{
  while (test ("-")) {

    std::string n = get ();
    std::string rn;
    std::vector<tl::GlobPattern> match;

    while (! peek (";")) {

      if (test ("+")) {

        //  gets the region name if there is one
        if (test ("REGION")) {
          rn = get ();
        }

        //  ignore the reset for now
        while (! peek (";")) {
          take ();
        }
        break;

      } else {

        match.push_back (tl::GlobPattern (get ()));

      }

    }

    groups.push_back (DEFImporterGroup (n, rn, match));

    test (";");

  }
}

void
DEFImporter::read_blockages (db::Layout &layout, db::Cell &design, double scale)
{
  while (test ("-")) {

    std::string layer;

    while (! test (";")) {

      if (test ("PLACEMENT")) {

        //  indicates a placement blockage
        layer = std::string ();

      } else if (test ("LAYER")) {

        layer = get ();

      } else if (test ("+")) {

        //  ignore options for now
        while (! peek ("RECT") && ! peek ("POLYGON") && ! peek ("+") && ! peek ("-") && ! peek (";")) {
          take ();
        }

      } else if (test ("POLYGON")) {

        db::Polygon p;
        read_polygon (p, scale);

        std::pair <bool, unsigned int> dl = open_layer (layout, layer, layer.empty () ? PlacementBlockage : Blockage, 0);
        if (dl.first) {
          design.shapes (dl.second).insert (p);
        }

      } else if (test ("RECT")) {

        db::Polygon p;
        read_rect (p, scale);

        std::pair <bool, unsigned int> dl = open_layer (layout, layer, layer.empty () ? PlacementBlockage : Blockage, 0);
        if (dl.first) {
          design.shapes (dl.second).insert (p);
        }

      } else {
        expect (";");
      }

    }

  }
}

void
DEFImporter::produce_routing_geometry (db::Cell &design, const Polygon *style, unsigned int layer, properties_id_type prop_id, const std::vector<db::Point> &pts, const std::vector<std::pair<db::Coord, db::Coord> > &ext, std::pair<db::Coord, db::Coord> w)
{
  if (! style) {

    //  Use the default style (octagon "pen" for non-manhattan segments, paths for
    //  horizontal/vertical segments).
    //  Manhattan paths are stitched together from two-point paths if the
    //  horizontal and vertical width is different.

    bool is_isotropic = (w.first == w.second);
    bool was_path_before = false;

    std::vector<db::Point>::const_iterator pt = pts.begin ();
    while (pt != pts.end ()) {

      std::vector<db::Point>::const_iterator pt0 = pt;
      ++pt;
      if (pt == pts.end ()) {
        break;
      }

      bool multipart = false;
      if (is_isotropic) {
        while (pt != pts.end () && (pt[-1].x () == pt[0].x () || pt[-1].y () == pt[0].y())) {
          ++pt;
          multipart = true;
        }
        if (multipart) {
          --pt;
        }
      }

      //  The next part is the interval [pt0..pt] (pt inclusive)

      if (multipart || (pt0->x () == pt0[1].x () || pt0->y () == pt0[1].y())) {

        db::Coord wxy, wxy_perp;

        if (pt0->x () == pt0 [1].x ()) {
          wxy = w.second;
          wxy_perp = w.first;
        } else {
          wxy = w.first;
          wxy_perp = w.second;
        }

        //  compute begin extension
        db::Coord be = 0;
        if (pt0 == pts.begin ()) {
          be = ext.front ().first;
        } else if (was_path_before) {
          //  provides the overlap to the previous segment
          be = wxy_perp / 2;
        }

        //  compute end extension
        db::Coord ee = 0;
        if (pt + 1 == pts.end ()) {
          ee = ext.back ().first;
        }

        db::Path p (pt0, pt + 1, wxy, be, ee, false);
        if (prop_id != 0) {
          design.shapes (layer).insert (db::object_with_properties<db::Path> (p, prop_id));
        } else {
          design.shapes (layer).insert (p);
        }

        was_path_before = true;

      } else {

        if (! is_isotropic) {
          warn("Anisotropic wire widths not supported for diagonal wires");
        }

        db::Coord s = (w.first + 1) / 2;
        db::Coord t = db::Coord (ceil (w.first * (M_SQRT2 - 1) / 2));

        db::Point octagon[8] = {
          db::Point (-s, t),
          db::Point (-t, s),
          db::Point (t, s),
          db::Point (s, t),
          db::Point (s, -t),
          db::Point (t, -s),
          db::Point (-t, -s),
          db::Point (-s, -t)
        };

        db::Polygon k;
        k.assign_hull (octagon, octagon + sizeof (octagon) / sizeof (octagon[0]));

        db::Polygon p = db::minkowsky_sum (k, db::Edge (*pt0, *pt));
        if (prop_id != 0) {
          design.shapes (layer).insert (db::object_with_properties<db::Polygon> (p, prop_id));
        } else {
          design.shapes (layer).insert (p);
        }

        was_path_before = false;

      }

    }

  } else {

    for (size_t i = 0; i < pts.size () - 1; ++i) {
      db::Polygon p = db::minkowsky_sum (*style, db::Edge (pts [i], pts [i + 1]));
      if (prop_id != 0) {
        design.shapes (layer).insert (db::object_with_properties<db::Polygon> (p, prop_id));
      } else {
        design.shapes (layer).insert (p);
      }
    }

  }
}

void
DEFImporter::read_single_net (std::string &nondefaultrule, Layout &layout, db::Cell &design, double scale, db::properties_id_type prop_id, bool specialnets)
{
  std::string taperrule;

  do {

    std::string ln = get ();

    taperrule.clear ();
    const std::string *rulename = 0;

    std::pair<db::Coord, db::Coord> w (0, 0);
    if (specialnets) {
      db::Coord n = db::coord_traits<db::Coord>::rounded (get_double () * scale);
      w = std::make_pair (n, n);
    }

    const db::Polygon *style = 0;

    int sn = std::numeric_limits<int>::max ();

    if (specialnets) {

      while (test ("+")) {

        if (test ("STYLE")) {
          sn = get_long ();
        } else if (test ("SHAPE")) {
          take ();
        }

      }

    } else {

      while (true) {
        if (test ("TAPER")) {
          taperrule.clear ();
          rulename = &taperrule;
        } else if (test ("TAPERRULE")) {
          taperrule = get ();
          rulename = &taperrule;
        } else if (test ("STYLE")) {
          sn = get_long ();
        } else {
          break;
        }
      }

    }

    if (! rulename) {
      rulename = &nondefaultrule;
    }

    std::pair<db::Coord, db::Coord> def_ext (0, 0);

    if (! specialnets) {
      w = get_wire_width_for_rule (*rulename, ln, layout.dbu ());
      def_ext = get_def_ext (ln, w, layout.dbu ());
    }

    std::map<int, db::Polygon>::const_iterator s = m_styles.find (sn);
    if (s != m_styles.end ()) {
      style = &s->second;
    }

    std::vector<std::pair<db::Coord, db::Coord> > ext;
    std::vector<db::Point> pts;

    double x = 0.0, y = 0.0;
    unsigned int mask = 0;

    while (true) {

      if (test ("MASK")) {
        mask = get_mask (get_long ());
      }

      if (test ("RECT")) {

        if (! test ("(")) {
          error (tl::to_string (tr ("RECT routing specification not followed by coordinate list")));
        }

        //  breaks wiring
        pts.clear ();

        //  rect spec

        double x1 = get_double ();
        double y1 = get_double ();
        double x2 = get_double ();
        double y2 = get_double ();

        test (")");

        std::pair <bool, unsigned int> dl = open_layer (layout, ln, specialnets ? SpecialRouting : Routing, mask);
        if (dl.first) {

          db::Point p (x, y);
          db::Box rect (db::Point (db::DPoint ((x + x1) * scale, (y + y1) * scale)),
                        db::Point (db::DPoint ((x + x2) * scale, (y + y2) * scale)));

          if (prop_id != 0) {
            design.shapes (dl.second).insert (db::object_with_properties<db::Box> (rect, prop_id));
          } else {
            design.shapes (dl.second).insert (rect);
          }

        }

      } else if (test ("VIRTUAL")) {

        //  virtual specs simply create a new segment
        pts.clear ();

      } else if (peek ("(")) {

        while (peek ("(") || peek ("MASK")) {

          if (test ("MASK")) {
            mask = get_mask (get_long ());
          }

          if (! test ("(")) {
            //  We could have a via here: in that case we have swallowed MASK already, but
            //  since we don't do anything with that, this does not hurt for now.
            break;
          }

          if (! test ("*")) {
            x = get_double ();
          }
          if (! test ("*")) {
            y = get_double ();
          }
          pts.push_back (db::Point (db::DPoint (x * scale, y * scale)));
          std::pair<db::Coord, db::Coord> ee = def_ext;
          if (! peek (")")) {
            db::Coord e = db::coord_traits<db::Coord>::rounded (get_double () * scale);
            ee.first = ee.second = e;
          }
          ext.push_back (ee);

          test (")");

        }

        if (pts.size () > 1) {
          std::pair <bool, unsigned int> dl = open_layer (layout, ln, specialnets ? SpecialRouting : Routing, mask);
          if (dl.first) {
            produce_routing_geometry (design, style, dl.second, prop_id, pts, ext, w);
          }
        }

      } else if (! peek ("NEW") && ! peek ("+") && ! peek ("-") && ! peek (";")) {

        //  indicates a via
        std::string vn = get ();
        db::FTrans ft = get_orient (true /*optional*/);

        db::Coord dx = 0, dy = 0;
        long nx = 1, ny = 1;

        if (specialnets && test ("DO")) {

          nx = std::max (0l, get_long ());
          test ("BY");
          ny = std::max (0l, get_long ());
          test ("STEP");
          dx = db::coord_traits<db::Coord>::rounded (get_double () * scale);
          dy = db::coord_traits<db::Coord>::rounded (get_double () * scale);

          if (nx < 0) {
            dx = -dx;
            nx = -nx;
          }
          if (ny < 0) {
            dy = -dy;
            ny = -ny;
          }

        }

        std::map<std::string, ViaDesc>::const_iterator vd = m_via_desc.find (vn);
        if (vd != m_via_desc.end () && ! pts.empty ()) {
          if (nx <= 1 && ny <= 1) {
            design.insert (db::CellInstArray (db::CellInst (vd->second.cell->cell_index ()), db::Trans (ft.rot (), db::Vector (pts.back ()))));
          } else {
            design.insert (db::CellInstArray (db::CellInst (vd->second.cell->cell_index ()), db::Trans (ft.rot (), db::Vector (pts.back ())), db::Vector (dx, 0), db::Vector (0, dy), (unsigned long) nx, (unsigned long) ny));
          }
          if (ln == vd->second.m1) {
            ln = vd->second.m2;
          } else if (ln == vd->second.m2) {
            ln = vd->second.m1;
          }
        }

        if (! specialnets) {
          w = get_wire_width_for_rule (*rulename, ln, layout.dbu ());
          def_ext = get_def_ext (ln, w, layout.dbu ());
        }

        //  continue a segment with the current point and the new layer
        if (pts.size () > 1) {
          pts.erase (pts.begin (), pts.end () - 1);
        }

        ext.clear ();
        ext.push_back (def_ext);

      } else {
        break;
      }

    }

  } while (test ("NEW"));
}

void
DEFImporter::read_nets (db::Layout &layout, db::Cell &design, double scale, bool specialnets)
{
  while (test ("-")) {

    std::string net = get ();
    std::string nondefaultrule;
    std::string stored_netname, stored_nondefaultrule;
    db::properties_id_type stored_prop_id;
    bool in_subnet = false;

    db::properties_id_type prop_id = 0;
    if (produce_net_props ()) {
      db::PropertiesRepository::properties_set props;
      props.insert (std::make_pair (net_prop_name_id (), tl::Variant (net)));
      prop_id = layout.properties_repository ().properties_id (props);
    }

    while (test ("(")) {
      while (! test (")")) {
        take ();
      }
    }

    while (test ("+")) {

      bool was_shield = false;
      unsigned int mask = 0;

      if (! specialnets && test ("SUBNET")) {

        std::string subnetname = get ();

        while (test ("(")) {
          while (! test (")")) {
            take ();
          }
        }

        if (! in_subnet) {
          stored_netname = net;
          stored_nondefaultrule = nondefaultrule;
          stored_prop_id = prop_id;
          in_subnet = true;
        } else {
          warn ("Nested subnets");
        }

        net = stored_netname + "/" + subnetname;

        if (produce_net_props ()) {
          db::PropertiesRepository::properties_set props;
          props.insert (std::make_pair (net_prop_name_id (), tl::Variant (net)));
          prop_id = layout.properties_repository ().properties_id (props);
        }

      } else if (! specialnets && test ("NONDEFAULTRULE")) {

        nondefaultrule = get ();

      } else {

        bool prefixed = false;
        bool can_have_rect_polygon_or_via = true;

        if ((was_shield = test ("SHIELD")) == true || test ("NOSHIELD") || test ("ROUTED") || test ("FIXED") || test ("COVER")) {
          if (was_shield) {
            take ();
          }
          prefixed = true;
          can_have_rect_polygon_or_via = test ("+");
        }

        bool any = false;

        if (can_have_rect_polygon_or_via) {
          if (test ("SHAPE")) {
            take ();
            test ("+");
          }
          if (test ("MASK")) {
            mask = get_mask (get_long ());
            test ("+");
          }
        }

        if (can_have_rect_polygon_or_via && test ("POLYGON")) {

          std::string ln = get ();

          db::Polygon p;
          read_polygon (p, scale);

          std::pair <bool, unsigned int> dl = open_layer (layout, ln, specialnets ? SpecialRouting : Routing, mask);
          if (dl.first) {
            if (prop_id != 0) {
              design.shapes (dl.second).insert (db::object_with_properties<db::Polygon> (p, prop_id));
            } else {
              design.shapes (dl.second).insert (p);
            }
          }

          any = true;

        } else if (can_have_rect_polygon_or_via && test ("RECT")) {

          std::string ln = get ();

          db::Polygon p;
          read_rect (p, scale);

          std::pair <bool, unsigned int> dl = open_layer (layout, ln, specialnets ? SpecialRouting : Routing, mask);
          if (dl.first) {
            if (prop_id != 0) {
              design.shapes (dl.second).insert (db::object_with_properties<db::Polygon> (p, prop_id));
            } else {
              design.shapes (dl.second).insert (p);
            }
          }

          any = true;

        } else if (can_have_rect_polygon_or_via && test ("VIA")) {

          std::string vn = get ();
          db::FTrans ft = get_orient (true /*optional*/);

          test ("(");
          db::Vector pt = get_vector (scale);
          test (")");

          std::map<std::string, ViaDesc>::const_iterator vd = m_via_desc.find (vn);
          if (vd != m_via_desc.end ()) {
            design.insert (db::CellInstArray (db::CellInst (vd->second.cell->cell_index ()), db::Trans (ft.rot (), pt)));
          } else {
            error (tl::to_string (tr ("Invalid via name: ")) + vn);
          }

          any = true;

        } else if (prefixed) {

          read_single_net (nondefaultrule, layout, design, scale, prop_id, specialnets);
          any = true;

        } else {

          //  lazily skip everything else
          while (! peek ("+") && ! peek ("-") && ! peek (";")) {
            take ();
          }

        }

        if (any && in_subnet) {

          in_subnet = false;

          net = stored_netname;
          nondefaultrule = stored_nondefaultrule;
          prop_id = stored_prop_id;

          stored_netname.clear ();
          stored_nondefaultrule.clear ();
          stored_prop_id = 0;

        }

      }

    }

    expect (";");

  }
}

void
DEFImporter::read_vias (db::Layout &layout, db::Cell & /*design*/, double scale)
{
  while (test ("-")) {

    std::string n = get ();
    ViaDesc &vd = m_via_desc.insert (std::make_pair (n, ViaDesc ())).first->second;

    //  produce a cell for vias
    std::string cellname = options ().via_cellname_prefix () + n;
    db::Cell &cell = layout.cell (layout.add_cell (cellname.c_str ()));
    reader_state ()->register_via_cell (n, &cell);
    vd.cell = &cell;

    bool has_via_rule = false;

    db::Vector cutsize, cutspacing;
    db::Vector be, te;
    db::Vector bo, to;
    db::Point offset;
    int rows = 1, columns = 1;
    std::string pattern;
    unsigned int mask = 0;

    std::map<std::string, std::vector<db::Polygon> > geometry;
    std::vector<db::Polygon> *top = 0, *cut = 0, *bottom = 0;

    while (test ("+")) {

      if (test ("VIARULE")) {

        has_via_rule = true;
        take ();

      } else if (test ("CUTSIZE")) {

        cutsize = get_vector (scale);

      } else if (test ("CUTSPACING")) {

        cutspacing = get_vector (scale);

      } else if (test ("ORIGIN")) {

        offset = get_point (scale);

      } else if (test ("ENCLOSURE")) {

        be = get_vector (scale);
        te = get_vector (scale);

      } else if (test ("OFFSET")) {

        bo = get_vector (scale);
        to = get_vector (scale);

      } else if (test ("ROWCOL")) {

        rows = get_long ();
        columns = get_long ();

      } else if (test ("PATTERN")) {

        pattern = get ();

      } else if (test ("LAYERS")) {

        std::string bn = get ();
        std::string cn = get ();
        std::string tn = get ();

        bottom = &geometry.insert (std::make_pair (bn, std::vector<db::Polygon> ())).first->second;
        cut = &geometry.insert (std::make_pair (cn, std::vector<db::Polygon> ())).first->second;
        top = &geometry.insert (std::make_pair (tn, std::vector<db::Polygon> ())).first->second;

        vd.m1 = bn;
        vd.m2 = tn;

      } else if (test ("POLYGON")) {

        std::string ln = get ();

        if (test ("+")) {
          expect ("MASK");
          mask = get_mask (get_long ());
        }

        std::vector<db::Polygon> &polygons = geometry.insert (std::make_pair (ln, std::vector<db::Polygon> ())).first->second;
        polygons.push_back (db::Polygon ());
        read_polygon (polygons.back (), scale);

      } else if (test ("RECT")) {

        std::string ln = get ();

        if (test ("+")) {
          expect ("MASK");
          mask = get_mask (get_long ());
        }

        std::vector<db::Polygon> &polygons = geometry.insert (std::make_pair (ln, std::vector<db::Polygon> ())).first->second;
        polygons.push_back (db::Polygon ());
        read_rect (polygons.back (), scale);

      }

    }

    if (vd.m1.empty () && vd.m2.empty ()) {

      //  analyze the layers to find the metals
      std::vector<std::string> routing_layers;
      for (std::map<std::string, std::vector<db::Polygon> >::const_iterator b = geometry.begin (); b != geometry.end (); ++b) {
        if (m_lef_importer.is_routing_layer (b->first)) {
          routing_layers.push_back (b->first);
        }
      }

      if (routing_layers.size () == 2) {
        vd.m1 = routing_layers[0];
        vd.m2 = routing_layers[1];
      } else {
        warn ("Can't determine routing layers for via: " + n);
      }

    }

    test (";");

    if (has_via_rule && top && cut && bottom) {
        create_generated_via (*bottom, *cut, *top,
                              cutsize, cutspacing, be, te, bo, to, offset, rows, columns, pattern);
    }

    for (std::map<std::string, std::vector<db::Polygon> >::const_iterator g = geometry.begin (); g != geometry.end (); ++g) {
      std::pair <bool, unsigned int> dl = open_layer (layout, g->first, ViaGeometry, mask);
      if (dl.first) {
        for (std::vector<db::Polygon>::const_iterator p = g->second.begin (); p != g->second.end (); ++p) {
          cell.shapes (dl.second).insert (*p);
        }
      }
    }

  }
}

void
DEFImporter::read_pins (db::Layout &layout, db::Cell &design, double scale)
{
  while (test ("-")) {

    take (); // pin name

    std::string net;
    std::string dir;
    std::map <std::string, std::vector <db::Polygon> > geometry;
    db::Trans trans;
    unsigned int mask = 0;

    while (test ("+")) {

      bool flush = false;

      if (test ("DIRECTION")) {
        dir = get ();
      } else if (test ("NET")) {
        net = get ();
      } else if (test ("LAYER")) {

        std::string ln = get ();

        while (test ("DESIGNRULEWIDTH") || test ("SPACING")) {
          take ();
        }

        test ("(");
        db::Point pt1 = get_point (scale);
        test (")");

        test ("(");
        db::Point pt2 = get_point (scale);
        test (")");

        geometry.insert (std::make_pair (ln, std::vector<db::Polygon> ())).first->second.push_back (db::Polygon (db::Box (pt1, pt2)));

      } else if (test ("POLYGON")) {

        std::string ln = get ();

        while (test ("DESIGNRULEWIDTH") || test ("SPACING")) {
          take ();
        }

        std::vector<db::Point> points;

        double x = 0.0, y = 0.0;

        while (! test ("+") && ! test (";")) {

          test ("(");
          if (! test ("*")) {
            x = get_double ();
          }
          if (! test ("*")) {
            y = get_double ();
          }
          points.push_back (db::Point (db::DPoint (x * scale, y * scale)));
          test (")");

        }

        std::vector<db::Polygon> &polygons = geometry.insert (std::make_pair (ln, std::vector<db::Polygon> ())).first->second;
        polygons.push_back (db::Polygon ());
        polygons.back ().assign_hull (points.begin (), points.end ());

      } else if (test ("PLACED") || test ("FIXED") || test ("COVER")) {

        test ("(");
        db::Vector d = get_vector (scale);
        test (")");

        db::FTrans ft = get_orient (false /*mandatory*/);
        trans = db::Trans (ft.rot (), d);

      } else if (test ("PORT")) {

        flush = true;

      } else {
        while (! peek ("+") && ! peek ("-") && ! peek (";")) {
          take ();
        }
      }

      if (flush || ! peek ("+")) {

        //  TODO: put a label on every single object?
        std::string label = net;
        /* don't add the direction currently, a name is sufficient
        if (! dir.empty ()) {
          label += ":";
          label += dir;
        }
        */

        //  Produce geometry collected so far
        for (std::map<std::string, std::vector<db::Polygon> >::const_iterator g = geometry.begin (); g != geometry.end (); ++g) {

          std::pair <bool, unsigned int> dl = open_layer (layout, g->first, Pins, mask);
          if (dl.first) {

            db::properties_id_type prop_id = 0;
            if (produce_pin_props ()) {
              db::PropertiesRepository::properties_set props;
              props.insert (std::make_pair (pin_prop_name_id (), tl::Variant (label)));
              prop_id = layout.properties_repository ().properties_id (props);
            }

            for (std::vector<db::Polygon>::const_iterator p = g->second.begin (); p != g->second.end (); ++p) {
              db::Polygon pt = p->transformed (trans);
              if (prop_id == 0) {
                design.shapes (dl.second).insert (pt);
              } else {
                design.shapes (dl.second).insert (db::PolygonWithProperties (pt, prop_id));
              }
            }

          }

          dl = open_layer (layout, g->first, Label, 0);
          if (dl.first) {
            db::Box bbox;
            if (! g->second.empty ()) {
              bbox = g->second.back ().box ().transformed (trans);
            }
            design.shapes (dl.second).insert (db::Text (label.c_str (), db::Trans (db::Vector (bbox.center ()))));
          }

        }

        geometry.clear ();
        trans = db::Trans ();

      }

    }

    expect (";");

  }
}

void
DEFImporter::read_styles (double scale)
{
  while (test ("-")) {

    test ("STYLE");

    int sn = get_long ();

    std::vector<db::Point> points;

    double x = 0.0, y = 0.0;

    while (! test (";")) {

      test ("(");
      if (! test ("*")) {
        x = get_double ();
      }
      if (! test ("*")) {
        y = get_double ();
      }
      points.push_back (db::Point (db::DPoint (x * scale, y * scale)));
      test (")");

    }

    m_styles.insert (std::make_pair (sn, db::Polygon ())).first->second.assign_hull (points.begin (), points.end ());

  }
}

void
DEFImporter::read_components (std::list<std::pair<std::string, CellInstArray> > &instances, double scale)
{
  while (test ("-")) {

    std::string inst_name = get ();
    std::string model = get ();

    std::pair<db::Cell *, db::Trans> ct = m_lef_importer.macro_by_name (model);

    while (test ("+")) {

      if (test ("PLACED") || test ("FIXED") || test ("COVER")) {

        test ("(");
        db::Point pt = get_point (scale);
        test (")");

        db::FTrans ft = get_orient (false /*mandatory*/);
        db::Vector d = pt - m_lef_importer.macro_bbox_by_name (model).transformed (ft).lower_left ();

        if (ct.first) {
          db::CellInstArray inst (db::CellInst (ct.first->cell_index ()), db::Trans (ft.rot (), d) * ct.second);
          instances.push_back (std::make_pair (inst_name, inst));
        } else {
          warn (tl::to_string (tr ("Macro not found in LEF file: ")) + model);
        }

      } else {
        while (! peek ("+") && ! peek ("-") && ! peek (";")) {
          take ();
        }
      }

    }

    expect (";");

  }
}

void 
DEFImporter::do_read (db::Layout &layout)
{
  db::LayoutLocker locker (&layout);

  double dbu_mic = 1000.0;
  double scale = 1.0 / (dbu_mic * layout.dbu ());

  std::map<std::string, std::vector<db::Polygon> > regions;
  std::list<DEFImporterGroup> groups;
  std::list<std::pair<std::string, db::CellInstArray> > instances;

  m_via_desc = m_lef_importer.vias ();
  m_styles.clear ();

  db::Cell &design = layout.cell (layout.add_cell ("TOP"));

  while (! at_end ()) {

    bool specialnets = false;

    if (test ("END")) {

      //  END DESIGN terminates the file
      expect ("DESIGN");
      break;

    } else if (test ("DESIGN")) {

      std::string cn = get ();
      layout.rename_cell (design.cell_index (), layout.uniquify_cell_name (cn.c_str ()).c_str ());

      expect (";");

    } else if (test ("VERSION")) {

      //  ignore VERSION statement currently 
      take ();
      expect (";");

    } else if (test ("UNITS")) {

      test ("DISTANCE");
      test ("MICRONS");

      double units = get_double ();
      if (fabs (units) > 1e-6) {
        scale = 1.0 / (units * layout.dbu ());
      }
      expect (";");

    } else if (test ("DIEAREA")) {

      read_diearea (layout, design, scale);

    } else if (test ("PROPERTYDEFINITIONS")) {
      //  read over PROPERTYDEFINITIONS sections
      while (! test ("END") || ! test ("PROPERTYDEFINITIONS")) {
        take ();
      }

    } else if (test ("NONDEFAULTRULES")) {

      //  read NONDEFAULTRULES sections
      get_long ();
      expect (";");

      read_nondefaultrules (scale);

      expect ("END");
      expect ("NONDEFAULTRULES");

    } else if (test ("REGIONS")) {

      //  Read REGION statements
      get_long ();
      expect (";");

      read_regions (regions, scale);

      expect ("END");
      expect ("REGIONS");

    } else if (test ("PINPROPERTIES")) {
      //  read over PINPROPERTIES statements 
      while (! test ("END") || ! test ("PINPROPERTIES")) {
        take ();
      }
    } else if (test ("SLOTS")) {
      //  read over SLOTS statements 
      while (! test ("END") || ! test ("SLOTS")) {
        take ();
      }
    } else if (test ("FILLS")) {
      //  read over FILLS statements 
      while (! test ("END") || ! test ("FILLS")) {
        take ();
      }
    } else if (test ("SCANCHAINS")) {
      //  read over SCANCHAINS statements 
      while (! test ("END") || ! test ("SCANCHAINS")) {
        take ();
      }
    } else if (test ("GROUPS")) {

      //  Read GROUPS statements
      get_long ();
      expect (";");

      read_groups (groups, scale);

      expect ("END");
      expect ("GROUPS");

    } else if (test ("BEGINEXT")) {

      //  read over BEGINEXT sections
      while (! test ("ENDEXT")) {
        take ();
      }

    } else if (test ("BLOCKAGES")) {

      get_long ();
      expect (";");

      read_blockages (layout, design, scale);

      expect ("END");
      expect ("BLOCKAGES");

    } else if ((specialnets = test ("SPECIALNETS")) == true || test ("NETS")) {

      get_long ();
      expect (";");

      read_nets (layout, design, scale, specialnets);

      expect ("END");
      if (specialnets) {
        expect ("SPECIALNETS");
      } else {
        expect ("NETS");
      }

    } else if (test ("VIAS")) {

      get_long ();
      expect (";");

      read_vias (layout, design, scale);

      expect ("END");
      expect ("VIAS");

    } else if (test ("STYLES")) {

      get_long ();
      expect (";");

      read_styles (scale);

      expect ("END");
      expect ("STYLES");

    } else if (test ("COMPONENTS")) {

      get_long ();
      expect (";");

      read_components (instances, scale);

      expect ("END");
      expect ("COMPONENTS");

    } else if (test ("PINS")) {

      get_long ();
      expect (";");

      read_pins (layout, design, scale);

      expect ("END");
      expect ("PINS");

    } else {
      while (! test (";")) {
        take ();
      }
    }

  }

  //  now we have collected the groups, regions and instances we create new subcells for each group
  //  and put the instances for this group there

  db::Cell *others_cell = &design;

  if (! groups.empty () && options ().separate_groups ()) {

    others_cell = &layout.cell (layout.add_cell ("NOGROUP"));
    design.insert (db::CellInstArray (others_cell->cell_index (), db::Trans ()));

    //  Walk through the groups, create a group container cell and put all instances
    //  that match the group match string there. Then delete these cells (spec says "do not assign any component to more than one group").

    for (std::list<DEFImporterGroup>::const_iterator g = groups.begin (); g != groups.end (); ++g) {

      db::Cell *group_cell = &layout.cell (layout.add_cell (("GROUP_" + g->name).c_str ()));
      design.insert (db::CellInstArray (group_cell->cell_index (), db::Trans ()));

      if (! g->region_name.empty ()) {

        std::map<std::string, std::vector<db::Polygon> >::iterator r = regions.find (g->region_name);
        if (r == regions.end ()) {

          warn (tl::sprintf (tl::to_string (tr ("Not a valid region name or region is already used: %s in group %s")), g->region_name, g->name));

        } else {

          std::pair <bool, unsigned int> dl = open_layer (layout, std::string (), Regions, 0);
          if (dl.first) {
            for (std::vector<db::Polygon>::const_iterator p = r->second.begin (); p != r->second.end (); ++p) {
              group_cell->shapes (dl.second).insert (*p);
            }
          }
          regions.erase (r);

        }

      }

      if (! g->comp_match.empty ()) {

        for (std::list<std::pair<std::string, db::CellInstArray> >::iterator i = instances.begin (); i != instances.end (); ) {

          std::list<std::pair<std::string, db::CellInstArray> >::iterator ii = i++;
          if (g->comp_matches (ii->first)) {

            if (produce_inst_props ()) {
              db::PropertiesRepository::properties_set props;
              props.insert (std::make_pair (inst_prop_name_id (), tl::Variant (ii->first)));
              group_cell->insert (db::CellInstArrayWithProperties (ii->second, layout.properties_repository ().properties_id (props)));
            } else {
              group_cell->insert (ii->second);
            }

            instances.erase (ii);

          }

        }

      }

    }

  }

  //  put all remaining regions into the "others_cell" which is the top cell if there are no groups.

  if (! regions.empty ()) {

    std::pair <bool, unsigned int> dl = open_layer (layout, std::string (), Regions, 0);
    if (dl.first) {

      for (std::map<std::string, std::vector<db::Polygon> >::const_iterator r = regions.begin (); r != regions.end (); ++r) {
        for (std::vector<db::Polygon>::const_iterator p = r->second.begin (); p != r->second.end (); ++p) {
          others_cell->shapes (dl.second).insert (*p);
        }
      }

    }

  }

  //  treat all remaining cells and put them into the "others_cell" which is the top cell
  //  if there are no groups.

  for (std::list<std::pair<std::string, db::CellInstArray> >::iterator ii = instances.begin (); ii != instances.end (); ++ii) {

    if (produce_inst_props ()) {
      db::PropertiesRepository::properties_set props;
      props.insert (std::make_pair (inst_prop_name_id (), tl::Variant (ii->first)));
      others_cell->insert (db::CellInstArrayWithProperties (ii->second, layout.properties_repository ().properties_id (props)));
    } else {
      others_cell->insert (ii->second);
    }

  }
}

}
