
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


#include "dbFillTool.h"
#include "dbPolygonTools.h"
#include "dbEdgeProcessor.h"
#include "dbRegion.h"
#include "dbCell.h"
#include "dbTilingProcessor.h"
#include "tlIntervalMap.h"
#include "tlMath.h"

namespace db
{

class GenericRasterizer
{
public:
  GenericRasterizer ()
    : m_row_step (), m_column_step (), m_row_steps (0), m_column_steps (0), m_origin (), m_dim ()
  {
    // .. nothing yet ..
  }

  GenericRasterizer (const db::Polygon &fp, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, const db::Vector &dim)
    : m_row_step (row_step), m_column_step (column_step), m_row_steps (0), m_column_steps (0), m_origin (origin), m_dim (dim)
  {
    rasterize (fp);
  }

  void move (const db::Vector &d)
  {
    m_origin += d;
    clear ();
  }

  void clear ()
  {
    m_area_maps.clear ();
  }

  void rasterize (const db::Polygon &fp)
  {
    db::Coord dx = m_row_step.x ();
    db::Coord dy = m_column_step.y ();

    if (m_row_step.y () == 0) {
      m_row_steps = 1;
    } else {
      m_row_steps = tl::lcm (dy, std::abs (m_row_step.y ())) / std::abs (m_row_step.y ());
    }

    if (m_column_step.x () == 0) {
      m_column_steps = 1;
    } else {
      m_column_steps = tl::lcm (dx, std::abs (m_column_step.x ())) / std::abs (m_column_step.x ());
    }

    //  because the rasterizer can't handle overlapping cells we need to multiply the row and columns steps
    //  with an integer until the effective rasterizer pitch get big enough.
    m_row_steps *= (m_dim.x () - 1) / (m_row_steps * m_row_step.x ()) + 1;
    m_column_steps *= (m_dim.y () - 1) / (m_column_steps * m_column_step.y ()) + 1;

    db::Box fp_bbox = fp.box ();

    //  compensate for distortion by sheared kernel
    fp_bbox.enlarge (db::Vector (db::coord_traits<db::Coord>::rounded (double (fp_bbox.height ()) * std::abs (m_column_step.x ()) / dy), db::coord_traits<db::Coord>::rounded (double (fp_bbox.width ()) * std::abs (m_row_step.y ()) / dx)));

    int columns_per_rows = (int (m_row_steps) * m_row_step.y ()) / dy;
    int rows_per_columns = (int (m_column_steps) * m_column_step.x ()) / dx;

    db::Coord ddx = dx * db::Coord (m_row_steps) - m_column_step.x () * columns_per_rows;
    db::Coord ddy = dy * db::Coord (m_column_steps) - m_row_step.y () * rows_per_columns;

    //  round polygon bbox
    db::Coord fp_left = db::Coord (tl::round_down (fp_bbox.left () - m_origin.x (), ddx)) + m_origin.x ();
    db::Coord fp_bottom = db::Coord (tl::round_down (fp_bbox.bottom () - m_origin.y (), ddy)) + m_origin.y ();
    db::Coord fp_right = db::Coord (tl::round_up (fp_bbox.right () - m_origin.x (), ddx)) + m_origin.x ();
    db::Coord fp_top = db::Coord (tl::round_up (fp_bbox.top () - m_origin.y (), ddy)) + m_origin.y ();
    fp_bbox = db::Box (fp_left, fp_bottom, fp_right, fp_top);

    size_t nx = fp_bbox.width () / ddx;
    size_t ny = fp_bbox.height () / ddy;

    tl_assert (fp.box ().inside (fp_bbox));

    if (nx == 0 || ny == 0) {
      //  nothing to rasterize:
      return;
    }

    m_area_maps.reserve (m_row_steps * m_column_steps + std::abs (columns_per_rows) * std::abs (rows_per_columns));

    db::AreaMap am;

    for (unsigned int ic = 0; ic < m_column_steps; ++ic) {

      for (unsigned int ir = 0; ir < m_row_steps; ++ir) {

        db::Vector dr = m_row_step * long (ir);
        db::Vector dc = m_column_step * long (ic);

        am.reinitialize (db::Point (fp_left, fp_bottom) + dr + dc, db::Vector (ddx, ddy), m_dim, nx, ny);

        if (db::rasterize (fp, am)) {
          m_area_maps.push_back (db::AreaMap ());
          m_area_maps.back ().swap (am);
        }

      }

    }

    //  adds the "dead corner" piece

    for (unsigned int ic = 0; ic < (unsigned int) std::abs (columns_per_rows); ++ic) {

      for (unsigned int ir = 0; ir < (unsigned int) std::abs (rows_per_columns); ++ir) {

        db::Vector dr = m_row_step * long ((rows_per_columns > 0 ? -int (ir + 1) : ir) + m_row_steps);
        db::Vector dc = m_column_step * long ((columns_per_rows > 0 ? -int (ic + 1) : ic) + m_column_steps);

        am.reinitialize (db::Point (fp_left, fp_bottom) + dr + dc, db::Vector (ddx, ddy), m_dim, nx, ny);

        if (db::rasterize (fp, am)) {
          m_area_maps.push_back (db::AreaMap ());
          m_area_maps.back ().swap (am);
        }

      }

    }
  }

