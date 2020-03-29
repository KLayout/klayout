
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

DEFImporter::DEFImporter ()
  : LEFDEFImporter ()
{
  //  .. nothing yet ..
}

void 
DEFImporter::read_lef (tl::InputStream &stream, db::Layout &layout, LEFDEFLayerDelegate &ld)
{
  m_lef_importer.read (stream, layout, ld);
}


db::FTrans 
DEFImporter::get_orient (bool optional)
{
  if (test ("N")) {
    return db::FTrans (db::FTrans::r0);
  } else if (test ("S")) {
    return db::FTrans (db::FTrans::r180);
  } else if (test ("W")) {
    return db::FTrans (db::FTrans::r90);
  } else if (test ("E")) {
    return db::FTrans (db::FTrans::r270);
  } else if (test ("FN")) {
    return db::FTrans (db::FTrans::m90);
  } else if (test ("FS")) {
    return db::FTrans (db::FTrans::m0);
  } else if (test ("FW")) {
    return db::FTrans (db::FTrans::m45);
  } else if (test ("FE")) {
    return db::FTrans (db::FTrans::m135);
  } else if (optional) {
    return db::FTrans (db::FTrans::r0);
  } else {
    error (tl::to_string (tr ("Invalid orientation specification: ")) + get ());
    return db::FTrans (db::FTrans::r0);
  }
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
  double x = 0.0, y = 0.0;

  test ("(");
  x = get_double ();
  y = get_double ();
  db::Point pt1 = db::Point (db::DPoint (x * scale, y * scale));
  test (")");

  test ("(");
  x = get_double ();
  y = get_double ();
  db::Point pt2 = db::Point (db::DPoint (x * scale, y * scale));
  test (")");

  poly = db::Polygon (db::Box (pt1, pt2));
}

struct Group
{
  Group (const std::string &n, const std::string &rn, const std::vector<tl::GlobPattern> &m)
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

void 
DEFImporter::do_read (db::Layout &layout)
{
  double dbu_mic = 1000.0;
  double scale = 1.0 / (dbu_mic * layout.dbu ());
  std::map<int, db::Polygon> styles;
  std::map<std::string, ViaDesc> via_desc = m_lef_importer.vias ();
  std::map<std::string, std::vector<db::Polygon> > regions;
  std::list<Group> groups;
  std::list<std::pair<std::string, db::CellInstArray> > instances;

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

      std::vector<db::DPoint> points;

      while (! test (";")) {
        test ("(");
        double x = get_double ();
        double y = get_double ();
        points.push_back (db::DPoint (x * scale, y * scale));
        test (")");
      }

      if (points.size () >= 2) {

        //  create outline shape
        std::pair <bool, unsigned int> dl = open_layer (layout, std::string (), Outline);
        if (dl.first) {
          if (points.size () == 2) {
            design.shapes (dl.second).insert (db::Box (db::DBox (points [0], points [1])));
          } else {
            db::DPolygon p;
            p.assign_hull (points.begin (), points.end ());
            design.shapes (dl.second).insert (db::Polygon (p));
          }
        }

      }

    } else if (test ("PROPERTYDEFINITIONS")) {
      //  read over PROPERTYDEFINITIONS sections
      while (! test ("END") || ! test ("PROPERTYDEFINITIONS")) {
        take ();
      }

    } else if (test ("NONDEFAULTRULES")) {

      //  read NONDEFAULTRULES sections
      get_long ();
      expect (";");

      while (test ("-")) {

        std::string n = get ();

        while (test ("+")) {

          if (test ("LAYER")) {

            std::string l = get ();

            //  read the width for the layer
            if (test ("WIDTH")) {
              double w = get_double () * scale;
              m_nondefault_widths[n][l] = w;
            } 

          } 

          //  parse over the rest
          while (! peek ("+") && ! peek ("-") && ! peek (";")) {
            take ();
          }

        }

        test (";");

      }

      test ("END");
      test ("NONDEFAULTRULES");

    } else if (test ("REGIONS")) {

      //  Read REGION statements
      get_long ();
      expect (";");

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

      test ("END");
      test ("REGIONS");

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

        groups.push_back (Group (n, rn, match));

        test (";");

      }

