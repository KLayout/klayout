
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
#include "tlMath.h"

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

class GenericRasterizer
{
public:
  GenericRasterizer ()
    : m_row_step (), m_column_step (), m_row_steps (0), m_column_steps (0), m_origin ()
  {
    // .. nothing yet ..
  }

  GenericRasterizer (const db::Polygon &fp, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin)
    : m_row_step (row_step), m_column_step (column_step), m_row_steps (0), m_column_steps (0), m_origin (origin)
  {
    db::Coord dx = row_step.x ();
    db::Coord dy = column_step.y ();

    if (row_step.y () == 0) {
      m_row_steps = 1;
    } else {
      m_row_steps = tl::lcm (dy, std::abs (row_step.y ())) / std::abs (row_step.y ());
    }

    if (column_step.x () == 0) {
      m_column_steps = 1;
    } else {
      m_column_steps = tl::lcm (dx, std::abs (column_step.x ())) / std::abs (column_step.x ());
    }

    db::Box fp_bbox = fp.box ();

    //  compensate for distortion by sheared kernel
    fp_bbox.enlarge (db::Vector (db::coord_traits<db::Coord>::rounded (double (fp_bbox.height ()) * std::abs (column_step.x ()) / dy), db::coord_traits<db::Coord>::rounded (double (fp_bbox.width ()) * std::abs (row_step.y ()) / dx)));

    int columns_per_rows = (int (m_row_steps) * row_step.y ()) / dy;
    int rows_per_columns = (int (m_column_steps) * column_step.x ()) / dx;

    db::Coord ddx = dx * db::Coord (m_row_steps) - column_step.x () * columns_per_rows;
    db::Coord ddy = dy * db::Coord (m_column_steps) - row_step.y () * rows_per_columns;

    //  round polygon bbox
    db::Coord fp_left = db::Coord (tl::round_down (fp_bbox.left () - origin.x (), ddx)) + origin.x ();
    db::Coord fp_bottom = db::Coord (tl::round_down (fp_bbox.bottom () - origin.y (), ddy)) + origin.y ();
    db::Coord fp_right = db::Coord (tl::round_up (fp_bbox.right () - origin.x (), ddx)) + origin.x ();
    db::Coord fp_top = db::Coord (tl::round_up (fp_bbox.top () - origin.y (), ddy)) + origin.y ();
    fp_bbox = db::Box (fp_left, fp_bottom, fp_right, fp_top);

    size_t nx = fp_bbox.width () / ddx;
    size_t ny = fp_bbox.height () / ddy;

    tl_assert (fp.box ().inside (fp_bbox));

    if (nx == 0 || ny == 0) {
      //  nothing to rasterize:
      return;
    }

    m_area_maps.reserve (m_row_steps * m_column_steps + std::abs (columns_per_rows) * std::abs (rows_per_columns));

    for (unsigned int ic = 0; ic < m_column_steps; ++ic) {

      for (unsigned int ir = 0; ir < m_row_steps; ++ir) {

        db::Vector dr = m_row_step * long (ir);
        db::Vector dc = m_column_step * long (ic);

        m_area_maps.push_back (db::AreaMap ());
        m_area_maps.back ().reinitialize (db::Point (fp_left, fp_bottom) + dr + dc, db::Vector (ddx, ddy), db::Vector (dx, dy), nx, ny);

        db::rasterize (fp, m_area_maps.back ());

      }

    }

    //  adds the "dead corner" piece

    for (unsigned int ic = 0; ic < (unsigned int) std::abs (columns_per_rows); ++ic) {

      for (unsigned int ir = 0; ir < (unsigned int) std::abs (rows_per_columns); ++ir) {

        db::Vector dr = m_row_step * long ((rows_per_columns > 0 ? -(ir + 1) : ir) + m_row_steps);
        db::Vector dc = m_column_step * long ((columns_per_rows > 0 ? -(ic + 1) : ic) + m_column_steps);

        m_area_maps.push_back (db::AreaMap ());
        m_area_maps.back ().reinitialize (db::Point (fp_left, fp_bottom) + dr + dc, db::Vector (ddx, ddy), db::Vector (dx, dy), nx, ny);

        db::rasterize (fp, m_area_maps.back ());

      }

    }
  }

