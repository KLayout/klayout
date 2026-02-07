
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

  GenericRasterizer (const std::vector<db::Polygon> &fr, const db::Box &rasterized_area, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, const db::Vector &dim)
    : m_row_step (row_step), m_column_step (column_step), m_row_steps (0), m_column_steps (0), m_origin (origin), m_dim (dim)
  {
    rasterize (rasterized_area, fr);
  }

  GenericRasterizer (const db::Polygon &fp, const db::Box &rasterized_area, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, const db::Vector &dim)
    : m_row_step (row_step), m_column_step (column_step), m_row_steps (0), m_column_steps (0), m_origin (origin), m_dim (dim)
  {
    std::vector<db::Polygon> fr;
    fr.push_back (fp);
    rasterize (rasterized_area, fr);
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

  const db::Point &p0 () const { return m_origin; }

  unsigned int row_steps () const { return m_row_steps; }
  unsigned int column_steps () const { return m_column_steps; }

  unsigned int area_maps () const
  {
    return (unsigned int) m_area_maps.size ();
  }

  int index_for_p0 (const db::Point &p0) const
  {
    for (auto i = m_area_maps.begin (); i != m_area_maps.end (); ++i) {
      if (i->p0 () == p0) {
        return int (i - m_area_maps.begin ());
      }
    }
    return -1;
  }

  const db::AreaMap &area_map (unsigned int i) const
  {
    return m_area_maps [i];
  }

  db::AreaMap &area_map (unsigned int i)
  {
    return m_area_maps [i];
  }

private:
  std::vector<db::AreaMap> m_area_maps;
  db::Vector m_row_step, m_column_step;
  unsigned int m_row_steps, m_column_steps;
  db::Point m_origin;
  db::Vector m_dim;

  void rasterize (const db::Box &rasterized_area, const std::vector<db::Polygon> &fr)
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
    //  with an integer until the effective rasterizer pitch gets big enough.
    m_row_steps *= (m_dim.x () - 1) / (m_row_steps * m_row_step.x ()) + 1;
    m_column_steps *= (m_dim.y () - 1) / (m_column_steps * m_column_step.y ()) + 1;

    db::Box ra_org = rasterized_area;

    //  compensate for distortion by sheared kernel
    db::Coord ex = std::max (std::abs (db::Coord (m_column_step.x () * m_column_steps)), std::abs (db::Coord (m_row_step.x () * m_row_steps)));
    db::Coord ey = std::max (std::abs (db::Coord (m_column_step.y () * m_column_steps)), std::abs (db::Coord (m_row_step.y () * m_row_steps)));
    ra_org.enlarge (db::Vector (ex, ey));

    int columns_per_rows = (int (m_row_steps) * m_row_step.y ()) / dy;
    int rows_per_columns = (int (m_column_steps) * m_column_step.x ()) / dx;

    db::Coord ddx = dx * db::Coord (m_row_steps) - m_column_step.x () * columns_per_rows;
    db::Coord ddy = dy * db::Coord (m_column_steps) - m_row_step.y () * rows_per_columns;

    //  round polygon bbox
    db::Coord ra_left = db::Coord (tl::round_down (ra_org.left () - m_origin.x (), ddx)) + m_origin.x ();
    db::Coord ra_bottom = db::Coord (tl::round_down (ra_org.bottom () - m_origin.y (), ddy)) + m_origin.y ();
    db::Coord ra_right = db::Coord (tl::round_up (ra_org.right () - m_origin.x (), ddx)) + m_origin.x ();
    db::Coord ra_top = db::Coord (tl::round_up (ra_org.top () - m_origin.y (), ddy)) + m_origin.y ();
    db::Box ra = db::Box (ra_left, ra_bottom, ra_right, ra_top);

    size_t nx = ra.width () / ddx;
    size_t ny = ra.height () / ddy;

    tl_assert (ra_org.inside (ra));

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

        am.reinitialize (db::Point (ra_left, ra_bottom) + dr + dc, db::Vector (ddx, ddy), m_dim, nx, ny);

        bool any = false;
        for (auto i = fr.begin (); i != fr.end (); ++i) {
          if (db::rasterize (*i, am)) {
            any = true;
          }
        }
        if (any) {
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

        am.reinitialize (db::Point (ra_left, ra_bottom) + dr + dc, db::Vector (ddx, ddy), m_dim, nx, ny);

        bool any = false;
        for (auto i = fr.begin (); i != fr.end (); ++i) {
          if (db::rasterize (*i, am)) {
            any = true;
          }
        }
        if (any) {
          m_area_maps.push_back (db::AreaMap ());
          m_area_maps.back ().swap (am);
        }

      }

    }
  }
};

