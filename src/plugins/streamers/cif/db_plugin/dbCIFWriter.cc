
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


#include "dbCIFWriter.h"
#include "dbPolygonGenerators.h"
#include "tlStream.h"
#include "tlUtils.h"

#include <time.h>
#include <string.h>

namespace db
{

// ---------------------------------------------------------------------------------
//  CIFWriter implementation

CIFWriter::CIFWriter ()
  : mp_stream (0),
    m_progress (tl::to_string (tr ("Writing CIF file")), 10000),
    m_needs_emit (false)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

CIFWriter &
CIFWriter::operator<<(const char *s)
{
  mp_stream->put(s, strlen(s));
  return *this;
}

CIFWriter &
CIFWriter::operator<<(const std::string &s)
{
  mp_stream->put(s.c_str(), s.size());
  return *this;
}

CIFWriter &
CIFWriter::operator<<(endl_tag)
{
  *this << "\n";
  return *this;
}

const char *
CIFWriter::xy_sep () const
{
  return m_options.blank_separator ? " " : ",";
}

void 
CIFWriter::write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options)
{
  stream.set_as_text (true);

  m_options = options.get_options<CIFWriterOptions> ();
  mp_stream = &stream;

  //  compute the scale factor to get to the 10 nm basic database unit of CIF
  double tl_scale = options.scale_factor () * layout.dbu () / 0.01;

  std::vector <std::pair <unsigned int, db::LayerProperties> > layers;
  options.get_valid_layers (layout, layers, db::SaveLayoutOptions::LP_AssignName);

  std::set <db::cell_index_type> cell_set;
  options.get_cells (layout, cell_set, layers);

  //  create a cell index vector sorted bottom-up
  std::vector <db::cell_index_type> cells;
  cells.reserve (cell_set.size ());

  for (db::Layout::bottom_up_const_iterator cell = layout.begin_bottom_up (); cell != layout.end_bottom_up (); ++cell) {
    if (cell_set.find (*cell) != cell_set.end ()) {
      cells.push_back (*cell);
    }
  }

  time_t t = time(NULL);
  struct tm tt = *localtime(&t);

  char timestr[100];
  strftime(timestr, sizeof (timestr), "%F %T", &tt);

  //  Write header
  *this << "(CIF file written " << (const char *)timestr << " by KLayout);" << m_endl;

  //  TODO: this can be done more intelligently ..
  int tl_scale_divider;
  int tl_scale_denom;
  for (tl_scale_divider = 1; tl_scale_divider < 1000; ++tl_scale_divider) {
    tl_scale_denom = int (floor (0.5 + tl_scale * tl_scale_divider));
    if (fabs (tl_scale_denom - tl_scale * tl_scale_divider) < 1e-6) {
      break;
    }
  }

  int cell_index = 0;
  std::map <db::cell_index_type, int> db_to_cif_index_map;
  std::set <db::cell_index_type> called_cells;

  //  body
  for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end (); ++cell) {

    m_progress.set (mp_stream->pos ());

    //  cell body 
    const db::Cell &cref (layout.cell (*cell));

    ++cell_index;
    db_to_cif_index_map.insert (std::make_pair (*cell, cell_index));

    double sf = 1.0;

    *this << "DS " << cell_index << " " << tl_scale_denom << " " << tl_scale_divider << ";" << m_endl;
    *this << "9 " << tl::to_word_or_quoted_string (layout.cell_name (*cell)) << ";" << m_endl;

    //  instances
    for (db::Cell::const_iterator inst = cref.begin (); ! inst.at_end (); ++inst) {

      //  write only instances to selected cells
      if (cell_set.find (inst->cell_index ()) != cell_set.end ()) {

        called_cells.insert (inst->cell_index ());

        m_progress.set (mp_stream->pos ());

        std::map<db::cell_index_type, int>::const_iterator cif_index = db_to_cif_index_map.find (inst->cell_index ());
        tl_assert(cif_index != db_to_cif_index_map.end ());

        // resolve instance arrays
        for (db::Cell::cell_inst_array_type::iterator pp = inst->begin (); ! pp.at_end (); ++pp) {

          *this << "C" << cif_index->second;
          
          // convert the transformation into CIF's notation

          db::CplxTrans t (inst->complex_trans (*pp));
          db::Vector d (t.disp() * sf);

          if (t.is_mirror()) {
            *this << " MY";
          }
          
          double a = t.angle();
          while (a < 0) {
            a += 360.0;
          }
          double ya = 0.0, xa = 0.0;
          if (a < 45 || a > 315) {
            xa = 1.0;
            ya = tan(a / 180.0 * M_PI);
          } else if (a < 135) {
            xa = 1.0 / tan(a / 180.0 * M_PI);
            ya = 1.0;
          } else if (a < 225) {
            xa = -1.0;
            ya = tan(a / 180.0 * M_PI);
          } else {
            xa = 1.0 / tan(a / 180.0 * M_PI);
            ya = -1.0;
          } 

          //  TODO: that can be done smarter ...
          while (fabs (xa - floor (0.5 + xa)) > 1e-3 || fabs (ya - floor (0.5 + ya)) > 1e-3) {
            xa *= 2.0;
            ya *= 2.0;
          }

          *this << " R" << floor (0.5 + xa) << xy_sep () << floor (0.5 + ya);

          *this << " T" << d.x() << xy_sep () << d.y();

          *this << ";" << m_endl;

        }

      }

    }

    //  shapes
    for (std::vector <std::pair <unsigned int, db::LayerProperties> >::const_iterator l = layers.begin (); l != layers.end (); ++l) {

      m_needs_emit = true;
      m_layer = l->second;

      write_texts (layout, cref, l->first, sf);
      write_polygons (layout, cref, l->first, sf);
      write_paths (layout, cref, l->first, sf);
      write_boxes (layout, cref, l->first, sf);

      m_progress.set (mp_stream->pos ());

    }

    //  end of cell
    *this << "DF;" << m_endl;

  }

  if (m_options.dummy_calls) {

    //  If requested, write dummy calls for all top cells
    for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end (); ++cell) {

      if (called_cells.find (*cell) == called_cells.end ()) {

        std::map<db::cell_index_type, int>::const_iterator cif_index = db_to_cif_index_map.find (*cell);
        tl_assert(cif_index != db_to_cif_index_map.end ());
        *this << "C" << cif_index->second << ";" << m_endl;

      }

    }

  }

  //  end of file
  *this << "E" << m_endl;

  m_progress.set (mp_stream->pos ());

}