  const db::Point &p0 () const { return m_origin; }

  unsigned int row_steps () const { return m_row_steps; }
  unsigned int column_steps () const { return m_column_steps; }

  unsigned int area_maps () const
  {
    return m_area_maps.size ();
  }

  const db::AreaMap &area_map (unsigned int i) const
  {
    return m_area_maps [i];
  }

private:
  std::vector<db::AreaMap> m_area_maps;
  db::Vector m_row_step, m_column_step;
  unsigned int m_row_steps, m_column_steps;
  db::Point m_origin;
};


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

// @@@
  //  try to create a point for which the fill box is inside the polygon
  size_t nhull = fp.hull ().size ();
  if (nhull < 3) {
    return false;
  }

  db::Point p0 = fp.hull ()[0];
  db::Point p1 = fp.hull ()[1];
  db::Point pm1 = fp.hull ()[nhull - 1];

  db::Coord hx = (dx + 1) / 2;
  db::Coord hy = (dy + 1) / 2;

  db::Edge e1 (p0, p1);
  if (e1.dx () < 0) {
    e1.move (db::Vector (hx, hy));
  } else {
    e1.move (db::Vector (hx, -hy));
  }

  db::Edge em1 (p0, pm1);
  if (em1.dy () < 0) {
    em1.move (db::Vector (hx, hy));
  } else {
    em1.move (db::Vector (-hx, hy));
  }

  std::pair<bool, db::Point> cp = e1.cut_point (em1);

  db::Point o = fp_bbox.p1 ();
  if (cp.first) {
    db::Point po = cp.second - db::Vector (hx, hy);
    o = po - db::Vector (dx * ((po.x () - fp_bbox.p1 ().x ()) / dx), dy * ((po.y () - fp_bbox.p1 ().y ()) / dy));
  }

  printf("@@@ fp=%s\n", fp.to_string().c_str()); fflush(stdout); // @@@
  printf("@@@ -> o=%s\n", o.to_string().c_str()); fflush(stdout); // @@@
// @@@

  am.reinitialize (o, db::Vector (dx, dy), size_t (nx), size_t (ny));

  //  Rasterize to determine fill regions
  db::rasterize (fp, am);

// @@@
{
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
printf("@@@ -> n=%d\n", int(n)); fflush(stdout); // @@@
}
// @@@
  return true; // @@@

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
  if (fc_bbox.empty () || fc_bbox.width () == 0 || fc_bbox.height () == 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid fill cell footprint (empty or zero width/height)")));
  }

  return fill_region (cell, fp0, fill_cell_index, fc_bbox.p1 () - db::Point (), db::Vector (fc_bbox.width (), 0), db::Vector (0, fc_bbox.height ()), origin, enhanced_fill, remaining_parts, fill_margin);
}

