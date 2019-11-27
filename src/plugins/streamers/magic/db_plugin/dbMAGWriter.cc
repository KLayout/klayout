
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "dbMAGWriter.h"
#include "dbPolygonGenerators.h"
#include "dbPolygonTools.h"
#include "tlStream.h"
#include "tlUtils.h"
#include "tlFileUtils.h"
#include "tlUri.h"

#include <time.h>
#include <string.h>

namespace db
{

// ---------------------------------------------------------------------------------
//  MAGWriter implementation

MAGWriter::MAGWriter ()
  : mp_stream (0),
    m_progress (tl::to_string (tr ("Writing Magic file")), 10000)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
  m_timestamp = 0;
}

void 
MAGWriter::write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options)
{
  m_options = options.get_options<MAGWriterOptions> ();
  mp_stream = &stream;

  m_base_uri = tl::URI (stream.path ());
  m_ext = tl::extension (m_base_uri.path ());
  m_base_uri.set_path (tl::dirname (m_base_uri.path ()));

  m_cells_written.clear ();
  m_cells_to_write.clear ();
  m_layer_names.clear ();
  m_timestamp = 0;  //  @@@ set timestamp?

  if (layout.end_top_cells () - layout.begin_top_down () == 1) {

    //  write the one top cell to the given stream. Otherwise
    write_cell (*layout.begin_top_down (), layout, stream);

  } else {

    stream << "# KLayout is not writing this file as there are multiple top cells - see those files for the individual cells.";

    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_cells (); ++c) {
      m_cells_to_write.insert (std::make_pair (*c, filename_for_cell (*c, layout)));
    }

  }

  while (! m_cells_to_write.empty ()) {

    std::map<db::cell_index_type, std::string> cells_to_write;
    cells_to_write.swap (m_cells_to_write);

    for (std::map<db::cell_index_type, std::string>::const_iterator cw = cells_to_write.begin (); cw != cells_to_write.end (); ++cw) {
      tl::OutputStream os (cw->second, tl::OutputStream::OM_Auto, true);
      write_cell (cw->first, layout, os);
    }

  }
}

std::string
MAGWriter::layer_name (unsigned int li, const Layout &layout)
{
  //  @@@ TODO: avoid built-in names, like "end", "space", "labels" ...

  std::map<unsigned int, std::string>::const_iterator i = m_layer_names.find (li);
  if (i != m_layer_names.end ()) {
    return i->second;
  }

  if (m_layer_names.empty ()) {

    for (db::Layout::layer_iterator i = layout.begin_layers (); i != layout.end_layers (); ++i) {
      db::LayerProperties lp = layout.get_properties ((*i).first);
      if (lp.is_named ()) {
        m_layer_names.insert (std::make_pair ((*i).first, lp.name));
      }
    }

    for (db::Layout::layer_iterator i = layout.begin_layers (); i != layout.end_layers (); ++i) {
      db::LayerProperties lp = layout.get_properties ((*i).first);
      if (! lp.is_named ()) {
        //  @@@ TODO: avoid duplicates
        std::string ld_name = std::string ("L") + tl::to_string (lp.layer);
        if (lp.datatype) {
          ld_name += "D";
          ld_name += tl::to_string (lp.datatype);
        }
        m_layer_names.insert (std::make_pair ((*i).first, ld_name));
      }
    }

  }

  return m_layer_names [li];
}

std::string
MAGWriter::filename_for_cell (db::cell_index_type ci, db::Layout &layout)
{
  tl::URI uri (m_base_uri);
  if (uri.path ().empty ()) {
    uri.set_path (std::string (layout.cell_name (ci)) + m_ext);
  }
  return uri.to_string ();
}

