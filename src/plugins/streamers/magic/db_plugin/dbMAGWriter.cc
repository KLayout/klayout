
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


#include "dbMAGWriter.h"
#include "dbPolygonGenerators.h"
#include "dbPolygonTools.h"
#include "tlStream.h"
#include "tlUtils.h"
#include "tlFileUtils.h"
#include "tlUri.h"
#include "tlLog.h"
#include "tlUniqueName.h"
#include "tlTimer.h"

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
  std::vector <std::pair <unsigned int, db::LayerProperties> > layers;
  options.get_valid_layers (layout, layers, db::SaveLayoutOptions::LP_AssignNameWithPriority);

  std::set <db::cell_index_type> cell_set;
  options.get_cells (layout, cell_set, layers);

  tl::URI src (stream.path ());
  std::string basename = tl::basename (src.path ());
  std::pair<bool, db::cell_index_type> ci = layout.cell_by_name (basename.c_str ());
  if (! ci.first || cell_set.find (ci.second) == cell_set.end ()) {
    tl::warn << tl::to_string (tr ("The output file is not corresponding to an existing cell name. The content of this cell will not be a real layout: ")) << basename;
  }

  m_options = options.get_options<MAGWriterOptions> ();
  mp_stream = &stream;

  m_base_uri = tl::URI (stream.path ());
  m_ext = tl::extension (m_base_uri.path ());
  m_base_uri.set_path (tl::dirname (m_base_uri.path ()));

  m_timestamp = 0;
  if (m_options.write_timestamp) {
    timespec ts;
    tl::current_utc_time (&ts);
    m_timestamp = ts.tv_sec;
  }

  double lambda = m_options.lambda;
  if (lambda <= 0.0) {
    const tl::Variant &lv = layout.meta_info ("lambda").value;
    if (lv.is_nil ()) {
      throw tl::Exception (tl::to_string (tr ("No lambda value configured for MAG writer and no 'lambda' metadata present in layout.")));
    } else if (lv.is_a_string ()) {
      tl::from_string (lv.to_string (), lambda);
    } else if (lv.can_convert_to_double ()) {
      lambda = lv.to_double ();
    }
  }

  m_sf = layout.dbu () / lambda;

  //  As a favor, write a dummy top level file before closing the stream. If the file name corresponds to a real cell,
  //  this file is overwritten by the true cell.
  write_dummy_top (cell_set, layout, stream);
  stream.close ();

  for (std::set<db::cell_index_type>::const_iterator c = cell_set.begin (); c != cell_set.end (); ++c) {
    tl::OutputStream os (filename_for_cell (*c, layout), tl::OutputStream::OM_Auto, true);
    write_cell (*c, layers, layout, os);
  }
}

std::string
MAGWriter::filename_for_cell (db::cell_index_type ci, db::Layout &layout)
{
  tl::URI uri (m_base_uri);
  if (uri.path ().empty ()) {
    uri.set_path (make_string (layout.cell_name (ci)) + "." + m_ext);
  } else {
    uri.set_path (uri.path () + "/" + make_string (layout.cell_name (ci)) + "." + m_ext);
  }
  return uri.to_string ();
}