static size_t
create_instances (GenericRasterizer &am, db::Cell *cell, db::cell_index_type fill_cell_index, const db::Vector &kernel_origin, const db::Vector &fill_margin, const GenericRasterizer *exclude_rasterized, std::vector<db::Polygon> *filled_regions)
{
  size_t ninsts = 0;

  for (unsigned int i = 0; i < am.area_maps (); ++i) {

    db::AreaMap &am1 = am.area_map (i);
    const db::AreaMap *am1_excl = 0;
    if (exclude_rasterized) {
      int ie = exclude_rasterized->index_for_p0 (am1.p0 ());
      if (ie >= 0) {
        am1_excl = &exclude_rasterized->area_map (ie);
      }
    }

    size_t nx = am1.nx ();
    size_t ny = am1.ny ();

    //  Create the fill cell instances
    for (size_t i = 0; i < nx; ++i) {

      for (size_t j = 0; j < ny; ) {

        size_t jj = j + 1;
        if (am1.get (i, j) == am1.pixel_area () && (!am1_excl || am1_excl->get (i, j) == 0)) {

          while (jj != ny && am1.get (i, jj) == am1.pixel_area () && (!am1_excl || am1_excl->get (i, jj) == 0)) {
            ++jj;
          }

          db::Vector p0 = (am1.p0 () - db::Point ()) - kernel_origin;
          p0 += db::Vector (i * am1.d ().x (), j * am1.d ().y ());

          db::CellInstArray array;

          //  try to expand the array in x direction
          size_t ii = i + 1;
          for ( ; ii < nx; ++ii) {
            bool all = true;
            for (size_t k = j; k < jj && all; ++k) {
              all = (am1.get (ii, k) == am1.pixel_area () && (!am1_excl || am1_excl->get (ii, k) == 0));
            }
            if (all) {
              for (size_t k = j; k < jj; ++k) {
                //  disable pixel, so we do not see it again in the following columns
                am1.get (ii, k) = 0;
              }
            } else {
              break;
            }
          }

          ninsts += (jj - j) * (ii - i);

          if (jj > j + 1 || ii > i + 1) {
            array = db::CellInstArray (db::CellInst (fill_cell_index), db::Trans (p0), db::Vector (0, am1.d ().y ()), db::Vector (am1.d ().x (), 0), (unsigned long) (jj - j), (unsigned long) (ii - i));
          } else {
            array = db::CellInstArray (db::CellInst (fill_cell_index), db::Trans (p0));
          }

          {
            //  In case we run this from a tiling processor we need to lock against multithread races
            tl_assert (cell->layout () != 0);
            tl::MutexLocker locker (&cell->layout ()->lock ());
            cell->insert (array);
          }

          if (filled_regions) {
            if (am1.d ().y () == am1.p ().y () && am1.d ().x () == am1.p ().x ()) {
              db::Box fill_box (db::Point (), db::Point (am1.p ().x () * db::Coord (ii - i), am1.p ().y () * db::Coord (jj - j)));
              filled_regions->push_back (db::Polygon (fill_box.enlarged (fill_margin).moved (kernel_origin + p0)));
            } else {
              db::Box fill_box (db::Point (), db::Point () + am1.p ());
              fill_box.enlarge (fill_margin);
              for (size_t k = 0; k < jj - j; ++k) {
                for (size_t l = 0; l < ii - i; ++l) {
                  filled_regions->push_back (db::Polygon (fill_box.moved (kernel_origin + p0 + db::Vector (am1.d ().x () * db::Coord (l), am1.d ().y () * db::Coord (k)))));
                }
              }
            }
          }

        }

        j = jj;

      }

    }

  }

  return ninsts;
}

