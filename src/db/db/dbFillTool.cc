
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


#include "dbFillTool.h"
#include "dbPolygonTools.h"
#include "dbEdgeProcessor.h"
#include "dbRegion.h"
#include "dbCell.h"
#include "tlIntervalMap.h"

namespace db
{

namespace
{
  struct AddJoinOperator
  {
    void operator() (unsigned int &a, unsigned int b)
    {
      a += b;
    }
  };
}

static db::Vector
optimize_offset (const db::Polygon &fp, const db::AreaMap &am) 
{
  db::Coord xshift = 0, yshift = 0;

  {

    tl::interval_map <db::Coord, unsigned int> voting;
    AddJoinOperator op;

    db::AreaMap::area_type amax = db::AreaMap::area_type (am.d ().x ()) * db::AreaMap::area_type (am.d ().y ());

    db::Coord dx = am.d ().x ();
    db::Coord dy = am.d ().y ();
    size_t nx = am.nx ();
    size_t ny = am.ny ();

    //  Derive a optimal new x offset from the mapping
    for (size_t j = 0; j < size_t (ny); ++j) {

      bool x1set = false;
      bool x2set = false;
      db::Coord x1 = 0;
      db::Coord x2 = 0;

      for (size_t i = 0; i < size_t (nx); ++i) {

        if (am.get (i, j) >= amax) {

          if (! x1set) {

            x1 = 0;
            x1set = true;

          } else if (x2set) {

            x1 = x2;
            x1set = true;
            x2set = false;

          }

        } else if (am.get (i, j) > 0) {

          if (! x1set || x2set) {

            x1 = db::Coord (am.get (i, j) / dy);
            x1set = true;
            x2set = false;

          } else if (! x2set) {

            x2 = db::Coord (am.get (i, j) / dy);
            x2set = true;

          }

        } else if (am.get (i, j) == 0) {

          if (x1set) {
            
            if (! x2set) {
              x2 = 0;
            } 

            if (x1 + x2 < dx) {
              voting.add (-x1, x2 + 1, (unsigned int) 1, op);
            } else {
              voting.add (-x1, x2 - dx + 1, (unsigned int) 1, op);
              voting.add (dx - x1, x2 + 1, (unsigned int) 1, op);
            }

            x1set = false;
            x2set = false;

          }

        }

      }

      if (x1set) {

        if (! x2set) {
          x2 = 0;
        } 

        if (x1 + x2 < dx) {
          voting.add (-x1, x2 + 1, (unsigned int) 1, op);
        } else {
          voting.add (-x1, x2 - dx + 1, (unsigned int) 1, op);
          voting.add (dx - x1, x2 + 1, (unsigned int) 1, op);
        }

      }

    }

    std::set <db::Coord> xshifts;
    for (db::Polygon::polygon_edge_iterator e = fp.begin_edge (); ! e.at_end (); ++e) {
      xshifts.insert (((*e).p1 ().x () - am.p0 ().x ()) % dx); 
    }

    unsigned int max_votes = 0;
    for (std::set <db::Coord>::const_iterator xs = xshifts.begin (); xs != xshifts.end (); ++xs) {
      const unsigned int *z = voting.mapped (*xs);
      if (z && *z > max_votes) {
        xshift = *xs;
        max_votes = *z;
      }
    }

  }

  {

    tl::interval_map <db::Coord, unsigned int> voting;
    AddJoinOperator op;

    db::AreaMap::area_type amax = db::AreaMap::area_type (am.d ().x ()) * db::AreaMap::area_type (am.d ().y ());

    db::Coord dx = am.d ().x ();
    db::Coord dy = am.d ().y ();
    size_t nx = am.nx ();
    size_t ny = am.ny ();

    //  Derive a optimal new y offset from the mapping
    for (size_t i = 0; i < size_t (nx); ++i) {

      bool y1set = false;
      bool y2set = false;
      db::Coord y1 = 0;
      db::Coord y2 = 0;

      for (size_t j = 0; j < size_t (ny); ++j) {

        if (am.get (i, j) >= amax) {

          if (! y1set) {

            y1 = 0;
            y1set = true;

          } else if (y2set) {

            y1 = y2;
            y1set = true;
            y2set = false;

          }

        } else if (am.get (i, j) > 0) {

          if (! y1set || y2set) {

            y1 = db::Coord (am.get (i, j) / dx);
            y1set = true;
            y2set = false;

          } else if (! y2set) {

            y2 = db::Coord (am.get (i, j) / dx);
            y2set = true;

          }

        } else if (am.get (i, j) == 0) {

          if (y1set) {
            
            if (! y2set) {
              y2 = 0;
            } 

            if (y1 + y2 < dy) {
              voting.add (-y1, y2 + 1, (unsigned int) 1, op);
            } else {
              voting.add (-y1, y2 - dy + 1, (unsigned int) 1, op);
              voting.add (dy - y1, y2 + 1, (unsigned int) 1, op);
            }

            y1set = false;
            y2set = false;

          }

        }

      }

      if (y1set) {

        if (! y2set) {
          y2 = 0;
        } 

        if (y1 + y2 < dy) {
          voting.add (-y1, y2 + 1, (unsigned int) 1, op);
        } else {
          voting.add (-y1, y2 - dy + 1, (unsigned int) 1, op);
          voting.add (dy - y1, y2 + 1, (unsigned int) 1, op);
        }

      }

    }

    std::set <db::Coord> yshifts;
    for (db::Polygon::polygon_edge_iterator e = fp.begin_edge (); ! e.at_end (); ++e) {
      yshifts.insert (((*e).p1 ().y () - am.p0 ().y ()) % dy); 
    }

    unsigned int max_votes = 0;
    for (std::set <db::Coord>::const_iterator ys = yshifts.begin (); ys != yshifts.end (); ++ys) {
      const unsigned int *z = voting.mapped (*ys);
      if (z && *z > max_votes) {
        yshift = *ys;
        max_votes = *z;
      }
    }

  }

  return db::Vector (xshift, yshift);
}

static bool
rasterize_simple (const db::Polygon &fp, const db::Box &fc_bbox, const db::Point &p0, db::AreaMap &am)
{
  db::Coord dx = fc_bbox.width ();
  db::Coord dy = fc_bbox.height ();

  if (tl::verbosity () >= 50) {
    tl::info << "Simple rasterize polygon: " << fp.to_string () << " with box " << fc_bbox.to_string ();
  }

  db::Box fp_bbox = fp.box ();

  //  round polygon bbox 
  db::Coord fp_left = dx * ((fp_bbox.left () - p0.x ()) / dx) + p0.x ();
  db::Coord fp_bottom = dy * ((fp_bbox.bottom () - p0.y ()) / dy) + p0.y ();
  db::Coord fp_right = dx * ((fp_bbox.right () + dx - 1 - p0.x ()) / dx) + p0.x ();
  db::Coord fp_top = dy * ((fp_bbox.top () + dy - 1 - p0.y ()) / dy) + p0.y ();
  fp_bbox = db::Box (fp_left, fp_bottom, fp_right, fp_top);

  db::Coord nx = fp_bbox.width () / dx;
  db::Coord ny = fp_bbox.height () / dy;

  if (nx <= 0 || ny <= 0) {
    //  nothing to rasterize:
    return false;
  }

  am.reinitialize (fp_bbox.p1 (), db::Vector (dx, dy), size_t (nx), size_t (ny));

  //  Rasterize to determine fill regions
  db::rasterize (fp, am);

  return true;
}

static bool
rasterize_extended (const db::Polygon &fp, const db::Box &fc_bbox, db::AreaMap &am)
{
  db::Coord dx = fc_bbox.width ();
  db::Coord dy = fc_bbox.height ();

  if (tl::verbosity () >= 50) {
    tl::info << "Optimized rasterize polygon: " << fp.to_string () << " with box " << fc_bbox.to_string ();
  }

  db::Box fp_bbox = fp.box ();

  db::Coord nx = (fp_bbox.width () + dx - 1) / dx;
  db::Coord ny = (fp_bbox.height () + dy - 1) / dy;

  if (nx <= 0 || ny <= 0) {
    //  nothing to rasterize:
    return false;
  }

  am.reinitialize (fp_bbox.p1 (), db::Vector (dx, dy), size_t (nx), size_t (ny));

  //  Rasterize to determine fill regions
  db::rasterize (fp, am);

  if (tl::verbosity () >= 50) {

    db::Coord nx = db::Coord (am.nx ());
    db::Coord ny = db::Coord (am.ny ());
    db::AreaMap::area_type amax = am.pixel_area ();
    double n = 0;
    for (size_t i = 0; i < size_t (nx); ++i) {
      for (size_t j = 0; j < size_t (ny); ++j) {
        if (am.get (i, j) >= amax) {
          n += 1;
        }
      }
    }

    tl::info << "Number of fill regions before optimization: " << n;

  }

  db::Vector d = optimize_offset (fp, am);

  if (tl::verbosity () >= 50) {
    tl::info << "Shift vector: " << d.to_string ();
  }

  if (d.x () != 0 || d.y () != 0) {

    am.move (d);
    am.clear ();

    db::rasterize (fp, am);

    if (tl::verbosity () >= 50) {

      db::Coord nx = db::Coord (am.nx ());
      db::Coord ny = db::Coord (am.ny ());
      db::AreaMap::area_type amax = am.pixel_area ();
      double n = 0;
      for (size_t i = 0; i < size_t (nx); ++i) {
        for (size_t j = 0; j < size_t (ny); ++j) {
          if (am.get (i, j) >= amax) {
            n += 1;
          }
        }
      }

      tl::info << "Number of fill regions after optimization: " << n;

    }

  }

  return true;
}

DB_PUBLIC bool 
fill_region (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Point &origin, bool enhanced_fill, 
             std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin)
{
  std::vector <db::Polygon> filled_regions;
  db::EdgeProcessor ep;

  //  under- and oversize the polygon to remove slivers that cannot be filled.
  db::Coord dx = fc_bbox.width () / 2 - 1, dy = fc_bbox.height () / 2 - 1;

  std::vector <db::Polygon> fpa;
  std::vector <db::Polygon> fpb;
  fpa.push_back (fp0);

  ep.size (fpa, -dx, 0, fpb, 3 /*mode*/, false /*=don't resolve holes*/);
  fpa.swap (fpb);
  fpb.clear ();

  ep.size (fpa, dx, 0, fpb, 3 /*mode*/, false /*=don't resolve holes*/);
  fpa.swap (fpb);
  fpb.clear ();

  ep.size (fpa, 0, -dy, fpb, 3 /*mode*/, false /*=don't resolve holes*/);
  fpa.swap (fpb);
  fpb.clear ();

  ep.size (fpa, 0, dy, fpb, 3 /*mode*/, false /*=don't resolve holes*/);
  fpa.swap (fpb);
  fpb.clear ();

  ep.simple_merge (fpa, fpb, false /*=don't resolve holes*/);

  filled_regions.clear ();
  bool any_fill = false;

  for (std::vector <db::Polygon>::const_iterator fp = fpb.begin (); fp != fpb.end (); ++fp) {

    size_t ninsts = 0;

    db::AreaMap am;

    //  Rasterize to determine fill regions
    if ((enhanced_fill && rasterize_extended (*fp, fc_bbox, am)) || (!enhanced_fill && rasterize_simple (*fp, fc_bbox, origin, am))) {

      size_t nx = am.nx ();
      size_t ny = am.ny ();

      db::AreaMap::area_type amax = am.pixel_area ();

      //  Create the fill cell instances
      for (size_t i = 0; i < nx; ++i) {

        for (size_t j = 0; j < ny; ) {

          size_t jj = j + 1;
          if (am.get (i, j) >= amax) {

            while (jj != ny && am.get (i, jj) >= amax) {
              ++jj;
            }

            ninsts += (jj - j);

            db::Vector p0 (am.p0 () - fc_bbox.p1 ());
            p0 += db::Vector (db::Coord (i) * fc_bbox.width (), db::Coord (j) * fc_bbox.height ());

            db::CellInstArray array;

            if (jj > j + 1) {
              array = db::CellInstArray (db::CellInst (fill_cell_index), db::Trans (p0), db::Vector (0, fc_bbox.height ()), db::Vector (fc_bbox.width (), 0), (unsigned long) (jj - j), 1);
            } else {
              array = db::CellInstArray (db::CellInst (fill_cell_index), db::Trans (p0));
            }

            cell->insert (array);

            if (remaining_parts) {
              db::Box filled_box = array.raw_bbox () * fc_bbox; 
              filled_regions.push_back (db::Polygon (filled_box.enlarged (fill_margin)));
            }
            any_fill = true;

          }

          j = jj;

        }

      }

    }

    if (tl::verbosity () >= 30 && ninsts > 0) {
      tl::info << "Part " << fp->to_string ();
      tl::info << "Created " << ninsts << " instances";
    }

  }

  if (any_fill) {

    if (remaining_parts) {
      std::vector <db::Polygon> fp1;
      fp1.push_back (fp0);
      ep.boolean (fp1, filled_regions, *remaining_parts, db::BooleanOp::ANotB, false /*=don't resolve holes*/);
    }

    return true;

  } else {
    return false;
  }
}

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Point &origin, bool enhanced_fill, 
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons)
{
  std::vector<db::Polygon> rem_pp, rem_poly;

  for (db::Region::const_iterator p = fr.begin_merged (); !p.at_end (); ++p) {
    if (!fill_region (cell, *p, fill_cell_index, fc_box, origin, enhanced_fill, remaining_parts ? &rem_pp : 0, fill_margin)) {
      if (remaining_polygons) {
        rem_poly.push_back (*p);
      }
    }
  }

  if (remaining_parts == &fr) {
    remaining_parts->clear ();
  }
  if (remaining_polygons == &fr) {
    remaining_polygons->clear ();
  }

  if (remaining_parts) {
    for (std::vector<db::Polygon>::const_iterator p = rem_pp.begin (); p != rem_pp.end (); ++p) {
      remaining_parts->insert (*p);
    }
  }
  if (remaining_polygons) {
    for (std::vector<db::Polygon>::const_iterator p = rem_poly.begin (); p != rem_poly.end (); ++p) {
      remaining_polygons->insert (*p);
    }
  }
}

}