void
MAGWriter::write_dummy_top (const std::set<db::cell_index_type> &cell_set, const db::Layout &layout, tl::OutputStream &os)
{
  os.set_as_text (true);
  os << "magic\n";

  std::string tech = m_options.tech;
  if (tech.empty ()) {
    tech = layout.technology_name ();
  }
  if (! tech.empty ()) {
    os << "tech " << make_string (tl::to_lower_case (tech)) << "\n";
  }

  os << "timestamp " << m_timestamp << "\n";

  std::map<std::string, db::cell_index_type> cells_by_name;
  for (std::set<db::cell_index_type>::const_iterator c = cell_set.begin (); c != cell_set.end (); ++c) {
    cells_by_name.insert (std::make_pair (std::string (layout.cell_name (*c)), *c));
  }

  db::Coord y = 0;
  db::Coord w = 0;
  std::vector<db::CellInstArray> cell_instances;
  cell_instances.reserve (cells_by_name.size ());
  for (std::map<std::string, db::cell_index_type>::const_iterator c = cells_by_name.begin (); c != cells_by_name.end (); ++c) {
    //  instances are arrayed as stack
    db::Box bx = layout.cell (c->second).bbox ();
    cell_instances.push_back (db::CellInstArray (db::CellInst (c->second), db::Trans (db::Vector (0, y) + (db::Point () - bx.p1 ()))));
    y += bx.height ();
    w = std::max (w, db::Coord (bx.width ()));
  }

  os << "<< checkpaint >>\n";
  write_polygon (db::Polygon (db::Box (0, 0, w, y)), layout, os);

  m_cell_id.clear ();
  for (std::vector<db::CellInstArray>::const_iterator i = cell_instances.begin (); i != cell_instances.end (); ++i) {
    write_instance (*i, layout, os);
  }

  os << "<< end >>\n";
}

void
MAGWriter::write_cell (db::cell_index_type ci, const std::vector <std::pair <unsigned int, db::LayerProperties> > &layers, db::Layout &layout, tl::OutputStream &os)
{
  m_cellname = layout.cell_name (ci);
  try {
    do_write_cell (ci, layers, layout, os);
  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::to_string (tr (" when writing cell ")) + m_cellname);
  }
}

void
MAGWriter::do_write_cell (db::cell_index_type ci, const std::vector <std::pair <unsigned int, db::LayerProperties> > &layers, db::Layout &layout, tl::OutputStream &os)
{
  os.set_as_text (true);
  os << "magic\n";

  std::string tech = m_options.tech;
  if (tech.empty ()) {
    tech = layout.technology_name ();
  }
  if (! tech.empty ()) {
    os << "tech " << make_string (tl::to_lower_case (tech)) << "\n";
  }

  os << "timestamp " << m_timestamp << "\n";

  db::Cell &cell = layout.cell (ci);

  os << "<< checkpaint >>\n";
  write_polygon (db::Polygon (cell.bbox ()), layout, os);

  bool any;

  for (std::vector <std::pair <unsigned int, db::LayerProperties> >::const_iterator ll = layers.begin (); ll != layers.end (); ++ll) {
    any = false;
    for (db::Shapes::shape_iterator s = cell.shapes (ll->first).begin (db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Paths); ! s.at_end (); ++s) {
      if (! any) {
        os << "<< " << make_string (tl::to_lower_case (ll->second.name)) << " >>\n";
        any = true;
      }
      db::Polygon poly;
      s->polygon (poly);
      write_polygon (poly, layout, os);
    }
  }

  any = false;
  for (std::vector <std::pair <unsigned int, db::LayerProperties> >::const_iterator ll = layers.begin (); ll != layers.end (); ++ll) {
    for (db::Shapes::shape_iterator s = cell.shapes (ll->first).begin (db::ShapeIterator::Texts); ! s.at_end (); ++s) {
      if (! any) {
        os << "<< labels >>\n";
        any = true;
      }
      db::Text text;
      s->text (text);
      write_label (tl::to_lower_case (ll->second.name), text, layout, os);
    }
  }

  m_cell_id.clear ();
  for (db::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
    write_instance (i->cell_inst (), layout, os);
  }

  os << "<< end >>\n";
}

namespace {