static bool
fill_polygon_impl (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
                   std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin, const db::Box &glue_box, const db::Region &exclude_area)
{
  if (row_step.x () <= 0 || column_step.y () <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row step must have a positive x component while column step must have a positive y component")));
  }

  if (db::vprod_sign (row_step, column_step) <= 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid row or column step vectors in fill_region: row_step x column_step vector vector product must be > 0")));
  }

  db::Vector kernel_origin (fc_bbox.left (), fc_bbox.bottom ());

  db::Coord dx = fc_bbox.width () / 2 - 1, dy = fc_bbox.height () / 2 - 1;

  db::Region fr (fp0);
  db::Box rasterized_area = fp0.box ();

  std::unique_ptr<GenericRasterizer> exclude_rasterized;
  bool has_exclude_area = false;

  if (! exclude_area.empty ()) {

    auto it = exclude_area.begin_iter ();
    it.first.confine_region (fp0.box ());

    //  over- and undersize the polygons to fill gaps that cannot be filled.
    db::Region excluded (it.first, it.second);
    excluded.set_merged_semantics (false);
    excluded.size (dx, 0, 3 /*mode*/);
    excluded.set_merged_semantics (true);
    excluded.size (-dx, 0, 3 /*mode*/);
    excluded.set_merged_semantics (false);
    excluded.size (0, dy, 3 /*mode*/);
    excluded.set_merged_semantics (true);
    excluded.size (0, -dy, 3 /*mode*/);
    excluded.merge ();

    if (! excluded.empty ()) {

      has_exclude_area = true;

      if (enhanced_fill || remaining_parts != 0) {

        //  In enhanced fill or if the remaining parts are requested, it is better to implement the
        //  exclude area by a boolean NOT
        fr -= excluded;

      } else {

        //  Otherwise use a second rasterizer for the exclude polygons that must have a zero pixel coverage for the
        //  pixel to be filled.

        std::vector<db::Polygon> excluded_poly;
        excluded_poly.reserve (excluded.count ());
        for (auto i = excluded.begin (); ! i.at_end (); ++i) {
          excluded_poly.push_back (*i);
        }
        excluded.clear ();

        exclude_rasterized.reset (new GenericRasterizer (excluded_poly, rasterized_area, row_step, column_step, origin, fc_bbox.p2 () - fc_bbox.p1 ()));

      }

    }

  }

  std::vector <db::Polygon> filled_poly, filled_poly_uncleaned;

  //  save the uncleaned polygons, so we subtract the filled parts to
  //  form the remaining parts
  if (remaining_parts) {
    filled_poly_uncleaned.reserve (fr.count ());
    for (auto i = fr.begin (); ! i.at_end (); ++i) {
      filled_poly_uncleaned.push_back (*i);
    }
  }

  //  under- and oversize the polygon to remove slivers that cannot be filled.
  fr.set_merged_semantics (true);
  fr.size (-dx, 0, 3 /*mode*/);
  fr.set_merged_semantics (false);
  fr.size (dx, 0, 3 /*mode*/);
  fr.set_merged_semantics (true);
  fr.size (0, -dy, 3 /*mode*/);
  fr.set_merged_semantics (false);
  fr.size (0, dy, 3 /*mode*/);
  fr.set_merged_semantics (true);
  fr.merge ();

  filled_poly.reserve (fr.count ());
  for (auto i = fr.begin (); ! i.at_end (); ++i) {
    filled_poly.push_back (*i);
  }

  fr.clear ();

  std::vector <db::Polygon> filled_regions;
  bool any_fill = false;

  if (filled_poly.empty ()) {

    //  not need to do anything

  } else if (exclude_rasterized.get ()) {

    tl_assert (remaining_parts == 0);
    GenericRasterizer am (filled_poly, rasterized_area, row_step, column_step, origin, fc_bbox.p2 () - fc_bbox.p1 ());

    size_t ninsts = create_instances (am, cell, fill_cell_index, kernel_origin, fill_margin, exclude_rasterized.get (), 0);
    if (ninsts > 0) {
      any_fill = true;
    }

    if (tl::verbosity () >= 30 && ninsts > 0) {
      tl::info << "Part " << fp0.to_string ();
      tl::info << "Created " << ninsts << " instances";
    }

  } else {

    for (auto fp = filled_poly.begin (); fp != filled_poly.end (); ++fp) {

      if (fp->is_empty ()) {
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

      GenericRasterizer am (*fp, rasterized_area, row_step, column_step, o, fc_bbox.p2 () - fc_bbox.p1 ());

      size_t ninsts = create_instances (am, cell, fill_cell_index, kernel_origin, fill_margin, 0, remaining_parts ? &filled_regions : 0);
      if (ninsts > 0) {
        any_fill = true;
      }

      if (tl::verbosity () >= 30 && ninsts > 0) {
        tl::info << "Part " << fp->to_string ();
        tl::info << "Created " << ninsts << " instances";
      }

    }

  }

  if (any_fill || has_exclude_area) {

    if (remaining_parts) {
      db::EdgeProcessor ep;
      ep.boolean (filled_poly_uncleaned, filled_regions, *remaining_parts, db::BooleanOp::ANotB, false /*=don't resolve holes*/);
    }

    return true;

  } else {
    return false;
  }
}

DB_PUBLIC bool
fill_region (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const Box &fc_box, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin, const db::Box &glue_box, const db::Region &exclude_area)
{
  return fill_polygon_impl (cell, fp0, fill_cell_index, fc_box, row_step, column_step, origin, enhanced_fill, remaining_parts, fill_margin, glue_box, exclude_area);
}

DB_PUBLIC bool
fill_region (db::Cell *cell, const db::Polygon &fp0, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Point &origin, bool enhanced_fill,
             std::vector <db::Polygon> *remaining_parts, const db::Vector &fill_margin, const db::Box &glue_box, const db::Region &exclude_area)
{
  if (fc_bbox.empty () || fc_bbox.width () == 0 || fc_bbox.height () == 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid fill cell footprint (empty or zero width/height)")));
  }

  return fill_polygon_impl (cell, fp0, fill_cell_index, fc_bbox, db::Vector (fc_bbox.width (), 0), db::Vector (0, fc_bbox.height ()), origin, enhanced_fill, remaining_parts, fill_margin, glue_box, exclude_area);
}

static void
fill_region_impl (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
                  db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, int iteration, const db::Box &glue_box, const db::Region &exclude_area)
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
      if (! fill_polygon_impl (cell, *p, fill_cell_index, fc_bbox, row_step, column_step, origin, enhanced_fill, remaining_parts ? &rem_pp : 0, fill_margin, glue_box, exclude_area)) {
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
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box, const db::Region &exclude_area)
{
  fill_region_impl (cell, fr, fill_cell_index, fc_bbox, row_step, column_step, origin, enhanced_fill, remaining_parts, fill_margin, remaining_polygons, 0, glue_box, exclude_area);
}

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_bbox, const db::Point &origin, bool enhanced_fill,
             db::Region *remaining_parts, const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box, const db::Region &exclude_area)
{
  if (fc_bbox.empty () || fc_bbox.width () == 0 || fc_bbox.height () == 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid fill cell footprint (empty or zero width/height)")));
  }

  fill_region_impl (cell, fr, fill_cell_index, fc_bbox, db::Vector (fc_bbox.width (), 0), db::Vector (0, fc_bbox.height ()),
                    origin, enhanced_fill, remaining_parts, fill_margin, remaining_polygons, 0, glue_box, exclude_area);
}

DB_PUBLIC void
fill_region_repeat (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index,
                    const db::Box &fc_box, const db::Vector &row_step, const db::Vector &column_step,
                    const db::Vector &fill_margin, db::Region *remaining_polygons, const db::Box &glue_box, const db::Region &exclude_area)
{
  const db::Region *fill_region = &fr;

  db::Region new_fill_region;
  db::Region remaining;

  int iteration = 0;

  while (! fill_region->empty ()) {

    ++iteration;

    remaining.clear ();
    fill_region_impl (cell, *fill_region, fill_cell_index, fc_box, row_step, column_step, db::Point (), true, &remaining, fill_margin, remaining_polygons, iteration, glue_box, exclude_area);

    new_fill_region.swap (remaining);
    fill_region = &new_fill_region;

  }
}

}