  const db::Point &p0 () const { return m_origin; }

  unsigned int row_steps () const { return m_row_steps; }
  unsigned int column_steps () const { return m_column_steps; }

  unsigned int area_maps () const
  {
    return (unsigned int) m_area_maps.size ();
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
  db::Vector m_dim;
};


static bool
fill_polygon_impl (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
                   std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin, const db::Box &glue_box)
{
  if (row_step.x () <= 0 || column_step.y () <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row step must have a positive x component while column step must have a positive y component")));
  }

  if (db::vprod_sign (row_step, column_step) <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row_step x column_step vector vector product must be > 0")));
  }

  db::Vector kernel_origin (fc_bbox.left (), fc_bbox.bottom ());

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

    if (fp->hull ().size () == 0) {
      continue;
    }

    //  disable enhanced mode an obey the origin if the polygon is not entirely inside and not at the boundary of the glue box
    bool ef = enhanced_fill;
    if (ef && ! glue_box.empty () && ! fp->box ().enlarged (db::Vector (1, 1)).inside (glue_box)) {
      ef = false;
    }

    //  pick a heuristic "good" starting point in enhanced mode
    //  TODO: this is a pretty weak optimization.
    db::Point o = origin;
    if (ef) {
      o = fp->hull () [0];
    }

    size_t ninsts = 0;

    GenericRasterizer am (*fp, row_step, column_step, o, fc_bbox.p2 () - fc_bbox.p1 ());

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

            {
              //  In case we run this from a tiling processor we need to lock against multithread races
              tl::MutexLocker locker (&db::TilingProcessor::output_lock ());
              cell->insert (array);
            }

            if (remaining_parts) {
              if (am1.d ().y () == am1.p ().y ()) {
                filled_regions.push_back (db::Polygon (db::Box (db::Point (), db::Point (am1.p ().x (), am1.p ().y () * db::Coord (jj - j))).moved (kernel_origin + p0)));
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

DB_PUBLIC bool
fill_region (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const Box &fc_box, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin, const db::Box &glue_box)
{
  return fill_polygon_impl (cell, fp0, fill_cell_index, fc_box, row_step, column_step, origin, enhanced_fill, remaining_parts, fill_margin, glue_box);
}

DB_PUBLIC bool
fill_region (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Point &origin, bool enhanced_fill,
             std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin, const db::Box &glue_box)
{
  if (fc_bbox.empty () || fc_bbox.width () == 0 || fc_bbox.height () == 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid fill cell footprint (empty or zero width/height)")));
  }

  return fill_polygon_impl (cell, fp0, fill_cell_index, fc_bbox, db::Vector (fc_bbox.width (), 0), db::Vector (0, fc_bbox.height ()), origin, enhanced_fill, remaining_parts, fill_margin, glue_box);
}

static void
fill_region_impl (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
                  db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, int iteration, const db::Box &glue_box)
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
      if (! fill_polygon_impl (cell, *p, fill_cell_index, fc_bbox, row_step, column_step, origin, enhanced_fill, remaining_parts ? &rem_pp : 0, fill_margin, glue_box)) {
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
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box)
{
  fill_region_impl (cell, fr, fill_cell_index, fc_bbox, row_step, column_step, origin, enhanced_fill, remaining_parts, fill_margin, remaining_polygons, 0, glue_box);
}

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Point &origin, bool enhanced_fill,
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box)
{
  if (fc_bbox.empty () || fc_bbox.width () == 0 || fc_bbox.height () == 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid fill cell footprint (empty or zero width/height)")));
  }

  fill_region_impl (cell, fr, fill_cell_index, fc_bbox, db::Vector (fc_bbox.width (), 0), db::Vector (0, fc_bbox.height ()),
                    origin, enhanced_fill, remaining_parts, fill_margin, remaining_polygons, 0, glue_box);
}

DB_PUBLIC void
fill_region_repeat (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index,
                    const db::Box &fc_box, const db::Vector &row_step, const db::Vector &column_step,
                    const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box)
{
  const db::Region *fill_region = &fr;

  db::Region new_fill_region;
  db::Region remaining;

  int iteration = 0;

  while (! fill_region->empty ()) {

    ++iteration;

    remaining.clear ();
    fill_region_impl (cell, *fill_region, fill_cell_index, fc_box, row_step, column_step, db::Point (), true, &remaining, fill_margin, remaining_polygons, iteration, glue_box);

    new_fill_region.swap (remaining);
    fill_region = &new_fill_region;

  }
}

}