  /**
   *  @brief A simple polygon receiver that writes triangles and rectangles from trapezoids
   */
  class TrapezoidWriter
    : public SimplePolygonSink
  {
  public:
    TrapezoidWriter (tl::OutputStream &os)
      : mp_os (&os)
    { }

    virtual void put (const db::SimplePolygon &polygon)
    {
      db::Box b = polygon.box ();
      if (b.empty () || b.height () == 0 || b.width () == 0) {
        //  safe fallback for degenerated polygons
        return;
      }

      //  Determine the border triangles left and right

      //  tl, tr describe the two triangles: tl left and tr right one.
      //  If sl/sr indicate south half of the rectangle.
      db::Box tl, tr;
      bool sl = false, sr = false;

      for (db::SimplePolygon::polygon_edge_iterator ie = polygon.begin_edge (); ! ie.at_end (); ++ie) {
        db::Edge e = *ie;
        if (e.dy () > 0 /*left side*/) {
          tl = e.bbox ();
          sl = e.dx () > 0;
        } else if (e.dy () < 0 /*right side*/) {
          tr = e.bbox ();
          sr = e.dx () > 0;
        }
      }

      //  outputs the parts

      if (tl.width () > 0) {
        (*mp_os) << "tri " << tl.left () << " " << tl.bottom () << " " << tl.right () << " " << tl.top () << " " << (sl ? "s" : "") << "e\n";
      }

      db::Box ib (tl.right (), tl.bottom (), tr.left (), tr.top ());
      if (ib.width () > 0) {
        (*mp_os) << "rect " << ib.left () << " " << ib.bottom () << " " << ib.right () << " " << ib.top () << "\n";
      }

      if (tr.width () > 0) {
        (*mp_os) << "tri " << tr.left () << " " << tr.bottom () << " " << tr.right () << " " << tr.top () << " " << (sr ? "s" : "") << "\n";
      }
    }

  private:
    tl::OutputStream *mp_os;
  };
}

void
MAGWriter::write_polygon (const db::Polygon &poly, const db::Layout & /*layout*/, tl::OutputStream &os)
{
  db::EdgeProcessor ep;
  ep.insert (scaled (poly));
  db::MergeOp op;
  TrapezoidWriter writer (os);
  db::TrapezoidGenerator tpgen (writer);
  ep.process (tpgen, op);
}

void
MAGWriter::write_label (const std::string &layer, const db::Text &text, const db::Layout & /*layout*/, tl::OutputStream &os)
{
  db::DVector v = db::DVector (text.trans ().disp ()) * m_sf;

  std::string s = text.string ();
  if (s.find ("\n") != std::string::npos) {
    s = tl::replaced (s, "\n", "\\n");
  }

  os << "rlabel " << make_string (layer) << " " << v.x () << " " << v.y () << " " << v.x () << " " << v.y () << " 0 " << s << "\n";
}

void
MAGWriter::write_instance (const db::CellInstArray &inst, const db::Layout &layout, tl::OutputStream &os)
{
  db::Vector a, b;
  unsigned long na = 0, nb = 0;
  if (inst.is_regular_array (a, b, na, nb) && ((a.x () == 0 && b.y () == 0) || (a.y () == 0 && b.x () == 0)) && !needs_rounding (a) && !needs_rounding (b)) {

    db::ICplxTrans tr = inst.complex_trans ();
    write_single_instance (inst.object ().cell_index (), tr, a, b, na, nb, layout, os);

  } else {

    for (db::CellInstArray::iterator ia = inst.begin (); ! ia.at_end (); ++ia) {
      db::ICplxTrans tr = inst.complex_trans (*ia);
      write_single_instance (inst.object ().cell_index (), tr, db::Vector (), db::Vector (), 1, 1, layout, os);
    }

  }
}