void 
CIFWriter::emit_layer()
{
  if (m_needs_emit) {
    m_needs_emit = false;
    *this << "L " << tl::to_word_or_quoted_string (tl::to_upper_case (m_layer.name), "0123456789_.$") << ";" << m_endl;
  }
}

void 
CIFWriter::write_texts (const db::Layout &layout, const db::Cell &cell, unsigned int layer, double sf)
{
  db::ShapeIterator shape (cell.shapes (layer).begin (db::ShapeIterator::Texts));
  while (! shape.at_end ()) {

    m_progress.set (mp_stream->pos ());

    emit_layer ();

    *this << "94 " << tl::to_word_or_quoted_string (shape->text_string(), "0123456789:<>/&%$!.-_#+*?\\[]{}");

    double h = shape->text_size () * layout.dbu ();

    db::Vector p (shape->text_trans ().disp () * sf);
    *this << " " << p.x() << xy_sep () << p.y () << " " << h << ";" << m_endl;

    ++shape;

  }
}

void 
CIFWriter::write_polygons (const db::Layout & /*layout*/, const db::Cell &cell, unsigned int layer, double sf)
{
  db::ShapeIterator shape (cell.shapes (layer).begin (db::ShapeIterator::Polygons));
  while (! shape.at_end ()) {

    m_progress.set (mp_stream->pos ());

    db::Polygon poly;
    shape->polygon (poly);

    if (poly.holes () > 0) {

      //  resolve holes or merge polygon as a preparation step for split_polygon which only works properly
      //  on merged polygons ...
      std::vector<db::Polygon> polygons;

      db::EdgeProcessor ep;
      ep.insert_sequence (poly.begin_edge ());
      db::PolygonContainer pc (polygons);
      db::PolygonGenerator out (pc, true /*resolve holes*/, false /*min coherence for splitting*/);
      db::SimpleMerge op;
      ep.process (out, op);

      for (std::vector<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
        write_polygon (*p, sf);
      }

    } else {
      write_polygon (poly, sf);
    }

    ++shape;

  }
}