      test ("END");
      test ("GROUPS");

    } else if (test ("BEGINEXT")) {
      //  read over BEGINEXT sections
      while (! test ("ENDEXT")) {
        take ();
      }

    } else if (test ("BLOCKAGES")) {

      get_long ();
      expect (";");

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

            std::pair <bool, unsigned int> dl = open_layer (layout, layer, layer.empty () ? PlacementBlockage : Blockage);
            if (dl.first) {
              design.shapes (dl.second).insert (p);
            }

          } else if (test ("RECT")) {

            db::Polygon p;
            read_rect (p, scale);

            std::pair <bool, unsigned int> dl = open_layer (layout, layer, layer.empty () ? PlacementBlockage : Blockage);
            if (dl.first) {
              design.shapes (dl.second).insert (p);
            }

          } else {
            expect (";");
          }

        }

      }

      test ("END");
      test ("BLOCKAGES");

    } else if ((specialnets = test ("SPECIALNETS")) == true || test ("NETS")) {

      get_long ();
      expect (";");

      while (test ("-")) {

        std::string net = get ();
        std::string nondefaultrule;
        std::string stored_netname, stored_nondefaultrule;
        std::string taperrule;
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

          if (! specialnets && test ("SUBNET")) {

            while (test ("(")) {
              while (! test (")")) {
                take ();
              }
            }

            if (! in_subnet) {
              stored_netname = net;
              stored_nondefaultrule = nondefaultrule;
              in_subnet = true;
            }

          } else if (! specialnets && test ("NONDEFAULTRULE")) {

            nondefaultrule = get ();

          } else if ((was_shield = test ("SHIELD")) == true || test ("NOSHIELD") || test ("ROUTED") || test ("FIXED") || test ("COVER")) {

            if (was_shield) {
              take ();
            }

            do {

              std::string ln = get ();

              taperrule.clear ();
              const std::string *rulename = 0;

              db::Coord w = 0;
              if (specialnets) {
                w = db::coord_traits<db::Coord>::rounded (get_double () * scale);
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

              if (! specialnets) {

                if (! rulename) {
                  rulename = &nondefaultrule;
                }

                w = db::coord_traits<db::Coord>::rounded (m_lef_importer.layer_width (ln, *rulename, 0.0) / layout.dbu ());

                //  try to find local nondefault rule
                if (! rulename->empty ()) {
                  std::map<std::string, std::map<std::string, double> >::const_iterator nd = m_nondefault_widths.find (*rulename);
                  if (nd != m_nondefault_widths.end ()) {
                    std::map<std::string, double>::const_iterator ld = nd->second.find (ln);
                    if (ld != nd->second.end ()) {
                      w = ld->second;
                    }
                  }
                }

              }

              db::Coord def_ext = 0;
              if (! specialnets) {
                def_ext = db::coord_traits<db::Coord>::rounded (m_lef_importer.layer_ext (ln, w * 0.5 * layout.dbu ()) / layout.dbu ());
              }

              std::map<int, db::Polygon>::const_iterator s = styles.find (sn);
              if (s != styles.end ()) {
                style = &s->second;
              }

              std::vector<db::Coord> ext;
              std::vector<db::Point> pts;

              double x = 0.0, y = 0.0;

              while (true) {

                if (test ("MASK")) {
                  //  ignore mask spec
                  get_long ();
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

                  std::pair <bool, unsigned int> dl = open_layer (layout, ln, Routing);
                  if (dl.first) {

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

                  ext.clear ();

                  while (peek ("(") || peek ("MASK")) {

                    if (test ("MASK")) {
                      //  ignore MASK spec
                      get_long ();
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
                    db::Coord e = def_ext;
                    if (! peek (")")) {
                      e = db::coord_traits<db::Coord>::rounded (get_double () * scale);
                    }
                    ext.push_back (e);

                    test (")");

                  }

                  if (pts.size () > 1) {

                    std::pair <bool, unsigned int> dl = open_layer (layout, ln, Routing);
                    if (dl.first) {

                      if (! style) {

                        //  Use the default style (octagon "pen" for non-manhattan segments, paths for 
                        //  horizontal/vertical segments).

                        db::Coord e = std::max (ext.front (), ext.back ());

                        std::vector<db::Point>::const_iterator pt = pts.begin ();
                        while (pt != pts.end ()) {

                          std::vector<db::Point>::const_iterator pt0 = pt;
                          do {
                            ++pt;
                          } while (pt != pts.end () && (pt[-1].x () == pt[0].x () || pt[-1].y () == pt[0].y()));

                          if (pt - pt0 > 1) {

                            db::Path p (pt0, pt, w, pt0 == pts.begin () ? e : 0, pt == pts.end () ? e : 0, false);
                            if (prop_id != 0) {
                              design.shapes (dl.second).insert (db::object_with_properties<db::Path> (p, prop_id));
                            } else {
                              design.shapes (dl.second).insert (p);
                            }

                            if (pt == pts.end ()) {
                              break;
                            }

                            --pt;

                          } else if (pt != pts.end ()) {

                            db::Coord s = (w + 1) / 2;
                            db::Coord t = db::Coord (ceil (w * (M_SQRT2 - 1) / 2));

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
                              design.shapes (dl.second).insert (db::object_with_properties<db::Polygon> (p, prop_id));
                            } else {
                              design.shapes (dl.second).insert (p);
                            }

                          }

                        }

                      } else {

                        for (size_t i = 0; i < pts.size () - 1; ++i) {
                          db::Polygon p = db::minkowsky_sum (*style, db::Edge (pts [i], pts [i + 1]));
                          if (prop_id != 0) {
                            design.shapes (dl.second).insert (db::object_with_properties<db::Polygon> (p, prop_id));
                          } else {
                            design.shapes (dl.second).insert (p);
                          }
                        }

                      }

                    }
                    
                  }

                } else if (! peek ("NEW") && ! peek ("+") && ! peek ("-") && ! peek (";")) {

                  //  indicates a via
                  std::string vn = get ();
                  db::FTrans ft = get_orient (true /*optional*/);

                  std::map<std::string, ViaDesc>::const_iterator vd = via_desc.find (vn);
                  if (vd != via_desc.end () && ! pts.empty ()) {
                    design.insert (db::CellInstArray (db::CellInst (vd->second.cell->cell_index ()), db::Trans (ft.rot (), db::Vector (pts.back ()))));
                    if (ln == vd->second.m1) {
                      ln = vd->second.m2;
                    } else if (ln == vd->second.m2) {
                      ln = vd->second.m1;
                    }
                  }

                  //  continue a segment with the current point and the new layer
                  if (pts.size () > 1) {
                    pts.erase (pts.begin (), pts.end () - 1);
                  }

                } else {
                  break;
                }

              }

            } while (test ("NEW"));

            if (in_subnet) {
              in_subnet = false;
              net = stored_netname;
              stored_netname.clear ();
              nondefaultrule = stored_nondefaultrule;
              stored_nondefaultrule.clear ();
            }

          } else if (test ("POLYGON")) {

            std::string ln = get ();

            db::Polygon p;
            read_polygon (p, scale);

            std::pair <bool, unsigned int> dl = open_layer (layout, ln, Routing);
            if (dl.first) {
              if (prop_id != 0) {
                design.shapes (dl.second).insert (db::object_with_properties<db::Polygon> (p, prop_id));
              } else {
                design.shapes (dl.second).insert (p);
              }
            }

          } else if (test ("RECT")) {

            std::string ln = get ();

            db::Polygon p;
            read_rect (p, scale);

            std::pair <bool, unsigned int> dl = open_layer (layout, ln, Routing);
            if (dl.first) {
              if (prop_id != 0) {
                design.shapes (dl.second).insert (db::object_with_properties<db::Polygon> (p, prop_id));
              } else {
                design.shapes (dl.second).insert (p);
              }
            }

          } else {
            while (! peek ("+") && ! peek ("-") && ! peek (";")) {
              take ();
            }
          }

        }

        expect (";");

      }

      test ("END");
      if (specialnets) {
        test ("SPECIALNETS");
      } else {
        test ("NETS");
      }

    } else if (test ("VIAS")) {

      get_long ();
      expect (";");

      while (test ("-")) {

        std::string n = get ();
        ViaDesc &vd = via_desc.insert (std::make_pair (n, ViaDesc ())).first->second;

        //  produce a cell for vias
        std::string cellname = "VIA_" + n;
        db::Cell &cell = layout.cell (layout.add_cell (cellname.c_str ()));
        vd.cell = &cell;

        bool has_via_rule = false;

        db::Vector cutsize, cutspacing;
        db::Vector be, te;
        db::Vector bo, to;
        db::Point offset;
        int rows = 1, columns = 1;
        std::string pattern;

        std::map<std::string, std::vector<db::Polygon> > geometry;
        std::vector<db::Polygon> *top = 0, *cut = 0, *bottom = 0;

        while (test ("+")) {

          double x, y;

          if (test ("VIARULE")) {

            has_via_rule = true;
            take ();

          } else if (test ("CUTSIZE")) {

            x = get_double ();
            y = get_double ();
            cutsize = db::Vector (db::DVector (x * scale, y * scale));

          } else if (test ("CUTSPACING")) {

            x = get_double ();
            y = get_double ();
            cutspacing = db::Vector (db::DVector (x * scale, y * scale));

          } else if (test ("ORIGIN")) {

            x = get_double ();
            y = get_double ();
            offset = db::Point (db::DPoint (x * scale, y * scale));

          } else if (test ("ENCLOSURE")) {

            x = get_double ();
            y = get_double ();
            be = db::Vector (db::DVector (x * scale, y * scale));

            x = get_double ();
            y = get_double ();
            te = db::Vector (db::DVector (x * scale, y * scale));

          } else if (test ("OFFSET")) {

            x = get_double ();
            y = get_double ();
            bo = db::Vector (db::DVector (x * scale, y * scale));

            x = get_double ();
            y = get_double ();
            to = db::Vector (db::DVector (x * scale, y * scale));

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

            std::vector<db::Polygon> &polygons = geometry.insert (std::make_pair (ln, std::vector<db::Polygon> ())).first->second;
            polygons.push_back (db::Polygon ());
            read_polygon (polygons.back (), scale);

          } else if (test ("RECT")) {

            std::string ln = get ();

            std::vector<db::Polygon> &polygons = geometry.insert (std::make_pair (ln, std::vector<db::Polygon> ())).first->second;
            polygons.push_back (db::Polygon ());
            read_rect (polygons.back (), scale);

          }

        }

        test (";");

        if (has_via_rule && top && cut && bottom) {
            create_generated_via (*bottom, *cut, *top,
                                  cutsize, cutspacing, be, te, bo, to, offset, rows, columns, pattern);
        }

        for (std::map<std::string, std::vector<db::Polygon> >::const_iterator g = geometry.begin (); g != geometry.end (); ++g) {
          std::pair <bool, unsigned int> dl = open_layer (layout, g->first, ViaGeometry);
          if (dl.first) {
            for (std::vector<db::Polygon>::const_iterator p = g->second.begin (); p != g->second.end (); ++p) {
              cell.shapes (dl.second).insert (*p);
            }
          }
        }

      }

      expect ("END");
      expect ("VIAS");

    } else if (test ("STYLES")) {

      get_long ();
      expect (";");

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

        styles.insert (std::make_pair (sn, db::Polygon ())).first->second.assign_hull (points.begin (), points.end ());

      }

      test ("END");
      test ("STYLES");

    } else if (test ("COMPONENTS")) {

      get_long ();
      expect (";");

      while (test ("-")) {

        std::string inst_name = get ();
        std::string model = get ();

        db::Cell *cell = m_lef_importer.macro_by_name (model);

        while (test ("+")) {

          if (test ("PLACED") || test ("FIXED") || test ("COVER")) {

            test ("(");
            double x = get_double ();
            double y = get_double ();
            db::Point pt = db::Point (db::DPoint (x * scale, y * scale));
            test (")");

            db::FTrans ft = get_orient (false /*mandatory*/);
            db::Vector d = pt - m_lef_importer.macro_bbox_by_name (model).transformed (ft).lower_left ();

            if (cell) {
              db::CellInstArray inst (db::CellInst (cell->cell_index ()), db::Trans (ft.rot (), d));
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

      expect ("END");
      expect ("COMPONENTS");

    } else if (test ("PINS")) {

      get_long ();
      expect (";");

      while (test ("-")) {

        take (); // pin name

        std::string net;
        std::string dir;
        std::map <std::string, std::vector <db::Polygon> > geometry;
        db::Trans trans;

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

            double x, y;

            test ("(");
            x = get_double ();
            y = get_double ();
            db::Point pt1 = db::Point (db::DPoint (x * scale, y * scale));
            test (")");

            test ("(");
            x = get_double ();
            y = get_double ();
            db::Point pt2 = db::Point (db::DPoint (x * scale, y * scale));
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
            double x = get_double ();
            double y = get_double ();
            db::Vector d = db::Vector (db::DVector (x * scale, y * scale));
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

              std::pair <bool, unsigned int> dl = open_layer (layout, g->first, Pins);
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

              dl = open_layer (layout, g->first, Label);
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

  if (! groups.empty ()) {

    others_cell = &layout.cell (layout.add_cell ("NOGROUP"));
    design.insert (db::CellInstArray (others_cell->cell_index (), db::Trans ()));

    //  Walk through the groups, create a group container cell and put all instances
    //  that match the group match string there. Then delete these cells (spec says "do not assign any component to more than one group").

    for (std::list<Group>::const_iterator g = groups.begin (); g != groups.end (); ++g) {

      db::Cell *group_cell = &layout.cell (layout.add_cell (("GROUP_" + g->name).c_str ()));
      design.insert (db::CellInstArray (group_cell->cell_index (), db::Trans ()));

      if (! g->region_name.empty ()) {

        std::map<std::string, std::vector<db::Polygon> >::const_iterator r = regions.find (g->region_name);
        if (r == regions.end ()) {
          warn (tl::sprintf (tl::to_string (tr ("Not a valid region name: %s in group %s")), g->region_name, g->name));
        } else {
          std::pair <bool, unsigned int> dl = open_layer (layout, std::string (), Region);
          if (dl.first) {
            for (std::vector<db::Polygon>::const_iterator p = r->second.begin (); p != r->second.end (); ++p) {
              group_cell->shapes (dl.second).insert (*p);
            }
          }
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