void
MAGWriter::write_single_instance (db::cell_index_type ci, db::ICplxTrans trans, db::Vector a, db::Vector b, unsigned long na, unsigned long nb, const db::Layout &layout, tl::OutputStream &os)
{
  if (trans.is_mag ()) {
    throw tl::Exception (tl::to_string (tr ("Cannot write magnified instance to MAG files: ")) + trans.to_string () + tl::to_string (tr (" of cell ")) + layout.cell_name (ci));
  }

  int id = int (m_cell_id [ci] += 1);
  std::string cn = layout.cell_name (ci);
  os << "use " << make_string (cn) << " " << make_string (cn + "_" + tl::to_string (id)) << "\n";

  if (na > 1 || nb > 1) {

    na = std::max ((unsigned long) 1, na);
    nb = std::max ((unsigned long) 1, nb);

    db::ICplxTrans trinv = trans.inverted ();
    a = trinv * a;
    b = trinv * b;

    if (a.y () != 0 || b.x () != 0) {
      std::swap (a, b);
      std::swap (na, nb);
    }

    db::Vector da = scaled (a);
    db::Vector db = scaled (b);

    os << "array " << 0 << " " << (na - 1) << " " << da.x () << " " << 0 << " " << (nb - 1) << " " << db.y () << "\n";

  }

  os << "timestamp " << m_timestamp << "\n";

  db::Matrix2d m = trans.to_matrix2d ();

  db::Vector d = scaled (trans.disp ());
  os << "transform " << m.m11 () << " " << m.m12 () << " " << d.x () << " " << m.m21 () << " " << m.m22 () << " " << d.y () << "\n";

  db::Box bx = scaled (layout.cell (ci).bbox ());
  os << "box " << bx.left () << " " << bx.bottom () << " " << bx.right () << " " << bx.top () << "\n";
}

namespace {

  struct ScalingOp
  {
    ScalingOp (MAGWriter *wr) : mp_wr (wr) { }
    db::Point operator() (const db::Point &pt) const { return mp_wr->scaled (pt); }
  private:
    MAGWriter *mp_wr;
  };

}

db::Polygon
MAGWriter::scaled (const db::Polygon &poly)
{
  db::Polygon spoly;
  spoly.assign_hull (poly.begin_hull (), poly.end_hull (), ScalingOp (this));
  for (unsigned int h = 0; h < poly.holes (); ++h) {
    spoly.assign_hole (h, poly.begin_hole (h), poly.end_hole (h), ScalingOp (this));
  }
  return spoly;
}

db::Box
MAGWriter::scaled (const db::Box &bx) const
{
  return db::Box (scaled (bx.p1 ()), scaled (bx.p2 ()));
}

db::Vector
MAGWriter::scaled (const db::Vector &v) const
{
  db::Vector res (db::DVector (v) * m_sf);
  if (! db::DVector (res).equal (db::DVector (v) * m_sf)) {
    tl::warn << tl::sprintf (tl::to_string (tr ("Vector rounding occurred at %s in cell %s - not a multiple of lambda (%.12g)")), v.to_string (), m_cellname, m_options.lambda);
  }
  return res;
}

db::Point
MAGWriter::scaled (const db::Point &p) const
{
  db::Point res (db::DPoint (p) * m_sf);
  if (! db::DPoint (res).equal (db::DPoint (p) * m_sf)) {
    tl::warn << tl::sprintf (tl::to_string (tr ("Coordinate rounding occurred at %s in cell %s - not a multiple of lambda (%.12g)")), p.to_string (), m_cellname, m_options.lambda);
  }
  return res;
}

bool
MAGWriter::needs_rounding (const db::Vector &v) const
{
  db::Vector res (db::DVector (v) * m_sf);
  return ! db::DVector (res).equal (db::DVector (v) * m_sf);
}

static inline bool is_valid_char (uint32_t c32)
{
  return (c32 >= 'A' && c32 <= 'Z') ||
         (c32 >= 'a' && c32 <= 'z') ||
         (c32 >= '0' && c32 <= '9') ||
         c32 == '_' || c32 == '.';
}

std::string
MAGWriter::make_string (const std::string &s)
{
  std::string res;
  for (const char *cp = s.c_str (); *cp; ) {
    uint32_t c32 = tl::utf32_from_utf8 (cp);
    if (! is_valid_char (c32)) {
      res += tl::sprintf ("x%x", c32);
    } else {
      res += (char) c32;
    }
  }
  return res;
}

}