void 
CIFWriter::write_polygon (const db::Polygon &polygon, double sf)
{
  emit_layer ();
  *this << "P";
  for (db::Polygon::polygon_contour_iterator p = polygon.begin_hull (); p != polygon.end_hull (); ++p) {
    db::Point pp (*p * sf);
    *this << " " << pp.x () << xy_sep () << pp.y ();
  }
  *this << ";" << m_endl;
}

void 
CIFWriter::write_boxes (const db::Layout & /*layout*/, const db::Cell &cell, unsigned int layer, double sf)
{
  db::ShapeIterator shape (cell.shapes (layer).begin (db::ShapeIterator::Boxes));
  while (! shape.at_end ()) {

    m_progress.set (mp_stream->pos ());

    emit_layer ();

    db::Box b (shape->bbox () * sf);
    *this << "B " << b.width () << " " << b.height () << " " << b.center ().x () << xy_sep () << b.center ().y () << ";" << m_endl;

    ++shape;

  }
}

void 
CIFWriter::write_paths (const db::Layout & /*layout*/, const db::Cell &cell, unsigned int layer, double sf)
{
  db::ShapeIterator shape (cell.shapes (layer).begin (db::ShapeIterator::Paths));
  while (! shape.at_end ()) {

    m_progress.set (mp_stream->pos ());

#if 0 

    //  "official" code: write only round paths as such - other paths are converted to polygons
    if (shape->round_path ()) {

      emit_layer ();

      *this << "W " << long (floor (0.5 + sf * shape->path_width ()));

      for (db::Shape::point_iterator p = shape->begin_point (); p != shape->end_point (); ++p) {
        db::Point pp (*p * sf);
        *this << " " << pp.x () << xy_sep () << pp.y ();
      }

      *this << ";" << endl;

    } else {
      db::Polygon poly;
      shape->polygon (poly);
      write_polygon (poly, sf);
    }

#else

    //  Use 98 extension for path type. Only use polygons for custom extensions.
    int path_type = -1;
    if (shape->round_path ()) {
      if (shape->path_extensions ().first == shape->path_width () / 2 && shape->path_extensions ().second == shape->path_width () / 2) {
        path_type = 1;
      }
    } else {
      if (shape->path_extensions ().first == 0 && shape->path_extensions ().second == 0) {
        path_type = 0;
      } else if (shape->path_extensions ().first == shape->path_width () / 2 && shape->path_extensions ().second == shape->path_width () / 2) {
        path_type = 2;
      }
    }

    size_t npts = 0;
    for (db::Shape::point_iterator p = shape->begin_point (); p != shape->end_point () && npts < 2; ++p) {
      ++npts;
    }

    if (npts == 0) {

      //  ignore paths with zero points

    } else if (path_type == 1 && npts == 1) {

      //  produce a round flash for single-point round paths

      emit_layer ();

      *this << "R " << long (floor (0.5 + sf * shape->path_width ()));

      db::Point pp (*shape->begin_point () * sf);
      *this << " " << pp.x () << xy_sep () << pp.y ();

      *this << ";" << m_endl;

    } else if (path_type >= 0 && npts > 1) {

      emit_layer ();

      *this << "98 " << path_type << ";" << m_endl;

      *this << "W " << long (floor (0.5 + sf * shape->path_width ()));

      for (db::Shape::point_iterator p = shape->begin_point (); p != shape->end_point (); ++p) {
        db::Point pp (*p * sf);
        *this << " " << pp.x () << xy_sep () << pp.y ();
      }

      *this << ";" << m_endl;

    } else {
      db::Polygon poly;
      shape->polygon (poly);
      write_polygon (poly, sf);
    }

#endif

    ++shape;

  }
}

}