void
MAGWriter::write_cell (db::cell_index_type ci, db::Layout &layout, tl::OutputStream &os)
{
  os.set_as_text (true);
  os << "magic\n";

  //  @@@ write tech

  os << "timestamp " << m_timestamp << "\n";

  db::Cell &cell = layout.cell (ci);

  for (db::Layout::layer_iterator i = layout.begin_layers (); i != layout.end_layers (); ++i) {

    unsigned int li = (*i).first;
    if (! cell.shapes (li).empty ()) {
      os << "<< " << tl::to_word_or_quoted_string (layer_name (li, layout)) << " >>\n";
      for (db::Shapes::shape_iterator s = cell.shapes (li).begin (db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Paths); ! s.at_end (); ++s) {
        write_polygon (s->polygon (), layout, os);
      }
    }

  }

  bool any = false;

  for (db::Layout::layer_iterator i = layout.begin_layers (); i != layout.end_layers (); ++i) {
    for (db::Shapes::shape_iterator s = cell.shapes ((*i).first).begin (db::ShapeIterator::Texts); ! s.at_end (); ++s) {
      if (! any) {
        os << "<< labels >>\n";
      }
      write_label (layer_name ((*i).first, layout), s->text (), layout, os);
    }
  }

  m_cell_id.clear ();
  for (db::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
    if (m_cells_written.find (i->cell_index ()) == m_cells_written.end ()) {
      m_cells_written.insert (i->cell_index ());
      m_cells_to_write.insert (std::make_pair (i->cell_index (), filename_for_cell (i->cell_index (), layout)));
    }
    write_instance (i->cell_inst (), layout, os);
  }
}

namespace {

  class TrapezoidWriter
    : public SimplePolygonSink
  {
  public:
    TrapezoidWriter (tl::OutputStream &os, double scale)
      : mp_os (&os), m_scale (scale)
    { }

    virtual void put (const db::SimplePolygon &polygon)
    {
      if (! polygon.is_box ()) {
        //  @@@ TODO: handle non-boxes
      }

      db::DBox b = db::DBox (polygon.box ()) * m_scale;
      //  @@@ TODO: floating coords on output?
      (*mp_os) << "rect " << b.left () << " " << b.bottom () << " " << b.right () << " " << b.top () << "\n";
    }

  private:
    tl::OutputStream *mp_os;
    double m_scale;
  };
}

void
MAGWriter::write_polygon (const db::Polygon &poly, const db::Layout &layout, tl::OutputStream &os)
{
  TrapezoidWriter writer (os, layout.dbu () / m_options.lambda);
  db::decompose_trapezoids (poly, TD_simple, writer);
}

void
MAGWriter::write_label (const std::string &layer, const db::Text &text, const db::Layout &layout, tl::OutputStream &os)
{
  db::DVector v = db::DVector (text.trans ().disp ()) * (layout.dbu () / m_options.lambda);

  std::string s = text.string ();
  if (s.find ("\n") != std::string::npos) {
    s = tl::replaced (s, "\n", "\\n");
  }

  os << "rlabel " << tl::to_word_or_quoted_string (layer) << " " << v.x () << " " << v.y () << " " << v.x () << " " << v.y () << " 0 " << s << "\n";
}

void
MAGWriter::write_instance (const db::CellInstArray &inst, const db::Layout &layout, tl::OutputStream &os)
{
  double sf = layout.dbu () / m_options.lambda;

  int id = (m_cell_id [inst.object ().cell_index ()] += 1);
  std::string cn = layout.cell_name (inst.object ().cell_index ());
  os << "use " << tl::to_word_or_quoted_string (cn) << " " << tl::to_word_or_quoted_string (cn + "_" + tl::to_string (id));

  os << "timestamp " << m_timestamp << "\n";

  db::ICplxTrans tr = inst.complex_trans ();
  db::Matrix2d m = tr.to_matrix2d ();

  db::DVector d = db::DVector (tr.disp ()) * sf;
  os << "transform " << m.m11 () << " " << m.m12 () << " " << d.x () << " " << m.m21 () << " " << m.m22 () << " " << d.y () << "\n";

  {
    db::Vector a, b;
    unsigned long na = 0, nb = 0;
    if (inst.is_regular_array (a, b, na, nb) && ((a.x () == 0 && b.y () == 0) || (a.y () == 0 && b.x () == 0))) {

      na = std::max ((unsigned long) 1, na);
      nb = std::max ((unsigned long) 1, nb);

      if (a.y () != 0) {
        std::swap (a, b);
        std::swap (na, nb);
      }

      db::DVector da = db::DVector (a) * sf;
      db::DVector db = db::DVector (b) * sf;
      os << "array " << 0 << " " << (na - 1) << " " << da.x () << " " << 0 << " " << (nb - 1) << " " << db.y () << "\n";

    }
  }

  {
    db::DBox b = db::DBox (inst.bbox (db::box_convert<db::CellInst> ())) * sf;
    os << "box " << b.left () << " " << b.bottom () << " " << b.right () << " " << b.top () << "\n";
  }
}

}