DB_PUBLIC bool
fill_region (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const db::Vector &kernel_origin, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin)
{
  if (row_step.x () <= 0 || column_step.y () <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row step must have a positive x component while column step must have a positive y component")));
  }

  if (db::vprod_sign (row_step, column_step) <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row_step x column_step vector vector product must be > 0")));
  }

  std::vector <db::Polygon> filled_regions;
  db::EdgeProcessor ep;

  //  under- and oversize the polygon to remove slivers that cannot be filled.
  db::Coord dx = row_step.x () / 2 - 1, dy = column_step.y () / 2 - 1;

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

    GenericRasterizer am (*fp, row_step, column_step, origin);

    //  @@@ optimize fill offset ...

    for (unsigned int i = 0; i < am.area_maps (); ++i) {

      const db::AreaMap &am1 = am.area_map (i);

      size_t nx = am1.nx ();
      size_t ny = am1.ny ();

      //  Create the fill cell instances
      for (size_t i = 0; i < nx; ++i) {

        for (size_t j = 0; j < ny; ) {

          size_t jj = j + 1;
          if (am1.get (i, j) == am1.pixel_area ()) {

            while (jj != ny && am1.get (i, jj) == am1.pixel_area ()) {
              ++jj;
            }

            ninsts += (jj - j);

            db::Vector p0 = (am1.p0 () - db::Point ()) - kernel_origin;
            p0 += db::Vector (i * am1.d ().x (), j * am1.d ().y ());

            db::CellInstArray array;

            if (jj > j + 1) {
              array = db::CellInstArray (db::CellInst (fill_cell_index), db::Trans (p0), db::Vector (0, am1.d ().y ()), db::Vector (), (unsigned long) (jj - j), 1);
            } else {
              array = db::CellInstArray (db::CellInst (fill_cell_index), db::Trans (p0));
            }

            cell->insert (array);

            if (remaining_parts) {
              if (am1.d ().y () == am1.p ().y ()) {
                filled_regions.push_back (db::Polygon (db::Box (db::Point (), db::Point (am1.p ().x (), am1.p ().y () * (jj - j))).moved (kernel_origin + p0)));
              } else {
                for (size_t k = 0; k < jj - j; ++k) {
                  filled_regions.push_back (db::Polygon (db::Box (db::Point (), db::Point () + am1.p ()).moved (kernel_origin + p0 + db::Vector (0, am1.d ().y () * db::Coord (k)))));
                }
              }
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

      if (fill_margin != db::Vector ()) {
        ep.size (filled_regions, fill_margin.x (), fill_margin.y (), fp1, 3 /*mode*/, false /*=don't resolve holes*/);
        filled_regions.swap (fp1);
        fp1.clear ();
      }

      fp1.push_back (fp0);
      ep.boolean (fp1, filled_regions, *remaining_parts, db::BooleanOp::ANotB, false /*=don't resolve holes*/);


    }

    return true;

  } else {
    return false;
  }
}

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Point &origin, bool enhanced_fill,
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons)
{
  if (fc_bbox.empty () || fc_bbox.width () == 0 || fc_bbox.height () == 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid fill cell footprint (empty or zero width/height)")));
  }

  fill_region (cell, fr, fill_cell_index, fc_bbox.p1 () - db::Point (), db::Vector (fc_bbox.width (), 0), db::Vector (0, fc_bbox.height ()),
               origin, enhanced_fill, remaining_parts, fill_margin, remaining_polygons);
}

static void
fill_region_impl (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Vector &kernel_origin, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
                  db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, int iteration)
{
  if (row_step.x () <= 0 || column_step.y () <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row step must have a positive x component while column step must have a positive y component")));
  }

  if (db::vprod_sign (row_step, column_step) <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row_step x column_step vector vector product must be > 0")));
  }

  std::vector<db::Polygon> rem_pp, rem_poly;

  size_t n = 0;
  for (db::Region::const_iterator p = fr.begin_merged (); !p.at_end (); ++p) {
    ++n;
  }

  {
    std::string progress_title;
    if (iteration > 0) {
      progress_title = tl::sprintf (tl::to_string (tr ("Fill polygons (iteration #%d)")), iteration);
    } else {
      progress_title = tl::sprintf (tl::to_string (tr ("Fill polygons")));
    }
    tl::RelativeProgress progress (progress_title, n);

    for (db::Region::const_iterator p = fr.begin_merged (); !p.at_end (); ++p) {
      if (!fill_region (cell, *p, fill_cell_index, kernel_origin, row_step, column_step, origin, enhanced_fill, remaining_parts ? &rem_pp : 0, fill_margin)) {
        if (remaining_polygons) {
          rem_poly.push_back (*p);
        }
      }
      ++progress;
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

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Vector &kernel_origin, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons)
{
  fill_region_impl (cell, fr, fill_cell_index, kernel_origin, row_step, column_step, origin, enhanced_fill, remaining_parts, fill_margin, remaining_polygons, 0);
}

DB_PUBLIC void
fill_region_repeat (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index,
                    const db::Vector &kernel_origin, const db::Vector &row_step, const db::Vector &column_step,
                    const db::Vector &fill_margin, db::Region *remaining_polygons)
{
  const db::Region *fill_region = &fr;

  db::Region new_fill_region;
  db::Region remaining;

  int iteration = 0;

  while (! fill_region->empty ()) {

    ++iteration;

    remaining.clear ();
    fill_region_impl (cell, *fill_region, fill_cell_index, kernel_origin, row_step, column_step, db::Point (), true, &remaining, fill_margin, remaining_polygons, iteration);

    new_fill_region.swap (remaining);
    fill_region = &new_fill_region;

    break; // @@@

  }
}

}
